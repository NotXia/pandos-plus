# PandOS+

## Installazione
```
mkdir build
cd build
cmake -D CMAKE_TOOLCHAIN_FILE=../mips.cmake ..
```
Il file `mips.cmake` contiene le specifiche necessarie per la cross-compilazione.

## Crediti
Il file cmake per la cross-compilazione è stato realizzato sulla base del [progetto](https://github.com/Maldus512/umps_uarm_hello_world).