#pragma once

#include "camera_interface.h"

#include <experimental/filesystem>
#include <string>

namespace fs = std::experimental::filesystem;

class FakeCam : public CameraInterface
{
public:
    FakeCam(const std::string &image_dir);
    virtual void CaptureFrame(CameraData &cam_data) override;
    virtual bool WaitForData() override;

protected:
    std::string dir_;
    size_t idx_ = 0;
    bool stop_ = false;
};
