#pragma once

#include "delay.h"
#include "filter.h"
#include "midi.h"
#include "control.h"

namespace wade {


// struct simple_flute : has_controls<8>
// {
//     control & reed_pressure() { return controls[0]; }
//     control & damping() { return controls[6]; }
//     control & force() { return controls[7]; }

struct simple_flute
{
    float freq = 0;
    float reed_pressure = 0.5;
    float damping = 0.9;
    float force = 0;

    simple_flute()
    {   
        // _period = 1 / midi_freq(69);
        // _delay.set_delay(_period / 2 - 1.5);
    }
    
    void note_on(uint8_t note, uint8_t vel)
    {   
        _period = 1 / midi_freq(note);
        _delay.set_delay(_period / 2 - 1.5);
        // _delay.set_reflection(-0.95);
        _delay.clear();
        // _gate = 0.8 + vel * (0.6 / 127);
        _gate = 1;
        // _gate = 1.1 + vel * (0.2 / 127);
        // _softgate.rate = (vel + 100.0f) / 80000.0f;
        _softgate.rate = (vel + 100.0f) / 10000.0f;
        _noise_scale = 0.1;

        _vel = vel / 127.0f;
        _velbias = 1;

        _press = 0.5;

        _filter.set_rolloff(0.8);
        _delay.set_reflection(-0.95);

        // _insert = 1;

        // float rolloff = _controls[0];
        // _filter.set_rolloff(0.8);
        // _filter2.set_rolloff(0.8);
    }
    void note_off(uint8_t note, uint8_t vel)
    {   _gate = 0;
        _softgate.rate = 0.001;
    }
    void note_pressure(uint8_t note, uint8_t press)
    {   _press = press / 127.0f;
    }

    static float reed(float x)
    {   float y = 0.7 - 0.3 * x;
        if(y < 0) { return 0; }
        if(y > 1) { return 1; }
        return x * y;
    }
    static float jet(float x)
    {   float y = x - x * x * x;
        if(y < -1) { return -1; }
        if(y > 1) { return 1; }
        return y;
    }

    float mouth_pressure()
    {
        float press2 = _press + _velbias * (_vel - _press);
        _velbias *= 0.999;
        // press2 *= fast::exp(_controls[0] * 2 - 1);
        float sp = _softpress(press2);

        float press = _softgate(_gate);
        press = press * ((sp+0.5) + (_rng() >> 16) * ((_noise_scale + 0.05) / 65536));
        press += press * _lfo() * 0.05;


        // float press2 = _press + _velbias * (_vel - _press);
        // _velbias *= 0.999;
        // float sp = _softpress(press2);

        // float press = _softgate(_gate) * (sp+0.5);
        // press += press * (_rng() >> 16) * ((_noise_scale + 0.05) / 65536);
        // press += press * _lfo() * 0.05;

        _noise_scale *= 0.9997;

        return press;   
    }

    float apply_reed(float dpress)
    {
        // reed normally open, positive dpress pushes it closed
        float reed = 0.8 * _controls[1] + _lfo2() * 0.06;
        reed -= 0.3 * dpress;
        _reed += (reed - _reed) * 0.3;
        if(_reed < 0) { _reed = 0; }
        if(_reed > 1) { _reed = 1; }
        dpress *= _reed;
        // dpress *= fast::exp(_controls[0] - 0.5);
        return dpress;
    }

    float bow_speed()
    {
        float bowv = _softgate._y; // XXX computed in mouth_pressure
        bowv += bowv * (_rng() >> 16) * ((_noise_scale + 0.1) / 65536);
        return bowv;
    }

    float bow_force(float dv, float force_scale)
    {
        float softgate = _softgate._y; // XXX computed in mouth_pressure
        float ufric = 0.3f + 1 / (1 + 4 * std::abs(dv));
        float force = softgate * force_scale * ufric * wade::sign(dv);
        _force += 0.4 * (force - _force);
        if(std::abs(_force) > std::abs(dv))
        {   _force = dv;
        }
        return _force;
    }

    void render(float * buf, int len)
    {   
        // const float gain = 0.98;

        float gain = 1 - (1-_controls[6]) * 0.4;

        if(_period == 0) { return; }

        float blowpos = (0.01 + _controls[2]*0.23);

        float force_scale = .1 * (fast::exp(_controls[7] * 6) - 1);
        float speed_scale = .1 * (fast::exp(_controls[8] * 6) - 1);
        float bowpos = (0.01 + _controls[9]*0.23);
        float mouth_gain = fast::exp(_controls[0] * -2 + 1);

        // _filter.set_rolloff(0.8);
        // _delay.set_reflection(-0.95);

        for(float & y : span(buf,len))
        {   
            int tap = _blowpos(blowpos) * (_period-3);
            float dpress = mouth_pressure() + y - _delay.at(tap) * mouth_gain;
            _delay.insert(tap, apply_reed(dpress));

            tap = _bowpos(bowpos) * (_period-3);
            float dv = bow_speed() * speed_scale - _delay.at(tap);
            _delay.insert(tap, bow_force(dv, force_scale));

            _y = gain * _filter(_delay(_y)); // complete the loop
            y = _dcblock(_y); // tap the output
        }
    }

private:
    float * _controls;

    wade::wave_delay<2048> _delay;
    float _y = 0; // prev delay output

    float _vel = 0;
    float _gate = 0;
    wade::slew _softgate;

    float _period = 0;

    float _reed = 1;
    float _noise_scale = 0;

    float _force = 0;
    
    wade::slew _blowpos { 0.00002 };
    wade::slew _bowpos { 0.00002 };

    wade::fir_damper<7> _filter;
    
    wade::filter<2, 2> _dcblock {{1, -1}, {1, -0.99}}; // TODO .995

    wade::rng32 _rng;
    wade::sinwave _lfo { 6 / sample_rate() };
    wade::sinwave _lfo2 { 5.1f / sample_rate() };


    float _press = 0;
    wade::slew _softpress { 0.002 };
    float _velbias = 0; // transfer from vel to aftertouch

};

} // wade