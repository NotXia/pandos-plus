# add_library(initProc initProc.c)
# target_link_libraries(initProc PRIVATE sysSupport)
# target_link_libraries(initProc PRIVATE vmSupport)

add_library(sysSupport sysSupport.c)
target_link_libraries(sysSupport PRIVATE initial)
target_link_libraries(sysSupport PRIVATE utilities)
target_link_libraries(sysSupport PRIVATE vmSupport)

add_library(vmSupport vmSupport.c)
target_link_libraries(vmSupport PRIVATE initial)
target_link_libraries(vmSupport PRIVATE utilities)
target_link_libraries(vmSupport PRIVATE sysSupport)


set_target_properties(
    sysSupport vmSupport
    PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}"
)