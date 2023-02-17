#pragma once

#include <cstdint>

class Display
{
public:
    Display();

    inline static constexpr int Width = 64;
    inline static constexpr int Height = 32;

    void clear();
    void set_pixel(uint8_t x, uint8_t y, uint8_t value);
    uint8_t get_pixel(uint16_t index);

private:
    uint8_t m_gfx[Width * Height] = { 0 };
};
