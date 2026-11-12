#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Wraps a fixed-size editor so the window can be any size: it opens small enough to fit the
// screen, can be dragged bigger or smaller (aspect ratio kept), and remembers the size you chose.
template <typename Inner, typename Proc>
class ScaledEditor : public juce::AudioProcessorEditor
{
public:
    explicit ScaledEditor (Proc& p) : juce::AudioProcessorEditor (&p), proc (p), inner (p)
    {
        addAndMakeVisible (inner);
        baseW = inner.getWidth(); baseH = inner.getHeight();
        float scale = (float) proc.apvts.state.getProperty ("uiScale", 0.0);
        if (scale <= 0.0f)   // first time: fit 90% of the screen, never bigger than 100%
        {
            scale = 1.0f;
            if (auto* d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
            {
                const auto area = d->userArea.toFloat();
                scale = juce::jmin (1.0f, area.getWidth() * 0.92f / (float) baseW, (area.getHeight() - 60.0f) * 0.92f / (float) baseH);
            }
        }
        scale = juce::jlimit (0.45f, 1.6f, scale);
        setResizable (true, true);
        setResizeLimits ((int) (baseW * 0.45f), (int) (baseH * 0.45f), (int) (baseW * 1.6f), (int) (baseH * 1.6f));
        getConstrainer()->setFixedAspectRatio ((double) baseW / (double) baseH);
        setSize (juce::roundToInt (baseW * scale), juce::roundToInt (baseH * scale));
    }
    void resized() override
    {
        const float s = (float) getWidth() / (float) baseW;
        inner.setBounds (0, 0, baseW, baseH);
        inner.setTransform (juce::AffineTransform::scale (s));
        proc.apvts.state.setProperty ("uiScale", s, nullptr);
    }
    bool keyPressed (const juce::KeyPress& k) override { return inner.keyPressed (k); }
    Inner& content() { return inner; }
private:
    Proc& proc;
    Inner inner;
    int baseW = 100, baseH = 100;
};
