#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <string>

#include "image.hpp"

class Window
{
        public:
                enum PixelFormat {
                        RGB, ABGR, GREY
                };

		Window(Image* image, const PixelFormat& pixel_format, std::string title);
		virtual void refresh() = 0;

        protected:
                Image* m_image = NULL;
};

#endif
