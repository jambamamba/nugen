#include "fakecam.h"

#include <fstream>
#include <iostream>
#include <nzlogger.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

NEW_LOG_CATEGORY(FakeCam)

FakeCam::FakeCam()
    : CameraInterface()
    , dir_("/home/dev/oosman/repos/nugen/logs/01-13-2021")
    , it_(fs::directory_iterator(dir_))
{
    it_ = fs::begin(it_);
}

void FakeCam::CaptureFrame(CameraData &cam_data)
{
    if(it_ != fs::end(it_))
    {
        LOG_C(FakeCam, DEBUG) << it_->path();
        cv::Mat image = cv::imread(it_->path().c_str());
        memcpy(cam_data.buffer_, image.data, image.total() * image.elemSize());

        it_++;
    }
}

bool FakeCam::WaitForData()
{
    return (it_ != fs::end(it_));
}
