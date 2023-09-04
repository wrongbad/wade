# libwade

Portable SDK toolkit for realtime audio processing and synthesis.

Goals are:
- Efficient real-time computation and modulation
- Perceptual quality competitive with modern production synthesizers
- Portability to Teensy, Arduino, RasPi, as well as common PC platforms
- Clean modular code. Easy to integrate or extend. Less is more.

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
- Convenience classes for 1d and 2d audio arrays (audio.h)
- Easily adapted to audio with more than 2 channels

### Code style and philosophy

The author believes code bloat is the root of all evil. The code style of this library tends strongly toward more compact code, at the expense of some common design practices. However abbreviated names are discouraged.

For example: structs over classes, aggregate constructors, member var access rather than getters and setters.

Wade sources should compile with c++14 and up.

### Status: Pre-Alpha

Expect many bugs and breaking changes for now.

### Memory allocation

Since portability for Teensy/Arduino is required, most heap-allocations are avoided in general.
However larger buffers (delay, reverb, etc) are heap allocated due to global/stack limitations on Teensy platform. Such classes accept a template parameter for the buffer type, allowing custom allocation to be injected this way, but default to std::vector for convenience otherwise.

Some files may have relaxed portability requirements. 
For instance: `wade/rtmidi.h` has no need to run on Arduino.

### Frequencies

Most modules communicate frequency in `cycles / sample` units (i.e. Hz / sample_rate). It saves the need to pass sample_rate around so much. 

### Reasons I do this

A big reason I'm writing a lot of synth code from scratch is due to finding many things I hate in existing products and software packages.

I hate it when I change preset wave or effect and there's a hiccup in the output audio. 

I hate it when I turn a knob for delay/echo and it's clearly skipping/jumping over the "tape". 

I hate it when I turn a knob and the quantization stepping of the ADC or cheap encoder is clearly audible.

I hate it when I switch polyphonic-monophonic mode and it turns all notes off. 

I hate it when effects have unexpected audible distortion (e.g. aliasing) when playing high-notes. 

