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
    for (int i = 0; i < NumPads; ++i)
    {
        const juce::String n = juce::String (padName (i)).toLowerCase();
        add (new APF ({ pid ("lvl", i), 1 }, n + " level", NR (0.0f, 1.0f), 0.8f, fmt (pct)));
        add (new APF ({ pid ("tune", i), 1 }, n + " tune", NR (-12.0f, 12.0f, 0.1f), 0.0f, fmt ([] (float x) { return (x > 0 ? "+" : "") + juce::String (x, 1); })));
        add (new APF ({ pid ("decay", i), 1 }, n + " decay", NR (0.0f, 1.0f), 0.45f, fmt (pct)));
        add (new APF ({ pid ("push", i), 1 }, n + " push", NR (-20.0f, 30.0f, 0.5f), 0.0f,
                      fmt ([] (float x) { return (x > 0 ? "+" : "") + juce::String (juce::roundToInt (x)) + "ms"; })));
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
    }
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
            size_t i = h & 127u;
            while (slots[i].h != 0) { jassert (slots[i].h != h); i = (i + 1) & 127u; }
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

void ShuffleBuddyProcessor::rollPattern()
{
    setGrid (currentSlot(), groove::roll ((int) apvts.getRawParameterValue ("style")->load(), uiRng));
}

void ShuffleBuddyProcessor::varyPattern()
{
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
    const auto len = (int) juce::jmin<juce::int64> (reader->lengthInSamples, (juce::int64) (reader->sampleRate * 10.0));   // up to 10 s
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

void ShuffleBuddyProcessor::prepareToPlay (double, int)
{
    queued = 0; lastScheduled = -1; preparedBar = -1; stepPos = 0.0; wasPlaying = false;
    for (auto& vc : voices) vc = DrumVoice();
}

void ShuffleBuddyProcessor::trigger (int pad, float vel, int offset)
{
    juce::ignoreUnused (offset);
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
    pick->trigger (pad, vel, padTune[i]->load(), padDecay[i]->load(), padLevel[i]->load(), liveSample[i].load(),
                   (float) getSampleRate(), sampleClock + offset, (juce::uint32) rng.nextInt());
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
        activeSlot = (int) p ("slot");   // pattern changes land on the next bar
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
    const double sr = getSampleRate();
    buffer.clear();
    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getWritePointer (1);

    // MIDI in: GM-ish drum notes (36 kick, 38 snare, 39 clap, 37 rim, 42 hat, 46 open, 47 perc, 70 shaker) or C1.. upward
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
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

    // ---- transport: host when it is rolling, else the TEMPO knob
    double bpm = p ("tempo");
    bool hostRolling = false; double ppq = 0.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (pos->getIsPlaying()) if (auto q = pos->getPpqPosition()) { hostRolling = true; ppq = *q; }
        }
    const double sps = sr * 60.0 / bpm / 4.0;
    const bool play = p ("play") > 0.5f || hostRolling;
    if (play && ! wasPlaying)
    {
        stepPos = hostRolling ? ppq * 4.0 : -1.0e-6;
        lastScheduled = (juce::int64) std::ceil (stepPos - 1.0e-9) - 1;
        preparedBar = -1;
    }
    if (hostRolling)
    {
        const double hostStep = ppq * 4.0;
        if (std::abs (hostStep - stepPos) > 0.5)   // host jumped (loop / relocate)
        {
            stepPos = hostStep;
            lastScheduled = (juce::int64) std::ceil (stepPos - 1.0e-9) - 1;
            preparedBar = -1;
        }
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
    }

    // ---- render voices, starting each event at its exact sample
    int done = 0;
    while (done < n)
    {
        juce::int64 nextT = sampleClock + n;
        for (int i = 0; i < queued; ++i) nextT = juce::jmin (nextT, queue[(size_t) i].t);
        const int upto = (int) juce::jlimit ((juce::int64) done, (juce::int64) n, nextT - sampleClock);
        if (upto > done)
        {
            for (auto& vc : voices) vc.render (L, R, done, upto - done);
            done = upto;
        }
        if (done >= n) break;
        int keep = 0;
        for (int i = 0; i < queued; ++i)
        {
            const auto e = queue[(size_t) i];
            if (e.t <= sampleClock + done) trigger (e.pad, e.vel, done);
            else queue[(size_t) keep++] = e;
        }
        queued = keep;
    }

    // ---- dust: crackle, hiss, a little lowpass + bit grit (Burial territory)
    const float dust = p ("dust");
    const float g = juce::Decibels::decibelsToGain (p ("master"));
    const float lpc = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * (18000.0f - 12500.0f * dust) / (float) sr);
    const float crushLevels = std::exp2 (16.0f - 7.0f * dust);
    const int holdN = 1 + (int) (dust * 2.0f);
    for (int i = 0; i < n; ++i)
    {
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
        L[i] = softClip (std::tanh (l * 1.1f) * g);
        R[i] = softClip (std::tanh (r * 1.1f) * g);
    }
    outLvl.store (buffer.getRMSLevel (0, 0, n));
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
