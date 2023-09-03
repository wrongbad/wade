#pragma once

#include "wade/delay.h"

namespace wade {

template<class BufferType = wade::vector_buffer>
struct comb_feedback
{
    basic_delay<BufferType> delay;
    float gain = 0.7;
    float _y = 0;

    float operator()(float x)
    {   
        x += _y * gain;
        _y = delay(x);
        return _y;
    }
};

template<class BufferType = wade::vector_buffer>
struct allpass_feedback
{
    basic_delay<BufferType> delay;
    float gain = 0.7;
    float _y = 0;

    float operator()(float x)
    {
        x -= _y * gain;
        _y = delay(x);
        return _y + x * gain;
    }
};

template<class BufferType = wade::vector_buffer>
struct basic_reverb
{
    // {{buffer_size, delay}, feedback}
    allpass_feedback<BufferType> allpass[3] {
        {{384, 347}, 0.7},
        {{128, 113}, 0.7},
        {{64, 37}, 0.7},
    };
    allpass_feedback<BufferType> combs[4] {
        {{2048, 1687}, 0.773}, 
        {{2048, 1601}, 0.802}, 
        {{2560, 2053}, 0.753},
        {{2560, 2251}, 0.733},
    };

    void set_rt60(float rt60_samples)
    {
        float e = std::log(1e-6) / rt60_samples;
        for(auto & filter : combs)
            filter.gain = std::exp(filter.delay.delay * e);
    }

    float operator()(float x)
    {   
        float out = 0;
        for(auto & filter : allpass) { x = filter(x); }
        for(auto & filter : combs) { out += filter(x); }
        return out;
    }
};

template<class BufferType = wade::vector_buffer>
struct basic_reverb2 : basic_reverb<BufferType>
{
    basic_reverb2() : basic_reverb<BufferType> {
        {
            {{1280, 1051}, 0.7},
            {{512, 337}, 0.7},
            {{128, 113}, 0.7},
        },
        {
            {{5120, 4799}, 0.742},
            {{5120, 4999}, 0.733},
            {{6144, 5399}, 0.715},
            {{6144, 5801}, 0.697},
        }
    } 
    {}
};

} // wade