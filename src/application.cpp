#include "application.h"

#include "data/importer.h"
#include "rendering/main_pass.h"
#include "rendering/vulkan_context.h"
#include "scene/scene.h"
#include "ui/imgui_context.h"
#include "ui/ui_context.h"

#include <SDL3/SDL.h>
#include <nfd.h>

#include <cassert>
#include <numeric>

Vol::Application *Vol::Application::instance = nullptr;

double calculate_framerate();

Vol::Application::Application()
    : running(false),
      window(nullptr),
      vulkan_context(nullptr),
      imgui_context(nullptr),
      ui_context(nullptr),
      importer(std::make_unique<Data::Importer>()),
      scene(std::make_unique<Scene::Scene>())
{
    assert(instance == nullptr);
    instance = this;

    SDL_Init(SDL_INIT_VIDEO);
    NFD_Init();

    SDL_WindowFlags window_flags =
        (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN |
                          SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_MAXIMIZED);
    SDL_Window *window = SDL_CreateWindow("Template", 1280, 720, window_flags);

    vulkan_context = new Rendering::VulkanContext(window);
    imgui_context = new UI::ImGuiContext(window, vulkan_context);
    ui_context = new UI::UIContext();
}

Vol::Application::~Application()
{
    vulkan_context->wait_till_idle();

    delete ui_context;
    delete imgui_context;
    delete vulkan_context;

    SDL_DestroyWindow(window);

    NFD_Quit();
    SDL_Quit();
}

int Vol::Application::run()
{
    running = true;
    while (running) {
        // Calculate frame rate
        float framerate = calculate_framerate();

        // Handle events
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            imgui_context->process_event(&e);
            switch (e.type) {
                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
                    vulkan_context->get_main_pass()->framebuffer_size_changed();
                    break;
                }
                case SDL_EVENT_QUIT: {
                    running = false;
                    break;
                }
            }
        }

        // Update immediate mode ui
        ui_context->get_main_window().set_framerate(framerate);
        ui_context->update();

        // Render frame
        bool minimized = SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED;
        if (!minimized) {
            vulkan_context->render();
        } else {
            imgui_context->end_frame();
        }
    }
    return 0;
}

Vol::Application &Vol::Application::main()
{
    return *instance;
}

double calculate_framerate()
{
    static std::vector<double> frame_times(5, 0.0f);
    static size_t i = 0;
    static uint64_t last_time = SDL_GetTicks();

    // Add current frame duration
    uint64_t current_time = SDL_GetTicks();
    double frame_time = static_cast<double>(current_time - last_time) / 1000.0f;
    frame_times[i] = frame_time;

    // Update for next frame
    i = (i + 1) % frame_times.size();
    last_time = current_time;

    // Calculate new framerate
    double average_frame_time =
        std::accumulate(frame_times.begin(), frame_times.end(), 0.0f) /
        static_cast<double>(frame_times.size());
    return 1.0f / average_frame_time;
}
