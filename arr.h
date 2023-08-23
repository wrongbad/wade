#pragma once

#include "util.h"
#include <new>
#include <utility>

namespace wade {

// helper to allocate T but not initialize
template<class T>
union arr_slot
{   T v;
    arr_slot() {}
    ~arr_slot() {}
};

// array with ctor arg forwarding
template<class T, int N>
class arr
{
public:
    arr(T proto) noexcept
    {   for(int i=0 ; i<N ; i++)
        {   new (data(i)) T(proto);
        }
    }
    arr() noexcept : arr(T()) {}
    arr(arr const& o)
    {   for(int i=0 ; i<N ; i++)
        {   new (data(i)) T(o[i]);
        }
    }
    arr(arr && o)
    {   for(int i=0 ; i<N ; i++)
        {   new (data(i)) T(o[i]);
        }
    }
    ~arr() noexcept
    {   for(int i=0 ; i<N ; i++)
        {   data(i)->~T();
        }
        wade::free(_data);
    }
    int size() const { return N; }

    T * begin() { return data(0); }
    T const* begin() const { return data(0); }
    T * end() { return data(N); }
    T const* end() const { return data(N); }
    
    T & operator[](int i) { return *data(i); }
    T const& operator[](int i) const { return *data(i); }

    T * data(int i = 0)
    {   wade_assert(i >= 0 && i < N);
        return &_data[i].v;
    }
    T const* data(int i = 0) const
    {   wade_assert(i >= 0 && i < N);
        return &_data[i].v;
    }
private:
    arr_slot<T> * _data = (arr_slot<T> *)wade::malloc(sizeof(T) * N);
};


template<class T, int N>
class ring : public arr<T, N>
{
public:
    using arr<T, N>::arr;
    using arr<T, N>::size;
    using arr<T, N>::data;

    ring(ring const& o) = default;

    T & operator[](int i) { return *data((_zero + i) % N); }
    T const& operator[](int i) const { return *data((_zero + i) % N); }
    
    void shift(int i)
    {   _zero = (_zero + size() - i) % size();
    }
private:
    int _zero = 0;
};


template<class T>
struct span_t
{
    T * _data;
    int _size;

    int size() const { return _size; }
    
    T * data() { return _data; }
    T const* data() const { return _data; }

    T * begin() { return _data; }
    T const* begin() const { return _data; }
    
    T * end() { return _data + _size; }
    T const* end() const { return _data + _size; }

    T & operator[](int i) { return _data[i]; }
    T const& operator[](int i) const { return _data[i]; }
};

template<class T>
span_t<T> span(T * data, int sz) { return {data, sz}; }

} // wade