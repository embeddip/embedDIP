#pragma once

#include "ImageWrapper.hpp" 
#include "device/camera/camera.h" // camera_t, captureMode, etc.

namespace embedDIP
{

    class Camera
    {
    public:
        explicit Camera(camera_t *driver);

        bool init(ImageResolution res, ImageFormat format);
        bool capture(captureMode mode, Image &img);
        bool stop();
        bool setRes(ImageResolution res);

    private:
        camera_t *driver_;
    };

} // namespace embedDIP
