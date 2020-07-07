#
# ScottFree64 Makefile
# Reworking of ScottFree Revision 1.14b for the Commodore 64
#
# Scott Free, A Scott Adams game driver in C.  
# Release 1.14b (PC), (c) 1993,1994,1995 Swansea University Computer Society.  
# Port to Commodore 64 as ScottFree64 Release 0.9, (c) 2020 Mark Seelye  
# Distributed under the GNU software license  
# (C) 2020 - Mark Seelye - mseelye@yahoo.com - 2020-06-20
#

# Release Version
VERSION = v0.9.4

# Determine what OS this is running on and adjust
OSUNAME := $(shell uname)
ifeq "$(OSUNAME)" "Darwin"
	ECHO := echo
	CC65_HOME := /usr/local
else ifeq "$(OSUNAME)" "Linux"
	ECHO := echo -e
	CC65_HOME := /usr
else # Windows
	ECHO := echo -e
	CC65_HOME := /usr/local/cc65
endif

# Target System
SYS ?= c64

# Required Executables
AS = $(CC65_HOME)/bin/ca65
CC = $(CC65_HOME)/bin/cc65
LD = $(CC65_HOME)/bin/ld65
MKDIR_P = mkdir -p

# Optional Executables
# c1541 - used to create d64 and d81 disk images
C1541_EXE := c1541
C1541 := "$(shell command -v $(C1541_EXE) 2> /dev/null)"
C1541_CONSIDER = "please consider installing vice/$(C1541_EXE) from https://vice-emu.sourceforge.io/"

# petcat - used to assemble/create the readme.prg
PETCAT_EXE := petcat
PETCAT = "$(shell command -v $(PETCAT_EXE) 2> /dev/null)"
PETCAT_CONSIDER = "please consider installing $(PETCAT_EXE) from https://vice-emu.sourceforge.io/"

# exomizer - used to crunch the scottfree64 program file
CRUNCHER_EXE = exomizer
CRUNCHER = "$(shell command -v $(CRUNCHER_EXE) 2> /dev/null)"
CRUNCHER_CONSIDER = " please consider installing $(CRUNCHER_EXE) from https://bitbucket.org/magli143/exomizer/wiki/Home"
ifeq ("",$(CRUNCHER))
	CRUNCHER_EXT := 
else
	CRUNCHER_EXT := .sfx
endif

# Compiler flags
CRUNCHERFLAGS =  sfx sys -m 16384 -q -n
#CFLAGS = --static-locals -Ors --codesize 500 -T -g -t $(SYS)
CFLAGS = --static-locals -Ors --codesize 100 -t $(SYS)

# Check for required executables
REQ_EXECUTABLES = $(AS) $(CC) $(LD)
K := $(foreach exec,$(REQ_EXECUTABLES),\
        $(if $(shell which $(exec)),some string,$(error "Required tool $(exec) not found in PATH")))

# Version inject
DIST_SED:=s/{VERSION}/$(VERSION)/g;

# Directories
OBJDIR = out
SRCDIR = src
BINDIR = bin
DISTDIR = dist
GAMESDIR = games

# Main target
TARGET = scottfree64
DIST = $(DISTDIR)/$(TARGET)

# Note: 16 character limit on filenames in a d64/d81 disk image
#GAMES   := ghostking.dat sampler1.dat
GAMES   := ghostking.dat sampler1.dat ghostking.bdat sampler1.bdat
SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(SRCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
ASMS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

all: directories $(BINDIR)/$(TARGET) readme disk disk81

%: %.c
%: %.s

# Link main target
$(BINDIR)/$(TARGET): $(OBJECTS)
	@$(ECHO) "*** Linking $<"
	$(LD) $(LDFLAGS) -o $@ -t $(SYS) -m $@.map $(OBJECTS) $(SYS).lib
ifeq ("",$(CRUNCHER))
	@$(ECHO) "*** Note: $(CRUNCHER_EXE) is not in PATH, cannot crunch $@, $(CRUNCHER_CONSIDER)"
else
	$(CRUNCHER) $(CRUNCHERFLAGS) -o $@$(CRUNCHER_EXT) $@
endif
	@$(ECHO) "*** Linking complete\n"

# Compile main target
$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c $(INCLUDES)
	@$(ECHO) "*** Compiling $<"
	@cat $< | sed -e "$(DIST_SED)" > $<.tmp
	$(CC) $(CFLAGS) $<.tmp
	$(AS) $<.s -o $@
#	$(AS) $(<:.c =.s) -o $@
	@$(ECHO) "*** Compilation complete\n"
#	@rm $<.tmp $<.s

# petcat BASIC stub/readme program
# petcat -ic -w2 -o readme2.prg -- readme.txt
.PHONY: readme
readme: $(SRCDIR)/readme.prg
$(SRCDIR)/readme.prg: $(SRCDIR)/readme.c64basic
ifeq ("",$(PETCAT))
	@$(ECHO) "*** Note: $(PETCAT_EXE) is not in PATH, cannot build readme, $(PETCAT_CONSIDER)"
else
	@$(ECHO) "*** Tokenizing $< with $(PETCAT)"
	@cat $< | sed -e "$(DIST_SED)" > $<.tmp
	$(PETCAT) -ic -w2 -o $@ -- $<.tmp
	@$(ECHO) "*** Tokenization complete\n"
	@rm $<.tmp
endif

# Create d64 disk
.PHONY: disk
disk:  $(DIST).d64
$(DIST).d64: $(BINDIR)/$(TARGET) $(SRCDIR)/readme.prg
ifeq ("",$(C1541))
	@$(ECHO) "\n*** Note: c1541 is not in PATH, cannot build disk. $(C1541_CONSIDER)"
else
	@$(ECHO) "\n*** Building d64 disk...$@"
	@$(C1541) -format scottfree64,bh d64 $@ -attach $@ -write src/readme.prg readme
	@$(C1541) -attach $@ -write $(BINDIR)/$(TARGET)$(CRUNCHER_EXT) $(TARGET)
# Base Set of Games
	@for dat in  $(GAMES); \
	    do $(C1541) -attach $@ -write $(GAMESDIR)/$$dat $$dat; \
	done
# add your games
#	@$(C1541) -attach $@ -write games/hold_the_snappy.dat snappy.dat
#	@$(C1541) -attach $@ -write games/fish.dat fish.dat
# add your game save files
#	@$(C1541) -attach $@ -write games/save-1 save1
#	@$(C1541) -attach $@ -write games/atend atend
	@$(ECHO) "\n*** Disk Contents:"
	@$(C1541) -attach $@ -dir
	@$(ECHO) "\n*** Building d64 disk complete\n"
endif

# Create d81 disk
.PHONY: disk81
disk81: $(DIST).d81
$(DIST).d81: $(BINDIR)/$(TARGET) $(SRCDIR)/readme.prg
ifeq ("",$(C1541))
	@$(ECHO) "\n*** Note: c1541 is not in PATH, cannot build d81 disk. $(C1541_CONSIDER)"
else
	@$(ECHO) "\n*** Building d81 disk...$@\n"
	@$(C1541) -format scottfree64,bh d81 $@ -attach $@ -write src/readme.prg readme
	@$(C1541) -attach $@ -write $(BINDIR)/$(TARGET)$(CRUNCHER_EXT) $(TARGET)
# Base Set of Games
	@for dat in  $(GAMES); \
	    do $(C1541) -attach $@ -write $(GAMESDIR)/$$dat $$dat; \
	done
# Additional Games in the d81 image
	@$(C1541) -attach $@ -write games/1_baton.dat 1-baton.dat
	@$(C1541) -attach $@ -write games/2_timemachine.dat 2-timemach.dat
	@$(C1541) -attach $@ -write games/3_arrow1.dat 3-arrow1.dat
	@$(C1541) -attach $@ -write games/4_arrow2.dat 4-arrow2.dat
	@$(C1541) -attach $@ -write games/5_pulsar7.dat 5-pulsar7.dat
	@$(C1541) -attach $@ -write games/6_circus.dat 6-circus.dat
	@$(C1541) -attach $@ -write games/7_feasibility.dat 7-feas.dat
	@$(C1541) -attach $@ -write games/8_akyrz.dat 8-akyrz.dat
	@$(C1541) -attach $@ -write games/9_perseus.dat 9-perseus.dat
	@$(C1541) -attach $@ -write games/A_tenlittleindians.dat a-10indians.dat
	@$(C1541) -attach $@ -write games/B_waxworks.dat b-waxworks.dat
	@$(C1541) -attach $@ -write games/adv01.dat adv01.dat
	@$(C1541) -attach $@ -write games/adv02.dat adv02.dat
	@$(C1541) -attach $@ -write games/adv03.dat adv03.dat
	@$(C1541) -attach $@ -write games/adv04.dat adv04.dat
	@$(C1541) -attach $@ -write games/adv05.dat adv05.dat
	@$(C1541) -attach $@ -write games/adv06.dat adv06.dat
	@$(C1541) -attach $@ -write games/adv07.dat adv07.dat
	@$(C1541) -attach $@ -write games/adv08.dat adv08.dat
	@$(C1541) -attach $@ -write games/adv09.dat adv09.dat
	@$(C1541) -attach $@ -write games/adv10.dat adv10.dat
	@$(C1541) -attach $@ -write games/adv11.dat adv11.dat
	@$(C1541) -attach $@ -write games/adv12.dat adv12.dat
	@$(C1541) -attach $@ -write games/adv13.dat adv13.dat
	@$(C1541) -attach $@ -write games/adv14a.dat adv14a.dat
	@$(C1541) -attach $@ -write games/adv14b.dat adv14b.dat
	@$(C1541) -attach $@ -write games/quest1.dat quest1.dat
	@$(C1541) -attach $@ -write games/quest2.dat quest2.dat
	@$(C1541) -attach $@ -write games/1_baton.bdat 1-baton.bdat
	@$(C1541) -attach $@ -write games/2_timemachine.bdat 2-timemach.bdat
	@$(C1541) -attach $@ -write games/3_arrow1.bdat 3-arrow1.bdat
	@$(C1541) -attach $@ -write games/4_arrow2.bdat 4-arrow2.bdat
	@$(C1541) -attach $@ -write games/5_pulsar7.bdat 5-pulsar7.bdat
	@$(C1541) -attach $@ -write games/6_circus.bdat 6-circus.bdat
	@$(C1541) -attach $@ -write games/7_feasibility.bdat 7-feas.bdat
	@$(C1541) -attach $@ -write games/8_akyrz.bdat 8-akyrz.bdat
	@$(C1541) -attach $@ -write games/9_perseus.bdat 9-perseus.bdat
	@$(C1541) -attach $@ -write games/A_tenlittleindians.bdat a-10indians.bdat
	@$(C1541) -attach $@ -write games/B_waxworks.bdat b-waxworks.bdat
	@$(C1541) -attach $@ -write games/adv01.bdat adv01.bdat
	@$(C1541) -attach $@ -write games/adv02.bdat adv02.bdat
	@$(C1541) -attach $@ -write games/adv03.bdat adv03.bdat
	@$(C1541) -attach $@ -write games/adv04.bdat adv04.bdat
	@$(C1541) -attach $@ -write games/adv05.bdat adv05.bdat
	@$(C1541) -attach $@ -write games/adv06.bdat adv06.bdat
	@$(C1541) -attach $@ -write games/adv07.bdat adv07.bdat
	@$(C1541) -attach $@ -write games/adv08.bdat adv08.bdat
	@$(C1541) -attach $@ -write games/adv09.bdat adv09.bdat
	@$(C1541) -attach $@ -write games/adv10.bdat adv10.bdat
	@$(C1541) -attach $@ -write games/adv11.bdat adv11.bdat
	@$(C1541) -attach $@ -write games/adv12.bdat adv12.bdat
	@$(C1541) -attach $@ -write games/adv13.bdat adv13.bdat
	@$(C1541) -attach $@ -write games/adv14a.bdat adv14a.bdat
	@$(C1541) -attach $@ -write games/adv14b.bdat adv14b.bdat
	@$(C1541) -attach $@ -write games/quest1.bdat quest1.bdat
#	@$(C1541) -attach $@ -write games/quest2.bdat quest2.bdat
# add your games
#	@$(C1541) -attach $@ -write games/hold_the_snappy.dat snappy.dat
#	@$(C1541) -attach $@ -write games/fish.dat fish.dat
# add your game save files
#	@$(C1541) -attach $@ -write games/save-1 save1
#	@$(C1541) -attach $@ -write games/atend atend
	@$(ECHO) "\n*** Disk Contents:"
	@$(C1541) -attach $@ -dir
	@$(ECHO) "\n*** Building d81 disk complete\n"
endif

# Cleans
.PHONY: clean
clean:
	-rm -rf $(OBJDIR)
	-rm -rf $(BINDIR)
	-rm -f $(SRCDIR)/$(TARGET).s
ifeq ("",$(PETCAT))
	@$(ECHO) "*** Note: $(PETCAT_EXE) is not in PATH, not cleaning readme.prg, $(PETCAT_CONSIDER)"
else
	-rm -f $(SRCDIR)/readme.prg
endif

# Build out directories
.PHONY: directories
directories: $(OBJDIR) $(BINDIR) $(DISTDIR)
$(OBJDIR):
	@$(MKDIR_P) $(OBJDIR)
$(BINDIR):
	@$(MKDIR_P) $(BINDIR)
$(DISTDIR):
	@$(MKDIR_P) $(DISTDIR)
