project (libtreedbs)

add_library(libtreedbs STATIC
    src/treedbs-ll.c
    src/clt_table.c
    src/dbs-ll.c
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
