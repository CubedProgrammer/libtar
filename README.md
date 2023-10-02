# libtar
Tar library and command line tool
## Compilation
With cxso from [linux\_utility](https://github.com/CubedProgrammer/linux_utility)
```
cc -O3 -c main.c tar.c utils.c
cxso -o libtar.so main.o tar.o utils.o
```
Without cxso:
```
CC -O3 -c main.c tar.c utils.c
cc -shared -o libtar.so /usr/lib/Scrt1.o main.o tar.o utils.o
```
The exact location of Scrt1.o may not be /usr/lib on your computer, make sure you have the correct location.