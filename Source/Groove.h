#pragma once
#include <juce_core/juce_core.h>
#include "DrumVoices.h"
#include <array>

// Patterns: 4 slots x 8 drums x 16 steps. Cell levels: 0 rest, 1 ghost, 2 normal, 3 accent.
// Styles describe each drum as per-step probability + level, so ROLL writes a new pattern that
// always sounds like the style (2-step kick placement, shuffled hats, ghosted snares...).

namespace groove
{
    constexpr int numSlots = 4, numSteps = 16;
    enum Style { TwoStep, Burial, Funk, Breaks, NumStyles };

    inline const char* styleName (int s) { static const char* n[] = { "UK GARAGE 2-STEP", "BURIAL", "FUNK", "BREAKS" }; return n[juce::jlimit (0, 3, s)]; }

    // per-style defaults: tempo, swing %, ghosts, loosen, dust, push ms per drum
    struct StyleFeel { float tempo, swing, ghosts, loosen, dust; float push[NumPads]; };
    inline const StyleFeel& feel (int s)
    {
        static const StyleFeel f[NumStyles] = {
            // kick snare clap  rim   hat  open perc shaker
            { 132.0f, 58.0f, 0.40f, 0.15f, 0.10f, { 0.0f, 6.0f, 9.0f, -4.0f, 3.0f, 0.0f, -2.0f, 5.0f } },
            { 138.0f, 54.0f, 0.35f, 0.45f, 0.55f, { 0.0f, 18.0f, 22.0f, 6.0f, 8.0f, 4.0f, 10.0f, 12.0f } },
            { 100.0f, 57.0f, 0.60f, 0.12f, 0.05f, { 0.0f, 4.0f, 0.0f, -3.0f, -2.0f, 0.0f, 5.0f, 0.0f } },
            { 168.0f, 53.0f, 0.45f, 0.10f, 0.08f, { 0.0f, -3.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f } },
        };
        return f[juce::jlimit (0, 3, s)];
    }

    // Template strings, 16 chars each. prob: '.'=0, '1'..'9' = 10..90 %, 'X' = always. lvl: '1'..'3'.
    struct Tpl { const char* prob; const char* lvl; };
    inline Tpl tpl (int style, int pad)
    {
        static const Tpl T[NumStyles][NumPads] = {
            { // 2-STEP: kick on 1 and the skip before 3, never four-to-the-floor; claps on 2 & 4; shuffled hats
              { "X......2..8..2..", "3......2..2..2.." },
              { "....X..4....X.33", "....3..1....3.11" },
              { "....7.......7...", "....2.......2..." },
              { "..4...5..3...5..", "..1...2..1...2.." },
              { "8.39.8.98.39.8.9", "2.12.2.12.21.2.1" },
              { "..8...8...8...8.", "..2...2...2...2." },
              { "...3......3..2..", "...2......2..1.." },
              { "7777777777777777", "1111111111111111" } },
            { // BURIAL: sparse lopsided kicks, late heavy snare, crackly irregular hats
              { "9.........5..1..", "3.........2..2.." },
              { "....X..2....X..3", "....3..1....3..1" },
              { "............5...", "............2..." },
              { "...3.....3....3.", "...1.....2....1." },
              { "..5.4.5..4.5.3.5", "..2.1.2..1.2.1.1" },
              { "......4.......4.", "......2.......2." },
              { ".....1.....2....", ".....1.....2...." },
              { "5.5.5.5.5.5.5.5.", "1.1.1.1.1.1.1.1." } },
            { // FUNK: syncopated kick, backbeat with lots of ghost snares, 16th hats
              { "X..4..3...7X..2.", "3..2..2...22..2." },
              { ".4.4X.44.4.4X4.4", ".1.13.11.1.131.1" },
              { "................", "................" },
              { "......2.......2.", "......1.......2." },
              { "X7X7X7X7X7X7X7X.", "2121212121212121" },
              { "...............4", "...............2" },
              { ".3....2...3..2..", ".2....1...2..1.." },
              { "................", "................" } },
            { // BREAKS: classic chopped-break shapes, not any one record
              { "X.6.......X5....", "3.2.......22...." },
              { "....X..5.4..X..5", "....3..2.1..3..1" },
              { "................", "................" },
              { "..........2.....", "..........1....." },
              { "X4X4X4X4X4X4X4X4", "2121212121212121" },
              { "..............3.", "..............2." },
              { "......2.....2...", "......1.....1..." },
              { "................", "................" } },
        };
        return T[juce::jlimit (0, 3, style)][juce::jlimit (0, NumPads - 1, pad)];
    }

    inline float probAt (const char* s, int i) { const char c = s[i]; return c == 'X' ? 1.0f : (c >= '1' && c <= '9') ? (float) (c - '0') / 10.0f : 0.0f; }

    using Grid = std::array<std::array<juce::uint8, numSteps>, NumPads>;

    inline Grid roll (int style, juce::Random& r)
    {
        Grid g {};
        for (int p = 0; p < NumPads; ++p)
        {
            const auto t = tpl (style, p);
            for (int s = 0; s < numSteps; ++s)
                if (r.nextFloat() < probAt (t.prob, s)) g[(size_t) p][(size_t) s] = (juce::uint8) (t.lvl[s] - '0');
        }
        g[Kick][0] = 3;   // always anchor the bar
        // open hat chokes the closed hat on the same step
        for (int s = 0; s < numSteps; ++s) if (g[OpenHat][(size_t) s]) g[Hat][(size_t) s] = 0;
        return g;
    }

    // VARY: nudge a pattern without losing its identity
    inline void vary (Grid& g, int style, juce::Random& r, int moves = 3)
    {
        for (int m = 0; m < moves; ++m)
        {
            const int p = r.nextInt (NumPads), s = r.nextInt (numSteps);
            if (p == Kick && s == 0) continue;
            const auto t = tpl (style, p);
            const float pr = probAt (t.prob, s);
            auto& c = g[(size_t) p][(size_t) s];
            if (c == 0 && pr > 0.0f) c = (juce::uint8) (t.lvl[s] - '0');      // add a plausible hit
            else if (c != 0 && pr < 1.0f) c = 0;                               // drop a non-essential one
            else if (c == 2) c = 1;                                            // soften to a ghost
        }
    }

    // FILL: a short build over the second half of the bar
    inline void fill (Grid& g, juce::Random& r)
    {
        static const juce::uint8 roll[8] = { 1, 1, 2, 1, 2, 2, 3, 3 };
        for (int s = 8; s < numSteps; ++s)
        {
            g[Snare][(size_t) s] = r.nextFloat() < 0.8f ? roll[s - 8] : 0;
            g[Perc][(size_t) s] = (s & 1) && r.nextFloat() < 0.6f ? 2 : 0;
            g[Kick][(size_t) s] = 0;
        }
        g[Kick][14] = r.nextFloat() < 0.5f ? 2 : 0;
    }
}
