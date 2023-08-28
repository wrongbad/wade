#pragma once

#include <cmath>

namespace fast {

// inline float exp(float x)
// {   union { float f; int32_t i; } u;
//     u.i = (12102203 * x + 1064866805);
//     return u.f;
// }

inline float exp(float x)
{
    // https://github.com/ekmett/approximate/blob/master/cbits/fast.c
    union { float f; int32_t i; } p, n;
    p.i = 1056478197 + 6051102 * x; // exp(x/2)
    n.i = 1056478197 - 6051102 * x; // exp(-x/2)
    return p.f / n.f;
}

inline float tanh(float x)
{
    // https://github.com/ekmett/approximate/blob/master/cbits/fast.c
    union { float f; int32_t i; } p, n;
    p.i = 1064866805 + 12102203 * x; // exp(x)
    n.i = 1064866805 - 12102203 * x; // exp(-x)
    return (p.f - n.f) / (p.f + n.f);
}


inline int floor(float x)
{   return int(x) - (x<0);
}

inline float cos2pi(float x)
{   x -= .25f + floor(x + .25f);
    x *= 16 * (std::abs(x) - .5f);
    x += .225f * x * (std::abs(x) - 1);
    return x;
}

inline float sin2pi(float x)
{   x -= .5f + floor(x);
    x *= 16 * (std::abs(x) - .5f);
    x += .225f * x * (std::abs(x) - 1);
    return x;
}

template<int Bits>
struct sin_lut
{   static constexpr int N = (1<<Bits);
    static constexpr int Shift = (30-Bits);
    float tab[N];
    sin_lut()
    {   for(int i=0 ; i<N ; i++)
        {   tab[i] = std::sin(i * M_PI / 2 / N);
        }
    }
    float sin(uint32_t t) const
    {   t += 1 << (Shift-1); // centered rounding
        if(t & (1<<31)) { return -sin(t ^ (1<<31)); }
        if(t == (1<<30)) { return 1; }
        if(t & (1<<30)) { t = (1<<31) - t; }
        return tab[t >> Shift];
    }
};

} // fast

namespace wade {

template<class T> T min(T const& a, T const& b)
{   return a < b ? a : b;
}

template<class T> T max(T const& a, T const& b)
{   return a > b ? a : b;
}

inline int32_t modulo(int32_t x, int32_t mod)
{   int32_t r = x % mod;
    return r + r<0 ? mod : 0;
}

template<class T> int sign(T val)
{   return (T(0) < val) - (val < T(0));
}

inline float softsign(float x, float clip = 1)
{   return x / (1 + std::abs(x / clip));
}

template<class T>
float clip(T x, T min = -1, T max = 1)
{   if(x > max) { x = max; }
    if(x < min) { x = min; }
    return x;
}

inline float cubic_sigmoid(float x)
{   if(x > 1) { return 2/3.0f; }
    if(x < -1) { return -2/3.0f; }
    return x - x * x * x / 3.0f;
}

struct rng32
{   uint32_t x = ~uint32_t(0);
    uint32_t operator()()
    {   x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        return x;
    }
};

struct rng64
{   uint64_t x = ~uint64_t(0);
    uint64_t operator()()
    {   x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        return x;
    }
};

struct sinwave
{   float freq = 0;
    double _p = 0;
    sinwave(float f = 0) : freq(f) {}
    float operator()()
    {   _p += freq;
        _p -= (_p > 1);
        return fast::sin2pi(_p);
    }
};

struct slew
{   float rate = 0;
    float _y = 0;
    float operator()(float target)
    {   if(_y < target)
        {   if(_y + rate > target) { return _y = target; }
            return _y += rate;
        }
        else
        {   if(_y - rate < target) { return _y = target; }
            return _y -= rate;
        }
    }
};

} // wade