#include "opencl.h"
#include "error.h"

#include "imgui.h"

#include <format>
#include <iostream>

#include <SDL2/SDL_image.h>

Device::Device(cl_platform_id platform, cl_device_id device) {
        this->platform = platform;
        this->device = device;
}
std::string Device::name() { return this->m_name; }

Error<Unit> Device::init() {
    Error<Unit> ret;
    char platform_name[50];
    char device_name[100];

    cl_int err = clGetPlatformInfo(platform, CL_PLATFORM_NAME, 50, platform_name, NULL);
    if (err != CL_SUCCESS) return ret.set_error(std::format("Cannot get platform {} name", (long) platform));

    err = clGetDeviceInfo(device, CL_DEVICE_NAME, 100, &device_name, NULL);
    if (err != CL_SUCCESS) return ret.set_error(std::format("Cannot get device {}:{} name", (long) platform, (long) device));

    this->m_name = std::string(std::string(platform_name) + " - " + std::string(device_name));

    return ret.set_value(unit_value);
}

Error<std::vector<Device>> enumerateDevices() {
    Error<std::vector<Device>> ret;
    cl_uint num_platform;
    cl_platform_id platforms[10];

    cl_int err = clGetPlatformIDs(10, platforms, &num_platform);
    if (err != CL_SUCCESS) {
        return ret.set_error("Cannot get platforms");
    }

    std::vector<Device> values;

    for (cl_uint i=0;i<num_platform; i++) {
        cl_platform_id platform = platforms[i];
        cl_uint num_devices;
        cl_device_id devices[10];

        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 10, devices, &num_devices);
        if (err != CL_SUCCESS) {
            return ret.set_error(std::format("Cannot get devices for platform"));
        }

        for (cl_uint j=0;j<num_devices; j++) {
            values.push_back(Device(platform, devices[j]));
        }
    }

    return ret.set_value(values);
}

Error<Context> Context::init(Device dev) {
    Error<Context> ret;
    cl_int err;
    cl_device_id device_id = dev.device_id();
    this->ctx = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        return ret.set_error(std::format("Cannot create context"));
    }

    cl_command_queue_properties queue_properties[] = {CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0};
    this->queue = clCreateCommandQueueWithProperties(ctx, device_id, queue_properties, &err);
    if (err != CL_SUCCESS) {
        return ret.set_error(std::format("Cannot create command queue"));
    }

    this->dev = dev;
    return ret.set_value(*this);
}

Context::~Context() {
    // TODO: Implement
}


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