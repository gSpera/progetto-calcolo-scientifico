#ifndef H_ERROR
#define H_ERROR

#include <string>
#include <functional>
#include <unordered_map>

#include <CL/cl.h>

std::string errorToString(cl_int err);

typedef bool Unit;
const Unit unit_value = false;

template<typename T>
class Error {
public:
    Error() { this->error = ""; }
    Error set_value(T value) { this->value = value; return *this; }
    Error set_error(std::string error) {this->error = error; return *this; }
    bool has_value() { return error == ""; }
    bool is_error() {return error != ""; }
    T get_value() { return value; }
    std::string get_error() { return error; }
    T value_or_exit(std::function<void(std::string)> err_fn) {
        if (has_value()) return value;
        err_fn(error);
        exit(1);
        return value; // Not reached
    }
    T value_or_try(T (*err_fn)(std::string)) {
        if (has_value()) return value;
        return err_fn(error);
    }

private:
    T value;
    std::string error;
};

#endif
