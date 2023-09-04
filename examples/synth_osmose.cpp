#include <unistd.h> // sleep

#include "RtAudio.h"
#include "RtMidi.h"

#include "wade/rtaudio.h"
#include "wade/rtmidi.h"

#include "wade/shapes.h"
#include "wade/filters.h"
#include "wade/reverb.h"

using namespace wade;

struct square_driver
{
    lofi_square wave;

    void render(mono out, audio_context & ctx, voice_control & control)
    {
        if(!control.gate) { return; }
        
        wave.freq = midi_hz(control.note + control.pitch_bend) / ctx.sample_rate;
        wave.pulse = 0.48 + 0.5 * control.pressure;
        
        float amp = 0.1
            + 0.4 * control.pressure
            + 0.6 * control.controls[74];

        for(float & x : out) { x = wave() * amp; }
    }
};

struct blip_driver
{
    impulse_train wave;

    state_variable_filter filter;
    float pressure = 0;
    basic_lowpass envelope;
    tape_delay<> tape;
    
    void render(mono out, audio_context & ctx, voice_control const& control)
    {

        float raw_pressure = control.gate * (control.pressure + control.controls[74]) / 2;
        // float raw_pressure = control.gate * control.pressure;
        pressure += (raw_pressure - pressure) * 0.5;

        float note = control.note - 12;// + control.pitch_bend;
        wave.freq = midi_hz(note) / ctx.sample_rate;

        for(float & x : out) { x = wave() * envelope(pressure); }

        float bass_boost = 1 + 0.5 * softsign((36 - note) / 24);
        for(float & x : out) { x *= bass_boost; }

        filter.set(0.15 * (pressure) + 0.5 * wave.freq / 6.28, 0.6 + 0.8 * pressure);
        for(float & x : out) { x = filter(x).lp(); }

        tape.feedback = 0.2;
        tape.delay = 4000 + 200 * control.pitch_bend;
        for(float & x : out) { x += 0.5 * tape(x); }
    }
};

struct finn_driver
{
    finnwave wave;
    finnwave wave2;
    // impulse_train wave2;
    // lofi_square wave;

    state_variable_filter filter;
    float pressure = 0;
    float envelope = 0;

    tape_delay<> tape;
    
    void render(mono out, audio_context & ctx, voice_control const& control)
    {
        float raw_pressure = control.gate * (control.pressure + control.controls[74]) / 2;
        pressure += (raw_pressure - pressure) * 0.5;

        // if(pressure < 0.0001) { return; }

        float note = control.note - 24 + control.pitch_bend;
        wave.freq = midi_hz(note) / ctx.sample_rate;
        // wave.rolloff = (1-raw_pressure) * 1;
        wave.rolloff = std::exp(pressure * -4 + 0.035 * note);
        wave.rolloff = wave.rolloff * wave.rolloff + 0.2;
        wave.amp = pressure;
        wave.render(out);
        // for(float & x : out) { x *= -1; }

        // float note2 = note + 12;// + control.pitch_bend * 1; // detune with bend
        // wave2.freq = midi_hz(note2) / ctx.sample_rate;
        
        // wave2.render(out);
        float note2 = note + 12;
        wave2.freq = midi_hz(note2) / ctx.sample_rate;
        wave2.rolloff = wave.rolloff * 2 + (pressure)/4;
        wave2.amp = -std::exp(-wave.rolloff) * wave.amp;
        wave2.render(out);

        // for(float & x : out)
        // { 
        //     envelope += (pressure - envelope) * 0.01;
        //     x *= envelope;
        // }

        float boost = (1 - 0.5 * softsign((note - 24) / 24));
        for(float & x : out) { x *= boost; }

        // filter.set(0.15 * pressure + wave.freq / 6.28, 0.6 + 0.8 * pressure);
        // for(float & x : out) { x = filter(x).lp(); }

        // tape.feedback = 0.2;
        // tape.delay = 4000 + 200 * control.pitch_bend;
        // for(float & x : out) { x += 0.5 * tape(x); }
    }
};

struct synth_manager
{
    wade::rtaudio<synth_manager> audio_driver {*this};
    wade::rtmidi midi_driver;
    std::array<blip_driver, 4> voices = {};
    wade::mpe_handler mpe;
    wade::basic_reverb2<> reverb;

    std::vector<float> tmp_buf;

    void render(wade::audio in, wade::audio out, wade::audio_context & ctx)
    {
        midi_driver.receive(mpe);

        tmp_buf.resize(out.frames);
        wade::mono tmp {tmp_buf.data(), (int)tmp_buf.size()};

        // mpe.render(8, [&] (auto & voice) {
        //     voice.render(tmp, ctx);
        //     out[0] += tmp;
        // });

        // mpe.map_voices(voices.size());
        // for(int i=0 ; i<voices.size() ; i++)
        // {
        //     // use isolated tmp buffer for voice effects
        //     voices[mpe[i].voice_map].render(tmp, ctx, mpe[i]);
        //     // then mix into common out[0] buffer
        //     out[0] += tmp;
        // }

        mpe.map(voices, voices.size(), 
            [&] (auto & voice, voice_control const& ctl) {
                voice.render(tmp, ctx, ctl);
                out[0] += tmp;
            });

        // apply shared effects
        out[0].apply(reverb);

        // upmix to stereo
        for(wade::mono ch : out) { ch = out[0]; }
    }
};

int main(int argc, char ** argv)
{
    std::string hint = "Osmose";
    if(argc >= 2) { hint = argv[1]; }

    synth_manager synth;
    synth.midi_driver.connect_input(hint);
    synth.audio_driver.start();

    while(1) { sleep(5); } // wait for ctrl-c
}