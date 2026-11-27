; Game engine code --------------------------------------------------------------

; Arcade Game Designer.
; (C) 2008 Jonathan Cauldwell.
; ZX Spectrum Next Engine v0.1.

; Global definitions ------------------------------------------------------------

	FONT = font + data_address - data_start			; Font address

; Block characteristics.

	PLATFM = 1			; platform.
	WALL = PLATFM + 1	; solid wall.
	LADDER = WALL + 1	; ladder.
	FODDER = LADDER + 1	; fodder block.
	DEADLY = FODDER + 1	; deadly block.
	CUSTOM = DEADLY + 1	; custom block.
	WATER  = CUSTOM + 1	; water block.
    COLECT = WATER + 1      ; collectable block.
    NUMTYP = COLECT + 1     ; number of types.

; Sprites.

	NUMSPR = 12			; number of sprites.
	TABSIZ = 19			; size of each entry.
	SPRBUF = NUMSPR * TABSIZ	; size of entire table.
	NMESIZ = 4			; bytes stored in nmetab for each sprite.

; Sprite table variable offsets.

	var_Type = 0		; sprite type
	var_Image = 1		; sprite time number
	var_Frame = 2		; sprite frame
	var_Y = 3			; sprite y coordinate
	var_X = 4			; sprite X coordinate

	var_newType = 5		; sprite new type
	var_newImage = 6	; sprite new image number
	var_newFrame = 7	; sprite new frame
	var_newY = 8		; sprite new y coordinate
	var_newX = 9		; sprite new x coordinate
	
	var_Direction = 10	; sprite direction
	var_Param1 = 11		; sprite parameter 1
	var_Param2 = 12		; sprite parameter 2

	var_jumpLo = 13		; sprite jump ptr low
	var_jumpHi = 14		; sprite jump ptr high
	var_dataLo = 15		; sprite data ptr low
	var_dataHi = 16		; sprite data ptr high

	var_Ink = 17
	var_newInk = 18

; Particle engine.

	NUMSHR = 55			; pieces of shrapnel.
	SHRSIZ = 6			; bytes per particle.

; Conditional compilation flags
; Flags are set in commandline assembly

;	mflag = 0 		; MENU + INV
;	pflag = 0		; Particle engine
;	sflag = 0		; scrollytext
;	fflag = 1		; Fontflag

.if iflag
	TxtInvert   = $ff	; Invert byte for character printing
	ScrFillByte = $ff	; Screen fill byte for CLS
.else
	TxtInvert   = $00	; Invert byte for character printing
	ScrFillByte = $00	; Screen fill byte for CLS
.endif

	ASCII_NEWLINE = 13
	ASCII_CR = 10

;===============================================================
; Game starts here
;===============================================================

;--------------------------------------------------------------
; If a font is required...
;--------------------------------------------------------------

	lda #<(FONT-256)	; address of font.
	sta FontPtr
	lda #>(FONT-256)
	sta FontPtr+1

	jsr game	 		; start the game.
	rts

; Don't change the order of these four.  
; Menu routine relies on winlft following wintop.

wintop:	.byte WINDOWTOP		; top of window.
winlft:	.byte WINDOWLFT		; left edge.
winhgt:	.byte WINDOWHGT		; window height.
winwid:	.byte WINDOWWID		; window width.
numob:	.byte NUMOBJ		; number of objects in game.

; Pixel versions of wintop, winlft, winhgt, winwid.

wntopx:	.byte (8 * WINDOWTOP)
wnlftx:	.byte (8 * WINDOWLFT)
wnbotx:	.byte ((WINDOWTOP * 8) + (WINDOWHGT * 8) - 16)
wnrgtx:	.byte ((WINDOWLFT * 8) + (WINDOWWID * 8) - 16)-2

; Make sure pointers are arranged in the same order as the data itself.

frmptr:	.word frmlst + data_address - data_start         ; sprite frames.

; Assorted game routines which can go in contended memory.

;--------------------------------------------------------------
; Modify for inventory.
; called by the INV command
;
; Input:
;  X   = message nr with objects seperated with ,
;
; Output:
;  OPT = selected line nr of INV menu
;--------------------------------------------------------------

.if mflag
minve:
	lda #<(invdis)		; routine address.
	sta mod0+1		; set up menu routine.
	sta mod2+1		; set up count routine.
	lda #>(invdis)
	sta mod0+2
	sta mod2+2
	lda #<(fopt)		; find option from available objects.
	sta mod1+1		; set up routine.
	lda #>(fopt)
	sta mod1+1+1
	jmp dbox		; do menu routine.

;--------------------------------------------------------------
; Modify for menu.
; called by the MENU command
;
; Input:
;  X   = message nr with menu items seperated with ,
;
; Output:
;  OPT = selected line nr of MENU menu
;--------------------------------------------------------------

mmenu:
	lda #<(always)		; routine address.
	sta mod0+1		; set up routine.
	sta mod2+1		; set up count routine.
	lda #>(always)
	sta mod0+2
	sta mod2+2

	lda #<(fstd)		; standard option selection.
	sta mod1+1		; set up routine.
	lda #>(fstd)
	sta mod1+2

; Drop through into box routine.

;--------------------------------------------------------------
; Work out size of box for message or menu.
;--------------------------------------------------------------

dbox:
	lda #<(msgdat + data_address - data_start)		; pointer to messages.
	sta z80_l
	lda #>(msgdat + data_address - data_start)
	sta z80_h
	
	jsr getwrd		; get message number.

	lda z80_h		; store pointer to message.
	sta TmpAddr
	lda z80_l
	sta TmpAddr+1

	lda #1			; height.
	sta z80_d
	lda #0			; start at object zero.
	sta combyt		; store number of object in combyt.
	sta z80_e		; maximum width.
dbox5:
	lda #0			; this line"s width.
	sta z80_b
mod2:
	jsr always		; item in player"s possession?
	cmp #255
	bne dbox6		; not in inventory, skip this line.
	inc z80_d		; add to tally.
dbox6:
	ldy #0			; get character.
	lda (z80_hl),y
	sta z80_a
	inc z80_l		; next character.
	bne :+
	inc z80_h
:
	lda z80_a		; reached end of line?
	cmp #','
	beq dbox3		; yes.
	cmp #ASCII_NEWLINE
	beq dbox3		; yes.
	cmp #ASCII_CR
	beq dbox3		; yes.
	inc z80_b		; add to this line"s width.
	lda z80_a
	bmi dbox4		; end of message? yes, end count.
	jmp dbox6		; repeat until we find the end.
dbox3:
	lda z80_e		; maximum line width.
	cmp z80_b		; have we exceeded longest so far?
	bpl dbox5		; no, carry on looking.
	lda z80_b		; make this the widest so far.
	sta z80_e
	jmp dbox5		; keep looking.
dbox4:
	lda z80_e		; maximum line width.
	cmp z80_b		; have we exceeded longest so far?
	bpl dbox8		; no, carry on looking.
	lda z80_b		; final line is the longest so far.
	sta z80_e
dbox8:
	dec z80_d		; decrement items found.
	bne :+			; total was zero.
	jmp dbox15		
:
	lda z80_e		; longest line.
	bne :+			; was it zero?
	jmp dbox15		; total was zero.
:
	sta bwid		; set up size.
	lda z80_d
	sta blen

;--------------------------------------------------------------
; That's set up our box size.
;--------------------------------------------------------------

	lda winhgt		; window height in characters.
	sec
	sbc z80_d		; subtract height of box.
	lsr a			; divide by 2.
	clc
	adc wintop		; add top edge of window.
	sta btop		; set up box top.

	lda winwid		; window width in characters.
	sec	
	sbc z80_e		; subtract box width.
	lsr a			; divide by 2.
	clc
	adc winlft		; add left edge of window.
	sta blft		; box left.

	lda FontPtr		; font.
	sta grbase		; set up for text display.
	lda FontPtr+1
	sta grbase+1

	lda TmpAddr+1		; restore message pointer.
	sta z80_l
	lda TmpAddr
	sta z80_h

	lda btop		; box top.
	sta dispy		; set display coordinate.
	lda #0			; start at object zero.
	sta combyt		; store number of object in combyt.
dbox2:
	lda combyt		; get object number.
	sta z80_a
mod0:
	jsr always		; check inventory for display.
	cmp #255
	beq :+			
	jmp dbox13		; not in inventory, skip this line.
:
	lda blft		; box left.
	sta dispx		; set left display position.
	lda bwid		; box width.
	sta z80_b		; store width.
dbox0:
	ldy #0			; get character.
	lda (z80_hl),y
	cmp #','		; end of line?
	beq dbox1		; yes, next one.
	cmp #ASCII_NEWLINE			; end of line?
	beq dbox1		; yes, next one.
	cmp #ASCII_CR
	beq dbox1		; yes, next one.

	dec z80_b		; one less to display.
	and #127		; remove terminator.

	jsr pchr		; display on screen.

	ldy #0
	lda (z80_hl),y		; get character.
	sta z80_a
	inc z80_l		; next character.
	bne :+
	inc z80_h
:
	lda z80_a
	cmp #128		; end of message?
	bmi :+
	jmp dbox7		; yes, job done.
:
	lda z80_b		; chars remaining.
	beq :+			; are any left?
	jmp dbox0		; yes, continue.
:
;---------------------------------------------------
; Reached limit of characters per line.
;---------------------------------------------------

dbox9:
	ldy #0
	lda (z80_hl),y		; get character.
	inc z80_l		; next one.
	bne :+
	inc z80_h
:
	cmp #','		; another line?
	beq dbox10		; yes, do next line.
	cmp #ASCII_NEWLINE			; another line?
	beq dbox10		; yes, do next line.
	cmp #ASCII_CR			; another line?
	beq dbox10		; yes, do next line.
	cmp #128		; end of message?
	bcs :+
	jmp dbox11		; yes, finish message.
:
	jmp dbox9

;---------------------------------------------------
; Fill box to end of line.
;---------------------------------------------------

dboxf:
	lda #32			; space character.
	jsr pchr		; display character.
	dec z80_b
	beq :+
	jmp dboxf		; repeat for remaining chars on line.
:
	rts
dbox1:
	inc z80_l		; skip character.
	bne :+
	inc z80_h
:
	jsr dboxf		; fill box out to right side.
dbox10:
	inc dispy		; y coordinate down a line next position.
	jmp dbox2		; next line.
dbox7:
	lda z80_b		; chars remaining.
	bne :+			; are any left?
	jmp dbox11		; no, nothing to draw.
:
	jsr dboxf		; fill message to line.

;------------------------------------------------------
; Drawn the box menu, now select option.
;------------------------------------------------------

dbox11:
	lda btop		; box top.
	sta dispy		; set bar position.
dbox14:
	jsr joykey		; get controls.
	cmp #$7f		; anything pressed?
	bne dbox14		; yes, debounce it.
	jsr dbar		; draw bar.
dbox12:
	jsr joykey		; get controls.
	cmp #$7f		; anything pressed?
	beq dbox12		; no, nothing.
	and #16			; fire button pressed?
	bne :+
mod1:
	jmp fstd		; yes, job done.
:
	jsr dbar		; delete bar.

	lda joyval		; joystick reading.
	and #8			; going up?
	beq dboxu		; yes, go up.

	ldx dispy		; vertical position of bar.
	inx			; look down.
	txa
	sec
	sbc btop		; find distance from top.
	cmp blen		; top of box.
	bne :+
	jmp dbox14		; yes, go no further.
:
	inc dispy		; move bar.
	jmp dbox14		; continue.
dboxu:
	lda dispy		; vertical position of bar.
	cmp btop		; are we at the top?
	bne :+
	jmp dbox14		; yes, go no further.
:
	dec dispy		; move bar.
	jmp dbox14		; continue.
fstd:
	lda dispy		; bar position.
	sec
	sbc btop		; find selected option.
	sta varopt		; store the option.
	jmp redraw		; redraw the screen.

;------------------------------------------------------
; Option not available.  Skip this line.
;------------------------------------------------------

dbox13:
	ldy #0
	lda (z80_hl),y		; get character.
	inc z80_l		; next one.
	bne :+
	inc z80_h
:
	cmp #','		; another line?
	bne :+
	jmp dbox2		; yes, do next line.
:
	cmp #ASCII_NEWLINE			; another line?
	bne :+
	cmp #ASCII_CR			; another line?
	bne :+
	jmp dbox2		; yes, do next line.
:

	bpl :+			; end of message?
	jmp dbox11		; yes, finish message.
:
	jmp dbox13
dbox15:
	lda TmpAddr		; pop message pointer from the stack.
	sta z80_h
	lda TmpAddr+1
	sta z80_l
	rts

;------------------------------------------------------
; Invert bar
;------------------------------------------------------

dbar:
	lda blft		; box left.
	sta dispx		; set display coordinate.
	jsr gprad		; get printing address.

	lda bwid		; box width.
	sta z80_c		; loop counter in c.
	lda z80_h		
	sta z80_d		; store screen address high byte.
dbar1:
	ldy #7			; pixel height in b.
dbar0:
	lda (scraddr),y		; get screen byte.
	eor #255		; reverse all bits.
	sta (scraddr),y		; write back to screen.
	dey			; next line down.
	bpl dbar0		; draw rest of character.

	clc
	lda scraddr		; one char right.
	adc #8
	sta scraddr
	bcc :+
	inc scraddr+1
:

	dec z80_c		; decrement character counter.
	beq :+
	jmp dbar1		; repeat for whole line.
:
	rts

;------------------------------------------------------
; Point to object
;
; Input:
;  -
;
; Output:
;  A = object number, A=255 if already in possession
;------------------------------------------------------

invdis:
	lda z80_l		; store message text pointer.
	pha
	lda z80_h
	pha
	lda combyt		; object number.
	inc combyt		; ready for next one.
	jsr gotob		; check if we have object.
	tay
	pla
	sta z80_h
	pla
	sta z80_l
	tya
	rts

;------------------------------------------------------
; Find option selected.
;
; Input:
;  -
;
; Output:
;  OPT = selected object
;------------------------------------------------------

fopt:
	lda dispy
	sec
	sbc btop		; find selected option.
	sta tmp+2		; option selected in b register.
	inc tmp+2

	lda #0			; set to first item.
	sta combyt		; object number.
fopt0:
	jsr fobj		; find next object in inventory.
	dec tmp+2
	bne fopt0		; repeat for relevant steps down the list.

	lda combyt		; get option.
	sta varopt		; store the option.
	dec varopt		; one less, due to where we increment combyt.
	jmp redraw		; redraw the screen.
fobj:
	ldy combyt		; object number.
	inc combyt		; ready for next item.
	tya
	jsr gotob		; do we have this item?
	cmp #255
	bne :+
	rts
:
	jmp fobj		; yes, it's on the list.
.endif

;----------------------------------------------------
; Clear sprite table.
;
; sprtab[0] - sprtab[SPRBUF-1] = 255
;----------------------------------------------------

xspr:
	lda #255		; clear byte.
	ldx #0			; length of table.
xspr0:
	sta sprtab,x		; sprite table.
	inx			; move to next byte.
	cpx #SPRBUF
	bne xspr0		; repeat for rest of table.
	rts

;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
; Sound, NOT IMPLEMENTED!!!
;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
;
;silenc:
;	jsr silen1 		; silence channel 1.
;	jsr silen2 		; silence channel 2.
;	jsr silen3 		; silence channel 3.
;	jmp plsnd 		; play all channels to switch them off.
;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

;-------------------------------------------------------------
; Initialise all objects.
;
; Reset current room,y,x to start room,y,x for all objects
;-------------------------------------------------------------

iniob:
	lda #<(objdta + data_address - data_start) 		; objects table.
	sta z80_x
	lda #>(objdta + data_address - data_start)
	sta z80_i

	ldx numob 		; number of objects in the game.
iniob0:
	ldy #35
	lda (z80_ix),y 		; start screen.
	ldy #32
	sta (z80_ix),y 		; set start screen.

	ldy #36
	lda (z80_ix),y 		; find start y.
	ldy #33
	sta (z80_ix),y 		; set start y.

	ldy #37
	lda (z80_ix),y 		; get initial x.
	ldy #34
	sta (z80_ix),y 		; set x coord.

	clc 			; point to next object.
	lda z80_x
	adc #38			; distance between objects.
	sta z80_x
	bcc :+
	inc z80_i
:
	dex 			; repeat.
	bne iniob0

	rts

;-----------------------------------------------
; Redraw the screen.
;
; Remove old copy of all sprites for redraw.
;-----------------------------------------------

redraw:
	lda z80_i 		; place sprite pointer on stack.
	pha
	lda z80_x
	pha

	jsr droom		; show screen layout.
	jsr shwob		; draw objects.
numsp0:
	lda #NUMSPR		; sprites to draw.
	sta tmp

	lda #<sprtab		; sprite table.
	sta z80_x
	lda #>sprtab
	sta z80_i
redrw0:
	ldy #0
	lda (z80_ix),y		; old sprite type.
	cmp #255		; is it enabled?
	beq redrw1 		; no, find next one.

	ldy #var_Y
	lda (z80_ix),y 		; sprite y.
	cmp #177		; beyond maximum?
	bcs redrw1		; yes, nothing to draw.

	jsr sspria		; show single sprite.

redrw1:
	clc			; next sprite.
	lda z80_x
	adc #TABSIZ		; distance to next odd/even entry.
	sta z80_x
	bcc :+
	inc z80_i
:
	dec tmp			; repeat for remaining sprites.
	bne redrw0

rpblc1:
;	jsr dshrp		; redraw shrapnel.

	pla			; retrieve sprite pointer.
	sta z80_x
	pla
	sta z80_i

	rts

;----------------------------------------------------------------------
; Clear screen routine.
;
; Fill screenmem $8000-$97ff with ScrFillByte
;----------------------------------------------------------------------

cls:
	lda #>ScreenAddr		; screen address.
	sta clsloop+2
	lda #ScrFillByte
	ldy #0
clsloop:
	sta ScreenAddr,y
	iny
	bne clsloop
	inc clsloop+2
	ldx clsloop+2
	cpx #>(ScreenAddr+ScreenSize)		; _BEEB
	bne clsloop
	rts

;----------------------------------------------------------------------
; FODDER check
;----------------------------------------------------------------------

fdchk:
	cmp #FODDER 		; is it fodder?
	beq :+
	rts 			; no.
:
	lda #0			; wipe fodder in MAP
	ldy #0
	sta (bufaddr),y 	; rewrite block type.

	lda dispx		; x=x/8
	pha
	lsr a
	lsr a
	lsr a
	sta dispx

	lda dispy		; y=y/8
	pha
	lsr a
	lsr a
	lsr a
	sta dispy

	lda #0 			; block to write.
	jsr pattr 		; write block.

	pla
	sta dispy
	pla
	sta dispx
	rts

;----------------------------------------------------
; Scrolly text and puzzle variables.
;----------------------------------------------------

.if sflag
txtbit:	.byte 128		; bit to write.
txtwid:	.byte 16		; width of ticker message.
txtpos:	.word (msgdat + data_address - data_start)
txtini:	.word (msgdat + data_address - data_start)
txtscr:	.word ScreenAddr
.endif

;----------------------------------------------------
; Specialist routines.
; Process shrapnel.
;----------------------------------------------------
proshr:
.if pflag
	lda #<SHRAPN		; table.
	sta z80_x
	lda #>SHRAPN
	sta z80_i

	lda #NUMSHR		; shrapnel pieces to process.
	sta shrctr
prosh0:
	ldy #0
	lda (z80_ix),y		; on/off marker.
	asl a
proshx:
	bcs :+
	jsr prosh1 		; on, so process it.
:
	clc
	lda z80_x
	adc #SHRSIZ
	sta z80_x
	bcc :+
	inc z80_i
:
	dec shrctr		; round again.
	bne prosh0
.endif
.if sflag
	jsr scrly
.endif
	rts

.if pflag
;----------------------------------------------------
; Proces shrapnel piece
;----------------------------------------------------

prosh1:
	jsr plot 		; delete the pixel.

	lda #<shrptr		; shrapnel routine pointers.
	sta z80_l
	lda #>shrptr
	sta z80_h

	ldy #0
	lda (z80_ix),y		; restore shrapnel type.
	jsr prosh2 		; run the routine.
	jsr chkxy		; check x and y are good before we redisplay.

	lda #<SHRSIZ 		; distance to next.
	sta z80_e
	lda #>SHRSIZ
	sta z80_d
	rts

;----------------------------------------------------
; Run the routine
;----------------------------------------------------

prosh2:
	asl a 			; 2 bytes per address.
	tay
	lda shrptr,y
	sta z80_l
	lda shrptr+1,y 		; fetch high byte from table.
	sta z80_h
	jmp (z80_hl) 		; jump to routine.

;----------------------------------------------------
; Paricle routine table
;----------------------------------------------------

shrptr:	.word laser		; laser.
	.word trail		; vapour trail.
	.word shrap		; shrapnel from explosion.
	.word dotl		; horizontal starfield left.
	.word dotr		; horizontal starfield right.
	.word dotu		; vertical starfield up.
	.word dotd		; vertical starfield down.
	.word ptcusr		; user particle.

;----------------------------------------------------
; Explosion shrapnel.
;----------------------------------------------------

shrap:
	ldy #1
	lda (z80_ix),y 		; get the angle.
	clc
	adc #<shrsin		; shrapnel sine table.
	sta z80_l
	lda #>shrsin
	adc #0
	sta z80_h

	ldy #0
	lda (z80_hl),y 		; fetch value from table.
	sta z80_e
	inc z80_l 		; next byte of table.
	bne :+
	inc z80_h
:
	ldy #0
	lda (z80_hl),y		; fetch value from table.
	sta z80_d
	inc z80_l		; next byte of table.
	bne :+
	inc z80_h
:
	ldy #0
	lda (z80_hl),y 		; fetch value from table.
	sta z80_c
	inc z80_l 		; next byte of table.
	bne :+
	inc z80_h
:
	ldy #0
	lda (z80_hl),y 		; fetch value from table.
	sta z80_b

	ldy #2
	lda (z80_ix),y 		; x coordinate in hl.
	clc
	adc z80_e		; add sine lb
	sta (z80_ix),y		; store new coordinate lb.
	ldy #3
	lda (z80_ix),y
	adc z80_d		; add sine hb
	sta (z80_ix),y		; store new coordinate hb.

	ldy #4
	lda (z80_ix),y	 	; y coordinate in hl.
	clc
	adc z80_c		; add cosine lb
	sta (z80_ix),y		; store new coordinate lb.
	ldy #5
	lda (z80_ix),y
	adc z80_b		; add cosine lb
	sta (z80_ix),y		; store new coordinate hb.

	rts

;----------------------------------------------------
; Move dots
;----------------------------------------------------

dotl:
	ldy #5
	lda (z80_ix),y
	sec
	sbc #1		 	; move left.
	sta (z80_ix),y
	rts
dotr:
	ldy #5
	lda (z80_ix),y
	clc
	adc #1		 	; move left.
	sta (z80_ix),y
	rts
dotu:
	ldy #3
	lda (z80_ix),y
	sec
	sbc #1		 	; move up.
	sta (z80_ix),y
	rts
dotd:
	ldy #3
	lda (z80_ix),y
	clc
	adc #1			; move down.
	sta (z80_ix),y
	rts

;----------------------------------------------------
; Check if coordinates are ok before redrawing at new position.
;
; left:   X>L		X=L	Ok
; right:  R+15>X	X=R	Ok
; top:    Y>T		Y=T	Ok
; bottom: B+15>Y	Y=B	Ok
;----------------------------------------------------

chkxy:

; top:    Y>T		Y=T	Ok

	ldy #3
	lda (z80_ix),y	 	; fetch shrapnel coordinate.
	cmp wntopx		; window top.
	bcs :+			; compare with top window limit.
	jmp kilshr		; out of window, kill shrapnel.
:
; left:   X>L		X=L	Ok

	ldy #5
	lda (z80_ix),y	 	; fetch shrapnel coordinate.
	cmp wnlftx		; left edge.
	bcs :+			; compare with left window limit.
	jmp kilshr		; out of window, kill shrapnel.
:
; bottom: B+15>Y	Y=B	Ok

	lda wnbotx		; point to bottom.
	clc
	adc #15
	ldy #3
	cmp (z80_ix),y	 	; fetch shrapnel coordinate.
	bcs :+			; compare with shrapnel x coordinate.
	jmp kilshr		; off screen, kill shrapnel..
:
; right:  R+15>X	X=R	Ok

	lda wnrgtx		; point to right edge.
	clc
	adc #15
	ldy #5
	cmp (z80_ix),y	 	; fetch shrapnel coordinate.
	bcs :+			; compare with window limit.
	jmp kilshr		; off screen, kill shrapnel.
:

;----------------------------------------------------
; Drop through.
; Display shrapnel.
;----------------------------------------------------

plot:
	ldy #3
	lda (z80_ix),y		; y integer.
	sta dispy	 	; workspace coordinates.
	ldy #5
	lda (z80_ix),y	 	; x integer.
	sta dispx 		; workspace coordinates.

	ldy #0
	lda (z80_ix),y 		; type.
	bne :+			; is it a laser?
	jmp plot1 		; yes, draw laser instead.
:
plot0:
	lda dispx		; which pixel within byte do we
	and #7			; want to set first?
	tay
	lda dots,y 		; table of small pixel positions.
	sta z80_e 		; get value.

	jsr scadd 		; screen address.
	ldy #0
	lda (scraddr),y		; see what's already there.
	eor z80_e
	sta (scraddr),y 	; put back on screen.
	rts

plot1:
	jsr scadd 		; screen address.
	ldy #0
	lda (scraddr),y 	; fetch byte there.
	eor #255 		; toggle all bits.
	sta (scraddr),y 	; new byte.
	rts

;----------------------------------------------------
; Switch off shrapnel
;----------------------------------------------------

kilshr:
	lda #128
	ldy #0
	sta (z80_ix),y	; switch off shrapnel.
	rts

;----------------------------------------------------
; Sine/cosine table
;----------------------------------------------------

shrsin:	.word 0,1024,391,946,724,724,946,391
	.word 1024,0,946,65144,724,64811,391,64589
	.word 0,64512,65144,64589,64811,64811,64589,65144
	.word 64512,0,64589,391,64811,724,65144,946

;----------------------------------------------------
; Create trail
;----------------------------------------------------

trail:
	ldy #1
	lda (z80_ix),y 	; time remaining.
	sec
	sbc #1
	sta (z80_ix),y
	bne :+
	jmp trailk		; time to switch it off.
:
	jsr qrand		; get a random number.
	lsr a 			; x or y axis?
	bcc :+
	jmp trailv		; use y.
:
; Trail horizontal

	lsr a 			; which direction?
	bcc :+
	jmp traill		; go left.
:
; Trail right

	ldy #5
	lda (z80_ix),y
	clc
	adc #1	 		; go right.
	sta (z80_ix),y
	rts

; Trail left

traill:
	ldy #5
	lda (z80_ix),y
	sec
	sbc #1 			; go left.
	sta (z80_ix),y
	rts

; Trail vertical

trailv:
	lsr a		 	; which direction?
	bcc :+
	jmp trailu		; go up.
:
; Trail down

	ldy #3
	lda (z80_ix),y
	clc
	adc #1 			; go down.
	sta (z80_ix),y
	rts

; Trail up

trailu:
	ldy #3
	lda (z80_ix),y
	sec
	sbc #1 			; go up.
	sta (z80_ix),y
	rts

; Kill trail

trailk:
	lda #200		; set off-screen to kill vapour trail.
	ldy #3
	sta (z80_ix),y
	rts

;----------------------------------------------------
; Create laser beam
;----------------------------------------------------

laser:
	ldy #1
	lda (z80_ix),y 		; direction.
	ror a 			; left or right?
	bcs :+
	jmp laserl		; move left.
:
; Laser right

	lda #8			; distance to travel.
	sta z80_b
	jmp laserm		; move laser.

; Laser left

laserl:
	lda #248		; distance to travel.
	sta z80_b
laserm:
	ldy #5
	lda (z80_ix),y		; x position.
	clc
	adc z80_b		; add distance.
	sta (z80_ix),y		; set new x coordinate.

; Test new block.

	sta dispx 		; set x for block collision detection purposes.
	ldy #3
	lda (z80_ix),y 		; get y.
	sta dispy		; set coordinate for collision test.
	jsr tstbl 		; get block type there.
	cmp #WALL		; is it solid?
	bne :+
	jmp trailk		; yes, it cannot pass.
:
        cmp #FODDER             ; is it fodder?
        bne :+
        jsr fdchk               ; remove fodder block.
        jmp trailk              ; destroy laser.
:
        rts                     ; no, ignore it.

;----------------------------------------------------
; Dots mask
;----------------------------------------------------

dots:	.byte 128,64,32,16,8,4,2,1


;----------------------------------------------------
; Plot, preserving de.
;----------------------------------------------------

plotde:
	lda z80_d 		; put de on stack.
	pha
	lda z80_e
	pha

	jsr plot 		; plot pixel.

	pla			; restore de from stack.
	sta z80_e
	pla
	sta z80_d

	rts

;----------------------------------------------------
; Shoot a laser.
;----------------------------------------------------

shoot:
	sta z80_c		; store direction in c register.
	ldy #8
	lda (z80_ix),y 		; y coordinate.
shoot1:
	clc
	adc #7 			; down 7 pixels.
	sta z80_l 		; puty y coordinate in l.

	ldy #9
	lda (z80_ix),y 		; x coordinate in h.
	sta z80_h

	lda z80_i		; store pointer to sprite.
	pha
	lda z80_x
	pha

	jsr fpslot 		; find particle slot.
	bcs :+
	jmp vapou2		; failed, restore ix.
:
	lda #0
	ldy #0
	sta (z80_ix),y 		; set up a laser.

	lda z80_c
	ldy #1
	sta (z80_ix),y 		; set the direction.

	lda z80_l
	ldy #3
	sta (z80_ix),y		; set y coordinate.

	ror z80_c		; check direction we want.
	bcc :+
	jmp shootr		; shoot right.
:
	lda z80_h		; X position.
shoot0:
	and #248		; align on character boundary.
	ldy #5
	sta (z80_ix),y		; set x coordinate.
	jmp vapou0 		; draw first image.
shootr:
	lda z80_h		; x position.
	clc
	adc #15			; look right.
	jmp shoot0		; align and continue.

;----------------------------------------------------
; Create a bit of vapour trail.
;----------------------------------------------------

vapour:
	lda z80_i		; store pointer to sprite.
	pha
	lda z80_x
	pha

	ldy #8
	lda (z80_ix),y 		; y coordinate.
	clc
	adc #7			; mid-point of sprite.
	sta z80_l

	ldy #9
	lda (z80_ix),y 		; x coordinate.
	adc #7
	sta z80_h

	jsr fpslot 		; find particle slot.
	bcc :+
	jmp vapou1		; no, we can use it.
:
vapou2:
	pla
	sta z80_x
	pla
	sta z80_i
	rts
vapou1:
	lda z80_l
	ldy #3
	sta (z80_ix),y		; set up y.

	lda z80_h
	ldy #5
	sta (z80_ix),y 		; set up x coordinate.

	jsr qrand		; get quick random number.
	and #15			; random time.
	clc
	adc #15			; minimum time on screen.
	ldy #1
	sta (z80_ix),y		; set time on screen.

	lda #1
	ldy #0
	sta (z80_ix),y		; define particle as vapour trail.
vapou0:
	jsr chkxy		; plot first position.
	jmp vapou2

;----------------------------------------------------
; Create a user particle.
;----------------------------------------------------

ptusr:
	sta z80_f		; store timer.

	ldy #8
	lda (z80_ix),y 		; y coordinate.
	clc
	adc #7			; mid-point of sprite.
	sta z80_l

	ldy #9
	lda (z80_ix),y 		; x coordinate.
	clc
	adc #7			; mid-point of sprite.
	sta z80_h

	jsr fpslot 		; find particle slot.
	bcs ptusr1
	rts 			; out of slots, can't generate anything.
ptusr1:
	lda z80_l
	ldy #3
	sta (z80_ix),y 		; set up y.

	lda z80_h
	ldy #5
	sta (z80_ix),y		; set up x coordinate.

	lda z80_f 		; restore timer.
	ldy #1
	sta (z80_ix),y		; set time on screen.

	lda #7
	ldy #0
	sta (z80_ix),y		; define particle as user particle.

	jmp chkxy		; plot first position.

;----------------------------------------------------
; Create a vertical or horizontal star.
;----------------------------------------------------

star:
	lda z80_i		; store pointer to sprite.
	pha
	lda z80_x
	pha

	jsr fpslot 		; find particle slot.
	bcs star7		; found one we can use.
star0:
	pla 			; restore sprite pointer.
	sta z80_x
	pla
	sta z80_i
	rts 			; out of slots, can't generate anything.
star7:
	lda z80_c		; direction.
	and #3 			; is it left?
	bne :+
	jmp star1 		; yes, it's left.
:
	cmp #1 			; is it right?
	bne :+
	jmp star2 		; yes, it's right.
:
	cmp #2 			; is it up?
	bne :+
	jmp star3 		; yes, it's up.
:
	ldy wntopx 		; get edge of screen.
	iny			; down one pixel.
	tya
star8:
	ldy #3
	sta (z80_ix),y 		; set y coord.
	jsr qrand 		; get quick random number.
star9:
	ldy #5
	sta (z80_ix),y		; set x position.

	lda z80_c		; direction.
	and #3			; zero to three.
	clc
	adc #3			; 3 to 6 for starfield.
	ldy #0
	sta (z80_ix),y		; define particle as star.
	jsr chkxy		; plot first position.
	jmp star0
star1:
	jsr qrand		; get quick random number.
	ldy #3
	sta (z80_ix),y 		; set y coord.

	lda wnrgtx 		; get edge of screen.
	clc
	adc #15			; add width of sprite minus 1.
	jmp star9
star2:
	jsr qrand 		; get quick random number.
	ldy #3
	sta (z80_ix),y		; set y coord.

	lda wnlftx		; get edge of screen.
	jmp star9
star3:
	lda wnbotx 		; get edge of screen.
	clc
	adc #15 		; height of sprite minus one pixel.
	jmp star8

;----------------------------------------------------
; Find particle slot for lasers or vapour trail.
; can't use alternate accumulator.
;----------------------------------------------------

fpslot:
	lda #<SHRAPN 		; shrapnel table.
	sta z80_x
	lda #>SHRAPN
	sta z80_i

	lda #NUMSHR		; number of pieces in table.
	sta z80_b
fpslt0:
	ldy #0
	lda (z80_ix),y		; get type.
	asl a  			; is this slot in use?
	bcc :+
	rts			; no, we can use it.
:
	clc			; point to more shrapnel.
	lda z80_x
	adc #SHRSIZ
	sta z80_x
	bcc :+
	inc z80_i
:
	dec z80_b		; repeat for all shrapnel.
	bne fpslt0

	clc
	rts 			; out of slots, can't generate anything.

;----------------------------------------------------
; Create an explosion at sprite position.
;----------------------------------------------------

explod:
	sta z80_c 		; particles to create.

	lda z80_i 		; store pointer to sprite.
	pha
	lda z80_x
	pha

	ldy #8
	lda (z80_ix),y 		; y coordinate.
	sta z80_l
	ldy #9
	lda (z80_ix),y		; x coordinate.
	sta z80_h

	lda #<SHRAPN		; shrapnel table.
	sta z80_x
	lda #>SHRAPN
	sta z80_i

	lda #NUMSHR		; number of pieces in table.
	sta explcnt
expld0:
	ldy #0
	lda (z80_ix),y		; get type.
	asl a 			; is this slot in use?
	bcs expld1		; no, we can use it.
expld2:
	clc
	lda z80_x
	adc #SHRSIZ
	sta z80_x
	bcc :+
	inc z80_i
:
	dec explcnt		; repeat for all shrapnel.
	bne expld0
expld3:
	pla			; restore sprite pointer.
	sta z80_x
	pla
	sta z80_i
	rts 			; out of slots, can't generate any more.

expld1:
	lda z80_c		; shrapnel counter.
	and #15			; 0 to 15.
	clc			; add to x.
	adc z80_l
	ldy #3
	sta (z80_ix),y		; y coord.

	lda seed3 		; crap random number.
	and #15			; 0 to 15.
	clc 			; add to y.
	adc z80_h
	ldy #5
	sta (z80_ix),y		; x coord.

	lda #2
	ldy #0
	sta (z80_ix),y		; switch it on.

	jsr chkxy		; plot first position.
	jsr qrand		; quick random angle.
	and #60 		; keep within range.
	ldy #1
	sta (z80_ix),y		; angle.

	dec z80_c		; one less piece of shrapnel to generate.
	bne expld2 		; back to main explosion loop.
	jmp expld3 		; restore sprite pointer and exit.

;----------------------------------------------------
; Quick random
;----------------------------------------------------

qrand:
	jsr random		; r register.
	eor seed3		; combine with seed.
	sta seed3 		; new seed.
	rts

;----------------------------------------------------
; Display all shrapnel.
;----------------------------------------------------

dshrp:
	lda #<plotde		; display routine.
	sta proshx+1
	lda #>plotde
	sta proshx+2
	jsr proshr		; process shrapnel.

	lda #<prosh1		; processing routine.
	sta proshx+1
	lda #>prosh1
	sta proshx+2
	rts

;------------------------------------------------------
; Particle engine.
;
; Init particle data for 55 particles in SHRAPN table.
; Every particle has 6 bytes.
;
; global:	-
; local:	x,y,hl
; calls:	-
;------------------------------------------------------

inishr:
	lda #<SHRAPN 		; table.
	sta z80_l
	lda #>SHRAPN
	sta z80_h

	ldy #0
	ldx #NUMSHR		; shrapnel pieces to process.
inish0:
	lda #255 		; kill the shrapnel.
	sta (z80_hl),y

	clc 			; point there.
	lda z80_l
	adc #SHRSIZ		; distance to next.
	sta z80_l
	bcc :+
	inc z80_h
:
	dex
	bne inish0 		; round again.
	rts

;------------------------------------------------------
; Check for collision between laser and sprite.
;------------------------------------------------------

lcol:
	lda #<SHRAPN		; shrapnel table.
	sta z80_l
	lda #>SHRAPN
	sta z80_h

	lda #NUMSHR		; number of pieces in table.
	sta z80_b
lcol0:
	ldy #0
	lda (z80_hl),y 		; get type.
	beq lcol1		; yes, check collision.
lcol3:
	clc			; point to more shrapnel.
	lda z80_l
	adc #SHRSIZ
	sta z80_l
	bcc :+
	inc z80_h
:
	dec z80_b		; repeat for all shrapnel.
	bne lcol0
	rts 			; no collision, carry not set.
lcol1:
	ldy #3
	lda (z80_hl),y		; get y.
	sec
	ldy #8
	sbc (z80_ix),y		; subtract sprite y.
lcolh:
	cmp #16 		; within range?
	bcc :+
	jmp lcol2		; no, missed.
:
	ldy #5
	lda (z80_hl),y 		; get x.
	sec
	ldy #9
	sbc (z80_ix),y 		; subtract sprite y.
	cmp #16			; within range?
	bcs :+
	jmp lcol4 		; yes, collision occurred.
:
lcol2:
	jmp lcol3
lcol4:
	sec
	rts 			; return with carry set for collision.
.endif

;------------------------------------------------------
; Main game engine code starts here.
; After initialisation, mloop is the main loop
;------------------------------------------------------

game:

; Set up screen address table.

setsat:
	lda #<ScreenAddr		; start of screen.
	sta scraddr
	lda #>ScreenAddr
	sta scraddr+1

	ldy #0			; vertical lines on screen.
setsa0:
	lda scraddr
	sta SCADTB_lb,y		; write low byte.
	lda scraddr+1
	sta SCADTB_hb,y		; write high byte.
	jsr nline		; next line down.
	iny			; next position in table.
	bne setsa0

; Init graphics mode

	jsr screeninit

; Init AtoMMC joystick
	jsr joyinit		; AtoMMC joystick on PORT B

rpblc2:
.if pflag
	jsr inishr 		; initialise particle engine.
.endif
evintr:
	jsr evnt12 		; call intro/menu event.

	lda #WALL 		; write default property.
	ldx #0
clrmap:
	sta MAP,x 		; block properties.
	sta MAP+256,x
	sta MAP+512,x
	inx			; next byte.
	bne clrmap

	jsr iniob 		; initialise objects.

	lda #0			; put zero in accumulator.
	sta gamwon		; reset game won flag.

	jsr inisc 		; init the score.
mapst:
	lda stmap 		; start position on map.
	sta roomtb		; set up position in table, if there is one.

inipbl:
;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.if aflag
	lda #<eop		; reset blockpointer
	sta pbptr
	lda #>eop
	sta pbptr+1
.endif
;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	jsr initsc 		; set up first screen.

	lda #<ssprit 		; default to spare sprite in table.
	sta z80_x
	lda #>ssprit
	sta z80_i
evini:
	jsr evnt13 		; initialisation.

; Two restarts.
; First restart - clear all sprites and initialise everything.

rstrt:
	jsr rsevt 		; restart events.
	jsr xspr 		; clear sprite table.
	jsr sprlst 		; fetch pointer to screen sprites.
	jsr ispr 		; initialise sprite table.

	jmp rstrt0

; Second restart - clear all but player, and don't initialise him.

rstrtn:
	jsr rsevt		; restart events.
	jsr nspr 		; clear all non-player sprites.
	jsr sprlst 		; fetch pointer to screen sprites.
	jsr kspr 		; initialise sprite table, no more players.

; Set up the player and/or enemy sprites.

rstrt0:
	lda #0 			; zero in accumulator.
	sta nexlev 		; reset next level flag.
	sta restfl 		; reset restart flag.
	sta deadf 		; reset dead flag.
	jsr droom 		; show screen layout.
rpblc0:
.if pflag
	jsr inishr 		; initialise particle engine.
.endif
;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.if aflag
	jsr rbloc		; draw blocks for this screen
.endif
;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	jsr shwob		; draw objects.

	lda #<sprtab 		; address of sprite table, even sprites.
	sta z80_x
	lda #>sprtab
	sta z80_i

	jsr dspr 		; display sprites.

	lda #<(sprtab+TABSIZ) 	; address of first odd sprite.
	sta z80_x
	lda #>(sprtab+TABSIZ)
	sta z80_i
	jsr dspr 		; display sprites.

mloop:
	jsr bbcsync

	jsr vsync 		; synchronise with display.

	lda #<sprtab 		; address of sprite table, even sprites.
	sta z80_x
	lda #>sprtab
	sta z80_i

DEBUG_PAL PAL_red
	jsr dspr 		; display even sprites.
DEBUG_PAL PAL_black

	jsr plsnd 		; play sounds.
	jsr vsync 		; synchronise with display.

	lda #<(sprtab+TABSIZ) 	; address of first odd sprite.
	sta z80_x
	lda #>(sprtab+TABSIZ)
	sta z80_i

DEBUG_PAL PAL_red
	jsr dspr 		; display odd sprites.
DEBUG_PAL PAL_black

	lda #<(ssprit) 		; point to spare sprite for spawning purposes.
	sta z80_x
	lda #>(ssprit)
	sta z80_i
evlp1:
DEBUG_PAL PAL_green
	jsr evnt10 		; called once per main loop.
	jsr pspr 		; process sprites.
DEBUG_PAL PAL_black

; Main loop events.

	lda #<ssprit 		; point to spare sprite for spawning purposes.
	sta z80_x
	lda #>ssprit
	sta z80_i
DEBUG_PAL PAL_green
evlp2:
	jsr evnt11 		; called once per main loop.
bsortx:
	jsr bsort 		; sort sprites.
DEBUG_PAL PAL_black

	lda nexlev		; finished level flag.
	bne newlev		; is set, go to next level.
	lda gamwon		; finished game flag.
	bne evwon		; is set, finish the game.
	lda restfl 		; finished level flag.
	cmp #1			; has it been set?
	bne :+
	jmp rstrt		; yes, go to next level.
:
	cmp #2			; has it been set?
	bne :+
	jmp rstrtn		; yes, go to next level.
:
	lda deadf 		; dead flag.
	bne pdead		; yes, player dead.

; back to start of main loop.

	inc frmno
	inc clock
	jmp mloop		; switched to a jmp mloop during test mode.

;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

;----------------------------------------------------------
; Read blocks from list and update screen accordingly.
;----------------------------------------------------------

.if aflag
rbloc:
	lda #<eop		; reset temp blockpointer
	sta tmp
	lda #>eop
	sta tmp+1

rbloc2:
	lda tmp			; check for last block
	cmp pbptr
	bne rbloc1
	lda tmp+1
	cmp pbptr+1
	bne rbloc1
	rts
rbloc1:
	ldy #0
	lda (tmp),y		; check if block for this scno
	cmp scno
	bne rbloc0		; if not, skip
	iny
	lda (tmp),y		; get y
	sta dispy
	iny
	lda (tmp),y		; get x
	sta dispx
	iny
	lda (tmp),y		; get blocknr
	jsr pattr2		; draw block
rbloc0:
	clc			; point to next block
	lda tmp
	adc #4
	sta tmp
	bcc rbloc2
	inc tmp+1
	jmp rbloc2
.endif
;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

;----------------------------------------------------------
; New level
;----------------------------------------------------------

newlev:
	lda scno 			; current screen.
	clc
	adc #1				; next screen.
	cmp numsc + data_address - data_start			; total number of screens.
	bcs evwon			; yes, game finished.
	sta scno			; set new level number.
	jmp rstrt			; restart, clearing all aliens.

evwon:
	jsr evnt18		 	; game completed.
	jmp tidyup			; tidy up and return to BASIC/calling routine.

;----------------------------------------------------------
; Player dead.
;----------------------------------------------------------

pdead:
	lda #0				; zeroise accumulator.
	sta deadf			; reset dead flag.
evdie:
	jsr evnt16 			; death subroutine.
	lda numlif			; number of lives.
	beq :+
	jmp rstrt 			; restart game.
:
evfail:
	jsr evnt17 			; failure event.

;----------------------------------------------------------
; Tidy things up
;----------------------------------------------------------

tidyup:
	ldy #0				; digits to check.
tidyu2:
	lda score,y 			; get score digit.
	cmp hiscor 			; are we larger than high score digit?
	bcc tidyu0			; high score is bigger.
	bne tidyu1			; score is greater, record new high score.
	iny				; next digit of high score.
	cpy #6
	bne tidyu2			; repeat for all digits
tidyu0:
	lda #<score			; return pointing to score.
	sta z80_c
	lda #>score
	sta z80_b
	rts
tidyu1:
	ldy #5
tidyu3:
	lda score,y			; score.
	sta hiscor,y			; high score.
	dey
	bpl tidyu3 			; copy score to high score.
evnewh:
	jsr evnt19			; new high score event.
	jmp tidyu0			; tidy up.

;--------------------------------------------------
; Restart event.
;--------------------------------------------------

rsevt:
	lda #<ssprit 			; default to spare element in table.
	sta z80_x
	lda #>ssprit
	sta z80_i
evrs:
	jmp evnt14	 		; call restart event.

;------------------------------------------------------------------
; Copy number passed in a to string position bc, right-justified.
;
; Input:
;  A  = number
;  BC = string address
;
; Output:
;  BC = string with number
;-----------------------------------------------------------------

num2ch:
	sta z80_d		; Save number

	lda #0
	sta flag
numdg3:
	ldx #100		; hundreds column.
	stx z80_e
	jsr numdg		; show digit.
numdg2:
	ldx #10			; tens column.
	stx z80_e
	jsr numdg		; show digit.

	inc flag
	ldx #1			; units column.
	stx z80_e
numdg:
	lda #48			; clear digit.
	sta z80_a
numdg1:
	lda z80_d
	cmp z80_e
	bcc numdg0		; nothing to show.
	sec
	lda z80_d
	sbc z80_e		; subtract from column.
	sta z80_d
	inc z80_a		; increment digit.
	inc flag
	jmp numdg1		; repeat until column is zero.
numdg0:
	ldy #0
	lda z80_a
	sta (z80_bc),y		; write digit to buffer.
	lda flag
	beq :+
	inc z80_c		; next buffer position.
	bne :+
	inc z80_b
:
	rts
num2dd:
	sta z80_d		; Save number

	lda #1
	sta flag

	jmp numdg2
num2td:
	sta z80_d		; Save number

	lda #1
	sta flag
	jmp numdg3

;---------------------------------------------------------
; Reset score to "000000"
;---------------------------------------------------------

inisc:
	lda #'0'
	ldx #5			; digits to initialise.
inisc0:
	sta score,x 		; write zero digit.
	dex			; next column.
	bpl inisc0		; repeat for all digits.

	rts

;-----------------------------------------------------
; Multiply h by d and return in hl.
;
; Input:
;  H = first number
;  D = second number
;
; Output:
;  HL = result H x D
;-----------------------------------------------------

imul:
	lda z80_d		; HL = H * D
	sta z80_e
	lda z80_h
	sta z80_c		; make c first multiplier.
imul0:
	lda #0			; zeroise total.
	sta z80_l
	sta z80_h

	lda z80_h
	sta z80_d		; zeroise high byte.

	lda #8			; repeat 8 times.
	sta z80_b
imul1:
	lsr z80_c		; rotate rightmost bit into carry.
	bcc imul2		; wasn't set.
	clc			; bit was set, so add de.
	lda z80_l
	adc z80_e
	sta z80_l
	lda z80_h
	adc z80_d
	sta z80_h
	clc 			; reset carry.
imul2:
	asl z80_e 		; shift de 1 bit left.
	rol z80_d
	dec z80_b
	bne imul1		; repeat 8 times.

	rts

;-----------------------------------------------
; Divide d by e and return in d, remainder in a.
;
; Input:
;  D = first number
;  E = second number
;
; Output:
;  D = result D/E
;  A = remainder
;-----------------------------------------------

idiv:
	lda #0
	ldy #8		 	; bits to shift.
	asl z80_d
idiv0:
	rol a 			; multiply d by 2.
	cmp z80_e 		; test if e is smaller.
	bcc idiv1		; e is greater, no division this time.
	sbc z80_e		; subtract it.
idiv1:
	rol z80_d		; rotate into d.
	dey
	bne idiv0		; repeat
	rts

;---------------------------------------------------
; Play AY sound effect
;---------------------------------------------------

plsnd:
	rts

;---------------------------------------------------
; Objects handling.
; 32 bytes for image
; 3 for room, y and x
; 3 for starting room, y and x.
; 254 = disabled.
; 255 = object in player"s pockets.
;---------------------------------------------------

;---------------------------------------------------
; Show items present.
;---------------------------------------------------

shwob:
	lda #<(objdta + data_address - data_start) 			; objects table.
	sta z80_l
	lda #>(objdta + data_address - data_start)
	sta z80_h

	lda numob 			; number of objects in the game.
	sta sprcnt
shwob0:
	ldy #32 			; distance to room number.
	lda (z80_hl),y 			; same as an item?
	cmp scno 			; current location.
	bne :+
	jsr dobj 			; yes, display object.
:
	clc
	lda z80_l
	adc #38 			; distance to next item.
	sta z80_l
	lda z80_h
	adc #0
	sta z80_h	 		; point to it.
	dec sprcnt
	bne shwob0 			; repeat for others.
	rts

;---------------------------------------------------
; Display object.
; hl must point to object's start address.
;
; Input:
;  HL = object address
;---------------------------------------------------

dobj:
	ldy #33
	lda (z80_hl),y 			; point to y.
	sta dispy
	iny
	lda (z80_hl),y 			; point to x.
	sta dispx
dobj1:
	jmp sprite 			; draw this sprite.

;--------------------------------------
; Remove an object.
;
; Input:
;  A = object number
;--------------------------------------

remob:
	cmp numob			; number of objects in game.
	bcc :+				; are we checking past the end?
	rts				; yes, can't get non-existent item.
:
	pha				; remember object.
	jsr getob			; pick it up if we haven't already got it.
	pla				; retrieve object number.
	jsr gotob			; get its address.
	lda #254
	ldy #32
	sta (z80_hl),y			; remove it.
	rts

;---------------------------------------------------
; Pick up object number held in the accumulator.
;
; Input:
;  A = object number
;---------------------------------------------------

getob:
	cmp numob 		; number of objects in game.
	bcc :+			; are we checking past the end?
	rts			; yes, can't get non-existent item.
:
	jsr gotob 		; check if we already have it.
	cmp #255
	bne :+
	rts			; we already do.
:
	ldy #32
	lda (z80_hl),y		; is it on this screen?
	cmp scno 		; current screen.
	bne getob0		; not on screen, so nothing to delete.

	lda #255
	sta (z80_hl),y		; pick it up.
	iny 			; point to y coord.
getob1:
	ldy #33
	lda (z80_hl),y		; y coord.
	sta dispy
	ldy #34
	lda (z80_hl),y 		; x coord.
	sta dispx
	jmp dobj1 		; delete object sprite.
getob0:
	lda #255
	sta (z80_hl),y 		; pick it up.
	rts

;-----------------------------------------------------------------
; Got object check.
; Call with object in accumulator, returns zero set if in pockets.
;
; Input:
;  A = object number
;-----------------------------------------------------------------

gotob:
	cmp numob 		; number of objects in game.
	bcc :+ 			; are we checking past the end?
	jmp gotob0 		; yes, we can't have a non-existent object.
:
	jsr findob		; find the object.
gotob1:
	rts

gotob0:
	lda #254 		; missing.
	jmp gotob1

findob:
	pha			; save object number
	lda #<(objdta + data_address - data_start) 		; objects.
	sta z80_l
	lda #>(objdta + data_address - data_start)
	sta z80_h
	pla			; retreive object number
	beq fndob1 		; is it zero? yes, skip loop.
	tax 			; loop counter
fndob2:
	clc
	lda z80_l
	adc #38 		; size of each object.
	sta z80_l
	bcc :+
	inc z80_h
:
	dex 			; repeat until we find address.
	bne fndob2
fndob1:
	ldy #32			; distance to room it's in.
	lda (z80_hl),y		; fetch status.
	rts

;---------------------------------------------
; Drop object number at (dispx, dispy).
;
; Input:
;  A = object number
;---------------------------------------------

drpob:
	cmp numob 		; are we checking past the end?
	bcc :+
	rts			; yes, can't drop non-existent item.
:
	jsr gotob		; make sure object is in inventory.
	cmp scno		; already on this screen?
	bne :+
	rts			; yes, nothing to do.
:
	ldy #32
	lda scno
	sta (z80_hl),y		; bring onto screen.
	lda dispy		; sprite y coordinate.
	iny 
	sta (z80_hl),y		; point to object y.
	lda dispx 		; sprite x coordinate.
	iny
	sta (z80_hl),y 		; point to object x
	jmp dobj		; draw the object sprite.

;-----------------------------------------------
; Seek objects at sprite position.
;
; Output:
;  A = object number, if not found A=255
;-----------------------------------------------

skobj:
	lda #<(objdta + data_address - data_start) 		; pointer to objects.
	sta z80_l
	lda #>(objdta + data_address - data_start)
	sta z80_h

	lda numob 		; number of objects in game.
	sta z80_b 		; set up the loop counter.
skobj0:
	lda scno		; current room number.
	ldy #32
	cmp (z80_hl),y		; is object in here?
	bne :+
	jsr skobj1		; yes, check coordinates.
:
	clc			; point to next object in table.
	lda z80_l
	adc #38			; size of each object.
	sta z80_l
	bcc :+
	inc z80_h
:
	dec z80_b
	bne skobj0		; repeat for all objects.

	lda #255		; end of list and nothing found, return 255.
	rts

skobj1:
	ldy #33			; point to y coordinate.
	lda (z80_hl),y		; point to y coordinate.
	sec
	ldy #var_newY
	sbc (z80_ix),y 		; subtract sprite y.
	clc
	adc #15			; add sprite height minus one.
	cmp #31			; within range?
	bcc :+
	jmp skobj2		; no, ignore object.
:
	ldy #34			; point to x coordinate now.
	lda (z80_hl),y 		; get coordinate.
	sec
	ldy #var_newX
	sbc (z80_ix),y 		; subtract the sprite x.
	clc			; add sprite width minus one.
	adc #15
	cmp #31			; within range?
	bcc :+
	jmp skobj2		; no, ignore object.
:
	pla			; remove return address from stack.
	pla

	lda numob 		; objects in game.
	sec
	sbc z80_b		; subtract loop counter.
skobj2:
	rts			; accumulator now points to object.


;---------------------------------------------------------------------
; Spawn a new sprite.
;---------------------------------------------------------------------

spawn:
	lda #<sprtab		; sprite table.
	sta z80_l
	lda #>sprtab
	sta z80_h
numsp1:
	lda #NUMSPR		; number of sprites.
	sta spcnt
spaw0:
	ldy #var_Type
	lda (z80_hl),y		; get sprite type.
	cmp #255		; is it an unused slot?
	beq spaw1 		; yes, we can use this one.

	clc 			; point to next sprite in table.
	lda z80_l
	adc #TABSIZ		; size of each entry.
	sta z80_l
	bcc :+
	inc z80_h
:
	dec spcnt		; one less iteration.
	bne spaw0		; keep going until we find a slot.

; Didn't find one but drop through and set up a dummy sprite instead.

spaw1:
	lda z80_i		; address of original sprite.
	pha
	lda z80_x
	pha

	lda z80_l		; store spawned sprite address.
	sta spptr
	lda z80_h
	sta spptr+1

	lda z80_c
	ldy #var_Type
	sta (z80_hl),y 		; set the type.
	ldy #var_newType
	sta (z80_hl),y		; copy

	lda z80_b
	ldy #var_Image
	sta (z80_hl),y		; set the image.
	ldy #var_newImage
	sta (z80_hl),y		; copy

	lda #0 				; frame zero.
	ldy #var_Frame
	sta (z80_hl),y		; set frame.
	ldy #var_newFrame
	sta (z80_hl),y		; copy

	ldy #9
	lda (z80_ix),y 		; x coordinate.
	ldy #var_X
	sta (z80_hl),y		; set sprite coordinate.
	ldy #var_newX
	sta (z80_hl),y		; copy

	ldy #8
	lda (z80_ix),y 		; y coordinate.
	ldy #var_Y
	sta (z80_hl),y		; set sprite coordinate.
	ldy #var_newY
	sta (z80_hl),y		; copy

	ldy #10				; direction of original.
	lda (z80_ix),y
	ldy #var_Direction
	sta (z80_hl),y		; direction

	lda #7				; default to white ink - spriteink?
	ldy #var_Ink
	sta (z80_hl),y		; set sprite ink
	ldy #var_newInk
	sta (z80_hl),y		; set sprite ink

	lda #0
	ldy #var_jumpLo
	sta (z80_hl),y		; reset parameter.
	iny
	sta (z80_hl),y		; reset parameter.
	iny
	sta (z80_hl),y		; reset parameter.
	iny
	sta (z80_hl),y		; reset parameter.
rtssp:
	lda spptr			; address of new sprite.
	sta z80_x
	lda spptr+1
	sta z80_i
evis1:
	jsr evnt09 			; call sprite initialisation event.

	lda spptr 			; address of new sprite.
	sta z80_x
	lda spptr+1
	sta z80_i

	; _BEEB clipping code copied from CPC Engine - MISSING?!
	ldy #var_Y
	lda (z80_hl), y		; old x coord
	cmp #SpriteMaxY     ; beyond maximum?
	bcs :+				; yes, don't draw it.

	jsr sspria 			; display the new sprite.
:
	pla					; address of original sprite.
	sta z80_x
	pla
	sta z80_i

	rts

score:	.byte "000000"		; player"s score.
hiscor:	.byte "000000"		; high score.
bonus:	.byte "000000"		; bonus.
grbase:	.word ScreenAddr	; graphics base address.

;----------------------------------------------------
; Check y-pos
;----------------------------------------------------

checkx:
	lda dispy		; y position.
	cmp #24			; off screen?
	bcs :+
	rts			; no, it's okay.
:
	pla			; remove return address from stack.
	sta z80_l
	pla
	sta z80_h
	rts

;-----------------------------------------------
; Displays the current high score.
;-----------------------------------------------

dhisc:
	lda #<hiscor 		; high score text.
	sta z80_l
	lda #>hiscor
	sta z80_h
	jmp dscor1		; check in printable range then show 6 digits.

;------------------------------------------------------
; Displays the current score.
;------------------------------------------------------

dscor:
	lda #<score		; score text.
	sta z80_l
	lda #>score
	sta z80_h
dscor1:
	jsr preprt		; set up font and print position.
	jsr checkx		; make sure we're in a printable range.

	lda #6			; digits to display.
	sta z80_b
	lda prtmod		; get print mode.
	beq :+			; standard size text?
	jmp bscor0		; no, show double-height.
:
dscor0:
	ldy #0
	lda (z80_hl),y 		; fetch character.
	jsr pchar 		; display character.
	inc dispx		; move along x coordinate

	inc z80_l		; next score column.
	bne :+
	inc z80_h
:
	dec z80_b
	bne dscor0 		; repeat for all digits.
dscor2:
	lda dispx 		; set up display coordinates.
	sta charx
	lda dispy
	sta chary
	rts

;------------------------------------------------------
; Displays the current score in double-height characters.
;
; Input:
;  B  = digit number
;  HL = score string
;------------------------------------------------------

bscor0:
	ldy #0

	lda (z80_hl),y 		; fetch character.
	jsr bchar 		; display big char.

	inc z80_l 		; next score column.
	bne :+
	inc z80_h
:
	dec z80_b
	beq :+
	jmp bscor0 		; repeat for all digits.
:
	jmp dscor2 		; tidy up line and column variables.

;-----------------------------------------------------
; Adds number in the hl pair to the score.
;-----------------------------------------------------

addsc:
	lda #<(score+1) 	; ten thousands column.
	sta z80_e
	lda #>(score+1)
	sta z80_d
	lda #<10000		; amount to add each time.
	sta z80_c
	lda #>10000
	sta z80_b
	jsr incsc		; add to score.

	inc z80_e		; thousands column.
	bne :+
	inc z80_d
:
	lda #<1000		; amount to add each time.
	sta z80_c
	lda #>1000
	sta z80_b
	jsr incsc 		; add to score.

	inc z80_e		; hundreds column.
	bne :+
	inc z80_d
:
	lda #<100		; amount to add each time.
	sta z80_c
	lda #>100
	sta z80_b
	jsr incsc		; add to score.

	inc z80_e 		; tens column.
	bne :+
	inc z80_d
:
	lda #<10		; amount to add each time.
	sta z80_c
	lda #>10
	sta z80_b
	jsr incsc 		; add to score.

	inc z80_e		; units column.
	bne :+
	inc z80_d
:
	lda #<1			; units.
	sta z80_c
	lda #>1
	sta z80_b
incsc:
	lda z80_h		; store amount to add.
	pha
	lda z80_l
	pha

	sec			; subtract from amount to add.
	lda z80_l
	sbc z80_c
	sta z80_l
	lda z80_h
	sbc z80_b
	sta z80_h
	bcc incsc0		; too much, restore value.

	pla			; delete the previous amount from the stack.
	pla

	lda z80_d 		; store column position.
	pha
	lda z80_e
	pha
	jsr incsc2		; do the increment.

	pla			; restore column.
	sta z80_e
	pla
	sta z80_d
	jmp incsc		; repeat until all added.

incsc0:
	pla			; restore previous value.
	sta z80_l
	pla
	sta z80_h
	rts
incsc2:
	ldy #0
	lda (z80_de),y 		; get amount.
	clc
	adc #1			; add one to column.
	sta (z80_de),y		; write new column total.
	cmp #'9'+1		; gone beyond range of digits?
	bcs :+
	rts			; no, carry on.
:
	lda #'0'		; make it zero.
	sta (z80_de),y		; write new column total.
	dec z80_e		; back one column.
	bne :+
	dec z80_d
:
	jmp incsc2

;------------------------------------
; Add bonus to score and reset bonus
;------------------------------------

addbo:
	ldx #5			; last digit.
	clc			; clear carry.
addbo0:
	lda score,x		; get score.
	adc bonus,x		; add bonus.
	sec			; 0 to 18.
	sbc #48
	pha
	lda #'0'
	sta bonus,x		; zeroise bonus.
	pla
	cmp #58			; carried?
	bcs addbo2		; no, do next one.
	sec
	sbc #10			; subtract 10.
	sec
	jmp addbo1
addbo2:
	clc
addbo1:
	sta score,x		; write new score.
	dex			; next digit.
	bpl addbo0		; repeat for all 6 digits.
	rts

;------------------------------------
; Swap score and bonus.
;------------------------------------

swpsb:
	ldx #5			; digits to add.
swpsb0:
	lda score,x 		; get score digits.
	pha			; save digit
	lda bonus,x 		; get bonus digits.
	sta score,x		; switch score-bonus
	pla
	sta bonus,x
	dex 			; repeat for all 6 digits.
	bpl swpsb0
	rts

;----------------------------------------------
; Write block
;----------------------------------------------

;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.if aflag
wbloc:
	ldy #3
	sta (pbptr),y		; store block number
	dey
	lda dispx
	sta (pbptr),y		; write x position of block.
	dey
	lda dispy
	sta (pbptr),y		; write y position of block.
	dey
	lda scno
	sta (pbptr),y		; write screen.
	clc			; point to next free location
	lda pbptr
	adc #4
	sta pbptr
	bcc :+
	inc pbptr+1
:
	rts
.endif
;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

;----------------------------------------------
; Print character pixels, no more.
;
; Input:
;  A	= character to print
;----------------------------------------------

pchr:
	jsr pchar 		; show character in accumulator.
	inc dispx		; move along one.
	rts

;----------------------------------------------------
; Shifter sprite routine for objects.
;----------------------------------------------------

sprit7:
	lda z80_b
	beq sprit0
	tay
sprit3:
	lsr spr			; shift into position.
	ror spr+1
	ror spr+2
	dey				; one less iteration.
	bne sprit3
sprit0:
	rts 			; now apply to screen.

;-----------------------------------------------------------
; Get room address.
;-----------------------------------------------------------

groom:
	ldx scno 		; screen number.
	ldy #0
groomx:
	lda #<(scdat + data_address - data_start) 		; pointer to screens.
	sta z80_l
	lda #>(scdat + data_address - data_start)
	sta z80_h
groom1:
	cpx #0			; is it the first one?
	beq groom0 		; no more screens to skip.

	clc
	lda z80_l
	adc scdat + data_address - data_start,y 		; low byte of screen size.
	sta z80_l
	iny			; point to high byte.
	lda z80_h
	adc scdat + data_address - data_start,y 		; high byte of screen size.
	sta z80_h
	iny			; next address.

	dex 			; one less iteration.
	jmp groom1 		; loop until we reach the end.
groom0:
	lda numsc + data_address - data_start 		; add displacement.
	asl a
	clc			; add double displacement to address.
	adc z80_l
	sta z80_l
	lda z80_h
	adc #0
	sta z80_h
	rts

;-----------------------------------------------------------
; Draw present room.
;-----------------------------------------------------------

droom:
	lda wintop 		; window top.
	sta dispy		; set cursor y position.
droom2:
	jsr groom 		; get address of current room.
	lda #0	 		; zero in accumulator.
	sta comcnt 		; reset compression counter.
	lda winhgt 		; height of window.
	sta rrow		; set row counter
droom0:
	lda winlft 		; window left edge.
	sta dispx 		; set cursor x position.
	lda winwid 		; width of window.
	sta rcol		; set column counter
droom1:
	jsr flbyt 		; decompress next byte on the fly.
	jsr pattr2 		; show attributes and block.
	dec rcol		; one less column.
	bne droom1 		; repeat for entire line.
	inc dispy		; move down one line.
	dec rrow 		; one less row.
	bne droom0 		; repeat for all rows.
	rts

;----------------------------------------------
; Decompress bytes on-the-fly.
;----------------------------------------------

flbyt:
	lda comcnt 		; compression counter.
	bne flbyt1		; any more to decompress?  yes.

	ldy #0
	lda (z80_hl),y 		; fetch next byte.
	inc z80_l 		; point to next cell.
	bne :+
	inc z80_h
:
	cmp #255 		; is this byte a control code?
	beq :+
	rts 			; no, this byte is uncompressed.
:
	lda (z80_hl),y 		; fetch byte type.
	sta combyt 		; set up the type.
	inc z80_l 		; point to quantity.
	bne :+
	inc z80_h
:
	lda (z80_hl),y 		; get quantity.
	inc z80_l 		; point to next byte.
	bne :+
	inc z80_h
:
flbyt1:
	sta comcnt 		; store new quantity.
	dec comcnt		; one less.
	lda combyt 		; byte to expand.
	rts

;------------------------------------------
; Ladder down check.
;
; Input:
;  IX = sprite pointer
;------------------------------------------

laddd:
	ldy #var_newX
	lda (z80_ix),y		; x coordinate.
	sta dispx

	ldy #var_newY
	lda (z80_ix),y		; y coordinate.
	and #254		; make it even.
	sta (z80_ix),y 		; reset it.
numsp5:
	clc 			; look down 16 pixels.
	adc #16
	sta dispy		; coords in dispx,dispy.
	jmp laddv

;------------------------------------------
; Ladder up check.
;
; Input:
;  IX = sprite pointer
;
; Output:
;  A  = 0 is ok, A <>0 is not ok
;------------------------------------------

laddu:
	ldy #var_newX
	lda (z80_ix),y		; x coordinate.
	sta dispx

	ldy #var_newY
	lda (z80_ix),y		; y coordinate.
	and #254 		; make it even.
	sta (z80_ix),y		; reset it.
numsp6:
	clc 			; look 2 pixels above feet.
	adc #14
	sta dispy		; coords in dispx,dispy.
laddv:
	jsr tstbl 		; get map address.
	jsr ldchk 		; standard ladder check.
	beq :+
	rts 			; no way through.
:
	inc bufaddr 		; look right one cell.
	bne :+
	inc bufaddr+1
:
	jsr ldchk 		; do the check.
	beq :+
	rts 			; impassable.
:
	lda dispx 		; y coordinate.
	and #7 			; position straddling block cells.
	bne :+
	rts 			; no more checks needed.
:
	inc bufaddr 		; look to third cell.
	bne :+
	inc bufaddr+1
:
	jsr ldchk 		; do the check.
	rts  			; return with zero flag set accordingly.

;---------------------------------------------------------
; Can go up check.
;
; Input:
;  IX = sprite pointer
;
; Output:
;  A  = 0 is ok, A <>0 is not ok
;---------------------------------------------------------

cangu:
	ldy #var_newX
	lda (z80_ix),y		; x coordinate.
	sta dispx
	ldy #var_newY
	lda (z80_ix),y 		; y coordinate.
	sec
	sbc #2
	sta dispy		; coords in dispx,dispy.
	jsr tstbl 		; get map address.
	jsr lrchk 		; standard left/right check.
	beq :+
	rts			; no way through.
:
	inc bufaddr		; look right one cell.
	bne :+
	inc bufaddr+1
:
	jsr lrchk 		; do the check.
	beq :+
	rts			; impassable.
:
	lda dispx		; x coordinate.
	and #7			; position straddling block cells.
	bne :+
	rts			; no more checks needed.
:
	inc bufaddr		; look to third cell.
	bne :+
	inc bufaddr+1
:
	jsr lrchk		; do the check.
	rts 			; return with zero flag set accordingly.

;---------------------------------------------------------
; Can go down check.
;
; Input:
;  IX = sprite pointer
;
; Output:
;  A  = 0 is ok, A <>0 is not ok
;---------------------------------------------------------

cangd:
	ldy #var_newX
	lda (z80_ix),y 		; x coordinate.
	sta dispx
	ldy #var_newY
	lda (z80_ix),y		; y coordinate.
numsp3:
	clc
	adc #16 		; look down 16 pixels.
	sta dispy		; coords in dispx,dispy.
	jsr tstbl 		; get map address.
	jsr plchk 		; block, platform check.
	beq :+
	rts			; no way through.
:
	inc bufaddr		; look right one cell.
	bne :+
	inc bufaddr+1
:
	jsr plchk		; block, platform check.
	beq :+
	rts			; impassable.
:
	lda dispx		; x coordinate.
	and #7			; position straddling block cells.
	bne :+
	rts			; no more checks needed.
:
	inc bufaddr		; look to third cell.
	bne :+
	inc bufaddr+1
:
	jsr plchk		; block, platform check.
	rts			; return with zero flag set accordingly.

;---------------------------------------------------------
; Can go left check.
;
; Input:
;  IX = sprite pointer
;
; Output:
;  A  = 0 is ok, A <>0 is not ok
;---------------------------------------------------------

cangl:
	ldy #var_newY
	lda (z80_ix),y 		; y coordinate.
	sta dispy
	ldy #var_newX
	lda (z80_ix),y 		; x coordinate.
	sec
	sbc #2			; look left 2 pixels.
	sta dispx		; coords in dispx,dispy.
	jmp cangh		; test if we can go there.

;---------------------------------------------------------
; Can go right check.
;
; Input:
;  IX = sprite pointer
;
; Output:
;  A  = 0 is ok, A <>0 is not ok
;---------------------------------------------------------

cangr:
	ldy #var_newY
	lda (z80_ix),y		; y coordinate.
	sta dispy
	ldy #var_newX
	lda (z80_ix),y		; x coordinate.
	clc
	adc #16			; look right 16 pixels.
	sta dispx		; coords in dispx,dispy.
cangh:
cangh2:
	lda #3			; default rows to write.
	sta z80_b
	lda dispy		; y position.
	and #7			; does x straddle cells?
	bne cangh0		; yes, loop counter is good.
	dec z80_b		; one less row to write.
cangh0:
	jsr tstbl		; get map address.
cangh1:
	jsr lrchk		; standard left/right check.
	beq :+
	rts			; no way through.
:
	pha
	clc
	lda bufaddr
	adc #32			; look down.
	sta bufaddr
	bcc :+
	inc bufaddr+1
:
	pla

	dec z80_b
	bne cangh1
	rts

;-------------------------------------
; Check left/right movement is okay.
;
; Input:
;  bufaddr = MAP + x/8 + y/8*32
;
; Output:
;  A  = 0 is ok, A <>0 is not ok
;-------------------------------------

lrchk:
	ldy #0
	lda (bufaddr),y		; fetch map cell.
	cmp #WALL 		; is it passable?
	beq lrchkx		; no.

	cmp #FODDER		; fodder has to be dug.
	beq lrchkx		; not passable.
	lda #0
	rts

;--------------------------------------------------------------
; Called by mmenu
;--------------------------------------------------------------

always:
	lda #255		; report it as okay.
	rts

lrchkx:
	lda #1 			; reset all bits.
	rts


;--------------------------------------------------------------
; Check platform or solid item is not in way.
;
; Input:
;  bufaddr = MAP + x/8 + y/8*32
;
; Output:
;  A  = 0 is ok, A <>0 is not ok
;--------------------------------------------------------------

plchk:
	ldy #0
	lda (bufaddr),y 	; fetch map cell.
	cmp #WALL 		; is it passable?
	beq lrchkx		; no.

	cmp #FODDER		; fodder has to be dug.
	beq lrchkx		; not passable.

	cmp #PLATFM		; platform is solid.
	beq plchkx		; not passable.

	cmp #LADDER		; is it a ladder?
	beq lrchkx		; on ladder, deny movement.
plchk0:
	lda #0			; report as ok
	rts
plchkx:
	lda dispy		; x coordinate.
	and #7			; position straddling blocks.
	beq lrchkx		; on platform, deny movement.
	jmp plchk0

;--------------------------------------------------------------
; Check ladder is available.
;
; Input:
;  bufaddr = MAP + x/8 + y/8*32
;
; Output:
;  A  = 0 is ok, A <>0 is not ok
;--------------------------------------------------------------

ldchk:
	ldy #0
	lda (bufaddr),y 	; fetch cell.
	cmp #LADDER 		; is it a ladder?
	beq :+
	lda #1
	rts  			; return with zero flag set accordingly.
:
	lda #0
	rts

;--------------------------------------------------------------
; Get collectables.
;--------------------------------------------------------------

getcol:
    lda #COLECT             ; collectable blocks.
    sta z80_b
    jsr tded                ; test for collectable blocks.
    cmp z80_b               ; did we find one?
    beq :+
    rts                     ; none were found, job done.
:
    jsr gtblk               ; get block.

    jsr evnt20              ; collected block event.
    jmp getcol              ; repeat until none left.

; Get collectable block.

gtblk:
	ldy #0
	lda (bufaddr),y
	sta z80_a
        lda #0
        sta (bufaddr),y		; make it empty now.
       
	lda bufaddr		; set dispx
	and #31
	sta dispx

	lda bufaddr+1		; Set dispy
	sec
	sbc #>MAP
	sta bufaddr+1
	asl bufaddr
	rol bufaddr+1
	asl bufaddr
	rol bufaddr+1
	asl bufaddr
	rol bufaddr+1
	lda bufaddr+1
	sta dispy
 
	lda colpatt		; get blocknr
	sta z80_e		; displacement in e.
	lda #0
	sta z80_d		; no high byte.
	asl z80_e  		; multiply char by 8.
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
	ldy #7			; number of pixel rows to write.
gtblk0:
	lda (tileaddr),y 	; get image byte.
	eor (scraddr),y 	; XOR tile on screen
	sta (scraddr),y 	; copy to screen.
	dey	 		; repeat for 8 pixel rows.
	bpl gtblk0
	rts

;--------------------------------------------------------------
; Touched deadly block check.
; returns with DEADLY (must be non-zero) in accumulator if true.
;
; Input:
;  IX = sprite address
;
; Output:
;  A  = 0 is ok, A=5 is not ok
;--------------------------------------------------------------

tded:
	ldy #var_newY
	lda (z80_ix),y 		; y coordinate.
	sta dispy
	iny
	lda (z80_ix),y 		; x coordinate.
	sta dispx		; coords in dispx,dispy.
	jsr tstbl		; get map address.
	pha
	lda #31			; default distance to next line down.
	sta z80_e
	pla
	cmp z80_b		; is this the required block?
	bne :+
	rts			; yes.
:
	inc bufaddr 		; next cell.
	bne :+
	inc bufaddr+1
:
	ldy #0
	lda (bufaddr),y		; fetch type.
	cmp z80_b 		; is this deadly/custom?
	bne :+
	rts			; yes.
:
	lda dispx		; horizontal position.
	sta z80_c 		; store column in c register.
	and #7			; is it straddling cells?
	bne :+
	jmp tded0		; no.
:
	inc bufaddr 		; last cell.
	bne :+
	inc bufaddr+1
:
	ldy #0
	lda (bufaddr),y 	; fetch type.
	cmp z80_b		; is this the block?
	bne :+
	rts			; yes.
:
	dec z80_e		; one less cell to next row down.
tded0:
	clc 			; point to next row.
	lda bufaddr
	adc z80_e
	sta bufaddr
	bcc :+
	inc bufaddr+1
:
	ldy #0
	lda (bufaddr),y		; fetch left cell block.
	cmp z80_b		; is this fatal?
	bne :+
	rts			; yes.
:
	inc bufaddr 		; next cell.
	bne :+
	inc bufaddr+1
:
	ldy #0
	lda (bufaddr),y 	; fetch type.
	cmp z80_b		; is this fatal?
	bne :+
	rts			; yes.
:
	lda z80_c		; horizontal position.
	and #7			; is it straddling cells?
	bne :+
	jmp tded1 		; no.
:
	inc bufaddr		; last cell.
	bne :+
	inc bufaddr+1
:
	ldy #0
	lda (bufaddr),y		; fetch type.
	cmp z80_b		; is this fatal?
	bne :+
	rts			; yes.
:
tded1:
	lda dispy		; vertical position.
	and #7 			; is it straddling cells?
	bne :+
	rts			; no, job done.
:
	clc			; point to next row.
	lda bufaddr
	adc z80_e
	sta bufaddr
	bcc :+
	inc bufaddr+1
:
	ldy #0
	lda (bufaddr),y 	; fetch left cell block.
	cmp z80_b		; is this fatal?
	bne :+
	rts			; yes.
:
	inc bufaddr		; next cell.
	bne :+
	inc bufaddr+1
:
	ldy #0
	lda (bufaddr),y 	; fetch type.
	cmp z80_b		; is this fatal?
	bne :+
	rts			; yes.
:
	lda z80_c		; horizontal position.
	and #7			; is it straddling cells?
	bne :+
	rts			; no.
:
	inc bufaddr		; last cell.
	bne :+
	inc bufaddr+1
:
	ldy #0
	lda (bufaddr),y		; fetch final type.
	rts 			; return with final type in accumulator.

;---------------------------------------------------
; Fetch block type at (dispx, dispy).
;
; Output:
;  A = block type
;---------------------------------------------------

tstbl:
	lda dispy 			; fetch y coord.
	lsr a				; bufaddr = y/8
	lsr a
	lsr a
	sta chary

	sta bufaddr
	lda #0
	sta bufaddr+1

	asl bufaddr  		; bufaddr = y/8 * 32
	rol bufaddr+1
	asl bufaddr
	rol bufaddr+1
	asl bufaddr
	rol bufaddr+1
	asl bufaddr
	rol bufaddr+1
	asl bufaddr
	rol bufaddr+1

	lda dispx			; x/8
	lsr a
	lsr a
	lsr a
	sta charx

	clc					; bufaddr = MAP + x/8 + y/8*32
	adc bufaddr
;	adc #<MAP			; only works because <MAP == $0
	sta bufaddr
	lda bufaddr+1
	adc #>MAP
	sta bufaddr+1

	ldy #0
	lda (bufaddr),y 	; fetch byte there.
	rts

;-------------------------------------------------------------------
; Jump - if we can.
; Requires initial speed to be set up in accumulator prior to call.
;
; Input:
;  IX = sprite address
;-------------------------------------------------------------------

jump:
	ldy #var_jumpLo
	lda (z80_ix),y		; jump table low.
	ldy #var_jumpHi
	ora (z80_ix),y		; jump table high.
	beq :+
	rts			; already in the air.
:
	lda #>(jtab + data_address - data_start)
	ldy #var_jumpHi
	sta (z80_ix),y		; set jump high.
	lda #<(jtab + data_address - data_start)		; jump table start.
	ldy #var_jumpLo
	sta (z80_ix),y		; set jump low.
	rts

; Jump table.

;jtab:
;	.byte 248,250,252
;	.byte 254,254,255
;	.byte 255,255,0,0
;	.byte 0,1,1,1,2,2
;	.byte 4,6,8,8,8,99

;------------------------------------------------
; Random numbers code.
; Pseudo-random number generator, 8-bit.
;
; Output:
;  RND = random number
;------------------------------------------------

random:
	lda seed		; get last random number.
	asl a
	asl a
	clc
	adc seed
	clc
	adc #$45
	sta seed		; store new seed.
	sta varrnd		; return number in variable.
	rts

;-------------------------------------------------------
; Joystick and keyboard reading routines.
;
; contrl = 0, Keyboard
;          1, JoyKeyb
;          2, JoyMMC
;-------------------------------------------------------

joykey:
	lda contrl 		; control flag.
	cmp #1
	bne :+
	jmp joyjoy 		; read keyboard joystick
:
	cmp #2
	bne :+
	jmp joysin 		; read MMC joystick.
:
; Keyboard controls.

	lda #0		 	; zero reading.
	sta z80_e

	ldy #6	 		; address of last key.
joyke0:
	lda keys + data_address - data_start,y 		; get key from table.
	jsr ktest		; being pressed?
	rol z80_e 		; rotate into reading.

	dey		 	; next key.
	bpl joyke0 		; repeat for all keys.
	jmp joyjo1 		; store the value.

; Keyboard joystick controls.

joyjoy:
	lda #0		 	; zero reading.
	sta z80_e

	ldy #6	 		; address of last key.
joyjo3:
	lda jkeys,y 		; get key from table.
	jsr ktest		; being pressed?
	rol z80_e 		; rotate into reading.

	dey		 	; next key.
	bpl joyjo3 		; repeat for all keys.
joyjo1:
	lda z80_e 		; copy e register to accumulator.
joyjo2:
	sta joyval		; remember value.
	rts

;---------------------------------------------------------------
; Display message.
;
; Input:
;  A = message number
;---------------------------------------------------------------

dmsg:
	tax
	lda #<(msgdat + data_address - data_start)		; pointer to messages.
	sta z80_l
	lda #>(msgdat + data_address - data_start)
	sta z80_h
	jsr getwrd		; get message number.
dmsg3:
	jsr preprt		; pre-printing stuff.
	jsr checkx		; make sure we"re in a printable range.
	lda prtmod		; print mode.
	bne bmsg1		; no, double-height text.
dmsg0:
	lda z80_h		; store string pointer.
	pha
	lda z80_l
	pha

	ldy #0
	lda (z80_hl),y		; fetch byte to display.
	and #127		; remove any end marker.
	cmp #ASCII_NEWLINE
	beq dmsg1
	cmp #ASCII_CR
	beq dmsg1
	jsr pchar		; display character.
	jsr nexpos 		; display position.
	bne dmsg2		; not on a new line.
	jsr nexlin		; next line down.
dmsg2:
	pla			; retrieve string pointer
	sta z80_l
	pla
	sta z80_h

	ldy #0
	lda (z80_hl),y		; fetch last character.
	asl a  			; was it the end?
	bcc :+
	jmp dscor2		; yes, job done.
:
	inc z80_l		; next character to display.
	bne :+
	inc z80_h
:
	jmp dmsg0
dmsg1:
	inc dispy
	lda dispy
	cmp #24
	bcc dmsg4
	lda #0
	sta dispy
dmsg4:
	lda #0
	sta dispx
	jmp dmsg2

;----------------------------------------------------------
; Display message in big text.
;
; Input:
;  HL = string pointer
;----------------------------------------------------------

bmsg1:
	ldy #0
	lda (z80_hl),y 		; get character to display.
	and #127		; only want 7 bits.
	cmp #ASCII_NEWLINE
	beq bmsg2
	cmp #ASCII_CR
	beq bmsg2
	jsr bchar 		; display big char.
bmsg3:
	ldy #0
	lda (z80_hl),y 		; look at last character.
	pha
	inc z80_l 		; next character in list.
	bne :+
	inc z80_h
:
	pla
	asl a  			; was terminator flag set?
	bcc bmsg1		; no, keep going.
:
	rts
bmsg2:
	lda #0
	sta dispx
	inc dispy
	inc dispy
	lda dispy
	cmp #23
	bcc bmsg3
	lda #0
	sta dispy
	jmp bmsg3

;----------------------------------------------------------
; Big character display.
;
; Input:
;  A = character
;----------------------------------------------------------

bchar:
	sta z80_e		; save char in lb
	lda #0
	sta z80_d		; reset hb

	asl z80_e 		; multiply char by 8.
	rol z80_d
	asl z80_e
	rol z80_d
	asl z80_e
	rol z80_d		; de = a*8

	clc			; de = FontPtr + a*8
	lda z80_e
	adc FontPtr 		; address of font.
	sta z80_e
	lda z80_d
	adc FontPtr+1
	sta z80_d

	jsr gprad 		; get screen address.

	ldx #0			; height of character in font.
bchar0:
	ldy #0
	lda (z80_de),y 		; get a bit of the font.

	eor #TxtInvert		; Invert

	sta (scraddr),y
	pha
	jsr nline 		; next line down.
	pla
	sta (scraddr),y
	jsr nline 		; next line down.

	clc
	inc z80_e 		; next line of font.
	bne :+
	inc z80_d
:
	inx
	cpx #8
	bne bchar0
	
	jsr nexpos		; display position.
	bne bchar2 		; not on a new line.
bchar3:
	inc dispy
	jsr nexlin 		; next line check.
bchar2:
	jmp dscor2		; tidy up line and column variables.


;-------------------------------------------------
; Display a character.
;
; Input:
;  A = character
;-------------------------------------------------

achar:
	sta z80_b 		; copy to b.
	jsr preprt 		; get ready to print.
	lda z80_b		; character in accumulator.
	ldx prtmod 		; print mode.
	beq :+
	jmp bchar 		; no, double-height text.
:
	jsr pchar 		; display character.
	jsr nexpos 		; display position.
	beq bchar3		; next line down.
	jmp bchar2 		; tidy up.


;-------------------------------------------------
; Get next print column position.
;-------------------------------------------------

nexpos:
	inc dispx		; move along one position.
	lda dispx 		; get coordinate.
	and #31
	rts 			; return with status in zero flag.

;-------------------------------------------------
; Get next print line position.
;-------------------------------------------------

nexlin:
	inc dispy 		; newline.
	lda dispy		; vertical position.
	cmp #24			; past screen edge?
	bcs :+
	rts			; no, still okay.
:
	lda #0			; restart at top.
	sta dispy
	rts

;--------------------------------------------------------
; Pre-print preliminaries.
;--------------------------------------------------------

preprt:
	lda FontPtr		; font pointer.
	sta grbase		; set up graphics base.
	lda FontPtr+1
	sta grbase+1
prescr:
	lda charx 		; display coordinates.
	sta dispx		; set up general coordinates.
	lda chary
	sta dispy
	rts

;--------------------------------------------------------------
; Get messagenr x in hl
;
; Input:
;  HL = pointer to message list
;  X  = message number.
;--------------------------------------------------------------

getwrd:
	cpx #0
	bne:+ 			; first word in list?
	rts 			; yep, don't search.
:
	ldy #0
getwd0:
	lda (z80_hl),y
	pha
	inc z80_l
	bne :+
	inc z80_h
:
	pla
	cmp #128		; found end?
	bmi getwd0		; no, carry on.
	dex			; until we have right number.
	bne getwd0
	rts

;-----------------------------------------------------------
; Bubble sort.
;-----------------------------------------------------------

bsort:
	lda #NUMSPR - 1		; sprites to swap.
	sta qscnt

	lda #<sprtab 		; sprite table.
	sta z80_x
	lda #>sprtab
	sta z80_i
bsort0:
	ldy #0
	lda (z80_ix),y 		; first sprite type.
	cmp #255 		; is it switched off?
	beq swemp		; yes, may need to switch another in here.

	ldy #TABSIZ
	lda (z80_ix),y 		; check next slot exists.
	cmp #255 		; is it enabled?
	beq bsort2 		; no, nothing to swap.

	ldy #TABSIZ+3
	lda (z80_ix),y 		; fetch next sprite's coordinate.
	ldy #3
	cmp (z80_ix),y 		; compare with this x coordinate.
	bcc bsort1		; next sprite is higher - may need to switch.
bsort2:
	clc
	lda z80_x
	adc #TABSIZ 		; distance to next odd/even entry.
	sta z80_x
	bcc :+
	inc z80_i
:
	dec qscnt 
	bne bsort0		; repeat for remaining sprites.
	rts

bsort1:
	ldy #TABSIZ
	lda (z80_ix),y		; sprite on/off flag.
	cmp #255		; is it enabled?
	beq bsort2		; no, nothing to swap.
	jsr swspr		; swap positions.
	jmp bsort2
swemp:
	ldy #TABSIZ
	lda (z80_ix),y		; next table entry.
	cmp #255		; is that one on?
	beq bsort2		; no, nothing to swap.
	jsr swspr		; swap positions.
	jmp bsort2

; Swap sprites.

swspr:
	lda z80_x		; table address 
	sta z80_e		; copy to de pair.
	sta z80_l		; copy to hl pair.
	lda z80_i
	sta z80_h
	sta z80_d

	clc
	lda z80_l
	adc #TABSIZ		; distance to second entry.
	sta z80_l
	bcc :+
	inc z80_h
:
	lda #TABSIZ		; bytes to swap.
	sta z80_b
	ldy #0
swspr0:
	lda (z80_hl),y		; fetch second byte.
	pha
	lda (z80_de),y 		; fetch first byte.
	sta (z80_hl),y 		; copy to second.
	pla
	sta (z80_de),y 		; copy to first sprite entry.

	inc z80_e 		; next byte.
	bne :+	
	inc z80_d
:
	inc z80_l 		; next byte.
	bne :+
	inc z80_h
:
	dec z80_b
	bne swspr0 		; swap all bytes in table entry.
	rts

;----------------------------------------------------
; Process sprites.
;----------------------------------------------------

pspr:
	lda #NUMSPR		; sprites to process.
	sta sprptr

	lda #<sprtab 		; sprite table.
	sta z80_x
	lda #>sprtab
	sta z80_i
pspr1:
	ldy #0
	lda (z80_ix),y		; fetch sprite type.
	cmp #9 			; within range of sprite types?
	bcs :+
	jsr pspr2 		; yes, process this one.
:
	clc
	lda z80_x
	adc #TABSIZ 		; distance to next odd/even entry.
	sta z80_x
	bcc :+
	inc z80_i		; next sprite.
:
	dec sprptr 		; repeat for remaining sprites.
	bne pspr1
	rts

pspr2:
	lda z80_x 		; store original sprite pointer.
	sta ogptr
	lda z80_i
	sta ogptr+1
	jsr pspr3		; do the routine.
rtorg:
	lda ogptr 		; restore original pointer to sprite.
	sta z80_x
	lda ogptr+1
	sta z80_i
rtorg0:
	rts

pspr3:
	lda #<evtyp0		; sprite type events list.
	sta z80_l
	lda #>evtyp0
	sta z80_h
pspr4:
	lda (z80_ix),y
	asl a			; double accumulator.
	clc
	adc z80_l
	sta z80_l
	bcc :+
	inc z80_h
:
	lda (z80_hl),y
	sta z80_e 		; copy to de.
	pha

	inc z80_l 		; next byte of address.
	bne :+
	inc z80_h
:
	lda (z80_hl),y 		; address high.
	sta z80_d

	pha	 		; swap address into hl.
	lda z80_h
	sta z80_d
	pla
	sta z80_h
	pla
	sta z80_l
	lda z80_l
	sta z80_e
	
	jmp (z80_hl) 		; go there.

; Address of each sprite type's routine.

evtyp0:	.word evnt00
evtyp1:	.word evnt01
evtyp2:	.word evnt02
evtyp3:	.word evnt03
evtyp4:	.word evnt04
evtyp5:	.word evnt05
evtyp6:	.word evnt06
evtyp7:	.word evnt07
evtyp8:	.word evnt08

;--------------------------------------------------------------
; Display sprites.
;
; Input:
;  IX = sprite table
;--------------------------------------------------------------

dspr:
	lda #(NUMSPR/2)		; number of sprites to display.
	sta sprcnt

dspr0:
	ldy #var_Type
	lda (z80_ix),y 		; get sprite type.
	cmp #255 			; is it enabled?
	bne dspr1 			; yes, it needs deleting.

dspr5:
	ldy #var_newType
	lda (z80_ix),y 		; new type.
	cmp #255			; is it enabled?
	beq dspr2			; no, skip
	jmp dspr3 			; yes, it needs drawing.

dspr2:
	ldy #var_newType
	lda (z80_ix),y 		; copy new type.
	ldy #var_Type
	sta (z80_ix),y
	ldy #var_newImage
	lda (z80_ix),y 		; copy new image number.
	ldy #var_Image
	sta (z80_ix),y
	ldy #var_newFrame
	lda (z80_ix),y 		; copy new frame.
	ldy #var_Frame
	sta (z80_ix),y
	ldy #var_newY
	lda (z80_ix),y 		; copy new y.
	ldy #var_Y
	sta (z80_ix),y
	ldy #var_newX
	lda (z80_ix),y 		; copy new x.
	ldy #var_X
	sta (z80_ix),y

	ldy #var_newInk
	lda (z80_ix),y 		; copy new ink.
	ldy #var_Ink
	sta (z80_ix),y 		; copy new ink.

	clc
	lda z80_x
	adc #(TABSIZ*2)		; distance to next odd/even entry.
	sta z80_x
	lda z80_i
	adc #0
	sta z80_i 			; next sprite.
	dec sprcnt
	bne dspr0			; repeat for remaining sprites.
	rts

dspr1:
	; _BEEB clipping code copied from CPC Engine
	ldy #var_Y
	lda (z80_ix), y		; old x coord
	cmp #SpriteMaxY     ; beyond maximum?
	bcs dspr5			; yes, don't delete it.

	ldy #var_newType
	lda (z80_ix),y 		; type of new sprite.
	cmp #255			; is this enabled?
	bne dspr4 			; yes, display both.

dspr6:
	jsr sspria 			; show single sprite.
	jmp dspr2

; Displaying two sprites.  Don't bother redrawing if nothing has changed.

dspr4:
	; _BEEB clipping code copied from CPC Engine
	ldy #var_newY
	lda (z80_ix), y		; old x coord
	cmp #SpriteMaxY     ; beyond maximum?
	bcs dspr6			; yes, don't display it.

	ldy #var_X
	lda (z80_ix),y		; old x.
	ldy #var_newX
	cmp (z80_ix),y 		; compare with new value.
	bne dspr7 			; they differ, need to redraw.

	ldy #var_Y
	lda (z80_ix),y		; old y.
	ldy #var_newY
	cmp (z80_ix),y 		; compare against new value.
	bne dspr7			; they differ, need to redraw.

	ldy #var_Frame
	lda (z80_ix),y 		; old frame.
	ldy #var_newFrame
	cmp (z80_ix),y 		; compare against new value.
	bne dspr7 			; they differ, need to redraw.

	ldy #var_Image
	lda (z80_ix),y 		; old image.
	ldy #var_newImage
	cmp (z80_ix),y 		; compare against new value.
	bne dspr7 			; they differ, need to redraw.
	jmp dspr2			; everything is the same, don't redraw.

dspr7:
	jsr sspric 			; delete old sprite, draw new one simultaneously.
	jmp dspr2

dspr3:
	; _BEEB clipping code copied from CPC Engine
	ldy #var_newY
	lda (z80_ix), y		; old x coord
	cmp #SpriteMaxY     ; beyond maximum?
	bcc :+				; no, continue
	jmp dspr2			; yes, don't display it.
:
	jsr ssprib 			; show single sprite.
	jmp dspr2

;-----------------------------------------
; Get sprite address calculations.
; gspran = new sprite, gsprad = old sprite.
;
; Input:
;  IX = sprite address
;-----------------------------------------

gspran:
	ldy #var_newY
	lda (z80_ix),y 		; new y coordinate.
	sta dispy
	ldy #var_newX
	lda (z80_ix),y 		; new x coordinate.
	sta dispx
	ldy #var_newImage
	lda (z80_ix),y 		; new sprite image.
	jsr gfrm		; fetch start frame for this sprite.

	ldy #0
	lda (z80_hl),y 		; frame in accumulator.
	ldy #var_newFrame
	clc
	adc (z80_ix),y 		; new add frame number.
	jmp gspra0

;-----------------------------------------
; Calculate old sprite address
;
; Input:
;  IX = sprite address
;
; Output:
;  B  = right byte mask
;  C  = left byte mask
;  DE = spriteframe address
;  scraddr = screenaddress(dispx,dispy)
;-----------------------------------------

gsprad:
	ldy #var_Y
	lda (z80_ix),y		; y coordinate.
	sta dispy
	ldy #var_X
	lda (z80_ix),y		; x coordinate.
	sta dispx
	ldy #var_Image
	lda (z80_ix),y 		; sprite image.
	jsr gfrm 		; fetch start frame for this sprite.

	ldy #0
	lda (z80_hl),y 		; frame in accumulator.
	ldy #var_Frame
	clc
	adc (z80_ix),y 		; add frame number.

gspra0:
.if 0
	lsr a	  		; multiply by 128.
	sta z80_d 		; store in d.
	lda #0
	ror a
	sta z80_e 		; got low byte.
.else
	sta z80_e
	lda #0
	sta z80_d

	asl z80_e
	rol z80_d
	asl z80_e
	rol z80_d
	asl z80_e
	rol z80_d
	asl z80_e
	rol z80_d
	asl z80_e
	rol z80_d

.endif

	clc 			; address of play sprites.
	lda z80_e
	adc #<(sprgfx + data_address - data_start)
	sta z80_e
	lda z80_d
	adc #>(sprgfx + data_address - data_start)
	sta z80_d

	lda dispx 		; y coordinate.
	and #6 			; position within byte boundary.
	tax	 		; low byte of table displacement.
	stx sprshft

.if 0
	asl a	  		; multiply by 32.
	asl a  			; already a multiple
	asl a  			; of 2, so just 4
	asl a  			; shifts needed.

	clc 			; add to sprite address.
	adc z80_e
	sta z80_e
	bcc :+
	inc z80_d
:
.endif

	lda spmask,x		 ; pointer to mask table.
	sta z80_c 		; left mask.
	lda spmask+1,x
	sta z80_b 		; right mask.
; Drop into screen address routine.
	jmp scadd

spmask:	.byte 255,0,63,192,15,240,3,252


;-----------------------------------------------------------
; Animates a sprite.
;
; Input:
;  IX = sprite address
;  HL = last sprite address
;-----------------------------------------------------------

animsp:
	and frmno
	beq :+
	rts
:
	ldy #var_newImage
	lda (z80_ix),y		; sprite image
	jsr gfrm		; get frame data.

	inc z80_l		; point to frames.
	bne :+
	inc z80_h
:
	ldy #var_newFrame
	lda (z80_ix),y		; sprite frame.
	clc
	adc #1			; next one along.
	ldy #0
	cmp (z80_hl),y		; reached the last frame?
	bcc anims0		; no, not yet.
	lda #0			; start at first frame.
anims0:
	ldy #var_newFrame
	sta (z80_ix),y		; new frame.
	rts

;--------------------------------------------------------------
; Animate back
;
; Input:
;  IX = sprite address
;  HL = last sprite address
;--------------------------------------------------------------

animbk:
	and frmno
	beq :+
	rts
:
	ldy #var_newImage
	lda (z80_ix),y		; sprite image.
	jsr gfrm		; get frame data.

	inc z80_l 		; point to frames.
	bne :+
	inc z80_h
:
	ldy #var_newFrame
	lda (z80_ix),y 		; sprite frame.
	beq :+
	jmp rtanb0 		; yes, start at end.
:
	ldy #0
	lda (z80_hl),y 		; last sprite.
rtanb0:
	sec
	sbc #1			; next one along.
	jmp anims0		; set new frame.

;--------------------------------------------------------------
; Check for collision with other sprite, strict enforcement.
;
; Input:
;  b		= sprite to test for
;  ix		= current sprite pointer
;
; global:	b
; local:	x,y,hl,de,skptr
; calls:	-
;--------------------------------------------------------------

sktyp:
	lda #<sprtab				; sprite table.
	sta z80_l
	lda #>sprtab
	sta z80_h
numsp2:
	lda #NUMSPR				; number of sprites.
	sta sktptr
sktyp0:
	lda z80_l 				; store pointer to sprite.
	sta skptr
	lda z80_h
	sta skptr+1

	ldy #0
	lda (z80_hl),y 				; get sprite type.
	cmp z80_b				; is it the type we seek?
	beq coltyp				; yes, we can use this one.
:
sktyp1:
	clc
	lda skptr				; retrieve sprite pointer.
	adc #TABSIZ				; size of each entry.
	sta z80_l
	lda skptr+1
	adc #0
	sta z80_h
	dec sktptr					; one less iteration.
	bne sktyp0				; keep going until we find a slot.
:
	lda #0					; default to ROM address - no sprite.
	sta z80_l
	sta z80_h
	sta skptr				; store pointer to sprite.
	sta skptr+1

	clc					; don't return with zero flag set.
	rts 					; didn't find one.

coltyp:
	ldy #0
	lda (z80_ix),y				; current sprite type.
	cmp z80_b				; seeking sprite of same type?
	beq colty1				; yes, need to check we're not detecting ourselves.
colty0:
	ldy #9					; distance to x position in table.
	lda (z80_hl),y				; fetch x coordinate.
	sta z80_e
	dey
	lda (z80_hl),y				; fetch y coordinate.
	sta z80_d

; Drop into collision detection.

colc16:
	ldy #9
	lda (z80_ix),y			 	; x coord.
	sec					; subtract x.
	sbc z80_e
	bcs  colc1a 				; result is positive.
	eor #$ff				; make negative positive.
	clc
	adc #1
colc1a:
	cmp #16 				; within x range?
	bcs sktyp1				; no - they"ve missed.
	sta z80_c				; store difference.

	ldy #8
	lda (z80_ix),y				; y coord.
	sec
	sbc z80_d				; subtract y.
	bcs colc1b				; result is positive.
	eor #$ff				; make negative positive.
	clc
	adc #1
colc1b:
	cmp #16					; within y range?
	bcs sktyp1 				; no - they've missed.
	clc					; add x difference.
	adc z80_c
	cmp #26					; only 5 corner pixels touching?
	bcs :+
	sec
	rts 					; carry set if there's a collision.
:
	jmp sktyp1				; try next sprite in table.
colty1:
	lda z80_x  				; compare the two.
	cmp z80_l
	bne end_col
	lda z80_i
	cmp z80_h
	bne end_col
	jmp sktyp1 				; addresses are identical.
end_col:
	jmp colty0

;-----------------------------------------------------------
; Display number, left aligned
; 
; Input:
;  a		= number
;
; global:	-
; local:	a,y,bc,hl,displ0
; calls:	num2ch,dmsg3
;-----------------------------------------------------------

disply:
	sta z80_a
	lda #<displ0				; display workspace.
	sta z80_c
	lda #>displ0
	sta z80_b
	lda z80_a
	jsr num2ch				; convert accumulator to string.
displ1:
	dec z80_c				; back one character.
	bne :+
	dec z80_b
:
	ldy #0
	lda (z80_bc),y				; fetch digit.
	ora #128				; insert end marker.
	sta (z80_bc),y				; new value.

	lda #<displ0				; display space.
	sta z80_l
	lda #>displ0
	sta z80_h
	jmp dmsg3				; display the string.

displ0:	.byte 0,0,0,13+128

;----------------------------------------------------------------
; Initialise screen.
;
; global:	roomtb,scno
; local:	-
; calls:	tstcs
;----------------------------------------------------------------

initsc:
	lda roomtb 			; whereabouts in the map are we?
	jsr tstsc 			; find displacement.
	cmp #255 			; is it valid?
	beq init_end 			; no, it's rubbish.
	sta scno			; store new room number.
init_end:
	rts

;----------------------------------------------------------------
; Test screen.
;
; global:	-
; local:	x
; calls:	-
;----------------------------------------------------------------

tstsc:
	sta tmproom
	clc
	adc #MAPWID 			; add width in case we"re negative.
	tax 				; add displacement to map data.
	lda mapdat-MAPWID,x 		; find room number there.
	rts

;--------------------------
; Screen left.
;--------------------------

scrl:
	lda roomtb 			; present room table pointer.
	sec
	sbc #1				; room left.
scrl0:
	jsr tstsc			; test screen.
	cmp #255			; is there a screen this way?
	bne :+
	rts				; no, return to loop.
:
	lda tmproom			; restore room displacement.
	sta roomtb			; new room table position.
scrl1:
	jsr initsc 			; set new screen.
	lda #2
	sta restfl 			; set it.
	rts
scrr:
	lda roomtb 			; room table pointer.
	clc
	adc #1				; room right.
	jmp scrl0
scru:
	lda roomtb 			; room table pointer.
	sec
	sbc #MAPWID 			; room up.
	jmp scrl0
scrd:
	lda roomtb 			; room table pointer.
	clc
	adc #MAPWID 			; room down.
	jmp scrl0

;-----------------------------------------
; Jump to new screen.
;-----------------------------------------

nwscr:
	ldx #0				; start of map data.
nwscr0:
	cmp mapdat,x
	beq nwscr1			; have we found a match for screen?
	inx 				; next room.
	cpx #80				; zero room count, 80 to search.
	bne nwscr0			; keep looking.
	rts
nwscr1:
	stx roomtb			; set the map position.
	jmp scrl1			; draw new room.


;----------------------------------------------------------
; Gravity processing.
;----------------------------------------------------------

grav:
	ldy #var_jumpLo
	lda (z80_ix),y			; jump pointer low.
	sta z80_l
	ldy #var_jumpHi
	lda (z80_ix),y			; jump pointer high.
	sta z80_h
	ora z80_l			; merge in low byte.
	bne :+
	rts				; if neither is set, we're not in the air.
:
	ldy #0
	lda (z80_hl),y			; pixels to move.
	sta z80_a
	cmp #99				; reached the end?
	bne grav0			; no, continue.
grav2:
	dec z80_l			; go back to previous value.
	cmp #$ff
	bne :+
	dec z80_h
:
	lda (z80_hl),y			; fetch that from table.
	sta z80_a
grav0:
	inc z80_l			; point to next table entry.
	bne :+
	inc z80_h
:
	lda z80_l
	ldy #var_jumpLo
	sta (z80_ix),y			; store new pointer low.
	lda z80_h
	ldy #var_jumpHi
	sta (z80_ix),y			; store new pointer high.
grav1:
	lda z80_a
	bne :+				; any movement required?
	rts				; no, not this time.
:
	lda z80_a
	cmp #128			; is it up or down?
	bcs gravu			; it's up.
gravd:
	sta z80_b			; set pixels to move.
gravd0:
	jsr cangd			; can we go down?
	bne gravst			; can't move down, so stop.
	ldy #var_newY
	lda (z80_ix),y			; adjust new x coord.
	clc
	adc #1
	sta (z80_ix),y
	dec z80_b
	bne gravd0
	rts
gravu:
	eor #$ff			; flip the sign so it's positive.
	clc
	adc #1
	sta z80_b			; set pixels to move.
gravu0:
	jsr cangu			; can we go up?
	bne ifalls			; can't move up, go down next.
	ldy #var_newY
	lda (z80_ix),y
	sec
	sbc #1
	sta (z80_ix),y			; adjust new x coord.
	dec z80_b
	bne gravu0
	rts
gravst:
	ldy #var_jumpLo
	lda (z80_ix),y			; jump pointer low.
	sta z80_l
	ldy #var_jumpHi
	lda (z80_ix),y			; jump pointer high.
	sta z80_h

	lda #0				; null value in pointer.
	ldy #var_jumpLo
	sta (z80_ix),y			; store new pointer low.
	iny
	sta (z80_ix),y			; store new pointer high.

	ldy #0
	lda (z80_hl),y			; fetch byte from table.
	cmp #99				; is it the end marker?
evftf:
	beq :+				; yes, fallen too far.
	rts
:
	jmp evnt15			; EVENT FELLTOOFAR

;------------------------------------------------
; Initiate fall check.
;------------------------------------------------

ifall:
	ldy #var_jumpLo
	lda (z80_ix),y 			; jump pointer low.
	sta z80_l
	ldy #var_jumpHi
	lda (z80_ix),y 			; jump pointer high.
	sta z80_h			; high byte in accumulator.
	ora z80_l			; merge in low byte.
	beq :+
	rts				; if either is set, we're already in the air.
:
	ldy #var_newX
	lda (z80_ix),y			; y coordinate.
	sta dispx
numsp7:
	ldy #var_newY
	lda (z80_ix),y			; look x coordinate.
	clc
	adc #16				; add 16 pixels.
	sta dispy			; set up test coordinates.
	jsr tstbl			; get map address.
	jsr plchk			; block, platform check.
	beq :+
	rts				; it's solid, don't fall.
:
	inc bufaddr			; look right one cell.
	jsr plchk			; block, platform check.
	beq :+
	rts				; it's solid, don't fall.
:
	lda dispx			; y coordinate.
	and #7				; position straddling block cells.
	beq ifalls			; no more checks needed.
	inc bufaddr			; look to third cell.
	jsr plchk			; block, platform check.
	beq :+
	rts				; it's solid, don't fall.
:
ifalls:
	lda #<(jtab + data_address - data_start)			; jump table start.
	sta z80_l
	lda #>(jtab + data_address - data_start)
	sta z80_h
ifal0:
	inc z80_l			; point to next value.
	bne :+
	inc z80_h
:
	ldy #0
	lda (z80_hl),y			; fetch value.
	beq ifal0			; no, get next value.
	cmp #99				; reached end of table?
	bne :+
	rts				; yes, don't fall.
:
	cmp #128			; is it going up?
	bcs ifal0			; yes, looking for first movement down.

	ldy #var_jumpLo
	lda z80_l
	sta (z80_ix),y 			; set jump low.
	ldy #var_jumpHi
	lda z80_h
	sta (z80_ix),y 			; set jump high.
	rts


;----------------------------------------------------
; Get frame data for a particular sprite.
; Input:
;  a		= framenumer
; Output:
;  hl		= frame address
;
; global:	hl,frmptr
; local:	-
; calls:	-
;----------------------------------------------------

gfrm:
	asl a	 		 	; multiple of 2.
	clc
	adc frmptr 			; frames used by game.
	sta z80_l
	lda frmptr+1
	adc #0
	sta z80_h 			; point to frame start.
	rts

;----------------------------------------------------
; Find sprite list for current room.
;
; global:	hl
; local:	x,y
; calls:	-
;----------------------------------------------------

sprlst:
	lda #<(nmedat + data_address - data_start) 			; list of enemy sprites.
	sta z80_l
	lda #>(nmedat + data_address - data_start)
	sta z80_h
	ldx scno 			; screen number.
	bne sprls2 			; is it the first screen?
	rts 				; yes, don't need to search data.
sprls2:
	ldy #0
sprls1:
	lda (z80_hl),y 			; fetch type of sprite.
	cmp #255			; is it an end marker?
	beq sprls0 			; yes, end of this room.

	clc 				; point to next sprite in list.
	lda z80_l
	adc #NMESIZ
	sta z80_l
	bcc :+
	inc z80_h
:
	jmp sprls1 			; continue until end of room.
sprls0:
	inc z80_l 			; point to start of next screen.s
	bne :+
	inc z80_h
:
	dex
	bne sprls1 			; continue until room found.
	rts


;----------------------------------------------------
; Clear all but a single player sprite.
;
; global:	-
; local:	x,y,ix
; calls:	-
;----------------------------------------------------

nspr:
	lda #NUMSPR			; sprite slots in table.
	sta sprcnt
	lda #<sprtab 			; sprite table.
	sta z80_x
	lda #>sprtab
	sta z80_i
nspr0:
	ldy #0 				; fetch sprite type.
	lda (z80_ix),y 			; is it a player?
	beq nspr1 			; yes, keep this one.

	lda #255
	ldy #0 				; fetch sprite type.
	sta (z80_ix),y 			; delete sprite.
	ldy #5
	sta (z80_ix),y 			; remove next type.

	clc	 			; next sprite.
	lda z80_x
	adc #TABSIZ 			; distance to next odd/even entry.
	sta z80_x
	bcc :+
	inc z80_i
:
	dec sprcnt	 			; one less space in the table.
	bne nspr0
	rts
nspr1:
	lda #255
	ldy #0
	sta (z80_ix),y 			; delete sprite.

	clc	 			; point to next sprite.
	lda z80_x
	adc #TABSIZ 			; distance to next odd/even entry.
	sta z80_x
	bcc :+
	inc z80_i
:
	dec sprcnt	 			; one less to do.
	bne nspr2
	rts
nspr2:
	lda #255
	ldy #0
	sta (z80_ix),y 			; delete sprite.
	ldy #5
	sta (z80_ix),y 			; remove next type.

	clc	 			; next sprite.
	lda z80_x
	adc #TABSIZ 			; distance to next odd/even entry.
	sta z80_x
	bcc :+
	inc z80_i
:
	dec sprcnt	 			; one less space in table.
	bne nspr2
	rts

;----------------------------------------------------------
; Two initialisation routines.
; Initialise sprites - copy everything from list to table.
;
; global:	-
; local:	x,y,ix
; calls:	cpsp
;----------------------------------------------------------

ispr:
	lda #NUMSPR			; sprite slots in table.
	sta sprcnt
	lda #<sprtab			; sprite table.
	sta z80_x
	lda #>sprtab
	sta z80_i
ispr2:
	ldy #0
	lda (z80_hl),y 			; fetch byte.
	cmp #255 			; is it an end marker?
	bne :+
	rts 				; yes, no more to do.
:
ispr1:
	ldy #0
	lda (z80_ix),y 			; fetch sprite type.
	cmp #255 			; is it enabled yet?
	bne ispr4			; yes, try another slot.

	ldy #5
	lda (z80_ix),y		 	; next type.
	cmp #255 			; is it enabled yet?
	beq ispr3 			; no, process this one.
ispr4:
	clc 				; next sprite.
	lda z80_x
	adc #TABSIZ		 	; distance to next odd/even entry.
	sta z80_x
	bcc :+
	inc z80_i
:
	dec sprcnt
	bne ispr1 			; repeat for remaining sprites.
	rts  				; no more room in table.
ispr3:
	jsr cpsp			; initialise a sprite.
	dec sprcnt			; one less space in the table.
	bne ispr2
	rts


;-----------------------------------------------------------------------
; Initialise sprites - but not player, we're keeping the old one.
;
; global:	-
; local:	x,y,ix
; calls:	cpsp
;-----------------------------------------------------------------------

kspr:
	ldx #NUMSPR			; sprite slots in table.
	lda #<sprtab 			; sprite table.
	sta z80_x
	lda #>sprtab
	sta z80_i
kspr2:
	ldy #0
	lda (z80_hl),y 			; fetch byte.
	cmp #255 			; is it an end marker?
	bne :+
	rts 				; yes, no more to do.
:
	cmp #0
	bne kspr1 			; no, add to table as normal.

	clc 				; next sprite.
	lda z80_l
	adc #NMESIZ		 	; distance to next odd/even entry.
	sta z80_l
	bcc :+
	inc z80_h
:
	jmp kspr2
kspr1:
	ldy #0 				; fetch sprite type.
	lda (z80_ix),y
	cmp #255 			; is it enabled yet?
	bne kspr4 			; yes, try another slot.

	ldy #5 				; next type.
	lda (z80_ix),y
	cmp #255 			; is it enabled yet?
	beq kspr3 			; no, process this one.
kspr4:
	clc 				; next sprite.
	lda z80_x
	adc #TABSIZ		 	; distance to next odd/even entry.
	sta z80_x
	bcc :+
	inc z80_i
:
	dex	 			; repeat for remaining sprites.
	bne kspr1
	rts  				; no more room in table.
kspr3:
	jsr cpsp 			; copy sprite to table.
	dex	 			; one less space in the table.
	bne kspr2
	rts

;----------------------------------------------
; Copy sprite from list to table.
;
; global:	hl,ix
; local:	y
; calls:	evnt09
;----------------------------------------------

cpsp:
	ldy #0					; fetch byte from table.
	lda (z80_hl),y
	; y=var_Type
	sta (z80_ix),y			; set up type.
	ldy #var_newType
	sta (z80_ix),y 			; set up type.

	inc z80_l 				; move to next byte.
	bne :+
	inc z80_h
:
	ldy #0 					; fetch byte from table.
	lda (z80_hl),y
	ldy #var_newImage
	sta (z80_ix),y			; set up image.

	inc z80_l 				; move to next byte.
	bne :+
	inc z80_h
:
	ldy #0
	lda (z80_hl),y 			; fetch byte from table.
	ldy #var_newY
	sta (z80_ix),y 			; set up coordinate.

	lda #200 				; set initial coordinate off screen.
	ldy #var_Y
	sta (z80_ix),y

	inc z80_l 				; move to next byte.
	bne :+
	inc z80_h
:
	ldy #0 					; fetch byte from table.
	lda (z80_hl),y
	ldy #var_newX
	sta (z80_ix),y 			; set up coordinate.

	inc z80_l 				; move to next byte.
	bne :+
	inc z80_h
:
	lda #0					; zeroes in accumulator.
	ldy #var_newFrame 		; reset frame number.
	sta (z80_ix),y
	ldy #var_Direction 		; reset direction.
	sta (z80_ix),y
	ldy #var_jumpLo			; reset jump pointer low.
	sta (z80_ix),y
	ldy #var_jumpHi	 		; reset jump pointer high.
	sta (z80_ix),y

	lda #7					; default white ink - spriteink?
	ldy #var_Ink
	sta (z80_ix),y
	ldy #var_newInk
	sta (z80_ix),y

	lda #255 				; reset data pointer to auto-restore.
	ldy #var_dataHi
	sta (z80_ix),y
evis0:
	lda z80_i
	pha
	lda z80_x
	pha
	lda z80_h
	pha
	lda z80_l
	pha

	jsr evnt09 				; perform event.

	pla
	sta z80_l
	pla
	sta z80_h
	pla
	sta z80_x
	pla
	sta z80_i

	clc
	lda z80_x 			; distance to next odd/even entry.
	adc #TABSIZ		 	; next sprite.
	sta z80_x
	bcc :+
	inc z80_i
:
	rts


;-------------------------------------
; Clear the play area window.
;-------------------------------------

clw:
	lda wintop			; get coordinates of window.
	sta dispy			; put into dispx for calculation.
	lda winlft
	sta dispx

	lda winhgt			; height of window.
	sta rrow			; copy to b register.
clw3:
	lda winwid 			; width of window.
	sta rcol
clw2:
	jsr gprad 			; get print address.
	lda #0				; zero byte to write.
	ldy #7				; pixel height of each cell.
clw1:
	sta (scraddr),y 	; copy to screen.
	dey					; next screen row down.
	bpl clw1

	inc dispx			; next column.
	dec rcol			; one less to do.
	bne clw2			; repeat for remaining columns.

	lda winlft			; get left edge.
	sta dispx 			; reset x.
	inc dispy 			; next line down.

	dec rrow
	bne clw3			; repeat down the screen.

	lda wintop			; get coordinates of window.
	sta chary			; put into display position.
	lda winlft
	sta charx
	rts


;----------------------------------------------------------
; Effects code.
; Ticker routine is called 25 times per second.
;
; HL = txtscr = left text screen address
; DE = txtscr+txtwid-1 = right text screen address
; BC = txtpos = text scroller position
;
;----------------------------------------------------------

.if sflag
scrly:
	rts
	.word txtscr         	; get left screen address.
	sta scr_l
	lda txtscr+1
	sta scr_l+1
	sta scr_r+1
	
	stx xtmp

	clc         		; get right screen address.
	lda scr_l
	adc txtwid
	sta scr_r
	dec scr_r
scrly1:
	ldy txtwid		; set txtwide
	dey
	clc
scrly0:
	lda (scr_l),y		; scroll 1 line
	rol a
	sta (scr_l),y
	dey
	bpl scrly0

	clc			; point to next line
	lda scr_l
	adc #32
	sta scr_l
	bcc scrly1		; repeat 8 times

	lda txtpos 		; get text pointer.
	sta scr_txt
	lda txtpos+1
	sta scr_txt+1

	ldy #0
	lda (scr_txt),y 		; find character we're displaying.
	and #127 		; remove end marker bit if applicable.
	cmp #ASCII_NEWLINE			; is it newline?
	bne scrly5 		; no, it's okay.
	cmp #ASCII_CR			; is it newline?
	bne scrly5 		; no, it's okay.
	lda #32			; convert to a space instead.
scrly5:
	sta fntaddr		; calculate char address
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
	sta scrly3+1		; that's the low byte.
	lda fntaddr+1
	adc FontPtr+1
	sta scrly3+2		; add displacement.

	ldx #0
scrly3:
	lda $3333,x		; get image of char line.
	and txtbit
	beq scrly2		; don't plot pixel
	ldy scrline,x
	lda (scr_r),y
	clc
	ora #1
	sta (scr_r),y		; plot pixel
scrly2:
	inx			; next line of char.
	cpx #8
	bne scrly3

	lsr txtbit		; bit of text to display.
	bcs :+
	rts
:
	ldy #0
	lda (scr_txt),y 	; what was the character?
	asl a	  		; end of message?
	bcs scrly4
	inc txtpos
	bne :+
	inc txtpos+1
:
	jmp scrly6 		; not yet - continue.
scrly4:
	lda txtini 		; start of scrolling message.
	sta txtpos
	lda txtini+1
	sta txtpos+1
scrly6:
	lda #128
	sta txtbit
	ldx xtmp
	rts

scrline:	.byte $00,$20,$40,$60,$80,$a0,$c0,$e0

;-------------------------------------------------------
; Entry TICKER command
;
; Entry:
;  z80_b = message nr
;  z80_c = width
;-------------------------------------------------------

iscrly:
	jsr prescr 		; set up display position.

	lda #<(msgdat + data_address - data_start) 		; text messages.
	sta z80_l
	lda #>(msgdat + data_address - data_start)
	sta z80_h

	lda z80_c 		; width.
	sec
	sbc #1			; subtract one.
	cmp #32 		; is it between 1 and 32?
	bcc :+
	lda #$60
	jmp iscrl0		; no, disable messages.
:
	ldx z80_b		; message number.
	jsr getwrd 		; find message start.

	lda z80_l		; set initial text position.
	sta txtini
	lda z80_h
	sta txtini+1

	lda #$ad		; code for lda adrr
iscrl0:
	sta scrly		; enable/disable scrolling routine.

	jsr prescr 		; set up display position.
	jsr gprad 		; get print address.

	lda scraddr 		; set text screen address.
	sta txtscr
	lda scraddr+1
	sta txtscr+1

	lda z80_c		; width.
	sta txtwid		; set width in working storage.

	lda #128 		; start with leftmost bit.
	sta txtbit

	jmp scrly4
.endif

;------------------------------------------------------------------
; Dig routine, conditional assembly depending on dflag
;------------------------------------------------------------------
.if dflag
dig:
	and #3
	beq digr		; dig right
	cmp #1
	beq digl		; dig left
	cmp #2
	beq digd		; dig down

; Dig up.

digu:				; dig up
	ldy #var_newY
	lda (z80_ix),y
	sec
	sbc #2
	sta dispy		; set y

	iny
	lda (z80_ix),y
	sta dispx		; set x
	jmp digv

; Dig down.

digd:
	ldy #var_newX
	lda (z80_ix),y
	sta dispx		; set x

	dey
	clc
	lda (z80_ix),y
	adc #16
	sta dispy		; set y
	jmp digv

; Dig left.

digl:
	ldy #var_newY
	lda (z80_ix),y
	sta dispy		; set y

	iny
	lda (z80_ix),y
	sec
	sbc #2			; x=x-2
	sta dispx		; set x
	jmp digh

; Dig right.

digr:
	ldy #var_newY
	lda (z80_ix),y
	sta dispy		; set y

	iny
	lda (z80_ix),y
	clc
	adc #16
	sta dispx		; set x+16
	jmp digh

; Vertical digging

digv:
	jsr tstbl		; check blocktype in MAP
	jsr fdchk		; test if FODDER

	clc
	lda dispx		; look 1 cell down
	adc #8
	sta dispx
	jsr tstbl		; check blocktype in MAP
	jsr fdchk
	lda dispx
	and #7
	bne :+
	rts
:
	clc
	lda dispx		; look 1 cell down
	adc #8
	sta dispx
	jsr tstbl		; check blocktype in MAP
	jmp fdchk

; Horizontal digging

digh:
	jsr tstbl		; check blocktype in MAP
	jsr fdchk		; test if FODDER

	clc
	lda dispy		; look 1 cell down
	adc #8
	sta dispy
	jsr tstbl		; check blocktype in MAP
	jsr fdchk
	lda dispy
	and #7
	bne :+
	rts
:
	clc
	lda dispy		; look 1 cell down
	adc #8
	sta dispy
	jsr tstbl		; check blocktype in MAP
	jmp fdchk

digcnt:	.byte 0

.endif

;------------------------------------------------------------------
; Sprite table 
;------------------------------------------------------------------


; ix+0  = type.
; ix+1  = sprite image number.
; ix+2  = frame.
; ix+3  = y coord.
; ix+4  = x coord.

; ix+5  = new type.
; ix+6  = new image number.
; ix+7  = new frame.
; ix+8  = new y coord.
; ix+9  = new x coord.

; ix+10 = direction.
; ix+11 = parameter 1.
; ix+12 = parameter 2.
; ix+13 = jump pointer low.
; ix+14 = jump pointer high.
; ix+15 = data pointer low.
; ix+16 = data pointer high.

; ix+17 = ink
; ix+18 = new ink

; block NUMSPR * TABSIZ,255
; sprtab NUMSPR*TABSIZ,255

ssprit:	.byte 255,255,255,255,255,255,255,0,192,120,0,0,0,255,255,255,255,7,7
roomtb:	.byte 7                      ; start room map offset.

; User routine.  Put your own code in here to be called with USER instruction.
; if USER has an argument it will be passed in the accumulator.

user:
	rts

; Everything below here will be generated by the editors.

script_start:

WINDOWTOP = 1
WINDOWLFT = 1
WINDOWHGT = 17
WINDOWWID = 30 ;面面面面面面面面面面面面
MAPWID = 13
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255
mapdat:
        .byte 255,255,0,1,2,3,4,5,6,7,8,255,255,255,255,255,9,255,255,255,10,255,11,12,255,255,255,255,255,13,255,255,14,15,255,16,17,18,255,255,255,255,19,20,21,22,23,24,25,26,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,27,28,29,30,32,31,255,255,255,255,255,255
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255
stmap:  .byte 66

evnt00:
        lda #0
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a00434
        lda #255
        sta varo
        ; lda #255
        sta varn
        lda joyval
        and #1
        beq :+
        jmp a00067
:
        lda #0
        sta varo
a00067: lda joyval
        and #2
        beq :+
        jmp a00092
:
        lda #1
        sta varo
a00092: lda joyval
        and #16
        beq :+
        jmp a00118
:
        lda #1
        sta varn
a00118: lda #0
        cmp vard
        beq *+5
        jmp a00143
        lda #1
        sta varo
a00143: lda #0
        cmp varf
        beq *+5
        jmp a00168
        ; lda #0
        sta varo
a00168: lda #0
        cmp varh
        beq *+5
        jmp a00193
        lda #1
        sta varn
a00193: lda #0
        cmp vard
        beq *+5
        jmp a00243
        ; lda #0
        cmp varh
        beq *+5
        jmp a00243
        lda #1
        sta varo
        ; lda #1
        sta varn
a00243: lda #1
        cmp vard
        beq *+5
        jmp a00293
        lda #0
        cmp varh
        beq *+5
        jmp a00293
        ; lda #0
        sta varo
        lda #1
        sta varn
a00293: lda #2
        cmp varc
        beq *+5
        jmp a00318
        lda #1
        sta varo
a00318: lda #1
        cmp varc
        beq *+5
        jmp a00343
        lda #0
        sta varo
a00343: lda #16
        cmp varc
        beq *+5
        jmp a00368
        lda #1
        sta varn
a00368: lda #17
        cmp varc
        beq *+5
        jmp a00401
        lda #0
        sta varo
        lda #1
        sta varn
a00401: lda #18
        cmp varc
        beq *+5
        jmp a00434
        lda #1
        sta varo
        ; lda #1
        sta varn
a00434: lda #17
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a00568
        lda #1
        iny; ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a00492
        lda #50
        asl a
        sta sndtyp
a00492: lda #3
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a00526
        lda #30
        asl a
        sta sndtyp
a00526: lda #5
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a00556
        lda #1
        sta deadf
a00556: lda #0
        jsr animsp
        rts
a00568: lda #27
        cmp scno
        beq *+5
        jmp a01159
        lda #50
        ldy #12
        cmp (z80_ix),y
        bcs *+5
        jmp a00639
        ; ldy #12
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #12
        sta (z80_ix),y
        jmp a00722
a00639: lda #0
        ldy #12
        sta (z80_ix),y
        lda #6
        dey; ldy #11
        cmp (z80_ix),y
        bcs *+5
        jmp a00708
        ; ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
        jmp a00722
a00708: lda #0
        ldy #11
        sta (z80_ix),y
a00722: lda #7
        sta chary
        lda #11
        sta charx
        lda #6
        jsr dmsg
        lda #9
        sta chary
        lda #11
        sta charx
        lda #7
        jsr dmsg
        lda #16
        sta chary
        lda #6
        sta charx
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp a00820
        tya; lda #11
        jsr dmsg
a00820: lda #1
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp a00850
        lda #12
        jsr dmsg
a00850: lda #2
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp a00881
        lda #13
        jsr dmsg
a00881: lda #3
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp a00911
        lda #29
        jsr dmsg
a00911: lda #4
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp a00950
        lda #7
        sta charx
        lda #23
        jsr dmsg
a00950: lda #5
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp a00989
        lda #7
        sta charx
        lda #24
        jsr dmsg
a00989: lda #6
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp a01028
        lda #7
        sta charx
        lda #25
        jsr dmsg
a01028: lda #7
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp a01067
        ; lda #7
        sta charx
        lda #26
        jsr dmsg
a01067: lda #1
        cmp varn
        beq *+5
        jmp a01154
        lda #18
        sta scno
        jsr nwscr
        lda #31
        sta varm
        lda #100
        asl a
        sta sndtyp
        lda #50
        jsr delay
        lda #116
        ldy #8
        sta (z80_ix),y
        lda #120
        iny; ldy #9
        sta (z80_ix),y
a01154: jmp a01353
a01159: jsr skobj
        sta varobj
        lda #255
        cmp varobj
        bne *+5
        jmp a01280
        ldy #8
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #8
        sta (z80_ix),y
        lda varobj
        jsr getob
        lda varb
        clc
        adc #1
        sta varb
        lda #21
        sta chary
        lda #27
        sta charx
        lda varb
        jsr disply
        lda #90
        asl a
        sta sndtyp
        jmp a01353
a01280: lda #16
        sta z80_b
        jsr sktyp
        bcs :+
        jmp a01353
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
a01353: lda #2
        sta z80_b
        jsr sktyp
        bcs :+
        jmp a01602
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        ldy #8
        lda (z80_ix),y
        sta varp
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda varp
        ; ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp a01602
        ; ldy #8
        lda (z80_ix),y
        sta z80_c
        lda varp
        sec
        sbc z80_c
        sta varp
        lda #13
        cmp varp
        bcs *+5
        jmp a01602
        jsr gravst
        lda #1
        cmp varn
        beq *+5
        jmp a01540
        jsr cangu
        beq :+
        jmp a01535
:
        jsr jump
a01535: jmp a01602
a01540: jsr cangu
        beq :+
        jmp a01580
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp a01602
a01580: ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a01602: jsr cangu
        beq :+
        jmp a01620
:
        jmp a01646
a01620: ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
a01646: jsr cangd
        beq :+
        jmp a01678
:
        lda #0
        ldy #7
        sta (z80_ix),y
        jmp a01764
a01678: lda #0
        jsr animsp
        lda #1
        ldy #7
        cmp (z80_ix),y
        bcs *+5
        jmp a01764
        lda #27
        cmp scno
        beq *+5
        jmp a01730
        jmp a01764
a01730: lda #25
        asl a
        sta sndtyp
        lda #<1
        sta z80_l
        lda #>1
        sta z80_h
        jsr addsc
a01764: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01885
:
        jsr gravst
        jsr cangu
        beq :+
        jmp a01836
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp a01881
a01836: ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a01881: jmp a01912
a01885: jsr ifall
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a01912: lda #230
        ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp a02021
        lda #21
        cmp varb
        beq *+5
        jmp a01995
        lda #17
        cmp scno
        beq *+5
        jmp a01986
        lda #28
        sta scno
        jsr nwscr
        jmp a01990
a01986: jsr scrr
a01990: jmp a01999
a01995: jsr scrr
a01999: lda #18
        ldy #9
        sta (z80_ix),y
        lda #0
        sta vark
a02021: lda #8
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp a02103
        lda #28
        cmp scno
        beq *+5
        jmp a02077
        lda #17
        sta scno
        jsr nwscr
        jmp a02081
a02077: jsr scrl
a02081: lda #220
        ldy #9
        sta (z80_ix),y
        lda #0
        sta vark
a02103: lda #121
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a02151
        jsr scrd
        lda #15
        ldy #8
        sta (z80_ix),y
        lda #0
        sta vark
a02151: lda #9
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp a02199
        jsr scru
        lda #120
        ldy #8
        sta (z80_ix),y
        lda #0
        sta vark
a02199: lda #0
        cmp varo
        beq *+5
        jmp a02252
        jsr cangr
        beq :+
        jmp a02252
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a02252: lda #1
        cmp varo
        beq *+5
        jmp a02304
        jsr cangl
        beq :+
        jmp a02304
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a02304: lda #1
        cmp varn
        beq *+5
        jmp a02325
        jsr jump
a02325: lda joyval
        and #32
        beq :+
        jmp a02347
:
        jsr prskey
a02347: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a02405
:
        lda #1
        cmp vark
        beq *+5
        jmp a02405
        lda #17
        ldy #6
        sta (z80_ix),y
a02405: jmp grav
.out .sprintf("EVENT 0 SIZE = %d", (* - evnt00))

evnt01:
        lda #17
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp b00825
        lda #1
        iny; ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp b00760
        lda #56
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b00126
        lda #34
        sta varobj
        lda varobj
        jsr getob
        ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda varobj
        jsr drpob
b00126: lda #72
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b00205
        lda #35
        sta varobj
        lda varobj
        jsr getob
        ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda varobj
        jsr drpob
b00205: lda #88
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b00284
        lda #36
        sta varobj
        lda varobj
        jsr getob
        ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda varobj
        jsr drpob
b00284: lda #104
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b00363
        lda #37
        sta varobj
        lda varobj
        jsr getob
        ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda varobj
        jsr drpob
b00363: lda #128
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b00443
        lda #38
        sta varobj
        lda varobj
        jsr getob
        ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda varobj
        jsr drpob
b00443: lda #144
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b00522
        lda #39
        sta varobj
        lda varobj
        jsr getob
        ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda varobj
        jsr drpob
b00522: lda #160
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b00601
        lda #40
        sta varobj
        lda varobj
        jsr getob
        ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda varobj
        jsr drpob
b00601: lda #176
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b00680
        lda #41
        sta varobj
        lda varobj
        jsr getob
        ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda varobj
        jsr drpob
b00680: lda #192
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b00760
        lda #42
        sta varobj
        lda varobj
        jsr getob
        ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda varobj
        jsr drpob
b00760: lda #4
        ldy #7
        cmp (z80_ix),y
        bcs *+5
        jmp b00813
        ; ldy #7
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #7
        sta (z80_ix),y
        jmp b00822
b00813: lda #200
        jsr delay
b00822: rts
b00825: lda #14
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp b01288
        lda #4
        iny; ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp b01257
        lda #60
        asl a
        sta sndtyp
        lda #28
        cmp scno
        beq *+5
        jmp b00938
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #29
        sta scno
        jsr nwscr
        lda #10
        jsr delay
        rts
b00938: lda #8
        cmp scno
        beq *+5
        jmp b01042
        lda #30
        ldy #8
        sta (z80_ix),y
        lda #80
        iny; ldy #9
        sta (z80_ix),y
        lda #15
        sta scno
        jsr nwscr
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #10
        jsr delay
        lda #0
        sta z80_c
        ; lda #0
        sta z80_b
        jsr spawn
        rts
b01042: lda #19
        cmp scno
        beq *+5
        jmp b01147
        lda #32
        ldy #8
        sta (z80_ix),y
        lda #160
        iny; ldy #9
        sta (z80_ix),y
        lda #16
        sta scno
        jsr nwscr
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #10
        jsr delay
        lda #0
        sta z80_c
        ; lda #0
        sta z80_b
        jsr spawn
        rts
b01147: lda #16
        cmp scno
        beq *+5
        jmp b01252
        lda #32
        ldy #8
        sta (z80_ix),y
        lda #224
        iny; ldy #9
        sta (z80_ix),y
        lda #19
        sta scno
        jsr nwscr
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #10
        jsr delay
        lda #0
        sta z80_c
        ; lda #0
        sta z80_b
        jsr spawn
        rts
b01252: jmp b01283
b01257: ldy #7
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #7
        sta (z80_ix),y
b01283: jmp b01323
b01288: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b01323
:
        lda #255
        ldy #5
        sta (z80_ix),y
b01323: rts
.out .sprintf("EVENT 1 SIZE = %d", (* - evnt01))

evnt02:
        lda #13
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00382
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00382
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        ldy #9
        lda (z80_ix),y
        sta varp
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #5
        sta z80_c
        dey; ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00347
:
        lda #5
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        iny; ldy #9
        lda (z80_ix),y
        sta z80_c
        lda varp
        sec
        sbc z80_c
        sta varp
        lda #2
        cmp varp
        bcc *+5
        jmp c00243
        jmp c00312
c00243: lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        jsr gravst
        lda #14
        ldy #6
        sta (z80_ix),y
        lda #1
        dey; ldy #5
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
c00312: ldy #9
        lda (z80_ix),y
        sta z80_c
        lda varp
        clc
        adc z80_c
        sta varp
        jmp c00382
c00347: lda #5
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
c00382: lda #4
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00628
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c00570
        lda #15
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        jsr cangu
        beq :+
        jmp c00516
:
        lda #14
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp c00565
c00516: lda #15
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #1
        ldy #10
        sta (z80_ix),y
c00565: jmp c00628
c00570: jsr cangd
        beq :+
        jmp c00614
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
        jmp c00628
c00614: lda #0
        ldy #10
        sta (z80_ix),y
c00628: lda #8
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c01073
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c00876
        lda #2
        sta z80_c
        dey; ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #16
        sta z80_c
        dey; ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp c00785
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp c00799
c00785: lda #1
        ldy #10
        sta (z80_ix),y
c00799: lda #2
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #16
        sta z80_c
        dey; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp c01073
c00876: lda #2
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #16
        sta z80_c
        dey; ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp c00988
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp c01001
c00988: lda #0
        ldy #10
        sta (z80_ix),y
c01001: lda #2
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #16
        sta z80_c
        dey; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
c01073: lda #2
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c01584
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c01184
:
        lda #1
        cmp vark
        beq *+5
        jmp c01184
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #17
        ldy #6
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
c01184: lda #1
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp c01233
        lda #0
        jsr animsp
        lda #0
        ldy #11
        sta (z80_ix),y
        jmp c01260
c01233: ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
c01260: lda #1
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp c01584
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c01447
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        jsr cangu
        beq :+
        jmp c01393
:
        lda #7
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp c01442
c01393: lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #1
        ldy #10
        sta (z80_ix),y
c01442: jmp c01584
c01447: lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp c01535
:
        lda #7
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp c01584
c01535: lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #0
        ldy #10
        sta (z80_ix),y
c01584: rts
.out .sprintf("EVENT 2 SIZE = %d", (* - evnt02))

evnt03:
        lda #3
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00317
        lda #0
        jsr animsp
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00317
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        ldy #8
        lda (z80_ix),y
        sta varp
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        ; ldy #8
        lda (z80_ix),y
        sta varq
        lda varq
        sta z80_c
        lda varp
        sec
        sbc z80_c
        sta varp
        lda #9
        cmp varp
        bcs *+5
        jmp d00232
        lda #1
        cmp vark
        beq *+5
        jmp d00228
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #17
        ldy #6
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
d00228: jmp d00317
d00232: lda #16
        cmp varp
        bcc *+5
        jmp d00317
        lda #1
        cmp vark
        beq *+5
        jmp d00317
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #17
        ldy #6
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
d00317: lda #12
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00652
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp d00535
        ; lda #0
        jsr animsp
        lda #2
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp d00406
        lda #0
        ; ldy #7
        sta (z80_ix),y
d00406: jsr cangu
        beq :+
        jmp d00455
:
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
d00455: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp d00487
:
        jmp d00535
d00487: lda #1
        ldy #11
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
d00535: lda #1
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp d00652
        lda #2
        ldy #7
        sta (z80_ix),y
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp d00638
:
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp d00652
d00638: lda #0
        ldy #11
        sta (z80_ix),y
d00652: rts
.out .sprintf("EVENT 3 SIZE = %d", (* - evnt03))

evnt04:
        lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        jsr animsp
        lda #5
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00213
        jsr cangl
        beq :+
        jmp e00166
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #0
        cmp varrnd
        beq *+5
        jmp e00161
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
e00161: jmp e00213
e00166: lda #6
        ldy #6
        sta (z80_ix),y
        lda #27
        cmp scno
        beq *+5
        jmp e00201
        jmp e00213
e00201: lda #15
        asl a
        sta sndtyp
e00213: lda #6
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00385
        jsr cangr
        beq :+
        jmp e00337
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #0
        cmp varrnd
        beq *+5
        jmp e00333
        ; ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
e00333: jmp e00385
e00337: lda #5
        ldy #6
        sta (z80_ix),y
        lda #27
        cmp scno
        beq *+5
        jmp e00372
        jmp e00385
e00372: lda #15
        asl a
        sta sndtyp
e00385: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00669
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        ldy #8
        lda (z80_ix),y
        sta varp
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        ; ldy #8
        lda (z80_ix),y
        sta varq
        lda varq
        cmp varp
        bcs *+5
        jmp e00561
        lda #1
        cmp vark
        beq *+5
        jmp e00556
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #17
        ldy #6
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
e00556: jmp e00669
e00561: lda varq
        sta z80_c
        lda varp
        sec
        sbc z80_c
        sta varp
        lda #8
        cmp varp
        bcs *+5
        jmp e00669
        lda #1
        cmp vark
        beq *+5
        jmp e00669
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #17
        ldy #6
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
e00669: rts
.out .sprintf("EVENT 4 SIZE = %d", (* - evnt04))

evnt05:
        lda #1
        ldy #11
        cmp (z80_ix),y
        bcs *+5
        jmp f00064
        lda #0
        jsr animsp
        ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
        jmp f00077
f00064: lda #0
        ldy #11
        sta (z80_ix),y
f00077: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp f00100
f00100: lda #1
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp f00122
f00122: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00211
:
        lda #1
        cmp vark
        beq *+5
        jmp f00211
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #17
        ldy #6
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
f00211: rts
.out .sprintf("EVENT 5 SIZE = %d", (* - evnt05))

evnt06:
        lda #9
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp g00255
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp g00086
:
        lda #10
        ldy #6
        sta (z80_ix),y
g00086: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #3
        ldy #11
        cmp (z80_ix),y
        bcs *+5
        jmp g00242
        lda #0
        jsr animsp
        ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
        jsr cangr
        beq :+
        jmp g00224
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
        jmp g00237
g00224: lda #10
        ldy #6
        sta (z80_ix),y
g00237: jmp g00255
g00242: lda #0
        ldy #11
        sta (z80_ix),y
g00255: lda #10
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp g00509
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp g00340
:
        lda #9
        ldy #6
        sta (z80_ix),y
g00340: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #3
        ldy #11
        cmp (z80_ix),y
        bcs *+5
        jmp g00495
        lda #0
        jsr animsp
        ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
        jsr cangl
        beq :+
        jmp g00477
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #9
        sta (z80_ix),y
        jmp g00491
g00477: lda #9
        ldy #6
        sta (z80_ix),y
g00491: jmp g00509
g00495: lda #0
        ldy #11
        sta (z80_ix),y
g00509: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00690
:
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00655
:
        lda #1
        cmp vark
        beq *+5
        jmp g00655
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #17
        ldy #6
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
g00655: lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
g00690: rts
.out .sprintf("EVENT 6 SIZE = %d", (* - evnt06))

evnt07:
        lda #16
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h00196
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        lda #160
        iny; ldy #8
        sta (z80_ix),y
        lda #176
        iny; ldy #9
        sta (z80_ix),y
        lda #7
        ldy #11
        cmp (z80_ix),y
        bcs *+5
        jmp h00168
        lda #60
        sta z80_c
        ; ldy #11
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #11
        sta (z80_ix),y
        lda #59
        sta z80_c
        ; ldy #11
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #11
        sta (z80_ix),y
        jmp h00196
h00168: lda #0
        ldy #11
        sta (z80_ix),y
        lda #255
        ldy #5
        sta (z80_ix),y
h00196: lda #1
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h00977
        lda #27
        cmp scno
        beq *+5
        jmp h00672
        tya; lda #6
        ldy #12
        cmp (z80_ix),y
        bcs *+5
        jmp h00290
        ; ldy #12
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #12
        sta (z80_ix),y
        jmp h00304
h00290: lda #1
        ldy #12
        sta (z80_ix),y
h00304: ldy #12
        lda (z80_ix),y
        sta z80_c
        lda #64
        clc
        adc z80_c
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #16
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #16
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #16
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #16
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #16
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #16
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #16
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #16
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #48
        ; ldy #9
        sta (z80_ix),y
        jmp h00977
h00672: jsr skobj
        sta varobj
        lda #255
        cmp varobj
        bne *+5
        jmp h00814
        lda #0
        jsr animsp
        lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp h00743
        lda #1
        ; ldy #7
        sta (z80_ix),y
h00743: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp h00765
h00765: lda #1
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp h00787
h00787: lda #2
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp h00810
h00810: jmp h00909
h00814: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp h00896
:
        lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp h00863
        jmp h00891
h00863: lda #16
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
h00891: jmp h00909
h00896: lda #0
        ldy #7
        sta (z80_ix),y
h00909: lda #1
        ldy #11
        cmp (z80_ix),y
        bcs *+5
        jmp h00963
        ; ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
        jmp h00977
h00963: lda #0
        ldy #11
        sta (z80_ix),y
h00977: rts
.out .sprintf("EVENT 7 SIZE = %d", (* - evnt07))

evnt08:
        lda #19
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i00474
        lda #0
        jsr animsp
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i00340
:
        lda #90
        asl a
        sta sndtyp
        lda varb
        clc
        adc #1
        sta varb
        lda #21
        sta chary
        lda #27
        sta charx
        lda varb
        jsr disply
        lda #24
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp i00171
        ; lda #24
        iny; ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp i00171
        lda #216
        ; ldy #9
        sta (z80_ix),y
        rts
i00171: lda #216
        ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp i00226
        lda #96
        dey; ldy #8
        sta (z80_ix),y
        lda #48
        iny; ldy #9
        sta (z80_ix),y
        rts
i00226: lda #96
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp i00289
        lda #48
        iny; ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp i00289
        lda #192
        ; ldy #9
        sta (z80_ix),y
        rts
i00289: lda #192
        ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp i00340
        lda scno
        clc
        adc #1
        sta scno
        jsr nwscr
        lda #1
        sta nexlev
i00340: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp i00362
i00362: lda #1
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp i00384
i00384: lda #2
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp i00407
i00407: lda #2
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp i00448
        lda #0
        ; ldy #11
        sta (z80_ix),y
        jmp i00474
i00448: ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
i00474: lda #15
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i00505
        lda #0
        jsr animsp
i00505: lda #1
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i00982
        lda #32
        cmp scno
        bne *+5
        jmp i00822
        lda #55
        ldy #11
        cmp (z80_ix),y
        bcs *+5
        jmp i00599
        ; ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
        jmp i00818
i00599: lda #0
        ldy #11
        sta (z80_ix),y
        lda vare
        iny; ldy #12
        cmp (z80_ix),y
        bcs *+5
        jmp i00770
        ; ldy #12
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #12
        sta (z80_ix),y
        lda #2
        sta chary
        lda #8
        sta charx
        lda #28
        ; ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp i00708
        jmp i00708
i00708: ldy #12
        lda (z80_ix),y
        jsr dmsg
        lda #27
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp i00765
        lda #2
        sta chary
        lda #18
        sta charx
        jsr dscor
i00765: jmp i00818
i00770: lda #200
        jsr delay
        lda #100
        jsr delay
        lda #9
        cmp varl
        bcc *+5
        jmp i00818
        lda #1
        sta nexlev
        jmp i00818
i00818: jmp i00982
i00822: lda #5
        sta chary
        lda #3
        sta charx
        lda #30
        jsr dmsg
        lda #7
        sta chary
        lda #3
        sta charx
        lda #31
        jsr dmsg
        lda #9
        sta chary
        lda #3
        sta charx
        lda #32
        jsr dmsg
        lda #11
        sta chary
        lda #3
        sta charx
        lda #33
        jsr dmsg
        lda #13
        sta chary
        lda #3
        sta charx
        lda #34
        jsr dmsg
        lda #250
        jsr delay
        lda #250
        jsr delay
        lda #250
        jsr delay
        lda #250
        jsr delay
i00982: lda #0
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i01141
        jsr ifall
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp i01057
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
i01057: lda #29
        cmp scno
        beq *+5
        jmp i01079
        jmp i01119
i01079: jsr cangd
        beq :+
        jmp i01111
:
        lda #0
        ldy #7
        sta (z80_ix),y
        jmp i01119
i01111: lda #0
        jsr animsp
i01119: ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
i01141: lda #18
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i01226
        lda #8
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp i01213
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp i01226
i01213: lda #230
        ldy #9
        sta (z80_ix),y
i01226: jmp grav
.out .sprintf("EVENT 8 SIZE = %d", (* - evnt08))

evnt09:
        lda #7
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00088
        lda #30
        cmp scno
        beq *+5
        jmp j00088
        lda #1
        sta varobj
        ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda varobj
        jsr drpob
j00088: lda #8
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00184
        lda #18
        iny; ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp j00162
        lda #0
        ldy #11
        sta (z80_ix),y
        ; lda #0
        iny; ldy #12
        sta (z80_ix),y
j00162: lda #1
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp j00184
j00184: lda #1
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00249
        lda #17
        iny; ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp j00249
        ldy #9
        lda (z80_ix),y
        ldy #11
        sta (z80_ix),y
j00249: lda #0
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00301
        lda #51
        cmp scno
        beq *+5
        jmp j00301
        lda #200
        ldy #8
        sta (z80_ix),y
j00301: lda #5
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00337
        lda #0
        ldy #11
        sta (z80_ix),y
j00337: lda #2
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00430
        ; lda #2
        iny; ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp j00430
        ; lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        iny; ldy #7
        sta (z80_ix),y
j00430: lda #3
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00513
        tya; lda #5
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        ldy #11
        sta (z80_ix),y
        lda #1
        iny; ldy #12
        sta (z80_ix),y
j00513: lda #1
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp j00564
        lda #0
        ldy #11
        sta (z80_ix),y
        lda #13
        iny; ldy #12
        sta (z80_ix),y
j00564: lda #2
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp j00599
        lda #0
        ldy #11
        sta (z80_ix),y
j00599: lda #12
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp j00650
        lda #0
        ldy #11
        sta (z80_ix),y
        ; lda #0
        iny; ldy #12
        sta (z80_ix),y
j00650: rts
.out .sprintf("EVENT 9 SIZE = %d", (* - evnt09))

evnt10:
        lda #0
        cmp vark
        beq *+5
        jmp k00071
        lda #1
        cmp vart
        bcs *+5
        jmp k00055
        lda vart
        clc
        adc #1
        sta vart
        jmp k00071
k00055: lda #0
        sta vart
        lda #1
        sta vark
k00071: lda #1
        jsr delay
        rts
.out .sprintf("EVENT 10 SIZE = %d", (* - evnt10))

evnt11:
        lda #27
        sta vare
        rts
.out .sprintf("EVENT 11 SIZE = %d", (* - evnt11))

evnt12:
        lda #1
        jsr dmsg
        rts
.out .sprintf("EVENT 12 SIZE = %d", (* - evnt12))

evnt13:
        lda #0
        sta vara
        ; lda #0
        sta varb
        lda #1
        sta vark
        lda #15
        sta varl
        lda #0
        sta vart
        lda #20
        sta numlif
        ; lda #20
        sta chary
        lda #3
        sta charx
        lda #5
        sta loopa
n00077: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #91
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #94
        jsr pattr
        lda charx
        clc
        adc #1
        sta charx
        dec loopa
        beq :+
        jmp n00077
:
        lda #21
        sta chary
        lda #3
        sta charx
        lda #5
        sta loopa
n00184: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #92
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #93
        jsr pattr
        lda charx
        clc
        adc #1
        sta charx
        dec loopa
        beq :+
        jmp n00184
:
        lda #18
        sta chary
        lda #1
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #85
        jsr pattr
        lda #30
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #86
        jsr pattr
        lda #23
        sta chary
        lda #1
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #87
        jsr pattr
        lda #30
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #88
        jsr pattr
        lda #18
        sta chary
        lda #2
        sta charx
        lda #8
        sta loopa
n00448: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #82
        jsr pattr
        dec loopa
        beq :+
        jmp n00448
:
        lda #18
        sta chary
        lda #23
        sta charx
        lda #7
        sta loopa
n00513: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #82
        jsr pattr
        dec loopa
        beq :+
        jmp n00513
:
        lda #23
        sta chary
        lda #2
        sta charx
        lda #10
        sta loopa
n00578: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #81
        jsr pattr
        dec loopa
        beq :+
        jmp n00578
:
        lda #23
        sta chary
        lda #21
        sta charx
        lda #9
        sta loopa
n00643: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #81
        jsr pattr
        dec loopa
        beq :+
        jmp n00643
:
        lda #19
        sta chary
        lda #4
        sta loopa
n00700: lda #1
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #84
        jsr pattr
        lda #30
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #83
        jsr pattr
        lda chary
        clc
        adc #1
        sta chary
        dec loopa
        beq :+
        jmp n00700
:
        jsr cls
        lda #23
        sta chary
        lda #12
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #146
        jsr pattr
        lda #23
        sta chary
        lda #13
        sta charx
        lda #10
        jsr dmsg
        lda #20
        sta chary
        lda #22
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #68
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #69
        jsr pattr
        lda #21
        sta chary
        lda #22
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #70
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #71
        jsr pattr
        lda charx
        clc
        adc #1
        sta charx
        lda #3
        jsr dmsg
        lda #21
        sta chary
        lda #27
        sta charx
        lda varb
        jsr disply
        lda #18
        sta chary
        lda #10
        sta charx
        lda #5
        jsr dmsg
        lda #23
        sta chary
        lda #17
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #81
        jsr pattr
        rts
.out .sprintf("EVENT 13 SIZE = %d", (* - evnt13))

evnt14:
        lda #0
        sta vara
        lda #30
        cmp scno
        beq *+5
        jmp o00104
        lda #0
        sta varb
        lda #21
        sta chary
        lda #27
        sta charx
        lda varb
        jsr disply
        lda #21
        sta chary
        lda #28
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
o00104: lda #27
        cmp scno
        beq *+5
        jmp o00140
        lda #1
        sta varm
        lda #0
        sta varg
        rts
o00140: lda #29
        cmp scno
        beq *+5
        jmp o00176
        lda #4
        sta varm
        lda #0
        sta varg
        rts
o00176: lda #30
        cmp scno
        beq *+5
        jmp o00201
        lda #2
        sta varm
o00201: lda #31
        cmp scno
        beq *+5
        jmp o00237
        lda #3
        sta varm
        lda #0
        sta varg
        rts
o00237: lda #0
        cmp varg
        beq *+5
        jmp o00270
        lda #2
        sta varm
        lda #1
        sta varg
o00270: rts
.out .sprintf("EVENT 14 SIZE = %d", (* - evnt14))

evnt15:
        lda #0
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp p00024
p00024: rts
.out .sprintf("EVENT 15 SIZE = %d", (* - evnt15))

evnt16:
        lda #200
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp q00057
        lda #50
        asl a
        sta sndtyp
        lda #5
        jsr delay
        lda #30
        asl a
        sta sndtyp
q00057: lda #15
        cmp varl
        bcs *+5
        jmp q00409
        lda #30
        cmp scno
        beq *+5
        jmp q00104
        lda #0
        sta varl
        jmp q00409
q00104: lda #20
        sta chary
        lda varl
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #147
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #148
        jsr pattr
        lda #21
        sta chary
        lda varl
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #149
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #150
        jsr pattr
        lda #20
        sta chary
        lda #22
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #68
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #69
        jsr pattr
        lda #21
        sta chary
        lda #22
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #70
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #71
        jsr pattr
        lda #3
        sta z80_c
        lda varl
        sec
        sbc z80_c
        sta varl
q00409: lda #50
        jsr delay
        lda #0
        cmp varl
        beq *+5
        jmp q00447
        lda #31
        sta scno
        jsr nwscr
q00447: rts
.out .sprintf("EVENT 16 SIZE = %d", (* - evnt16))

evnt17:
        rts
.out .sprintf("EVENT 17 SIZE = %d", (* - evnt17))

evnt18:
        rts
.out .sprintf("EVENT 18 SIZE = %d", (* - evnt18))

evnt19:
        rts
.out .sprintf("EVENT 19 SIZE = %d", (* - evnt19))

evnt20:
        rts
.out .sprintf("EVENT 20 SIZE = %d", (* - evnt20))

ptcusr: rts
script_end:

.out .sprintf("TOTAL SCRIPT SIZE = %d", (* - script_start))

data_start:
msgdat:
        .byte "GAME*OVER",141
        .byte "SCORE",141
        .byte "BUBBLES",141
        .byte "X",141
        .byte "BY**DAVE*CLARKE",13
        .byte "***********MADE*IN*AGD",141
        .byte "IMPOSSABUBBLE",141
        .byte "PRESS*FIRE",141
        .byte "*TO*START",141
        .byte "SINCLAIR/KEMPSTON",141
        .byte 39
        .byte "DSC*2018",141
        .byte "2018*DSC",141
        .byte "*KEYS*ARE*O,P,SPACE*",141
        .byte "OR*SINCLAIR*JOYSTICK",141
        .byte "OR*KEMPSTON*JOYSTICK",141
        .byte "*YOU",39
        .byte "VE*ESCAPED***",141
        .byte "*YOU*SAVED*THEM*",141
        .byte "*CONGRATULATIONS**",141
        .byte "*****AND*NOW******",141
        .byte "*YOU",39
        .byte "RE*FLOATING**",141
        .byte "THROUGH*THE*CLOUDS",141
        .byte "*ALL*THE*WAY*HOME*",141
        .byte "THANKS*FOR*PLAYING",141
        .byte "*-MADE*WITH*AGD-**",141
        .byte "*****MUSIC*BY*****",141
        .byte "**DAVID**SAPHIER**",141
        .byte "**AGD*CREATED*BY**",141
        .byte "JONATHAN*CAULDWELL",141
        .byte "*BOUNCES:*********",141
        .byte "CAN*YOU*DO*BETTER?",141
        .byte "***AND*H*TO*PAUSE***",141
        .byte "*****INFINITE*LIVES:*****",141
        .byte "***POKE*32040*WITH*16****",141
        .byte "*COLLECT*ALL*21*OBJECTS:*",141
        .byte "***POKE*32030*WITH*21****",141
        .byte "(JUST*DON",39
        .byte "T*COLLECT*MORE)",141
nummsg:
        .byte 35
.out .sprintf("MESSAGE SIZE = %d", (* - msgdat))

chgfx:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 244,240,248,248,240,241,242,242,244,244,248,240,241,241,240,242
        .byte 15,30,45,135,90,15,45,15,75,15,15,15,15,15,15,15
        .byte 0,0,0,0,17,0,34,136,34,136,85,68,85,68,255,238
        .byte 187,170,85,68,85,68,34,136,34,136,17,0,0,0,0,0
        .byte 15,15,15,15,15,15,15,15,15,63,111,79,31,143,15,15
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,39,175
        .byte 95,95,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 95,95,0,0,0,0,0,0,0,0,0,0,0,0,175,175
        .byte 8,0,136,0,8,0,136,0,8,0,136,0,8,0,0,0
        .byte 0,17,0,1,0,17,0,1,0,17,0,1,0,17,0,1
        .byte 8,17,136,1,8,17,136,1,8,17,136,1,8,17,136,1
        .byte 95,95,136,0,8,0,136,0,8,0,136,0,8,0,136,0
        .byte 95,95,0,1,0,17,0,1,0,17,0,1,0,17,0,1
        .byte 8,0,136,0,8,0,136,0,8,0,136,0,8,0,39,175
        .byte 0,17,0,1,0,17,0,1,0,17,0,1,0,17,175,175
        .byte 180,180,90,90,240,240,90,90,225,165,240,240,165,180,90,210
        .byte 70,8,5,0,70,8,5,0,70,8,5,0,70,8,5,0
        .byte 17,70,0,170,17,70,0,170,17,70,0,170,17,70,0,170
        .byte 5,4,10,10,5,4,0,0,4,5,10,10,4,5,0,0
        .byte 4,0,10,0,4,0,0,0,4,5,10,10,4,5,0,0
        .byte 5,4,10,10,5,4,0,0,4,0,10,0,4,0,0,0
        .byte 5,4,10,10,5,4,0,0,0,5,0,10,0,5,0,0
        .byte 1,4,0,10,1,4,0,0,4,5,10,10,4,5,0,0
        .byte 5,4,11,10,7,4,7,12,7,13,11,10,4,5,0,0
        .byte 245,244,250,248,245,240,250,248,245,245,250,240,245,241,242,250
        .byte 70,8,5,0,70,8,5,0,70,8,5,0,70,8,5,0
        .byte 17,70,0,170,17,70,0,170,17,70,0,170,17,70,0,170
        .byte 17,25,0,175,0,8,0,39,0,2,0,34,0,2,0,34
        .byte 25,8,175,0,17,0,174,0,76,0,140,0,76,0,140,0
        .byte 0,2,0,1,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 76,0,136,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 45,165,30,30,135,165,15,30,15,165,15,90,15,15,75,75
        .byte 75,45,150,150,135,30,105,105,105,105,135,30,150,150,75,45
        .byte 180,165,210,210,165,240,90,240,225,225,210,210,240,180,90,240
        .byte 94,90,173,173,90,91,167,167,94,94,173,165,91,91,165,167
        .byte 45,165,30,90,15,165,30,90,165,165,15,90,135,165,90,75
        .byte 120,225,180,210,90,165,180,210,240,240,165,90,210,180,120,225
        .byte 165,180,90,120,165,240,90,120,165,165,90,240,165,225,210,90
        .byte 95,94,175,173,95,90,175,173,95,95,175,165,95,91,167,175
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 170,170,85,85,170,170,255,221,255,170,255,221,255,170,255,221
        .byte 255,221,255,170,255,221,255,170,255,221,255,170,255,204,255,136
        .byte 255,255,255,255,255,255,255,255,255,187,255,85,255,187,255,255
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
        .byte 15,15,10,10,5,4,0,0,4,5,10,10,4,5,0,0
        .byte 5,5,10,11,5,5,0,1,4,5,10,11,4,5,0,1
        .byte 13,4,10,10,13,4,8,0,12,5,10,10,12,5,8,0
        .byte 5,4,10,10,5,4,0,0,4,5,10,10,4,5,15,15
        .byte 15,15,10,10,13,4,8,0,12,5,10,10,12,5,8,0
        .byte 13,4,10,10,13,4,8,0,12,5,10,10,12,5,15,15
        .byte 5,5,10,11,5,5,0,1,4,5,10,11,4,5,15,15
        .byte 15,15,10,11,5,5,0,1,4,5,10,11,4,5,0,1
        .byte 240,240,210,210,225,225,120,90,165,165,210,210,165,180,90,120
        .byte 240,240,210,90,165,225,120,90,165,165,120,90,165,165,90,120
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 64,80,144,144,64,64,96,64,32,160,128,144,0,144,16,32
        .byte 80,32,144,64,32,64,96,64,32,128,32,64,32,128,80,32
        .byte 240,240,240,240,240,241,242,242,244,244,248,240,241,241,240,242
        .byte 244,240,248,248,240,240,242,240,244,244,248,240,241,240,240,240
        .byte 240,240,240,248,240,241,242,242,240,244,240,240,241,241,240,242
        .byte 244,240,248,248,240,241,242,242,244,244,248,240,240,240,240,240
        .byte 240,240,240,240,240,241,242,242,240,244,240,240,241,241,240,242
        .byte 240,240,240,248,240,241,242,242,240,244,240,240,240,240,240,240
        .byte 244,240,248,248,240,240,242,240,244,244,248,240,240,240,240,240
        .byte 240,240,240,240,240,240,242,240,244,244,248,240,241,240,240,240
        .byte 0,132,0,66,0,5,0,66,1,161,2,82,5,165,80,90
        .byte 33,0,10,0,36,0,10,0,133,128,74,64,165,160,90,10
        .byte 5,165,80,90,5,165,80,90,5,165,2,0,1,165,0,0
        .byte 165,160,90,10,165,160,90,10,165,160,0,64,165,128,0,0
        .byte 7,15,8,0,10,10,9,4,10,8,9,0,0,0,0,0
        .byte 15,14,0,1,0,1,0,1,0,1,0,1,0,1,0,10
        .byte 45,135,30,15,135,135,15,15,15,135,15,75,15,15,75,75
        .byte 45,165,30,30,15,165,15,30,15,165,15,90,15,15,15,75
        .byte 15,15,15,15,135,165,15,30,15,165,15,90,15,15,75,75
        .byte 45,165,30,30,135,165,15,30,15,165,15,90,15,15,15,15
        .byte 5,4,10,10,5,4,0,0,4,0,10,0,4,0,7,15
        .byte 5,4,10,10,5,4,0,0,0,5,0,10,0,5,7,15
        .byte 45,165,30,30,15,165,15,30,15,165,15,90,15,15,15,15
        .byte 95,95,0,0,10,10,5,5,10,10,0,0,95,95,0,0
        .byte 0,0,175,175,0,0,5,5,10,10,5,5,0,0,175,175
        .byte 10,10,137,34,10,10,137,34,10,10,137,34,10,10,137,34
        .byte 68,25,5,5,68,25,5,5,68,25,5,5,68,25,5,5
        .byte 19,95,39,175,95,0,174,5,76,10,141,5,76,10,141,4
        .byte 95,76,175,174,0,95,5,39,10,27,5,35,10,27,5,35
        .byte 76,10,141,5,76,10,141,5,78,10,175,0,87,95,35,175
        .byte 10,27,1,35,10,27,5,35,10,87,0,175,95,78,175,140
        .byte 244,240,248,248,240,241,242,242,244,244,248,240,241,241,240,242
        .byte 245,244,250,248,245,240,250,248,245,245,250,240,245,241,242,250
        .byte 0,0,0,39,17,8,34,0,2,0,4,0,68,0,4,0
        .byte 68,0,4,0,68,8,34,136,2,68,1,136,0,87,0,0
        .byte 0,2,0,34,0,2,0,4,0,68,1,136,78,0,0,0
        .byte 0,0,174,0,17,8,4,4,10,68,5,34,2,2,0,34
        .byte 70,8,5,0,70,8,5,0,70,8,5,0,70,8,39,175
        .byte 17,70,0,170,17,70,0,170,17,70,0,170,17,70,39,175
        .byte 7,15,8,0,10,10,9,4,10,8,9,0,0,0,0,0
        .byte 15,14,0,1,0,1,0,1,0,1,0,1,0,1,0,10
        .byte 5,4,10,10,5,4,0,0,4,5,10,10,4,5,0,0
        .byte 1,4,0,10,1,4,0,0,4,5,10,10,4,5,0,0
        .byte 5,0,10,0,5,0,0,0,4,5,10,10,4,5,0,0
        .byte 5,4,10,10,5,4,0,0,0,5,0,10,0,5,0,0
        .byte 5,4,10,10,5,4,0,0,4,0,10,0,4,0,0,0
        .byte 3,15,4,0,6,2,5,5,6,11,5,7,4,0,3,15
        .byte 15,14,0,3,15,13,15,13,15,13,15,13,0,3,15,14
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 8,0,136,0,8,0,136,0,8,0,136,0,8,0,136,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,136,0
        .byte 8,0,136,0,8,0,136,0,8,0,136,0,0,0,0,0
        .byte 95,95,175,175,95,95,175,175,95,95,175,175,95,95,175,175
        .byte 225,225,90,90,165,165,120,90,165,165,90,90,240,240,240,240
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,76,19,174,39
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,78,0,175,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,95,78,175,175
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,17,95,35,175
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,95,8,175,140
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,17,0,39
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,95,76,175,175
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,87,95,175,175
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,76,0,175,1
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,87,95,175,175
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,76,0,175,1
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,87,95,175,175
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,8,17,140,35
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,95,95,175,175
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,8,17,140,35
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,95,0,175,171
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,95,0,175,140
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,87,95,175,175
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,78,0,175,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,87,95,175,175
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,78,0,175,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,87,76,175,174
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,87,95,175,175
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,95,0,175,140
        .byte 0,17,0,1,0,17,0,1,0,17,0,1,0,17,0,1
        .byte 0,17,0,1,0,17,0,1,0,17,0,1,0,17,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,17,0,1
        .byte 0,0,0,0,0,0,140,0,76,0,140,0,0,0,0,0
        .byte 76,0,140,0,76,0,0,0,0,0,140,0,76,0,140,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 51,204,119,238,238,119,221,255,221,255,238,119,119,238,51,204
        .byte 0,68,1,0,0,17,5,136,0,0,136,0,0,0,34,0
        .byte 2,0,0,136,8,0,1,170,0,0,0,1,0,0,0,4
        .byte 2,0,0,0,8,0,0,0,85,8,0,1,17,0,0,4
        .byte 0,68,0,0,0,17,0,0,17,10,136,0,0,8,34,0
        .byte 0,0,0,0,51,136,68,204,68,68,68,68,51,136,0,0
        .byte 0,0,0,0,3,8,4,12,4,4,4,4,3,8,0,0
        .byte 0,0,0,0,48,128,64,192,64,64,64,64,48,128,0,0
        .byte 0,0,0,0,18,8,4,132,64,64,4,4,18,8,0,0
        .byte 0,0,0,0,33,128,64,72,4,4,64,64,33,128,0,0
        .byte 0,0,0,0,19,8,4,140,68,68,4,4,19,8,0,0
        .byte 0,0,0,0,49,128,64,200,68,68,64,64,49,128,0,0
        .byte 136,0,238,0,153,136,238,68,153,136,238,0,136,0,0,0
        .byte 0,17,0,119,17,153,34,119,17,153,0,119,0,17,0,0
        .byte 160,160,80,80,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 128,0,64,0,128,0,64,0,128,0,64,0,128,0,64,0
        .byte 160,160,80,80,128,0,64,0,128,0,64,0,128,0,64,0
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,7,1,15,3,15,7,15,7,15,15,15,15,15,15,15
        .byte 228,0,250,136,245,196,250,234,245,228,250,250,245,245,250,250
        .byte 15,15,15,15,15,15,15,14,15,14,15,12,15,8,14,0
        .byte 15,15,15,15,15,15,7,15,7,15,3,15,1,15,0,7
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
bprop:
        .byte 0
        .byte 2
        .byte 1
        .byte 5
        .byte 5
        .byte 6
        .byte 6
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 6
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
.out .sprintf("BLOCKS SIZE = %d", (* - chgfx))

sprgfx:
        .byte 0,0,7,224,24,24,32,68,32,164,64,82,64,34,64,2,64,2,64,2,72,2,40,4,36,4,24,24,7,224,0,0
        .byte 0,0,0,0,0,0,7,224,24,24,32,68,32,164,64,82,64,34,64,2,64,2,64,2,72,2,40,4,32,4,31,248
        .byte 0,0,0,0,0,0,0,0,7,224,24,24,32,4,64,66,64,162,128,81,128,33,128,1,128,1,144,1,64,2,63,252
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 4,32,0,0,0,0,0,128,4,32,0,0,1,0,0,0,0,0,2,0,0,64,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,1,0,0,0,0,0,0,128,0,0,0,0,2,64,0,0,0,128,0,0,0,0,0,0,0,0,0,0
        .byte 0,128,0,0,0,0,0,0,0,0,0,0,0,0,1,64,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 3,192,7,224,47,212,47,212,47,212,175,213,148,41,75,210,50,76,6,96,28,56,34,68,36,36,36,36,0,0,0,0
        .byte 3,192,7,224,15,208,47,212,47,212,47,212,20,40,75,210,114,78,6,96,28,56,18,72,2,64,2,64,0,0,0,0
        .byte 126,126,131,193,252,63,127,254,2,64,3,192,2,64,3,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 14,112,17,136,26,88,15,240,3,192,2,64,3,192,2,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 3,192,1,128,2,64,3,192,2,64,3,192,2,64,3,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 14,112,17,136,26,88,15,240,3,192,2,64,3,192,2,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 127,254,255,251,192,3,127,254,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 4,0,28,132,100,120,191,135,100,120,28,132,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 4,0,28,0,124,132,167,255,124,132,28,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,32,33,56,30,38,225,253,30,38,33,56,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,32,0,56,33,62,255,229,33,62,0,56,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,4,96,14,32,28,176,18,80,14,208,5,160
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,0,0,25,0,13,144,12,176,14,160,19,112,11,64,5,160
        .byte 127,254,255,251,192,3,127,254,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,17,0,10,0,4,7,220,122,170,170,158,127,228,40,20,40,34,40,34
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,10,0,10,0,4,7,220,122,170,170,158,127,228,40,20,84,34,84,65
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,10,0,10,0,4,7,222,122,170,170,190,127,244,68,34,170,34
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,136,0,80,0,32,0,59,224,85,94,121,85,39,254,40,20,68,20,68,20
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,80,0,80,0,32,0,59,224,85,94,121,85,39,254,40,20,68,42,130,42
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,80,0,80,0,32,0,123,224,85,94,125,85,47,254,68,34,68,85
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,2,0,2,0,2,0,66,0,66,0,64,0,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,64,0,64,0,64,0,66,0,66,0,2,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 43,244,43,244,43,244,43,244,43,244,64,2,127,254,160,5,127,254,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 24,24,32,68,32,164,64,82,64,34,64,2,64,2,64,2,72,2,40,4,36,4,24,24,7,224,0,0,0,0,0,0
        .byte 64,82,64,34,64,2,64,2,64,2,72,2,40,4,36,4,24,24,7,224,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 64,2,64,2,68,2,36,4,34,4,24,24,7,224,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 40,4,36,4,24,24,7,224,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 63,0,255,0,63,0,31,0,111,0,143,0,17,0,34,0,4,95,0,146,0,18,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 7,224,24,24,32,36,64,82,64,42,128,17,128,1,128,1,128,1,128,1,144,1,80,2,72,2,32,4,24,24,7,224
        .byte 7,224,24,24,32,36,64,82,64,42,129,145,128,1,132,33,132,33,128,1,145,129,80,2,72,2,32,4,24,24,7,224
        .byte 6,96,24,24,0,32,65,146,64,10,132,33,128,1,16,8,16,8,128,1,148,33,80,2,73,130,0,0,24,24,6,96
        .byte 4,32,16,8,1,128,88,26,0,0,128,1,0,0,32,4,32,4,0,0,128,1,0,0,88,26,1,128,16,8,4,32
        .byte 4,32,64,2,0,0,0,0,0,0,128,1,0,0,0,0,0,0,0,0,128,1,0,0,0,0,0,0,64,2,4,32
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,12,56,30,124,63,254,127,255,127,255,125,253,34,210,28,12,0,0,0,0,0,0,0,0,0,0
        .byte 8,16,5,32,4,32,4,32,31,248,39,100,95,250,95,186,94,250,95,242,93,234,95,82,95,170,32,4,31,248,0,0
frmlst:
        .byte 0,3
        .byte 3,4
        .byte 7,2
        .byte 9,4
        .byte 13,1
        .byte 14,2
        .byte 16,2
        .byte 18,2
        .byte 20,1
        .byte 21,3
        .byte 24,3
        .byte 27,1
        .byte 28,3
        .byte 31,1
        .byte 32,5
        .byte 37,1
        .byte 38,1
        .byte 39,6
        .byte 45,1
        .byte 46,1,47,0
.out .sprintf("SPRITES SIZE = %d", (* - sprgfx))

scdat:
        .word 293,240,281,187,172,258,152,269,279,374,320,301,261,377,305,267,315,213,236,394,250,343,299,289,246,298
        .word 302,227,257,130,178,120,89
        .byte 27,255,63,24,255,89,4,90,61,72,73,72,73,24,255,21,7,255,4,4,255,21,7,23,97,98,97,98,89,61,72,73,255,0,24
        .byte 97,98,89,61,23,255,0,26,102,89,61,255,0,10,8,8,255,0,13,30,31,0,89,61,8,8,255,0,8,2,2,11,255,0,12
        .byte 32,33,0,90,61,2,2,11,255,0,7,8,255,0,18,61,19,29,255,0,8,2,11,255,3,4,255,0,13,61,95,96,255,0,8
        .byte 59,0,255,2,4,11,255,0,12,61,2,2,11,255,0,5,8,0,0,0,19,59,58,29,255,0,13,61,19,29,255,0,6
        .byte 2,11,0,0,19,0,0,29,0,0,8,8,255,0,4,8,8,0,0,0,61,95,96,255,0,6,59,0,0,0,19,6,6
        .byte 29,0,0,2,2,11,0,0,0,2,2,11,0,27,61,2,2,11,255,0,9,19,6,6,29,0,0,59,58,255,0,4,58
        .byte 59,0,0,62,61,19,29,255,0,10,19,6,6,29,255,0,12,62,61,95,96,255,0,10,19,6,6,29,255,0,12,62,61
        .byte 2,2,11,0,3,0,0,0,3,0,0,0,19,7,7,29,0,0,3,255,0,6,3,0,25,62,27,55,56,55,56,55,56
        .byte 55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,27
        .byte 27,255,63,28,27,61,72,73,72,73,24,255,21,18,23,72,73,72,73,62,61,72,73,255,0,24,72,73,62,61,23,255,0,26
        .byte 24,62,61,255,0,28,62,27,255,0,28,27,255,0,34,8,255,0,5,8,8,255,0,22,2,11,255,0,4,2,2,11,255,0,11
        .byte 8,8,8,255,0,7,59,255,0,18,8,2,2,2,16,255,0,6,8,255,0,5,8,8,255,0,10,8,2,11,58,0,2
        .byte 16,0,27,0,0,0,2,11,255,0,4,2,2,11,255,0,8,8,2,11,255,0,4,2,27,61,0,0,0,58,255,0,8
        .byte 25,21,21,22,0,0,0,8,2,59,255,0,5,58,62,61,255,0,9,8,8,0,255,5,4,0,0,8,2,11,255,0,7
        .byte 62,61,255,0,9,2,2,11,255,5,4,0,8,2,58,255,0,8,62,61,3,3,0,3,3,0,3,3,0,3,3,0,255,5,4
        .byte 0,2,11,255,0,8,25,62,27,55,56,55,56,55,56,55,56,55,56,27,0,255,5,4,0,27,55,56,55,56,55,56,55
        .byte 56,55,56,27
        .byte 27,255,63,28,27,61,72,73,24,255,21,22,23,72,73,62,61,23,255,0,26,24,62,61,255,0,28,62,61,255,0,28,62
        .byte 27,255,0,10,8,8,0,0,8,8,0,0,8,8,0,0,8,8,255,0,4,27,255,0,11,2,2,11,0,2,2,11,0
        .byte 2,2,11,0,2,2,11,255,0,15,19,20,3,3,19,20,3,3,19,20,3,3,19,20,255,0,16,95,96,255,77,12,255,0,16
        .byte 2,2,59,255,0,10,58,255,0,16,19,20,255,0,17,27,255,8,4,255,0,6,95,96,255,0,5,8,8,0,0,0,8,8
        .byte 255,0,4,27,61,255,2,4,16,8,255,0,4,2,2,11,255,0,4,2,2,11,0,0,2,2,11,0,0,0,62,61,28
        .byte 20,19,20,2,2,16,8,0,0,95,96,255,0,5,19,20,0,0,0,19,20,255,0,4,62,61,28,20,19,20,19,20,2,2
        .byte 16,8,2,2,11,255,0,4,19,20,0,0,0,19,20,255,0,4,62,61,28,20,19,20,19,20,19,20,2,2,19,20,0,0,0
        .byte 3,0,19,20,3,0,3,19,20,0,3,0,3,62,27,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56
        .byte 55,56,55,56,55,56,55,56,55,56,27
        .byte 27,255,63,28,27,61,72,73,72,73,24,255,21,10,4,21,74,255,21,5,23,72,73,72,73,62,61,72,73,255,0,15,74
        .byte 255,0,8,72,73,62,61,23,255,0,16,74,255,0,9,24,62,61,255,0,11,8,8,255,0,4,74,255,0,10,62,27,255,0,11
        .byte 2,2,11,0,0,0,74,255,0,10,27,255,0,18,59,255,0,61,8,8,255,0,28,2,2,11,255,0,10,8,8,255,0,25
        .byte 27,0,0,2,2,11,255,0,23,27,61,255,0,28,62,61,255,0,28,62,61,255,0,28,62,61,3,0,3,0,3,0,3
        .byte 0,3,0,3,0,3,0,3,0,3,0,3,0,3,0,3,0,3,0,3,0,62,27,55,56,55,56,55,56,55,56,55,56
        .byte 55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,27
        .byte 27,255,63,28,27,61,72,73,72,73,24,255,21,18,23,72,73,72,73,62,61,72,73,255,0,24,72,73,62,61,23,255,0,26
        .byte 24,62,61,255,0,28,62,27,255,0,12,8,8,8,0,0,0,8,8,8,0,0,0,8,8,8,0,27,255,0,13,2,2,2
        .byte 11,0,0,2,2,2,11,0,0,2,2,2,11,255,0,15,58,255,0,5,59,255,0,5,58,255,0,93,27,255,0,12,255,8,16
        .byte 27,61,255,0,12,255,2,16,62,61,255,0,28,62,61,255,0,5,8,8,255,0,21,62,61,255,0,5,2,2,11,255,0,4
        .byte 75,255,0,15,62,27,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55
        .byte 56,27
        .byte 27,255,63,28,27,61,72,73,72,73,24,255,21,18,23,72,73,72,73,62,61,72,73,255,0,24,72,73,62,61,23,255,0,26
        .byte 24,62,61,255,0,28,62,27,255,0,28,27,255,0,10,74,255,0,8,75,255,0,15,8,8,0,0,0,74,0,0,0,8,8
        .byte 0,0,0,75,0,0,0,8,8,255,0,10,2,2,77,77,77,74,0,0,0,2,2,11,0,0,75,77,77,77,2,2,11
        .byte 255,0,9,59,255,0,4,74,255,0,8,75,255,0,4,58,255,0,10,8,8,0,0,0,74,255,0,8,75,0,0,0,8,8
        .byte 255,0,5,27,255,0,4,2,2,11,0,0,24,255,76,8,23,0,0,0,2,2,11,0,0,0,27,61,255,0,5,58,255,0,4
        .byte 24,21,21,4,4,21,21,23,255,0,4,59,255,0,5,62,61,255,0,7,8,8,255,0,10,8,8,255,0,7,62,61,255,0,7
        .byte 2,2,11,255,0,9,2,2,11,0,0,0,104,105,0,62,61,22,255,0,23,104,105,104,105,62,27,55,56,55,56,55,56
        .byte 55,56,55,56,27,255,0,6,27,55,56,55,56,55,56,55,56,55,56,27
        .byte 27,255,63,28,27,61,72,73,72,73,24,255,21,18,23,72,73,72,73,62,61,72,73,255,0,24,72,73,62,61,23,255,0,26
        .byte 24,62,61,255,0,28,62,27,255,0,28,27,255,0,150,27,8,8,255,0,24,8,8,27,61,2,2,11,255,0,10,8,8
        .byte 255,0,11,2,2,62,61,58,59,255,0,11,2,2,11,255,0,10,59,58,62,61,255,0,28,62,61,0,0,0,3,0,0,0
        .byte 3,0,0,0,3,255,0,4,3,0,0,0,3,0,0,0,3,0,0,0,62,27,55,56,55,56,55,56,55,56,55,56,55
        .byte 56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,27
        .byte 27,255,63,28,27,61,72,73,72,73,24,255,21,18,23,72,73,72,73,62,61,72,73,255,0,24,72,73,62,61,23,255,0,8
        .byte 8,8,0,0,8,8,0,0,8,8,255,0,8,24,62,61,255,0,9,2,2,11,0,2,2,11,0,2,2,11,0,255,8,7
        .byte 62,27,255,0,9,58,8,8,0,8,8,0,8,8,59,0,0,255,2,7,27,255,0,11,2,2,11,2,2,11,2,2,11
        .byte 255,0,21,59,8,8,0,0,8,8,58,255,0,23,2,2,11,0,2,2,11,255,0,12,6,6,255,0,9,58,0,8,8
        .byte 0,59,255,0,13,6,6,255,0,11,2,2,11,255,0,13,27,6,6,255,0,11,8,8,255,0,11,8,8,27,61,6,6
        .byte 255,0,11,2,2,11,255,0,10,2,2,62,61,6,6,255,0,10,25,21,21,22,255,0,10,8,8,62,61,6,6,255,0,10
        .byte 255,5,4,255,0,6,104,105,0,0,2,2,62,61,7,7,3,0,3,0,3,255,0,5,255,5,4,255,0,4,104,105,0
        .byte 104,105,0,0,0,62,27,55,56,55,56,55,56,55,56,55,56,27,0,255,5,4,0,27,55,56,55,56,55,56,55,56,55
        .byte 56,27
        .byte 27,255,63,28,27,61,107,106,255,21,21,23,72,73,72,73,62,61,255,0,26,72,73,62,61,255,0,27,24,62,61,255,0,6
        .byte 255,8,4,255,0,12,8,8,255,0,4,62,27,255,77,5,34,255,2,4,34,255,77,6,74,255,0,4,2,2,75,0,0,0
        .byte 62,255,0,6,75,8,8,0,0,74,255,0,6,74,255,0,4,8,8,75,0,0,0,62,255,0,6,75,2,2,11,0,74
        .byte 255,0,6,74,255,0,4,2,2,75,0,0,0,62,255,0,6,75,58,0,0,0,74,255,0,6,74,255,0,4,8,8,75
        .byte 0,0,0,62,255,0,6,75,255,8,4,74,255,0,6,74,255,0,4,2,2,75,0,0,0,62,255,0,6,75,255,2,4
        .byte 74,255,0,6,34,255,76,6,34,0,0,0,62,27,255,76,5,75,255,0,4,74,255,0,8,59,0,58,0,59,255,0,4
        .byte 62,61,0,0,58,0,0,75,255,8,4,74,255,0,17,62,61,255,0,6,255,2,4,11,255,0,17,62,61,255,0,17,255,18,8
        .byte 0,0,0,62,61,104,105,255,0,15,255,18,8,3,3,3,62,27,55,56,55,56,55,56,55,56,55,56,27,255,0,6,27
        .byte 55,56,55,56,55,56,55,56,55,56,27
        .byte 27,255,63,10,27,0,255,5,4,0,27,255,63,10,27,61,72,73,72,73,24,21,4,75,21,4,23,0,255,5,4,0,24
        .byte 255,21,5,23,72,73,72,73,62,61,72,73,255,0,5,75,255,0,4,255,5,4,255,0,10,72,73,62,61,23,255,0,6
        .byte 75,255,0,4,255,5,4,255,0,11,24,62,61,255,0,7,75,255,0,4,255,5,4,255,0,12,62,61,255,0,5,8,8
        .byte 75,255,0,4,255,5,4,255,0,12,62,61,255,0,5,2,2,75,255,0,4,255,5,4,255,0,5,8,8,8,255,0,4
        .byte 62,61,255,0,5,8,8,75,255,0,4,255,5,4,255,0,4,8,2,2,2,16,0,0,0,62,61,255,0,5,2,2,75
        .byte 255,0,4,255,5,4,0,0,0,8,2,11,59,8,2,16,0,0,62,61,255,0,4,8,8,58,75,255,0,4,255,5,4
        .byte 0,0,0,2,11,0,0,2,11,2,11,0,62,61,255,0,4,2,2,11,75,255,0,4,255,5,4,0,0,0,2,11,255,0,4
        .byte 2,11,0,62,61,255,0,4,59,8,8,75,255,0,4,255,5,4,0,0,0,2,16,0,0,0,8,2,11,0,62,61,255,0,5
        .byte 2,2,75,255,0,4,255,5,4,0,0,0,58,2,16,8,8,2,11,0,0,62,61,255,0,5,8,8,255,0,5,255,5,4
        .byte 255,0,5,2,2,2,11,0,0,0,62,61,255,0,5,2,2,11,255,0,4,255,5,4,255,0,7,59,255,0,4,62,61
        .byte 255,0,12,255,5,4,255,0,12,62,27,55,56,55,56,55,56,55,56,55,56,27,0,255,5,4,0,27,55,56,55,56,55
        .byte 56,55,56,55,56,27
        .byte 90,255,89,10,90,255,0,6,27,255,63,10,27,89,97,98,97,98,99,97,98,97,98,75,23,255,0,6,24,255,21,7,23
        .byte 72,73,62,89,97,98,255,0,5,97,98,75,255,0,17,24,62,89,103,255,0,7,102,75,255,0,18,62,89,0,30,31,0,0,0
        .byte 30,31,0,75,255,0,11,18,255,0,6,62,61,0,32,33,0,0,0,32,33,0,75,255,8,7,34,34,77,77,77,255,0,6
        .byte 62,61,8,8,8,255,0,6,75,255,2,7,34,74,0,4,0,8,8,255,0,4,62,61,2,2,2,11,255,0,6,59,0,0
        .byte 58,0,0,59,34,74,0,0,0,2,2,11,0,0,0,62,61,0,59,255,0,15,34,74,0,0,0,59,255,0,5,62,61
        .byte 0,0,8,8,8,255,0,12,34,74,0,0,0,8,8,255,0,4,62,61,0,0,2,2,2,11,255,0,11,34,74,0,0,0
        .byte 2,2,11,0,0,0,62,61,0,0,0,58,255,0,13,34,74,255,0,4,58,255,0,4,62,61,8,8,8,255,0,10,8,8
        .byte 255,0,7,8,8,255,0,4,62,61,2,2,2,11,255,0,9,2,2,11,255,0,6,2,2,11,0,0,0,62,61,0,59
        .byte 255,0,11,58,255,0,8,59,255,0,5,62,61,3,0,0,3,255,0,22,3,0,62,27,55,56,55,56,55,56,55,56,55
        .byte 56,27,255,0,6,27,55,56,55,56,55,56,55,56,55,56,27
        .byte 27,255,63,10,27,0,255,5,4,0,27,255,63,10,27,61,72,73,72,73,24,255,21,5,23,0,255,5,4,0,24,255,21,5
        .byte 23,72,73,72,73,62,61,72,73,255,0,10,255,5,4,255,0,10,72,73,62,61,23,255,0,11,255,5,4,255,0,11,24
        .byte 62,61,255,0,12,255,5,4,255,0,12,62,61,0,0,8,8,255,0,5,8,8,0,255,5,4,0,8,8,255,0,5,8,8
        .byte 0,0,62,61,0,0,2,2,11,255,0,4,2,2,11,255,5,4,0,2,2,11,255,0,4,2,2,11,0,62,61,8,8
        .byte 0,58,255,0,5,59,0,0,255,5,4,0,58,255,0,6,59,0,8,8,62,61,2,2,11,255,0,9,255,5,4,255,0,10
        .byte 2,2,62,61,8,8,255,0,10,255,5,4,255,0,10,8,8,62,61,2,2,11,255,0,9,255,5,4,255,0,10,2,2
        .byte 62,61,0,58,255,0,10,255,77,4,255,0,10,59,0,62,61,8,8,255,0,10,255,5,4,255,0,10,8,8,62,61,2,2
        .byte 11,255,0,9,255,5,4,255,0,10,2,2,62,61,58,255,0,11,255,5,4,255,0,11,59,62,61,255,0,12,255,5,4
        .byte 255,0,12,62,27,55,56,55,56,55,56,55,56,55,56,27,0,255,5,4,0,27,55,56,55,56,55,56,55,56,55,56,27
        .byte 90,255,89,4,255,63,6,27,255,0,6,27,255,63,6,255,89,4,90,89,97,98,97,98,24,255,21,5,23,255,0,6,24
        .byte 255,21,5,23,97,98,97,98,89,89,97,98,255,0,11,6,6,255,0,11,97,98,89,89,103,255,0,12,6,6,255,0,12
        .byte 102,89,89,0,30,31,255,0,10,6,6,255,0,10,30,31,0,89,61,0,32,33,255,0,10,7,7,255,0,10,32,33,0
        .byte 62,61,255,0,13,2,2,11,0,8,8,255,0,9,62,61,255,0,9,8,8,255,0,6,2,2,11,255,0,8,62,61,255,0,9
        .byte 2,2,11,0,6,6,0,0,58,255,0,10,62,61,255,0,9,59,0,0,0,6,6,255,0,13,62,61,255,0,13,6,6
        .byte 255,0,13,62,61,255,0,13,7,7,255,0,13,62,61,255,0,9,8,8,0,0,77,77,0,0,8,8,255,0,9,62,61
        .byte 255,0,9,2,2,11,255,0,5,2,2,11,255,0,8,62,61,255,0,10,58,255,0,7,59,255,0,9,62,61,255,0,28
        .byte 62,27,55,56,55,56,55,56,55,56,55,56,27,255,0,6,27,55,56,55,56,55,56,55,56,55,56,27
        .byte 27,255,63,10,27,0,255,5,4,0,27,255,63,10,27,61,72,73,24,255,21,4,23,72,73,75,0,255,5,4,0,74,72
        .byte 73,24,4,21,21,21,23,72,73,61,61,23,255,0,8,24,75,0,255,5,4,0,74,23,255,0,8,24,61,61,255,0,10
        .byte 75,0,255,5,4,0,74,255,0,10,61,61,0,0,8,8,0,0,0,8,8,0,75,0,255,5,4,0,74,255,0,10,61,61
        .byte 0,0,2,2,11,0,0,2,2,11,75,0,255,5,4,0,74,255,0,4,75,0,0,0,8,8,61,61,8,8,0,59,255,0,4
        .byte 58,0,59,0,255,5,4,0,74,255,0,4,75,0,0,0,2,2,61,61,2,2,11,255,0,9,255,5,4,0,74,255,0,4
        .byte 75,255,0,5,61,61,8,8,255,0,10,255,5,4,0,34,255,76,4,34,0,0,0,8,8,61,61,2,2,11,255,0,9
        .byte 255,5,4,0,0,59,0,0,58,255,0,4,2,2,61,61,0,58,0,0,0,8,8,8,255,0,4,255,5,4,255,0,10
        .byte 58,0,61,61,255,0,5,2,2,2,11,0,0,0,255,5,4,0,8,8,0,0,8,8,0,0,8,8,8,61,61,255,0,6
        .byte 59,255,0,5,255,5,4,0,2,2,11,0,2,2,11,0,2,2,2,61,61,255,0,12,255,5,4,0,59,255,0,4,58
        .byte 0,0,0,59,0,61,61,255,0,12,255,5,4,255,0,12,61,61,3,0,3,0,3,0,3,0,3,0,3,0,255,5,4
        .byte 0,3,0,3,0,3,0,3,0,3,0,3,61,27,55,56,55,56,55,56,55,56,55,56,27,0,255,5,4,0,27,55,56
        .byte 55,56,55,56,55,56,55,56,27
        .byte 27,255,63,24,255,89,4,90,61,72,73,72,73,24,255,21,18,23,97,98,97,98,89,61,72,73,255,0,24,97,98,89,61
        .byte 78,255,8,5,255,0,21,102,89,61,255,2,6,74,8,8,8,255,0,15,30,31,0,89,61,8,8,0,0,0,24,74,2,2,2
        .byte 16,8,8,255,0,12,32,33,0,90,61,2,2,11,0,0,0,74,19,58,20,2,2,2,16,8,8,255,0,13,61,95,96
        .byte 255,0,4,74,19,0,20,19,59,20,2,2,2,16,8,8,255,0,10,61,2,2,11,0,0,0,74,19,0,20,19,0,20
        .byte 19,58,20,2,2,2,11,255,0,9,61,19,20,0,8,8,0,74,19,21,20,19,0,20,19,0,20,19,59,29,255,0,10
        .byte 61,19,20,0,2,2,11,74,19,21,20,19,24,20,19,23,20,19,0,29,255,0,10,61,95,96,0,19,20,0,74,19,24
        .byte 20,19,21,20,19,21,20,19,23,29,255,0,5,255,8,4,27,61,2,2,11,19,20,0,34,255,76,12,255,0,5,255,2,4
        .byte 62,61,19,20,0,95,96,255,0,19,28,58,58,20,62,61,19,20,0,2,2,11,255,0,18,28,25,22,20,62,61,19,20
        .byte 0,19,20,255,0,19,28,24,23,20,62,27,55,56,55,56,55,56,55,56,55,56,27,255,0,6,27,55,56,55,56,55,56
        .byte 55,56,55,56,27
        .byte 90,255,89,4,255,63,6,27,255,0,6,27,255,63,10,27,89,97,98,97,98,24,21,21,21,106,107,23,255,0,6,74,72
        .byte 73,72,73,24,23,72,73,72,73,62,89,97,98,255,0,15,74,72,73,255,0,6,72,73,62,89,103,255,0,16,74,23,255,0,8
        .byte 24,62,89,0,30,31,255,0,14,74,255,0,10,62,90,0,32,33,255,0,14,74,255,0,10,62,255,0,18,34,255,76,5
        .byte 255,0,5,62,255,0,27,8,8,62,255,0,27,2,2,62,255,0,27,59,0,62,0,255,8,11,255,0,12,6,6,0,0,0
        .byte 62,27,255,2,11,11,255,0,11,6,6,0,0,0,62,61,8,8,0,59,255,0,4,58,0,20,255,0,4,8,8,255,0,6
        .byte 6,6,0,0,0,62,61,2,2,16,8,255,0,6,20,255,0,4,2,2,11,255,0,5,6,6,0,0,0,62,61,19,20
        .byte 2,2,11,255,0,5,20,255,0,4,58,255,0,7,6,6,0,0,0,62,61,19,20,21,21,255,0,6,20,255,0,12,7,7
        .byte 0,3,25,62,27,55,56,55,56,55,56,55,56,55,56,27,255,0,6,27,55,56,55,56,55,56,55,56,55,56,27
        .byte 27,255,63,10,27,0,255,5,4,0,27,255,63,6,255,89,4,90,61,72,73,72,73,24,255,21,5,23,0,255,5,4,0
        .byte 24,107,106,21,21,21,23,97,98,97,98,89,61,72,73,255,0,10,255,5,4,255,0,10,97,98,89,61,78,255,8,10,0
        .byte 255,5,4,255,0,11,102,89,61,255,2,11,75,255,5,4,255,0,9,30,31,0,89,61,59,0,0,58,0,0,59,0,0
        .byte 58,0,75,255,5,4,255,0,9,32,33,0,62,61,255,8,11,75,255,5,4,255,0,12,62,61,255,2,11,75,255,5,4
        .byte 255,0,12,62,61,58,0,0,59,0,0,58,0,0,59,0,75,255,5,4,255,0,12,62,61,255,8,11,75,255,5,4,255,0,12
        .byte 62,61,255,2,11,75,255,5,4,255,0,12,62,61,59,0,0,58,0,0,59,0,0,58,0,75,255,5,4,255,0,12,62
        .byte 61,255,8,11,75,255,5,4,255,0,4,8,8,8,255,0,5,62,61,255,2,11,80,255,5,4,255,0,4,2,2,2,11
        .byte 255,0,4,62,61,58,0,0,59,0,0,58,0,0,59,0,0,255,5,4,0,0,18,18,0,0,0,18,18,0,0,0,62
        .byte 61,255,0,12,255,5,4,0,0,18,18,0,0,0,18,18,0,3,0,62,27,55,56,55,56,55,56,55,56,55,56,27,0
        .byte 255,5,4,0,27,55,56,55,56,55,56,55,56,55,56,27
        .byte 27,255,63,10,27,255,0,6,27,255,63,6,255,89,4,90,61,72,73,72,73,24,255,21,5,23,255,0,6,24,255,21,5
        .byte 23,97,98,97,98,89,61,72,73,255,0,24,97,98,89,61,23,255,0,26,102,89,61,255,0,4,255,77,15,255,0,6,30
        .byte 31,0,89,61,255,0,4,58,255,0,12,59,255,0,7,32,33,0,90,61,18,18,255,0,27,61,18,18,255,0,27,61,255,77,19
        .byte 255,0,10,61,0,59,255,0,5,58,255,0,5,59,255,0,9,75,255,0,5,61,255,0,21,18,18,75,255,0,5,61,255,0,21
        .byte 18,18,75,255,0,4,27,61,255,0,4,255,76,19,34,255,0,4,62,61,255,0,4,59,255,0,10,58,255,0,6,59,255,0,5
        .byte 62,61,18,18,255,0,26,62,61,18,18,255,0,26,62,27,55,56,55,56,55,56,55,56,55,56,27,255,0,6,27,55,56
        .byte 55,56,55,56,55,56,55,56,27
        .byte 90,255,89,4,255,63,20,255,89,4,90,89,97,98,97,98,97,98,97,98,255,47,5,106,107,255,47,5,97,98,97,98,97
        .byte 98,97,98,89,89,97,98,97,98,255,0,20,97,98,97,98,89,89,97,255,0,26,98,89,89,103,30,31,255,0,22,30,31
        .byte 102,89,90,0,32,33,255,0,22,32,33,0,62,255,0,29,62,255,0,29,62,255,0,21,104,105,255,0,6,62,255,0,20
        .byte 104,105,104,105,255,0,5,62,0,0,0,255,8,16,104,105,104,105,104,105,8,8,0,0,62,27,8,8,255,2,24,16,8
        .byte 62,61,2,2,11,58,255,0,9,8,8,255,0,9,59,0,2,2,62,61,255,0,13,2,2,11,255,0,12,62,61,255,0,10
        .byte 8,8,255,0,4,8,8,255,0,10,62,61,0,3,0,0,0,3,255,0,4,2,2,11,0,0,0,2,2,11,0,0,0
        .byte 3,0,0,0,3,0,62,27,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55
        .byte 56,55,56,27
        .byte 90,255,89,4,255,63,6,27,0,255,5,4,0,27,255,63,10,27,89,97,98,97,98,23,255,21,4,4,23,0,255,5,4
        .byte 0,74,4,255,21,4,23,72,73,106,107,62,89,97,98,255,0,10,255,5,4,0,74,255,0,10,62,89,103,255,0,11,255,5,4
        .byte 0,74,255,0,10,62,89,0,30,31,255,0,9,255,2,4,11,74,255,0,4,75,0,0,0,8,8,62,61,0,32,33,0,0
        .byte 8,8,255,0,10,74,255,0,4,75,0,8,8,2,2,27,61,255,0,5,2,2,11,255,0,9,74,255,0,4,75,0,2,2
        .byte 11,0,0,61,255,0,5,19,20,255,0,6,6,6,0,0,74,255,0,4,75,0,19,20,0,0,0,61,255,0,5,19,96
        .byte 8,8,8,0,0,255,6,4,0,74,255,0,4,75,8,95,20,0,0,0,61,255,0,5,19,255,2,4,11,0,255,6,4
        .byte 0,74,255,0,4,75,2,2,20,0,0,0,61,0,0,0,8,8,95,19,59,58,20,0,0,255,7,4,3,74,255,0,4
        .byte 75,59,8,96,0,0,0,61,0,0,0,2,2,2,19,0,0,20,76,76,255,2,4,76,34,76,0,0,0,75,0,2,2
        .byte 11,0,27,61,8,8,8,19,59,20,19,0,0,20,58,4,4,4,58,4,4,4,59,0,0,0,75,0,8,8,0,0,62
        .byte 61,2,2,2,19,0,20,19,0,0,20,255,0,12,75,0,2,2,11,0,62,61,19,0,20,19,0,20,18,18,0,20,255,0,12
        .byte 75,0,19,20,0,0,62,61,3,0,20,19,0,20,18,18,0,20,255,0,12,75,3,3,3,0,0,62,27,55,56,55,56
        .byte 55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,27
        .byte 27,255,63,28,27,61,72,73,72,73,24,255,21,19,75,34,34,34,62,61,72,73,255,0,22,75,34,34,34,62,61,23,255,0,23
        .byte 75,34,34,34,62,61,255,0,24,80,77,77,77,62,27,255,8,4,255,0,4,8,8,255,0,18,27,0,255,2,4,11,0,0,0
        .byte 2,2,11,255,0,21,59,0,0,34,34,0,0,0,8,8,255,0,23,77,77,0,0,0,2,2,11,255,0,27,58,255,0,31
        .byte 255,8,4,255,0,13,27,8,8,8,255,0,9,255,2,4,11,255,0,8,8,8,8,27,61,2,2,2,11,8,8,8,255,0,6
        .byte 59,255,0,7,8,8,8,0,2,2,2,62,61,19,0,20,0,2,2,2,11,255,0,13,2,2,2,11,19,0,20,62,61
        .byte 19,0,20,0,19,0,20,255,0,14,19,0,20,0,19,0,20,62,61,19,0,20,0,19,0,20,0,3,3,0,0,0,3,3
        .byte 0,0,0,3,3,0,19,0,20,0,19,0,20,62,27,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56
        .byte 55,56,55,56,55,56,55,56,55,56,27
        .byte 90,255,89,4,255,63,20,255,89,4,90,89,97,98,97,98,24,255,21,8,4,4,255,21,8,23,97,98,97,98,89,89,97
        .byte 98,255,0,24,97,98,89,89,103,255,0,26,102,89,89,0,30,31,255,0,9,255,8,4,255,0,9,30,31,0,89,90,0
        .byte 32,33,255,0,9,255,2,4,11,255,0,8,32,33,0,90,255,0,13,19,59,58,20,255,0,24,6,6,19,0,0,20,6,6
        .byte 255,0,18,6,6,0,0,6,6,19,0,0,20,6,6,0,0,6,6,255,0,15,6,0,0,6,6,19,0,0,20,6,6
        .byte 0,0,6,6,255,0,10,255,8,4,6,6,8,8,6,6,19,0,0,20,6,6,8,8,6,6,255,8,4,0,0,0,27
        .byte 0,0,255,2,4,11,0,2,2,11,6,19,0,0,20,6,6,2,2,11,6,255,2,4,11,0,27,61,0,0,19,34,34
        .byte 20,6,6,19,20,6,6,19,0,0,20,6,6,19,20,6,6,19,34,34,20,0,0,62,61,0,0,19,34,34,20,6,6
        .byte 19,20,6,6,19,0,0,20,6,6,19,20,6,6,19,34,34,20,0,0,62,61,0,0,19,34,34,20,6,6,19,20,6,6
        .byte 19,0,0,20,6,6,19,20,6,6,19,34,34,20,0,0,62,61,3,3,19,34,34,20,7,7,19,20,7,7,19,3,3
        .byte 20,7,7,19,20,7,7,19,34,34,20,3,3,62,27,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56
        .byte 55,56,55,56,55,56,55,56,55,56,27
        .byte 90,255,89,4,255,63,6,27,255,0,6,27,255,63,6,255,89,4,90,89,97,98,97,98,24,255,21,5,23,255,0,6,24
        .byte 255,21,5,23,97,98,97,98,89,89,97,98,255,0,24,97,98,89,89,103,255,0,12,6,6,255,0,12,102,89,89,0,30
        .byte 31,255,0,9,255,6,4,255,0,9,30,31,0,89,90,0,32,33,255,0,9,255,6,4,255,0,9,32,33,0,90,255,0,13
        .byte 255,6,4,255,0,26,255,7,4,255,0,15,8,8,8,255,0,5,8,8,0,255,2,4,11,8,8,255,0,5,8,8,8
        .byte 255,0,4,2,2,2,11,8,8,8,0,2,2,11,24,21,21,21,0,2,2,11,8,8,8,0,2,2,2,11,0,0,0
        .byte 19,58,20,0,2,2,2,11,19,20,0,21,22,21,21,0,19,20,0,2,2,2,11,19,59,20,0,0,27,0,19,59,20
        .byte 0,21,21,21,0,19,20,0,21,21,21,22,0,19,20,0,21,26,21,0,19,58,20,0,27,61,0,19,0,255,2,22,59
        .byte 20,0,62,61,8,95,0,24,255,21,14,22,255,21,4,26,23,0,96,8,62,61,2,2,255,0,24,2,2,62,61,22,255,0,26
        .byte 25,62,27,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,27
        .byte 27,255,63,10,27,255,0,6,27,255,63,10,27,61,72,73,72,73,24,255,21,5,23,255,0,6,74,255,21,5,23,72,73
        .byte 72,73,62,61,72,73,255,0,24,72,73,62,61,23,255,0,14,8,8,255,0,10,24,62,61,255,0,11,75,0,0,0,2,2
        .byte 11,6,6,255,0,8,62,27,255,0,11,75,0,0,0,59,58,0,6,6,255,0,8,27,255,0,12,75,255,0,6,6,6
        .byte 255,0,20,25,75,255,0,6,6,6,255,0,4,6,6,255,0,10,255,77,5,80,255,0,6,6,6,255,0,4,6,6,255,0,14
        .byte 24,75,255,0,6,7,7,255,0,4,6,6,255,0,15,75,255,0,6,75,34,255,0,4,6,6,0,0,0,27,255,0,11
        .byte 75,255,0,6,58,59,255,0,4,6,6,0,0,27,61,8,8,255,0,9,75,255,0,6,59,58,255,0,4,6,6,0,0
        .byte 62,61,2,2,11,255,0,8,80,255,0,12,6,6,0,0,62,61,8,8,255,0,22,7,7,0,0,62,61,2,2,11,3,3
        .byte 25,18,18,22,3,3,25,18,18,22,3,3,25,18,18,22,3,3,25,18,18,3,3,62,27,55,56,55,56,55,56,55,56
        .byte 55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,27
        .byte 27,255,63,28,27,61,72,73,72,73,24,255,21,18,23,72,73,72,73,62,61,72,73,255,0,11,34,34,255,0,11,72,73
        .byte 62,61,23,255,0,12,34,34,255,0,12,24,62,61,255,0,13,34,34,255,0,13,62,27,255,0,8,74,255,0,4,34,34
        .byte 255,0,4,75,255,0,8,27,255,0,8,8,74,255,0,4,34,34,255,0,4,75,8,255,0,16,2,74,255,0,4,77,77
        .byte 255,0,4,75,2,11,255,0,15,8,74,255,0,10,75,8,255,0,16,2,74,255,0,10,75,2,11,255,0,15,59,74,255,0,10
        .byte 75,58,255,0,8,27,8,8,255,0,6,74,255,0,4,77,77,255,0,4,75,255,0,6,8,8,27,61,2,2,11,255,0,10
        .byte 59,58,255,0,11,2,2,62,61,255,0,28,62,61,255,0,28,62,61,3,0,3,0,3,0,3,0,3,0,3,0,3,0
        .byte 3,0,3,0,3,0,3,0,3,0,3,0,3,0,62,27,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55
        .byte 56,55,56,55,56,55,56,55,56,55,56,27
        .byte 27,255,63,10,27,0,255,5,4,0,27,255,63,10,27,61,72,73,72,73,24,255,21,5,23,0,255,5,4,0,24,255,21,5
        .byte 23,72,73,72,73,62,61,72,73,255,0,10,255,5,4,255,0,10,72,73,62,61,23,255,0,11,255,5,4,255,0,11,24
        .byte 62,61,255,0,12,255,77,4,255,0,12,62,27,255,0,9,8,8,8,0,58,59,0,8,8,8,255,0,9,27,255,0,7
        .byte 8,8,8,2,2,2,11,0,0,0,2,2,2,16,8,8,255,0,11,8,8,8,2,2,2,11,58,255,0,6,59,0,2,2,2
        .byte 16,8,8,255,0,7,8,2,2,2,11,59,255,0,4,255,8,4,255,0,4,58,0,2,2,2,16,255,0,6,2,11,58
        .byte 255,0,7,255,2,4,11,255,0,6,59,0,2,11,255,0,15,59,0,0,58,255,0,13,27,8,8,255,0,4,8,8,255,0,4
        .byte 255,8,4,255,0,10,8,8,27,61,2,2,11,0,0,0,2,2,11,0,0,0,255,2,4,11,255,0,9,2,2,62,61
        .byte 58,59,255,0,10,58,0,0,59,255,0,10,59,58,62,61,255,0,28,62,61,3,255,0,17,104,105,255,0,7,3,62,27
        .byte 55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,27
        .byte 27,255,63,10,27,255,0,6,27,255,63,10,27,61,72,73,72,73,255,47,7,255,0,6,255,47,7,72,73,72,73,62,61
        .byte 72,73,255,0,24,72,73,62,61,78,255,8,5,255,0,5,255,8,16,79,62,61,255,2,6,11,255,0,4,255,2,17,62
        .byte 27,255,0,28,62,0,0,0,8,8,0,0,0,8,8,8,0,0,0,8,8,8,0,0,0,8,8,8,0,0,0,8,8,8
        .byte 62,0,0,0,2,2,11,0,0,2,2,2,11,0,0,2,2,2,11,0,0,2,2,2,11,0,0,2,2,2,62,0,0,0
        .byte 58,59,255,0,4,58,255,0,5,58,255,0,5,58,255,0,5,58,0,62,255,0,29,62,255,0,9,6,6,255,0,4,6,6
        .byte 255,0,10,18,18,62,27,255,0,8,6,6,255,0,4,6,6,255,0,10,18,18,62,61,255,0,8,6,6,255,0,4,6,6
        .byte 255,0,8,255,18,4,62,61,255,0,8,6,6,255,0,4,6,6,255,0,8,255,18,4,62,61,255,0,8,6,6,255,0,4
        .byte 6,6,255,0,6,255,18,6,62,61,3,0,0,0,3,0,0,0,7,7,0,3,3,0,7,7,0,104,105,0,0,3,255,18,6
        .byte 62,27,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,27
        .byte 91,141,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,0,94,0,139,255,0,26
        .byte 142,0,0,140,255,0,26,143,0,92,0,0,145,145,255,0,20,145,145,0,0,93,255,0,8,90,255,63,12,90,255,0,5
        .byte 153,255,0,7,151,0,0,61,21,26,21,26,21,26,26,21,26,21,26,21,62,255,0,7,157,0,156,255,0,6,61,23,255,0,10
        .byte 24,62,0,151,255,0,12,154,0,61,255,0,12,62,255,0,4,156,255,0,6,156,255,0,4,61,255,0,12,62,155,255,0,12
        .byte 156,0,0,61,255,0,12,62,0,0,0,156,0,0,0,153,153,255,0,7,61,255,0,12,62,255,0,14,153,0,61,22,0,0
        .byte 3,3,0,0,3,3,0,0,25,62,255,0,6,151,0,0,151,255,0,6,90,255,60,12,90,0,151,255,0,33,145,145,255,0,4
        .byte 145,255,0,22,145,144,144,145,255,0,26,145,144,144,145,255,0,27,145,145,0
        .byte 90,255,89,4,255,63,20,255,89,4,90,89,97,98,97,98,97,98,97,98,255,47,5,106,107,255,47,5,97,98,97,98,97
        .byte 98,97,98,89,89,97,98,97,98,255,0,20,97,98,97,98,89,89,97,255,0,26,98,89,89,103,30,31,255,0,10,8,8
        .byte 255,0,10,30,31,102,89,90,0,32,33,255,0,10,2,2,11,255,0,9,32,33,0,62,255,0,14,8,8,0,0,0,8,8
        .byte 255,0,8,62,255,0,14,2,2,11,0,0,2,2,11,0,0,8,8,0,0,0,62,255,0,24,2,2,11,0,0,62,255,0,29
        .byte 62,0,0,0,255,8,24,0,0,62,27,8,8,255,2,24,16,8,62,61,2,2,11,58,255,0,9,8,8,255,0,9,59
        .byte 0,2,2,62,61,255,0,13,2,2,11,255,0,12,62,61,255,0,10,8,8,255,0,4,8,8,255,0,10,62,61,0,3
        .byte 0,0,0,3,255,0,4,2,2,11,0,0,0,2,2,11,0,0,0,3,0,0,0,3,0,62,27,55,56,55,56,55,56
        .byte 55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,27
        .byte 255,166,32,167,255,163,23,171,168,255,166,4,255,163,24,171,171,255,166,4,255,163,26,255,166,4,255,163,26,255,166,4,255,163,12
        .byte 164,164,255,163,12,255,166,4,255,163,12,164,164,255,163,12,255,166,4,255,163,12,164,164,255,163,12,255,166,4,255,163,12
        .byte 164,164,255,163,12,255,166,4,255,163,12,164,164,255,163,12,255,166,4,255,163,12,165,165,255,163,12,255,166,4,255,163,26
        .byte 255,166,4,255,163,26,255,166,4,255,163,26,255,166,4,255,163,26,255,166,4,170,255,163,24,169,255,166,32
        .byte 27,255,63,28,27,61,255,21,28,62,61,23,255,0,26,24,62,61,255,8,4,255,0,20,255,8,4,62,61,255,2,4,11
        .byte 255,0,19,255,2,4,62,61,255,0,28,62,61,255,8,6,255,0,16,255,8,6,62,61,255,2,6,11,255,0,15,255,2,6
        .byte 62,61,255,0,28,62,61,255,8,8,255,0,12,255,8,8,62,61,255,2,8,11,255,0,11,255,2,8,62,61,255,0,28
        .byte 62,61,8,8,3,255,8,4,3,8,8,255,0,8,8,8,3,255,8,4,3,8,8,62,61,255,2,10,11,255,0,7,255,2,10
        .byte 62,61,255,0,28,62,61,22,255,0,26,25,62,27,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55,56,55
        .byte 56,55,56,55,56,55,56,55,56,27
        .byte 255,0,61,25,255,26,26,22,0,0,26,255,21,26,26,0,0,26,21,23,0,255,4,20,0,24,21,26,0,0,26,21,255,0,24
        .byte 21,26,0,0,26,21,158,255,0,22,159,21,26,0,0,26,21,158,255,0,22,159,21,26,0,0,26,21,158,255,0,22,159
        .byte 21,26,0,0,26,21,158,255,0,22,159,21,26,0,0,26,21,255,0,24,21,26,0,0,26,21,22,0,255,3,20,0,25
        .byte 21,26,0,0,26,255,21,26,26,0,0,24,255,26,26,23,255,0,91
        .byte 27,255,63,28,27,61,255,21,28,62,61,23,255,0,26,24,62,61,255,0,28,62,61,255,0,28,62,61,255,0,28,62,61
        .byte 255,0,28,62,61,255,0,28,62,61,255,0,28,62,61,255,0,28,62,61,255,0,28,62,61,255,0,28,62,61,255,0,28
        .byte 62,61,255,0,28,62,61,22,255,0,26,25,62,61,255,21,28,62,27,255,60,28,27
numsc:
        .byte 33
.out .sprintf("SCREENS SIZE = %d", (* - scdat))

nmedat:
        .byte 0,0,112,224,7,1,32,96,5,7,24,216,2,2,112,184,2,2,32,56,3,3,128,120,3,12,120,120,255
        .byte 0,0,88,120,2,4,32,64,4,6,48,16,2,2,56,200,7,1,120,208,255
        .byte 0,0,120,120,2,2,48,216,7,1,88,152,255
        .byte 2,4,24,64,0,0,96,128,2,4,120,128,2,4,80,208,2,2,112,184,4,6,112,16,7,1,32,104,255
        .byte 2,4,120,56,0,0,120,224,4,5,96,208,4,6,24,16,2,2,64,80,6,10,120,176,7,1,40,208,255
        .byte 0,0,56,120,2,2,24,96,2,2,80,144,4,6,24,32,7,1,80,120,255
        .byte 0,0,96,120,2,8,104,208,2,2,24,48,2,2,104,80,2,2,40,152,2,2,80,184,2,8,104,32,7,1,120,120,255
        .byte 0,0,120,88,3,3,128,16,2,2,72,72,6,9,32,216,7,1,32,208,3,12,120,16,255
        .byte 0,0,32,16,2,2,48,160,7,1,72,176,2,13,16,16,255
        .byte 6,10,120,200,6,9,120,16,0,0,120,120,7,1,40,56,7,1,96,192,255
        .byte 0,0,40,96,2,2,72,88,4,6,72,16,5,7,24,24,2,2,96,216,5,7,24,64,7,1,24,40,255
        .byte 0,0,120,24,4,5,96,224,4,5,40,168,4,6,96,24,4,6,24,24,255
        .byte 0,0,120,80,3,3,96,120,3,3,48,120,6,9,120,40,6,10,120,216,5,7,24,24,5,7,24,216,3,12,40,120,3,12,88,120,255
        .byte 0,0,120,120,7,1,32,72,7,1,56,168,255
        .byte 5,7,24,216,0,0,24,32,255
        .byte 0,0,56,224,3,3,128,200,5,7,24,24,2,2,40,56,6,10,120,80,7,1,40,168,3,12,120,200,2,13,16,80,255
        .byte 0,0,96,24,2,4,104,160,2,4,32,200,6,9,24,24,6,10,72,64,6,9,120,40,5,7,24,216,7,1,120,184,2,13,16,160,255
        .byte 0,0,40,16,4,5,24,88,5,7,24,216,255
        .byte 0,0,120,120,2,13,16,120,5,7,24,24,5,7,24,216,255
        .byte 3,3,88,112,3,3,88,128,0,0,120,224,5,7,24,24,7,1,40,56,3,12,80,112,3,12,80,128,2,13,16,224,255
        .byte 0,0,80,120,2,8,112,120,7,1,40,24,255
        .byte 3,3,128,64,3,3,128,96,3,3,128,144,3,3,128,176,0,0,64,120,5,7,24,24,5,7,24,216,7,1,32,120,3,12,112,64,3,12,112,96,3,12,112,144,3,12,112,176,255
        .byte 0,0,112,120,3,3,64,112,3,3,64,128,5,7,24,24,5,7,24,216,6,9,120,48,3,12,48,112,3,12,40,128,255
        .byte 0,0,56,80,3,3,120,208,3,3,80,160,3,12,72,160,3,12,112,208,255
        .byte 0,0,80,120,2,4,24,96,2,8,96,48,2,4,120,144,2,8,96,192,255
        .byte 0,0,64,120,7,1,88,120,255
        .byte 0,0,112,24,4,6,88,56,3,3,128,80,3,3,128,128,6,9,24,152,7,1,24,32,3,12,120,80,3,12,120,128,255
        .byte 0,0,120,224,4,6,8,128,4,5,8,112,2,2,48,32,2,2,104,208,8,0,56,88,8,0,64,120,8,0,72,152,7,1,120,48,255
        .byte 0,0,120,120,5,7,24,24,5,7,24,216,2,13,16,120,2,2,40,144,2,2,72,184,4,6,88,64,255
        .byte 8,0,64,120,8,18,32,232,8,18,112,64,8,18,96,128,8,18,48,96,8,18,112,176,8,18,72,48,8,18,64,192,8,18,32,72,8,1,16,120,8,15,16,216,255
        .byte 0,0,120,120,8,19,24,24,2,2,24,80,2,2,88,160,4,5,96,96,4,6,96,144,4,6,48,80,4,5,48,160,6,9,72,16,6,10,72,224,255
        .byte 1,17,64,88,1,17,64,104,1,17,64,128,1,17,64,144,1,17,64,160,1,17,64,176,1,17,64,72,1,17,64,56,1,17,64,192,255
        .byte 8,1,8,8,255
.out .sprintf("POSITIONS SIZE = %d", (* - nmedat))

NUMOBJ = 43
objdta:
        .byte 12,48,4,32,4,32,4,32,27,216,39,228,95,250,95,250,95,250,95,242,95,234,95,210,95,170,32,4,31,248,0,0,0,32,96,0,32,96
        .byte 12,48,4,32,4,32,4,32,27,216,39,228,95,250,95,250,95,250,95,242,95,234,95,210,95,170,32,4,31,248,0,0,1,120,208,1,120,208
        .byte 12,48,4,32,4,32,4,32,27,216,39,228,95,250,95,250,95,250,95,242,95,234,95,210,95,170,32,4,31,248,0,0,2,88,152,2,88,152
        .byte 12,48,4,32,4,32,4,32,27,216,39,228,95,250,95,250,95,250,95,242,95,234,95,210,95,170,32,4,31,248,0,0,3,32,104,3,32,104
        .byte 12,48,4,32,4,32,4,32,27,216,39,228,95,250,95,250,95,250,95,242,95,234,95,210,95,170,32,4,31,248,0,0,4,40,208,4,40,208
        .byte 12,48,4,32,4,32,4,32,27,216,39,228,95,250,95,250,95,250,95,242,95,234,95,210,95,170,32,4,31,248,0,0,5,80,120,5,80,120
        .byte 12,48,4,32,4,32,4,32,27,216,39,228,95,250,95,250,95,250,95,242,95,234,95,210,95,170,32,4,31,248,0,0,6,120,120,6,120,120
        .byte 12,48,4,32,4,32,4,32,27,216,39,228,95,250,95,250,95,250,95,242,95,234,95,210,95,170,32,4,31,248,0,0,7,32,208,7,32,208
        .byte 12,48,4,32,4,32,4,32,27,216,39,228,95,250,95,250,95,250,95,242,95,234,95,210,95,170,32,4,31,248,0,0,8,72,176,8,72,176
        .byte 12,48,4,32,4,32,4,32,27,216,39,228,95,250,95,250,95,250,95,242,95,234,95,210,95,170,32,4,31,248,0,0,9,40,56,9,40,56
        .byte 12,48,4,32,4,32,4,32,27,216,39,228,95,250,95,250,95,250,95,242,95,234,95,210,95,170,32,4,31,248,0,0,9,96,192,9,96,192
        .byte 12,48,4,32,4,32,4,32,27,216,39,228,95,250,95,250,95,250,95,242,95,234,95,210,95,170,32,4,31,248,0,0,13,32,72,13,32,72
        .byte 12,48,4,32,4,32,4,32,27,216,39,228,95,250,95,250,95,250,95,242,95,234,95,210,95,170,32,4,31,248,0,0,13,56,168,13,56,168
        .byte 12,48,4,32,4,32,4,32,27,216,39,228,95,250,95,250,95,250,95,242,95,234,95,210,95,170,32,4,31,248,0,0,10,24,40,10,24,40
        .byte 12,48,4,32,4,32,4,32,27,216,39,228,95,250,95,250,95,250,95,242,95,234,95,210,95,170,32,4,31,248,0,0,15,40,168,15,40,168
        .byte 12,48,4,32,4,32,4,32,27,216,39,228,95,250,95,250,95,250,95,242,95,234,95,210,95,170,32,4,31,248,0,0,16,120,184,16,120,184
        .byte 12,48,4,32,4,32,4,32,27,216,39,228,95,250,95,250,95,250,95,242,95,234,95,210,95,170,32,4,31,248,0,0,19,40,56,19,40,56
        .byte 12,48,4,32,4,32,4,32,27,216,39,228,95,250,95,250,95,250,95,242,95,234,95,210,95,170,32,4,31,248,0,0,20,40,24,20,40,24
        .byte 12,48,4,32,4,32,4,32,27,216,39,228,95,250,95,250,95,250,95,242,95,234,95,210,95,170,32,4,31,248,0,0,21,32,120,21,32,120
        .byte 12,48,4,32,4,32,4,32,27,216,39,228,95,250,95,250,95,250,95,242,95,234,95,210,95,170,32,4,31,248,0,0,25,88,120,25,88,120
        .byte 12,48,4,32,4,32,4,32,27,216,39,228,95,250,95,250,95,250,95,242,95,234,95,210,95,170,32,4,31,248,0,0,26,24,32,26,24,32
        .byte 255,253,255,253,207,231,207,231,207,225,207,225,207,225,207,224,207,224,207,224,207,224,207,224,207,230,207,230,207,231,126,253,27,16,24,27,16,24
        .byte 255,255,255,255,254,127,254,127,248,127,248,127,248,127,224,127,224,127,224,127,0,127,0,127,14,127,14,127,190,119,255,193,27,16,40,27,16,40
        .byte 255,31,255,31,1,255,1,255,0,127,60,127,60,127,0,127,0,127,1,223,63,7,63,7,56,1,56,1,56,0,224,0,27,16,56,27,16,56
        .byte 255,195,255,195,224,115,224,123,128,57,15,14,15,14,63,207,63,207,63,207,15,15,15,15,128,57,128,57,224,112,63,192,27,16,72,27,16,72
        .byte 255,255,255,255,248,7,248,7,240,7,112,252,112,252,248,113,248,113,254,31,255,135,255,135,240,7,240,7,112,28,31,240,27,16,88,27,16,88
        .byte 255,255,255,255,248,7,248,7,240,7,112,255,112,255,248,115,248,115,254,31,255,135,255,135,240,7,240,7,112,28,31,240,27,16,104,27,16,104
        .byte 255,243,255,243,248,31,248,31,240,7,243,199,243,199,243,199,243,199,243,199,240,7,240,7,240,7,240,7,115,199,30,124,27,16,120,27,16,120
        .byte 255,243,255,243,240,31,243,223,243,199,243,199,243,199,240,31,240,31,240,31,243,199,243,199,243,199,243,199,112,28,31,240,27,16,136,27,16,136
        .byte 255,255,255,255,243,243,243,243,243,243,243,243,243,243,243,243,243,243,243,243,240,3,240,3,112,3,112,3,24,7,15,252,27,16,152,27,16,152
        .byte 255,252,255,252,252,7,252,247,252,241,252,241,252,241,252,7,252,7,252,7,252,241,252,241,252,241,252,241,28,7,7,252,27,16,168,27,16,168
        .byte 255,252,255,252,252,7,252,247,252,241,252,241,252,241,252,7,252,7,252,7,252,241,252,241,252,241,252,241,28,7,7,252,27,16,184,27,16,184
        .byte 255,129,255,129,252,225,252,225,252,225,252,225,252,225,252,241,252,241,252,253,252,255,252,255,252,1,252,1,28,1,7,255,27,16,200,27,16,200
        .byte 255,255,255,255,252,1,252,1,252,1,252,255,252,255,252,1,252,1,252,1,252,255,252,255,252,1,252,1,220,1,7,255,27,16,216,27,16,216
        .byte 63,192,127,241,255,249,252,57,240,57,243,255,227,255,238,7,238,7,238,7,227,247,243,247,112,7,28,14,31,254,15,248,255,8,8,255,8,8
        .byte 255,227,255,243,255,255,252,31,248,15,251,239,251,239,251,239,251,239,251,239,248,15,248,15,248,15,59,238,63,254,31,124,255,8,8,255,8,8
        .byte 248,255,255,255,255,255,247,252,241,248,241,248,241,248,240,224,240,224,240,224,240,0,247,28,247,28,119,252,127,255,63,255,255,8,8,255,8,8
        .byte 127,252,255,255,255,255,254,3,254,3,254,254,254,254,254,3,254,3,254,3,254,254,254,254,254,3,238,3,239,255,135,254,255,8,8,255,8,8
        .byte 0,1,0,7,0,15,0,15,0,15,0,15,0,14,0,14,0,14,0,14,0,14,0,15,0,7,0,1,0,1,0,0,255,8,8,255,8,8
        .byte 252,31,255,31,255,159,195,159,0,255,60,255,60,127,255,127,255,127,255,127,60,115,60,243,0,225,195,128,255,128,255,0,255,8,8,255,8,8
        .byte 255,63,255,255,255,255,190,255,190,255,190,255,190,255,190,255,190,255,190,255,193,255,193,255,193,223,119,7,127,7,62,3,255,8,8,255,8,8
        .byte 254,127,255,255,255,255,1,254,1,254,127,254,127,254,1,254,1,254,1,254,127,254,127,254,1,254,1,206,255,207,255,135,255,8,8,255,8,8
        .byte 252,0,254,0,255,0,7,0,3,0,251,0,251,0,3,0,7,0,7,0,226,0,251,0,249,0,253,0,255,128,199,128,255,8,8,255,8,8
.out .sprintf("OBJECTS SIZE = %d", (* - objdta))

font:
        .byte 3,79,58,40,125,145,50,40
        .byte 125,62,50,71,205,236,126,62
        .byte 0,33,40,125,190,194,32,122
        .byte 62,31,50,27,125,205,122,141
        .byte 201,201,201,142,228,142,220,120
        .byte 224,182,251,118,46,91,127,55
        .byte 112,216,174,194,183,195,127,63
        .byte 28,54,46,60,24,0,0,0
        .byte 28,54,46,44,44,52,30,14
        .byte 56,44,54,22,54,46,60,24
        .byte 0,0,0,0,0,0,0,0
        .byte 56,40,238,131,239,47,60,28
        .byte 0,0,0,0,56,108,92,120
        .byte 0,0,254,131,255,127,0,0
        .byte 0,0,0,60,38,38,62,30
        .byte 162,85,162,0,8,20,8,0
        .byte 124,198,155,171,179,199,127,62
        .byte 56,108,76,108,238,131,255,127
        .byte 252,134,251,199,190,131,255,127
        .byte 124,198,187,231,186,199,127,62
        .byte 28,54,102,214,131,247,126,60
        .byte 254,131,191,134,251,135,254,124
        .byte 124,198,191,134,187,199,127,62
        .byte 254,131,251,55,46,44,60,28
        .byte 124,198,187,199,186,199,127,62
        .byte 124,198,187,195,123,71,127,62
        .byte 0,56,44,60,56,44,60,28
        .byte 0,56,44,60,56,108,92,120
        .byte 28,54,110,92,108,54,30,12
        .byte 0,126,67,127,67,127,63,0
        .byte 112,88,108,54,110,92,120,48
        .byte 124,198,187,247,46,60,44,60
        .byte 0,60,74,86,94,64,60,0
        .byte 124,198,147,147,131,147,255,239
        .byte 252,198,147,135,146,199,255,126
        .byte 124,198,147,191,146,199,255,126
        .byte 252,134,179,179,179,135,255,126
        .byte 254,195,158,131,158,195,255,127
        .byte 254,131,191,143,184,184,240,112
        .byte 124,198,147,191,154,195,255,127
        .byte 238,171,187,131,187,187,255,119
        .byte 254,131,239,108,238,131,255,127
        .byte 14,11,11,235,187,199,127,62
        .byte 238,187,167,140,166,187,255,103
        .byte 224,176,176,176,190,131,255,127
        .byte 238,187,147,171,187,187,239,103
        .byte 238,187,155,171,179,187,255,111
        .byte 124,198,187,187,187,199,127,62
        .byte 252,134,187,135,190,188,240,112
        .byte 124,198,187,171,182,202,127,59
        .byte 252,134,187,135,182,187,239,103
        .byte 126,195,191,198,123,135,254,124
        .byte 254,131,239,47,44,44,60,28
        .byte 238,187,187,187,187,199,126,60
        .byte 238,187,187,187,215,110,60,24
        .byte 238,187,187,171,171,215,126,52
        .byte 238,187,215,110,214,187,255,111
        .byte 238,187,187,199,110,44,60,28
        .byte 254,131,247,110,222,131,255,127
        .byte 1,31,0,237,120,50,31,125
        .byte 1,254,239,237,120,230,16,50
        .byte 32,125,237,120,230,1,50,36
        .byte 125,237,120,230,8,50,34,125
        .byte 201,175,50,32,125,201,62,183
        .byte 50,32,125,201,62,190,50,32
        .byte 125,201,201,195,187,195,127,63
        .byte 224,160,188,134,187,135,255,126
        .byte 0,126,195,191,190,195,127,63
        .byte 14,11,123,195,187,195,127,63
        .byte 0,124,198,150,142,195,127,63
        .byte 60,100,92,76,92,88,120,56
        .byte 126,195,187,195,123,123,70,124
        .byte 224,160,188,134,187,187,255,103
        .byte 56,44,60,40,108,70,126,62
        .byte 28,22,30,20,246,182,206,124
        .byte 112,92,86,78,76,86,126,54
        .byte 112,88,88,88,92,102,62,30
        .byte 0,252,150,171,171,171,255,127
        .byte 0,252,134,187,187,187,255,103
        .byte 0,124,198,187,187,199,127,62
        .byte 0,252,134,187,135,190,188,240
        .byte 0,124,198,182,198,118,19,31
        .byte 0,60,102,94,94,88,120,56
        .byte 0,124,198,158,228,142,252,120
        .byte 112,88,76,88,92,102,62,28
        .byte 0,238,187,187,187,199,254,124
        .byte 0,238,187,187,215,111,62,28
        .byte 0,238,187,171,171,215,126,52
        .byte 0,254,147,239,238,147,255,127
        .byte 0,238,187,187,195,123,71,126
        .byte 0,254,131,119,207,130,255,127
        .byte 62,35,239,156,238,35,63,31
        .byte 56,44,44,44,44,44,60,28
        .byte 248,136,238,115,239,143,248,120
        .byte 62,107,87,127,62,0,0,0
        .byte 60,126,231,223,223,231,126,60
.out .sprintf("FONT SIZE = %d", (* - font))

jtab:
        .byte 248,250,252,254,254,255,255,255,0,0,0,1,1,1,2,2,4,6,8,8,8,99
keys:   .byte 66,97,104,72,73,16,55,48,49,17,18
.out .sprintf("KEYS SIZE = %d", (* - keys))

data_end:
.out .sprintf("TOTAL DATA SIZE = %d", (* - data_start))
