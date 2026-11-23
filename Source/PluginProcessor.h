#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>

#include "Parameters.h"
#include "DSP/WahEngine.h"
#include "Presets/PresetManager.h"

class CryBabyAudioProcessor : public juce::AudioProcessor
{
public:
    CryBabyAudioProcessor();
    ~CryBabyAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    cbaw::PresetManager& getPresetManager() { return presetManager; }

    // A/B comparison (message thread).
    void toggleAB();
    int  getActiveSlot() const { return activeSlot; }

    // Read-only data for the GUI (written on the audio thread).
    std::atomic<float> inputPeak { 0.0f }, outputPeak { 0.0f };
    std::atomic<float> livePosition { 0.0f }, liveFrequency { 350.0f }, liveQ { 5.0f }, livePeakGain { 2.0f };

private:
    juce::AudioProcessorValueTreeState apvts;
    cbaw::PresetManager presetManager;
    cbaw::dsp::WahEngine engine;

    std::atomic<float>* pMode = nullptr;
    std::atomic<float>* pPosition = nullptr;
    std::atomic<float>* pSensitivity = nullptr;
    std::atomic<float>* pDepth = nullptr;
    std::atomic<float>* pResonance = nullptr;
    std::atomic<float>* pDrive = nullptr;
    std::atomic<float>* pTone = nullptr;
    std::atomic<float>* pSpeed = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pOutput = nullptr;
    std::atomic<float>* pAventura = nullptr;
    std::atomic<float>* pBypass = nullptr;

    juce::SmoothedValue<float> mixSmooth, outSmooth, activeSmooth;

    juce::ValueTree abSlot[2];
    int activeSlot = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CryBabyAudioProcessor)
};
