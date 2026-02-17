#include "SerialWrapper.hpp"

namespace embedDIP
{

    SERIAL_CLASS_NAME::SERIAL_CLASS_NAME(serial_t *driver)
        : driver_(driver) {}

    void SERIAL_CLASS_NAME::init()
    {
        if (!driver_ || !driver_->init) {
            return; // Silent failure - no exceptions in embedded systems
        }
        driver_->init();
    }

    void SERIAL_CLASS_NAME::flush()
    {
        if (!driver_ || !driver_->flush) {
            return; // Silent failure
        }
        driver_->flush();
    }

    void SERIAL_CLASS_NAME::capture(Image &img)
    {
        if (!driver_ || !driver_->capture) {
            return; // Silent failure
        }
        driver_->capture(img.raw());
    }

    void SERIAL_CLASS_NAME::send(const Image &img)
    {
        if (!driver_ || !driver_->send) {
            return; // Silent failure
        }
        driver_->send(img.raw());
    }

    void SERIAL_CLASS_NAME::sendJPEG(const Image &img)
    {
        if (!driver_ || !driver_->sendJPEG) {
            return; // Silent failure
        }
        driver_->sendJPEG(img.raw());
    }

    void SERIAL_CLASS_NAME::send1D(const void *data, uint8_t elem_size, uint32_t length, Serial1DDataType type)
    {
        if (!driver_ || !driver_->send1D) {
            return; // Silent failure
        }
        driver_->send1D(data, elem_size, length, type);
    }

} // namespace embedDIP
