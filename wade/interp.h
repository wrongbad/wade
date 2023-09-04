#pragma once

namespace wade {

struct allpass_interp
{   
    float _g = 0;
    float _x = 0;
    float _y = 0;

    void set_delay(float delay)
    {   
        _g = (1-delay) / (1+delay);
    }

    float operator()(float x)
    {   
        float y = _g * (x - _y) + _x;
        _x = x; 
        _y = y;
        return y;
    }
};

struct variable_resampler
{
    analog_cheby1_lowpass<5, 3> filter;
    bool printed = false;

    template<class Audio>
    float operator()(Audio && in, float from, float to)
    {
        // dt > 1 : lowpass, dt <= 1 : interpolator
        float scale = std::min(1.0f, 1 / std::abs(to-from));
        int ifrom = fast::floor(from);
        int ito = fast::floor(to);
        while(ifrom < ito)
        {
            float step = ifrom + 1 - from;
            if(step) filter.step(in[ifrom], step * scale);
            from = ++ifrom;
        }
        while(ifrom > ito)
        {
            float step = from - ifrom;
            if(step) filter.step(in[ifrom], step * scale);
            from = ifrom--;
        }
        float step = std::abs(to - from);
        printed = true;
        return filter.step(in[ifrom], step * scale);
    }
};

} // wade