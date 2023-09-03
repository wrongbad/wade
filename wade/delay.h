#pragma once

#include "wade/math.h"
#include "wade/audio.h"
#include "wade/interp.h"

namespace wade {

// BufferType concept:
// BufferType::BufferType(size_t)
// size_t BufferType::size() const
// float & BufferType::operator[](size_t)

template<class BufferType = wade::vector_buffer>
struct basic_delay
{   
    BufferType buffer {16384};
    int delay = buffer.size() - 1;
    int head = 0;

    void clear()
    {
        for(size_t i=0 ; i<buffer.size() ; i++)
            buffer[i] = 0;
    }

    basic_delay & push(float x)
    {
        head = (head + buffer.size() - 1) % buffer.size();
        buffer[head] = x;
        return *this;
    }

    float & operator[](int offset)
    {
        return buffer[(head + offset) % buffer.size()];
    }

    float operator()(float x)
    {
        return push(x)[delay];
    }
};


template<class BufferType = wade::vector_buffer>
struct interp_delay
{   
    basic_delay<BufferType> delay;
    allpass_interp interp;

    void clear() { delay.clear(); }

    void set_delay(float fsamples)
    {   
        fsamples = clip(fsamples, 1, delay.buffer.size());
        delay.delay = int(fsamples - 0.5); // 0.5 <= frac < 1.5
        interp.set_delay(fsamples - delay.delay);
    }

    float operator()(float x)
    {   
        return interp(delay(x));
    }
};


// template<int MaxSize>
// struct wave_delay : public interp_delay<MaxSize>
// {   
//     float _reflect = 1;
//     using interp_delay<MaxSize>::_buf;
//     using interp_delay<MaxSize>::_idelay;
    
//     void set_reflection(float reflect)
//     {   _reflect = reflect;
//     }

//     float at(int offset)
//     {   return _buf[offset] + _buf[_idelay - offset];
//     }

//     void insert(int offset, float x)
//     {   _buf[offset] += x / 2;
//         _buf[_idelay - offset] += x / 2;
//     }

//     void insert(int offset, float forward, float reverse)
//     {   _buf[offset] += forward;
//         _buf[_idelay - offset] += reverse;
//     }

//     float & mid() { return _buf[_idelay/2]; }

//     float operator()(float x)
//     {   mid() *= _reflect;
//         return interp_delay<MaxSize>::operator()(x);
//     }
// };


template<class BufferType = wade::vector_buffer>
struct tape_delay
{
    basic_delay<BufferType> tape {65536};
    float delay = 10000; // input (command) delay
    float feedback = 0;
    
    variable_resampler _interp;
    float _delay = -1; // actual cursor
    float _last_out = 0;

    float operator()(float x)
    {   
        if(_delay < 0) { _delay = delay; }

        tape.push(x + feedback * _last_out);
        // tape.push(x);
        
        float from = _delay + 1; // +1 because tape.push()
        _delay += (delay - _delay) * 0.0006;

        _last_out = _interp(tape, from, _delay);
        // _last_out = _interp.filter.step(tape[_delay], 1);
        // _last_out = tape[_delay];
        _last_out = wade::softsign(_last_out / 20) * 20;
        return _last_out;
    }
};


// template<int MaxSize>
// struct recorder
// {
//     static constexpr int num_controls() { return 0; }

//     wade::arr<float, MaxSize> _buf;
    
//     bool _record_pressed = false;
//     bool _play_pressed = false;

//     bool _record = false;
//     bool _play = false;

//     int _begin = 0;
//     int _end = 1;
//     int _playat = 0;

//     int _recbegin = 0;
//     int _recend = 0;

//     recorder(float *)
//     {
//         _buf[0] = 0;
//     }

//     void record() // thread isolation flag
//     {   _record_pressed = true;
//     }

//     void play() // thread isolation flag
//     {   _play_pressed = true;
//     }

//     int loopstate()
//     {   return 1 | (_play<<1) | (_record<<2);
//     }

//     void record_toggle()
//     {   _record = !_record;
//         if(_record)
//         {   _recbegin = _end;
//             _recend = _end;
//         }
//         else
//         {   _begin = _recbegin;
//             _end = _recend;
//             _playat = _recbegin;
//             _play = true;
//         }
//     }

//     void render(float * buf, int len)
//     {
//         if(_record_pressed)
//         {   record_toggle();
//             _record_pressed = false;
//         }
//         if(_play_pressed)
//         {   _play = !_play;
//             _play_pressed = false;
//         }
//         for(int i=0 ; i<len ; i++)
//         {   if(_play)
//             {   buf[i] += _buf[_playat];
//             }
//             _playat ++;
//             if(_playat == MaxSize) { _playat = 0; }
//             if(_playat == _end) { _playat = _begin; }
//             if(_record)
//             {   _buf[_recend] = buf[i];
//                 _recend ++;
//                 if(_recend == MaxSize) { _recend = 0; }
//                 if(_recend == _recbegin) { record_toggle(); }
//             }
//         }


//     }
// };

} // wade