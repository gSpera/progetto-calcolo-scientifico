
#include "error.h"

#include <format>

#define X(X) case X: return #X
std::string errorToString(cl_int error) {
    switch (error) {
        X(CL_INVALID_PROGRAM);
        X(CL_INVALID_PROGRAM_EXECUTABLE);
        X(CL_INVALID_KERNEL_NAME);
        X(CL_INVALID_KERNEL_DEFINITION);
        X(CL_INVALID_VALUE);
        X(CL_OUT_OF_RESOURCES);
        X(CL_OUT_OF_HOST_MEMORY);
        X(CL_INVALID_DEVICE);
        X(CL_INVALID_BINARY);
        X(CL_INVALID_BUILD_OPTIONS);
        X(CL_COMPILER_NOT_AVAILABLE);
        X(CL_BUILD_PROGRAM_FAILURE);
        X(CL_INVALID_OPERATION);
        X(CL_INVALID_COMMAND_QUEUE);
        X(CL_INVALID_KERNEL);
        X(CL_INVALID_CONTEXT);
        X(CL_INVALID_KERNEL_ARGS);
        X(CL_INVALID_WORK_DIMENSION);
        X(CL_INVALID_GLOBAL_WORK_SIZE);
        X(CL_INVALID_GLOBAL_OFFSET);
        X(CL_INVALID_WORK_GROUP_SIZE);
        X(CL_INVALID_WORK_ITEM_SIZE);
        X(CL_MISALIGNED_SUB_BUFFER_OFFSET);
        X(CL_INVALID_IMAGE_SIZE);
        X(CL_IMAGE_FORMAT_NOT_SUPPORTED);
        X(CL_MEM_OBJECT_ALLOCATION_FAILURE);
        X(CL_INVALID_EVENT_WAIT_LIST);
    }
    return std::format("Unkown error ({})", (int) error);
}