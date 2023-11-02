#include <SDL.h>
#include <SDL_main.h>
#include <vulkan/vulkan.h>

#include <iostream>
#include <memory>
#include <stdexcept>

#include "vulkan_context.h"

int SDL_main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = SDL_CreateWindow(
        "Template", 1280, 720, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN
    );

    VulkanContext *vulkan;
    try {
        vulkan = new VulkanContext(window);
    } catch (std::runtime_error e) {
        std::cout << e.what() << std::endl;
    }

    SDL_ShowWindow(window);

    bool running = true;
    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
                    vulkan->framebuffer_size_changed();
                    break;
                }
                case SDL_EVENT_QUIT: {
                    running = false;
                    break;
                }
            }
        }

        bool minimized = SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED;
        if (!minimized) {
            vulkan->draw_frame();
        }
    }

    vulkan->wait_till_idle();
    delete vulkan;

    SDL_DestroyWindow(window);

    return 0;
}
