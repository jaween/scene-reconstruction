#include "graphics_factory_sdl.hpp"
#include "image_sdl.hpp"
#include "window_manager_sdl.hpp"

WindowManager* GraphicsFactorySdl::createWindowManager()
{
        WindowManager* window_manager = new WindowManagerSdl();
        window_managers.push_back(window_manager);
        return window_manager;
}

Image* GraphicsFactorySdl::createImage(unsigned int width, unsigned int height, unsigned int words_per_pixel)
{
        Image* image = new ImageSdl(width, height, words_per_pixel);
        images.push_back(image);
        return image;
}
