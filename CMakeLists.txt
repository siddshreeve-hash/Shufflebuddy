cmake_minimum_required(VERSION 3.22)

# Static MSVC runtime: the .exe runs on any Windows PC without the VC++ redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(ShuffleBuddy VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.9 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_binary_data(SBFonts SOURCES fonts/Fredoka-Bold.ttf fonts/Fredoka-Medium.ttf fonts/VT323-Regular.ttf)

juce_add_plugin(ShuffleBuddy
    COMPANY_NAME "Simon"
    PLUGIN_MANUFACTURER_CODE Smon
    PLUGIN_CODE Shbd
    FORMATS VST3 Standalone
    PRODUCT_NAME "Shuffle Buddy"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(ShuffleBuddy PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(ShuffleBuddy PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(ShuffleBuddy
    PRIVATE SBFonts juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

option(SB_BUILD_TESTS "Build offline test tools" OFF)
if(SB_BUILD_TESTS)
    foreach(t Render UiSnapshot Perf)
        juce_add_console_app(${t} PRODUCT_NAME "${t}")
        target_sources(${t} PRIVATE tests/${t}.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
        target_compile_definitions(${t} PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
        target_link_libraries(${t} PRIVATE SBFonts juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats juce::juce_recommended_config_flags)
    endforeach()
endif()
