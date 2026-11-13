// Level/character audit: renders one hit of every kick preset and every library sound, dry, at full level.
// Audit <outdir>   -> kick_<i>.wav, kick_default.wav, snd_<i>.wav (+ snd_<i>_hi.wav for toms), index.tsv
#include "../Source/PluginProcessor.h"
#include <iostream>
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if (argc < 2) return 1;
    const bool verify = argc > 2 && juce::String (argv[2]) == "verify";   // keep the preset trims (to check the match)
    const int takes = 5;   // round robins + random noise: average a few hits
    juce::File dir (argv[1]); dir.createDirectory();
    const double sr = 48000; const int bs = 256;
    ShuffleBuddyProcessor proc; proc.setPlayConfigDetails (0, 2, sr, bs);
    auto set = [&] (const juce::String& id, float v) { auto* p = proc.apvts.getParameter (id); p->setValueNotifyingHost (p->convertTo0to1 (v)); };
    set ("play", 0); set ("squash", 0); set ("dust", 0); set ("echo", 0); set ("space", 0); set ("krumble", 0);
    for (int p = 0; p < NumPads; ++p)
    {
        set (ShuffleBuddyProcessor::pid ("lvl", p), 0.5f); set (ShuffleBuddyProcessor::pid ("esend", p), 0); set (ShuffleBuddyProcessor::pid ("ssend", p), 0);
        set (ShuffleBuddyProcessor::pid ("tune", p), 0); set (ShuffleBuddyProcessor::pid ("decay", p), 0.45f);
    }
    proc.prepareToPlay (sr, bs);
    juce::WavAudioFormat wav;
    juce::String index;
    auto render = [&] (const juce::String& name, int pad, int note, float vel, double secs)
    {
        // silence first so nothing rings over
        juce::AudioBuffer<float> blk (2, bs);
        for (int i = 0; i < (int) (sr * 0.6 / bs); ++i) { juce::MidiBuffer m; proc.processBlock (blk, m); }
        const int total = (int) (sr * secs);
        juce::AudioBuffer<float> out (2, total);
        for (int pos = 0; pos < total; pos += bs)
        {
            juce::MidiBuffer m;
            if (pos == 0) { if (note >= 0) m.addEvent (juce::MidiMessage::noteOn (1, note, vel), 0); else proc.auditionPad (pad, vel); }
            proc.processBlock (blk, m);
            for (int c = 0; c < 2; ++c) out.copyFrom (c, pos, blk, c, 0, juce::jmin (bs, total - pos));
        }
        juce::File f = dir.getChildFile (name + ".wav"); f.deleteFile();
        std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), sr, 2, 24, {}, 0));
        w->writeFromAudioSampleBuffer (out, 0, total);
    };
    for (int k = 0; k < takes; ++k) render ("kick_default_r" + juce::String (k), Kick, -1, 1.0f, 1.5);
    index << "kick_default\tKick\tdefault\n";
    const auto& kp = sounds::kickPresets();
    for (size_t i = 0; i < kp.size(); ++i)
    {
        proc.applyKickPreset ((int) i); if (! verify) set ("kgain", 1.0f); set ("krumble", 0.0f);
        for (int k = 0; k < takes; ++k) render ("kick_" + juce::String ((int) i) + "_r" + juce::String (k), Kick, -1, 1.0f, 1.5);
        index << "kick_" << (int) i << "\tKick\t" << kp[i].name << "\n";
    }
    const auto& lib = sounds::library();
    for (size_t i = 0; i < lib.size(); ++i)
    {
        const juce::String cat (lib[i].cat);
        const int pad = cat == "Toms" ? (int) Toms : (int) Perc;
        set (ShuffleBuddyProcessor::pid ("snd", pad), (float) i);
        const double secs = cat.contains ("Cymbal") ? 2.0 : 1.2;
        for (int k = 0; k < takes; ++k) render ("snd_" + juce::String ((int) i) + "_r" + juce::String (k), pad, -1, 1.0f, secs);
        index << "snd_" << (int) i << "\t" << cat << "\t" << lib[i].name << "\n";
        if (cat == "Toms")
            for (int a = 0; a < 4; ++a) render ("snd_" + juce::String ((int) i) + "_alt" + juce::String (a), pad, proc.noteFor (Toms, a), 1.0f, 1.2);
    }
    dir.getChildFile ("index.tsv").replaceWithText (index);
    std::cout << "rendered " << kp.size() + 1 << " kicks, " << lib.size() << " sounds -> " << dir.getFullPathName() << "\n";
}
