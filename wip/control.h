#pragma once

namespace wade {

struct control
{
    float value = 0;
    float target = 0;
    float rate = 1;

    void aim(float target, int nsamples)
    {
        this->target = target;
        rate = std::abs(target - value) / nsamples;
    }
    float next()
    {
        // return value += rate;
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

template<int N>
struct has_controls
{
    control controls[N];
    static constexpr int num_controls() { return N; }
};

} // wade