#pragma once

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"

class SaturatorEditor : public juce::AudioProcessorEditor,
                        private juce::Timer
{
public:
    explicit SaturatorEditor (SaturatorProcessor&);
    ~SaturatorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override { repaint (graphBounds); }
    void setupKnob (juce::Slider&, juce::Label&, const juce::String& name, const juce::String& suffix);

    SaturatorProcessor& processor;
    juce::Rectangle<int> graphBounds;

    juce::ComboBox curveBox;
    juce::Slider driveSlider, outputSlider, mixSlider;
    juce::Label driveLabel, outputLabel, mixLabel;
    juce::ToggleButton dcButton { "DC Filter" };

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;

    // Declared last so they are destroyed before the controls they point at.
    std::unique_ptr<SliderAttachment> driveAttachment, outputAttachment, mixAttachment;
    std::unique_ptr<ComboBoxAttachment> curveAttachment;
    std::unique_ptr<ButtonAttachment> dcAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SaturatorEditor)
};
