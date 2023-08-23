#pragma once

#include <cmath>
#include <complex>

namespace wade {

// WIP... problems:
// teensy 4.1 50% CPU with 4 voices @ 32 harmonics
// 2 pole filters not very stable at low damping with float32

template<int MaxN = 32>
class modal
{
public:
    modal(float const* controls)
    :   _controls(controls)
    {   
    }
    void key_down(byte chan, byte note, byte vel)
    {   
        _freq = midi_freq(note);
        _n = min(int(0.5 / _freq), MaxN);
        constexpr float beta = 0.0003;
        // Serial.printf("n: %d\n", _n);
        float r = 0.9998;
        for(int h=0 ; h<_n ; h++)
        {   float f = _freq * h * std::sqrt(1 + beta * h * h);
            _fb[h][0] = 2 * r * std::cos(f * M_PI);
            _fb[h][1] = -r * r;
            _y[h][0] = _y[h][1] = 0;
        }
        _t = 0;
        _gate = 1;
    }
    void key_up(byte chan, byte note, byte vel = 0)
    {   
        _gate = 0;
    }
    void render(float * out, int samples)
    {   //if(!_gate) { return; }
        float volume = _controls[6] * 0.3;
        for(int i=0 ; i<samples ; i++)
        {   float noise = 0;
            // if(_gate) { noise = int32_t(_rng()) / float(1<<31) / float(1<<16); }
            constexpr int pulse = 255;
            if(_t++ < pulse)
            {   
                noise = 0.01 * volume * fast::sin2pi(_t / 2.0f / pulse);
                // noise = 1;
            }

            for(int h=0 ; h<_n ; h++)
            {   float y = noise;
                y += _fb[h][0] * _y[h][0];
                y += _fb[h][1] * _y[h][1];
                y = fast::hardclip(y, 2);
                _y[h][1] = _y[h][0];
                _y[h][0] = y;
                out[i] += y;
            }
        }
    }
private:
    float const* _controls;
    int _print = 0;
    // XorShift32 _rng;
    float _freq = 0;
    float _phase = 0;
    float _pulse = 0.4;
    int _n = 0;
    int _t = 0;
    bool _gate = 0;
    float _fb[MaxN][2] = {{0}};
    float _y[MaxN][2] = {{0}};
};


template<int MaxN = 32>
class modal2
{
public:
    using complex = std::complex<float>;
    modal2(float const* controls)
    :   _controls(controls)
    {   
    }
    void key_down(byte chan, byte note, byte vel)
    {   _freq = midi_freq(note);
        _n = min(int(0.5 / _freq), MaxN);
        float hpower = _controls[0] * 3;
        _pulse = 4 + _controls[1] * 1020;
        float inharm = _controls[2] * 0.003;
        float decay = (1-_controls[3]) * -0.0001;
        _release = std::exp((1-_controls[4]) * -0.001);
        for(int h=0 ; h<_n ; h++)
        {   float f = _freq * (h+1) * std::sqrt(1 + inharm * h * h);
            float hdecay = decay * std::pow(h+1, hpower);
            _dy[h] = std::exp(complex(hdecay, f * 2 * M_PI));
            _y[h] = 0;
        }
        _pulset = 0;
        _gate = 1;
    }
    void key_up(byte chan, byte note, byte vel = 0)
    {   _gate = 0;
    }
    void render(float * out, int samples)
    {   float volume = _controls[6] * 0.3f;
        for(int i=0 ; i<samples ; i++)
        {   float noise = 0;
            if(_pulset < _pulse)
            {   noise = volume * fast::sin2pi(_pulset / 2.0f / _pulse);
                _pulset ++;
            }
            for(int h=0 ; h<_n ; h++)
            {   _y[h] = _y[h] * _dy[h] + noise;
                if(!_gate) { _y[h] *= _release; }
                out[i] += _y[h].real();
            }
        }
    }
private:
    float const* _controls;
    float _freq = 0;
    int _n = 0;
    int _pulset = 0;
    int _pulse = 0;
    bool _gate = 0;
    float _release = 0.999f;

    complex _y[MaxN] = {0};
    complex _dy[MaxN] = {0};
};


} // wade