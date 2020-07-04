# ScottFree-64 Tool-Chain Docs
Mark Seelye / mseelye@yahoo.com

Giving credit and giving links to tools used to make and build this.  

### cc65 
**CC65** development tools are required to build this. CC65 binaries can be obtained for some platforms, but it can also be built from source.  
https://cc65.github.io/

### Vice / c1541 / petcat
**c1541** is a tool that comes packaged with the VICE emulator suite. Binaries for c1541 are available for most platfroms but it too can be built from source.  
**petcat** is also a tool that comes packaged with the VICE emulator suite. Binaries for petcat are available for most platfroms but it too can be built from source.  
https://sourceforge.net/projects/vice-emu/  
https://sourceforge.net/p/vice-emu/code/HEAD/tree/trunk/vice/src/Makefile.am (look for c1541-all and petcat-all target)  

### exomizer
**Exomizer** is a tool used to compresses files, it can generate a self-extracting program that is compatible with  cc65 arguments passing. Exomizer has a windows binary available, but can also be built from source. It is available from: (Look for the text, "Download it here.")  
https://bitbucket.org/magli143/exomizer/wiki/Home

### TMPx
**TMPx** (pronounced "T-M-P cross") is the multiplatform cross assembler version of Turbo Macro Pro, made by my friends and the c64 group "Style"!
TMPx was used previously to assemble the old BASIC shim "scottfree64-basic-loader.asm", however the shim grew and petcat was a better option. Check out TMPx anyway, it's a great cross assembler!
http://turbo.style64.org/

### make
I use GNU Make 4.3 in my Windows MSys2 environment, GNU Make 4.1 in my Ubuntu environment, and GNU Make 3.8.1 on my MacBook Pro.  

### Various
The Makefile also makes use of various shell commands like **echo, cat, sed, rm** and **mkdir**. Hopefully these are available in your build environment.  
