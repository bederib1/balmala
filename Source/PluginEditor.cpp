#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <juce_audio_basics/juce_audio_basics.h>

DemuextractAudioProcessorEditor::DemuextractAudioProcessorEditor(DemuextractAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(400, 300);

    // Load the background image
    juce::File imageFile("C:/Users/Bederib/Pictures/Screenshots/Screenshot 2025-01-03 024448.png");
    backgroundImage = juce::ImageFileFormat::loadFrom(imageFile);

    // Existing buttons
    addAndMakeVisible(extractStemsButton);
    extractStemsButton.setButtonText("Extract Stems");
    extractStemsButton.onClick = [this] { audioProcessor.openFileAndRunDemucs(); };

    addAndMakeVisible(extractDrumPatternButton);
    extractDrumPatternButton.setButtonText("Extract Drum Pattern");
    extractDrumPatternButton.onClick = [this] { audioProcessor.extractDrumPattern(); };

    addAndMakeVisible(detectTempoButton);
    detectTempoButton.setButtonText("Detect Tempo");
    detectTempoButton.onClick = [this]
        {
            audioProcessor.openFileAndDetectTempo();
        };

    addAndMakeVisible(tempoLabel);
    tempoLabel.setText("Tempo: --", juce::dontSendNotification);
    tempoLabel.setFont(juce::Font(15.0f));
    tempoLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(songTitleLabel);
    songTitleLabel.setText("Song Title: --", juce::dontSendNotification);
    songTitleLabel.setFont(juce::Font(15.0f));
    songTitleLabel.setJustificationType(juce::Justification::centred);
}

DemuextractAudioProcessorEditor::~DemuextractAudioProcessorEditor() {}

void DemuextractAudioProcessorEditor::paint(juce::Graphics& g)
{
    if (backgroundImage.isValid())
    {
        g.drawImage(backgroundImage, getLocalBounds().toFloat());
    }
    else
    {
        g.fillAll(juce::Colours::black);
        g.setColour(juce::Colours::white);
        g.setFont(15.0f);
        g.drawFittedText("Stem Rippa", getLocalBounds(), juce::Justification::centred, 1);
    }
}

void DemuextractAudioProcessorEditor::resized()
{
    int buttonWidth = 140;
    int buttonHeight = 30;
    int labelWidth = 150;
    int labelHeight = 30;
    int spacing = 10;

    // Position buttons on the left, make them slightly wider and shorter
    extractStemsButton.setBounds(spacing, spacing, buttonWidth, buttonHeight);
    extractDrumPatternButton.setBounds(spacing, extractStemsButton.getBottom() + spacing, buttonWidth, buttonHeight);
    detectTempoButton.setBounds(spacing, extractDrumPatternButton.getBottom() + spacing, buttonWidth, buttonHeight);

    // Position labels on the right, raise them higher and put song title first
    songTitleLabel.setBounds(getWidth() - labelWidth - spacing, spacing, labelWidth, labelHeight);
    tempoLabel.setBounds(getWidth() - labelWidth - spacing, songTitleLabel.getBottom() + spacing, labelWidth, labelHeight);
}

void DemuextractAudioProcessorEditor::setTempoLabel(float tempo)
{
    tempoLabel.setText("Tempo: " + juce::String(tempo, 2) + " BPM", juce::dontSendNotification);
}

void DemuextractAudioProcessorEditor::setSongTitleLabel(const juce::String& title)
{
    songTitleLabel.setText("Song Title: " + title, juce::dontSendNotification);
}
