cmake_minimum_required(VERSION 3.16)
project(chip8 LANGUAGES CXX)

add_executable(chip8
    src/main.cpp
    src/Chip8.cpp
    src/Chip8Execution.cpp
    src/Chip8Memory.cpp
    src/Chip8Timers.cpp
    src/Display.cpp
    src/Input.cpp
)
target_compile_features(chip8 PRIVATE cxx_std_17)
set_target_properties(chip8 PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(chip8 PRIVATE src)

# Awais: add find_package(SFML ...) and target_link_libraries here when
# implementing Display/Input. Agree on an SFML version before adding it.
