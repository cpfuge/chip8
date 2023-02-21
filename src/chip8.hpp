#pragma once

#include "cpu.hpp"
#include <cstdint>
#include <string>
#include <Windows.h>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

class Chip8
{
public:
    Chip8();
    ~Chip8();

    bool init();
    void run();

private:
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    SDL_Texture* m_screen_texture = nullptr;
    int m_window_width = 800;
    int m_window_height = 600;
    bool m_exit = false;
    bool m_rom_loaded = false;
    bool m_paused = false;
    uint32_t m_screen_buffer[CPU::DisplayWidth * CPU::DisplayHeight] = { 0 };

    CPU m_cpu;

    static inline constexpr auto TimersCycleDivision = 9;

    static inline constexpr auto MENU_ID_LOAD_ROM = 1;
    static inline constexpr auto MENU_ID_EXIT = 2;
    static inline constexpr auto MENU_ID_PAUSE_RESUME = 3;
    static inline constexpr auto MENU_ID_RESET = 4;

    HMENU m_menu_bar;
    HMENU m_file_menu;
    HMENU m_emulator_menu;

    void create_main_menu();
    void process_input();
    void update_screen_buffer();
    void render();

    void open_rom_file();
    void toggle_pause();
    void reset();
    HWND Chip8::get_window_handle(SDL_Window* window);
    std::string Chip8::open_file_dialog(SDL_Window* owner);
};
