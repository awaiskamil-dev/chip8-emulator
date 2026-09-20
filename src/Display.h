#ifndef DISPLAY_H
#define DISPLAY_H

#include "Chip8.h"
#include "Frontend.h"
#include <SFML/Graphics.hpp>
#include <string>

class Display
{
public:
    bool setupGraphics();
    sf::RenderWindow& getWindow();
    void render(const Chip8& chip8, const FrontendState& state);

    // Also supports an offscreen SFML target for screenshot/resize tests.
    void drawGraphics(sf::RenderTarget& target, const Chip8& chip8,
                      const FrontendState& state);

private:
    sf::RenderWindow window;
    std::string lastTitle;

    void drawPlay(sf::RenderTarget& target, const Chip8& chip8,
                  const FrontendState& state);
    void drawDebug(sf::RenderTarget& target, const Chip8& chip8,
                   const FrontendState& state);
    void drawScreen(sf::RenderTarget& target, const Chip8& chip8,
                    float x, float y, float width, float height);
};

#endif
