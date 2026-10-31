#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout XpaceverbAudioProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;

    auto addF = [&] (const char* id, const char* name, float lo, float hi, float step, float skew, float def)
    {
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
                                                      NormalisableRange<float> (lo, hi, step, skew), def));
    };

    addF ("mix",      "Mix",         0.f,   100.f,   0.1f,  1.f,   35.f);
    addF ("decay",    "Decay",       0.2f,  40.f,    0.01f, 0.35f, 3.f);
    addF ("predelay", "Predelay",    0.f,   500.f,   0.1f,  0.5f,  20.f);
    addF ("hfreq",    "HighFreq",    1000.f, 20000.f, 1.f,  0.4f,  6000.f);
    addF ("hshelf",   "HighShelf",  -24.f,  0.f,     0.01f, 1.f,  -6.f);
    addF ("bfreq",    "BassFreq",    50.f,  1000.f,  1.f,   0.5f,  500.f);
    addF ("bmult",    "BassMult",    0.5f,  3.f,     0.01f, 1.f,   1.2f);
    addF ("size",     "Size",        0.f,   100.f,   0.1f,  1.f,   100.f);
    addF ("attack",   "Attack",      0.f,   100.f,   0.1f,  1.f,   50.f);
    addF ("early",    "Early",       0.f,   100.f,   0.1f,  1.f,   100.f);
    addF ("late",     "Late",        0.f,   100.f,   0.1f,  1.f,   100.f);
    addF ("rate",     "Mod Rate",    0.1f,  8.f,     0.01f, 0.5f,  1.5f);
    addF ("depth",    "Mod Depth",   0.f,   100.f,   0.1f,  1.f,   30.f);
    addF ("hicut",    "HighCut",     1000.f, 20000.f, 1.f,  0.4f,  10000.f);
    addF ("locut",    "LowCut",      10.f,  1000.f,  1.f,   0.4f,  20.f);

    StringArray modes;
    for (int i = 0; i < ReverbEngine::kModes; ++i) modes.add (ReverbEngine::modeName (i));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Mode", modes, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "color", 1 }, "Color",
                                                   StringArray { "NOW", "1970s", "1980s" }, 0));
    return l;
}

XpaceverbAudioProcessor::XpaceverbAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

bool XpaceverbAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    auto o = layouts.getMainOutputChannelSet();
    if (o != juce::AudioChannelSet::mono() && o != juce::AudioChannelSet::stereo()) return false;
    return o == layouts.getMainInputChannelSet();
}

void XpaceverbAudioProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    rmsSm = 0.f;
}

void XpaceverbAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (n == 0 || buffer.getNumChannels() == 0) return;

    auto P = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    ReverbParams p;
    p.mix        = P ("mix") * 0.01f;
    p.decay      = P ("decay");
    p.predelayMs = P ("predelay");
    p.hfreq      = P ("hfreq");
    p.hshelfDb   = P ("hshelf");
    p.bfreq      = P ("bfreq");
    p.bmult      = P ("bmult");
    p.size       = P ("size") * 0.01f;
    p.attack     = P ("attack") * 0.01f;
    p.early      = P ("early") * 0.01f;
    p.late       = P ("late") * 0.01f;
    p.rate       = P ("rate");
    p.depth      = P ("depth") * 0.01f;
    p.hicut      = P ("hicut");
    p.locut      = P ("locut");
    p.mode       = (int) std::round (P ("mode"));
    p.color      = (int) std::round (P ("color"));

    float* l = buffer.getWritePointer (0);
    float* r = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    if (r == nullptr)
    {
        // mono: se procesa como estereo con una copia y se devuelve el canal izquierdo
        std::vector<float> tmp ((size_t) n);
        std::copy (l, l + n, tmp.begin());
        engine.process (l, tmp.data(), n, p);
    }
    else engine.process (l, r, n, p);

    // medidor de la parte humeda para la animacion
    const float target = engine.getBlockWetRms();
    rmsSm += (target - rmsSm) * (target > rmsSm ? 0.5f : 0.1f);
    wetRms.store (rmsSm);
    blockCounter.fetch_add (1);
}

juce::AudioProcessorEditor* XpaceverbAudioProcessor::createEditor()
{
    return new XpaceverbAudioProcessorEditor (*this);
}

void XpaceverbAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void XpaceverbAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new XpaceverbAudioProcessor();
}
