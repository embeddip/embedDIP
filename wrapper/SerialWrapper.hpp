#pragma once

extern "C"
{
#include "device/serial/serial.h"
}

#include "ImageWrapper.hpp"
#include <cstdint>

namespace embedDIP
{

    class Serial
    {
    public:
        explicit Serial(serial_t *driver);

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
