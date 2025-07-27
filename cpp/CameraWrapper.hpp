#pragma once

#include "ImageWrapper.hpp" 
#include "camera.h" // camera_t, captureMode, etc.

namespace embedDIP
{

    class Camera
    {
    public:
        explicit Camera(camera_t *driver);

        void init(ImageResolution res);
        void capture(captureMode mode, Image &img);
        void stop();
        void setRes(ImageResolution res);

    private:
        camera_t *driver_;
    };

} // namespace embedDIP
