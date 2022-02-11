#
# Test fase 1
#

add_executable(kernel p1test.c)
target_link_libraries(kernel crtso libumps pcb asl)
set_target_properties(
    kernel
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)