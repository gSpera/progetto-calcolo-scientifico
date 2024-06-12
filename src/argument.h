#ifndef H_ARGUMENT
#define H_ARGUMENT

#include <SDL.h>
#include <CL/cl.h>
#include <vector>

#include "memory.h"
#include "error.h"

class Argument {
public:
    Argument(std::string name, MemoryType type);
    Argument(std::string name, SDL_Renderer *renderer);
    std::string get_name() {return this->name; }
    MemoryType get_type() {return this->type; }
    Error<Memory> push_to_gpu(Context ctx);
    Error<Unit> pop_from_gpu(Memory mem);
    void show();
    size_t size();
private:
    MemoryType type;
    std::string name;

    int floatMatrixRows;
    int floatMatrixCols;
    std::vector<float> floatVector;
    SDL_Texture *texture;
    char image_path_tmp[128];
    SDL_Renderer *image_renderer;
};

#endif
