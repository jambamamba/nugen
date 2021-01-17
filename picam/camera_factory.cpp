#include "camera_factory.h"

#if defined(USE_PICAM)
#include "picam.h"
#else
#include "fakecam.h"
#endif

CameraInterface &CameraFactory::Camera()
{
#if defined(USE_PICAM)
    static PiCam picam;
    static CameraInterface &ci = picam;
#else
    static FakeCam fakecam;
    static CameraInterface &ci = fakecam;
#endif
    return ci;
}
