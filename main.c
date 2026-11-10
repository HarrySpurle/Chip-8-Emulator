
#include <SDL3/SDL.h>
#include <stdio.h>
#include "chip8.h"

#define SCALE 10

static int map_key(SDL_Keycode key)
{
    switch (key)
    {
        case SDLK_1: return 0x1;
        case SDLK_2: return 0x2;
        case SDLK_3: return 0x3;
        case SDLK_4: return 0xC;

        case SDLK_Q: return 0x4;
        case SDLK_W: return 0x5;
        case SDLK_E: return 0x6;
        case SDLK_R: return 0xD;

        case SDLK_A: return 0x7;
        case SDLK_S: return 0x8;
        case SDLK_D: return 0x9;
        case SDLK_F: return 0xE;

        case SDLK_Z: return 0xA;
        case SDLK_X: return 0x0;
        case SDLK_C: return 0xB;
        case SDLK_V: return 0xF;

        default: return -1;
    }
}

int main(int argc, char *argv[])
{
    Chip8 chip8;
    chip8_init(&chip8);

    if (argc < 2)
    {
        printf("Usage: ./chip8.exe game.ch8\n");
        return 1;
    }

    if (!chip8_load_rom(&chip8, argv[1]))
    {
        return 1;
    }

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_AudioSpec audio_spec = {
        .format = SDL_AUDIO_S16,
        .channels = 1,
        .freq = 48000
    };

    SDL_AudioStream *audio_stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &audio_spec,
        NULL,
        NULL
    );

    if (audio_stream == NULL)
    {
        printf("Audio failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_ResumeAudioStreamDevice(audio_stream);

    SDL_Window *window = SDL_CreateWindow(
        "CHIP-8 Emulator",
        64 * SCALE,
        32 * SCALE,
        SDL_WINDOW_RESIZABLE
    );

    if (window == NULL)
    {
        printf("Window failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);

    if (renderer == NULL)
    {
        printf("Renderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    int running = 1;
    SDL_Event event;

    const double CPU_RATE = 700.0;
    const double TIMER_RATE = 60.0;

    double cpu_accumulator = 0.0;
    double timer_accumulator = 0.0;

    double audio_phase = 0.0;
    int sound_was_on = 0;

    Uint64 last_time = SDL_GetTicks();

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                running = 0;
            }

            if (event.type == SDL_EVENT_KEY_DOWN)
            {
                int key = map_key(event.key.key);

                if (key != -1)
                    chip8.keypad[key] = 1;
            }

            if (event.type == SDL_EVENT_KEY_UP)
            {
                int key = map_key(event.key.key);

                if (key != -1)
                    chip8.keypad[key] = 0;
            }
        }

        // Calculate elapsed time
        Uint64 current_time = SDL_GetTicks();

        double elapsed =
            (current_time - last_time) / 1000.0;

        last_time = current_time;

        // Add elapsed time to both clocks
        cpu_accumulator += elapsed;
        timer_accumulator += elapsed;

        // Run CPU at 700 instructions per second
        while (cpu_accumulator >= 1.0 / CPU_RATE)
        {
            chip8_cycle(&chip8);
            cpu_accumulator -= 1.0 / CPU_RATE;
        }

        // Update timers at 60Hz
        while (timer_accumulator >= 1.0 / TIMER_RATE)
        {
            chip8_update_timers(&chip8);
            timer_accumulator -= 1.0 / TIMER_RATE;
        }

        if (chip8.sound_timer > 0)
        {
            sound_was_on = 1;

            // Generate a 440 Hz square wave.
            Sint16 samples[480];

            for (int i = 0; i < 480; i++)
            {
                samples[i] = (audio_phase < 0.5) ? 6000 : -6000;

                audio_phase += 440.0 / 48000.0;

                if (audio_phase >= 1.0)
                    audio_phase -= 1.0;
            }

            // Keep only a small amount of audio queued.
            if (SDL_GetAudioStreamQueued(audio_stream) < (int)sizeof(samples))
            {
                SDL_PutAudioStreamData(
                    audio_stream,
                    samples,
                    sizeof(samples)
                );
            }
        }
        else if (sound_was_on)
        {
            SDL_ClearAudioStream(audio_stream);
            sound_was_on = 0;
        }

        // Black background
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw CHIP-8 pixels
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        for (int y = 0; y < 32; y++)
        {
            for (int x = 0; x < 64; x++)
            {
                int index = y * 64 + x;

                if (chip8.display[index])
                {
                    SDL_FRect pixel = {
                        x * SCALE,
                        y * SCALE,
                        SCALE,
                        SCALE
                    };

                    SDL_RenderFillRect(renderer, &pixel);
                }
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(1);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_DestroyAudioStream(audio_stream);
    SDL_Quit();

    return 0;
}
