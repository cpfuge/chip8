#pragma once

#include "cpu.hpp"
#include "display.hpp"
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
    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    SDL_Texture* m_screen_texture;
    int m_window_width;
    int m_window_height;
    bool m_exit;
    uint32_t m_screen_buffer[Display::Width * Display::Height] = { 0 };

    CPU m_cpu;
    Display m_display;

    static inline constexpr auto MENU_ID_LOAD_ROM = 1;
    static inline constexpr auto MENU_ID_EXIT = 2;

    HMENU m_menu_bar;
    HMENU m_file_menu;

    void create_main_menu();
    void process_input();
    void update_screen_buffer();
    void render();

    void open_rom_file();
    HWND Chip8::get_window_handle(SDL_Window* window);
    std::string Chip8::open_file_dialog(SDL_Window* owner);
};
