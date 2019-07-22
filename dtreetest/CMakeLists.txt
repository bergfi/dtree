add_executable(dtreetest
    dtreetest.cpp
)
set_property(TARGET dtreetest PROPERTY CXX_STANDARD 17)
set_property(TARGET dtreetest PROPERTY CXX_STANDARD_REQUIRED ON)

add_definitions(-Wall -Wextra -Wno-vla -Wno-implicit-fallthrough -Wno-unused-parameter)

set(WITH_LIBFRUGI "" CACHE FILEPATH "Path to LTSmin install directory")
if(WITH_LIBFRUGI)
    if(EXISTS "${WITH_LIBFRUGI}/include/libfrugi/Settings.h")
		include_directories(
			"${WITH_LIBFRUGI}/include"
		)
		set(HAVE_LIBFRUGI 1)
    else()
        message(SEND_ERROR "${WITH_LIBFRUGI} does not contain include/libfrugi/Settings.h")
    endif()
else()
#    message(SEND_ERROR "Please specify root source directory of LTSmin using -DWITH_LTSMIN=...")
	set(HAVE_LIBFRUGI 0)
endif()
