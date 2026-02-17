#include "SerialWrapper.hpp"
#include <stdexcept>

namespace embedDIP
{

    SERIAL_CLASS_NAME::SERIAL_CLASS_NAME(serial_t *driver)
        : driver_(driver) {}

    void SERIAL_CLASS_NAME::init()
    {
        if (!driver_ || !driver_->init) {
            throw std::runtime_error("Serial driver not initialized or init function missing");
        }
        driver_->init();
    }

    void SERIAL_CLASS_NAME::flush()
    {
        if (!driver_ || !driver_->flush) {
            throw std::runtime_error("Serial driver not initialized or flush function missing");
        }
        driver_->flush();
    }

    void SERIAL_CLASS_NAME::capture(Image &img)
    {
        if (!driver_ || !driver_->capture) {
            throw std::runtime_error("Serial driver not initialized or capture function missing");
        }
        driver_->capture(img.raw());
    }

    void SERIAL_CLASS_NAME::send(const Image &img)
    {
        if (!driver_ || !driver_->send) {
            throw std::runtime_error("Serial driver not initialized or send function missing");
        }
        driver_->send(img.raw());
    }

    void SERIAL_CLASS_NAME::sendJPEG(const Image &img)
    {
        if (!driver_ || !driver_->sendJPEG) {
            throw std::runtime_error("Serial driver not initialized or sendJPEG function missing");
        }
        driver_->sendJPEG(img.raw());
    }

    void SERIAL_CLASS_NAME::send1D(const void *data, uint8_t elem_size, uint32_t length, Serial1DDataType type)
    {
        if (!driver_ || !driver_->send1D) {
            throw std::runtime_error("Serial driver not initialized or send1D function missing");
        }
        driver_->send1D(data, elem_size, length, type);
    }

} // namespace embedDIP
