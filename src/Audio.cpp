#include "Audio.h"
#include <cstdint>
#include <vector>

Audio::Audio() : buzzer(buffer)
{
}

bool Audio::setupAudio()
{
    // 480 Hz square wave: 100 samples per period at 48,000 samples/second.
    // Store 0.1 seconds once, then loop it for as long as the timer is positive.
    std::vector<std::int16_t> samples(4800);
    for (std::size_t i = 0; i < samples.size(); ++i) {
        samples[i] = (i % 100 < 50) ? 8000 : -8000;
    }
    ready = buffer.loadFromSamples(samples.data(), samples.size(), 1, 48000,
                                   {sf::SoundChannel::Mono});
    if (!ready) return false;

    buzzer.setBuffer(buffer);
    buzzer.setLooping(true);
    buzzer.setVolume(20.f); // SFML volume is 0-100. Keep the buzzer gentle.
    return true;
}

void Audio::update(const Chip8& chip8, const FrontendState& state)
{
    if (!ready) return;
    bool shouldPlay = state.romLoaded && chip8.running && state.focused &&
                      !state.paused && chip8.sound_timer > 0;
    if (shouldPlay && !isPlaying()) {
        buzzer.play();
    } else if (!shouldPlay && isPlaying()) {
        buzzer.stop();
    }
    // If it is already playing, leave it alone instead of restarting each frame.
}

void Audio::stop()
{
    buzzer.stop();
}

bool Audio::isPlaying() const
{
    return buzzer.getStatus() == sf::SoundSource::Status::Playing;
}
