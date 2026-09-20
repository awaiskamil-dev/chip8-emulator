#include "Chip8.h"
#include "Display.h"
#include "Input.h"
#include "Frontend.h"
#include <SFML/System/Clock.hpp>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

void printUsage()
{
    std::cout << "Usage: chip8 [ROM.ch8] [--debug] [--paused] [--hz 100..2000]\n"
              << "F1: Play/Debug  Space: Pause  F2: Step  F5: Reset  Esc: Close\n"
              << "+/-: CPU speed  Keypad: 1234 / QWER / ASDF / ZXCV\n";
}

int main(int argc, char* argv[])
{
    FrontendState state;
    for (int i = 1; i < argc; ++i) {
        std::string argument = argv[i];
        if (argument == "--help" || argument == "-h") {
            printUsage();
            return 0;
        } else if (argument == "--debug") {
            state.debugMode = true;
        } else if (argument == "--paused") {
            state.paused = true;
        } else if (argument == "--hz") {
            if (i + 1 >= argc) {
                std::cerr << "--hz needs a number between 100 and 2000.\n";
                return 1;
            }
            char* end = nullptr;
            long speed = std::strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || speed < 100 || speed > 2000) {
                std::cerr << "CPU speed must be between 100 and 2000.\n";
                return 1;
            }
            state.cpuHz = static_cast<int>(speed);
        } else if (argument.empty() || argument[0] == '-' || !state.romPath.empty()) {
            std::cerr << "Unexpected argument: " << argument << '\n';
            printUsage();
            return 1;
        } else {
            state.romPath = argument;
        }
    }

    try {
        Chip8 chip8;
        if (!state.romPath.empty()) {
            if (!loadCurrentRom(chip8, state)) {
                return 1;
            }
        } else {
            // A no-ROM window is useful for explaining the debugger at a demo.
            state.debugMode = true;
            state.message = "NO ROM LOADED. START WITH A ROM FILE PATH.";
            printUsage();
        }

        Display display;
        if (!display.setupGraphics()) {
            return 1;
        }
        Input input;
        sf::RenderWindow& window = display.getWindow();
        state.focused = window.hasFocus();
        sf::Clock clock;

        while (window.isOpen()) {
            double elapsedSeconds = clock.restart().asSeconds();
            input.handleInput(window, chip8, state);
            if (!window.isOpen()) {
                break;
            }
            updateEmulation(chip8, state, elapsedSeconds);
            display.render(chip8, state);
            // Always render the UI, even when no new game pixels were drawn.
            chip8.drawFlag = false;
        }
    } catch (const std::exception& error) {
        std::cerr << "Frontend error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
