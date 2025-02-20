cmake_minimum_required(VERSION 3.13)
project(nCine)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
get_filename_component(PARENT_SOURCE_DIR ${CMAKE_SOURCE_DIR} DIRECTORY)

string(FIND ${CMAKE_SYSTEM_PROCESSOR} "arm" ARM_SUBSTRING_FOUND)
string(FIND ${CMAKE_SYSTEM_PROCESSOR} "aarch64" AARCH64_SUBSTRING_FOUND)
if (ARM_SUBSTRING_FOUND GREATER -1 OR AARCH64_SUBSTRING_FOUND GREATER -1)
	set(NCINE_ARM_PROCESSOR TRUE)
endif()

set(NCINE_ROOT ${CMAKE_SOURCE_DIR})
set(NCINE_SOURCE_DIR "${NCINE_ROOT}/Jazz2/")
set(NCINE_REVERSE_DNS "jazz2.resurrection")
set(NCINE_APP_NAME "Jazz² Resurrection")
set(NCINE_APP_DESCRIPTION "Open-source reimplementation of Jazz Jackrabbit 2")
set(NCINE_APP_VENDOR "Dan R.")
set(NCINE_VERSION "1.0.0")

include(ncine_options)

if(NOT IS_DIRECTORY ${NCINE_DATA_DIR})
	message(WARNING "nCine data directory not found at: ${NCINE_DATA_DIR}")
else()
	message(STATUS "nCine data directory: ${NCINE_DATA_DIR}")
endif()

include(ncine_get_version)
include(ncine_imported_targets)
include(ncine_tracy)

if(NCINE_BUILD_ANDROID)
	if(NCINE_DYNAMIC_LIBRARY)
		add_library(ncine SHARED)
	else()
		add_library(ncine STATIC)
		target_compile_definitions(ncine PUBLIC "NCINE_STATIC")
	endif()
elseif(WINDOWS_PHONE OR WINDOWS_STORE)
	message(STATUS "Compiling for Windows RT")
	add_executable(ncine)
else()
	# Falling back to either GLFW or SDL2 if the other one is not available
	if(NOT GLFW_FOUND AND NOT SDL2_FOUND AND NOT Qt5_FOUND)
		message(FATAL_ERROR "No backend between SDL2, GLFW, and QT5 has been found")
	elseif(GLFW_FOUND AND NCINE_PREFERRED_BACKEND STREQUAL "GLFW")
		message(STATUS "Using GLFW as the preferred backend")
	elseif(SDL2_FOUND AND NCINE_PREFERRED_BACKEND STREQUAL "SDL2")
		message(STATUS "Using SDL2 as the preferred backend")
	elseif(Qt5_FOUND AND NCINE_PREFERRED_BACKEND STREQUAL "QT5")
		message(STATUS "Using QT5 as the preferred backend")
	elseif(SDL2_FOUND AND NOT GLFW_FOUND AND NCINE_PREFERRED_BACKEND STREQUAL "GLFW")
		set(NCINE_PREFERRED_BACKEND "SDL2")
		message(WARNING "Using SDL2 as backend because GLFW cannot be found")
	elseif(GLFW_FOUND AND NOT SDL2_FOUND AND NCINE_PREFERRED_BACKEND STREQUAL "SDL2")
		set(NCINE_PREFERRED_BACKEND "GLFW")
		message(WARNING "Using GLFW as backend because SDL2 cannot be found")
	endif()

	add_executable(ncine)
endif()

include(ncine_compiler_options)
include(ncine_headers)
include(ncine_sources)
include(ncine_extra_sources)
include(ncine_generated_sources)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

foreach(FILE ${HEADERS} ${SOURCES}) 
	get_filename_component(PARENT_DIR "${FILE}" DIRECTORY)
	string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/Jazz2" "" GROUP "${PARENT_DIR}")
	string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" GROUP "${GROUP}")
	string(REPLACE "/" "\\" GROUP "${GROUP}")

	# Group into "Source Files" and "Header Files"
	if ("${FILE}" MATCHES ".*\\.cpp")
		set(GROUP "Source Files${GROUP}")
	elseif("${FILE}" MATCHES ".*\\.h")
		set(GROUP "Header Files${GROUP}")
	endif()
	source_group("${GROUP}" FILES "${FILE}")
endforeach()

foreach(SOURCE_FILE IN LISTS SHADER_FILES)
	source_group("Shaders" FILES ${SOURCE_FILE})
endforeach()
foreach(SOURCE_FILE ${GENERATED_SOURCES})
	source_group("Generated" FILES ${SOURCE_FILE})
endforeach()

if(OPENGL_FOUND)
	target_link_libraries(ncine PRIVATE OpenGL::GL)
endif()
target_sources(ncine PRIVATE ${SOURCES} ${HEADERS} ${SHADER_FILES} ${GENERATED_SOURCES})

if(NCINE_WITH_TRACY)
	target_include_directories(ncine
		PUBLIC $<BUILD_INTERFACE:${TRACY_INCLUDE_ONLY_DIR}/tracy>
		PUBLIC $<INSTALL_INTERFACE:include/tracy>)
endif()
target_include_directories(ncine PRIVATE ${NCINE_ROOT}/Shared)
if(NOT NCINE_DYNAMIC_LIBRARY)
	target_include_directories(ncine
		INTERFACE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/ncine>)
endif()
if(IS_DIRECTORY ${GENERATED_INCLUDE_DIR})
	get_filename_component(PARENT_GENERATED_INCLUDE_DIR ${GENERATED_INCLUDE_DIR} DIRECTORY)
	target_include_directories(ncine
		INTERFACE $<BUILD_INTERFACE:${PARENT_GENERATED_INCLUDE_DIR}>
		# Internal sources can access a generated header with or without the parent directory
		PRIVATE $<BUILD_INTERFACE:${PARENT_GENERATED_INCLUDE_DIR}>
		PRIVATE $<BUILD_INTERFACE:${GENERATED_INCLUDE_DIR}>)
endif()

include(ncine_installation)
include(ncine_build_android)
include(ncine_strip_binaries)
