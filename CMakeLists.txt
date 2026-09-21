cmake_minimum_required(VERSION 3.16)
project(chip8 LANGUAGES CXX)

# Use the same compiler/toolchain that your SFML package was built with.
find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System Audio)

set(CHIP8_CORE
    src/Chip8.cpp
    src/Chip8Execution.cpp
    src/Chip8Memory.cpp
    src/Chip8Timers.cpp
)
set(CHIP8_FRONTEND
    src/Display.cpp
    src/Input.cpp
    src/Frontend.cpp
    src/Controls.cpp
    src/Audio.cpp
)

add_executable(chip8 src/main.cpp ${CHIP8_CORE} ${CHIP8_FRONTEND})
target_compile_features(chip8 PRIVATE cxx_std_17)
set_target_properties(chip8 PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(chip8 PRIVATE src)
target_link_libraries(chip8 PRIVATE SFML::Graphics SFML::Window SFML::System SFML::Audio)

# Tests use synthetic instructions and do not need downloaded game ROMs.
option(CHIP8_BUILD_TESTS "Build frontend integration tests" OFF)
if(CHIP8_BUILD_TESTS)
    enable_testing()
    add_executable(frontend_tests tests/FrontendTests.cpp ${CHIP8_CORE} ${CHIP8_FRONTEND})
    target_compile_features(frontend_tests PRIVATE cxx_std_17)
    target_include_directories(frontend_tests PRIVATE src)
    target_link_libraries(frontend_tests PRIVATE SFML::Graphics SFML::Window SFML::System SFML::Audio)
    add_test(NAME frontend_tests COMMAND frontend_tests)
endif()
