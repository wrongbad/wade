#pragma once

#include "wade/audio.h"
#include "wade/math.h"

namespace wade {

template<class Delegate>
struct rtaudio
{
    Delegate & delegate;
    int sample_rate = 44100;
    int in_channels = 0;
    int out_channels = 2;
    bool clip = true;
    RtAudio _driver;

    static int render(void * out, void * in, unsigned frames,
            double /*time*/, RtAudioStreamStatus /*status*/, void * data)
    {
        rtaudio * self = (rtaudio*)data;
        wade::audio in_buf { (float*)in, self->in_channels, int(frames) };
        wade::audio out_buf { (float*)out, self->out_channels, int(frames) };
        wade::audio_context ctx { self->sample_rate, false };
        out_buf.fill(0);
        self->delegate.render(in_buf, out_buf, ctx);
        if(clip)
            for(float & x : out_buf.flatten())
                x = wade::clip(x);
        return (int)ctx.abort;
    }

    void start(int buffer_size = 128)
    {
        RtAudio::StreamParameters out_params;
        out_params.deviceId = _driver.getDefaultOutputDevice();
        out_params.nChannels = out_channels;

        RtAudio::StreamParameters in_params;
        in_params.deviceId = _driver.getDefaultInputDevice();
        in_params.nChannels = in_channels;
        
        RtAudio::StreamOptions options;
        options.flags = RTAUDIO_SCHEDULE_REALTIME;
        options.flags |= RTAUDIO_NONINTERLEAVED;
        // options.flags |= RTAUDIO_HOG_DEVICE;

        unsigned buffer_request = buffer_size;
        _driver.openStream(
            out_channels ? &out_params : nullptr,
            in_channels ? &in_params : nullptr,
            RTAUDIO_FLOAT32, 
            sample_rate,
            &buffer_request,
            &render,
            (void*)this,
            &options,
            nullptr // err callback
        );
        _driver.startStream();
    }
    void stop()
    {
        _driver.stopStream();
        _driver.closeStream();
    }
};

} // wade