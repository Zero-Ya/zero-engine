# Set CMake minimum required version
cmake_minimum_required(VERSION 3.23)

project(zero-engine)

# CMakePresets.json later...
#set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE PATH "vcpkg toolchain file")
#set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "VCPKG Target Triplet")
# Might not do this since I'm not using vcpkg anymore or yet...

# Standards
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Subdirectories (sub-projects)
add_subdirectory(Engine)
add_subdirectory(Sandbox)

# Set startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Sandbox)

# Preprocessor definitions
target_compile_definitions(Engine PRIVATE
	ZE_PLATFORM_WINDOWS
	ZE_BUILD_DLL

	$<$<CONFIG:Debug>:ZE_DEBUG> 
    $<$<CONFIG:Release>:ZE_RELEASE>
	# $<$<CONFIG:Dist>:ZE_DIST> 

	PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
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

# Shader spirv compilation
function (add_slang_shader_target TARGET)
  cmake_parse_arguments ("SHADER" "" "" "SOURCES" ${ARGN})
  set (SHADERS_DIR ${CMAKE_CURRENT_LIST_DIR}/Sandbox/assets/shaders)
  set (ENTRY_POINTS -entry vertMain -entry fragMain)
  set (SLANGC_EXECUTABLE C:/VulkanSDK/1.4.335.0/bin/slangc.exe)
  #add_custom_command (
  #        OUTPUT ${SHADERS_DIR}
  #        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR}
  #)
  add_custom_command (
          OUTPUT  ${SHADERS_DIR}/slang.spv
          COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCES} -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name ${ENTRY_POINTS} -o slang.spv
          WORKING_DIRECTORY ${SHADERS_DIR}
          DEPENDS ${SHADERS_DIR} ${SHADER_SOURCES}
          COMMENT "Compiling Slang Shaders"
          VERBATIM
  )
  add_custom_target (${TARGET} DEPENDS ${SHADERS_DIR}/slang.spv)
endfunction()

add_slang_shader_target(shader_spirv_compile SOURCES ${CMAKE_CURRENT_LIST_DIR}/Sandbox/assets/shaders/shader.slang)
add_dependencies(Engine shader_spirv_compile)
add_dependencies(Sandbox shader_spirv_compile)