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
    impulse_train wave2;
    // lofi_square wave;

    state_variable_filter filter;
    float pressure = 0;

    tape_delay<> tape;
    
    void render(mono out, audio_context & ctx, voice_control & control)
    {
        // if(!control.gate) { return; }

        float note = control.note - 12;// + control.pitch_bend;
        wave.freq = midi_hz(note) / ctx.sample_rate;

        float note2 = note + 12;// + control.pitch_bend * 1; // detune with bend
        wave2.freq = midi_hz(note2) / ctx.sample_rate;
        
        float raw_pressure = control.gate * (control.pressure + control.controls[74]) / 2;
        pressure += (raw_pressure - pressure) * 0.5;
        filter.set(0.15 * pressure + wave.freq / 6.28, 0.6 + 0.8 * pressure);
        
        wave.render(out);
        for(float & x : out) { x *= -1; }

        wave2.render(out);

        // for(float & x : out) { x = wave(); }

        float amp = pressure;
        for(float & x : out) { x *= amp; }

        for(float & x : out) { x = filter(x).lp(); }

        tape.feedback = 0.8;
        tape.delay = 4000 + 200 * control.pitch_bend;
        for(float & x : out) { x += 0.5 * tape(x); }
    }
};

struct synth_manager
{
    wade::rtaudio<synth_manager> audio_driver {*this};
    wade::rtmidi midi_driver;
    wade::mpe_handler mpe;
    blip_driver voices[16] = {};
    wade::basic_reverb2<> reverb;

    std::vector<float> buf0;

    void render(wade::audio in, wade::audio out, wade::audio_context & ctx)
    {
        midi_driver.receive(mpe);

        out.fill(0);

        buf0.resize(out.frames);
        wade::mono ch0 {buf0.data(), (int)buf0.size()};
        for(int i=0 ; i<16 ; i++)
        {
            ch0.fill(0);
            voices[i].render(ch0, ctx, mpe.voices[i]);
            out[0] += ch0;
        }

        out[0].apply(reverb);

        for(mono ch : out) { ch = out[0]; }
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