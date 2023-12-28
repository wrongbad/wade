#include <unistd.h> // sleep

#include "RtAudio.h"
#include "RtMidi.h"

#include "wade/rtaudio.h"
#include "wade/rtmidi.h"

#include "wade/shapes.h"
#include "wade/filters.h"
#include "wade/reverb.h"

using namespace wade;

struct Voice
{
    lofi_square wave;
    basic_lowpass envelope;

    void render(mono out, audio_context & ctx, voice_control & control)
    {
        if(!control.gate) { return; }
        
        wave.freq = midi_hz(control.note + control.pitch_bend) / ctx.sample_rate;
        
        for(float & x : out) { x = wave() * envelope(control.gate); }
    }
};

struct Synth
{
    wade::rtaudio<Synth> audio_driver {*this};
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

        mpe.map(voices, voices.size(), [&] (auto & voice, auto & ctl) {
            // use isolated tmp buffer for voice effects
            voice.render(tmp, ctx, ctl);
            // then mix into common out[0] buffer
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