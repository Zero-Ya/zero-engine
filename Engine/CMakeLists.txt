# DLL
add_library(Engine SHARED)

# Second option...
target_sources(Engine
	PRIVATE
		src/ZCore/Application.cpp
		src/ZCore/Log.cpp

	PUBLIC
		FILE_SET HEADERS
		BASE_DIRS src
		FILES
			src/ZCore.h
			src/ZCore/Application.h
			src/ZCore/Core.h
			src/ZCore/EntryPoint.h
			src/ZCore/Log.h
			src/ZCore/Events/ApplicationEvent.h
			src/ZCore/Events/Event.h
			src/ZCore/Events/KeyEvent.h
			src/ZCore/Events/MouseEvent.h
)

target_include_directories(Engine PRIVATE
	src
	dependencies/spdlog/include
)
