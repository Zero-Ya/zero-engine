# Executable
add_executable(Game)

function (copy_dlls target)
	add_custom_command (
	    TARGET "${target}" POST_BUILD
	    COMMAND "${CMAKE_COMMAND}" -E copy -t "$<TARGET_FILE_DIR:${target}>"
	            "$<TARGET_RUNTIME_DLLS:${target}>" USES_TERMINAL COMMAND_EXPAND_LISTS
	)
endfunction ()

target_sources(Game PRIVATE
	src/GameApp.cpp
)

target_link_libraries(Game PRIVATE Engine)

target_include_directories(Game PRIVATE
	${CMAKE_SOURCE_DIR}/Engine/dependencies/spdlog/include
	${CMAKE_SOURCE_DIR}/Engine/src
)

# Copy dll to binary dir
copy_dlls(Game)
