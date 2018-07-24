;----------------------------------------------
; Common AGD engine
; Z80 conversion by Kees van Oss 2017
; BBC Micro version by Kieran Connell 2018
;----------------------------------------------
	.DEFINE asm_code $0e00	; assembly address _BEEB
	.DEFINE load_address $1100	; load address _BEEB
	.DEFINE header   0		; Header Wouter Ras emulator
	.DEFINE filenaam "AGD"

.org asm_code-22*header

.IF header
;********************************************************************
; ATM Header for Atom emulator Wouter Ras

name_start:
	.byte filenaam			; Filename
name_end:
	.repeat 16-name_end+name_start	; Fill with 0 till 16 chars
	  .byte $0
	.endrep

	.word asm_code			; 2 bytes startaddress
	.word exec			; 2 bytes linkaddress
	.word eind_asm-start_asm	; 2 bytes filelength

;********************************************************************
.ENDIF

start_asm:
	.include "z80-zp.inc"

.segment "CODE"

	jmp relocate + load_address - asm_code

exec_game:
	.include "game.inc"
	.include "z80.asm"

eind_asm:

relocate:
; Issue *TAPE otherwise DFS goes mental that we've overwritten workspace from &E00 - &1100

    lda #$8C
    ldx #$0C
    ldy #$00
    jsr OSBYTE					; *FX &8C,0,0 - *TAPE 1200

; Relocate all code down to &E00
	ldx #>(eind_asm - start_asm) + 1
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
	jmp exec_game
