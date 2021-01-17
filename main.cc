#include <experimental/filesystem>
#include <nzlogger.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <sys/syscall.h>
//#include <sys/types.h>
//#include <unistd.h>
#include <mutex>
#include <condition_variable>

#include <opencv2/core/mat.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "picam/camera_factory.h"
#include "tfhelper/tfLiteInterpreter.h"

NEW_LOG_CATEGORY(MainLog)

namespace fs = std::experimental::filesystem;
namespace  {
/////////////////////////////////////////////////////
bool UsbDeviceMounted()
{
    LOG_C(MainLog, DEBUG) << "Foo";
    struct stat info;
    const std::string mount_point("/dev/disk/by-uuid/31cc5498-c35c-4b67-a23e-31dc5499fc92");
    if( stat( mount_point.c_str(), &info ) != 0 )
    {
        std::cout << "Cannot access " << mount_point << "\n";
        return false;
    }
    else if( info.st_mode & S_IFLNK )
    {
        std::cout << "Found USB device " << mount_point << "\n";
        return true;
    }
    return false;
}
/////////////////////////////////////////////////////
struct OutputFilePaths
{
    std::string original;
    std::string inferenced;
};
/////////////////////////////////////////////////////
OutputFilePaths PrepareOutputImageDirectory(size_t idx)
{
    std::string output_path;
    if(UsbDeviceMounted())
    {
        output_path = std::string("/media/usb-drive/nugen/");
        fs::create_directories(output_path);
        for (const auto& entry : fs::recursive_directory_iterator(output_path))
        {fs::remove_all(entry.path());}
    }
    fs::create_directories(output_path + "raw");
    fs::create_directories(output_path + "inf");

    OutputFilePaths output_paths;
    char *buffer = (char *)calloc(output_path.size() + 64, 1);
    sprintf(buffer, "%sraw/frame%05ul.jpg", output_path.c_str(), idx);
    output_paths.original = std::string(buffer);
    sprintf(buffer, "%sinf/frame%05ul.jpg", output_path.c_str(), idx);
    output_paths.inferenced = std::string(buffer);
    free(buffer);

    return output_paths;
}
/////////////////////////////////////////////////////
void ShowResults(const TfLiteInterpreter::Result &&res,
                 cv::Mat &original_img,
                 cv::Mat &inferenced_img,
                 const std::string &inferenced_file_path,
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
        int thickness=1;
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
                        cv::Point(rect.x, rect.height), cv::FONT_HERSHEY_SIMPLEX, .75,
                        color, thickness);
        }
    }

    if(inference_type == TfLiteInterpreter::Type::Detector)
    {
        cv::imwrite(inferenced_file_path.c_str(), original_img);
        std::cout << "Wrote to " << inferenced_file_path << "\n";
    }
}
/////////////////////////////////////////////////////
void InferenceImage(const std::string &image_path,
                    TfLiteInterpreter &interpreter,
                    TfLiteInterpreter::Type inference_type)
{
    cv::Mat img;
    cv::Mat original_img = cv::imread(image_path);
    cv::resize(original_img, img,
               cv::Size(interpreter.GetImageDimensions().width_,
                        interpreter.GetImageDimensions().height_),
               0, 0, cv::INTER_NEAREST);
    if(!interpreter.LoadImage(img.data, img.total() * img.elemSize()))
    {
        return;
    }

    std::string inferenced_file_path(image_path);
    size_t pos = inferenced_file_path.rfind(".");
    inferenced_file_path.insert(pos, ".inferenced");

    ShowResults(interpreter.Inference(),
                original_img,
                img,
                inferenced_file_path,
                inference_type);
}
/////////////////////////////////////////////////////
bool InferenceCameraImage(TfLiteInterpreter &interpreter,
                          TfLiteInterpreter::Type inference_type,
                          cv::Mat &original_img,
                          const std::string &inferenced_file_path)
{
    cv::Mat img;
    cv::resize(original_img, img,
               cv::Size(interpreter.GetImageDimensions().width_,
                        interpreter.GetImageDimensions().height_),
               0, 0, cv::INTER_NEAREST);

    if(!interpreter.LoadImage(img.data, img.total() * img.elemSize()))
    {
        return false;
    }

    ShowResults(interpreter.Inference(),
                original_img,
                img,
                inferenced_file_path,
                inference_type);

    return true;
}
/////////////////////////////////////////////////////
void InferenceCameraImageLoop(TfLiteInterpreter &interpreter,
                              TfLiteInterpreter::Type inference_type,
                              bool &killed)
{
    uint8_t *rgb_data[2];
    constexpr size_t camera_frame_width = 640;
    constexpr size_t camera_frame_height = 480;
    constexpr size_t camera_buffer_size = camera_frame_width * camera_frame_height * 3;
    rgb_data[0] = (uint8_t*) malloc(camera_buffer_size);
    rgb_data[1] = (uint8_t*) malloc(camera_buffer_size);
    int buf = 0;
    auto &camera = CameraFactory::Camera();
    for(size_t it = 0; !killed; ++it)
    {
        //std::cout << "main tid: " << syscall(SYS_gettid) << "\n";

        uint8_t *buffer1 = rgb_data[buf];
        uint8_t *buffer2 = rgb_data[(buf+1)%2];
        camera.CaptureFrame(buffer1);

        cv::Mat original_img(cv::Size(camera_frame_width, camera_frame_height),
                    CV_8UC3, buffer2,
                    cv::Mat::AUTO_STEP);

        std::string original_file_path, inferenced_file_path;
        auto output_paths = PrepareOutputImageDirectory(it);

        cv::imwrite(output_paths.original, original_img);
        std::cout << "wrote file " << output_paths.original << "\n";

        if(!InferenceCameraImage(interpreter, inference_type, original_img, output_paths.inferenced)) { return; }
        //std::cin.get();
        if(!camera.WaitForData()) { break; }

        buf = (buf+1)%2;
        if(it == 10000) { it = 0; }
    }
}
}//namespace

/////////////////////////////////////////////////////
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

    if(image_path == "/dev/camera")
    {
        bool killed = false;
        InferenceCameraImageLoop(interpreter, inference_type, killed);
    }
    else
    {
        InferenceImage(image_path, interpreter, inference_type);
    }

    return 0;
}
