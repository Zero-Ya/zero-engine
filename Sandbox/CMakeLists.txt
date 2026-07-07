# Executable
add_executable(Sandbox)

#function (copy_dlls target)
#	add_custom_command (
#	    TARGET "${target}" POST_BUILD
#	    COMMAND "${CMAKE_COMMAND}" -E copy -t "$<TARGET_FILE_DIR:${target}>"
#	            "$<TARGET_RUNTIME_DLLS:${target}>" USES_TERMINAL COMMAND_EXPAND_LISTS
#	)
#endfunction ()

target_sources(Sandbox PRIVATE
	src/SandboxApp.cpp
)

# Link sandbox with engine
target_link_libraries(Sandbox PRIVATE Engine)

target_precompile_headers(Sandbox PRIVATE
	${CMAKE_SOURCE_DIR}/Engine/src/zepch.h
)

# Includes
target_include_directories(Sandbox PRIVATE
	${CMAKE_SOURCE_DIR}/Engine/dependencies/spdlog/include
	${CMAKE_SOURCE_DIR}/Engine/src
)

# Copy dll to binary dir
#copy_dlls(Sandbox)
