#pragma once

#include <JuceHeader.h>

class DemuextractAudioProcessor : public juce::AudioProcessor
{
public:
    DemuextractAudioProcessor();
    ~DemuextractAudioProcessor() override;

    void runTempoDetection(const juce::File& originalFile, const juce::String& outputDirectory);
    void openFileAndDetectTempo(); // Ensure it has NO parameters here


    void saveMidiToFile(const juce::String& filePath, const juce::MidiBuffer& midiBuffer);


    void runOnsetDetection(); // Declare the function here


    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void openFileAndRunDemucs();
    void extractDrumPattern();
    void openFileAndRetrieveTempo(); // <-- Add this line
    float getDetectedTempo() const { return detectedTempo; }

private:
    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<juce::Thread> backgroundThread;
    float detectedTempo = 0.0f;

    std::string runPythonScript(const std::string& command);
    void convertOnsetTimesToMidi(const std::string& onsetTimes, const juce::File& selectedFile);
    //void saveMidiToFile(const juce::String& filePath, juce::MidiBuffer& midiBuffer);


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DemuextractAudioProcessor)
};

