all:
	make install
	make pong

install:
	pacman -Syu
	pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-sdl3

compile: main.c chip8.c
	gcc main.c chip8.c -o chip8.exe $$(pkg-config --cflags --libs sdl3)

pong: main.c chip8.c
	gcc main.c chip8.c -o chip8.exe $$(pkg-config --cflags --libs sdl3)
	./chip8.exe games/PONG

clean: chip8.exe
	rm chip8.exe