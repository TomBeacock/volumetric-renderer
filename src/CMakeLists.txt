cmake_minimum_required(VERSION 3.12)

project(volumetric-renderer)

message(STATUS "Building executable...")

if(WIN32)
	set(OPTIONS WIN32)
endif()

add_executable(${PROJECT_NAME} ${OPTIONS}
	"main.cpp"
	"application.h" "application.cpp"

	"rendering/vulkan_context.h" "rendering/vulkan_context.cpp"
	"rendering/main_pass.h" "rendering/main_pass.cpp"
	"rendering/offscreen_pass.h" "rendering/offscreen_pass.cpp"
	"rendering/util.h" "rendering/util.cpp"
	 
	"ui/imgui_context.h" "ui/imgui_context.cpp"
	"ui/ui_context.h" "ui/ui_context.cpp"
	"ui/main_window.h" "ui/main_window.cpp"
	"ui/error_popup.h" "ui/error_popup.cpp"
	"ui/components/gradient.h" "ui/components/gradient.cpp"
	"ui/components/image_rounded.h" "ui/components/image_rounded.cpp"
	"ui/components/attribute_fields.h" "ui/components/attribute_fields.cpp"
	"ui/components/slider.h" "ui/components/slider.cpp"

	"data/dataset.h"
	"data/importer.h" "data/importer.cpp"
	"data/file_parser.h"
	"data/nrrd_file_parser.h" "data/nrrd_file_parser.cpp"
	"data/csv_file_parser.h" "data/csv_file_parser.cpp"
	
	"scene/scene.h"
	"scene/camera.h" "scene/camera.cpp"
 )
target_link_libraries(${PROJECT_NAME} PRIVATE extern)
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(${PROJECT_NAME} PRIVATE RES_PATH="${CMAKE_SOURCE_DIR}/res/")

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)