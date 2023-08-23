#pragma once

#include <cmath>
#include "wmath.h"

namespace wade {

// digital causal linear filter
template<int XTaps, int YTaps = 1>
struct filter
{
    float _cx[XTaps] = {1};
    float _cy[YTaps] = {0};
    float _x[XTaps] = {0};
    float _y[YTaps] = {0};

    // using IL = std::initializer_list<float>
    // filter() = default;
    // filter(IL const& cx, IL const& cy) : _cx(cx), _cy(cy) {}

    float operator()(float x)
    {   _x[0] = x;
        _y[0] = 0;
        for(int i=0 ; i<XTaps ; i++)
        {   _y[0] += _cx[i] * _x[i];
        }
        for(int i=1 ; i<YTaps ; i++)
        {   _y[0] -= _cy[i] * _y[i];
        }
        _y[0] /= _cy[0];
        for(int i=XTaps-1 ; i>0 ; i--)
        {   _x[i] = _x[i-1];
        }
        for(int i=YTaps-1 ; i>0 ; i--)
        {   _y[i] = _y[i-1];
        }
        return _y[0];
    }
}; 


template<int N>
struct fir_damper : public filter<N>
{
    void set_rolloff(float rolloff)
    {   for(int i=0 ; i<N ; i++)
        {   this->_cx[i] = 0;
        }
        for(int j=0 ; j<=N/2 ; j++)
        {   for(int i=0 ; i<N ; i++)
            {   float x = std::pow(rolloff, j * 2.0f / N);
                x *= std::cos(i * j * M_PI * 2 / N) / N;
                x *= (j>0) + (j*2<N);
                this->_cx[i] += x;
            }
        }
    }
};


struct ap_interp
{   float _g = 0, _x = 0, _y = 0;

    void set_delay(float delay)
    {   _g = (1-delay) / (1+delay);
    }

    float operator()(float x)
    {   float y = _g * (x - _y) + _x;
        _x = x; _y = y;
        return y;
    }
};

// virtual analog filter
template<int Order = 7, int Taylor = 3>
class va_filter
{
public:
    float step(float in, float dt)
    {   
        float y[Order + Taylor];
        for(int i=0 ; i<Order ; i++)
        {   y[i] = _state[i];
        }
        float input[Taylor] = {in, 0, 0};
        for(int i=0 ; i<Taylor ; i++)
        {   y[Order+i] = input[i];
            for(int j=0 ; j<Order ; j++)
            {   y[Order+i] -= y[i+j] * _coef[j];
            }
        }
        for(int i=0 ; i<Order ; i++)
        {   _state[i] = y[i];
            float d = 1;
            for(int j=1 ; j<=Taylor ; j++)
            {   _state[i] += y[i+j] * (d *= dt / j);
            }
        }
        return out();
    }
    // give same output as step() but 
    // without advancing filter state
    float project(float in, float dt)
    {
        float y[Order + Taylor];
        for(int i=0 ; i<Order ; i++)
        {   y[i] = _state[i];
        }
        float input[Taylor] = {in, 0, 0};
        for(int i=0 ; i<Taylor ; i++)
        {   y[Order+i] = input[i];
            for(int j=0 ; j<Order ; j++)
            {   y[Order+i] -= y[i+j] * _coef[j];
            }
        }
        float p = y[0];
        float d = 1;
        for(int j=1 ; j<=Taylor ; j++)
        {   p += y[j] * (d *= dt / j);
        }
        return p * _coef[0];
    }
    float out() const
    {   return _state[0] * _coef[0];   
    }
protected:
    float _coef[Order] = {0};
    float _state[Order] = {0};
};


template<int Order = 7, int Taylor = 3>
class va_cheby1 : public va_filter<Order, Taylor>
{
public:
    // TODO constexpr
    va_cheby1(float cutoff = 1, float ripple_db = 2)
    {
        using std::sin; using std::cos;
        using std::pow; using std::tanh;
        using std::sinh; using std::cosh;
        using std::sqrt;

        float * coef = this->_coef;
        float temp[Order+1];
        for(int i=0 ; i<Order+1 ; i++)
        {   coef[i] = temp[i] = 0;
        }
        coef[0] = 1;
        float e = sqrt(pow(10.0f, ripple_db/10.0f) - 1);
        // butterworth:
        // float rad_real = 1, rad_imag = 1;
        // chebyshev-I:
        float rad_real = sinh(asinh(1/e)/Order) * cutoff;
        float rad_imag = cosh(asinh(1/e)/Order) * cutoff;
        
        for(int i=1 ; i<=Order ; i+=2)
        {   // measured CCW from +i axis
            float theta = i * M_PI / (2*Order);
            float poleR = rad_real * -sin(theta);
            float poleI = rad_imag * cos(theta);
            
            float a = 0;
            float b = 1;
            float c = -poleR;
            if(i<Order) // if pole not on real axis, add conjugate too
            {   a = 1;
                b = -poleR * 2;
                c = poleR * poleR + poleI * poleI;
            }
            // temp = polynomial(a,b,c) * coef
            for(int j=0 ; j<i ; j++)
            {   if(j+2<Order) { temp[j+2] += a * coef[j]; }
                temp[j+1] += b * coef[j];
                temp[j+0] += c * coef[j];
            }
            // copy result back to filter polynomial
            for(int j=0 ; j<Order+1 ; j++)
            {   coef[j] = temp[j];
                temp[j] = 0;
            }
        }
        // TODO
        // // normalize gain
        // float norm = 1 / sqrt(1 + e*e*((Order+1)%2));
        // std::cout << "norm = " << norm << std::endl;
        // for(int i=0 ; i<Order ; i++)
        // {   coef[i] *= norm;
        //     std::cout << "coef = " << coef[i] << std::endl;
        // }
    }
};


// this resonant filter emits LP, BP and HP outputs
// and reacts very smoothly to rapid parameter modulation (unlike biquad)
class svf
{
public:
    svf(float f0 = 1, float res = 1)
    {
        set(f0, res);
    }
    // f0 = radians/sample, res = 
    void set_exact(float f0, float res)
    {
        set(std::tan(f0/2)*2, res);
    }
    void set(float f0, float res)
    {
        // _R = res;
        // _R2_g = r2g;

        float r2g = res * 2 + _g;
        _g = f0 / 2;
        _c_hp = r2g * _g;
        _c_bp = r2g + _g;
        _norm = 1 / (1 + _c_hp);
    }

    // void step(float x)
    // {
    //     _hp = ( x - _R2_g * _s1 - _s2 ) * _norm;

    //     _bp = _g * _hp + _s1;
    //     _s1 = _g * _hp + _bp;

    //     _lp = _g * _bp + _s2;
    //     _s2 = _g * _bp + _lp;
    // }

    svf & operator()(float x)
    {
        // _s1 = _g * _hp + _bp;
        // _s2 = _g * _bp + _lp;

        // _hp = ( x - _R2_g * _s1 - _s2 ) * _norm;
        // _hp = ( x - _R2_g * _g * _hp - _R2_g * _bp - _g * _bp - _lp ) * _norm;
        // _hp = ( x - (_R2_g * _g) * _hp - (_R2_g + g) * _bp - _lp ) * _norm;
        float hp1 = _hp;
        float bp1 = _bp;

        _hp = (x - (_c_hp * _hp) - (_c_bp * _bp) - _lp) * _norm;
        _bp += _g * (_hp + hp1);
        _lp += _g * (_bp + bp1);

        return *this;
    }

    float hp() const { return _hp; }
    float bp() const { return _bp; }
    float lp() const { return _lp; }

private:
    float _g;
    float _c_hp;
    float _c_bp;
    float _norm;
    float _hp = 0;
    float _bp = 0;
    float _lp = 0;
};



class filter_module
{
public:
    static constexpr int num_controls() { return 7; }

    filter_module(float * controls)
    :   _controls(controls)
    {
    }

    void render(float * audio, int frames)
    {
        _filter.set(_controls[0] * 1, _controls[1] * 4 + 0.5);
        for(int i=0 ; i<frames ; i++)
        {
            audio[i] = _filter(audio[i]).lp();
        }
    }

private:
    float * _controls;
    svf _filter;
};


class formant_module
{
public:
    static constexpr int num_controls() { return 7; }

    formant_module(float * controls)
    :   _controls(controls)
    {
        _controls[1] = 0;
        _controls[5] = 0;
        _controls[6] = 0;
    }

    void render(float * audio, int frames)
    {
        float lfo_amp = _controls[5] * 0.05;
        _lfo.freq = fast::exp(_controls[6] * 7) * 0.000002;

        float gain = 0.8 / (0.4 + _controls[0]);
        for(int i=0 ; i<frames ; i++)
        {
            float f = lfo_amp * _lfo() + _controls[1] * 0.3;
            f = wade::max(0.0f, wade::min(f, 1.0f));
            _f0.set(f, 1.03 - _controls[0]);
            _f1.set(f + _controls[1] * 0.3 + _controls[2] * 0.3, 1.03 - _controls[0]);

            float x = _f0(audio[i]).hp();
            x = _f1(x).lp();
            audio[i] = x * gain;
        }
    }

private:
    float * _controls;
    sinwave _lfo;
    svf _f0;
    svf _f1;
};

} // wade