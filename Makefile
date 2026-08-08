# WARNING: THIS MAKEFILE IS MEANT FOR SIMULATION ONLY!!!
# The simulation's main file is Sim/main.c.
# The real program's entry point is Src/core1Entry.cpp.
#
# Linux / macOS only — requires pkg-config and POSIX mkdir.
# On Windows use the CMake build instead:
#   cmake -S Sim -B Sim/build -G Ninja && cmake --build Sim/build

CC  = clang
CXX = clang++
LIBS = raylib
CFLAGS   = -DSIMULATION -IInc -ISim -ISrc/mcufont_decoder -I../pico_canlib/Inc \
           $(shell pkg-config --cflags $(LIBS))
CXXFLAGS = $(CFLAGS) -std=c++17
LFLAGS   = $(shell pkg-config --libs $(LIBS))

# C sources: everything under Sim/ and Src/ except pdl.cpp and core1Entry.cpp
CSRCS   = $(wildcard Sim/*.c) $(wildcard Src/*.c) $(wildcard Src/mcufont_decoder/*.c)
# C++ sources: only pdl.cpp (core1Entry.cpp requires pico multicore headers)
CXXSRCS = Src/pdl.cpp

COBJS   = $(patsubst %.c,simbuild/%.o,$(CSRCS))
CXXOBJS = $(patsubst %.cpp,simbuild/%.o,$(CXXSRCS))
OBJS    = $(COBJS) $(CXXOBJS)
EXE     = pico_drawlib

all: simbuild/$(EXE)

simbuild/$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(LFLAGS)

simbuild/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) -o $@ -c $< $(CFLAGS)

simbuild/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) -o $@ -c $< $(CXXFLAGS)
