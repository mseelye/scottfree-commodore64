; ScottFree-64 - a Reworking of ScottFree Revision 1.14b for the Commodore 64
; Optimization Library by Mark Seelye
; (C) 2020 - Mark Seelye / mseelye@yahoo.com
; Version 0.1
;
; Original copyright and license for ScottFree Revision 1.14b follows:
; * ScottFree Revision 1.14b
; * This program is free software; you can redistribute it and/or
; * modify it under the terms of the GNU General Public License
; * as published by the Free Software Foundation; either version
; * 2 of the License, or (at your option) any later version.
; * You must have an ANSI C compiler to build this program.

;   .autoimport on ; no

; print exports
    .export _print
    .export _print_char
    .export _bufnum8
    .export _bufnum16
    .export _bufnum32
    .export _print_number
    .export _print_signed_8

; ascii to petscii exports
    .export _a2p
    .export _a2p_string
    
; word parse exports
    .export _parse_verb_noun
    .export _is_space

;runtime imports
    .import incsp5, incsp7
    .import ldax0sp, ldaxysp
    .import pusha, pushax, pushwysp
    .import popax
    .importzp sp, ptr1, sreg

; DEBUG - get stack pointer
;    .export _getsp

    .data

; ------------------------------------------------------------------------------
; Print Single Character
; void __fastcall__ print_char(char c);
_print_char:
    jsr $ffd2
    rts

; ------------------------------------------------------------------------------
; Print from *text until a 0 is reached.
; void __fastcall__ print(char *text);
_print:
.scope
    sta ptr1
    stx ptr1+1
    ldy #0
loop:
    lda (ptr1),y
    beq done
    jsr $ffd2
    iny
    bne loop
    inc ptr1+1  ; > 255 characters!
    bne loop
done:
    rts
.endscope

; ------------------------------------------------------------------------------
; Print 32 bit Number
;  .x/.a number hi/lo
; void __fastcall__ print_number(uint32_t);
_print_number:
.scope
        jsr _bufnum32
        sty _len ; length
        jsr _print
        rts
_len: .byte 0
.endscope

; ------------------------------------------------------------------------------
; Print Signed 8 bit Number
;  .a number
;void __fastcall__ print_signed_8(int8_t);
_print_signed_8:
.scope
        sta _v
        and #$80
        beq print
        dec _v
        lda _v
        eor #$ff
        sta _v
        lda #'-'
        jsr _print_char
print:  lda _v
        jsr _bufnum8
        jsr _print
        rts
_v: .byte 0
.endscope

; ------------------------------------------------------------------------------
; bufnum* (Note: Derived from Graham's 32 bit print)
;  prints a 8/16/32 bit value to a buffer
;  Returns pointer to buffer in .a .x (lo/hi)
;  Note: .y contains num chars in result (for internal/asm use)
; uint16_t __fastcall__ bufnum8(uint8_t);
_bufnum8:
    sta bufnum_value
    ldy #0
    sty bufnum_value+1
    sty bufnum_value+2
    sty bufnum_value+3
    jmp bufnum
; uint16_t __fastcall__ bufnum16(uint16_t);
_bufnum16:
    sta bufnum_value
    stx bufnum_value+1
    ldy #0
    sty bufnum_value+2
    sty bufnum_value+3
    jmp bufnum
; uint16_t __fastcall__ bufnum32(uint32_t);
_bufnum32:
    sta bufnum_value
    stx bufnum_value+1
    lda sreg
    sta bufnum_value+2
    lda sreg+1
    sta bufnum_value+3
; bufnum - internal - argument: 32 bit value lo/hi .a .x sreg sreg+1
;                     returns pointer to buffer in .a .x (lo/hi)
bufnum:
.scope
        jsr hex2dec
; skips leading zeros
        ldx #9
sk:     lda result,x
        bne go
        dex 
        bne sk
go:     ldy #0
p:      lda result,x
        ora #$30 ; -> petscii
        sta buffer,y
        iny
        dex
        bpl p
        lda #0
        sta buffer,y ; zero at end
        lda #<buffer ; return lo/hi of buffer to print
        ldx #>buffer
        sty buffer+11
        rts ; .y = num chars (not usable by C programs)
hex2dec:
        ldx #0
div:    jsr div10
        sta result,x
        inx
        cpx #10      ; 8 digits
        bne div
        rts
; divides a 16(32) bit value by 10, remainder in .a
div10 = *
        ldy #32 ; 32 bits
        lda #0
        clc
again:  rol
        cmp #10
        bcc rl
        sbc #10
rl:     rol bufnum_value
        rol bufnum_value+1
        rol bufnum_value+2
        rol bufnum_value+3
        dey
        bpl again
        rts
; bufnum 32 bit storage
result: .byte 0,0,0,0,0,0,0,0,0,0 ; 0 - 4294967295 (10 digits)
buffer: .byte 0,0,0,0,0,0,0,0,0,0,0 ; "xxxxxxxxxx@" (11 characters with \0)
size:   .byte 0
.endscope
bufnum_value: .byte $ff,$ff,$ff,$ff


; ------------------------------------------------------------------------------
; ASCII to PETSCII
;   If ASCII and the chr$ is 65 to 90 then add 32.
;   If ASCII and the chr$ is 97 to 122 then subtract 32.
;   If ASCII and the chr$ is 8 then change it to 20.
; Note: Must not trash .y (used by _a2p_string)
; uint8_t __fastcall__ a2p (uint8_t)
_a2p:
.scope
    clc
    cmp #65         ; A
    bcc checklower
    cmp #91         ; Z+1
    bcs checklower
    adc #32         ; in+32
    rts
checklower:
    clc
    cmp #97         ; a
    bcc done
    cmp #123        ; z+1
    bcs done
    sec
    sbc #32         ; in-32
done:
    rts
.endscope

; ------------------------------------------------------------------------------
; Transcode from *char until a 0 is reached.
;  Note: works just like _print
; void __fastcall__ a2p_string (unsigned char *)
_a2p_string:
.scope
    sta ptr1
    stx ptr1+1
    ldy #0
loop:
    lda (ptr1),y
    beq done
    jsr _a2p        ; must not trash .y
    sta (ptr1),y
    iny
    bne loop
    inc ptr1+1      ; > 255 characters!
    bne loop
done:
    tya
    ldx #0
    rts
.endscope

; ------------------------------------------------------------------------------
; Parse Verb and Noun - optimized "scanf" replacement.
; Similar C code:
; uint8_t ParseVerbNoun(char *buf, uint8_t wl, char *verb, char *noun) {
;     uint8_t ct = 0;
;     ct = NextWord(buf, wl, verb);
;     NextWord((char *)buf+ct, wl, noun);
;     return( (verb[0]?1:0) + (noun[0]?1:0));
; }
; Returns 0, 1, or 2; No words, Verb, Verb + Noun
; uint8_t __fastcall__ parse_verb_noun(char *buf, uint8_t wl, char *verb, char *noun);
_parse_verb_noun:
.scope
    jsr pushax          ; *noun
    lda #$00            ; uint8_t ct = 0;
    sta ct
    ldy #$08
    jsr pushwysp        ;  > *buf
    ldy #$06
    lda (sp),y          ; read wl from param stack
    jsr pusha           ;  > wl
    ldy #$06
    jsr ldaxysp         ;  | *verb
    jsr _next_word      ; ct = NextWord(buf, wl, verb);
    sta ct              ; result .a into ct
    ldy #$06
    jsr ldaxysp         ; *buf
    clc
    adc ct              ; add ct
    bcc nh              ; hi not set
    inx                 ; inc *buf hi
nh: jsr pushax          ;  > *buf (+ ct)
    ldy #$06
    lda (sp),y          ; read wl from param stack
    jsr pusha           ;  > wl
    ldy #$04
    jsr ldaxysp         ;  | *noun
    jsr _next_word      ; ct NextWord((char *)buf+ct, wl, noun);
    ldy #$03
    jsr ldaxysp         ; *verb
    jsr firstbyte       ; .a <- verb[0]
    beq done            ; no verb, then no noun either; return 0
    jsr ldax0sp         ; *noun
    jsr firstbyte       ; .a <- noun[0]
    beq nn              ; no noun
    lda #2              ; noun and a verb; return 2
    bne done
nn: lda #1              ; no noun, but verb; return 1
done:
    ldx #$00            ; clear high return
    jmp incsp7          ; inc sp by 7, rts
firstbyte:              ; get first byte
    sta ptr1
    stx ptr1+1
    ldy #$00
    lda (ptr1),y        ; word[0]
    rts
ct: .res 1,$00
.endscope

; ------------------------------------------------------------------------------
; Next Word in Buffer - optimized for "scanf" replacement.
; Similar C code:
;uint8_t NextWord(char *buf, uint8_t wl, char *word) {
;    uint8_t ct = 0;
;    uint8_t i = 0;
;    while(buf[ct]!=0 && buf[ct]==' ') {
;        ct++;
;    }
;    while(buf[ct]!=0) {
;        if(buf[ct]==' ' || i >= wl)
;            break;
;        word[i++]=buf[ct++];
;    }
;    word[i]=0;
;    while(buf[ct]!=0) {
;        if(buf[ct]==' ')
;            break;
;        ct++;
;    }
;    return ct;
;}
; Note: Skips leading spaces
;       Reads up to space or word length (wl) # characters into buffer at *word
;       Skips trailing characters until space or EOL
; uint8_t __fastcall__ next_word(char *buf, uint8_t wl, char *word);
_next_word:
.scope
    jsr pushax      ; *word
    lda #$00
    sta ct          ; uint8_t ct = 0;
    sta idx         ; uint8_t idx = 0;
s1:                 ; setup ptr for while #1
    ldy #$04
    jsr ldaxysp     ; *buf
    sta ptr1
    stx ptr1+1
    jmp w1          ; jmp to while #1
c1: inc ct          ; ct++
w1: ldy ct
    lda (ptr1),y    ;  buf[ct]!=0
    beq s2          ;  otherwise to while #2
    cmp #$20        ;  buf[ct]==' '
    beq c1          ;    if (buf[ct]!=0 && buf[ct]==' ') continue
    jmp s2          ; otherwise while #2
s2:                 ; setup while #2 (ptr1 setup already)
    ldy #$02        
    lda (sp),y
    sta wl          ; wl
    jsr ldax0sp     ; *word
    sta sreg
    stx sreg+1
    jmp w2          ; jmp to while #2 bounds check
c2: cmp #$20        ;  buf[ct] == ' ' jmp to word term
    beq wordterm
    lda idx
    cmp wl
    bcs wordterm    ; idx >= wl jmp to word term
    ldy ct
    inc ct          ; ct++
    lda (ptr1),y    ; buf[ct]
    ldy idx
    inc idx         ; idx++
    sta (sreg),y    ; word[idx]=buf[ct]
w2: ldy ct          ; while #2 bounds check
    lda (ptr1),y    ; this loads buf[ct] for the c2 body
    bne c2
wordterm:           ; Terminate word at current idx
    lda #$00
    ldy idx
    sta (sreg),y    ; word[idx]=0
s3:                 ; setup ptr for while #3
    ldy ct
    jmp w3
c3: cmp #$20        ; buf[ct] == ' ' jmp to done
    beq done
    iny
w3: lda (ptr1),y
    bne c3
done:
    tya             ; ct value from .y in return lo
    ldx #$00        ; 0 in return high
    jmp incsp5      ; clear sp
ct: .res 1,$00
wl: .res 1,$00
idx: .res 1,$00
.endscope

; ------------------------------------------------------------------------------
; Is Character a Whitespace Character - Replacement for larger cc65 "isspace"
; uint8_t __fastcall__ isspace(uint8_t);
_is_space:
.scope
    ldx #0
    ldy #0
loop:
    cmp sp,y
    beq yes
    iny
    cpy #14
    bne loop
no: lda #0
    rts
yes: 
    lda #1
    rts
; From cc65: Enable Sh/C=, \n, \r, Cur Down, Home, Del, Cur Right, Space, Sh Return, Cur Up, Clr, Inst, Cur Left, Sh Space
sp: .byte               9, 10, 13,       17,   19,  20,        29,    32,       141,    145, 147,  148,      157,      160
.endscope

; ------------------------------------------------------------------------------
;; DEBUG - Get Stack Pointer - Used for debugging stack issues.
;;uint16_t __fastcall__ getsp(void);
;_getsp:
;    lda sp
;    ldx sp+1
;    rts
