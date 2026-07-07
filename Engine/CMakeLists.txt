# Static
add_library(Engine STATIC)

# Dependencies files
set(DEPEN_FILES
	dependencies/stb/stb_image.cpp

	dependencies/imgui/imgui.cpp
	dependencies/imgui/imgui_draw.cpp
	dependencies/imgui/imgui_widgets.cpp
	dependencies/imgui/imgui_tables.cpp
	dependencies/imgui/imgui_demo.cpp
)

# All implementation files
file(GLOB_RECURSE MAIN_SOURCES CONFIGURE_DEPENDS
	"src/ZEngine/*.h"
	"src/ZEngine/*.cpp"
	"src/ZEngine/*.c"
	"src/Platform/Vulkan/*.h"
	"src/Platform/Vulkan/*.cpp"
)

# Only compile if the OS is Windows
if(WIN32)
    file(GLOB_RECURSE WINDOWS_SOURCES
        "src/Platform/Windows/*.h" 
        "src/Platform/Windows/*.cpp"
    )
    list(APPEND MAIN_SOURCES ${WINDOWS_SOURCES})
endif()

target_sources(Engine PRIVATE ${MAIN_SOURCES} ${DEPEN_FILES} "src/ZEngine.h" "src/zepch.h")

# Precompiled headers
target_precompile_headers(Engine PRIVATE
	src/zepch.h
)

# Includes
target_include_directories(Engine PRIVATE
	src
	dependencies/spdlog/include
	dependencies/GLFW/include
	dependencies/imgui
	dependencies/vulkan/Include
	dependencies/glm
	dependencies/stb
)

# GLFW
add_subdirectory(dependencies/GLFW)

# Link static libs
target_link_libraries(Engine PRIVATE
	glfw
)

# Vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(Engine PRIVATE Vulkan::Vulkan)