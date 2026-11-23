cmake_minimum_required(VERSION 3.22)

project(CryBabyAutoWah VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Static MSVC runtime: the plugin runs on any Windows PC without the VC++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(CBAW_BUILD_STANDALONE "Also build the Standalone application (handy for testing without a DAW)" ON)
option(CBAW_BUILD_TESTS      "Build the JUCE-free DSP self test (Tests/DspSelfTest.cpp)" ON)
set(CBAW_JUCE_TAG "8.0.4" CACHE STRING "JUCE git tag downloaded when External/JUCE is not present")

# --------------------------------------------------------------------------
# JUCE: use the git submodule in External/JUCE if it exists, otherwise
# download it automatically with FetchContent (needs internet + git).
# --------------------------------------------------------------------------
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/External/JUCE/CMakeLists.txt")
    message(STATUS "CryBabyAutoWah: using JUCE submodule in External/JUCE")
    add_subdirectory(External/JUCE)
else()
    message(STATUS "CryBabyAutoWah: downloading JUCE ${CBAW_JUCE_TAG} with FetchContent")
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        ${CBAW_JUCE_TAG}
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# --------------------------------------------------------------------------
# Plugin
# --------------------------------------------------------------------------
set(CBAW_FORMATS VST3)
if(CBAW_BUILD_STANDALONE)
    list(APPEND CBAW_FORMATS Standalone)
endif()
if(APPLE)
    list(APPEND CBAW_FORMATS AU)
endif()

juce_add_plugin(CryBabyAutoWah
    COMPANY_NAME                "ORYMY"
    COMPANY_WEBSITE             "https://github.com/"
    BUNDLE_ID                   com.orymy.crybabyautowah
    PLUGIN_MANUFACTURER_CODE    Orym
    PLUGIN_CODE                 Cbaw
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    FORMATS                     ${CBAW_FORMATS}
    VST3_CATEGORIES             Fx Filter
    AU_MAIN_TYPE                kAudioUnitType_Effect
    PRODUCT_NAME                "CRY BABY AUTO WAH")

target_sources(CryBabyAutoWah PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/Parameters.h
    Source/DSP/DspCommon.h
    Source/DSP/EnvelopeFollower.h
    Source/DSP/AttackDetector.h
    Source/DSP/WahFilter.h
    Source/DSP/AutoWahController.h
    Source/DSP/WahEngine.h
    Source/DSP/WahEngine.cpp
    Source/Presets/FactoryPresets.h
    Source/Presets/PresetManager.h
    Source/Presets/PresetManager.cpp
    Source/UI/CryBabyLookAndFeel.h
    Source/UI/FilterDisplay.h
    Source/UI/LevelMeter.h
    Source/UI/PedalComponent.h)

target_include_directories(CryBabyAutoWah PRIVATE Source)

target_compile_definitions(CryBabyAutoWah PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(CryBabyAutoWah
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(MSVC)
    target_compile_options(CryBabyAutoWah PRIVATE /utf-8)
endif()

# --------------------------------------------------------------------------
# DSP self test (pure C++, no JUCE): run with  ctest --test-dir build -C Release
# --------------------------------------------------------------------------
if(CBAW_BUILD_TESTS)
    enable_testing()
    add_executable(DspSelfTest Tests/DspSelfTest.cpp Source/DSP/WahEngine.cpp)
    target_compile_features(DspSelfTest PRIVATE cxx_std_17)
    if(MSVC)
        target_compile_options(DspSelfTest PRIVATE /utf-8 /fp:precise)
    endif()
    add_test(NAME DspSelfTest COMMAND DspSelfTest)
endif()
