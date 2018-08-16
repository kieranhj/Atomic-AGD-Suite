;----------------------------------------------------------------------
; BBC Platform Specific functions
;----------------------------------------------------------------------

;--------------------------------------------------------
; Keys
;
; Out: joyval=x65FUDLR (bit cleared if key pressed)
;             ||||||||
;             |||||||+> Right    KEY 0  - X
;             ||||||+-> Left     KEY 1  - Z
;             |||||+--> Down     KEY 2  - .
;             ||||+---> Up       KEY 3  - ;
;             |||+----> Fire1    KEY 4  - SPC
;             ||+-----> Fire2    KEY 5  - Q
;             |+------> Fire3    KEY 6  - P
;             +-------> Not used
;
;                       Option1  KEY 7  - 1
;                       Option2  KEY 8  - 2
;                       Option3  KEY 9  - 3
;                       Option4  KEY 10 - 4
;--------------------------------------------------------

;              X   Z   .   ;  SPC  Q   P
;keys:   .byte $42,$61,$68,$48,$62,$10,$37       ; Keys defined by game designer.
;        .byte $30,$31,$11,$12                   ; menu options.

jkeys:  .byte $42,$61,$68,$48,$49,$10,$37       ; Keys defined by game designer.
        .byte $30,$31,$11,$12                   ; menu options.

;----------------------------------------------------
; Wait for keypress.
;----------------------------------------------------

prskey:
	txa
	pha
prsloop:
	jsr vsync

	lda #$79
	ldx #$01
	jsr OSBYTE

	cpx #$ff
	beq prsloop		; wait until key pressed
	pla
	tax
	rts

bbcinit:
	sei
	lda EVENTV
	sta old_eventv
	lda EVENTV+1
	sta old_eventv+1

	lda #<event_handler
	sta EVENTV
	lda #>event_handler
	sta EVENTV+1
	cli
	
	; Enable VSYNC event.
	lda #14
	ldx #4
	jmp OSBYTE

bbckill:
	sei
	lda old_eventv
	sta EVENTV
	lda old_eventv+1
	sta EVENTV+1
	cli
	
	; Disable VSYNC event.
	lda #13
	ldx #4
	jmp OSBYTE


old_eventv:
	.res 2

event_handler:
	php
	cmp #4
	bne not_vsync

	inc vsync_count

not_vsync:
	plp
	jmp (old_eventv)

bbcsync:
	lda vsync_count
	cmp #2
	bcc bbcsync
	lda #0
	sta vsync_count
	rts

;--------------------------------------------------------
; Keyboard test routine.
;
; Input:
;  A = key to read, high nibble=row and low nibble=col
;
; Output:
;  carry clr = key pressed
;  carry set = key not pressed
;--------------------------------------------------------

ktest:
	sta z80_a		; save key
	tya
	pha

; _BEEB keyboard read
	lda z80_a
	eor #$80		; _BEEB just look for this key
	tax

	lda #$79
	jsr OSBYTE

	txa
	bmi pressed 	; _BEEB X is negative if key is pressed

	sec			; key not pressed
	pla
	tay
	rts

pressed:
	clc			; key pressed
	pla
	tay
	rts

;---------------------------------------------------------------
; Getkey in column,row format
;
; Output:
;  A = high nibble=row and low nibble=column key in matrix
;---------------------------------------------------------------

kget:
	stx xtmp

kget1:
	lda #$79			; _BEEB read keyboard with OSBYTE &79
	ldx #$01
	jsr OSBYTE

	cpx #$ff
	beq kget1

	txa

	ldx xtmp
	rts

;----------------------------------------------------
; AtoMMC joystick controls.
; _BEEB TODO
;----------------------------------------------------

joyinit:
joysin:
	rts


;----------------------------------------------------
; Delay routine 1/50 sec
;
; Wait 1/60 sec = 16666 usec
; Wait 208 x 16 =  3328 usec
;                 19994 usec
; rts           =     6 usec
; Total         = 20000 usec
;----------------------------------------------------

delay:
	sta xtmp
del_loop:
	lda #19
	jsr OSBYTE		; wait for vsync _BEEB

	ldy #208		; wait 208 x 16 = 3328 usec
delay1:
	adc ($80,x)		;	 6 usec
	adc ($80,x)		;	 6 usec
	dey			;	 2 usec
	bne delay1		;	 2 usec
	nop			; 2 usec
	nop			; 2 usec
				; tot: 20000 usec
	dec xtmp
	bne del_loop
	rts

;-------------------------------------------------------------
; Screen synchronisation.
;
;  - read joystick/keyboard
;  - handle sound
;  - sync framerate with clock
;  - handle shrapnel every even frame
;-------------------------------------------------------------

vsync:
	pha
	tya
	pha
	txa
	pha
	jsr joykey		; read joystick/keyboard.
vsync1:
; Don't sync to vsync here - we can sync to every other vsync in main loop
;	lda #19
;	jsr OSBYTE		; _BEEB vsync

	lda clock
	and #1
	bne:+
	jsr proshr		; handle shrapnel every even frame
:
	lda sndtyp
	beq sndskip

; _BEEB TODO SOUND
;sndloop:
;	lda SpeakerBit		; handle sound
;	ldy sndtyp
;sndwait:
;	dey
;	bne sndwait
;	eor #4
;	sta SpeakerBit
;	dec sndtyp
;	bne sndloop

sndskip: 
	pla
	tax
	pla
	tay
	pla
	rts

sndtyp:	.byte 0

;----------------------------------------------------------------------
; BBC video hardware fns
;----------------------------------------------------------------------

screeninit:
	ldx #13
crtcloop:
	stx $FE00
	lda crtc_regs_high,x
	sta $FE01
	dex
	bpl crtcloop

    ; Set ULA
    lda #$D8            ; MODE 4=$88 MODE 1=$D8
    sta $248            ; Tell the OS or it will mess with ULA settings at vsync
    sta $FE20

; fall through to palette

setpal:
	ldx #15
palloop:
	lda ula_pal,x
	sta $fe21
	dex
	bpl palloop
	rts

crtc_regs_high:
	.byte 127				; R0  horizontal total
	.byte 64				; R1  horizontal displayed
	.byte 98				; R2  horizontal position
	.byte $28				; R3  sync width 40 = &28
	.byte 38				; R4  vertical total
	.byte 0					; R5  vertical total adjust
	.byte 24				; R6  vertical displayed
	.byte 35				; R7  vertical position; 35=top of screen
	.byte $00				; R8  interlace
	.byte 7					; R9  scanlines per row
	.byte 32				; R10 cursor start
	.byte 8					; R11 cursor end
	.byte >(ScreenAddr/8)	; R12 screen start address, high
	.byte <(ScreenAddr/8)	; R13 screen start address, low

ula_pal:
	.byte $00 + PAL_black
	.byte $10 + PAL_black
	.byte $20 + PAL_blue
	.byte $30 + PAL_blue
	.byte $40 + PAL_black
	.byte $50 + PAL_black
	.byte $60 + PAL_blue
	.byte $70 + PAL_blue
	.byte $80 + PAL_red
	.byte $90 + PAL_red
	.byte $a0 + PAL_white
	.byte $b0 + PAL_white
	.byte $c0 + PAL_red
	.byte $d0 + PAL_red
	.byte $e0 + PAL_white
	.byte $f0 + PAL_white

;----------------------------------------------------
; Draw sprite
;----------------------------------------------------

sprite:
	stx xtmp		; Save X-reg
	jsr scadd 		; get screen address in scraddr.

	lda dispx 		; x position.
	and #7 			; position straddling cells.
	sta z80_b		; store in b register.

	lda z80_l		; store sprite graphic address.
	sta sprit1+1
	sta sprit2+1
	lda z80_h
	sta sprit1+2
	sta sprit2+2
.if 0
	lda spriteink
;	and #7
	asl a
	tax
	lda ink_to_mode1,x
	sta colour_byte
	lda ink_to_mode1 + 1,x
	sta colour_xor
.endif
	ldx #0				; pixel height.
sprit1:
	lda objdta + data_address - data_start,x		; fetch first byte.
	sta spr
	inx
sprit2:
	lda objdta + data_address - data_start,x
	sta spr+1

	lda #0
	sta spr+2
	jsr sprit7			; shift sprite

	lda spr				; fetch graphic.
	lsr a				; top nibble
	lsr a
	lsr a
	lsr a
	tay
	lda pixels_to_mode1,y
	and colour_byte

	ldy #0				; _BEEB
	eor (scraddr),y		; merge with screen image.
	sta (scraddr),y		; write to screen.

	lda spr				; fetch graphic.
	and #$0f
	tay
	lda pixels_to_mode1,y
	and colour_byte

	ldy #8				; _BEEB
	eor (scraddr),y		; merge with screen image.
	sta (scraddr),y		; write to screen.

	lda spr+1			; fetch graphic.
	lsr a				; top nibble
	lsr a
	lsr a
	lsr a
	tay
	lda pixels_to_mode1,y
	and colour_byte

	ldy #16				; _BEEB
	eor (scraddr),y		; merge with screen image.
	sta (scraddr),y		; write to screen.

	lda spr+1			; fetch graphic.
	and #$0f
	tay
	lda pixels_to_mode1,y
	and colour_byte
	ldy #24				; _BEEB
	eor (scraddr),y		; merge with screen image.
	sta (scraddr),y		; write to screen.

	lda spr+2			; fetch graphic.
	lsr a				; top nibble
	lsr a
	lsr a
	lsr a
	tay
	lda pixels_to_mode1,y
	and colour_byte

	ldy #32				; _BEEB
	eor (scraddr),y		; merge with screen image.
	sta (scraddr),y		; write to screen.

	lda spr+2			; fetch graphic.
	and #$0f
	tay
	lda pixels_to_mode1,y
	and colour_byte
	ldy #40				; _BEEB
	eor (scraddr),y		; merge with screen image.
	sta (scraddr),y		; write to screen.

	jsr nline

	lda colour_byte
	eor colour_xor
	sta colour_byte		; toggle colour mask each line

	inx					; next source byte.
	cpx #32
	bcs :+				; repeat
	jmp sprit1
:
	ldx xtmp			; retreive X-reg
	rts

;------------------------------------------------------------------
; This routine returns a screen address for (dispx, dispy) in scraddr.
;------------------------------------------------------------------

scadd:
	ldx dispy
	
	lda #0
	sta scraddr+1

	lda dispx		; x*2
	and #$f8
	asl a
	rol scraddr+1

	clc
	adc SCADTB_lb,x
	sta scraddr
	lda SCADTB_hb,x
	adc scraddr+1
	sta scraddr+1
	rts

;-----------------------------------------------------------------
; These are the sprite routines.
; sspria = single sprite, old (ix).
; ssprib = single sprite, new (ix+5).
; sspric = both sprites, old (ix) and new (ix+5).
;-----------------------------------------------------------------

sspria:
	jsr gsprad		; get old sprite address.
sspri2:

	lda z80_e				; 3c
	sta dline_spraddr1+1	; 4c
	sta dline_spraddr2+1	; 4c

	lda z80_d				; 3c
	sta dline_spraddr1+2	; 4c
	sta dline_spraddr2+2	; 4c

	ldx sprshft
	lda shift_table,x
	sta dline_shift1+1
	sta dline_shift2+1
	lda shift_table+1,x
	sta dline_shift1+2
	sta dline_shift2+2

.if 0
	lda spriteink
;	and #7
	asl a
	tax
	lda ink_to_mode1,x
	sta colour_byte
	lda ink_to_mode1 + 1,x
	sta colour_xor
.endif

	ldx #0			; vertical lines.
sspri0:
	jsr dline		; draw a line.
	cpx #32
	bne sspri0		; repeat 16 times x 2 bytes
	rts

;-----------------------------------------------------------------

ssprib:
	jsr gspran 		; get new sprite address.
	jmp sspri2

;-----------------------------------------------------------------

sspric:
	jsr gsprad 		; get old sprite address.

	lda z80_e				; 3c
	sta ddline_spraddr3+1	; 4c
	sta ddline_spraddr4+1	; 4c

	lda z80_d				; 3c
	sta ddline_spraddr3+2	; 4c
	sta ddline_spraddr4+2	; 4c

	ldx sprshft
	lda shift_table,x
	sta ddline_shift3+1
	sta ddline_shift4+1
	lda shift_table+1,x
	sta ddline_shift3+2
	sta ddline_shift4+2

.if 0
	lda spriteink
;	and #7
	asl a
	tax
	lda ink_to_mode1,x
	sta colour_byte
	lda ink_to_mode1 + 1,x
	sta colour_xor
.endif

	jsr exx  		; store addresses.
	jsr gspran 		; get new sprite addresses.

	lda z80_e				; 3c
	sta ddline_spraddr1+1	; 4c
	sta ddline_spraddr2+1	; 4c

	lda z80_d				; 3c
	sta ddline_spraddr1+2	; 4c
	sta ddline_spraddr2+2	; 4c

	ldx sprshft
	lda shift_table,x
	sta ddline_shift1+1
	sta ddline_shift2+1
	lda shift_table+1,x
	sta ddline_shift1+2
	sta ddline_shift2+2

	ldx #0
lloop:
	jsr ddline 		; draw a line.
	cpx #32
	bne lloop
	rts

shift_table:
.word shift0
.word shift2
.word shift4
.word shift6

;-------------------------------------------------------------
; Drop through.
; Line drawn, now work out next target address.
;
; Input:
;  B  = right mask
;  C  = left mask
;  DE = spriteaddress
;  scraddr = screen address
;-------------------------------------------------------------

dline:

; first screen byte

dline_spraddr1:
	ldy $ffff,x 		; graphic data.
dline_shift1:
	lda $ffff,y
	sta sprtmp
	and z80_c 			; mask away what's not needed.

	lsr a				; top nibble
	lsr a
	lsr a
	lsr a
	tay
	lda pixels_to_mode1,y
	and colour_byte

	ldy #0
	eor (scraddr),y 	; XOR with what's there.
	sta (scraddr),y 	; bung it in.

	lda sprtmp			; fetch data.
	and z80_c 			; mask away what's not needed.
	and #$f				; bottom nibble
	tay
	lda pixels_to_mode1,y
	and colour_byte
	ldy #8
	eor (scraddr),y 	; XOR with what's there.
	sta (scraddr),y 	; bung it in.

; carry to next screen byte

	lda sprtmp			; fetch data.
	and z80_b 			; mask away unwanted
	sta sprtmp

; middle screen byte

	inx
dline_spraddr2:
	ldy $ffff,x 		; second bit of data.
dline_shift2:
	lda $ffff,y
	sta sprtmp2
	and z80_c 			; mask away what's not needed.
	ora sprtmp
	sta sprtmp

	lsr a				; top nibble
	lsr a
	lsr a
	lsr a
	tay
	lda pixels_to_mode1,y
	and colour_byte

	ldy #16
	eor (scraddr),y 	; XOR with what's there.
	sta (scraddr),y 	; bung it in.

	lda sprtmp			; fetch data.
	and #$f				; bottom nibble
	tay
	lda pixels_to_mode1,y
	and colour_byte
	ldy #24
	eor (scraddr),y 	; XOR with what's there.
	sta (scraddr),y 	; bung it in.

; carry to last screen byte

	lda sprtmp2
	and z80_b 			; mask away unwanted

	lsr a				; top nibble
	lsr a
	lsr a
	lsr a
	tay
	lda pixels_to_mode1,y
	and colour_byte

	ldy #32
	eor (scraddr),y 	; XOR with what's there.
	sta (scraddr),y 	; bung it in.

	lda sprtmp2
	and z80_b 			; mask away unwanted
	and #$f				; bottom nibble
	tay
	lda pixels_to_mode1,y
	and colour_byte
	ldy #40
	eor (scraddr),y 	; XOR with what's there.
	sta (scraddr),y 	; bung it in.

	lda colour_byte
	eor colour_xor
	sta colour_byte

	inx

;----------------------------------------------------------------------
; Line drawn, now work out next target address.
;----------------------------------------------------------------------

; _BEEB screen arrangement

nline:
	lda scraddr 		; get low byte of address.
	and #7				
	cmp #7				; is this last line of row?
	beq beeb_next_row

	; within same row
	inc scraddr			; new low byte of address.
;	bne :+
;	inc scraddr+1		; new high byte of address.
:
	rts

beeb_next_row:
	clc
	lda scraddr
	adc #<(ScreenRowBytes-7)
	sta scraddr			; new low byte of address.
	lda scraddr+1
	adc #>(ScreenRowBytes-7)
	sta scraddr+1		; new high byte of address.
	rts

;-------------------------------------------------------------
; Drop through.
; Line drawn, now work out next target address.
;
; Input:
;  B  = right mask
;  C  = left mask
;  DE = spriteaddress
;  scraddr = screen address
;-------------------------------------------------------------

ddline:

; NEW SPRITE

; first screen byte

ddline_spraddr1:
	ldy $ffff,x 		; graphic data.
ddline_shift1:
	lda $ffff,y
	sta sprtmp
	and z80_c 			; mask away what's not needed.

	lsr a				; top nibble
	lsr a
	lsr a
	lsr a
	tay
	lda pixels_to_mode1,y
	and colour_byte

	ldy #0
	eor (scraddr),y 	; XOR with what's there.
	sta (scraddr),y 	; bung it in.

	lda sprtmp
	and z80_c 			; mask away what's not needed.
	and #$f				; bottom nibble
	tay
	lda pixels_to_mode1,y
	and colour_byte
	ldy #8
	eor (scraddr),y 	; XOR with what's there.
	sta (scraddr),y 	; bung it in.

; carry to next screen byte

	lda sprtmp			; fetch data.
	and z80_b 			; mask away unwanted
	sta sprtmp

; middle screen byte

	inx
ddline_spraddr2:
	ldy $ffff,x 		; second bit of data.
ddline_shift2:
	lda $ffff,y
	sta sprtmp2
	and z80_c 			; mask away what's not needed.
	ora sprtmp
	sta sprtmp

	lsr a				; top nibble
	lsr a
	lsr a
	lsr a
	tay
	lda pixels_to_mode1,y
	and colour_byte

	ldy #16
	eor (scraddr),y 	; XOR with what's there.
	sta (scraddr),y 	; bung it in.

	lda sprtmp
	and #$f				; bottom nibble
	tay
	lda pixels_to_mode1,y
	and colour_byte
	ldy #24
	eor (scraddr),y 	; XOR with what's there.
	sta (scraddr),y 	; bung it in.

; carry to last screen byte

	lda sprtmp2
	and z80_b 			; mask away unwanted

	lsr a				; top nibble
	lsr a
	lsr a
	lsr a
	tay
	lda pixels_to_mode1,y
	and colour_byte

	ldy #32
	eor (scraddr),y 	; XOR with what's there.
	sta (scraddr),y 	; bung it in.

	lda sprtmp2
	and z80_b 			; mask away unwanted
	and #$f				; bottom nibble
	tay
	lda pixels_to_mode1,y
	and colour_byte
	ldy #40
	eor (scraddr),y 	; XOR with what's there.
	sta (scraddr),y 	; bung it in.

; _BEEB next row

	lda scraddr 		; get low byte of address.
	and #7				
	cmp #7				; is this last line of row?
	beq :+

	; within same row can't increment page
	inc scraddr			; new low byte of address.
	bne ddline2

:
	clc
	lda scraddr
	adc #<(ScreenRowBytes-7)
	sta scraddr			; new low byte of address.
	lda scraddr+1
	adc #>(ScreenRowBytes-7)
	sta scraddr+1		; new high byte of address.

; OLD SPRITE

ddline2:

; still first row but different data!

	dex

; first screen byte

ddline_spraddr3:
	ldy $ffff,x			; graphic data.
ddline_shift3:
	lda $ffff,y
	sta sprtmp
	and z80_cp 			; mask away what's not needed.

	lsr a				; top nibble
	lsr a
	lsr a
	lsr a
	tay
	lda pixels_to_mode1,y
	and colour_byte

	ldy #0
	eor (z80_hlp),y 	; XOR with what's there.
	sta (z80_hlp),y 	; bung it in.

	lda sprtmp
	and z80_cp 			; mask away what's not needed.
	and #$f				; bottom nibble
	tay
	lda pixels_to_mode1,y
	and colour_byte
	ldy #8
	eor (z80_hlp),y 	; XOR with what's there.
	sta (z80_hlp),y 	; bung it in.

; carry to next screen byte

	lda sprtmp			; fetch data.
	and z80_bp 			; mask away unwanted
	sta sprtmp

; middle screen byte

	inx
ddline_spraddr4:
	ldy $ffff,x 		; second bit of data.
ddline_shift4:
	lda $ffff,y
	sta sprtmp2
	and z80_cp 			; mask away what's not needed.
	ora sprtmp
	sta sprtmp

	lsr a				; top nibble
	lsr a
	lsr a
	lsr a
	tay
	lda pixels_to_mode1,y
	and colour_byte

	ldy #16
	eor (z80_hlp),y 	; XOR with what's there.
	sta (z80_hlp),y 	; bung it in.

	lda sprtmp
	and #$f				; bottom nibble
	tay
	lda pixels_to_mode1,y
	and colour_byte
	ldy #24
	eor (z80_hlp),y 	; XOR with what's there.
	sta (z80_hlp),y 	; bung it in.

; last screen byte

	lda sprtmp2			; fetch data.
	and z80_bp 			; mask away unwanted

	lsr a				; top nibble
	lsr a
	lsr a
	lsr a
	tay
	lda pixels_to_mode1,y
	and colour_byte

	ldy #32
	eor (z80_hlp),y 	; XOR with what's there.
	sta (z80_hlp),y 	; bung it in.

	lda sprtmp2			; fetch data.
	and z80_bp 			; mask away unwanted
	and #$f				; bottom nibble
	tay
	lda pixels_to_mode1,y
	and colour_byte
	ldy #40
	eor (z80_hlp),y 	; XOR with what's there.
	sta (z80_hlp),y 	; bung it in.

; next row of sprite data

	inx

	lda colour_byte
	eor colour_xor
	sta colour_byte

; _BEEB screen arrangement

	lda z80_lp 			; get low byte of address.
	and #7				
	cmp #7				; is this last line of row?
	beq :+

	; within same row can't increment page
	inc z80_lp			; new low byte of address.
	rts

:
	clc
	lda z80_lp
	adc #<(ScreenRowBytes-7)
	sta z80_lp			; new low byte of address.
	lda z80_lp+1
	adc #>(ScreenRowBytes-7)
	sta z80_lp+1		; new high byte of address.
	rts

;----------------------------------------------------
; Display character in A at dispx,dispy.
;
; Input:
;  A 	   = character to print
;----------------------------------------------------

pchar:
	sta fntaddr
	lda #0
	sta fntaddr+1
	asl fntaddr  		; multiply char by 8.
	rol fntaddr+1
	asl fntaddr
	rol fntaddr+1
	asl fntaddr
	rol fntaddr+1

	lda fntaddr
	clc
	adc FontPtr
	sta fntaddr		; that's the low byte.
	lda fntaddr+1
	adc FontPtr+1
	sta fntaddr+1		; add displacement.
pchark:
	jsr gprad		; get screen address.
	ldy #0
	ldx #0

pchar0:
	txa
	tay
	lda (fntaddr),y 	; get image byte.
	eor #TxtInvert		; Invert
	sta tmp

	lsr a
	lsr a
	lsr a
	lsr a
	tay
	lda pixels_to_mode1,y
	ldy #0
	sta (scraddr),y 	; copy to screen.

	lda tmp
	and #$0f
	tay
	lda pixels_to_mode1,y
	ldy #8
	sta (scraddr),y 	; copy to screen.

	inc scraddr

	inx					; next screen row down.
	cpx #8				; number lines
	bcc pchar0			; repeat.
	rts

;----------------------------------------------------
; Get print address.
;----------------------------------------------------

gprad:
	tya
	pha

	lda dispx 		; x coordinate.
	sta scraddr
	lda #0
	sta scraddr+1
	asl scraddr  	; multiply char by 16
	rol scraddr+1
	asl scraddr
	rol scraddr+1
	asl scraddr
	rol scraddr+1
	asl scraddr
	rol scraddr+1

	lda dispy		; y coordinate.
	asl a
	asl a
	asl a			; multiply char by 8
	tay

	clc
	lda scraddr
	adc SCADTB_lb,y
	sta scraddr
	lda scraddr+1
	adc SCADTB_hb,y
	sta scraddr+1

	pla
	tay
	rts

;--------------------------------------------------------------
; Get property buffer address of char at (dispx, dispy) in hl.
; 
; Output:
;  bufaddr = MAP + dispy*32 + dispx
;--------------------------------------------------------------

pradd:
	lda dispy 		; y coordinate.
	sta bufaddr
	lda #0
	sta bufaddr+1
	asl bufaddr  		; multiply char by 32
	rol bufaddr+1
	asl bufaddr
	rol bufaddr+1
	asl bufaddr
	rol bufaddr+1
	asl bufaddr
	rol bufaddr+1
	asl bufaddr
	rol bufaddr+1
	clc			; add address of MAP graphics.
	lda bufaddr
	adc dispx
	adc #<MAP
	sta bufaddr
	lda bufaddr+1
	adc #>MAP
	sta bufaddr+1
	rts

;----------------------------------------------
; Print attributes, properties and pixels.
;
; Input:
;  A	= tile number
;----------------------------------------------

pattr:
;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.if aflag
	pha
	jsr wbloc		; save blockinfo
	pla
.endif
;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

pattr2:
	sta z80_b		; store cell in b register for now.
	tax
	lda bprop + data_address - data_start,x 		; block properties.
	sta z80_c
	cmp #COLECT
	bne :+
	lda z80_b
	sta colpatt
:
	jsr pradd 		; get property buffer address.
	lda z80_c
	ldy #0
	sta (bufaddr),y 	; write property.
	lda z80_b 		; restore cell.

; Print attributes, no properties.

panp:
	sta z80_e		; displacement in e.
	lda #0
	sta z80_d		; no high byte.
	asl z80_e  		; multiply char by 16.
	rol z80_d
	asl z80_e
	rol z80_d
	asl z80_e
	rol z80_d
	asl z80_e
	rol z80_d
	clc
	lda z80_e
	adc #<(chgfx + data_address - data_start) 		; address of graphics.
	sta tileaddr
	lda z80_d
	adc #>(chgfx + data_address - data_start)
	sta tileaddr+1
	
	jsr gprad 		; get screen address.
	ldx #0			; number of pixel rows to write.

panp0:
	txa
	tay
	lda (tileaddr),y 	; get image byte.
	eor #TxtInvert		; Invert
	ldy #0
	sta (scraddr),y 	; copy to screen.

	inx
	txa
	tay
	lda (tileaddr),y 	; get image byte.
	eor #TxtInvert		; Invert
	ldy #8
	sta (scraddr),y 	; copy to screen.

	inc scraddr			; within char
	inx	 		; repeat for 8 pixel rows.
	cpx #16
	bcc panp0
	inc dispx 		; move along one.
	inc charx
	rts

;----------------------------------------------------
; MODE 1 Colour Lookups
;----------------------------------------------------

ink_to_mode1:
	.byte $00, $00 ^ $00 ; solid 0
	.byte $0f, $0f ^ $0f ; solid 1
	.byte $f0, $f0 ^ $f0 ; solid 2
	.byte $5a, $a5 ^ $5a ; stipple 1,2
	.byte $da, $a7 ^ $da ; stripple 1,2,3
	.byte $5f, $af ^ $5f ; stipple 1,3
	.byte $f5, $fa ^ $f5 ; stipple 2,3
	.byte $ff, $ff ^ $ff ; solid 3

pixels_to_mode1:
	.byte $00,$11,$22,$33,$44,$55,$66,$77
	.byte $88,$99,$aa,$bb,$cc,$dd,$ee,$ff

;----------------------------------------------------
; Shift tables
;----------------------------------------------------

.if 1
.align 256
shift0:
.repeat 256, i
	.byte i
.endrep

shift2:
.repeat 256, i
	.byte (i >> 2) | (i << 6) & $c0
.endrep

shift4:
.repeat 256, i
	.byte (i >> 4) | ((i << 4) & $f0)
.endrep

shift6:
.repeat 256, i
	.byte (i >> 6) | ((i << 2) & $fc)
.endrep
.align 1
.endif
