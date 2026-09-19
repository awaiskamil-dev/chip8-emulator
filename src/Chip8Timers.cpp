#include "Chip8.h"

// Partner: implement these functions.

void Chip8::update_timers()
{
    // TODO: decrease delay_timer and sound_timer only if they are above zero.
    // Main calls this at 60 Hz, separately from emulate_cycle().
    // This function does not change pc.
}

void Chip8::timer_instructions()
{
    // TODO: FX07, FX15, FX18
    // Each completed instruction advances pc by 2.
}

bool Chip8::sound_active()
{
    return sound_timer > 0;
}
