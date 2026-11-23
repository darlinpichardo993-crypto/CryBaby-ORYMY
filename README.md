# CRY BABY AUTO WAH

Automatic wah plugin for electric guitar (bachata / Aventura-style rhythmic and lead wah).
**It is an automatic wah, not an automated one:** the plugin listens to the guitar and moves the
virtual pedal by itself. No DAW automation, MIDI CC or expression pedal is needed.

* Formats: **VST3** (Windows x64), Standalone, AU (only when built on macOS)
* Language / framework: C++17, [JUCE 8](https://juce.com), CMake
* Interface: horizontal, resizable (16:9), black hammertone pedal drawn entirely in code

> ## Verification status (please read)
> * The **DSP engine** (everything in `Source/DSP`, plus the factory preset table) has a JUCE-free self test
>   (`Tests/DspSelfTest.cpp`). It was compiled with g++ 13 and **passed all checks** in the environment where this
>   project was written: filter response vs. analytic curve, attack detection, the four modes, robustness against
>   NaN/extreme settings, preset validity.
> * The **JUCE layer** (processor, editor, GUI, preset manager) and the Windows installer were written carefully against the
>   JUCE 8 API but **have NOT been compiled** yet: the authoring environment had no internet, no JUCE and no Windows/MSVC.
>   The first build on your PC / GitHub Actions is the real compilation test. If the compiler reports something,
>   it will most likely be a small API detail; send me the log and it gets fixed.
> * Nobody has listened to it yet. Sound and feel must be judged by ear in your DAW (see *Tuning* below).

## 1. Requirements (Windows 10/11 x64)

* Visual Studio 2022 with the **Desktop development with C++** workload
* CMake 3.22 or newer
* Git (for downloading JUCE)
* Optional: [Inno Setup 6](https://jrsoftware.org/isinfo.php) to build `Setup.exe`

## 2. Build

```bat
cmake -B build -S . -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The first configure downloads JUCE (see *JUCE* below). Output:

```
build\CryBabyAutoWah_artefacts\Release\VST3\CRY BABY AUTO WAH.vst3\     <- the plugin (a folder bundle)
build\CryBabyAutoWah_artefacts\Release\Standalone\CRY BABY AUTO WAH.exe  <- test app (optional)
```

Copy the `.vst3` **folder** to `C:\Program Files\Common Files\VST3\` and rescan plugins in FL Studio
(Options > Manage plugins > Find more plugins).

Run the DSP self test: `ctest --test-dir build -C Release --output-on-failure`

Options: `-DCBAW_BUILD_STANDALONE=OFF` (VST3 only), `-DCBAW_BUILD_TESTS=OFF`, `-DCBAW_JUCE_TAG=8.0.4`.

## 3. JUCE

Default: CMake downloads JUCE automatically with `FetchContent`.
Alternative: git submodule, see [`External/README.md`](External/README.md) (`git submodule update --init --recursive`).

## 4. GitHub + automatic Windows build

1. Create the repository and push this folder (`git init`, `git add .`, `git commit`, `git push`).
2. GitHub Actions (`.github/workflows/build-windows.yml`) runs on every push: configures with VS 2022 x64,
   builds Release, runs the DSP self test, builds the installer and uploads two artifacts:
   `CryBabyAutoWah-VST3-win64` (zip with the VST3) and `CryBabyAutoWah-Setup` (the `.exe`).
3. Push a tag such as `v1.0.0` to also publish a GitHub Release with both files.

## 5. Installer (Setup.exe)

After a Release build, with Inno Setup installed:

```bat
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" Installer\CryBabyAutoWah.iss
```

Result: `Builds\Installer\CryBabyAutoWah-Setup-1.0.0.exe`. It installs the VST3 into
`C:\Program Files\Common Files\VST3\` (+ optional Standalone) and registers an uninstaller.
Script: `Installer/CryBabyAutoWah.iss` (override version with `/DMyAppVersion=1.2.0`).

## 6. Project layout

```
CMakeLists.txt            JUCE plugin target + DSP self test
Source/
  PluginProcessor.*       audio processing, state, A/B, meters
  PluginEditor.*          main window (scales a 960x540 design)
  Parameters.h            parameter ids and layout (AudioProcessorValueTreeState)
  DSP/                    WahFilter, AutoWahController, WahEngine, envelope/attack detectors (no JUCE)
  Presets/                14 factory presets (FactoryPresets.h) + PresetManager (user presets, XML)
  UI/                     look and feel, pedal, filter scope, meters
Tests/DspSelfTest.cpp     JUCE-free DSP tests
Installer/                Inno Setup script
Resources/  Builds/  External/
.github/workflows/        CI
```

## 7. How it works

**Filter (Cry Baby model).** Zero-delay-feedback state-variable filter used as a resonant band-pass
(the inductor/LC tank of a wah), plus 25 % of the low-pass output so the heel position stays thick.
Frequency sweeps exponentially between ~350 Hz and ~2.2 kHz with an S-shaped pedal taper;
peak gain rises toward the toe position; Q is reduced slightly on hard picking (dynamic response).
After the filter: asymmetric tanh stage (transistor-style saturation) anti-aliased with first-order
antiderivative anti-aliasing (ADAA), so no oversampling latency is needed, then a tone low-pass and DC blockers.

**Virtual foot (`AutoWahController`).** Fast/slow envelope followers and an attack detector (with refractory time)
drive the pedal position. The pedal has simulated inertia (two cascaded smoothers).

| Mode | Behaviour |
|------|-----------|
| AUTO | Level + a "kick" on every attack + slow random drift + a sustained-note sweep that fades in after ~0.2 s; returns to rest in silence; never repeats mechanically |
| ENVELOPE | Opening follows the playing intensity (SENSITIVITY = detector gain, SPEED = release time) |
| CYCLIC | Continuous musical sweep, 0.15 - 9 Hz, asymmetric up/down shape and tiny rate jitter |
| TOUCH | Every pick attack throws the pedal open (fast) and it closes with a decay set by SPEED; stronger picks open further |

**AVENTURA MODE** (toggle): narrower, mid-focused sweep (~450 Hz - 1.9 kHz), slightly higher Q and drive, a touch
more tone and tighter decays: a nasal, rhythmic wah character. It is original DSP voicing, no samples or recordings.

**Controls:** WAH POSITION (drag the treadle on screen, mouse wheel, double-click = reset) is the resting / start
position of the sweep; SENSITIVITY, DEPTH (sweep range), RESONANCE, DRIVE, TONE, SPEED, MIX, OUTPUT (-24..+12 dB);
real BYPASS (click the chrome footswitch, 15 ms crossfade, exact pass-through when off).
The filter display is read-only: it shows what the DSP is doing, it is not an automation lane.

**Presets:** 14 factory presets (AVENTURA CLASSIC ... LIVE PERFORMANCE) that set real parameters; SAVE stores user presets in
`%APPDATA%\ORYMY\CryBabyAutoWah\Presets\*.cbpreset`; A/B compares two full states. All parameters are exposed to the host
and saved with the project.

## 8. Tuning (after your first listening session)

All "feel" constants are in two files: `Source/DSP/WahEngine.cpp` (frequency range, Q range, peak gain, drive) and
`Source/DSP/AutoWahController.h` (attack kick, drift, sustain sweep, touch decay). Factory preset values are in
`Source/Presets/FactoryPresets.h`.

## 9. Licence notes

See `LICENSE`. JUCE requires a commercial licence for closed-source commercial distribution.
"Cry Baby" is a third-party trademark: review the plugin name and the label artwork before selling.
