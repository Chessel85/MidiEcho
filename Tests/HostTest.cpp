// Loads the built VST3 the way a host does, plays one short note and prints
// every MIDI event the plug-in sends back, with its time in milliseconds.
// Exits with 1 unless the original note and its repeat both come out on time,
// so it can be used as an automated test.
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_events/juce_events.h>
#include <iostream>
#include <vector>

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;

    if (argc < 2)
    {
        std::cout << "usage: MidiEchoHostTest <path to .vst3> [delay ms]\n";
        return 2;
    }

    const juce::String path (argv[1]);
    const auto delayMs = argc > 2 ? juce::String (argv[2]).getIntValue() : 250;

    juce::VST3PluginFormat format;
    juce::OwnedArray<juce::PluginDescription> found;
    format.findAllTypesForFile (found, path);
    if (found.isEmpty())
    {
        std::cout << "FAIL: no plug-in found in " << path << "\n";
        return 1;
    }

    const double sampleRate = 48000.0;
    const int blockSize = 512;

    juce::String error;
    auto plugin = format.createInstanceFromDescription (*found[0], sampleRate, blockSize, error);
    if (plugin == nullptr)
    {
        std::cout << "FAIL: load failed: " << error << "\n";
        return 1;
    }

    std::cout << "Loaded " << plugin->getName() << ", producesMidi="
              << plugin->producesMidi() << "\n";

    // Expect only Delay and the host-facing Bypass, not JUCE's hidden MIDI CC
    // parameters, which would flood OSARA's FX parameter list.
    const auto& params = plugin->getParameters();
    std::cout << "Parameters: " << params.size() << "\n";
    if (params.size() > 2)
    {
        std::cout << "FAIL: expected at most 2 parameters\n";
        return 1;
    }

    for (auto* p : params)
        if (p->getName (32) == "Delay")
        {
            p->setValueNotifyingHost (p->getValueForText (juce::String (delayMs)));
            std::cout << "Delay set to " << p->getCurrentValueAsText() << "\n";
        }

    plugin->setPlayConfigDetails (2, 2, sampleRate, blockSize);
    plugin->prepareToPlay (sampleRate, blockSize);

    juce::AudioBuffer<float> audio (2, blockSize);
    const auto totalBlocks = (int) ((delayMs + 1000) * 0.001 * sampleRate / blockSize);
    std::vector<std::pair<double, juce::MidiMessage>> received;

    for (int block = 0; block < totalBlocks; ++block)
    {
        audio.clear();
        juce::MidiBuffer midi;
        if (block == 0)
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 10);
        if (block == 10) // note-off after about 107 ms
            midi.addEvent (juce::MidiMessage::noteOff (1, 60), 10);

        plugin->processBlock (audio, midi);

        for (const auto m : midi)
        {
            const auto ms = (block * blockSize + m.samplePosition) * 1000.0 / sampleRate;
            received.emplace_back (ms, m.getMessage());
            std::cout << juce::String (ms, 1) << " ms  " << m.getMessage().getDescription() << "\n";
        }
    }

    plugin->releaseResources();

    const auto noteOnAt = [&] (double expectedMs)
    {
        for (const auto& [ms, message] : received)
            if (message.isNoteOn() && std::abs (ms - expectedMs) < 1.0)
                return true;
        return false;
    };

    const auto expectedEvents = delayMs > 0 ? 4u : 2u;
    if (received.size() != expectedEvents || ! noteOnAt (0.2)
        || (delayMs > 0 && ! noteOnAt (0.2 + delayMs)))
    {
        std::cout << "FAIL: expected the note and its repeat " << delayMs << " ms later\n";
        return 1;
    }

    std::cout << "PASS\n";
    return 0;
}
