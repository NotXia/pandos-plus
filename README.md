# PandOS+

## Introduction
Project for the Operating Systems course at the University of Bologna (A.Y. 2021-2022).

### Description
PandOS+ is a didactic operating system for the MIPS architecture running on the [µMPS3 emulator](https://wiki.virtualsquare.org/#!education/umps.md).\
The project consists of three phases:
- Phase 1: implementation of some basic data structures.
- Phase 2: implementation of the kernel basic functionalities such as process scheduling, kernel mode system calls and interrupts handling.
- Phase 3: implementation of the virtual memory manager and user processes handling.

## Installation
µMPS3 emulator is required to run the operating system. Refer to the [official wiki](https://wiki.virtualsquare.org/#!education/tutorials/umps/installation.md) for the installation.\
To compile the source code, a GCC cross-compiler to MIPS is also required (Note: it should come along with µMPS3).

## Compiling
To compile the project run the following commands:
```
mkdir build
cd build
cmake -D CMAKE_TOOLCHAIN_FILE=../mips.cmake ..
make
```
`mips.cmake` contains the cross-compiler information.

## Testing
Move into the `build` directory and compile the test programs by running:
```
cp -r ../phase3/test/{testers,umps3.json} .
make -C testers
```
Then load the `umps3.json` configuration into µMPS3. (Depending on the installation, some paths in `umps3.json` may need to be adjusted).\
Since the test programs run concurrently and each of them outputs on different terminals, it is recommended to open all terminals (_Windows > Terminal [0-7]_) before starting the O.S.

## Credits
The CMake file structure was written starting from the one of this [project](https://github.com/Maldus512/umps_uarm_hello_world).
