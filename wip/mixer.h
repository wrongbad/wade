#pragma once

#include "synth.h"

namespace wade {

template<int Channels, int BufSize = 128>
struct mixer : public synth<>
{
    synth<> * _channels[Channels];
    arr<float, Channels + 1> _vol { 0.7 };
    arr<float, BufSize> _buf { 0 };

    template<class... Args>
    mixer(Args &&... args)
    :   _channels{ std::forward<Args>(args)... }
    {
        _vol[Channels] = 1;
    }

    int num_controls() override { return Channels + 1; };
    float * controls() override { return _vol.data(); }
    
    void render(float * buf, int len)
    {   for(int i=0 ; i<Channels ; i++)
        {   _channels[i]->render(_buf.data(), len);
            float v = _vol[i] * _vol[Channels];
            for(int j=0 ; j<len ; j++)
            {   buf[j] += v * _buf[j];
            }
        }
    }
};

} // wade