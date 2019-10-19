#==============================================================================
#	CTest Project
#		: Windows-Device-Serial-Cpp--tests
#
#	Jensen Miller
#	Copyright (C) 2019 LooUQ Incorporated.
#	Licensed under the GNU license.
#==============================================================================
cmake_minimum_required(VERSION 3.11)

set (UNIT_TEST_TARGET_NAME "${PROJECT_NAME}--tests")





#
#	Build Options
#

#	"google-gtest"
#	||	"microsoft-unittestingframework"
#	||	"boost-test"
set	(TEST_FRAMEWORK "google-gtest")

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
	message(FATAL_ERROR "Cannot build unit tests in source!")
endif()





#
#	Compiler Options
#

add_definitions(-DUNIT_TESTS)

#	-std=c++11
set (CMAKE_CXX_STANDARD 11)
set	(CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_STANDARD_REQUIRED OFF)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

if (WIN32)
	set(CUSTOM_COMPILER_FLAGS "/W4")
endif()





#
#	Configure testing
#


#	determine test framework
if ("${TEST_FRAMEWORK}" STREQUAL "microsoft-unittestingframework" AND MSVC)
	#
	#	Find Microsoft UnitTesting Framework
	#
	set (VS_INSTALL_DIR "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community" CACHE STRING "Visual Studio install location")
	set (TARGET_TEST_FRAMEWORK_DIR "${VS_INSTALL_DIR}\\VC\\Auxiliary\\VS\\UnitTest")	
elseif("${TEST_FRAMEWORK}" STREQUAL "google-gtest")
	#
	#	Git GoogleTest
	#		https://google.github.io/styleguide/cppguide.html#Classes

	include (FetchContent)
	#	git google/googletest
	FetchContent_Declare(
		googletest
		GIT_REPOSITORY https://github.com/google/googletest.git
		GIT_TAG v1.10.x
	)

	FetchContent_GetProperties(googletest)
	#	prevent google test compiler/linker option overriding
	set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
	if(NOT googletest_POPULATED)
		FetchContent_Populate(googletest)
		add_subdirectory("${googletest_SOURCE_DIR}" "${googletest_BINARY_DIR}")
	endif()	
	#	filter google test targets
	set_target_properties(gtest PROPERTIES FOLDER ${EXTERNAL_DEPENDENCIES_FILTER}/google)
	set_target_properties(gtest_main PROPERTIES FOLDER ${EXTERNAL_DEPENDENCIES_FILTER}/google)
	set_target_properties(gmock PROPERTIES FOLDER ${EXTERNAL_DEPENDENCIES_FILTER}/google)
	set_target_properties(gmock_main PROPERTIES FOLDER ${EXTERNAL_DEPENDENCIES_FILTER}/google)
elseif("${TEST_FRAMEWORK}" STREQUAL "boost-test")
	#
	#	Boost Test
	#
	include (FetchContent)
	#	get boostorg/test
	FetchContent_Declare(
		boosttest
		GIT_REPOSITORY https://github.com/boostorg/test.git
		GIT_TAG master
	)

	FetchContent_GetProperties(boosttest)
	#	prevent boost test compiler/linker option overriding
	set (boosttest_force_shared_crt ON CACHE BOOL "" FORCE)
	if (NOT boosttest_POPULATED)
		FetchContent_Populate(boosttest)
		add_subdirectory("${boosttest_SOURCE_DIR}" "${boosttest_BINARY_DIR}")
	endif()
	#	filter boost test targets
endif()

#	define method for adding test method
macro(add_unit_test UNIT_TEST_NAME)	
	add_executable("${UNIT_TEST_NAME}" "${ARGN}")
	add_test(
		NAME "${UNIT_TEST_NAME}"
		COMMAND "${UNIT_TEST_NAME}"
	)
	target_include_directories("${UNIT_TEST_NAME}" PRIVATE "${CMAKE_SOURCE_DIR}/include")
	target_link_libraries("${UNIT_TEST_NAME}" "${PROJECT_LIB_NAME}")
	target_link_libraries("${UNIT_TEST_NAME}" gtest gmock gtest_main)
	set_target_properties("${UNIT_TEST_NAME}" PROPERTIES FOLDER ${UNIT_TESTS_FILTER})
endmacro()





#
#	Add unit tests
#
add_unit_test("SerialDevice-tests" "src/SerialDeviceTests.cpp")
target_include_directories("SerialDevice-tests" PRIVATE "{CMAKE_SOURCE_DIR}/../../LooUQ/CoreZero-SDk/include")