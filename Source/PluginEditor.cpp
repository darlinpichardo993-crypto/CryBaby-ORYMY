#include "PluginEditor.h"

using namespace cbaw;

namespace
{
    constexpr int kUserPresetIdBase = 1000;
    const char* kModeNames[4] = { "AUTO", "ENVELOPE", "CYCLIC", "TOUCH" };
}

CryBabyAudioProcessorEditor::CryBabyAudioProcessorEditor (CryBabyAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      audioProcessor (p),
      pedal (p.getAPVTS()),
      modeAttachment (*p.getAPVTS().getParameter (ids::mode),
                      [this] (float v) { updateModeButtons ((int) std::lround (v)); }),
      backgroundTexture (ui::makeNoiseTexture (256, 77123u))
{
    setLookAndFeel (&lnf);

    // ---- pedal / displays ------------------------------------------------------
    addAndMakeVisible (pedal);
    addAndMakeVisible (filterDisplay);
    addAndMakeVisible (inMeter);
    addAndMakeVisible (outMeter);

    // ---- knobs ----------------------------------------------------------------------
    initKnob (knobs[0], ids::sensitivity, "SENSITIVITY");
    initKnob (knobs[1], ids::depth,       "DEPTH");
    initKnob (knobs[2], ids::resonance,   "RESONANCE");
    initKnob (knobs[3], ids::drive,       "DRIVE");
    initKnob (knobs[4], ids::tone,        "TONE");
    initKnob (knobs[5], ids::speed,       "SPEED");
    initKnob (knobs[6], ids::mix,         "MIX");
    initKnob (knobs[7], ids::output,      "OUTPUT");

    // ---- mode selector -------------------------------------------------------------------
    for (int i = 0; i < 4; ++i)
    {
        auto& b = modeButtons[(size_t) i];
        b.setButtonText (kModeNames[i]);
        b.getProperties().set ("led", true);
        b.getProperties().set ("ledColour", (int) 0xff56e07a);
        b.onClick = [this, i] { modeAttachment.setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (b);
    }
    modeAttachment.sendInitialUpdate();

    aventuraButton.setClickingTogglesState (true);
    aventuraButton.getProperties().set ("led", true);
    aventuraButton.getProperties().set ("ledColour", (int) 0xffff8a1f);
    aventuraAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.getAPVTS(), ids::aventura, aventuraButton);
    addAndMakeVisible (aventuraButton);

    // ---- presets ---------------------------------------------------------------------------------
    addAndMakeVisible (presetBox);
    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);
    addAndMakeVisible (saveButton);
    addAndMakeVisible (abButton);

    presetBox.setJustificationType (juce::Justification::centred);
    presetBox.onChange = [this] { selectPresetId (presetBox.getSelectedId()); };
    prevButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (+1); };
    saveButton.onClick = [this] { showSaveDialog(); };
    abButton.onClick = [this]
    {
        audioProcessor.toggleAB();
        abButton.setButtonText (audioProcessor.getActiveSlot() == 0 ? "A" : "B");
        presetBox.setText (audioProcessor.getPresetManager().getCurrentPresetName(), juce::dontSendNotification);
    };
    abButton.setButtonText (audioProcessor.getActiveSlot() == 0 ? "A" : "B");
    refreshPresetList();

    // ---- window ---------------------------------------------------------------------------------------
    setResizable (true, true);
    setResizeLimits (640, 360, 1920, 1080);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) kBaseW / (double) kBaseH);
    setSize (960, 540);

    startTimerHz (30);
}

CryBabyAudioProcessorEditor::~CryBabyAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void CryBabyAudioProcessorEditor::initKnob (Knob& k, const char* paramId, const char* text)
{
    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (k.label);

    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 84, 18);
    k.slider.setScrollWheelEnabled (true);
    addAndMakeVisible (k.slider);

    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.getAPVTS(), paramId, k.slider);
}

void CryBabyAudioProcessorEditor::updateModeButtons (int mode)
{
    for (int i = 0; i < 4; ++i)
        modeButtons[(size_t) i].setToggleState (i == mode, juce::dontSendNotification);
}

// ---------------------------------------------------------------------------------------------
void CryBabyAudioProcessorEditor::refreshPresetList()
{
    auto& pm = audioProcessor.getPresetManager();
    presetBox.clear (juce::dontSendNotification);
    presetIds.clear();

    presetBox.addSectionHeading ("FACTORY");
    for (int i = 0; i < pm.getNumFactoryPresets(); ++i)
    {
        presetBox.addItem (pm.getFactoryPresetName (i), i + 1);
        presetIds.push_back (i + 1);
    }

    userNames = pm.getUserPresetNames();
    if (userNames.size() > 0)
    {
        presetBox.addSectionHeading ("USER");
        for (int i = 0; i < userNames.size(); ++i)
        {
            presetBox.addItem (userNames[i], kUserPresetIdBase + i);
            presetIds.push_back (kUserPresetIdBase + i);
        }
    }

    presetBox.setText (pm.getCurrentPresetName(), juce::dontSendNotification);
}

void CryBabyAudioProcessorEditor::selectPresetId (int id)
{
    auto& pm = audioProcessor.getPresetManager();
    if (id >= kUserPresetIdBase)
    {
        const int idx = id - kUserPresetIdBase;
        if (idx >= 0 && idx < userNames.size()) pm.loadUserPreset (userNames[idx]);
    }
    else if (id >= 1)
        pm.loadFactoryPreset (id - 1);
}

void CryBabyAudioProcessorEditor::stepPreset (int delta)
{
    if (presetIds.empty()) return;
    int current = -1;
    for (size_t i = 0; i < presetIds.size(); ++i)
        if (presetIds[i] == presetBox.getSelectedId()) current = (int) i;

    const int n = (int) presetIds.size();
    const int next = current < 0 ? (delta > 0 ? 0 : n - 1) : (current + delta + n) % n;
    presetBox.setSelectedId (presetIds[(size_t) next], juce::sendNotificationSync);
}

void CryBabyAudioProcessorEditor::showSaveDialog()
{
    auto* w = new juce::AlertWindow ("Save preset", "Name for your custom preset:", juce::MessageBoxIconType::NoIcon);
    w->addTextEditor ("name", audioProcessor.getPresetManager().getCurrentPresetName());
    w->addButton ("Save",   1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<CryBabyAudioProcessorEditor> safe (this);
    w->enterModalState (true,
                        juce::ModalCallbackFunction::create ([safe, w] (int result)
                        {
                            if (result != 1 || safe == nullptr) return;
                            const auto name = w->getTextEditorContents ("name");
                            if (safe->audioProcessor.getPresetManager().saveUserPreset (name))
                                safe->refreshPresetList();
                        }),
                        true);
}

// ---------------------------------------------------------------------------------------------
void CryBabyAudioProcessorEditor::timerCallback()
{
    auto toDb = [] (float g) { return g > 0.000001f ? juce::Decibels::gainToDecibels (g) : -60.0f; };

    const float in  = audioProcessor.inputPeak.load();
    const float out = audioProcessor.outputPeak.load();
    inLevelDb  = juce::jmax (toDb (in),  inLevelDb  - 3.0f);   // fast attack, smooth fall
    outLevelDb = juce::jmax (toDb (out), outLevelDb - 3.0f);
    inMeter.setLevelDb (inLevelDb);
    outMeter.setLevelDb (outLevelDb);

    pedal.setLivePosition (audioProcessor.livePosition.load());
    filterDisplay.setState (audioProcessor.liveFrequency.load(), audioProcessor.liveQ.load(), audioProcessor.livePeakGain.load());

    const auto current = audioProcessor.getPresetManager().getCurrentPresetName();
    if (presetBox.getText() != current)
        presetBox.setText (current, juce::dontSendNotification);
}

// ---------------------------------------------------------------------------------------------
void CryBabyAudioProcessorEditor::paint (juce::Graphics& g)
{
    const float s = (float) getWidth() / kBaseW;
    g.addTransform (juce::AffineTransform::scale (s));

    // brushed black-steel faceplate
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff202226), 0.0f, 0.0f, juce::Colour (0xff09090a), 0.0f, kBaseH, false));
    g.fillRect (0.0f, 0.0f, kBaseW, kBaseH);
    g.setTiledImageFill (backgroundTexture, 0, 0, 0.35f);
    g.fillRect (0.0f, 0.0f, kBaseW, kBaseH);
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, kBaseW * 0.5f, kBaseH * 0.5f,
                                             juce::Colours::black.withAlpha (0.55f), 0.0f, 0.0f, true));
    g.fillRect (0.0f, 0.0f, kBaseW, kBaseH);

    // top bar
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c2e33), 0.0f, 0.0f, juce::Colour (0xff111214), 0.0f, 56.0f, false));
    g.fillRect (0.0f, 0.0f, kBaseW, 56.0f);
    g.setColour (juce::Colours::black);
    g.fillRect (0.0f, 56.0f, kBaseW, 2.0f);
    g.setColour (juce::Colour (0xff6c6e75).withAlpha (0.6f));
    g.fillRect (0.0f, 58.0f, kBaseW, 1.0f);

    g.setColour (juce::Colour (0xfff2f2f2));
    g.setFont (ui::makeFont (21.0f, true));
    g.drawText ("CRY BABY AUTO WAH", juce::Rectangle<int> (24, 8, 300, 26), juce::Justification::centredLeft, false);
    g.setColour (juce::Colour (0xffd8c27a));
    g.setFont (ui::makeFont (10.5f, true));
    g.drawText ("A U T O M A T I C   E D I T I O N", juce::Rectangle<int> (25, 33, 300, 16), juce::Justification::centredLeft, false);
    g.setColour (juce::Colour (0xff9a9ca3));
    g.setFont (ui::makeFont (11.0f, true));
    g.drawText ("ORYMY  -  YMY RECORDS", juce::Rectangle<int> (700, 8, 236, 40), juce::Justification::centredRight, false);

    // corner screws
    ui::drawPhillipsScrew (g, 10.0f, 10.0f, 4.0f, 0.4f);
    ui::drawPhillipsScrew (g, kBaseW - 10.0f, 10.0f, 4.0f, 1.4f);
    ui::drawPhillipsScrew (g, 10.0f, kBaseH - 10.0f, 4.0f, 2.2f);
    ui::drawPhillipsScrew (g, kBaseW - 10.0f, kBaseH - 10.0f, 4.0f, 0.9f);

    // meter captions
    g.setColour (juce::Colour (0xffc9c9cd));
    g.setFont (ui::makeFont (11.0f, true));
    g.drawText ("INPUT",  juce::Rectangle<int> (30, 452, 220, 14),  juce::Justification::centredLeft, false);
    g.drawText ("OUTPUT", juce::Rectangle<int> (710, 452, 220, 14), juce::Justification::centredLeft, false);
}

void CryBabyAudioProcessorEditor::resized()
{
    const float s = (float) getWidth() / kBaseW;
    auto R = [s] (float x, float y, float w, float h)
    {
        return juce::Rectangle<int> (juce::roundToInt (x * s), juce::roundToInt (y * s), juce::roundToInt (w * s), juce::roundToInt (h * s));
    };

    // preset bar
    prevButton.setBounds (R (330, 14, 28, 30));
    presetBox.setBounds  (R (362, 14, 236, 30));
    nextButton.setBounds (R (602, 14, 28, 30));
    saveButton.setBounds (R (640, 14, 62, 30));
    abButton.setBounds   (R (710, 14, 46, 30));

    // mode row
    float x = 216.0f;
    for (auto& b : modeButtons) { b.setBounds (R (x, 66, 92, 28)); x += 100.0f; }
    aventuraButton.setBounds (R (x + 12.0f, 66, 116, 28));

    // pedal
    pedal.setBounds (R (290, 100, ui::PedalComponent::kW, ui::PedalComponent::kH));

    // knob grid: two columns on each side, two rows
    const float cellW = 112.0f, cellH = 156.0f;
    const float colX[4] = { 30.0f, 156.0f, 692.0f, 818.0f };
    const float rowY[2] = { 120.0f, 288.0f };
    int idx = 0;
    for (int side = 0; side < 2; ++side)
        for (int row = 0; row < 2; ++row)
            for (int col = 0; col < 2; ++col)
            {
                auto& k = knobs[(size_t) idx++];
                const float cx = colX[side * 2 + col], cy = rowY[row];
                k.label.setBounds (R (cx, cy, cellW, 16));
                k.label.setFont (ui::makeFont (12.5f * s, true));
                k.slider.setBounds (R (cx, cy + 18, cellW, cellH - 18));
                k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, juce::roundToInt (84 * s), juce::roundToInt (18 * s));
            }

    // meters + filter scope
    inMeter.setBounds  (R (30, 470, 220, 16));
    outMeter.setBounds (R (710, 470, 220, 16));
    filterDisplay.setBounds (R (290, 462, 380, 66));
}
