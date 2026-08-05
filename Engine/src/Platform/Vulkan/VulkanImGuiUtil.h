#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <imgui.h>
#include <GLFW/glfw3.h>

namespace ZEngine {
	class RenderCommandBuffer;

	class VulkanImGuiUtil {
	public:
		VulkanImGuiUtil() = default;
		~VulkanImGuiUtil() = default;

		// Core functions
		void Init(GLFWwindow* window);
		void SetStyle();
		void Shutdown();

		// Frame rendering operations
		void BeginFrame();
		void EndFrame(const std::shared_ptr<RenderCommandBuffer>& renderCommandBuffer);

	private:
		void CreateDescriptorPool();

	private:
		vk::raii::DescriptorPool m_DescriptorPool = nullptr;
	};

}