// "Performance jam" video: a 4-bar 2-step phrase (patterns A-B-C-D chained) played for 32 bars,
// with a visible cursor making live moves: filter open, chain, swing, Kick Lab, echo + THROW, vary, ghosts, fill, build.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include "/home/claude/songs/videorec.h"
#include <iostream>

// the editor isn't on a real desktop, so "showing" means visible all the way up the tree
static bool shown (juce::Component* c) { for (; c != nullptr; c = c->getParentComponent()) if (! c->isVisible()) return false; return true; }
static juce::Component* findById (juce::Component* c, const juce::String& id)
{
    if (c->getComponentID() == id && shown (c)) return c;
    for (auto* k : c->getChildren()) if (auto* r = findById (k, id)) return r;
    return nullptr;
}
static juce::Button* findButton (juce::Component* c, const juce::String& textOrName)
{
    if (auto* b = dynamic_cast<juce::Button*> (c)) if (shown (b) && (b->getButtonText() == textOrName || (b->getButtonText().isEmpty() && b->getName() == textOrName))) return b;
    for (auto* k : c->getChildren()) if (auto* r = findButton (k, textOrName)) return r;
    return nullptr;
}
static groove::Grid parseGrid (std::initializer_list<const char*> rows)
{
    groove::Grid g {}; int p = 0;
    for (auto* r : rows) { for (int s = 0; s < 16 && r[s]; ++s) g[(size_t) p][(size_t) s] = (juce::uint8) (r[s] >= '0' && r[s] <= '3' ? r[s] - '0' : 0); ++p; }
    return g;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    ShuffleBuddyProcessor proc; const double sr = 48000;
    proc.setPlayConfigDetails (0, 2, sr, 160);
    auto setP = [&] (const juce::String& id, float v) { proc.setParamValue (id, v); };
    auto getP = [&] (const juce::String& id) { return proc.apvts.getRawParameterValue (id)->load(); };

    // ---- the kit: deep 2-step with recorded layers
    const char* sounds[] = { "", "Fat Snare", "Room Clap", "Dusty Rim", "Real Closed Hat", "Dusty Open Hat", "Foley Knock", "Rattle" };
    for (int p = 1; p < NumPads; ++p) setP (ShuffleBuddyProcessor::pid ("snd", p), (float) sounds::find (sounds[p]));
    proc.applyKickPreset (0);   // Garage Thud
    const float lv[] = { 0.85f, 0.8f, 0.55f, 0.6f, 0.7f, 0.55f, 0.65f, 0.4f };
    for (int p = 0; p < NumPads; ++p) setP (ShuffleBuddyProcessor::pid ("lvl", p), lv[p]);
    // ---- the 4-bar phrase
    proc.setGrid (0, parseGrid ({ "3.........2.....", "....3.......3...", "....2.......2...", "...1...1..1...1.",
                                  "1.2.1.2.1.2.1.2.", "..............2.", ".......1.....1..", "1111111111111111" }));
    proc.setGrid (1, parseGrid ({ "3.......1.2.....", "....3..1....3...", "....2.......2...", "..1....1..1..1..",
                                  "1.2.1.2.1.2.1.22", "......2.........", "...1.........1..", "1111111111111111" }));
    proc.setGrid (2, parseGrid ({ "3.....1...2.....", "....3.......3.1.", "....2.......2...", "...1...1..1...1.",
                                  "1.2.1.2.1.2.1.2.", "..............2.", ".......1.....1..", "1111111111111111" }));
    proc.setGrid (3, parseGrid ({ "3.........2...2.", "....3....1..3.21", "....2.......3...", "..1..1....1.1...",
                                  "1.2.1.2.1.2.2222", "..2...........2.", ".......1..1.....", "1111111111111111" }));
    setP ("tempo", 134); setP ("swing", 57); setP ("ghosts", 0.45f); setP ("loosen", 0.2f); setP ("variety", 0.35f);
    setP ("dust", 0.3f); setP ("squash", 0.45f); setP ("echo", 0.3f); setP ("space", 0.3f); setP ("echotime", 2);
    setP ("fxfilter", -0.85f); setP ("chain", 0); setP ("slot", 0); setP ("play", 1);
    proc.prepareToPlay (sr, 160);

    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    ed->setVisible (true);
    juce::MessageManager::getInstance()->runDispatchLoopUntil (300);

    const double bar = 60.0 / 134.0 * 4.0;
    enum Kind { Drag, Hold, Click };
    struct Act { double b0, b1; Kind kind; juce::String target, label; float v0 = 0, v1 = 0; bool done = false; };
    std::vector<Act> acts = {
        { 1.4, 6.0, Drag, "fxfilter", "FILTER OPEN", -0.85f, 0.0f },
        { 6.6, 6.7, Click, "CHAIN", "CHAIN A-B" },
        { 7.2, 7.3, Click, "A-B", "CHAIN A-B-C-D" },
        { 9.0, 11.0, Drag, "swing", "MORE SWING", 57.0f, 62.0f },
        { 13.0, 14.0, Drag, "swing", "SWING BACK", 62.0f, 58.0f },
        { 15.0, 15.1, Click, "KICK LAB", "KICK LAB" },
        { 16.0, 18.0, Drag, "kdrive", "KICK DRIVE", 0.25f, 0.62f },
        { 18.5, 20.0, Drag, "krumble", "RUMBLE", 0.0f, 0.4f },
        { 20.5, 20.6, Click, "GRID", "BACK TO GRID" },
        { 21.0, 22.8, Drag, "echo", "ECHO UP", 0.3f, 0.6f },
        { 23.5, 24.0, Hold, "throw", "THROW!" },
        { 24.3, 24.9, Drag, "echo", "ECHO DOWN", 0.6f, 0.3f },
        { 25.4, 25.5, Click, "VARY", "VARY PATTERN A" },
        { 26.4, 28.0, Drag, "ghosts", "MORE GHOSTS", 0.45f, 0.72f },
        { 28.45, 29.0, Hold, "FILL", "FILL" },
        { 29.6, 31.4, Drag, "fxfilter", "BUILD", 0.0f, 0.65f },
        { 31.5, 32.0, Hold, "throw", "THROW!" },
        { 32.05, 32.15, Click, "stop", "STOP" },
    };
    auto centreOf = [&] (const Act& a) -> juce::Point<float>
    {
        juce::Component* c = a.kind == Drag ? findById (ed.get(), a.target) : nullptr;
        if (c == nullptr && a.kind != Drag) c = a.target == "throw" ? findById (ed.get(), "throw") : findButton (ed.get(), a.target);
        if (c == nullptr) return { (float) ed->getWidth() * 0.5f, (float) ed->getHeight() * 0.5f };
        auto r = ed->getLocalArea (c, c->getLocalBounds()).toFloat();
        return { r.getCentreX() + (a.kind == Drag ? 4.0f : 10.0f), r.getCentreY() + (a.kind == Drag ? 6.0f : 4.0f) };
    };
    std::vector<juce::Point<float>> pos (acts.size());
    juce::Point<float> rest { 700.0f, 980.0f };
    auto ease = [] (double x) { x = juce::jlimit (0.0, 1.0, x); return x * x * (3.0 - 2.0 * x); };

    // per-frame: drive the moves on the message thread (like a real hand would)
    auto perVideoFrame = [&] (double t)
    {
        const double b = t / bar;
        for (size_t i = 0; i < acts.size(); ++i)
        {
            auto& a = acts[i];
            if (b < a.b0 - 1.0) continue;
            if (b < a.b0 + 0.01 || pos[i] == juce::Point<float>()) pos[i] = centreOf (a);   // follow the control until the move starts (it may only just have appeared)
            if (a.kind == Drag && b >= a.b0 && b <= a.b1 + 0.02)
                setP (a.target, a.v0 + (a.v1 - a.v0) * (float) ease ((b - a.b0) / (a.b1 - a.b0)));
            if (a.kind == Hold)
                if (auto* btn = a.target == "throw" ? dynamic_cast<juce::Button*> (findById (ed.get(), "throw")) : findButton (ed.get(), a.target))
                {
                    const bool down = b >= a.b0 && b < a.b1;
                    if (down && btn->getState() != juce::Button::buttonDown) btn->setState (juce::Button::buttonDown);
                    if (! down && b >= a.b1 && ! a.done) { btn->setState (juce::Button::buttonNormal); a.done = true; }
                }
            if (a.kind == Click && b >= a.b0 && ! a.done)
                if (auto* btn = findButton (ed.get(), a.target))
                {
                    if (b < a.b1) btn->setState (juce::Button::buttonDown);
                    else { btn->setState (juce::Button::buttonNormal); if (btn->onClick) btn->onClick(); a.done = true; }
                }
        }
    };

    // the hand: glides to each control, presses, drags; a small label says what the move is
    auto overlay = [&] (juce::Graphics& g, double t)
    {
        const double b = t / bar;
        juce::Point<float> p = rest; bool pressing = false; juce::String label; float dragUp = 0.0f;
        int prev = -1, next = -1;
        for (int i = 0; i < (int) acts.size(); ++i) { if (acts[(size_t) i].b0 <= b) prev = i; else { next = i; break; } }
        if (prev >= 0)
        {
            const auto& a = acts[(size_t) prev];
            p = pos[(size_t) prev];
            if (b <= a.b1 + 0.15)
            {
                pressing = b <= a.b1; label = a.label;
                if (a.kind == Drag) dragUp = 26.0f * (float) ease ((b - a.b0) / (a.b1 - a.b0)) * (a.v1 >= a.v0 ? 1.0f : -1.0f);
            }
        }
        if (next >= 0)
        {
            const auto& n = acts[(size_t) next];
            const double glide = 0.45;   // bars
            const double from = prev >= 0 ? juce::jmax (acts[(size_t) prev].b1 + 0.1, n.b0 - glide) : n.b0 - glide;
            if (b > from && pos[(size_t) next] != juce::Point<float>())
            {
                const auto a = prev >= 0 ? pos[(size_t) prev].translated (0.0f, prev >= 0 && acts[(size_t) prev].kind == Drag ? -26.0f * (acts[(size_t) prev].v1 >= acts[(size_t) prev].v0 ? 1.0f : -1.0f) : 0.0f) : rest;
                const float k = (float) ease ((b - from) / (n.b0 - from));
                p = a + (pos[(size_t) next] - a) * k;
                dragUp = 0.0f;
                pressing = false; label = {};
            }
        }
        p = p.translated (0.0f, -dragUp);
        if (pressing)
        {
            g.setColour (juce::Colours::white.withAlpha (0.55f));
            g.drawEllipse (p.x - 22, p.y - 22, 44, 44, 3.0f);
            g.setColour (juce::Colour (0x40ff7a1a));
            g.fillEllipse (p.x - 16, p.y - 16, 32, 32);
        }
        juce::Path arrow;
        arrow.startNewSubPath (0, 0); arrow.lineTo (0, 26); arrow.lineTo (6.5f, 20); arrow.lineTo (11, 30); arrow.lineTo (15, 28);
        arrow.lineTo (10.5f, 18.5f); arrow.lineTo (19, 18.5f); arrow.closeSubPath();
        auto tr = juce::AffineTransform::scale (1.25f).translated (p.x, p.y);
        g.setColour (juce::Colours::black.withAlpha (0.3f)); g.fillPath (arrow, tr.translated (3, 4));
        g.setColour (juce::Colours::white); g.fillPath (arrow, tr);
        g.setColour (juce::Colour (0xff1f2333)); g.strokePath (arrow, juce::PathStrokeType (2.2f), tr);
        if (label.isNotEmpty())
        {
            g.setFont (toy::font (18.0f));
            const float w = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), label) + 26.0f;
            juce::Rectangle<float> chip (p.x + 30, p.y + 22, w, 32);
            if (chip.getRight() > ed->getWidth() - 10) chip.setX (p.x - w - 14);
            g.setColour (juce::Colour (0xff1f2333)); g.fillRoundedRectangle (chip.translated (0, 3), 16);
            g.setColour (juce::Colour (0xffff7a1a)); g.fillRoundedRectangle (chip, 16);
            g.setColour (juce::Colours::white); g.drawText (label, chip, juce::Justification::centred);
        }
    };

    const bool ok = recordEditor (proc, *ed, bar * 32.0 + 2.0, argc > 1 ? argv[1] : "/tmp/jam.mp4",
        [&] (double, juce::MidiBuffer&) {}, sr, perVideoFrame, overlay);
    std::cout << (ok ? "wrote video\n" : "FAILED\n");
    return ok ? 0 : 1;
}
