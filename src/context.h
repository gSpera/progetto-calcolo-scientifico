#ifndef H_CONTEXT
#define H_CONTEXT

#include <CL/cl.h>
#include "error.h"
#include "device.h"

class Context {
public:
    Context() {};
    ~Context();
    Error<Context> init(Device dev);

    cl_context get_context() { return this->ctx; }
    cl_command_queue get_queue() { return this->queue; }
    cl_device_id get_device_id() { return this->dev.device_id(); }
private:
    Device dev;
    cl_context ctx;
    cl_command_queue queue;
};

#endif
