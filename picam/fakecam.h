#pragma once

#include "camera_interface.h"

#include <experimental/filesystem>
#include <string>

namespace fs = std::experimental::filesystem;

class FakeCam : public CameraInterface
{
public:
    FakeCam();
    virtual void CaptureFrame(uint8_t *rgb_data) override;
    virtual bool WaitForData() override;

protected:
    std::string dir_;
    fs::directory_iterator it_;
};
