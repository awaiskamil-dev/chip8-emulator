#ifndef INPUT_H
#define INPUT_H

#include "Chip8.h"

namespace sf
{
    class RenderWindow;
}

// Awais: use the same window that main passes to Display.
void handleInput(sf::RenderWindow& window, Chip8& chip8);

#endif
