#pragma once

#include <cstdint>

class CPU
{
public:
    CPU();

    void init();
    void reset();
    void execute();
    void update_timers();
    bool load_rom_in_memory(const char* rom, uint32_t size);
    bool display_updated() { return m_display_updated; }
    void display_rendered() { m_display_updated = false; }
    const uint8_t* get_display() const { return m_display; }

    static inline constexpr auto MemorySize = 4096;
    static inline constexpr auto StackSize = 16;
    static inline constexpr auto FontSize = 80;
    static inline constexpr auto ResetVector = 0x200;
    static inline constexpr auto DisplayWidth = 64;
    static inline constexpr auto DisplayHeight = 32;
    static inline constexpr auto KeyCount = 16;

    struct Registers
    {
        uint16_t PC = 0x0000;
        uint16_t SP = 0x0000;
        uint16_t I = 0x0000;
        uint8_t V[16] = { 0 };
    };

private:
    Registers m_registers;
    uint16_t m_opcode = 0x0000;
    uint8_t m_memory[MemorySize] = { 0 };
    uint16_t m_stack[StackSize] = { 0 };
    uint8_t m_delay_timer = 0;
    uint8_t m_sound_timer = 0;
    static uint8_t m_font[FontSize];
    uint8_t m_display[DisplayWidth * DisplayHeight] = { 0 };
    bool m_display_updated = false;
    static int m_keymap[KeyCount];

    void stack_push(uint16_t value);
    uint16_t stack_pop();
    uint8_t read(uint16_t address);
    uint16_t read_word(uint16_t address);
    void CPU::write(uint16_t address, uint8_t value);
    void draw_pixel(uint16_t opcode);
    bool wait_key_press(uint16_t opcode);
    void fetch();
    void execute_instruction();
};
