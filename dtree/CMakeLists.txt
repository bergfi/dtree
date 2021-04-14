add_library(dtree INTERFACE)

install(TARGETS dtree
        EXPORT dtreeTargets
        RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin
        LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT shlib
        PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}/dtree"
        COMPONENT dev
)
install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/include/dtree/dtree.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/dtree/hashset.h
        DESTINATION include/dtree
)

configure_file(dtreeConfig.h.in "${CMAKE_BINARY_DIR}/dtreeConfig.h")

target_include_directories(dtree
        INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
        )