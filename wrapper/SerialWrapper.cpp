#include "SerialWrapper.hpp"
#include <cassert>

namespace embedDIP
{

    Serial::Serial(serial_t *driver)
        : driver_(driver) {}

    void Serial::init()
    {
        assert(driver_ && driver_->init);
        driver_->init();
    }

    void Serial::flush()
    {
        assert(driver_ && driver_->flush);
        driver_->flush();
    }

    void Serial::capture(Image &img)
    {
        assert(driver_ && driver_->capture);
        driver_->capture(img.raw());
    }

    void Serial::send(const Image &img)
    {
        assert(driver_ && driver_->send);
        driver_->send(img.raw());
    }

    void Serial::sendJPEG(const Image &img)
    {
        assert(driver_ && driver_->sendJPEG);
        driver_->sendJPEG(img.raw());
    }

    void Serial::send1D(const void *data, uint8_t elem_size, uint32_t length, Serial1DDataType type)
    {
        assert(driver_ && driver_->send1D);
        driver_->send1D(data, elem_size, length, type);
    }

} // namespace embedDIP
