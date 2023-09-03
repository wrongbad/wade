#pragma once

#include "midi.h"

namespace wade {

struct rtmidi
{
    int output_count() 
    {   
        return _output.getPortCount();
    }
    std::string output_name(int i)
    {   
        return _output.getPortName(i);
    }
    void connect_output(int i)
    {   
        _output.openPort(i);
    }
    void connect_output(std::string hint = "", bool print = true)
    {
        int ports = _output.getPortCount();
        if(!ports) { throw std::runtime_error("no midi outputs!"); }
        int select = 0;
        for(int i=0 ; print && i<ports ; i++)
        {   std::string name = _output.getPortName(i);
            if(print) { std::cout << "midi out " << i << ": " << name << std::endl; }
            if(name.find(hint) < name.size()) { select = i; break; }
        }
        if(print) { std::cout << "using " << select << std::endl; }
        _output.openPort(select);
    }

    int input_count() 
    {   
        return _input.getPortCount();
    }
    std::string input_name(int i)
    {   
        return _input.getPortName(i);
    }
    void connect_input(int i)
    {   
        bool ignore_sysex = true, ignore_time = true, ignore_sense = true;
        _input.ignoreTypes(ignore_sysex, ignore_time, ignore_sense);
        
        _input.openPort(i);
    }
    void connect_input(std::string hint = "", bool print = true)
    {
        int ports = _input.getPortCount();
        if(!ports) { throw std::runtime_error("no midi inputs!"); }
        int select = 0;
        for(int i=0 ; print && i<ports ; i++)
        {   std::string name = _input.getPortName(i);
            if(print) { std::cout << "midi in " << i << ": " << name << std::endl; }
            if(name.find(hint) < name.size()) { select = i; break; }
        }

        bool ignore_sysex = true, ignore_time = true, ignore_sense = true;
        _input.ignoreTypes(ignore_sysex, ignore_time, ignore_sense);

        if(print) { std::cout << "using " << select << std::endl; }
        _input.openPort(select);
    }


    template<class Handler>
    void receive(Handler && handler)
    {
        while(1)
        {
            double timestamp = _input.getMessage(&_msg);
            if(_msg.empty()) { break; }
            handler(_msg.data(), _msg.size(), timestamp);
        }
    }

    void send(uint8_t const* msg, int len)
    {
        _output.sendMessage(msg, len);
    }

private:

    RtMidiIn _input;
    RtMidiOut _output;
    std::vector<uint8_t> _msg = std::vector<uint8_t>(8);
};


} // wade