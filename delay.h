#pragma once

#include "filter.h"
#include "wmath.h"
#include "arr.h"
#include <algorithm>

namespace wade {

template<int MaxSize>
struct delay
{   wade::ring<float, MaxSize> _buf {0};
    int _delay = 0;

    void clear()
    {   for(int i=0 ; i<MaxSize ; i++)
        {   _buf[i] = 0;
        }
    }

    int get_delay() const { return _delay; }

    void set_delay(float del) { _delay = del; }

    float operator()(float x)
    {   _buf.shift(1);
        _buf[0] = x;
        return _buf[_delay];
    }
};


template<int MaxSize>
struct interp_delay
{   wade::ring<float, MaxSize> _buf {0};
    wade::ap_interp _interp;
    float _delay = 0;
    int _idelay = 0;

    void clear()
    {   for(int i=0 ; i<MaxSize ; i++)
        {   _buf[i] = 0;
        }
    }

    float get_delay() const { return _delay; }

    void set_delay(float del)
    {   if(del < 1) { del = 1; }
        _delay = del;
        _idelay = int(del - 0.5); // 0.5 <= frac < 1.5
        _interp.set_delay(del - _idelay);
    }

    float operator()(float x)
    {   _buf.shift(1);
        _buf[0] = x;
        return _interp(_buf[_idelay]);
    }
};


template<int MaxSize>
struct wave_delay : public interp_delay<MaxSize>
{   float _reflect = 1;
    using interp_delay<MaxSize>::_buf;
    using interp_delay<MaxSize>::_idelay;
    
    void set_reflection(float reflect)
    {   _reflect = reflect;
    }

    float at(int offset)
    {   return _buf[offset] + _buf[_idelay - offset];
    }

    void insert(int offset, float x)
    {   _buf[offset] += x / 2;
        _buf[_idelay - offset] += x / 2;
    }

    void insert(int offset, float forward, float reverse)
    {   _buf[offset] += forward;
        _buf[_idelay - offset] += reverse;
    }

    float & mid() { return _buf[_idelay/2]; }

    float operator()(float x)
    {   mid() *= _reflect;
        return interp_delay<MaxSize>::operator()(x);
    }
};


template<int MaxSize>
class tape_delay
{
public:
    static constexpr int num_controls() { return 7; }

    tape_delay(float * controls)
    :   _controls(controls),
        _buf(0)
    {
        _controls[0] = 1; // clean
        _controls[1] = 0; // fx-send
        _controls[5] = 0; // lfo dep
        _controls[6] = 0; // lfo freq
    }
    static float next(float x)
    {   float y = std::ceil(x);
        return y==x ? y+1 : y;
    }
    static float prev(float x)
    {   float y = std::floor(x);
        return y==x ? y-1 : y;
    }
    void render(float * audio, int frames)
    {   using std::min; using std::max;
        int s = MaxSize;
        float clean = _controls[0];
        float fx_send = _controls[1];
        float g = min(_controls[2], 0.999f);

        float lfo_dep = (fast::exp(_controls[5] * 3) - 1) * 100;
        _lfo.freq = fast::exp(_controls[6] * 7 - 13);

        // float delay_in = 1 + _controls[2] * (s-2);

        float delay_in = lfo_dep+1 + _controls[3] * (s - 2*(lfo_dep+1));

        if(_delay_in < 0)
        {   _delay_in = delay_in;
            _delay = _delay_in + 1;
        }

        for(int i=0 ; i<frames ; i++)
        {   
            _delay_in += (delay_in + _lfo() * lfo_dep - _delay_in) * 0.0003;
            
            float tot_dt = std::abs(_delay_in - _delay);
            int minstep = max(1, int(tot_dt/4));
            
            if(_delay < _delay_in)
            {   float target = next(_delay);
                while(target < _delay_in)
                {   float dt = target - _delay;
                    _filter.step(_buf[(_bufi + int(_delay)) % s], dt / tot_dt);
                    _delay = target;
                    target += minstep;
                }
            }
            else
            {   float target = prev(_delay);
                while(target > _delay_in)
                {   float dt = _delay - target;
                    _filter.step(_buf[(_bufi + int(target)) % s], dt / tot_dt);
                    _delay = target;
                    target -= minstep;
                }
            }
            float dt = tot_dt ? std::abs(_delay_in - _delay) / tot_dt : 1;
            float fb = _filter.step(_buf[(_bufi + int(_delay_in)) % s], dt);
            
            _delay = _delay_in;

            fb = wade::softsign(fb, 20);

            _buf[_bufi] = audio[i] * fx_send + fb * g;
            audio[i] = audio[i] * clean + fb;

            _bufi = (_bufi + s - 1) % s;
            _delay ++; // keep filter aligned
        }
    }
private:
    float * _controls;

    wade::arr<float, MaxSize> _buf;
    int _bufi = 0;

    va_cheby1<7, 3> _filter;
    float _delay = -1;
    float _delay_in = -1;

    wade::sinwave _lfo;
};


template<int MaxSize>
struct recorder
{
    static constexpr int num_controls() { return 0; }

    wade::arr<float, MaxSize> _buf;
    
    bool _record_pressed = false;
    bool _play_pressed = false;

    bool _record = false;
    bool _play = false;

    int _begin = 0;
    int _end = 1;
    int _playat = 0;

    int _recbegin = 0;
    int _recend = 0;

    recorder(float *)
    {
        _buf[0] = 0;
    }

    void record() // thread isolation flag
    {   _record_pressed = true;
    }

    void play() // thread isolation flag
    {   _play_pressed = true;
    }

    int loopstate()
    {   return 1 | (_play<<1) | (_record<<2);
    }

    void record_toggle()
    {   _record = !_record;
        if(_record)
        {   _recbegin = _end;
            _recend = _end;
        }
        else
        {   _begin = _recbegin;
            _end = _recend;
            _playat = _recbegin;
            _play = true;
        }
    }

    void render(float * buf, int len)
    {
        if(_record_pressed)
        {   record_toggle();
            _record_pressed = false;
        }
        if(_play_pressed)
        {   _play = !_play;
            _play_pressed = false;
        }
        for(int i=0 ; i<len ; i++)
        {   if(_play)
            {   buf[i] += _buf[_playat];
            }
            _playat ++;
            if(_playat == MaxSize) { _playat = 0; }
            if(_playat == _end) { _playat = _begin; }
            if(_record)
            {   _buf[_recend] = buf[i];
                _recend ++;
                if(_recend == MaxSize) { _recend = 0; }
                if(_recend == _recbegin) { record_toggle(); }
            }
        }


    }
};

} // wade