#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

// Keyboard steps: arrows 1 ms, Shift+arrows 10 ms, Page Up/Down 100 ms,
// Ctrl+Page Up/Down 1000 ms, Home/End jump to the ends of the range.
class DelaySlider final : public juce::Slider
{
public:
    bool keyPressed (const juce::KeyPress& key) override;
};

class MidiEchoAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit MidiEchoAudioProcessorEditor (MidiEchoAudioProcessor&);
    ~MidiEchoAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;
    void focusGained (FocusChangeType) override;

private:
    void applyTypedDelay();
    void scheduleInitialFocus();
    void performInitialFocus();

    MidiEchoAudioProcessor& processor;

    juce::Label heading;
    DelaySlider delaySlider;
    juce::Label delayEntryLabel;
    juce::TextEditor delayEntry;
    juce::Label keysHelp;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayAttachment;

    bool initialFocusPending = false;
    bool initialFocusDone = false;
    int initialFocusAttempts = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiEchoAudioProcessorEditor)
};
