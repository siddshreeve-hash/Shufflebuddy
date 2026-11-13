#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ScaledEditor.h"
#include "SBSamples.h"

using APF = juce::AudioParameterFloat;
using NR  = juce::NormalisableRange<float>;

namespace
{
    juce::AudioParameterFloatAttributes fmt (std::function<juce::String (float)> f)
    {
        return juce::AudioParameterFloatAttributes().withStringFromValueFunction ([f] (float v, int) { return f (v); });
    }
    juce::String pct (float v) { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; }
    float softClip (float x) { const float a = std::abs (x); return a <= 0.8f ? x : std::copysign (0.8f + 0.2f * std::tanh ((a - 0.8f) / 0.2f), x); }
}

juce::AudioProcessorValueTreeState::ParameterLayout ShuffleBuddyProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> v;
    auto add = [&] (auto* prm) { v.emplace_back (prm); };
    add (new APF ({ "tempo", 1 }, "Tempo", NR (60.0f, 180.0f, 1.0f), 132.0f, fmt ([] (float x) { return juce::String (juce::roundToInt (x)); })));
    add (new APF ({ "swing", 1 }, "Swing", NR (50.0f, 75.0f, 0.5f), 58.0f, fmt ([] (float x) { return juce::String (x, 0) + "%"; })));
    add (new APF ({ "ghosts", 1 }, "Ghosts", NR (0.0f, 1.0f), 0.4f, fmt (pct)));
    add (new APF ({ "loosen", 1 }, "Loosen", NR (0.0f, 1.0f), 0.15f, fmt (pct)));
    add (new APF ({ "variety", 1 }, "Variety", NR (0.0f, 1.0f), 0.3f, fmt (pct)));
    add (new APF ({ "dust", 1 }, "Dust", NR (0.0f, 1.0f), 0.1f, fmt (pct)));
    add (new APF ({ "master", 1 }, "Volume", NR (-24.0f, 6.0f), -3.0f, fmt ([] (float x) { return juce::String (x, 1) + " dB"; })));
    juce::StringArray genreNames;
    for (int i = 0; i < groove::NumStyles; ++i) genreNames.add (groove::styleName (i));
    add (new juce::AudioParameterChoice ({ "style", 1 }, "Genre", genreNames, 0));
    add (new juce::AudioParameterChoice ({ "slot", 1 }, "Pattern", juce::StringArray { "A", "B", "C", "D" }, 0));
    add (new juce::AudioParameterBool ({ "play", 1 }, "Play", false));
    add (new juce::AudioParameterBool ({ "fill", 1 }, "Fill", false));
    add (new juce::AudioParameterBool ({ "throw", 1 }, "Throw", false));
    add (new juce::AudioParameterChoice ({ "chain", 1 }, "Chain", juce::StringArray { "Off", "A-B", "A-B-C-D", "A-A-A-B" }, 0));
    add (new APF ({ "echo", 1 }, "Echo", NR (0.0f, 1.0f), 0.15f, fmt (pct)));
    add (new APF ({ "space", 1 }, "Space", NR (0.0f, 1.0f), 0.12f, fmt (pct)));
    add (new juce::AudioParameterChoice ({ "echotime", 1 }, "Echo Time", juce::StringArray { "1/16", "1/8", "1/8 dot", "1/4" }, 2));
    add (new juce::AudioParameterChoice ({ "echotype", 1 }, "Echo Type", juce::StringArray { "Tape", "Space Echo", "BBD" }, 0));
    add (new APF ({ "fxfilter", 1 }, "Filter", NR (-1.0f, 1.0f), 0.0f,
                  fmt ([] (float x) { return std::abs (x) < 0.03f ? juce::String ("OFF") : (x < 0.0f ? "LOW " : "HIGH ") + juce::String (juce::roundToInt (std::abs (x) * 100)) + "%"; })));
    add (new APF ({ "squash", 1 }, "Squash", NR (0.0f, 1.0f), 0.25f, fmt (pct)));
    // ---- KICK LAB (defaults = "Garage Thud")
    {
        auto skew = [] (float lo, float hi, float centre) { NR r (lo, hi); r.setSkewForCentre (centre); return r; };
        auto ms = [] (float x) { return x < 1.0f ? juce::String (juce::roundToInt (x * 1000.0f)) + " ms" : juce::String (x, 2) + " s"; };
        add (new juce::AudioParameterChoice ({ "kwave", 1 }, "Kick Wave", juce::StringArray { "Sine", "Triangle", "Round" }, 0));
        add (new APF ({ "kfreq", 1 }, "Kick Pitch", skew (30.0f, 120.0f, 55.0f), 52.0f, fmt ([] (float x) { return juce::String (juce::roundToInt (x)) + " Hz"; })));
        add (new juce::AudioParameterChoice ({ "kkey", 1 }, "Kick Key", juce::StringArray { "Free", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0));
        add (new APF ({ "kstart", 1 }, "Kick Sweep", skew (1.0f, 10.0f, 3.5f), 3.6f, fmt ([] (float x) { return "x" + juce::String (x, 1); })));
        add (new APF ({ "kpdec", 1 }, "Kick Drop", skew (0.005f, 0.25f, 0.035f), 0.026f, fmt (ms)));
        add (new APF ({ "kcurve", 1 }, "Kick Curve", NR (0.0f, 1.0f), 0.2f, fmt (pct)));
        add (new APF ({ "khold", 1 }, "Kick Hold", skew (0.0f, 0.15f, 0.03f), 0.015f, fmt (ms)));
        add (new APF ({ "kdecay", 1 }, "Kick Decay", skew (0.03f, 1.5f, 0.2f), 0.10f, fmt (ms)));
        add (new APF ({ "kdrive", 1 }, "Kick Drive", NR (0.0f, 1.0f), 0.25f, fmt (pct)));
        add (new APF ({ "kclick", 1 }, "Kick Click", NR (0.0f, 1.0f), 0.35f, fmt (pct)));
        add (new juce::AudioParameterChoice ({ "kctype", 1 }, "Click Type", juce::StringArray { "Noise", "Beater", "Tick", "Snap" }, 1));
        add (new APF ({ "kctone", 1 }, "Click Tone", NR (0.0f, 1.0f), 0.55f, fmt (pct)));
        add (new APF ({ "ksub", 1 }, "Kick Sub", NR (0.0f, 1.0f), 0.35f, fmt (pct)));
        add (new APF ({ "ksubdec", 1 }, "Sub Decay", skew (0.05f, 1.5f, 0.3f), 0.15f, fmt (ms)));
        add (new APF ({ "kknock", 1 }, "Kick Knock", NR (0.0f, 1.0f), 0.15f, fmt (pct)));
        add (new APF ({ "kknockf", 1 }, "Knock Pitch", skew (80.0f, 500.0f, 200.0f), 190.0f, fmt ([] (float x) { return juce::String (juce::roundToInt (x)) + " Hz"; })));
        add (new juce::AudioParameterChoice ({ "klen", 1 }, "Kick Length", juce::StringArray { "Free", "1/16", "1/8", "1/4", "1/2", "Bar" }, 0));
        add (new APF ({ "kduck", 1 }, "Kick Duck", NR (0.0f, 1.0f), 0.15f, fmt (pct)));
        add (new APF ({ "krumble", 1 }, "Kick Rumble", NR (0.0f, 1.0f), 0.0f, fmt (pct)));
        add (new APF ({ "kreal", 1 }, "Kick Real Layer", NR (0.0f, 1.0f), 0.2f, fmt (pct)));
        add (new APF ({ "kroom", 1 }, "Kick Room Mic", NR (0.0f, 1.0f), 0.35f, fmt (pct)));
        add (new APF ({ "kgain", 1 }, "Kick Output", NR (0.25f, 2.0f), 1.0f, fmt (pct)));   // presets set it so they all sit at the same loudness
    }
    // ---- FILLS + MIDI
    add (new juce::AudioParameterChoice ({ "filltype", 1 }, "Fill Type", juce::StringArray { "Auto", "Toms", "Snare Roll", "Kick Doubles", "Hat Lift", "Drop-out" }, 0));
    add (new juce::AudioParameterChoice ({ "filllen", 1 }, "Fill Length", juce::StringArray { "1 Beat", "2 Beats", "1 Bar" }, 1));
    add (new juce::AudioParameterChoice ({ "autofill", 1 }, "Auto Fill", juce::StringArray { "Off", "Every 4", "Every 8", "Every 16" }, 0));
    add (new juce::AudioParameterBool ({ "fillcrash", 1 }, "Crash After Fill", true));
    add (new juce::AudioParameterBool ({ "midiout", 1 }, "MIDI Out", true));
    add (new APF ({ "feel", 1 }, "Feel", NR (0.0f, 1.0f), 0.5f, fmt (pct)));
    add (new juce::AudioParameterBool ({ "sound", 1 }, "Kit Sound", true));
    add (new APF ({ "drift", 1 }, "Drift", NR (0.0f, 1.0f), 0.3f, fmt (pct)));
    add (new juce::AudioParameterChoice ({ "notemap", 1 }, "Note Map", juce::StringArray { "Live Kits", "Push Pads", "Custom" }, 0));
    juce::StringArray soundNames;
    for (auto& snd : sounds::library()) soundNames.add (snd.name);
    static const char* padDefault[NumPads] = { "", "Garage Snare", "808 Clap", "808 Rim", "808 Hat", "Garage Open Hat", "Perc Thud", "Shaker", "Studio Tom", "Crash" };
    static const int noteDefault[NumPads] = { 36, 38, 39, 37, 42, 46, 47, 44, 45, 49 };   // Ableton Drum Rack layout (toms: 41/45/48/50 by pitch)
    for (int i = 0; i < NumPads; ++i)
    {
        const juce::String n = juce::String (padName (i)).toLowerCase();
        if (i > 0) add (new juce::AudioParameterChoice ({ pid ("snd", i), 1 }, n + " sound", soundNames, sounds::find (padDefault[i])));
        add (new juce::AudioParameterBool ({ pid ("layer", i), 1 }, n + " layer WAV + synth", false));
        add (new APF ({ pid ("real", i), 1 }, n + " real layer", NR (0.0f, 2.0f), 1.0f, fmt (pct)));
        add (new juce::AudioParameterBool ({ pid ("mute", i), 1 }, n + " mute", false));
        add (new juce::AudioParameterBool ({ pid ("solo", i), 1 }, n + " solo", false));
        add (new APF ({ pid ("lswing", i), 1 }, n + " swing", NR (0.0f, 1.5f, 0.05f), 1.0f, fmt (pct)));
        add (new juce::AudioParameterInt ({ pid ("note", i), 1 }, n + " MIDI note", 0, 127, noteDefault[i]));
        add (new APF ({ pid ("room", i), 1 }, n + " room", NR (0.0f, 2.0f), 1.0f, fmt (pct)));
        // mix balance: every sound is loudness-matched (SoundTrims.h), so the pad levels alone set the mix:
        // kick on top, snare/clap ~15 dB under it by loudness, hats and percussion sitting in, toms up for fills
        add (new APF ({ pid ("lvl", i), 1 }, n + " level", NR (0.0f, 1.0f), sounds::defaultLevel (i), fmt (pct)));
        add (new APF ({ pid ("tune", i), 1 }, n + " tune", NR (-12.0f, 12.0f, 0.1f), 0.0f, fmt ([] (float x) { return (x > 0 ? "+" : "") + juce::String (x, 1); })));
        add (new APF ({ pid ("decay", i), 1 }, n + " decay", NR (0.0f, 1.0f), 0.45f, fmt (pct)));
        add (new APF ({ pid ("push", i), 1 }, n + " push", NR (-20.0f, 30.0f, 0.5f), 0.0f,
                      fmt ([] (float x) { return (x > 0 ? "+" : "") + juce::String (juce::roundToInt (x)) + "ms"; })));
        static const float eDef[NumPads] = { 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.05f, 0.25f, 0.0f, 0.1f, 0.0f };
        static const float sDef[NumPads] = { 0.0f, 0.05f, 0.06f, 0.12f, 0.04f, 0.08f, 0.12f, 0.04f, 0.1f, 0.15f };
        add (new APF ({ pid ("esend", i), 1 }, n + " echo send", NR (0.0f, 1.0f), eDef[i], fmt (pct)));
        add (new APF ({ pid ("ssend", i), 1 }, n + " space send", NR (0.0f, 1.0f), sDef[i], fmt (pct)));
    }
    return { v.begin(), v.end() };
}

ShuffleBuddyProcessor::ShuffleBuddyProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    formats.registerBasicFormats();
    loadRealSets();
    for (auto& s : liveSample) s.store (nullptr);
    for (auto& a : pendingAudition) a.store (-1.0f);
    for (auto& h : hitCount) h.store (0);
    for (auto& h : hitVel) h.store (0.0f);
    buildSlots();
    for (int i = 0; i < NumPads; ++i)
    {
        padLevel[(size_t) i] = apvts.getRawParameterValue (pid ("lvl", i));
        padTune[(size_t) i]  = apvts.getRawParameterValue (pid ("tune", i));
        padDecay[(size_t) i] = apvts.getRawParameterValue (pid ("decay", i));
        padPush[(size_t) i]  = apvts.getRawParameterValue (pid ("push", i));
        padEcho[(size_t) i]  = apvts.getRawParameterValue (pid ("esend", i));
        padSpace[(size_t) i] = apvts.getRawParameterValue (pid ("ssend", i));
        padLayer[(size_t) i] = apvts.getRawParameterValue (pid ("layer", i));
        padSnd[(size_t) i]   = i > 0 ? apvts.getRawParameterValue (pid ("snd", i)) : nullptr;
        padReal[(size_t) i]  = apvts.getRawParameterValue (pid ("real", i));
        padRoom[(size_t) i]  = apvts.getRawParameterValue (pid ("room", i));
        padMute[(size_t) i]  = apvts.getRawParameterValue (pid ("mute", i));
        padSolo[(size_t) i]  = apvts.getRawParameterValue (pid ("solo", i));
        padSwing[(size_t) i] = apvts.getRawParameterValue (pid ("lswing", i));
        padNote[(size_t) i]  = apvts.getRawParameterValue (pid ("note", i));
    }
    for (auto& c : ccMap) c.store (-1);
    for (auto& r : played) for (auto& c : r) c.store (0);
    engineSeed = (juce::uint32) juce::Random::getSystemRandom().nextInt();
    for (auto& r : playedSrc) for (auto& c : r) c.store (0);
    // starter patterns: one per style, so it grooves the moment you press play
    // start on the first genre's four bars, so it grooves the moment you press play
    for (int sl = 0; sl < groove::numSlots; ++sl) setGrid (sl, groove::genrePattern (0, sl));
    applyStyleFeel (0);
    startTimer (1000);
}

ShuffleBuddyProcessor::~ShuffleBuddyProcessor() { stopTimer(); }

void ShuffleBuddyProcessor::buildSlots()
{
    for (auto* ap : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (ap))
        {
            const auto h = fnv (rp->getParameterID().toRawUTF8());
            size_t i = h & 255u;
            while (slots[i].h != 0) { jassert (slots[i].h != h); i = (i + 1) & 255u; }
            slots[i] = { h, apvts.getRawParameterValue (rp->getParameterID()) };
        }
}

groove::Grid ShuffleBuddyProcessor::grid (int slot) const
{
    groove::Grid g {};
    for (int pd = 0; pd < NumPads; ++pd) for (int s = 0; s < groove::numSteps; ++s) g[(size_t) pd][(size_t) s] = cellData (slot, pd, s);
    return g;
}

void ShuffleBuddyProcessor::setGrid (int slot, const groove::Grid& g)
{
    for (int pd = 0; pd < NumPads; ++pd) for (int s = 0; s < groove::numSteps; ++s) setCellData (slot, pd, s, g[(size_t) pd][(size_t) s]);
}

void ShuffleBuddyProcessor::loadRealSets()
{
    // decode the bundled recorded hits once, at startup (never on the audio thread)
    const auto& names = sounds::realNames();
    realSets.clear();
    realSets.resize ((size_t) names.size());
    for (int i = 0; i < names.size(); ++i) realSets[(size_t) i].name = names[i];
    juce::FlacAudioFormat flac;
    for (int r = 0; r < SBSamples::namedResourceListSize; ++r)
    {
        const juce::String file (SBSamples::originalFilenames[r]);
        const int setIdx = names.indexOf (file.upToLastOccurrenceOf ("_v", false, false));
        if (setIdx < 0) continue;
        int size = 0;
        const char* data = SBSamples::getNamedResource (SBSamples::namedResourceList[r], size);
        std::unique_ptr<juce::AudioFormatReader> rd (flac.createReaderFor (new juce::MemoryInputStream (data, (size_t) size, false), true));
        if (rd == nullptr) continue;
        RealClip c;
        c.vel = file.fromLastOccurrenceOf ("_v", false, false).getIntValue();
        c.buf.setSize ((int) rd->numChannels, (int) rd->lengthInSamples);
        rd->read (&c.buf, 0, (int) rd->lengthInSamples, 0, true, true);
        realSets[(size_t) setIdx].clips.push_back (std::move (c));
    }
}

const RealSet* ShuffleBuddyProcessor::realSet (int index) const
{
    return index >= 0 && index < (int) realSets.size() && ! realSets[(size_t) index].clips.empty() ? &realSets[(size_t) index] : nullptr;
}

Patch ShuffleBuddyProcessor::kickPatch (double bpm) const
{
    Patch k;
    k.wave = (int) p ("kwave");
    const int key = (int) p ("kkey");
    k.freq = key == 0 ? p ("kfreq") : 32.703f * std::exp2 ((float) (key - 1) / 12.0f);   // KEY LOCK: C1..B1
    if (key != 0 && k.freq < 38.0f) k.freq *= 2.0f;                                      // keep it audible above ~38 Hz
    k.sweep = p ("kstart"); k.drop = p ("kpdec"); k.curve = p ("kcurve");
    k.hold = p ("khold"); k.decay = p ("kdecay"); k.body = 1.0f;
    k.drive = p ("kdrive");
    k.click = p ("kclick"); k.clickType = (int) p ("kctype"); k.clickTone = p ("kctone");
    k.sub = p ("ksub"); k.subDecay = p ("ksubdec");
    k.knock = p ("kknock"); k.knockHz = p ("kknockf"); k.knockDecay = 0.035f;
    static const float steps[] = { 0.0f, 1.0f, 2.0f, 4.0f, 8.0f, 16.0f };
    const int len = juce::jlimit (0, 5, (int) p ("klen"));
    // LENGTH locked to the grid; on Free the tail is still trimmed where it stops being useful (~-22 dB)
    k.gate = len == 0 ? juce::jmax (k.hold + 2.5f * k.decay, k.sub > 0.0f ? 2.5f * k.subDecay : 0.0f) + 0.01f
                      : (float) (60.0 / bpm / 4.0) * steps[len] * 0.92f;
    k.gateFade = 0.03f;
    k.velPitch = 0.5f;
    k.real = sounds::real ("kick"); k.realMix = p ("kreal"); k.realRoom = p ("kroom");
    k.gain = 0.9f / (1.0f + 0.35f * k.sub);
    k.trim = p ("kgain");
    return k;
}

void ShuffleBuddyProcessor::setParamValue (const juce::String& id, float value)
{
    if (auto* prm = apvts.getParameter (id)) { prm->beginChangeGesture(); prm->setValueNotifyingHost (prm->convertTo0to1 (value)); prm->endChangeGesture(); }
}

void ShuffleBuddyProcessor::applyKickPreset (int index)
{
    const auto& all = sounds::kickPresets();
    if (index < 0 || index >= (int) all.size()) return;
    const auto& k = all[(size_t) index];
    setParamValue ("kwave", (float) k.wave);   setParamValue ("kfreq", k.freq);   setParamValue ("kkey", 0.0f);
    setParamValue ("kstart", k.start);         setParamValue ("kpdec", k.drop);   setParamValue ("kcurve", k.curve);
    setParamValue ("khold", k.hold);           setParamValue ("kdecay", k.decay); setParamValue ("kdrive", k.drive);
    setParamValue ("kclick", k.click);         setParamValue ("kctype", (float) k.clickType); setParamValue ("kctone", k.clickTone);
    setParamValue ("ksub", k.sub);             setParamValue ("ksubdec", k.subDecay);
    setParamValue ("kknock", k.knock);         setParamValue ("kknockf", k.knockHz);
    setParamValue ("klen", (float) k.length);  setParamValue ("kduck", k.duck);   setParamValue ("krumble", k.rumble);
    setParamValue ("kreal", k.real);           setParamValue ("kroom", k.roomMic);
    setParamValue ("kgain", sounds::kickTrim (k.name));
    setParamValue ("tune1", 0.0f);             setParamValue ("decay1", 0.45f);
    kickName = k.name;
}

void ShuffleBuddyProcessor::applyKit (int index)
{
    const auto& all = sounds::kits();
    if (index < 0 || index >= (int) all.size()) return;
    const auto& kit = all[(size_t) index];
    const auto& kp = sounds::kickPresets();
    for (size_t i = 0; i < kp.size(); ++i) if (juce::String (kp[i].name) == kit.kick) applyKickPreset ((int) i);
    for (int pd = 1; pd < NumPads; ++pd)
    {
        if (kit.pads[pd - 1] == nullptr) continue;
        setParamValue (pid ("snd", pd), (float) sounds::find (kit.pads[pd - 1]));
        setParamValue (pid ("tune", pd), 0.0f);
        setParamValue (pid ("decay", pd), 0.45f);
        setParamValue (pid ("real", pd), 1.0f);
        setParamValue (pid ("room", pd), 1.0f);
    }
    kitName = kit.name;
}

void ShuffleBuddyProcessor::surpriseKick()
{
    // musical dice: every roll is a usable kick, never a mess
    juce::Random& r = uiRng;
    auto rr = [&r] (float lo, float hi) { return lo + (hi - lo) * r.nextFloat(); };
    setParamValue ("kwave", (float) r.nextInt (3));
    setParamValue ("kfreq", rr (42.0f, 66.0f));
    setParamValue ("kstart", rr (2.0f, 6.5f));
    setParamValue ("kpdec", rr (0.012f, 0.06f));
    setParamValue ("kcurve", r.nextFloat() < 0.8f ? rr (0.0f, 0.35f) : rr (0.6f, 1.0f));
    setParamValue ("khold", rr (0.0f, 0.04f));
    setParamValue ("kdecay", rr (0.07f, 0.3f));
    setParamValue ("kdrive", rr (0.0f, 0.8f));
    setParamValue ("kclick", rr (0.1f, 0.7f));
    setParamValue ("kctype", (float) r.nextInt (4));
    setParamValue ("kctone", rr (0.2f, 0.9f));
    setParamValue ("ksub", rr (0.0f, 0.7f));
    setParamValue ("ksubdec", rr (0.1f, 0.5f));
    setParamValue ("kknock", r.nextFloat() < 0.5f ? 0.0f : rr (0.1f, 0.7f));
    setParamValue ("kknockf", rr (140.0f, 280.0f));
    kickName = "Surprise #" + juce::String (r.nextInt (900) + 100);
}

void ShuffleBuddyProcessor::pushUndo (int slot)
{
    history.push_back ({ slot, grid (slot) });
    if (history.size() > 48) history.erase (history.begin());
}

bool ShuffleBuddyProcessor::undo()
{
    if (history.empty()) return false;
    auto [slot, g] = history.back();
    history.pop_back();
    setGrid (slot, g);
    auto* prm = apvts.getParameter ("slot");
    prm->setValueNotifyingHost (prm->convertTo0to1 ((float) slot));
    return true;
}

void ShuffleBuddyProcessor::copySlot (int from, int to)
{
    if (from == to) return;
    pushUndo (to);
    setGrid (to, grid (from));
}

void ShuffleBuddyProcessor::clearPattern()
{
    pushUndo (currentSlot());
    setGrid (currentSlot(), groove::Grid {});
}

void ShuffleBuddyProcessor::clearMapping (int paramIndex)
{
    for (auto& c : ccMap) if (c.load() == paramIndex) c.store (-1);
    ++mapVersion;
}

int ShuffleBuddyProcessor::ccFor (int paramIndex) const
{
    for (int cc = 0; cc < 128; ++cc) if (ccMap[(size_t) cc].load() == paramIndex) return cc;
    return -1;
}

void ShuffleBuddyProcessor::rollPattern()
{
    pushUndo (currentSlot());
    setGrid (currentSlot(), groove::roll ((int) apvts.getRawParameterValue ("style")->load(), uiRng));
}

void ShuffleBuddyProcessor::varyPattern()
{
    pushUndo (currentSlot());
    auto g = grid (currentSlot());
    groove::vary (g, (int) apvts.getRawParameterValue ("style")->load(), uiRng, 4);
    setGrid (currentSlot(), g);
}

void ShuffleBuddyProcessor::loadGenre (int style)
{
    // the genre's feel + its four hand-written bars into A-D (undoable), ready to CHAIN as a 4-bar phrase
    for (int sl = groove::numSlots - 1; sl >= 0; --sl) pushUndo (sl);
    applyStyleFeel (style);
    for (int v = 0; v < groove::numSlots; ++v) setGrid (v, groove::genrePattern (style, v));
    const int fl = groove::genre (style).fillLen;
    setParamValue ("filllen", fl <= 4 ? 0.0f : fl <= 8 ? 1.0f : 2.0f);
    setParamValue ("slot", 0.0f);
}

void ShuffleBuddyProcessor::applyStyleFeel (int style)
{
    auto set = [this] (const juce::String& id, float v) { auto* prm = apvts.getParameter (id); prm->setValueNotifyingHost (prm->convertTo0to1 (v)); };
    const auto& f = groove::feel (style);
    set ("style", (float) style);
    set ("tempo", f.tempo); set ("swing", f.swing); set ("ghosts", f.ghosts); set ("loosen", f.loosen); set ("dust", f.dust);
    for (int i = 0; i < NumPads; ++i) set (pid ("push", i), f.push[i]);
}

bool ShuffleBuddyProcessor::loadSample (int pad, const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
    if (reader == nullptr) return false;
    const auto len = (int) std::min<juce::int64> (reader->lengthInSamples, (juce::int64) (reader->sampleRate * 10.0));   // up to 10 s
    SampleData::Ptr s = new SampleData();
    s->buffer.setSize ((int) juce::jmin (2u, reader->numChannels), len);
    reader->read (&s->buffer, 0, len, 0, true, true);
    s->sampleRate = reader->sampleRate;
    s->path = file.getFullPathName();
    if (auto old = sampleSlots[(size_t) pad]) trash.push_back ({ old, juce::Time::getMillisecondCounter() });
    sampleSlots[(size_t) pad] = s;
    liveSample[(size_t) pad].store (s.get());
    return true;
}

void ShuffleBuddyProcessor::clearSample (int pad)
{
    liveSample[(size_t) pad].store (nullptr);
    if (auto old = sampleSlots[(size_t) pad]) trash.push_back ({ old, juce::Time::getMillisecondCounter() });
    sampleSlots[(size_t) pad] = nullptr;
}

void ShuffleBuddyProcessor::timerCallback()
{
    // a replaced sample may still be playing for a few seconds; free it after that
    const auto now = juce::Time::getMillisecondCounter();
    trash.erase (std::remove_if (trash.begin(), trash.end(), [now] (const Trash& t) { return now - t.at > 12000; }), trash.end());
}

bool ShuffleBuddyProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void ShuffleBuddyProcessor::prepareToPlay (double sampleRate, int)
{
    queued = 0; stepPos = 0.0; wasPlaying = false; nNoteOffs = 0;
    for (auto& pl : plans) pl.bar = -1;
    outMidi.ensureSize (8192);
    for (auto& vc : voices) vc = DrumVoice();
    eBuf.setSize (2, maxChunk); sBuf.setSize (2, maxChunk); kBuf.setSize (2, maxChunk); rBuf.setSize (2, maxChunk); rumbleTail = 0;
    rumbleVerb.prepare (sampleRate);
    rumbleVerb.setParameters (0.92f, 0.75f);
    rumbleLp1.reset(); rumbleLp2.reset();
    rumbleLp1.set (140.0f, 0.75f, (float) sampleRate); rumbleLp2.set (140.0f, 0.75f, (float) sampleRate);
    duckEnv = 0.0f; duckHold = 0;
    echo.prepare (sampleRate);
    space.prepare (sampleRate);
    fxFilter.prepare (sampleRate);
    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
    oversampler->initProcessing ((size_t) maxChunk);
    setLatencySamples ((int) oversampler->getLatencyInSamples());
    compEnv = 0.0f; compGain = 1.0f; throwAmt = throwTail = 0.0f;
}

void ShuffleBuddyProcessor::trigger (int pad, float vel, int alt)
{
    static const float tomSemis[4] = { -5.0f, 0.0f, 4.0f, 7.0f };   // floor, low, mid, high
    const float altTune = pad == Toms ? tomSemis[juce::jlimit (0, 3, alt)] : 0.0f;
    if (pad == Hat) for (auto& vc : voices) if (vc.active() && vc.pad() == OpenHat) vc.choke();   // closed hat chokes open
    // voice stealing: free voice, else this pad's oldest, else the oldest overall
    DrumVoice* pick = nullptr;
    int samePad = 0; DrumVoice* oldestSame = nullptr; DrumVoice* oldest = nullptr;
    for (auto& vc : voices)
    {
        if (! vc.active()) { if (pick == nullptr) pick = &vc; continue; }
        if (vc.pad() == pad) { ++samePad; if (oldestSame == nullptr || vc.startedAt() < oldestSame->startedAt()) oldestSame = &vc; }
        if (oldest == nullptr || vc.startedAt() < oldest->startedAt()) oldest = &vc;
    }
    if (samePad >= 3 && oldestSame != nullptr) pick = oldestSame;
    if (pick == nullptr) pick = oldest;
    const auto i = (size_t) pad;
    if (pad == Kick)   // one kick at a time: the new hit cuts the old tail, like a real machine
    {
        for (auto& vc : voices) if (vc.active() && vc.pad() == Kick) vc.choke();
        duckEnv = 1.0f; duckHold = (int) (0.03 * getSampleRate());
    }
    if (pad == Kick) kickNow = kickPatch (bpmNow);
    const Patch& patch = pad == Kick ? kickNow : sounds::library()[(size_t) juce::jlimit (0, (int) sounds::library().size() - 1, (int) padSnd[i]->load())].p;
    auto* smpl = liveSample[i].load();
    const RealSet* rs = realSet (patch.real);
    float realExtra = 0.0f; bool hiDrum = false;
    if (pad == Toms && patch.realHi >= 0 && alt >= 2) { rs = realSet (patch.realHi); realExtra = -patch.realHiSemis; hiDrum = true; }   // mid/high toms: the smaller drum
    pick->trigger (pad, vel, patch, padTune[i]->load() + altTune, padDecay[i]->load(), padLevel[i]->load(), smpl,
                   (float) getSampleRate(), sampleClock, (juce::uint32) rng.nextInt(), padEcho[i]->load(), padSpace[i]->load(),
                   rs, padReal[i]->load(), padRoom[i]->load(), realExtra, hiDrum);
    if (smpl != nullptr && padLayer[i]->load() > 0.5f)   // LAYER: the synth plays underneath the WAV
    {
        DrumVoice* second = nullptr;
        for (auto& vc : voices) if (! vc.active() && &vc != pick) { second = &vc; break; }
        if (second != nullptr)
            second->trigger (pad, vel, patch, padTune[i]->load() + altTune, padDecay[i]->load(), padLevel[i]->load(), nullptr,
                             (float) getSampleRate(), sampleClock, (juce::uint32) rng.nextInt(), padEcho[i]->load(), padSpace[i]->load(),
                             rs, padReal[i]->load(), padRoom[i]->load(), realExtra, hiDrum);
    }
    hitVel[i].store (vel);
    hitCount[i].fetch_add (1);
}

int ShuffleBuddyProcessor::noteFor (int lane, int alt) const
{
    if (lane == Toms)
    {
        if ((int) p ("notemap") == 1) return juce::jlimit (0, 127, (int) padNote[(size_t) Toms]->load() + juce::jlimit (0, 3, alt));   // Push: floor..high on 4 pads in a row
        static const int tn[4] = { 41, 45, 48, 50 }; return juce::jlimit (0, 127, tn[juce::jlimit (0, 3, alt)] + ((int) padNote[(size_t) Toms]->load() - 45));
    }
    return juce::jlimit (0, 127, (int) padNote[(size_t) lane]->load());
}

groove::EngineIn ShuffleBuddyProcessor::engineIn() const
{
    groove::EngineIn in;
    in.swing = (p ("swing") - 50.0f) / 50.0f;
    in.ghosts = p ("ghosts"); in.loosen = p ("loosen"); in.variety = p ("variety");
    in.style = (int) p ("style"); in.chain = (int) p ("chain"); in.slot = (int) p ("slot");
    static const int lens[3] = { 4, 8, 16 }, autos[4] = { 0, 4, 8, 16 };
    in.fillType = (int) p ("filltype"); in.fillLen = lens[juce::jlimit (0, 2, (int) p ("filllen"))];
    in.autoFill = autos[juce::jlimit (0, 3, (int) p ("autofill"))]; in.crash = p ("fillcrash") > 0.5f;
    bool anySolo = false;
    for (int i = 0; i < NumPads; ++i) anySolo = anySolo || padSolo[(size_t) i]->load() > 0.5f;
    for (int i = 0; i < NumPads; ++i)
    {
        in.laneSwing[(size_t) i] = padSwing[(size_t) i]->load();
        in.push[(size_t) i] = padPush[(size_t) i]->load();
        in.audible[(size_t) i] = anySolo ? padSolo[(size_t) i]->load() > 0.5f : padMute[(size_t) i]->load() < 0.5f;
    }
    in.feel = p ("feel"); in.drift = p ("drift");
    in.seed = engineSeed;
    return in;
}

void ShuffleBuddyProcessor::planBar (groove::BarPlan& plan, juce::int64 bar, bool fillNow, bool prevFilled, const groove::EngineIn& in)
{
    const int slot = groove::slotForBar (in, bar);
    groove::Grid pat;
    for (int pd = 0; pd < NumPads; ++pd) for (int s = 0; s < groove::numSteps; ++s) pat[(size_t) pd][(size_t) s] = cellData (slot, pd, s);
    groove::makeBar (plan, bar, pat, in, fillNow, prevFilled);
}

groove::BarPlan* ShuffleBuddyProcessor::planFor (juce::int64 bar)
{
    for (auto& pl : plans) if (pl.bar == bar) return &pl;
    return nullptr;
}

void ShuffleBuddyProcessor::publishBar (const groove::BarPlan& pl)
{
    for (int pd = 0; pd < NumPads; ++pd)
        for (int s = 0; s < groove::numSteps; ++s)
        {
            played[(size_t) pd][(size_t) s].store ((juce::uint8) groove::levelOfVel (pl.played[(size_t) pd][(size_t) s].vel));
            playedSrc[(size_t) pd][(size_t) s].store (pl.src[(size_t) pd][(size_t) s]);
        }
    playedSlotNow.store (pl.slot);
    playSlotNow.store (pl.slot);
}

// ---------------------------------------------------------------- MIDI: record + print
void ShuffleBuddyProcessor::pollRecording()
{
    if (! recording) return;
    const double ticksPer16 = 240.0;   // 960 PPQ
    recFifo.read (recFifo.getNumReady()).forEach ([this, ticksPer16] (int i)
    {
        const auto& h = recBuf[(size_t) i];
        const double t = juce::jmax (0.0, h.pos16 * ticksPer16);
        recSeq.addEvent (juce::MidiMessage::noteOn (1, h.note, juce::jlimit (0.01f, 1.0f, h.vel)), t);
        recSeq.addEvent (juce::MidiMessage::noteOff (1, h.note), t + 110.0);
        recLastPos = juce::jmax (recLastPos, h.pos16);
    });
    recBars = (int) std::ceil ((recLastPos + 0.5) / 16.0);
}

void ShuffleBuddyProcessor::stopRecording()
{
    recArmed.store (false);
    pollRecording();
    recording = false;
    recSeq.sort();
    recSeq.updateMatchedPairs();
    recBars = juce::jmax (recBars, 1);
}

juce::MidiMessageSequence ShuffleBuddyProcessor::printBars (int bars)
{
    // render N bars offline with the same engine (new dice each print): ghosts, variety, chance, fills, swing all baked in
    juce::MidiMessageSequence seq;
    PrintSpec sp; sp.bars = bars;
    bool prev = false;
    printInto (seq, sp, (juce::uint32) uiRng.nextInt(), 0, prev);
    seq.sort(); seq.updateMatchedPairs();
    return seq;
}

void ShuffleBuddyProcessor::printInto (juce::MidiMessageSequence& seq, const PrintSpec& sp, juce::uint32 seed, int barOffset, bool& prevFilled)
{
    auto in = engineIn();
    in.seed = seed;
    if (sp.chain >= 0) in.chain = sp.chain;
    if (sp.slot >= 0) in.slot = sp.slot;
    if (sp.autoFill >= 0) in.autoFill = sp.autoFill;
    if (sp.fillType >= 0) in.fillType = sp.fillType;
    if (sp.fillLen > 0) in.fillLen = sp.fillLen;
    in.variety = juce::jlimit (0.0f, 1.0f, in.variety * sp.varMul);
    in.ghosts = juce::jlimit (0.0f, 1.0f, in.ghosts * sp.ghostMul);
    for (int i = 0; i < NumPads; ++i) in.audible[(size_t) i] = in.audible[(size_t) i] && ((sp.laneMask >> i) & 1u) != 0;
    auto plan = std::make_unique<groove::BarPlan>();
    const double bpm = tempoNow(), ticksPer16 = 240.0, ticksPerMs = 960.0 * bpm / 60000.0;
    for (int b = 0; b < sp.bars; ++b)
    {
        in.energy = sp.bars > 1 ? sp.e0 + (sp.e1 - sp.e0) * (float) b / (float) (sp.bars - 1) : sp.e0;
        const bool fillNow = sp.fillLast && b == sp.bars - 1;
        const bool landed = prevFilled && in.crash;
        planBar (*plan, b, fillNow, prevFilled, in);
        prevFilled = plan->filled;
        const double barTicks = (double) (barOffset + b) * 16.0 * ticksPer16;
        if (b == 0 && sp.crashStart && ! landed && in.audible[(size_t) Crash])
        {
            seq.addEvent (juce::MidiMessage::noteOn (1, noteFor (Crash, 1), 0.85f * in.energy), barTicks);
            seq.addEvent (juce::MidiMessage::noteOff (1, noteFor (Crash, 1)), barTicks + 110.0);
        }
        for (int h = 0; h < plan->nHits; ++h)
        {
            const auto& hit = plan->hits[(size_t) h];
            const double t = juce::jmax (0.0, barTicks + hit.pos * ticksPer16 + hit.ms * ticksPerMs);
            const int note = noteFor (hit.lane, hit.alt);
            seq.addEvent (juce::MidiMessage::noteOn (1, note, juce::jlimit (0.01f, 1.0f, hit.vel)), t);
            seq.addEvent (juce::MidiMessage::noteOff (1, note), t + 110.0);
        }
    }
}

juce::Array<juce::File> ShuffleBuddyProcessor::printPack (const juce::File& parent)
{
    const double bpm = tempoNow();
    const juce::String genre = juce::String (groove::styleName ((int) p ("style"))).trim();
    auto dir = parent.getChildFile (juce::File::createLegalFileName (genre + " " + juce::String (juce::roundToInt (bpm)) + "bpm "
                                                                       + juce::Time::getCurrentTime().formatted ("%Y-%m-%d %H%M%S")));
    dir.createDirectory();
    juce::Array<juce::File> files;
    juce::uint32 seed = (juce::uint32) uiRng.nextInt();
    auto write = [&] (const juce::String& name, const juce::Array<PrintSpec>& parts)
    {
        juce::MidiMessageSequence seq; bool prev = false; int off = 0;
        for (auto& sp : parts) { printInto (seq, sp, seed += 7777u, off, prev); off += sp.bars; }
        seq.sort(); seq.updateMatchedPairs();
        auto f = dir.getChildFile (name + ".mid");
        if (writeMidiFile (seq, bpm, f)) files.add (f);
    };
    enum : juce::uint32 { K = 1u << Kick, S = 1u << Snare, C = 1u << Clap, R = 1u << Rim, H = 1u << Hat, O = 1u << OpenHat,
                          P = 1u << Perc, Sh = 1u << Shaker, T = 1u << Toms, Cr = 1u << Crash, All = 0x3ffu };
    // the genre's four bars, each as a 4-bar loop with the feel baked in (no fills: loop them)
    const char* letters[4] = { "A", "B", "C", "D" };
    for (int k = 0; k < 4; ++k) { PrintSpec sp; sp.bars = 4; sp.chain = 0; sp.slot = k; sp.autoFill = 0; write ("0" + juce::String (k + 1) + " Groove " + letters[k], { sp }); }
    // phrases: A B C D chained, fill on the last bar
    { PrintSpec sp; sp.bars = 8;  sp.chain = 2; sp.autoFill = 0; sp.fillLast = true; write ("05 Phrase 8 bars", { sp }); }
    { PrintSpec sp; sp.bars = 16; sp.chain = 2; sp.autoFill = 8; sp.fillLast = true; write ("06 Phrase 16 bars", { sp }); }
    // one-bar fills to drop at the end of any section
    const char* fills[5] = { "Toms", "Snare Roll", "Kicks", "Hats", "Drop" };
    for (int k = 0; k < 5; ++k) { PrintSpec sp; sp.bars = 1; sp.chain = 0; sp.slot = 3; sp.fillLast = true; sp.fillType = k + 1; write (juce::String (7 + k).paddedLeft ('0', 2) + " Fill " + fills[k], { sp }); }
    // a full arrangement: intro, groove, break, build, drop, outro
    {
        juce::Array<PrintSpec> a;
        PrintSpec intro;  intro.bars = 8;  intro.laneMask = K | H | O | P | Sh; intro.chain = 2; intro.autoFill = 0; intro.fillLast = true; intro.fillType = 4; intro.varMul = 0.6f; intro.e0 = 0.82f; intro.e1 = 0.95f; a.add (intro);
        PrintSpec groove; groove.bars = 16; groove.chain = 2; groove.autoFill = 8; groove.crashStart = true; a.add (groove);
        PrintSpec brk;    brk.bars = 8;    brk.laneMask = S | C | R | H | O | P | Sh | Cr; brk.chain = 2; brk.autoFill = 0; brk.fillLast = true; brk.fillType = 2; brk.ghostMul = 1.3f; brk.e0 = 0.85f; brk.e1 = 0.9f; a.add (brk);
        PrintSpec build;  build.bars = 8;  build.laneMask = K | S | H | P | Sh | T | Cr; build.chain = 2; build.autoFill = 4; build.fillType = 2; build.fillLast = true; build.fillLen = 16; build.e0 = 0.9f; build.e1 = 1.05f; a.add (build);
        PrintSpec drop;   drop.bars = 16;  drop.chain = 2; drop.autoFill = 8; drop.crashStart = true; drop.varMul = 1.3f; drop.e0 = 1.05f; drop.e1 = 1.05f; a.add (drop);
        PrintSpec outro;  outro.bars = 8;  outro.laneMask = K | R | H | P | Sh | Cr; outro.chain = 2; outro.autoFill = 0; outro.crashStart = true; outro.e0 = 1.0f; outro.e1 = 0.6f; a.add (outro);
        write ("12 Arrangement 64 bars", a);
    }
    return files;
}

void ShuffleBuddyProcessor::setPatternRecord (bool on)
{
    if (on && ! patRec.load()) for (int sl = 0; sl < 4; ++sl) pushUndo (sl);   // one CTRL+Z takes the whole take back
    patRec.store (on);
}

void ShuffleBuddyProcessor::recordHit (int pad, int alt, float vel, double pos)
{
    // snap to the nearest step, allowing for the swing (a swung 16th you played late belongs to its step)
    const auto in = engineIn();
    const double sw = in.swing * in.laneSwing[(size_t) pad];
    double best = 1.0e9; juce::int64 step = 0;
    for (auto cand = (juce::int64) std::floor (pos) - 1; cand <= (juce::int64) std::floor (pos) + 1; ++cand)
    {
        const double g = (double) cand + ((cand & 1) ? sw : 0.0);
        if (std::abs (pos - g) < std::abs (best)) { best = pos - g; step = cand; }
    }
    if (step < 0) step = 0;
    const int slot = groove::slotForBar (in, step / 16), st = (int) (step % 16);
    auto c = cellData (slot, pad, st);
    const auto v = (juce::uint8) juce::jlimit (1, 127, juce::roundToInt (vel * 127.0f));
    c.vel = c.on() ? juce::jmax (c.vel, v) : v;
    c.prob = 100; c.ratchet = 1;
    c.nudge = recKeepFeel.load() ? (juce::int8) juce::jlimit (-50, 50, juce::roundToInt (best * 100.0)) : (juce::int8) 0;
    if (pad == Toms) c.alt = (juce::uint8) juce::jlimit (0, 3, alt);
    setCellData (slot, pad, st, c);
    ++patVersion;
}

int ShuffleBuddyProcessor::laneForNote (int note, int& alt) const
{
    alt = 1;
    for (int i = 0; i < NumPads; ++i)   // the current note map first
    {
        if (i == Toms) { for (int a = 0; a < 4; ++a) if (noteFor (Toms, a) == note) { alt = a; return Toms; } }
        else if (noteFor (i, 1) == note) return i;
    }
    switch (note)   // then General MIDI drums, so clips from anywhere land somewhere sensible
    {
        case 35: case 36: return Kick;
        case 38: case 40: return Snare;
        case 39: return Clap;
        case 37: case 75: case 76: case 77: return Rim;
        case 42: case 44: return Hat;
        case 46: return OpenHat;
        case 49: case 52: case 55: case 57: return Crash;
        case 51: case 53: case 59: case 56: case 60: case 61: case 62: case 63: case 64: case 65: case 66: case 67: case 68: return Perc;
        case 54: case 58: case 69: case 70: case 82: return Shaker;
        case 41: case 43: alt = 0; return Toms;
        case 45: case 47: alt = 1; return Toms;
        case 48: alt = 2; return Toms;
        case 50: alt = 3; return Toms;
        default: break;
    }
    if (note >= 60 && note < 60 + NumPads) return note - 60;
    return -1;
}

int ShuffleBuddyProcessor::importMidiFile (const juce::File& file, int firstSlot, juce::String& info)
{
    juce::MidiFile mf;
    { juce::FileInputStream is (file); if (! is.openedOk() || ! mf.readFrom (is)) { info = "NOT A MIDI FILE"; return 0; } }
    const double ppq = mf.getTimeFormat() > 0 ? (double) mf.getTimeFormat() : 960.0;
    struct N { double pos; int lane, alt; float vel; };
    std::vector<N> notes; int skipped = 0;
    for (int t = 0; t < mf.getNumTracks(); ++t)
        for (auto* e : *mf.getTrack (t))
            if (e->message.isNoteOn())
            {
                int alt = 1; const int lane = laneForNote (e->message.getNoteNumber(), alt);
                if (lane < 0) { ++skipped; continue; }
                notes.push_back ({ e->message.getTimeStamp() / ppq * 4.0, lane, alt, e->message.getFloatVelocity() });
            }
    if (notes.empty()) { info = "NO DRUM NOTES FOUND"; return 0; }
    double first = 1.0e9, last = 0; for (auto& n : notes) { first = juce::jmin (first, n.pos); last = juce::jmax (last, n.pos); }
    const double start = std::floor (first / 16.0) * 16.0;   // clips that start late: from the bar the first note is in
    const int bars = juce::jlimit (1, 4 - firstSlot, (int) std::floor ((last - start) / 16.0) + 1);
    // swing: how late the off-16ths sit on average
    double swSum = 0; int swN = 0;
    for (auto& n : notes) { const double r = n.pos - start; const auto st = (juce::int64) std::round (r); if ((st & 1) && std::abs (r - (double) st) < 0.45) { swSum += r - (double) st; ++swN; } }
    const double swingSteps = swN >= 4 && swSum / swN > 0.03 ? juce::jlimit (0.0, 0.5, swSum / swN) : 0.0;
    for (int b = 0; b < bars; ++b) { pushUndo (firstSlot + b); for (int pd = 0; pd < NumPads; ++pd) for (int s = 0; s < 16; ++s) setCellData (firstSlot + b, pd, s, {}); }
    // count hits per cell first: several in one step become a ratchet
    std::array<std::array<std::array<int, 16>, NumPads>, 4> count {};
    int placed = 0, cut = 0;
    for (auto& n : notes)
    {
        double r = n.pos - start;
        auto st = (juce::int64) std::round (r - ((((juce::int64) std::round (r)) & 1) ? swingSteps : 0.0));
        double off = r - (double) st - ((st & 1) ? swingSteps : 0.0);
        const int bar = (int) (st / 16);
        if (bar >= bars) { ++cut; continue; }
        const int slot = firstSlot + bar, s = (int) (st % 16);
        auto c = cellData (slot, n.lane, s);
        auto& k = count[(size_t) bar][(size_t) n.lane][(size_t) s];
        const auto v = (juce::uint8) juce::jlimit (1, 127, juce::roundToInt (n.vel * 127.0f));
        if (k == 0) { c.vel = v; c.nudge = (juce::int8) juce::jlimit (-50, 50, juce::roundToInt (off * 100.0)); if (n.lane == Toms) c.alt = (juce::uint8) n.alt; ++placed; }
        else c.vel = juce::jmax (c.vel, v);
        c.prob = 100; c.ratchet = (juce::uint8) juce::jlimit (1, 4, k + 1);
        ++k;
        setCellData (slot, n.lane, s, c);
    }
    if (swingSteps > 0.0) setParamValue ("swing", (float) (50.0 + swingSteps * 50.0));
    if (bars >= 3 && firstSlot == 0) setParamValue ("chain", 2.0f);
    else if (bars == 2 && firstSlot == 0) setParamValue ("chain", 1.0f);
    setParamValue ("slot", (float) firstSlot);
    ++patVersion;
    info = juce::String (bars) + (bars == 1 ? " BAR " : " BARS ") + "/ " + juce::String (placed) + " HITS"
         + (swingSteps > 0.0 ? " / SWING " + juce::String (juce::roundToInt (50.0 + swingSteps * 50.0)) + "%" : juce::String())
         + (cut > 0 ? " / " + juce::String (cut) + " PAST BAR 4 CUT" : juce::String()) + (skipped > 0 ? " / " + juce::String (skipped) + " UNKNOWN NOTES" : juce::String());
    return bars;
}

juce::String ShuffleBuddyProcessor::noteName (int note)
{
    static const char* n[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return juce::String (n[(note % 12 + 12) % 12]) + juce::String (note / 12 - 2);
}

void ShuffleBuddyProcessor::applyNoteMap (int map)
{
    // 0: Live's drum kits (GM layout: C1 kick, D1 snare, D#1 clap, C#1 rim, F#1 hat, A#1 open hat, toms spread, C#2 crash)
    // 1: Push / any Drum Rack: the first pads in order from C1 (toms take 4 pads, floor..high)
    static const int live[NumPads] = { 36, 38, 39, 37, 42, 46, 47, 44, 45, 49 };
    static const int push[NumPads] = { 36, 37, 38, 39, 40, 41, 42, 43, 44, 48 };
    if (map == 0 || map == 1)
        for (int i = 0; i < NumPads; ++i) setParamValue (pid ("note", i), (float) (map == 0 ? live[i] : push[i]));
    setParamValue ("notemap", (float) map);
}

bool ShuffleBuddyProcessor::writeMidiFile (const juce::MidiMessageSequence& notes, double bpm, const juce::File& f)
{
    juce::MidiMessageSequence track;
    track.addEvent (juce::MidiMessage::textMetaEvent (3, "Shuffle Buddy"), 0);
    track.addEvent (juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / bpm)), 0);
    track.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0);
    track.addSequence (notes, 0.0);
    track.updateMatchedPairs();
    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (960);
    mf.addTrack (track);
    f.deleteFile();
    juce::FileOutputStream os (f);
    return os.openedOk() && mf.writeTo (os);
}

void ShuffleBuddyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();

    // MIDI: CC learn / control, and drum notes (GM: 36 kick, 38 snare, 39 clap, 37 rim, 42 hat, 46 open, 47 perc, 70 shaker; or 48-55)
    const auto& allParams = getParameters();
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isController() && m.getControllerNumber() < 120)
        {
            const int cc = m.getControllerNumber();
            const int learn = learnParam.load();
            if (learn >= 0)
            {
                for (auto& c : ccMap) if (c.load() == learn) c.store (-1);
                ccMap[(size_t) cc].store (learn);
                learnParam.store (-1);
                ++mapVersion;
            }
            const int idx = ccMap[(size_t) cc].load();
            if (idx >= 0 && idx < allParams.size()) allParams[idx]->setValueNotifyingHost ((float) m.getControllerValue() / 127.0f);
            continue;
        }
        if (! m.isNoteOnOrOff()) continue;
        int pd = -1, alt = 1;
        for (int i = 0; i < NumPads && pd < 0; ++i)
        {
            if (i == Toms) { for (int a = 0; a < 4; ++a) if (noteFor (Toms, a) == m.getNoteNumber()) { pd = Toms; alt = a; } }
            else if (noteFor (i, 0) == m.getNoteNumber()) pd = i;
        }
        if (pd < 0 && m.getNoteNumber() >= 60 && m.getNoteNumber() < 60 + NumPads) pd = m.getNoteNumber() - 60;   // C3 upwards: pads in order
        if (pd < 0) continue;
        if (m.isNoteOff()) { midiHeld[(size_t) pd] = false; continue; }
        if (queued < (int) queue.size()) queue[(size_t) queued++] = { sampleClock + meta.samplePosition, pd, m.getFloatVelocity(), alt, false };
        if (patRec.load() && wasPlaying) { recordHit (pd, alt, m.getFloatVelocity(), stepPos + (double) meta.samplePosition / sps); midiHeld[(size_t) pd] = true; heldFrom[(size_t) pd] = stepPos; }
    }
    for (int i = 0; i < NumPads; ++i)
    {
        const float a = pendingAudition[(size_t) i].exchange (-1.0f);
        if (a >= 0.0f && queued < (int) queue.size()) queue[(size_t) queued++] = { sampleClock, i, a, 1, false };
        if (pendingRec[(size_t) i].exchange (false) && a >= 0.0f && patRec.load() && wasPlaying) { recordHit (i, 1, a, stepPos); heldFrom[(size_t) i] = stepPos; }
    }

    // transport: host tempo/position when rolling, else the TEMPO control
    bpmNow = p ("tempo");
    hostRolling = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto bq = pos->getBpm()) bpmNow = *bq;
            if (pos->getIsPlaying()) if (auto q = pos->getPpqPosition()) { hostRolling = true; hostPpq = *q; }
        }

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getWritePointer (1);
    outMidi.clear();
    for (int start = 0; start < n; start += maxChunk)   // fixed-size chunks: nothing allocates, whatever the host block size
    {
        chunkBase = start;
        runChunk (L + start, R + start, juce::jmin (maxChunk, n - start));
    }
    if (p ("sound") < 0.5f) buffer.clear();   // MIDI ONLY: drive a Drum Rack, make no sound of its own
    midi.swapWith (outMidi);   // the plugin's MIDI output: every note it plays, ready to record in the DAW
    outMidi.ensureSize (8192);
    outLvl.store (buffer.getRMSLevel (0, 0, n));
}

void ShuffleBuddyProcessor::runChunk (float* L, float* R, int n)
{
    const double sr = getSampleRate();
    sps = sr * 60.0 / bpmNow / 4.0;
    const bool play = p ("play") > 0.5f || hostRolling;
    auto resetPlans = [this] { for (auto& pl : plans) pl.bar = -1; emittedUpTo = { -1.0e9, -1.0e9 }; publishedBar = -1; };
    if (play && ! wasPlaying)
    {
        stepPos = hostRolling ? hostPpq * 4.0 : -1.0e-6;
        resetPlans();
    }
    if (hostRolling && std::abs (hostPpq * 4.0 - stepPos) > 0.5)   // host jumped (loop / relocate)
    {
        stepPos = hostPpq * 4.0;
        resetPlans();
    }
    wasPlaying = play;

    if (play)
    {
        // look ahead 40 ms (+2 steps of planning) so hits pushed or nudged EARLY still land before their step
        const double look = 0.04 * sr;
        const double w0 = stepPos, w1 = stepPos + ((double) n + look) / sps;
        const auto in = engineIn();
        const double msTo16 = sr * 0.001 / sps;
        const bool fillOn = p ("fill") > 0.5f;
        if (patRec.load())   // REC PATTERN: hold a pad and it wipes that drum from the steps it passes
            for (int pd = 0; pd < NumPads; ++pd)
                if ((uiHeld[(size_t) pd].load() || midiHeld[(size_t) pd]) && w0 - heldFrom[(size_t) pd] > 1.0)
                    for (double st = std::ceil (w0); st < stepPos + (double) n / sps; st += 1.0)
                    {
                        const auto bar = (juce::int64) std::floor (st / 16.0);
                        auto c = cellData (groove::slotForBar (in, bar), pd, (int) st % 16);
                        if (c.on()) { c.vel = 0; setCellData (groove::slotForBar (in, bar), pd, (int) st % 16, c); ++patVersion; }
                    }
        const juce::int64 curBar = (juce::int64) std::floor (juce::jmax (0.0, w0) / 16.0);
        // FILL pressed during a bar: re-plan it with the fill (same dice, so what already played stays the same)
        if (fillOn && ! fillWasOn)
            if (auto* pl = planFor (curBar))
                if (! pl->filled)
                {
                    const auto* prev = planFor (curBar - 1);
                    planBar (*pl, curBar, true, prev != nullptr && prev->filled, in);
                    publishedBar = -1;
                }
        fillWasOn = fillOn;
        for (juce::int64 b = curBar; (double) b * 16.0 - 2.0 < w1; ++b)
        {
            auto* pl = planFor (b);
            if (pl == nullptr)
            {
                int idx = plans[0].bar < plans[1].bar ? 0 : 1;   // reuse the older plan slot
                if (plans[(size_t) idx].bar == curBar) idx = 1 - idx;
                pl = &plans[(size_t) idx];
                const auto* prev = planFor (b - 1);
                planBar (*pl, b, fillOn, prev != nullptr && prev->filled, in);
                emittedUpTo[(size_t) idx] = -1.0e9;
            }
            const int idx = (int) (pl - plans.data());
            const bool rec = recArmed.load();
            if (rec && recStartBar.load() < 0 && b > curBar - 1 && (double) b * 16.0 >= w0) recStartBar.store (b);
            for (int h = 0; h < pl->nHits; ++h)
            {
                const auto& hit = pl->hits[(size_t) h];
                const double abs16 = (double) b * 16.0 + hit.pos + hit.ms * msTo16;
                if (abs16 <= emittedUpTo[(size_t) idx] || abs16 > w1) continue;
                const auto t = juce::jmax (sampleClock, sampleClock + (juce::int64) std::llround ((abs16 - w0) * sps));
                if (queued < (int) queue.size()) queue[(size_t) queued++] = { t, hit.lane, hit.vel, hit.alt, true };
                const auto rs = recStartBar.load();
                if (rec && rs >= 0 && abs16 >= (double) rs * 16.0 - 1.0)
                {
                    int s1, n1, s2, n2;
                    recFifo.prepareToWrite (1, s1, n1, s2, n2);
                    if (n1 > 0) { recBuf[(size_t) s1] = { abs16 - (double) rs * 16.0, noteFor (hit.lane, hit.alt), hit.vel }; recFifo.finishedWrite (1); }
                }
            }
            emittedUpTo[(size_t) idx] = juce::jmax (emittedUpTo[(size_t) idx], w1);
        }
        // show the bar that's actually sounding now
        if (curBar != publishedBar) if (auto* pl = planFor (curBar)) { publishBar (*pl); publishedBar = curBar; }
        stepNow.store ((int) (((juce::int64) std::floor (stepPos)) & 15));
        barNow.store ((int) (((juce::int64) std::floor (stepPos)) >> 4) & 3);
        stepPos += (double) n / sps;
        if (hostRolling) hostPpq += (double) n / sps / 4.0;
    }

    const bool midiOutOn = p ("midiout") > 0.5f;
    {   // note-offs that fall inside this chunk
        int keep = 0;
        for (int i = 0; i < nNoteOffs; ++i)
        {
            const auto& o = noteOffs[(size_t) i];
            if (o.t < sampleClock + n) outMidi.addEvent (juce::MidiMessage::noteOff (1, o.note), chunkBase + (int) juce::jmax ((juce::int64) 0, o.t - sampleClock));
            else noteOffs[(size_t) keep++] = o;
        }
        nNoteOffs = keep;
    }
    // ---- voices: dry + echo send + space send, each hit starting on its exact sample
    eBuf.clear(); sBuf.clear(); kBuf.clear();
    auto* KL = kBuf.getWritePointer (0); auto* KR = kBuf.getWritePointer (1);
    auto* EL = eBuf.getWritePointer (0); auto* ER = eBuf.getWritePointer (1);
    auto* SL = sBuf.getWritePointer (0); auto* SR = sBuf.getWritePointer (1);
    int done = 0;
    while (done < n)
    {
        juce::int64 nextT = sampleClock + n;
        for (int i = 0; i < queued; ++i) nextT = std::min (nextT, queue[(size_t) i].t);
        const int upto = (int) std::clamp<juce::int64> (nextT - sampleClock, (juce::int64) done, (juce::int64) n);
        if (upto > done)
        {
            for (auto& vc : voices) vc.render (L, R, KL, KR, EL, ER, SL, SR, done, upto - done);
            done = upto;
        }
        if (done >= n) break;
        int keep = 0;
        const juce::int64 nowAbs = sampleClock + done;
        for (int i = 0; i < queued; ++i)
        {
            const auto e = queue[(size_t) i];
            if (e.t <= nowAbs)
            {
                const auto saved = sampleClock; sampleClock = nowAbs; trigger (e.pad, e.vel, e.alt); sampleClock = saved;
                if (e.seq && midiOutOn)
                {
                    const int note = noteFor (e.pad, e.alt);
                    outMidi.addEvent (juce::MidiMessage::noteOn (1, note, juce::jlimit (0.01f, 1.0f, e.vel)), chunkBase + done);
                    if (nNoteOffs < (int) noteOffs.size()) noteOffs[(size_t) nNoteOffs++] = { nowAbs + (juce::int64) (0.06 * getSampleRate()), note };
                }
            }
            else queue[(size_t) keep++] = e;
        }
        queued = keep;
    }

    // ---- echo (DubChord's tape echo) with THROW
    static const float beats[] = { 0.25f, 0.5f, 0.75f, 1.0f };
    const float echoAmt = p ("echo"), spaceAmt = p ("space");
    DubDelay::Params dp;
    dp.timeSec  = (float) (60.0 / bpmNow) * beats[juce::jlimit (0, 3, (int) p ("echotime"))];
    dp.feedback = 0.3f + 0.42f * echoAmt;
    dp.toneHz   = 3200.0f;
    dp.lowCutHz = 320.0f;
    dp.wobble   = 0.2f;
    dp.spread   = 0.6f;
    dp.mix      = juce::jmin (1.0f, echoAmt * 1.4f);
    dp.style    = (int) p ("echotype");
    dp.heads    = 6;
    dp.wetOnly  = true;
    {
        const bool held = p ("throw") > 0.5f;
        const float sec = (float) n / (float) sr;
        throwAmt  += ((held ? 1.0f : 0.0f) - throwAmt)  * (1.0f - std::exp (-sec / (held ? 0.03f : 2.5f)));
        throwTail += ((held ? 1.0f : 0.0f) - throwTail) * (1.0f - std::exp (-sec / (held ? 0.03f : 6.0f)));
        dp.feedback = dp.feedback + (1.05f - dp.feedback) * throwAmt;
        dp.mix      = dp.mix + (juce::jmax (dp.mix, 0.9f) - dp.mix) * throwTail;
        dp.send     = held ? 1.0f + 2.5f * throwAmt : 1.0f;   // everything gets thrown, not just the sent pads
        dp.lowCutHz = dp.lowCutHz * (1.0f - 0.5f * throwTail);
        if (held) for (int i = 0; i < n; ++i) { EL[i] += L[i] * 0.4f * throwAmt; ER[i] += R[i] * 0.4f * throwAmt; }
    }
    echo.process (EL, ER, n, dp);

    // ---- space (DubChord's reverb): sends + a little of the echo
    for (int i = 0; i < n; ++i) { SL[i] += EL[i] * 0.35f; SR[i] += ER[i] * 0.35f; }
    space.setParameters (0.3f + 0.6f * spaceAmt, 0.55f);
    space.process (SL, SR, n, spaceAmt * 1.1f, 0.0f);

    // ---- kick bus: RUMBLE (dark reverb tail, low-passed, pumped by the kick) and DUCK (the rest of the kit breathes around the kick)
    const float duck = p ("kduck"), rumble = p ("krumble");
    auto* RL = rBuf.getWritePointer (0); auto* RR = rBuf.getWritePointer (1);
    if (rumble > 0.001f || rumbleTail > 0)
    {
        for (int i = 0; i < n; ++i) { RL[i] = RR[i] = (KL[i] + KR[i]) * 0.5f * rumble; }
        rumbleVerb.process (RL, RR, n, 1.0f, 0.0f);
        float lp, bp, hp, lp2;
        for (int i = 0; i < n; ++i) { rumbleLp1.run (RL[i], lp, bp, hp); rumbleLp2.run (lp, lp2, bp, hp); RL[i] = std::tanh (lp2 * 2.5f) * 0.5f; }
        rumbleTail = rumble > 0.001f ? (int) (6.0 * sr) : juce::jmax (0, rumbleTail - n);
    }
    else std::fill (RL, RL + n, 0.0f);
    const float holdRel = std::exp (-1.0f / (0.16f * (float) sr));
    const float rDuck = juce::jmax (0.85f, duck);
    for (int i = 0; i < n; ++i)
    {
        if (duckHold > 0) --duckHold; else duckEnv *= holdRel;
        const float rmb = RL[i] * (1.0f - rDuck * duckEnv);
        const float g = 1.0f - duck * 0.8f * duckEnv;
        L[i] = (L[i] + EL[i] + SL[i]) * g + KL[i] + rmb;
        R[i] = (R[i] + ER[i] + SR[i]) * g + KR[i] + rmb;
    }
    fxFilter.process (L, R, n, p ("fxfilter"), 0.45f);

    // ---- SQUASH: glue compressor + 4x oversampled saturation
    const float squash = p ("squash");
    {
        const float thrDb = -4.0f - 18.0f * squash, ratio = 1.5f + 3.5f * squash;
        const float att = std::exp (-1.0f / (0.008f * (float) sr)), rel = std::exp (-1.0f / (0.14f * (float) sr));
        const float makeup = juce::Decibels::decibelsToGain (-thrDb * (1.0f - 1.0f / ratio) * 0.45f * squash);
        float grMax = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            const float x = juce::jmax (std::abs (L[i]), std::abs (R[i]));
            compEnv = x > compEnv ? att * compEnv + (1.0f - att) * x : rel * compEnv + (1.0f - rel) * x;
            const float envDb = juce::Decibels::gainToDecibels (compEnv, -100.0f);
            const float grDb = envDb > thrDb ? (envDb - thrDb) * (1.0f - 1.0f / ratio) : 0.0f;
            const float target = squash > 0.001f ? juce::Decibels::decibelsToGain (-grDb) * makeup : 1.0f;
            compGain += (target - compGain) * 0.02f;
            grMax = juce::jmax (grMax, grDb);
            L[i] *= compGain; R[i] *= compGain;
        }
        grNow.store (grMax * (squash > 0.001f ? 1.0f : 0.0f));
    }
    if (oversampler != nullptr)
    {
        float* chans[2] = { L, R };
        juce::dsp::AudioBlock<float> block (chans, 2, (size_t) n);
        auto up = oversampler->processSamplesUp (block);
        // tape-ish warmth that comes in with SQUASH. At 0 it is clean up to a soft knee near full scale,
        // so long kick tails and toms don't get a constant fuzz on them.
        const float drive = 1.0f + squash * 1.6f, comp = 1.0f / std::tanh (drive);
        const float gw = drive * comp * 0.92f, wet = juce::jmin (1.0f, squash * 2.0f);
        auto knee = [] (float x) { const float a = std::abs (x); return a <= 0.75f ? x : std::copysign (0.75f + 0.25f * std::tanh ((a - 0.75f) / 0.25f), x); };
        for (size_t c = 0; c < 2; ++c)
        {
            auto* d = up.getChannelPointer (c);
            for (size_t i = 0; i < up.getNumSamples(); ++i) { const float x = d[i]; d[i] = knee (x * gw) + (std::tanh (x * drive) * comp * 0.92f - knee (x * gw)) * wet; }
        }
        oversampler->processSamplesDown (block);
    }

    // ---- dust: crackle, hiss, a little lowpass + bit grit (Burial territory), then volume
    const float dust = p ("dust");
    const float g = juce::Decibels::decibelsToGain (p ("master"));
    const float lpc = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * (18000.0f - 12500.0f * dust) / (float) sr);
    const float crushLevels = std::exp2 (16.0f - 7.0f * dust);
    const int holdN = 1 + (int) (dust * 2.0f);
    float ePk = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        ePk = juce::jmax (ePk, std::abs (EL[i]));
        float l = L[i], r = R[i];
        if (dust > 0.001f)
        {
            if (rng.nextFloat() < dust * 0.0009f) crackleEnv = 0.1f + 0.6f * rng.nextFloat();
            crackle = (rng.nextFloat() * 2.0f - 1.0f) * crackleEnv;
            crackleEnv *= 0.93f;
            hissLp += ((rng.nextFloat() * 2.0f - 1.0f) - hissLp) * 0.3f;
            dustLpL += (l - dustLpL) * lpc;
            dustLpR += (r - dustLpR) * lpc;
            l = dustLpL; r = dustLpR;
            const float add = (crackle * 0.6f + hissLp * 0.012f) * dust;
            if (++crushHold >= holdN) { crushHold = 0; crushL = std::round (l * crushLevels) / crushLevels; crushR = std::round (r * crushLevels) / crushLevels; }
            l = crushL + add; r = crushR + add;
        }
        L[i] = softClip (l * g);
        R[i] = softClip (r * g);
    }
    echoLvl.store (juce::jmax (ePk, echoLvl.load() * 0.85f));
    sampleClock += n;
}

juce::AudioProcessorEditor* ShuffleBuddyProcessor::createEditor() { return new ScaledEditor<ShuffleBuddyEditor, ShuffleBuddyProcessor> (*this); }

void ShuffleBuddyProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    auto* pat = xml->createNewChildElement ("PATTERNS2");   // per step: velocity, chance, nudge, ratchet, tom pitch (hex)
    for (int sl = 0; sl < groove::numSlots; ++sl)
    {
        juce::String str;
        for (int pd = 0; pd < NumPads; ++pd)
            for (int st = 0; st < groove::numSteps; ++st)
                str << juce::String::toHexString ((juce::int64) groove::pack (cellData (sl, pd, st))).paddedLeft ('0', 10);
        pat->setAttribute ("s" + juce::String (sl), str);
    }
    auto* map = xml->createNewChildElement ("MIDIMAP");
    for (int cc = 0; cc < 128; ++cc)
    {
        const int idx = ccMap[(size_t) cc].load();
        if (idx >= 0 && idx < getParameters().size())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (getParameters()[idx])) map->setAttribute ("cc" + juce::String (cc), rp->getParameterID());
    }
    auto* names = xml->createNewChildElement ("NAMES");
    names->setAttribute ("kick", kickName); names->setAttribute ("kit", kitName);
    auto* smp = xml->createNewChildElement ("SAMPLES");
    for (int pd = 0; pd < NumPads; ++pd) if (samplePath (pd).isNotEmpty()) smp->setAttribute ("p" + juce::String (pd), samplePath (pd));
    copyXmlToBinary (*xml, dest);
}

void ShuffleBuddyProcessor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return;
    if (auto* pat = xml->getChildByName ("PATTERNS2"))
    {
        for (int sl = 0; sl < groove::numSlots; ++sl)
        {
            const auto str = pat->getStringAttribute ("s" + juce::String (sl));
            if (str.length() == NumPads * groove::numSteps * 10)
                for (int i = 0; i < NumPads * groove::numSteps; ++i)
                    setCellData (sl, i / groove::numSteps, i % groove::numSteps, groove::unpack ((juce::uint64) str.substring (i * 10, i * 10 + 10).getHexValue64()));
        }
        xml->removeChildElement (pat, true);
    }
    if (auto* pat = xml->getChildByName ("PATTERNS"))
    {
        for (int s = 0; s < groove::numSlots; ++s)
        {
            const auto str = pat->getStringAttribute ("s" + juce::String (s));
            if (str.length() == 8 * groove::numSteps)   // older 8-lane saves
                for (int i = 0; i < str.length(); ++i) setCell (s, i / groove::numSteps, i % groove::numSteps, (juce::uint8) juce::jlimit (0, 3, (int) (str[i] - '0')));
        }
        xml->removeChildElement (pat, true);
    }
    if (auto* map = xml->getChildByName ("MIDIMAP"))
    {
        for (auto& c : ccMap) c.store (-1);
        for (int i = 0; i < map->getNumAttributes(); ++i)
            if (auto* prm = apvts.getParameter (map->getAttributeValue (i)))
            {
                const int cc = map->getAttributeName (i).substring (2).getIntValue();
                if (cc >= 0 && cc < 128) ccMap[(size_t) cc].store (prm->getParameterIndex());
            }
        xml->removeChildElement (map, true);
        ++mapVersion;
    }
    if (auto* names = xml->getChildByName ("NAMES"))
    {
        kickName = names->getStringAttribute ("kick", kickName);
        kitName = names->getStringAttribute ("kit", kitName);
        xml->removeChildElement (names, true);
    }
    if (auto* smp = xml->getChildByName ("SAMPLES"))
    {
        for (int pd = 0; pd < NumPads; ++pd)
        {
            const auto path = smp->getStringAttribute ("p" + juce::String (pd));
            if (path.isNotEmpty() && juce::File (path).existsAsFile()) loadSample (pd, juce::File (path));
        }
        xml->removeChildElement (smp, true);
    }
    apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ShuffleBuddyProcessor(); }
