#ifndef GRAPHICS_FACTORY_SDL_HPP
#define GRAPHICS_FACTORY_SDL_HPP

#include <SDL2/SDL.h>

#include "graphics_factory.hpp"

class GraphicsFactorySdl : public GraphicsFactory
{
    public:
        virtual WindowManager* createWindowManager();
        virtual Image* createImage(unsigned int width, unsigned int height, unsigned int words_per_pixel);
};

#endif
