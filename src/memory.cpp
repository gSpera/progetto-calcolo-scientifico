#include "memory.h"

#include <format>

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

Error<Unit> Memory::read_to(void *dst) {
    cl_int err = clEnqueueReadBuffer(command_queue, mem, CL_TRUE, 0,
        size, dst, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        return Error<Unit>().set_error(std::format("Cannot read buffer: {}", errorToString(err)));
    }

    return Error<Unit>().set_value(unit_value);
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

