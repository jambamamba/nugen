#include "picam.h"

#include <iostream>
#include <fstream>
#include <mmalpp/mmalpp.h>
//#include <sys/syscall.h>
//#include <unistd.h>

namespace  {

mmalpp::Component camera_("vc.ril.camera");
mmalpp::Component null_sink_("vc.null_sink");

MMAL_PARAMETER_CHANGE_EVENT_REQUEST_T change_event_request_ =
{
    {MMAL_PARAMETER_CHANGE_EVENT_REQUEST, sizeof(MMAL_PARAMETER_CHANGE_EVENT_REQUEST_T)},
    MMAL_PARAMETER_CAMERA_SETTINGS,
    1
};
MMAL_PARAMETER_CAMERA_CONFIG_T cam_config_ = {
    {MMAL_PARAMETER_CAMERA_CONFIG, sizeof ( cam_config_ ) },
    3280, // max_stills_w
    2464, // max_stills_h
    1, // stills_yuv422
    1, // one_shot_stills
    3280, // max_preview_video_w
    2464, // max_preview_video_h
    3, // num_preview_video_frames
    0, // stills_capture_circular_buffer_height
    0, // fast_preview_resume
    MMAL_PARAM_TIMESTAMP_MODE_ZERO // use_stc_timestamp
};
MMAL_PARAMETER_EXPOSUREMODE_T exp_mode_ =
{
    { MMAL_PARAMETER_EXPOSURE_MODE, sizeof ( exp_mode_ ) },
    MMAL_PARAM_EXPOSUREMODE_AUTO
};
}//namespace

PiCam::PiCam()
    : CameraInterface()
{
    camera_.control().parameter().set_header(&change_event_request_.hdr);
    camera_.control().enable([](mmalpp::Generic_port& port, mmalpp::Buffer buffer){
        if (port.is_enabled())
            buffer.release();
    });
    camera_.control().parameter().set_header(&cam_config_.hdr);
    camera_.output(2).set_default_buffer();
    CommitStillPortFormat();
    camera_.enable();
    null_sink_.enable();
    camera_.output(2).enable([this](mmalpp::Generic_port& port, mmalpp::Buffer buffer){
        HandleFrameData(port, buffer);
    });
    camera_.control().parameter().set_header(&exp_mode_.hdr);
    camera_.output(2).create_pool(camera_.output(2).buffer_num_recommended(),
                                  camera_.output(2).buffer_size_recommended());
    camera_.output(2).send_all_buffers();
}

PiCam::~PiCam()
{
    null_sink_.disconnect();
//    camera_.disconnect();

    null_sink_.close();
    camera_.close();
}

void PiCam::CommitStillPortFormat()
{
    MMAL_ES_FORMAT_T * format = camera_.output(2).format();
    format->encoding = MMAL_ENCODING_RGB24;
    format->es->video.width = 640;
    format->es->video.height = 480;
    format->es->video.crop.x = 0;
    format->es->video.crop.y = 0;
    format->es->video.crop.width = 640;
    format->es->video.crop.height = 480;
    format->es->video.frame_rate.num = 1;
    format->es->video.frame_rate.den = 1;

    camera_.output(2).commit();
}

void PiCam::CaptureFrame(uint8_t *rgb_data)
{
    camera_data_available_ = false;
    rgb_data.clear();
    camera_.output(2).set_userdata(rgb_data);
    camera_.output(2).parameter().set_boolean(MMAL_PARAMETER_CAPTURE, true);
}

void PiCam::HandleFrameData(mmalpp::Generic_port &port, mmalpp::Buffer &buffer)
{
    //std::cout << "irq tid: " << syscall(SYS_gettid) << "\n";
    std::vector<u_char>& v = port.get_userdata_as<std::vector<u_char>>();
    v.insert(v.end(), buffer.begin(), buffer.end());

//    std::cout << "callback got data. buffer size: " << buffer.size() << "\n";
    if (buffer.size() == 0)
    {
//        std::ofstream f("/tmp/test.rgb", std::ios::binary);
//        f.write(reinterpret_cast<char*>(v.data()), static_cast<std::streamsize>(v.size()));
//        f.close();
        std::cout << "Finished capture. vector size: " << v.size() << std::endl;
		std::unique_lock<std::mutex> lk(m_);
		camera_data_available_ = true;
		cv_.notify_one();
    }
    buffer.release();

    if (port.is_enabled())
    {
        port.send_buffer(port.pool().queue().get_buffer());
    }
}

bool PiCam::WaitForData()
{
    std::unique_lock<std::mutex> lk(m_);
    cv_.wait(lk, [this]{return camera_data_available_;});
    return camera_data_available_;
}
