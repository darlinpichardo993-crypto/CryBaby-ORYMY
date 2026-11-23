#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace cbaw
{
namespace ids
{
    inline constexpr const char* mode        = "mode";
    inline constexpr const char* position    = "position";
    inline constexpr const char* sensitivity = "sensitivity";
    inline constexpr const char* depth       = "depth";
    inline constexpr const char* resonance   = "resonance";
    inline constexpr const char* drive       = "drive";
    inline constexpr const char* tone        = "tone";
    inline constexpr const char* speed       = "speed";
    inline constexpr const char* mix         = "mix";
    inline constexpr const char* output      = "output";
    inline constexpr const char* aventura    = "aventura";
    inline constexpr const char* bypass      = "bypass";
}

// Parameters stored inside presets (bypass is deliberately NOT part of a preset).
inline constexpr const char* kPresetParamIds[] = {
    ids::mode, ids::position, ids::sensitivity, ids::depth, ids::resonance,
    ids::drive, ids::tone, ids::speed, ids::mix, ids::output, ids::aventura
};

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using FloatParam  = juce::AudioParameterFloat;
    using Attributes  = juce::AudioParameterFloatAttributes;
    using Range       = juce::NormalisableRange<float>;

    auto percent = [] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; };
    auto dbText  = [] (float v, int) { return juce::String (v, 1) + " dB"; };

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ids::mode, 1 }, "Mode",
        juce::StringArray { "AUTO", "ENVELOPE", "CYCLIC", "TOUCH" }, 0));

    auto add01 = [&] (const char* id, const char* name, float def)
    {
        p.push_back (std::make_unique<FloatParam> (juce::ParameterID { id, 1 }, name,
                                                   Range (0.0f, 1.0f, 0.001f), def,
                                                   Attributes().withStringFromValueFunction (percent)));
    };

    add01 (ids::position,    "Wah Position", 0.30f);
    add01 (ids::sensitivity, "Sensitivity",  0.50f);
    add01 (ids::depth,       "Depth",        0.60f);
    add01 (ids::resonance,   "Resonance",    0.50f);
    add01 (ids::drive,       "Drive",        0.20f);
    add01 (ids::tone,        "Tone",         0.50f);
    add01 (ids::speed,       "Speed",        0.50f);
    add01 (ids::mix,         "Mix",          1.00f);

    p.push_back (std::make_unique<FloatParam> (juce::ParameterID { ids::output, 1 }, "Output",
                                               Range (-24.0f, 12.0f, 0.1f), 0.0f,
                                               Attributes().withStringFromValueFunction (dbText)));

    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ids::aventura, 1 }, "Aventura Mode", false));
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ids::bypass,   1 }, "Bypass", false));

    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (auto& param : p)
        layout.add (std::move (param));
    return layout;
}
} // namespace cbaw
