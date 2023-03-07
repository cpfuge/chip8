#include "emulator.hpp"
#include "utils.hpp"

#include <fstream>
#include <thread>
#include <SDL.h>
#include <SDL_syswm.h>

Emulator::Emulator()
{
}

Emulator::~Emulator()
{
    SDL_DestroyTexture(m_screen_texture);
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

bool Emulator::init()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        std::string message = "SDL_Init error: " + std::string(SDL_GetError());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, m_window_title.c_str(), message.c_str(), nullptr);
        return false;
    }

    uint32_t window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN;
    m_window = SDL_CreateWindow(m_window_title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_window_width, m_window_height, window_flags);
    if (!m_window)
    {
        std::string message = "SDL_CreateWindow error: " + std::string(SDL_GetError());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, m_window_title.c_str(), message.c_str(), nullptr);
        return false;
    }

    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
    if (!m_renderer)
    {
        std::string message = "SDL_CreateRenderer error: " + std::string(SDL_GetError());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, m_window_title.c_str(), message.c_str(), nullptr);
        return false;
    }

    m_screen_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, CHIP8::DisplayWidth, CHIP8::DisplayHeight);
    if (!m_screen_texture)
    {
        std::string message = "SDL_CreateTexture error: " + std::string(SDL_GetError());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, m_window_title.c_str(), message.c_str(), nullptr);
        return false;
    }

    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

    if (!m_chip8.init())
        return false;

    create_main_menu();

    return true;
}

void Emulator::run()
{
    int cycles = 0;

    while (!m_exit)
    {
        if (m_rom_loaded && !m_paused)
        {
            m_chip8.execute();
            cycles++;
        }

        process_input();
        if (m_chip8.display_updated())
            render();

        if (cycles == TimersCycleDivision)
        {
            m_chip8.update_timers();
            cycles = 0;
        }

        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
}

void Emulator::create_main_menu()
{
    m_menu_bar = CreateMenu();
    m_file_menu = CreateMenu();
    m_emulator_menu = CreateMenu();

    AppendMenu(m_menu_bar, MF_POPUP, (UINT_PTR)m_file_menu, "File");
    AppendMenu(m_menu_bar, MF_POPUP, (UINT_PTR)m_emulator_menu, "Emulator");

    AppendMenu(m_file_menu, MF_STRING, MENU_ID_LOAD_ROM, "Open ROM...\tCtr+O");
    AppendMenu(m_file_menu, MF_SEPARATOR, 0, "");
    AppendMenu(m_file_menu, MF_STRING, MENU_ID_EXIT, "Exit\tAlt+F4");

    AppendMenu(m_emulator_menu, MF_STRING, MENU_ID_PAUSE_RESUME, "Pause\tCtr+P");
    AppendMenu(m_emulator_menu, MF_SEPARATOR, 0, "");
    AppendMenu(m_emulator_menu, MF_STRING, MENU_ID_RESET, "Reset\tCtr+R");

    HWND window_handle = get_window_handle(m_window);
    SetMenu(window_handle, m_menu_bar);
}

void Emulator::process_input()
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

                if (LOWORD(event.syswm.msg->msg.win.wParam) == MENU_ID_PAUSE_RESUME)
                    toggle_pause();

                if (LOWORD(event.syswm.msg->msg.win.wParam) == MENU_ID_RESET)
                    reset();
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

            if (event.key.keysym.sym == SDLK_p &&
                event.key.keysym.mod & KMOD_CTRL &&
                event.key.repeat == 0)
            {
                toggle_pause();
            }

            if (event.key.keysym.sym == SDLK_r &&
                event.key.keysym.mod & KMOD_CTRL &&
                event.key.repeat == 0)
            {
                reset();
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

void Emulator::update_screen_buffer()
{
    for (int index = 0; index < (CHIP8::DisplayWidth * CHIP8::DisplayHeight); index++)
    {
        uint8_t pixel = m_chip8.get_display()[index];
        m_screen_buffer[index] = (0x00FFFF00 * pixel) | 0xFF000000;
    }
}

void Emulator::render()
{
    SDL_RenderClear(m_renderer);
    update_screen_buffer();
    SDL_UpdateTexture(m_screen_texture, nullptr, m_screen_buffer, CHIP8::DisplayWidth * sizeof(uint32_t));
    SDL_RenderCopy(m_renderer, m_screen_texture, nullptr, nullptr);
    SDL_RenderPresent(m_renderer);
    m_chip8.display_rendered();
}

void Emulator::open_rom_file()
{
    std::string path = open_file_dialog(m_window);
    if (path.empty())
        return;

    std::ifstream rom_file(path, std::ifstream::binary);
    if (!rom_file.is_open())
    {
        std::string message = "Cannot open ROM file " + path;
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, m_window_title.c_str(), message.c_str(), m_window);
        return;
    }

    uint32_t buffer_size = get_file_size(path);
    char* buffer = static_cast<char*>(malloc(sizeof(char) * buffer_size));
    if (!rom_file.read(buffer, buffer_size))
    {
        std::string message = "Cannot read ROM file " + path;
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, m_window_title.c_str(), message.c_str(), m_window);
        return;
    }

    if (!m_chip8.load_rom_in_memory(buffer, buffer_size))
    {
        std::string message = "Cannot load ROM file " + path + " into memory, size is " + std::to_string(buffer_size);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, m_window_title.c_str(), message.c_str(), m_window);
        return;
    }


    if (m_paused)
        toggle_pause();

    m_window_title = "CHIP-8 [" + path + "]";
    set_window_title(m_window_title + " Running");
    m_rom_loaded = true;
    free(buffer);
}

void Emulator::toggle_pause()
{
    if (!m_rom_loaded)
        return;

    m_paused = !m_paused;

    if (m_paused)
    {
        set_window_title(m_window_title + " Paused");
        ModifyMenu(m_emulator_menu, MENU_ID_PAUSE_RESUME, MF_STRING, MENU_ID_PAUSE_RESUME, "Resume\tCrt+P");
    }
    else
    {
        set_window_title(m_window_title + " Running");
        ModifyMenu(m_emulator_menu, MENU_ID_PAUSE_RESUME, MF_STRING, MENU_ID_PAUSE_RESUME, "Pause\tCrt+P");
    }
}

void Emulator::reset()
{
    if (!m_rom_loaded)
        return;

    if (m_paused)
        toggle_pause();

    m_chip8.reset();
}

void Emulator::set_window_title(const std::string& title)
{
    if (!m_window)
        return;

    SDL_SetWindowTitle(m_window, title.c_str());
}

HWND Emulator::get_window_handle(SDL_Window* window)
{
    if (!window)
        return nullptr;

    SDL_SysWMinfo win_info {};
    SDL_VERSION(&win_info.version);
    SDL_GetWindowWMInfo(window, &win_info);

    return win_info.info.win.window;
}

std::string Emulator::open_file_dialog(SDL_Window* owner)
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
    ofn.lpstrFilter = "Chip8 ROM (*.ch8)\0 *.ch8\0All Files (*.*)\0 * .*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetCurrentDirectoryA(PATH_MAX_SIZE, current_dir))
        ofn.lpstrInitialDir = current_dir;

    if (GetOpenFileNameA(&ofn) == TRUE)
        return ofn.lpstrFile;

    return {};
}
