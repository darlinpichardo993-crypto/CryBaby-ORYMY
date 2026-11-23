#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <memory>
#include <vector>

#include "PluginProcessor.h"
#include "UI/CryBabyLookAndFeel.h"
#include "UI/FilterDisplay.h"
#include "UI/LevelMeter.h"
#include "UI/PedalComponent.h"

class CryBabyAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit CryBabyAudioProcessorEditor (CryBabyAudioProcessor&);
    ~CryBabyAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Label  label;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    static constexpr float kBaseW = 960.0f, kBaseH = 540.0f;

    void timerCallback() override;
    void initKnob (Knob& k, const char* paramId, const char* text);
    void updateModeButtons (int mode);
    void refreshPresetList();
    void selectPresetId (int id);
    void stepPreset (int delta);
    void showSaveDialog();

    CryBabyAudioProcessor& audioProcessor;
    cbaw::ui::CryBabyLookAndFeel lnf;      // must outlive every child component

    cbaw::ui::PedalComponent  pedal;
    cbaw::ui::FilterDisplay   filterDisplay;
    cbaw::ui::LevelMeter      inMeter, outMeter;

    std::array<Knob, 8> knobs;             // sens, depth, res, drive | tone, speed, mix, output

    std::array<juce::TextButton, 4> modeButtons;
    juce::TextButton aventuraButton { "AVENTURA MODE" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> aventuraAttachment;
    juce::ParameterAttachment modeAttachment;

    juce::ComboBox   presetBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" }, saveButton { "SAVE" }, abButton { "A" };
    std::vector<int> presetIds;
    juce::StringArray userNames;

    juce::Image backgroundTexture;
    float inLevelDb = -60.0f, outLevelDb = -60.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CryBabyAudioProcessorEditor)
};
