#include "image_memory.hpp"

ImageMemory::ImageMemory(unsigned int width, unsigned int height, unsigned int words_per_pixel)
{
        m_width = width;
        m_height = height;
        m_words_per_pixel = words_per_pixel;
        m_data = new uint32_t[width * height * words_per_pixel];
}

ImageMemory::~ImageMemory()
{
        delete [] m_data;
}

uint32_t* ImageMemory::getPixels()
{
        return m_data;
}

void ImageMemory::load(std::string filename)
{
        // No implementation
}

void ImageMemory::save(std::string prefix)
{
        // No implementation
}

void ImageMemory::fill(unsigned int colour)
{
        for (unsigned int y = 0; y < m_height; y++)
        {
                for (unsigned int x = 0; x < m_width; x++)
                {
                        // Fills multi-word pixels
                        unsigned int pixel_index = y * m_width + x;
                        for (unsigned int channel_index = 0; channel_index < m_words_per_pixel; channel_index++)
                        {
                                m_data[pixel_index + channel_index] = colour;
                        }
                }
        }
}
