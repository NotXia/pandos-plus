#
# Test fase 2
#

add_executable(kernel p2test.c)
target_link_libraries(kernel crtso libumps initial)

# Indica di mettere il file generato nella radice della cartella in cui si esegue make
set_target_properties(
    kernel
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)