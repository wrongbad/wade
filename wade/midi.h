#pragma once

#include <cmath>

namespace wade {

// returns cycles/sample units
inline float midi_hz(float note, float tune_a4 = 440)
{
    static const float midi_semi = std::logf(2) / 12;
    return tune_a4 * std::expf((note-69) * midi_semi);
}

// inline float midi_freq(uint8_t note)
// {   
//     static const float b = std::log(2.0f) / 12.0f; // +12 notes = 2x freq
//     static const float a = tune_a4() / std::exp(69 * b) / sample_rate();
//     return a * std::exp(note * b);
// }


// template<class Delegate>
// struct midi_parser
// {
//     Delegate & _delegate;

//     static constexpr int note_off = 0x80;
//     static constexpr int note_on = 0x90;
//     static constexpr int note_atouch = 0xA0;
//     static constexpr int set_ctl = 0xB0;

//     void operator()(uint8_t const* msg, int len)
//     {
//         uint8_t cmd = msg[0] & 0xF0;
//         uint8_t chan = msg[0] & 0x0F;
//         // std::cout << (cmd>>4) << std::endl;
//         switch(cmd)
//         {   case note_on:
//                 if(msg[2] != 0) 
//                 {   _delegate.note_on(msg[1], msg[2], chan);
//                     break;
//                 } // else fall-through
//             case note_off:
//                 _delegate.note_off(msg[1], msg[2], chan);
//                 break;
//             case note_atouch:
//                 _delegate.note_pressure(msg[1], msg[2], chan);
//                 break;
//             case set_ctl:
//                 _delegate.control(msg[1], msg[2], chan);
//                 break;
//         }
//     }
// };


enum midi_command {
    NOTE_OFF = 0x80,
    NOTE_ON = 0x90,
    NOTE_PRESSURE = 0xA0,
    SET_CONTROL = 0xB0,
    SET_PROGRAM = 0xC0,
    MONO_PRESSURE = 0xD0,
    PITCH_BEND = 0xE0,
    SYSTEM = 0xF0,
};

struct voice_control
{
    float note = 0;
    float gate = 0;
    float velocity = 0;
    float pitch_bend = 0;
    float pressure = 0;
    float controls[128] = {0}; // TODO optimize sparse array
};

struct mpe_handler
{
    voice_control voices[16] = {};

    void operator()(uint8_t const* msg, size_t len, double ts = 0)
    {
        if(len == 0) { return; }

        uint8_t cmd = msg[0] & 0xF0;
        uint8_t chan = msg[0] & 0x0F;

        voice_control & voice = voices[chan];

        switch(cmd)
        {   
            case NOTE_ON:
                if(msg[2] != 0) 
                {   
                    voice.note = msg[1];
                    voice.gate = 1;
                    voice.velocity = msg[2] / 127.0f;
                    break;
                } // else fall through
            case NOTE_OFF:
                voice.gate = 0;
                voice.velocity = msg[2] / 127.0f;
                break;
            case SET_CONTROL:
                voice.controls[msg[1]] = msg[2] / 127.0;
                break;
            case MONO_PRESSURE:
                voice.pressure = msg[1] / 127.0f;
                break;
            case PITCH_BEND:
                int v = (msg[2]<<7) + msg[1] - (64<<7);
                voice.pitch_bend = v / float(1<<7);
                break;
        }
    }
};

} // wade