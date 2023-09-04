#pragma once

#include "util.h"
#include <new>
#include <utility>

namespace wade {

// TODO just use vector allocator



// // helper to allocate T but not initialize
// template<class T>
// union arr_slot
// {   T v;
//     arr_slot() {}
//     ~arr_slot() {}
// };

// // array with ctor arg forwarding
// template<class T, int N>
// class arr
// {
// public:
//     arr(T proto) noexcept
//     {   for(int i=0 ; i<N ; i++)
//         {   new (data(i)) T(proto);
//         }
//     }
//     arr() noexcept : arr(T()) {}
//     arr(arr const& o)
//     {   for(int i=0 ; i<N ; i++)
//         {   new (data(i)) T(o[i]);
//         }
//     }
//     arr(arr && o)
//     {   for(int i=0 ; i<N ; i++)
//         {   new (data(i)) T(o[i]);
//         }
//     }
//     ~arr() noexcept
//     {   for(int i=0 ; i<N ; i++)
//         {   data(i)->~T();
//         }
//         wade::free(_data);
//     }
//     int size() const { return N; }

//     T * begin() { return data(0); }
//     T const* begin() const { return data(0); }
//     T * end() { return data(N); }
//     T const* end() const { return data(N); }
    
//     T & operator[](int i) { return *data(i); }
//     T const& operator[](int i) const { return *data(i); }

//     T * data(int i = 0)
//     {   wade_assert(i >= 0 && i < N);
//         return &_data[i].v;
//     }
//     T const* data(int i = 0) const
//     {   wade_assert(i >= 0 && i < N);
//         return &_data[i].v;
//     }
// private:
//     arr_slot<T> * _data = (arr_slot<T> *)wade::malloc(sizeof(T) * N);
// };


// template<class T, int N>
// class ring : public arr<T, N>
// {
// public:
//     using arr<T, N>::arr;
//     using arr<T, N>::size;
//     using arr<T, N>::data;

//     ring(ring const& o) = default;

//     T & operator[](int i) { return *data((_zero + i) % N); }
//     T const& operator[](int i) const { return *data((_zero + i) % N); }
    
//     void shift(int i)
//     {   _zero = (_zero + size() - i) % size();
//     }
// private:
//     int _zero = 0;
// };


// template<class T>
// struct span_t
// {
//     T * _data;
//     int _size;

//     int size() const { return _size; }
    
//     T * data() { return _data; }
//     T const* data() const { return _data; }

//     T * begin() { return _data; }
//     T const* begin() const { return _data; }
    
//     T * end() { return _data + _size; }
//     T const* end() const { return _data + _size; }

//     T & operator[](int i) { return _data[i]; }
//     T const& operator[](int i) const { return _data[i]; }
// };

// template<class T>
// span_t<T> span(T * data, int sz) { return {data, sz}; }


struct audio_context
{
    int const sample_rate;
    bool abort;
};

struct mono
{
    float * data;
    int size;

    float operator[](int i) const { return data[i]; }
    float * begin() const { return data; }
    float * end() const { return data + size; }

    mono & fill(float v = 0)
    {
        for(int i=0 ; i<size ; i++) { data[i] = v; }
        return *this;
    }

    mono & operator=(mono o)
    {
        int n = std::min(size, o.size);
        for(int i=0 ; i<n ; i++) { data[i] = o.data[i]; }
        return *this;
    }

    mono & operator+=(mono o)
    {
        int n = std::min(size, o.size);
        for(int i=0 ; i<n ; i++) { data[i] += o.data[i]; }
        return *this;
    }

    template<class Func>
    mono & apply(Func && func)
    {
        for(float & x : *this) { x = func(x); }
        return *this;
    }
};

struct audio
{
    float * data; // non-interleaved chanels
    int channels;
    int frames;

    int size() const { return channels * frames; }
    mono channel(int c) const { return {data + c*frames, frames}; }
    mono operator[](int c) const { return channel(c); }
    mono flatten() const { return {data, size()}; }

    audio & fill(float v = 0)
    {
        for(int i=0 ; i<size() ; i++) { data[i] = v; }
        return *this;
    }

    struct Iter
    {
        audio const* base;
        int chan;

        mono operator*() const { return base->channel(chan); }
        Iter & operator++() { chan++; return *this; }
        bool operator!=(Iter const& o) const { return base!=o.base || chan!=o.chan; }
    };

    Iter begin() const { return {this, 0}; }
    Iter end() const { return {this, channels}; }
};

void deinterleave(float const* in, int channels, int frames, float * out)
{
    // TODO in-place would be cool, but tricky to implement
    for(int f=0 ; f<frames ; f++)
    {
        for(int c=0 ; c<channels ; c++)
        {
            int i = c + f * channels;
            int d = c * frames + f;
            out[d] = in[i];
        }
    }
}

void interleave(float const* in, int channels, int frames, float * out)
{
    // TODO in-place would be cool, but tricky to implement
    for(int f=0 ; f<frames ; f++)
    {
        for(int c=0 ; c<channels ; c++)
        {
            int i = c + f * channels;
            int d = c * frames + f;
            out[d] = in[i];
        }
    }
}

struct vector_buffer : std::vector<float>
{
    // override initializer_list ctor
    // vector_buffer a {6}; // will have size 6
    vector_buffer(size_t size)
    : std::vector<float>(size, 0.0f) {}
};


} // wade