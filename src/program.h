#ifndef H_PROGRAM
#define H_PROGRAM

#include "opencl.h"

class Program {
public:
    Program() {};
    Program(Context ctx, std::string source) {
        this->ctx = ctx;
        this->source = source;
    }
    std::string build(bool *ok);
    std::string prepare_for_execution(std::string kernel_name, std::vector<Argument> args, bool *ok);
    Error<std::string> execute(size_t n_cores, size_t count, cl_long *time);
    Error<std::vector<std::string>> kernel_names();
private:
    Context ctx;
    std::string source;
    std::string build_log;
    cl_program prg;
    cl_kernel kernel;
};

#endif
