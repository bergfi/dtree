add_executable(dtreetest
    dtreetest.cpp
)
set_property(TARGET dtreetest PROPERTY CXX_STANDARD 17)
set_property(TARGET dtreetest PROPERTY CXX_STANDARD_REQUIRED ON)

add_definitions(-Wall -Wextra -Wno-vla -Wno-implicit-fallthrough -Wno-unused-parameter)

target_link_libraries(dtreetest libfrugi)