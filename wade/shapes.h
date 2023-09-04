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

    float operator()()
    {   
        int N = (int(0.25 / freq) - 1) | 1; // round down to odd number
        nphase +=  N * freq / 2;
        if(nphase > N/4) { nphase -= N/2.0f; }
        return (std::abs(nphase) < 0.1) ? 1 :
            fast::sin2pi(nphase) / (N * fast::sin2pi(nphase / N));
    }
};



// generate band limited waveforms 
// with exponentially decaying harmonics
// using complex finite geomtric series sum
// Kyle Finn 2014

// two such waves, one inverted at 2 * f0
// can selectively cancel out even harmonics, giving square-like sounds

// adjusting the harmonic rolloff rate has a low pass effect
// but sounds much more "natural" than sliding a filter cutoff

// tying harmonic rolloff to envelope can perfectly model
// a decay rate propoertional to frequency

struct finnwave
{
    float freq = 0.001;
    float rolloff = 0.3; // 0 = flat, 1 = heavy low-pass
    float amp = 1;
    float nphase = 0;

    float smooth_rolloff = rolloff;
    float smooth_amp = 0;

    void render(mono out)
    {   
        int N = int(0.5 / freq);
        // N = N - 1 | 1;
        // N = N & ~1;
        float nfreq = N * freq;
        while(nphase > N/2) { nphase -= N; }


        float b = rolloff;
        float e_b = std::exp(-b);
        float e_2b = e_b * e_b;
        float e_n1b = std::exp(-(N+1) * b);
        float e_n2b = e_n1b * e_b;

        for(float & y : out)
        {   
            nphase += nfreq;
            if(nphase > N) { nphase -= N; }

            smooth_rolloff += (rolloff - smooth_rolloff) * 0.01;
            smooth_amp += (amp - smooth_amp) * 0.01;

            float b = smooth_rolloff;
            float e_b = std::exp(-b);
            float e_2b = e_b * e_b;
            float e_n1b = std::exp(-(N+1) * b);
            float e_n2b = e_n1b * e_b;
            
            // TODO call sin/cos first sample of block
            // then rotation formula within block
            float sin_p = fast::sin2pi(nphase / N);
            float cos_p = fast::cos2pi(nphase / N);
            float sin_np = fast::sin2pi(nphase);
            float cos_np = fast::cos2pi(nphase);
        
            y += b * smooth_amp * ( 
                sin_p * (e_b - e_n1b * cos_np) +
                sin_np * (e_n2b - e_n1b * cos_p)
            ) / ( 1 - 2*e_b*cos_p + e_2b );
        }
    }
};

} // wade