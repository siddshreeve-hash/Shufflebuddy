#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>
#include <cmath>

// Dub hall: pre-delay -> 4-stage allpass diffusion -> 8-line feedback delay network
// with a Hadamard mixing matrix, slow per-line modulation (kills metallic ringing),
// and a damping lowpass + rumble highpass inside the loop. Decay is set as RT60.
class DubReverb
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        const float scale = sr / 48000.0f;

        pre.assign ((size_t) (0.25f * sr) + 4, 0.0f);
        preW = 0;

        static const int apLen[4] = { 142, 107, 379, 277 };
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < 4; ++i)
            {
                auto& ap = diff[(size_t) c][(size_t) i];
                ap.buf.assign ((size_t) ((float) (apLen[i] + c * 23) * scale) + 2, 0.0f);
                ap.pos = 0;
            }

        // max line length at size 1 + modulation headroom
        for (size_t i = 0; i < N; ++i)
        {
            auto& l = lines[i];
            l.buf.assign ((size_t) (baseMs[i] * 2.2f * 0.001f * sr + 0.01f * sr) + 8, 0.0f);
            l.w = 0;
            l.lp = l.hp = l.hpIn = 0.0f;
            l.curLen = -1.0f;
            l.modPhase = (float) i / (float) N;
        }
    }

    // size 0..1, damp 0..1
    void setParameters (float size, float damp)
    {
        sizeT = size;
        rt60 = 1.5f + 10.5f * size * size;                       // ~1.5 s to 12 s
        preDelaySamples = (0.012f + 0.05f * size) * sr;
        const float fc = 900.0f + 11000.0f * std::pow (1.0f - damp, 1.8f);
        lpCoef = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * fc / sr);
        hpCoef = std::exp (-juce::MathConstants<float>::twoPi * 140.0f / sr);
    }

    void process (float* L, float* R, int num, float wet, float dry)
    {
        const float lenScale = 0.55f + 1.6f * sizeT;
        const float slew = 0.0004f;
        const float hadNorm = 1.0f / std::sqrt ((float) N);
        const float modDepth = 0.0007f * sr;                    // +-0.7 ms
        const int preSize = (int) pre.size();
        static const float modHz[N] = { 0.11f, 0.17f, 0.23f, 0.29f, 0.37f, 0.43f, 0.53f, 0.61f };

        std::array<float, N> g;
        for (size_t i = 0; i < N; ++i)
        {
            const float lenSec = baseMs[i] * lenScale * 0.001f;
            g[i] = std::pow (10.0f, -3.0f * lenSec / rt60);
            targetLen[i] = lenSec * sr;
        }

        for (int s = 0; s < num; ++s)
        {
            // pre-delay (mono-ish: keep a little stereo by delaying the sum)
            const float inL = L[s], inR = R[s];
            const float mono = 0.5f * (inL + inR);
            const float hpOut = hpCoef * (inHp + mono - inHpIn); inHpIn = mono; inHp = hpOut;
            pre[(size_t) preW] = hpOut;
            float rp = (float) preW - preDelaySamples;
            if (rp < 0) rp += (float) preSize;
            const int i0 = (int) rp; const float fr = rp - (float) i0;
            const float pd = pre[(size_t) i0] + (pre[(size_t) ((i0 + 1) % preSize)] - pre[(size_t) i0]) * fr;
            if (++preW >= preSize) preW = 0;

            // diffusion, two decorrelated chains
            float dL = pd + 0.25f * (inL - inR), dR = pd - 0.25f * (inL - inR);
            for (int i = 0; i < 4; ++i) { dL = diff[0][(size_t) i].process (dL, 0.62f); dR = diff[1][(size_t) i].process (dR, 0.62f); }

            // read lines
            std::array<float, N> y;
            for (size_t i = 0; i < N; ++i)
            {
                auto& l = lines[i];
                if (l.curLen < 0.0f) l.curLen = targetLen[i];
                l.curLen += (targetLen[i] - l.curLen) * slew;
                l.modPhase += modHz[i] / sr; if (l.modPhase >= 1.0f) l.modPhase -= 1.0f;
                const float d = l.curLen + modDepth * std::sin (juce::MathConstants<float>::twoPi * l.modPhase);
                y[i] = l.read (d);
            }

            // Hadamard mix (fast Walsh-Hadamard, N = 8)
            std::array<float, N> m = y;
            for (size_t h = 1; h < N; h <<= 1)
                for (size_t i = 0; i < N; i += h << 1)
                    for (size_t j = i; j < i + h; ++j)
                    {
                        const float a = m[j], b = m[j + h];
                        m[j] = a + b; m[j + h] = a - b;
                    }

            // damping + rumble filter in the loop, write back with input injected
            for (size_t i = 0; i < N; ++i)
            {
                auto& l = lines[i];
                float v = m[i] * hadNorm * g[i];
                l.lp += (v - l.lp) * lpCoef;
                const float inj = (i & 1) ? dR : dL;
                l.write (l.lp + inj * 0.5f);
            }

            // decorrelated stereo taps
            const float outL = (y[0] - y[2] + y[4] - y[6] + y[1] * 0.5f) * 0.95f;
            const float outR = (y[1] - y[3] + y[5] - y[7] + y[0] * 0.5f) * 0.95f;
            L[s] = inL * dry + outL * wet;
            R[s] = inR * dry + outR * wet;
        }
    }

private:
    static constexpr size_t N = 8;
    static constexpr float baseMs[N] = { 37.1f, 41.9f, 47.3f, 53.7f, 61.1f, 67.9f, 73.3f, 81.7f };

    struct Allpass
    {
        std::vector<float> buf; int pos = 0;
        float process (float x, float g)
        {
            const float d = buf[(size_t) pos];
            const float v = x + g * d;
            buf[(size_t) pos] = v;
            if (++pos >= (int) buf.size()) pos = 0;
            return d - g * v;
        }
    };

    struct Line
    {
        std::vector<float> buf; int w = 0;
        float lp = 0, hp = 0, hpIn = 0, curLen = -1.0f, modPhase = 0;
        float read (float delay) const
        {
            const int size = (int) buf.size();
            float rp = (float) w - delay;
            while (rp < 0) rp += (float) size;
            const int i0 = (int) rp; const float t = rp - (float) i0;
            const float a = buf[(size_t) (i0 % size)], b = buf[(size_t) ((i0 + 1) % size)];
            return a + (b - a) * t;
        }
        void write (float v) { buf[(size_t) w] = v; if (++w >= (int) buf.size()) w = 0; }
    };

    float sr = 48000.0f, sizeT = 0.8f, rt60 = 6.0f, preDelaySamples = 1000.0f;
    float lpCoef = 0.3f, hpCoef = 0.99f, inHp = 0.0f, inHpIn = 0.0f;
    std::vector<float> pre; int preW = 0;
    std::array<std::array<Allpass, 4>, 2> diff;
    std::array<Line, N> lines;
    std::array<float, N> targetLen {};
};
