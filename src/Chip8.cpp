#include "Chip8.h"
#include <random>
#include "time.h"

// Awais: implement these functions.

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
        int y = (opcode & 0x00F0) >> 4;

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
        
        // 8XY4 - Adds VY to VX. VF is set to 1 when there's a carry,
        // and to 0 when there isn't.
        case 0x4:
            if(V[x] > (0xFF - V[x])){ // 0xFF = 255 in dec
                V[0xF] = 1; // carry
            }
            else{
                V[0xF] = 0;
            }
            V[x] += V[y];
            pc += 2;
            break;

        // 8XY5 - VY is subtracted from VX. VF is set to 0 when
        // there's a borrow, and 1 when there isn't.
        case 0x5:
            if(V[y] > V[x]){
                V[0xF] = 0; //borrow
            }
            else{
                V[0xF] = 1;
            }
            V[x] -= V[y];
            pc += 2;
            break;

        // 0x8XY6 - Shifts VX right by one. VF is set to the value of
        // the least significant bit of VX before the shift.
        case 0x6:
            V[0xF] = V[x] & 0x1;
            V[x] >>= 1;
            pc += 2;
            break;

        // 0x8XY7: Sets VX to VY minus VX. VF is set to 0 when there's
        // a borrow, and 1 when there isn't.
        case 0x7:
            if(V[x] > V[y]){
                V[0xF] = 0; // borrow
            }
            else{
                V[0xF] = 1;
            }
            V[x] = V[y] - V[x];
            pc += 2;
            break;
        
        // 0x8XYE: Shifts VX left by one. VF is set to the value of
        // the most significant bit of VX before the shift.
        case 0xE:
            V[0xF] = V[x] >> 7;
            V[x] <<= 1;
            pc += 2;
            break;
        default:
            printf("\nUnknown op code: %.4X\n", opcode);
            exit(3);
            break;
        }

        break;
    default:
        printf("\nUnknown op code: %.4X\n", opcode);
        exit(3);
        break;
    }
}

void Chip8::drawing()
{
    // TODO: 00E0 -- clear gfx.
    // TODO: DXYN -- draw sprites using XOR and set V[0xF] for collisions.
    // Both instructions set drawFlag and advance pc by 2.
}

void Chip8::keypad()
{
    // TODO: EX9E, EXA1 -- check key and skip when needed.
    // TODO: FX0A -- wait for a key without blocking the window loop.
    // Leave pc unchanged while waiting; advance it when a key is found.
}
