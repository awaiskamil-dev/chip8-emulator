#include "Chip8.h"

// Partner: implement these functions.

void Chip8::emulate_cycle()
{
    // TODO: fetch two bytes from memory[pc] into opcode.
    // TODO: use switch/if statements to call the right function:
    // arithmetic(), drawing(), keypad(), flow_control(), random_number(),
    // memory_instructions() or timer_instructions().
    // See the routing table in README.md.
    // The selected function updates pc. Do not advance pc here too.
    // Timers are updated separately by main at 60 Hz.
}

void Chip8::flow_control()
{
    // TODO: 1NNN, 2NNN, 00EE
    // TODO: 3XNN, 4XNN, 5XY0, 9XY0, BNNN
    // CALL stores the current pc at stack[sp], then increases sp.
    // RETURN decreases sp, restores the saved pc, then adds 2.
    // Check stack bounds before using stack[sp].
}

void Chip8::random_number()
{
    // TODO: CXNN -- random byte AND NN, stored in VX.
    // Advance pc by 2. Seed the generator once when loading a game.
}
