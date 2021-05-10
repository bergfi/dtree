include(FetchContent)

FetchContent_Declare(
    libfrugi
    GIT_REPOSITORY https://github.com/bergfi/libfrugi.git
    GIT_TAG        6dce992af08f56b8d20f4612ec86dbb45300e631
)
FetchContent_MakeAvailable(libfrugi)

add_executable(dtreetest
    dtreetest.cpp
)
set_property(TARGET dtreetest PROPERTY CXX_STANDARD 17)
set_property(TARGET dtreetest PROPERTY CXX_STANDARD_REQUIRED ON)

add_definitions(-Wall -Wextra -Wno-vla -Wno-implicit-fallthrough -Wno-unused-parameter)

target_link_libraries(dtreetest
    PUBLIC libfrugi dtree
    )
target_include_directories(dtreetest
    PUBLIC include
    )