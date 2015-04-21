#include <iostream>
#include <string>

#include "window.hpp"

Window::Window(unsigned int width, unsigned int height, std::string title)
{
    m_surface = NULL;
    createWindow(width, height, title);
}

Window::Window(const Image& image, std::string title)
{
    createWindow(image.getWidth(), image.getHeight(), title);
    setPixels(image);
}

Window::~Window()
{
    SDL_DestroyTexture(m_texture);
    
    SDL_RenderClear(m_renderer);
    SDL_DestroyRenderer(m_renderer);
    
    SDL_DestroyWindow(m_window);
}

void Window::createWindow(unsigned int width, unsigned int height, std::string title)
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
    
            m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, width, height);
        }
    }
    else
    {
        std::cerr << "Window could not be created, SDL Error " << SDL_GetError() << std::endl;
    }
}

void Window::setPixels(const Image& image)
{
    m_surface = image.getSurface();
}

void Window::render()
{
    if (m_surface != NULL)
    {
        SDL_UpdateTexture(m_texture, NULL, m_surface->pixels, m_surface->pitch);
    }
    SDL_RenderClear(m_renderer);
    SDL_RenderCopy(m_renderer, m_texture, NULL, NULL);
    SDL_RenderPresent(m_renderer);
}
