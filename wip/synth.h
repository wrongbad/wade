#pragma once

namespace wade {

// TODO this file repetitively repeats itself repeatedly

template<class... Chains>
struct synth;

template<>
struct synth<>
{
    virtual ~synth() {}

    virtual int num_controls() { return 0; }
    virtual float * controls() { return nullptr; }

    virtual void note_on(uint8_t note, uint8_t v) {}
    virtual void note_off(uint8_t note, uint8_t v) {}
    virtual void note_pressure(uint8_t note, uint8_t v) {}

    virtual void record() {}
    virtual void play() {}
    virtual int loopstate() { return 0; } // 1=enabled, 2=playing, 4=recording

    virtual void render(float * buf, int len) {}
};

namespace maybe {

template<class... Args>
void note_on(Args&&...) { }

template<class T>
auto note_on(T && t, uint8_t n, uint8_t v) -> decltype(t.note_on(n,v))
{   t.note_on(n, v);
}

template<class... Args>
void note_off(Args&&...) { }

template<class T>
auto note_off(T && t, uint8_t n, uint8_t v) -> decltype(t.note_off(n,v))
{   t.note_off(n, v);
}

template<class... Args>
void note_pressure(Args&&...) { }

template<class T>
auto note_pressure(T && t, uint8_t n, uint8_t v) -> decltype(t.note_pressure(n,v))
{   t.note_pressure(n, v);
}

template<class... Args>
void record(Args&&...) { }

template<class T>
auto record(T && t) -> decltype(t.record())
{   t.record();
}

template<class... Args>
void play(Args&&...) { }

template<class T>
auto play(T && t) -> decltype(t.play())
{   t.play();
}

template<class... Args>
int loopstate(Args&&...) { return 0; }

template<class T>
auto loopstate(T && t) -> decltype(t.loopstate())
{   return t.loopstate();
}

template<class... Args>
void render(Args&&...) { }

template<class T>
auto render(T && t, float * b, int l) -> decltype(t.render(b, l))
{   t.render(b, l);
}

} // maybe


template<class... Chains>
struct synth_chainer;

template<>
struct synth_chainer<>
{   synth_chainer(float *) {}
    static constexpr int num_controls() { return 0; }
};

template<class Chain0, class... Chains>
struct synth_chainer<Chain0, Chains...>
{
    static constexpr int num_controls() { return Chain0::num_controls() + synth_chainer<Chains...>::num_controls(); }

    Chain0 _c0;
    synth_chainer<Chains...> _cs;

    synth_chainer(float * controls)
    :   _c0(controls),
        _cs(controls + Chain0::num_controls())
    {
    }
    void note_on(uint8_t note, uint8_t v)
    {   maybe::note_on(_c0, note, v);
        maybe::note_on(_cs, note, v);
    }
    void note_off(uint8_t note, uint8_t v)
    {   maybe::note_off(_c0, note, v);
        maybe::note_off(_cs, note, v);
    }
    void note_pressure(uint8_t note, uint8_t v)
    {   maybe::note_pressure(_c0, note, v);
        maybe::note_pressure(_cs, note, v);
    }
    void record()
    {   maybe::record(_c0);
        maybe::record(_cs);
    }
    void play()
    {   maybe::play(_c0);
        maybe::play(_cs);
    }
    int loopstate() // 1=enabled, 2=playing, 4=recording
    {   return maybe::loopstate(_c0) 
            | maybe::loopstate(_cs);
    }
    void render(float * buf, int len)
    {   maybe::render(_c0, buf, len);
        maybe::render(_cs, buf, len);
    }

};

template<class Chain0, class... Chains>
struct synth<Chain0, Chains...> : public synth<>
{
    arr<float, synth_chainer<Chain0, Chains...>::num_controls()> _controls { 0.5 };
    synth_chainer<Chain0, Chains...> _synth { _controls.data() };

    int num_controls() override { return _controls.size(); }
    float * controls() override { return _controls.data(); }

    void note_on(uint8_t note, uint8_t v) override
    {   _synth.note_on(note, v);
    }
    void note_off(uint8_t note, uint8_t v) override
    {   _synth.note_off(note, v);
    }
    void note_pressure(uint8_t note, uint8_t v) override
    {   _synth.note_pressure(note, v);
    }
    void render(float * buf, int len) override
    {   _synth.render(buf, len);
    }
    void record() override
    {   _synth.record();
    }
    void play() override
    {   _synth.play();
    }
    int loopstate() override
    {   return _synth.loopstate();
    }
};

} // wade
