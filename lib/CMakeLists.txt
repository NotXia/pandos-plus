#
# Funzioni per gestire liste e strutture di PandoOS+
#

add_library(listx listx.h)

add_library(pandos_const pandos_const.h)

add_library(pandos_types pandos_types.h)
target_link_libraries(pandos_types PUBLIC listx pandos_const)

set_target_properties(
    listx pandos_const pandos_types
    PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}"
        LINKER_LANGUAGE C
)