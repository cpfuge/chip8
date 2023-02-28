#include "cpu.hpp"
#include "sound.hpp"
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
    m_delay_timer = 0;
    m_sound_timer = 0;

    m_opcode.type = 0;
    m_opcode.x = 0;
    m_opcode.y = 0;
    m_opcode.n = 0;
    m_opcode.kk = 0;
    m_opcode.nnn = 0;

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

void CPU::draw_pixel()
{
    auto x = m_registers.V[m_opcode.x];
    auto y = m_registers.V[m_opcode.y];
    auto height = m_opcode.n;

    m_registers.V[0xF] = 0;
    for (uint8_t row = 0; row < height; row++)
    {
        uint8_t data = m_memory[m_registers.I + row];
        for (uint8_t column = 0; column < 8; column++)
        {
            if ((data & 0x80) != 0)
            {
                if (m_display[(x + column + ((y + row) * 64))] == 1)
                {
                    m_registers.V[0xF] = 1;
                }
                m_display[x + column + ((y + row) * 64)] ^= 1;
            }

            data <<= 1;
        }
    }

    m_display_updated = true;
}

bool CPU::wait_key_press()
{
    bool key_pressed = false;

    for (int index = 0; index < KeyCount; index++)
    {
        if (SDL_GetKeyboardState(nullptr)[m_keymap[index]])
        {
            m_registers.V[m_opcode.x] = index;
            key_pressed = true;
        }
    }

    return key_pressed;
}

void CPU::fetch()
{
    uint16_t value = read_word(m_registers.PC);

    // Decode instruction
    m_opcode.type = (value >> 12) & 0x000F;
    m_opcode.x = (value >> 8) & 0x000F;
    m_opcode.y = (value >> 4) & 0x000F;
    m_opcode.n = value & 0x000F;
    m_opcode.kk = value & 0x00FF;
    m_opcode.nnn = value & 0x0FFF;

    // Increment program counter
    m_registers.PC += 2;
}

void CPU::execute_instruction()
{
    switch (m_opcode.type)
    {
    case 0x0:
        switch (m_opcode.nnn)
        {
        case 0x0E0:
            std::memset(m_display, 0x00, sizeof(m_display));
            m_display_updated = true;
            break;

        case 0x00EE:
            m_registers.PC = stack_pop();
            break;
        }
        break;

    case 0x1:
        m_registers.PC = m_opcode.nnn;
        break;

    case 0x2:
        stack_push(m_registers.PC);
        m_registers.PC = m_opcode.nnn;
        break;

    case 0x3:
        if (m_registers.V[m_opcode.x] == m_opcode.kk)
            m_registers.PC += 2;
        break;

    case 0x4:
        if (m_registers.V[m_opcode.x] != m_opcode.kk)
            m_registers.PC += 2;
        break;

    case 0x5:
        if (m_registers.V[m_opcode.x] == m_registers.V[m_opcode.y])
            m_registers.PC += 2;
        break;

    case 0x6:
        m_registers.V[m_opcode.x] = m_opcode.kk;
        break;

    case 0x7:
        m_registers.V[m_opcode.x] += m_opcode.kk;
        break;

    case 0x8:
        switch (m_opcode.n)
        {
        case 0x0:
            m_registers.V[m_opcode.x] = m_registers.V[m_opcode.y];
            break;

        case 0x1:
            m_registers.V[m_opcode.x] |= m_registers.V[m_opcode.y];
            break;

        case 0x2:
            m_registers.V[m_opcode.x] &= m_registers.V[m_opcode.y];
            break;

        case 0x3:
            m_registers.V[m_opcode.x] ^= m_registers.V[m_opcode.y];
            break;

        case 0x4:
            m_registers.V[0xF] = (((uint16_t)m_registers.V[m_opcode.x] + (uint16_t)m_registers.V[m_opcode.y]) > 0xFF) ? 1 : 0;
            m_registers.V[m_opcode.x] += m_registers.V[m_opcode.y];
            break;

        case 0x5:
            m_registers.V[0xF] = (m_registers.V[m_opcode.x] > m_registers.V[m_opcode.y]) ? 1 : 0;
            m_registers.V[m_opcode.x] -= m_registers.V[m_opcode.y];
            break;

        case 0x6:
            m_registers.V[0xF] = m_registers.V[m_opcode.x] & 1;
            m_registers.V[m_opcode.x] >>= 1;
            break;

        case 0x7:
            m_registers.V[0xF] = (m_registers.V[m_opcode.y] > m_registers.V[m_opcode.x]) ? 1 : 0;
            m_registers.V[m_opcode.x] = m_registers.V[m_opcode.y] - m_registers.V[m_opcode.x];
            break;

        case 0xE:
            m_registers.V[0xF] = (m_registers.V[m_opcode.y] >> 7) & 1;
            m_registers.V[m_opcode.x] = m_registers.V[m_opcode.y] << 1;
            break;
        }
        break;

    case 0x9:
        if (m_registers.V[m_opcode.x] != m_registers.V[m_opcode.y])
            m_registers.PC += 2;
        break;

    case 0xA:
        m_registers.I = m_opcode.nnn;
        break;

    case 0xB:
        m_registers.PC = m_opcode.nnn + m_registers.V[0];
        break;

    case 0xC:
        m_registers.V[m_opcode.x] = (std::rand() % 0xFF) & m_opcode.kk;
        break;

    case 0xD:
        draw_pixel();
        break;

    case 0xE:
        switch (m_opcode.kk)
        {
        case 0x9E:
            if (SDL_GetKeyboardState(nullptr)[m_keymap[m_registers.V[m_opcode.x] & 15]])
                m_registers.PC += 2;
            break;

        case 0xA1:
            if (!SDL_GetKeyboardState(nullptr)[m_keymap[m_registers.V[m_opcode.x] & 15]])
                m_registers.PC += 2;
            break;
        }
        break;

    case 0xF:
        switch (m_opcode.kk)
        {
        case 0x07:
            m_registers.V[m_opcode.x] = m_delay_timer;
            break;

        case 0x0A:
            if (!wait_key_press())
                m_registers.PC -= 2;
                return;
            break;

        case 0x15:
            m_delay_timer =m_registers.V[m_opcode.x];
            break;

        case 0x18:
            m_sound_timer = m_registers.V[m_opcode.x];
            break;

        case 0x1E:
            m_registers.V[0xF] = ((m_registers.I + m_registers.V[m_opcode.x]) > 0xFFF) ? 1 : 0;
            m_registers.I += m_registers.V[m_opcode.x];
            break;

        case 0x29:
            m_registers.I = m_registers.V[m_opcode.x] * 5;
            break;

        case 0x33:
            m_memory[m_registers.I & 0xFFF] = (m_registers.V[m_opcode.x] % 1000) / 100;
            m_memory[(m_registers.I + 1) & 0xFFF] =  (m_registers.V[m_opcode.x] % 10) / 10;
            m_memory[(m_registers.I + 2) & 0xFFF] = m_registers.V[m_opcode.x] % 10;
            break;

        case 0x55:
            for (int index = 0; index <= m_opcode.x; index++)
                m_memory[(m_registers.I++) & 0xFFF] = m_registers.V[index];
            break;

        case 0x65:
            for (int index = 0; index <= m_opcode.x; index++)
                m_registers.V[index] = m_memory[(m_registers.I++) & 0xFFF];
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
    {
        if (m_sound_device)
            m_sound_device->play();
        m_sound_timer--;
    }
    else
    {
        if (m_sound_device)
            m_sound_device->stop();
    }
}
