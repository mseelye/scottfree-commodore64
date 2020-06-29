#
# ScottFree64 Makefile
# (C) 2020 - Mark Seelye - mseelye@yahoo.com - 2020-06-20
#

# Release Version
VERSION = v0.9

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

# tmpx - used to assemble/create the readme.prg
TMPX_EXE := tmpx
TMPX = "$(shell command -v $(TMPX_EXE) 2> /dev/null)"
TMPX_CONSIDER = "please consider installing $(TMPX_EXE) from http://turbo.style64.org"

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
CFLAGS = --static-locals -Ors --codesize 500 -T -g -t $(SYS)

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
GAMES   := ghostking.dat sampler1.dat
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
	@rm $<.tmp $<.s

# Assemble BASIC stub/readme program
.PHONY: readme
readme: $(SRCDIR)/readme.prg
$(SRCDIR)/readme.prg: $(SRCDIR)/scottfree64-basic-loader.asm
ifeq ("",$(TMPX))
	@$(ECHO) "*** Note: $(TMPX_EXE) is not in PATH, cannot build readme, $(TMPX_CONSIDER)"
else
	@$(ECHO) "*** Assembling $< with $(TMPX)"
	@cat $< | sed -e "$(DIST_SED)" > $<.tmp
	$(TMPX) $<.tmp -o $@
	@$(ECHO) "*** Assembling complete   $<.tmp \n"
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
	-rm -rf $(DISTDIR)
ifeq ("",$(TMPX))
	@$(ECHO) "*** Note: $(TMPX_EXE) is not in PATH, not cleaning readme.prg, $(TMPX_CONSIDER)"
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
