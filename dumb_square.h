#pragma once

namespace wade {

// "dumb" because it "aliases"

struct dumb_square
{
    bool on = 0;
    float freq = 0;
    float phase = 0;
    float bend = 1;
    float pulse = 0.5;

    void key_down(byte chan, byte note, byte vel)
    {   freq = midi_freq(note);
        on = 1;
    }
    void key_up(byte chan, byte note, byte vel = 0)
    {   on = 0;
    }
    void render(float * out, int len)
    {   if(!on) { return; }
        for(int i=0 ; i<len ; i++)
        {   phase += freq * bend;
            phase -= phase > 1;
            out[i] += phase > pulse ? 0.2 : -0.2;   
        }
    }
};

} // wade