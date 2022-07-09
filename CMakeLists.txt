#
# Realizzato a partire da: https://github.com/Maldus512/umps_uarm_hello_world
# cmake -D CMAKE_TOOLCHAIN_FILE=../mips.cmake ..
#
cmake_minimum_required(VERSION 3.3)
project(
	PandOS+
	VERSION 2.0
	LANGUAGES C ASM
)

# Implementa correttamente lo __start() (punto di entrata del programma)
add_library(crtso STATIC ${UMPS_SRC}/crtso.S)
# Libreria di supporto con istruzioni assembly richiamabili dal C
add_library(libumps STATIC ${UMPS_SRC}/libumps.S)

include_directories("lib")
add_subdirectory("lib")
add_subdirectory("phase1")
add_subdirectory("phase2")
add_subdirectory("phase3")

# add_subdirectory("phase1/test")
# add_subdirectory("phase2/test")
add_subdirectory("phase3/test")

# Esegue umps3-elf2umps
# Converte i file oggetto ELF prodotti dalla cross-compilazione in file oggetto riconosciuti da umps3
add_custom_target(
	kernel.core.umps ALL
	COMMAND umps3-elf2umps -k ${PROJECT_BINARY_DIR}/kernel
	BYPRODUCTS kernel.core.umps kernel.stab.umps
	DEPENDS kernel
)