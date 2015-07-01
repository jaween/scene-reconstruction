#ifndef WINDOW_MANAGER_HPP
#define WINDOW_MANAGER_HPP

#include <vector>

#include "image.hpp"
#include "window.hpp"

class WindowManager
{
    public:
        ~WindowManager();
        virtual Window* createWindow(Image* image, const Window::PixelFormat& pixel_format, std::string title) = 0;
        virtual void refresh();

    protected:
        std::vector<Window*> windows;
};

#endif
