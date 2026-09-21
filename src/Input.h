#ifndef INPUT_H
#define INPUT_H

#include "Chip8.h"
#include "Frontend.h"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

class Input
{
public:
    void handleInput(sf::RenderWindow& window, Chip8& chip8, FrontendState& state);
    void handleEvent(const sf::Event& event, sf::RenderWindow& window,
                     Chip8& chip8, FrontendState& state);

private:
    void minimizeWindow(sf::RenderWindow& window, Chip8& chip8, FrontendState& state);
    int chip8Key(sf::Keyboard::Scancode code);
};

#endif
