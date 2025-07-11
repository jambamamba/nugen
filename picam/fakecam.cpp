#include "fakecam.h"

#include <fstream>
#include <iostream>
#include <nzlogger.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

NEW_LOG_CATEGORY(FakeCam)

FakeCam::FakeCam(const std::string &image_dir)
    : CameraInterface()
    , dir_(image_dir)
{}

void FakeCam::CaptureFrame(CameraData &cam_data)
{
    char filename[64] = {0};
    sprintf(filename, "/frame%05d.jpg", idx_++);

    std::string path = std::string(dir_) + filename;
    if(!fs::exists(path))
    {
        stop_ = true;
        return;
    }
    LOG_C(FakeCam, DEBUG) << path;
    cv::Mat image = cv::imread(path.c_str());
    memcpy(cam_data.buffer_, image.data, image.total() * image.elemSize());
}

bool FakeCam::WaitForData()
{
    return !stop_;
}
