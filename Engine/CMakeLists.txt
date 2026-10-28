# Static
add_library(Engine STATIC)

# Dependencies files
set(DEPEN_FILES
	dependencies/stb/stb_image.cpp

	dependencies/imgui/imgui.h
	dependencies/imgui/imgui.cpp
	dependencies/imgui/imgui_demo.cpp
	dependencies/imgui/imgui_draw.cpp
	dependencies/imgui/imgui_widgets.cpp
	dependencies/imgui/imgui_tables.cpp
	dependencies/imgui/backends/imgui_impl_vulkan.cpp
	dependencies/imgui/backends/imgui_impl_glfw.cpp
)

# All implementation files
file(GLOB_RECURSE MAIN_SOURCES CONFIGURE_DEPENDS
	"src/ZEngine/*.h"
	"src/ZEngine/*.cpp"
	"src/ZEngine/*.c"
	"src/Platform/Vulkan/*.h"
	"src/Platform/Vulkan/*.cpp"
)

# Visual studio filters
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src FILES ${MAIN_SOURCES})

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
	dependencies/fastgltf/include
	dependencies/ktx/lib/include
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

# KTX
add_subdirectory(dependencies/ktx)
target_link_libraries(Engine PRIVATE ktx)

# fastgltf
add_subdirectory(dependencies/fastgltf)
target_link_libraries(Engine PRIVATE fastgltf::fastgltf)

# 
target_compile_definitions(Engine PRIVATE VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)

# Group dependencies
set_target_properties(glfw PROPERTIES FOLDER "Dependencies")
set_target_properties(ktx PROPERTIES FOLDER "Dependencies")
set_target_properties(fastgltf PROPERTIES FOLDER "Dependencies")
