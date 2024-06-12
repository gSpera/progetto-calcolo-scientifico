#include "context.h"

#include <format>

Error<Context> Context::init(Device dev) {
    Error<Context> ret;
    cl_int err;
    cl_device_id device_id = dev.device_id();
    this->ctx = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        return ret.set_error(std::format("Cannot create context"));
    }

    cl_command_queue_properties queue_properties[] = {CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0};
    this->queue = clCreateCommandQueueWithProperties(ctx, device_id, queue_properties, &err);
    if (err != CL_SUCCESS) {
        return ret.set_error(std::format("Cannot create command queue"));
    }

    this->dev = dev;
    return ret.set_value(*this);
}

Context::~Context() {
    // TODO: Implement
}

