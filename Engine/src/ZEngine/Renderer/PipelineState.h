#pragma once

#include "Buffer.h"

namespace ZEngine {

	class LayoutManager;
	class Shader;

	struct PipelineSpecification {
		std::shared_ptr<Shader> Shader;
		BufferLayout Layout;
		bool DepthTest = true;
		bool Wireframe = false;
	};

	class PipelineState {
	public:
		virtual ~PipelineState() = default;
		virtual void Bind() const = 0;

		static std::shared_ptr<PipelineState> Create(const PipelineSpecification& spec, const std::unique_ptr<LayoutManager>& layoutManager);
	};

}