#include "SerialWrapper.hpp"
#include <cassert>

namespace embedDIP
{

    SERIAL_CLASS_NAME::SERIAL_CLASS_NAME(serial_t *driver)
        : driver_(driver) {}

    void SERIAL_CLASS_NAME::init()
    {
        assert(driver_ && driver_->init);
        driver_->init();
    }

    void SERIAL_CLASS_NAME::flush()
    {
        assert(driver_ && driver_->flush);
        driver_->flush();
    }

    void SERIAL_CLASS_NAME::capture(Image &img)
    {
        assert(driver_ && driver_->capture);
        driver_->capture(img.raw());
    }

    void SERIAL_CLASS_NAME::send(const Image &img)
    {
        assert(driver_ && driver_->send);
        driver_->send(img.raw());
    }

    void SERIAL_CLASS_NAME::sendJPEG(const Image &img)
    {
        assert(driver_ && driver_->sendJPEG);
        driver_->sendJPEG(img.raw());
    }

    void SERIAL_CLASS_NAME::send1D(const void *data, uint8_t elem_size, uint32_t length, Serial1DDataType type)
    {
        assert(driver_ && driver_->send1D);
        driver_->send1D(data, elem_size, length, type);
    }

} // namespace embedDIP
