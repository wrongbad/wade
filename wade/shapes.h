#pragma once

#include "wade/math.h"

namespace wade {

struct lofi_square
{
    float freq = 0.001;
    float pulse = 0.5;
    float phase = 0;

    float operator()()
    {   
        phase += freq - (phase > 1);
        return phase>pulse ? 1 : -1;
    }
};


struct impulse_train
{
    float freq = 0.001;
    float nphase = 0;

    void render(mono out)
    {   
        int N = (int(0.25 / freq) - 1) | 1; // round down to odd number
        float nfreq = N * freq / 2;
        for(float & o : out)
        {
            nphase += nfreq;
            if(nphase > N/4) { nphase -= N/2.0f; }
            o += (std::abs(nphase) < 0.01) ? 1 :
                fast::sin2pi(nphase) / (N * fast::sin2pi(nphase / N));
        }
    }
};


} // wade