// Offline renders: Render <out.wav> <bars> [key=value ...]
// keys: style (0-3, applies its feel), slot, roll=<seed>, swing, ghosts, loosen, variety, dust, tempo,
//       straight=1 (swing 50, no push, no loosen, no ghosts, no variety), fillbar=<bar index>
#include "../Source/PluginProcessor.h"
#include <iostream>
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if (argc < 3) { std::cerr << "usage\n"; return 1; }
    ShuffleBuddyProcessor proc; const double sr = 48000; const int bs = 256;
    proc.setPlayConfigDetails (0, 2, sr, bs);
    auto set = [&] (const juce::String& id, float v) { auto* p = proc.apvts.getParameter (id); p->setValueNotifyingHost (p->convertTo0to1 (v)); };
    const int bars = juce::String (argv[2]).getIntValue();
    int fillBar = -1;
    std::vector<std::pair<juce::String, juce::String>> kv;
    for (int i = 3; i < argc; ++i) { juce::String a (argv[i]); kv.push_back ({ a.upToFirstOccurrenceOf ("=", false, false), a.fromFirstOccurrenceOf ("=", false, false) }); }
    int throwBar = -1, sweepBar = -1; double tailSec = 1.0; juce::String midiPath, recPath; int midiOutNotes = 0;
    for (auto& [k, v] : kv) if (k == "style") proc.applyStyleFeel (v.getIntValue());
    for (auto& [k, v] : kv)
    {
        if (k == "slot") set ("slot", v.getFloatValue());
        else if (k == "roll") { juce::Random r (v.getLargeIntValue()); proc.setGrid (proc.currentSlot(), groove::roll ((int) proc.apvts.getRawParameterValue ("style")->load(), r)); }
        else if (k == "straight") { set ("swing", 50); set ("loosen", 0); set ("ghosts", 0); set ("variety", 0); for (int p = 0; p < NumPads; ++p) set (ShuffleBuddyProcessor::pid ("push", p), 0); }
        else if (k == "fillbar") fillBar = v.getIntValue();
        else if (k == "midi") { midiPath = v; }
        else if (k == "rec") { recPath = v; }
        else if (k == "cellx") { auto t = juce::StringArray::fromTokens (v, ",", ""); auto c = proc.cellData (0, t[0].getIntValue(), t[1].getIntValue()); c.ratchet = (juce::uint8) t[2].getIntValue(); c.nudge = (juce::int8) t[3].getIntValue(); c.prob = (juce::uint8) t[4].getIntValue(); proc.setCellData (0, t[0].getIntValue(), t[1].getIntValue(), c); }
        else if (k == "tomrow")   // 16 chars: F L M H = floor/low/mid/high tom, '.' = rest
            for (int st = 0; st < juce::jmin (16, v.length()); ++st)
            {
                const int a = juce::String ("FLMH").indexOfChar (v[st]);
                auto c = proc.cellData (proc.currentSlot(), Toms, st);
                c.vel = a >= 0 ? (juce::uint8) 100 : (juce::uint8) 0; c.prob = 100; if (a >= 0) c.alt = (juce::uint8) a;
                proc.setCellData (proc.currentSlot(), Toms, st, c);
            }
        else if (k == "genre") { proc.loadGenre (v.getIntValue()); set ("chain", 2); }
        else if (k == "kick") proc.applyKickPreset (v.getIntValue());
        else if (k == "kit") proc.applyKit (v.getIntValue());
        else if (k == "surprise") { for (int q = 0; q < v.getIntValue(); ++q) proc.surpriseKick(); }
        else if (k == "throwbar") throwBar = v.getIntValue();
        else if (k == "sweepbar") sweepBar = v.getIntValue();
        else if (k == "tail") tailSec = v.getDoubleValue();
        else if (k == "grid" && v.length() == NumPads * 16)
            for (int i = 0; i < v.length(); ++i) proc.setCell (proc.currentSlot(), i / 16, i % 16, (juce::uint8) (v[i] - '0'));
        else if (k != "style" && k != "log" && k != "chainlog" && k != "midi" && k != "rec" && k != "cellx" && k != "genre" && k != "tomrow") set (k, v.getFloatValue());
    }
    set ("play", 1);
    proc.prepareToPlay (sr, bs);
    if (recPath.isNotEmpty()) proc.startRecording();
    const double bpm = proc.apvts.getRawParameterValue ("tempo")->load();
    const double barLen = sr * 60.0 / bpm * 4.0;
    const int total = (int) (barLen * bars + sr * tailSec);
    juce::AudioBuffer<float> out (2, total), blk (2, bs);
    bool logBars = false; for (auto& [k, v] : kv) if (k == "log") logBars = true;
    std::array<int, NumPads> lastHits {}; int lastBar = 0;
    for (int pos = 0; pos < total; pos += bs)
    {
        if (fillBar >= 0) set ("fill", pos >= barLen * fillBar && pos < barLen * (fillBar + 1) ? 1.0f : 0.0f);
        if (throwBar >= 0) set ("throw", pos >= barLen * throwBar && pos < barLen * throwBar + barLen * 0.5 ? 1.0f : 0.0f);
        if (sweepBar >= 0 && pos >= barLen * sweepBar && pos < barLen * (sweepBar + 2)) { const double t = (pos - barLen * sweepBar) / (barLen * 2); set ("fxfilter", (float) (t < 0.5 ? -t * 1.8 : (t - 0.5) * 1.8)); }
        else if (sweepBar >= 0) set ("fxfilter", 0.0f);
        if (pos >= barLen * bars) set ("play", 0);
        juce::MidiBuffer m; proc.processBlock (blk, m);
        if (recPath.isNotEmpty()) proc.pollRecording();
        for (const auto meta : m) if (meta.getMessage().isNoteOn()) ++midiOutNotes;
        if (logBars && (int) (pos / barLen) != lastBar)
        {
            std::cout << "bar " << lastBar + 1 << ":";
            for (int p = 0; p < NumPads; ++p) { std::cout << " " << ShuffleBuddyProcessor::padName (p) << "=" << proc.padHits (p) - lastHits[(size_t) p]; lastHits[(size_t) p] = proc.padHits (p); }
            std::cout << "  slot " << proc.playingSlot() << "  GR " << proc.squashGR() << "dB\n"; lastBar = (int) (pos / barLen);
        }
        for (int c = 0; c < 2; ++c) out.copyFrom (c, pos, blk, c, 0, juce::jmin (bs, total - pos));
    }
    std::cout << "plugin MIDI out: " << midiOutNotes << " note-ons\n";
    if (recPath.isNotEmpty()) { proc.stopRecording(); std::cout << "recorded " << proc.recordedBars() << " bars, " << proc.recordedSequence().getNumEvents() / 2 << " notes\n"; ShuffleBuddyProcessor::writeMidiFile (proc.recordedSequence(), 132, juce::File (recPath)); }
    if (midiPath.isNotEmpty()) { auto seq = proc.printBars (8); std::cout << "printed " << seq.getNumEvents() / 2 << " notes -> " << (ShuffleBuddyProcessor::writeMidiFile (seq, proc.tempoNow() > 0 ? proc.apvts.getRawParameterValue ("tempo")->load() : 120, juce::File (midiPath)) ? midiPath : juce::String ("FAILED")) << "\n"; }
    juce::File f (argv[1]); f.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), sr, 2, 24, {}, 0));
    w->writeFromAudioSampleBuffer (out, 0, total);
    float pk = out.getMagnitude (0, total); bool finite = true;
    for (int c = 0; c < 2; ++c) for (int i = 0; i < total; ++i) if (! std::isfinite (out.getSample (c, i))) finite = false;
    std::cout << f.getFileName() << "  " << total / sr << " s  peak " << pk << "  rms " << out.getRMSLevel (0, 0, total) << (finite ? "" : "  NaN!") << "\n";
}
