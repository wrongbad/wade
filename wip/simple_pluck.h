#pragma once

#include "delay.h"
#include "filter.h"
#include "midi.h"

namespace wade {

class simple_pluck
{
public:
    simple_pluck(float const* controls)
    :   _controls(controls)
    {   
    }
    void note_on(uint8_t chan, uint8_t note, uint8_t vel)
    {   float _period = 1 / midi_freq(note);
        _pulse = 4 + (1-_controls[0]) * 128;
        _insert = _period * (_controls[1] * -0.48 + 0.49);
        _delay.set_delay(_period - 1);
        _pulset = 0;
        _gate = 1;
    }
    void note_off(uint8_t chan, uint8_t note, uint8_t vel = 0)
    {   _gate = 0;
    }
    void render(float * out, int samples)
    {   float volume = _controls[6] * 1.3f;

        float bleed = (1-_controls[3]) + 0.0001;
        float gain = std::exp(-_period * bleed / 4e3);
        float rolloff = _controls[4];
        _filter.set_rolloff(rolloff);
        _delay.set_reflection(-gain);

        for(int i=0 ; i<samples ; i++)
        {   if(_pulset < _pulse)
            {   _pulset ++;
                float n = volume * fast::sin2pi(_pulset / 2.0f / _pulse);
                _delay.insert(_insert, n);
            }
            _y = _delay(_y);
            _y = _filter(_y);
            _y = -wade::softsign(_y, 4);
            out[i] += _delay.at(_insert);
        }
    }
private:
    float const* _controls;
    float _period = 1000;
    int _insert = 10;
    int _pulset = 0;
    int _pulse = 0;
    bool _gate = 0;
    wade::wave_delay<2048> _delay;
    wade::fir_damper<7> _filter;
    float _y = 0;
};

} // wade