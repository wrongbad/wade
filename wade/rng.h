#pragma once

namespace wade {

struct rng32
{   
    uint32_t x = ~uint32_t(0);
    uint32_t operator()()
    {   
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        return x;
    }
};

struct rng64
{   
    uint64_t x = ~uint64_t(0);
    uint64_t operator()()
    {   
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        return x;
    }
};

} // wade