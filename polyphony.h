#pragma once

#include "arr.h"
#include "synth.h"

namespace wade {

// template<class MonoSynth, int MaxPoly = 16, int BufSize = 128>
// struct poly
// {   
//     static constexpr int num_controls() { return MonoSynth::num_controls(); }

//     struct VoiceState
//     {   uint16_t seq = 0;
//         uint8_t on = 0;
//         uint8_t note = 0;
//     };

//     wade::arr<MonoSynth, MaxPoly> _voices;
//     wade::arr<VoiceState, MaxPoly> _states;
//     wade::arr<float, BufSize> _audio_tmp;
//     uint16_t _poly = MaxPoly;
//     uint16_t _seq = 0;

//     template<class... Args>
//     poly(Args &&... args)
//     :   _voices(std::forward<Args>(args)...)
//     {
//     }

//     void set_polyphony(uint16_t poly)
//     {   _poly = poly;
//     }

//     void note_on(uint8_t note, uint8_t vel)
//     {   uint32_t bestscore = 0, besti = 0;
//         for(int i=0 ; i<_poly ; i++)
//         {   VoiceState & v = _states[i];
//             // prefer not on, then same note, then oldest seq
//             uint32_t score = (v.on == 0) << 17;
//             score += (v.note == note) << 16;
//             score += _seq - v.seq;
//             if(score > bestscore)
//             {   bestscore = score;
//                 besti = i;
//             }
//         }
//         _voices[besti].note_on(note, vel);
//         _states[besti].seq = _seq++;
//         _states[besti].note = note;
//         _states[besti].on = 1;
//     }
    
//     void note_off(uint8_t note, uint8_t vel)
//     {   for(int i=0 ; i<MaxPoly ; i++)
//         {   VoiceState & v = _states[i];
//             if(v.on == 1 && v.note == note)
//             {   _voices[i].note_off(note, vel);
//                 v.on = 0;
//                 break;
//             }
//             // TODO restore over-poly notes 
//         }
//     }

//     void note_pressure(uint8_t note, uint8_t press)
//     {   for(int i=0 ; i<MaxPoly ; i++)
//         {   VoiceState & v = _states[i];
//             if(v.on == 1 && v.note == note)
//             {   _voices[i].note_pressure(note, press);
//             }
//         }
//     }

//     void render(float * buf, int len)
//     {   wade_assert(len <= BufSize);
//         for(int i=0 ; i<len ; i++) { buf[i] = 0; }
//         for(int i=0 ; i<_poly ; i++)
//         {   _voices[i].render(_audio_tmp.data(), len);
//             for(int i=0 ; i<len ; i++) { buf[i] += _audio_tmp[i]; }
//         }
//     }
// };

template<class T, int N>
struct fifo
{
    T _data[N];
    int _read;
    int _write;
    int _size;
    
    void push(T x)
    {   _data[_write] = x;
        _write = (_write + 1) % N;
        _size += 1;
    }
    T pop()
    {   T x = _data[_read];
        _read = (_read + 1) % N;
        _size -= 1;
        return x;
    }
    int size() const { return _size; }
};




template<class MonoSynth, int MaxPoly = 16, int BufSize = 128>
struct poly
{   
    static constexpr int num_controls() { return MonoSynth::num_controls(); }
    
    static constexpr int stack_size() { return 32; }


    struct Note
    {   int8_t note;
        uint8_t vel;
        int voice; 
    };
    struct Voice
    {   MonoSynth synth;
        int note;
    };

    wade::arr<MonoSynth, MaxPoly> _voices;

    wade::fifo<int, MaxPoly> _off;
    // wade::fifo<int, MaxPoly> _on;

    wade::arr<Note, stack_size()> _notes;
    int _n_notes = 0;

    // struct Voice
    // {   MonoSynth synth;
    //     wade::arr<Note, stack_size()> notes;
    // };

    // wade::arr<int8_t, 128> _note_voice {-1};

    // int _next = 0;
    const int _poly = MaxPoly;

    wade::arr<float, BufSize> _audio_tmp;

    poly(MonoSynth synth)
    :   _voices(synth)
    {
        for(int i=0 ; i<MaxPoly ; i++)
        {   _off.push(i);
        }
    }

    // XXX: Too many edge cases for now
    // void set_polyphony(int poly)
    // {   
    //     int new_poly = wade::clip(poly, 1, MaxPoly);

    //     for(int vi=new_poly ; vi<_poly ; vi++)
    //     {   Voice & v = _voices[vi];
    //         if(v.notes[0].note >= 0)
    //         {   v.note_off(v.notes[0].note, 0);
    //         }
    //     }

    //     _poly = new_poly;
    //     _next = 0;
    // }

    void note_on(uint8_t note, uint8_t vel)
    {   
        int vi = -1;
        if(_n_notes < _poly) { vi = _off.pop(); }
        else
        { 
            vi = _notes[_n_notes-_poly].voice;
            _voices[vi].note_off(note, 0);
        }

        _notes[_n_notes] = Note{int8_t(note), vel, vi};
        _n_notes += 1;

        _voices[vi].note_on(note, vel);


        // int vi = _note_voice[note];
        // if(vi < 0)
        // {   vi = _next; 
        //     _next = (_next + 1) % _poly;
        //     _note_voice[note] = vi;
        // }

        // Voice & v = _voices[vi];

        // if(v.notes[0].note >= 0)
        // {   v.synth.note_off(v.notes[0].note, 0);
        // }
        // for(int i=1 ; i<v.notes.size() ; i++)
        // {   v.notes[i] = v.notes[i-1];
        // }
        // v.notes[0] = Note{int8_t(note), vel};
        // v.synth.note_on(note, vel);        
    }
    
    void note_off(uint8_t note, uint8_t vel)
    {   
        // find note
        int ni = -1;
        for(int i=_n_notes-1 ; i>=0 ; i--)
        {   if(_notes[i].note == note) { ni = i; }
        }
        if(ni < 0) { return; }
        // turn off if active
        if(ni + _poly >= _n_notes)
        {
            int vi = _notes[ni].voice;
            _voices[vi].note_off(note, vel);
            _off.push(vi);

        }
        // delete and shift back
        _n_notes -= 1;
        for(int i=ni ; i<_n_notes ; i++)
        {   _notes[i] = _notes[i+1];
        }
        // resume overbooked notes
        if(_n_notes >= _poly)
        {
            int ni = _n_notes-_poly;
            int vi = _off.pop();
            _notes[ni].voice = vi;
            _voices[vi].note_on(_notes[ni].note, _notes[ni].vel);
        }




        // int vi = _note_voice[note];
        // if(vi < 0) { return; }

        // int nni = -1, ncount = 0;
        // Voice & v = _voices[vi];
        // for(int ni=0 ; ni<stack_size() ; ni++)
        // {   if(v.notes[ni].note == note)
        //     {   ncount ++;
        //         nni = ni;
        //     }
        // }

        // if(nni >= 0)
        // {   if(nni == 0)
        //     {   v.synth.note_off(note, vel);
        //         if(v.notes[1].note >= 0)
        //         {   v.synth.note_on(v.notes[1].note, v.notes[1].vel);
        //         }
        //     }
        //     for(int ni=nni ; ni+1<stack_size() ; ni++)
        //     {   v.notes[ni] = v.notes[ni+1];
        //     }
        //     v.notes[stack_size()-1].note = -1;
        // }
        // if(ncount == 1)
        // {   _note_voice[note] = -1;
        // }
    }

    void note_pressure(uint8_t note, uint8_t press)
    {   
        // for(int vi=0 ; vi<_poly ; vi++)
        // {   Voice & v = _voices[vi];
        //     if(v.notes[0].note == note)
        //     {   v.synth.note_pressure(note, press);
        //     }
        // }
    }

    void render(float * buf, int len)
    {   wade_assert(len <= BufSize);
        for(int i=0 ; i<len ; i++) { buf[i] = 0; }
        for(int i=0 ; i<_poly ; i++)
        {   
            // _voices[i].synth.render(_audio_tmp.data(), len);
            _voices[i].render(_audio_tmp.data(), len);
            for(int i=0 ; i<len ; i++) { buf[i] += _audio_tmp[i]; }
        }
    }
};


// template<class Synth, int Tracks = 4>
// struct multitrack
// {

//     wade::arr<float, 16> _voice_ctl { 0.5f };
//     wade::arr<Synth, Tracks> _tracks;
//     int _active = 0;

//     template<class... Args>
//     multitrack(Args &&... args)
//     :   _tracks(std::forward<Args>(args)...)
//     {
//     }

// };


} // wade