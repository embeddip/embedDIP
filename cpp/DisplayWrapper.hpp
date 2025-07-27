#pragma once

extern "C"
{
#include "display.h"
}

#include "ImageWrapper.hpp" // embedDIP::Image

namespace embedDIP
{

    class Display
    {
    public:
        explicit Display(display_t *driver);

        void init();
        void deinit();
        void reset();
        void clear(displayColor color);
        void show(const Image &image);

    private:
        display_t *driver_;
    };

} // namespace embedDIP
