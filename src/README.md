## Scottfree64 Source

This source is ideally built with the Makefile in the root directory, or it can be built with cc65 directly (CC65_HOME being `/usr/local/cc65`):  
```
/usr/local/cc65/bin/cc65.exe --static-locals -Ors --codesize 500 -T -g -t c64 scottfree64.c
/usr/local/cc65/bin/ca65.exe scottfree64.s -o scottfree64.o
/usr/local/cc65/bin/ld65.exe -o scottfree64 -t c64 scottfree64.o c64.lib
```

This would produce the scottfree64 Commodore 64 program binary.  You can put it on a disk with your DAT "ScottFree" format games.

```
LOAD "SCOTTFREE64",8,1
LOADING
READY
RUN:REM -D -R GHOSTKING.DAT
```

### Files
## readme.prg/readme.c64basic:  
[readme.prg](readme.prg) is a BASIC program that gives users similar instructions to those above. The source for this program is in [readme.c64basic](readme.c64basic) and can be built with petcat.

## scottfree.h  
This is actually a copy of [SCOTT.H](http://ifarchive.org/if-archive/scott-adams/interpreters/scottfree/scott.zip), with the only difference being that the `Tail` struct is commented out.

## scottfree64.c  
This is a C program for the Commodore 64. It's a port of Scott Free, A Scott Adams game driver in C.
