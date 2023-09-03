#pragma once

namespace wade {

// inline float sample_rate(float set = 0)
// {
//     static float _sr = 44100;
//     if(set) { _sr = set; }
//     return _sr;
// }

// inline float tune_a4(float set = 0)
// {
//     static float _a4 = 440 / sample_rate();
//     if(set) { _a4 = set / sample_rate(); }
//     return _a4;
// }


// struct context 
// {
//     float const sample_rate = 44100;
//     float const tune_a4 = 440;
//     float const midi_semi = std::log(2.0) / 12;
//     float const midi_0 = tune_a4 / std::exp(69 * midi_semi) / sample_rate;

//     float midi_freq(uint8_t note) const
//     {   
//         return midi_0 * std::exp(note * midi_semi);
//     }
// };


} // wade