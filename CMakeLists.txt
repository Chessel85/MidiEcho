cmake_minimum_required(VERSION 3.22)

# On macOS, build one universal binary for Apple Silicon and Intel Macs.
# These must be set before project().
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

project(MidiEcho VERSION 0.1.0 LANGUAGES C CXX)

# Static MSVC runtime, as in LJuno-116, so no VC Redistributable is needed.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(FetchContent)

set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.15
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

# A plain effect with stereo audio pass-through plus MIDI in and out. REAPER
# hosts this more reliably than a VST3 "MIDI effect" with no audio buses.
juce_add_plugin(MidiEcho
    COMPANY_NAME "Chessel"
    BUNDLE_ID "org.chessel.midiecho"
    PLUGIN_MANUFACTURER_CODE Chsl
    PLUGIN_CODE Mech
    FORMATS VST3
    PRODUCT_NAME "MIDI Echo"
    DESCRIPTION "Passes MIDI through and repeats it after a delay"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES Fx Tools
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(MidiEcho PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

target_compile_features(MidiEcho PRIVATE cxx_std_20)

target_compile_definitions(MidiEcho
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        # Otherwise JUCE adds 2048 hidden "MIDI CC" parameters, which flood
        # OSARA's FX parameter list.
        JUCE_VST3_EMULATE_MIDI_CC_WITH_PARAMETERS=0
)

target_link_libraries(MidiEcho
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Command-line host that loads the built VST3 and prints the MIDI it outputs.
juce_add_console_app(MidiEchoHostTest PRODUCT_NAME "MidiEchoHostTest")
target_sources(MidiEchoHostTest PRIVATE Tests/HostTest.cpp)
target_compile_features(MidiEchoHostTest PRIVATE cxx_std_20)
target_compile_definitions(MidiEchoHostTest PRIVATE
    JUCE_PLUGINHOST_VST3=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(MidiEchoHostTest PRIVATE
    juce::juce_audio_processors juce::juce_recommended_config_flags)

# Drives the processor directly, bypassing the VST3 wrapper.
juce_add_console_app(MidiEchoProcessorTest PRODUCT_NAME "MidiEchoProcessorTest")
target_sources(MidiEchoProcessorTest PRIVATE Tests/ProcessorTest.cpp)
target_compile_features(MidiEchoProcessorTest PRIVATE cxx_std_20)
target_link_libraries(MidiEchoProcessorTest PRIVATE MidiEcho juce::juce_audio_utils)
