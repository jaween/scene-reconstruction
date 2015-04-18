#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <SDL2/SDL.h>
#include <SDL2/SDL.h>

#include <string>

class Image
{
    public:
		Image(unsigned int width, unsigned int height);
		Image(std::string filename);
		~Image();
		void load(std::string filename);
		void save(std::string prefix);
		void fill(unsigned int intensity);
		unsigned int getWidth() const;
		unsigned int getHeight() const;
		//unsigned int get(unsigned int x, unsigned int y) const;
		unsigned int getIntensity(unsigned int x, unsigned int y) const;
		void set(unsigned int x, unsigned int y, unsigned int intensity);
		SDL_Surface* getSurface() const;
		unsigned int* getPixels();
		void setPixels(unsigned int* pixel_data);

    private:
		void grayscale();
		std::string getDateTime();
		SDL_Surface* m_surface;
};

#endif
