set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR mips)

set(CMAKE_C_COMPILER mipsel-linux-gnu-gcc)
set(CMAKE_ASM_COMPILER mipsel-linux-gnu-gcc)
set(CMAKE_C_LINKER mipsel-linux-gnu-ld)

# Forza a chiamare il linker reale anziché passare per il compilatore
SET(CMAKE_C_LINK_EXECUTABLE "mipsel-linux-gnu-ld <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")


# Sorgenti e librerie per MIPS
set(UMPS_SRC /usr/share/umps3)
set(UMPS_INC /usr/include/umps3)

# Include la cartella degli header MIPS
include_directories(${UMPS_INC})


# Flag per il compilatore
set(CFLAGS_UMPS -ffreestanding -mips1 -mabi=32 -mno-gpopt -G 0 -mno-abicalls -fno-pic -mfp32 -Wall -O0)
add_compile_options(${CFLAGS_UMPS})

# Flag per il linker
set(LINK_SCRIPT ${UMPS_SRC}/umpscore.ldscript)
set(CMAKE_EXE_LINKER_FLAGS "-G 0 -nostdlib -T ${LINK_SCRIPT}")

