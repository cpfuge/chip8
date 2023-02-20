#include "cpu.hpp"
#include <cstring>
#include <random>
#include <cassert>
#include <time.h>
#include <SDL.h>

uint8_t CPU::m_font[FontSize] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,
    0x20, 0x60, 0x20, 0x20, 0x70,
    0xF0, 0x10, 0xF0, 0x80, 0xF0,
    0xF0, 0x10, 0xF0, 0x10, 0xF0,
    0x90, 0x90, 0xF0, 0x10, 0x10,
    0xF0, 0x80, 0xF0, 0x10, 0xF0,
    0xF0, 0x80, 0xF0, 0x90, 0xF0,
    0xF0, 0x10, 0x20, 0x40, 0x40,
    0xF0, 0x90, 0xF0, 0x90, 0xF0,
    0xF0, 0x90, 0xF0, 0x10, 0xF0,
    0xF0, 0x90, 0xF0, 0x90, 0x90,
    0xE0, 0x90, 0xE0, 0x90, 0xE0,
    0xF0, 0x80, 0x80, 0x80, 0xF0,
    0xE0, 0x90, 0x90, 0x90, 0xE0,
    0xF0, 0x80, 0xF0, 0x80, 0xF0,
    0xF0, 0x80, 0xF0, 0x80, 0x80
};

int CPU::m_keymap[CPU::KeyCount] = {
    SDL_SCANCODE_1,
    SDL_SCANCODE_2,
    SDL_SCANCODE_3,
    SDL_SCANCODE_4,
    SDL_SCANCODE_Q,
    SDL_SCANCODE_W,
    SDL_SCANCODE_E,
    SDL_SCANCODE_R,
    SDL_SCANCODE_A,
    SDL_SCANCODE_S,
    SDL_SCANCODE_D,
    SDL_SCANCODE_F,
    SDL_SCANCODE_Z,
    SDL_SCANCODE_X,
    SDL_SCANCODE_C,
    SDL_SCANCODE_V
};

CPU::CPU()
{
    init();
}

void CPU::init()
{
    for (int index = 0; index < 16; index++)
        m_registers.V[index] = 0x00;

    std::memset(m_memory, 0x00, sizeof(m_memory));
    for (int index = 0; index < FontSize; index++)
        m_memory[index] = m_font[index];

    std::srand(time(NULL));
}

void CPU::reset()
{
    m_registers.PC = ResetVector;
    m_registers.SP = 0x00;
    m_registers.I = 0x00;
    m_opcode = 0x0000;
    m_delay_timer = 0;
    m_sound_timer = 0;

    std::memset(m_stack, 0x00, sizeof(m_stack));
    std::memset(m_display, 0x00, sizeof(m_display));
    m_display_updated = true;
}

void CPU::execute()
{
    fetch();
    execute_instruction();
}

bool CPU::load_rom_in_memory(const char* rom, uint32_t size)
{
    assert(rom);
    if ((MemorySize - ResetVector) < size)
        return false;

    init();

    for (int index = 0; index < size; index++)
        m_memory[index + ResetVector] = (uint8_t)rom[index];

    reset();

    return true;
}

void CPU::stack_push(uint16_t value)
{
    m_stack[m_registers.SP] = value;
    m_registers.SP++;
}

uint16_t CPU::stack_pop()
{
    m_registers.SP--;
    return m_stack[m_registers.SP];
}

uint8_t CPU::read(uint16_t address)
{
    assert(address < MemorySize);
    return m_memory[address];
}

uint16_t CPU::read_word(uint16_t address)
{
    return (read(address) << 8 | read(address + 1));
}

void CPU::write(uint16_t address, uint8_t value)
{
    assert(address < MemorySize);
    m_memory[address] = value;
}

void CPU::draw_pixel(uint16_t m_opcode)
{
    uint16_t x = m_registers.V[(m_opcode & 0x0F00) >> 8];
    uint16_t y = m_registers.V[(m_opcode & 0x00F0) >> 4];
    uint16_t height = m_opcode & 0x000F;

    m_registers.V[0xF] = 0;
    for (int line_y = 0; line_y < height; line_y++)
    {
        uint16_t pixel = m_memory[m_registers.I + line_y];
        for (int line_x = 0; line_x < 8; line_x++)
        {
            if ((pixel & (0x80 >> line_x)) != 0)
            {
                if (m_display[(x + line_x + ((y + line_y) * 64))] == 1)
                {
                    m_registers.V[0xF] = 1;
                }
                m_display[x + line_x + ((y + line_y) * 64)] ^= 1;
            }
        }
    }

    m_display_updated = true;
}

bool CPU::wait_key_press(uint16_t m_opcode)
{
    bool key_pressed = false;

    for (int index = 0; index < KeyCount; index++)
    {
        if (SDL_GetKeyboardState(nullptr)[m_keymap[index]])
        {
            m_registers.V[(m_opcode & 0x0F00) >> 8] = index;
            key_pressed = true;
        }
    }

    return key_pressed;
}

void CPU::fetch()
{
    m_opcode = read_word(m_registers.PC);
}

void CPU::execute_instruction()
{
    switch (m_opcode & 0xF000)
    {
    case 0x0000:
        switch (m_opcode & 0x000F)
        {
        case 0x0000:
            std::memset(m_display, 0x00, sizeof(m_display));
            m_display_updated = true;
            m_registers.PC += 2;
            break;

        case 0x000E:
            m_registers.PC = stack_pop();
            m_registers.PC += 2;
            break;
        }
        break;

    case 0x1000:
        m_registers.PC = m_opcode & 0x0FFF;
        break;

    case 0x2000:
        stack_push(m_registers.PC);
        m_registers.PC = m_opcode & 0x0FFF;
        break;

    case 0x3000:
        if (m_registers.V[(m_opcode & 0x0F00) >> 8] == (m_opcode & 0x00FF))
            m_registers.PC += 4;
        else
            m_registers.PC += 2;
        break;

    case 0x4000:
        if (m_registers.V[(m_opcode & 0x0F00) >> 8] != (m_opcode & 0x00FF))
            m_registers.PC += 4;
        else
            m_registers.PC += 2;
        break;

    case 0x5000:
        if (m_registers.V[(m_opcode & 0x0F00) >> 8] == m_registers.V[(m_opcode & 0x000F) >> 4])
            m_registers.PC += 4;
        else
            m_registers.PC += 2;
        break;
        break;

    case 0x6000:
        m_registers.V[(m_opcode & 0x0F00) >> 8] = m_opcode & 0x00FF;
        m_registers.PC += 2;
        break;

    case 0x7000:
        m_registers.V[(m_opcode & 0x0F00) >> 8] += m_opcode & 0x00FF;
        m_registers.PC += 2;
        break;

    case 0x8000:
        switch (m_opcode & 0x000F)
        {
        case 0x0000:
            m_registers.V[(m_opcode & 0x0F00) >> 8] = m_registers.V[(m_opcode & 0x00F0) >> 4];
            m_registers.PC += 2;
            break;

        case 0x0001:
            m_registers.V[(m_opcode & 0x0F00) >> 8] |= m_registers.V[(m_opcode & 0x00F0) >> 4];
            m_registers.PC += 2;
            break;

        case 0x0002:
            m_registers.V[(m_opcode & 0x0F00) >> 8] &= m_registers.V[(m_opcode & 0x00F0) >> 4];
            m_registers.PC += 2;
            break;

        case 0x0003:
            m_registers.V[(m_opcode & 0x0F00) >> 8] ^= m_registers.V[(m_opcode & 0x00F0) >> 4];
            m_registers.PC += 2;
            break;

        case 0x0004:
            m_registers.V[(m_opcode & 0x0F00) >> 8] += m_registers.V[(m_opcode & 0x00F0) >> 4];
            if (m_registers.V[(m_opcode & 0x00F0) >> 4] > (0xFF - m_registers.V[(m_opcode & 0x0F00) >> 8]))
                m_registers.V[0xF] = 1;
            else
                m_registers.V[0xF] = 0;
            m_registers.PC += 2;
            break;

        case 0x0005:
            if (m_registers.V[(m_opcode & 0x00F0) >> 4] > m_registers.V[(m_opcode & 0x0F00) >> 8])
                m_registers.V[0xF] = 0;
            else
                m_registers.V[0xF] = 1;
            m_registers.V[(m_opcode & 0x0F00) >> 8] -= m_registers.V[(m_opcode & 0x00F0) >> 4];
            m_registers.PC += 2;
            break;

        case 0x0006:
            m_registers.V[0xF] = m_registers.V[(m_opcode & 0x0F00) >> 8] & 0x1;
            m_registers.V[(m_opcode & 0x0F00) >> 8] >>= 1;
            m_registers.PC += 2;
            break;

        case 0x0007:
            if (m_registers.V[(m_opcode & 0x0F00) >> 8] > m_registers.V[(m_opcode & 0x00F0) >> 4])
                m_registers.V[0xF] = 0;
            else
                m_registers.V[0xF] = 1;
            m_registers.V[(m_opcode & 0x0F00) >> 8] = m_registers.V[(m_opcode & 0x00F0) >> 4] - m_registers.V[(m_opcode & 0x0F00) >> 8];
            m_registers.PC += 2;
            break;

        case 0x000E:
            m_registers.V[0xF] = m_registers.V[(m_opcode & 0x0F00) >> 8] >> 7;
            m_registers.V[(m_opcode & 0x0F00) >> 8] <<= 1;
            m_registers.PC += 2;
            break;
        }
        break;

    case 0x9000:
        if (m_registers.V[(m_opcode & 0x0F00) >> 8] != m_registers.V[(m_opcode & 0x00F0) >> 4])
            m_registers.PC += 4;
        else
            m_registers.PC += 2;
        break;

    case 0xA000:
        m_registers.I = m_opcode & 0x0FFF;
        m_registers.PC += 2;
        break;

    case 0xB000:
        m_registers.PC = (m_opcode & 0x0FFF) + m_registers.V[0];
        break;

    case 0xC000:
        m_registers.V[(m_opcode & 0x0F00) >> 8] = (std::rand() % (0xFF + 1)) & (m_opcode & 0x00FF);
        m_registers.PC += 2;
        break;

    case 0xD000:
        draw_pixel(m_opcode);
        m_registers.PC += 2;
        break;

    case 0xE000:
        switch (m_opcode & 0x00FF)
        {
        case 0x009E:
            if (SDL_GetKeyboardState(nullptr)[m_keymap[m_registers.V[(m_opcode & 0x0F00) >> 8]]])
                m_registers.PC += 4;
            else
                m_registers.PC += 2;
            break;

        case 0x00A1:
            if (!SDL_GetKeyboardState(nullptr)[m_keymap[m_registers.V[(m_opcode & 0x0F00) >> 8]]])
                m_registers.PC += 4;
            else
                m_registers.PC += 2;
            break;
        }
        break;

    case 0xF000:
        switch (m_opcode & 0x00FF)
        {
        case 0x0007:
            m_registers.V[(m_opcode & 0x0F00) >> 8] = m_delay_timer;
            m_registers.PC += 2;
            break;

        case 0x000A:
            if (!wait_key_press(m_opcode))
                return;
            m_registers.PC += 2;
            break;

        case 0x0015:
            m_delay_timer = m_registers.V[(m_opcode & 0x0F00) >> 8];
            m_registers.PC += 2;
            break;

        case 0x0018:
            m_sound_timer = m_registers.V[(m_opcode & 0x0F00) >> 8];
            m_registers.PC += 2;
            break;

        case 0x001E:
            if (m_registers.I + m_registers.V[(m_opcode & 0x0F00) >> 8] > 0xFFF)
                m_registers.V[0xF] = 1;
            else
                m_registers.V[0xF] = 0;
            m_registers.I += m_registers.V[(m_opcode & 0x0F00) >> 8];
            m_registers.PC += 2;
            break;

        case 0x0029:
            m_registers.I = m_registers.V[(m_opcode & 0x0F00) >> 8] * 0x5;
            m_registers.PC += 2;
            break;

        case 0x0033:
            m_memory[m_registers.I] = m_registers.V[(m_opcode & 0x0F00) >> 8] / 100;
            m_memory[m_registers.I + 1] = (m_registers.V[(m_opcode & 0x0F00) >> 8] / 10) % 10;
            m_memory[m_registers.I + 2] = m_registers.V[(m_opcode & 0x0F00) >> 8] % 10;
            m_registers.PC += 2;
            break;

        case 0x0055:
            for (int index = 0; index <= ((m_opcode & 0x0F00) >> 8); index++)
                m_memory[m_registers.I + index] = m_registers.V[index];

            m_registers.I += ((m_opcode & 0x0F00) >> 8) + 1;
            m_registers.PC += 2;
            break;

        case 0x0065:
            for (int index = 0; index <= ((m_opcode & 0x0F00) >> 8); index++)
                m_registers.V[index] = m_memory[m_registers.I + index];

            m_registers.I += ((m_opcode & 0x0F00) >> 8) + 1;
            m_registers.PC += 2;
            break;
        }
        break;
    }
}

void CPU::update_timers()
{
    if (m_delay_timer > 0)
        m_delay_timer--;

    if (m_sound_timer > 0)
        m_sound_timer--;
}
