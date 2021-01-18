#pragma once

#include <mutex>
#include <condition_variable>

#include "camera_interface.h"

namespace mmalpp {
    class Buffer;
    class Component;
    class Generic_port;
}
class PiCam : public CameraInterface
{
public:
    PiCam();
    virtual ~PiCam();
    void CaptureFrame(CameraData &cam_data) override;
    bool WaitForData() override;

protected:
    void CommitStillPortFormat();
    void EnableCameraComponents();
    void HandleFrameData(mmalpp::Generic_port &port, mmalpp::Buffer &buffer);
	
	std::mutex m_;
    std::condition_variable cv_;
	bool camera_data_available_ = false;
};

