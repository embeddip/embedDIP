#include "CameraWrapper.hpp"

namespace embedDIP
{

    Camera::Camera(camera_t *driver)
        : driver_(driver) {}

    void Camera::init(ImageResolution res)
    {
        driver_->init(res);
    }

    void Camera::capture(captureMode mode, Image &img)
    {
        driver_->capture(mode, img.raw());
    }

    void Camera::stop()
    {
        driver_->stop();
    }

    void Camera::setRes(ImageResolution res)
    {
        driver_->setRes(res);
    }

} // namespace embedDIP
