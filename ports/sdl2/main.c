/* Guardians: Denjin Makai II - SDL2 front end.
 *
 * Native translated program + Seta 2 board runtime.  The guest ROM is read from
 * the user's archive at run time; no ROM data is built into this executable.
 */
#include "grdians/board.h"
#include "grdians/bus.h"
#include "m68k/runner.h"
#include "program.h"

#include <SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FRAMES_PER_SECOND 57.55
#define INSTRUCTION_BUDGET 35000u

typedef struct config {
    char rom[1024];
    int fullscreen;
    int scale;
} config;

static void default_config(config *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    snprintf(cfg->rom, sizeof(cfg->rom), "grdians.zip");
    cfg->fullscreen = 0;
    cfg->scale = 2;
}

static void load_config(config *cfg)
{
    default_config(cfg);
    FILE *f = fopen("guardians.cfg", "r");
    if (!f) return;
    char line[1200];
    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq++ = 0;
        char *value = eq;
        while (*value == ' ' || *value == '\t') value++;
        char *end = value + strlen(value);
        while (end > value && (end[-1] == '\n' || end[-1] == '\r' || end[-1] == ' ')) *--end = 0;
        if (!strcmp(line, "rom")) snprintf(cfg->rom, sizeof(cfg->rom), "%s", value);
        else if (!strcmp(line, "fullscreen")) cfg->fullscreen = atoi(value) != 0;
        else if (!strcmp(line, "scale")) cfg->scale = atoi(value) > 0 ? atoi(value) : 2;
    }
    fclose(f);
}

static uint32_t controller_input(SDL_GameController *pad)
{
    uint32_t input = 0;
    if (!pad) return 0;
    if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_LEFT)) input |= GR_INPUT_P1_LEFT;
    if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) input |= GR_INPUT_P1_RIGHT;
    if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_UP)) input |= GR_INPUT_P1_UP;
    if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN)) input |= GR_INPUT_P1_DOWN;
    if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_A)) input |= GR_INPUT_P1_B1;
    if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_B)) input |= GR_INPUT_P1_B2;
    if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_X)) input |= GR_INPUT_P1_B3;
    if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_START)) input |= GR_INPUT_P1_START;
    if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_BACK)) input |= GR_INPUT_COIN1;
    return input;
}

int main(int argc, char **argv)
{
    config cfg;
    load_config(&cfg);
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--rom") && i + 1 < argc) snprintf(cfg.rom, sizeof(cfg.rom), "%s", argv[++i]);
        else if (!strcmp(argv[i], "--fullscreen")) cfg.fullscreen = 1;
        else if (!strcmp(argv[i], "--windowed")) cfg.fullscreen = 0;
        else if (!strcmp(argv[i], "--scale") && i + 1 < argc) cfg.scale = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--help")) {
            printf("Guardians native port\n"
                   "  --rom FILE.zip   player-supplied grdians.zip\n"
                   "  --fullscreen     start fullscreen\n"
                   "  --windowed       start windowed\n"
                   "  --scale N        integer window scale (default 2)\n");
            return 0;
        }
    }
    if (cfg.scale < 1) cfg.scale = 1;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    gr_board board;
    if (!gr_board_load(&board, cfg.rom)) {
        fprintf(stderr, "error: %s\n", gr_board_last_error(&board));
        SDL_Quit();
        return 1;
    }
    gr_board_reset(&board);
    m68k_program_init();

    uint32_t flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
    if (cfg.fullscreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    SDL_Window *window = SDL_CreateWindow("Guardians: Denjin Makai II",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          GR_SCREEN_WIDTH * cfg.scale,
                                          GR_SCREEN_HEIGHT * cfg.scale, flags);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             GR_SCREEN_WIDTH, GR_SCREEN_HEIGHT);
    if (!texture) {
        fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GameController *pad = NULL;
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) { pad = SDL_GameControllerOpen(i); if (pad) break; }
    }

    uint16_t *pixels = (uint16_t *)calloc(GR_SCREEN_WIDTH * GR_SCREEN_HEIGHT, sizeof(uint16_t));
    if (!pixels) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    const uint32_t frame_ms = (uint32_t)(1000.0 / FRAMES_PER_SECOND);
    uint32_t next = SDL_GetTicks();
    int running = 1;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
            else if (event.type == SDL_KEYDOWN && !event.key.repeat) {
                if (event.key.keysym.sym == SDLK_ESCAPE) running = 0;
                else if (event.key.keysym.sym == SDLK_F11)
                    SDL_SetWindowFullscreen(window, cfg.fullscreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
                else if (event.key.keysym.sym == SDLK_F12)
                    gr_board_save_png("guardians-shot.png", pixels, GR_SCREEN_WIDTH, GR_SCREEN_HEIGHT);
            } else if (event.type == SDL_CONTROLLERDEVICEADDED && !pad) {
                pad = SDL_GameControllerOpen(event.cdevice.which);
            }
        }

        const uint8_t *keys = SDL_GetKeyboardState(NULL);
        uint32_t input = controller_input(pad);
        if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A]) input |= GR_INPUT_P1_LEFT;
        if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]) input |= GR_INPUT_P1_RIGHT;
        if (keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W]) input |= GR_INPUT_P1_UP;
        if (keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S]) input |= GR_INPUT_P1_DOWN;
        if (keys[SDL_SCANCODE_Z] || keys[SDL_SCANCODE_J]) input |= GR_INPUT_P1_B1;
        if (keys[SDL_SCANCODE_X] || keys[SDL_SCANCODE_K]) input |= GR_INPUT_P1_B2;
        if (keys[SDL_SCANCODE_C] || keys[SDL_SCANCODE_L]) input |= GR_INPUT_P1_B3;
        if (keys[SDL_SCANCODE_RETURN]) input |= GR_INPUT_P1_START;
        if (keys[SDL_SCANCODE_5]) input |= GR_INPUT_COIN1;
        if (keys[SDL_SCANCODE_1]) input |= GR_INPUT_SERVICE;
        gr_board_set_inputs(&board, input);

        gr_board_raise_vblank(&board);
        int result = m68k_run(&board.cpu, INSTRUCTION_BUDGET);
        if (result != M68K_RUN_OK) {
            char message[160];
            snprintf(message, sizeof(message),
                     "Native translation reached %06X with no translated code.\n"
                     "This build stops instead of interpreting instructions.",
                     m68k_last_missing_pc());
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Guardians native runtime", message, window);
            running = 0;
            continue;
        }
        gr_board_render(&board, pixels, GR_SCREEN_WIDTH, GR_SCREEN_HEIGHT);

        SDL_UpdateTexture(texture, NULL, pixels, GR_SCREEN_WIDTH * (int)sizeof(uint16_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        next += frame_ms;
        uint32_t now = SDL_GetTicks();
        if ((int32_t)(next - now) > 0) SDL_Delay(next - now);
        else next = now;
    }

    free(pixels);
    if (pad) SDL_GameControllerClose(pad);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    gr_board_free(&board);
    SDL_Quit();
    return 0;
}
