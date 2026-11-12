#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>

// One drum synth engine for every pad. A Patch describes a sound as a handful of
// layers (pitched body, sub, knock, 808-style metal, noise, FM, click); the library
// and the Kick Lab are just different Patches. Velocity-aware: ghost notes come out
// quieter, darker, shorter and with less pitch sweep, like a real player.

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


// Real recorded hits (close mic + stereo room mic), a few velocity layers and round robins each
struct RealClip { juce::AudioBuffer<float> buf; int vel = 0; };   // ch0 close, ch1/2 room L/R
struct RealSet  { juce::String name; std::vector<RealClip> clips; };

struct Patch
{
    // pitched body: osc with a pitch sweep (start = sweep x freq, falling over 'drop')
    int   wave = 0;           // 0 sine, 1 triangle, 2 round (saturated sine), 3 soft square
    float freq = 200.0f, sweep = 1.0f, drop = 0.03f, curve = 0.0f;   // curve 0 = snappy exponential, 1 = long slide
    float hold = 0.0f, decay = 0.1f, body = 0.0f;
    float ratio2 = 0.0f, level2 = 0.0f;                               // second partial, same envelope
    float sub = 0.0f, subDecay = 0.25f;                               // clean sine phase-locked to the body (no drive)
    float knock = 0.0f, knockHz = 180.0f, knockDecay = 0.03f;         // short resonant ping
    float metal = 0.0f, metalScale = 1.0f, metalBp = 9000.0f, metalHp = 6000.0f, metalDecay = 0.05f;   // TR-808 six-square cluster
    float noise = 0.0f, noiseDecay = 0.1f, noiseHz = 2000.0f, noiseQ = 0.8f;
    int   noiseMode = 1, bursts = 1;                                  // 0 lp, 1 bp, 2 hp; bursts > 1 = clap
    float burstGap = 0.009f;
    float fmRatio = 0.0f, fmIndex = 0.0f, fmDecay = 0.05f;            // phase modulation of the body
    float click = 0.0f; int clickType = 0; float clickTone = 0.5f;    // 0 noise, 1 beater, 2 tick, 3 snap
    float drive = 0.0f;
    float gate = 0.0f;        // seconds; after it everything fades out (0 = off)
    float gateFade = 0.008f;
    float attack = 0.0f;      // seconds (shakers, brushes)
    float velPitch = 0.3f;    // how much softer hits lose pitch sweep
    // ---- character: what makes a synth hit sound like a hit sampled off a record
    float lpHz = 20000.0f, hpHz = 0.0f;   // tone: dark tops (Burial) and no mud
    float crush = 0.0f;       // sampler grit: lower sample rate + fewer bits (0.5 ~ 12-bit, 26 kHz)
    float room = 0.0f;        // a small room around every hit (early reflections)
    float roomSize = 1.0f;    // 0.5 = closet, 1 = small room, 2 = big room; also darker as it grows
    int   real = -1;          // index of a RealSet layered underneath (-1 = none)
    float realMix = 0.0f, realRoom = 0.6f, realTune = 0.0f;   // level, room-mic level, semitones
    int   grains = 0;         // >0: noise comes as random little grains (rattles, shuffles, foley)
    float grainSpread = 0.06f, grainDecay = 0.006f;
    float gain = 1.0f;
};

class DrumVoice
{
public:
    bool active() const { return on; }
    int pad() const { return padId; }
    juce::int64 startedAt() const { return started; }

    void trigger (int p, float vel, const Patch& patch, float tuneSemis, float decay, float level, const SampleData* sample,
                  float sr, juce::int64 now, juce::uint32 seed, float echoSend = 0.0f, float spaceSend = 0.0f,
                  const RealSet* realSet = nullptr, float realScale = 1.0f, float roomScale = 1.0f)
    {
        padId = p; on = true; started = now; fs = sr; t = 0;
        sendE = echoSend; sendS = spaceSend;
        v = juce::jlimit (0.0f, 1.0f, vel);
        bright = 0.55f + 0.45f * v;
        gain = level;
        tune = std::exp2 (tuneSemis / 12.0f);
        smp = sample; smpPos = 0.0;
        rng = seed | 1u;
        for (auto& ph : ph6) ph = 0.0;
        ph0 = ph2 = phM = phK = 0.0;
        fN.reset(); fM.reset(); fC.reset(); lpState = 0; hpM = 0; hpN = 0; clickLp = 0;
        doneAt = -1; realBuf = nullptr; roomOutL = roomOutR = 0.0f;
        fade = 1.0f; fadeC = std::exp (-1.0f / ((smp != nullptr ? 0.008f : juce::jmax (0.002f, patch.gateFade)) * sr)); gateN = 1 << 30; holdN = 0; attN = 1;
        subA = knA = meA = noA = 0.0f;

        static const float pans[NumPads] = { 0.0f, 0.02f, 0.12f, -0.28f, 0.22f, 0.26f, -0.32f, 0.36f };
        const float pn = pans[p];
        panL = std::sqrt (0.5f * (1.0f - pn)) * 1.414f;
        panR = std::sqrt (0.5f * (1.0f + pn)) * 1.414f;

        const auto coef = [this] (float seconds) { return std::exp (-1.0f / (juce::jmax (0.0015f, seconds) * fs)); };
        const float vd = 0.6f + 0.4f * v;                                // ghosts are shorter
        const float dm = std::exp2 ((decay - 0.45f) * 3.2f) * vd;        // DECAY knob: 0.37x .. 3.4x
        if (smp != nullptr)
        {
            ampC = decay >= 0.99f ? 1.0f : coef (0.04f + decay * decay * 2.5f);
            amp = 1.0f;
        }
        else
        {
            P = patch;
            const float st = std::sqrt (tune);
            P.freq *= tune; P.knockHz *= tune; P.noiseHz *= st; P.metalBp *= st; P.metalScale *= st;
            P.sweep = 1.0f + (P.sweep - 1.0f) * (1.0f - P.velPitch * (1.0f - v));
            holdN = (int) (P.hold * fs);
            gateN = P.gate > 0.0f ? (int) (P.gate * dm / vd * fs) : (1 << 30);
            attN = juce::jmax (1, (int) (P.attack * fs));
            dropN = juce::jmax (1.0f, P.drop * fs);
            ampC = coef (P.decay * dm);   amp = P.body > 0.0f || P.level2 > 0.0f ? 1.0f : 0.0f;
            subC = coef (P.subDecay * dm); subA = P.sub > 0.0f ? 1.0f : 0.0f;
            knC = coef (P.knockDecay);    knA = P.knock > 0.0f ? 1.0f : 0.0f;
            meC = coef (P.metalDecay * dm); meA = P.metal > 0.0f ? 1.0f : 0.0f;
            noC = coef (P.noiseDecay * dm); noA = P.noise > 0.0f ? 1.0f : 0.0f;
            fmC = coef (P.fmDecay * dm);  fmA = 1.0f;
            fN.set (P.noiseHz, P.noiseQ, fs);
            fM.set (P.metalBp, 1.2f, fs);
            fC.set (2400.0f + 4500.0f * P.clickTone, 1.4f, fs);
            hpMc = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * P.metalHp / fs);
            lpC = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * juce::jmin (P.lpHz, fs * 0.45f) / fs);
            hpC = P.hpHz > 0.0f ? 1.0f - std::exp (-juce::MathConstants<float>::twoPi * P.hpHz / fs) : 0.0f;
            crushStep = 1.0f + P.crush * 1.6f * (fs / 44100.0f);           // hold each sample this long
            crushLv = std::exp2 (16.0f - 8.0f * P.crush);
            crushPh = 0.0f; crushHeld = 0.0f;
            std::fill (roomBuf.begin(), roomBuf.end(), 0.0f); roomW = 0; roomLp = 0.0f;
            P.room *= roomScale;
            for (int k = 0; k < 4; ++k)   // every sound has its own room size, and every hit lands slightly differently in it
            {
                const float base = (0.0045f + 0.0075f * (float) k * (1.0f + 0.2f * (float) k)) * P.roomSize;
                const float jitter = 1.0f + 0.07f * noise();
                roomTap[(size_t) k] = juce::jlimit (16, (int) roomBuf.size() - 1, (int) (fs * base * jitter));
            }
            roomFb = juce::jlimit (0.2f, 0.62f, 0.3f + 0.16f * P.roomSize);
            roomDamp = juce::jlimit (0.12f, 0.6f, 0.5f - 0.17f * P.roomSize);
            roomTail = P.room > 0.0f ? (int) ((0.04f + 0.06f * P.roomSize) * fs) : 0;
            // real recorded layer: velocity picks the layer, round robin picks the take
            realBuf = nullptr; realPos = 0.0;
            if (realSet != nullptr && ! realSet->clips.empty() && P.realMix * realScale > 0.0f)
            {
                const int layer = v < 0.45f ? 0 : (v < 0.8f ? 1 : 2);
                int cands[16]; int nc = 0;
                for (int k = 0; k < (int) realSet->clips.size() && nc < 16; ++k) if (realSet->clips[(size_t) k].vel == layer) cands[nc++] = k;
                if (nc == 0) for (int k = 0; k < (int) realSet->clips.size() && nc < 16; ++k) cands[nc++] = k;
                const int pickK = cands[(int) ((0.5f * (noise() + 1.0f)) * (float) nc) % nc];
                realBuf = &realSet->clips[(size_t) pickK].buf;
                realRate = 44100.0 / fs * tune * std::exp2 (P.realTune / 12.0f);
                realGainC = P.realMix * realScale;
                realGainR = P.realRoom * roomScale * P.realMix * juce::jmax (0.35f, realScale);
                if (decay < 0.99f) realEnvC = coef (0.08f + decay * decay * 1.6f); else realEnvC = 1.0f;
                realEnv = 1.0f;
            }
            roomOutL = roomOutR = 0.0f;
            const float spread = P.grainSpread * dm;
            for (int k = 0; k < juce::jmin (16, P.grains); ++k) { grainT[(size_t) k] = (k == 0 ? 0.0f : (0.5f * (noise() + 1.0f)) * spread * fs); grainA[(size_t) k] = 0.45f + 0.55f * (0.5f * (noise() + 1.0f)); }
            grainK = 1.0f / juce::jmax (1.0f, P.grainDecay * fs);
            lpS1 = lpS2 = hpS = 0.0f;
            driveK = 1.0f + 8.0f * P.drive * P.drive;
            driveN = 1.0f / std::tanh (driveK);
        }
        ghostLp = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * (2500.0f + 16000.0f * v * v) / fs);
    }

    void choke() { if (on) { fadeC = std::exp (-1.0f / (0.008f * fs)); gateN = juce::jmin (gateN, (int) t); ampC = juce::jmin (ampC, fadeC); } }

    // dry into L/R (or the kick bus), plus per-pad sends into the echo and space buses
    void render (float* L, float* R, float* KL, float* KR, float* EL, float* ER, float* SL, float* SR, int start, int num)
    {
        if (! on) return;
        float* dL = padId == Kick ? KL : L;
        float* dR = padId == Kick ? KR : R;
        for (int i = start; i < start + num; ++i)
        {
            const float s = smp != nullptr ? nextSample() : nextSynth();
            lpState += (s - lpState) * ghostLp;
            const float o = lpState * gain * v;
            const float l = o * panL + roomOutL * gain * v, r = o * panR + roomOutR * gain * v;
            dL[i] += l; dR[i] += r;
            EL[i] += l * sendE; ER[i] += r * sendE;
            SL[i] += l * sendS; SR[i] += r * sendS;
            if (t > 64 && (finished() || fade < 0.0003f)) { on = false; break; }
        }
    }

private:
    bool finished()
    {
        if (smp != nullptr) return amp < 0.0005f;
        const bool envDone = amp < 0.0005f && subA < 0.0005f && knA < 0.0005f && meA < 0.0005f && noA < 0.0005f && t > (juce::int64) holdN + attN
                             && (P.grains == 0 || t > (juce::int64) (P.grainSpread * fs * 4.0f));
        if (realBuf != nullptr) return false;
        if (! envDone) { doneAt = -1; return false; }
        if (doneAt < 0) doneAt = t;
        return t - doneAt > roomTail;
    }
    float noise() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return (float) (int) rng * (1.0f / 2147483648.0f); }

    float nextSample()
    {
        const auto& b = smp->buffer;
        const int len = b.getNumSamples();
        const int i0 = (int) smpPos;
        if (i0 >= len - 1) { amp = 0.0f; ++t; return 0.0f; }
        const float fr = (float) (smpPos - i0);
        float s = 0.0f;
        for (int c = 0; c < juce::jmin (2, b.getNumChannels()); ++c)
            s += b.getSample (c, i0) + (b.getSample (c, i0 + 1) - b.getSample (c, i0)) * fr;
        if (b.getNumChannels() > 1) s *= 0.5f;
        smpPos += smp->sampleRate / fs * tune;
        const float a = amp; amp *= ampC; ++t;
        if (t > gateN) fade *= fadeC;
        return s * a * fade;
    }

    static float shape (int wave, double ph)
    {
        const float x = (float) std::sin (juce::MathConstants<double>::twoPi * ph);
        switch (wave)
        {
            case 1: { const double f = ph - std::floor (ph); return (float) (f < 0.5 ? 4.0 * f - 1.0 : 3.0 - 4.0 * f); }
            case 2: return std::tanh (2.2f * x) * 1.025f;
            case 3: return std::tanh (6.0f * x);
            default: return x;
        }
    }

    float nextSynth()
    {
        const float twoPi = juce::MathConstants<float>::twoPi;
        float lp, bp, hp, out = 0.0f;
        const float tf = (float) t;

        // ---- body (+ 2nd partial, FM, sub share its pitch)
        float pitchMul = 1.0f;
        if (P.sweep != 1.0f)
        {
            const float e = std::exp (-tf / dropN);
            const float lin = juce::jmax (0.0f, 1.0f - tf / (dropN * 3.0f));
            pitchMul = 1.0f + (P.sweep - 1.0f) * ((1.0f - P.curve) * e + P.curve * lin);
        }
        const double f = (double) P.freq * pitchMul;
        ph0 += f / fs; ph0 -= std::floor (ph0);
        if (amp > 0.0f || subA > 0.0f)
        {
            double pm = 0.0;
            if (P.fmIndex > 0.0f)
            {
                phM += f * P.fmRatio / fs; phM -= std::floor (phM);
                pm = P.fmIndex * fmA * std::sin (juce::MathConstants<double>::twoPi * phM) / juce::MathConstants<double>::twoPi;
                fmA *= fmC;
            }
            float b = 0.0f;
            if (amp > 0.0005f)
            {
                b = shape (P.wave, ph0 + pm) * P.body;
                if (P.level2 > 0.0f) { ph2 += f * P.ratio2 / fs; ph2 -= std::floor (ph2); b += shape (P.wave, ph2 + pm) * P.level2; }
                if (P.drive > 0.0f) b = std::tanh (b * driveK) * driveN;   // drive before the envelope: grit without stretching the tail
                b *= amp;
                if (t >= holdN) amp *= ampC;
            }
            float sb = 0.0f;
            if (subA > 0.0005f) { sb = std::sin (twoPi * (float) ph0) * subA * P.sub; if (t >= holdN) subA *= subC; }
            out += b + sb;
        }
        // ---- knock: short ping that gives the kick its "thock"
        if (knA > 0.0005f)
        {
            phK += P.knockHz / fs; phK -= std::floor (phK);
            out += std::tanh (1.5f * std::sin (twoPi * (float) phK)) * knA * P.knock * 0.8f;
            knA *= knC;
        }
        // ---- metal: six detuned squares (TR-808 ratios), band-passed and high-passed
        if (meA > 0.0005f)
        {
            static const double r[6] = { 205.3, 304.4, 369.6, 522.7, 540.0, 800.0 };
            float m = 0.0f;
            for (int o = 0; o < 6; ++o) { ph6[(size_t) o] += r[o] * 1.71 * P.metalScale / fs; ph6[(size_t) o] -= std::floor (ph6[(size_t) o]); m += ph6[(size_t) o] < 0.5 ? 1.0f : -1.0f; }
            fM.run (m * 0.16f, lp, bp, hp);
            hpM += (bp - hpM) * hpMc;
            out += (bp - hpM) * 3.0f * meA * P.metal * bright;
            meA *= meC;
        }
        // ---- noise (with clap bursts)
        if (noA > 0.0005f || (P.bursts > 1 && tf < P.burstGap * fs * (float) P.bursts) || (P.grains > 0 && tf < P.grainSpread * fs * 4.0f))
        {
            float e = noA;
            if (P.bursts > 1)
            {
                const float ms = tf / fs * 1000.0f, gap = P.burstGap * 1000.0f;
                const float tailStart = gap * (float) P.bursts;
                e = 0.0f;
                for (int b = 0; b < P.bursts; ++b) { const float d = ms - (float) b * gap; if (d >= 0.0f && d < gap) e = juce::jmax (e, std::exp (-d / 2.8f)); }
                if (ms >= tailStart) { e = juce::jmax (e, noA); noA *= noC; }
            }
            else if (P.grains > 0)
            {
                float gsum = 0.0f;
                for (int k = 0; k < juce::jmin (16, P.grains); ++k) { const float d = tf - grainT[(size_t) k]; if (d >= 0.0f) gsum += grainA[(size_t) k] * std::exp (-d * grainK); }
                e = juce::jmin (1.2f, gsum) * (0.35f + 0.65f * noA);
                noA *= noC;
            }
            else noA *= noC;
            fN.run (noise(), lp, bp, hp);
            const float x = P.noiseMode == 0 ? lp : P.noiseMode == 1 ? bp * 1.6f : hp;
            out += x * e * P.noise * (P.noiseMode == 2 ? bright : 1.0f);
        }
        // ---- click / transient
        if (P.click > 0.0f && tf < 0.02f * fs)
        {
            const float ms = tf / fs * 1000.0f;
            float c = 0.0f;
            switch (P.clickType)
            {
                case 0: { const float n = noise(); clickLp += (n - clickLp) * (0.2f + 0.75f * P.clickTone); c = (P.clickTone > 0.5f ? n - clickLp * 0.6f : clickLp) * std::exp (-ms / 1.4f) * 0.9f; break; }
                case 1: c = std::sin (twoPi * (1200.0f + 2600.0f * P.clickTone) * tf / fs) * std::exp (-ms / 1.8f); break;
                case 2: c = (t == 0 ? 1.0f : t == 1 ? -0.6f : 0.0f) * (0.6f + 0.4f * P.clickTone) + noise() * std::exp (-ms / 0.35f) * 0.3f; break;
                default: { fC.run (noise(), lp, bp, hp); c = bp * 2.4f * std::exp (-ms / 4.0f); break; }
            }
            out += c * P.click * bright;
        }
        // ---- attack + gate
        if (t < attN) out *= tf / (float) attN;
        if (t > gateN) { fade *= fadeC; out *= fade; }
        // ---- real recorded layer: close mic joins the synth (and its character), room mic stays stereo and natural
        roomOutL = roomOutR = 0.0f;
        if (realBuf != nullptr)
        {
            const int len = realBuf->getNumSamples();
            const int i0 = (int) realPos;
            if (i0 >= len - 1) realBuf = nullptr;
            else
            {
                const float fr = (float) (realPos - i0);
                auto rd = [&] (int ch) { const float* d = realBuf->getReadPointer (ch); return d[i0] + (d[i0 + 1] - d[i0]) * fr; };
                const float env = realEnv * (t > gateN ? fade : 1.0f);
                out += rd (0) * realGainC * env;
                if (realBuf->getNumChannels() >= 3) { roomOutL = rd (1) * realGainR * env; roomOutR = rd (2) * realGainR * env; }
                realPos += realRate; realEnv *= realEnvC;
                if (realEnv < 0.0005f) realBuf = nullptr;
            }
        }
        // ---- character: sampler grit -> tone -> room
        if (P.crush > 0.0f)
        {
            crushPh += 1.0f;
            if (crushPh >= crushStep) { crushPh -= crushStep; crushHeld = std::round (out * crushLv) / crushLv; }
            out = crushHeld;
        }
        if (P.lpHz < 19000.0f) { lpS1 += (out - lpS1) * lpC; lpS2 += (lpS1 - lpS2) * lpC; out = lpS2; }
        if (hpC > 0.0f) { hpS += (out - hpS) * hpC; out -= hpS; }
        if (P.room > 0.0f)
        {
            const int sz = (int) roomBuf.size();
            auto rd = [&] (int tap) { int i = roomW - tap; if (i < 0) i += sz; return roomBuf[(size_t) i]; };
            float r = rd (roomTap[0]) * 0.55f + rd (roomTap[1]) * 0.45f - rd (roomTap[2]) * 0.38f + rd (roomTap[3]) * 0.3f;
            roomLp += (r - roomLp) * roomDamp;
            roomBuf[(size_t) roomW] = out + roomLp * roomFb;
            if (++roomW >= sz) roomW = 0;
            out += roomLp * P.room * 1.3f;
        }
        ++t;
        return out * P.gain;
    }

    bool on = false;
    int padId = 0;
    juce::int64 started = 0;
    Patch P;
    float fs = 48000.0f, v = 1.0f, bright = 1.0f, gain = 1.0f, tune = 1.0f;
    float amp = 0, ampC = 0.999f, subA = 0, subC = 0.999f, knA = 0, knC = 0.99f, meA = 0, meC = 0.99f, noA = 0, noC = 0.99f, fmA = 1, fmC = 0.999f;
    float fade = 1.0f, fadeC = 0.998f, driveK = 1.0f, driveN = 1.0f, dropN = 1000.0f;
    int holdN = 0, gateN = 1 << 30, attN = 1;
    float sendE = 0.0f, sendS = 0.0f;
    float ghostLp = 1.0f, lpState = 0.0f, hpM = 0.0f, hpMc = 0.5f, hpN = 0.0f, clickLp = 0.0f, panL = 1.0f, panR = 1.0f;
    juce::int64 t = 0, doneAt = -1;
    float lpC = 1.0f, hpC = 0.0f, lpS1 = 0, lpS2 = 0, hpS = 0, crushStep = 1.0f, crushLv = 65536.0f, crushPh = 0, crushHeld = 0;
    std::array<float, 4096> roomBuf {};
    int roomW = 0, roomTail = 0;
    std::array<int, 4> roomTap {};
    float roomLp = 0.0f, grainK = 0.1f, roomFb = 0.42f, roomDamp = 0.35f, roomOutL = 0.0f, roomOutR = 0.0f;
    const juce::AudioBuffer<float>* realBuf = nullptr;
    double realPos = 0.0, realRate = 1.0;
    float realGainC = 0.0f, realGainR = 0.0f, realEnv = 1.0f, realEnvC = 1.0f;
    std::array<float, 16> grainT {}, grainA {};
    double ph0 = 0, ph2 = 0, phM = 0, phK = 0;
    std::array<double, 6> ph6 {};
    Svf fN, fM, fC;
    const SampleData* smp = nullptr;
    double smpPos = 0.0;
    juce::uint32 rng = 1;
};
