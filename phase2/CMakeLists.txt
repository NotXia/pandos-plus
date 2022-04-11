add_library(initial initial.c)
target_link_libraries(initial PUBLIC pcb)
target_link_libraries(initial PRIVATE asl)
target_link_libraries(initial PRIVATE exceptions)
target_link_libraries(initial PRIVATE interrupts)
target_link_libraries(initial PRIVATE scheduler)

add_library(scheduler scheduler.c)
target_link_libraries(scheduler PRIVATE initial)
target_link_libraries(scheduler PUBLIC pcb)

add_library(exceptions exceptions.c)
target_link_libraries(exceptions PRIVATE initial)
target_link_libraries(exceptions PRIVATE asl)
target_link_libraries(exceptions PRIVATE scheduler)
target_link_libraries(exceptions PRIVATE interrupts)

add_library(interrupts interrupts.c)
target_link_libraries(interrupts PRIVATE scheduler)
target_link_libraries(interrupts PRIVATE initial)
target_link_libraries(interrupts PRIVATE asl)


set_target_properties(
    initial scheduler exceptions interrupts
    PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}"
)