#pragma once
#include "PluginProcessor.h"

namespace toy
{
    juce::Font font (float h);      // Fredoka Bold
    juce::Font fontMed (float h);   // Fredoka Medium
    juce::Font lcd (float h);       // VT323

    const juce::Colour ink      { 0xff1f2333 };
    const juce::Colour yellow   { 0xffffd23f };
    const juce::Colour yellowHi { 0xffffe582 };
    const juce::Colour yellowLo { 0xffe8b213 };
    const juce::Colour yellowSh { 0xffc9970e };
    const juce::Colour brown    { 0xff6b4a00 };
    const juce::Colour cream    { 0xfffffaf0 };
    const juce::Colour lcdBg    { 0xffa6d96a };
    const juce::Colour lcdInk   { 0xff20331a };
    const juce::Colour bezel    { 0xff33405e };
    const juce::Colour red      { 0xffe63b2e };
    const juce::Colour blue     { 0xff2b6cf0 };
    const juce::Colour purple   { 0xff6c4bd8 };
    const juce::Colour green    { 0xff18a058 };
    const juce::Colour orange   { 0xffff7a1a };
    const juce::Colour pink     { 0xffe0457b };
    const juce::Colour teal     { 0xff0f9fb5 };
    const juce::Colour olive    { 0xff8a6d1f };
    // DubChord's FX colour language: echo orange, space violet, filter magenta
    const juce::Colour echoCol   { 0xffff8a1f };
    const juce::Colour spaceCol  { 0xff9b6bff };
    const juce::Colour filterCol { 0xffe83fd0 };
    const juce::Colour squashCol { 0xff22c7a9 };
    const juce::Colour deck      { 0xff283252 };
    const juce::Colour deckLo    { 0xff161c30 };
    inline juce::Colour padColour (int p) { const juce::Colour c[] = { red, blue, purple, green, orange, pink, teal, olive }; return c[juce::jlimit (0, 7, p)]; }
}

class ToyLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ToyLookAndFeel();
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    juce::Font getLabelFont (juce::Label& l) override { return l.getFont(); }
    juce::Font getPopupMenuFont() override { return toy::fontMed (17.0f); }
};

class LcdDisplay : public juce::Component
{
public:
    explicit LcdDisplay (ShuffleBuddyProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    int selectedPad = -1;
    juce::uint32 padShownAt = 0;
    void flash (const juce::String& a, const juce::String& b) { msgA = a; msgB = b; msgAt = juce::Time::getMillisecondCounter(); repaint(); }
    juce::String msgA, msgB;
    juce::uint32 msgAt = 0;
private:
    ShuffleBuddyProcessor& proc;
};

class StepGrid : public juce::Component
{
public:
    explicit StepGrid (ShuffleBuddyProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    std::function<void (int)> onSelectPad;
    int lastStep = -1;
private:
    struct Hit { int pad = -1, step = -1; bool push = false; bool label = false; };
    Hit hitTest (juce::Point<int>) const;
    juce::Rectangle<float> cellRect (int pad, int step) const;
    juce::Rectangle<float> pushRect (int pad) const;
    juce::Rectangle<float> labelRect (int pad) const;
    ShuffleBuddyProcessor& proc;
    int paintValue = -1, dragPushPad = -1;
    float dragStartY = 0, dragStartPush = 0;
    std::array<std::array<bool, 16>, NumPads> painted {};
};

class FxMeter : public juce::Component, public juce::SettableTooltipClient
{
public:
    std::function<float()> value;   // 0..1
    juce::Colour colour;
    bool vertical = true;
    void paint (juce::Graphics&) override;
    float shown = -1.0f;
};

class PadButton : public juce::Component
{
public:
    PadButton (ShuffleBuddyProcessor& p, int index) : proc (p), pad (index) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void tick();
    std::function<void (int)> onSelect;
    bool selected = false, dropHover = false;
private:
    ShuffleBuddyProcessor& proc;
    int pad;
    int lastHits = 0;
    float flash = 0.0f;
    bool down = false;
};

class ShuffleBuddyEditor : public juce::AudioProcessorEditor, private juce::Timer, public juce::FileDragAndDropTarget
{
public:
    explicit ShuffleBuddyEditor (ShuffleBuddyProcessor&);
    ~ShuffleBuddyEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    void mouseDown (const juce::MouseEvent&) override;

    bool isInterestedInFileDrag (const juce::StringArray&) override { return true; }
    void fileDragMove (const juce::StringArray&, int x, int y) override;
    void fileDragExit (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int x, int y) override;

private:
    void timerCallback() override;
    void selectPad (int pad);
    void setParam (const juce::String& id, float value);
    juce::Slider& makeKnob (const juce::String& id, const juce::String& label, juce::Colour c, bool onDeck = false);
    void learnMenu (const juce::String& paramId, juce::Component* target);
    void refreshMappings();
    void doUndo();

    ShuffleBuddyProcessor& proc;
    ToyLookAndFeel lnf;
    LcdDisplay lcd;
    StepGrid grid;
    std::vector<std::unique_ptr<PadButton>> pads;

    struct Knob { std::unique_ptr<juce::Slider> s; std::unique_ptr<juce::Label> name; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att; juce::String id; };
    std::vector<Knob> knobs;          // groove knobs (bottom row) then FX deck knobs
    std::vector<Knob> padKnobs;       // tune / decay / level / echo / space for the selected pad
    int selected = 1;

    juce::TextButton styleBtns[4], rollBtn { "ROLL" }, varyBtn { "VARY" }, fillBtn { "FILL" };
    juce::TextButton playBtn, stopBtn, slotBtns[4], tempoDown { "-" }, tempoUp { "+" };
    juce::TextButton undoBtn { "UNDO" }, chainBtn { "CHAIN" }, throwBtn { "THROW" };
    juce::TextButton timeBtns[4], typeBtns[3];
    FxMeter grMeter, echoLed;
    int lastMapVersion = -1, lastLearn = -2;
    juce::Label padEditLabel, tempoLabel, hint;
    juce::Image backdrop;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShuffleBuddyEditor)
};
