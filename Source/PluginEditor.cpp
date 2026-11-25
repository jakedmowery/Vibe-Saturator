#include "PluginEditor.h"

namespace
{
    const juce::Colour bgColour     { 0xff1b1d22 };
    const juce::Colour panelColour  { 0xff252830 };
    const juce::Colour accentColour { 0xffffb347 };
    const juce::Colour textColour   { 0xffe6e6e6 };
}

SaturatorEditor::SaturatorEditor (SaturatorProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (520, 320);

    curveBox.addItemList (SaturatorProcessor::curveNames(), 1);
    addAndMakeVisible (curveBox);

    setupKnob (driveSlider,  driveLabel,  "Drive",   " dB");
    setupKnob (outputSlider, outputLabel, "Output",  " dB");
    setupKnob (mixSlider,    mixLabel,    "Dry/Wet", " %");

    dcButton.setColour (juce::ToggleButton::textColourId, textColour);
    addAndMakeVisible (dcButton);

    auto& apvts = processor.apvts;
    driveAttachment  = std::make_unique<SliderAttachment>   (apvts, "drive",  driveSlider);
    outputAttachment = std::make_unique<SliderAttachment>   (apvts, "output", outputSlider);
    mixAttachment    = std::make_unique<SliderAttachment>   (apvts, "mix",    mixSlider);
    curveAttachment  = std::make_unique<ComboBoxAttachment> (apvts, "curve",  curveBox);
    dcAttachment     = std::make_unique<ButtonAttachment>   (apvts, "dc",     dcButton);

    startTimerHz (30);
}

SaturatorEditor::~SaturatorEditor()
{
    stopTimer();
}

void SaturatorEditor::setupKnob (juce::Slider& s, juce::Label& l,
                                 const juce::String& name, const juce::String& suffix)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    s.setTextValueSuffix (suffix);
    s.setColour (juce::Slider::rotarySliderFillColourId, accentColour);
    s.setColour (juce::Slider::textBoxTextColourId, textColour);
    s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (s);

    l.setText (name, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
    l.setColour (juce::Label::textColourId, textColour);
    addAndMakeVisible (l);
}

void SaturatorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bgColour);

    g.setColour (accentColour);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("Vibe Saturator", 20, 12, 300, 30, juce::Justification::centredLeft);

    // Transfer curve: input level on X, output level on Y.
    g.setColour (panelColour);
    g.fillRoundedRectangle (graphBounds.toFloat(), 8.0f);

    g.saveState();
    g.reduceClipRegion (graphBounds);

    const auto area = graphBounds.toFloat().reduced (8.0f);
    const float cx = area.getCentreX();
    const float cy = area.getCentreY();

    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawHorizontalLine ((int) cy, area.getX(), area.getRight());
    g.drawVerticalLine   ((int) cx, area.getY(), area.getBottom());

    const float gain = juce::Decibels::decibelsToGain (processor.apvts.getRawParameterValue ("drive")->load());
    const int curve  = (int) processor.apvts.getRawParameterValue ("curve")->load();

    juce::Path path;
    const int steps = (int) area.getWidth();
    for (int i = 0; i <= steps; ++i)
    {
        const float x = juce::jmap ((float) i, 0.0f, (float) steps, -1.0f, 1.0f);
        const float y = juce::jlimit (-1.0f, 1.0f, vibe::shape (curve, x * gain));
        const float px = area.getX() + (x * 0.5f + 0.5f) * area.getWidth();
        const float py = area.getBottom() - (y * 0.5f + 0.5f) * area.getHeight();

        if (i == 0) path.startNewSubPath (px, py);
        else        path.lineTo (px, py);
    }

    g.setColour (accentColour);
    g.strokePath (path, juce::PathStrokeType (2.0f));
    g.restoreState();
}

void SaturatorEditor::resized()
{
    graphBounds = { 20, 60, 220, 220 };

    curveBox.setBounds (260, 60, 240, 28);

    const int knobW = 80;
    const int knobY = 120;
    for (int i = 0; i < 3; ++i)
    {
        const int x = 260 + i * (knobW + 0) + (i * 0);
        juce::Slider* sliders[] = { &driveSlider, &outputSlider, &mixSlider };
        juce::Label*  labels[]  = { &driveLabel,  &outputLabel,  &mixLabel };
        labels[i]->setBounds (x, knobY, knobW, 20);
        sliders[i]->setBounds (x, knobY + 20, knobW, 110);
    }

    dcButton.setBounds (260, 255, 240, 28);
}
