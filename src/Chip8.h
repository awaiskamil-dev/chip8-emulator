#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>

class Chip8
{
public:
    // Partner: load() calls init(), then loads the ROM.
    bool load(const char* file_path);
    void emulate_cycle();
    void update_timers(); // Main calls this at 60 Hz.
    bool sound_active();

    // Awais: Display reads gfx; Input writes key.
    uint8_t gfx[64 * 32] = {0};
    uint8_t key[16] = {0};
    bool drawFlag = false;

private:
    // One shared set of CPU variables for both people.
    uint8_t memory[4096] = {0};
    uint8_t V[16] = {0};
    uint16_t I = 0;
    uint16_t pc = 0x200;
    uint16_t opcode = 0;
    uint16_t stack[16] = {0};
    uint16_t sp = 0;
    uint8_t delay_timer = 0;
    uint8_t sound_timer = 0;

    // Awais -- Chip8.cpp
    void init();
    void arithmetic();
    void drawing();
    void keypad();

    // Partner -- Chip8Execution.cpp
    void flow_control();
    void random_number();

    // Partner -- Chip8Memory.cpp and Chip8Timers.cpp
    void memory_instructions();
    void timer_instructions();
};

#endif
