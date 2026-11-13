#pragma once
#include "DrumVoices.h"
#include "SoundTrims.h"
#include <vector>
#include <functional>

// Every synth sound in the box. Any pad (except the kick, which has the Kick Lab)
// can load any of these. Recipes follow the classic machines: 808 metal = six
// detuned squares, 808 cowbell = two squares at 540/800 Hz, claps = 3-5 noise
// bursts and a tail, snares = two tuned partials + filtered noise.
namespace sounds
{
    struct Sound { const char* name; const char* cat; Patch p; };

    // the recorded layers bundled with the app (Virtuosity Drums, CC0): order = RealSet index
    inline const juce::StringArray& realNames()
    {
        static const juce::StringArray n { "kick", "snare", "snareoff", "rimshot", "crossstick", "hhclosed", "hh34", "hhopen", "hhpedal",
                                           "shaker", "cabasa", "tambourine", "claves", "woodblock", "conga", "bongo", "cowbell", "guiro",
                                           // added: toms, hand drums, rims, bells (append only: patches store these indexes)
                                           "tomlo", "tomhi", "tommute", "tomrim", "stickshot", "congaopen", "congamute", "tumba", "bongolo", "agogohi", "agogolo" };
        return n;
    }
    inline int real (const char* name) { const int i = realNames().indexOf (name); jassert (i >= 0); return i; }

    inline Sound mk (const char* n, const char* c, const std::function<void (Patch&)>& f) { Sound s { n, c, {} }; f (s.p); return s; }

    inline const std::vector<Sound>& library()
    {
        static const std::vector<Sound> lib = []
        {
            std::vector<Sound> v;
            // ---------------- KICKS (for a second kick on another pad)
            v.push_back (mk ("808 Boom", "Kicks", [] (Patch& p) { p.freq = 47; p.sweep = 2.2f; p.drop = 0.04f; p.curve = 0.2f; p.decay = 0.55f; p.body = 1; p.click = 0.12f; p.clickType = 2; p.gain = 0.95f; }));
            v.push_back (mk ("909 Kick", "Kicks", [] (Patch& p) { p.wave = 2; p.freq = 53; p.sweep = 4.2f; p.drop = 0.024f; p.hold = 0.02f; p.decay = 0.2f; p.body = 1; p.drive = 0.35f; p.click = 0.5f; p.clickTone = 0.7f; p.gain = 0.9f; p.real = real ("kick"); p.realMix = 0.25f; p.realRoom = 0.5f; }));
            v.push_back (mk ("Garage Thud", "Kicks", [] (Patch& p) { p.freq = 55; p.sweep = 3.2f; p.drop = 0.022f; p.hold = 0.012f; p.decay = 0.13f; p.body = 1; p.drive = 0.2f; p.click = 0.3f; p.clickType = 1; p.sub = 0.3f; p.subDecay = 0.2f; p.gain = 0.9f; p.real = real ("kick"); p.realMix = 0.2f; p.realRoom = 0.4f; }));
            v.push_back (mk ("Lo-Fi Kick", "Kicks", [] (Patch& p) { p.freq = 60; p.sweep = 2.5f; p.drop = 0.03f; p.decay = 0.16f; p.body = 1; p.drive = 0.6f; p.click = 0.2f; p.clickTone = 0.25f; p.gain = 0.85f; p.real = real ("kick"); p.realMix = 0.35f; p.realRoom = 0.6f; }));
            // ---------------- SNARES
            v.push_back (mk ("808 Snare", "Snares", [] (Patch& p) { p.freq = 180; p.ratio2 = 1.83f; p.level2 = 0.5f; p.body = 0.8f; p.decay = 0.1f; p.noise = 0.6f; p.noiseHz = 3500; p.noiseQ = 0.5f; p.noiseDecay = 0.14f; p.lpHz = 12000; p.room = 0.15f; p.gain = 0.9f; p.real = real ("snareoff"); p.realMix = 0.3f; p.realRoom = 0.6f; p.roomSize = 0.8f; }));
            v.push_back (mk ("909 Snare", "Snares", [] (Patch& p) { p.freq = 190; p.sweep = 1.6f; p.drop = 0.012f; p.ratio2 = 1.75f; p.level2 = 0.35f; p.body = 0.7f; p.decay = 0.08f; p.noise = 0.8f; p.noiseHz = 4000; p.noiseQ = 0.45f; p.noiseDecay = 0.18f; p.drive = 0.2f; p.click = 0.2f; p.clickType = 3; p.lpHz = 13000; p.room = 0.15f; p.gain = 0.85f; p.real = real ("snare"); p.realMix = 0.35f; p.realRoom = 0.5f; p.roomSize = 1.0f; }));
            v.push_back (mk ("Garage Snare", "Snares", [] (Patch& p) { p.freq = 210; p.sweep = 1.5f; p.drop = 0.012f; p.ratio2 = 1.6f; p.level2 = 0.35f; p.body = 0.7f; p.decay = 0.07f; p.noise = 0.85f; p.noiseHz = 2600; p.noiseQ = 0.6f; p.noiseDecay = 0.13f; p.drive = 0.3f; p.click = 0.2f; p.clickType = 3; p.crush = 0.2f; p.lpHz = 12000; p.hpHz = 90; p.room = 0.2f; p.gain = 0.95f; p.real = real ("rimshot"); p.realMix = 0.45f; p.realRoom = 0.5f; p.roomSize = 0.7f; }));
            v.push_back (mk ("Burial Snare", "Snares", [] (Patch& p) { p.freq = 185; p.sweep = 1.5f; p.drop = 0.015f; p.ratio2 = 1.55f; p.level2 = 0.3f; p.body = 0.75f; p.decay = 0.09f; p.noise = 0.9f; p.noiseHz = 1700; p.noiseQ = 0.5f; p.noiseDecay = 0.26f; p.bursts = 2; p.burstGap = 0.012f; p.drive = 0.35f; p.crush = 0.45f; p.lpHz = 7500; p.hpHz = 120; p.room = 0.45f; p.gain = 1.15f; p.real = real ("snare"); p.realMix = 0.55f; p.realRoom = 1.0f; p.roomSize = 1.6f; }));
            v.push_back (mk ("Linn Snare", "Snares", [] (Patch& p) { p.wave = 2; p.freq = 205; p.sweep = 1.3f; p.drop = 0.015f; p.ratio2 = 2.2f; p.level2 = 0.25f; p.body = 0.6f; p.decay = 0.09f; p.noise = 0.6f; p.noiseHz = 1800; p.noiseQ = 0.9f; p.noiseDecay = 0.14f; p.gain = 0.85f; p.crush = 0.25f; p.lpHz = 11000; p.room = 0.2f; p.real = real ("snare"); p.realMix = 0.4f; p.realRoom = 0.6f; p.roomSize = 0.9f; }));
            v.push_back (mk ("Rimshot Snare", "Snares", [] (Patch& p) { p.freq = 330; p.ratio2 = 1.62f; p.level2 = 0.5f; p.body = 0.5f; p.decay = 0.03f; p.noise = 0.7f; p.noiseHz = 5000; p.noiseQ = 1.0f; p.noiseDecay = 0.07f; p.click = 0.5f; p.clickType = 3; p.drive = 0.3f; p.gain = 0.8f; p.lpHz = 10000; p.room = 0.2f; p.real = real ("rimshot"); p.realMix = 0.6f; p.realRoom = 0.6f; p.roomSize = 0.8f; }));
            v.push_back (mk ("Brush", "Snares", [] (Patch& p) { p.noise = 0.85f; p.noiseHz = 4000; p.noiseQ = 0.4f; p.noiseDecay = 0.18f; p.attack = 0.012f; p.gain = 1.4f; p.lpHz = 9000; p.room = 0.3f; p.roomSize = 1.2f; }));
            v.push_back (mk ("Noise Snare", "Snares", [] (Patch& p) { p.freq = 200; p.body = 0.3f; p.decay = 0.04f; p.noise = 1.0f; p.noiseMode = 0; p.noiseHz = 6000; p.noiseQ = 0.7f; p.noiseDecay = 0.2f; p.gain = 0.95f; p.room = 0.3f; p.crush = 0.3f; p.real = real ("snare"); p.realMix = 0.3f; p.realRoom = 0.8f; p.roomSize = 1.4f; }));
            v.push_back (mk ("Vinyl Snare", "Snares", [] (Patch& p) { p.freq = 200; p.sweep = 1.3f; p.drop = 0.015f; p.decay = 0.08f; p.body = 0.7f; p.noise = 0.85f; p.noiseHz = 2200; p.noiseQ = 0.55f; p.noiseDecay = 0.2f; p.drive = 0.5f; p.crush = 0.6f; p.lpHz = 6500; p.hpHz = 150; p.room = 0.3f; p.gain = 1.1f; p.real = real ("snare"); p.realMix = 0.5f; p.realRoom = 0.6f; p.roomSize = 1.1f; }));
            v.push_back (mk ("Rainy Snare", "Snares", [] (Patch& p) { p.freq = 170; p.body = 0.5f; p.decay = 0.06f; p.noise = 1.0f; p.noiseHz = 2000; p.noiseQ = 0.6f; p.noiseDecay = 0.22f; p.grains = 8; p.grainSpread = 0.03f; p.grainDecay = 0.008f; p.crush = 0.35f; p.lpHz = 7000; p.hpHz = 140; p.room = 0.55f; p.gain = 1.15f; p.real = real ("snareoff"); p.realMix = 0.4f; p.realRoom = 1.0f; p.roomSize = 1.9f; }));
            v.push_back (mk ("Fat Snare", "Snares", [] (Patch& p) { p.freq = 160; p.sweep = 1.8f; p.drop = 0.02f; p.decay = 0.14f; p.body = 0.9f; p.ratio2 = 1.5f; p.level2 = 0.4f; p.noise = 0.8f; p.noiseHz = 1400; p.noiseQ = 0.5f; p.noiseDecay = 0.2f; p.drive = 0.45f; p.lpHz = 10000; p.hpHz = 90; p.room = 0.08f; p.gain = 0.8f; p.real = real ("snareoff"); p.realMix = 0.4f; p.realRoom = 0.22f; p.realTune = -2.0f; p.roomSize = 0.8f; }));
            // ---------------- CLAPS
            v.push_back (mk ("808 Clap", "Claps", [] (Patch& p) { p.noise = 1; p.noiseHz = 1100; p.noiseQ = 1.6f; p.bursts = 3; p.burstGap = 0.009f; p.noiseDecay = 0.16f; p.gain = 1.35f; p.noiseQ = 1.0f; p.noiseHz = 1200; p.room = 0.3f; p.lpHz = 9000; p.roomSize = 0.9f; }));
            v.push_back (mk ("909 Clap", "Claps", [] (Patch& p) { p.noise = 1; p.noiseHz = 1350; p.noiseQ = 1.2f; p.bursts = 4; p.burstGap = 0.007f; p.noiseDecay = 0.2f; p.gain = 1.3f; p.noiseQ = 0.9f; p.room = 0.3f; p.lpHz = 10000; p.roomSize = 1.1f; }));
            v.push_back (mk ("Tight Clap", "Claps", [] (Patch& p) { p.noise = 1; p.noiseHz = 1600; p.noiseQ = 1.4f; p.bursts = 3; p.burstGap = 0.006f; p.noiseDecay = 0.08f; p.gain = 1.1f; p.room = 0.15f; p.lpHz = 10000; p.gain *= 1.3f; p.roomSize = 0.5f; }));
            v.push_back (mk ("Big Room Clap", "Claps", [] (Patch& p) { p.noise = 1; p.noiseHz = 1000; p.noiseQ = 0.9f; p.bursts = 5; p.burstGap = 0.011f; p.noiseDecay = 0.35f; p.gain = 1.4f; p.room = 0.6f; p.lpHz = 9500; p.roomSize = 2.0f; }));
            v.push_back (mk ("Finger Snap", "Claps", [] (Patch& p) { p.freq = 1800; p.body = 0.3f; p.decay = 0.01f; p.noise = 0.9f; p.noiseHz = 2600; p.noiseQ = 2.0f; p.noiseDecay = 0.03f; p.click = 0.6f; p.clickType = 3; p.gain = 0.9f; p.room = 0.35f; p.lpHz = 9000; p.roomSize = 0.6f; }));
            v.push_back (mk ("Room Clap", "Claps", [] (Patch& p) { p.noise = 1; p.noiseHz = 1300; p.noiseQ = 0.7f; p.bursts = 3; p.burstGap = 0.01f; p.noiseDecay = 0.22f; p.drive = 0.2f; p.crush = 0.3f; p.lpHz = 8000; p.room = 0.7f; p.gain = 1.3f; p.real = real ("snareoff"); p.realMix = 0.15f; p.realRoom = 1.2f; p.roomSize = 1.8f; }));
            v.push_back (mk ("Dusty Clap", "Claps", [] (Patch& p) { p.noise = 1; p.noiseHz = 1500; p.noiseQ = 0.8f; p.bursts = 4; p.burstGap = 0.008f; p.noiseDecay = 0.16f; p.crush = 0.55f; p.lpHz = 7000; p.hpHz = 200; p.room = 0.35f; p.gain = 1.35f; p.roomSize = 1.0f; }));
            v.push_back (mk ("Lighter Flick", "Claps", [] (Patch& p) { p.noise = 0.9f; p.noiseHz = 3800; p.noiseQ = 1.2f; p.noiseDecay = 0.02f; p.grains = 5; p.grainSpread = 0.025f; p.grainDecay = 0.004f; p.metal = 0.3f; p.metalScale = 2.0f; p.metalDecay = 0.02f; p.crush = 0.4f; p.lpHz = 9000; p.room = 0.3f; p.gain = 1.2f; p.gain *= 1.3f; p.roomSize = 0.6f; }));
            // ---------------- RIMS & CLICKS
            v.push_back (mk ("808 Rim", "Rims & Clicks", [] (Patch& p) { p.freq = 480; p.ratio2 = 3.52f; p.level2 = 0.5f; p.body = 0.5f; p.decay = 0.022f; p.noise = 0.4f; p.noiseHz = 1750; p.noiseQ = 3.0f; p.noiseDecay = 0.02f; p.gain = 0.9f; p.real = real ("crossstick"); p.realMix = 0.5f; p.realRoom = 0.6f; p.roomSize = 0.7f; }));
            v.push_back (mk ("909 Rim", "Rims & Clicks", [] (Patch& p) { p.wave = 3; p.freq = 500; p.ratio2 = 2.2f; p.level2 = 0.4f; p.body = 0.4f; p.decay = 0.015f; p.noise = 0.3f; p.noiseHz = 3000; p.noiseDecay = 0.012f; p.click = 0.4f; p.clickType = 2; p.gain = 0.8f; p.real = real ("crossstick"); p.realMix = 0.4f; p.realRoom = 0.5f; }));
            v.push_back (mk ("Woodblock", "Rims & Clicks", [] (Patch& p) { p.wave = 1; p.freq = 900; p.sweep = 1.15f; p.drop = 0.004f; p.ratio2 = 2.8f; p.level2 = 0.15f; p.body = 0.8f; p.decay = 0.035f; p.gain = 0.85f; p.real = real ("woodblock"); p.realMix = 0.6f; p.realRoom = 0.0f; }));
            v.push_back (mk ("Clave", "Rims & Clicks", [] (Patch& p) { p.freq = 2500; p.body = 0.8f; p.decay = 0.028f; p.gain = 0.8f; p.real = real ("claves"); p.realMix = 0.7f; p.realRoom = 0.8f; }));
            v.push_back (mk ("Click", "Rims & Clicks", [] (Patch& p) { p.click = 1.0f; p.clickType = 2; p.clickTone = 0.8f; p.noise = 0.3f; p.noiseMode = 2; p.noiseHz = 6000; p.noiseDecay = 0.006f; p.gain = 1.6f; }));
            v.push_back (mk ("Stick", "Rims & Clicks", [] (Patch& p) { p.freq = 1200; p.body = 0.4f; p.decay = 0.018f; p.noise = 0.6f; p.noiseHz = 3500; p.noiseQ = 1.5f; p.noiseDecay = 0.018f; p.click = 0.3f; p.clickType = 3; p.gain = 0.9f; p.real = real ("crossstick"); p.realMix = 0.4f; p.realRoom = 0.5f; }));
            v.push_back (mk ("Dusty Rim", "Rims & Clicks", [] (Patch& p) { p.freq = 480; p.ratio2 = 3.52f; p.level2 = 0.5f; p.body = 0.55f; p.decay = 0.025f; p.noise = 0.4f; p.noiseHz = 1750; p.noiseQ = 3.0f; p.noiseDecay = 0.02f; p.crush = 0.5f; p.lpHz = 6000; p.room = 0.35f; p.gain = 1.0f; p.real = real ("crossstick"); p.realMix = 0.6f; p.realRoom = 0.9f; p.roomSize = 1.3f; }));
            v.push_back (mk ("Foley Knock", "Rims & Clicks", [] (Patch& p) { p.wave = 1; p.freq = 650; p.sweep = 1.3f; p.drop = 0.004f; p.decay = 0.03f; p.body = 0.8f; p.noise = 0.5f; p.noiseHz = 1500; p.noiseQ = 1.0f; p.noiseDecay = 0.02f; p.crush = 0.4f; p.lpHz = 7000; p.room = 0.4f; p.gain = 0.95f; p.real = real ("snareoff"); p.realMix = 0.5f; p.realRoom = 1.0f; p.realTune = 3.0f; p.roomSize = 1.4f; }));
            // ---------------- CLOSED HATS
            v.push_back (mk ("808 Hat", "Closed Hats", [] (Patch& p) { p.metal = 0.85f; p.metalBp = 10000; p.metalHp = 7000; p.metalDecay = 0.045f; p.noise = 0.2f; p.noiseMode = 2; p.noiseHz = 9000; p.noiseDecay = 0.04f; p.gain = 1.5f; p.lpHz = 12000; p.lpHz = 10000; p.real = real ("hhclosed"); p.realMix = 0.3f; p.realRoom = 0.5f; }));
            v.push_back (mk ("909 Hat", "Closed Hats", [] (Patch& p) { p.metal = 0.4f; p.metalBp = 11000; p.metalHp = 8000; p.metalDecay = 0.05f; p.noise = 0.7f; p.noiseMode = 2; p.noiseHz = 8000; p.noiseDecay = 0.05f; p.gain = 0.85f; p.lpHz = 12500; p.lpHz = 10500; p.real = real ("hhclosed"); p.realMix = 0.4f; p.realRoom = 0.5f; }));
            v.push_back (mk ("Tick Hat", "Closed Hats", [] (Patch& p) { p.metal = 0.5f; p.metalScale = 1.4f; p.metalBp = 12000; p.metalDecay = 0.02f; p.noise = 0.4f; p.noiseMode = 2; p.noiseHz = 10000; p.noiseDecay = 0.018f; p.gain = 1.3f; p.lpHz = 11000; p.lpHz = 10000; p.real = real ("hhpedal"); p.realMix = 0.4f; p.realRoom = 0.5f; }));
            v.push_back (mk ("Dusty Hat", "Closed Hats", [] (Patch& p) { p.noise = 0.85f; p.noiseHz = 6000; p.noiseQ = 0.9f; p.noiseDecay = 0.05f; p.metal = 0.25f; p.metalBp = 8000; p.metalDecay = 0.04f; p.crush = 0.45f; p.lpHz = 9000; p.room = 0.15f; p.gain = 0.95f; p.real = real ("hh34"); p.realMix = 0.5f; p.realRoom = 0.7f; p.roomSize = 1.0f; }));
            v.push_back (mk ("FM Hat", "Closed Hats", [] (Patch& p) { p.freq = 3200; p.fmRatio = 2.71f; p.fmIndex = 4.0f; p.fmDecay = 0.03f; p.body = 0.5f; p.decay = 0.035f; p.noise = 0.3f; p.noiseMode = 2; p.noiseHz = 9000; p.noiseDecay = 0.03f; p.gain = 0.7f; p.lpHz = 11000; }));
            v.push_back (mk ("Burial Hat", "Closed Hats", [] (Patch& p) { p.noise = 1.0f; p.noiseHz = 5500; p.noiseQ = 1.0f; p.noiseDecay = 0.035f; p.grains = 3; p.grainSpread = 0.012f; p.grainDecay = 0.004f; p.click = 0.3f; p.clickType = 2; p.clickTone = 0.3f; p.crush = 0.5f; p.lpHz = 8000; p.hpHz = 2500; p.room = 0.2f; p.gain = 0.85f; p.noiseDecay = 0.05f; p.real = real ("hhpedal"); p.realMix = 0.6f; p.realRoom = 0.8f; p.roomSize = 1.3f; }));
            v.push_back (mk ("Shuffle Tick", "Closed Hats", [] (Patch& p) { p.noise = 0.8f; p.noiseMode = 2; p.noiseHz = 4000; p.noiseDecay = 0.02f; p.click = 0.5f; p.clickType = 3; p.clickTone = 0.4f; p.crush = 0.4f; p.lpHz = 9000; p.gain = 1.1f; p.real = real ("hhclosed"); p.realMix = 0.5f; p.realRoom = 0.6f; }));
            // ---------------- OPEN HATS & CYMBALS
            v.push_back (mk ("808 Open Hat", "Open Hats & Cymbals", [] (Patch& p) { p.metal = 0.85f; p.metalBp = 9500; p.metalHp = 7000; p.metalDecay = 0.32f; p.noise = 0.2f; p.noiseMode = 2; p.noiseHz = 9000; p.noiseDecay = 0.3f; p.gain = 1.3f; p.lpHz = 12000; p.lpHz = 10500; p.real = real ("hhopen"); p.realMix = 0.3f; p.realRoom = 0.5f; }));
            v.push_back (mk ("909 Open Hat", "Open Hats & Cymbals", [] (Patch& p) { p.metal = 0.4f; p.metalBp = 10500; p.metalDecay = 0.38f; p.noise = 0.6f; p.noiseMode = 2; p.noiseHz = 7500; p.noiseDecay = 0.35f; p.gain = 0.75f; p.lpHz = 12000; p.lpHz = 10500; p.real = real ("hhopen"); p.realMix = 0.4f; p.realRoom = 0.5f; }));
            v.push_back (mk ("Garage Open Hat", "Open Hats & Cymbals", [] (Patch& p) { p.metal = 0.6f; p.metalScale = 1.15f; p.metalBp = 11000; p.metalDecay = 0.2f; p.noise = 0.45f; p.noiseMode = 2; p.noiseHz = 9000; p.noiseDecay = 0.18f; p.gain = 0.8f; p.lpHz = 11500; p.lpHz = 10500; p.real = real ("hh34"); p.realMix = 0.4f; p.realRoom = 0.5f; }));
            v.push_back (mk ("Dusty Open Hat", "Open Hats & Cymbals", [] (Patch& p) { p.noise = 0.9f; p.noiseHz = 6500; p.noiseQ = 0.8f; p.noiseDecay = 0.25f; p.metal = 0.25f; p.metalDecay = 0.2f; p.crush = 0.45f; p.lpHz = 8500; p.hpHz = 1500; p.room = 0.2f; p.gain = 0.9f; p.real = real ("hhopen"); p.realMix = 0.5f; p.realRoom = 0.8f; p.roomSize = 1.2f; }));
            v.push_back (mk ("Ride", "Open Hats & Cymbals", [] (Patch& p) { p.metal = 0.7f; p.metalScale = 0.9f; p.metalBp = 6500; p.metalHp = 4000; p.metalDecay = 1.0f; p.freq = 3400; p.body = 0.12f; p.decay = 0.6f; p.fmRatio = 1.41f; p.fmIndex = 0.6f; p.fmDecay = 0.5f; p.noise = 0.15f; p.noiseMode = 2; p.noiseHz = 7000; p.noiseDecay = 0.8f; p.gain = 0.95f; }));
            v.push_back (mk ("Crash", "Open Hats & Cymbals", [] (Patch& p) { p.metal = 0.6f; p.metalScale = 1.2f; p.metalBp = 7000; p.metalHp = 4500; p.metalDecay = 1.4f; p.noise = 0.7f; p.noiseMode = 2; p.noiseHz = 5000; p.noiseDecay = 1.3f; p.gain = 0.55f; }));
            // ---------------- SHAKERS
            v.push_back (mk ("Shaker", "Shakers", [] (Patch& p) { p.noise = 1; p.noiseHz = 6500; p.noiseQ = 1.4f; p.attack = 0.008f; p.noiseDecay = 0.06f; p.gain = 0.85f; p.real = real ("shaker"); p.realMix = 0.6f; p.realRoom = 0.6f; }));
            v.push_back (mk ("Cabasa", "Shakers", [] (Patch& p) { p.noise = 1; p.noiseMode = 2; p.noiseHz = 8000; p.attack = 0.004f; p.noiseDecay = 0.045f; p.gain = 0.8f; p.lpHz = 10500; p.lpHz = 9500; p.real = real ("cabasa"); p.realMix = 0.6f; p.realRoom = 0.6f; }));
            v.push_back (mk ("Maracas", "Shakers", [] (Patch& p) { p.noise = 1; p.noiseMode = 2; p.noiseHz = 5000; p.attack = 0.002f; p.noiseDecay = 0.03f; p.gain = 0.8f; p.lpHz = 10000; p.real = real ("shaker"); p.realMix = 0.4f; p.realRoom = 0.5f; p.realTune = 2.0f; }));
            v.push_back (mk ("Tambourine", "Shakers", [] (Patch& p) { p.metal = 0.5f; p.metalScale = 2.2f; p.metalBp = 9000; p.metalDecay = 0.12f; p.noise = 0.5f; p.noiseHz = 7000; p.noiseDecay = 0.1f; p.bursts = 2; p.burstGap = 0.012f; p.gain = 1.1f; p.real = real ("tambourine"); p.realMix = 0.6f; p.realRoom = 0.6f; }));
            v.push_back (mk ("Egg Shaker", "Shakers", [] (Patch& p) { p.noise = 1; p.noiseHz = 4500; p.noiseQ = 1.0f; p.attack = 0.015f; p.noiseDecay = 0.05f; p.gain = 0.85f; p.real = real ("shaker"); p.realMix = 0.5f; p.realRoom = 0.6f; p.realTune = -2.0f; }));
            v.push_back (mk ("Rattle", "Shakers", [] (Patch& p) { p.noise = 1.0f; p.noiseHz = 4500; p.noiseQ = 1.1f; p.noiseDecay = 0.05f; p.grains = 10; p.grainSpread = 0.07f; p.grainDecay = 0.005f; p.crush = 0.3f; p.lpHz = 9000; p.room = 0.15f; p.gain = 1.1f; p.real = real ("guiro"); p.realMix = 0.3f; p.realRoom = 0.6f; }));
            v.push_back (mk ("Keys Jangle", "Shakers", [] (Patch& p) { p.noise = 0.7f; p.noiseHz = 7000; p.noiseQ = 1.5f; p.noiseDecay = 0.12f; p.grains = 14; p.grainSpread = 0.12f; p.grainDecay = 0.008f; p.metal = 0.4f; p.metalScale = 2.5f; p.metalBp = 7000; p.metalDecay = 0.15f; p.lpHz = 10000; p.room = 0.2f; p.gain = 1.0f; p.real = real ("tambourine"); p.realMix = 0.3f; p.realRoom = 0.7f; }));
            // ---------------- TOMS & CONGAS
            v.push_back (mk ("808 Tom Low", "Toms", [] (Patch& p) { p.freq = 92; p.sweep = 1.16f; p.drop = 0.045f; p.decay = 0.2f; p.body = 1; p.modes = 0.06f; p.noise = 0.1f; p.noiseMode = 0; p.noiseHz = 2500; p.noiseDecay = 0.03f; p.gain = 0.85f; }));
            v.push_back (mk ("808 Tom High", "Toms", [] (Patch& p) { p.freq = 150; p.sweep = 1.16f; p.drop = 0.04f; p.decay = 0.16f; p.body = 1; p.modes = 0.06f; p.noise = 0.1f; p.noiseMode = 0; p.noiseHz = 3000; p.noiseDecay = 0.025f; p.gain = 0.8f; }));
            v.push_back (mk ("909 Tom", "Toms", [] (Patch& p) { p.freq = 120; p.sweep = 1.35f; p.drop = 0.022f; p.decay = 0.2f; p.body = 1; p.modes = 0.15f; p.noise = 0.3f; p.noiseHz = 1500; p.noiseDecay = 0.04f; p.drive = 0.2f; p.click = 0.2f; p.clickType = 3; p.gain = 0.8f; }));
            v.push_back (mk ("808 Conga", "Congas & Hand Drums", [] (Patch& p) { p.freq = 330; p.sweep = 1.1f; p.drop = 0.01f; p.decay = 0.11f; p.body = 1; p.gain = 0.8f; p.real = real ("congaopen"); p.realMix = 0.3f; p.realRoom = 0.5f; p.realTune = 4.0f; }));
            v.push_back (mk ("Low Conga", "Congas & Hand Drums", [] (Patch& p) { p.freq = 210; p.sweep = 1.15f; p.drop = 0.012f; p.decay = 0.13f; p.body = 1; p.gain = 0.8f; p.real = real ("congaopen"); p.realMix = 0.4f; p.realRoom = 0.6f; p.realTune = -1.0f; }));
            v.push_back (mk ("Bongo", "Congas & Hand Drums", [] (Patch& p) { p.freq = 420; p.sweep = 1.2f; p.drop = 0.006f; p.decay = 0.09f; p.body = 1; p.click = 0.2f; p.clickType = 1; p.gain = 0.8f; p.real = real ("bongo"); p.realMix = 0.6f; p.realRoom = 0.6f; }));
            v.push_back (mk ("Talking Drum", "Congas & Hand Drums", [] (Patch& p) { p.freq = 180; p.sweep = 1.8f; p.drop = 0.12f; p.curve = 0.9f; p.decay = 0.2f; p.body = 1; p.gain = 0.8f; }));
            v.push_back (mk ("Perc Thud", "Congas & Hand Drums", [] (Patch& p) { p.wave = 2; p.freq = 262; p.sweep = 1.45f; p.drop = 0.018f; p.decay = 0.12f; p.body = 0.9f; p.gain = 0.85f; p.real = real ("congaopen"); p.realMix = 0.3f; p.realRoom = 0.5f; }));
            // ---------------- BELLS & METAL
            v.push_back (mk ("808 Cowbell", "Bells & Metal", [] (Patch& p) { p.wave = 3; p.freq = 540; p.ratio2 = 1.48f; p.level2 = 1.0f; p.body = 0.5f; p.hold = 0.006f; p.decay = 0.12f; p.gain = 0.55f; p.real = real ("cowbell"); p.realMix = 0.25f; p.realRoom = 0.5f; }));
            v.push_back (mk ("Agogo", "Bells & Metal", [] (Patch& p) { p.freq = 900; p.fmRatio = 1.4f; p.fmIndex = 1.2f; p.fmDecay = 0.08f; p.body = 0.8f; p.decay = 0.15f; p.gain = 0.7f; }));
            v.push_back (mk ("FM Bell", "Bells & Metal", [] (Patch& p) { p.freq = 1200; p.fmRatio = 3.5f; p.fmIndex = 3.0f; p.fmDecay = 0.2f; p.body = 0.8f; p.decay = 0.5f; p.gain = 0.6f; }));
            v.push_back (mk ("Triangle", "Bells & Metal", [] (Patch& p) { p.freq = 4200; p.ratio2 = 2.76f; p.level2 = 0.3f; p.fmRatio = 1.41f; p.fmIndex = 0.5f; p.fmDecay = 0.4f; p.body = 0.6f; p.decay = 0.8f; p.gain = 0.5f; }));
            v.push_back (mk ("Clank", "Bells & Metal", [] (Patch& p) { p.metal = 0.9f; p.metalScale = 0.6f; p.metalBp = 3000; p.metalHp = 1500; p.metalDecay = 0.12f; p.freq = 380; p.fmRatio = 1.73f; p.fmIndex = 3.0f; p.fmDecay = 0.06f; p.body = 0.35f; p.decay = 0.1f; p.drive = 0.5f; p.gain = 0.7f; }));
            v.push_back (mk ("Glass Ping", "Bells & Metal", [] (Patch& p) { p.freq = 2600; p.fmRatio = 5.2f; p.fmIndex = 1.0f; p.fmDecay = 0.1f; p.body = 0.7f; p.decay = 0.25f; p.gain = 0.6f; }));
            // ---------------- ZAPS & FX
            v.push_back (mk ("Laser Zap", "Zaps & FX", [] (Patch& p) { p.wave = 1; p.freq = 300; p.sweep = 8.0f; p.drop = 0.03f; p.curve = 0.5f; p.decay = 0.12f; p.body = 0.8f; p.velPitch = 0.0f; p.gain = 0.7f; }));
            v.push_back (mk ("Pew", "Zaps & FX", [] (Patch& p) { p.wave = 2; p.freq = 900; p.sweep = 4.0f; p.drop = 0.06f; p.curve = 0.8f; p.decay = 0.1f; p.body = 0.7f; p.gain = 0.6f; }));
            v.push_back (mk ("Blip", "Zaps & FX", [] (Patch& p) { p.freq = 1800; p.body = 0.7f; p.decay = 0.02f; p.gain = 0.7f; }));
            v.push_back (mk ("Dub Siren Hit", "Zaps & FX", [] (Patch& p) { p.wave = 1; p.freq = 700; p.sweep = 1.5f; p.drop = 0.25f; p.curve = 1.0f; p.fmRatio = 0.5f; p.fmIndex = 0.5f; p.fmDecay = 0.4f; p.decay = 0.5f; p.body = 0.6f; p.gain = 0.55f; }));
            v.push_back (mk ("Vinyl Pop", "Zaps & FX", [] (Patch& p) { p.click = 1.0f; p.clickType = 2; p.clickTone = 0.2f; p.noise = 0.4f; p.noiseMode = 0; p.noiseHz = 1500; p.noiseDecay = 0.01f; p.gain = 1.6f; }));
            // ---------------- REAL KIT: recorded hits (Virtuosity Drums, CC0) with their own room mics
            v.push_back (mk ("Real Kick", "Real Kit (recorded)", [] (Patch& p) { p.real = real ("kick"); p.realMix = 1.0f; p.realRoom = 0.4f; p.gain = 1.3f; }));
            v.push_back (mk ("Real Snare", "Real Kit (recorded)", [] (Patch& p) { p.real = real ("snare"); p.realMix = 1.0f; p.realRoom = 0.7f; p.gain = 1.6f; }));
            v.push_back (mk ("Real Snare (Snares Off)", "Real Kit (recorded)", [] (Patch& p) { p.real = real ("snareoff"); p.realMix = 1.0f; p.realRoom = 0.7f; p.gain = 1.6f; }));
            v.push_back (mk ("Real Rimshot", "Real Kit (recorded)", [] (Patch& p) { p.real = real ("rimshot"); p.realMix = 1.0f; p.realRoom = 0.6f; p.gain = 1.5f; }));
            v.push_back (mk ("Real Cross Stick", "Real Kit (recorded)", [] (Patch& p) { p.real = real ("crossstick"); p.realMix = 1.0f; p.realRoom = 0.6f; p.gain = 1.6f; }));
            v.push_back (mk ("Real Closed Hat", "Real Kit (recorded)", [] (Patch& p) { p.real = real ("hhclosed"); p.realMix = 1.0f; p.realRoom = 0.5f; p.gain = 1.3f; }));
            v.push_back (mk ("Real Half Hat", "Real Kit (recorded)", [] (Patch& p) { p.real = real ("hh34"); p.realMix = 1.0f; p.realRoom = 0.5f; p.gain = 1.3f; }));
            v.push_back (mk ("Real Open Hat", "Real Kit (recorded)", [] (Patch& p) { p.real = real ("hhopen"); p.realMix = 1.0f; p.realRoom = 0.5f; p.gain = 1.2f; }));
            v.push_back (mk ("Real Pedal Hat", "Real Kit (recorded)", [] (Patch& p) { p.real = real ("hhpedal"); p.realMix = 1.0f; p.realRoom = 0.5f; p.gain = 1.5f; }));
            v.push_back (mk ("Real Shaker", "Real Kit (recorded)", [] (Patch& p) { p.real = real ("shaker"); p.realMix = 1.0f; p.realRoom = 0.6f; p.gain = 1.5f; }));
            v.push_back (mk ("Real Cabasa", "Real Kit (recorded)", [] (Patch& p) { p.real = real ("cabasa"); p.realMix = 1.0f; p.realRoom = 0.6f; p.gain = 3.0f; }));
            v.push_back (mk ("Real Tambourine", "Real Kit (recorded)", [] (Patch& p) { p.real = real ("tambourine"); p.realMix = 1.0f; p.realRoom = 0.6f; p.gain = 1.3f; }));
            v.push_back (mk ("Real Claves", "Real Kit (recorded)", [] (Patch& p) { p.real = real ("claves"); p.realMix = 1.0f; p.realRoom = 0.7f; p.gain = 1.2f; }));
            v.push_back (mk ("Real Woodblock", "Real Kit (recorded)", [] (Patch& p) { p.real = real ("woodblock"); p.realMix = 1.0f; p.realRoom = 0.0f; p.gain = 1.0f; }));
            v.push_back (mk ("Real Conga", "Real Kit (recorded)", [] (Patch& p) { p.real = real ("congaopen"); p.realMix = 1.0f; p.realRoom = 0.6f; p.gain = 1.0f; }));
            v.push_back (mk ("Real Bongo", "Real Kit (recorded)", [] (Patch& p) { p.real = real ("bongo"); p.realMix = 1.0f; p.realRoom = 0.6f; p.gain = 1.0f; }));
            v.push_back (mk ("Real Cowbell", "Real Kit (recorded)", [] (Patch& p) { p.real = real ("cowbell"); p.realMix = 1.0f; p.realRoom = 0.5f; p.gain = 1.0f; }));
            v.push_back (mk ("Real Guiro", "Real Kit (recorded)", [] (Patch& p) { p.real = real ("guiro"); p.realMix = 1.0f; p.realRoom = 0.6f; p.gain = 1.6f; }));
            // ---------------- added: real toms, hand drums, rims and bells (append only: pads store the index)
            // TOMS: recorded close + room mics underneath a membrane synth. Low/floor toms play the big drum,
            // mid/high toms switch to the small drum, so fills don't sound pitch-shifted.
            v.push_back (mk ("Studio Tom", "Toms", [] (Patch& p) { p.freq = 116; p.sweep = 1.1f; p.drop = 0.05f; p.decay = 0.3f; p.body = 0.3f; p.modes = 0.25f; p.modeDecay = 0.45f; p.click = 0.12f; p.clickType = 3; p.real = real ("tomlo"); p.realHi = real ("tomhi"); p.realHiSemis = 7.8f; p.realHiGain = 1.3f; p.realMix = 1.0f; p.realRoom = 0.6f; }));
            v.push_back (mk ("Muted Tom", "Toms", [] (Patch& p) { p.freq = 110; p.sweep = 1.15f; p.drop = 0.03f; p.decay = 0.09f; p.body = 0.3f; p.real = real ("tommute"); p.realMix = 1.0f; p.realRoom = 0.35f; }));
            v.push_back (mk ("Tom Rim", "Toms", [] (Patch& p) { p.real = real ("tomrim"); p.realMix = 1.0f; p.realRoom = 0.5f; p.freq = 190; p.body = 0.15f; p.decay = 0.06f; }));
            v.push_back (mk ("Dub Tom", "Toms", [] (Patch& p) { p.freq = 98; p.sweep = 1.15f; p.drop = 0.05f; p.decay = 0.3f; p.body = 1; p.modes = 0.3f; p.modeDecay = 0.4f; p.noise = 0.18f; p.noiseMode = 0; p.noiseHz = 1200; p.noiseDecay = 0.03f; p.lpHz = 5000; p.room = 0.3f; p.real = real ("tomlo"); p.realHi = real ("tomhi"); p.realHiSemis = 7.8f; p.realHiGain = 1.3f; p.realMix = 0.35f; p.realRoom = 0.8f; p.realTune = -2.0f; }));
            v.push_back (mk ("Floor Thud", "Toms", [] (Patch& p) { p.freq = 72; p.sweep = 1.3f; p.drop = 0.03f; p.decay = 0.17f; p.body = 1; p.modes = 0.3f; p.modeDecay = 0.35f; p.click = 0.25f; p.clickType = 1; p.clickTone = 0.2f; p.lpHz = 6500; p.real = real ("tomlo"); p.realMix = 0.5f; p.realRoom = 0.5f; p.realTune = -5.0f; }));
            v.push_back (mk ("Simmons Tom", "Toms", [] (Patch& p) { p.freq = 105; p.sweep = 2.0f; p.drop = 0.07f; p.curve = 0.35f; p.decay = 0.22f; p.body = 1; p.noise = 0.3f; p.noiseHz = 2400; p.noiseDecay = 0.12f; p.drive = 0.15f; p.room = 0.3f; }));
            // CONGAS & HAND DRUMS
            v.push_back (mk ("Conga Open", "Congas & Hand Drums", [] (Patch& p) { p.freq = 165; p.sweep = 1.05f; p.drop = 0.02f; p.decay = 0.15f; p.body = 0.25f; p.modes = 0.12f; p.real = real ("congaopen"); p.realMix = 1.0f; p.realRoom = 0.5f; }));
            v.push_back (mk ("Conga Mute", "Congas & Hand Drums", [] (Patch& p) { p.real = real ("congamute"); p.realMix = 1.0f; p.realRoom = 0.4f; p.freq = 240; p.body = 0.12f; p.decay = 0.04f; }));
            v.push_back (mk ("Tumba", "Congas & Hand Drums", [] (Patch& p) { p.freq = 139; p.sweep = 1.05f; p.drop = 0.02f; p.decay = 0.2f; p.body = 0.25f; p.modes = 0.12f; p.real = real ("tumba"); p.realMix = 1.0f; p.realRoom = 0.5f; }));
            v.push_back (mk ("Bongo Low", "Congas & Hand Drums", [] (Patch& p) { p.real = real ("bongolo"); p.realMix = 1.0f; p.realRoom = 0.5f; p.freq = 263; p.body = 0.15f; p.decay = 0.07f; }));
            v.push_back (mk ("Djembe", "Congas & Hand Drums", [] (Patch& p) { p.freq = 92; p.sweep = 1.15f; p.drop = 0.02f; p.decay = 0.13f; p.body = 0.8f; p.modes = 0.4f; p.modeDecay = 0.35f; p.noise = 0.3f; p.noiseHz = 1800; p.noiseDecay = 0.025f; p.click = 0.3f; p.clickType = 3; p.real = real ("tumba"); p.realMix = 0.45f; p.realRoom = 0.5f; p.realTune = -3.0f; }));
            v.push_back (mk ("Timbale", "Congas & Hand Drums", [] (Patch& p) { p.freq = 430; p.sweep = 1.04f; p.drop = 0.01f; p.decay = 0.2f; p.body = 0.7f; p.modes = 0.5f; p.modeDecay = 0.5f; p.metal = 0.15f; p.metalScale = 0.8f; p.metalBp = 4500; p.metalHp = 2500; p.metalDecay = 0.25f; p.noise = 0.2f; p.noiseHz = 3000; p.noiseDecay = 0.02f; p.click = 0.4f; p.clickType = 3; p.room = 0.2f; }));
            v.push_back (mk ("Timbale Shell", "Congas & Hand Drums", [] (Patch& p) { p.wave = 1; p.freq = 1700; p.decay = 0.025f; p.body = 0.5f; p.noise = 0.5f; p.noiseHz = 4000; p.noiseQ = 2.0f; p.noiseDecay = 0.02f; p.click = 0.6f; p.clickType = 2; p.metal = 0.2f; p.metalDecay = 0.04f; p.real = real ("stickshot"); p.realMix = 0.4f; p.realRoom = 0.4f; p.realTune = 4.0f; }));
            // RIMS
            v.push_back (mk ("Stick Shot", "Rims & Clicks", [] (Patch& p) { p.real = real ("stickshot"); p.realMix = 1.0f; p.realRoom = 0.4f; }));
            v.push_back (mk ("Rim Crack", "Rims & Clicks", [] (Patch& p) { p.freq = 520; p.ratio2 = 1.7f; p.level2 = 0.5f; p.body = 0.35f; p.decay = 0.05f; p.click = 0.4f; p.clickType = 3; p.room = 0.15f; p.roomSize = 0.7f; p.real = real ("rimshot"); p.realMix = 0.9f; p.realRoom = 0.4f; }));
            v.push_back (mk ("Side Stick", "Rims & Clicks", [] (Patch& p) { p.freq = 1700; p.body = 0.2f; p.decay = 0.012f; p.real = real ("crossstick"); p.realMix = 1.0f; p.realRoom = 0.15f; }));
            v.push_back (mk ("Dub Rim", "Rims & Clicks", [] (Patch& p) { p.lpHz = 6000; p.room = 0.4f; p.roomSize = 1.0f; p.real = real ("crossstick"); p.realMix = 1.0f; p.realRoom = 0.8f; p.realTune = -2.0f; }));
            // BELLS
            v.push_back (mk ("Agogo High", "Bells & Metal", [] (Patch& p) { p.real = real ("agogohi"); p.realMix = 1.0f; p.realRoom = 0.4f; }));
            v.push_back (mk ("Agogo Low", "Bells & Metal", [] (Patch& p) { p.real = real ("agogolo"); p.realMix = 1.0f; p.realRoom = 0.4f; }));
            // SNARES for breaks and drum & bass
            v.push_back (mk ("DnB Snare", "Snares", [] (Patch& p) { p.freq = 225; p.sweep = 1.6f; p.drop = 0.01f; p.ratio2 = 1.65f; p.level2 = 0.3f; p.body = 0.6f; p.decay = 0.07f; p.noise = 0.75f; p.noiseHz = 3800; p.noiseQ = 0.5f; p.noiseDecay = 0.13f; p.drive = 0.35f; p.click = 0.3f; p.clickType = 3; p.hpHz = 140; p.room = 0.1f; p.roomSize = 0.6f; p.real = real ("snare"); p.realMix = 0.7f; p.realRoom = 0.35f; p.realTune = 1.0f; }));
            v.push_back (mk ("Break Snare", "Snares", [] (Patch& p) { p.freq = 200; p.sweep = 1.4f; p.drop = 0.012f; p.body = 0.5f; p.decay = 0.08f; p.noise = 0.6f; p.noiseHz = 2600; p.noiseQ = 0.6f; p.noiseDecay = 0.14f; p.drive = 0.3f; p.crush = 0.35f; p.lpHz = 9500; p.hpHz = 110; p.room = 0.2f; p.roomSize = 0.8f; p.real = real ("snare"); p.realMix = 0.85f; p.realRoom = 0.6f; p.realTune = 2.0f; }));
            // tighten: shorter snare/clap tails, and every room (real room mics + synth room) at half size
            for (auto& snd : v)
            {
                const juce::String cat (snd.cat);
                if (cat == "Snares" || cat == "Claps") { snd.p.noiseDecay *= 0.5f; snd.p.decay *= 0.7f; snd.p.gate = 0.2f; snd.p.gateFade = 0.035f; }
                snd.p.realRoom *= 0.45f;
                snd.p.room *= 0.5f;
                snd.p.roomSize = juce::jmin (snd.p.roomSize, 1.0f);
                snd.p.trim = soundTrim (snd.name);
            }
            return v;
        }();
        return lib;
    }

    inline int find (const char* name)
    {
        const auto& l = library();
        for (size_t i = 0; i < l.size(); ++i) if (juce::String (l[i].name) == name) return (int) i;
        jassertfalse; return 0;
    }

    // ---------------- KICK LAB presets
    struct KickPreset
    {
        const char* name;
        int wave; float freq, start, drop, curve, hold, decay, drive, click; int clickType; float clickTone;
        float sub, subDecay, knock, knockHz; int length; float duck, rumble;
        float real = 0.15f, roomMic = 0.3f;   // recorded kick underneath + its room mic
    };
    inline const std::vector<KickPreset>& kickPresets()
    {
        static const std::vector<KickPreset> k = {
            //  name                wave freq start drop   curve hold  decay drive click type tone  sub  subdec knock khz  len duck rumble
            { "Garage Thud",        0, 52, 3.6f, 0.026f, 0.2f, 0.015f, 0.10f, 0.25f, 0.35f, 1, 0.55f, 0.35f, 0.15f, 0.15f, 190, 0, 0.15f, 0.0f , 0.2f, 0.35f },
            { "808 Long",           0, 45, 2.0f, 0.04f,  0.15f,0.01f,  0.3f,  0.0f,  0.1f,  2, 0.5f,  0.7f,  0.8f,  0.0f,  180, 0, 0.2f,  0.0f , 0.0f, 0.15f },
            { "808 Short",          0, 48, 2.2f, 0.035f, 0.15f,0.01f,  0.12f, 0.1f,  0.15f, 2, 0.5f,  0.5f,  0.2f,  0.0f,  180, 0, 0.1f,  0.0f , 0.05f, 0.2f },
            { "909 Punch",          2, 54, 4.5f, 0.022f, 0.15f,0.02f,  0.16f, 0.4f,  0.55f, 0, 0.7f,  0.3f,  0.2f,  0.1f,  200, 0, 0.2f,  0.0f },
            { "909 Distorted",      2, 55, 5.0f, 0.02f,  0.1f, 0.03f,  0.2f,  0.85f, 0.5f,  0, 0.8f,  0.25f, 0.2f,  0.2f,  210, 0, 0.3f,  0.0f },
            { "Burial Muffle",      0, 50, 2.5f, 0.03f,  0.2f, 0.0f,   0.14f, 0.15f, 0.1f,  1, 0.15f, 0.4f,  0.18f, 0.0f,  180, 0, 0.1f,  0.1f , 0.3f, 0.6f },
            { "Dub Techno Rumble",  0, 48, 3.2f, 0.03f,  0.15f,0.02f,  0.18f, 0.3f,  0.3f,  0, 0.4f,  0.5f,  0.25f, 0.1f,  190, 0, 0.5f,  0.6f },
            { "Berlin Knock",       2, 50, 4.0f, 0.02f,  0.1f, 0.02f,  0.14f, 0.55f, 0.4f,  2, 0.6f,  0.35f, 0.25f, 0.7f,  230, 0, 0.35f, 0.35f },
            { "Minimal Tight",      0, 54, 3.4f, 0.016f, 0.05f,0.005f, 0.11f, 0.25f, 0.55f, 2, 0.85f, 0.3f,  0.12f, 0.25f, 240, 2, 0.1f,  0.0f , 0.15f, 0.15f },
            { "Break Kick",         0, 58, 2.6f, 0.018f, 0.05f,0.01f,  0.09f, 0.3f,  0.3f,  1, 0.45f, 0.3f,  0.1f,  0.35f, 160, 0, 0.0f,  0.0f , 0.7f, 0.55f },
            { "Lo-Fi Boom Bap",     0, 55, 2.2f, 0.03f,  0.15f,0.02f,  0.16f, 0.6f,  0.25f, 0, 0.3f,  0.3f,  0.2f,  0.0f,  180, 0, 0.1f,  0.0f , 0.4f, 0.5f },
            { "Deep House",         0, 50, 3.0f, 0.028f, 0.1f, 0.02f,  0.17f, 0.15f, 0.25f, 1, 0.4f,  0.5f,  0.22f, 0.0f,  180, 0, 0.25f, 0.0f , 0.15f, 0.3f },
            { "Hard Techno",        2, 52, 6.0f, 0.025f, 0.1f, 0.04f,  0.25f, 1.0f,  0.6f,  0, 0.9f,  0.35f, 0.3f,  0.4f,  220, 3, 0.6f,  0.5f , 0.1f, 0.15f },
            { "DnB Punch",          2, 56, 4.0f, 0.016f, 0.05f,0.008f, 0.10f, 0.45f, 0.6f,  1, 0.75f, 0.25f, 0.12f, 0.35f, 210, 2, 0.2f,  0.0f , 0.3f, 0.2f },
            { "Jungle Boom",        0, 47, 2.6f, 0.03f,  0.1f, 0.01f,  0.24f, 0.15f, 0.3f,  0, 0.6f,  0.7f,  0.32f, 0.1f,  180, 0, 0.25f, 0.0f , 0.25f, 0.3f },
            { "Acoustic Kick",      0, 60, 2.0f, 0.02f,  0.05f,0.0f,   0.06f, 0.0f,  0.0f,  1, 0.4f,  0.35f, 0.12f, 0.0f,  180, 0, 0.1f,  0.0f , 1.0f, 0.5f },
        };
        return k;
    }

    // ---------------- KITS: a kick preset + a sound on each of the other seven pads
    struct Kit { const char* name; const char* kick; const char* pads[NumPads - 1]; };   // unlisted pads keep their sound
    inline const std::vector<Kit>& kits()
    {
        static const std::vector<Kit> k = {
            { "2-Step Classic", "Garage Thud",       { "Garage Snare", "808 Clap", "808 Rim", "808 Hat", "Garage Open Hat", "Perc Thud", "Shaker" } },
            { "808 Garage",     "808 Short",         { "808 Snare", "808 Clap", "808 Rim", "808 Hat", "808 Open Hat", "808 Conga", "Maracas" } },
            { "Burial Dust",    "Burial Muffle",     { "Burial Snare", "Room Clap", "Dusty Rim", "Burial Hat", "Dusty Open Hat", "Foley Knock", "Rattle" } },
            { "Untrue Foley",   "Burial Muffle",     { "Rainy Snare", "Lighter Flick", "Foley Knock", "Shuffle Tick", "Dusty Open Hat", "Keys Jangle", "Rattle" } },
            { "909 Techno",     "909 Punch",         { "909 Snare", "909 Clap", "909 Rim", "909 Hat", "909 Open Hat", "909 Tom", "Cabasa" } },
            { "Dub Techno",     "Dub Techno Rumble", { "Noise Snare", "Tight Clap", "Click", "Tick Hat", "Ride", "Clank", "Shaker" } },
            { "Funk Breaks",    "Break Kick",        { "Rimshot Snare", "Finger Snap", "Woodblock", "Dusty Hat", "909 Open Hat", "Bongo", "Tambourine" } },
            { "Minimal Clicks", "Minimal Tight",     { "Rimshot Snare", "Finger Snap", "Click", "Tick Hat", "Garage Open Hat", "FM Bell", "Cabasa" } },
            { "Lo-Fi Boom Bap", "Lo-Fi Boom Bap",    { "Brush", "808 Clap", "Stick", "Dusty Hat", "808 Open Hat", "Low Conga", "Egg Shaker" } },
            { "Electro Zap",    "808 Long",          { "808 Snare", "909 Clap", "Blip", "FM Hat", "808 Open Hat", "Laser Zap", "Agogo" } },
            { "Real Room Breaks", "Acoustic Kick",   { "Real Snare", "Real Rimshot", "Real Cross Stick", "Real Closed Hat", "Real Half Hat", "Conga Open", "Real Shaker", "Studio Tom" } },
            { "Drum & Bass",    "DnB Punch",         { "DnB Snare", "Tight Clap", "Side Stick", "Real Closed Hat", "Garage Open Hat", "Conga Mute", "Shaker", "Studio Tom", "Crash" } },
            { "Jungle",         "Jungle Boom",       { "Break Snare", "Real Rimshot", "Stick Shot", "Dusty Hat", "Real Open Hat", "Bongo Low", "Tambourine", "Studio Tom", "Crash" } },
            { "Hard Warehouse", "Hard Techno",       { "909 Snare", "Big Room Clap", "909 Rim", "909 Hat", "Ride", "Clank", "Tambourine" } },
        };
        return k;
    }
}
