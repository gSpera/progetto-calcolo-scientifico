#ifndef H_DEVICE
#define H_DEVICE

#include <CL/cl.h>
#include "error.h"

class Device {
public:
    Device() {}; // TODO: Delete
    Device(cl_platform_id platform, cl_device_id device);
    Error<Unit> init();
    std::string name();

    cl_platform_id platform_id() { return platform; }
    cl_device_id device_id() { return device; }
private:
    cl_platform_id platform;
    cl_device_id device;

    std::string m_name;
};

Error<std::vector<Device>> enumerateDevices();

#endif