#pragma once
#include <juce_core/juce_core.h>
#include "DrumVoices.h"
#include "GenreBank.h"
#include <array>

// Patterns: 4 slots x 10 lanes x 16 steps. Each step holds velocity, chance, nudge, ratchet and (toms) pitch.
// Old-style levels still exist for the quick click-to-cycle editing: 0 rest, 1 ghost, 2 normal, 3 accent.
// Styles describe each drum as per-step probability + level, so ROLL writes a new pattern that
// always sounds like the style (2-step kick placement, shuffled hats, ghosted snares...).

namespace groove
{
    constexpr int numSlots = 4, numSteps = 16;
    // Styles = the genre bank (tools/genres.py -> GenreBank.h): each has a feel, four hand-written variations
    // that make a 4-bar phrase, and fill preferences. ROLL / VARY learn their odds from those four variations.
    constexpr int NumStyles = genres::count;
    inline const char* styleName (int s) { return genres::all()[juce::jlimit (0, NumStyles - 1, s)].name; }
    inline const genres::Genre& genre (int s) { return genres::all()[juce::jlimit (0, NumStyles - 1, s)]; }

    struct StyleFeel { float tempo, swing, ghosts, loosen, dust; float push[NumPads]; };
    inline StyleFeel feel (int s)
    {
        const auto& g = genre (s);
        StyleFeel f { g.tempo, g.swing, g.ghosts, g.loosen, g.dust, {} };
        for (int i = 0; i < NumPads; ++i) f.push[i] = g.push[i];
        return f;
    }

    // ROLL / VARY odds per step, learned from the genre's four variations: in all four = always, in none = never
    struct Tpl { const char* prob; const char* lvl; };
    inline Tpl tpl (int style, int pad)
    {
        struct Table { char prob[NumStyles][NumPads][17]; char lvl[NumStyles][NumPads][17]; };
        static const Table T = []
        {
            Table t {};
            auto lvlOf = [] (char c) { return c == '3' ? 3 : (c == '2' || c == 'x' || c == 'z' || c == 'L' || c == 'M' || c == 'H' || c == 'F') ? 2 : (c == '1' || c == 'y') ? 1 : 0; };
            for (int st = 0; st < NumStyles; ++st)
                for (int p = 0; p < NumPads; ++p)
                {
                    for (int s = 0; s < 16; ++s)
                    {
                        int n = 0, best = 0;
                        for (int v = 0; v < 4; ++v) { const int l = lvlOf (genres::all()[st].v[v][p][s]); if (l > 0) { ++n; best = juce::jmax (best, l); } }
                        static const char pc[5] = { '.', '3', '6', '8', 'X' };
                        t.prob[st][p][s] = pc[n];
                        t.lvl[st][p][s] = (char) ('0' + juce::jmax (1, best));
                    }
                    t.prob[st][p][16] = t.lvl[st][p][16] = 0;
                }
            return t;
        }();
        const int st = juce::jlimit (0, NumStyles - 1, style), p = juce::jlimit (0, NumPads - 1, pad);
        return { T.prob[st][p], T.lvl[st][p] };
    }

    inline float probAt (const char* s, int i) { const char c = s[i]; return c == 'X' ? 1.0f : (c >= '1' && c <= '9') ? (float) (c - '0') / 10.0f : 0.0f; }

    // ---------------------------------------------------------------- step data
    struct Cell
    {
        juce::uint8 vel = 0;       // 0 = off, 1..127
        juce::uint8 prob = 100;    // chance to play, %
        juce::int8  nudge = 0;     // -50..+50 % of a 16th (early / late)
        juce::uint8 ratchet = 1;   // 1..4 hits inside the step
        juce::uint8 alt = 1;       // toms: 0 floor, 1 low, 2 mid, 3 high
        bool on() const { return vel > 0; }
    };
    inline juce::uint8 velOfLevel (int l) { return (juce::uint8) (l <= 0 ? 0 : l == 1 ? 40 : l == 2 ? 96 : 127); }
    inline int levelOfVel (int v) { return v <= 0 ? 0 : v < 64 ? 1 : v < 112 ? 2 : 3; }
    inline juce::uint64 pack (Cell c)
    {
        return (juce::uint64) c.vel | ((juce::uint64) c.prob << 8) | ((juce::uint64) (juce::uint8) c.nudge << 16) | ((juce::uint64) c.ratchet << 24) | ((juce::uint64) c.alt << 32);
    }
    inline Cell unpack (juce::uint64 v)
    {
        Cell c; c.vel = (juce::uint8) v; c.prob = (juce::uint8) (v >> 8); c.nudge = (juce::int8) (juce::uint8) (v >> 16);
        c.ratchet = (juce::uint8) juce::jlimit (1, 4, (int) (juce::uint8) (v >> 24)); c.alt = (juce::uint8) juce::jlimit (0, 3, (int) (juce::uint8) (v >> 32));
        return c;
    }
    inline Cell hit (int vel, int alt = 1) { Cell c; c.vel = (juce::uint8) juce::jlimit (0, 127, vel); c.alt = (juce::uint8) alt; return c; }

    using Grid = std::array<std::array<Cell, numSteps>, NumPads>;

    inline Grid fromLevels (std::initializer_list<const char*> rows)   // "3..2.." strings, one per lane; missing lanes stay empty
    {
        Grid g {}; int p = 0;
        for (auto* r : rows) { for (int s = 0; s < numSteps && r[s]; ++s) if (r[s] >= '1' && r[s] <= '3') g[(size_t) p][(size_t) s] = hit (velOfLevel (r[s] - '0')); if (++p >= NumPads) break; }
        return g;
    }

    // one of a genre's four hand-written variations as a full pattern (ratchets and tom pitches included)
    inline Grid genrePattern (int style, int variation)
    {
        Grid g {};
        const auto& gen = genre (style);
        for (int p = 0; p < NumPads; ++p)
            for (int s = 0; s < numSteps; ++s)
            {
                const char c = gen.v[juce::jlimit (0, 3, variation)][p][s];
                Cell x;
                switch (c)
                {
                    case '1': x = hit (40); break;   case '2': x = hit (96); break;   case '3': x = hit (127); break;
                    case 'x': x = hit (96); x.ratchet = 2; break;   case 'y': x = hit (40); x.ratchet = 2; break;   case 'z': x = hit (96); x.ratchet = 3; break;
                    case 'F': x = hit (100, 0); break; case 'L': x = hit (100, 1); break; case 'M': x = hit (100, 2); break; case 'H': x = hit (100, 3); break;
                    default: break;
                }
                g[(size_t) p][(size_t) s] = x;
            }
        return g;
    }

    inline Grid roll (int style, juce::Random& r)
    {
        Grid g {};
        for (int p = 0; p < 8; ++p)
        {
            const auto t = tpl (style, p);
            for (int s = 0; s < numSteps; ++s)
                if (r.nextFloat() < probAt (t.prob, s)) g[(size_t) p][(size_t) s] = hit (velOfLevel (t.lvl[s] - '0') - r.nextInt (12));
        }
        g[Kick][0] = hit (127);   // always anchor the bar
        for (int s = 0; s < numSteps; ++s) if (g[OpenHat][(size_t) s].on()) g[Hat][(size_t) s] = Cell();
        return g;
    }

    // VARY: nudge a pattern without losing its identity
    inline void vary (Grid& g, int style, juce::Random& r, int moves = 3)
    {
        for (int m = 0; m < moves; ++m)
        {
            const int p = r.nextInt (8), s = r.nextInt (numSteps);
            if (p == Kick && s == 0) continue;
            const auto t = tpl (style, p);
            const float pr = probAt (t.prob, s);
            auto& c = g[(size_t) p][(size_t) s];
            if (! c.on() && pr > 0.0f) c = hit (velOfLevel (t.lvl[s] - '0'));     // add a plausible hit
            else if (c.on() && pr < 1.0f) c.vel = 0;                               // drop a non-essential one
            else if (levelOfVel (c.vel) == 2) c.vel = 40;                          // soften to a ghost
        }
    }

    // ---------------------------------------------------------------- FILLS
    // Real fills aren't just snare: toms walking down, snare rolls that build, kick doubles,
    // open-hat lifts, and the house/garage drop-out where everything but the hats cuts out.
    enum FillType { FillAuto, FillToms, FillRoll, FillKicks, FillHats, FillDrop, NumFillTypes };
    inline const char* fillName (int t) { static const char* n[] = { "AUTO", "TOMS", "ROLL", "KICKS", "HATS", "DROP" }; return n[juce::jlimit (0, NumFillTypes - 1, t)]; }

    inline int pickFill (int style, juce::Random& r)
    {
        const auto& w = genre (style).fill;   // toms, roll, kicks, hats, drop
        float total = 0.0f; for (int i = 0; i < 5; ++i) total += w[i];
        float x = r.nextFloat() * juce::jmax (0.001f, total);
        for (int i = 0; i < 5; ++i) { x -= w[i]; if (x <= 0.0f) return FillToms + i; }
        return FillRoll;
    }

    inline void applyFill (Grid& g, int type, int len, int style, juce::Random& r)
    {
        len = juce::jlimit (2, numSteps, len);
        const int s0 = numSteps - len;
        if (type == FillAuto) type = pickFill (style, r);
        auto clear = [&] (std::initializer_list<int> lanes, int from) { for (int p : lanes) for (int s = from; s < numSteps; ++s) g[(size_t) p][(size_t) s] = Cell(); };
        auto ramp = [&] (int s, int lo, int hi) { return lo + (hi - lo) * (s - s0) / juce::jmax (1, len - 1); };
        switch (type)
        {
            case FillToms:   // walk down the kit: high tom to floor tom, building up
            {
                clear ({ Snare, Clap, Rim, Hat, OpenHat, Perc, Kick }, s0);
                for (int s = s0; s < numSteps; ++s)
                {
                    if (len >= 12 && s < s0 + len / 2 && (s & 1)) continue;          // long fills start in 8ths
                    if (s < numSteps - 2 && r.nextFloat() < 0.12f) continue;          // a gap or two so it breathes
                    const int alt = 3 - juce::jlimit (0, 3, (s - s0) * 4 / len);
                    g[Toms][(size_t) s] = hit (ramp (s, 78, 124), alt);
                }
                g[Kick][(size_t) (numSteps - 1)] = hit (110);
                break;
            }
            case FillRoll:   // snare roll that builds, doubling up at the end
            {
                clear ({ Snare, Clap, Rim, Hat, OpenHat, Perc }, s0);
                for (int s = s0; s < numSteps; ++s)
                {
                    auto c = hit (ramp (s, 46, 127));
                    if (s >= numSteps - juce::jmax (2, len / 4)) c.ratchet = 2;
                    g[Snare][(size_t) s] = c;
                }
                for (int s = s0 + 1; s < numSteps; ++s) g[Kick][(size_t) s] = Cell();
                break;
            }
            case FillKicks:  // kick doubles on the off-16ths, snare + clap crack to finish
            {
                for (int s = s0; s < numSteps - 1; ++s)
                    if ((s & 1) ? r.nextFloat() < 0.55f : r.nextFloat() < 0.3f) g[Kick][(size_t) s] = hit (s & 1 ? 92 : 112);
                g[Snare][(size_t) (numSteps - 1)] = hit (127);
                g[Clap][(size_t) (numSteps - 1)] = hit (110);
                break;
            }
            case FillHats:   // open-hat lift: open hats on the offbeats, closed hats rolling and building
            {
                clear ({ Hat, OpenHat, Rim }, s0);
                for (int s = s0 + 1; s < numSteps; ++s) g[Kick][(size_t) s] = Cell();
                for (int s = s0; s < numSteps; ++s)
                {
                    if ((s & 3) == 2) g[OpenHat][(size_t) s] = hit (ramp (s, 80, 120));
                    else { auto c = hit (ramp (s, 50, 110)); if (s >= numSteps - juce::jmax (2, len / 4)) c.ratchet = 2; g[Hat][(size_t) s] = c; }
                }
                g[Snare][(size_t) (numSteps - 1)] = hit (120);
                break;
            }
            case FillDrop:   // cut-out: everything except the hats goes silent, then a pickup
            default:
            {
                clear ({ Kick, Snare, Clap, Rim, Perc, Toms }, s0);
                if (r.nextFloat() < 0.6f) g[Snare][(size_t) (numSteps - 1)] = hit (52);
                if (r.nextFloat() < 0.4f) g[Kick][(size_t) (numSteps - 1)] = hit (90);
                break;
            }
        }
    }

    // ---------------------------------------------------------------- the engine: one bar at a time
    // Live playback, RECORD and PRINT all call this, so what you hear is what you get as MIDI.
    enum HitSource : juce::uint8 { FromPattern = 0, FromVariety = 1, FromGhosts = 2, FromFill = 3, Dropped = 4 };
    constexpr int maxHits = 512;

    struct Hit { float pos; float ms; int lane; float vel; int alt; };   // pos: 16ths from the bar start

    struct EngineIn
    {
        float swing = 0.0f;                                   // 0..0.5 of a 16th
        std::array<float, NumPads> laneSwing {}, push {};    // swing multiplier, push in ms
        float ghosts = 0.0f, loosen = 0.0f, variety = 0.0f;
        float feel = 0.0f;     // accents (hats lead on the beat), phrases that build into fills, fills that crescendo, energy that wanders
        float drift = 0.0f;    // each limb leans slowly ahead/behind over the bars (not random jitter: that's LOOSEN)
        float energy = 1.0f;   // velocity scale (arrangement sections)
        int style = 0, chain = 0, slot = 0;
        std::array<bool, NumPads> audible {};
        int fillType = FillAuto, fillLen = 8, autoFill = 0;
        bool crash = true;
        juce::uint32 seed = 1;
    };

    struct BarPlan
    {
        juce::int64 bar = -1;
        int slot = 0, nHits = 0;
        bool filled = false;
        std::array<Hit, maxHits> hits {};
        Grid played {};
        std::array<std::array<juce::uint8, numSteps>, NumPads> src {};
    };

    // smooth noise: same seed + x gives the same value, neighbouring x drift slowly (a drummer's lean, not dice)
    inline float smoothNoise (juce::uint32 seed, int lane, double x)
    {
        auto h = [&] (juce::int64 k)
        {
            juce::uint32 v = seed * 2654435761u ^ ((juce::uint32) lane * 40503u) ^ ((juce::uint32) k * 2246822519u);
            v ^= v >> 15; v *= 2246822519u; v ^= v >> 13; v *= 3266489917u; v ^= v >> 16;
            return (float) (v & 0xffffff) / 8388607.5f - 1.0f;
        };
        const double fl = std::floor (x); const auto i = (juce::int64) fl; const float f = (float) (x - fl), sm = f * f * (3.0f - 2.0f * f);
        return h (i) + (h (i + 1) - h (i)) * sm;
    }

    inline int slotForBar (const EngineIn& in, juce::int64 bar)
    {
        switch (in.chain) { case 1: return (int) (bar & 1); case 2: return (int) (bar & 3); case 3: return (bar & 3) == 3 ? 1 : 0; default: return in.slot; }
    }

    inline void makeBar (BarPlan& out, juce::int64 bar, const Grid& pattern, const EngineIn& in, bool fillNow, bool prevFilled)
    {
        // a fixed seed per bar: re-planning the same bar (e.g. FILL pressed halfway) keeps the first half identical
        juce::Random r ((juce::int64) in.seed * 1000003LL + bar * 7919LL + 17);
        out.bar = bar; out.nHits = 0;
        Grid g = pattern;
        for (auto& row : out.src) row.fill (FromPattern);
        auto mark = [&] (const Grid& before, juce::uint8 why)
        {
            for (int p = 0; p < NumPads; ++p) for (int s = 0; s < numSteps; ++s)
            {
                const auto a = before[(size_t) p][(size_t) s].vel, b = g[(size_t) p][(size_t) s].vel;
                if ((a > 0) != (b > 0) || (b > 0 && a != b)) out.src[(size_t) p][(size_t) s] = b == 0 ? (juce::uint8) Dropped : why;
            }
        };
        if (in.variety > 0.0f && bar > 0)
        {
            const auto before = g;
            vary (g, in.style, r, (int) std::floor (in.variety * 4.0f + r.nextFloat()));
            if (in.variety > 0.4f && (bar % 4) == 3 && r.nextFloat() < in.variety)   // end of a 4-bar phrase: little lift
                for (int s = 12; s < 16; ++s) if (! g[Snare][(size_t) s].on() && r.nextFloat() < 0.5f) g[Snare][(size_t) s] = hit (40);
            mark (before, FromVariety);
        }
        if (in.ghosts > 0.25f)   // auto ghosts in the gaps, on the off 16ths
        {
            const auto before = g;
            for (int s = 1; s < 16; s += 2)
            {
                if (! g[Snare][(size_t) s].on() && r.nextFloat() < (in.ghosts - 0.25f) * 0.4f) g[Snare][(size_t) s] = hit (36);
                if (! g[Hat][(size_t) s].on() && ! g[OpenHat][(size_t) s].on() && r.nextFloat() < (in.ghosts - 0.25f) * 0.6f) g[Hat][(size_t) s] = hit (40);
            }
            mark (before, FromGhosts);
        }
        const bool autoNow = in.autoFill > 0 && (bar % in.autoFill) == in.autoFill - 1;
        out.filled = fillNow || autoNow;
        if (out.filled || (prevFilled && in.crash))
        {
            const auto before = g;
            if (out.filled) applyFill (g, in.fillType, in.fillLen, in.style, r);
            if (prevFilled && in.crash) g[Crash][0] = hit (118);   // land the fill
            mark (before, FromFill);
        }
        for (int p = 0; p < NumPads; ++p) for (int s = 0; s < numSteps; ++s)   // per-step chance
        {
            auto& c = g[(size_t) p][(size_t) s];
            if (c.on() && c.prob < 100 && r.nextInt (100) >= c.prob) { c.vel = 0; out.src[(size_t) p][(size_t) s] = Dropped; }
        }
        out.played = g;
        out.slot = slotForBar (in, bar);

        // timing + velocity: swing (per lane), nudge, push, loosen; ratchets split the step
        for (int p = 0; p < NumPads; ++p)
        {
            if (! in.audible[(size_t) p]) continue;
            for (int s = 0; s < numSteps; ++s)
            {
                const auto c = g[(size_t) p][(size_t) s];
                if (! c.on()) continue;
                float v = (float) c.vel / 127.0f;
                if (c.vel < 64) v *= 0.55f + 0.9f * in.ghosts;   // GHOSTS also sets how loud ghost notes are
                if (in.feel > 0.0f)
                {
                    // the hand on the hats leads on the beat and lightens on the offs
                    static const float acc[4] = { 1.0f, 0.76f, 0.9f, 0.72f };
                    if ((p == Hat || p == Shaker || p == Perc || p == OpenHat) && c.vel >= 64) v *= 1.0f - in.feel * (1.0f - acc[s & 3]);
                    // energy wanders bar to bar, and each 4-bar phrase leans into its last bar
                    v *= 1.0f + in.feel * (0.08f * smoothNoise (in.seed, 99, (double) bar * 0.5) + 0.06f * ((float) (bar & 3) / 3.0f) - 0.03f);
                    // fills crescendo into the one
                    if (out.src[(size_t) p][(size_t) s] == FromFill && p != Crash) v *= 1.0f - in.feel * 0.3f * (1.0f - (float) s / 15.0f);
                }
                v *= in.energy;
                const float g1 = r.nextFloat() + r.nextFloat() - 1.0f, g2 = r.nextFloat() + r.nextFloat() - 1.0f;
                v = juce::jlimit (0.05f, 1.0f, v * (1.0f + g1 * 0.35f * in.loosen));
                float pos = (float) s + (float) c.nudge / 100.0f;
                if (s & 1) pos += in.swing * in.laneSwing[(size_t) p];
                float ms = in.push[(size_t) p] + g2 * in.loosen * 14.0f;
                if (in.drift > 0.0f)   // each limb leans on its own, slowly; the kick stays the anchor
                    ms += in.drift * ((p == Kick ? 2.5f : 8.0f) * smoothNoise (in.seed, p, (double) bar * 0.5 + (double) s / 32.0)
                                      + 4.0f * smoothNoise (in.seed, 50, (double) bar * 0.25));
                const int n = juce::jlimit (1, 4, (int) c.ratchet);
                for (int k = 0; k < n && out.nHits < maxHits; ++k)
                    out.hits[(size_t) out.nHits++] = { pos + (float) k / (float) n, ms, p, juce::jlimit (0.05f, 1.0f, v * (k == 0 ? 1.0f : 0.78f + 0.08f * (float) k)), (int) c.alt };
            }
        }
    }
}
