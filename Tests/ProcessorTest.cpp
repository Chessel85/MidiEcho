// Drives MidiEchoAudioProcessor directly, without any plug-in wrapper.
// Exits with 1 if the default 50 ms repeat is missing or late.
#include "../Source/PluginProcessor.h"
#include <iostream>
#include <vector>

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    MidiEchoAudioProcessor processor;
    const double sampleRate = 48000.0;
    const int blockSize = 512;
    processor.setPlayConfigDetails (2, 2, sampleRate, blockSize);
    processor.prepareToPlay (sampleRate, blockSize);

    juce::AudioBuffer<float> audio (2, blockSize);
    std::vector<int> noteOnSamples, noteOffSamples;

    for (int block = 0; block < 100; ++block)
    {
        juce::MidiBuffer midi;
        if (block == 0)
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 10);
        if (block == 10)
            midi.addEvent (juce::MidiMessage::noteOff (1, 60), 10);
        processor.processBlock (audio, midi);
        for (const auto m : midi)
        {
            const auto sample = block * blockSize + m.samplePosition;
            std::cout << sample * 1000.0 / sampleRate << " ms  "
                      << m.getMessage().getDescription() << "\n";
            (m.getMessage().isNoteOn() ? noteOnSamples : noteOffSamples).push_back (sample);
        }
    }

    // 50 ms at 48 kHz is 2400 samples.
    const std::vector<int> expectedOn { 10, 2410 }, expectedOff { 5130, 7530 };
    if (noteOnSamples != expectedOn || noteOffSamples != expectedOff)
    {
        std::cout << "FAIL: repeat missing or not exactly 50 ms late\n";
        return 1;
    }

    std::cout << "PASS\n";
    return 0;
}
