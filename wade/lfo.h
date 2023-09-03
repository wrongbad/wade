#pragma once

#include "wade/math.h"

namespace wade {
    
struct sinwave
{   
    float freq = 0;
    double phase = 0;
    
    float operator()()
    {
        phase += freq - (phase > 1);
        return fast::sin2pi(phase);
    }
};

struct slew
{
    float rate = 0;
    float value = 0;

    float operator()(float target)
    {   
        if(value < target)
        {   if(value + rate > target) { return value = target; }
            return value += rate;
        }
        else
        {   if(value - rate < target) { return value = target; }
            return value -= rate;
        }
    }
    operator float() const { return value; }
};

} // wade