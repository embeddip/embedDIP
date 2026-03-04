#pragma once

extern "C"
{
#include "device/serial/serial.h"
#include "embedDIP_configs.h"
}

#include "ImageWrapper.hpp"
#include <cstdint>
namespace embedDIP
{

// Define class name based on platform
#if defined(ARDUINO_ARCH_ESP32)
#define SERIAL_CLASS_NAME SerialDev
#else
#define SERIAL_CLASS_NAME Serial
#endif

    class SERIAL_CLASS_NAME
    {
    public:
        explicit SERIAL_CLASS_NAME(serial_t *driver);

        void init();
        void flush();
        void capture(Image &img);
        void send(const Image &img);
        void sendJPEG(const Image &img);
        void send1D(const void *data, uint8_t elem_size, uint32_t length, Serial1DDataType type);

    private:
        serial_t *driver_;
    };

} // namespace embedDIP
