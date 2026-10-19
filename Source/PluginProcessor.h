#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <vector>

class MidiEchoAudioProcessor final : public juce::AudioProcessor
{
public:
    static constexpr int maxDelayMs = 10000;

    MidiEchoAudioProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return maxDelayMs / 1000.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;

private:
    struct PendingEvent
    {
        juce::int64 time = 0; // absolute sample position
        juce::MidiMessage message;
    };

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    static bool shouldEcho (const juce::MidiMessage&);

    void clearPending();
    void trackEchoedNote (const juce::MidiMessage&);

    std::atomic<float>* delayMs = nullptr;
    double currentSampleRate = 44100.0;
    juce::int64 samplePosition = 0;

    // Preallocated in the constructor; the audio thread never grows it.
    std::vector<PendingEvent> pending;
    juce::MidiBuffer output;

    // How many echoed note-ons are still waiting for their note-off, so that
    // bypass or reset can silence them instead of leaving stuck notes.
    std::array<std::array<int, 128>, 16> echoedNotesOn {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiEchoAudioProcessor)
};
