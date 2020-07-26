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

### C128 Notes

## Bank 1
I'm storing all Message data in the Commodore 128's bank 1. To do this I store an index in the last 8 pages of available in bank 1 (F7..FE), the index has 4 bytes per message that contain the page(byte), offset(byte) in that page, and length(word) for each message. The engine can take a message #, and use the index to find the Message in bank 1.  

Since all of this is stored in Bank 1, there is a lot of room for Messages, and no penalty in bank 0 for having a lot of messages in a game.  

## Layout
Page Page Start Address in Bank 1
0    0400
1    0500
2    0600
3    0700
 ... ...
239  F300
240  F400
241  F500
242  F600
     Page(byte), Offset(byte), Length (word) x 64 
     Each Page has 64 sets of 4 bytes: POLnPOLnPOLn...POLn
                               111111
     ----            0123456789012345
243  F700: 000-063 : 0000111122223333...6060616162626363
244  F800: 064-127 :
245  F900: 128-191 : 128-129-130-
246  FA00: 192-255 :
247  FB00: 256-319 :
248  FC00: 320-383 :
249  FD00: 384-447 :
250  FE00: 448-511 :
     ----
     FF00 
M#
P = 243 + ((# x 4)/256)
O = (# x 4) % 256

Example:
Message #130 (0 based)
Page = 243(0xF7) + (130x4)/256 = ~2 = Page 245
Offset = (130x4) % 256 = 8
Length = 4 (always 4 for the index)

So, at Page 245 Offset 8 if there were the bytes: 0f,2b,bc01  or  15,43,444  
I would use those to do a second bank 1 "copyfrom" with Page 15, Offset 43, and a Length of 444




