# External dependencies

JUCE is the only dependency. You have two options, the build picks automatically:

1. **Nothing to do (default):** if this folder has no `JUCE/` subfolder, CMake downloads
   JUCE 8.0.4 with `FetchContent` (needs internet and git).
2. **Git submodule (reproducible, offline-friendly):**
   ```bash
   git submodule add https://github.com/juce-framework/JUCE.git External/JUCE
   git -C External/JUCE checkout 8.0.4
   git add .gitmodules External/JUCE && git commit -m "Add JUCE 8.0.4 submodule"
   ```
   After cloning the repo on another PC: `git submodule update --init --recursive`.
