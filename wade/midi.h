#pragma once

#include <cmath>
#include <array>

namespace wade {

namespace detail {

template<class T, int B, int E>
constexpr std::array<T, E-B> arange()
{
    std::array<T, E-B> out;
    for(int i=B ; i<E ; i++) { out[i-B] = i; }
    return out;
}

} // detail 


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
    int voice_map = -1;
    float note = 0;
    float gate = 0;
    float velocity = 0;
    float pitch_bend = 0;
    float pressure = 0;
    float controls[128] = {0}; // TODO optimize sparse array
};

struct mpe_handler
{
    static constexpr int N = 16;
    voice_control voices[N];
    std::array<int,N> order = detail::arange<int,0,N>();
    int polyphony_mapped = 0;

    template<class Voices, class Func>
    void map(Voices && vs, int poly, Func && func)
    {
        if(poly > N) { poly = N; }
        map_voices(poly);
        for(int i=0 ; i<poly ; i++)
        {
            voice_control const& ctl = (*this)[i];
            assert(ctl.voice_map < poly);
            func(vs[ctl.voice_map], ctl);
        }
    }

    voice_control const& operator[](int i) const
    {
        return voices[order[i]];
    }
    int size() const { return N; }

    void map_voices(int polyphony)
    {
        if(polyphony > N) { polyphony = N; }
        if(polyphony_mapped == polyphony) { return; }
        bool taken[N] = {false};

        for(int i=0 ; i<polyphony ; i++)
        {
            int & map = voices[order[i]].voice_map;
            if(map >= 0)
            { 
                if(taken[map] || map >= polyphony) { map = -1; }
                else { taken[map] = true; }
            }
        }
        for(int i=0 ; i<polyphony ; i++)
        {
            int & map = voices[order[i]].voice_map;
            if(map < 0)
            {
                while(taken[++map]) {} // find slot
            }
            taken[map] = true;
        }
        polyphony_mapped = polyphony;
    }

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
                    move_to_front(chan);
                    break;
                } // else fall through
            case NOTE_OFF:
                voice.gate = 0;
                voice.velocity = msg[2] / 127.0f;
                move_to_back(chan);
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
    void move_to_front(int chan)
    {
        for(int i=N-1 ; i>0 ; i--)
            if(order[i] == chan)
                std::swap(order[i-1], order[i]);
        polyphony_mapped = 0;
    }
    void move_to_back(int chan)
    {
        for(int i=0 ; i<N-1 ; i++)
            if(order[i] == chan)
                std::swap(order[i], order[i+1]);
        polyphony_mapped = 0;
    }
};


template<class Voice, int N>
struct Polyphony
{
    Voice voices[N];

    template<class ControlBank>
    void render(ControlBank && controls, int poly)
    {
        if(poly > N) { poly = N; }
        controls.map_voices(poly);



        // using Control = std::remove_refernce_t<decltype(controls[0])>;
        // Control * sorted[N];
        // for(int i=0 ; i<N ; i++)
        // {
        //     sorted[i] = &controls[i];
        // }
        // for(int i=0 ; i<N ; i++)
        // {   
        //     int aff = sorted[i]->voice_affinity;
        //     if(aff >= 0 && aff != i) std::swap(sorted[i], sorted[aff])
        // }
    }
};

} // wade