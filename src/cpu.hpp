#pragma once

#include <cstdint>

class CPU
{
public:
    CPU();

    void reset();
    void execute();

    static inline constexpr auto MemorySize = 4096;
    static inline constexpr auto StackSize = 16;
    static inline constexpr auto FontsetSize = 80;
    static inline constexpr auto ResetVector = 0x200;

    struct Registers
    {
        uint16_t PC = 0x0000;
        uint16_t SP = 0x0000;
        uint16_t I = 0x0000;
        uint8_t V[16] = { 0 };
    };

private:
    Registers m_registers;
    uint8_t m_ram[MemorySize];
    uint16_t m_stack[StackSize];
    static uint8_t m_fontset[FontsetSize];
};
