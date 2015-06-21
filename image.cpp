#include <ctime>
#include <fstream>
#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>

#include "image.hpp"

Image::Image()
{
    // No implementation
}

Image::Image(unsigned int width, unsigned int height)
{
    unsigned int flag = 0;
    unsigned int depth = 32;
    unsigned int r_mask = 0x00FF0000;
    unsigned int g_mask = 0x0000FF00;
    unsigned int b_mask = 0x000000FF;
    unsigned int a_mask = 0xFF000000;
    m_surface = SDL_CreateRGBSurface(flag, width, height, depth, r_mask, g_mask, b_mask, a_mask);
}

Image::Image(std::string filename)
{
    load(filename);
    //grayscale();
}

Image::~Image()
{
    if (m_surface != NULL)
    {
        SDL_FreeSurface(m_surface);
    }

    if (pgm_data != NULL)
    {
        delete pgm_data;
    }
}

void Image::load(std::string filename)
{
    // Frees any existing image
    if (m_surface != NULL)
    {
        SDL_FreeSurface(m_surface);
    }
    
    m_surface = IMG_Load(filename.c_str());
    if (m_surface == NULL)
    {
        std::cerr << "Failed to load image" << std::endl;
    }
}

void Image::loadPGM(std::string filename)
{
    std::streampos begin, end;
    std::ifstream image(filename, std::ios::binary);
    std::string word;
    
    image >> word;
    if (word.compare("P5"))
    {
        std::cerr << "Didn't start with magic number 'P5'" << std::endl;
    }

    image >> word;
    pgm_width = std::stoi(word);
    
    image >> word;
    pgm_height = std::stoi(word);

    image >> word;
    pgm_max_grey = std::stoi(word);

    pgm_data_size = pgm_width * pgm_height * 2;
    pgm_data = new char[pgm_data_size];
    image.read(pgm_data, pgm_data_size); 

    image.close();
    
}

void Image::savePGM(std::string prefix)
{
    std::string date_time = getDateTime();
    std::string directory = "out/";
    std::string extension = ".pgm";
    
    std::string filename = directory + prefix + " " + date_time + extension;
    
    std::filebuf fb;
    fb.open(filename, std::ios::out);
    std::ostream image(&fb);

    image << "P5\n";
    image << pgm_width << " " << pgm_height << "\n";
    image << pgm_max_grey << "\n";
    image << *pgm_data;

    fb.close();
    std::cout << filename << std::endl;
}

void Image::save(std::string prefix)
{
    std::string date_time = getDateTime();
    std::string directory = "out/";
    std::string extension = ".bmp";
    
    std::string filename = directory + prefix + " " + date_time + extension;
    std::cout << filename << std::endl;
    SDL_SaveBMP(m_surface, filename.c_str());
}

void Image::fill(unsigned int intensity)
{
    unsigned int* pixels = (unsigned int*) m_surface->pixels;
    
    for (int y = 0; y < m_surface->h; y++)
    {
        for (int x = 0; x < m_surface->w; x++)
        {
            pixels[y * m_surface->w + x] = intensity;
        }
    }
}

unsigned int Image::getWidth() const
{
    return m_surface->w;
}

unsigned int Image::getHeight() const
{
    return m_surface->h;
}

/*unsigned int Image::get(unsigned int x, unsigned int y) const
{
    unsigned int* pixels = (unsigned int*) m_surface->pixels;
    return pixels[y * m_surface->w + x];
}*/

unsigned int Image::getIntensity(unsigned int x, unsigned int y) const
{
    unsigned int* pixels = (unsigned int*) m_surface->pixels;
    unsigned int pixel = pixels[y * m_surface->w + x] & 0x000000FF;
    return pixel;
}

void Image::set(unsigned int x, unsigned int y, unsigned int intensity)
{
    unsigned int* pixels = (unsigned int*) m_surface->pixels;
    pixels[y * m_surface->w + x] = intensity;
}

SDL_Surface* Image::getSurface() const
{
    return m_surface;
}

unsigned int* Image::getPixels()
{
    return (unsigned int*) m_surface->pixels;
}

// TODO: Confirm if this is necessary, we're passing a pointer to these pixels in getPixels() anyway!
void Image::setPixels(unsigned int* pixel_data)
{
    m_surface->pixels = pixel_data;
}

void Image::grayscale()
{
    unsigned int* pixels = (unsigned int*) m_surface->pixels;
    
    for (int y = 0; y < m_surface->h; y++)
    {
        for (int x = 0; x < m_surface->w; x++)
        {
            unsigned int pixel = pixels[y * m_surface->w + x];
            
            unsigned int r = pixel >> 16 & 0xFF;
            unsigned int g = pixel >> 8 & 0xFF;
            unsigned int b = pixel & 0xFF;
            unsigned int v = 0.212671f * r + 0.715160f * g + 0.072169f * b;
            
            pixel = (0xFF << 24) | (v << 16) | (v << 8) | v;
            pixels[y * m_surface->w + x] = pixel;
        }
    }
}

std::string Image::getDateTime()
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
