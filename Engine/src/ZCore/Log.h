#pragma once

#include "Core.h"
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <memory>

namespace ZEngine {

	class ZE_API Log
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;

	};

}

// Core log macros
#define ZE_CORE_TRACE(...) ::ZEngine::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define ZE_CORE_INFO(...)  ::ZEngine::Log::GetCoreLogger()->info(__VA_ARGS__)
#define ZE_CORE_WARN(...)  ::ZEngine::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define ZE_CORE_ERROR(...) ::ZEngine::Log::GetCoreLogger()->error(__VA_ARGS__)
#define ZE_CORE_FATAL(...) ::ZEngine::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define ZE_TRACE(...)	   ::ZEngine::Log::GetClientLogger()->trace(__VA_ARGS__)
#define ZE_INFO(...)	   ::ZEngine::Log::GetClientLogger()->info(__VA_ARGS__)
#define ZE_WARN(...)	   ::ZEngine::Log::GetClientLogger()->warn(__VA_ARGS__)
#define ZE_ERROR(...)	   ::ZEngine::Log::GetClientLogger()->error(__VA_ARGS__)
#define ZE_FATAL(...)	   ::ZEngine::Log::GetClientLogger()->critical(__VA_ARGS__)