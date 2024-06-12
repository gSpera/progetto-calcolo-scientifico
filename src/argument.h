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
    int count();

private:
    MemoryType type;
    std::string name;

    int m_rows;
    int m_cols;
    void resize();
    void set_rows(int rows) { this->m_rows = rows; resize(); }
    void set_cols(int cols) { this->m_cols = cols; resize(); }
    void set_rowscols(int rows, int cols) { this->m_rows = rows; this->m_cols = cols; resize(); }
    int get_rows() { return this->m_rows; }
    int get_cols() { return this->m_cols; }

    std::vector<uint8_t> data;
    template<class T>
    T *view_as();

    template<class T>
    size_t size_as();

    // std::vector<float> floatVector;
    SDL_Texture *texture;
    char image_path_tmp[128];
    SDL_Renderer *image_renderer;
};

#endif
