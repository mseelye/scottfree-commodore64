; Basic Tokens: Macros
; version 1.0.3
;  Created by Mark Seelye on 2020-05-25.
;  Copyright (c) 2020 Mark Seelye. All rights reserved.
; Mark Seelye  mseelye@yahoo.com
; (a.k.a. Burning Horizon/FTA)
; http://www.burninghorizon.com

; Used to build out C64 BASIC programs in asm
; Currently only have a few statements buildout with macros:

; Ex:
;   .include "src/basic-tokens.asm"
;   * = $0801
;   #LINE 2018,eof
;   #SYS "4096"
;   #COLON
;   #REM "here is a rem statement"
;   #EOF

; LINE takes 2 words, a line number and a pointer to the next line
; Ex:
;   l1  #LINE 1,l2
;       #SYS "49152"
;       #EOL
;   l2  #LINE 2,eof
;       #SYS "49155"
;       #EOF
LINE .macro
    .word \2
    .word \1
.endm

; LINE V2
;   Sets a global scop label automatically, differs from #LINE in that the 
;   line number is passed as a string.
; Ex:
;   #L "1",_line_2
;   #SYS "49152"
;   #EOL
;   #L "2",eof
;   #SYS "49155"
;   #EOF
L .segment
_line_@1 .word \2
    ; word:
    .byte <@1
    .byte >@1
.endm

;Must have this between lines
EOL .macro
  .byte 0
.endm

; EOF puts three 0 bytes, and sets the eof label
; Should be used once and only usually at the end of a BASIC listing.
EOF .macro
eof .var *
  .byte 0,0,0
.endm

; COLON is used to out a : (colon) into line
;   Can be used to separate multiple statements on a single line.
COLON .macro
    .text ":"
.endm

; Used to Construct a REM (remark) in the BASIC statement
;   REM takes a single quoted string
; Ex:
;   #REM "fish"
REM .macro
    .byte TOK_REM
    .text " @1"
    .byte 0
.endm

; Used to construct a SYS statement in the BASIC statement
;   SYS takes a single quoted string representing the decimal address to sys to.
; Ex:
;   #SYS "4096"
SYS .macro
    .byte TOK_SYS
    .text "@1"
.endm

; Used to construct a simple PRINT statement in the BASIC statement
;   PRINT takes a single quoted string representing the string to print.
; Ex:
;   #PRINT "FISH" -> PRINT"FISH"
PRINT .macro
    .byte TOK_PRINT
    .byte 32,34
    .text "@1"
    .byte 34
.endm

CHR .macro
    .byte TOK_CHR_D
    .byte 40 ; (
    .text "@1"
    .byte 41 ; )
.endm

; Add more macros here! :D
; Basic Tokens: Labels
; version 1.0.3
;  Created by Mark Seelye on 2020-05-25.
;  Copyright (c) 2020 Mark Seelye. All rights reserved.
; Mark Seelye  mseelye@yahoo.com
; (a.k.a. Burning Horizon/FTA)
; http://www.burninghorizon.com

; Actual C64 BASIC Token Values
TOK_END       =  $80
TOK_FOR       =  $81
TOK_NEXT      =  $82
TOK_DATA      =  $83
TOK_INPUT_P   =  $84 ; INPUT#
TOK_INPUT     =  $85
TOK_DIM       =  $86
TOK_READ      =  $87
TOK_LET       =  $88
TOK_GOTO      =  $89
TOK_RUN       =  $8A
TOK_IF        =  $8B
TOK_RESTORE   =  $8C
TOK_GOSUB     =  $8D
TOK_RETURN    =  $8E
TOK_REM       =  $8F
TOK_STOP      =  $90
TOK_ON        =  $91
TOK_WAIT      =  $92
TOK_LOAD      =  $93
TOK_SAVE      =  $94
TOK_VERIFY    =  $95
TOK_DEF       =  $96
TOK_POKE      =  $97
TOK_PRINT_P   =  $98 ; PRINT#
TOK_PRINT     =  $99
TOK_CONT      =  $9A
TOK_LIST      =  $9B
TOK_CLR       =  $9C
TOK_CMD       =  $9D
TOK_SYS       =  $9E
TOK_OPEN      =  $9F
TOK_CLOSE     =  $A0
TOK_GET       =  $A1
TOK_NEW       =  $A2
TOK_TAB_P     =  $A3 ; TAB(
TOK_TO        =  $A4
TOK_FN        =  $A5
TOK_SPC_P     =  $A6 ; SPC(
TOK_THEN      =  $A7
TOK_NOT       =  $A8
TOK_STEP      =  $A9
TOK_O_ADD     =  $AA ; + (Addition)
TOK_O_SUB     =  $AB ; − (Subtraction)
TOK_O_MULT    =  $AC ; * (Multiplication)
TOK_O_DIV     =  $AD ; / (Division)
TOK_O_POW     =  $AE ; ↑ (Power)
TOK_O_AND     =  $AF ; AND
TOK_O_OR      =  $B0 ; OR
TOK_O_GT      =  $B1 ; > (greater-than operator)
TOK_O_EQ      =  $B2 ; = (equals operator)
TOK_O_LT      =  $B3 ; < (less-t=han$B3 operator)
TOK_SGN       =  $B4
TOK_INT       =  $B5
TOK_ABS       =  $B6
TOK_USR       =  $B7
TOK_FRE       =  $B8
TOK_POS       =  $B9
TOK_SQR       =  $BA
TOK_RND       =  $BB
TOK_LOG       =  $BC
TOK_EXP       =  $BD
TOK_COS       =  $BE
TOK_SIN       =  $BF
TOK_TAN       =  $C0
TOK_ATN       =  $C1
TOK_PEEK      =  $C2
TOK_LEN       =  $C3
TOK_STR_D     =  $C4 ; STR$
TOK_VAL       =  $C5
TOK_ASC       =  $C6
TOK_CHR_D     =  $C7 ; CHR$
TOK_LEFT_D    =  $C8 ; LEFT$
TOK_RIGHT_D   =  $C9 ; RIGHT$
TOK_MID_D     =  $CA ; MID$
TOK_GO        =  $CB