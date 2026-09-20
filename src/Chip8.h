#ifndef CHIP8_H
#define CHIP8_H

#include <cstdint>

class Chip8 {
public:
    uint8_t V[16] = {0};
    uint8_t memory[4096] = {0};
    uint8_t gfx[64 * 32] = {0};
    uint8_t key[16] = {0};
    uint16_t stack[16] = {0};

    uint16_t opcode = 0;
    uint16_t I = 0;
    uint16_t pc = 0x200;
    uint8_t sp = 0;
    uint8_t delay_timer = 0;
    uint8_t sound_timer = 0;
    bool drawFlag = false;
    bool running = true;

    void init();
    bool load(const char* file_path);
    void emulate_cycle();
    void update_timers();

    void arithmetic();
    void drawing();
    void keypad();
    void flow_control();
    void random_number();
    void memory_instructions();
    void timer_instructions();

private:
    void stop(const char* message);
    bool can_read_memory(uint16_t address, uint16_t length) const;
};

#endif
