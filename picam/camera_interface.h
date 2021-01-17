#pragma once

#include <cstdint>
#include <vector>

class CameraInterface
{
public:
    CameraInterface() {}
    virtual ~CameraInterface() {}
    virtual void CaptureFrame(uint8_t *rgb_data) = 0;
    virtual bool WaitForData() = 0;
};

