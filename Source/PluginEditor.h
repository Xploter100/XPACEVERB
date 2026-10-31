#pragma once
#include "PluginProcessor.h"

// Perillas monocromaticas estilo "disco con aguja"
struct XvLAF : public juce::LookAndFeel_V4
{
    juce::Colour fg { juce::Colours::white }, fill { 0xff1b1b1b }, track { 0xff333333 };

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider& s) override
    {
        const bool big = (bool) s.getProperties().getWithDefault ("big", false);
        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (big ? 4.f : 5.f);
        const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
        const auto c = b.getCentre();
        const float ang = startAngle + pos * (endAngle - startAngle);
        const float rr = r * (big ? 0.86f : 0.74f);

        juce::Path trackP, valP;
        trackP.addCentredArc (c.x, c.y, r, r, 0.f, startAngle, endAngle, true);
        valP.addCentredArc   (c.x, c.y, r, r, 0.f, startAngle, ang, true);
        g.setColour (track);
        g.strokePath (trackP, juce::PathStrokeType (big ? 4.f : 2.2f));
        g.setColour (fg);
        g.strokePath (valP, juce::PathStrokeType (big ? 4.f : 2.2f));

        g.setColour (fill);
        g.fillEllipse (c.x - rr, c.y - rr, 2.f * rr, 2.f * rr);
        g.setColour (fg);
        g.drawEllipse (c.x - rr, c.y - rr, 2.f * rr, 2.f * rr, big ? 2.5f : 1.6f);

        juce::Path ptr;
        ptr.startNewSubPath (0.f, -rr * 0.2f);
        ptr.lineTo (0.f, -rr * 0.88f);
        g.strokePath (ptr, juce::PathStrokeType (big ? 5.f : 3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                      juce::AffineTransform::rotation (ang).translated (c.x, c.y));
    }
};

struct Star
{
    float x = 0.f, y = 0.f, age = 0.f, life = 1.f, size = 3.f, phase = 0.f;
    bool alive = false;
};

// Capa transparente por encima de todo donde se dibujan las estrellitas
class SparkleLayer : public juce::Component
{
public:
    std::array<Star, 110> stars;
    juce::Colour fg { juce::Colours::white };
    float spark = 0.f;

    SparkleLayer() { setInterceptsMouseClicks (false, false); }

    void paint (juce::Graphics& g) override
    {
        for (const auto& s : stars)
        {
            if (! s.alive) continue;
            const float t = juce::jlimit (0.f, 1.f, s.age / s.life);
            const float e = std::sin (juce::MathConstants<float>::pi * t);
            const float a = e * e * (0.3f + 0.7f * spark);
            if (a < 0.02f) continue;

            const float tw = 0.75f + 0.25f * std::sin (s.phase + s.age * 9.f);
            const float L = s.size * tw;
            g.setColour (fg.withAlpha (a * 0.18f));
            g.fillEllipse (s.x - L * 1.6f, s.y - L * 1.6f, L * 3.2f, L * 3.2f);
            g.setColour (fg.withAlpha (juce::jmin (1.f, a)));
            g.drawLine (s.x - L, s.y, s.x + L, s.y, 1.2f);
            g.drawLine (s.x, s.y - L, s.x, s.y + L, 1.2f);
            const float d = L * 0.45f;
            g.drawLine (s.x - d, s.y - d, s.x + d, s.y + d, 0.8f);
            g.drawLine (s.x - d, s.y + d, s.x + d, s.y - d, 0.8f);
            g.fillEllipse (s.x - 1.3f, s.y - 1.3f, 2.6f, 2.6f);
        }
    }
};

class XpaceverbAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit XpaceverbAudioProcessorEditor (XpaceverbAudioProcessor&);
    ~XpaceverbAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    struct Pal { juce::Colour bg, fg, dim, fill; };

    Pal  pal() const;
    bool isDark() const;
    void applyTheme();
    void setParam (const juce::String& id, float value);
    int  getChoice (const char* id) const;
    void showModeMenu();
    void updateSparkles();
    void timerCallback() override;

    XpaceverbAudioProcessor& proc;
    XvLAF laf;
    SparkleLayer sparkles;

    std::array<juce::Slider, 15> knobs;
    std::array<std::unique_ptr<SliderAtt>, 15> atts;
    juce::TextButton themeBtn;

    juce::Rectangle<int> modeRect, colorRect, prevRect, nextRect;
    std::array<juce::Rectangle<int>, 5> boxes;

    juce::Random rng;
    float spark = 0.f, spawnAcc = 0.f;
    int lastBlock = 0;
    bool lastDark = true;
};
