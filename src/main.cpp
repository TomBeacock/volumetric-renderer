#include "application.h"

#include <SDL3/SDL_main.h>

int SDL_main(int argc, char *argv[])
{
    Vol::Application application{};
    return application.run();
}