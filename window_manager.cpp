#include <iostream>
#include <SDL2/SDL_image.h>

#include "window_manager.hpp"

WindowManager::WindowManager()
{
    // Initialises window
    bool result = initialiseSDL();
    if (result == false)
    {
	return;
    }
}

WindowManager::~WindowManager()
{
    SDL_Quit();
}

void WindowManager::addWindow(Window* window)
{
    windows.push_back(window);
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
