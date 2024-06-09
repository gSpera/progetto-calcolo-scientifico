#ifndef H_OPENCL
#define H_OPENCL

#include <CL/cl.h>
#include <vector>
#include <format>

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

class Context {
public:
    Context() {};
    ~Context();
    Error<Context> init(Device dev);

    cl_context get_context() { return this->ctx; }
    cl_command_queue get_queue() { return this->queue; }
private:
    Device dev;
    cl_context ctx;
    cl_command_queue queue;
};

class Program {
public:
    Program(Context ctx, std::string source) {
        this->ctx = ctx;
        this->source = source;
    }
    Error<Unit> build() {
        Error<Unit> ret;
        cl_int err;
        const char *source = this->source.c_str();

        prg = clCreateProgramWithSource(ctx.get_context(), 1, &source, NULL, &err);
        if (err != CL_SUCCESS) return ret.set_error("Cannot create program: " + err);

        err = clBuildProgram(prg, 0, NULL, NULL, NULL, NULL);
        if (err != CL_SUCCESS) return ret.set_error("Cannot build: " + err);

        kernel = clCreateKernel(prg, "kernel", &err);
        if (err != CL_SUCCESS) return ret.set_error("Cannot create kernel: " + err);

        return ret.set_value(unit_value);
    }
private:
    Context ctx;
    std::string source;
    cl_program prg;
    cl_kernel kernel;
};

Error<std::vector<Device>> enumerateDevices();


#endif