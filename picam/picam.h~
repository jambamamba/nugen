#pragma once

#include <cstdint>
#include <vector>

namespace mmalpp {
    class Buffer;
    class Component;
    class Generic_port;
}
class PiCam
{
public:
    PiCam();
    virtual ~PiCam();
    void CaptureFrame(std::vector<uint8_t> &rgb_data);

protected:
    void CommitStillPortFormat();
    void EnableCameraComponents();
    void HandleFrameData(mmalpp::Generic_port &port, mmalpp::Buffer &buffer);
};

