#pragma once 

#include "arr.h"
#include "wmath.h"
#include "midi.h"
#include <algorithm>

namespace wade {

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

class finnwave
{
public:
    static constexpr int num_controls() { return 7; }

    finnwave(float const* controls)
    :   _controls(controls)
    {   
    }
    void note_on(uint8_t note, uint8_t vel)
    {   _freq = midi_freq(note) * _srate;
        _key = note;
        _gate = 1;
        _phase = 0;
        _attacking = true;
    }
    void note_off(uint8_t note, uint8_t vel = 0)
    {   _gate = 0;
        _attacking = false;
    }
    void note_pressure(uint8_t note, uint8_t press)
    {   _press = press / 160.0f;
    }
    void render(float * buf, int len)
    {   
        using std::min; using std::max;

        // bass boost
        float volume = 0.3 * (1 - 0.5 * softsign((_freq - 150) / 100));

        float attack = fast::exp(max(_controls[5]-0.5f, 0.0f) * 25) + 50;
        float decay = fast::exp(min(_controls[5], 0.6f) * 30 + 5) + 50;
        float release = fast::exp(_controls[6] * 13) + 50;
        float env2filter = _controls[4];

        // harmonics below nyquist
        int n = _srate / 2 / _freq;

        float p = _phase;
        float dp = _freq / _srate;

        // softer than low-pass cutoff
        float mush = _softpress(_press) + (1 - env2filter + env2filter * _env) * _controls[0];
        float lp_mushoff = fast::exp(mush * -4 + _key * 0.02);

        float b = lp_mushoff;// / (_env + 0.001);
        float e_b = std::exp(-b);
        float e_2b = e_b * e_b;
        float e_n1b = std::exp(-(n+1) * b);
        float e_n2b = e_n1b * e_b;

        for(float & y : span(buf,len))
        {   
            p += dp;

            if(_attacking) { _env += max(_gate - _env, 0.0f) / attack; }
            else if(decay < _srate*10) { _env += min(0 - _env, 0.0f) / decay; }
            _env += max((_gate - 1)/release, -_env);

            if(_env > 0.95) { _attacking = false; }
            
            if(volume == 0 || _env == 0) { y = 0; continue; }

            float a = b * 10 * _env;// * (1-env2filter) + env2filter);
            
            // TODO call sin/cos first sample of block
            // then rotation formula within block
            float sin_p = fast::sin2pi(p);
            float sin_np = fast::sin2pi(n*p);
            float cos_p = fast::cos2pi(p);
            float cos_np = fast::cos2pi(n*p);
        
            y = a * ( 
                sin_p * (e_b - e_n1b * cos_np) +
                sin_np * (e_n2b - e_n1b * cos_p)
            ) / ( 1 - 2*e_b*cos_p + e_2b );
        }
        _phase = p - fast::floor(p);
    }
private:
    float const* _controls;
    int _key = 0;
    bool _attacking = false;
    float _freq = 0;
    float _srate = 44100;
    float _gate = 0;
    float _env = 0;
    float _phase = 0;
    float _press = 0;
    wade::slew _softpress { 0.001 };
};

} // wade