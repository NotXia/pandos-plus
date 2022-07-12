# PandOS+

## Installazione
```
mkdir build
cd build
cmake -D CMAKE_TOOLCHAIN_FILE=../mips.cmake ..
make
```
Il file `mips.cmake` contiene le specifiche necessarie per la cross-compilazione.

## Testing
All'interno di `build` eseguire:
```
cp -r ../phase3/test/{testers,umps3.json} .
make -C testers
```
Quindi avviare µMPS3 utilizzando la macchina con le configurazioni contenute in `umps3.json`.

### Nota
In base all'installazione, potrebbe essere necessario modificare i percorsi contenuti in `umps3.json`.

## Crediti
Il file cmake per la cross-compilazione è stato realizzato sulla base del [progetto](https://github.com/Maldus512/umps_uarm_hello_world).
