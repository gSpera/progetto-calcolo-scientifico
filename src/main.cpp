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

#if !SDL_VERSION_ATLEAST(2, 0, 17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

// Main code
int main_(int, char**) {
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

int main(int argc, char** argv) {
    cl_int err = CL_SUCCESS;
    cl_uint num_platform = 0;
    cl_platform_id platform;

    err = clGetPlatformIDs(1, &platform, &num_platform);
    if (err == CL_SUCCESS) {
        printf("%d platforms\n", num_platform);
    } else {
        printf("Cannot get num platforms %d\n", err);
        return 1;
    }

    cl_uint num_devices;
    cl_device_id device;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, &num_devices);
    if (err == CL_SUCCESS) {
        printf("%d devices\n", num_devices);
    } else {
        printf("Cannot get num devices %d\n", err);
        return 1;
    }

    char name[100] = {0};
    err = clGetDeviceInfo(device, CL_DEVICE_VENDOR, sizeof(name) / sizeof(char), &name, NULL);
    if (err != CL_SUCCESS) {
        printf("Cannot retrieve device name\n");
    }
    printf("Device: %s\n", name);

    cl_context ctx = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("Cannot create context\n");
        return 1;
    }

    cl_command_queue command_queue =
        clCreateCommandQueueWithProperties(ctx, device, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("Cannot create command queue: %d\n", err);
    }

    int count = 20;
    cl_mem buff_a = clCreateBuffer(ctx, CL_MEM_READ_ONLY, sizeof(float) * count,
                                   NULL, NULL);
    cl_mem buff_b = clCreateBuffer(ctx, CL_MEM_READ_ONLY, sizeof(float) * count,
                                   NULL, NULL);
    cl_mem buff_c = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY,
                                   sizeof(float) * count, NULL, NULL);

    float buff_host[count];
    printf("Source:\n");
    for (int i = 0; i < count; i++) {
        printf("%3.1g ", buff_host[i]);
    }
    printf("\n");

    clEnqueueWriteBuffer(command_queue, buff_a, CL_FALSE, 0,
                         sizeof(float) * count, buff_host, 0, NULL, NULL);
    clEnqueueWriteBuffer(command_queue, buff_b, CL_FALSE, 0,
                         sizeof(float) * count, buff_host, 0, NULL, NULL);

    const char *prg_src = "__kernel test() {}";
    cl_program prg = clCreateProgramWithSource(ctx, 1, &prg_src, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("Cannot create program: %d\n", err);
    }
    // err = clBuildProgram(prg, 0, NULL, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("Cannot build program: %d\n", err);
    }


    //clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, 100, NULL, 0, NULL,
    //                       NULL);

    //err = clEnqueueReadBuffer(command_queue, buff_c, CL_TRUE, 0,
    //                          sizeof(float) * count, buff_host, 0, NULL, NULL);

    /*
    cl_int dev_id;
    cl_context ctx =
        clCreateContextFromType(0, CL_DEVICE_TYPE_GPU, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("Cannot create context: %d\n", err);
    }
    cl_device_id devices[num_platform];
    clGetContextInfo(ctx, devices, 0, NULL);
    clEnqueueReadBuffer(command_queue, buff_c, CL_TRUE, sizeof(float) * count,
                        buff_host, NULL, NULL, NULL);

    cl_kernel kernel = clCreateKernel(prg, "addition", &err);
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &buff_a);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &buff_b);
    clSetKernelArg(kernel, 2, sizeof(int), count);

    if (err != CL_SUCCESS) {
        printf("Cannot read memory: %d\n", err);
    }

    printf("Output:\n");
    for (int i = 0; i < count; i++) {
        printf("%f ", buff_host[i]);
    }*/
}