# libtar
Command line tool for managing tar files, and C library.
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
## Usage
This program tries to be compatible with POSIX tar utility, so read the man page.
However, not all features have been implemented.

If the archive file is not specified, standard input will be used for extraction, and standard output otherwise.

Long form options are available.

Options available are:
```
-A -C -U -c -f -k -m -p -r -t -u -W -v -x
--skip-old-files
--keep-newer-files
```
