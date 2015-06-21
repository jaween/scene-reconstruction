#include <iostream>
#include <SDL2/SDL_image.h>

#include "window_manager.hpp"

WindowManager::WindowManager()
{
    initialiseSDL();
}

WindowManager::~WindowManager()
{
    for (Window* window : windows)
    {
        delete window;
    }
    SDL_Quit();
}

Window* WindowManager::createWindow(Image* image, const Window::PixelFormat& pixel_format, std::string title)
{
    Window* window = new Window(image, pixel_format, title);
    windows.push_back(window);
    return window;
}

void WindowManager::render()
{
    for (int i = 0; i < windows.size(); i++)
    {
	windows.at(i)->render();
    }
}

bool WindowManager::initialiseSDL()
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
