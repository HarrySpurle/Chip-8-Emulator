#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>

typedef struct
{
    // 4KB of memory
    uint8_t memory[4096];

    // 16 general-purpose registers
    uint8_t V[16];

    // Index register
    uint16_t I;

    // Program counter
    uint16_t pc;

    // Stack (16 levels)
    uint16_t stack[16];
    uint8_t sp;

    // Timers
    uint8_t delay_timer;
    uint8_t sound_timer;

    // 64x32 display
    uint8_t display[64 * 32];

    // 16-key keypad
    uint8_t keypad[16];

    uint8_t waiting_for_key;
    uint8_t key_register;

} Chip8;

void chip8_init(Chip8 *chip8);

void chip8_cycle(Chip8 *chip8);

void chip8_update_timers(Chip8 *chip8);

int chip8_load_rom(Chip8 *chip8, const char *filename);

#endif