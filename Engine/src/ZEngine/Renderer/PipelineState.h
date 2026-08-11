#pragma once

#include "Buffer.h"

namespace ZEngine {

	class LayoutManager;
	class Shader;

	struct PipelineSpecification {
		Ref<Shader> Shader;
		BufferLayout Layout;
		bool DepthTest = true;
		bool Wireframe = false;
	};

	class PipelineState {
	public:
		virtual ~PipelineState() = default;
		virtual void Bind() const = 0;

		static Ref<PipelineState> Create(const PipelineSpecification& spec, const Scope<LayoutManager>& layoutManager);
	};

}