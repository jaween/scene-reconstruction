#include <iostream>
#include <string>

#include "window.hpp"

Window::Window(unsigned int width, unsigned int height, const PixelFormat& pixel_format, std::string title)
{
    m_surface = NULL;
    createWindow(width, height, pixel_format, title);
}

Window::Window(Image* image, const PixelFormat& pixel_format, std::string title)
{
    createWindow(image->getWidth(), image->getHeight(), pixel_format, title);
    m_image = image;
}

Window::~Window()
{
    SDL_DestroyTexture(m_texture);
    
    SDL_RenderClear(m_renderer);
    SDL_DestroyRenderer(m_renderer);
    
    SDL_DestroyWindow(m_window);
}

void Window::createWindow(unsigned int width, unsigned int height, const PixelFormat& pixel_format, std::string title)
{
    m_window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);
    if (m_window != NULL)
    {
        m_width = width;
        m_height = height;
    
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
    
            m_window_id = SDL_GetWindowID(m_window);

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

void Window::setImage(Image* image)
{
    m_image = image;
}

void Window::render()
{
    if (m_surface != NULL)
    {
        m_surface = m_image->getSurface();
        SDL_UpdateTexture(m_texture, NULL, m_surface->pixels, m_surface->pitch);
    }
    SDL_RenderClear(m_renderer);
    SDL_RenderCopy(m_renderer, m_texture, NULL, NULL);
    SDL_RenderPresent(m_renderer);
}
