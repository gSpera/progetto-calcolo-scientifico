#include "argument.h"

#include "imgui.h"

#include <SDL2/SDL_image.h>
#include <iostream>
#include <format>

Argument::Argument(std::string name, MemoryType type, size_t type_size) {
    this->name = name;
    this->type = type;
    this->type_size = type_size;

    set_rows(1);
    set_cols(1);
    image_path_tmp[0] = 0;
}
Argument::Argument(std::string name, SDL_Renderer *renderer) {
    this->name = name;
    this->type = IMAGE;
    this->type_size = sizeof(uint32_t);

    set_rows(1);
    set_cols(1);

    this->image_renderer = renderer;
    strcpy(image_path_tmp, "monalisa-483x720.jpg");
}

Error<Memory> Argument::push_to_gpu(Context ctx) {
    if (this->type == IMAGE) {
        return push_to_gpu_image(ctx);
    }

    // VECTOR, MATRIX
    size_t size = this->size_as<uint8_t>();
    Error<Memory> mem_ = Memory().init(ctx, size);
    if (mem_.is_error()) { return mem_; }
    Memory mem = mem_.get_value();
    Error<Unit> ret = mem.write((void *) this->data.data());

    if (ret.is_error()) {
        return Error<Memory>().set_error(ret.get_error());
    } else {
        return Error<Memory>().set_value(mem);
    }
}

Error<Memory> Argument::push_to_gpu_image(Context ctx) {
    if (SDL_LockSurface(this->image_surface) != 0) {
        return Error<Memory>().set_error(std::format("Cannot lock image surface {}", SDL_GetError()));
    }
    Error<Memory> mem_ = Memory().init(ctx, count() * sizeof(uint32_t));
    if (mem_.is_error()) { return mem_; }
    Memory mem = mem_.get_value();
    Error<Unit> ret = mem.write((void *) this->image_surface->pixels);

    SDL_UnlockSurface(this->image_surface);
    if (ret.is_error()) {
        return Error<Memory>().set_error(ret.get_error());
    } else {
        return Error<Memory>().set_value(mem);
    }
}

Error<Unit> Argument::pop_from_gpu(Memory mem) {
    if (this->type == IMAGE) {
        return pop_from_gpu_image(mem);
    }

    Error<uint8_t *> buffer_ = mem.read();
    if (buffer_.is_error()) return Error<Unit>().set_error("Cannot read memory: " + buffer_.get_error());
    uint8_t *buffer = buffer_.get_value();
    memcpy(this->data.data(), buffer, size_as<uint8_t>());
    return Error<Unit>().set_value(unit_value);
}

Error<Unit> Argument::pop_from_gpu_image(Memory mem) {
    if (SDL_LockSurface(this->image_surface) != 0) {
        return Error<Unit>().set_error(std::format("Cannot lock image surface {}", SDL_GetError()));
    }
    Error<Unit> ret = mem.read_to((void *) this->image_surface->pixels);
    SDL_UnlockSurface(this->image_surface);

    // Update texture
    if (this->texture) SDL_DestroyTexture(this->texture);
    this->texture = SDL_CreateTextureFromSurface(this->image_renderer, this->image_surface);
    if (this->texture == nullptr) {
        return Error<Unit>().set_error(std::format("Cannot create texture from popped image: {}", SDL_GetError()));
    }

    return ret;
}

void Argument::show() {
    if (get_rows() < 1) set_rows(1);
    if (get_cols() < 1) set_cols(1);

    int rows = get_rows();
    int cols = get_cols();
    float *floatVector = view_as<float>();

    switch(this->type) {
        case VECTOR:
            ImGui::InputInt("Dimensione", &rows);
            set_rows(rows);
            floatVector = view_as<float>();

            if(ImGui::Button("Randomizza")) {
                for(int i=0;i<count();i++) {
                    floatVector[i] = ((float) (rand() % 1000)) / 10;
                }
            }

            for (int i=0;i<this->count();i++) {
                ImGui::Text(std::format("Posizione {}:", i).c_str());
                ImGui::SameLine();
                ImGui::InputScalar(std::format("##arg_{}_{}", name, i).c_str(), ImGuiDataType_Float, &floatVector[i], NULL);
            }
            break;
        case MATRIX:
            ImGui::InputInt("Righe", &rows);
            ImGui::InputInt("Colonne", &cols);
            set_rowscols(rows, cols);

            if(ImGui::Button("Randomizza")) {
                for (int row = 0; row < rows; row++) {
                    for(int col=0;col<cols;col++) {
                        floatVector[row * cols + col] = ((float) (rand() % 1000)) / 10;
                    }
                }
            }
            ImGui::BeginChild("##arg_matrix", ImVec2(-FLT_MIN, -FLT_MIN), 0, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::PushItemWidth(5 * 12.0f);
            for (int row = 0; row < rows; row++) {
                for(int col=0;col<cols;col++) {
                    if (col != 0) ImGui::SameLine();
                    ImGui::InputScalar(std::format("##arg_{}_{}_{}", name, row, col).c_str(), ImGuiDataType_Float, &floatVector[row * cols + col], NULL);
                }
            }
            ImGui::PopItemWidth();
            ImGui::EndChild();
            break;
        case IMAGE:
            ImGui::InputText("Percorso", this->image_path_tmp, IM_ARRAYSIZE(this->image_path_tmp));
            ImGui::SameLine();
            if(ImGui::Button("Carica")) {
                std::cout<<"Loading image: "<<this->image_path_tmp<<", renderer: "<<image_renderer<<std::endl;
                SDL_Surface *img = IMG_Load(this->image_path_tmp);
                if (img == nullptr) {
                    std::cout<<"Cannot load image: "<<this->image_path_tmp<<std::endl;
                    std::cout<<SDL_GetError()<<std::endl;
                }
                img = SDL_ConvertSurfaceFormat(img, SDL_PIXELFORMAT_RGB888, 0);
                if (img == nullptr) {
                    std::cout<<"Cannot convert image surface: "<<this->image_path_tmp<<std::endl;
                    std::cout<<SDL_GetError()<<std::endl;
                }
                SDL_Texture *img_texture = SDL_CreateTextureFromSurface(this->image_renderer, img);
                if (img_texture == nullptr) {
                    std::cout<<"Cannot create texture: "<<this->image_path_tmp<<std::endl;
                    std::cout<<SDL_GetError()<<std::endl;
                }

                int rows = 1;
                int cols = 1;
                if (SDL_QueryTexture(img_texture, NULL, NULL, &cols, &rows) != 0) {
                    std::cout<<"Cannot query image size: "<<SDL_GetError()<<std::endl;
                }
                set_rowscols(rows, cols);

                std::cout<<"Done loading image, dimensions: "<<rows<<" x "<<cols<<std::endl;
                this->image_surface = img;
                this->texture = img_texture;
            }

            ImGui::SameLine();
            if (ImGui::Button("Salva")) {
                if (IMG_SavePNG(this->image_surface, this->image_path_tmp) != 0) {
                    std::cout<<"Impossibile salvare immagine: "<<SDL_GetError()<<std::endl;
                }
            }

            int rows = get_rows();
            int cols = get_cols();
            ImGui::InputInt("Larghezza", &cols);
            ImGui::InputInt("Altezza", &rows);
            set_rowscols(rows, cols);

            if (ImGui::Button("Nuova immagine")) {
                this->image_surface = SDL_CreateRGBSurface(0, cols, rows, 32, 0, 0, 0, 0);
                if (this->image_surface == nullptr) {
                    std::cout<<"Cannot create surface: "<<SDL_GetError()<<std::endl;
                }
                this->texture = SDL_CreateTextureFromSurface(this->image_renderer, this->image_surface);
                if (this->texture == nullptr) {
                    std::cout<<"Cannot create texture:"<<SDL_GetError()<<std::endl;
                }
            }

            if (this->texture != nullptr)
                ImGui::Image((void *) this->texture, ImVec2(cols, rows));
            else
                ImGui::Text("Nessuna immagine caricata");
    }
}

int Argument::count() {
    return (size_t) (get_rows() * get_cols());
}

void Argument::resize() {
    size_t new_size = get_rows() * get_cols() * this->type_size;
    data.resize(new_size);
}

template<class T>
T *Argument::view_as() {
    return reinterpret_cast<T *>(data.data());
}

template<class T>
size_t Argument::size_as() {
    return data.size() / sizeof(T);
}
