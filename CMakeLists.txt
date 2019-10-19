#==============================================================================
#	Top-Level CoreZero.SDK CMakeLists
#
#	Jensen Miller
#	Copyright (c) 2019 LooUQ Incorporated.
#	Licensed under the GNU license.
#==============================================================================
cmake_minimum_required(VERSION 3.8)

set(PROJECT_NAME "Windows-Device-Serial-Cpp")
set(PROJECT_LIB_NAME "libwinserial")



#
#	Project Name Assignment
#
project ("${PROJECT_NAME}")





#
#	Configure Directories
#
set	(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")


#	filters
set (EXTERNAL_DEPENDENCIES_FILTER "extern")
set (UNIT_TESTS_FILTER "tests")





#
#	Build Options
#
option(BUILD_LIBRARY_AS_SHARED "Build this library as a shared object rather than the default static library." OFF)
option(BUILD_EXAMPLES "Build examples of this library." OFF)
option(BUILD_TESTS "Build the tests of this library" OFF)

if (WIN32)
	add_definitions(-DWIN32)
	add_definitions(-DUNICODE)
endif()


if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(mcu CMAKE_HOST_SYSTEM_PROCESSOR)
endif()

find_library(CoreZero-SDK
	NAMES corezero.lib
	HINTS "${PROJECT_SOURCE_DIR}/../../LooUQ/CoreZero-SDK"
)

include_directories("${PROJECT_SOURCE_DIR}/../../LooUQ/CoreZero-SDK/include")



#
#	Include all sub-projects
#

#	Examples
if (BUILD_EXAMPLES)
	add_subdirectory("${PROJECT_SOURCE_DIR}/examples")
endif()

#	Tests
if (BUILD_TESTS)
	enable_testing()
	add_subdirectory("${PROJECT_SOURCE_DIR}/tests")
endif()



#
#	Discover all library code
#
file(GLOB_RECURSE LIB_SOURCES FOLLOW_SYMLINKS "${PROJECT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE LIB_INCLUDES FOLLOW_SYMLINKS "${PROJECT_SOURCE_DIR}/include/*.hpp" "${PROJECT_SOURCE_DIR}/include/*.h")



#
#	Setup Library by root
#
if (MSVC)
	set (CMAKE_CXX_FLAGS "/IGNORE:4996")
endif()


#	Build library as shared or static
if (BUILD_LIBRARY_AS_SHARED)
	add_library("${PROJECT_LIB_NAME}" SHARED "")
	set_target_properties("${PROJECT_LIB_NAME}" PROPERTIES SUFFIX ".so")
else()
	add_library("${PROJECT_LIB_NAME}" STATIC "")
	set_target_properties("${PROJECT_LIB_NAME}" PROPERTIES
		POSITION_INDEPENDENT_CODE ON
		LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
endif()

#	Set include directories
target_include_directories("${PROJECT_LIB_NAME}" PRIVATE "${PROJECT_SOURCE_DIR}/include")

target_sources("${PROJECT_LIB_NAME}" PUBLIC "${LIB_INCLUDES}")	# include library headers
target_sources("${PROJECT_LIB_NAME}" PRIVATE "${LIB_SOURCES}")	# include library source code