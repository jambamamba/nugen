#pragma once

#include "camera_interface.h"

class CameraFactory
{
public:
    static CameraInterface &Camera();
protected:
    CameraFactory() {}
};
