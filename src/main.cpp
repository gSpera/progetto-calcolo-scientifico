#include <SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "implot.h"

#include "device.h"
#include "program.h"
#include "argument.h"
#include "context.h"
#include "memory.h"
#include "error.h"

#include <iostream>
#include <format>
#include <fstream>

#if !SDL_VERSION_ATLEAST(2, 0, 17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

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
    size_t exec_count[2] = {1, 1};
    char new_arg_buff[16];
    MemoryType new_arg_type;
    std::vector<float> benchmark_sizes = {1, 10, 100, 200, 400, 800, 1000, 10000}; // TODO: Calculate
    std::vector<float> benchmark_results;
    benchmark_results.reserve(benchmark_sizes.size());


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

    /*SDL_Texture* framebuffer_texture =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888,
                          SDL_TEXTUREACCESS_STREAMING, 800, 800);
    if (framebuffer_texture == nullptr) {
        SDL_Log("Error creating framebuffer");
        return 0;
    }
    uint32_t* framebuffer = new uint32_t[800 * 800];
    int framebuffer_pitch;*/

    // Load image
    SDL_Surface *img = IMG_Load("monalisa-483x720.jpg");
    img = SDL_ConvertSurfaceFormat(img, SDL_PIXELFORMAT_RGB888, 0);
    // SDL_Texture *img_texture = SDL_CreateTextureFromSurface(renderer, img);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
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
    auto font = io.Fonts->AddFontFromFileTTF("./FiraCode-Regular.ttf", 17.0f);
    if (font == nullptr || !font->IsLoaded()) {
        std::cout<<"Cannot load font"<<std::endl;
    } else {
        ImGui::PushFont(font);
    }

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    while (!done) {
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

        {
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
            if (kernel_names.size() > 0 && (ImGui::SameLine(), ImGui::Button("Benchmark"))) {
                benchmark_results.clear();
                for (int b_size: benchmark_sizes) {
                    std::cout<<"Preparing for benchmark with size:"<<b_size<<std::endl;
                    bool ok;
                    std::string build_log = prg.prepare_for_execution(kernel_names[current_kernel_index], exec_args, &ok);
                    build_error.append(build_log);

                    if (!ok) {
                        build_error.append("*** Benchmark FAILED, error while preparing for execution ***\n");
                        break;
                    } else {
                        cl_long exec_time;
                        build_error.append(std::format("Executing with size {}\n", b_size));
                        Error<std::string> err = prg.execute(b_size, exec_count, &exec_time);
                        if (err.is_error()) {
                            build_error.append("*** Benchmark Execution FAILED ***\n");
                            build_error.append(err.get_error());
                        } else {
                            build_error.append("*** Benchmark Execution SUCCEDDED ***\n");
                            std::cout<<std::format("Executing with size: {} took: {}ns", b_size, (size_t) exec_time)<<std::endl;
                            build_error.append(std::format("Executing with size: {} took: {}ns", b_size, (size_t) exec_time));
                            build_error.append(err.get_value());

                            for (size_t i=0;i<exec_args.size();i++) {
                                exec_args[i].pop_from_gpu(prg.mems[i]);
                            }
                            benchmark_results.push_back(((float) exec_time) / 1000 / 1000);
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
                Argument &arg = exec_args[i];
                ImGui::Text(std::format("{}: {}", i, arg.get_name()).c_str());
                ImGui::SameLine();
                ImGui::Text(memoryTypeToString(arg.get_type()).c_str());
                ImGui::SameLine();
                if(ImGui::Button(std::format("Elimina##{}", i).c_str())) {
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
                // Check if name is unique
                bool name_is_unique = true;
                for (size_t i=0;i<exec_args.size();i++) {
                    if (strcmp(new_arg_buff, exec_args[i].get_name().c_str()) == 0) {
                        std::cout<<"Name in not unique"<<std::endl;
                        name_is_unique = false;
                        break;
                    }
                }

                // Create new argument
                if (name_is_unique && std::string(new_arg_buff) != "") {
                    Argument arg("", VECTOR, 0);
                    switch(new_arg_type) {
                    case VECTOR:
                    case MATRIX:
                        arg = Argument(std::string(new_arg_buff), new_arg_type, sizeof(float));
                        exec_args.push_back(arg);
                        break;
                    case IMAGE:
                        arg = Argument(std::string(new_arg_buff), renderer);
                        exec_args.push_back(arg);
                        break;
                    }
                }
            }
            const size_t u64_one = 1;
            ImGui::InputScalar("Range esecuzione x", ImGuiDataType_U64, &exec_count[0], &u64_one);
            ImGui::InputScalar("Range esecuzione y", ImGuiDataType_U64, &exec_count[1], &u64_one);
            ImGui::End();

            for (size_t i=0;i<exec_args.size();i++) {
                Argument& arg = exec_args[i];
                ImGui::Begin(std::format("Argomento: {}", arg.get_name()).c_str());
                arg.show();
                ImGui::End();
            }

            if (benchmark_results.size() != 0) {
                ImGui::Begin("Risultati benchmark");
                if (ImGui::Button("Salva CSV")) {
                    std::ofstream fl;
                    fl.open("out.csv");
                    for (size_t i=0; i<benchmark_results.size(); i++) {
                        fl<<benchmark_sizes[i]<<","<<benchmark_results[i]<<std::endl;
                    }
                    fl.close();
                }

                if (ImPlot::BeginPlot("##result_benchmark")) {
                    ImPlot::SetupAxesLimits(0, benchmark_sizes[benchmark_sizes.size() -1] / 100, 0, benchmark_results[0]);
                    // ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
                    ImPlot::SetupAxis(ImAxis_X1, "Numero core");
                    ImPlot::SetupAxis(ImAxis_Y1, "Tempo di esecuzione (ms)");
                    ImPlot::PlotLine("##Tempo esecuzione (ms)", &benchmark_sizes[0], &benchmark_results[0], benchmark_results.size());
                    ImPlot::EndPlot();
                }

                ImGui::Text(std::format("Tempo di esecuzione sequenziale: {}ms", benchmark_results[0]).c_str());
                ImGui::End();
            }
        }

        /*SDL_LockTexture(framebuffer_texture, NULL, (void**)&framebuffer,
                        &framebuffer_pitch);
        for (int i = 0; i < 800; i++) {
            framebuffer[i + i * 800] = 0xFFFFFFFF;
        }
        SDL_UnlockTexture(framebuffer_texture);*/

        // Rendering
        ImGui::Render();
        SDL_RenderClear(renderer);

        /*int err = SDL_RenderCopy(renderer, framebuffer_texture, NULL, NULL);
        if (err != 0) {
            printf("Error: %s\n", SDL_GetError());
            return 1;
        }*/

        SDL_SetRenderDrawColor(renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
