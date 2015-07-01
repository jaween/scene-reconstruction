#include <ctime>
#include <fstream>
#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>

#include "image_sdl.hpp"

ImageSdl::ImageSdl(unsigned int width, unsigned int height, unsigned int words_per_pixel)
{
        m_words_per_pixel = 1;
        unsigned int flag = 0;
        unsigned int depth = 32;
        unsigned int r_mask = 0x00FF0000;
        unsigned int g_mask = 0x0000FF00;
        unsigned int b_mask = 0x000000FF;
        unsigned int a_mask = 0xFF000000;
        m_surface = SDL_CreateRGBSurface(flag, width, height, depth, r_mask, g_mask, b_mask, a_mask);

        m_width = m_surface->w;
        m_height = m_surface->h;
        m_words_per_pixel = 1;
}

ImageSdl::ImageSdl(std::string filename)
{
        load(filename);
}

ImageSdl::~ImageSdl()
{
        SDL_FreeSurface(m_surface);
}

uint32_t* ImageSdl::getPixels()
{
        return (uint32_t*) m_surface->pixels;
}

SDL_Surface* ImageSdl::getSurface()
{
        return m_surface;
}

void ImageSdl::load(std::string filename)
{
        // Frees any existing image
        if (m_surface != NULL)
        {
                SDL_FreeSurface(m_surface);
        }

        // Loads in the image
        m_surface = IMG_Load(filename.c_str());
        if (m_surface == NULL)
        {
                std::cerr << "Failed to load image" << std::endl;
        }

        // Saves the dimensions of the image
        m_width = m_surface->w;
        m_height = m_surface->h;
        m_words_per_pixel = 1;
}

void ImageSdl::save(std::string prefix)
{
        std::string date_time = getDateTime();
        std::string directory = "out/";
        std::string extension = ".bmp";

        std::string filename = directory + prefix + " " + date_time + extension;
        std::cout << filename << std::endl;
        SDL_SaveBMP(m_surface, filename.c_str());
}

void ImageSdl::fill(unsigned int colour)
{
        for (unsigned int y = 0; y < m_height; y++)
        {
                for (unsigned int x = 0; x < m_width; x++)
                {
                    unsigned int index = y * m_width + x;
                    ((unsigned int*) m_surface->pixels)[index] = colour;
                }
        }
}

void ImageSdl::grayscale()
{
        unsigned int* pixels = (unsigned int*) m_surface->pixels;

        for (unsigned int y = 0; y < m_height; y++)
        {
                for (unsigned int x = 0; x < m_width; x++)
                {
                        unsigned int index = y * m_width + x;
                        unsigned int pixel = pixels[index];

                        unsigned int r = pixel >> 16 & 0xFF;
                        unsigned int g = pixel >> 8 & 0xFF;
                        unsigned int b = pixel & 0xFF;
                        unsigned int v = 0.212671f * r + 0.715160f * g + 0.072169f * b;
                        unsigned int gray_pixel = (0xFF << 24) | (v << 16) | (v << 8) | v;

                        pixels[index] = gray_pixel;
                }
        }
}

std::string ImageSdl::getDateTime()
{
        time_t rawtime;
        struct tm * timeinfo;
        int size = 80;
        char buffer[size];

        time(&rawtime);
        timeinfo = localtime(&rawtime);

        strftime(buffer, size, "%y-%m-%d %T", timeinfo);
        return std::string(buffer);
}
