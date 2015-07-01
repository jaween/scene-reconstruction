#include <iostream>
#include <string>

#include "window_sdl.hpp"

WindowSdl::WindowSdl(Image* image, const PixelFormat& pixel_format, std::string title) : Window(image, pixel_format, title)
{
	createWindow(image->getWidth(), image->getHeight(), pixel_format, title);
}

WindowSdl::~WindowSdl()
{
	SDL_DestroyTexture(m_texture);

	SDL_RenderClear(m_renderer);
	SDL_DestroyRenderer(m_renderer);

	SDL_DestroyWindow(m_window);
}

void WindowSdl::createWindow(unsigned int width, unsigned int height, const PixelFormat& pixel_format, std::string title)
{
	m_window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);
	if (m_window != NULL)
	{
		unsigned int options = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
		m_renderer = SDL_CreateRenderer(m_window, -1, options);
		if (m_renderer == NULL)
		{
			std::cerr << "Failed to create renderer, SDL error " << SDL_GetError() << std::endl;
			m_window = NULL;
		}
		else
		{
			SDL_SetRenderDrawColor(m_renderer, 0xFF, 0xFF, 0xFF, 0xFF);

			unsigned int sdl_pixel_format;
			switch (pixel_format)
			{
				case RGB:
					sdl_pixel_format = SDL_PIXELFORMAT_RGB24;
					break;
				case ABGR:
					sdl_pixel_format = SDL_PIXELFORMAT_ABGR8888;
					break;
				case GREY:
				default:
					std::cout << "Window: Setting pixel format to ABGR8888" << std::endl;
					sdl_pixel_format = SDL_PIXELFORMAT_ABGR8888;
			}
			m_texture = SDL_CreateTexture(m_renderer, sdl_pixel_format, SDL_TEXTUREACCESS_STREAMING, width, height);
		}
	}
	else
	{
		std::cerr << "Window could not be created, SDL Error " << SDL_GetError() << std::endl;
	}
}

void WindowSdl::refresh()
{
	// ?? To do: Is this the way concrete classes created from a factory access extended functionality?
	SDL_Surface* surface = static_cast<ImageSdl*>(m_image)->getSurface();
	if (surface != NULL)
	{
		SDL_UpdateTexture(m_texture, NULL, surface->pixels, surface->pitch);
	}

	// Renders using the GPU
	SDL_RenderClear(m_renderer);
	SDL_RenderCopy(m_renderer, m_texture, NULL, NULL);
	SDL_RenderPresent(m_renderer);
}
