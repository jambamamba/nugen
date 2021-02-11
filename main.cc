#include <chrono>
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

NEW_LOG_CATEGORY(Main)

namespace fs = std::experimental::filesystem;
namespace  {
/////////////////////////////////////////////////////
bool UsbStorageDeviceMounted()
{
    struct stat info;
    const std::string mount_point("/dev/disk/by-uuid/31cc5498-c35c-4b67-a23e-31dc5499fc92");
    if( stat( mount_point.c_str(), &info ) != 0 )
    {
        LOG_C(Main, WARNING) << "Cannot access USB storage device at mount point: " << mount_point;
        return false;
    }
    else if( info.st_mode & S_IFLNK )
    {
        LOG_C(Main, DEBUG) << "Found USB storage device " << mount_point;
        return true;
    }
    return false;
}

/////////////////////////////////////////////////////
void RecursivelyDeleteDirectory(const std::string &dir)
{
    for (const auto& entry : fs::directory_iterator(dir))
    {
        if(fs::is_directory(entry.path()))
        {RecursivelyDeleteDirectory(entry.path());}
        else
        {fs::remove(entry.path());}
    }
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
    if(UsbStorageDeviceMounted())
    {
        output_path = std::string("/media/usb-drive/nugen/");
        fs::create_directories(output_path);
        RecursivelyDeleteDirectory(output_path);
    }
    std::string raw = std::string(output_path) + "raw";
    fs::create_directories(raw);
    RecursivelyDeleteDirectory(raw);

    std::string inf = std::string(output_path) + "inf";
    fs::create_directories(inf);
    RecursivelyDeleteDirectory(inf);

    return output_path;
}
/////////////////////////////////////////////////////
OutputFilePaths GenerateOutputPaths(const std::string& output_path, size_t idx)
{
    OutputFilePaths output_paths;
    char *buffer = (char *)calloc(output_path.size() + 64, 1);
    sprintf(buffer, "%sraw/frame%05u.jpg", output_path.c_str(), idx);
    output_paths.original = std::string(buffer);
    sprintf(buffer, "%sinf/frame%05u.jpg", output_path.c_str(), idx);
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
    LOG_C(Main, DEBUG) << "Inferenced in " << res.milliseconds_
              << " milliseconds";
    for(const auto &object: res.objects_)
    {
        LOG_C(Main, DEBUG) << "class \"" << object.class_
                  << "\", score:"
                  << object.score_;
        if(inference_type == TfLiteInterpreter::Type::Detector)
        {
            LOG_C(Main, DEBUG)
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
        LOG_C(Main, DEBUG) << "Wrote to " << inferenced_file_path;
    }
}
/////////////////////////////////////////////////////
bool InferenceCameraImage(TfLiteInterpreter::Type inference_type,
                          TfLiteInterpreter &interpreter,
                          cv::Mat &original_img,
                          const std::string &inferenced_file_path,
                          bool log_images)
{
    if(inference_type != TfLiteInterpreter::Type::Classifier &&
            inference_type  != TfLiteInterpreter::Type::Detector)
    { return true; }

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
struct CameraBuffers
{
    static constexpr size_t frame_width_ = 640;
    static constexpr size_t frame_height_ = 480;
    static constexpr size_t buffer_size_ = frame_width_ * frame_height_ * 3;
    static constexpr int num_buffers_ = 2;
    CameraBuffers()
    {
        rgb_data_[0] = (uint8_t*) calloc(buffer_size_, 1);
        rgb_data_[1] = (uint8_t*) calloc(buffer_size_, 1);
    }
    ~CameraBuffers()
    {
        free(rgb_data_[0]);
        free(rgb_data_[1]);
    }
    uint8_t *AtIndex(int idx)
    {
        return rgb_data_[idx];
    }
    uint8_t *rgb_data_[num_buffers_];
};
/////////////////////////////////////////////////////
void InferenceCameraImageLoop(const std::string &image_path,
                              TfLiteInterpreter::Type inference_type,
                              bool log_images,
                              bool &killed)
{
    CameraBuffers buffers;
    int idx = 0;
    auto &camera = CameraFactory::Camera(image_path);
    std::string output_path = PrepareOutputImageDirectory();

    TfLiteInterpreter interpreter(inference_type);
    if(inference_type == TfLiteInterpreter::Type::Classifier ||
            inference_type  == TfLiteInterpreter::Type::Detector)
    {
        if(!interpreter.Create())
        {
            return;
        }
    }

    auto start = std::chrono::steady_clock::now();
    for(size_t it = 0, num_frames_processed = 0; !killed; ++it, ++num_frames_processed)
    {
        CameraInterface::CameraData cam_data(buffers.AtIndex(idx), buffers.buffer_size_);
        camera.CaptureFrame(cam_data);

        cv::Mat original_img(cv::Size(buffers.frame_width_,
                                      buffers.frame_height_),
                             CV_8UC3,
                             buffers.AtIndex((idx + 1) % buffers.num_buffers_),
                             cv::Mat::AUTO_STEP);

        std::string original_file_path, inferenced_file_path;
        auto output_paths = GenerateOutputPaths(output_path, it);

        if(log_images)
        {
            cv::imwrite(output_paths.original, original_img);
            LOG_C(Main, DEBUG) << "Wrote file " << output_paths.original;
        }
        if(!InferenceCameraImage(inference_type,
                                 interpreter,
                                 original_img,
                                 output_paths.inferenced,
                                 log_images))
        { return; }

        //std::cin.get();
        if(!camera.WaitForData()) { break; }

        idx = (idx + 1) % buffers.num_buffers_;
        if(it == 10000) { it = 0; }

        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed_seconds = end-start;
        LOG_C(Main, DEBUG) << "FPS: " << (num_frames_processed/elapsed_seconds.count());
    }
}
}//namespace

/////////////////////////////////////////////////////
int main(int argc, char**argv)
{
    if(argc < 2)
    {
        LOG_C(Main, WARNING) << "./detect <path to image file, jpg, png, etc or /dev/camera> --detect|classify [--log]";
        return -1;
    }
    bool log_images = false;
    TfLiteInterpreter::Type inference_type = TfLiteInterpreter::Type::None;
    std::string image_path = argv[1];
    for(int idx = 1; idx < argc; ++idx)
    {
        std::string token(argv[idx]);
        if(token == "--classify") { inference_type  = TfLiteInterpreter::Type::Classifier; }
        else if(token == "--detect") { inference_type  = TfLiteInterpreter::Type::Detector; }
        else if(token == "--log") { log_images  = true; }
    }

    bool killed = false;
    InferenceCameraImageLoop(image_path, inference_type, log_images, killed);

    LOG_C(Main, INFO) << "Exiting";
    return 0;
}
