#pragma once

namespace ZEngine {

	class Shader {
	public:
		virtual ~Shader() = default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual const std::string& GetName() const = 0;

		static Ref<Shader> Create(const std::string& name,
											  const std::string& spirvFilePath,
											  const std::string& vertEntryPoint = "vertMain",
											  const std::string& fragEntryPoint = "fragMain");
	};

}