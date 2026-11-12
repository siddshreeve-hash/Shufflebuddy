#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <iostream>
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    ShuffleBuddyProcessor proc; const double sr = 48000; const int bs = 256;
    proc.setPlayConfigDetails (0, 2, sr, bs);
    auto set = [&] (const char* id, float v) { auto* p = proc.apvts.getParameter (id); p->setValueNotifyingHost (p->convertTo0to1 (v)); };
    set ("play", 1);
    proc.prepareToPlay (sr, bs);
    juce::AudioBuffer<float> b (2, bs);
    const double bar = sr * 60.0 / 132.0 * 4.0;
    set ("squash", 0.55f); set ("chain", 1); set ("fxfilter", -0.3f);
    proc.startLearn (proc.apvts.getParameter ("echo")->getParameterIndex());
    { juce::MidiBuffer m; m.addEvent (juce::MidiMessage::controllerEvent (1, 21, 50), 0); proc.processBlock (b, m); }
    for (int k = 0; k < (int) ((bar * 2 + bar * 9.4 / 16.0) / bs); ++k) { juce::MidiBuffer m; proc.processBlock (b, m); }
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    ed->setVisible (true);
    if (argc > 2 && juce::String (argv[2]) == "lab")
    {
        proc.applyKit (5);   // dub techno: shows rumble + duck
        ed->keyPressed (juce::KeyPress ('k', {}, 'k'));
    }
    juce::MessageManager::getInstance()->runDispatchLoopUntil (120);
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    juce::File f (argc > 1 ? argv[1] : "/tmp/sb_ui.png"); f.deleteFile();
    juce::FileOutputStream os (f); juce::PNGImageFormat png; png.writeImageToStream (img, os);
    std::cout << "wrote " << f.getFullPathName() << "\n";
}
