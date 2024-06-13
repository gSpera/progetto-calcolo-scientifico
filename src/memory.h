#ifndef H_MEMORY
#define H_MEMORY

#include <CL/cl.h>
#include "context.h"
#include "error.h"

enum MemoryType {
    VECTOR,
    MATRIX,
    IMAGE,
};
std::string memoryTypeToString(MemoryType type);

class Memory {
public:
    Memory() {}
    Error<Memory> init(Context ctx, size_t size);
    Error<Unit> write(void *data);
    Error<uint8_t *> read();
    Error<Unit> read_to(void *dst);
    cl_mem get_mem() { return this->mem; }
private:
    cl_command_queue command_queue;
    cl_mem mem;
    size_t size;
};

#endif
