#include "graphics_factory.hpp"
#include "image_memory.hpp"

GraphicsFactory::~GraphicsFactory()
{
        for(WindowManager* window_manager : window_managers)
        {
                delete window_manager;
        }

        for(Image* image : images)
        {
                delete image;
        }
}

Image* GraphicsFactory::createImageMemory(unsigned int width, unsigned int height, unsigned int words_per_pixel)
{
        Image* image = new ImageMemory(width, height, words_per_pixel);
        images.push_back(image);
        return image;
}
