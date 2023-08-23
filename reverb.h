#pragma once

#include "delay.h"

namespace wade {

template<int MaxSize>
struct comb_feedback : public delay<MaxSize>
{   float _fb = 0.7;
    float _y = 0;
    comb_feedback(int delay, float fb) : _fb(fb)
    {   this->set_delay(delay);
    }
    float operator()(float x, float boost = 0)
    {   float fb = _fb + (1 - _fb) * boost;
        x += _y * fb;
        _y = delay<MaxSize>::operator()(x);
        return _y;
    }
};

template<int MaxSize>
struct ap_feedback : public delay<MaxSize>
{   float _fb = 0.7;
    float _y = 0;
    ap_feedback(int delay, float fb) : _fb(fb)
    {   this->set_delay(delay);
    }
    float operator()(float x, float boost = 0)
    {   float fb = _fb + (1 - _fb) * boost;
        x += _y * fb;
        _y = delay<MaxSize>::operator()(x);
        return _y - x * fb;
    }
};

struct reverb
{
    float _boost;
    ap_feedback<512> _ap0 {347, 0.7};
    ap_feedback<128> _ap1 {113, 0.7};
    ap_feedback<64> _ap2 {37, 0.7};
    ap_feedback<2560> _comb[4] {
        {1687, 0.773}, {1601, 0.802}, 
        {2053, 0.753}, {2251, 0.733}
    };

    reverb(float boost = 0) : _boost(boost) {}

    void set_boost(float boost) { _boost = boost; }

    float operator()(float x)
    {   x = _ap0(x);
        x = _ap1(x);
        x = _ap2(x);
        float y = 0;
        for(int i=0 ; i<4 ; i++)
        {   y += _comb[i](x, _boost);
        }
        return y;
    }
};

struct reverb2
{
    float _boost;
    ap_feedback<2048> _ap0 {1051, 0.7};
    ap_feedback<512> _ap1 {337, 0.7};
    ap_feedback<128> _ap2 {113, 0.7};
    ap_feedback<8192> _comb[4] {
        {4799, 0.742}, {4999, 0.733}, 
        {5399, 0.715}, {5801, 0.697}
    };

    reverb2(float boost = 0) : _boost(boost) {}

    void set_scale(float boost) { _boost = boost; }

    float operator()(float x)
    {   x = _ap0(x);
        x = _ap1(x);
        x = _ap2(x);
        float y = 0;
        for(int i=0 ; i<4 ; i++)
        {   y += _comb[i](x, _boost);
        }
        return y;
    }
};

} // wade