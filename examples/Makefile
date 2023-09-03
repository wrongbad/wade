CXX_FLAGS = -std=c++14 -I.. -O3
# CXX_FLAGS += -Wfatal-errors -Wall -Wextra -Wpedantic -Wconversion -Wshadow 

RT_CXX_FLAGS = $(CXX_FLAGS)
RT_CXX_FLAGS += $(shell pkg-config --cflags rtmidi rtaudio)
RT_CXX_FLAGS += $(shell pkg-config --libs rtmidi rtaudio)

bin/synth_osmose : synth_osmose.cpp
	mkdir -p bin
	$(CXX) $(RT_CXX_FLAGS) $< -o $@