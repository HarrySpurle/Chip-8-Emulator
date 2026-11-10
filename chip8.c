#include "chip8.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const uint8_t chip8_fontset[80] =
{
    // 0
    0xF0, 0x90, 0x90, 0x90, 0xF0,
    // 1
    0x20, 0x60, 0x20, 0x20, 0x70,
    // 2
    0xF0, 0x10, 0xF0, 0x80, 0xF0,
    // 3
    0xF0, 0x10, 0xF0, 0x10, 0xF0,
    // 4
    0x90, 0x90, 0xF0, 0x10, 0x10,
    // 5
    0xF0, 0x80, 0xF0, 0x10, 0xF0,
    // 6
    0xF0, 0x80, 0xF0, 0x90, 0xF0,
    // 7
    0xF0, 0x10, 0x20, 0x40, 0x40,
    // 8
    0xF0, 0x90, 0xF0, 0x90, 0xF0,
    // 9
    0xF0, 0x90, 0xF0, 0x10, 0xF0,
    // A
    0xF0, 0x90, 0xF0, 0x90, 0x90,
    // B
    0xE0, 0x90, 0xE0, 0x90, 0xE0,
    // C
    0xF0, 0x80, 0x80, 0x80, 0xF0,
    // D
    0xE0, 0x90, 0x90, 0x90, 0xE0,
    // E
    0xF0, 0x80, 0xF0, 0x80, 0xF0,
    // F
    0xF0, 0x80, 0xF0, 0x80, 0x80
};

void chip8_init(Chip8 *chip8)
{
    // Clear the entire virtual machine
    memset(chip8, 0, sizeof(Chip8));

    // CHIP-8 programs start at memory address 0x200
    chip8->pc = 0x200;

    // Load the fontset into memory at 0x050
    memcpy(&chip8->memory[0x050],
           chip8_fontset,
           sizeof(chip8_fontset));
}



void chip8_cycle(Chip8 *chip8)
{
    if (chip8->waiting_for_key)
    {
        for (int i = 0; i < 16; i++)
        {
            if (chip8->keypad[i])
            {
                chip8->V[chip8->key_register] = i;
                chip8->waiting_for_key = 0;

                // Wait until this key is released
                chip8->keypad[i] = 0;

                return;
            }
        }

        return;
    }

    uint16_t opcode =
        (chip8->memory[chip8->pc] << 8) |
         chip8->memory[chip8->pc + 1];

    chip8->pc += 2;

    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;
    uint8_t nn = opcode & 0x00FF;
    uint16_t nnn = opcode & 0x0FFF;

    switch (opcode & 0xF000)
    {
        // 1NNN: Jump to address NNN
        case 0x1000:
            chip8->pc = nnn;
            break;

        // 2NNN: Call subroutine
        case 0x2000:
            if (chip8->sp >= 16) {
                printf("Stack overflow!\n");
                break;
            }

            chip8->stack[chip8->sp++] = chip8->pc;
            chip8->pc = nnn;
            break;
        
        // 3XNN: Skip if VX == NN
        case 0x3000:
            if (chip8->V[x] == nn)
                chip8->pc += 2;
            break;

        // 4XNN: Skip if VX != NN
        case 0x4000:
            if (chip8->V[x] != nn)
                chip8->pc += 2;
            break;

        // 5XY0: Skip if VX == VY
        case 0x5000:
            if ((opcode & 0x000F) == 0 &&
                chip8->V[x] == chip8->V[y])
                chip8->pc += 2;
            break;

        // 6XNN: Set VX to NN
        case 0x6000:
            chip8->V[x] = nn;
            break;

        // 7XNN: Add NN to VX
        case 0x7000:
            chip8->V[x] += nn;
            break;

        // 8XY0 and 8XY4
        case 0x8000:
            switch (opcode & 0x000F)
            {
                // 8XY0: VX = VY
                case 0x0:
                    chip8->V[x] = chip8->V[y];
                    break;

                // 8XY1: VX = VX OR VY
                case 0x1:
                    chip8->V[x] |= chip8->V[y];
                    break;

                // 8XY2: VX = VX AND VY
                case 0x2:
                    chip8->V[x] &= chip8->V[y];
                    break;

                // 8XY3: VX = VX XOR VY
                case 0x3:
                    chip8->V[x] ^= chip8->V[y];
                    break;

                // 8XY4: VX = VX + VY
                case 0x4: {
                    uint16_t sum =
                        chip8->V[x] + chip8->V[y];

                    chip8->V[0xF] = sum > 255;
                    chip8->V[x] = (uint8_t)sum;
                    break;
                }

                // 8XY5 VX = VX - VY
                case 0x5:
                    chip8->V[0xF] =
                        chip8->V[x] >= chip8->V[y];

                    chip8->V[x] -= chip8->V[y];
                    break;

                default:
                    printf("Unknown opcode: 0x%04X\n", opcode);
                    break;
            }
            break;
        
        // 9XY0: Skip if VX != VY
        case 0x9000:
            if ((opcode & 0x000F) == 0 &&
                chip8->V[x] != chip8->V[y])
                chip8->pc += 2;
            break;

        // 00EE: Return from subroutine
        case 0x0000:
            if (opcode == 0x00EE) {
                if (chip8->sp == 0) {
                    printf("Stack underflow!\n");
                    break;
                }

                chip8->pc = chip8->stack[--chip8->sp];
            } else {
                printf("Unknown opcode: 0x%04X\n", opcode);
            }
            break;
        
        case 0xA000:
            chip8->I = nnn;
            break;

        // CXNN: VX = random byte AND NN
        case 0xC000:
            chip8->V[x] = (rand() % 256) & nn;
            break;

        // DXYN: Draw sprite at VX, VY
        case 0xD000: {
            uint8_t xpos = chip8->V[x] % 64;
            uint8_t ypos = chip8->V[y] % 32;
            uint8_t height = opcode & 0x000F;

            chip8->V[0xF] = 0;

            for (int row = 0; row < height; row++) {
                if (chip8->I + row >= 4096)
                    break;
                uint8_t sprite =
                    chip8->memory[chip8->I + row];

                for (int col = 0; col < 8; col++) {
                    if (sprite & (0x80 >> col)) {

                        int px = (xpos + col) % 64;
                        int py = (ypos + row) % 32;

                        int index = py * 64 + px;

                        // Detect collision
                        if (chip8->display[index]) {
                            chip8->V[0xF] = 1;
                        }

                        // XOR the pixel
                        chip8->display[index] ^= 1;
                    }
                }
            }

            break;
        }

        // EX9E: Skip if key VX is pressed
        case 0xE000:
            switch (opcode & 0x00FF)
            {
                case 0x9E:
                    if (chip8->keypad[chip8->V[x] & 0x0F])
                        chip8->pc += 2;
                    break;

                // EXA1: Skip if key VX is not pressed
                case 0xA1:
                    if (!chip8->keypad[chip8->V[x] & 0x0F])
                        chip8->pc += 2;
                    break;

                default:
                    printf("Unknown opcode: 0x%04X\n",
                           opcode);
                    break;
            }
            break;


        // FXNN instructions
        case 0xF000:
            switch (opcode & 0x00FF)
            {
                // FX07: VX = delay timer
                case 0x07:
                    chip8->V[x] = chip8->delay_timer;
                    break;

                // FX15: Delay timer = VX
                case 0x15:
                    chip8->delay_timer = chip8->V[x];
                    break;

                // FX18: Sound timer = VX
                case 0x18:
                    chip8->sound_timer = chip8->V[x];
                    break;

                // FX1E: I = I + VX
                case 0x1E:
                    chip8->I += chip8->V[x];
                    break;

                // FX29: Point I at character sprite
                case 0x29:
                    chip8->I =
                        0x050 + (chip8->V[x] & 0x0F) * 5;
                    break;

                // FX33: Store binary-coded decimal
                case 0x33:
                    if (chip8->I + 2 < 4096)
                    {
                        chip8->memory[chip8->I]     = chip8->V[x] / 100;
                        chip8->memory[chip8->I + 1] = (chip8->V[x] / 10) % 10;
                        chip8->memory[chip8->I + 2] = chip8->V[x] % 10;
                    }
                    break;

                // FX55: Store V0 through VX in memory
                case 0x55:
                    if (chip8->I + x >= 4096) {
                        printf("Register store out of range!\n");
                        break;
                    }

                    for (int i = 0; i <= x; i++) {
                        chip8->memory[chip8->I + i] =
                            chip8->V[i];
                    }
                    break;

                // FX65: Load V0 through VX from memory
                case 0x65:
                    if (chip8->I + x >= 4096) {
                        printf("Register load out of range!\n");
                        break;
                    }

                    for (int i = 0; i <= x; i++) {
                        chip8->V[i] =
                            chip8->memory[chip8->I + i];
                    }
                    break;

                // FX0A: Wait for key press
                case 0x0A:
                    chip8->waiting_for_key = 1;
                    chip8->key_register = x;
                    break;

                default:
                    printf("Unknown opcode: 0x%04X\n",
                           opcode);
                    break;
            }
            break;

        default:
            printf("Unknown opcode: 0x%04X\n", opcode);
            break;
    }
}

void chip8_update_timers(Chip8 *chip8)
{
    if (chip8->delay_timer > 0)
    {
        chip8->delay_timer--;
    }

    if (chip8->sound_timer > 0)
    {
        chip8->sound_timer--;
    }
}

int chip8_load_rom(Chip8 *chip8, const char *filename)
{
    FILE *file = fopen(filename, "rb");

    if (file == NULL)
    {
        printf("Failed to open ROM: %s\n", filename);
        return 0;
    }

    size_t bytes_read = fread(
        &chip8->memory[0x200],
        1,
        sizeof(chip8->memory) - 0x200,
        file
    );

    // Check whether the ROM was too large
    if (ferror(file) || !feof(file))
    {
        printf("ROM is too large or could not be read!\n");
        fclose(file);
        return 0;
    }

    fclose(file);

    printf("Loaded ROM: %s (%zu bytes)\n", filename, bytes_read);

    return 1;
}