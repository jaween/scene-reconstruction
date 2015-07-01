#ifndef GRAPHICS_FACTORY_HPP
#define GRAPHICS_FACTORY_HPP

#include "image.hpp"
#include "window_manager.hpp"

class GraphicsFactory
{
        public:
                ~GraphicsFactory();
                virtual WindowManager* createWindowManager() = 0;
                virtual Image* createImage(unsigned int width, unsigned int height, unsigned int words_per_pixel) = 0;
                virtual Image* createImageMemory(unsigned int width, unsigned int height, unsigned int words_per_pixel);

        protected:
                std::vector<WindowManager*> window_managers;
                std::vector<Image*> images;
};

#endif
