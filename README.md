## Chip-8 Emulator (Written in C)

The Chip-8 is a virtual machine developed for creating simple games. It has 4KiB of memory and 16 8-bit general purpose registers. This program runs the Chip-8 emulator and allows the user to play games such as pong and Tetris. I used C for this project because it is a low-level systems language that allows me to manipulate the memory and registers directly.

## Quickstart

To run my emulator, open the directory in a terminal such as msys UCRT64 and run "make". This will install the required dependencies, compile the program, and open pong on the emulator. If you would like to play another game from the games/ folder, run "./chip8.exe games/GAMENAME"

## NOTE: Not all operands have yet been implemented, so not all games are fully functional
