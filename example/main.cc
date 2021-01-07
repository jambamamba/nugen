#include <iomanip>
#include <iostream>
#include <sstream>

#include <opencv2/core/mat.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "picam/picam.h"
#include "tfLiteInterpreter.h"

namespace  {
std::string ZeroPad(const int value, const unsigned precision)
{
     std::ostringstream oss;
     oss << std::setw(precision) << std::setfill('0') << value;
     return oss.str();
}
void ShowResults(const TfLiteInterpreter::Result &&res,
                 cv::Mat &original_img,
                 cv::Mat &inferenced_img,
                 const std::string original_image_path,
                 TfLiteInterpreter::Type inference_type)
{
    std::cout << "Inferenced in " << res.milliseconds_
              << " milliseconds"
              << "\n";
    for(const auto &object: res.objects_)
    {
        std::cout << "class \"" << object.class_
                  << "\", score:"
                  << object.score_;
        if(inference_type == TfLiteInterpreter::Type::Detector)
        {
            std::cout
                    << ", at "
                    << "(" << object.bounding_rect_.x << "," << object.bounding_rect_.y << ")=>"
                    << "(" << object.bounding_rect_.x + object.bounding_rect_.width << "," << object.bounding_rect_.y + object.bounding_rect_.height << ")";
        }
        std::cout << "\n";
        int thickness=3;//1 when using inferenced image as original
        int lineType=8;
        int shift=0;
        if(object.score_ > .75 &&
                inference_type == TfLiteInterpreter::Type::Detector)
        {
            auto color = cv::Scalar(0, 0xff, 0xff);
            cv::Rect rect(object.bounding_rect_);
            rect.x *= original_img.cols/static_cast<float>(inferenced_img.cols);
            rect.y *= original_img.rows/static_cast<float>(inferenced_img.rows);
            rect.width *= original_img.cols/static_cast<float>(inferenced_img.cols);
            rect.height *= original_img.rows/static_cast<float>(inferenced_img.rows);
            cv::rectangle(original_img, rect, color, thickness, lineType, shift);
            cv::putText(original_img, object.class_ + " " + std::to_string((int)(object.score_ * 100)) + "%",
                        rect.tl(), cv::FONT_HERSHEY_SIMPLEX, 2, //.2 when using inferenced image as original
                        color, thickness);
        }
    }

    if(inference_type == TfLiteInterpreter::Type::Detector)
    {
        std::string output_path(original_image_path);
        size_t pos = output_path.rfind(".");
        output_path.insert(pos, ".inferenced");
        cv::imwrite(output_path.c_str(), original_img);
    }
}
}//namespace

int main(int argc, char**argv)
{
    if(argc < 2)
    {
        std::cerr << "./detect <path to image file, jpg, png, etc>\n";
        std::cerr << "./detect <path to image file, jpg, png, etc> --classify\n";
        return -1;
    }
    TfLiteInterpreter::Type inference_type = TfLiteInterpreter::Type::Detector;
    std::string image_path = argv[1];
    if(argc == 3 && (std::string(argv[1]) == "--classify" || std::string(argv[2]) == "--classify"))
    {
        inference_type  = TfLiteInterpreter::Type::Classifier;
        if (std::string(argv[1]) == "--classify") { image_path = argv[2]; }
    }

    TfLiteInterpreter interpreter(inference_type);
    if(!interpreter.Create())
    {
        return -1;
    }

    PiCam picam;
    if(image_path == "/dev/camera")
    {
        std::vector<uint8_t> rgb_data[2];
        constexpr size_t camera_frame_width = 640;
        constexpr size_t camera_frame_height = 480;
        constexpr size_t camera_buffer_size = camera_frame_width * camera_frame_height * 3;
        for(size_t i = 0; i < camera_buffer_size; ++i)
        {
            rgb_data[0].push_back(0);
            rgb_data[1].push_back(0);
        }
        int buf = 0;
        for(size_t it = 0;; ++it)
        {
            std::vector<uint8_t> &buffer1 = rgb_data[buf];
            std::vector<uint8_t> &buffer2 = rgb_data[(buf+1)%2];
            picam.CaptureFrame(buffer1);

            cv::Mat img;
            cv::Mat original_img(cv::Size(camera_frame_width, camera_frame_height),
                        CV_8UC3, buffer2.data(),
                        cv::Mat::AUTO_STEP);

            cv::resize(original_img, img,
                       cv::Size(interpreter.GetImageDimensions().width_,
                                interpreter.GetImageDimensions().height_),
                       0, 0, cv::INTER_NEAREST);

            if(!interpreter.LoadImage(img.data, img.total() * img.elemSize()))
            {
                return -1;
            }

            std::string output_path = std::string("/tmp/frame") + ZeroPad(it, 5) + ".jpg";
            ShowResults(interpreter.Inference(),
                        original_img,
                        img,
                        output_path,
                    inference_type);
            //std::cin.get();

            buf = (buf+1)%2;
            if(it == 100) { it = 0; }
        }
    }
    else
    {
        cv::Mat img;
        cv::Mat original_img = cv::imread(image_path);
        cv::resize(original_img, img,
                   cv::Size(interpreter.GetImageDimensions().width_,
                            interpreter.GetImageDimensions().height_),
                   0, 0, cv::INTER_NEAREST);
        if(!interpreter.LoadImage(img.data, img.total() * img.elemSize()))
        {
            return -1;
        }

        std::string output_path(argv[1]);
        size_t pos = output_path.rfind(".");
        output_path.insert(pos, ".inferenced");

        ShowResults(interpreter.Inference(),
                    original_img,
                    img,
                    output_path,
                    inference_type);
    }


    return 0;
}
