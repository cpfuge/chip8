#include "emulator.hpp"
#include <Windows.h>

int application_main(int argc, char* argv[])
{
    Emulator chip8;
    if (!chip8.init())
        return -1;
    chip8.run();

    return 0;
}

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nShowCmd;

    return application_main(__argc, __argv);
}
