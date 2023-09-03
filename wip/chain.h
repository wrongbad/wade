#pragma once

#include "synth.h"

namespace wade {

template<class... Models>
class chain : public synth
{
public:
    model_chain(float const* controls)
    :   _controls(controls),
        _wave0(controls),
        simple_flute(controls+6)
    {   
    }
    void note_on(uint8_t note, uint8_t vel)
    {   
    }
    void note_off(uint8_t note, uint8_t vel)
    {   
    }
    void note_pressure(uint8_t note, uint8_t press)
    {   
    }
    void render(float * buf, int len)
    {   for(int i=0 ; i<len ; i++) { buf[i] = 0; }
        _wave0.render(buf, len);
        _wave1.render(buf, len);
    }

private:
    float const* _controls;
};

} // wade