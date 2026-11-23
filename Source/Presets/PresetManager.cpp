#include "PresetManager.h"
#include "FactoryPresets.h"
#include "../Parameters.h"

namespace cbaw
{
static const char* kPresetNameProperty = "presetName";
static const char* kPresetExtension    = ".cbpreset";

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& s) : apvts (s)
{
    if (! apvts.state.hasProperty (kPresetNameProperty))
        apvts.state.setProperty (kPresetNameProperty, "Init", nullptr);
}

int PresetManager::getNumFactoryPresets() const { return presets::kNumFactoryPresets; }

juce::String PresetManager::getFactoryPresetName (int index) const
{
    if (index < 0 || index >= presets::kNumFactoryPresets) return {};
    return juce::String::fromUTF8 (presets::kFactoryPresets[index].name);
}

juce::File PresetManager::getUserPresetDirectory()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("ORYMY").getChildFile ("CryBabyAutoWah").getChildFile ("Presets");
}

juce::String PresetManager::getCurrentPresetName() const
{
    return apvts.state.getProperty (kPresetNameProperty, "Init").toString();
}

void PresetManager::setCurrentPresetName (const juce::String& name)
{
    apvts.state.setProperty (kPresetNameProperty, name, nullptr);
}

void PresetManager::setParameterValue (const juce::String& id, float realValue)
{
    if (auto* param = apvts.getParameter (id))
        param->setValueNotifyingHost (param->convertTo0to1 (realValue));
}

void PresetManager::loadFactoryPreset (int index)
{
    if (index < 0 || index >= presets::kNumFactoryPresets) return;
    const auto& p = presets::kFactoryPresets[index];

    setParameterValue (ids::mode,        (float) p.mode);
    setParameterValue (ids::position,    p.position);
    setParameterValue (ids::sensitivity, p.sensitivity);
    setParameterValue (ids::depth,       p.depth);
    setParameterValue (ids::resonance,   p.resonance);
    setParameterValue (ids::drive,       p.drive);
    setParameterValue (ids::tone,        p.tone);
    setParameterValue (ids::speed,       p.speed);
    setParameterValue (ids::mix,         p.mix);
    setParameterValue (ids::output,      p.outputDb);
    setParameterValue (ids::aventura,    p.aventura ? 1.0f : 0.0f);
    setCurrentPresetName (getFactoryPresetName (index));
}

juce::StringArray PresetManager::getUserPresetNames() const
{
    juce::StringArray names;
    auto dir = getUserPresetDirectory();
    if (dir.isDirectory())
        for (const auto& f : dir.findChildFiles (juce::File::findFiles, false, "*" + juce::String (kPresetExtension)))
            names.add (f.getFileNameWithoutExtension());
    names.sort (true);
    return names;
}

bool PresetManager::saveUserPreset (const juce::String& rawName)
{
    const auto name = rawName.trim();
    if (name.isEmpty()) return false;

    auto dir = getUserPresetDirectory();
    if (! dir.createDirectory().wasOk()) return false;

    juce::XmlElement xml ("CBAWPreset");
    xml.setAttribute ("name", name);
    xml.setAttribute ("version", 1);

    for (const char* id : kPresetParamIds)
    {
        if (auto* raw = apvts.getRawParameterValue (id))
        {
            auto* e = xml.createNewChildElement ("P");
            e->setAttribute ("id", id);
            e->setAttribute ("v", (double) raw->load());
        }
    }

    auto file = dir.getChildFile (juce::File::createLegalFileName (name) + kPresetExtension);
    if (! xml.writeTo (file)) return false;

    setCurrentPresetName (name);
    return true;
}

bool PresetManager::loadUserPreset (const juce::String& name)
{
    auto file = getUserPresetDirectory().getChildFile (juce::File::createLegalFileName (name) + kPresetExtension);
    if (! file.existsAsFile()) return false;

    std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (file));
    if (xml == nullptr || ! xml->hasTagName ("CBAWPreset")) return false;

    for (auto* e : xml->getChildWithTagNameIterator ("P"))
    {
        const auto id = e->getStringAttribute ("id");
        for (const char* known : kPresetParamIds)          // only accept known parameter ids
            if (id == known)
                setParameterValue (id, (float) e->getDoubleAttribute ("v"));
    }

    setCurrentPresetName (name);
    return true;
}

bool PresetManager::deleteUserPreset (const juce::String& name)
{
    auto file = getUserPresetDirectory().getChildFile (juce::File::createLegalFileName (name) + kPresetExtension);
    return file.existsAsFile() && file.deleteFile();
}
} // namespace cbaw
