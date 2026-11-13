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
static groove::Grid parseGrid (std::initializer_list<const char*> rows) { return groove::fromLevels (rows); }

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
    const float lv[] = { 0.85f, 0.6f, 0.42f, 0.6f, 0.72f, 0.55f, 0.65f, 0.42f };
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
    // start from the bare pattern: straight, nothing added, then switch the groove tools on one at a time
    setP ("tempo", 134); setP ("swing", 50); setP ("ghosts", 0.0f); setP ("loosen", 0.0f); setP ("variety", 0.0f);
    setP ("dust", 0.25f); setP ("squash", 0.4f); setP ("echo", 0.25f); setP ("space", 0.25f); setP ("echotime", 2);
    setP ("fxfilter", 0.0f); setP ("chain", 0); setP ("slot", 0); setP ("play", 1);
    proc.prepareToPlay (sr, 160);

    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    ed->setVisible (true);
    juce::MessageManager::getInstance()->runDispatchLoopUntil (300);

    const double bar = 60.0 / 134.0 * 4.0;
    enum Kind { Drag, Hold, Click };
    struct Act { double b0, b1; Kind kind; juce::String target, label; float v0 = 0, v1 = 0; bool done = false; };
    std::vector<Act> acts = {
        { 4.3, 6.6, Drag, "swing", "SWING 50% > 62%", 50.0f, 62.0f },
        { 8.3, 9.8, Drag, "ghosts", "GHOSTS 0% > 85%", 0.0f, 0.85f },
        { 11.6, 11.95, Drag, "ghosts", "GHOSTS > 40%", 0.85f, 0.4f },
        { 12.3, 13.8, Drag, "variety", "VARIETY 0% > 75%", 0.0f, 0.75f },
        { 17.35, 17.95, Hold, "FILL", "HOLD FILL" },
        { 19.35, 19.95, Hold, "FILL", "HOLD FILL" },
        { 20.3, 21.6, Drag, "loosen", "LOOSEN 0% > 55%", 0.0f, 0.55f },
        { 23.6, 23.7, Click, "CHAIN", "CHAIN A-B" },
        { 23.85, 23.95, Click, "A-B", "CHAIN A-B-C-D" },
        { 28.2, 29.3, Drag, "echo", "ECHO UP", 0.25f, 0.5f },
        { 29.4, 29.95, Hold, "FILL", "HOLD FILL" },
        { 31.4, 31.95, Hold, "throw", "THROW!" },
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
        // ---- the lesson strip under the machine
        {
            struct Cap { double b; const char* title; const char* text; };
            static const Cap caps[] = {
                { 0,  "1  THE PATTERN", "Pattern A exactly as programmed: straight timing, nothing added." },
                { 4,  "2  SWING", "The off-beat 16ths lean late. Watch the grid lean right: 50% straight, 62% shuffle." },
                { 8,  "3  GHOSTS", "Quiet snares and hats appear in the gaps (blue boxes). Different ones every bar." },
                { 12, "4  VARIETY", "Every bar plays its own version: green = added, X = dropped. Your pattern is never changed." },
                { 16, "5  FILL", "Hold FILL and the second half of the bar breaks into a fill (orange boxes)." },
                { 20, "6  LOOSEN", "Human timing: hits drift a few ms early and late, velocities wobble. Listen to it relax." },
                { 24, "7  CHAIN", "Patterns A, B, C, D play one bar each: a 4-bar phrase, still with ghosts + variety on top." },
                { 28, "8  ALL TOGETHER", "Swing + ghosts + variety + fills + loosen + chain, then echo and a THROW to finish." },
            };
            int ci = 0; for (int i = 0; i < 8; ++i) if (b >= caps[i].b) ci = i;
            const float top = (float) ed->getHeight();
            const float fadeIn = (float) juce::jlimit (0.0, 1.0, (b - caps[ci].b) / 0.25);
            g.setColour (juce::Colour (0xff1f2333)); g.fillRect (0.0f, top, (float) ed->getWidth(), 130.0f);
            for (int i = 0; i < 8; ++i)   // progress: eight lessons
            {
                const float x = 40.0f + (float) i * 150.0f;
                g.setColour (i < ci ? juce::Colour (0xffffd23f) : i == ci ? juce::Colour (0xffff7a1a) : juce::Colour (0xff3a4566));
                g.fillRoundedRectangle (x, top + 16.0f, 138.0f, 8.0f, 4.0f);
            }
            g.setColour (juce::Colour (0xffffd23f).withAlpha (fadeIn));
            g.setFont (toy::font (36.0f));
            g.drawText (caps[ci].title, juce::Rectangle<float> (40.0f, top + 34.0f, 400.0f, 46.0f), juce::Justification::centredLeft);
            g.setColour (juce::Colours::white.withAlpha (fadeIn));
            g.setFont (toy::fontMed (22.0f));
            g.drawText (caps[ci].text, juce::Rectangle<float> (40.0f, top + 80.0f, (float) ed->getWidth() - 80.0f, 34.0f), juce::Justification::centredLeft);
        }
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
        [&] (double, juce::MidiBuffer&) {}, sr, perVideoFrame, overlay, 130);
    std::cout << (ok ? "wrote video\n" : "FAILED\n");
    return ok ? 0 : 1;
}
