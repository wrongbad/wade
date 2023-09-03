# libwade

Portable SDK toolkit for realtime audio processing and synthesis.

Goals are:
- Efficient real-time computation and modulation
- Perceptual quality competitive with modern production synthesizers
- Portability to Teensy, Arduino, RasPi, as well as common PC platforms
- Clean modular code. Easy to extend and use. Less is more.

### Features

- Simple wrappers for rtaudio/rtmidi APIs for quick start up (rtaudio.h/rtmidi.h)
- MPE (midi polyphonic expression) first class support (midi.h)
- High quality variable-rate resampler (e.g. tape delay and more) (interp.h)
- Tapped delay-line helper classes (delay.h)
- High quality inpulse-train generators (shapes.h)
- Ultra-light RNG for noise and modulations (rng.h)
- Allpass network reverb (reverb.h)
- Fast approx math functions incl. sin2pi, exp, tanh (math.h)
- Modulation-stable resonant filters (filters.h)
- Virtual-analog variable-step filters (filters.h)

### Code style and philosophy

The author believes code bloat is the root of all evil. The code style of this library tends strongly toward more compact code, at the expense of some common design practices. However abbreviated names are discouraged.

For example, structs used over classes, aggregate constructors frequently used, public member vars rather than getters and setters.

`wade` sources should compile with c++14 and up

### Status: Pre-Alpha

Much refactoring will be needed to achieve organization and clean-code goals.
Expect many bugs and breaking changes for now.

### Memory allocation

Since portability for Teensy/Arduino is required, most heap-allocations are avoided in general.
However larger buffers (delay, reverb, etc) are heap allocated due to global/stack limitations on Teensy platform. Such classes accept a template parameter for the buffer type, allowing custom allocation to be injected this way, but default to std::vector for convenience otherwise.

Some files may have relaxed portability requirements. 
For instance: `wade/rtmidi.h` has no need to run on Arduino.

### Frequencies

Most modules communicate frequency in `cycles / sample` units (i.e. Hz / sample_rate). It saves many accesses to `sample_rate()` along with extra multiplies and divides.

