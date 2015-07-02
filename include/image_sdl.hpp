#ifndef IMAGE_SDL_HPP
#define IMAGE_SDL_HPP

#include <SDL2/SDL.h>
#include <SDL2/SDL.h>
#include <string>

#include "image.hpp"

// Concrete implementation of Image the class, for use with SDL image data
class ImageSdl : public Image
{
        public:
                ImageSdl(unsigned int width, unsigned int height, unsigned int words_per_pixel);
                ImageSdl(std::string filename);
                ~ImageSdl();
                virtual uint32_t* getPixels();
                SDL_Surface* getSurface();
                virtual void load(std::string filename);
                virtual void save(std::string prefix);
                virtual void fill(unsigned int colour);

        private:
                void grayscale();
                std::string getDateTime();

                SDL_Surface* m_surface = NULL;
};

#endif
