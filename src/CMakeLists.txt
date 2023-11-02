project(volumetric-renderer)

message(STATUS "Building executable...")

if(WIN32)
	set(OPTIONS WIN32)
endif()

add_executable(${PROJECT_NAME} ${OPTIONS}
	"main.cpp"
	"vulkan_context.h" "vulkan_context.cpp"
)
target_link_libraries(${PROJECT_NAME} PRIVATE extern)
target_compile_definitions(${PROJECT_NAME} PRIVATE RES_PATH="${CMAKE_SOURCE_DIR}/res/")

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)