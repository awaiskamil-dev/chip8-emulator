#include "Chip8.h"

#include <cstdio>
#include <cstdlib>

// Implemented by Mustafa

void Chip8::stop(const char* message) {
    std::fprintf(stderr, "CHIP-8 error: %s\n", message);
    running = false;
}

bool Chip8::can_read_memory(uint16_t address, uint16_t length) const {
    return address < 4096 && length <= 4096 - address;//avoiding overflow
}

void Chip8::emulate_cycle() {
    if (!running) {
        return;
    }

    if (!can_read_memory(pc, 2)) {
        stop("program counter is outside memory");
        return;
    }

    opcode = static_cast<uint16_t>(memory[pc] << 8) | memory[pc + 1];

    switch (opcode & 0xF000) {
    case 0x0000:
        if (opcode == 0x00E0 || opcode == 0x00EE) {
            if (opcode == 0x00E0) {
                drawing();
            } else {
                flow_control();
            }
        } else {
            stop("unsupported 0NNN instruction");
        }
        break;
    case 0x1000:
    case 0x2000:
    case 0x3000:
    case 0x4000:
    case 0x5000:
    case 0x9000:
    case 0xB000:
        if ((opcode & 0xF000) == 0x5000 || (opcode & 0xF000) == 0x9000) {
            if ((opcode & 0x000F) != 0) {
                stop("unsupported 5XYN or 9XYN instruction");
                break;
            }
        }
        flow_control();
        break;
    case 0x6000:
    case 0x7000:
    case 0x8000:
        arithmetic();
        break;
    case 0xA000:
        memory_instructions();
        break;
    case 0xC000:
        random_number();
        break;
    case 0xD000:
        drawing();
        break;
    case 0xE000:
        if ((opcode & 0x00FF) == 0x009E || (opcode & 0x00FF) == 0x00A1) {
            keypad();
        } else {
            stop("unsupported EX instruction");
        }
        break;
    case 0xF000:
        switch (opcode & 0x00FF) {
        case 0x0007:
        case 0x0015:
        case 0x0018:
            timer_instructions();
            break;
        case 0x000A:
            keypad();
            break;
        case 0x001E:
        case 0x0029:
        case 0x0033:
        case 0x0055:
        case 0x0065:
            memory_instructions();
            break;
        default:
            stop("unsupported FX instruction");
            break;
        }
        break;
    default:
        stop("unsupported instruction");
        break;
    }
}

void Chip8::flow_control() {
    const uint16_t nnn = opcode & 0x0FFF;
    const uint8_t x = static_cast<uint8_t>((opcode & 0x0F00) >> 8);
    const uint8_t y = static_cast<uint8_t>((opcode & 0x00F0) >> 4);
    const uint8_t nn = static_cast<uint8_t>(opcode & 0x00FF);

    switch (opcode & 0xF000) {
    case 0x0000:
        if (sp == 0) {
            stop("stack underflow on return");
            return;
        }
        --sp;
        if (stack[sp] > 4092) {
            stop("return address is outside memory");
            return;
        }
        pc = static_cast<uint16_t>(stack[sp] + 2);
        break;
    case 0x1000:
        if (!can_read_memory(nnn, 2)) {
            stop("jump address is outside memory");
            return;
        }
        pc = nnn;
        break;
    case 0x2000:
        if (sp >= 16) {
            stop("stack overflow on call");
            return;
        }
        if (!can_read_memory(nnn, 2)) {
            stop("call address is outside memory");
            return;
        }
        stack[sp] = pc;
        ++sp;
        pc = nnn;
        break;
    case 0x3000:
        pc = static_cast<uint16_t>(pc + (V[x] == nn ? 4 : 2));
        break;
    case 0x4000:
        pc = static_cast<uint16_t>(pc + (V[x] != nn ? 4 : 2));
        break;
    case 0x5000:
        pc = static_cast<uint16_t>(pc + (V[x] == V[y] ? 4 : 2));
        break;
    case 0x9000:
        pc = static_cast<uint16_t>(pc + (V[x] != V[y] ? 4 : 2));
        break;
    case 0xB000: {
        const uint16_t address = static_cast<uint16_t>(nnn + V[0]);
        if (!can_read_memory(address, 2)) {
            stop("BNNN target is outside memory");
            return;
        }
        pc = address;
        break;
    }
    default:
        stop("invalid flow-control instruction");
        break;
    }
}

void Chip8::random_number() {
    const uint8_t x = static_cast<uint8_t>((opcode & 0x0F00) >> 8);
    const uint8_t nn = static_cast<uint8_t>(opcode & 0x00FF);
    V[x] = static_cast<uint8_t>((std::rand() & 0xFF) & nn);
    pc = static_cast<uint16_t>(pc + 2);
}
