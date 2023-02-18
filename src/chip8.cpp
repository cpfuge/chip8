#include "chip8.hpp"
#include "utils.hpp"
#include <fstream>
#include <SDL.h>
#include <SDL_syswm.h>

Chip8::Chip8()
{
    m_key_map[0x0] = SDLK_x;
    m_key_map[0x1] = SDLK_1;
    m_key_map[0x2] = SDLK_2;
    m_key_map[0x3] = SDLK_3;
    m_key_map[0x4] = SDLK_q;
    m_key_map[0x5] = SDLK_w;
    m_key_map[0x6] = SDLK_e;
    m_key_map[0x7] = SDLK_a;
    m_key_map[0x8] = SDLK_s;
    m_key_map[0x9] = SDLK_d;
    m_key_map[0xA] = SDLK_z;
    m_key_map[0xB] = SDLK_c;
    m_key_map[0xC] = SDLK_4;
    m_key_map[0xD] = SDLK_r;
    m_key_map[0xE] = SDLK_f;
    m_key_map[0xF] = SDLK_v;
}

Chip8::~Chip8()
{
    SDL_DestroyTexture(m_screen_texture);
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

bool Chip8::init()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        std::string message = "SDL_Init error: " + std::string(SDL_GetError());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "CHIP-8", message.c_str(), nullptr);
        return false;
    }

    uint32_t window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN;
    m_window = SDL_CreateWindow("CHIP-8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_window_width, m_window_height, window_flags);
    if (!m_window)
    {
        std::string message = "SDL_CreateWindow error: " + std::string(SDL_GetError());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "CHIP-8", message.c_str(), nullptr);
        return false;
    }

    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_renderer)
    {
        std::string message = "SDL_CreateRenderer error: " + std::string(SDL_GetError());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "CHIP-8", message.c_str(), nullptr);
        return false;
    }

    m_screen_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, CPU::DisplayWidth, CPU::DisplayHeight);
    if (!m_screen_texture)
    {
        std::string message = "SDL_CreateTexture error: " + std::string(SDL_GetError());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "CHIP-8", message.c_str(), nullptr);
        return false;
    }

    create_main_menu();
    SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

    return true;
}

void Chip8::run()
{
    while (!m_exit)
    {
        if (m_rom_loaded)
            m_cpu.execute();

        process_input();
        render();
    }
}

void Chip8::create_main_menu()
{
    m_menu_bar = CreateMenu();
    m_file_menu = CreateMenu();

    AppendMenu(m_menu_bar, MF_POPUP, (UINT_PTR)m_file_menu, "File");

    AppendMenu(m_file_menu, MF_STRING, MENU_ID_LOAD_ROM, "Open ROM...\tCtr+O");
    AppendMenu(m_file_menu, MF_SEPARATOR, 0, "");
    AppendMenu(m_file_menu, MF_STRING, MENU_ID_EXIT, "Exit\tAlt+F4");

    HWND window_handle = get_window_handle(m_window);
    SetMenu(window_handle, m_menu_bar);
}

void Chip8::process_input()
{
    SDL_Event event {};

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_SYSWMEVENT:
            if (event.syswm.msg->msg.win.msg == WM_COMMAND)
            {
                if (LOWORD(event.syswm.msg->msg.win.wParam) == MENU_ID_LOAD_ROM)
                    open_rom_file();

                if (LOWORD(event.syswm.msg->msg.win.wParam) == MENU_ID_EXIT)
                    m_exit = true;
            }
            break;

        case SDL_QUIT:
            m_exit = true;
            break;

        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_o &&
                event.key.keysym.mod & KMOD_CTRL &&
                event.key.repeat == 0)
            {
                open_rom_file();
            }

            for (int index = 0; index < CPU::KeyCount; index++)
            {
                if (event.key.keysym.sym == m_key_map[index])
                    m_cpu.key_down(index);
            }
            break;

        case SDL_KEYUP:
            for (int index = 0; index < CPU::KeyCount; index++)
            {
                if (event.key.keysym.sym == m_key_map[index])
                    m_cpu.key_up(index);
            }
            break;

        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                m_window_width = event.window.data1;
                m_window_height = event.window.data2;
            }
            break;

        default:
            break;
        }
    }
}

void Chip8::update_screen_buffer()
{
    for (int index = 0; index < (CPU::DisplayWidth * CPU::DisplayHeight); index++)
    {
        uint8_t pixel = m_cpu.get_display()[index];
        m_screen_buffer[index] = (0xFF00FF00 * pixel) | 0xFF000000;
    }
}

void Chip8::render()
{
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_renderer);
    update_screen_buffer();
    SDL_UpdateTexture(m_screen_texture, nullptr, m_screen_buffer, CPU::DisplayWidth * sizeof(uint32_t));
    SDL_RenderCopy(m_renderer, m_screen_texture, nullptr, nullptr);
    SDL_RenderPresent(m_renderer);
}

void Chip8::open_rom_file()
{
    std::string path = open_file_dialog(m_window);
    if (path.empty())
        return;

    std::ifstream rom_file(path, std::ifstream::binary);
    if (!rom_file.is_open())
    {
        std::string message = "Cannot open ROM file " + path;
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "CHIP-8", message.c_str(), m_window);
        return;
    }

    uint32_t buffer_size = get_file_size(path);
    char* buffer = static_cast<char*>(malloc(sizeof(char) * buffer_size));
    if (!rom_file.read(buffer, buffer_size))
    {
        std::string message = "Cannot read ROM file " + path;
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "CHIP-8", message.c_str(), m_window);
        return;
    }

    if (!m_cpu.load_rom_in_memory(buffer, buffer_size))
    {
        std::string message = "Cannot load ROM file " + path + " into memory, size is " + std::to_string(buffer_size);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "CHIP-8", message.c_str(), m_window);
        return;
    }

    m_rom_loaded = true;
    free(buffer);
}

HWND Chip8::get_window_handle(SDL_Window* window)
{
    if (!window)
        return nullptr;

    SDL_SysWMinfo win_info {};
    SDL_VERSION(&win_info.version);
    SDL_GetWindowWMInfo(window, &win_info);

    return win_info.info.win.window;
}

std::string Chip8::open_file_dialog(SDL_Window* owner)
{
    constexpr auto PATH_MAX_SIZE = 256;

    CHAR file_name[PATH_MAX_SIZE] = { 0 };
    CHAR current_dir[PATH_MAX_SIZE] = { 0 };

    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(OPENFILENAME));

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = get_window_handle(owner);
    ofn.lpstrFile = file_name;
    ofn.nMaxFile = sizeof(file_name);
    ofn.lpstrFilter = "Chip8 ROM (*.chip8)\0 * .chip8\0All Files (*.*)\0 * .*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetCurrentDirectoryA(PATH_MAX_SIZE, current_dir))
        ofn.lpstrInitialDir = current_dir;

    if (GetOpenFileNameA(&ofn) == TRUE)
        return ofn.lpstrFile;

    return {};
}
