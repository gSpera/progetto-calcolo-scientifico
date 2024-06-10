// Dear ImGui: standalone example application for SDL2 + SDL_Renderer
// (SDL is a cross-platform general purpose library for handling windows,
// inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/
// folder).
// - Introduction, links and more at the top of imgui.cpp

// Important to understand: SDL_Renderer is an _optional_ component of SDL2.
// For a multi-platform app consider using e.g. SDL+DirectX on Windows and
// SDL+OpenGL on Linux/OSX.

#include <SDL.h>
#include <stdio.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

#include "opencl.h"
#include "program.h"

#include <iostream>

#if !SDL_VERSION_ATLEAST(2, 0, 17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

void change_device(int i) {
}
// Main code
int main(int, char**) {
    Error<std::vector<Device>> devices_ = enumerateDevices();
    if (devices_.is_error()) {
        std::cout<<"Cannot enumarate devices: "<< devices_.get_error()<<std::endl;
        return 1;
    }
    auto devices = devices_.get_value();

    for (size_t i=0;i<devices.size(); i++) {
        devices[i].init().value_or_exit([](std::string error) {
            std::cout<<"Cannot init device:" << error <<std::endl;
        });

        std::cout<<"Device: "<< devices[i].name() << " ("<<devices[i].device_id()<<")" <<std::endl;
    }

    size_t current_device_index;
    Device current_device;
    Context context;
    auto change_device = [&](size_t index) {
        if (index > devices.size()) {
            std::cout<<"Called change_device with index ("<<index<<") > ("<<devices.size()<<") ignoring"<<std::endl;
            return;
        }

        current_device_index = index;
        current_device = devices[index];
        context = Context().init(current_device).value_or_exit([](std::string error) {
            std::cout<<"Cannot change device: " << error<<std::endl;
        });
    };

    change_device(0);

    // State variables
    std::string build_error;
    std::vector<std::string> kernel_names;
    size_t current_kernel_index = 0;
    Program prg;
    std::vector<Argument> exec_args;
    size_t n_cores = 1024;
    size_t exec_count = 1;
    char new_arg_buff[16];
    MemoryType new_arg_type;

    char editor_source[1024 * 16] = {0};
    FILE *fp = fopen("kernel_opencl.c", "r");
    if (fp == NULL) {
        std::cout<<"Cannot open kernel source\n"<<std::endl;
        return 1;
    }
    fread(editor_source, 1024*16, 1, fp);
    if (ferror(fp) != 0) {
        printf("Cannot read kernel: %d\n", ferror(fp));
        return 1;
    }

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) !=
        0) {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Create window with SDL_Renderer graphics context
    SDL_WindowFlags window_flags =
        (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow(
        "Progetto Calcolo Scientifico Spera Giovanni", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    if (window == nullptr) {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        SDL_Log("Error creating SDL_Renderer!");
        return 0;
    }
    SDL_RendererInfo info;
    SDL_GetRendererInfo(renderer, &info);
    SDL_Log("Current SDL_Renderer: %s", info.name);

    SDL_Texture* framebuffer_texture =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888,
                          SDL_TEXTUREACCESS_STREAMING, 800, 800);
    if (framebuffer_texture == nullptr) {
        SDL_Log("Error creating framebuffer");
        return 0;
    }
    uint32_t* framebuffer = new uint32_t[800 * 800];
    int framebuffer_pitch;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    // Setup Dear ImGui style
    // ImGui::StyleColorsDark();
    ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can
    // also load multiple fonts and use ImGui::PushFont()/PopFont() to select
    // them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you
    // need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr.
    // Please handle those errors in your application (e.g. use an assertion, or
    // display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and
    // stored into a texture when calling
    // ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame
    // below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use
    // Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string
    // literal you need to write a double backslash \\ !
    // io.Fonts->AddFontDefault();
    // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    // ImFont* font =
    // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f,
    // nullptr, io.Fonts->GetGlyphRangesJapanese()); IM_ASSERT(font != nullptr);
    auto font = io.Fonts->AddFontFromFileTTF("./FiraCode-Regular.ttf", 13.0f);
    if (font == nullptr || !font->IsLoaded()) {
        std::cout<<"Cannot load font"<<std::endl;
    } else {
        ImGui::PushFont(font);
    }

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop

    bool done = false;
    while (!done) {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to
        // tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data
        // to your main application, or clear/overwrite your copy of the mouse
        // data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input
        // data to your main application, or clear/overwrite your copy of the
        // keyboard data. Generally you may always pass all inputs to dear
        // imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) done = true;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // 2. Show a simple window that we create ourselves. We use a Begin/End
        // pair to create a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Finestra");  // Create a window called "Hello, world!"
                                       // and append into it.

            ImGui::Text("Lorem ipsum");  // Display some text (you can use a
                                         // format strings too)

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
            ImGui::ColorEdit3("Colore", (float*)&clear_color);

            if (ImGui::Button("Button")) counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                        1000.0f / io.Framerate, io.Framerate);
            
            ImGui::End();

            ImGui::Begin("Editor");
            ImGui::Text("Dispositivo:");
            if(ImGui::BeginCombo("##device_select", current_device.name().c_str(), 0)) {
                for (size_t i=0;i<devices.size(); i++) {
                    if(ImGui::Selectable(devices[i].name().c_str(), current_device_index == i)) {
                        change_device(i);
                    }
                }
                ImGui::EndCombo();
            }
            if (kernel_names.size() > 0) {
                ImGui::Text("Kernel:");
                if(ImGui::BeginCombo("##kernel_select", kernel_names[current_kernel_index].c_str(), 0)) {
                    for(size_t i=0;i<kernel_names.size(); i++) {
                        if(ImGui::Selectable(kernel_names[i].c_str(), current_kernel_index == i)) {
                            current_kernel_index = i;
                        }
                    }
                    ImGui::EndCombo();
                }
            }

            if (ImGui::Button("Compila")) {
                std::cout<<"Compiling"<<std::endl;
                prg = Program(context, editor_source);
                bool ok;
                build_error = prg.build(&ok);

                auto kernels_ = prg.kernel_names();
                if (kernels_.is_error()) {
                    std::cout<<"Cannot get kernel names: "<< kernels_.get_error() << std::endl;
                }
                kernel_names = kernels_.get_value();

                if (!ok) {
                    std::cout<<"Cannot build program"<<std::endl;
                    std::cout<<build_error<<std::endl;
                }
            }
            if(kernel_names.size() > 0 && (ImGui::SameLine(), ImGui::Button("Esegui"))) {
                std::cout<<"Preparing for execution"<<std::endl;
                bool ok;
                std::string build_log = prg.prepare_for_execution(kernel_names[current_kernel_index], exec_args, &ok);
                build_error.append(build_log);

                if (ok) {
                    cl_long exec_time;
                    build_error.append("Executing\n");
                    Error<std::string> err = prg.execute(n_cores, exec_count, &exec_time);
                    if (err.is_error()) {
                        build_error.append("*** Execution FAILED ***\n");
                        build_error.append(err.get_error());
                    } else {
                        build_error.append("*** Execution SUCCEDDED ***\n");
                        build_error.append(err.get_value());

                        for (size_t i=0;i<exec_args.size();i++) {
                            exec_args[i].pop_from_gpu(prg.mems[i]);
                        }
                    }
                }
            }

            ImGui::InputTextMultiline("##editor", editor_source, IM_ARRAYSIZE(editor_source), ImVec2(-FLT_MIN, -FLT_MIN), ImGuiInputTextFlags_AllowTabInput);
            ImGui::End();

            if (build_error != "") {
                ImGui::Begin("Log");
                ImGui::InputTextMultiline("Build Error", (char *) build_error.c_str(), build_error.size(), ImVec2(-FLT_MIN, -FLT_MIN), ImGuiInputTextFlags_ReadOnly);
                ImGui::End();
            }

            ImGui::Begin("Argomenti");
            for (size_t i=0;i<exec_args.size();i++) {
                Argument arg = exec_args[i];
                ImGui::Text(std::format("{}: {}", i, arg.get_name()).c_str());
                ImGui::SameLine();
                ImGui::Text(memoryTypeToString(arg.get_type()).c_str());
                ImGui::SameLine();
                if(ImGui::Button("Elimina")) {
                    exec_args.erase(exec_args.begin() + i);
                }
                ImGui::SameLine();
                if(ImGui::Button(std::format("/\\##{}", i).c_str()) && i != 0) {
                    Argument tmp = exec_args[i-1];
                    exec_args[i-1] = exec_args[i];
                    exec_args[i] = tmp;
                }
                ImGui::SameLine();
                if(ImGui::Button(std::format("\\/##{}", i).c_str()) && i != exec_args.size()-1) {
                    Argument tmp = exec_args[i+1];
                    exec_args[i+1] = exec_args[i];
                    exec_args[i] = tmp;
                }
            }

            ImGui::SeparatorText("Nuovo argomento");
            ImGui::Text("Nome:");
            ImGui::SameLine();
            ImGui::InputText("##new_arg_name", new_arg_buff, IM_ARRAYSIZE(new_arg_buff), 0);
            ImGui::Text("Tipo:");
            ImGui::SameLine();
            if(ImGui::BeginCombo("##new_arg_type", memoryTypeToString(new_arg_type).c_str(), 0)) {
                #define X(X) if(ImGui::Selectable(memoryTypeToString(X).c_str(), X == new_arg_type)) { new_arg_type = X; }
                X(VECTOR);
                X(MATRIX);
                X(IMAGE);
                #undef X
                ImGui::EndCombo();
            }

            if (ImGui::Button("Aggiungi")) {
                // Create new argument
                if (std::string(new_arg_buff) != "") {
                    Argument arg = Argument(std::string(new_arg_buff), new_arg_type);
                    exec_args.push_back(arg);
                }
            }
            const size_t u64_one = 1;
            ImGui::InputScalar("Range esecuzione", ImGuiDataType_U64, &exec_count, &u64_one);
            ImGui::End();

            for (size_t i=0;i<exec_args.size();i++) {
                Argument& arg = exec_args[i];
                ImGui::Begin(std::format("Argomento: {}", arg.get_name()).c_str());
                arg.show();
                ImGui::End();
            }
        }

        SDL_LockTexture(framebuffer_texture, NULL, (void**)&framebuffer,
                        &framebuffer_pitch);
        for (int i = 0; i < 800; i++) {
            framebuffer[i + i * 800] = 0xFFFFFFFF;
        }
        SDL_UnlockTexture(framebuffer_texture);

        // Rendering
        ImGui::Render();
        SDL_RenderClear(renderer);

        int err = SDL_RenderCopy(renderer, framebuffer_texture, NULL, NULL);
        if (err != 0) {
            printf("Error: %s\n", SDL_GetError());
            return 1;
        }

        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

#include <CL/cl.h>

#include <iostream>

#define TYPE_CL cl_float

int main_(int argc, char** argv) {
    cl_int err = CL_SUCCESS;
    cl_uint num_platform = 0;
    cl_platform_id platforms[10];
    cl_platform_id platform;

    cl_uint num_devices;
    cl_device_id devices[10];
    cl_device_id device;

    err = clGetPlatformIDs(10, platforms, &num_platform);
    if (err == CL_SUCCESS) {
        printf("%d platforms\n", num_platform);
    } else {
        printf("Cannot get num platforms %d\n", err);
        return 1;
    }
    platform = platforms[0];
    for (cl_uint i=0;i<num_platform; i++) {
        char name[100];
        err = clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 100, &name, NULL);
        if (err != CL_SUCCESS) printf("Cannot get platform info: %d\n", err);
        err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 10, devices, &num_devices);
        if (err != CL_SUCCESS) printf("Cannot get device ids: %d\n", err);
        printf("Platform: %s %d devices\n", name, num_devices);
        for (cl_uint j=0;j<num_devices;j++) {
            err = clGetDeviceInfo(devices[j], CL_DEVICE_NAME, 100, &name, NULL);
            if (err != CL_SUCCESS) printf("Cannot get device info: %d\n", err);
            printf(" - Device: %s\n", name);
        }
    }

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 10, &device, &num_devices);
    if (err != CL_SUCCESS) {
        printf("Cannot get num devices %d\n", err);
        return 1;
    }

    char name[100];
    err = clGetDeviceInfo(device, CL_DEVICE_NAME, 100, &name, NULL);
    if (err != CL_SUCCESS) printf("Cannot get device info: %d\n", err);
    printf("Chosed Device: %s\n", name);

    cl_uint max_compute_unit;
    err = clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(max_compute_unit), &max_compute_unit, NULL);
    if (err != CL_SUCCESS) printf("Cannot get device max compute units: %d\n", err);
    printf("Max compute unit: %d\n", max_compute_unit);

    size_t max_work_group_size;
    err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(max_work_group_size), &max_work_group_size, NULL);
    if (err != CL_SUCCESS) printf("Cannot get max work group size: %d\n", err);
    printf("Max work group size: %lu\n", max_work_group_size);

    size_t max_work_item[256];
    size_t max_work_group_size_count;
    err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(max_work_item), &max_work_group_size, &max_work_group_size_count);
    printf("Max work work group size: %lu %lu\n", max_work_group_size_count, max_work_group_size_count / sizeof(size_t));
    max_work_group_size_count /= sizeof(size_t);
    for (size_t i=0;i<max_work_group_size_count;i++) {
        // printf("Max work item on dimension %lu: %lu\n", i, max_work_item[i]);
    }

    cl_device_partition_property sub_device_properties[] = {
        CL_DEVICE_PARTITION_BY_COUNTS,
        4,
        CL_DEVICE_PARTITION_BY_COUNTS_LIST_END,
        0,
    };

    cl_uint max_subdevices;
    err = clGetDeviceInfo(device, CL_DEVICE_PARTITION_MAX_SUB_DEVICES, sizeof(max_subdevices), &max_subdevices, NULL);
    if (err != CL_SUCCESS) {
        printf("Cannot get device max subdevices: %d\n", err);
    }
    printf("Max subdevices: %d\n", max_subdevices);

    cl_device_partition_property partition_properties[10];
    size_t partition_properties_count;
    err = clGetDeviceInfo(device, CL_DEVICE_PARTITION_PROPERTIES, sizeof(partition_properties), &partition_properties, &partition_properties_count);
    printf("Size: %lu %lu\n", partition_properties_count, partition_properties_count / sizeof(cl_device_partition_property));
    partition_properties_count /= sizeof(cl_device_partition_property);
    if (err != CL_SUCCESS) {
        printf("Cannot get partition properties: %d\n", err);
    }
    for (size_t i=0;i<partition_properties_count; i++) {
        printf("- Device support: %ld ", partition_properties[i]);
        switch (partition_properties[i]) {
            case 0:
                puts("Not Supported");
            case CL_DEVICE_PARTITION_EQUALLY:
                puts("Equally");
                break;
            case CL_DEVICE_PARTITION_BY_COUNTS:
                puts("By Count");
                break;
            case CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN:
                puts("By Affinity Domain");
                break;
            default:
                puts("Unkown");
        }
    }

    cl_uint sub_devices_count;
    err = clCreateSubDevices(device, sub_device_properties, 0, NULL, &sub_devices_count);
    if (err != CL_SUCCESS) {
        printf("Cannot estimate subdevices count: %d\n", err);
    }
    printf("Estimated subdevices: %d\n", sub_devices_count);
    err = clCreateSubDevices(device, sub_device_properties, 10, devices, &sub_devices_count);
    if (err != CL_SUCCESS) {
        printf("Cannot create subdevice: %d\n", err);
    }
    switch(err) {
        case CL_SUCCESS:
            break;
        case CL_INVALID_DEVICE:
            puts(" - Invalid device");
            break;
        case CL_INVALID_VALUE:
            printf(" - Invalid value\n");
            break;
        case CL_INVALID_DEVICE_PARTITION_COUNT:
            printf(" - Invalid partition count\n");
            break;
        case CL_DEVICE_PARTITION_FAILED:
            printf(" - Partitioning failed\n");
            break;
        case CL_OUT_OF_RESOURCES:
            printf(" - Out of resources\n");
            break;
        case CL_OUT_OF_HOST_MEMORY:
            printf(" - Out of host memory\n");
            break;
        default:
            puts(" - Unown error");
    }
    printf("Created %d sub-devices\n", sub_devices_count);

    cl_context ctx = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("Cannot create context\n");
        return 1;
    }

    cl_command_queue_properties queue_properties[] = {CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0};
    cl_command_queue command_queue =
        clCreateCommandQueueWithProperties(ctx, device, queue_properties, &err);
    if (err != CL_SUCCESS) {
        printf("Cannot create command queue: %d\n", err);
    }

    clFinish(command_queue);

    size_t count = 20;
    cl_mem buff_a = clCreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, sizeof(TYPE_CL) * count,
                                   NULL, &err);
    if (err != CL_SUCCESS) printf("Cannot allocate buffer a: %d\n", err);
    cl_mem buff_b = clCreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, sizeof(TYPE_CL) * count,
                                   NULL, &err);
    if (err != CL_SUCCESS) printf("Cannot allocate buffer b: %d\n", err);
    cl_mem buff_c = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
                                   sizeof(TYPE_CL) * count, NULL, &err);
    if (err != CL_SUCCESS) printf("Cannot allocate buffer c: %d\n", err);

    clFinish(command_queue);
    TYPE_CL buff_host[count] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    printf("Source:\n");
    for (size_t i = 0; i < count; i++) {
        printf("%3.2f ", buff_host[i]);
    }
    printf("\n");

    clEnqueueWriteBuffer(command_queue, buff_a, CL_FALSE, 0,
                         sizeof(TYPE_CL) * count, buff_host, 0, NULL, NULL);
    clEnqueueWriteBuffer(command_queue, buff_b, CL_FALSE, 0,
                         sizeof(TYPE_CL) * count, buff_host, 0, NULL, NULL);
                        
    clFinish(command_queue);

    FILE *fp = fopen("kernel_opencl.c", "r");
    if (fp == NULL) {
        printf("Cannot open kernel source\n");
        return 1;
    }
    fseek(fp, 0, SEEK_END);
    size_t fp_size = ftell(fp);
    char *prg_src = (char *) malloc(fp_size);
    fseek(fp, 0, SEEK_SET);
    fread(prg_src, fp_size, 1, fp);
    if (ferror(fp) != 0) {
        printf("Cannot read kernel: %d\n", ferror(fp));
        return 1;
    }


    cl_program prg = clCreateProgramWithSource(ctx, 1, (const char **) &prg_src, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("Cannot create program: %d\n", err);
    }
    err = clBuildProgram(prg, 0, NULL, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("Cannot build program: %d\n", err);
    }

    char buffer[1024];
    err = clGetProgramBuildInfo(prg, device, CL_PROGRAM_BUILD_LOG, sizeof(buffer) / sizeof(char), &buffer, NULL);
    if (err != CL_SUCCESS) {
        printf("Cannot get program build info: %d\n", err);
    }
    printf("%s\n", buffer);

    cl_kernel kernel = clCreateKernel(prg, "test", &err);
    if (err != CL_SUCCESS) {
        printf("Cannot build kernel: %d\n", err);
    }

    err = clSetKernelArg(kernel, 0, sizeof(buff_a), &buff_a);
    if (err != CL_SUCCESS) printf("Cannot set kernel arg 0: %d\n", err);
    err = clSetKernelArg(kernel, 1, sizeof(buff_b), &buff_b);
    if (err != CL_SUCCESS) printf("Cannot set kernel arg 1: %d\n", err);
    err = clSetKernelArg(kernel, 2, sizeof(buff_c), &buff_c);
    if (err != CL_SUCCESS) printf("Cannot set kernel arg 2: %d\n", err);

    cl_ulong start_time; //ns
    cl_ulong end_time; //ns
    cl_event event;

    size_t n_cores = 100;
    int n_invocations = count / n_cores;
    size_t remain = count % n_cores;
    cl_ulong total_time = 0;

    for (int i=0; i<n_invocations; i++) {
        size_t offset = i * n_cores;
        printf("Invoking from %ld to %ld\n", offset, offset + n_cores);
        err = clEnqueueNDRangeKernel(command_queue, kernel,
            1, &offset, &n_cores, // Dim, Offset, Size
            NULL, // Local Work Size
        0, NULL, &event);
        clFinish(command_queue);
        clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(start_time), &start_time, NULL);
        clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(end_time), &end_time, NULL);
        cl_ulong time_ns = end_time - start_time;
        total_time += time_ns;
        printf("Took: %ldns, %ldus\n", time_ns, time_ns / 1000);
    }
    if (remain > 0) {
        size_t offset = count - remain;
        err = clEnqueueNDRangeKernel(command_queue, kernel,
            1, &offset, &remain, // Dim, Offset, Size
            NULL, // Local Work Size
        0, NULL, &event);
        clFinish(command_queue);
        clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(start_time), &start_time, NULL);
        clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(end_time), &end_time, NULL);
        cl_ulong time_ns = end_time - start_time;
        total_time += time_ns;
        printf("Took: %ldns, %ldus\n", time_ns, time_ns / 1000);
    }

    printf("Total Took: %ldns, %ldus\n", total_time, total_time / 1000);

    // err = clEnqueueNDRangeKernel(command_queue, kernel,
    //     1, &offset, &count, // Dim, Offset, Size
    //     NULL, // Local Work Size
    // 0, NULL, &event);
    // clFinish(command_queue);
    // clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(start_time), &start_time, NULL);
    // clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(end_time), &end_time, NULL);
    // cl_ulong time_ns = end_time - start_time;
    // printf("Took: %ldns, %ldus\n", time_ns, time_ns / 1000);

    if (err != CL_SUCCESS) printf("Cannot enqueue nd range: %d\n", err);
    err = clFinish(command_queue);
    if (err != CL_SUCCESS) { printf("Cannot finish: %d\n", err); }

    err = clEnqueueReadBuffer(command_queue, buff_c, CL_TRUE, 0, sizeof(TYPE_CL) * count, &buff_host, 0, NULL, NULL);
    if (err != CL_SUCCESS) { printf("Cannot read buffer: %d\n", err); }

    printf("After:\n");
    for (size_t i = 0; i < count; i++) {
        printf("%3.2f ", buff_host[i]);
    }
    printf("\n");

    printf("DONE\n");
    return 0;
}