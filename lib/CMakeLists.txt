add_library(utilities utilities.c)
add_library(klog klog.c)

set_target_properties(
    utilities
    PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}"
)