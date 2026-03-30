#pragma once

#ifdef ZE_PLATFORM_WINDOWS
	#ifdef ZE_BUILD_DLL
		#define ZE_API __declspec(dllexport)
	#else
		#define ZE_API __declspec(dllimport)
	#endif
#else
	#error Zero engine only supports Windows.
#endif

#ifdef ZE_ENABLE_ASSERTS
	#define ZE_ASSERT(x, ...) { if(!(x)) { ZE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
	#define ZE_CORE_ASSERT(x, ...) { if(!(x)) { ZE_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define ZE_ASSERT(x, ...)
	#define ZE_CORE_ASSERT(x, ...)
#endif

#define BIT(x) (1 << x)