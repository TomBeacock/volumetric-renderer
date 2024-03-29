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
target_link_libraries(${PROJECT_NAME} INTERFACE glm::glm)

# freetype
message(STATUS "Building freetype...")
add_subdirectory("freetype")

# imgui
message(STATUS "Building imgui...")
add_library(imgui STATIC
	"imgui/imgui.h" "imgui/imgui.cpp"
	"imgui/imgui_demo.cpp"
	"imgui/imgui_draw.cpp"
	"imgui/imgui_internal.h"
	"imgui/imgui_tables.cpp"
	"imgui/imgui_widgets.cpp"
	"imgui/imstb_rectpack.h"
	"imgui/imstb_textedit.h"
	"imgui/misc/freetype/imgui_freetype.h" "imgui/misc/freetype/imgui_freetype.cpp"
	"imgui/misc/cpp/imgui_stdlib.h" "imgui/misc/cpp/imgui_stdlib.cpp"
	"imgui/backends/imgui_impl_sdl3.h" "imgui/backends/imgui_impl_sdl3.cpp"
	"imgui/backends/imgui_impl_vulkan.h" "imgui/backends/imgui_impl_vulkan.cpp"
)

target_link_libraries(imgui PRIVATE SDL3::SDL3 Vulkan::Vulkan freetype)
target_include_directories(imgui PUBLIC "${PROJECT_SOURCE_DIR}/imgui")
target_include_directories(imgui INTERFACE "${PROJECT_SOURCE_DIR}/imgui/backends")
target_compile_definitions(imgui PRIVATE IMGUI_ENABLE_FREETYPE)

target_link_libraries(${PROJECT_NAME} INTERFACE imgui)

# native file dialog extended
message(STATUS "Building native file dialog extended...")
add_subdirectory(nativefiledialog-extended)
target_link_libraries(${PROJECT_NAME} INTERFACE nfd)

# NrrdIO
message(STATUS "Building NrrdIO...")
set(QNANHIBIT ON)
add_subdirectory("NrrdIO")
target_compile_definitions(NrrdIO PUBLIC TEEM_STATIC)
target_include_directories(NrrdIO INTERFACE "${PROJECT_SOURCE_DIR}/NrrdIO")
target_link_libraries(${PROJECT_NAME} INTERFACE NrrdIO)