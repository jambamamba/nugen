#include <iostream>

#include <opencv2/core/mat.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "TfLiteInterpreter.h"

//convert cat.png -resize 300x300! cat300x300.rgb


int main(int argc, char**argv)
{
    if(argc != 2)
    {
        std::cerr << "./classify <path to 300x300 rgb file>\n";
        return -1;
    }
    cv::Mat img = cv::imread("/tmp/IMG_8633.JPG");
    cv::resize(img, img, cv::Size(300, 300), 0, 0, cv::INTER_CUBIC);

    TfLiteInterpreter interpreter(TfLiteInterpreter::Type::Detector);

    if(!interpreter.Create())
    {
        return -1;
    }
//    if(!interpreter.LoadImage(argv[1]))
//    {
//        return -1;
//    }

//    for(ssize_t i = 0; i < img.rows; ++i)
//    {
//        for(ssize_t j = 0; j < img.cols; ++j)
//        {
//            uchar &b = img.data[(img.cols * i + j) + 0];
////            uchar &g = img.data[(img.cols * i + j) + 1];
//            uchar &r = img.data[(img.cols * i + j) + 2];
//            std::swap(b, r);
//        }
//    }
    if(!interpreter.LoadImage(img.data, img.total() * img.elemSize()))
    {
        return -1;
    }

    auto res = interpreter.Inference();
    std::cout << "Inferenced in " << res.milliseconds_
              << " milliseconds"
              << "\n";
    for(const auto &object: res.objects_)
    {
        std::cout << "class \"" << object.class_
                  << "\", score:"
                  << object.score_
                  << ", at "
                  << "(" << object.bounding_rect_.x << "," << object.bounding_rect_.y << ")=>"
                  << "(" << object.bounding_rect_.x + object.bounding_rect_.width << "," << object.bounding_rect_.y + object.bounding_rect_.height << ")"
                  << "\n";
        int thickness=1;
        int lineType=8;
        int shift=0;
        cv::rectangle(img, object.bounding_rect_, cv::Scalar(0xff, 0xff, 0), thickness, lineType, shift);
    }

    cv::imwrite("/tmp/IMG_8633b.JPG", img);

    return 0;
}
