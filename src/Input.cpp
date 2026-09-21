#include "Input.h"
#include "UiLayout.h"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <algorithm>

namespace
{
void togglePause(const Chip8& chip8, FrontendState& state)
{
    if (!state.romLoaded || !chip8.running) return;
    state.paused = !state.paused;
    state.clearTiming = true;
    state.stepRequests = 0;
}
}

int Input::chip8Key(sf::Keyboard::Scancode code)
{
    // Physical positions: 1234 / QWER / ASDF / ZXCV.
    switch (code) {
    case sf::Keyboard::Scancode::Num1: return 0x1;
    case sf::Keyboard::Scancode::Num2: return 0x2;
    case sf::Keyboard::Scancode::Num3: return 0x3;
    case sf::Keyboard::Scancode::Num4: return 0xC;
    case sf::Keyboard::Scancode::Q: return 0x4;
    case sf::Keyboard::Scancode::W: return 0x5;
    case sf::Keyboard::Scancode::E: return 0x6;
    case sf::Keyboard::Scancode::R: return 0xD;
    case sf::Keyboard::Scancode::A: return 0x7;
    case sf::Keyboard::Scancode::S: return 0x8;
    case sf::Keyboard::Scancode::D: return 0x9;
    case sf::Keyboard::Scancode::F: return 0xE;
    case sf::Keyboard::Scancode::Z: return 0xA;
    case sf::Keyboard::Scancode::X: return 0x0;
    case sf::Keyboard::Scancode::C: return 0xB;
    case sf::Keyboard::Scancode::V: return 0xF;
    default: return -1;
    }
}

void Input::handleInput(sf::RenderWindow& window, Chip8& chip8, FrontendState& state)
{
    // SFML 3 returns an optional event. Key repeat is disabled by Display.
    while (const auto event = window.pollEvent()) {
        handleEvent(*event, window, chip8, state);
        if (!window.isOpen()) {
            return;
        }
    }
}

void Input::handleEvent(const sf::Event& event, sf::RenderWindow& window,
                        Chip8& chip8, FrontendState& state)
{
    if (event.is<sf::Event::Closed>()) {
        window.close();
    } else if (event.is<sf::Event::FocusLost>()) {
        state.focused = false;
        state.mousePosition = {-1, -1};
        state.clearTiming = true;
        state.stepRequests = 0;
        for (int i = 0; i < 16; ++i) {
            chip8.key[i] = 0;
        }
    } else if (event.is<sf::Event::FocusGained>()) {
        state.focused = true;
        state.clearTiming = true;
    } else if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        state.mousePosition = sf::Vector2f(moved->position);
    } else if (event.is<sf::Event::MouseLeft>()) {
        state.mousePosition = {-1, -1};
    } else if (const auto* clicked = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (!state.focused || clicked->button != sf::Mouse::Button::Left) return;
        state.mousePosition = sf::Vector2f(clicked->position);
        if (UiLayout::closeButton.contains(state.mousePosition)) {
            window.close();
        } else if (UiLayout::minimizeButton.contains(state.mousePosition)) {
            minimizeWindow(window, chip8, state);
        } else if ((state.debugMode ? UiLayout::expandButton : UiLayout::debugButton)
                       .contains(state.mousePosition)) {
            state.debugMode = !state.debugMode;
        } else if (UiLayout::pauseButton(state.debugMode).contains(state.mousePosition)) {
            togglePause(chip8, state);
        }
    } else if (const auto* released = event.getIf<sf::Event::KeyReleased>()) {
        int index = chip8Key(released->scancode);
        if (index >= 0) {
            chip8.key[index] = 0;
        }
    } else if (const auto* pressed = event.getIf<sf::Event::KeyPressed>()) {
        if (!state.focused) {
            return;
        }
        switch (pressed->scancode) {
        case sf::Keyboard::Scancode::Escape:
            minimizeWindow(window, chip8, state);
            return;
        case sf::Keyboard::Scancode::F1:
            // Only the layout changes. No ROM reload or CPU state change.
            state.debugMode = !state.debugMode;
            return;
        case sf::Keyboard::Scancode::Space:
            togglePause(chip8, state);
            return;
        case sf::Keyboard::Scancode::F2:
            if (state.paused && state.romLoaded && chip8.running) {
                ++state.stepRequests;
            }
            return;
        case sf::Keyboard::Scancode::F5:
            state.resetRequested = true;
            return;
        case sf::Keyboard::Scancode::Equal:
        case sf::Keyboard::Scancode::NumpadPlus:
            state.cpuHz = std::min(2000, state.cpuHz + 100);
            state.clearTiming = true;
            return;
        case sf::Keyboard::Scancode::Hyphen:
        case sf::Keyboard::Scancode::NumpadMinus:
            state.cpuHz = std::max(100, state.cpuHz - 100);
            state.clearTiming = true;
            return;
        default:
            break;
        }

        int index = chip8Key(pressed->scancode);
        if (index >= 0) {
            chip8.key[index] = 1;
        }
    }
}

void Input::minimizeWindow(sf::RenderWindow& window, Chip8& chip8, FrontendState& state)
{
#ifdef _WIN32
    if (window.isOpen()) {
        // SFML has no minimize function; use the Windows window handle.
        ShowWindow(window.getNativeHandle(), SW_MINIMIZE);
        handleEvent(sf::Event(sf::Event::FocusLost{}), window, chip8, state);
    }
#else
    (void)window;
    (void)chip8;
    (void)state;
#endif
}
