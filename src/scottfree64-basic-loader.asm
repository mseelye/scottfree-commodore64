; scottfree64 - Commodore 64 BASIC bootstrap
;
; (C) 2020 - Mark Seelye - mseelye@yahoo.com - 2020-06-20
;
; This can be compiled with tmpx and the bhz-basic-tokens.asm
; Note: Replace {VERSION} with actual version "vMajor.Minor"
; Version 0.9.1

.include "tools/bhz-basic-tokens.asm"
* = $0801

;2020 
#LINE 2020,_line_2100
.byte TOK_POKE  
.text "53280,11"
#COLON
.byte TOK_POKE
.text "53281,0"
#COLON
#PRINT "{clear}{home}{dark grey}s{medium grey}c{light gray}o{white}ttf{light gray}r{medium grey}e{dark grey}e{white}64{medium gray} {VERSION}{white} - {blue}mseelye 2020-06-20"
#COLON
#PRINT "{yellow}usage{light blue}: copy your favorite .dat scottfree games onto this disk, {yellow}first{light blue} load this:"
#COLON
.byte TOK_PRINT
#EOL
;2100

#L "2100",_line_2200
; print "load ";chr$(34);"scottfree";chr$(34);",";chr$(48+peek(186));",1"
.byte TOK_PRINT
.text "{$22}{white}load {$22}"
.text ";{$c7}(34);"
.text "{$22}scottfree64{$22}"
.text ";{$c7}(34);"
.text "{$22},{$22}"
.text ";{$c7}(48{$aa}{$c2}(186));"
.text "{$22},1{$22}"
#EOL

#L "2200",_line_2300
.byte TOK_PRINT
#COLON
#PRINT "  {yellow}then{light blue} to start your game:"
#COLON
.byte TOK_PRINT
#COLON
#PRINT "{white}run:rem {medium grey}-d{white} {medium grey}-r{white} game.dat {medium grey}savedgame{light blue}"
#COLON
.byte TOK_PRINT
#COLON
#PRINT "  {medium grey}-d, -r{light blue}, {medium grey}savedgame{light blue} are {medium grey}optional{light blue}!"
#COLON
#PRINT "  {medium grey}-d{light blue} shows loading progress"
#COLON
#PRINT "  {medium grey}-r{light blue} restores recent save on death"
#COLON
.byte TOK_PRINT
#COLON
#PRINT "  {yellow}examples{light blue}:"
#EOL

#L "2300",_line_2400
.byte TOK_PRINT
#COLON
.byte TOK_PRINT
#COLON
#PRINT "{white}run:rem -d -r ghostking.dat"
#COLON
#PRINT "run:rem -d -r sampler1.dat save1"
#COLON
#PRINT "run:rem adv03.dat a3save2"
#COLON
#PRINT "run:rem -r 1-baton.dat batonsave3"
#COLON
#PRINT "run:rem -d adv01.dat{light blue}"
#EOL

#L "2400",_line_2500
#REM "scottfree64 back-port of scottfree 1.14b"
#L "2500",eof
#REM "2020-06-20 - mark seelye"
#EOF
