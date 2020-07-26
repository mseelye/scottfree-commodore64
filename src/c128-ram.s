;
; Extended memory driver for the C128 RAM in bank #1. Driver works without
; problems when statically linked.
;
; Ullrich von Bassewitz, 2002-12-04
;
; Custom Shrunk version for ScottFree128 - by Mark Seelye 2020-07-25
;  Notes: Removed unused MAP, USE and COMMIT - all just "rts"

        .include        "zeropage.inc"
        .include        "em-kernel.inc"
        .include        "em-error.inc"
        .include        "c128.inc"

; ------------------------------------------------------------------------
; Header. Includes jump table

        .data
        .export         _c128_ram_emd
_c128_ram_emd:

; Driver signature

        .byte   $65, $6d, $64           ; "emd"
        .byte   EMD_API_VERSION         ; EM API version number

; Library reference

        .addr   $0000

; Jump table

        .addr   INSTALL
        .addr   UNINSTALL
        .addr   PAGECOUNT
        .addr   MAP
        .addr   USE
        .addr   COMMIT
        .addr   COPYFROM
        .addr   COPYTO

; ------------------------------------------------------------------------
; Constants

BASE    = $400
TOPMEM  = $FF00
PAGES   = (TOPMEM - BASE) / 256

.code

; ------------------------------------------------------------------------
; INSTALL routine. Is called after the driver is loaded into memory. If
; possible, check if the hardware is present and determine the amount of
; memory available.
; Must return an EM_ERR_xx code in a/x.
INSTALL:
        ldx     #$00
        txa                             ; A = X = EM_ERR_OK
;       rts                             ; Run into UNINSTALL instead

; ------------------------------------------------------------------------
; MAP: Map the page in a/x into memory and return a pointer to the page in
; a/x. The contents of the currently mapped page (if any) may be discarded
; by the driver.
; seelye - don't need map
MAP:
;       rts                             ; Run into UNINSTALL instead

; ------------------------------------------------------------------------
; USE: Tell the driver that the window is now associated with a given page.
; seelye - don't need USE
USE:
;       rts                             ; Run into UNINSTALL instead

; ------------------------------------------------------------------------
; COMMIT: Commit changes in the memory window to extended storage.
; seelye - don't need COMMIT
COMMIT:
;       rts                             ; Run into UNINSTALL instead

; ------------------------------------------------------------------------
; UNINSTALL routine. Is called before the driver is removed from memory.
; Can do cleanup or whatever. Must not return anything.
UNINSTALL:
        rts

; ------------------------------------------------------------------------
; PAGECOUNT: Return the total number of available pages in a/x.
PAGECOUNT:
        lda     #<PAGES
        ldx     #>PAGES
        rts

;; ------------------------------------------------------------------------
;; COPYFROM/TO Setup:
;;  Setup offset, page, to/from address, and count of bytes, and init .y
copy_setup:
        sta     ptr3
        stx     ptr3+1

        ldy     #EM_COPY::OFFS
        lda     (ptr3),y
        sta     ptr1
        ldy     #EM_COPY::PAGE
        lda     (ptr3),y
        clc
        adc     #>BASE
        sta     ptr1+1

        ldy     #EM_COPY::BUF
        lda     (ptr3),y
        sta     ptr2
        iny
        lda     (ptr3),y
        sta     ptr2+1

        lda     #<ptr1
        sta     FETVEC
        sta     STAVEC
        ldy     #EM_COPY::COUNT+1
        lda     (ptr3),y
        beq     @remainder
        sta     tmp1
        ldy     #$00
        sei
        rts
@remainder:
        ldy     #$ff
        sei
        rts

;; ------------------------------------------------------------------------
;; COPYFROM: Copy from extended into linear memory. A pointer to a structure
;; describing the request is passed in a/x.
;; The function must not return anything.
COPYFROM:
        jsr     copy_setup
        cpy #$ff
        beq @L2
; Copy full pages
@L1:    ldx     #MMU_CFG_RAM1
        jsr     FETCH
        sta     (ptr2),y
        iny
        bne     @L1
        inc     ptr1+1
        inc     ptr2+1
        dec     tmp1
        bne     @L1
; Copy the remainder of the page
@L2:    ldy     #EM_COPY::COUNT
        lda     (ptr3),y
        beq     @L4
        sta     tmp1

        ldy     #$00
@L3:    ldx     #MMU_CFG_RAM1
        jsr     FETCH
        sta     (ptr2),y
        iny
        dec     tmp1
        bne     @L3
; Done
@L4:    cli
        rts

;; ------------------------------------------------------------------------
;; COPYTO: Copy from linear into extended memory. A pointer to a structure
;; describing the request is passed in a/x.
;; The function must not return anything.
COPYTO:
        jsr     copy_setup
        cpy #$ff
        beq @L2
; Copy full pages
@L1:    lda     (ptr2),y
        ldx     #MMU_CFG_RAM1
        jsr     STASH
        iny
        bne     @L1
        inc     ptr1+1
        inc     ptr2+1
        dec     tmp1
        bne     @L1
; Copy the remainder of the page
@L2:    ldy     #EM_COPY::COUNT
        lda     (ptr3),y
        beq     @L4
        sta     tmp1

        ldy     #$00
@L3:    lda     (ptr2),y
        ldx     #MMU_CFG_RAM1
        jsr     STASH
        iny
        dec     tmp1
        bne     @L3

; Done

@L4:    cli
        rts


; Super short version that only did 1 page, couldn't use this with v2.1.0 rework
;copysetup:
;        sta     ptr3
;        stx     ptr3+1          ; save pointer to copyinfo struct
;
;        lda     #$00            ; no offset ever
;        sta     ptr1
;        ldy     #EM_COPY::PAGE
;        lda     (ptr3),y
;        clc
;        adc     #>BASE
;        sta     ptr1+1          ; page hi
;
;        ldy     #EM_COPY::BUF
;        lda     (ptr3),y
;        sta     ptr2
;        iny
;        lda     (ptr3),y
;        sta     ptr2+1          ; to/from
;
;        lda     #<ptr1          ; zp pointer address for STASH/FETCH
;        sta     FETVEC          ; just store it in both places
;        sta     STAVEC          ;  even though only one is actually used
;        ldy     #EM_COPY::COUNT ; count of bytes
;        lda     (ptr3),y
;        sta     tmp1
;        ldy     #$00
;        rts
;
;COPYFROM:
;        jsr     copysetup
;        lda     tmp1
;        beq     @L4
;; Copy the single page
;@L3:    ldx     #MMU_CFG_RAM1
;        jsr     FETCH
;        sta     (ptr2),y
;        iny
;        dec     tmp1
;        bne     @L3
;; Done
;@L4:;    cli ; sei only set on multiple pages?
;        rts
;
;COPYTO:
;        jsr     copysetup
;        lda     tmp1
;        beq     @L4
;; Copy the page
;@L3:    lda     (ptr2),y
;        ldx     #MMU_CFG_RAM1
;        jsr     STASH
;        iny
;        dec     tmp1
;        bne     @L3
;; Done
;@L4:;    cli ; sei only set on multiple pages?
;        rts
