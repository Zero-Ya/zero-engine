# DLL
add_library(Engine SHARED)

set(CPP_FILES
	src/ZCore/Application.cpp
	src/ZCore/Log.cpp
)

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
)

target_sources(Engine
	PRIVATE
		${CPP_FILES}

	PUBLIC
		FILE_SET HEADERS
		BASE_DIRS src
		FILES
			${H_FILES}

)

target_precompile_headers(Engine PRIVATE
	src/zepch.h
)

target_include_directories(Engine PRIVATE
	src
	dependencies/spdlog/include
)
