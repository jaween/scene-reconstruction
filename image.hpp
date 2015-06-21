#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <SDL2/SDL.h>
#include <SDL2/SDL.h>

#include <string>

class Image
{
    public:
                Image();
		Image(unsigned int width, unsigned int height);
		Image(std::string filename);
		~Image();
		void load(std::string filename);
                void loadPGM(std::string filename);
                void savePGM(std::string prefix);
		void save(std::string prefix);
		void fill(unsigned int intensity);
		unsigned int getWidth() const;
		unsigned int getHeight() const;
		unsigned int getIntensity(unsigned int x, unsigned int y) const;
		void set(unsigned int x, unsigned int y, unsigned int intensity);
		SDL_Surface* getSurface() const;
		unsigned int* getPixels();
		void setPixels(unsigned int* pixel_data);

    private:
		void grayscale();
		std::string getDateTime();
		SDL_Surface* m_surface = NULL;

                char* pgm_data = NULL;
                unsigned int pgm_width;
                unsigned int pgm_height;
                unsigned int pgm_max_grey;
                unsigned int pgm_data_size;
};

#endif
