#include "DisplayWrapper.hpp"

namespace embedDIP
{

    Display::Display(display_t *driver)
        : driver_(driver) {}

    void Display::init()
    {
        if (driver_ && driver_->init)
            driver_->init();
    }

    void Display::deinit()
    {
        if (driver_ && driver_->deinit)
            driver_->deinit();
    }

    void Display::reset()
    {
        if (driver_ && driver_->reset)
            driver_->reset();
    }

    void Display::clear(displayColor color)
    {
        if (driver_ && driver_->clear)
            driver_->clear(color);
    }

    void Display::show(const Image &image)
    {
        if (driver_ && driver_->show)
        {
            driver_->show(image.raw());
        }
    }

} // namespace embedDIP
