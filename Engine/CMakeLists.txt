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

# Only compile if the OS is Windows
if(WIN32)
    file(GLOB_RECURSE WINDOWS_SOURCES
        "src/Platform/Windows/*.h" 
        "src/Platform/Windows/*.cpp"
    )
    list(APPEND MAIN_SOURCES ${WINDOWS_SOURCES})
endif()

# Visual studio filters
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src FILES ${MAIN_SOURCES})

target_sources(Engine PRIVATE 
	${MAIN_SOURCES}
	${DEPEN_FILES}

	"src/ZEngine.h"
	"src/zepch.h"
)

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
	#dependencies/ktx/lib/include
	dependencies/ktx_precompiled/include
)

# GLFW
# Disable internal extra targets for GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "")
set(GLFW_INSTALL OFF CACHE INTERNAL "")

add_subdirectory(dependencies/GLFW EXCLUDE_FROM_ALL)
target_link_libraries(Engine PRIVATE glfw)

# Vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(Engine PRIVATE Vulkan::Vulkan)

# KTX
# Disable internal extra targets for KTX
set(KTX_FEATURE_TESTS OFF CACHE INTERNAL "")
set(KTX_FEATURE_DOCS OFF CACHE INTERNAL "")
set(KTX_FEATURE_APPS OFF CACHE INTERNAL "")

#add_subdirectory(dependencies/ktx EXCLUDE_FROM_ALL)
#target_link_libraries(Engine PRIVATE ktx)
target_link_libraries(Engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/ktx_precompiled/$<CONFIG>/ktx.lib)

# fastgltf
# Disable internal extra targets for fastgltf
set(FASTGLTF_ENABLE_TESTS OFF CACHE INTERNAL "")
set(FASTGLTF_ENABLE_EXAMPLES OFF CACHE INTERNAL "")

add_subdirectory(dependencies/fastgltf EXCLUDE_FROM_ALL)
target_link_libraries(Engine PRIVATE fastgltf::fastgltf)

# 
target_compile_definitions(Engine PRIVATE VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)

# Group dependencies
set_target_properties(glfw PROPERTIES FOLDER "Dependencies" VS_GLOBAL_DebuggerFlavor "None")
#set_target_properties(ktx PROPERTIES FOLDER "Dependencies" VS_GLOBAL_DebuggerFlavor "None")
if (TARGET fastgltf)
	set_target_properties(fastgltf PROPERTIES FOLDER "Dependencies" VS_GLOBAL_DebuggerFlavor "None")
endif()
