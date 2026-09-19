#ifndef DISPLAY_H
#define DISPLAY_H

#include "Chip8.h"

// A declaration only; SFML supplies the actual window class later.
namespace sf
{
    class RenderWindow;
}

// Awais: main owns one SFML window and passes it to these functions.
void setupGraphics(sf::RenderWindow& window);
void drawGraphics(sf::RenderWindow& window, const Chip8& chip8);

#endif
