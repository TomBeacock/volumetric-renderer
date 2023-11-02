project(extern)

add_library(${PROJECT_NAME} INTERFACE)

# Vulkan
message(STATUS "Finding Vulkan SDK")
find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} INTERFACE	Vulkan::Vulkan)

# SDL
message(STATUS "Building SDL...")
set(SDL_SHARED OFF)
set(SDL_STATIC ON)
set(SDL_TEST_LIBRARY OFF)
add_subdirectory("SDL")
target_link_libraries(${PROJECT_NAME} INTERFACE SDL3::SDL3)

# glm
message(STATUS "Building glm...")
add_subdirectory("glm")
target_link_libraries(${PROJECT_NAME} INTERFACE glm)
