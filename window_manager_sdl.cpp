#include <iostream>
#include <SDL2/SDL_image.h>

#include "window_manager_sdl.hpp"

WindowManagerSdl::WindowManagerSdl()
{
    initialiseSDL();
}

WindowManagerSdl::~WindowManagerSdl()
{
    SDL_Quit();
}

Window* WindowManagerSdl::createWindow(Image* image, const Window::PixelFormat& pixel_format, std::string title)
{
    Window* window = new WindowSdl(image, pixel_format, title);
    windows.push_back(window);
    return window;
}

bool WindowManagerSdl::initialiseSDL()
{
    if (SDL_Init(SDL_INIT_VIDEO) == -1)
    {
        std::cerr << "Failed to initialise SDL" << std::endl;
        return false;
    }

    int flags = IMG_INIT_PNG;
    if ((IMG_Init(flags) & flags) == -1)
    {
        std::cerr << "Failed to initialise SDL_image" << std::endl;
        return false;
    }

    return true;
}
