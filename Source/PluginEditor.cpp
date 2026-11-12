#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
    constexpr int W = 1260, H = 1050;
    constexpr int gridLabelW = 92, gridPushW = 58, gridPad = 16, gridTop = 42, gridRowH = 44;
}

namespace toy
{
    static juce::Typeface::Ptr tfBold, tfMed, tfLcd;
    static void loadFonts()
    {
        if (tfBold == nullptr) tfBold = juce::Typeface::createSystemTypefaceFor (BinaryData::FredokaBold_ttf, BinaryData::FredokaBold_ttfSize);
        if (tfMed == nullptr)  tfMed  = juce::Typeface::createSystemTypefaceFor (BinaryData::FredokaMedium_ttf, BinaryData::FredokaMedium_ttfSize);
        if (tfLcd == nullptr)  tfLcd  = juce::Typeface::createSystemTypefaceFor (BinaryData::VT323Regular_ttf, BinaryData::VT323Regular_ttfSize);
    }
    juce::Font font (float h)    { loadFonts(); return juce::Font (juce::FontOptions (tfBold).withHeight (h)); }
    juce::Font fontMed (float h) { loadFonts(); return juce::Font (juce::FontOptions (tfMed).withHeight (h)); }
    juce::Font lcd (float h)     { loadFonts(); return juce::Font (juce::FontOptions (tfLcd).withHeight (h)); }
}

// ================================================================ look and feel
ToyLookAndFeel::ToyLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, toy::cream);
    setColour (juce::PopupMenu::textColourId, toy::ink);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, toy::yellow);
    setColour (juce::PopupMenu::highlightedTextColourId, toy::ink);
    setColour (juce::TooltipWindow::backgroundColourId, toy::cream);
    setColour (juce::TooltipWindow::textColourId, toy::ink);
}

void ToyLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    if (b.getName() == "throw")
    {
        // big hazard button: black/yellow stripes round a cream plate, sinks when held
        auto r = b.getLocalBounds().toFloat();
        const float depth = 10.0f, sink = down ? depth - 3.0f : 0.0f;
        auto face = r.withTrimmedBottom (depth).translated (0.0f, sink);
        g.setColour (juce::Colour (0xff8a6a00));
        g.fillRoundedRectangle (r.withTrimmedTop (depth), 26.0f);
        juce::Path clip; clip.addRoundedRectangle (face, 26.0f);
        {
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (clip);
            g.setColour (toy::yellow);
            g.fillRect (face);
            g.setColour (toy::ink);
            for (float x = face.getX() - face.getHeight(); x < face.getRight(); x += 36.0f)
            {
                juce::Path st;
                st.addQuadrilateral (x, face.getBottom(), x + 18.0f, face.getBottom(), x + 18.0f + face.getHeight(), face.getY(), x + face.getHeight(), face.getY());
                g.fillPath (st);
            }
        }
        auto plate = face.reduced (22.0f, 18.0f);
        g.setColour (down ? toy::echoCol : toy::cream);
        g.fillRoundedRectangle (plate, 16.0f);
        g.setColour (toy::ink);
        g.drawRoundedRectangle (plate, 16.0f, 3.0f);
        if (over && ! down) { g.setColour (juce::Colours::white.withAlpha (0.12f)); g.fillRoundedRectangle (face, 26.0f); }
        return;
    }
    if (b.getName() == "pill")
    {
        auto r = b.getLocalBounds().toFloat();
        const bool on = b.getToggleState();
        const float sink = (on || down) ? 2.0f : 0.0f;
        auto face = r.withTrimmedBottom (4.0f).translated (0.0f, sink);
        const auto col = b.findColour (juce::TextButton::buttonOnColourId);
        g.setColour (toy::deckLo);
        g.fillRoundedRectangle (r.withTrimmedTop (4.0f), 9.0f);
        g.setColour (on ? col : (over ? toy::deck.brighter (0.25f) : toy::deck.brighter (0.12f)));
        g.fillRoundedRectangle (face, 9.0f);
        if (on) { g.setColour (juce::Colours::white.withAlpha (0.25f)); g.fillRoundedRectangle (face.withHeight (face.getHeight() * 0.4f).reduced (4.0f, 2.0f), 5.0f); }
        return;
    }
    const auto base = b.findColour (juce::TextButton::buttonColourId);
    const float depth = 6.0f;
    auto r = b.getLocalBounds().toFloat();
    const bool pressed = down || (b.getToggleState() && b.getName() != "style");
    const float sink = pressed ? depth - 2.0f : 0.0f;
    auto face = r.withTrimmedBottom (depth).translated (0.0f, sink);
    const float rad = juce::jmin (face.getHeight() * 0.5f, b.getName() == "slot" ? 10.0f : 26.0f);
    g.setColour (base.darker (0.45f));
    g.fillRoundedRectangle (r.withTrimmedTop (depth), rad);
    g.setColour (over ? base.brighter (0.08f) : base);
    g.fillRoundedRectangle (face, rad);
    g.setColour (juce::Colours::white.withAlpha (0.22f));
    g.fillRoundedRectangle (face.withHeight (face.getHeight() * 0.42f).reduced (6.0f, 3.0f), rad * 0.6f);
    if (b.getToggleState() && b.getName() == "style")
    {
        g.setColour (juce::Colours::white);
        g.drawRoundedRectangle (face.reduced (3.0f), rad - 3.0f, 3.0f);
    }
}

void ToyLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool down)
{
    if (b.getName() == "throw")
    {
        auto face = b.getLocalBounds().toFloat().withTrimmedBottom (10.0f).translated (0.0f, down ? 7.0f : 0.0f).reduced (22.0f, 18.0f);
        g.setColour (toy::ink);
        g.setFont (toy::font (40.0f));
        g.drawText ("THROW!", face.withTrimmedBottom (18.0f), juce::Justification::centred);
        g.setFont (toy::font (12.0f));
        g.drawText (down ? "DUBBING IT OUT..." : "HOLD TO SEND EVERYTHING INTO THE ECHO", face.withTrimmedTop (face.getHeight() - 26.0f), juce::Justification::centred);
        return;
    }
    if (b.getName() == "pill")
    {
        const bool on = b.getToggleState();
        auto face = b.getLocalBounds().toFloat().withTrimmedBottom (4.0f).translated (0.0f, (on || down) ? 2.0f : 0.0f);
        g.setColour (on ? toy::deckLo : toy::cream.withAlpha (0.8f));
        g.setFont (toy::font (15.0f));
        g.drawText (b.getButtonText(), face, juce::Justification::centred);
        return;
    }
    const bool pressed = down || (b.getToggleState() && b.getName() != "style");
    auto face = b.getLocalBounds().toFloat().withTrimmedBottom (6.0f).translated (0.0f, pressed ? 4.0f : 0.0f);
    const auto txt = b.findColour (juce::TextButton::textColourOffId);
    if (! b.isEnabled()) g.setOpacity (0.35f);
    if (b.getName() == "play")
    {
        juce::Path p; const auto c = face.getCentre();
        p.addTriangle (c.x - 9, c.y - 12, c.x - 9, c.y + 12, c.x + 13, c.y);
        g.setColour (txt); g.fillPath (p); return;
    }
    if (b.getName() == "stop")
    {
        g.setColour (txt); g.fillRoundedRectangle (face.withSizeKeepingCentre (20, 20), 3.0f); return;
    }
    g.setColour (txt);
    g.setFont (toy::font (b.getName() == "slot" ? 17.0f : (face.getHeight() > 56 ? 26.0f : 20.0f)));
    g.drawText (b.getButtonText(), face, juce::Justification::centred);
}

void ToyLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& s)
{
    const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat();
    const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 6.0f;
    const auto c = bounds.getCentre().translated (0.0f, -2.0f);
    const auto col = s.findColour (juce::Slider::rotarySliderFillColourId);
    // tick dots around the knob
    const bool bipolar = s.getProperties()["bipolar"];
    const bool onDeck = s.getProperties()["deck"];
    for (int i = 0; i <= 10; ++i)
    {
        const float a = start + (end - start) * (float) i / 10.0f;
        const float f = (float) i / 10.0f;
        const bool lit = bipolar ? (pos < 0.5f ? (f >= pos - 0.05f && f <= 0.5f) : (f >= 0.5f && f <= pos + 0.05f)) : f <= pos + 0.001f;
        g.setColour (lit ? col : (onDeck ? toy::cream.withAlpha (0.18f) : toy::brown.withAlpha (0.3f)));
        g.fillEllipse (c.x + std::sin (a) * (r + 5.0f) - 2.0f, c.y - std::cos (a) * (r + 5.0f) - 2.0f, 4.0f, 4.0f);
    }
    const float kr = r - 2.0f;
    g.setColour (col.darker (0.45f));
    g.fillEllipse (c.x - kr, c.y - kr + 5.0f, kr * 2, kr * 2);
    g.setColour (col);
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
    g.setColour (juce::Colours::white.withAlpha (0.3f));
    g.fillEllipse (c.x - kr * 0.75f, c.y - kr * 0.92f, kr * 1.5f, kr * 0.9f);
    const float a = start + pos * (end - start);
    juce::Path pointer;
    pointer.addRoundedRectangle (-3.0f, -kr + 4.0f, 6.0f, kr * 0.62f, 3.0f);
    g.setColour (juce::Colours::white);
    g.fillPath (pointer, juce::AffineTransform::rotation (a).translated (c.x, c.y));
}

// ================================================================ LCD
void LcdDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (toy::ink);
    g.fillRoundedRectangle (r.translated (0, 6), 22.0f);
    g.setColour (toy::bezel);
    g.fillRoundedRectangle (r.withTrimmedBottom (6.0f), 22.0f);
    auto scr = r.withTrimmedBottom (6.0f).reduced (14.0f);
    g.setColour (toy::lcdBg);
    g.fillRoundedRectangle (scr, 12.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.22f), scr.getX(), scr.getY(), juce::Colours::transparentBlack, scr.getX(), scr.getY() + 14, false));
    g.fillRoundedRectangle (scr, 12.0f);

    auto pv = [this] (const char* id) { return proc.apvts.getRawParameterValue (id)->load(); };
    const int slot = (int) pv ("slot");
    const bool playing = pv ("play") > 0.5f;
    auto t = scr.reduced (14.0f, 8.0f);
    g.setColour (toy::lcdInk);
    auto line = [&] (float h, const juce::String& left, const juce::String& right, float size)
    {
        auto lr = t.removeFromTop (h);
        g.setFont (toy::lcd (size));
        g.drawText (left, lr, juce::Justification::centredLeft);
        if (right.isNotEmpty()) g.drawText (right, lr, juce::Justification::centredRight);
    };
    const int chain = (int) pv ("chain");
    const juce::String chainTxt = chain == 0 ? juce::String() : (playing ? "PLAYING " + juce::String::charToString ((juce::juce_wchar) ('A' + proc.playingSlot())) : juce::String ("CHAIN ") + proc.apvts.getParameter ("chain")->getCurrentValueAsText());
    line (24.0f, "PATTERN " + juce::String::charToString ((juce::juce_wchar) ('A' + slot)), chainTxt.isNotEmpty() ? chainTxt : juce::String (juce::roundToInt (pv ("tempo"))) + " BPM", 26.0f);
    line (34.0f, groove::styleName ((int) pv ("style")), {}, 34.0f);
    const bool showPad = selectedPad >= 0 && juce::Time::getMillisecondCounter() - padShownAt < 3000;
    const int learn = proc.learningParam();
    if (learn >= 0 && learn < proc.getParameters().size())
    {
        const bool blink = (juce::Time::getMillisecondCounter() / 400) % 2 == 0;
        line (24.0f, blink ? "MIDI LEARN: TURN A KNOB" : "", {}, 24.0f);
        line (24.0f, "> " + proc.getParameters()[learn]->getName (24).toUpperCase(), {}, 24.0f);
    }
    else if (msgA.isNotEmpty() && juce::Time::getMillisecondCounter() - msgAt < 2200)
    {
        line (24.0f, msgA, {}, 24.0f);
        line (24.0f, msgB, {}, 24.0f);
    }
    else if (showPad)
    {
        const auto P = [&] (const char* b) { return proc.apvts.getParameter (ShuffleBuddyProcessor::pid (b, selectedPad))->getCurrentValueAsText(); };
        line (24.0f, ShuffleBuddyProcessor::padName (selectedPad) + juce::String ("  TUNE ") + P ("tune"), proc.samplePath (selectedPad).isNotEmpty() ? "WAV" : "SYNTH", 24.0f);
        line (24.0f, "DECAY " + P ("decay"), "PUSH " + P ("push").toUpperCase(), 24.0f);
    }
    else
    {
        line (24.0f, "SWING " + juce::String (juce::roundToInt (pv ("swing"))) + "%", "GHOSTS " + juce::String (juce::roundToInt (pv ("ghosts") * 100)) + "%", 24.0f);
        line (24.0f, "LOOSEN " + juce::String (juce::roundToInt (pv ("loosen") * 100)) + "%",
              playing ? "BAR " + juce::String (proc.playBar() + 1) + "/4" : juce::String ("READY"), 24.0f);
    }
}

// ================================================================ meters
void FxMeter::paint (juce::Graphics& g)
{
    const float v = value ? juce::jlimit (0.0f, 1.0f, value()) : 0.0f;
    shown = v;
    auto r = getLocalBounds().toFloat();
    g.setColour (toy::deckLo);
    g.fillRoundedRectangle (r, 5.0f);
    if (! vertical)   // a single round LED
    {
        auto led = r.reduced (3.0f);
        g.setColour (colour.withAlpha (0.15f + 0.85f * v));
        g.fillEllipse (led);
        g.setColour (juce::Colours::white.withAlpha (0.35f * v));
        g.fillEllipse (led.reduced (led.getWidth() * 0.3f).translated (-1.5f, -1.5f));
        return;
    }
    const int n = 10;
    auto in = r.reduced (3.0f);
    const float h = in.getHeight() / (float) n;
    for (int i = 0; i < n; ++i)   // fills from the top down, like gain reduction on a console
    {
        const bool lit = (float) i < v * (float) n;
        g.setColour (lit ? colour.interpolatedWith (toy::red, (float) i / (float) n) : toy::deck.brighter (0.1f));
        g.fillRoundedRectangle (in.getX(), in.getY() + (float) i * h + 1.0f, in.getWidth(), h - 2.0f, 2.0f);
    }
}

// ================================================================ step grid
juce::Rectangle<float> StepGrid::labelRect (int pad) const
{
    return { (float) gridPad, (float) (gridTop + pad * gridRowH + 4), (float) gridLabelW, (float) (gridRowH - 10) };
}
juce::Rectangle<float> StepGrid::pushRect (int pad) const
{
    return { (float) (getWidth() - gridPad - gridPushW), (float) (gridTop + pad * gridRowH + 7), (float) gridPushW, (float) (gridRowH - 16) };
}
juce::Rectangle<float> StepGrid::cellRect (int pad, int step) const
{
    const float x0 = (float) (gridPad + gridLabelW + 10), x1 = (float) (getWidth() - gridPad - gridPushW - 10);
    const float cw = (x1 - x0) / 16.0f;
    return { x0 + cw * (float) step + 2.0f, (float) (gridTop + pad * gridRowH + 4), cw - 4.0f, (float) (gridRowH - 10) };
}

void StepGrid::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xffd9c58f));
    g.fillRoundedRectangle (r.translated (0, 7), 28.0f);
    g.setColour (toy::cream);
    g.fillRoundedRectangle (r.withTrimmedBottom (7.0f), 28.0f);

    const int slot = proc.currentSlot();
    const bool playing = proc.apvts.getRawParameterValue ("play")->load() > 0.5f;
    const int cur = playing ? proc.playStep() : -1;
    const float swing = (proc.apvts.getRawParameterValue ("swing")->load() - 50.0f) / 25.0f;   // 0..1
    const float lean = swing * 9.0f;

    // header
    g.setFont (toy::font (13.0f));
    g.setColour (toy::brown);
    g.drawText ("DRUM", gridPad, 14, gridLabelW, 20, juce::Justification::centredLeft);
    g.drawText ("PUSH", getWidth() - gridPad - gridPushW, 14, gridPushW, 20, juce::Justification::centred);
    for (int s = 0; s < 16; ++s)
    {
        auto c = cellRect (0, s);
        g.setColour (s == cur ? toy::red : toy::brown);
        g.drawText (s % 4 == 0 ? juce::String (s / 4 + 1) : juce::String ("."), juce::Rectangle<float> (c.getX() + ((s & 1) ? lean : 0.0f), 14.0f, c.getWidth(), 20.0f), juce::Justification::centred);
    }

    for (int pd = 0; pd < NumPads; ++pd)
    {
        const auto col = toy::padColour (pd);
        auto lr = labelRect (pd);
        g.setColour (col.darker (0.45f));
        g.fillRoundedRectangle (lr.translated (0, 4), 12.0f);
        g.setColour (col);
        g.fillRoundedRectangle (lr, 12.0f);
        g.setColour (juce::Colours::white);
        g.setFont (toy::font (15.0f));
        g.drawText (ShuffleBuddyProcessor::padName (pd), lr, juce::Justification::centred);

        for (int s = 0; s < 16; ++s)
        {
            auto c = cellRect (pd, s);
            if (s & 1) c = c.translated (lean, 0.0f);   // off-beats lean right: that's the swing
            const bool beatShade = (s / 4) % 2 == 0;
            g.setColour (s == cur ? juce::Colour (0xffffe9a8) : (beatShade ? juce::Colour (0xfff1e6cc) : juce::Colour (0xffe9ddbf)));
            g.fillRoundedRectangle (c, 10.0f);
            const int v = proc.cell (slot, pd, s);
            if (v > 0)
            {
                const float d = v == 3 ? 22.0f : v == 2 ? 16.0f : 10.0f;
                g.setColour (v == 1 ? juce::Colour (0xff9aa1b6) : (v == 3 ? col.darker (0.2f) : toy::ink));
                g.fillEllipse (c.getCentreX() - d * 0.5f, c.getCentreY() - d * 0.5f, d, d);
            }
        }

        auto pr = pushRect (pd);
        g.setColour (toy::bezel);
        g.fillRoundedRectangle (pr, pr.getHeight() * 0.5f);
        g.setColour (toy::lcdBg);
        g.setFont (toy::lcd (21.0f));
        g.drawText (proc.apvts.getParameter (ShuffleBuddyProcessor::pid ("push", pd))->getCurrentValueAsText().toUpperCase(), pr, juce::Justification::centred);
    }

    // legend
    const float ly = (float) (gridTop + NumPads * gridRowH + 8);
    float lx = (float) gridPad;
    g.setFont (toy::fontMed (13.0f));
    auto legend = [&] (float d, juce::Colour c, const juce::String& txt)
    {
        g.setColour (c); g.fillEllipse (lx, ly + 9.0f - d * 0.5f, d, d);
        lx += d + 6.0f;
        g.setColour (toy::brown);
        const float tw = toy::fontMed (13.0f).getStringWidthFloat (txt) + 4.0f;
        g.drawText (txt, juce::Rectangle<float> (lx, ly, tw, 18.0f), juce::Justification::centredLeft);
        lx += tw + 14.0f;
    };
    legend (16, toy::red, "BIG HIT");
    legend (12, toy::ink, "NORMAL");
    legend (8, juce::Colour (0xff9aa1b6), "GHOST");
    g.setColour (toy::brown);
    g.drawText ("CLICK: ADD > BIG > GHOST > OFF   RIGHT-CLICK: GHOST   DRAG PUSH: EARLY/LATE", juce::Rectangle<float> (lx, ly, (float) getWidth() - lx - gridPad, 18.0f), juce::Justification::centredRight);
}

StepGrid::Hit StepGrid::hitTest (juce::Point<int> p) const
{
    Hit h;
    const auto pf = p.toFloat();
    const float lean = (proc.apvts.getRawParameterValue ("swing")->load() - 50.0f) / 25.0f * 9.0f;
    for (int pd = 0; pd < NumPads; ++pd)
    {
        if (pushRect (pd).expanded (2).contains (pf)) { h.pad = pd; h.push = true; return h; }
        if (labelRect (pd).contains (pf)) { h.pad = pd; h.label = true; return h; }
        for (int s = 0; s < 16; ++s)
        {
            auto c = cellRect (pd, s);
            if (s & 1) c = c.translated (lean, 0.0f);
            if (c.expanded (2.0f, 4.0f).contains (pf)) { h.pad = pd; h.step = s; return h; }
        }
    }
    return h;
}

void StepGrid::mouseDown (const juce::MouseEvent& e)
{
    const auto h = hitTest (e.getPosition());
    if (h.pad < 0) return;
    if (h.label) { if (onSelectPad) onSelectPad (h.pad); proc.auditionPad (h.pad, 0.85f); return; }
    if (h.push)
    {
        dragPushPad = h.pad; dragStartY = (float) e.y;
        auto* prm = proc.apvts.getParameter (ShuffleBuddyProcessor::pid ("push", h.pad));
        dragStartPush = prm->convertFrom0to1 (prm->getValue());
        prm->beginChangeGesture();
        if (onSelectPad) onSelectPad (h.pad);
        return;
    }
    const int slot = proc.currentSlot();
    const int v = proc.cell (slot, h.pad, h.step);
    proc.pushUndo (slot);
    int nv;
    if (e.mods.isPopupMenu()) nv = v == 1 ? 0 : 1;
    else if (e.mods.isShiftDown()) nv = 3;
    else nv = v == 0 ? 2 : v == 2 ? 3 : v == 3 ? 1 : 0;   // add > big > ghost > off
    proc.setCell (slot, h.pad, h.step, (juce::uint8) nv);
    paintValue = nv;
    for (auto& row : painted) row.fill (false);
    painted[(size_t) h.pad][(size_t) h.step] = true;
    if (nv > 0) proc.auditionPad (h.pad, nv == 3 ? 1.0f : nv == 2 ? 0.72f : 0.3f);
    repaint();
}

void StepGrid::mouseDrag (const juce::MouseEvent& e)
{
    if (dragPushPad >= 0)
    {
        auto* prm = proc.apvts.getParameter (ShuffleBuddyProcessor::pid ("push", dragPushPad));
        const float v = juce::jlimit (-20.0f, 30.0f, dragStartPush + (dragStartY - (float) e.y) * 0.25f);
        prm->setValueNotifyingHost (prm->convertTo0to1 (v));
        repaint();
        return;
    }
    if (paintValue < 0) return;
    const auto h = hitTest (e.getPosition());
    if (h.pad < 0 || h.step < 0 || painted[(size_t) h.pad][(size_t) h.step]) return;
    painted[(size_t) h.pad][(size_t) h.step] = true;
    proc.setCell (proc.currentSlot(), h.pad, h.step, (juce::uint8) paintValue);
    repaint();
}

void StepGrid::mouseUp (const juce::MouseEvent&)
{
    if (dragPushPad >= 0) proc.apvts.getParameter (ShuffleBuddyProcessor::pid ("push", dragPushPad))->endChangeGesture();
    dragPushPad = -1; paintValue = -1;
}

void StepGrid::mouseDoubleClick (const juce::MouseEvent& e)
{
    const auto h = hitTest (e.getPosition());
    if (h.pad >= 0 && h.push)
    {
        auto* prm = proc.apvts.getParameter (ShuffleBuddyProcessor::pid ("push", h.pad));
        prm->setValueNotifyingHost (prm->convertTo0to1 (0.0f));
        repaint();
    }
}

// ================================================================ pads
void PadButton::paint (juce::Graphics& g)
{
    const auto col = toy::padColour (pad);
    auto r = getLocalBounds().toFloat();
    const float depth = 7.0f, sink = down ? 4.0f : 0.0f;
    auto face = r.withTrimmedBottom (depth).translated (0, sink);
    g.setColour (col.darker (0.45f));
    g.fillRoundedRectangle (r.withTrimmedTop (depth), 18.0f);
    g.setColour (col.interpolatedWith (juce::Colours::white, flash * 0.55f));
    g.fillRoundedRectangle (face, 18.0f);
    g.setColour (juce::Colours::white.withAlpha (0.3f));
    g.fillRoundedRectangle (face.withHeight (face.getHeight() * 0.32f).reduced (6.0f, 4.0f), 10.0f);
    if (selected)
    {
        g.setColour (juce::Colours::white);
        g.drawRoundedRectangle (face.reduced (3.0f), 15.0f, 3.0f);
    }
    if (dropHover)
    {
        g.setColour (toy::ink);
        g.drawRoundedRectangle (face.reduced (1.5f), 17.0f, 3.0f);
    }
    if (proc.samplePath (pad).isNotEmpty())
    {
        auto st = juce::Rectangle<float> (face.getX() + 6, face.getY() + 6, 32, 16);
        g.setColour (toy::yellow); g.fillRoundedRectangle (st, 5.0f);
        g.setColour (toy::ink); g.setFont (toy::font (11.0f)); g.drawText ("WAV", st, juce::Justification::centred);
    }
    g.setColour (juce::Colours::white);
    g.setFont (toy::font (13.0f));
    g.drawText (ShuffleBuddyProcessor::padName (pad), face.removeFromBottom (26.0f), juce::Justification::centred);
    g.setFont (toy::font (11.0f));
    g.setColour (juce::Colours::white.withAlpha (0.75f));
    g.drawText (juce::String (pad + 1), face.removeFromTop (20.0f).withTrimmedRight (8.0f), juce::Justification::centredRight);
}

void PadButton::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        juce::PopupMenu m;
        m.addSectionHeader (ShuffleBuddyProcessor::padName (pad));
        m.addItem (1, "Load a WAV...");
        m.addItem (2, "Back to the synth sound", proc.samplePath (pad).isNotEmpty());
        const bool layered = proc.apvts.getRawParameterValue (ShuffleBuddyProcessor::pid ("layer", pad))->load() > 0.5f;
        m.addItem (3, "Layer: play the WAV AND the synth together", proc.samplePath (pad).isNotEmpty(), layered);
        juce::Component::SafePointer<PadButton> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe] (int r)
        {
            if (safe == nullptr) return;
            if (r == 1)
            {
                auto chooser = std::make_shared<juce::FileChooser> ("Pick a sound for this pad", juce::File(), "*.wav;*.aif;*.aiff;*.flac");
                chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [safe, chooser] (const juce::FileChooser& fc)
                {
                    if (safe != nullptr && fc.getResult().existsAsFile()) { safe->proc.loadSample (safe->pad, fc.getResult()); safe->repaint(); }
                });
            }
            if (r == 2) { safe->proc.clearSample (safe->pad); safe->repaint(); }
            if (r == 3)
            {
                const auto id = ShuffleBuddyProcessor::pid ("layer", safe->pad);
                safe->proc.setParamValue (id, safe->proc.apvts.getRawParameterValue (id)->load() > 0.5f ? 0.0f : 1.0f);
                safe->proc.auditionPad (safe->pad, 0.9f);
            }
        });
        return;
    }
    down = true;
    proc.auditionPad (pad, juce::jlimit (0.3f, 1.0f, 1.0f - (float) e.y / (float) getHeight() * 0.5f));   // hit higher = louder
    if (onSelect) onSelect (pad);
    repaint();
}

void PadButton::mouseUp (const juce::MouseEvent&) { down = false; repaint(); }

void PadButton::tick()
{
    const int h = proc.padHits (pad);
    if (h != lastHits) { lastHits = h; flash = 0.35f + 0.65f * proc.padLastVel (pad); repaint(); }
    else if (flash > 0.0f) { flash = juce::jmax (0.0f, flash - 0.12f); repaint(); }
}


// ================================================================ KICK LAB
KickLab::KickLab (ShuffleBuddyProcessor& p, LcdDisplay& l) : proc (p), lcd (l)
{
    auto pill = [this] (juce::TextButton& b, const juce::String& txt, const juce::String& param, float value, juce::Colour on, const juce::String& tip)
    {
        b.setButtonText (txt); b.setName ("pill"); b.setComponentID (param);
        b.setColour (juce::TextButton::buttonOnColourId, on);
        b.setWantsKeyboardFocus (false); b.setTooltip (tip);
        b.onClick = [this, param, value] { proc.setParamValue (param, value); proc.auditionPad (Kick, 0.95f); };
        addAndMakeVisible (b);
    };
    const char* waves[] = { "SINE", "TRIANGLE", "ROUND" };
    const char* waveTips[] = { "Pure and deep: 808, house, sub kicks", "Woodier and brighter: breaks, funk", "Fat and pushy, like a hot 909 body" };
    for (int i = 0; i < 3; ++i) pill (waveBtns[i], waves[i], "kwave", (float) i, toy::red, waveTips[i]);
    const char* clicks[] = { "NOISE", "BEATER", "TICK", "SNAP" };
    const char* clickTips[] = { "A burst of noise: 909-style attack", "The felt beater hitting the skin: round, mid-range knock", "A one-sample tick: minimal / techno precision", "A bright band-passed snap: cuts through anything" };
    for (int i = 0; i < 4; ++i) pill (clickBtns[i], clicks[i], "kctype", (float) i, toy::orange, clickTips[i]);
    const char* lens[] = { "FREE", "1/16", "1/8", "1/4", "1/2", "BAR" };
    for (int i = 0; i < 6; ++i) pill (lenBtns[i], lens[i], "klen", (float) i, toy::squashCol, i == 0 ? "Kick length set by DECAY" : "Kick length locked to the tempo: it ends exactly here, at any BPM");

    knob ("kfreq", "PITCH", toy::red).setTooltip ("Where the kick lands after its pitch drop. KEY overrides it with a musical note");
    knob ("kstart", "SWEEP", toy::red).setTooltip ("How high the pitch starts. More = more punch and 'dooh'");
    knob ("kpdec", "DROP", toy::red).setTooltip ("How fast the pitch falls. Short = punchy, long = boing / 808 slide");
    knob ("kcurve", "CURVE", toy::red).setTooltip ("Shape of the pitch fall: snappy curve, or a straight slide down");
    knob ("khold", "HOLD", toy::red).setTooltip ("Keeps the body at full level before it decays. Adds weight and punch");
    knob ("kdecay", "DECAY", toy::red).setTooltip ("How long the body rings");
    knob ("kdrive", "DRIVE", toy::red).setTooltip ("Saturation on the body. Adds harmonics so it's heard on small speakers");
    knob ("kclick", "CLICK", toy::orange).setTooltip ("Level of the attack transient");
    knob ("kctone", "TONE", toy::orange).setTooltip ("Brightness of the click");
    knob ("ksub", "SUB", toy::blue).setTooltip ("Layer: a clean sine under the body, phase-locked so it never cancels. No drive on it");
    knob ("ksubdec", "SUB DECAY", toy::blue).setTooltip ("How long the sub layer lasts, separately from the body");
    knob ("kknock", "KNOCK", toy::green).setTooltip ("A short resonant ping: the hollow 'thock' of Berlin techno kicks");
    knob ("kknockf", "KNOCK HZ", toy::green).setTooltip ("Pitch of the knock");
    knob ("kduck", "DUCK", toy::squashCol).setTooltip ("Every kick pushes the rest of the kit and the FX down, then lets them swell back: built-in sidechain pump");
    knob ("krumble", "RUMBLE", toy::spaceCol).setTooltip ("A dark, low-passed reverb tail that pumps between kicks: dub techno / warehouse rumble");
    knob ("kreal", "REAL KICK", toy::olive).setTooltip ("A recorded acoustic kick (with its snare buzzing) under the synth: depth and air no synth has");
    knob ("kroom", "ROOM MIC", toy::teal).setTooltip ("The real room mic from that recording: a natural space around the kick");

    auto btn = [this] (juce::TextButton& b, juce::Colour c, juce::Colour txt, const juce::String& tip)
    {
        b.setName ("slot"); b.setWantsKeyboardFocus (false); b.setTooltip (tip);
        b.setColour (juce::TextButton::buttonColourId, c); b.setColour (juce::TextButton::textColourOffId, txt);
        addAndMakeVisible (b);
    };
    btn (presetBtn, juce::Colours::white, toy::ink, "Kick presets");
    btn (keyBtn, juce::Colours::white, toy::ink, "KEY LOCK: tune the kick to your track's key (it picks the note in the low octave)");
    btn (surpriseBtn, toy::orange, juce::Colours::white, "Roll the dice: a new kick every time, always a usable one");
    btn (hitBtn, toy::red, juce::Colours::white, "Play the kick (key 1)");
    btn (closeBtn, toy::ink, toy::yellow, "Back to the pattern");
    layerBtn.setName ("pill"); layerBtn.setComponentID ("layer1");
    layerBtn.setColour (juce::TextButton::buttonOnColourId, toy::yellow);
    layerBtn.setWantsKeyboardFocus (false);
    layerBtn.setTooltip ("Drop a WAV on the KICK pad, then turn this on: the WAV and the synth kick play together. Use the synth for the sub and the WAV for the top");
    layerBtn.onClick = [this] { proc.setParamValue ("layer1", proc.apvts.getRawParameterValue ("layer1")->load() > 0.5f ? 0.0f : 1.0f); proc.auditionPad (Kick, 0.95f); };
    addAndMakeVisible (layerBtn);

    presetBtn.onClick = [this]
    {
        juce::PopupMenu m;
        const auto& kp = sounds::kickPresets();
        for (size_t i = 0; i < kp.size(); ++i) m.addItem ((int) i + 1, kp[i].name, true, proc.kickName == kp[i].name);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetBtn), [this] (int r)
        {
            if (r <= 0) return;
            proc.applyKickPreset (r - 1);
            proc.auditionPad (Kick, 0.95f);
            lcd.flash ("KICK: " + proc.kickName.toUpperCase(), "LOADED");
            refreshNames();
        });
    };
    keyBtn.onClick = [this]
    {
        juce::PopupMenu m;
        auto* prm = proc.apvts.getParameter ("kkey");
        const int cur = (int) proc.apvts.getRawParameterValue ("kkey")->load();
        auto* choice = dynamic_cast<juce::AudioParameterChoice*> (prm);
        for (int i = 0; i < choice->choices.size(); ++i) m.addItem (i + 1, i == 0 ? juce::String ("Free (use PITCH)") : choice->choices[i], true, i == cur);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&keyBtn), [this] (int r) { if (r > 0) { proc.setParamValue ("kkey", (float) (r - 1)); proc.auditionPad (Kick, 0.95f); refreshNames(); } });
    };
    surpriseBtn.onClick = [this] { proc.surpriseKick(); proc.auditionPad (Kick, 0.95f); lcd.flash ("KICK: " + proc.kickName.toUpperCase(), "PRESS AGAIN FOR ANOTHER"); refreshNames(); };
    hitBtn.onClick = [this] { proc.auditionPad (Kick, 0.95f); };
    closeBtn.onClick = [this] { if (onClose) onClose(); };

    for (auto& b : bufs) b.resize (34000);
    refreshNames();
    startTimerHz (15);
}

KickLab::~KickLab() { stopTimer(); }

juce::Slider& KickLab::knob (const juce::String& id, const juce::String& label, juce::Colour c)
{
    K k;
    k.id = id;
    k.s = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
    k.s->setColour (juce::Slider::rotarySliderFillColourId, c);
    k.s->setWantsKeyboardFocus (false);
    k.s->setComponentID (id);
    auto* prm = proc.apvts.getParameter (id);
    k.s->setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
    k.a = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, *k.s);
    auto* raw = k.s.get();
    k.s->onDragEnd = [this] { proc.auditionPad (Kick, 0.95f); };
    k.s->onValueChange = [this, prm, label, raw] { if (raw->isMouseButtonDown()) lcd.flash (label, prm->getCurrentValueAsText().toUpperCase()); };
    k.l = std::make_unique<juce::Label> (id, label);
    k.l->setFont (toy::font (12.0f));
    k.l->setColour (juce::Label::textColourId, toy::brown);
    k.l->setJustificationType (juce::Justification::centred);
    addAndMakeVisible (*k.s);
    addAndMakeVisible (*k.l);
    knobs.push_back (std::move (k));
    return *knobs.back().s;
}

std::vector<juce::Component*> KickLab::learnables()
{
    std::vector<juce::Component*> v;
    for (auto& k : knobs) v.push_back (k.s.get());
    for (auto& b : waveBtns) v.push_back (&b);
    for (auto& b : clickBtns) v.push_back (&b);
    for (auto& b : lenBtns) v.push_back (&b);
    v.push_back (&layerBtn);
    return v;
}

void KickLab::refreshNames()
{
    presetBtn.setButtonText ("PRESET: " + proc.kickName.toUpperCase());
    const int key = (int) proc.apvts.getRawParameterValue ("kkey")->load();
    keyBtn.setButtonText (key == 0 ? juce::String ("KEY: FREE") : "KEY: " + proc.apvts.getParameter ("kkey")->getCurrentValueAsText());
}

void KickLab::timerCallback()
{
    auto pv = [this] (const char* id) { return proc.apvts.getRawParameterValue (id)->load(); };
    const int wave = (int) pv ("kwave"), ct = (int) pv ("kctype"), len = (int) pv ("klen");
    for (int i = 0; i < 3; ++i) waveBtns[i].setToggleState (i == wave, juce::dontSendNotification);
    for (int i = 0; i < 4; ++i) clickBtns[i].setToggleState (i == ct, juce::dontSendNotification);
    for (int i = 0; i < 6; ++i) lenBtns[i].setToggleState (i == len, juce::dontSendNotification);
    layerBtn.setToggleState (pv ("layer1") > 0.5f, juce::dontSendNotification);
    layerBtn.setButtonText (proc.samplePath (Kick).isEmpty() ? "YOUR WAV: DROP ONE ON THE KICK PAD" : (pv ("layer1") > 0.5f ? "YOUR WAV LAYER: ON" : "YOUR WAV LAYER: OFF"));
    // re-draw the scope only when something that shapes the kick changed
    const char* ids[] = { "kwave", "kfreq", "kkey", "kstart", "kpdec", "kcurve", "khold", "kdecay", "kdrive", "kclick", "kctype", "kctone",
                          "ksub", "ksubdec", "kknock", "kknockf", "klen", "tune1", "decay1", "tempo", "kreal", "kroom" };
    double h = 0.0;
    for (int i = 0; i < 22; ++i) h += (double) pv (ids[i]) * (1.0 + i * 0.6180339);
    if (h != lastHash) { lastHash = h; renderScope(); refreshNames(); repaint (scopeArea); }
}

void KickLab::renderScope()
{
    // play one kick offline, right here on the UI thread, and draw it
    const float sr = 48000.0f;
    const int n = (int) bufs[0].size();
    for (auto& b : bufs) std::fill (b.begin(), b.end(), 0.0f);
    const auto patch = proc.kickPatch (proc.apvts.getRawParameterValue ("tempo")->load());
    DrumVoice v;
    v.trigger (Kick, 1.0f, patch, proc.apvts.getRawParameterValue ("tune1")->load(), proc.apvts.getRawParameterValue ("decay1")->load(), 1.0f, nullptr, sr, 0, 12345u,
               0.0f, 0.0f, proc.realSet (patch.real), proc.apvts.getRawParameterValue ("real1")->load(), proc.apvts.getRawParameterValue ("room1")->load());
    v.render (bufs[0].data(), bufs[1].data(), bufs[2].data(), bufs[3].data(), bufs[4].data(), bufs[5].data(), bufs[6].data(), bufs[7].data(), 0, n);
    const int cols = juce::jmax (10, scopeArea.getWidth() - 20);
    scopeMin.assign ((size_t) cols, 0.0f); scopeMax.assign ((size_t) cols, 0.0f); pitchLine.assign ((size_t) cols, 0.0f);
    float pk = 0.001f;
    for (int i = 0; i < n; ++i) pk = juce::jmax (pk, std::abs (bufs[2][(size_t) i]));
    const float tune = std::exp2 (proc.apvts.getRawParameterValue ("tune1")->load() / 12.0f);
    for (int c = 0; c < cols; ++c)
    {
        const int a = c * n / cols, b = (c + 1) * n / cols;
        float lo = 0.0f, hi = 0.0f;
        for (int i = a; i < b; ++i) { lo = juce::jmin (lo, bufs[2][(size_t) i]); hi = juce::jmax (hi, bufs[2][(size_t) i]); }
        scopeMin[(size_t) c] = lo / pk; scopeMax[(size_t) c] = hi / pk;
        // the pitch envelope, same maths as the voice
        const float t = (float) a, dropN = juce::jmax (1.0f, patch.drop * sr);
        const float e = std::exp (-t / dropN), lin = juce::jmax (0.0f, 1.0f - t / (dropN * 3.0f));
        const float f = patch.freq * tune * (1.0f + (patch.sweep - 1.0f) * ((1.0f - patch.curve) * e + patch.curve * lin));
        pitchLine[(size_t) c] = juce::jlimit (0.0f, 1.0f, std::log2 (f / 25.0f) / std::log2 (800.0f / 25.0f));
    }
    const float dm = std::exp2 ((proc.apvts.getRawParameterValue ("decay1")->load() - 0.45f) * 3.2f);
    gateX = patch.gate > 0.0f ? juce::jmin (1.0f, patch.gate * dm * sr / (float) n) : -1.0f;
}

void KickLab::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (toy::ink); g.fillRoundedRectangle (r.translated (0, 4), 28.0f);
    g.setColour (toy::cream); g.fillRoundedRectangle (r.withTrimmedBottom (4), 28.0f);
    g.setColour (toy::red); g.drawRoundedRectangle (r.withTrimmedBottom (4).reduced (3.0f), 26.0f, 4.0f);
    g.setFont (toy::font (30.0f)); g.setColour (toy::red);
    g.drawText ("KICK LAB", 24, 10, 170, 36, juce::Justification::centredLeft);

    // scope: a little green LCD, like the main screen
    auto sc = scopeArea.toFloat();
    g.setColour (toy::bezel); g.fillRoundedRectangle (sc, 14.0f);
    auto in = sc.reduced (8.0f);
    g.setColour (toy::lcdBg); g.fillRoundedRectangle (in, 8.0f);
    if (! scopeMin.empty())
    {
        const float mid = in.getCentreY(), hh = in.getHeight() * 0.42f;
        g.setColour (toy::lcdInk.withAlpha (0.18f)); g.drawHorizontalLine ((int) mid, in.getX() + 4, in.getRight() - 4);
        g.setColour (toy::lcdInk);
        const float x0 = in.getX() + 2.0f, w = (in.getWidth() - 4.0f) / (float) scopeMin.size();
        for (size_t c = 0; c < scopeMin.size(); ++c)
            g.fillRect (x0 + (float) c * w, mid - scopeMax[c] * hh, juce::jmax (1.0f, w), juce::jmax (1.0f, (scopeMax[c] - scopeMin[c]) * hh));
        juce::Path pl;
        for (size_t c = 0; c < pitchLine.size(); ++c)
        {
            const float x = x0 + (float) c * w, y = in.getBottom() - 6.0f - pitchLine[c] * (in.getHeight() - 12.0f);
            if (c == 0) pl.startNewSubPath (x, y); else pl.lineTo (x, y);
        }
        g.setColour (toy::red.withAlpha (0.85f)); g.strokePath (pl, juce::PathStrokeType (2.0f));
        if (gateX > 0.0f)
        {
            const float gx = in.getX() + 2.0f + gateX * (in.getWidth() - 4.0f);
            for (float y = in.getY() + 4; y < in.getBottom() - 4; y += 8) g.drawLine (gx, y, gx, y + 4, 1.5f);
        }
        g.setFont (toy::lcd (18.0f)); g.setColour (toy::lcdInk);
        g.drawText ("700 MS", in.reduced (8, 4), juce::Justification::bottomRight);
        g.setColour (toy::red);
        g.drawText ("PITCH", in.reduced (8, 4), juce::Justification::topRight);
    }

    g.setFont (toy::font (12.0f)); g.setColour (toy::brown);
    g.drawText ("CLICK TYPE", 364, 54, 200, 14, juce::Justification::centredLeft);
    g.drawText ("TRANSIENT", 364, 108, 190, 14, juce::Justification::centred);
    g.drawText ("SUB LAYER", 560, 108, 190, 14, juce::Justification::centred);
    g.drawText ("BODY", 20, 230, 60, 18, juce::Justification::centredLeft);
    g.drawText ("KNOCK", 16, 362, 200, 14, juce::Justification::centred);
    g.drawText ("LENGTH (LOCKS TO TEMPO)", 216, 362, 260, 14, juce::Justification::centred);
    g.drawText ("MIX + REAL LAYER", 482, 362, 272, 14, juce::Justification::centred);
    g.setColour (toy::brown.withAlpha (0.25f));
    g.fillRoundedRectangle (16, 222, (float) getWidth() - 32, 2, 1);
    g.fillRoundedRectangle (16, 354, (float) getWidth() - 32, 2, 1);
}

void KickLab::resized()
{
    presetBtn.setBounds (200, 10, 250, 36);
    surpriseBtn.setBounds (460, 10, 150, 36);
    hitBtn.setBounds (620, 10, 70, 36);
    closeBtn.setBounds (700, 10, 48, 36);
    scopeArea = { 16, 54, 334, 160 };
    for (int i = 0; i < 4; ++i) clickBtns[i].setBounds (364 + i * 98, 70, 92, 32);
    auto place = [this] (size_t i, int x, int y, int w)
    {
        if (i >= knobs.size()) return;
        const int kw = juce::jmin (70, w);
        knobs[i].s->setBounds (x + (w - kw) / 2, y, kw, 66);
        knobs[i].l->setBounds (x, y + 64, w, 16);
    };
    place (7, 364, 124, 96); place (8, 462, 124, 96);    // click, tone
    place (9, 560, 124, 96); place (10, 658, 124, 96);   // sub, sub decay
    for (int i = 0; i < 3; ++i) waveBtns[i].setBounds (80 + i * 104, 226, 98, 28);
    keyBtn.setBounds (402, 224, 170, 32);
    for (size_t i = 0; i < 7; ++i) place (i, 16 + (int) i * 105, 262, 100);
    place (11, 16, 380, 96); place (12, 116, 380, 96);   // knock
    for (int i = 0; i < 6; ++i) lenBtns[i].setBounds (216 + i * 44, 384, 41, 32);
    layerBtn.setBounds (216, 424, 260, 32);
    place (13, 482, 380, 68); place (14, 550, 380, 68);  // duck, rumble
    place (15, 618, 380, 68); place (16, 686, 380, 68);  // real kick, room mic
}

// ================================================================ editor
ShuffleBuddyEditor::ShuffleBuddyEditor (ShuffleBuddyProcessor& p)
    : AudioProcessorEditor (&p), proc (p), lcd (p), grid (p)
{
    setLookAndFeel (&lnf);

    // backdrop: desk, plastic case, handle, logo, sticker, speaker grille (rendered once)
    backdrop = juce::Image (juce::Image::ARGB, W, H, true);
    {
        juce::Graphics g (backdrop);
        g.fillAll (juce::Colour (0xffdbe6f7));
        auto c = juce::Rectangle<float> (30, 30, 1200, (float) H - 70.0f);
        g.setColour (juce::Colour (0x401f2333));
        g.fillRoundedRectangle (c.translated (0, 26).expanded (6, 0), 52.0f);
        g.setColour (toy::yellowSh);
        g.fillRoundedRectangle (c.translated (0, 16), 48.0f);
        g.setColour (toy::yellow);
        g.fillRoundedRectangle (c, 48.0f);
        g.setColour (toy::yellowHi);
        g.fillRoundedRectangle (c.withHeight (10).reduced (40, 0).translated (0, 4), 5.0f);
        g.setColour (toy::yellowLo);
        g.fillRoundedRectangle (c.withTrimmedTop (c.getHeight() - 18).reduced (30, 0), 9.0f);
        // handle
        g.setColour (toy::yellowLo);
        g.fillRoundedRectangle (64, 54, 150, 44, 22);
        g.setColour (toy::yellowSh);
        g.fillRoundedRectangle (64, 54, 150, 10, 5);
        // logo
        g.setFont (toy::font (42.0f));
        g.setColour (juce::Colour (0xff9e2219));
        g.drawText ("SHUFFLE BUDDY", 234, 49, 420, 46, juce::Justification::centredLeft);
        g.setColour (toy::red);
        g.drawText ("SHUFFLE BUDDY", 234, 46, 420, 46, juce::Justification::centredLeft);
        g.setFont (toy::font (14.0f));
        g.setColour (toy::brown);
        g.drawText ("MY FIRST GROOVE COMPUTER", 236, 90, 320, 18, juce::Justification::centredLeft);
        // speaker grille
        g.setColour (toy::brown.withAlpha (0.55f));
        for (int row = 0; row < 3; ++row) for (int col = 0; col < 12; ++col) g.fillEllipse (1010.0f + col * 15.0f, 62.0f + row * 15.0f, 10, 10);
        // section labels
        g.setFont (toy::font (13.0f));
        g.setColour (toy::brown);
        g.drawText ("PICK A STYLE", 60, 398, 200, 16, juce::Justification::centredLeft);
        // tempo window in the header
        g.setColour (toy::ink);  g.fillRoundedRectangle (770, 56, 210, 50, 14);
        g.setColour (toy::bezel); g.fillRoundedRectangle (770, 52, 210, 50, 14);
        g.setColour (toy::lcdBg); g.fillRoundedRectangle (820, 59, 110, 36, 8);
        g.setFont (toy::font (11.0f)); g.setColour (toy::brown);
        g.drawText ("TEMPO", 770, 104, 210, 14, juce::Justification::centred);

        // FX deck: a dark tray set into the plastic, like DubChord's flight deck
        auto d = juce::Rectangle<float> (48, 806, 1164, 160);
        g.setColour (toy::yellowSh); g.fillRoundedRectangle (d.expanded (6).translated (0, -4), 30);
        g.setColour (toy::deckLo);   g.fillRoundedRectangle (d, 26);
        g.setColour (toy::deck);     g.fillRoundedRectangle (d.withTrimmedTop (6), 26);
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        for (int i = 0; i < 40; ++i) g.fillEllipse (d.getX() + 30.0f + (float) i * 28.0f, d.getBottom() - 14.0f, 4, 4);
        // sticker label
        {
            juce::Graphics::ScopedSaveState ss (g);
            g.addTransform (juce::AffineTransform::rotation (-0.06f, 120, 806));
            g.setColour (juce::Colour (0xffb3420a)); g.fillRoundedRectangle (66, 792, 130, 30, 11);
            g.setColour (toy::echoCol); g.fillRoundedRectangle (66, 789, 130, 30, 11);
            g.setColour (juce::Colours::white); g.setFont (toy::font (17.0f));
            g.drawText ("DUB DECK", 66, 789, 130, 30, juce::Justification::centred);
        }
        g.setFont (toy::font (12.0f));
        g.setColour (toy::cream.withAlpha (0.6f));
        g.drawText ("ECHO TIME", 214, 830, 236, 14, juce::Justification::centredLeft);
        g.drawText ("ECHO TYPE", 214, 892, 236, 14, juce::Justification::centredLeft);
        g.drawText ("GR", 828, 832, 24, 14, juce::Justification::centred);
        // little patch cables between sections (decoration)
        g.setColour (toy::echoCol.withAlpha (0.5f));
        g.drawLine (186, 840, 206, 840, 3.0f);
        // dividers
        g.setColour (toy::deckLo);
        for (float x : { 462.0f, 860.0f }) g.fillRoundedRectangle (x, 828, 3, 128, 1.5f);
    }

    addAndMakeVisible (lcd);
    addAndMakeVisible (grid);
    grid.onSelectPad = [this] (int pd) { selectPad (pd); };

    for (int i = 0; i < NumPads; ++i)
    {
        pads.push_back (std::make_unique<PadButton> (proc, i));
        pads.back()->onSelect = [this] (int pd) { selectPad (pd); };
        addAndMakeVisible (*pads.back());
    }

    // style buttons: set the feel (tempo, swing, ghosts, loosen, dust, pushes) for that style
    const char* styles[] = { "2-STEP", "BURIAL", "FUNK", "BREAKS" };
    const juce::Colour sc[] = { toy::red, toy::purple, toy::green, toy::blue };
    for (int i = 0; i < 4; ++i)
    {
        styleBtns[i].setButtonText (styles[i]);
        styleBtns[i].setName ("style");
        styleBtns[i].setColour (juce::TextButton::buttonColourId, sc[i]);
        styleBtns[i].setColour (juce::TextButton::textColourOffId, juce::Colours::white);
        styleBtns[i].setWantsKeyboardFocus (false);
        styleBtns[i].onClick = [this, i] { proc.applyStyleFeel (i); };
        styleBtns[i].setTooltip ("Sets tempo, swing, ghosts, loosen, dust and push for this style. Then press ROLL.");
        addAndMakeVisible (styleBtns[i]);
    }

    auto setupBtn = [this] (juce::TextButton& b, juce::Colour c, juce::Colour txt, const juce::String& tip)
    {
        b.setColour (juce::TextButton::buttonColourId, c);
        b.setColour (juce::TextButton::textColourOffId, txt);
        b.setWantsKeyboardFocus (false);
        b.setTooltip (tip);
        addAndMakeVisible (b);
    };
    setupBtn (rollBtn, toy::orange, juce::Colours::white, "Write a brand-new pattern in the chosen style");
    setupBtn (varyBtn, juce::Colours::white, toy::ink, "Nudge this pattern: add, drop or soften a few hits");
    setupBtn (fillBtn, juce::Colours::white, toy::ink, "Hold for a fill on the second half of the bar");
    rollBtn.onClick = [this] { proc.rollPattern(); grid.repaint(); };
    varyBtn.onClick = [this] { proc.varyPattern(); grid.repaint(); };
    fillBtn.onStateChange = [this] { setParam ("fill", fillBtn.isDown() ? 1.0f : 0.0f); };

    playBtn.setName ("play"); stopBtn.setName ("stop");
    setupBtn (playBtn, toy::green, juce::Colours::white, "Play (space)");
    setupBtn (stopBtn, toy::red, juce::Colours::white, "Stop (space)");
    playBtn.onClick = [this] { setParam ("play", 1.0f); };
    stopBtn.onClick = [this] { setParam ("play", 0.0f); };

    for (int i = 0; i < 4; ++i)
    {
        slotBtns[i].setButtonText (juce::String::charToString ((juce::juce_wchar) ('A' + i)));
        slotBtns[i].setName ("slot");
        setupBtn (slotBtns[i], juce::Colours::white, toy::ink, "Pattern " + slotBtns[i].getButtonText() + " (switches on the next bar). Shift-click: copy the current pattern here");
        slotBtns[i].onClick = [this, i]
        {
            const int from = proc.currentSlot();
            if (juce::ModifierKeys::currentModifiers.isShiftDown() && from != i)
            {
                proc.copySlot (from, i);
                lcd.flash ("COPIED " + juce::String::charToString ((juce::juce_wchar) ('A' + from)) + " > " + juce::String::charToString ((juce::juce_wchar) ('A' + i)), "NOW EDIT THE COPY");
            }
            setParam ("slot", (float) i); grid.repaint();
        };
    }

    setupBtn (tempoDown, juce::Colours::white, toy::ink, "Slower");
    setupBtn (tempoUp, juce::Colours::white, toy::ink, "Faster");
    tempoDown.onClick = [this] { setParam ("tempo", proc.apvts.getRawParameterValue ("tempo")->load() - 1.0f); };
    tempoUp.onClick = [this] { setParam ("tempo", proc.apvts.getRawParameterValue ("tempo")->load() + 1.0f); };

    // groove knobs
    makeKnob ("swing", "SWING", toy::red).setTooltip ("How much the off-beat 16ths lean late. 50% = straight, 58% = garage, 66% = triplet shuffle");
    makeKnob ("ghosts", "GHOSTS", toy::blue).setTooltip ("Louder quiet notes, and extra ghost snares/hats in the gaps");
    makeKnob ("loosen", "LOOSEN", toy::purple).setTooltip ("Human timing and velocity wobble. High = Burial-style drunk drums");
    makeKnob ("variety", "VARIETY", toy::green).setTooltip ("Every bar plays a slightly different version of the pattern, without changing it");
    makeKnob ("dust", "DUST", toy::olive).setTooltip ("Vinyl crackle, hiss and a little lo-fi grit");
    makeKnob ("master", "VOLUME", toy::ink);

    // DUB DECK (borrowed from DubChord): echo, space, filter, squash, throw
    {
        auto& e = makeKnob ("echo", "ECHO", toy::echoCol, true);
        e.setTooltip ("Tape echo return. Each pad's ECHO send (pad edit) decides how much of it goes in");
        makeKnob ("space", "SPACE", toy::spaceCol, true).setTooltip ("Dub reverb return. Each pad's SPACE send decides how much goes in");
        auto& f = makeKnob ("fxfilter", "FILTER", toy::filterCol, true);
        f.getProperties().set ("bipolar", true);
        f.setTooltip ("Left = low-pass (muffled, from the next room). Right = high-pass (thin, for build-ups). Middle = off");
        makeKnob ("squash", "SQUASH", toy::squashCol, true).setTooltip ("Glue compressor + warm saturation on the whole kit. Up = punchier, louder, more pumping");
    }
    const char* times[] = { "1/16", "1/8", "1/8.", "1/4" };
    for (int i = 0; i < 4; ++i)
    {
        timeBtns[i].setButtonText (times[i]); timeBtns[i].setName ("pill");
        timeBtns[i].setColour (juce::TextButton::buttonOnColourId, toy::echoCol);
        timeBtns[i].setWantsKeyboardFocus (false);
        timeBtns[i].setTooltip (i == 2 ? "Dotted 8th: the classic dub echo" : "Echo repeat time, locked to the tempo");
        timeBtns[i].onClick = [this, i] { setParam ("echotime", (float) i); };
        timeBtns[i].setComponentID ("echotime");
        addAndMakeVisible (timeBtns[i]);
    }
    const char* types[] = { "TAPE", "SPACE-E", "BBD" };
    const char* typeTips[] = { "Tape echo: warm, wobbly, darker every repeat", "Space Echo: multi-head tape, rhythmic clusters", "Bucket brigade: grainy, dark, chorusy" };
    for (int i = 0; i < 3; ++i)
    {
        typeBtns[i].setButtonText (types[i]); typeBtns[i].setName ("pill");
        typeBtns[i].setColour (juce::TextButton::buttonOnColourId, toy::echoCol);
        typeBtns[i].setWantsKeyboardFocus (false);
        typeBtns[i].setTooltip (typeTips[i]);
        typeBtns[i].onClick = [this, i] { setParam ("echotype", (float) i); };
        typeBtns[i].setComponentID ("echotype");
        addAndMakeVisible (typeBtns[i]);
    }
    throwBtn.setName ("throw");
    throwBtn.setWantsKeyboardFocus (false);
    throwBtn.setComponentID ("throw");
    throwBtn.setTooltip ("Hold: everything feeds the echo and it runs away. Let go: the echoes ring out. (key T)");
    throwBtn.onStateChange = [this] { const bool d = throwBtn.isDown(); if (d != (proc.apvts.getRawParameterValue ("throw")->load() > 0.5f)) setParam ("throw", d ? 1.0f : 0.0f); };
    addAndMakeVisible (throwBtn);
    grMeter.colour = toy::squashCol;
    grMeter.value = [this] { return proc.squashGR() / 12.0f; };
    grMeter.setTooltip ("How hard SQUASH is working (gain reduction)");
    addAndMakeVisible (grMeter);
    echoLed.colour = toy::echoCol; echoLed.vertical = false;
    echoLed.value = [this] { return juce::jlimit (0.0f, 1.0f, proc.echoLevel() * 4.0f); };
    addAndMakeVisible (echoLed);

    setupBtn (undoBtn, juce::Colours::white, toy::ink, "Undo the last pattern change (Ctrl+Z)");
    undoBtn.setName ("slot");
    undoBtn.onClick = [this] { doUndo(); };
    setupBtn (chainBtn, juce::Colours::white, toy::ink, "Chain patterns: A-B, A-B-C-D or A-A-A-B. Each pattern plays for one bar");
    chainBtn.setName ("slot");
    chainBtn.onClick = [this]
    {
        const int c = ((int) proc.apvts.getRawParameterValue ("chain")->load() + 1) % 4;
        setParam ("chain", (float) c);
        lcd.flash (c == 0 ? juce::String ("CHAIN OFF") : "CHAIN " + proc.apvts.getParameter ("chain")->getCurrentValueAsText(), c == 0 ? juce::String ("ONE PATTERN LOOPS") : juce::String ("ONE BAR EACH"));
    };

    // sounds: SOUND menu for the selected pad, KICK LAB, KITS
    setupBtn (soundBtn, juce::Colours::white, toy::ink, "Pick this pad's synth sound (or kick preset)");
    soundBtn.setName ("slot");
    soundBtn.onClick = [this] { soundMenu(); };
    setupBtn (labBtn, toy::red, juce::Colours::white, "Open the Kick Lab: build your own kick");
    labBtn.setName ("slot");
    labBtn.onClick = [this] { showLab (lab == nullptr || ! lab->isVisible()); };
    setupBtn (kitsBtn, toy::blue, juce::Colours::white, "Load a whole kit: a kick plus a sound on every pad");
    kitsBtn.onClick = [this] { kitsMenu(); };

    // anything with a parameter can be MIDI-learned from its right-click menu
    for (auto* c : getChildren()) c->addMouseListener (this, false);
    tempoLabel.setFont (toy::lcd (30.0f));
    tempoLabel.setColour (juce::Label::textColourId, toy::lcdInk);
    tempoLabel.setJustificationType (juce::Justification::centred);
    tempoLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (tempoLabel);

    padEditLabel.setFont (toy::font (13.0f));
    padEditLabel.setColour (juce::Label::textColourId, toy::brown);
    padEditLabel.setVisible (false);
    hint.setFont (toy::fontMed (13.0f));
    hint.setColour (juce::Label::textColourId, toy::brown);
    hint.setJustificationType (juce::Justification::centred);
    hint.setText ("KEYS 1-8 PLAY PADS  /  SPACE PLAY-STOP  /  T THROW  /  K KICK LAB  /  CTRL+Z UNDO  /  SHIFT-CLICK A SLOT TO COPY INTO IT  /  DRAG WAVS ONTO PADS  /  RIGHT-CLICK A KNOB TO MIDI LEARN", juce::dontSendNotification);
    addAndMakeVisible (hint);

    selectPad (1);
    setWantsKeyboardFocus (true);
    setSize (W, H);
    startTimerHz (30);
}

ShuffleBuddyEditor::~ShuffleBuddyEditor() { setLookAndFeel (nullptr); }

juce::Slider& ShuffleBuddyEditor::makeKnob (const juce::String& id, const juce::String& label, juce::Colour c, bool onDeck)
{
    Knob k;
    k.id = id;
    k.s = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
    k.s->setColour (juce::Slider::rotarySliderFillColourId, c);
    k.s->setColour (juce::Slider::textBoxTextColourId, onDeck ? toy::cream : toy::ink);
    k.s->setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    k.s->setTextBoxStyle (juce::Slider::TextBoxBelow, true, 80, 16);
    k.s->setComponentID (id);
    k.s->getProperties().set ("deck", onDeck);
    k.s->setWantsKeyboardFocus (false);
    k.s->setDoubleClickReturnValue (true, proc.apvts.getParameter (id)->convertFrom0to1 (proc.apvts.getParameter (id)->getDefaultValue()));
    k.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, *k.s);
    k.name = std::make_unique<juce::Label> (id, label);
    k.name->setFont (toy::font (13.0f));
    k.name->setColour (juce::Label::textColourId, onDeck ? c : toy::brown);
    k.name->getProperties().set ("deck", onDeck);
    k.name->setJustificationType (juce::Justification::centred);
    addAndMakeVisible (*k.s);
    addAndMakeVisible (*k.name);
    knobs.push_back (std::move (k));
    return *knobs.back().s;
}

void ShuffleBuddyEditor::selectPad (int pad)
{
    selected = pad;
    for (int i = 0; i < NumPads; ++i) { pads[(size_t) i]->selected = i == pad; pads[(size_t) i]->repaint(); }
    lcd.selectedPad = pad; lcd.padShownAt = juce::Time::getMillisecondCounter(); lcd.repaint();
    padEditLabel.setText ("PAD EDIT: " + juce::String (ShuffleBuddyProcessor::padName (pad)), juce::dontSendNotification);
    {
        const juce::String nm = pad == Kick ? proc.kickName : proc.apvts.getParameter (ShuffleBuddyProcessor::pid ("snd", pad))->getCurrentValueAsText();
        soundBtn.setButtonText (juce::String (ShuffleBuddyProcessor::padName (pad)) + ": " + nm.toUpperCase());
    }

    // rebuild the three pad knobs for this pad
    for (auto& k : padKnobs) { removeChildComponent (k.s.get()); removeChildComponent (k.name.get()); }
    padKnobs.clear();
    const char* ids[] = { "tune", "decay", "lvl", "real", "room", "esend", "ssend" };
    const char* names[] = { "TUNE", "DECAY", "LEVEL", "REAL", "ROOM", "ECHO", "SPACE" };
    const juce::Colour cols[] = { toy::padColour (pad), toy::padColour (pad), toy::padColour (pad), toy::olive, toy::teal, toy::echoCol, toy::spaceCol };
    for (int i = 0; i < 7; ++i)
    {
        Knob k;
        k.id = ShuffleBuddyProcessor::pid (ids[i], pad);
        k.s = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
        k.s->setColour (juce::Slider::rotarySliderFillColourId, cols[i]);
        k.s->setComponentID (k.id);
        k.s->addMouseListener (this, false);
        if (i == 3) k.s->setTooltip ("How much of the recorded drum (real kit, close mic) sits under the synth. 100% = as designed");
        if (i == 4) k.s->setTooltip ("Room around this sound: the real room mic plus the small synth room. 100% = as designed");
        if (i >= 5) k.s->setTooltip (juce::String ("How much ") + ShuffleBuddyProcessor::padName (pad) + (i == 5 ? " goes into the echo" : " goes into the space reverb"));
        k.s->setWantsKeyboardFocus (false);
        k.s->setDoubleClickReturnValue (true, proc.apvts.getParameter (k.id)->convertFrom0to1 (proc.apvts.getParameter (k.id)->getDefaultValue()));
        k.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, k.id, *k.s);
        k.s->onValueChange = [this] { lcd.padShownAt = juce::Time::getMillisecondCounter(); };
        k.name = std::make_unique<juce::Label> (k.id, names[i]);
        k.name->setFont (toy::font (11.0f));
        k.name->setColour (juce::Label::textColourId, toy::brown);
        k.name->setJustificationType (juce::Justification::centred);
        addAndMakeVisible (*k.s);
        addAndMakeVisible (*k.name);
        padKnobs.push_back (std::move (k));
    }
    lastMapVersion = -1;
    resized();
}

void ShuffleBuddyEditor::setParam (const juce::String& id, float value)
{
    auto* prm = proc.apvts.getParameter (id);
    prm->beginChangeGesture();
    prm->setValueNotifyingHost (prm->convertTo0to1 (value));
    prm->endChangeGesture();
}

void ShuffleBuddyEditor::timerCallback()
{
    for (auto& pd : pads) pd->tick();
    const int step = proc.playStep();
    const bool playing = proc.apvts.getRawParameterValue ("play")->load() > 0.5f;
    if (step != grid.lastStep || ! playing) { grid.lastStep = step; grid.repaint(); }
    lcd.repaint();
    const int style = (int) proc.apvts.getRawParameterValue ("style")->load();
    for (int i = 0; i < 4; ++i) styleBtns[i].setToggleState (i == style, juce::dontSendNotification);
    const int slot = proc.currentSlot();
    for (int i = 0; i < 4; ++i)
    {
        slotBtns[i].setToggleState (i == slot, juce::dontSendNotification);
        slotBtns[i].setColour (juce::TextButton::buttonColourId, i == slot ? toy::ink : juce::Colours::white);
        slotBtns[i].setColour (juce::TextButton::textColourOffId, i == slot ? toy::yellow : toy::ink);
    }
    playBtn.setToggleState (playing, juce::dontSendNotification);

    const int chain = (int) proc.apvts.getRawParameterValue ("chain")->load();
    if (chain != 0)
        for (int i = 0; i < 4; ++i)
            if (i == proc.playingSlot() && playing && i != slot)
            {
                slotBtns[i].setColour (juce::TextButton::buttonColourId, toy::orange);
                slotBtns[i].setColour (juce::TextButton::textColourOffId, juce::Colours::white);
            }
    chainBtn.setButtonText (chain == 0 ? "CHAIN" : proc.apvts.getParameter ("chain")->getCurrentValueAsText());
    chainBtn.setColour (juce::TextButton::buttonColourId, chain == 0 ? juce::Colours::white : toy::orange);
    chainBtn.setColour (juce::TextButton::textColourOffId, chain == 0 ? toy::ink : juce::Colours::white);
    undoBtn.setEnabled (proc.canUndo());

    const int et = (int) proc.apvts.getRawParameterValue ("echotime")->load();
    const int ty = (int) proc.apvts.getRawParameterValue ("echotype")->load();
    for (int i = 0; i < 4; ++i) timeBtns[i].setToggleState (i == et, juce::dontSendNotification);
    for (int i = 0; i < 3; ++i) typeBtns[i].setToggleState (i == ty, juce::dontSendNotification);
    tempoLabel.setText (juce::String (juce::roundToInt (proc.apvts.getRawParameterValue ("tempo")->load())), juce::dontSendNotification);
    grMeter.repaint();
    echoLed.repaint();
    if (proc.mappingVersion() != lastMapVersion || proc.learningParam() != lastLearn) refreshMappings();
}

void ShuffleBuddyEditor::refreshMappings()
{
    lastMapVersion = proc.mappingVersion();
    lastLearn = proc.learningParam();
    auto update = [this] (Knob& k)
    {
        auto* prm = proc.apvts.getParameter (k.id);
        const int cc = proc.ccFor (prm->getParameterIndex());
        const bool learning = proc.learningParam() == prm->getParameterIndex();
        const bool deck = k.name->getProperties()["deck"];
        const auto base = deck ? k.s->findColour (juce::Slider::rotarySliderFillColourId) : toy::brown;
        k.name->setColour (juce::Label::textColourId, learning ? toy::red : (cc >= 0 ? (deck ? juce::Colour (0xff7fd7ff) : toy::blue) : base));
        auto t = k.name->getText().upToFirstOccurrenceOf (" CC", false, false);
        k.name->setText (cc >= 0 ? t + " CC" + juce::String (cc) : t, juce::dontSendNotification);
    };
    for (auto& k : knobs) update (k);
    for (auto& k : padKnobs) update (k);
}

void ShuffleBuddyEditor::showLab (bool show)
{
    if (show && lab == nullptr)
    {
        lab = std::make_unique<KickLab> (proc, lcd);
        lab->onClose = [this] { showLab (false); };
        for (auto* c : lab->learnables()) c->addMouseListener (this, false);
        addChildComponent (*lab);
    }
    if (lab != nullptr) { lab->setBounds (grid.getBounds()); lab->setVisible (show); }
    grid.setVisible (! show);
    labBtn.setButtonText (show ? "GRID" : "KICK LAB");
    if (show) selectPad (Kick);
    lastMapVersion = -1;
}

void ShuffleBuddyEditor::soundMenu()
{
    const int pad = selected;
    juce::PopupMenu m;
    if (pad == Kick)
    {
        const auto& kp = sounds::kickPresets();
        m.addSectionHeader ("KICK PRESETS");
        for (size_t i = 0; i < kp.size(); ++i) m.addItem ((int) i + 1, kp[i].name, true, proc.kickName == kp[i].name);
        m.addSeparator();
        m.addItem (999, "Open the Kick Lab...");
    }
    else
    {
        const auto& lib = sounds::library();
        const int cur = (int) proc.apvts.getRawParameterValue (ShuffleBuddyProcessor::pid ("snd", pad))->load();
        juce::StringArray cats;
        for (auto& snd : lib) cats.addIfNotAlreadyThere (snd.cat);
        for (auto& c : cats)
        {
            juce::PopupMenu sub;
            bool has = false;
            for (size_t i = 0; i < lib.size(); ++i) if (c == lib[i].cat) { sub.addItem ((int) i + 1, lib[i].name, true, (int) i == cur); has = has || (int) i == cur; }
            m.addSubMenu (c.toUpperCase(), sub, true, nullptr, has);
        }
    }
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&soundBtn), [this, pad] (int r)
    {
        if (r <= 0) return;
        if (pad == Kick)
        {
            if (r == 999) { showLab (true); return; }
            proc.applyKickPreset (r - 1);
            lcd.flash ("KICK: " + proc.kickName.toUpperCase(), "LOADED");
            if (lab != nullptr) lab->refreshNames();
        }
        else
        {
            proc.setParamValue (ShuffleBuddyProcessor::pid ("snd", pad), (float) (r - 1));
            lcd.flash (juce::String (ShuffleBuddyProcessor::padName (pad)) + " SOUND", juce::String (sounds::library()[(size_t) r - 1].name).toUpperCase());
        }
        proc.auditionPad (pad, 0.9f);
        selectPad (pad);
    });
}

void ShuffleBuddyEditor::kitsMenu()
{
    juce::PopupMenu m;
    m.addSectionHeader ("KITS");
    const auto& kits = sounds::kits();
    for (size_t i = 0; i < kits.size(); ++i) m.addItem ((int) i + 1, kits[i].name, true, proc.kitName == kits[i].name);
    m.addSeparator();
    m.addItem (999, "Surprise kick");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&kitsBtn), [this] (int r)
    {
        if (r <= 0) return;
        if (r == 999) { proc.surpriseKick(); proc.auditionPad (Kick, 0.95f); lcd.flash ("KICK: " + proc.kickName.toUpperCase(), "FROM THE DICE"); }
        else { proc.applyKit (r - 1); lcd.flash ("KIT: " + proc.kitName.toUpperCase(), "8 NEW SOUNDS"); }
        if (lab != nullptr) lab->refreshNames();
        selectPad (selected);
    });
}

void ShuffleBuddyEditor::doUndo()
{
    if (proc.undo()) { lcd.flash ("UNDO", "PATTERN " + juce::String::charToString ((juce::juce_wchar) ('A' + proc.currentSlot())) + " RESTORED"); grid.repaint(); }
}

void ShuffleBuddyEditor::mouseDown (const juce::MouseEvent& e)
{
    if (! e.mods.isPopupMenu() || e.eventComponent == this) return;
    const auto id = e.eventComponent->getComponentID();
    if (id.isNotEmpty() && proc.apvts.getParameter (id) != nullptr) learnMenu (id, e.eventComponent);
}

void ShuffleBuddyEditor::learnMenu (const juce::String& paramId, juce::Component* target)
{
    auto* prm = proc.apvts.getParameter (paramId);
    const int idx = prm->getParameterIndex();
    const int cc = proc.ccFor (idx);
    juce::PopupMenu m;
    m.addSectionHeader (prm->getName (40).toUpperCase());
    m.addItem (1, proc.learningParam() == idx ? "Waiting for a MIDI knob..." : "MIDI Learn", true, proc.learningParam() == idx);
    m.addItem (2, cc >= 0 ? "Forget CC " + juce::String (cc) : "Not mapped", cc >= 0);
    if (proc.learningParam() >= 0) m.addItem (3, "Cancel learn");
    m.addItem (4, "Reset to default");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target), [this, idx, paramId] (int r)
    {
        if (r == 1) proc.startLearn (idx);
        else if (r == 2) proc.clearMapping (idx);
        else if (r == 3) proc.cancelLearn();
        else if (r == 4) { auto* q = proc.apvts.getParameter (paramId); q->beginChangeGesture(); q->setValueNotifyingHost (q->getDefaultValue()); q->endChangeGesture(); }
        refreshMappings();
    });
}

bool ShuffleBuddyEditor::keyPressed (const juce::KeyPress& key)
{
    const auto c = key.getTextCharacter();
    if (c >= '1' && c <= '8') { proc.auditionPad (c - '1', 0.9f); selectPad (c - '1'); return true; }
    if ((key.getModifiers().isCommandDown() || key.getModifiers().isCtrlDown()) && (c == 'z' || c == 'Z' || key.getKeyCode() == 'Z')) { doUndo(); return true; }
    if (c == 'k' || c == 'K') { showLab (lab == nullptr || ! lab->isVisible()); return true; }
    if (c == 't' || c == 'T') { setParam ("throw", 1.0f); juce::Timer::callAfterDelay (700, [this] { setParam ("throw", 0.0f); }); return true; }
    if (key == juce::KeyPress::backspaceKey || key == juce::KeyPress::deleteKey) { proc.clearPattern(); lcd.flash ("CLEARED", "CTRL+Z TO BRING IT BACK"); grid.repaint(); return true; }
    if (key == juce::KeyPress::escapeKey && proc.learningParam() >= 0) { proc.cancelLearn(); return true; }
    if (key == juce::KeyPress::spaceKey)
    {
        setParam ("play", proc.apvts.getRawParameterValue ("play")->load() > 0.5f ? 0.0f : 1.0f);
        return true;
    }
    return false;
}

void ShuffleBuddyEditor::fileDragMove (const juce::StringArray&, int x, int y)
{
    for (auto& pd : pads)
    {
        const bool over = pd->getBounds().contains (x, y);
        if (over != pd->dropHover) { pd->dropHover = over; pd->repaint(); }
    }
}

void ShuffleBuddyEditor::fileDragExit (const juce::StringArray&)
{
    for (auto& pd : pads) if (pd->dropHover) { pd->dropHover = false; pd->repaint(); }
}

void ShuffleBuddyEditor::filesDropped (const juce::StringArray& files, int x, int y)
{
    fileDragExit (files);
    for (int i = 0; i < NumPads; ++i)
        if (pads[(size_t) i]->getBounds().contains (x, y) && files.size() > 0)
        {
            if (proc.loadSample (i, juce::File (files[0]))) selectPad (i);
            pads[(size_t) i]->repaint();
        }
}

void ShuffleBuddyEditor::paint (juce::Graphics& g)
{
    g.drawImageAt (backdrop, 0, 0);
}

void ShuffleBuddyEditor::resized()
{
    lcd.setBounds (60, 136, 350, 156);
    tempoDown.setBounds (776, 56, 40, 42);
    tempoUp.setBounds (934, 56, 40, 42);
    tempoLabel.setBounds (820, 59, 110, 36);

    soundBtn.setBounds (60, 296, 252, 28);
    labBtn.setBounds (318, 296, 92, 28);
    kitsBtn.setBounds (590, 52, 120, 50);
    for (size_t i = 0; i < padKnobs.size(); ++i)
    {
        padKnobs[i].s->setBounds (56 + (int) i * 51, 326, 50, 46);
        padKnobs[i].name->setBounds (50 + (int) i * 51, 372, 62, 16);
    }
    if (lab != nullptr) lab->setBounds (432, 136, 768, 474);

    for (int i = 0; i < 4; ++i) styleBtns[i].setBounds (60 + (i % 2) * 181, 418 + (i / 2) * 60, 169, 54);
    rollBtn.setBounds (60, 540, 170, 70);
    varyBtn.setBounds (236, 540, 84, 70);
    fillBtn.setBounds (326, 540, 84, 70);

    grid.setBounds (432, 136, 768, 474);

    for (int i = 0; i < NumPads; ++i) pads[(size_t) i]->setBounds (60 + i * 62, 636, 54, 104);
    for (size_t i = 0; i < 6 && i < knobs.size(); ++i)
    {
        knobs[i].name->setBounds (568 + (int) i * 76, 630, 70, 16);
        knobs[i].s->setBounds (568 + (int) i * 76, 646, 70, 86);
    }
    playBtn.setBounds (1036, 634, 78, 60);
    stopBtn.setBounds (1122, 634, 78, 60);
    for (int i = 0; i < 4; ++i) slotBtns[i].setBounds (1036 + i * 42, 702, 38, 40);
    undoBtn.setBounds (1036, 748, 78, 40);
    chainBtn.setBounds (1122, 748, 78, 40);

    // DUB DECK
    const int dy = 826;
    auto deckKnob = [&] (size_t i, int x)
    {
        if (i >= knobs.size()) return;
        knobs[i].name->setBounds (x, dy + 2, 96, 16);
        knobs[i].s->setBounds (x + 3, dy + 18, 90, 110);
    };
    deckKnob (6, 86);                      // echo
    echoLed.setBounds (170, dy + 2, 16, 16);
    for (int i = 0; i < 4; ++i) timeBtns[i].setBounds (214 + i * 60, dy + 22, 56, 36);
    for (int i = 0; i < 3; ++i) typeBtns[i].setBounds (214 + i * 80, dy + 84, 76, 36);
    deckKnob (7, 480);                     // space
    deckKnob (8, 600);                     // filter
    deckKnob (9, 720);                     // squash
    grMeter.setBounds (830, dy + 22, 20, 104);
    throwBtn.setBounds (884, dy - 2, 310, 134);
    hint.setBounds (40, 972, 1180, 18);
}
