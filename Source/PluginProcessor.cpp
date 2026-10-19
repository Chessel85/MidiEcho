#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr std::size_t pendingCapacity = 8192;
}

MidiEchoAudioProcessor::MidiEchoAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "MidiEcho", createParameterLayout())
{
    delayMs = parameters.getRawParameterValue ("delay");
    pending.reserve (pendingCapacity);
    output.ensureSize (pendingCapacity * 4);
}

juce::AudioProcessorValueTreeState::ParameterLayout
MidiEchoAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "delay", 1 }, "Delay", 0, maxDelayMs, 50,
        juce::AudioParameterIntAttributes().withLabel ("ms")));
    return layout;
}

void MidiEchoAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    clearPending();
}

void MidiEchoAudioProcessor::releaseResources() {}

void MidiEchoAudioProcessor::reset()
{
    clearPending();
}

void MidiEchoAudioProcessor::clearPending()
{
    pending.clear();
    samplePosition = 0;
    for (auto& channel : echoedNotesOn)
        channel.fill (0);
}

bool MidiEchoAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

bool MidiEchoAudioProcessor::shouldEcho (const juce::MidiMessage& message)
{
    // Repeat channel messages (notes, controllers, pitch bend and so on), but
    // never SysEx or system messages such as MIDI clock and start/stop.
    return message.getChannel() > 0;
}

void MidiEchoAudioProcessor::trackEchoedNote (const juce::MidiMessage& message)
{
    auto& count = echoedNotesOn[(size_t) (message.getChannel() - 1)]
                               [(size_t) message.getNoteNumber()];
    if (message.isNoteOn())
        ++count;
    else if (message.isNoteOff() && count > 0)
        --count;
}

void MidiEchoAudioProcessor::processBlock (juce::AudioBuffer<float>& audio,
                                           juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    // Audio passes straight through; only clear outputs that have no input.
    for (auto ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        audio.clear (ch, 0, audio.getNumSamples());

    const auto numSamples = audio.getNumSamples();
    const auto blockEnd = samplePosition + numSamples;
    const auto delaySamples = (juce::int64) std::llround (
        delayMs->load() * 0.001 * currentSampleRate);

    output.clear();

    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();
        output.addEvent (message, metadata.samplePosition);

        // A delay of 0 would put the repeat on top of the original, which
        // doubles notes for no audible gain, so 0 means "repeat off".
        if (delaySamples > 0 && shouldEcho (message) && pending.size() < pending.capacity())
            pending.push_back ({ samplePosition + metadata.samplePosition + delaySamples, message });
    }

    // Emit every repeat that falls due inside this block. The whole queue is
    // scanned because a shorter delay can make newer repeats due before older.
    std::size_t kept = 0;
    for (std::size_t i = 0; i < pending.size(); ++i)
    {
        auto& event = pending[i];
        if (event.time < blockEnd)
        {
            const auto offset = (int) juce::jmax ((juce::int64) 0, event.time - samplePosition);
            output.addEvent (event.message, offset);
            if (event.message.isNoteOnOrOff())
                trackEchoedNote (event.message);
        }
        else
        {
            // Never move an event onto itself: MidiMessage's move assignment
            // empties the source, so a self-move would erase the message.
            if (kept != i)
                pending[kept] = std::move (event);
            ++kept;
        }
    }
    pending.resize (kept);

    samplePosition = blockEnd;
    midi.swapWith (output);
}

void MidiEchoAudioProcessor::processBlockBypassed (juce::AudioBuffer<float>& audio,
                                                   juce::MidiBuffer& midi)
{
    for (auto ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        audio.clear (ch, 0, audio.getNumSamples());

    // Drop queued repeats, but release any repeated notes still sounding so
    // that bypassing never leaves a stuck note on the instrument downstream.
    for (int channel = 0; channel < 16; ++channel)
        for (int note = 0; note < 128; ++note)
            if (echoedNotesOn[(size_t) channel][(size_t) note] > 0)
                midi.addEvent (juce::MidiMessage::noteOff (channel + 1, note), 0);

    clearPending();
}

juce::AudioProcessorEditor* MidiEchoAudioProcessor::createEditor()
{
    return new MidiEchoAudioProcessorEditor (*this);
}

void MidiEchoAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void MidiEchoAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidiEchoAudioProcessor();
}
