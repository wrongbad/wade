#pragma once

#include "util.h"

namespace wade {

// struct param
// {   float def = 0.5;
//     float * ptr = nullptr;

//     param(float d) : def(d) {}
//     param & operator=(float * p) { ptr = p; return *this; }
//     float & operator*() { return ptr ? *ptr : def; }
//     float const& operator*() const { return ptr ? *ptr : def; }
// };


inline float midi_freq(uint8_t note)
{   static const float b = std::log(2.0f) / 12.0f; // +12 notes = 2x freq
    static const float a = tune_a4() / std::exp(69 * b) / sample_rate();
    return a * std::exp(note * b);
}

// inline float midi_freq(byte note)
// {   static const float semi[12] = {
//         std::pow(2.0f, 0 / 12.0f),
//         std::pow(2.0f, 1 / 12.0f),
//         std::pow(2.0f, 2 / 12.0f),
//         std::pow(2.0f, 3 / 12.0f),
//         std::pow(2.0f, 4 / 12.0f),
//         std::pow(2.0f, 5 / 12.0f),
//         std::pow(2.0f, 6 / 12.0f),
//         std::pow(2.0f, 7 / 12.0f),
//         std::pow(2.0f, 8 / 12.0f),
//         std::pow(2.0f, 9 / 12.0f),
//         std::pow(2.0f, 10 / 12.0f),
//         std::pow(2.0f, 11 / 12.0f),
//     };
//     static const float tune_note0 = tune_a4() / std::pow(2.0f, 69 / 12.0f);
//     return tune_note0 * semi[note%12] * (1 << note/12);
// }

template<class Delegate>
struct midi_parser
{
    Delegate & _delegate;

    static constexpr int note_off = 0x80;
    static constexpr int note_on = 0x90;
    static constexpr int note_atouch = 0xA0;
    static constexpr int set_ctl = 0xB0;

    void operator()(uint8_t const* msg, int len)
    {
        uint8_t cmd = msg[0] & 0xF0;
        uint8_t chan = msg[0] & 0x0F;
        // std::cout << (cmd>>4) << std::endl;
        switch(cmd)
        {   case note_on:
                if(msg[2] != 0) 
                {   _delegate.note_on(msg[1], msg[2], chan);
                    break;
                } // else fall-through
            case note_off:
                _delegate.note_off(msg[1], msg[2], chan);
                break;
            case note_atouch:
                _delegate.note_pressure(msg[1], msg[2], chan);
                break;
            case set_ctl:
                _delegate.control(msg[1], msg[2], chan);
                break;
        }
    }
};

} // wade