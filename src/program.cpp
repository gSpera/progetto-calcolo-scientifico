#include <iostream>
#include "program.h"

#include <format>

std::string Program::build(bool *ok) {
    *ok = false;
    cl_int err;
    const char *source = this->source.c_str();
    std::string build_log;

    build_log.append("*** Creating program ***\n");
    prg = clCreateProgramWithSource(ctx.get_context(), 1, &source, NULL, &err);
    if (err != CL_SUCCESS) {
        return build_log + "Cannot create program: " + errorToString(err);
    }

    build_log.append("*** Building program ***\n");
    cl_device_id dev = ctx.get_device_id();
    err = clBuildProgram(prg, 1, &dev, NULL, NULL, NULL);
    char build_log_cl[1024];
    cl_int err2 = clGetProgramBuildInfo(prg, dev, CL_PROGRAM_BUILD_LOG, sizeof(build_log_cl), &build_log_cl, NULL);
    if (err2 != CL_SUCCESS) {
        std::cout<<"Cannot get build log: "<<errorToString(err2)<<std::endl;
        build_log.append("Cannot get build log: "+errorToString(err2)+"\n");
    } else {
        build_log.append(build_log_cl);
        build_log.append("\n");
    }
    if (err != CL_SUCCESS) return build_log + "Cannot build: " + errorToString(err);

    build_log.append("*** DONE ***\n");
    *ok = true;
    return build_log;
}

std::string Program::prepare_for_execution(std::string kernel_name, std::vector<Argument> args, bool *ok) {
    *ok = false;
    cl_int err;
    std::string build_log;

    build_log.append("\n*** Creating kernel ***\n");
    kernel = clCreateKernel(prg, kernel_name.c_str(), &err);
    if (err != CL_SUCCESS) return build_log + "\nCannot create kernel: " + errorToString(err);

    cl_uint arg_count;
    err = clGetKernelInfo(kernel, CL_KERNEL_NUM_ARGS, sizeof(arg_count), &arg_count, NULL);
    if (err != CL_SUCCESS) return build_log + "\nCannot get kernel number of arguments: " + errorToString(err);

    if (arg_count != args.size()) {
        return std::format("{}\nWrong number of arguments {} (required by kernel) != {} (provided)\n", build_log, arg_count, args.size());
    }
    
    build_log.append(std::format("Preparing {} arguments\n", args.size()));
    this->mems.clear();
    this->mems.reserve(args.size());
    for (size_t i=0;i<args.size();i++) {
        Error<Memory> mem_ = args[i].push_to_gpu(ctx);
        if (mem_.is_error()) {
            return std::format("Cannot allocate memory: {}", mem_.get_error());
        }

        void *ptr = (void *) mem_.get_value().get_mem();
        err = clSetKernelArg(kernel, i, sizeof(ptr), &ptr);
        if (err != CL_SUCCESS) {
            return std::format("{}\nCannot set {}nth argument: {}\n", build_log, i, errorToString(err));
        }

        this->mems.push_back(mem_.get_value());
        build_log.append(std::format(" - Argument {} prepared ({})\n", args[i].get_name(), ptr));
    }

    build_log.append("Done\n");
    *ok = true;
    return build_log;
}

Error<std::string> Program::execute(size_t n_cores, std::span<size_t> count, cl_long *time) {
    if (count.size() == 1) return this->execute_1(n_cores, count[0], time);
    return this->execute_1(n_cores, count[0] * count[1], time);
    return Error<std::string>().set_error("Not implemented");
}
Error<std::string> Program::execute_1(size_t n_cores, size_t count, cl_long *time) {
    Error<std::string> ret;
    std::string log;
    int n_invocations = count / n_cores;
    size_t remain = count % n_cores;
    cl_ulong total_time = 0;
    cl_int err;
    cl_event event;

    cl_ulong start_time;
    cl_ulong end_time;

    auto execute = [&] (size_t offset_, size_t len_) {
        size_t offset = offset_;
        size_t len = len_;
        // log.append(std::format(" - Sub Execution offset: {} len: {}\n", offset, len));
        err = clEnqueueNDRangeKernel(ctx.get_queue(), kernel,
            1, &offset, &len, // Dim, Offset, Size
            NULL, // Local Work Size
        0, NULL, &event);
        if (err != CL_SUCCESS) {
            return Error<Unit>().set_error("Cannot enqueue nd range: " + errorToString(err));
        }

        clFinish(ctx.get_queue());
        clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(start_time), &start_time, NULL);
        clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(end_time), &end_time, NULL);
        cl_ulong time_ns = end_time - start_time;
        total_time += time_ns;
        // log.append(std::format("Iteration took: {}ns, {}us\n", time_ns, time_ns / 1000));
        return Error<Unit>().set_value(unit_value);
    };

    for (int i=0; i<n_invocations; i++) {
        size_t offset = i * n_cores;
        // log.append(std::format("Invoking from {} to {}\n", offset, offset + n_cores));
        auto ret2 = execute(offset, n_cores);
        if (ret2.is_error()) return ret.set_error(std::format("{}Error: {}", log, ret2.get_error()));
    }
    if (remain > 0) {
        size_t offset = count - remain;
        auto ret2 = execute(offset, remain);
        if (ret2.is_error()) return ret.set_error(std::format("{}Error: {}", log, ret2.get_error()));
    }

    log.append(std::format("Total took: {}ns, {}us\n", total_time, total_time / 1000));
    *time = total_time;
    return ret.set_value(log);
}

Error<std::vector<std::string>> Program::kernel_names() {
    Error<std::vector<std::string>> ret;
    char buffer[1024];
    cl_int err = clGetProgramInfo(prg, CL_PROGRAM_KERNEL_NAMES, sizeof(buffer), &buffer, NULL);
    if (err != CL_SUCCESS) {
        return ret.set_error(errorToString(err));
    }

    std::vector<std::string> values;
    std::string str(buffer);

    size_t curr_index = 0;
    while(1) {
        size_t index = str.find(";", curr_index);
        if (index == str.npos) {
            std::string sub = str.substr(curr_index);
            values.push_back(sub);
            break;
        }

        std::string sub = str.substr(0, index);
        values.push_back(sub);
        curr_index = index+1;
    }

    return ret.set_value(values);
}
