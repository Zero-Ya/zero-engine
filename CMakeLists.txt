# Set CMake minimum required version
cmake_minimum_required(VERSION 3.23)

project(zero-engine)

# Standards
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Subdirectories (sub-projects)
add_subdirectory(Engine)
add_subdirectory(Sandbox)

# Set startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Sandbox)

# Preprocessor definitions
set(ASSETS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Sandbox/assets)
target_compile_definitions(Engine PRIVATE
	ZE_PLATFORM_WINDOWS
	ZE_BUILD_DLL

	$<$<CONFIG:Debug>:ZE_DEBUG> 
    $<$<CONFIG:Release>:ZE_RELEASE>
	# $<$<CONFIG:Dist>:ZE_DIST> 

	PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
	ASSETS_DIR="${ASSETS_DIR}"
)

target_compile_definitions(Sandbox PRIVATE
	ZE_PLATFORM_WINDOWS

	$<$<CONFIG:Debug>:ZE_DEBUG> 
    $<$<CONFIG:Release>:ZE_RELEASE>
	# $<$<CONFIG:Dist>:ZE_DIST> 
)

# Set compile stuff
if(MSVC)
	target_compile_options(Engine PRIVATE
		/utf-8
		$<$<CONFIG:Debug>:/MDd>
	)
	target_compile_options(Sandbox PRIVATE
		/utf-8
	)
endif()

set(SHADER_SOURCES
	${ASSETS_DIR}/shaders/shader.slang
	${ASSETS_DIR}/shaders/imgui.slang
)
set(SPIRV_FILES "")
set(ENTRY_POINTS -entry vertMain -entry fragMain)
set(SLANGC_EXECUTABLE C:/VulkanSDK/1.4.335.0/bin/slangc.exe)

# Shader spirv compilation
foreach(SHADER_FILE ${SHADER_SOURCES})
	get_filename_component(FILE_NAME ${SHADER_FILE} NAME_WE)
	set(SPIRV_OUTPUT "${ASSETS_DIR}/shaders/${FILE_NAME}.spv")

	# compile command
	add_custom_command(
		OUTPUT ${SPIRV_OUTPUT}
		COMMAND ${SLANGC_EXECUTABLE} ${SHADER_FILE} -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name ${ENTRY_POINTS} -o ${SPIRV_OUTPUT}
		DEPENDS ${SHADER_FILE}
	)
	list(APPEND SPIRV_FILES ${SPIRV_OUTPUT})
endforeach()

add_custom_target(shader_spirv_compile ALL DEPENDS ${SPIRV_FILES})

# Make sure the shaders compile each time they change
add_dependencies(Engine shader_spirv_compile)
add_dependencies(Sandbox shader_spirv_compile)
