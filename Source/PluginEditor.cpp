#include "PluginEditor.h"

using namespace juce;

static const char* kIds[15]    { "mix", "predelay", "decay", "hfreq", "hshelf", "bfreq", "bmult", "size",
                                 "attack", "early", "late", "rate", "depth", "hicut", "locut" };
static const char* kLabels[15] { "MIX", "PREDELAY", "DECAY", "HighFreq", "HighShelf", "BassFreq", "BassMult", "Size",
                                 "Attack", "Early", "Late", "Rate", "Depth", "HighCut", "LowCut" };

static String fmtValue (int i, double v)
{
    switch (i)
    {
        case 0: case 7: case 8: case 9: case 10: case 12: return String (v, 1) + " %";
        case 1:  return String (v, 2) + " ms";
        case 2:  return String (v, 2) + " s";
        case 3: case 5: case 13: case 14: return String ((int) std::round (v)) + " Hz";
        case 4:  return String (v, 2) + " dB";
        case 6:  return String (v, 2) + " X";
        default: return String (v, 2) + " Hz";    // rate
    }
}

//==============================================================================
XpaceverbAudioProcessorEditor::XpaceverbAudioProcessorEditor (XpaceverbAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);

    for (size_t i = 0; i < 15; ++i)
    {
        auto& s = knobs[i];
        s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        const bool big = (i == 2);
        if (big) s.getProperties().set ("big", true);
        s.setTextBoxStyle (Slider::TextBoxBelow, false, big ? 130 : 112, big ? 22 : 16);
        addAndMakeVisible (s);
        atts[i] = std::make_unique<SliderAtt> (proc.apvts, kIds[i], s);

        const int idx = (int) i;
        s.textFromValueFunction = [idx] (double v) { return fmtValue (idx, v); };
        s.updateText();
    }

    themeBtn.onClick = [this]
    {
        proc.apvts.state.setProperty ("dark", ! isDark(), nullptr);
        applyTheme();
    };
    addAndMakeVisible (themeBtn);

    addAndMakeVisible (sparkles);      // siempre por encima de todo
    sparkles.toFront (false);

    applyTheme();
    setSize (1096, 410);
    startTimerHz (30);
}

XpaceverbAudioProcessorEditor::~XpaceverbAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

//==============================================================================
bool XpaceverbAudioProcessorEditor::isDark() const
{
    return (bool) proc.apvts.state.getProperty ("dark", true);
}

XpaceverbAudioProcessorEditor::Pal XpaceverbAudioProcessorEditor::pal() const
{
    if (isDark()) return { Colour (0xff0b0b0b), Colours::white, Colour (0xff808080), Colour (0xff1a1a1a) };
    return { Colour (0xffe9e9e7), Colour (0xff0e0e0e), Colour (0xff7c7c7c), Colour (0xffdcdcd9) };
}

void XpaceverbAudioProcessorEditor::applyTheme()
{
    const auto p = pal();
    laf.fg = p.fg;
    laf.fill = p.fill;
    laf.track = p.dim.withAlpha (0.35f);

    laf.setColour (Slider::textBoxTextColourId, p.fg);
    laf.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    laf.setColour (PopupMenu::backgroundColourId, p.bg);
    laf.setColour (PopupMenu::textColourId, p.fg);
    laf.setColour (PopupMenu::highlightedBackgroundColourId, p.fg);
    laf.setColour (PopupMenu::highlightedTextColourId, p.bg);
    laf.setColour (TextButton::buttonColourId, p.bg);
    laf.setColour (TextButton::buttonOnColourId, p.bg);
    laf.setColour (TextButton::textColourOffId, p.fg);
    laf.setColour (TextButton::textColourOnId, p.fg);
    laf.setColour (ComboBox::outlineColourId, p.dim);
    laf.setColour (Label::textColourId, p.fg);

    sparkles.fg = p.fg;      // blanco sobre fondo negro, negro sobre fondo claro
    themeBtn.setButtonText (isDark() ? "LIGHT" : "DARK");
    lastDark = isDark();
    sendLookAndFeelChange();
    repaint();
}

void XpaceverbAudioProcessorEditor::setParam (const String& id, float v)
{
    if (auto* p = proc.apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (v));
        p->endChangeGesture();
    }
}

int XpaceverbAudioProcessorEditor::getChoice (const char* id) const
{
    return (int) std::round (proc.apvts.getRawParameterValue (id)->load());
}

//==============================================================================
void XpaceverbAudioProcessorEditor::resized()
{
    const int W = getWidth();
    sparkles.setBounds (getLocalBounds());

    themeBtn.setBounds (W - 24 - 80, 46, 80, 24);

    knobs[0].setBounds (24, 120, 100, 96);       // MIX
    knobs[1].setBounds (24, 230, 100, 96);       // PREDELAY
    knobs[2].setBounds (136, 100, 190, 224);     // DECAY (grande)

    // cajas: DAMPING, SHAPE, DIFF, MOD, EQ
    boxes[0] = { 336, 76, 280, 246 };
    boxes[1] = { 626, 76, 104, 246 };
    boxes[2] = { 740, 76, 104, 246 };
    boxes[3] = { 854, 76, 104, 246 };
    boxes[4] = { 968, 76, 104, 246 };

    const int r1 = 122, r2 = 230, h = 88;
    knobs[3].setBounds (346, r1, 130, h);        // HighFreq
    knobs[4].setBounds (476, r1, 130, h);        // HighShelf
    knobs[5].setBounds (346, r2, 130, h);        // BassFreq
    knobs[6].setBounds (476, r2, 130, h);        // BassMult
    knobs[7].setBounds (626, r1, 104, h);        // Size
    knobs[8].setBounds (626, r2, 104, h);        // Attack
    knobs[9].setBounds (740, r1, 104, h);        // Early
    knobs[10].setBounds (740, r2, 104, h);       // Late
    knobs[11].setBounds (854, r1, 104, h);       // Rate
    knobs[12].setBounds (854, r2, 104, h);       // Depth
    knobs[13].setBounds (968, r1, 104, h);       // HighCut
    knobs[14].setBounds (968, r2, 104, h);       // LowCut

    modeRect  = { 24, 338, 300, 34 };
    colorRect = { 336, 338, 220, 34 };
    prevRect  = { W - 24 - 64, 342, 28, 26 };
    nextRect  = { W - 24 - 30, 342, 28, 26 };
}

//==============================================================================
void XpaceverbAudioProcessorEditor::showModeMenu()
{
    PopupMenu m;
    const int cur = getChoice ("mode");
    for (int i = 0; i < ReverbEngine::kModes; ++i)
        m.addItem (i + 1, ReverbEngine::modeName (i), true, i == cur);

    m.showMenuAsync (PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (modeRect)), [this] (int r)
    {
        if (r > 0) setParam ("mode", (float) (r - 1));
    });
}

void XpaceverbAudioProcessorEditor::mouseDown (const MouseEvent& e)
{
    const auto pt = e.getPosition();
    const int nModes = ReverbEngine::kModes;
    if (modeRect.contains (pt))       showModeMenu();
    else if (colorRect.contains (pt)) setParam ("color", (float) ((getChoice ("color") + 1) % 3));
    else if (prevRect.contains (pt))  setParam ("mode", (float) ((getChoice ("mode") + nModes - 1) % nModes));
    else if (nextRect.contains (pt))  setParam ("mode", (float) ((getChoice ("mode") + 1) % nModes));
}

//==============================================================================
// Animacion de brillo: mas reverb sonando = mas estrellitas y mas brillantes
void XpaceverbAudioProcessorEditor::updateSparkles()
{
    const float dt = 1.f / 30.f;
    for (auto& s : sparkles.stars)
    {
        if (! s.alive) continue;
        s.age += dt;
        if (s.age >= s.life) s.alive = false;
    }

    const int bc = proc.blockCounter.load();
    const bool running = (bc != lastBlock);          // el plugin esta procesando audio
    lastBlock = bc;

    const float mix01 = proc.apvts.getRawParameterValue ("mix")->load() * 0.01f;
    const float wet = proc.wetRms.load();
    const float target = running ? jlimit (0.f, 1.f, std::sqrt (wet) * 2.4f) * (0.3f + 0.7f * mix01) : 0.f;
    spark += (target - spark) * 0.15f;

    spawnAcc += (running ? 0.8f + 26.f * spark : 0.f) * dt;

    const auto kb = knobs[2].getBounds();
    const Point<float> c ((float) kb.getCentreX(), (float) kb.getY() + (float) (kb.getHeight() - 22) * 0.5f);
    const float R = (float) jmin (kb.getWidth(), kb.getHeight() - 22) * 0.5f * 0.84f;

    while (spawnAcc >= 1.f)
    {
        spawnAcc -= 1.f;
        Star* slot = nullptr;
        for (auto& s : sparkles.stars) if (! s.alive) { slot = &s; break; }
        if (slot == nullptr) break;

        slot->alive = true;
        slot->age = 0.f;
        slot->life = 0.7f + rng.nextFloat() * 1.3f;
        slot->size = (2.5f + rng.nextFloat() * 4.5f) * (0.6f + 0.8f * spark);
        slot->phase = rng.nextFloat() * 6.28f;

        if (rng.nextFloat() < 0.6f)          // la mayoria dentro del disco de DECAY
        {
            const float a = rng.nextFloat() * 6.2831853f;
            const float rad = std::sqrt (rng.nextFloat()) * R;
            slot->x = c.x + std::cos (a) * rad;
            slot->y = c.y + std::sin (a) * rad;
        }
        else
        {
            slot->x = 24.f + rng.nextFloat() * (float) (getWidth() - 48);
            slot->y = 76.f + rng.nextFloat() * 246.f;
        }
    }

    sparkles.spark = spark;
    sparkles.repaint();
}

void XpaceverbAudioProcessorEditor::timerCallback()
{
    if (isDark() != lastDark) applyTheme();
    updateSparkles();
    repaint (0, 330, getWidth(), 50);     // fila de MODE / COLOR
}

//==============================================================================
void XpaceverbAudioProcessorEditor::paint (Graphics& g)
{
    const auto p = pal();
    g.fillAll (p.bg);

    // titulo y version (como el original: titulo grande, datos arriba a la derecha)
    g.setColour (p.fg);
    g.setFont (Font (FontOptions (36.f)).boldened());
    g.drawText ("XPACEVERB", 24, 8, 420, 48, Justification::centredLeft);

    g.setFont (11.f);
    g.setColour (p.fg);
    g.drawText ("Version 1.0", getWidth() - 300, 8, 276, 14, Justification::centredRight);
    g.drawText ("by TapeLab", getWidth() - 300, 22, 276, 14, Justification::centredRight);
    g.drawText (String ("GUI Mode: ") + (isDark() ? "Darkness" : "Lightness"), getWidth() - 300, 36, 276, 14, Justification::centredRight);

    // cajas de seccion
    static const char* titles[5] { "DAMPING", "SHAPE", "DIFF", "MOD", "EQ" };
    g.setColour (p.fg);
    for (int i = 0; i < 5; ++i)
    {
        g.drawRoundedRectangle (boxes[(size_t) i].toFloat().reduced (1.f), 16.f, 2.f);
        g.setFont (Font (FontOptions (15.f)).boldened());
        g.drawText (titles[i], boxes[(size_t) i].getX(), boxes[(size_t) i].getY() + 6, boxes[(size_t) i].getWidth(), 20,
                    Justification::centred);
    }

    // etiquetas de perillas
    for (size_t i = 0; i < 15; ++i)
    {
        const auto b = knobs[i].getBounds();
        const bool big = (i == 2);
        g.setColour (i == 0 ? p.dim : p.fg);
        g.setFont (Font (FontOptions (big ? 19.f : 13.f)).boldened());
        g.drawText (kLabels[i], b.getX(), b.getY() - (big ? 22 : 15), b.getWidth(), big ? 20 : 15, Justification::centred);
    }

    // fila MODE / COLOR
    g.setColour (p.fg);
    g.setFont (Font (FontOptions (16.f)).boldened());
    g.drawText ("MODE:", modeRect.getX(), modeRect.getY(), 60, modeRect.getHeight(), Justification::centredLeft);
    g.setFont (16.f);
    g.drawText (ReverbEngine::modeName (getChoice ("mode")), modeRect.getX() + 64, modeRect.getY(),
                modeRect.getWidth() - 64, modeRect.getHeight(), Justification::centredLeft);

    g.setFont (Font (FontOptions (16.f)).boldened());
    g.drawText ("COLOR:", colorRect.getX(), colorRect.getY(), 70, colorRect.getHeight(), Justification::centredLeft);
    g.setFont (16.f);
    g.drawText (ReverbEngine::colorName (getChoice ("color")), colorRect.getX() + 76, colorRect.getY(),
                colorRect.getWidth() - 76, colorRect.getHeight(), Justification::centredLeft);

    // flechas
    Path left, right;
    left.addTriangle ((float) prevRect.getRight() - 4.f, (float) prevRect.getY() + 2.f,
                      (float) prevRect.getRight() - 4.f, (float) prevRect.getBottom() - 2.f,
                      (float) prevRect.getX() + 3.f, (float) prevRect.getCentreY());
    right.addTriangle ((float) nextRect.getX() + 4.f, (float) nextRect.getY() + 2.f,
                       (float) nextRect.getX() + 4.f, (float) nextRect.getBottom() - 2.f,
                       (float) nextRect.getRight() - 3.f, (float) nextRect.getCentreY());
    g.fillPath (left);
    g.fillPath (right);

    g.setColour (p.dim);
    g.setFont (10.f);
    g.drawText ("Click MODE to choose among 17 reverb types   -   the sparkles follow how much reverb is ringing",
                24, 384, getWidth() - 48, 14, Justification::centredLeft);
}
