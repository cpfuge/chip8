#pragma once

#include <cstdint>
#include <SDL.h>

class Sound
{
public:
    Sound();
    ~Sound();

    bool init();
    void play();
    void stop();

    const SDL_AudioSpec& get_audio_spec() const { return m_audio_spec; }
    double get_sample();

private:
    SDL_AudioSpec m_audio_spec {};
    SDL_AudioDeviceID m_audio_device = 0;
    int m_position = 0;
};
