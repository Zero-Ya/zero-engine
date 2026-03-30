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
add_subdirectory(Game)

# Set startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Game)

# Preprocessor definitions
target_compile_definitions(Engine PRIVATE
	ZE_PLATFORM_WINDOWS
	ZE_BUILD_DLL

	$<$<CONFIG:Debug>:ZE_DEBUG> 
    $<$<CONFIG:Release>:ZE_RELEASE>
	# $<$<CONFIG:Dist>:ZE_DIST> 
)

target_compile_definitions(Game PRIVATE
	ZE_PLATFORM_WINDOWS

	$<$<CONFIG:Debug>:ZE_DEBUG> 
    $<$<CONFIG:Release>:ZE_RELEASE>
	# $<$<CONFIG:Dist>:ZE_DIST> 
)

# Set compile stuff
if(MSVC)
	target_compile_options(Engine PRIVATE
		/utf-8
		#/MDd
	)
	target_compile_options(Game PRIVATE
		/utf-8
	)
endif()
