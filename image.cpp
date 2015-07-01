#include "image.hpp"

Image::Image()
{
        // Default empty constructor
}

Image::Image(unsigned int width, unsigned int height, unsigned int words_per_pixel)
{
        // No implementation (exists for external interface purposes)
}

unsigned int Image::getWidth()
{
        return m_width;
}

unsigned int Image::getHeight()
{
        return m_height;
}

unsigned int Image::getWordsPerPixel()
{
        return m_words_per_pixel;
}
