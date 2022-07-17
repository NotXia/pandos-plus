#
# Test fase 3
#

add_executable(kernel ../initProc.c)
target_link_libraries(kernel crtso libumps initial sysSupport vmSupport)
set_target_properties(
    kernel
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)