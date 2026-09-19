# CHIP-8 starter

A simple scaffold for Awais and Partner. You will implement the emulator yourselves. Most functions contain only TODO comments; `load()` returns false until you implement it. The current program only prints a starter message.

Style reference: [James Griffin's CHIP-8 emulator](https://github.com/JamesGriffin/CHIP-8-Emulator). We use SFML for graphics and input, and keep separate files so you can work independently.

## Files to work on

| File | Owner | Work |
| --- | --- | --- |
| `src/Chip8.h` | Both | Shared variables and function declarations; agree before editing |
| `src/Chip8.cpp` | Awais | Initialization, arithmetic, drawing, keypad instructions |
| `src/Chip8Execution.cpp` | Partner | Fetch/decode, flow control, stack operations, random |
| `src/Chip8Memory.cpp` | Partner | ROM loading and memory/index instructions |
| `src/Chip8Timers.cpp` | Partner | Timer instructions and 60 Hz updates |
| `src/Display.h/.cpp` | Awais | SFML window setup and drawing |
| `src/Input.h/.cpp` | Awais | SFML events and keyboard mapping |
| `src/main.cpp` | Both | Connect everything and write the main loop |
| `roms/` | Both | Put your `.ch8` files here |

## Simple syntax

- `uint8_t` holds a number from 0 to 255. `uint16_t` holds a number from 0 to 65535.
- `V[16]`, `memory[4096]`, `gfx[64 * 32]` and `key[16]` are ordinary arrays.
- `= {0}` starts an array with every element zero. `init()` still needs to reset state and install the font.
- `Chip8::arithmetic()` means the function belongs to the `Chip8` class. All the `Chip8*.cpp` files share the same class members.
- `load(const char* file_path)` accepts a file path such as `"roms/pong.ch8"` and returns true or false.
- `Chip8& chip8` passes the existing object to a function, so Input can update its keys. `const Chip8&` lets Display read the object.

No custom type aliases or exceptions are needed. Use ordinary loops, `if` statements and `switch` statements to fill in the TODOs.

## Connecting the instruction functions

Partner writes the fetch and switch/if logic inside `emulate_cycle()`. Store the fetched instruction in `opcode`, then call exactly one function from this table. Each function reads that same `opcode` to select its instruction and operands.

| Function | Instructions |
| --- | --- |
| `arithmetic()` | 6XNN, 7XNN, 8XY0, 8XY1, 8XY2, 8XY3, 8XY4, 8XY5, 8XY6, 8XY7, 8XYE |
| `drawing()` | 00E0, DXYN |
| `keypad()` | EX9E, EXA1, FX0A |
| `flow_control()` | 1NNN, 2NNN, 00EE, 3XNN, 4XNN, 5XY0, 9XY0, BNNN |
| `random_number()` | CXNN |
| `memory_instructions()` | ANNN, FX1E, FX29, FX33, FX55, FX65 |
| `timer_instructions()` | FX07, FX15, FX18 |

Check the full instruction pattern: for example, the F instructions belong to three different functions. Report unsupported instructions instead of running an unrelated function.

## Rules you both follow

- `load()` calls Awais's `init()` before loading a game. ROMs start at `0x200`; the font starts at `0x050`, with 5 bytes per character.
- Each instruction updates `pc` itself. Ordinary instructions add 2; a taken skip adds 4. `emulate_cycle()` does not also advance it.
- CALL saves the current `pc`; RETURN restores it and adds 2. `sp` is the next free stack slot. Ordinary jumps do not use the stack.
- FX0A leaves `pc` unchanged if no key is held. Return to the main loop while waiting. If several keys are held, choose the lowest key index.
- Main calls `update_timers()` at 60 Hz, independently of CPU speed. It continues during a key wait and never changes `pc`.
- Display reads `gfx[y * 64 + x]`. Initialization, clearing and drawing set `drawFlag`; main clears it after rendering.
- Input updates `chip8.key[index]` directly: 1 for pressed, 0 for released.
- Check memory, stack and key indices before using them. Print an error and stop emulation for invalid accesses.

Keep the earlier compatibility choices: shifts use Vx; BNNN uses V0; FX55/FX65 leave I unchanged; FX1E leaves VF unchanged; sprites wrap at both edges. Read arithmetic operands before writing the result and then VF. Logic operations do not separately clear VF.

## SFML and keyboard

Main will own one `sf::RenderWindow`. Pass it to `setupGraphics()`, `drawGraphics()` and `handleInput()`. The declarations in the headers allow the empty functions to compile before SFML is installed. Add the SFML includes and CMake dependency when you start that work.

```text
Keyboard                CHIP-8 keys
1 2 3 4                 1 2 3 C
Q W E R                 4 5 6 D
A S D F                 7 8 9 E
Z X C V                 A 0 B F
```

## Build

From PowerShell with the available MinGW compiler:

```powershell
New-Item -ItemType Directory -Force build
g++ -std=c++17 -Wall -Wpedantic src/*.cpp -o build/chip8.exe
./build/chip8.exe
```

Or with CMake installed:

```sh
cmake -S . -B build
cmake --build build
```
