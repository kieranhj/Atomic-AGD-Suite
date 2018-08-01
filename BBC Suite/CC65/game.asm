;----------------------------------------------
; BBC AGD Engine
; Z80 conversion by Kees van Oss 2017
; BBC Micro version by Kieran Connell 2018
;----------------------------------------------

.DEFINE asm_code $0e00		; assembly address _BEEB
.DEFINE load_address $1200	; load address _BEEB

;----------------------------------------------------------------------
; BBC MICRO PLATFORM DEFINES
;----------------------------------------------------------------------

; _BEEB MOS calls

	OSBYTE	 = $fff4
	OSFILE	 = $ffdd
	OSWRCH	 = $ffee
	OSASCI	 = $ffe3
	OSWORD	 = $fff1
	OSFIND	 = $ffce
	OSGBPB	 = $ffd1
	OSARGS	 = $ffda

	PAL_black = 0 ^ 7
	PAL_red = 1 ^ 7
	PAL_green = 2 ^ 7
	PAL_yellow = 3 ^ 7
	PAL_blue = 4 ^ 7
	PAL_magenta = 5 ^ 7
	PAL_cyan = 6 ^ 7
	PAL_white = 7 ^ 7

; System constants

	ScreenSize  = $1800	; Startaddress video RAM _BEEB
	ScreenAddr 	= $8000 - ScreenSize	; Screen size bytes _BEEB
	ScreenRowBytes = 256				; 40 columns

	SpriteMaxY	= 177	; used for clipping bottom of screen

; AGD Engine Workspace

	MAP 		= $300				; properties map buffer (3x256 bytes)
	SCADTB_lb	= MAP + $300
	SCADTB_hb	= SCADTB_lb + $100

.if pflag
    SHRAPN 		= $B00 - (NUMSHR * SHRSIZ)	; shrapnel table (55x6 bytes)
.endif

	sprtab		= $B00				; NUMSPR*TABSIZ

	.macro DEBUG_PAL pal
		SET_PAL pal
	.endmacro

	.macro SET_PAL pal
		lda #$00+pal
		sta $fe21
		lda #$10+pal
		sta $fe21
		lda #$20+pal
		sta $fe21
		lda #$30+pal
		sta $fe21
		lda #$40+pal
		sta $fe21
		lda #$50+pal
		sta $fe21
		lda #$60+pal
		sta $fe21
		lda #$70+pal
		sta $fe21
	.endmacro

;----------------------------------------------------------------------
; ZERO PAGE SEGMENT
;----------------------------------------------------------------------

.segment "ZEROPAGE"

.include "z80-zp.inc"
.include "engine-zp.inc"

;----------------------------------------------------------------------
; ZCODE SEGMENT
;----------------------------------------------------------------------

.segment "CODE"
.org asm_code

start_asm:

	jmp relocate + load_address - asm_code

boot_game:

; Zero ZP vars

clear_zp:
	ldx #0
	txa
	:
	sta $00, x
	inx
	cpx #$a0
	bne :-

	; Init non-zero vars
	lda #3
	sta numlif

	ldx #255
	stx varrnd
	stx varopt
	stx varblk
	dex
	stx varobj

	; Call AGD Engine start game
	jsr start_game

    ; Wait for keypress
	ldx #$ff
	ldy #$7f
	lda #$81
	jsr OSBYTE

	; Restart or exit
	jmp boot_game

;----------------------------------------------------------------------
; PLATFORM SPECIFIC ENGINE CODE
;----------------------------------------------------------------------

	.include "z80.asm"
	.include "bbc.inc"

;----------------------------------------------------------------------
; AGD 6502 ENGINE CODE + COMPILED GAME SCRIPT
;----------------------------------------------------------------------

start_game:

	.include "game.inc"

end_asm:

;----------------------------------------------------------------------
; RELOCATION OF BEEB CODE FROM LOAD ADDRESS
;----------------------------------------------------------------------

relocate:
; Issue *TAPE otherwise DFS goes mental that we've overwritten workspace from &E00 - &1100

    lda #$8C
    ldx #$0C
    ldy #$00
    jsr OSBYTE					; *FX &8C,0,0 - *TAPE 1200

; Other one off initialisation could happen here...

; Relocate all code down to &E00
	ldx #>(end_asm - start_asm) + 1
	ldy #0
reloop:
	lda load_address, y
	sta asm_code, y
	iny
	bne reloop
	inc reloop + 2 + load_address - asm_code
	inc reloop + 5 + load_address - asm_code
	dex
	bne reloop
	jmp boot_game
