#pragma once

#include <cmath>

// some std math functions are re-implemented here
// with less precision and less edge-case handling

namespace wade {
namespace fast {

// https://github.com/ekmett/approximate/blob/master/cbits/fast.c
inline float exp(float x)
{
    union { float f; int32_t i; } p, n;
    p.i = 1056478197 + 6051102 * x; // exp(x/2)
    n.i = 1056478197 - 6051102 * x; // exp(-x/2)
    return p.f / n.f;
}

// https://github.com/ekmett/approximate/blob/master/cbits/fast.c
inline float tanh(float x)
{
    union { float f; int32_t i; } p, n;
    p.i = 1064866805 + 12102203 * x; // exp(x)
    n.i = 1064866805 - 12102203 * x; // exp(-x)
    return (p.f - n.f) / (p.f + n.f);
}

inline int floor(float x)
{   
    return int(x) - (x<0);
}

inline float cos2pi(float x)
{   
    x -= .25f + floor(x + .25f);
    x *= 16 * (std::abs(x) - .5f);
    x += .225f * x * (std::abs(x) - 1);
    return x;
}

inline float sin2pi(float x)
{   
    x -= .5f + floor(x);
    x *= 16 * (std::abs(x) - .5f);
    x += .225f * x * (std::abs(x) - 1);
    return x;
}

template<int Bits>
struct sin_quarter_lut
{   
    static constexpr int N = (1<<Bits);
    static constexpr int Shift = (30-Bits);
    static constexpr int m31 = (1u<<31)-1;
    float tab[N];
    sin_quarter_lut()
    {   
        for(int i=0 ; i<N ; i++)
        {   
            tab[i] = std::sin((i + 0.5) * M_PI / 2 / N);
        }
    }
    // period of 2^32
    float sin(uint32_t t) const
    {   
        if(t & (1u<<30)) { t ^= m31; }
        if(t <= m31) { return tab[t >> Shift]; }
        return -tab[(t & m31) >> Shift];
    }
};

template<int Bits>
struct sin_full_lut
{   
    static constexpr int N = (1<<Bits);
    static constexpr int Shift = (32-Bits);
    float tab[N];
    sin_full_lut()
    {   
        for(int i=0 ; i<N ; i++)
        {   
            tab[i] = std::sin((i + 0.5) * M_PI * 2 / N);
        }
    }
    // period of 2^32
    float sin(uint32_t t) const
    {   
        return tab[t >> Shift];
    }
};

} // fast

// // std::min is missing on Arduino
// template<class T> T min(T const& a, T const& b)
// {   
//     return a < b ? a : b;
// }
// // std::max is missing on Arduino
// template<class T> T max(T const& a, T const& b)
// {   
//     return a > b ? a : b;
// }

// positive mod operator (python style)
template<class T> T modulo(T x, T mod)
{   
    return x < 0 ? (x % mod + mod) : (x % mod);
}

template<class T> int sign(T val)
{   
    return (T(0) < val) - (val < T(0));
}

template<class T> T clip(T x, float min = -1, float max = 1)
{   
    if(x >= max) { return max; }
    if(x >= min) { return x; }
    return min;
}

// simple soft-clipping function (-1 < output < 1)
template<class T> T softsign(T x)
{   
    return x / (1 + std::abs(x));
}

// cubic soft-clipping function (-1 < output < 1)
template<class T> T cubic_tanh(T x)
{   
    if(x >= 1) { return 1; }
    if(x > -1) { return (x * 3 - x * x * x) / 2; }
    return -1;
}

} // wade