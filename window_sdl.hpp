#ifndef WINDOW_SDL_HPP
#define WINDOW_SDL_HPP

#include <string>

#include "image_sdl.hpp"
#include "window.hpp"

class WindowSdl : public Window
{
        public:
		WindowSdl(Image* image, const PixelFormat& pixel_format, std::string title);
		~WindowSdl();
		virtual void refresh();

        private:
		void createWindow(unsigned int width, unsigned int height, const PixelFormat& pixel_format, std::string title);
                SDL_Window* m_window = NULL;
                SDL_Texture* m_texture = NULL;
                SDL_Renderer* m_renderer = NULL;
};

#endif
