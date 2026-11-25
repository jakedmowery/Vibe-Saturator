#include "PluginProcessor.h"
#include "PluginEditor.h"

SaturatorProcessor::SaturatorProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    driveParam  = apvts.getRawParameterValue ("drive");
    curveParam  = apvts.getRawParameterValue ("curve");
    outputParam = apvts.getRawParameterValue ("output");
    mixParam    = apvts.getRawParameterValue ("mix");
    dcParam     = apvts.getRawParameterValue ("dc");
}

const juce::StringArray& SaturatorProcessor::curveNames()
{
    static const juce::StringArray names { "Analog Clip", "Soft Sine", "Medium",
                                           "Hard", "Sinoid Fold", "Digital Clip" };
    return names;
}

juce::AudioProcessorValueTreeState::ParameterLayout SaturatorProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "drive", 1 }, "Drive",
        NormalisableRange<float> (-36.0f, 36.0f, 0.1f), 5.0f,
        AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { "curve", 1 }, "Curve", curveNames(), 0));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "output", 1 }, "Output",
        NormalisableRange<float> (-36.0f, 12.0f, 0.1f), 0.0f,
        AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "mix", 1 }, "Dry/Wet",
        NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f,
        AudioParameterFloatAttributes().withLabel ("%")));

    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { "dc", 1 }, "DC Filter", false));

    return layout;
}

bool SaturatorProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();

    if (in != out)
        return false;

    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void SaturatorProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto numChannels = (juce::uint32) juce::jmax (1, getTotalNumOutputChannels());
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, numChannels };

    oversampling.initProcessing ((size_t) samplesPerBlock);
    oversampling.reset();

    mixer.prepare (spec);
    mixer.setWetLatency (oversampling.getLatencyInSamples());
    setLatencySamples (juce::roundToInt (oversampling.getLatencyInSamples()));

    driveSmoothed.reset (sampleRate * 4.0, 0.02);
    driveSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (driveParam->load()));

    outputSmoothed.reset (sampleRate, 0.02);
    outputSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outputParam->load()));

    // ~10 Hz high-pass, only applied to the wet signal when "DC Filter" is on.
    auto dcCoeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass (sampleRate, 10.0f);
    for (auto& f : dcFilters)
    {
        f.coefficients = dcCoeffs;
        f.reset();
    }

    driveGains.assign ((size_t) samplesPerBlock * 4, 1.0f);
}

void SaturatorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    juce::dsp::AudioBlock<float> block (buffer);

    // Keep a copy of the dry signal for the dry/wet mix.
    mixer.setWetMixProportion (juce::jlimit (0.0f, 1.0f, mixParam->load() * 0.01f));
    mixer.pushDrySamples (block);

    // Up to 4x sample rate.
    auto osBlock = oversampling.processSamplesUp (block);
    const size_t osSamples = osBlock.getNumSamples();

    if (driveGains.size() < osSamples)
        driveGains.resize (osSamples, 1.0f);

    driveSmoothed.setTargetValue (juce::Decibels::decibelsToGain (driveParam->load()));
    for (size_t i = 0; i < osSamples; ++i)
        driveGains[i] = driveSmoothed.getNextValue();

    // Waveshape.
    const int curve = (int) curveParam->load();
    for (size_t ch = 0; ch < osBlock.getNumChannels(); ++ch)
    {
        float* d = osBlock.getChannelPointer (ch);
        for (size_t i = 0; i < osSamples; ++i)
            d[i] = vibe::shape (curve, d[i] * driveGains[i]);
    }

    // Back to the host sample rate.
    oversampling.processSamplesDown (block);

    // Optional DC filter on the wet signal (always run so its state stays current).
    const bool dcOn = dcParam->load() > 0.5f;
    for (int ch = 0; ch < buffer.getNumChannels() && ch < (int) dcFilters.size(); ++ch)
    {
        float* d = buffer.getWritePointer (ch);
        auto& filter = dcFilters[(size_t) ch];
        for (int i = 0; i < numSamples; ++i)
        {
            const float filtered = filter.processSample (d[i]);
            if (dcOn)
                d[i] = filtered;
        }
    }

    // Mix dry and wet (latency-compensated).
    mixer.mixWetSamples (block);

    // Output level.
    outputSmoothed.setTargetValue (juce::Decibels::decibelsToGain (outputParam->load()));
    for (int i = 0; i < numSamples; ++i)
    {
        const float g = outputSmoothed.getNextValue();
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.getWritePointer (ch)[i] *= g;
    }
}

juce::AudioProcessorEditor* SaturatorProcessor::createEditor()
{
    return new SaturatorEditor (*this);
}

void SaturatorProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void SaturatorProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SaturatorProcessor();
}
