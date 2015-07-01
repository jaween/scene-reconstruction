#ifndef IMAGE_MEMORY_HPP
#define IMAGE_MEMORY_HPP

#include "image.hpp"

// Concrete implementation of Image the class, for use library independent image data
class ImageMemory : public Image
{
        public:
                ImageMemory(unsigned int width, unsigned int height, unsigned int words_per_pixel);
                ~ImageMemory();
                virtual uint32_t* getPixels();
                virtual void load(std::string filename);
		virtual void save(std::string prefix);
		virtual void fill(unsigned int colour);

        private:
                uint32_t* m_data = NULL;
};

#endif
