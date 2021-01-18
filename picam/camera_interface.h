#pragma once

#include <cstdint>
#include <vector>

class CameraInterface
{

public:
    struct CameraData
    {
        uint8_t *buffer_ = nullptr;
        size_t size_ = 0;
        CameraData(uint8_t *buffer, size_t size)
            : buffer_(buffer)
            , size_(size)
        {}
    };
    CameraInterface() {}
    virtual ~CameraInterface() {}
    virtual void CaptureFrame(CameraData &cam_data) = 0;
    virtual bool WaitForData() = 0;
};

