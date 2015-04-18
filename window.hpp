#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <SDL2/SDL.h>
#include <string>

#include "image.hpp"

class Window
{
    public:
		Window(unsigned int width, unsigned int height, std::string title);
		Window(const Image& image, std::string title);
		~Window();
		void setPixels(const Image& image);
		void render();

    private:
		void createWindow(unsigned int width, unsigned int height, std::string title);
		SDL_Window* m_window;
		SDL_Renderer* m_renderer;
		SDL_Texture* m_texture;
		SDL_Surface* m_surface;
		int m_window_id;
	
		unsigned int m_width;
		unsigned int m_height;
		unsigned long currentTimeMillis();
};

#endif
