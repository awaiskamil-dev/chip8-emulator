#include "Chip8.h"

// Awais: implement these functions.

void Chip8::init()
{
    // TODO: clear memory, V, stack, gfx and key using simple loops.
    // TODO: reset I, opcode, sp and both timers; set pc to 0x200.
    // TODO: put the 80 font bytes in memory starting at 0x050.
    // TODO: set drawFlag to true so the first frame gets drawn.
}

void Chip8::arithmetic()
{
    // TODO: use opcode to select and implement:
    // 6XNN, 7XNN
    // 8XY0, 8XY1, 8XY2, 8XY3
    // 8XY4, 8XY5, 8XY6, 8XY7, 8XYE
    // Each completed instruction advances pc by 2.
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
