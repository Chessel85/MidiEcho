#include "PluginEditor.h"

namespace
{
const auto background = juce::Colour (0xff1e2a3a);
const auto foreground = juce::Colour (0xffe8eef5);
const auto accent = juce::Colour (0xff6fb3ff);

void announce (const juce::String& text)
{
    juce::AccessibilityHandler::postAnnouncement (
        text, juce::AccessibilityHandler::AnnouncementPriority::high);
}
}

bool DelaySlider::keyPressed (const juce::KeyPress& key)
{
    const auto code = key.getKeyCode();
    const auto mods = key.getModifiers();

    double step = 0.0;
    if (code == juce::KeyPress::upKey || code == juce::KeyPress::rightKey)
        step = mods.isShiftDown() ? 10.0 : 1.0;
    else if (code == juce::KeyPress::downKey || code == juce::KeyPress::leftKey)
        step = mods.isShiftDown() ? -10.0 : -1.0;
    else if (code == juce::KeyPress::pageUpKey)
        step = mods.isCtrlDown() ? 1000.0 : 100.0;
    else if (code == juce::KeyPress::pageDownKey)
        step = mods.isCtrlDown() ? -1000.0 : -100.0;

    if (step != 0.0)
    {
        setValue (getValue() + step, juce::sendNotificationSync);
        return true;
    }

    if (code == juce::KeyPress::homeKey)
    {
        setValue (getMinimum(), juce::sendNotificationSync);
        return true;
    }

    if (code == juce::KeyPress::endKey)
    {
        setValue (getMaximum(), juce::sendNotificationSync);
        return true;
    }

    return juce::Slider::keyPressed (key);
}

MidiEchoAudioProcessorEditor::MidiEchoAudioProcessorEditor (MidiEchoAudioProcessor& p)
    : AudioProcessorEditor (p), processor (p)
{
    setTitle ("MIDI Echo");
    setFocusContainerType (FocusContainerType::keyboardFocusContainer);

    heading.setText ("MIDI Echo", juce::dontSendNotification);
    heading.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    heading.setColour (juce::Label::textColourId, foreground);
    heading.setAccessible (false); // REAPER already speaks the window title.
    addAndMakeVisible (heading);

    delaySlider.setSliderStyle (juce::Slider::LinearHorizontal);
    delaySlider.setTextBoxStyle (juce::Slider::TextBoxRight, true, 90, 24);
    delaySlider.setColour (juce::Slider::thumbColourId, accent);
    delaySlider.setColour (juce::Slider::trackColourId, accent.withAlpha (0.5f));
    delaySlider.setColour (juce::Slider::textBoxTextColourId, foreground);
    delaySlider.setTitle ("Delay");
    delaySlider.setWantsKeyboardFocus (true);
    addAndMakeVisible (delaySlider);

    // The attachment copies the parameter's range and its "ms" value text, so
    // NVDA reads the slider as "Delay, slider, 50 ms".
    delayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.parameters, "delay", delaySlider);
    delaySlider.setNumDecimalPlacesToDisplay (0);
    delaySlider.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v)) + " ms"; };
    delaySlider.updateText();

    delayEntryLabel.setText ("Type delay in ms, then Enter:", juce::dontSendNotification);
    delayEntryLabel.setColour (juce::Label::textColourId, foreground);
    delayEntryLabel.setAccessible (false); // The edit box carries the same title.
    addAndMakeVisible (delayEntryLabel);

    delayEntry.setTitle ("Type delay in milliseconds, then Enter");
    delayEntry.setInputRestrictions (5, "0123456789");
    delayEntry.setSelectAllWhenFocused (true);
    delayEntry.setColour (juce::TextEditor::backgroundColourId, background.brighter (0.2f));
    delayEntry.setColour (juce::TextEditor::textColourId, foreground);
    delayEntry.setColour (juce::TextEditor::outlineColourId, accent);
    delayEntry.setText (juce::String (juce::roundToInt (delaySlider.getValue())), false);
    delayEntry.onReturnKey = [this] { applyTypedDelay(); };
    delayEntry.onEscapeKey = [this]
    {
        delayEntry.setText (juce::String (juce::roundToInt (delaySlider.getValue())), false);
        delaySlider.grabKeyboardFocus();
    };
    addAndMakeVisible (delayEntry);

    delaySlider.onValueChange = [this]
    {
        if (! delayEntry.hasKeyboardFocus (true))
            delayEntry.setText (juce::String (juce::roundToInt (delaySlider.getValue())), false);
    };

    keysHelp.setText ("Arrows 1 ms, Shift+arrows 10, Page Up/Down 100, "
                      "Ctrl+Page Up/Down 1000, Home/End min/max. 0 turns the repeat off.",
                      juce::dontSendNotification);
    keysHelp.setColour (juce::Label::textColourId, foreground.withAlpha (0.8f));
    keysHelp.setFont (juce::FontOptions (13.0f));
    keysHelp.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (keysHelp);

    setWantsKeyboardFocus (true);
    setSize (460, 200);
}

MidiEchoAudioProcessorEditor::~MidiEchoAudioProcessorEditor() = default;

void MidiEchoAudioProcessorEditor::applyTypedDelay()
{
    const auto text = delayEntry.getText().trim();
    const auto value = text.getIntValue();

    if (text.isEmpty() || value > MidiEchoAudioProcessor::maxDelayMs)
    {
        announce ("Enter a number from 0 to "
                  + juce::String (MidiEchoAudioProcessor::maxDelayMs));
        delayEntry.selectAll();
        return;
    }

    if (auto* param = processor.parameters.getParameter ("delay"))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 ((float) value));
        param->endChangeGesture();
    }

    delayEntry.setText (juce::String (value), false);
    delayEntry.selectAll();
    announce ("Delay " + juce::String (value) + " ms");
}

void MidiEchoAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (background);
}

void MidiEchoAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (16);
    heading.setBounds (area.removeFromTop (30));
    area.removeFromTop (10);
    delaySlider.setBounds (area.removeFromTop (32));
    area.removeFromTop (12);

    auto entryRow = area.removeFromTop (28);
    delayEntryLabel.setBounds (entryRow.removeFromLeft (220));
    delayEntry.setBounds (entryRow.removeFromLeft (90));

    area.removeFromTop (12);
    keysHelp.setBounds (area);
}

void MidiEchoAudioProcessorEditor::focusGained (FocusChangeType)
{
    scheduleInitialFocus();
}

void MidiEchoAudioProcessorEditor::visibilityChanged()
{
    if (isVisible())
        scheduleInitialFocus();
}

// Same approach as LJuno-116: wait until the host window has focus and the
// native UIA tree exists, then move focus to the slider so NVDA announces it.
void MidiEchoAudioProcessorEditor::scheduleInitialFocus()
{
    if (initialFocusPending || initialFocusDone)
        return;

    initialFocusPending = true;
    juce::Timer::callAfterDelay (150, [safeThis = juce::Component::SafePointer (this)]
    {
        if (safeThis == nullptr)
            return;
        safeThis->initialFocusPending = false;
        safeThis->performInitialFocus();
    });
}

void MidiEchoAudioProcessorEditor::performInitialFocus()
{
    if (! isShowing())
        return;

    auto* focused = juce::Component::getCurrentlyFocusedComponent();
    if (focused != nullptr && focused != this && isParentOf (focused))
    {
        initialFocusDone = true; // The user has already moved to a control.
        return;
    }

    auto* peer = getPeer();
    auto* handler = delaySlider.getAccessibilityHandler();
    const auto nativeReady = handler != nullptr && handler->getNativeImplementation() != nullptr;
    if (peer == nullptr || ! peer->isFocused() || ! nativeReady)
    {
        if (++initialFocusAttempts < 8)
            scheduleInitialFocus();
        return;
    }

    delaySlider.grabKeyboardFocus();
    if (delaySlider.hasKeyboardFocus (false))
    {
        handler->grabFocus();
        initialFocusDone = true;
    }
    else if (++initialFocusAttempts < 8)
    {
        scheduleInitialFocus();
    }
}
