#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

// DJ-style bipolar filter for the effects return.
// position -1..0: lowpass sweeping 20 kHz -> 200 Hz;  0: bypass;  0..+1: highpass 20 Hz -> 4 kHz
class FxFilter
{
public:
    void prepare (double sampleRate) { sr = (float) sampleRate; reset(); }
    void reset() { for (auto& s : st) s = {}; pos = 0.0f; }

    void process (float* L, float* R, int num, float target, float reso)
    {
        const float k = 2.0f - 1.85f * juce::jlimit (0.0f, 1.0f, reso);
        const float smooth = 1.0f - std::exp (-1.0f / (0.02f * sr));   // 20 ms glide on the knob
        float g = 0.0f;
        for (int i = 0; i < num; ++i)
        {
            pos += (target - pos) * smooth;
            const float a = std::abs (pos);
            const float wet = juce::jlimit (0.0f, 1.0f, a / 0.04f);      // fade in around centre: no click
            if (wet <= 0.0f) continue;
            if ((i & 15) == 0)
            {
                const float fc = pos < 0.0f ? 20000.0f * std::pow (0.01f, a)          // 20k -> 200
                                            : 20.0f * std::pow (200.0f, a);          // 20 -> 4k
                g = std::tan (juce::MathConstants<float>::pi * juce::jlimit (15.0f, sr * 0.45f, fc) / sr);
            }
            const bool hp = pos > 0.0f;
            L[i] = L[i] + (run (st[0], L[i], g, k, hp) - L[i]) * wet;
            R[i] = R[i] + (run (st[1], R[i], g, k, hp) - R[i]) * wet;
        }
    }

private:
    struct State { float ic1 = 0.0f, ic2 = 0.0f; };
    static float run (State& s, float x, float g, float k, bool hp)
    {
        const float a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1, a3 = g * a2;
        const float v3 = x - s.ic2;
        const float v1 = a1 * s.ic1 + a2 * v3;
        const float v2 = s.ic2 + a2 * s.ic1 + a3 * v3;
        s.ic1 = 2.0f * v1 - s.ic1;
        s.ic2 = 2.0f * v2 - s.ic2;
        return hp ? x - k * v1 - v2 : v2;
    }
    float sr = 48000.0f, pos = 0.0f;
    State st[2];
};
