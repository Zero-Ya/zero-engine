# DLL
add_library(Engine SHARED)

# All implementation files
set(CPP_FILES
	src/ZCore/Application.cpp
	src/ZCore/Log.cpp

	src/Platform/Windows/WindowsWindow.cpp

	src/ZCore/Layer.cpp
	src/ZCore/LayerStack.cpp
)

# All header files
set(H_FILES
	src/ZCore.h
	src/ZCore/Application.h
	src/ZCore/Core.h
	src/ZCore/EntryPoint.h
	src/ZCore/Log.h

	src/ZCore/Events/ApplicationEvent.h
	src/ZCore/Events/Event.h
	src/ZCore/Events/KeyEvent.h
	src/ZCore/Events/MouseEvent.h

	src/ZCore/Window.h
	src/Platform/Windows/WindowsWindow.h

	src/ZCore/Layer.h
	src/ZCore/LayerStack.h

	src/ZCore/ImGui/ImGuiLayer.h
	src/ZCore/ImGui/ImGuiLayer.cpp
)

target_sources(Engine
	PRIVATE
		${CPP_FILES}

	PUBLIC
		FILE_SET HEADERS
		BASE_DIRS src
		FILES
			${H_FILES}
			src/ZCore/ImGui/ImGuiVulkanUtil.h
			src/ZCore/ImGui/ImGuiVulkanUtil.cpp
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