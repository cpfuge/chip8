#include "display.hpp"
#include <cstring>

Display::Display()
{
}

void Display::clear()
{
    std::memset(m_gfx, 0x00, sizeof(m_gfx));
}

void Display::set_pixel(uint8_t x, uint8_t y, uint8_t value)
{
    m_gfx[(Width * y) + x] = value;
}

uint8_t Display::get_pixel(uint16_t index)
{
    return m_gfx[index];
}
