#include "Chip8.h"
#include <random>
#include <cstdlib>
#include <cstring>
#include "time.h"

// Implemented by Awais

uint8_t chip8_fontset[80] =
{
    0xF0, 0x90, 0x90, 0x90, 0xF0, //0
    0x20, 0x60, 0x20, 0x20, 0x70, //1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
    0x90, 0x90, 0xF0, 0x10, 0x10, //4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
    0xF0, 0x10, 0x20, 0x40, 0x40, //7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
    0xF0, 0x90, 0xF0, 0x90, 0x90, //A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
    0xF0, 0x80, 0x80, 0x80, 0xF0, //C
    0xE0, 0x90, 0x90, 0x90, 0xE0, //D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
    0xF0, 0x80, 0xF0, 0x80, 0x80  //F
};

void Chip8::init()
{
    pc = 0x200; // reset program counter 
    opcode = 0; // reset opcode 
    I = 0;      // reset I
    sp = 0;     // reset stack pointer
    running = true;
    drawFlag = true;

    // clear display
    for(int i = 0; i < (64 * 32); i++){
        gfx[i] = 0;
    }

    // clear stack, key and registers
    for(int i = 0; i < 16; i++){
        stack[i] = 0;
        key[i] = 0;
        V[i] = 0;
    }

    // clear memory
    for(int i = 0; i < 4096; i++){
        memory[i] = 0;
    }

    // load fontset into memory
    for(int i = 0; i < 80; i++){
        memory[i + 0x050] = chip8_fontset[i];
    }

    // reset timers
    delay_timer = 0;
    sound_timer = 0;

    // seed random generation
    srand(time(NULL));
}

void Chip8::arithmetic()
{
    // Implements 11 arithemetic opcodes
    int x = (opcode & 0x0F00) >> 8;
    int nn = opcode & 0x00FF;
    
    switch (opcode & 0xF000)
    {
    // 6XNN - Sets VX to NN
    case 0x6000: 
        V[x] = nn;
        pc += 2;
        break;
    
    // 7XNN - Adds NN to VX
    case 0x7000: 
        V[x] += nn;
        pc += 2; 
        break;

    case 0x8000:
    {
        int y = (opcode & 0x00F0) >> 4;
        int vx = V[x];
        int vy = V[y];

        switch (opcode & 0x000F)
        {
        // 8XY0 - Sets VX to VY
        case 0x0: 
            V[x] = V[y];
            pc += 2;
            break;
        
        // 8XY1 - Sets VX to (VX OR VY)
        case 0x1:
            V[x] |= V[y];
            pc += 2;
            break;
        
        // 8XY2 - Sets VX to (VX AND VY)
        case 0x2:
            V[x] &= V[y];
            pc += 2;
            break;
        
        // 8XY3 - Sets VX to (VX XOR VY)
        case 0x3:
            V[x] ^= V[y];
            pc += 2;
            break;
        
        // 8XY4 - Add, with carry
        case 0x4:
            V[x] = vx + vy;
            V[0xF] = (vx + vy > 255);
            pc += 2;
            break;

        // 8XY5 - VX minus VY
        case 0x5:
            V[x] = vx - vy;
            V[0xF] = (vx >= vy); // no borrow
            pc += 2;
            break;

        // 8XY6 - Shift VX right
        case 0x6:
            V[x] = vx >> 1;
            V[0xF] = vx & 1;
            pc += 2;
            break;

        // 8XY7 - VY minus VX
        case 0x7:
            V[x] = vy - vx;
            V[0xF] = (vy >= vx); //no borrow
            pc += 2;
            break;

        // 8XYE - Shift VX left
        case 0xE:
            V[x] = vx << 1;
            V[0xF] = (vx >> 7) & 1;
            pc += 2;
            break;

        default:
            stop("unsupported 8XY instruction");
            break;
        }

        break;
    }
    default:
        stop("invalid arithemetic instruction");
        break;
    }
}

void Chip8::drawing() {
    if (opcode == 0x00E0) {
        std::memset(gfx, 0, sizeof(gfx));
        drawFlag = true;
        pc = static_cast<uint16_t>(pc + 2);
        return;
    }

    const uint8_t x_register = static_cast<uint8_t>((opcode & 0x0F00) >> 8);
    const uint8_t y_register = static_cast<uint8_t>((opcode & 0x00F0) >> 4);
    const uint8_t height = static_cast<uint8_t>(opcode & 0x000F);
    const uint8_t x_position = V[x_register];
    const uint8_t y_position = V[y_register];
    if (!can_read_memory(I, height)) {
        stop("sprite reads outside memory");
        return;
    }

    V[0xF] = 0;
    for (uint8_t row = 0; row < height; ++row) {
        const uint8_t sprite = memory[I + row];
        const uint8_t screen_y = static_cast<uint8_t>((y_position + row) % 32);
        for (uint8_t column = 0; column < 8; ++column) {
            if ((sprite & (0x80 >> column)) == 0) {
                continue;
            }
            const uint8_t screen_x = static_cast<uint8_t>((x_position + column) % 64);
            const uint16_t pixel = static_cast<uint16_t>(screen_y * 64 + screen_x);
            if (gfx[pixel] == 1) {
                V[0xF] = 1;
            }
            gfx[pixel] ^= 1;
        }
    }

    drawFlag = true;
    pc = static_cast<uint16_t>(pc + 2);
}

void Chip8::keypad() {
    const uint8_t x = static_cast<uint8_t>((opcode & 0x0F00) >> 8);

    if ((opcode & 0xF000) == 0xE000) {
        const uint8_t key_index = V[x];
        if (key_index >= 16) {
            stop("EX instruction used an invalid key index");
            return;
        }

        if ((opcode & 0x00FF) == 0x009E) {
            pc = static_cast<uint16_t>(pc + (key[key_index] ? 4 : 2));
            return;
        }
        if ((opcode & 0x00FF) == 0x00A1) {
            pc = static_cast<uint16_t>(pc + (key[key_index] ? 2 : 4));
            return;
        }
        stop("invalid keypad instruction");
        return;
    }

    if (opcode == static_cast<uint16_t>(0xF00A | (x << 8))) {
        for (uint8_t key_index = 0; key_index < 16; ++key_index) {
            if (key[key_index]) {
                V[x] = key_index;
                pc = static_cast<uint16_t>(pc + 2);
                return;
            }
        }
        return;
    }

    stop("invalid keypad instruction");
}