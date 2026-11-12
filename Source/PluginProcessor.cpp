#include "PluginProcessor.h"
#include "PluginEditor.h"

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
    add (new juce::AudioParameterChoice ({ "style", 1 }, "Style", juce::StringArray { "2-Step", "Burial", "Funk", "Breaks" }, 0));
    add (new juce::AudioParameterChoice ({ "slot", 1 }, "Pattern", juce::StringArray { "A", "B", "C", "D" }, 0));
    add (new juce::AudioParameterBool ({ "play", 1 }, "Play", false));
    add (new juce::AudioParameterBool ({ "fill", 1 }, "Fill", false));
    add (new juce::AudioParameterBool ({ "throw", 1 }, "Throw", false));
    add (new juce::AudioParameterChoice ({ "chain", 1 }, "Chain", juce::StringArray { "Off", "A-B", "A-B-C-D", "A-A-A-B" }, 0));
    add (new APF ({ "echo", 1 }, "Echo", NR (0.0f, 1.0f), 0.35f, fmt (pct)));
    add (new APF ({ "space", 1 }, "Space", NR (0.0f, 1.0f), 0.25f, fmt (pct)));
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
    }
    juce::StringArray soundNames;
    for (auto& snd : sounds::library()) soundNames.add (snd.name);
    static const char* padDefault[NumPads] = { "", "Garage Snare", "808 Clap", "808 Rim", "808 Hat", "Garage Open Hat", "Perc Thud", "Shaker" };
    for (int i = 0; i < NumPads; ++i)
    {
        const juce::String n = juce::String (padName (i)).toLowerCase();
        if (i > 0) add (new juce::AudioParameterChoice ({ pid ("snd", i), 1 }, n + " sound", soundNames, sounds::find (padDefault[i])));
        add (new juce::AudioParameterBool ({ pid ("layer", i), 1 }, n + " layer WAV + synth", false));
        add (new APF ({ pid ("lvl", i), 1 }, n + " level", NR (0.0f, 1.0f), 0.8f, fmt (pct)));
        add (new APF ({ pid ("tune", i), 1 }, n + " tune", NR (-12.0f, 12.0f, 0.1f), 0.0f, fmt ([] (float x) { return (x > 0 ? "+" : "") + juce::String (x, 1); })));
        add (new APF ({ pid ("decay", i), 1 }, n + " decay", NR (0.0f, 1.0f), 0.45f, fmt (pct)));
        add (new APF ({ pid ("push", i), 1 }, n + " push", NR (-20.0f, 30.0f, 0.5f), 0.0f,
                      fmt ([] (float x) { return (x > 0 ? "+" : "") + juce::String (juce::roundToInt (x)) + "ms"; })));
        static const float eDef[NumPads] = { 0.0f, 0.35f, 0.3f, 0.45f, 0.05f, 0.1f, 0.4f, 0.0f };
        static const float sDef[NumPads] = { 0.0f, 0.3f, 0.35f, 0.2f, 0.1f, 0.2f, 0.25f, 0.1f };
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
    }
    for (auto& c : ccMap) c.store (-1);
    // starter patterns: one per style, so it grooves the moment you press play
    juce::Random seed (2026);
    for (int s = 0; s < groove::numSlots; ++s) setGrid (s, groove::roll (s, seed));
    // pattern A: a hand-written 2-step to start from (the one on the mockup)
    const groove::Grid a = {{
        { 3,0,0,0, 0,0,0,0, 0,0,2,0, 0,0,0,0 },   // kick: 1 and the skip before 3
        { 0,0,0,0, 3,0,0,1, 0,0,0,0, 3,0,1,0 },   // snare
        { 0,0,0,0, 2,0,0,0, 0,0,0,0, 2,0,0,0 },   // clap
        { 0,0,1,0, 0,0,2,0, 0,1,0,0, 0,0,2,0 },   // rim
        { 2,1,0,1, 2,1,0,1, 2,1,0,1, 2,1,0,1 },   // hat
        { 0,0,2,0, 0,0,2,0, 0,0,2,0, 0,0,2,0 },   // open hat on the off-beats
        { 0,0,0,2, 0,0,0,0, 0,0,0,2, 0,1,0,0 },   // perc
        { 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1 } }}; // shaker
    setGrid (0, a);
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
    for (int pd = 0; pd < NumPads; ++pd) for (int s = 0; s < groove::numSteps; ++s) g[(size_t) pd][(size_t) s] = cell (slot, pd, s);
    return g;
}

void ShuffleBuddyProcessor::setGrid (int slot, const groove::Grid& g)
{
    for (int pd = 0; pd < NumPads; ++pd) for (int s = 0; s < groove::numSteps; ++s) setCell (slot, pd, s, g[(size_t) pd][(size_t) s]);
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
    k.gain = 0.9f / (1.0f + 0.35f * k.sub);
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
        setParamValue (pid ("snd", pd), (float) sounds::find (kit.pads[pd - 1]));
        setParamValue (pid ("tune", pd), 0.0f);
        setParamValue (pid ("decay", pd), 0.45f);
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
    queued = 0; lastScheduled = -1; preparedBar = -1; stepPos = 0.0; wasPlaying = false;
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

void ShuffleBuddyProcessor::trigger (int pad, float vel)
{
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
    pick->trigger (pad, vel, patch, padTune[i]->load(), padDecay[i]->load(), padLevel[i]->load(), smpl,
                   (float) getSampleRate(), sampleClock, (juce::uint32) rng.nextInt(), padEcho[i]->load(), padSpace[i]->load());
    if (smpl != nullptr && padLayer[i]->load() > 0.5f)   // LAYER: the synth plays underneath the WAV
    {
        DrumVoice* second = nullptr;
        for (auto& vc : voices) if (! vc.active() && &vc != pick) { second = &vc; break; }
        if (second != nullptr)
            second->trigger (pad, vel, patch, padTune[i]->load(), padDecay[i]->load(), padLevel[i]->load(), nullptr,
                             (float) getSampleRate(), sampleClock, (juce::uint32) rng.nextInt(), padEcho[i]->load(), padSpace[i]->load());
    }
    hitVel[i].store (vel);
    hitCount[i].fetch_add (1);
}

void ShuffleBuddyProcessor::prepareBar (juce::int64 bar)
{
    // this bar's playing copy: stored pattern + per-bar variety + auto ghosts + fill
    barGrid = grid (activeSlot);
    const float variety = p ("variety"), ghosts = p ("ghosts");
    const int style = (int) p ("style");
    if (variety > 0.0f && bar > 0)
    {
        const int moves = (int) std::floor (variety * 4.0f + rng.nextFloat());
        groove::vary (barGrid, style, rng, moves);
        if (variety > 0.4f && (bar % 4) == 3 && rng.nextFloat() < variety)   // end of a 4-bar phrase: little lift
            for (int s = 12; s < 16; ++s) if (barGrid[Snare][(size_t) s] == 0 && rng.nextFloat() < 0.5f) barGrid[Snare][(size_t) s] = 1;
    }
    if (ghosts > 0.25f)   // auto ghosts: quiet snare/hat notes in the gaps, on the off 16ths
        for (int s = 1; s < 16; s += 2)
        {
            if (barGrid[Snare][(size_t) s] == 0 && rng.nextFloat() < (ghosts - 0.25f) * 0.4f) barGrid[Snare][(size_t) s] = 1;
            if (barGrid[Hat][(size_t) s] == 0 && barGrid[OpenHat][(size_t) s] == 0 && rng.nextFloat() < (ghosts - 0.25f) * 0.6f) barGrid[Hat][(size_t) s] = 1;
        }
    if (p ("fill") > 0.5f) groove::fill (barGrid, rng);
}

void ShuffleBuddyProcessor::scheduleStep (juce::int64 step, juce::int64 at, double sps)
{
    const int s = (int) (step & 15);
    const juce::int64 bar = step >> 4;
    if (s == 0 || bar != preparedBar)
    {
        const int chain = (int) p ("chain");   // pattern changes land on the next bar
        activeSlot = chain == 1 ? (int) (bar & 1) : chain == 2 ? (int) (bar & 3) : chain == 3 ? ((bar & 3) == 3 ? 1 : 0) : (int) p ("slot");
        playSlotNow.store (activeSlot);
        prepareBar (bar);
        preparedBar = bar;
        fillApplied = p ("fill") > 0.5f;
    }
    else if (s == 8 && ! fillApplied && p ("fill") > 0.5f)   // FILL pressed during the bar: fill its second half
    {
        groove::fill (barGrid, rng);
        fillApplied = true;
    }
    const float sr = (float) getSampleRate();
    const float swing = (p ("swing") - 50.0f) / 50.0f;   // 0 .. 0.5 of a 16th
    const float ghostVel = 0.12f + 0.42f * p ("ghosts");
    const float loosen = p ("loosen");
    for (int pd = 0; pd < NumPads; ++pd)
    {
        const int lvl = barGrid[(size_t) pd][(size_t) s];
        if (lvl == 0) continue;
        float vel = lvl == 3 ? 1.0f : lvl == 2 ? 0.72f : ghostVel;
        // loosen: human timing + velocity wobble (gaussian-ish: average of two uniforms)
        const float g1 = rng.nextFloat() + rng.nextFloat() - 1.0f, g2 = rng.nextFloat() + rng.nextFloat() - 1.0f;
        vel = juce::jlimit (0.05f, 1.0f, vel * (1.0f + g1 * 0.35f * loosen));
        double t = (double) at;
        if (s & 1) t += swing * sps;
        t += padPush[(size_t) pd]->load() * 0.001 * sr;
        t += g2 * loosen * 0.014 * sr;
        if (queued < (int) queue.size())
            queue[(size_t) queued++] = { juce::jmax (sampleClock, (juce::int64) std::llround (t)), pd, vel };
    }
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
        if (! m.isNoteOn()) continue;
        static const int gm[NumPads] = { 36, 38, 39, 37, 42, 46, 47, 70 };
        int pd = -1;
        for (int i = 0; i < NumPads; ++i) if (m.getNoteNumber() == gm[i]) pd = i;
        if (pd < 0 && m.getNoteNumber() >= 48 && m.getNoteNumber() < 56) pd = m.getNoteNumber() - 48;
        if (pd >= 0 && queued < (int) queue.size()) queue[(size_t) queued++] = { sampleClock + meta.samplePosition, pd, m.getFloatVelocity() };
    }
    for (int i = 0; i < NumPads; ++i)
    {
        const float a = pendingAudition[(size_t) i].exchange (-1.0f);
        if (a >= 0.0f && queued < (int) queue.size()) queue[(size_t) queued++] = { sampleClock, i, a };
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
    for (int start = 0; start < n; start += maxChunk)   // fixed-size chunks: nothing allocates, whatever the host block size
        runChunk (L + start, R + start, juce::jmin (maxChunk, n - start));
    outLvl.store (buffer.getRMSLevel (0, 0, n));
}

void ShuffleBuddyProcessor::runChunk (float* L, float* R, int n)
{
    const double sr = getSampleRate();
    sps = sr * 60.0 / bpmNow / 4.0;
    const bool play = p ("play") > 0.5f || hostRolling;
    if (play && ! wasPlaying)
    {
        stepPos = hostRolling ? hostPpq * 4.0 : -1.0e-6;
        lastScheduled = (juce::int64) std::ceil (stepPos - 1.0e-9) - 1;
        preparedBar = -1;
    }
    if (hostRolling && std::abs (hostPpq * 4.0 - stepPos) > 0.5)   // host jumped (loop / relocate)
    {
        stepPos = hostPpq * 4.0;
        lastScheduled = (juce::int64) std::ceil (stepPos - 1.0e-9) - 1;
        preparedBar = -1;
    }
    wasPlaying = play;

    if (play)
    {
        // look ahead 40 ms so drums pushed EARLY can still land before their step
        const double look = 0.04 * sr;
        for (juce::int64 k = lastScheduled + 1; (double) (k - stepPos) * sps < (double) n + look; ++k)
        {
            scheduleStep (k, sampleClock + (juce::int64) std::llround ((k - stepPos) * sps), sps);
            lastScheduled = k;
        }
        stepNow.store ((int) (((juce::int64) std::floor (stepPos)) & 15));
        barNow.store ((int) (((juce::int64) std::floor (stepPos)) >> 4) & 3);
        stepPos += (double) n / sps;
        if (hostRolling) hostPpq += (double) n / sps / 4.0;
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
            if (e.t <= nowAbs) { const auto saved = sampleClock; sampleClock = nowAbs; trigger (e.pad, e.vel); sampleClock = saved; }
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
        const float drive = 1.0f + squash * 1.6f, comp = 1.0f / std::tanh (drive);
        for (size_t c = 0; c < 2; ++c)
        {
            auto* d = up.getChannelPointer (c);
            for (size_t i = 0; i < up.getNumSamples(); ++i) d[i] = std::tanh (d[i] * drive) * comp * 0.92f;
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

juce::AudioProcessorEditor* ShuffleBuddyProcessor::createEditor() { return new ShuffleBuddyEditor (*this); }

void ShuffleBuddyProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    auto* pat = xml->createNewChildElement ("PATTERNS");
    for (int s = 0; s < groove::numSlots; ++s)
    {
        juce::String str;
        for (int pd = 0; pd < NumPads; ++pd) for (int st = 0; st < groove::numSteps; ++st) str << (int) cell (s, pd, st);
        pat->setAttribute ("s" + juce::String (s), str);
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
    if (auto* pat = xml->getChildByName ("PATTERNS"))
    {
        for (int s = 0; s < groove::numSlots; ++s)
        {
            const auto str = pat->getStringAttribute ("s" + juce::String (s));
            if (str.length() == NumPads * groove::numSteps)
                for (int i = 0; i < str.length(); ++i) setCell (s, i / groove::numSteps, i % groove::numSteps, (juce::uint8) juce::jlimit (0, 3, str[i] - '0'));
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
