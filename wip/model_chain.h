#pragma once

#include "finnwave.h"
#include "simple_flute.h"
#include "filter.h"
#include "midi.h"

namespace wade {

class model_chain
{
public:
    model_chain(float const* controls)
    :   _controls(controls),
        _wave0(controls),
        _wave1(controls+6)
    {   
    }
    void note_on(uint8_t note, uint8_t vel)
    {   _wave0.note_on(note, vel);
        _wave1.note_on(note, vel);
    }
    void note_off(uint8_t note, uint8_t vel)
    {   _wave0.note_off(note, vel);
        _wave1.note_off(note, vel);
    }
    void note_pressure(uint8_t note, uint8_t press)
    {   _wave0.note_pressure(note, press);
        _wave1.note_pressure(note, press);
    }
    void render(float * buf, int len)
    {   for(int i=0 ; i<len ; i++) { buf[i] = 0; }
        _wave0.render(buf, len);
        _wave1.render(buf, len);

        // TODO move to fx_chain
        for(int i=0 ; i<len ; i++) { buf[i] *= _controls[36]; }
    }

private:
    float const* _controls;

    finnwave _wave0;
    simple_flute _wave1;


};

} // wade