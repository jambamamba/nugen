#include <iostream>

#include <opencv2/core/mat.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "TfLiteInterpreter.h"

namespace  {
void ShowResults(const TfLiteInterpreter::Result &&res,
                 cv::Mat &original_img,
                 cv::Mat &inferenced_img,
                 const std::string original_image_path)
{
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
        int thickness=3;
        int lineType=8;
        int shift=0;
        if(object.score_ > .75)
        {
            auto color = cv::Scalar(0, 0xff, 0xff);
            cv::Rect rect(object.bounding_rect_);
            rect.x *= original_img.cols/inferenced_img.cols;
            rect.y *= original_img.rows/inferenced_img.rows;
            rect.width *= original_img.cols/inferenced_img.cols;
            rect.height *= original_img.rows/inferenced_img.rows;
            cv::rectangle(original_img, rect, color, thickness, lineType, shift);
            cv::putText(original_img, object.class_ + " " + std::to_string((int)(object.score_ * 100)) + "%",
                        rect.tl(), cv::FONT_HERSHEY_SIMPLEX, 2, color, thickness);
        }
    }

    std::string output_path(original_image_path);
    size_t pos = output_path.rfind(".");
    output_path.insert(pos, ".inferenced");

    cv::imwrite(output_path.c_str(), original_img);
}
}//namespace

int main(int argc, char**argv)
{
    if(argc != 2)
    {
        std::cerr << "./detect <path to image file, jpg, png, etc>\n";
        return -1;
    }
    TfLiteInterpreter interpreter(TfLiteInterpreter::Type::Detector);
    if(!interpreter.Create())
    {
        return -1;
    }

    cv::Mat original_img = cv::imread(argv[1]);
    cv::Mat img;
    cv::resize(original_img, img,
               cv::Size(interpreter.GetImageDimensions().width_, interpreter.GetImageDimensions().height_),
               0, 0, cv::INTER_NEAREST);


    if(!interpreter.LoadImage(img.data, img.total() * img.elemSize()))
    {
        return -1;
    }

    ShowResults(interpreter.Inference(),
                original_img,
                img,
                argv[1]);

    return 0;
}
