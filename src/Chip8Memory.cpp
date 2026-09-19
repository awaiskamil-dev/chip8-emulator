#include "Chip8.h"

// Partner: implement these functions.

bool Chip8::load(const char* file_path)
{
    // TODO: call init() (Awais implements it).
    // TODO: read file_path in binary mode and check that it fits in memory.
    // TODO: load the ROM starting at memory[0x200].
    // Return true on success; print an error and return false on failure.
    return false; // Placeholder: ROM loading is not implemented yet.
}

void Chip8::memory_instructions()
{
    // TODO: ANNN, FX1E, FX29, FX33, FX55, FX65
    // Use 0x050 as the font start address for FX29 (5 bytes per character).
    // Each completed instruction advances pc by 2.
}
