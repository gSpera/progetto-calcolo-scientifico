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
    cl_device_id get_device_id() { return this->dev.device_id(); }
private:
    Device dev;
    cl_context ctx;
    cl_command_queue queue;
};

enum MemoryType {
    VECTOR,
    MATRIX,
    IMAGE,
};
std::string memoryTypeToString(MemoryType type);

class Memory {
public:
    Memory() {}
    Error<Unit> init(Context ctx, size_t size);
    Error<Unit> write(void *data);
    Error<std::vector<char>> read();
    cl_mem get_mem() { return this->mem; }
private:
    cl_command_queue command_queue;
    cl_mem mem;
    size_t size;
};

class Argument {
public:
    Argument(std::string name, MemoryType type);
    std::string get_name() {return this->name; }
    MemoryType get_type() {return this->type; }
    Error<Memory> allocate();
    void show();
private:
    MemoryType type;
    std::string name;

    int floatVectorSize;
    std::vector<float> floatVector;
};


Error<std::vector<Device>> enumerateDevices();


#endif