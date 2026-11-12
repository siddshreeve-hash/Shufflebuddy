#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
    constexpr int W = 1260, H = 860;
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
    const bool pressed = down || (b.getToggleState() && b.getName() != "style");
    auto face = b.getLocalBounds().toFloat().withTrimmedBottom (6.0f).translated (0.0f, pressed ? 4.0f : 0.0f);
    const auto txt = b.findColour (juce::TextButton::textColourOffId);
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
    for (int i = 0; i <= 10; ++i)
    {
        const float a = start + (end - start) * (float) i / 10.0f;
        const bool lit = (float) i / 10.0f <= pos + 0.001f;
        g.setColour (lit ? col : toy::brown.withAlpha (0.3f));
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
    line (24.0f, "PATTERN " + juce::String::charToString ((juce::juce_wchar) ('A' + slot)), juce::String (juce::roundToInt (pv ("tempo"))) + " BPM", 26.0f);
    line (34.0f, groove::styleName ((int) pv ("style")), {}, 34.0f);
    const bool showPad = selectedPad >= 0 && juce::Time::getMillisecondCounter() - padShownAt < 3000;
    if (showPad)
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
        auto c = juce::Rectangle<float> (30, 30, 1200, 800);
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
        // sticker
        {
            juce::Graphics::ScopedSaveState ss (g);
            g.addTransform (juce::AffineTransform::rotation (-0.1f, 640, 74));
            g.setColour (juce::Colour (0xff1b47a8)); g.fillRoundedRectangle (580, 58, 124, 38, 14);
            g.setColour (toy::blue); g.fillRoundedRectangle (580, 54, 124, 38, 14);
            g.setColour (juce::Colours::white); g.setFont (toy::font (16.0f));
            g.drawText ("SKIP MODE!", 580, 54, 124, 38, juce::Justification::centred);
        }
        // speaker grille
        g.setColour (toy::brown.withAlpha (0.55f));
        for (int row = 0; row < 3; ++row) for (int col = 0; col < 12; ++col) g.fillEllipse (1010.0f + col * 15.0f, 62.0f + row * 15.0f, 10, 10);
        // section labels
        g.setFont (toy::font (13.0f));
        g.setColour (toy::brown);
        g.drawText ("PICK A STYLE", 60, 398, 200, 16, juce::Justification::centredLeft);
        g.drawText ("TEMPO", 296, 300, 108, 16, juce::Justification::centred);
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
        setupBtn (slotBtns[i], juce::Colours::white, toy::ink, "Pattern " + slotBtns[i].getButtonText() + " (switches on the next bar)");
        slotBtns[i].onClick = [this, i] { setParam ("slot", (float) i); grid.repaint(); };
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

    padEditLabel.setFont (toy::font (13.0f));
    padEditLabel.setColour (juce::Label::textColourId, toy::brown);
    addAndMakeVisible (padEditLabel);
    hint.setFont (toy::fontMed (13.0f));
    hint.setColour (juce::Label::textColourId, toy::brown);
    hint.setJustificationType (juce::Justification::centred);
    hint.setText ("KEYS 1-8 PLAY THE PADS   /   SPACE PLAYS + STOPS   /   DRAG WAV FILES ONTO A PAD   /   RIGHT-CLICK A PAD FOR MORE", juce::dontSendNotification);
    addAndMakeVisible (hint);

    selectPad (1);
    setWantsKeyboardFocus (true);
    setSize (W, H);
    startTimerHz (30);
}

ShuffleBuddyEditor::~ShuffleBuddyEditor() { setLookAndFeel (nullptr); }

juce::Slider& ShuffleBuddyEditor::makeKnob (const juce::String& id, const juce::String& label, juce::Colour c)
{
    Knob k;
    k.id = id;
    k.s = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
    k.s->setColour (juce::Slider::rotarySliderFillColourId, c);
    k.s->setColour (juce::Slider::textBoxTextColourId, toy::ink);
    k.s->setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    k.s->setTextBoxStyle (juce::Slider::TextBoxBelow, true, 70, 16);
    k.s->setWantsKeyboardFocus (false);
    k.s->setDoubleClickReturnValue (true, proc.apvts.getParameter (id)->convertFrom0to1 (proc.apvts.getParameter (id)->getDefaultValue()));
    k.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, *k.s);
    k.name = std::make_unique<juce::Label> (id, label);
    k.name->setFont (toy::font (13.0f));
    k.name->setColour (juce::Label::textColourId, toy::brown);
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

    // rebuild the three pad knobs for this pad
    for (auto& k : padKnobs) { removeChildComponent (k.s.get()); removeChildComponent (k.name.get()); }
    padKnobs.clear();
    const char* ids[] = { "tune", "decay", "lvl" };
    const char* names[] = { "TUNE", "DECAY", "LEVEL" };
    for (int i = 0; i < 3; ++i)
    {
        Knob k;
        k.id = ShuffleBuddyProcessor::pid (ids[i], pad);
        k.s = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
        k.s->setColour (juce::Slider::rotarySliderFillColourId, toy::padColour (pad));
        k.s->setWantsKeyboardFocus (false);
        k.s->setDoubleClickReturnValue (true, proc.apvts.getParameter (k.id)->convertFrom0to1 (proc.apvts.getParameter (k.id)->getDefaultValue()));
        k.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, k.id, *k.s);
        k.s->onValueChange = [this] { lcd.padShownAt = juce::Time::getMillisecondCounter(); };
        k.name = std::make_unique<juce::Label> (k.id, names[i]);
        k.name->setFont (toy::font (12.0f));
        k.name->setColour (juce::Label::textColourId, toy::brown);
        k.name->setJustificationType (juce::Justification::centred);
        addAndMakeVisible (*k.s);
        addAndMakeVisible (*k.name);
        padKnobs.push_back (std::move (k));
    }
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
}

bool ShuffleBuddyEditor::keyPressed (const juce::KeyPress& key)
{
    const auto c = key.getTextCharacter();
    if (c >= '1' && c <= '8') { proc.auditionPad (c - '1', 0.9f); selectPad (c - '1'); return true; }
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
    padEditLabel.setBounds (60, 298, 230, 18);
    for (size_t i = 0; i < padKnobs.size(); ++i)
    {
        padKnobs[i].s->setBounds (60 + (int) i * 76, 316, 62, 56);
        padKnobs[i].name->setBounds (52 + (int) i * 76, 370, 78, 16);
    }
    tempoDown.setBounds (296, 320, 50, 48);
    tempoUp.setBounds (354, 320, 50, 48);

    for (int i = 0; i < 4; ++i) styleBtns[i].setBounds (60 + (i % 2) * 181, 418 + (i / 2) * 60, 169, 54);
    rollBtn.setBounds (60, 540, 170, 70);
    varyBtn.setBounds (236, 540, 84, 70);
    fillBtn.setBounds (326, 540, 84, 70);

    grid.setBounds (432, 136, 768, 474);

    for (int i = 0; i < NumPads; ++i) pads[(size_t) i]->setBounds (60 + i * 62, 636, 54, 104);
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        knobs[i].name->setBounds (568 + (int) i * 76, 630, 70, 16);
        knobs[i].s->setBounds (568 + (int) i * 76, 646, 70, 86);
    }
    playBtn.setBounds (1036, 634, 78, 60);
    stopBtn.setBounds (1122, 634, 78, 60);
    for (int i = 0; i < 4; ++i) slotBtns[i].setBounds (1036 + i * 42, 702, 38, 40);
    hint.setBounds (60, 760, 1140, 20);
}
