#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <cstddef>
#include <cstdint>
#include <string>

// Abstract base class for image data tied to various graphics libaries
class Image
{
        public:
                Image();
                Image(unsigned int width, unsigned int height, unsigned int words_per_pixel);
                virtual unsigned int getWidth();
                virtual unsigned int getHeight();
                virtual unsigned int getWordsPerPixel();
                virtual uint32_t* getPixels() = 0;
                virtual void load(std::string filename) = 0;
                virtual void save(std::string prefix) = 0;
                virtual void fill(unsigned int colour) = 0;

        protected:
                unsigned int m_width = 0;
                unsigned int m_height = 0;
                unsigned int m_words_per_pixel = 1;
};

#endif
