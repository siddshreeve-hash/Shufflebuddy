// Shuffle Buddy offline song render from a timeline (see /home/claude/songs/timeline.h).
// Extra commands:  grid SLOT kick snare clap rim hat open perc shaker   (16 chars each, 0-3, '.' = 0)
//                  kit N | kickpreset N | style N        (applied at their bar)
//                  only PAD   (render just this pad: 1-8)  |  without PAD
#include "../Source/PluginProcessor.h"
#include "/home/claude/songs/timeline.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <iostream>
int main (int argc, char** argv)
{
    if (argc < 3) { std::cerr << "SongRender spec.txt out.wav [only=N|without=N]\n"; return 1; }
    juce::ScopedJuceInitialiser_GUI init;
    ShuffleBuddyProcessor proc; const double sr = 48000; const int bs = 128;
    proc.setPlayConfigDetails (0, 2, sr, bs);
    Timeline tl; tl.load (juce::File (argv[1]));
    auto setP = [&] (const juce::String& id, float v) { if (auto* p = proc.apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v)); else std::cerr << "no param " << id << "\n"; };
    int only = -1, without = -1;
    for (int i = 3; i < argc; ++i) { juce::String a (argv[i]); if (a.startsWith ("only=")) only = a.fromFirstOccurrenceOf ("=", false, false).getIntValue() - 1; if (a.startsWith ("without=")) without = a.fromFirstOccurrenceOf ("=", false, false).getIntValue() - 1; }
    std::vector<float> lvl (NumPads, 0.8f);
    auto applyMute = [&]
    {
        for (int p = 0; p < NumPads; ++p)
        {
            const auto id = ShuffleBuddyProcessor::pid ("lvl", p);
            if (p == only || (only < 0 && p != without)) {} else setP (id, 0.0f);
        }
    };
    auto runCmd = [&] (const juce::StringArray& t)
    {
        if (t[0] == "grid" && t.size() >= 10)
        {
            groove::Grid g {};
            for (int p = 0; p < NumPads; ++p)
                for (int s = 0; s < 16 && s < t[p + 2].length(); ++s) { const auto c = t[p + 2][s]; g[(size_t) p][(size_t) s] = (juce::uint8) (c >= '0' && c <= '3' ? c - '0' : 0); }
            proc.setGrid (t[1].getIntValue(), g);
        }
        else if (t[0] == "kit") proc.applyKit (t[1].getIntValue());
        else if (t[0] == "kickpreset") proc.applyKickPreset (t[1].getIntValue());
        else if (t[0] == "style") proc.applyStyleFeel (t[1].getIntValue());
        else std::cerr << "unknown cmd " << t[0] << "\n";
        applyMute();
    };
    size_t si = 0, ci = 0;
    while (ci < tl.cmds.size() && tl.cmds[ci].bar <= 0) runCmd (tl.cmds[ci++].args);
    setP ("tempo", (float) tl.bpm);
    while (si < tl.sets.size() && tl.sets[si].bar <= 0) { setP (tl.sets[si].id, tl.sets[si].v); ++si; }
    applyMute();
    proc.prepareToPlay (sr, bs);
    const double barLen = sr * 60.0 / tl.bpm * 4.0;
    const int total = (int) (barLen * tl.bars + tl.tail * sr);
    struct Ev { int at; juce::MidiMessage m; }; std::vector<Ev> ev;
    for (auto& n : tl.notes) ev.push_back ({ (int) (n.bar * barLen), juce::MidiMessage::noteOn (1, n.note, (juce::uint8) n.vel) });
    std::sort (ev.begin(), ev.end(), [] (auto& a, auto& b) { return a.at < b.at; });
    juce::AudioBuffer<float> out (2, total), blk (2, bs);
    size_t ei = 0;
    for (int pos = 0; pos < total; pos += bs)
    {
        // events land a little before the bar so the sequencer's next-bar logic picks them up in time
        const double bar = (pos + 0.25 * barLen / 16.0) / barLen;
        while (ci < tl.cmds.size() && tl.cmds[ci].bar <= bar) runCmd (tl.cmds[ci++].args);
        // pattern switches must be in before the sequencer prepares the next bar (it looks ahead), so they go in half a beat early
        while (si < tl.sets.size() && tl.sets[si].bar <= bar + (tl.sets[si].id == "slot" ? 0.125 : 0.0)) { setP (tl.sets[si].id, tl.sets[si].v); ++si; }
        for (auto& r : tl.ramps) if (bar >= r.b0 && bar <= r.b1) setP (r.id, r.v0 + (r.v1 - r.v0) * (float) ((bar - r.b0) / juce::jmax (1e-6, r.b1 - r.b0)));
        if (pos >= barLen * tl.bars) setP ("play", 0.0f);
        juce::MidiBuffer midi;
        while (ei < ev.size() && ev[ei].at < pos + bs) { midi.addEvent (ev[ei].m, juce::jmax (0, ev[ei].at - pos)); ++ei; }
        proc.processBlock (blk, midi);
        for (int c = 0; c < 2; ++c) out.copyFrom (c, pos, blk, c, 0, juce::jmin (bs, total - pos));
    }
    juce::File f (argv[2]); f.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), sr, 2, 24, {}, 0));
    w->writeFromAudioSampleBuffer (out, 0, total);
    std::cout << f.getFileName() << " " << total / sr << " s peak " << out.getMagnitude (0, total) << "\n";
}
