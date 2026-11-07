# Executable
add_executable(Sandbox)

target_sources(Sandbox PRIVATE src/SandboxApp.cpp)

# Link sandbox with engine
target_link_libraries(Sandbox PRIVATE Engine)

target_precompile_headers(Sandbox PRIVATE
	${CMAKE_SOURCE_DIR}/Engine/src/zepch.h
)

# Includes
target_include_directories(Sandbox PRIVATE
	${CMAKE_SOURCE_DIR}/Engine/dependencies/spdlog/include
	${CMAKE_SOURCE_DIR}/Engine/src
	${CMAKE_SOURCE_DIR}/Engine/dependencies/glm
	${CMAKE_SOURCE_DIR}/Engine/dependencies/imgui
)

# Copy KTX dll to binary dir
add_custom_command(TARGET Sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Sandbox>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/Engine/dependencies/ktx_precompiled/$<CONFIG>/ktx.dll"
    "$<TARGET_FILE_DIR:Sandbox>/"
    COMMENT "Copying precompiled ktx.dll to the Sandbox runtime folder..."
)
