#include "Display.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <iostream>

namespace
{
const sf::Color background(12, 17, 22);
const sf::Color panelColor(19, 26, 33);
const sf::Color borderColor(44, 58, 67);
const sf::Color screenColor(7, 13, 17);
const sf::Color ink(221, 230, 233);
const sf::Color muted(128, 150, 161);
const sf::Color accent(140, 233, 190);
const sf::Color amber(240, 190, 111);

// A tiny 5x7 UI alphabet. Each number is one row of five pixels.
// This avoids a platform-dependent font path or a missing .ttf file.
struct Glyph
{
    char character;
    unsigned char rows[7];
};

const Glyph alphabet[] = {
    {'A', {14,17,17,31,17,17,17}}, {'B', {30,17,17,30,17,17,30}},
    {'C', {14,17,16,16,16,17,14}}, {'D', {30,17,17,17,17,17,30}},
    {'E', {31,16,16,30,16,16,31}}, {'F', {31,16,16,30,16,16,16}},
    {'G', {14,17,16,23,17,17,15}}, {'H', {17,17,17,31,17,17,17}},
    {'I', {14,4,4,4,4,4,14}},     {'J', {7,2,2,2,18,18,12}},
    {'K', {17,18,20,24,20,18,17}}, {'L', {16,16,16,16,16,16,31}},
    {'M', {17,27,21,21,17,17,17}}, {'N', {17,25,21,19,17,17,17}},
    {'O', {14,17,17,17,17,17,14}}, {'P', {30,17,17,30,16,16,16}},
    {'Q', {14,17,17,17,21,18,13}}, {'R', {30,17,17,30,20,18,17}},
    {'S', {15,16,16,14,1,1,30}},   {'T', {31,4,4,4,4,4,4}},
    {'U', {17,17,17,17,17,17,14}}, {'V', {17,17,17,17,17,10,4}},
    {'W', {17,17,17,21,21,21,10}}, {'X', {17,17,10,4,10,17,17}},
    {'Y', {17,17,10,4,4,4,4}},     {'Z', {31,1,2,4,8,16,31}},
    {'0', {14,17,19,21,25,17,14}}, {'1', {4,12,4,4,4,4,14}},
    {'2', {14,17,1,2,4,8,31}},    {'3', {30,1,1,14,1,1,30}},
    {'4', {2,6,10,18,31,2,2}},    {'5', {31,16,16,30,1,1,30}},
    {'6', {14,16,16,30,17,17,14}}, {'7', {31,1,2,4,8,8,8}},
    {'8', {14,17,17,14,17,17,14}}, {'9', {14,17,17,15,1,1,14}},
    {':', {0,4,4,0,4,4,0}},      {'.', {0,0,0,0,0,6,6}},
    {'-', {0,0,0,31,0,0,0}},     {'+', {0,4,4,31,4,4,0}},
    {'/', {1,2,2,4,8,8,16}},     {'_', {0,0,0,0,0,0,31}},
    {'[', {14,8,8,8,8,8,14}},    {']', {14,2,2,2,2,2,14}},
    {'(', {2,4,8,8,8,4,2}},      {')', {8,4,2,2,2,4,8}},
    {'=', {0,0,31,0,31,0,0}},    {'>', {16,8,4,2,4,8,16}},
    {'?', {14,17,1,2,4,0,4}},    {'!', {4,4,4,4,4,0,4}},
    {' ', {0,0,0,0,0,0,0}}
};

void addBlock(sf::VertexArray& vertices, float x, float y,
              float width, float height, sf::Color color)
{
    // Two triangles form a square. Batch blocks into one SFML draw call.
    const sf::Vector2f points[6] = {
        {x, y}, {x + width, y}, {x, y + height},
        {x + width, y}, {x + width, y + height}, {x, y + height}
    };
    for (int i = 0; i < 6; ++i) {
        vertices.append(sf::Vertex{points[i], color});
    }
}

void text(sf::RenderTarget& target, const std::string& value,
          float x, float y, int scale, sf::Color color)
{
    sf::VertexArray pixels(sf::PrimitiveType::Triangles);
    x = std::floor(x);
    y = std::floor(y);
    for (unsigned char character : value) {
        char upper = static_cast<char>(std::toupper(character));
        const Glyph missing = {'?', {14,17,1,2,4,0,4}};
        const Glyph* glyph = nullptr;
        for (const Glyph& candidate : alphabet) {
            if (candidate.character == upper) {
                glyph = &candidate;
                break;
            }
        }
        // An unsupported character is shown as a question mark.
        if (glyph == nullptr) {
            glyph = &missing;
        }
        for (int row = 0; row < 7; ++row) {
            for (int column = 0; column < 5; ++column) {
                if (glyph->rows[row] & (1 << (4 - column))) {
                    addBlock(pixels, x + column * scale, y + row * scale,
                             static_cast<float>(scale), static_cast<float>(scale), color);
                }
            }
        }
        x += 6 * scale;
    }
    target.draw(pixels);
}

void rectangle(sf::RenderTarget& target, float x, float y,
               float width, float height, sf::Color color, bool outline = false)
{
    sf::RectangleShape shape({width, height});
    shape.setPosition({x, y});
    shape.setFillColor(color);
    if (outline) {
        shape.setOutlineThickness(1);
        shape.setOutlineColor(borderColor);
    }
    target.draw(shape);
}

std::string hex(unsigned int value, int digits)
{
    char result[16];
    std::snprintf(result, sizeof(result), "%0*X", digits, value);
    return result;
}

std::string fit(const std::string& value, int characters)
{
    if (characters <= 3) {
        return "";
    }
    if (value.size() <= static_cast<std::size_t>(characters)) {
        return value;
    }
    return value.substr(0, characters - 3) + "...";
}

std::string romName(const FrontendState& state)
{
    if (state.romPath.empty()) {
        return "NO ROM";
    }
    std::size_t slash = state.romPath.find_last_of("/\\");
    if (slash == std::string::npos) return state.romPath;
    return state.romPath.substr(slash + 1);
}

std::string status(const Chip8& chip8, const FrontendState& state)
{
    if (!state.romLoaded) return "NO ROM";
    if (!chip8.running) return "STOPPED";
    if (!state.focused) return "UNFOCUSED";
    if (state.paused) return "PAUSED";
    return "RUNNING";
}
}

bool Display::setupGraphics()
{
    window.create(sf::VideoMode({1280, 860}), "CHIP-8");
    if (!window.isOpen()) {
        std::cerr << "Could not create the SFML window.\n";
        return false;
    }
    window.setMinimumSize(sf::Vector2u{960, 720});
    window.setKeyRepeatEnabled(false);
    window.setFramerateLimit(60);
    return true;
}

sf::RenderWindow& Display::getWindow()
{
    return window;
}

void Display::render(const Chip8& chip8, const FrontendState& state)
{
    std::string title = "CHIP-8 | " + romName(state) + " | " + status(chip8, state)
                      + (state.debugMode ? " | DEBUG" : " | PLAY")
                      + " | F1: mode  Space: pause  F2: step  F5: reset";
    if (title != lastTitle) {
        window.setTitle(title);
        lastTitle = title;
    }
    drawGraphics(window, chip8, state);
    window.display();
}

void Display::drawGraphics(sf::RenderTarget& target, const Chip8& chip8,
                           const FrontendState& state)
{
    sf::Vector2u size = target.getSize();
    if (size.x == 0 || size.y == 0) {
        return;
    }
    // Coordinates remain actual window pixels after resizing, not stretched UI.
    target.setView(sf::View(sf::FloatRect({0, 0},
                   {static_cast<float>(size.x), static_cast<float>(size.y)})));
    target.clear(background);
    if (state.debugMode && size.x >= 960 && size.y >= 720) {
        drawDebug(target, chip8, state);
    } else {
        drawPlay(target, chip8, state);
    }
}

void Display::drawScreen(sf::RenderTarget& target, const Chip8& chip8,
                         float x, float y, float width, float height)
{
    int scale = static_cast<int>(std::min(width / 64, height / 32));
    if (scale < 1) {
        return;
    }
    float left = std::floor(x + (width - 64 * scale) / 2);
    float top = std::floor(y + (height - 32 * scale) / 2);
    rectangle(target, left, top, 64.0f * scale, 32.0f * scale, screenColor);
    sf::VertexArray pixels(sf::PrimitiveType::Triangles);
    // Read exactly the existing 2048 framebuffer entries. No sprite/opcode work.
    for (int row = 0; row < 32; ++row) {
        for (int column = 0; column < 64; ++column) {
            if (chip8.gfx[row * 64 + column] != 0) {
                addBlock(pixels, left + column * scale, top + row * scale,
                         static_cast<float>(scale), static_cast<float>(scale), accent);
            }
        }
    }
    target.draw(pixels);
}

void Display::drawPlay(sf::RenderTarget& target, const Chip8& chip8,
                       const FrontendState& state)
{
    float width = static_cast<float>(target.getSize().x);
    float height = static_cast<float>(target.getSize().y);
    drawScreen(target, chip8, 24, 24, width - 48, height - 48);
    // Normal play contains only the game. Pause/status is in the window title.
    if (!state.romLoaded || !chip8.running) {
        std::string message = fit(state.message.empty() ? status(chip8, state) : state.message,
                                  static_cast<int>((width - 48) / 12));
        rectangle(target, 0, height - 48, width, 48, background);
        text(target, message, 24, height - 30, 2, amber);
    }
}

void Display::drawDebug(sf::RenderTarget& target, const Chip8& chip8,
                        const FrontendState& state)
{
    float width = static_cast<float>(target.getSize().x);
    float height = static_cast<float>(target.getSize().y);
    const float margin = 24;
    const float gap = 16;
    const float rightWidth = 320;
    float leftWidth = width - margin * 2 - gap - rightWidth;
    float rightX = margin + leftWidth + gap;
    float bodyY = 100;
    float bodyHeight = height - bodyY - 72;
    float gameHeight = bodyHeight - 228 - gap;
    float keypadY = bodyY + gameHeight + gap;

    rectangle(target, 24, 28, 7, 21, accent);
    text(target, "CHIP-8", 44, 28, 3, ink);
    text(target, "/ VIRTUAL MACHINE", 172, 36, 1, muted);
    std::string machineStatus = status(chip8, state);
    sf::Color stateColor = machineStatus == "RUNNING" ? accent : amber;
    rectangle(target, width - 180, 24, 156, 30, panelColor, true);
    rectangle(target, width - 168, 36, 6, 6, stateColor);
    text(target, machineStatus, width - 148, 32, 2, stateColor);
    text(target, "ROM / " + fit(romName(state), static_cast<int>((width - 400) / 12)),
         24, 68, 2, muted);
    text(target, std::to_string(state.cpuHz) + " HZ", width - 304, 68, 2, accent);
    text(target, "DEBUG MODE", width - 144, 68, 2, ink);
    rectangle(target, 24, 88, width - 48, 1, borderColor);

    // Game display panel.
    rectangle(target, margin, bodyY, leftWidth, gameHeight, panelColor, true);
    text(target, "01 / DISPLAY", margin + 20, bodyY + 18, 2, muted);
    text(target, "64 X 32", margin + leftWidth - 104, bodyY + 20, 1, muted);
    drawScreen(target, chip8, margin + 16, bodyY + 48, leftWidth - 32, gameHeight - 64);

    // Keypad visualization reads the same key[] that the opcodes use.
    rectangle(target, margin, keypadY, leftWidth, 228, panelColor, true);
    text(target, "02 / KEYPAD", margin + 20, keypadY + 18, 2, muted);
    const int keyOrder[16] = {1,2,3,12, 4,5,6,13, 7,8,9,14, 10,0,11,15};
    const char* physicalKeys = "1234QWERASDFZXCV";
    for (int index = 0; index < 16; ++index) {
        float x = margin + 20 + (index % 4) * 60;
        float y = keypadY + 48 + (index / 4) * 42;
        bool held = chip8.key[keyOrder[index]] != 0;
        rectangle(target, x, y, 52, 34, held ? accent : background, true);
        text(target, hex(keyOrder[index], 1), x + 10, y + 10, 2,
             held ? background : ink);
        text(target, std::string(1, physicalKeys[index]), x + 38, y + 7, 1,
             held ? background : muted);
    }
    float guideX = margin + 288;
    text(target, "PHYSICAL KEYS", guideX, keypadY + 52, 1, muted);
    text(target, "1 2 3 4", guideX, keypadY + 76, 2, ink);
    text(target, "Q W E R", guideX, keypadY + 104, 2, ink);
    text(target, "A S D F", guideX, keypadY + 132, 2, ink);
    text(target, "Z X C V", guideX, keypadY + 160, 2, ink);
    text(target, "GREEN = HELD", guideX, keypadY + 194, 1, accent);

    // Register panel. Values are formatted directly from Chip8 on each frame.
    float registersHeight = std::min(400.0f, bodyHeight - 240);
    rectangle(target, rightX, bodyY, rightWidth, registersHeight, panelColor, true);
    text(target, "03 / REGISTERS", rightX + 20, bodyY + 18, 2, muted);
    float rowSpacing = std::floor((registersHeight - 124) / 8);
    for (int row = 0; row < 8; ++row) {
        float y = bodyY + 52 + row * rowSpacing;
        text(target, "V" + hex(row, 1), rightX + 20, y, 2, muted);
        text(target, hex(chip8.V[row], 2), rightX + 68, y, 2, ink);
        text(target, "V" + hex(row + 8, 1), rightX + 176, y, 2, muted);
        text(target, hex(chip8.V[row + 8], 2), rightX + 224, y, 2,
             row == 7 ? accent : ink);
    }
    float infoY = bodyY + registersHeight - 72;
    rectangle(target, rightX + 20, infoY, rightWidth - 40, 1, borderColor);
    text(target, "PC " + hex(chip8.pc, 4), rightX + 20, infoY + 18, 2, accent);
    text(target, "I " + hex(chip8.I, 4), rightX + 176, infoY + 18, 2, ink);
    text(target, "SP " + hex(chip8.sp, 2) + " / 10", rightX + 20, infoY + 44, 2, muted);

    float opcodeY = bodyY + registersHeight + gap;
    rectangle(target, rightX, opcodeY, rightWidth, 112, panelColor, true);
    text(target, "04 / CURRENT OPCODE", rightX + 20, opcodeY + 18, 2, muted);
    text(target, hex(chip8.opcode, 4), rightX + 20, opcodeY + 48, 5, accent);
    text(target, "LAST FETCHED", rightX + 164, opcodeY + 66, 1, muted);

    float timersY = opcodeY + 112 + gap;
    float timersHeight = bodyY + bodyHeight - timersY;
    rectangle(target, rightX, timersY, rightWidth, timersHeight, panelColor, true);
    text(target, "05 / TIMERS", rightX + 20, timersY + 18, 2, muted);
    text(target, "DT " + hex(chip8.delay_timer, 2), rightX + 20, timersY + 48, 3, ink);
    text(target, "ST " + hex(chip8.sound_timer, 2), rightX + 176, timersY + 48, 3,
         chip8.sound_timer > 0 ? amber : ink);
    if (timersHeight >= 112) {
        text(target, "60 HZ", rightX + 20, timersY + 88, 1, muted);
        text(target, chip8.sound_timer > 0 ? "SOUND SIGNAL ON" : "SOUND SIGNAL OFF",
             rightX + 164, timersY + 88, 1, chip8.sound_timer > 0 ? amber : muted);
    }

    rectangle(target, 24, height - 56, width - 48, 1, borderColor);
    text(target, "F1 MODE   SPACE PAUSE   F2 STEP   F5 RESET   +/- SPEED   ESC EXIT",
         24, height - 42, 2, muted);
    std::string message = state.message.empty()
        ? "TIMERS FREEZE ON PAUSE / F2 RUNS ONE CPU CYCLE / FOCUS LOSS RELEASES ALL KEYS"
        : state.message;
    text(target, fit(message, static_cast<int>((width - 48) / 6)),
         24, height - 18, 1, state.message.empty() ? muted : amber);
}
