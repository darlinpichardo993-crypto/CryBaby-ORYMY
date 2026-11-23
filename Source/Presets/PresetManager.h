#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace cbaw
{
// Factory presets (compiled in) + user presets (.cbpreset XML files in the
// user application-data folder). Message-thread only.
class PresetManager
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState& apvtsToUse);

    int               getNumFactoryPresets() const;
    juce::String      getFactoryPresetName (int index) const;
    void              loadFactoryPreset (int index);

    juce::StringArray getUserPresetNames() const;
    bool              saveUserPreset (const juce::String& name);
    bool              loadUserPreset (const juce::String& name);
    bool              deleteUserPreset (const juce::String& name);

    juce::String      getCurrentPresetName() const;
    static juce::File getUserPresetDirectory();

private:
    void setParameterValue (const juce::String& id, float realValue);
    void setCurrentPresetName (const juce::String& name);

    juce::AudioProcessorValueTreeState& apvts;
};
} // namespace cbaw
