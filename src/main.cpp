#include <SDL.h>
#include <SDL_main.h>
#include <nfd.h>
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>

#include "imgui_context.h"
#include "ui/ui_context.h"
#include "vulkan_context.h"

int SDL_main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    NFD_Init();

    SDL_WindowFlags window_flags =
        (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN |
                          SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_MAXIMIZED);
    SDL_Window *window = SDL_CreateWindow("Template", 1280, 720, window_flags);

    Vol::VulkanContext *vulkan_context = new Vol::VulkanContext(window);
    Vol::ImGuiContext *imgui_context =
        new Vol::ImGuiContext(window, vulkan_context);

    Vol::UI::UIContext ui_context{};

    bool running = true;
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

        ui_context.update();

        bool minimized = SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED;
        if (!minimized) {
            vulkan_context->begin_render_pass();
            imgui_context->render();
            vulkan_context->end_render_pass();
        } else {
            imgui_context->end_frame();
        }
    }

    vulkan_context->wait_till_idle();

    delete imgui_context;
    delete vulkan_context;

    SDL_DestroyWindow(window);

    NFD_Quit();
    SDL_Quit();

    return 0;
}