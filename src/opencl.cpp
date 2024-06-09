#include "opencl.h"
#include "error.h"

#include <format>

Device::Device(cl_platform_id platform, cl_device_id device) {
        this->platform = platform;
        this->device = device;
}
Error<std::string> Device::name() {
    Error<std::string> ret;
    char platform_name[50];
    char device_name[100];

    cl_int err = clGetPlatformInfo(platform, CL_PLATFORM_NAME, 50, platform_name, NULL);
    if (err != CL_SUCCESS) return ret.set_error(std::format("Cannot get platform {} name", (long) platform));

    err = clGetDeviceInfo(device, CL_DEVICE_NAME, 100, &device_name, NULL);
    if (err != CL_SUCCESS) return ret.set_error(std::format("Cannot get device {}:{} name", (long) platform, (long) device));

    std::string name = std::string(std::string(platform_name) + " - " + std::string(device_name));
    return ret.set_value(name);
}

Error<std::vector<Device>> enumerateDevices() {
    Error<std::vector<Device>> ret;
    cl_uint num_platform;
    cl_platform_id platforms[10];

    cl_int err = clGetPlatformIDs(10, platforms, &num_platform);
    if (err != CL_SUCCESS) {
        return ret.set_error("Cannot get platforms");
    }

    std::vector<Device> values;

    for (cl_uint i=0;i<num_platform; i++) {
        cl_platform_id platform = platforms[i];
        cl_uint num_devices;
        cl_device_id devices[10];

        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 10, devices, &num_devices);
        if (err != CL_SUCCESS) {
            return ret.set_error(std::format("Cannot get devices for platform"));
        }

        for (cl_uint j=0;j<num_devices; j++) {
            values.push_back(Device(platform, devices[j]));
        }
    }

    return ret.set_value(values);
}

Error<Context> Context::init(Device dev) {
    Error<Context> ret;
    cl_int err;
    cl_device_id device_id = dev.device_id();
    ctx = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        return ret.set_error(std::format("Cannot create context"));
    }

    cl_command_queue_properties queue_properties[] = {CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0};
    queue = clCreateCommandQueueWithProperties(ctx, device_id, queue_properties, &err);
    if (err != CL_SUCCESS) {
        return ret.set_error(std::format("Cannot create command queue"));
    }

    return ret.set_value(*this);
};