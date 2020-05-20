cmake_minimum_required(VERSION 2.8)
cmake_host_system_information(RESULT HOSTNAME QUERY HOSTNAME)

include(CheckIncludeFile)

project (dtree)

include(CheckIncludeFiles)
#include(CheckIncludeFileCXX)
include(CheckCXXCompilerFlag)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Profile" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_DEBUG "-O0 -UNDEBUG -DYYDEBUG=1 ${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_RELEASE "-O4 -DNDEBUG -g")
set(CMAKE_C_FLAGS_PROFILE "${CMAKE_C_FLAGS_RELEASE} -g")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -UNDEBUG -DYYDEBUG=1 ${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_RELEASE "-O4 -DNDEBUG -g")
set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_RELEASE} -g")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY HELPSTRING "Choose the type of build")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release;Profile")

CHECK_CXX_COMPILER_FLAG("-mcx16" COMPILER_SUPPORTS_MARCH_NATIVE)
if(COMPILER_SUPPORTS_MARCH_NATIVE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcx16")
endif()

include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/dtree/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/dtreetest/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/treedbs/include"
)

set(WITH_LIBFRUGI "" CACHE FILEPATH "Path to libfrugi library install directory")
if(WITH_LIBFRUGI)
    if(EXISTS "${WITH_LIBFRUGI}/include/libfrugi/Settings.h")
        add_subdirectory("${WITH_LIBFRUGI}" "external/libfrugi")
        set(HAVE_LIBFRUGI 1)
    else()
        message(SEND_ERROR "${WITH_LIBFRUGI} does not contain include/libfrugi/Settings.h")
    endif()
else()
    set(HAVE_LIBFRUGI 0)
endif()

add_subdirectory("treedbs" EXCLUDE_FROM_ALL)
add_subdirectory("dtree")
add_subdirectory("dtreetest" EXCLUDE_FROM_ALL)

add_dependencies(dtreetest dtree)

message(STATUS "Host: ${HOSTNAME}")
message(STATUS "Available: ${CMAKE_CONFIGURATION_TYPES}")
message(STATUS "Building configuration: ${CMAKE_BUILD_TYPE}")
message(STATUS "    C flags: ${CMAKE_C_FLAGS}")
message(STATUS "  CXX flags: ${CMAKE_CXX_FLAGS}")

