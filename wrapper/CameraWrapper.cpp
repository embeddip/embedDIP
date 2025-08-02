#include "CameraWrapper.hpp"

namespace embedDIP
{

    Camera::Camera(camera_t *driver)
        : driver_(driver) {}

    bool Camera::init(ImageResolution res)
    {
        return driver_ && driver_->init ? driver_->init(res) == 0 : false;
    }

    bool Camera::capture(captureMode mode, Image &img)
    {
        return driver_ && driver_->capture ? driver_->capture(mode, img.raw()) == 0 : false;
    }

    bool Camera::stop()
    {
        return driver_ && driver_->stop ? driver_->stop() == 0 : false;
    }

    bool Camera::setRes(ImageResolution res)
    {
        return driver_ && driver_->setRes ? driver_->setRes(res) == 0 : false;
    }

} // namespace embedDIP
