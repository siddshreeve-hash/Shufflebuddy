#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>

// Eight synthesised drums, each with tune / decay / level, velocity-aware
// (ghost notes come out quieter AND darker AND shorter, like a real player),
// or a user WAV in place of the synth sound.

enum Pad { Kick, Snare, Clap, Rim, Hat, OpenHat, Perc, Shaker, NumPads };

struct SampleData : juce::ReferenceCountedObject
{
    using Ptr = juce::ReferenceCountedObjectPtr<SampleData>;
    juce::AudioBuffer<float> buffer;
    double sampleRate = 44100.0;
    juce::String path;
};

struct Svf   // TPT state-variable filter: lp / bp / hp
{
    float ic1 = 0, ic2 = 0, g = 0.1f, k = 1.4f;
    void set (float fc, float q, float sr)
    {
        g = std::tan (juce::MathConstants<float>::pi * juce::jlimit (20.0f, sr * 0.45f, fc) / sr);
        k = 1.0f / q;
    }
    void reset() { ic1 = ic2 = 0; }
    void run (float x, float& lp, float& bp, float& hp)
    {
        const float a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1, a3 = g * a2;
        const float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2;
        lp = v2; bp = v1; hp = x - k * v1 - v2;
    }
};

class DrumVoice
{
public:
    bool active() const { return on; }
    int pad() const { return padId; }
    juce::int64 startedAt() const { return started; }

    void trigger (int p, float vel, float tuneSemis, float decay, float level, const SampleData* sample,
                  float sr, juce::int64 now, juce::uint32 seed, float echoSend = 0.0f, float spaceSend = 0.0f)
    {
        padId = p; on = true; started = now; fs = sr; t = 0;
        sendE = echoSend; sendS = spaceSend;
        v = juce::jlimit (0.0f, 1.0f, vel);
        bright = 0.55f + 0.45f * v;
        gain = level;
        tune = std::exp2 (tuneSemis / 12.0f);
        dec = decay;
        smp = sample; smpPos = 0.0;
        rng = seed | 1u;
        for (auto& ph : phases) ph = 0.0;
        f1.reset(); f2.reset(); f3.reset(); lpState = 0; hpState = 0; hpIn = 0;

        static const float pans[NumPads] = { 0.0f, 0.02f, 0.12f, -0.28f, 0.22f, 0.26f, -0.32f, 0.36f };
        const float pn = pans[p];
        panL = std::sqrt (0.5f * (1.0f - pn)) * 1.414f;
        panR = std::sqrt (0.5f * (1.0f + pn)) * 1.414f;

        const auto timeCoef = [this] (float seconds) { return std::exp (-1.0f / (juce::jmax (0.002f, seconds) * fs)); };
        // ghosts are shorter: decay scales with velocity
        const float vd = 0.6f + 0.4f * v;
        switch (p)
        {
            case Kick:    ampC = timeCoef ((0.05f + dec * 0.16f) * vd); pitchC = timeCoef (0.026f);
                          gateAt = (int) ((0.09f + dec * 0.45f) * vd * fs); gateC = timeCoef (0.012f); break;   // tight: short body, then a fast gate so no boom lingers
            case Snare:   ampC = timeCoef (0.06f * vd); noiseC = timeCoef ((0.05f + dec * 0.25f) * vd);
                          f1.set (1900.0f * tune, 0.9f, fs); break;
            case Clap:    noiseC = timeCoef ((0.05f + dec * 0.25f) * vd); f1.set (1150.0f * tune, 1.6f, fs); break;
            case Rim:     ampC = timeCoef (0.022f + dec * 0.03f); f1.set (1750.0f * tune, 3.0f, fs); break;
            case Hat:     ampC = timeCoef ((0.03f + dec * 0.09f) * vd); f1.set (9500.0f * std::sqrt (tune), 1.2f, fs); break;
            case OpenHat: ampC = timeCoef ((0.12f + dec * 0.5f) * vd); f1.set (9000.0f * std::sqrt (tune), 1.1f, fs); break;
            case Perc:    ampC = timeCoef ((0.06f + dec * 0.25f) * vd); pitchC = timeCoef (0.018f); break;
            case Shaker:  ampC = timeCoef ((0.035f + dec * 0.12f) * vd); f1.set (6500.0f * std::sqrt (tune), 1.4f, fs); break;
            default: break;
        }
        // samples: decay = how much of the sample plays (1 = all of it)
        if (smp != nullptr) ampC = dec >= 0.99f ? 1.0f : timeCoef (0.04f + dec * dec * 2.5f);
        amp = 1.0f; noiseAmp = 1.0f; pitchEnv = 1.0f;
        // darker ghosts: one-pole lowpass whose cutoff follows velocity
        ghostLp = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * (2500.0f + 16000.0f * v * v) / fs);
    }

    void choke() { if (on) { ampC = juce::jmin (ampC, std::exp (-1.0f / (0.008f * fs))); noiseC = ampC; } }

    // dry into L/R, plus per-pad sends into the echo and space buses
    void render (float* L, float* R, float* EL, float* ER, float* SL, float* SR, int start, int num)
    {
        if (! on) return;
        for (int i = start; i < start + num; ++i)
        {
            const float s = smp != nullptr ? nextSample() : nextSynth();
            lpState += (s - lpState) * ghostLp;
            const float o = lpState * gain * v;
            const float l = o * panL, r = o * panR;
            L[i] += l; R[i] += r;
            EL[i] += l * sendE; ER[i] += r * sendE;
            SL[i] += l * sendS; SR[i] += r * sendS;
            if (amp < 0.0005f && noiseAmp < 0.0005f && t > 64) { on = false; break; }
        }
    }

private:
    float noise() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return (float) (int) rng * (1.0f / 2147483648.0f); }

    float nextSample()
    {
        const auto& b = smp->buffer;
        const int len = b.getNumSamples();
        const int i0 = (int) smpPos;
        if (i0 >= len - 1) { amp = noiseAmp = 0.0f; ++t; return 0.0f; }
        const float fr = (float) (smpPos - i0);
        float s = 0.0f;
        for (int c = 0; c < juce::jmin (2, b.getNumChannels()); ++c)
            s += b.getSample (c, i0) + (b.getSample (c, i0 + 1) - b.getSample (c, i0)) * fr;
        if (b.getNumChannels() > 1) s *= 0.5f;
        smpPos += smp->sampleRate / fs * tune;
        const float a = amp; amp *= ampC; noiseAmp = amp; ++t;
        return s * a;
    }

    float nextSynth()
    {
        const float twoPi = juce::MathConstants<float>::twoPi;
        float out = 0.0f, lp, bp, hp;
        const float ts = (float) t / fs;
        switch (padId)
        {
            case Kick:
            {
                const float f0 = 50.0f * tune;
                const double f = f0 * (1.0 + 2.6 * pitchEnv);
                phases[0] += f / fs; if (phases[0] >= 1.0) phases[0] -= 1.0;
                pitchEnv *= pitchC;
                const float body = std::tanh (1.7f * std::sin (twoPi * (float) phases[0])) * 0.85f;
                const float click = t < (int) (0.004f * fs) ? noise() * 0.35f * bright * (1.0f - ts / 0.004f) : 0.0f;
                out = body * amp + click;
                amp *= t > gateAt ? ampC * gateC : ampC;
                break;
            }
            case Snare:
            {
                phases[0] += 182.0 * tune / fs; if (phases[0] >= 1.0) phases[0] -= 1.0;
                phases[1] += 331.0 * tune / fs; if (phases[1] >= 1.0) phases[1] -= 1.0;
                const float tone = (std::sin (twoPi * (float) phases[0]) * 0.6f + std::sin (twoPi * (float) phases[1]) * 0.35f) * amp;
                f1.run (noise(), lp, bp, hp);
                const float n = (bp * 1.35f + hp * 0.2f * bright) * noiseAmp;
                out = tone * 0.6f + n * 0.75f;
                amp *= ampC; noiseAmp *= noiseC;
                break;
            }
            case Clap:
            {
                // three quick bursts then a tail, like hands slightly out of sync
                const float ms = ts * 1000.0f;
                float e = 0.0f;
                for (int b = 0; b < 3; ++b) { const float d = ms - (float) b * 9.0f; if (d >= 0.0f && d < 9.0f) e = juce::jmax (e, std::exp (-d / 2.8f)); }
                if (ms >= 27.0f) { e = juce::jmax (e, noiseAmp); noiseAmp *= noiseC; }
                f1.run (noise(), lp, bp, hp);
                out = bp * 2.2f * e;
                amp = e;
                if (ms < 27.0f) noiseAmp = 1.0f;
                break;
            }
            case Rim:
            {
                phases[0] += 480.0 * tune / fs; if (phases[0] >= 1.0) phases[0] -= 1.0;
                phases[1] += 1690.0 * tune / fs; if (phases[1] >= 1.0) phases[1] -= 1.0;
                f1.run (noise(), lp, bp, hp);
                out = (std::sin (twoPi * (float) phases[0]) * 0.5f + std::sin (twoPi * (float) phases[1]) * 0.5f + bp * 0.6f) * amp;
                amp *= ampC; noiseAmp = amp;
                break;
            }
            case Hat:
            case OpenHat:
            {
                // six detuned square waves (the classic metallic hat recipe) + noise, bandpassed high
                static const double ratios[6] = { 205.3, 304.4, 369.6, 522.7, 540.0, 800.0 };
                float m = 0.0f;
                for (int o = 0; o < 6; ++o)
                {
                    phases[(size_t) o] += ratios[o] * 1.71 * tune / fs; if (phases[(size_t) o] >= 1.0) phases[(size_t) o] -= 1.0;
                    m += phases[(size_t) o] < 0.5 ? 1.0f : -1.0f;
                }
                const float x = m * 0.12f + noise() * 0.5f;
                f1.run (x, lp, bp, hp);
                const float hpo = hp - (hpState += (hp - hpState) * 0.25f);   // extra high-pass
                out = (bp * 0.8f + hpo * 0.6f * bright) * amp;
                amp *= ampC; noiseAmp = amp;
                break;
            }
            case Perc:
            {
                const double f = 262.0 * tune * (1.0 + 0.45 * pitchEnv);
                phases[0] += f / fs; if (phases[0] >= 1.0) phases[0] -= 1.0;
                pitchEnv *= pitchC;
                const float s = std::sin (twoPi * (float) phases[0]);
                out = (s + 0.25f * s * s * s) * amp * 0.9f;
                amp *= ampC; noiseAmp = amp;
                break;
            }
            case Shaker:
            {
                const float att = juce::jmin (1.0f, ts / 0.006f);
                f1.run (noise(), lp, bp, hp);
                out = (bp * 1.4f + hp * 0.3f) * amp * att;
                amp *= ampC; noiseAmp = amp;
                break;
            }
            default: break;
        }
        ++t;
        return out;
    }

    bool on = false;
    int padId = 0;
    juce::int64 started = 0;
    float fs = 48000.0f, v = 1.0f, bright = 1.0f, gain = 1.0f, tune = 1.0f, dec = 0.5f;
    int gateAt = 1 << 30; float gateC = 1.0f;
    float amp = 1.0f, ampC = 0.999f, noiseAmp = 1.0f, noiseC = 0.999f, pitchEnv = 1.0f, pitchC = 0.99f;
    float sendE = 0.0f, sendS = 0.0f;
    float ghostLp = 1.0f, lpState = 0.0f, hpState = 0.0f, hpIn = 0.0f, panL = 1.0f, panR = 1.0f;
    juce::int64 t = 0;
    std::array<double, 6> phases {};
    Svf f1, f2, f3;
    const SampleData* smp = nullptr;
    double smpPos = 0.0;
    juce::uint32 rng = 1;
};
