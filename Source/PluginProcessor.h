#pragma once

#include <array>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "Shaper.h"

class SaturatorProcessor : public juce::AudioProcessor
{
public:
    SaturatorProcessor();
    ~SaturatorProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static const juce::StringArray& curveNames();

    juce::AudioProcessorValueTreeState apvts;

private:
    std::atomic<float>* driveParam  = nullptr;
    std::atomic<float>* curveParam  = nullptr;
    std::atomic<float>* outputParam = nullptr;
    std::atomic<float>* mixParam    = nullptr;
    std::atomic<float>* dcParam     = nullptr;

    // 4x oversampling (2 stages) keeps the harsh curves from aliasing.
    juce::dsp::Oversampling<float> oversampling { 2, 2,
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true };

    // Dry/wet mixer that also compensates for the oversampling latency.
    juce::dsp::DryWetMixer<float> mixer { 1024 };

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> driveSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> outputSmoothed;

    std::array<juce::dsp::IIR::Filter<float>, 2> dcFilters;
    std::vector<float> driveGains;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SaturatorProcessor)
};
