#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"


// Forward declaration of DemuextractAudioProcessor
class DemuextractAudioProcessor;

class DemuextractAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    DemuextractAudioProcessorEditor(DemuextractAudioProcessor&);
    ~DemuextractAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // Function to set the tempo on the label
    void setTempoLabel(float tempo);

    // Function to set the song title on the label
    void setSongTitleLabel(const juce::String& title);

private:
    DemuextractAudioProcessor& audioProcessor;


    juce::Image backgroundImage;
    juce::TextButton extractStemsButton;
    juce::TextButton extractDrumPatternButton;
    juce::TextButton detectTempoButton;
    juce::Label tempoLabel;  // Label to display the tempo
    juce::Label songTitleLabel;  // Label to display the song title

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DemuextractAudioProcessorEditor)
};
