cmake_minimum_required(VERSION 3.22)

project(GreenOnions VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json for IDE tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Allow passing a local JUCE checkout via -DJUCE_DIR=/path/to/JUCE
if(DEFINED JUCE_DIR AND EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(STATUS "Using local JUCE from: ${JUCE_DIR}")
    add_subdirectory("${JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")
else()
    include(FetchContent)
    message(STATUS "Fetching JUCE 8.0.4 via FetchContent...")
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(GreenOnions
    COMPANY_NAME                "Signal Forge Audio Labs"
    COMPANY_WEBSITE             "https://signalforgeaudio.com"
    BUNDLE_ID                   "com.signalforgeaudio.greenonions"
    PLUGIN_MANUFACTURER_CODE    Sfal
    PLUGIN_CODE                 Gron
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    PLUGIN_NAME                 "Green Onions"
    PRODUCT_NAME                "Green Onions"
    FORMATS                     VST3 Standalone
)

target_sources(GreenOnions
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/Parameters.h
        Source/Presets.h
        Source/DSP/TonewheelTable.h
        Source/DSP/OrganVoicing.h
        Source/DSP/KeyMatrix.h
        Source/DSP/PercussionEngine.h
        Source/DSP/TonewheelEngine.h
        Source/DSP/ScannerVibrato.h
        Source/DSP/PreampSwell.h
        Source/DSP/SpringReverb.h
        Source/DSP/Leslie122.h
        Source/DSP/LeslieMicEQ.h
        Source/DSP/MarshallBluesbreaker.h
        Source/UI/VintageLookAndFeel.h
        Source/UI/DrawbarComponent.h
        Source/UI/DualDrawbarDeck.h
        Source/UI/TiltRockerSwitch.h
        Source/UI/PresetOctaveComponent.h
        Source/UI/RockerSwitch.h
        Source/UI/HalfMoonSwitch.h
        Source/UI/SwellPedalComponent.h
        Source/UI/LeslieVisualizerComponent.h
        Source/UI/StageMicLayerComponent.h
        Source/UI/RotorMeter.h
        Source/UI/OrganKeyboard.h
)

target_include_directories(GreenOnions
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Source"
)

target_compile_definitions(GreenOnions
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_ALSA=1
        JUCE_JACK=1
)

# Optional BinaryData PNG sprite pipeline from Resources/Stage/*.png with automatic procedural fallback
file(GLOB STAGE_PNG_ASSETS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Stage/*.png")

if(STAGE_PNG_ASSETS)
    message(STATUS "Embedding Stage PNG assets via BinaryData: ${STAGE_PNG_ASSETS}")
    juce_add_binary_data(GreenOnionsStageAssets SOURCES ${STAGE_PNG_ASSETS})
    target_link_libraries(GreenOnions PRIVATE GreenOnionsStageAssets)
    target_compile_definitions(GreenOnions PRIVATE SFA_HAS_STAGE_PNG_ASSETS=1)
else()
    target_compile_definitions(GreenOnions PRIVATE SFA_HAS_STAGE_PNG_ASSETS=0)
endif()

target_link_libraries(GreenOnions
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Standalone pure-C++20 DSP verification tests (no JUCE dependency required)
enable_testing()
add_executable(GreenOnionsDspTests Tests/DspTests.cpp)
target_include_directories(GreenOnionsDspTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
target_compile_features(GreenOnionsDspTests PRIVATE cxx_std_20)
add_test(NAME DspTests COMMAND GreenOnionsDspTests)
