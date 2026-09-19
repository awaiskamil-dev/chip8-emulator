#include <iostream>
#include "Chip8.h"
#include "Display.h"
#include "Input.h"

int main()
{
    // Integration -- do this together after the other functions are ready.
    // 1. Make a Chip8 object and read the ROM path.
    // 2. Call chip8.load(path); stop if it returns false.
    // 3. Make an SFML RenderWindow and pass it to setupGraphics(window).
    // 4. While the window is open:
    //    - Call handleInput(window, chip8).
    //    - Call chip8.emulate_cycle() at the chosen CPU speed.
    //    - Call chip8.update_timers() at 60 Hz, even while waiting for a key.
    //    - If drawFlag is true, call drawGraphics(window, chip8), then clear it.
    //    - Use chip8.sound_active() when adding sound.

    std::cout << "CHIP-8 starter. Emulator code is still TODO.\n";
    return 0;
}
