#ifndef AUDIO_H
#define AUDIO_H

#include "Chip8.h"
#include "Frontend.h"
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>

class Audio
{
public:
    Audio();
    bool setupAudio();
    void update(const Chip8& chip8, const FrontendState& state);
    void stop();
    bool isPlaying() const;

private:
    // The buffer must outlive the sound that uses it (destruction is reversed).
    sf::SoundBuffer buffer;
    sf::Sound buzzer;
    bool ready = false;
};

#endif
