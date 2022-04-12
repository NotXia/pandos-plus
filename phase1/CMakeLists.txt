#
# Strutture dati per PCB e semafori
#

add_library(pcb pcb.c)

add_library(asl asl.c)
target_link_libraries(asl PRIVATE pcb)
target_link_libraries(asl PRIVATE scheduler)

set_target_properties(
    pcb asl
    PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}"
)