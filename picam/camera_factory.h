#pragma once

#include <string>

#include "camera_interface.h"

class CameraFactory
{
public:
    static CameraInterface &Camera(const std::string &image_path);
protected:
    CameraFactory() {}
};
