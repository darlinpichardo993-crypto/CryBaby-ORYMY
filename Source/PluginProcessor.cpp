#include "PluginProcessor.h"
#include "PluginEditor.h"

CryBabyAudioProcessor::CryBabyAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "CRYBABYAUTOWAH", cbaw::createParameterLayout()),
      presetManager (apvts)
{
    pMode        = apvts.getRawParameterValue (cbaw::ids::mode);
    pPosition    = apvts.getRawParameterValue (cbaw::ids::position);
    pSensitivity = apvts.getRawParameterValue (cbaw::ids::sensitivity);
    pDepth       = apvts.getRawParameterValue (cbaw::ids::depth);
    pResonance   = apvts.getRawParameterValue (cbaw::ids::resonance);
    pDrive       = apvts.getRawParameterValue (cbaw::ids::drive);
    pTone        = apvts.getRawParameterValue (cbaw::ids::tone);
    pSpeed       = apvts.getRawParameterValue (cbaw::ids::speed);
    pMix         = apvts.getRawParameterValue (cbaw::ids::mix);
    pOutput      = apvts.getRawParameterValue (cbaw::ids::output);
    pAventura    = apvts.getRawParameterValue (cbaw::ids::aventura);
    pBypass      = apvts.getRawParameterValue (cbaw::ids::bypass);
}

bool CryBabyAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return in == out;
}

void CryBabyAudioProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate, juce::jmax (1, getTotalNumOutputChannels()));

    mixSmooth.reset (sampleRate, 0.02);    mixSmooth.setCurrentAndTargetValue (pMix->load());
    outSmooth.reset (sampleRate, 0.02);    outSmooth.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pOutput->load()));
    activeSmooth.reset (sampleRate, 0.015); activeSmooth.setCurrentAndTargetValue (pBypass->load() > 0.5f ? 0.0f : 1.0f);
}

void CryBabyAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh      = juce::jmin (buffer.getNumChannels(), cbaw::dsp::WahEngine::kMaxChannels);
    const int numSamples = buffer.getNumSamples();
    if (numCh == 0 || numSamples == 0) return;

    // ---- read parameters (lock-free) -----------------------------------------
    cbaw::dsp::EngineParams ep;
    ep.mode        = (cbaw::dsp::WahMode) juce::jlimit (0, 3, (int) std::lround (pMode->load()));
    ep.position    = pPosition->load();
    ep.sensitivity = pSensitivity->load();
    ep.depth       = pDepth->load();
    ep.resonance   = pResonance->load();
    ep.drive       = pDrive->load();
    ep.tone        = pTone->load();
    ep.speed       = pSpeed->load();
    ep.aventura    = pAventura->load() > 0.5f;
    engine.setParams (ep);

    mixSmooth.setTargetValue (pMix->load());
    outSmooth.setTargetValue (juce::Decibels::decibelsToGain (pOutput->load()));
    activeSmooth.setTargetValue (pBypass->load() > 0.5f ? 0.0f : 1.0f);

    float* ch[cbaw::dsp::WahEngine::kMaxChannels] = { nullptr, nullptr };
    for (int c = 0; c < numCh; ++c) ch[c] = buffer.getWritePointer (c);

    float inPk = 0.0f, outPk = 0.0f;

    // Fully bypassed and settled: true pass-through, no processing.
    const bool fullyBypassed = ! activeSmooth.isSmoothing() && activeSmooth.getTargetValue() == 0.0f;

    if (fullyBypassed)
    {
        for (int c = 0; c < numCh; ++c)
        {
            const float pk = buffer.getMagnitude (c, 0, numSamples);
            inPk = juce::jmax (inPk, pk);
        }
        outPk = inPk;
    }
    else
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float in[2] = { 0.0f, 0.0f }, wet[2] = { 0.0f, 0.0f };
            for (int c = 0; c < numCh; ++c) in[c] = ch[c][i];

            engine.processFrame (in, wet, numCh);

            const float m = mixSmooth.getNextValue();
            const float g = outSmooth.getNextValue();
            const float a = activeSmooth.getNextValue();   // 1 = effect on, 0 = bypass

            for (int c = 0; c < numCh; ++c)
            {
                const float processed = (in[c] * (1.0f - m) + wet[c] * m) * g;
                const float out = in[c] + a * (processed - in[c]);
                ch[c][i] = out;
                inPk  = juce::jmax (inPk,  std::fabs (in[c]));
                outPk = juce::jmax (outPk, std::fabs (out));
            }
        }
    }

    inputPeak.store (inPk);
    outputPeak.store (outPk);
    livePosition.store (engine.getPosition());
    liveFrequency.store (engine.getFrequency());
    liveQ.store (engine.getQ());
    livePeakGain.store (engine.getPeakGain());
}

juce::AudioProcessorEditor* CryBabyAudioProcessor::createEditor()
{
    return new CryBabyAudioProcessorEditor (*this);
}

void CryBabyAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void CryBabyAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

void CryBabyAudioProcessor::toggleAB()
{
    abSlot[activeSlot] = apvts.copyState();
    activeSlot ^= 1;

    if (abSlot[activeSlot].isValid())
        apvts.replaceState (abSlot[activeSlot].createCopy());
    else
        abSlot[activeSlot] = apvts.copyState();   // first time: B starts as a copy of A
}

// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CryBabyAudioProcessor();
}
