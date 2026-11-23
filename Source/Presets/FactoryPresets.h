#pragma once
// Factory presets. Plain C++ (no JUCE) so they can be validated by the unit tests.
// Every value maps to a real parameter of the DSP engine.
// mode: 0 = AUTO, 1 = ENVELOPE, 2 = CYCLIC, 3 = TOUCH   |   outputDb in dB

namespace cbaw::presets
{
struct PresetValues
{
    const char* name;
    int   mode;
    float position, sensitivity, depth, resonance, drive, tone, speed, mix, outputDb;
    bool  aventura;
};

inline constexpr int kNumFactoryPresets = 14;

inline constexpr PresetValues kFactoryPresets[kNumFactoryPresets] =
{
    //  name                   mode  pos   sens  depth res   drive tone  speed mix   out   aventura
    { "AVENTURA CLASSIC",       3,  0.30f, 0.60f, 0.62f, 0.58f, 0.22f, 0.55f, 0.35f, 1.00f,  0.0f, true  },
    { "ROMEO GUITAR",           0,  0.25f, 0.55f, 0.55f, 0.50f, 0.18f, 0.60f, 0.45f, 1.00f,  0.0f, true  },
    { "BACHATA LEAD",           1,  0.35f, 0.65f, 0.60f, 0.55f, 0.30f, 0.62f, 0.40f, 1.00f,  0.0f, false },
    { "CRY BABY ORIGINAL",      1,  0.15f, 0.50f, 0.85f, 0.60f, 0.12f, 0.50f, 0.50f, 1.00f,  0.0f, false },
    { "LATIN SOUL",             0,  0.30f, 0.50f, 0.50f, 0.45f, 0.20f, 0.48f, 0.40f, 0.85f,  0.0f, false },
    { "SWEET TALK",             1,  0.22f, 0.45f, 0.45f, 0.40f, 0.10f, 0.40f, 0.65f, 0.80f,  0.0f, false },
    { "DARK BACHATA",           0,  0.10f, 0.55f, 0.50f, 0.62f, 0.35f, 0.22f, 0.35f, 1.00f, -1.0f, false },
    { "VOCAL WAH",              2,  0.30f, 0.50f, 0.55f, 0.70f, 0.15f, 0.55f, 0.30f, 1.00f,  0.0f, false },
    { "FUNKY GUITAR",           3,  0.18f, 0.75f, 0.85f, 0.65f, 0.35f, 0.60f, 0.25f, 1.00f, -1.0f, false },
    { "MODERN BACHATA",         3,  0.32f, 0.62f, 0.55f, 0.50f, 0.25f, 0.68f, 0.30f, 0.90f,  0.0f, true  },
    { "SOFT AUTO WAH",          0,  0.20f, 0.40f, 0.40f, 0.35f, 0.08f, 0.45f, 0.50f, 0.70f,  0.0f, false },
    { "DEEP CRY",               1,  0.05f, 0.60f, 0.95f, 0.75f, 0.30f, 0.35f, 0.60f, 1.00f, -1.0f, false },
    { "ANTOLOG" "\xC3\x8D" "A STYLE", 0, 0.28f, 0.58f, 0.58f, 0.52f, 0.24f, 0.58f, 0.42f, 0.95f, 0.0f, true },
    { "LIVE PERFORMANCE",       0,  0.25f, 0.60f, 0.60f, 0.50f, 0.20f, 0.58f, 0.50f, 1.00f,  0.0f, false },
};
} // namespace cbaw::presets
