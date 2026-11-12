#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <array>
#include <cmath>

// Stereo dub echo with three characters:
//  TAPE  - single head, filtered + saturated feedback, wow/flutter/drift
//  SPACE - Space-Echo style: up to three playback heads (ratios ~1 : 1.95 : 2.9)
//          mixed into both the output and the feedback, so one stab returns as a rhythm
//  BBD   - bucket-brigade analog delay: darker, chorus-like modulation, smeared repeats
// All modes: slewed delay time (time/BPM changes bend pitch), ping-pong cross-feed.
class DubDelay
{
public:
    struct Params
    {
        float timeSec  = 0.375f;
        float feedback = 0.65f;
        float toneHz   = 2500.0f;  // lowpass in the loop
        float lowCutHz = 200.0f;   // highpass in the loop
        float wobble   = 0.25f;
        float spread   = 0.5f;
        float mix      = 0.45f;
        float send     = 1.0f;     // input level into the echo (Dub Throw boosts it)
        int   style    = 0;        // 0 tape, 1 space echo, 2 bbd
        int   heads    = 6;        // space echo head mask: bit0 = head 1, bit1 = head 2, bit2 = head 3
        bool  wetOnly  = false;    // output only the echoes (dry handled by the caller)
    };

    float headLevel (int h) const { return headLvl[(size_t) juce::jlimit (0, 2, h)]; }

    float wetLevel() const { return wetRms; }

    void prepare (double sampleRate, float maxSeconds = 4.0f)
    {
        sr = (float) sampleRate;
        size = (int) (maxSeconds * sr) + 8;
        bufL.assign ((size_t) size, 0.0f);
        bufR.assign ((size_t) size, 0.0f);
        writePos = 0;
        curDelay = -1.0f;
        lpL = lpR = hpL = hpR = hpInL = hpInR = 0.0f;
        wowPhase = flutPhase = 0.0f;
        drift = driftTarget = 0.0f;
        const int apLen = (int) (0.0071f * sr);
        apL.assign ((size_t) apLen, 0.0f); apR.assign ((size_t) (apLen + 37), 0.0f); apPosL = apPosR = 0;
        wetRms = 0.0f;
    }

    void process (float* L, float* R, int num, const Params& p)
    {
        const float target = juce::jlimit (1.0f, (float) size - 8.0f, p.timeSec * sr);
        if (curDelay < 0.0f) curDelay = target;

        const bool bbd = p.style == 2, space = p.style == 1;
        const float toneHz = bbd ? juce::jmin (p.toneHz * 0.4f, 2400.0f) : p.toneHz;
        const float slew   = 1.0f - std::exp (-1.0f / (0.12f * sr));  // ~120 ms tape slew
        const float lpCoef = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * toneHz / sr);
        const float hpCoef = std::exp (-juce::MathConstants<float>::twoPi * p.lowCutHz / sr);
        const float wowInc  = (bbd ? 0.85f : 0.45f) / sr;
        const float flutInc = 6.3f / sr;
        const float fb = p.feedback;
        const float s  = p.spread;
        const float rOffset = s * 0.011f * sr;   // up to 11 ms extra on the right for width

        // head layout
        static const float headRatio[3] = { 0.345f, 0.672f, 1.0f };
        float ratios[3]; int nHeads = 0;
        int headSlot[3] = { 2, 2, 2 };
        if (space) { for (int h = 0; h < 3; ++h) if (p.heads & (1 << h)) { headSlot[nHeads] = h; ratios[nHeads++] = headRatio[h]; } }
        headPeak = { 0.0f, 0.0f, 0.0f };
        if (nHeads == 0) { ratios[0] = 1.0f; nHeads = 1; }
        const float outNorm = 1.0f / std::sqrt ((float) nHeads);
        const float fbNorm  = 1.0f / std::pow ((float) nHeads, 0.8f);
        const float satK    = bbd ? 0.8f : 1.1f;
        double wetAcc = 0.0;

        for (int i = 0; i < num; ++i)
        {
            curDelay += (target - curDelay) * slew;

            // wow + flutter + slow random drift (BBD: faster, deeper chorus wobble, no tape flutter)
            wowPhase  += wowInc;  if (wowPhase  >= 1.0f) wowPhase  -= 1.0f;
            flutPhase += flutInc; if (flutPhase >= 1.0f) flutPhase -= 1.0f;
            if (--driftCounter <= 0)
            {
                driftTarget = rng.nextFloat() * 2.0f - 1.0f;
                driftCounter = (int) (sr * 0.5f);
            }
            drift += (driftTarget - drift) * 0.00002f;

            const float wowS = std::sin (juce::MathConstants<float>::twoPi * wowPhase);
            const float mod = bbd ? p.wobble * 0.004f * wowS + 0.001f * wowS
                                  : p.wobble * (0.0035f * wowS
                                              + 0.0006f * std::sin (juce::MathConstants<float>::twoPi * flutPhase)
                                              + 0.0025f * drift);

            float tapL = 0.0f, tapR = 0.0f;
            for (int h = 0; h < nHeads; ++h)
            {
                const float d = curDelay * ratios[h];
                const float tl = read (bufL, d * (1.0f + mod));
                tapL += tl;
                tapR += read (bufR, (d + rOffset) * (1.0f - mod * 0.7f));
                const int slot = space ? headSlot[h] : 2;
                headPeak[(size_t) slot] = juce::jmax (headPeak[(size_t) slot], std::abs (tl) * p.mix);
            }
            const float outL = tapL * outNorm, outR = tapR * outNorm;

            // cross-feed for ping-pong
            float fL = ((1.0f - s) * tapL + s * tapR) * fbNorm;
            float fR = ((1.0f - s) * tapR + s * tapL) * fbNorm;

            // highpass (one-pole)
            float hL = hpCoef * (hpL + fL - hpInL); hpInL = fL; hpL = hL;
            float hR = hpCoef * (hpR + fR - hpInR); hpInR = fR; hpR = hR;
            // lowpass (one-pole): every repeat gets darker
            lpL += (hL - lpL) * lpCoef;
            lpR += (hR - lpR) * lpCoef;

            float loopL = lpL, loopR = lpR;
            if (bbd)   // bucket-brigade smear: an allpass in the loop blurs each repeat a little more
            {
                loopL = allpass (apL, apPosL, loopL, 0.5f);
                loopR = allpass (apR, apPosR, loopR, 0.5f);
            }

            // saturation keeps high feedback musical instead of exploding
            const float inL = L[i], inR = R[i];
            bufL[(size_t) writePos] = inL * p.send + std::tanh (loopL * fb * satK) / satK;
            bufR[(size_t) writePos] = inR * p.send + std::tanh (loopR * fb * satK) / satK;
            if (++writePos >= size) writePos = 0;

            L[i] = (p.wetOnly ? 0.0f : inL) + outL * p.mix;
            R[i] = (p.wetOnly ? 0.0f : inR) + outR * p.mix;
            wetAcc += (double) (outL * outL);
        }
        const float blockRms = (float) std::sqrt (wetAcc / juce::jmax (1, num)) * p.mix;
        wetRms = juce::jmax (blockRms, wetRms * 0.9f);
        for (size_t h = 0; h < 3; ++h) headLvl[h] = juce::jmax (headPeak[h], headLvl[h] * 0.85f);
    }

private:
    static float allpass (std::vector<float>& buf, int& pos, float x, float g)
    {
        const float d = buf[(size_t) pos];
        const float v = x + g * d;
        buf[(size_t) pos] = v;
        if (++pos >= (int) buf.size()) pos = 0;
        return d - g * v;
    }

    float read (const std::vector<float>& buf, float delay) const
    {
        float rp = (float) writePos - delay;
        while (rp < 0.0f) rp += (float) size;
        const int i1 = (int) rp;
        const float t = rp - (float) i1;
        const int i0 = (i1 - 1 + size) % size;
        const int i2 = (i1 + 1) % size;
        const int i3 = (i1 + 2) % size;
        const float y0 = buf[(size_t) i0], y1 = buf[(size_t) (i1 % size)], y2 = buf[(size_t) i2], y3 = buf[(size_t) i3];
        // 4-point Hermite
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * t + c2) * t + c1) * t + y1;
    }

    float sr = 44100.0f;
    int size = 0, writePos = 0;
    std::vector<float> bufL, bufR;
    float curDelay = -1.0f;
    float lpL = 0, lpR = 0, hpL = 0, hpR = 0, hpInL = 0, hpInR = 0;
    float wowPhase = 0, flutPhase = 0, drift = 0, driftTarget = 0;
    int driftCounter = 0;
    juce::Random rng;
    std::vector<float> apL, apR; int apPosL = 0, apPosR = 0;
    float wetRms = 0.0f;
    std::array<float, 3> headPeak {}, headLvl {};
};
