#include "camera_factory.h"

#if defined(USE_PICAM)
#include "picam.h"
#endif
#include "fakecam.h"

CameraInterface &CameraFactory::Camera(const std::string &image_path)
{
#if defined(USE_PICAM)
    static PiCam picam;
    static FakeCam fakecam(image_path);
    static CameraInterface &ci1 = picam;
    static CameraInterface &ci2 = fakecam;

    if(image_path == "/dev/camera") {return ci1;}
    else {return ci2;}
#else
    static FakeCam fakecam(image_path);
    static CameraInterface &ci2 = fakecam;
    return fakecam;
#endif
}
