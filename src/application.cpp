#include "application.h"

#include "data/importer.h"
#include "imgui_context.h"
#include "ui/ui_context.h"
#include "vulkan_context.h"

#include <SDL3/SDL.h>
#include <nfd.h>

#include <cassert>

Vol::Application *Vol::Application::instance = nullptr;

Vol::Application::Application()
    : running(false),
      window(nullptr),
      vulkan_context(nullptr),
      imgui_context(nullptr),
      ui_context(nullptr),
      importer(nullptr)
{
    assert(instance == nullptr);
    instance = this;

    SDL_Init(SDL_INIT_VIDEO);
    NFD_Init();

    SDL_WindowFlags window_flags =
        (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN |
                          SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_MAXIMIZED);
    SDL_Window *window = SDL_CreateWindow("Template", 1280, 720, window_flags);

    vulkan_context = new VulkanContext(window);
    imgui_context = new ImGuiContext(window, vulkan_context);
    ui_context = new UI::UIContext();
    importer = new Data::Importer();
}

Vol::Application::~Application()
{
    vulkan_context->wait_till_idle();

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
        // Handle events
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            imgui_context->process_event(&e);
            switch (e.type) {
                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
                    vulkan_context->framebuffer_size_changed();
                    break;
                }
                case SDL_EVENT_QUIT: {
                    running = false;
                    break;
                }
            }
        }

        // Update immediate mode ui
        ui_context->update();

        // Render frame
        bool minimized = SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED;
        if (!minimized) {
            vulkan_context->begin_render_pass();
            imgui_context->render();
            vulkan_context->end_render_pass();
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
