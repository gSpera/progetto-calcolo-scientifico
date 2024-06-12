#include "argument.h"

#include "imgui.h"

#include <SDL2/SDL_image.h>
#include <iostream>
#include <format>

Argument::Argument(std::string name, MemoryType type) {
    this->name = name;
    this->type = type;

    floatMatrixRows = 1;
    floatMatrixCols = 1;
    image_path_tmp[0] = 0;
}
Argument::Argument(std::string name, SDL_Renderer *renderer) {
    this->name = name;
    this->type = IMAGE;

    floatMatrixRows = 1;
    floatMatrixCols = 1;

    this->image_renderer = renderer;
    strcpy(image_path_tmp, "monalisa-483x720.jpg");
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
            break;
        case MATRIX:
            ImGui::InputInt("Righe", &floatMatrixRows);
            ImGui::InputInt("Colonne", &floatMatrixCols);
            this->floatVector.resize(floatMatrixRows * floatMatrixCols);

            if(ImGui::Button("Randomizza")) {
                for (int row = 0; row < floatMatrixRows; row++) {
                    for(int col=0;col<floatMatrixCols;col++) {
                        floatVector[row * floatMatrixCols + col] = ((float) (rand() % 1000)) / 10;
                    }
                }
            }
            ImGui::BeginChild("##arg_matrix", ImVec2(-FLT_MIN, -FLT_MIN), 0, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::PushItemWidth(5 * 12.0f);
            for (int row = 0; row < floatMatrixRows; row++) {
                for(int col=0;col<floatMatrixCols;col++) {
                    if (col != 0) ImGui::SameLine();
                    ImGui::InputScalar(std::format("##arg_{}_{}_{}", name, row, col).c_str(), ImGuiDataType_Float, &floatVector[row * floatMatrixCols + col], NULL);
                }
            }
            ImGui::PopItemWidth();
            ImGui::EndChild();
            break;
        case IMAGE:
            ImGui::InputText("Percorso", this->image_path_tmp, IM_ARRAYSIZE(this->image_path_tmp));
            ImGui::SameLine();
            if(ImGui::Button("Carica")) {
                std::cout<<"Loading image: "<<this->image_path_tmp<<std::endl;
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

                if (SDL_QueryTexture(img_texture, NULL, NULL, &this->floatMatrixCols, &this->floatMatrixRows) != 0) {
                    std::cout<<"Cannot query image size: "<<SDL_GetError()<<std::endl;
                }
                std::cout<<"Done loading image"<<std::endl;
                this->texture = img_texture;
            }

            ImGui::InputInt("Larghezza", &this->floatMatrixCols);
            ImGui::InputInt("Altezza", &this->floatMatrixRows);

            if (this->texture != nullptr)
                ImGui::Image((void *) this->texture, ImVec2(floatMatrixCols, floatMatrixRows));
            else
                ImGui::Text("Nessuna immagine caricata");
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
