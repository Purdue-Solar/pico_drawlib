# WARNING: THIS MAKEFILE IS MEANT FOR SIMULATION ONLY!!!

CC = clang
LIBS = raylib
CFLAGS = -DSIMULATION -IInc -ISim $(shell pkg-config --cflags $(LIBS))
LFLAGS = $(shell pkg-config --libs $(LIBS))

SRCS = $(wildcard Sim/*.c) $(wildcard Src/*.c)
OBJS = $(patsubst %.c,simbuild/%.o,$(SRCS))
EXE = pico_drawlib

all: simbuild/$(EXE)

simbuild/$(EXE): $(OBJS)
	$(CC) -o $@ $^ $(LFLAGS)

simbuild/%.o: %.c
	mkdir -p simbuild/$(shell dirname $<)
	$(CC) -o $@ -c $< $(CFLAGS)
