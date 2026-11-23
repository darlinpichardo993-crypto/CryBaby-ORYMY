Resources
=========
CRY BABY AUTO WAH needs NO external image, font or audio files.

The whole interface (the black hammertone pedal, chrome footswitch, screws,
knobs, LEDs, meters and the filter scope) is drawn in code with JUCE vector
graphics and a procedural metal texture, see Source/UI/. That makes the
plugin fully resizable and the repository self-contained.

If you later want photographic artwork, add the PNG files here and register
them with juce_add_binary_data() in CMakeLists.txt.
