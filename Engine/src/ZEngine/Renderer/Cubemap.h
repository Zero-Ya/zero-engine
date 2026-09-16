#pragma once

namespace ZEngine {

	class PipelineSpecification;

	class Cubemap {
	public:
		virtual ~Cubemap() = default;

		virtual void Init(const PipelineSpecification& spec) = 0;

		virtual void LoadCubemap(std::vector<std::string> faces) = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		static Scope<Cubemap> Create();
	};

}