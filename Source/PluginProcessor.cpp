#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <juce_audio_basics/juce_audio_basics.h>

// Constructor and Destructor for the Audio Processor
DemuextractAudioProcessor::DemuextractAudioProcessor() {}
DemuextractAudioProcessor::~DemuextractAudioProcessor() {}

void DemuextractAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {}
void DemuextractAudioProcessor::releaseResources() {}

// Clears unused audio buffer channels for clean processing
void DemuextractAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
}

// -------------------------------------------------------------------------------------------
// Tempo Detection (unchanged logic, just for reference)

void DemuextractAudioProcessor::openFileAndDetectTempo()
{
    fileChooser = std::make_unique<juce::FileChooser>("Select an audio file to detect tempo", juce::File{}, "*.wav;*.mp3");

    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            juce::File selectedFile = fc.getResult();
            if (selectedFile.existsAsFile())
            {
                DBG("File selected for tempo detection: " + selectedFile.getFullPathName());

                juce::String pythonScriptPath = "C:\\Users\\Bederib\\Desktop\\Demuextract\\Builds\\VisualStudio2022\\tempo_detection.py";
                juce::String command = "python \"" + pythonScriptPath + "\" \"" + selectedFile.getFullPathName() + "\"";

                DBG("Executing tempo detection command: " + command);

                try
                {
                    std::string output = runPythonScript(command.toStdString());
                    DBG("Python script output: " + juce::String(output));

                    std::stringstream ss(output);
                    std::string line;
                    detectedTempo = 0.0f;

                    while (std::getline(ss, line))
                    {
                        if (line.find("tempo,") == 0)
                        {
                            auto start = line.find("[[");
                            auto end = line.find("]]");
                            if (start != std::string::npos && end != std::string::npos)
                            {
                                std::string tempoStr = line.substr(start + 2, end - start - 2);
                                detectedTempo = std::stof(tempoStr);

                                // *** ROUND THE TEMPO ***
                                detectedTempo = std::round(detectedTempo);  // e.g. 120.185 => 120
                                DBG("Rounded tempo to nearest integer: " + juce::String(detectedTempo));
                            }
                            break;
                        }
                    }

                    if (detectedTempo <= 0.0f)
                    {
                        detectedTempo = 120.0f; // Fallback tempo
                        DBG("Fallback tempo applied: " + juce::String(detectedTempo));
                    }

                    if (auto* editor = dynamic_cast<DemuextractAudioProcessorEditor*>(getActiveEditor()))
                    {
                        editor->setTempoLabel(detectedTempo);
                        editor->setSongTitleLabel(selectedFile.getFileNameWithoutExtension());
                    }
                }
                catch (const std::exception& e)
                {
                    DBG("Exception during tempo detection: " + juce::String(e.what()));
                }
            }
            else
            {
                DBG("No file selected or file does not exist.");
            }
        });
}


// -------------------------------------------------------------------------------------------
// Another tempo detection function (unchanged logic)

void DemuextractAudioProcessor::runTempoDetection(const juce::File& originalFile, const juce::String& outputDirectory)
{
    juce::File drumStem = juce::File(outputDirectory).getChildFile("htdemucs").getChildFile("drums.wav");
    if (drumStem.existsAsFile())
    {
        juce::String pythonScriptPath = "C:\\Users\\Bederib\\Desktop\\Demuextract\\Builds\\VisualStudio2022\\tempo_detection.py";
        juce::String quotedDrumStemPath = "\"" + drumStem.getFullPathName() + "\"";
        juce::String tempoCommand = "python \"" + pythonScriptPath + "\" " + quotedDrumStemPath;

        std::string output = runPythonScript(tempoCommand.toStdString());

        std::stringstream ss(output);
        std::string line;
        detectedTempo = 0.0f;

        while (std::getline(ss, line))
        {
            if (line.find("tempo,") == 0)
            {
                auto start = line.find('[');
                auto end = line.find(']');
                if (start != std::string::npos && end != std::string::npos)
                {
                    std::string tempoStr = line.substr(start + 1, end - start - 1);
                    detectedTempo = std::stof(tempoStr);
                }
                break;
            }
        }

        if (auto* editor = dynamic_cast<DemuextractAudioProcessorEditor*>(getActiveEditor()))
        {
            editor->setTempoLabel(detectedTempo);
        }
    }
    else
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Error",
            "Drum stem file not found in the output directory. Ensure extraction completed successfully.");
    }
}

// -------------------------------------------------------------------------------------------
// Onset detection / pattern extraction

void DemuextractAudioProcessor::extractDrumPattern()
{
    if (detectedTempo <= 0.0f)
    {
        DBG("[New Code] Tempo has not been detected. Running tempo detection first.");
        fileChooser = std::make_unique<juce::FileChooser>("Select an audio file for tempo detection", juce::File{}, "*.wav;*.mp3");
        fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc)
            {
                juce::File selectedFile = fc.getResult();
                if (selectedFile.existsAsFile())
                {
                    DBG("[New Code] File selected for tempo detection: " + selectedFile.getFullPathName());

                    juce::String pythonScriptPath = "C:\\Users\\Bederib\\Desktop\\Demuextract\\Builds\\VisualStudio2022\\tempo_detection.py";
                    juce::String quotedFilePath = "\"" + selectedFile.getFullPathName() + "\"";
                    juce::String tempoCommand = "python \"" + pythonScriptPath + "\" " + quotedFilePath;

                    try
                    {
                        std::string output = runPythonScript(tempoCommand.toStdString());
                        DBG("[New Code] Python script output: " + juce::String(output));

                        std::stringstream ss(output);
                        std::string line;
                        detectedTempo = 0.0f;

                        while (std::getline(ss, line))
                        {
                            DBG("[New Code] Processing line: " + juce::String(line));
                            if (line.find("tempo,") == 0)
                            {
                                auto start = line.find("[[");
                                auto end = line.find("]]");
                                if (start != std::string::npos && end != std::string::npos)
                                {
                                    std::string tempoStr = line.substr(start + 2, end - start - 2);
                                    detectedTempo = std::stof(tempoStr);
                                    DBG("[New Code] Detected tempo: " + juce::String(detectedTempo));
                                    break;
                                }
                            }
                        }

                        if (detectedTempo <= 0.0f)
                        {
                            DBG("[New Code] Invalid tempo detected. Using fallback tempo.");
                            detectedTempo = 120.0f; // Fallback tempo
                        }

                        DBG("[New Code] Running drum pattern extraction using detected tempo: " + juce::String(detectedTempo));
                        runOnsetDetection();
                    }
                    catch (const std::exception& e)
                    {
                        DBG("[New Code] Exception during tempo detection: " + juce::String(e.what()));
                        juce::AlertWindow::showMessageBoxAsync(
                            juce::AlertWindow::WarningIcon,
                            "Error",
                            "Failed to detect tempo. Please check the Python script or file format.");
                    }
                }
                else
                {
                    DBG("[New Code] No file selected for tempo detection.");
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        "Error",
                        "No audio file selected. Please select a valid file.");
                }
            });
    }
    else
    {
        runOnsetDetection();
    }
}

void DemuextractAudioProcessor::extractClapsSnares()
{
    if (detectedTempo <= 0.0f)
    {
        DBG("[New Code] Tempo has not been detected. Running tempo detection first.");
        fileChooser = std::make_unique<juce::FileChooser>("Select an audio file for tempo detection", juce::File{}, "*.wav;*.mp3");
        fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc)
            {
                juce::File selectedFile = fc.getResult();
                if (selectedFile.existsAsFile())
                {
                    DBG("[New Code] File selected for tempo detection: " + selectedFile.getFullPathName());

                    juce::String pythonScriptPath = "C:\\Users\\Bederib\\Desktop\\Demuextract\\Builds\\VisualStudio2022\\tempo_detection.py";
                    juce::String quotedFilePath = "\"" + selectedFile.getFullPathName() + "\"";
                    juce::String tempoCommand = "python \"" + pythonScriptPath + "\" " + quotedFilePath;

                    try
                    {
                        std::string output = runPythonScript(tempoCommand.toStdString());
                        DBG("[New Code] Python script output: " + juce::String(output));

                        std::stringstream ss(output);
                        std::string line;
                        detectedTempo = 0.0f;

                        while (std::getline(ss, line))
                        {
                            DBG("[New Code] Processing line: " + juce::String(line));
                            if (line.find("tempo,") == 0)
                            {
                                auto start = line.find("[[");
                                auto end = line.find("]]");
                                if (start != std::string::npos && end != std::string::npos)
                                {
                                    std::string tempoStr = line.substr(start + 2, end - start - 2);
                                    detectedTempo = std::stof(tempoStr);
                                    DBG("[New Code] Detected tempo: " + juce::String(detectedTempo));
                                    break;
                                }
                            }
                        }

                        if (detectedTempo <= 0.0f)
                        {
                            DBG("[New Code] Invalid tempo detected. Using fallback tempo.");
                            detectedTempo = 120.0f; // Fallback tempo
                        }

                        DBG("[New Code] Running claps/snares extraction using detected tempo: " + juce::String(detectedTempo));
                        runClapsSnaresDetection();
                    }
                    catch (const std::exception& e)
                    {
                        DBG("[New Code] Exception during tempo detection: " + juce::String(e.what()));
                        juce::AlertWindow::showMessageBoxAsync(
                            juce::AlertWindow::WarningIcon,
                            "Error",
                            "Failed to detect tempo. Please check the Python script or file format.");
                    }
                }
                else
                {
                    DBG("[New Code] No file selected for tempo detection.");
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        "Error",
                        "No audio file selected. Please select a valid file.");
                }
            });
    }
    else
    {
        runClapsSnaresDetection();
    }
}

void DemuextractAudioProcessor::runClapsSnaresDetection()
{
    fileChooser = std::make_unique<juce::FileChooser>("Select the audio file for claps/snares extraction", juce::File{}, "*.wav");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            juce::File selectedFile = fc.getResult();
            if (selectedFile.existsAsFile())
            {
                DBG("[New Code] File selected for claps/snares extraction: " + selectedFile.getFullPathName());

                juce::String pythonScriptPath = "C:\\Users\\Bederib\\Desktop\\Debug balmala\\Demuextract\\Builds\\VisualStudio2022\\onset_detection2.py";
                juce::String quotedFilePath = "\"" + selectedFile.getFullPathName() + "\"";
                juce::String onsetCommand = "python \"" + pythonScriptPath + "\" " + quotedFilePath;

                try
                {
                    std::string output = runPythonScript(onsetCommand.toStdString());
                    DBG("[New Code] Onset detection output: " + juce::String(output));

                    // Convert onset times to MIDI using previously detected tempo
                    convertOnsetTimesToMidi(output, selectedFile);
                }
                catch (const std::exception& e)
                {
                    DBG("[New Code] Exception during claps/snares extraction: " + juce::String(e.what()));
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        "Error",
                        "Failed to extract claps/snares. Please check the Python script or file format.");
                }
            }
            else
            {
                DBG("[New Code] No file selected for claps/snares extraction.");
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "Error",
                    "No audio file selected. Please select a valid file.");
            }
        });
}



void DemuextractAudioProcessor::runOnsetDetection()
{
    fileChooser = std::make_unique<juce::FileChooser>("Select the drum stem for extraction", juce::File{}, "*.wav");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            juce::File drumStem = fc.getResult();
            if (drumStem.existsAsFile())
            {
                DBG("[New Code] File selected for drum pattern extraction: " + drumStem.getFullPathName());

                juce::String pythonScriptPath = "C:\\Users\\Bederib\\Desktop\\Demuextract\\Builds\\VisualStudio2022\\onset_detection.py";
                juce::String quotedDrumStemPath = "\"" + drumStem.getFullPathName() + "\"";
                juce::String onsetCommand = "python \"" + pythonScriptPath + "\" " + quotedDrumStemPath;

                try
                {
                    std::string output = runPythonScript(onsetCommand.toStdString());
                    DBG("[New Code] Onset detection output: " + juce::String(output));

                    // Convert onset times to MIDI using previously detected tempo
                    convertOnsetTimesToMidi(output, drumStem);
                }
                catch (const std::exception& e)
                {
                    DBG("[New Code] Exception during onset detection: " + juce::String(e.what()));
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        "Error",
                        "Failed to extract drum pattern. Please check the Python script or file format.");
                }
            }
            else
            {
                DBG("[New Code] No file selected for onset detection.");
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "Error",
                    "No drum stem selected. Please select a valid file.");
            }
        });
}

// -------------------------------------------------------------------------------------------
// Python Script Execution

std::string DemuextractAudioProcessor::runPythonScript(const std::string& command)
{
    std::array<char, 128> buffer;
    std::string result;

    DBG("[New Code] Running Python script command: " + juce::String(command));

    std::shared_ptr<FILE> pipe(_popen(command.c_str(), "r"), _pclose);
    if (!pipe)
    {
        DBG("[New Code] Failed to open pipe for Python script execution.");
        throw std::runtime_error("Failed to open pipe for Python script execution.");
    }

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }

    DBG("[New Code] Raw Python script output: " + juce::String(result));
    return result;
}

// -------------------------------------------------------------------------------------------
// The CRUCIAL part: We multiply the final tick position by 4 so it should 4x space

void DemuextractAudioProcessor::convertOnsetTimesToMidi(const std::string& onsetTimes, const juce::File& selectedFile)
{
    DBG("[Tempo+Offset] Parsing tempo via bracket cleanup, then offset onsets by 0.13s...");

    try
    {
        juce::MidiBuffer midiBuffer;

        // ---------------------------
        // PASS (A): Find & parse the tempo line
        // ---------------------------
        float rawTempo = 0.0f;
        bool foundTempo = false;

        {
            std::stringstream tempoScan(onsetTimes);
            std::string line;

            while (std::getline(tempoScan, line))
            {
                // Does it begin with "tempo,"?
                if (line.rfind("tempo,", 0) == 0)
                {
                    // e.g. "tempo,[[[120.18531977]]]"
                    auto firstBracketPos = line.find('[');
                    auto lastBracketPos = line.rfind(']');

                    if (firstBracketPos != std::string::npos &&
                        lastBracketPos != std::string::npos &&
                        lastBracketPos > firstBracketPos)
                    {
                        // Extract the substring inside
                        std::string inside = line.substr(
                            firstBracketPos + 1,
                            lastBracketPos - (firstBracketPos + 1)
                        );
                        // e.g. inside = "[120.18531977]]"

                        // Remove leading/trailing brackets
                        while (!inside.empty() && inside.front() == '[')
                            inside.erase(inside.begin());
                        while (!inside.empty() && inside.back() == ']')
                            inside.pop_back();

                        // Now should be something like "120.18531977"
                        DBG("[Tempo+Offset] Extracted tempo substring: " + juce::String(inside));

                        try
                        {
                            float parsed = std::stof(inside);
                            // Optional: round to nearest int
                            // parsed = std::round(parsed);

                            rawTempo = parsed;
                            DBG("[Tempo+Offset] Final BPM => " + juce::String(rawTempo));
                            foundTempo = true;
                        }
                        catch (const std::exception& e)
                        {
                            DBG("[Tempo+Offset] Failed to parse tempo: " + juce::String(e.what()));
                        }
                    }

                    break; // once we find a "tempo," line, we can exit the while
                }
            }
        }

        // If we never found or parsed a valid tempo, fallback:
        if (!foundTempo || rawTempo <= 0.0f)
        {
            rawTempo = 120.0f;
            DBG("[Tempo+Offset] Using fallback BPM: " + juce::String(rawTempo));
        }

        // Convert tempo to ticks/sec
        double detectedTempoD = static_cast<double>(rawTempo);
        double beatsPerSecond = detectedTempoD / 60.0;
        double ticksPerSecond = beatsPerSecond * 960.0;

        DBG("[Tempo+Offset] Using ticks/sec = " + juce::String(ticksPerSecond, 3));

        // ---------------------------
        // PASS (B): Read onset lines, offset by -0.13s,
        // then halve the tick positions
        // ---------------------------
        std::stringstream onsetScan(onsetTimes);
        std::string line;

        static constexpr double SHIFT_AMOUNT = 0.16; // 130 ms earlier

        while (std::getline(onsetScan, line))
        {
            // We'll skip the tempo or debug lines here
            if (line.rfind("tempo,", 0) == 0 || line.rfind("Debug:", 0) == 0)
                continue;

            std::stringstream lineStream(line);
            std::string timeStr, typeStr;
            std::getline(lineStream, timeStr, ',');
            std::getline(lineStream, typeStr, ',');

            // Handle different types of onsets
            if (timeStr.empty() || typeStr.empty())
                continue;

            int midiNote = 0;
            if (typeStr == "kick")
                midiNote = 36; // MIDI note for kick
            else if (typeStr == "clap" || typeStr == "snare")
                midiNote = 38; // MIDI note for clap/snare

            if (midiNote == 0)
                continue;

            try
            {
                double onsetTime = std::stod(timeStr);

                // Shift onset earlier by 0.13 s
                double shiftedTime = onsetTime - SHIFT_AMOUNT;
                if (shiftedTime < 0.0)
                    shiftedTime = 0.0;

                // Convert to ticks
                int baseTickPos = static_cast<int>(shiftedTime * ticksPerSecond);

                // ***** Halve the final tick position *****
                int finalTickPos = baseTickPos / 2;  // e.g. 1650 => 825

                DBG("[Tempo+Offset] onset=" + juce::String(onsetTime, 3)
                    + " => shifted=" + juce::String(shiftedTime, 3)
                    + " => baseTick=" + juce::String(baseTickPos)
                    + " => halfTick=" + juce::String(finalTickPos));

                // Place note on/off
                midiBuffer.addEvent(juce::MidiMessage::noteOn(1, midiNote, (juce::uint8)127), finalTickPos);
                midiBuffer.addEvent(juce::MidiMessage::noteOff(1, midiNote), finalTickPos + 480);
            }
            catch (...)
            {
                // skip invalid lines
            }
        }

        // If no events
        if (midiBuffer.getNumEvents() == 0)
        {
            DBG("[Tempo+Offset] No MIDI events generated.");
            return;
        }

        // Finally save the MIDI file
        juce::String outputMidiPath = "C:\\Users\\Bederib\\Desktop\\stems\\claps_snares.mid";
        saveMidiToFile(outputMidiPath, midiBuffer);
        DBG("[Tempo+Offset] MIDI file saved to: " + outputMidiPath);
    }
    catch (const std::exception& e)
    {
        DBG("[Tempo+Offset] Exception: " + juce::String(e.what()));
    }
}










// -------------------------------------------------------------------------------------------
// Writes the actual MIDI file with PPQ = 960
void DemuextractAudioProcessor::saveMidiToFile(const juce::String& filePath, const juce::MidiBuffer& midiBuffer)
{
    juce::File outputFile(filePath);
    std::unique_ptr<juce::FileOutputStream> outputStream(outputFile.createOutputStream());

    if (outputStream)
    {
        juce::MidiFile midiFile;
        midiFile.setTicksPerQuarterNote(960);  // ensure 960 PPQ in the file

        // Build your track data
        juce::MidiMessageSequence sequence;
        for (const auto& metadata : midiBuffer)
        {
            sequence.addEvent(metadata.getMessage(), metadata.samplePosition);
        }

        midiFile.addTrack(sequence);
        midiFile.writeTo(*outputStream);
    }
}

// -------------------------------------------------------------------------------------------
// Demucs extraction code (unchanged)
void DemuextractAudioProcessor::openFileAndRunDemucs()
{
    fileChooser = std::make_unique<juce::FileChooser>("Select an audio file to extract stems", juce::File{}, "*.wav;*.mp3");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            juce::File selectedFile = fc.getResult();
            if (selectedFile.existsAsFile())
            {
                juce::String quotedFilePath = "\"" + selectedFile.getFullPathName() + "\"";
                juce::String outputDirectory = "C:\\Users\\Bederib\\Desktop\\stems\\htdemucs";
                juce::String demucsCommand = "demucs -n htdemucs " + quotedFilePath + " -o \"" + outputDirectory + "\"";

                int result = std::system(demucsCommand.toRawUTF8());
                if (result == 0)
                {
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::InfoIcon,
                        "Success",
                        "Drum stems successfully extracted for: " + selectedFile.getFileName());
                }
                else
                {
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        "Error",
                        "Failed to extract stems. Ensure Demucs is installed and configured correctly.");
                }
            }
            else
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "Error",
                    "No audio file selected. Please select a valid file.");
            }
        });
}

// -------------------------------------------------------------------------------------------
// JUCE Boilerplate
bool DemuextractAudioProcessor::hasEditor() const { return true; }
const juce::String DemuextractAudioProcessor::getName() const { return JucePlugin_Name; }
bool DemuextractAudioProcessor::acceptsMidi() const { return false; }
bool DemuextractAudioProcessor::producesMidi() const { return false; }
bool DemuextractAudioProcessor::isMidiEffect() const { return false; }
double DemuextractAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int DemuextractAudioProcessor::getNumPrograms() { return 1; }
int DemuextractAudioProcessor::getCurrentProgram() { return 0; }
void DemuextractAudioProcessor::setCurrentProgram(int index) {}
const juce::String DemuextractAudioProcessor::getProgramName(int index) { return {}; }
void DemuextractAudioProcessor::changeProgramName(int index, const juce::String& newName) {}
void DemuextractAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {}
void DemuextractAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {}

juce::AudioProcessor* createPluginFilter()
{
    return new DemuextractAudioProcessor();
}
juce::AudioProcessorEditor* DemuextractAudioProcessor::createEditor()
{
    return new DemuextractAudioProcessorEditor(*this);
}