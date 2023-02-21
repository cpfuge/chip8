#include "sound.hpp"
#include <string>

int calculate_offset(Sound* device, int sample, int channel)
{
    return (sample * sizeof(int16_t) * device->get_audio_spec().channels) + (channel * sizeof(int16_t));
}

static void write_data(uint8_t* buffer, double data)
{
    int16_t* b = (int16_t*)buffer;
    double range = (double)INT16_MAX - (double)INT16_MIN;
    double normalized = data * range / 2.0;
    *b = normalized;
}

static void audio_callback(void* userdata, uint8_t* stream, int size)
{
    Sound* device = (Sound*)userdata;

    for (int sample = 0; sample < device->get_audio_spec().samples; sample++)
    {
        double data = device->get_sample();

        for (int channel = 0; channel < device->get_audio_spec().channels; channel++)
        {
            int offset = calculate_offset(device, sample, channel);
            uint8_t* buffer = stream + offset;
            write_data(buffer, data);
        }
    }
}

Sound::Sound()
{
}

Sound::~Sound()
{
    SDL_CloseAudioDevice(m_audio_device);
}

bool Sound::init()
{
    SDL_AudioSpec audio;
    SDL_zero(audio);

    audio.freq = 44100;
    audio.format = AUDIO_S16;
    audio.samples = 512;
    audio.channels = 1;
    audio.callback = audio_callback;
    audio.userdata = this;

    m_audio_device = SDL_OpenAudioDevice(nullptr, 0, &audio, &m_audio_spec, 0);
    if (m_audio_device == 0)
    {
        std::string message = "SDL_OpenAudioDevice error: " + std::string(SDL_GetError());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "CHIP-8", message.c_str(), nullptr);
        return false;
    }

    return true;
}

void Sound::play()
{
    SDL_PauseAudioDevice(m_audio_device, 0);
}

void Sound::stop()
{
    SDL_PauseAudioDevice(m_audio_device, 1);
}

double Sound::get_sample()
{
    double sample_rate = (double)(m_audio_spec.freq);
    double period = sample_rate / 800;

    m_position++;
    if (m_position % (int)period == 0)
        m_position = 0;

    double angular_freq = (1.0 / period) * 2.0 * M_PI;

    return sin(m_position * angular_freq);
}
