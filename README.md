# PandOS+

## Installazione
```
mkdir build
cd build
cmake -D CMAKE_TOOLCHAIN_FILE=../mips.cmake ..
```
Il file `mips.cmake` contiene le specifiche necessarie per la cross-compilazione.