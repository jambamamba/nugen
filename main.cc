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
    LOG_C(MainLog, DEBUG) << "UsbDeviceMounted";
    struct stat info;
    const std::string mount_point("/dev/disk/by-uuid/31cc5498-c35c-4b67-a23e-31dc5499fc92");
    if( stat( mount_point.c_str(), &info ) != 0 )
    {
        LOG_C(MainLog, DEBUG) << "Cannot access " << mount_point;
        return false;
    }
    else if( info.st_mode & S_IFLNK )
    {
        LOG_C(MainLog, DEBUG) << "Found USB device " << mount_point;
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
std::string PrepareOutputImageDirectory()
{
    std::string output_path("/tmp/");
    if(UsbDeviceMounted())
    {
        output_path = std::string("/media/usb-drive/nugen/");
        fs::create_directories(output_path);
        for (const auto& entry : fs::directory_iterator(output_path))
        {fs::remove_all(entry.path());}
    }
    std::string raw = std::string(output_path) + "raw";
    fs::create_directories(raw);
    for (const auto& entry : fs::directory_iterator(raw))
    {fs::remove_all(entry.path());}

    std::string inf = std::string(output_path) + "inf";
    fs::create_directories(inf);
    for (const auto& entry : fs::directory_iterator(inf))
    {fs::remove_all(entry.path());}

    return output_path;
}
/////////////////////////////////////////////////////
OutputFilePaths GenerateOutputPaths(const std::string& output_path, size_t idx)
{
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
void DrawBoundingBoxesAndLabels(const TfLiteInterpreter::DetectedObject &object, cv::Mat &original_img, const cv::Mat &inferenced_img)
{
    int thickness=1;
    int lineType=8;
    int shift=0;
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
/////////////////////////////////////////////////////
void ShowResults(const TfLiteInterpreter::Result &&res,
                 cv::Mat &original_img,
                 cv::Mat &inferenced_img,
                 const std::string &inferenced_file_path,
                 TfLiteInterpreter::Type inference_type,
                 bool log_images)
{
    LOG_C(MainLog, DEBUG) << "Inferenced in " << res.milliseconds_
              << " milliseconds";
    for(const auto &object: res.objects_)
    {
        LOG_C(MainLog, DEBUG) << "class \"" << object.class_
                  << "\", score:"
                  << object.score_;
        if(inference_type == TfLiteInterpreter::Type::Detector)
        {
            LOG_C(MainLog, DEBUG)
                    << ", at "
                    << "(" << object.bounding_rect_.x << "," << object.bounding_rect_.y << ")=>"
                    << "(" << object.bounding_rect_.x + object.bounding_rect_.width << "," << object.bounding_rect_.y + object.bounding_rect_.height << ")";
        }
        if(object.score_ > .75 &&
                log_images &&
                inference_type == TfLiteInterpreter::Type::Detector)
        {
            DrawBoundingBoxesAndLabels(object, original_img, inferenced_img);
        }
    }

    if(log_images &&
            inference_type == TfLiteInterpreter::Type::Detector)
    {
        cv::imwrite(inferenced_file_path.c_str(), original_img);
        LOG_C(MainLog, DEBUG) << "Wrote to " << inferenced_file_path;
    }
}
/////////////////////////////////////////////////////
void InferenceImage(const std::string &image_path,
                    TfLiteInterpreter::Type inference_type,
                    bool log_images)
{
    TfLiteInterpreter interpreter(inference_type);
    if(!interpreter.Create())
    {
        return;
    }

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
                inference_type,
                log_images);
}
/////////////////////////////////////////////////////
bool InferenceCameraImage(TfLiteInterpreter::Type inference_type,
                          cv::Mat &original_img,
                          const std::string &inferenced_file_path,
                          bool log_images)
{
    if(inference_type != TfLiteInterpreter::Type::Classifier &&
            inference_type  != TfLiteInterpreter::Type::Detector)
    { return true; }

    TfLiteInterpreter interpreter(inference_type);
    if(!interpreter.Create())
    {
        return false;
    }

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
                inference_type,
                log_images);

    return true;
}
/////////////////////////////////////////////////////
void InferenceCameraImageLoop(TfLiteInterpreter::Type inference_type,
                              bool log_images,
                              bool &killed)
{
    uint8_t *rgb_data[2];
    constexpr size_t camera_frame_width = 640;
    constexpr size_t camera_frame_height = 480;
    constexpr size_t camera_buffer_size = camera_frame_width * camera_frame_height * 3;
    rgb_data[0] = (uint8_t*) calloc(camera_buffer_size, 1);
    rgb_data[1] = (uint8_t*) calloc(camera_buffer_size, 1);
    int buf = 0;
    auto &camera = CameraFactory::Camera();
    std::string output_path = PrepareOutputImageDirectory();
    for(size_t it = 0; !killed; ++it)
    {
        uint8_t *buffer1 = rgb_data[buf];
        uint8_t *buffer2 = rgb_data[(buf+1)%2];
        CameraInterface::CameraData cam_data(buffer1, camera_buffer_size);
        camera.CaptureFrame(cam_data);

        cv::Mat original_img(cv::Size(camera_frame_width, camera_frame_height),
                    CV_8UC3, buffer2,
                    cv::Mat::AUTO_STEP);

        std::string original_file_path, inferenced_file_path;
        auto output_paths = GenerateOutputPaths(output_path, it);

        if(log_images)
        {
            cv::imwrite(output_paths.original, original_img);
            LOG_C(MainLog, DEBUG) << "wrote file " << output_paths.original;
        }
        if(!InferenceCameraImage(inference_type,
                                 original_img,
                                 output_paths.inferenced,
                                 log_images))
        { return; }

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
        LOG_C(MainLog, WARNING) << "./detect <path to image file, jpg, png, etc or /dev/camera> --detect|classify [--log]";
        return -1;
    }
    bool log_images = false;
    TfLiteInterpreter::Type inference_type = TfLiteInterpreter::Type::None;
    std::string image_path = argv[1];
    for(int idx = 1; idx < argc; ++idx)
    {
        if(std::string(argv[idx]) == "--classify") { inference_type  = TfLiteInterpreter::Type::Classifier; }
        else if(std::string(argv[idx]) == "--detect") { inference_type  = TfLiteInterpreter::Type::Detector; }
        else if(std::string(argv[idx]) == "--log") { log_images  = true; }
    }

    if(image_path == "/dev/camera")
    {
        bool killed = false;
        InferenceCameraImageLoop(inference_type, log_images, killed);
    }
    else
    {
        InferenceImage(image_path, inference_type, log_images);
    }

    return 0;
}
