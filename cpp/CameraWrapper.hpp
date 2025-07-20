#pragma once

#include "ImageWrapper.hpp" 
#include "camera.h" // camera_t, captureMode, etc.

namespace embedDIP
{

    class Camera
    {
    public:
        explicit Camera(camera_t *driver);

        bool init(ImageResolution res);
        bool capture(captureMode mode, Image &img);
        bool stop();
        bool setResolution(ImageResolution res);

    private:
        camera_t *driver_;
    };

} // namespace embedDIP
