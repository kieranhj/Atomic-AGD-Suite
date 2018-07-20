;----------------------------------------------
; Common AGD engine
; Z80 conversion by Kees van Oss 2017
; BBC Micro version by Kieran Connell 2018
;----------------------------------------------
	.DEFINE asm_code $1100	; assembly address _BEEB
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

exec:
start_asm:
	.include "z80-zp.inc"
	.include "game.inc"
	.include "z80.asm"
eind_asm:
