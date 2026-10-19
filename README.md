# MIDI Echo

A minimal JUCE VST3 that passes MIDI straight through and plays a second copy of it after a set delay. It has one parameter, **Delay**, from 0 to 10,000 ms (default 50 ms). It is built the same way as LJuno-116, with the same JUCE version and CMake setup, so NVDA can read its controls.

## Using it in REAPER

1. Put **MIDI Echo** on a track *before* your instrument, in the same FX chain.
2. Play or record MIDI. Each note, controller and pitch bend is passed through, then repeated after the delay. SysEx and MIDI clock are passed through but not repeated.
3. Audio passes through unchanged.

A delay of **0 turns the repeat off**, because a repeat at 0 ms lands on top of the original note and only doubles it.

If you bypass the plug-in while a repeated note is still sounding, it sends a note-off, so no note is left stuck.

## Keyboard and NVDA

When the window opens, focus moves to the Delay slider and NVDA says something like "Delay, slider, 50 ms".

| Keys | Change |
|---|---|
| Up / Right arrow | +1 ms |
| Down / Left arrow | −1 ms |
| Shift + arrows | ±10 ms |
| Page Up / Page Down | ±100 ms |
| Ctrl + Page Up / Page Down | ±1000 ms |
| Home / End | 0 / 10,000 ms |

To set an exact value, press **Tab** to reach the edit field "Type delay in milliseconds, then Enter". Type the number and press **Enter**; NVDA confirms "Delay 250 ms". Press **Escape** to cancel and go back to the slider.

**Delay** is also a normal VST3 parameter, so OSARA's FX parameters dialog and REAPER automation envelopes work as well.

## Building on Windows

You need Visual Studio 2022 with the "Desktop development with C++" workload, which includes CMake. Run this from this folder in a Developer PowerShell (or call VS's bundled `cmake.exe` by its full path):

```
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target MidiEcho_VST3
```

The first configure downloads JUCE 8.0.15. Two test programs are built with `cmake --build build --config Release`:

- `MidiEchoProcessorTest.exe` drives the processor directly.
- `MidiEchoHostTest.exe "<path to MIDI Echo.vst3>" 1000` loads the built VST3 like a host does, plays one note, and prints each MIDI event that comes out, with its time in ms.

 The output is the folder `build\MidiEcho_artefacts\Release\VST3\MIDI Echo.vst3`. Copy that whole folder into `C:\Program Files\Common Files\VST3`, then rescan plug-ins in REAPER.
