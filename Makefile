RGB_INCDIR=../include
RGB_LIBDIR=../lib

EXE=tetris
CC=cc
CFLAGS=-Wall -O3 -g -Wextra -Wno-unused-parameter
INCLUDE=$(RGB_INCDIR) `sdl-config --cflags`
LDFLAGS=`sdl-config --libs` -L$(RGB_LIBDIR) -lrgbmatrix -lm -lpthread -lstdc++

all: $(EXE)

$(EXE): $(EXE).o
	$(CC) $< -o $@ $(LDFLAGS)

%.o : %.c
	$(CC) -I$(INCLUDE) $(CFLAGS) -c -o $@ $<

32x64: $(EXE)
	sudo ./$(EXE) --led-rows=32 --led-cols=64 --led-pixel-mapper="Rotate:90"

clean:
	rm $(EXE) *.o
