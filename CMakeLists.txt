cmake_minimum_required(VERSION 3.2)
cmake_host_system_information(RESULT HOSTNAME QUERY HOSTNAME)
include(CMakePackageConfigHelpers)
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckIncludeFileCXX)
include(CheckCXXCompilerFlag)

project (dtree)
set(dtree_MAJOR_VERSION 0)
set(dtree_MINOR_VERSION 2)
set(dtree_PATCH_VERSION 0)
set(dtree_VERSION ${dtree_MAJOR_VERSION}.${dtree_MINOR_VERSION}.${dtree_PATCH_VERSION})
set(INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for executables")
set(INSTALL_INCLUDE_DIR include CACHE PATH "Installation directory for header files")
if(WIN32 AND NOT CYGWIN)
    set(DEF_INSTALL_CMAKE_DIR CMake)
else()
    set(DEF_INSTALL_CMAKE_DIR lib/cmake/dtree)
endif()

find_package(libfrugi CONFIG)

set(INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH "Installation directory for CMake files")

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Profile" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_DEBUG "-O0 -UNDEBUG -DYYDEBUG=1 ${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_RELEASE "-O4 -DNDEBUG -g")
set(CMAKE_C_FLAGS_PROFILE "${CMAKE_C_FLAGS_RELEASE} -g")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -UNDEBUG -DYYDEBUG=1 ${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_RELEASE "-O4 -DNDEBUG -g")
set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_RELEASE} -g")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY HELPSTRING "Choose the type of build")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release;Profile")

set(INCLUDE_INSTALL_DIR include)
set(LIB_INSTALL_DIR lib)
set(SYSCONFIG_INSTALL_DIR etc/dtree)

CHECK_CXX_COMPILER_FLAG("-mcx16" COMPILER_SUPPORTS_MARCH_NATIVE)
if(COMPILER_SUPPORTS_MARCH_NATIVE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcx16")
endif()

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

#add_subdirectory("treedbs" EXCLUDE_FROM_ALL)
add_subdirectory("dtree")
if(${libfrugi_FOUND})
    add_subdirectory("dtreetest" EXCLUDE_FROM_ALL)
endif()

#add_dependencies(dtreetest dtree)

export(TARGETS dtree FILE "${CMAKE_BINARY_DIR}/dtreeTargets.cmake")
export(PACKAGE dtree)

configure_package_config_file(cmake/dtreeConfig.cmake.in
        "${CMAKE_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/dtreeConfig.cmake"
        INSTALL_DESTINATION ${LIB_INSTALL_DIR}/dtree/cmake
        )

write_basic_package_version_file(
        "${CMAKE_BINARY_DIR}/cmake/dtreeConfigVersion.cmake"
        VERSION $VERSION
        COMPATIBILITY SameMajorVersion
)

install(FILES
        "${CMAKE_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/dtreeConfig.cmake"
        "${CMAKE_BINARY_DIR}/cmake/dtreeConfigVersion.cmake"
        DESTINATION "${INSTALL_CMAKE_DIR}" COMPONENT dev)

install(EXPORT dtreeTargets DESTINATION
        "${INSTALL_CMAKE_DIR}" COMPONENT dev)

message(STATUS "Host: ${HOSTNAME}")
message(STATUS "Available: ${CMAKE_CONFIGURATION_TYPES}")
message(STATUS "Building configuration: ${CMAKE_BUILD_TYPE}")
message(STATUS "    C flags: ${CMAKE_C_FLAGS}")
message(STATUS "  CXX flags: ${CMAKE_CXX_FLAGS}")

