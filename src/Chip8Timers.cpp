#include "Chip8.h"

// Implemented by Mustafa

void Chip8::timer_instructions() {
    const uint8_t x = static_cast<uint8_t>((opcode & 0x0F00) >> 8);

    switch (opcode & 0x00FF) {
    case 0x0007:
        V[x] = delay_timer;
        break;
    case 0x0015:
        delay_timer = V[x];
        break;
    case 0x0018:
        sound_timer = V[x];
        break;
    default:
        stop("invalid timer instruction");
        return;
    }
    pc = static_cast<uint16_t>(pc + 2);
}

void Chip8::update_timers() {
    if (delay_timer > 0) {
        --delay_timer;
    }
    if (sound_timer > 0) {
        --sound_timer;
    }
}
