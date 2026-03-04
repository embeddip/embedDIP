#pragma once

#include "ImageWrapper.hpp" 
#include "device/camera/camera.h" // camera_t, captureMode, etc.

namespace embedDIP
{

    class Camera
    {
    public:
        explicit Camera(camera_t *driver);

        bool init(ImageResolution resolution);
        bool capture(captureMode mode, Image &img);
        bool stop();
        bool setRes(ImageResolution resolution);

    private:
        camera_t *driver_;
    };

} // namespace embedDIP
