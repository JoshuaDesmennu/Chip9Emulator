#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#include <time.h>
#include "stack.h"

const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 320;
const int C_WIDTH = 64;
const int C_HEIGHT = 32;
const int tile_width = WINDOW_WIDTH / C_WIDTH;
const int tile_height = WINDOW_HEIGHT / C_HEIGHT;
uint8_t memory[4096];
uint8_t screen[32][64]; // access height then width
int PC = 0x200;
uint16_t I = 0;
int delayTimer = 0;
int soundTimer = 0;
uint8_t V[16];
StackNode *stack = NULL;
bool isLegacy = false;
bool isQuirky = false;

void drawScreen(SDL_Renderer *renderer);
uint32_t SDLCALL tick(uint32_t interval, void *params);
void clearScreen();

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: ./main [ROM FILENAME]\n");
        return -1;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    memset(memory, 0, 4096);
    srand((unsigned int)time(NULL));

    clearScreen();

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    const int FONT_COUNT = 80;
    uint8_t font[] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

    const int FONT_OFFSET = 0x050;

    // load the font into memory
    memcpy(memory + FONT_OFFSET, font, FONT_COUNT);

    // load the code into memory
    FILE *f = fopen(argv[1], "rb");
    if (f == NULL) {
        fprintf(stderr, "Cannot open file for reading");
        return -1;
    }
    fseek(f, 0, SEEK_END);
    int filesize = ftell(f);
    fseek(f, 0, SEEK_SET);
    fread(memory + 0x200, filesize, 1, f);
    fclose(f);

    if (SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer) != 0) {
        fprintf(stderr, "Failed to create window and renderer: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_SetWindowTitle(window, "CHIP8 Emulator");

    SDL_AddTimer(1000 / 60, tick, NULL);

    SDL_Event e;
    bool shouldClose = false;
    bool isAwaitingKeyPress = false;
    bool isDown = false;
    uint8_t keyAwaited = 0;
    int8_t keyDown = -1;
    SDL_Rect rect;
    uint8_t waitingRegister = 0;
    while (!shouldClose) {

             // fetch
        const uint8_t X = (memory[PC] & 0x0f);
        const uint8_t BEGIN = (memory[PC] & 0xf0) >> 4;
        uint16_t instruction = memory[PC++];
        instruction <<= 8;
        const uint8_t N = (memory[PC] & 0x0f);
        const uint8_t Y = (memory[PC] & 0xf0) >> 4;
        const uint8_t NN = memory[PC];
        instruction |= memory[PC++];
        const uint16_t NNN = instruction & 0x0fff;

        if (isAwaitingKeyPress) PC -= 2;

        SDL_Log("%x", instruction);

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                shouldClose = true;
            } else if (e.type == SDL_KEYDOWN && !isDown) {
                switch (e.key.keysym.scancode)
                {
                    case SDL_SCANCODE_1:
                        keyDown = 1;
                        break;
                    case SDL_SCANCODE_2:
                        keyDown = 2;
                        break;
                    case SDL_SCANCODE_3:
                        keyDown = 3;
                        break;
                    case SDL_SCANCODE_4:
                        keyDown = 0xc;
                        break;
                    case SDL_SCANCODE_Q:
                        keyDown = 4;
                        break;
                    case SDL_SCANCODE_W:
                        keyDown = 5;
                        break;
                    case SDL_SCANCODE_E:
                        keyDown = 6;
                        break;
                    case SDL_SCANCODE_R:
                        keyDown = 0xd;
                        break;
                    case SDL_SCANCODE_A:
                        keyDown = 7;
                        break;
                    case SDL_SCANCODE_S:
                        keyDown = 8;
                        break;
                    case SDL_SCANCODE_D:
                        keyDown = 9;
                        break;
                    case SDL_SCANCODE_F:
                        keyDown = 0xe;
                        break;
                    case SDL_SCANCODE_Z:
                        keyDown = 0xa;
                        break;
                    case SDL_SCANCODE_X:
                        keyDown = 0;
                        break;
                    case SDL_SCANCODE_C:
                        keyDown = 0xb;
                        break;
                    case SDL_SCANCODE_V:
                        keyDown = 0xf;
                        break;
                    default:
                        break;
                }
                isDown = true;
            } else if (e.type == SDL_KEYUP) {
                if (isAwaitingKeyPress) {
                    isAwaitingKeyPress = false;
                    V[waitingRegister] = keyDown;
                    PC += 2;
                }
                keyDown = -1;
                isDown = false;
            }
        }

        if (!isAwaitingKeyPress) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);

            if (instruction == 0x00E0) { //
                clearScreen();
            } else if (BEGIN == 1) { //
                PC = NNN;
            } else if (BEGIN == 2) {//
                stack = push(stack, PC);
                PC = NNN;
            } else if (instruction == 0x00EE) {
                PC = stack->data;
                stack = pop(stack);
            } else if (BEGIN == 3) {//
                if (V[X] == NN) {
                    PC += 2;
                }
            } else if (BEGIN == 4) {//
                if (V[X] != NN) {
                    PC += 2;
                }  
            } else if (BEGIN == 5 && N == 0) {//
                if (V[X] == V[Y]) {
                    PC += 2;
                }
            } else if (BEGIN == 9 && N == 0) {//
                if (V[X] != V[Y]) {
                    PC += 2;
                }
            } else if (BEGIN == 6) {//
                V[X] = NN;
            } else if (BEGIN == 7) {//
                V[X] += NN;
            } else if (BEGIN == 8) {
                switch (N) {
                    case 0://
                        V[X] = V[Y];
                        break;
                    case 1://
                        V[X] |= V[Y];
                        V[0xF] = 0;
                        break;
                    case 2://
                        V[X] &= V[Y];
                        V[0xF] = 0;
                        break;
                    case 3://
                        V[X] ^= V[Y];
                        V[0xF] = 0;
                        break;
                    case 4://
                        if (V[X] + V[Y] > 255) {
                            V[X] += V[Y];
                            V[0xF] = 1;
                        } else {
                            V[X] += V[Y];
                            V[0xF] = 0;
                        }
                        
                        break;
                    case 5://
                        if (V[X] >= V[Y]) {
                            V[X] -= V[Y];
                            V[0xF] = 1;
                        } else {
                            V[X] -= V[Y];
                            V[0xF] = 0;
                        }
                        
                        break;
                    case 6://
                        if (!isLegacy) V[X] = V[Y];
                        if (V[X] & 1 == 1) {
                            V[X] >>= 1;
                            V[0xF] = 1;
                        } else {
                            V[X] >>= 1;
                            V[0xF] = 0;
                        }
                        break;
                    case 7://
                        if (V[Y] >= V[X]) {
                            V[X] = V[Y] - V[X];
                            V[0xF] = 1;
                        } else {
                            V[X] = V[Y] - V[X];
                            V[0xF] = 0;
                        }
                        
                        break;
                    case 0x0E://
                        if (!isLegacy) V[X] = V[Y];
                        if ((V[X] >> 7) & 1 == 1) {
                            V[X] <<= 1;
                            V[0xF] = 1;
                        } else {
                            V[X] <<= 1;
                            V[0xF] = 0;
                        }
                        
                        break;
                }
            } else if (BEGIN == 0xA) {//
                I = NNN;
            } else if (BEGIN == 0XB) {
                if (!isQuirky) {
                    PC = NNN + V[0];
                } else {
                    PC = (X << 16) + NN + V[X];
                }
            } else if (BEGIN == 0xE) {
                switch (NN) {
                    case 0x9E:
                        if (keyDown == V[X]) {
                            PC += 2;
                        }
                        break;
                    case 0xA1:
                        if (keyDown != V[X]) {
                            PC += 2;
                        }
                }
            } else if (BEGIN == 0xC) {
                V[X] = rand() & NN;
            } else if (BEGIN == 0XD) {//
                int xCoord = V[X] % C_WIDTH;
                int yCoord = V[Y] % C_HEIGHT;
                V[0xF] = 0;
                for (int j = 0; j < N && yCoord + j < C_HEIGHT; j++) {
                    for (int i = 0; i < 8 && xCoord + i < C_WIDTH; i++) {
                        uint8_t spriteValue = (memory[I + j] >> (7-i)) & 1;
                        if (spriteValue == 1) {
                            if (screen[yCoord+j][xCoord+i] == 1) V[0xF] = 1;
                            screen[yCoord+j][xCoord+i] = screen[yCoord+j][xCoord+i] == 1 ? 0 : 1;
                        }
                    }
                }
            } else if (BEGIN == 0xF) {
                switch (NN) {
                    case 0x07:
                        V[X] = delayTimer;
                        break;
                    case 0x0A:
                        isAwaitingKeyPress = true;
                        waitingRegister = X;
                        // PC -= 2;
                        break;
                    case 0x15:
                        delayTimer = V[X];
                        break;
                    case 0x18:
                        soundTimer = V[X];
                        break;
                    case 0x1E://
                        if (I + V[X] > 0xFFF) {
                            I += V[X];
                            V[0xF] = 1;
                        } else {
                            I += V[X];
                            V[0xF] = 0;
                        }
                        
                        break;
                    case 0x29:
                        I = FONT_OFFSET + V[X] * 5;
                        break;
                    case 0x33://
                        uint8_t d1 = (V[X] / 100) % 10;
                        uint8_t d2 = (V[X] / 10) % 10;
                        uint8_t d3 = V[X] % 10;
                        memory[I] = d1;
                        memory[I+1] = d2;
                        memory[I+2] = d3;
                        break;
                    case 0x55://
                        for (int i = 0; i <= X; i++) {
                            memory[I + i] = V[i];
                        }
                        if (!isLegacy) I += X + 1;
                        break;
                    case 0x65://
                        for (int i = 0; i <= X; i++) {
                            V[i] = memory[I + i];
                        }
                        if (!isLegacy) I += X + 1;
                        break;
                    default:
                        SDL_Log("IDK Opcode: %x", instruction);
                        break;
                }
            }

            drawScreen(renderer);
            SDL_RenderPresent(renderer);
        }
        SDL_Delay(1);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    freeStack(stack);
    SDL_Quit();
}

void drawScreen(SDL_Renderer *renderer) {
    static SDL_Rect rect;
    for (int j = 0; j < C_HEIGHT; j++) {
        for (int i = 0; i < C_WIDTH; i++) {
            
            if (screen[j][i] == 0) {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            } else if (screen[j][i] == 1) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            }

            rect.w = tile_width;
            rect.h = tile_height;
            rect.x = i * tile_width;
            rect.y = j * tile_height;
            SDL_RenderFillRect(renderer, &rect);
        }
    }

}

void clearScreen() {
    for (int j = 0; j < C_HEIGHT; j++) {
        for (int i = 0; i < C_WIDTH; i++) {
            screen[j][i] = 0;
        }
    }
}

uint32_t SDLCALL tick(uint32_t interval, void *params) {
    if (delayTimer > 0) delayTimer--;
    if (soundTimer > 0) soundTimer--;
    return interval;
}

