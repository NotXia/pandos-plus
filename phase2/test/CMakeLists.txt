#
# Test fase 2
#

add_executable(kernel p2test.0.2.c)
target_link_libraries(kernel crtso libumps initial)
set_target_properties(
    kernel
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)