#ifndef WINDOW_MANAGER
#define WINDOW_MANAGER

#include <SDL2/SDL.h>
#include <vector>

#include "window.hpp"

class WindowManager
{
    public:
        WindowManager();
        ~WindowManager();
        Window* createWindow(Image*, const Window::PixelFormat& pixel_format, std::string title);
        void render();

    private:
        bool initialiseSDL();
        std::vector<Window*> windows;
};

#endif
