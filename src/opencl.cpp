#include "opencl.h"
#include "error.h"

#include "imgui.h"

#include <format>
#include <iostream>

Device::Device(cl_platform_id platform, cl_device_id device) {
        this->platform = platform;
        this->device = device;
}
std::string Device::name() { return this->m_name; }

Error<Unit> Device::init() {
    Error<Unit> ret;
    char platform_name[50];
    char device_name[100];

    cl_int err = clGetPlatformInfo(platform, CL_PLATFORM_NAME, 50, platform_name, NULL);
    if (err != CL_SUCCESS) return ret.set_error(std::format("Cannot get platform {} name", (long) platform));

    err = clGetDeviceInfo(device, CL_DEVICE_NAME, 100, &device_name, NULL);
    if (err != CL_SUCCESS) return ret.set_error(std::format("Cannot get device {}:{} name", (long) platform, (long) device));

    this->m_name = std::string(std::string(platform_name) + " - " + std::string(device_name));

    return ret.set_value(unit_value);
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


Error<Memory> Memory::init(Context ctx, size_t size) {
    cl_int err;
    this->mem = clCreateBuffer(
        ctx.get_context(),
        CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
        size, NULL, &err);
    if (err != CL_SUCCESS) {
        return Error<Memory>().set_error(std::format("Cannot create memory buffer: {}", errorToString(err)));
    }
    this->command_queue = ctx.get_queue();
    this->size = size;
    return Error<Memory>().set_value(*this);
}

Error<Unit> Memory::write(void *data) {
    if (data == nullptr) return Error<Unit>().set_error("Cannot write data from nullptr");
    cl_int err = clEnqueueWriteBuffer(command_queue, mem, CL_TRUE, 0,
                         size, data, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        return Error<Unit>().set_error(std::format("Cannot write to memory buffer: {}", errorToString(err)));
    }

    return Error<Unit>().set_value(unit_value);
}

Error<uint8_t *> Memory::read() {
    uint8_t *data = new uint8_t[size];
    cl_int err = clEnqueueReadBuffer(command_queue, mem, CL_TRUE, 0,
        size, data, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        return Error<uint8_t *>().set_error(std::format("Cannot read buffer: {}", errorToString(err)));
    }

    return Error<uint8_t *>().set_value(data);
}

std::string memoryTypeToString(MemoryType type) {
    switch(type) {
        case VECTOR:
            return "Vettore";
        case MATRIX:
            return "Matrice";
        case IMAGE:
            return "Immagine";
        default:
            return "UNKOWN";
    }
}

Argument::Argument(std::string name, MemoryType type) {
    this->name = name;
    this->type = type;

    floatMatrixRows = 1;
    floatMatrixCols = 1;
}

Error<Memory> Argument::push_to_gpu(Context ctx) {
    size_t size = this->size();
    Error<Memory> mem_ = Memory().init(ctx, size);
    if (mem_.is_error()) { return mem_; }
    Memory mem = mem_.get_value();
    Error<Unit> ret = mem.write((void *) floatVector.data());

    if (ret.is_error()) {
        return Error<Memory>().set_error(ret.get_error());
    } else {
        return Error<Memory>().set_value(mem);
    }
}

Error<Unit> Argument::pop_from_gpu(Memory mem) {
    Error<uint8_t *> buffer_ = mem.read();
    if (buffer_.is_error()) return Error<Unit>().set_error("Cannot read memory: " + buffer_.get_error());
    uint8_t *buffer = buffer_.get_value();
    memcpy(this->floatVector.data(), buffer, size());
    return Error<Unit>().set_value(unit_value);
}

void Argument::show() {
    if (floatMatrixRows < 1) floatMatrixRows = 1;
    if (floatMatrixCols < 1) floatMatrixCols = 1;

    switch(this->type) {
        case VECTOR:
            ImGui::InputInt("Dimensione", &floatMatrixRows);
            this->floatVector.resize(floatMatrixRows);

            if(ImGui::Button("Randomizza")) {
                for(size_t i=0;i<floatVector.size();i++) {
                    floatVector[i] = ((float) (rand() % 1000)) / 10;
                }
            }

            for (size_t i=0;i<floatVector.size();i++) {
                ImGui::Text(std::format("Posizione {}:", i).c_str());
                ImGui::SameLine();
                ImGui::InputScalar(std::format("##arg_{}_{}", name, i).c_str(), ImGuiDataType_Float, &floatVector[i], NULL);
            }
            ImGui::Text("");
    }
}

size_t Argument::size() {
    switch(this->type) {
        case MATRIX:
        case VECTOR:
            return floatMatrixRows * floatMatrixCols * sizeof(cl_float);
    }

    exit(1);
    return -1;
}