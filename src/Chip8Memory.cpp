#include "Chip8.h"

#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>

// Implemented by Mustafa

bool Chip8::load(const char* file_path) {
    init();

    if (file_path == nullptr) {
        std::fprintf(stderr, "CHIP-8 error: ROM path is null\n");
        running = false;
        return false;
    }

    std::ifstream rom(file_path, std::ios::binary);
    if (!rom) {
        std::fprintf(stderr, "CHIP-8 error: could not open ROM '%s'\n", file_path);
        running = false;
        return false;
    }

    const std::vector<char> bytes((std::istreambuf_iterator<char>(rom)),
                                  std::istreambuf_iterator<char>());
    const std::size_t available = sizeof(memory) - 0x200;

    if (bytes.empty()) {
        stop("ROM file is empty");
        return false;
    }
    if (bytes.size() > available) {
        std::fprintf(stderr, "CHIP-8 error: ROM is too large (%zu bytes; maximum is %zu)\n",
                     bytes.size(), available);
        running = false;
        return false;
    }

    for (std::size_t index = 0; index < bytes.size(); ++index) {
        memory[0x200 + index] = static_cast<uint8_t>(
            static_cast<unsigned char>(bytes[index]));
    }
    return true;
}

void Chip8::memory_instructions() {
    const uint8_t x = static_cast<uint8_t>((opcode & 0x0F00) >> 8);

    if ((opcode & 0xF000) == 0xA000) {
        I = opcode & 0x0FFF;
        pc = static_cast<uint16_t>(pc + 2);
        return;
    }

    switch (opcode & 0x00FF) {
    case 0x001E:
        I = static_cast<uint16_t>(I + V[x]);
        pc = static_cast<uint16_t>(pc + 2);
        break;
    case 0x0029:
        if (V[x] > 0x0F) {
            stop("FX29 received a non-hex digit");
            return;
        }
        I = static_cast<uint16_t>(0x050 + (V[x] * 5));
        pc = static_cast<uint16_t>(pc + 2);
        break;
    case 0x0033:
        if (!can_read_memory(I, 3)) {
            stop("FX33 writes outside memory");
            return;
        }
        memory[I] = static_cast<uint8_t>(V[x] / 100);
        memory[I + 1] = static_cast<uint8_t>((V[x] / 10) % 10);
        memory[I + 2] = static_cast<uint8_t>(V[x] % 10);
        pc = static_cast<uint16_t>(pc + 2);
        break;
    case 0x0055:
        if (!can_read_memory(I, static_cast<uint16_t>(x + 1))) {
            stop("FX55 writes outside memory");
            return;
        }
        for (uint8_t index = 0; index <= x; ++index) {
            memory[I + index] = V[index];
        }
        pc = static_cast<uint16_t>(pc + 2);
        break;
    case 0x0065:
        if (!can_read_memory(I, static_cast<uint16_t>(x + 1))) {
            stop("FX65 reads outside memory");
            return;
        }
        for (uint8_t index = 0; index <= x; ++index) {
            V[index] = memory[I + index];
        }
        pc = static_cast<uint16_t>(pc + 2);
        break;
    default:
        stop("invalid memory instruction");
        break;
    }
}
