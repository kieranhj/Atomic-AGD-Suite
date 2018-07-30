; Game engine code --------------------------------------------------------------

; Arcade Game Designer.
; (C) 2008 Jonathan Cauldwell.
; ZX Spectrum Next Engine v0.1.

; Global definitions ------------------------------------------------------------

	FONT = font			; Font address

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
	TABSIZ = 17			; size of each entry.
	SPRBUF = NUMSPR * TABSIZ; size of entire table.
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

frmptr:	.word frmlst         ; sprite frames.

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
	lda #<(msgdat)		; pointer to messages.
	sta z80_l
	lda #>(msgdat)
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
	ldx #7			; pixel height in b.
dbar0:
	ldy scrtab,x
	lda (scraddr),y		; get screen byte.
	eor #255		; reverse all bits.
	sta (scraddr),y		; write back to screen.
	dex			; next line down.
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
	lda #<objdta 		; objects table.
	sta z80_x
	lda #>objdta
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
	ldy #0
	lda (z80_hl),y	 	; fetch cell.
	cmp #FODDER 		; is it fodder?
	beq :+
	rts 			; no.
:
	lda #0
	ldy #0
	sta (z80_hl),y 		; rewrite block type.

	lda z80_h		; store pointer to block.
	pha
	lda z80_l
	pha

	sec			; set carry flag for subtraction.
	lda z80_l 		; find simple displacement for block.
	sbc #<MAP
	sta z80_l
	lda z80_h
	sbc #>MAP
	sta z80_h

	lda z80_l		; low byte is y coordinate.
	and #31 		; column position 0 - 31.
	sta dispx		; set up x position.

	asl z80_l		; multiply displacement by 8.
	rol z80_h
	asl z80_l
	rol z80_h
	asl z80_l
	rol z80_h

	lda z80_h		; x coordinate now in h.
	sta dispy		; set the display coordinate.

	lda #0 			; block to write.
	jsr pattr 		; write block.

	pla 			; restore block pointer.
	sta z80_l
	pla
	sta z80_h

	rts

;----------------------------------------------------
; Scrolly text and puzzle variables.
;----------------------------------------------------

.if sflag
txtbit:	.byte 128		; bit to write.
txtwid:	.byte 16		; width of ticker message.
txtpos:	.word msgdat
txtini:	.word msgdat
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
	jsr vsync 		; synchronise with display.

	lda #<sprtab 		; address of sprite table, even sprites.
	sta z80_x
	lda #>sprtab
	sta z80_i
	jsr dspr 		; display even sprites.

	jsr plsnd 		; play sounds.
	jsr vsync 		; synchronise with display.

	lda #<(sprtab+TABSIZ) 	; address of first odd sprite.
	sta z80_x
	lda #>(sprtab+TABSIZ)
	sta z80_i
	jsr dspr 		; display odd sprites.

	lda #<(ssprit) 		; point to spare sprite for spawning purposes.
	sta z80_x
	lda #>(ssprit)
	sta z80_i
evlp1:
	jsr evnt10 		; called once per main loop.
	jsr pspr 		; process sprites.

; Main loop events.

	lda #<ssprit 		; point to spare sprite for spawning purposes.
	sta z80_x
	lda #>ssprit
	sta z80_i
evlp2:
	jsr evnt11 		; called once per main loop.
bsortx:
	jsr bsort 		; sort sprites.

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

;----------------------------------------------------------
; New level
;----------------------------------------------------------

newlev:
	lda scno 			; current screen.
	clc
	adc #1				; next screen.
	cmp numsc			; total number of screens.
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
	lda #<objdta 			; objects table.
	sta z80_l
	lda #>objdta
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
	lda #<objdta 		; objects.
	sta z80_l
	lda #>objdta
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
	lda #<objdta 		; pointer to objects.
	sta z80_l
	lda #>objdta
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
	asl scraddr  	; multiply char by 8
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

colpatt:	.byte 0

pattr:
	sta z80_b		; store cell in b register for now.
	tax
	lda bprop,x 		; block properties.
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
	asl z80_e  		; multiply char by 8.
	rol z80_d
	asl z80_e
	rol z80_d
	asl z80_e
	rol z80_d
	clc
	lda z80_e
	adc #<chgfx 		; address of graphics.
	sta tileaddr
	lda z80_d
	adc #>chgfx
	sta tileaddr+1
	jsr gprad 		; get screen address.
	ldx #7			; number of pixel rows to write.
panp0:
	ldy #0
	lda (tileaddr),y 	; get image byte.
	eor #TxtInvert		; Invert
	ldy scrtab,x
	sta (scraddr),y 	; copy to screen.
	inc tileaddr 		; next image byte.
	bne :+
	inc tileaddr+1
:
	dex	 		; repeat for 8 pixel rows.
	bpl panp0
	inc dispx 		; move along one.
	inc charx
	rts

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
	sta z80_a
sprit3:
	lsr spr			; shift into position.
	ror spr+1
	ror spr+2
	dec z80_a		; one less iteration.
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
	lda #<scdat 		; pointer to screens.
	sta z80_l
	lda #>scdat
	sta z80_h
groom1:
	cpx #0			; is it the first one?
	beq groom0 		; no more screens to skip.

	clc
	lda z80_l
	adc scdat,y 		; low byte of screen size.
	sta z80_l
	iny			; point to high byte.
	lda z80_h
	adc scdat,y 		; high byte of screen size.
	sta z80_h
	iny			; next address.

	dex 			; one less iteration.
	jmp groom1 		; loop until we reach the end.
groom0:
	lda numsc 		; add displacement.
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
	jsr pattr 		; show attributes and block.
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
	ldy #9
	lda (z80_ix),y		; x coordinate.
	sta dispx

	ldy #8
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
	ldy #9
	lda (z80_ix),y		; x coordinate.
	sta dispx

	ldy #8
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
	ldy #9
	lda (z80_ix),y		; x coordinate.
	sta dispx
	ldy #8
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
	ldy #9
	lda (z80_ix),y 		; x coordinate.
	sta dispx
	ldy #8
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
	ldy #8
	lda (z80_ix),y 		; y coordinate.
	sta dispy
	ldy #9
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
	ldy #8
	lda (z80_ix),y		; y coordinate.
	sta dispy
	ldy #9
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
	adc #<chgfx 		; address of graphics.
	sta tileaddr
	lda z80_d
	adc #>chgfx
	sta tileaddr+1
	jsr gprad 		; get screen address.
	ldx #7			; number of pixel rows to write.
gtblk0:
	ldy #0
	lda (tileaddr),y 	; get image byte.
	ldy scrtab,x
	eor (scraddr),y 	; XOR tile on screen
	sta (scraddr),y 	; copy to screen.
	inc tileaddr 		; next image byte.
	bne :+
	inc tileaddr+1
:
	dex	 		; repeat for 8 pixel rows.
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
	ldy #8
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
	lda dispy 		; fetch y coord.
	lsr a			; bufaddr = y/8
	lsr a
	lsr a
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

	lda dispx		; x/8
	lsr a
	lsr a
	lsr a

	clc			; bufaddr = MAP + x/8 + y/8*32
	adc bufaddr
	adc #<MAP
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
	lda #>jtab
	ldy #var_jumpHi
	sta (z80_ix),y		; set jump high.
	lda #<jtab		; jump table start.
	ldy #var_jumpLo
	sta (z80_ix),y		; set jump low.
	rts

; Jump table.

jtab:
	.byte 248,250,252
	.byte 254,254,255
	.byte 255,255,0,0
	.byte 0,1,1,1,2,2
	.byte 4,6,8,8,8,99

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
	lda keys,y 		; get key from table.
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
	lda #<msgdat		; pointer to messages.
	sta z80_l
	lda #>msgdat
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
	lsr a	  		; multiply by 128.
	sta z80_d 		; store in d.
	lda #0
	ror a
	sta z80_e 		; got low byte.

	clc 			; address of play sprites.
	lda z80_e
	adc #<sprgfx
	sta z80_e
	lda z80_d
	adc #>sprgfx
	sta z80_d

	lda dispx 		; y coordinate.
	and #6 			; position within byte boundary.
	tax	 		; low byte of table displacement.

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
	; 6502 WARNING - not updating 16-bit pointer properly!
	lda (z80_hl),y			; fetch that from table.
	sta z80_a
grav0:
	inc z80_l			; point to next table entry.
	; 6502 WARNING - not updating 16-bit pointer properly!
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
	ldy #8
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
	ldy #8
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
	ldy #9
	lda (z80_ix),y			; y coordinate.
	sta dispx
numsp7:
	ldy #8
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
	lda #<jtab			; jump table start.
	sta z80_l
	lda #>jtab
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
	lda #<nmedat 			; list of enemy sprites.
	sta z80_l
	lda #>nmedat
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
	ldx #7				; pixel height of each cell.
clw1:
	ldy scrtab,x
	sta (scraddr),y 			; copy to screen.
	dex				; next screen row down.
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

	lda #<msgdat 		; text messages.
	sta z80_l
	lda #>msgdat
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

; block NUMSPR * TABSIZ,255
; sprtab NUMSPR*TABSIZ,255

ssprit:	.byte 255,255,255,255,255,255,255,0,192,120,0,0,0,255,255,255,255
roomtb:	.byte 7                      ; start room map offset.

; User routine.  Put your own code in here to be called with USER instruction.
; if USER has an argument it will be passed in the accumulator.

user:
	rts

; Everything below here will be generated by the editors.

script_start:

WINDOWTOP = 0
WINDOWLFT = 0
WINDOWHGT = 23
WINDOWWID = 30 ;面面面面面面面面面面面面
MAPWID = 3
        .byte 255,255,255
mapdat:
        .byte 255,0,255
        .byte 255,255,255
stmap:  .byte 2

evnt00:
        lda #1
        cmp vare
        bne a00114
        lda #2
        ldy #6
        cmp (z80_ix),y
        bne a00051
        jsr cangr
        beq :+
        jmp a00051
:
        jmp a00051
a00051: lda #3
        ldy #6
        cmp (z80_ix),y
        bne a00087
        jsr cangl
        beq :+
        jmp a00087
:
        jmp a00087
a00087: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a00114
:
a00114: ldy #8
        lda (z80_ix),y
        cmp #200
        bcs a03618
        lda #15
        ldy #8
        cmp (z80_ix),y
        bcs a00547
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #2
        ldy #6
        cmp (z80_ix),y
        bne a00298
        lda #1
        cmp vare
        bne a00235
        jsr cangr
        beq :+
        jmp a00235
:
        jmp a00235
a00235: lda #3
        ldy #6
        cmp (z80_ix),y
        bne a00271
        jsr cangl
        beq :+
        jmp a00271
:
        jmp a00271
a00271: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a00298
:
a00298: lda #0
        sta varq
        lda clock
        cmp #50
        bcs a00328
        lda #1
        sta varq
a00328: lda #100
        cmp clock
        bcs a00363
        lda clock
        cmp #150
        bcs a00363
        lda #1
        sta varq
a00363: lda #200
        cmp clock
        bcs a00384
        lda #1
        sta varq
a00384: lda #0
        cmp varq
        bne a00454
        lda #1
        jsr getob
        lda #0
        jsr getob
        ldy #9
        lda (z80_ix),y
        sta dispx
        ldy #8
        lda (z80_ix),y
        sta dispy
        lda #0
        jsr drpob
        jmp a00507
a00454: lda #1
        jsr getob
        lda #0
        jsr getob
        ldy #9
        lda (z80_ix),y
        sta dispx
        ldy #8
        lda (z80_ix),y
        sta dispy
        lda #1
        jsr drpob
a00507: lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        jmp a00563
a00547: lda #0
        jsr getob
        lda #1
        jsr getob
a00563: ldy #10
        lda (z80_ix),y
        sta varo
        ldy #10
        lda (z80_ix),y
        sta varp
        lda varo
        lsr a
        lsr a
        lsr a
        lsr a
        sta varo
        lda varo
        sta varq
        lda varq
        asl a
        asl a
        asl a
        asl a
        sta varq
        lda varq
        sta z80_c
        lda varp
        sec
        sbc z80_c
        sta varp
        lda #1
        cmp varh
        bne a00755
        lda #<50
        sta z80_l
        lda #>50
        sta z80_h
        jsr addsc
        lda #10
        sta varo
        lda #2
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #0
        sta varh
a00755: lda #8
        cmp varp
        bne a00776
        lda #0
        sta varp
a00776: lda #8
        cmp varo
        bne a00797
        lda #0
        sta varo
a00797: ldy #9
        lda (z80_ix),y
        cmp #4
        bcs a00829
        lda #240
        ldy #9
        sta (z80_ix),y
a00829: ldy #8
        lda (z80_ix),y
        cmp #16
        bcs a00868
        lda #16
        ldy #8
        sta (z80_ix),y
        lda #1
        sta varo
a00868: lda #240
        ldy #9
        cmp (z80_ix),y
        bcs a00900
        lda #4
        ldy #9
        sta (z80_ix),y
a00900: lda #160
        ldy #8
        cmp (z80_ix),y
        bcs a00927
        lda #12
        sta varo
a00927: lda varo
        cmp #8
        bcs a01100
        lda #0
        cmp varo
        bcs a00992
        jsr cangd
        beq :+
        jmp a00992
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ldy #8
        sta (z80_ix),y
a00992: lda #1
        cmp varo
        bcs a01044
        jsr cangd
        beq :+
        jmp a01044
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ldy #8
        sta (z80_ix),y
a01044: lda #2
        cmp varo
        bcs a01096
        jsr cangd
        beq :+
        jmp a01096
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ldy #8
        sta (z80_ix),y
a01096: jmp a01428
a01100: lda #8
        cmp varo
        bcs a01231
        lda #0
        sta varq
        lda #15
        ldy #8
        cmp (z80_ix),y
        bcs a01182
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #1
        sta varq
a01182: jsr cangu
        beq :+
        jmp a01231
:
        lda #255
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
a01231: lda #9
        cmp varo
        bcs a01305
        jsr cangu
        beq :+
        jmp a01297
:
        lda #255
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        jmp a01305
a01297: lda #1
        sta varo
a01305: lda #10
        cmp varo
        bcs a01380
        jsr cangu
        beq :+
        jmp a01372
:
        lda #255
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        jmp a01380
a01372: lda #1
        sta varo
a01380: lda #1
        cmp varq
        bne a01428
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
a01428: lda varp
        cmp #8
        bcs a01640
        lda #0
        cmp varp
        bcs a01505
        jsr cangr
        beq :+
        jmp a01497
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ldy #9
        sta (z80_ix),y
        jmp a01505
a01497: lda #10
        sta varp
a01505: lda #1
        cmp varp
        bcs a01570
        jsr cangr
        beq :+
        jmp a01562
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ldy #9
        sta (z80_ix),y
        jmp a01570
a01562: lda #10
        sta varp
a01570: lda #2
        cmp varp
        bcs a01635
        jsr cangr
        beq :+
        jmp a01627
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ldy #9
        sta (z80_ix),y
        jmp a01635
a01627: lda #10
        sta varp
a01635: jmp a01863
a01640: lda #8
        cmp varp
        bcs a01714
        jsr cangl
        beq :+
        jmp a01706
:
        lda #255
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        jmp a01714
a01706: lda #2
        sta varp
a01714: lda #9
        cmp varp
        bcs a01789
        jsr cangl
        beq :+
        jmp a01781
:
        lda #255
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        jmp a01789
a01781: lda #2
        sta varp
a01789: lda #10
        cmp varp
        bcs a01863
        jsr cangl
        beq :+
        jmp a01855
:
        lda #255
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        jmp a01863
a01855: lda #2
        sta varp
a01863: lda #0
        cmp varp
        bcs a02014
        jsr cangd
        beq :+
        jmp a01894
:
        jmp a02014
a01894: ldy #9
        lda (z80_ix),y
        cmp varb
        bcs a01913
a01913: lda varb
        ldy #9
        cmp (z80_ix),y
        bcs a01931
a01931: lda #0
        sta varq
        lda #0
        cmp varc
        bne a01960
        lda #1
        sta varq
a01960: lda #2
        cmp varc
        bne a01981
        lda #1
        sta varq
a01981: lda #1
        cmp varq
        bne a02014
        lda #5
        asl a
        sta sndtyp
        lda #0
        jsr animsp
a02014: lda joyval
        and #1
        beq :+
        jmp a02333
:
        lda #8
        cmp varp
        bcs a02215
        jsr cangd
        beq :+
        jmp a02087
:
        lda #255
        sta z80_c
        lda varp
        clc
        adc z80_c
        sta varp
        jmp a02211
a02087: lda joyval
        and #2
        beq :+
        jmp a02109
:
        jmp a02211
a02109: lda #3
        ldy #6
        sta (z80_ix),y
        lda #1
        ldy #7
        sta (z80_ix),y
        lda #2
        cmp varc
        bne a02161
        lda #10
        asl a
        sta sndtyp
a02161: lda #0
        cmp varc
        bne a02211
        lda #10
        asl a
        sta sndtyp
        lda #255
        sta z80_c
        lda varp
        clc
        adc z80_c
        sta varp
a02211: jmp a02229
a02215: lda #0
        ldy #6
        sta (z80_ix),y
a02229: lda varp
        cmp #3
        bcs a02328
        lda #0
        cmp varp
        bne a02300
        lda #0
        cmp varc
        bne a02287
        lda varp
        clc
        adc #1
        sta varp
        jmp a02295
a02287: lda #0
        jsr animsp
a02295: jmp a02328
a02300: lda #0
        cmp varc
        bne a02328
        lda varp
        clc
        adc #1
        sta varp
a02328: jmp a02519
a02333: lda joyval
        and #2
        beq :+
        jmp a02355
:
        jmp a02519
a02355: lda #0
        cmp varp
        bcs a02519
        jsr cangd
        beq :+
        jmp a02386
:
        jmp a02519
a02386: lda #0
        cmp varc
        bne a02436
        lda #10
        asl a
        sta sndtyp
        lda #255
        sta z80_c
        lda varp
        clc
        adc z80_c
        sta varp
a02436: lda #2
        cmp varc
        bne a02461
        lda #10
        asl a
        sta sndtyp
a02461: lda #2
        ldy #6
        sta (z80_ix),y
        lda #1
        ldy #7
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y
        cmp varb
        bcs a02519
        lda #3
        ldy #6
        sta (z80_ix),y
a02519: lda joyval
        and #2
        beq :+
        jmp a02843
:
        lda #8
        cmp varp
        bcs a02577
        lda #0
        cmp varc
        bne a02577
        lda varp
        clc
        adc #1
        sta varp
a02577: lda #11
        cmp varp
        bcs a02599
        lda #11
        sta varp
a02599: lda #0
        cmp varp
        bne a02645
        lda #0
        cmp varc
        bne a02637
        lda #9
        sta varp
        jmp a02645
a02637: lda #0
        jsr animsp
a02645: lda varp
        cmp #5
        bcs a02829
        jsr cangd
        beq :+
        jmp a02701
:
        lda #255
        sta z80_c
        lda varp
        clc
        adc z80_c
        sta varp
        jmp a02825
a02701: lda joyval
        and #1
        beq :+
        jmp a02723
:
        jmp a02825
a02723: lda #2
        ldy #6
        sta (z80_ix),y
        lda #1
        ldy #7
        sta (z80_ix),y
        lda #2
        cmp varc
        bne a02775
        lda #10
        asl a
        sta sndtyp
a02775: lda #0
        cmp varc
        bne a02825
        lda #10
        asl a
        sta sndtyp
        lda #255
        sta z80_c
        lda varp
        clc
        adc z80_c
        sta varp
a02825: jmp a02843
a02829: lda #1
        ldy #6
        sta (z80_ix),y
a02843: lda joyval
        and #16
        beq :+
        jmp a02975
:
        lda #0
        ldy #12
        cmp (z80_ix),y
        bne a02962
        lda #10
        asl a
        sta sndtyp
        lda #9
        cmp varo
        bcs a02941
        lda varo
        cmp #13
        bcs a02941
        lda #2
        sta z80_c
        lda varo
        clc
        adc z80_c
        sta varo
a02941: lda varo
        cmp #5
        bcs a02962
        lda #11
        sta varo
a02962: lda #2
        ldy #12
        sta (z80_ix),y
a02975: lda #0
        sta chary
        lda #0
        sta charx
        lda #0
        ldy #12
        cmp (z80_ix),y
        bcs a03087
        lda joyval
        and #16
        beq :+
        jmp a03033
:
        jmp a03087
a03033: ldy #12
        lda (z80_ix),y
        cmp #7
        bcs a03087
        lda #255
        sta z80_c
        ldy #12
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #12
        sta (z80_ix),y
a03087: jsr cangd
        beq :+
        jmp a03367
:
        ldy #6
        lda (z80_ix),y
        cmp #2
        bcs a03153
        lda #2
        sta z80_c
        ldy #6
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #6
        sta (z80_ix),y
a03153: lda joyval
        and #16
        beq :+
        jmp a03270
:
        ldy #7
        lda (z80_ix),y
        cmp #2
        bcs a03266
        lda #0
        cmp varc
        bne a03227
        ldy #7
        lda (z80_ix),y
        clc
        adc #1
        ldy #7
        sta (z80_ix),y
a03227: lda #2
        cmp varc
        bne a03266
        ldy #7
        lda (z80_ix),y
        clc
        adc #1
        ldy #7
        sta (z80_ix),y
a03266: jmp a03284
a03270: lda #0
        ldy #7
        sta (z80_ix),y
a03284: lda #0
        cmp varc
        bne a03362
        lda #8
        cmp varo
        bcs a03334
        lda #255
        sta z80_c
        lda varo
        clc
        adc z80_c
        sta varo
a03334: lda varo
        cmp #4
        bcs a03362
        lda varo
        clc
        adc #1
        sta varo
a03362: jmp a03530
a03367: lda #0
        ldy #12
        cmp (z80_ix),y
        bne a03393
        lda #0
        sta varo
a03393: lda #0
        cmp varp
        bne a03530
        lda #1
        ldy #6
        cmp (z80_ix),y
        bcs a03476
        lda #2
        sta z80_c
        ldy #6
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #6
        sta (z80_ix),y
        lda #0
        ldy #7
        sta (z80_ix),y
        jmp a03530
a03476: lda #1
        ldy #6
        cmp (z80_ix),y
        bcs a03530
        lda #254
        sta z80_c
        ldy #6
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #6
        sta (z80_ix),y
a03530: lda varo
        asl a
        asl a
        asl a
        asl a
        sta varo
        lda varp
        sta z80_c
        lda varo
        clc
        adc z80_c
        sta varo
        lda varo
        ldy #10
        sta (z80_ix),y
        ldy #8
        lda (z80_ix),y
        sta vara
        ldy #9
        lda (z80_ix),y
        sta varb
a03618: lda #1
        cmp vare
        bne a03719
        lda #15
        ldy #8
        cmp (z80_ix),y
        bcs a03719
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
a03719: rts
.out .sprintf("EVENT 0 SIZE = %d", (* - evnt00))

evnt01:
        ldy #10
        lda (z80_ix),y
        sta varo
        ldy #10
        lda (z80_ix),y
        sta varrnd
        lda varrnd
        lsr a
        sta varrnd
        lda varrnd
        sta varp
        lda varrnd
        asl a
        sta varrnd
        lda varrnd
        sta z80_c
        lda varo
        sec
        sbc z80_c
        sta varo
        lda varp
        sta varrnd
        lda varrnd
        lsr a
        sta varrnd
        lda varrnd
        asl a
        sta varrnd
        lda varrnd
        sta z80_c
        lda varp
        sec
        sbc z80_c
        sta varp
        lda varrnd
        lsr a
        sta varrnd
        lda varrnd
        ldy #10
        sta (z80_ix),y
        lda #0
        sta varrnd
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00235
:
        lda vara
        ldy #8
        cmp (z80_ix),y
        bcs b00235
        lda #1
        sta varrnd
b00235: lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00319
:
        lda vara
        ldy #8
        cmp (z80_ix),y
        bcs b00319
        lda #1
        sta varrnd
b00319: lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        lda #1
        cmp varrnd
        bne b00513
        lda #4
        sta z80_c
        ldy #10
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #10
        sta (z80_ix),y
        ldy #10
        lda (z80_ix),y
        jsr getob
        lda #4
        sta z80_c
        ldy #10
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #10
        sta (z80_ix),y
        lda #1
        sta varh
        lda #20
        ldy #12
        sta (z80_ix),y
        ldy #10
        lda (z80_ix),y
        asl a
        ldy #10
        sta (z80_ix),y
        lda #2
        ldy #5
        sta (z80_ix),y
        rts
b00513: lda #20
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        jsr cangu
        beq :+
        jmp b00729
:
        lda #20
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        bne b00645
        ldy #8
        lda (z80_ix),y
        cmp vara
        bcs b00637
        lda #0
        sta varp
        jmp b00645
b00637: lda #1
        sta varp
b00645: lda #20
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        bne b00729
        ldy #9
        lda (z80_ix),y
        cmp varb
        bcs b00721
        lda #0
        sta varo
        jmp b00729
b00721: lda #1
        sta varo
b00729: lda #20
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        lda #0
        cmp varo
        bne b00933
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00842
:
        lda #1
        sta varo
b00842: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        jsr cangr
        beq :+
        jmp b00921
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ldy #9
        sta (z80_ix),y
        jmp b00929
b00921: lda #1
        sta varo
b00929: jmp b01085
b00933: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00998
:
        lda #0
        sta varo
b00998: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        jsr cangl
        beq :+
        jmp b01077
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #1
        ldy #9
        sta (z80_ix),y
        jmp b01085
b01077: lda #0
        sta varo
b01085: lda #0
        cmp varp
        bne b01316
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b01163
:
        lda #1
        sta varp
b01163: lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #1
        sta varrnd
        jsr cangd
        beq :+
        jmp b01224
:
        jmp b01233
b01224: lda #0
        sta varrnd
b01233: lda #140
        ldy #8
        cmp (z80_ix),y
        bcs b01260
        lda #0
        sta varrnd
b01260: lda #1
        cmp varrnd
        bne b01304
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ldy #8
        sta (z80_ix),y
        jmp b01312
b01304: lda #1
        sta varp
b01312: jmp b01530
b01316: lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b01381
:
        lda #0
        sta varp
b01381: lda #1
        sta varrnd
        jsr cangu
        beq :+
        jmp b01408
:
        jmp b01416
b01408: lda #0
        sta varrnd
b01416: lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        lda #0
        ldy #8
        cmp (z80_ix),y
        bne b01478
        lda #0
        sta varrnd
b01478: lda #1
        cmp varrnd
        bne b01522
        ldy #8
        lda (z80_ix),y
        sec
        sbc #1
        ldy #8
        sta (z80_ix),y
        jmp b01530
b01522: lda #0
        sta varp
b01530: lda #0
        cmp varo
        bne b01560
        lda #7
        ldy #6
        sta (z80_ix),y
        jmp b01574
b01560: lda #6
        ldy #6
        sta (z80_ix),y
b01574: lda #0
        sta varrnd
        lda #0
        cmp varc
        bne b01603
        lda #1
        sta varrnd
b01603: lda #2
        cmp varc
        bne b01625
        lda #1
        sta varrnd
b01625: lda #1
        cmp varrnd
        bne b01659
        lda #1
        cmp varp
        bne b01659
        lda #0
        jsr animsp
b01659: lda #4
        sta z80_c
        ldy #10
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #10
        sta (z80_ix),y
        lda #16
        ldy #8
        cmp (z80_ix),y
        bcs b01844
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        ldy #10
        lda (z80_ix),y
        jsr getob
        ldy #9
        lda (z80_ix),y
        sta dispx
        ldy #8
        lda (z80_ix),y
        sta dispy
        ldy #10
        lda (z80_ix),y
        jsr drpob
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        jmp b01858
b01844: ldy #10
        lda (z80_ix),y
        jsr getob
b01858: lda #4
        sta z80_c
        ldy #10
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #10
        sta (z80_ix),y
        ldy #10
        lda (z80_ix),y
        asl a
        asl a
        ldy #10
        sta (z80_ix),y
        lda varo
        sta z80_c
        ldy #10
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #10
        sta (z80_ix),y
        lda varp
        sta z80_c
        ldy #10
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #10
        sta (z80_ix),y
        lda varp
        sta z80_c
        ldy #10
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #10
        sta (z80_ix),y
        rts
.out .sprintf("EVENT 1 SIZE = %d", (* - evnt01))

evnt02:
        lda #9
        ldy #6
        sta (z80_ix),y
        lda #0
        ldy #7
        sta (z80_ix),y
        ldy #10
        lda (z80_ix),y
        sta varp
        ldy #10
        lda (z80_ix),y
        lsr a
        ldy #10
        sta (z80_ix),y
        ldy #10
        lda (z80_ix),y
        sta varq
        lda varq
        asl a
        sta varq
        lda varq
        sta z80_c
        lda varp
        sec
        sbc z80_c
        sta varp
        lda #0
        ldy #12
        cmp (z80_ix),y
        bcs c00160
        ldy #12
        lda (z80_ix),y
        sec
        sbc #1
        ldy #12
        sta (z80_ix),y
c00160: lda #0
        ldy #12
        cmp (z80_ix),y
        bne c00499
        lda #0
        sta varrnd
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00236
:
        lda vara
        ldy #8
        cmp (z80_ix),y
        bcs c00236
        lda #1
        sta varrnd
c00236: lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00320
:
        lda vara
        ldy #8
        cmp (z80_ix),y
        bcs c00320
        lda #1
        sta varrnd
c00320: lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        lda #1
        cmp varrnd
        bne c00499
        lda #1
        sta varh
        lda #1
        sta varp
        lda #10
        ldy #12
        sta (z80_ix),y
        lda #10
        sta z80_c
        ldy #10
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #10
        sta (z80_ix),y
        ldy #10
        lda (z80_ix),y
        jsr getob
        lda #10
        sta z80_c
        ldy #10
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #10
        sta (z80_ix),y
        lda #3
        ldy #5
        sta (z80_ix),y
        rts
c00499: lda #168
        ldy #8
        cmp (z80_ix),y
        bcs c00660
        lda #10
        sta z80_c
        ldy #10
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #10
        sta (z80_ix),y
        ldy #10
        lda (z80_ix),y
        jsr getob
        lda #10
        sta z80_c
        ldy #10
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #10
        sta (z80_ix),y
        lda #0
        ldy #7
        sta (z80_ix),y
        lda #12
        ldy #6
        sta (z80_ix),y
        lda #220
        ldy #12
        sta (z80_ix),y
        lda #4
        ldy #5
        sta (z80_ix),y
        rts
c00660: jsr cangd
        beq :+
        jmp c01378
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ldy #8
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp c00717
:
        jmp c00806
c00717: lda #10
        sta z80_c
        ldy #10
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #10
        sta (z80_ix),y
        ldy #10
        lda (z80_ix),y
        jsr getob
        lda #10
        sta z80_c
        ldy #10
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #10
        sta (z80_ix),y
        rts
c00806: lda #30
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #1
        cmp varrnd
        bne c00884
        lda #0
        cmp varp
        bne c00876
        lda #1
        sta varp
        jmp c00884
c00876: lda #0
        sta varp
c00884: lda #0
        cmp varp
        bne c00984
        ldy #9
        lda (z80_ix),y
        cmp #240
        bcs c00972
        jsr cangr
        beq :+
        jmp c00959
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ldy #9
        sta (z80_ix),y
        jmp c00967
c00959: lda #1
        sta varp
c00967: jmp c00980
c00972: lda #1
        sta varp
c00980: jmp c01067
c00984: lda #0
        ldy #9
        cmp (z80_ix),y
        bcs c01059
        jsr cangl
        beq :+
        jmp c01046
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #1
        ldy #9
        sta (z80_ix),y
        jmp c01054
c01046: lda #0
        sta varp
c01054: jmp c01067
c01059: lda #0
        sta varp
c01067: lda #16
        ldy #8
        cmp (z80_ix),y
        bcs c01288
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #10
        sta z80_c
        ldy #10
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #10
        sta (z80_ix),y
        ldy #10
        lda (z80_ix),y
        jsr getob
        ldy #9
        lda (z80_ix),y
        sta dispx
        ldy #8
        lda (z80_ix),y
        sta dispy
        ldy #10
        lda (z80_ix),y
        jsr drpob
        lda #10
        sta z80_c
        ldy #10
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #10
        sta (z80_ix),y
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        jmp c01373
c01288: lda #10
        sta z80_c
        ldy #10
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #10
        sta (z80_ix),y
        ldy #10
        lda (z80_ix),y
        jsr getob
        lda #10
        sta z80_c
        ldy #10
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #10
        sta (z80_ix),y
c01373: jmp c01517
c01378: lda #10
        sta z80_c
        ldy #10
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #10
        sta (z80_ix),y
        ldy #10
        lda (z80_ix),y
        jsr getob
        lda #10
        sta z80_c
        ldy #10
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #10
        sta (z80_ix),y
        lda #255
        ldy #12
        sta (z80_ix),y
        lda #8
        ldy #6
        sta (z80_ix),y
        lda #0
        ldy #7
        sta (z80_ix),y
        lda #8
        ldy #5
        sta (z80_ix),y
c01517: ldy #10
        lda (z80_ix),y
        asl a
        ldy #10
        sta (z80_ix),y
        lda varp
        sta z80_c
        ldy #10
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #10
        sta (z80_ix),y
        rts
.out .sprintf("EVENT 2 SIZE = %d", (* - evnt02))

evnt03:
        lda #10
        ldy #6
        sta (z80_ix),y
        lda #0
        jsr animsp
        ldy #8
        lda (z80_ix),y
        cmp #168
        bcs d00177
        ldy #11
        lda (z80_ix),y
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        lda #0
        ldy #12
        cmp (z80_ix),y
        bcs d00132
        ldy #12
        lda (z80_ix),y
        sec
        sbc #1
        ldy #12
        sta (z80_ix),y
        jmp d00172
d00132: ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ldy #11
        sta (z80_ix),y
        lda #3
        ldy #12
        sta (z80_ix),y
d00172: jmp d00231
d00177: lda #0
        ldy #7
        sta (z80_ix),y
        lda #12
        ldy #6
        sta (z80_ix),y
        lda #220
        ldy #12
        sta (z80_ix),y
        lda #4
        ldy #5
        sta (z80_ix),y
d00231: rts
.out .sprintf("EVENT 3 SIZE = %d", (* - evnt03))

evnt04:
        lda #120
        ldy #12
        cmp (z80_ix),y
        bcs e00221
        lda #12
        ldy #6
        sta (z80_ix),y
        ldy #12
        lda (z80_ix),y
        sec
        sbc #1
        ldy #12
        sta (z80_ix),y
        lda #217
        ldy #12
        cmp (z80_ix),y
        bne e00092
        lda #1
        ldy #7
        sta (z80_ix),y
e00092: lda #214
        ldy #12
        cmp (z80_ix),y
        bne e00124
        lda #2
        ldy #7
        sta (z80_ix),y
e00124: lda #211
        ldy #12
        cmp (z80_ix),y
        bne e00157
        lda #200
        ldy #8
        sta (z80_ix),y
e00157: lda #120
        ldy #12
        cmp (z80_ix),y
        bne e00216
        lda #176
        ldy #8
        sta (z80_ix),y
        lda #13
        ldy #6
        sta (z80_ix),y
        lda #0
        ldy #7
        sta (z80_ix),y
e00216: jmp e00408
e00221: lda #2
        ldy #7
        cmp (z80_ix),y
        bne e00255
        lda #255
        ldy #5
        sta (z80_ix),y
        rts
e00255: ldy #8
        lda (z80_ix),y
        sec
        sbc #1
        ldy #8
        sta (z80_ix),y
        lda varc
        cmp #2
        bcs e00338
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ldy #9
        sta (z80_ix),y
        lda #0
        ldy #7
        sta (z80_ix),y
        jmp e00377
e00338: ldy #9
        lda (z80_ix),y
        sec
        sbc #1
        ldy #9
        sta (z80_ix),y
        lda #1
        ldy #7
        sta (z80_ix),y
e00377: lda #0
        ldy #8
        cmp (z80_ix),y
        bne e00408
        lda #2
        ldy #7
        sta (z80_ix),y
e00408: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00443
:
        lda #2
        ldy #7
        sta (z80_ix),y
e00443: rts
.out .sprintf("EVENT 4 SIZE = %d", (* - evnt04))

evnt05:
        rts
.out .sprintf("EVENT 5 SIZE = %d", (* - evnt05))

evnt06:
        rts
.out .sprintf("EVENT 6 SIZE = %d", (* - evnt06))

evnt07:
        rts
.out .sprintf("EVENT 7 SIZE = %d", (* - evnt07))

evnt08:
        lda #0
        ldy #12
        cmp (z80_ix),y
        bcs i00056
        lda #255
        sta z80_c
        ldy #12
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #12
        sta (z80_ix),y
i00056: lda #210
        ldy #12
        cmp (z80_ix),y
        bne i00083
        lda #0
        jsr animsp
i00083: lda #180
        ldy #12
        cmp (z80_ix),y
        bne i00110
        lda #0
        jsr animsp
i00110: lda #160
        ldy #12
        cmp (z80_ix),y
        bne i00182
        ldy #10
        lda (z80_ix),y
        asl a
        asl a
        ldy #10
        sta (z80_ix),y
        lda #1
        ldy #5
        sta (z80_ix),y
        lda #6
        ldy #6
        sta (z80_ix),y
i00182: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i00238
:
        lda #1
        sta varh
        lda #3
        ldy #12
        sta (z80_ix),y
        lda #3
        ldy #5
        sta (z80_ix),y
i00238: rts
.out .sprintf("EVENT 8 SIZE = %d", (* - evnt08))

evnt09:
        lda #8
        ldy #5
        cmp (z80_ix),y
        bne j00063
        lda #255
        ldy #12
        sta (z80_ix),y
        lda vari
        ldy #10
        sta (z80_ix),y
        lda vari
        clc
        adc #1
        sta vari
j00063: rts
.out .sprintf("EVENT 9 SIZE = %d", (* - evnt09))

evnt10:
        lda varc
        clc
        adc #1
        sta varc
        lda #3
        cmp varc
        bcs k00038
        lda #0
        sta varc
k00038: lda #0
        sta chary
        lda #0
        sta charx
        jsr dscor
        rts
.out .sprintf("EVENT 10 SIZE = %d", (* - evnt10))

evnt11:
        rts
.out .sprintf("EVENT 11 SIZE = %d", (* - evnt11))

evnt12:
        rts
.out .sprintf("EVENT 12 SIZE = %d", (* - evnt12))

evnt13:
        lda #0
        sta vari
        lda #0
        sta vare
        rts
.out .sprintf("EVENT 13 SIZE = %d", (* - evnt13))

evnt14:
        lda #0
        sta varq
        lda #21
        sta loopa
o00018: lda varq
        sta chary
        lda #30
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        lda varq
        clc
        adc #1
        sta varq
        dec loopa
        beq :+
        jmp o00018
:
        lda #23
        sta chary
        lda #0
        sta charx
        lda #32
        sta loopa
o00142: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #3
        jsr pattr
        dec loopa
        beq :+
        jmp o00142
:
        lda #21
        sta chary
        lda #30
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda #22
        sta chary
        lda #30
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #2
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #2
        jsr pattr
        rts
.out .sprintf("EVENT 14 SIZE = %d", (* - evnt14))

evnt15:
        rts
.out .sprintf("EVENT 15 SIZE = %d", (* - evnt15))

evnt16:
        rts
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
        .byte "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT",13
        .byte "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT",13
        .byte "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT",13
        .byte "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT",13
        .byte "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT",13
        .byte "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT",13
        .byte 13
        .byte 141
        .byte "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",13
        .byte "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",13
        .byte "A",141
nummsg:
        .byte 2
.out .sprintf("MESSAGE SIZE = %d", (* - msgdat))

chgfx:
        .byte 0,0,0,0,0,0,0,0
        .byte 0,26,165,83,148,37,170,85
        .byte 255,222,106,89,107,92,107,191
        .byte 128,97,63,2,0,96,185,30
        .byte 0,0,0,0,0,4,106,149
        .byte 213,164,1,0,0,0,0,0
        .byte 213,164,1,0,0,0,0,0
        .byte 0,0,6,31,63,49,96,96
        .byte 0,57,127,102,194,192,248,252
        .byte 0,192,224,32,87,30,28,8
        .byte 0,0,0,0,128,96,16,16
        .byte 96,33,17,49,121,97,64,128
        .byte 242,192,128,128,0,0,128,0
        .byte 8,24,4,0,0,0,0,0
        .byte 8,8,8,8,24,124,62,38
        .byte 128,128,128,71,63,31,30,60
        .byte 0,0,1,0,0,0,0,2
        .byte 0,4,4,248,112,0,0,0
        .byte 3,1,1,1,2,44,16,16
        .byte 60,60,28,28,14,7,3,0
        .byte 20,12,14,6,15,51,192,0
        .byte 4,2,1,4,8,224,0,0
        .byte 0,32,192,0,0,0,0,0
        .byte 0,0,0,16,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,40,16,40,0,0,0
        .byte 0,0,40,16,40,0,0,0
        .byte 106,213,171,213,171,213,171,86
        .byte 106,213,171,213,171,86,106,86
        .byte 60,44,52,44,20,24,24,16
bprop:
        .byte 0
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 6
        .byte 6
        .byte 6
        .byte 6
        .byte 6
        .byte 6
        .byte 6
        .byte 6
        .byte 6
        .byte 6
        .byte 6
        .byte 6
        .byte 6
        .byte 6
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
.out .sprintf("BLOCKS SIZE = %d", (* - chgfx))

sprgfx:
        .byte 7,224,8,16,18,8,20,8,20,240,17,80,17,88,9,120,7,224,2,0,5,224,2,232,6,104,7,128,0,112,3,128
        .byte 1,248,2,4,4,130,5,2,5,60,4,84,4,86,2,94,1,248,0,128,1,120,0,186,1,154,1,224,0,28,0,224
        .byte 0,126,0,129,129,32,129,64,1,79,1,21,129,21,128,151,0,126,0,32,0,94,128,46,128,102,0,120,0,7,0,56
        .byte 128,31,64,32,32,72,32,80,192,83,64,69,96,69,224,37,128,31,0,8,128,23,160,11,160,25,0,30,192,1,0,14
        .byte 7,224,8,16,18,8,20,8,20,240,17,80,17,88,9,120,7,224,12,12,9,236,3,240,27,96,22,96,0,0,0,224
        .byte 1,248,2,4,4,130,5,2,5,60,4,84,4,86,2,94,1,248,3,3,2,123,0,252,6,216,5,152,0,0,0,56
        .byte 0,126,0,129,129,32,129,64,1,79,1,21,129,21,128,151,0,126,192,192,192,158,0,63,1,182,1,102,0,0,0,14
        .byte 128,31,64,32,32,72,32,80,192,83,64,69,96,69,224,37,128,31,48,48,176,39,192,15,128,109,128,89,0,0,128,3
        .byte 7,224,8,16,18,8,20,8,20,240,17,80,17,88,9,120,55,224,54,24,16,232,3,228,3,236,11,216,13,16,6,0
        .byte 1,248,2,4,4,130,5,2,5,60,4,84,4,86,2,94,13,248,13,134,4,58,0,249,0,251,2,246,3,68,1,128
        .byte 0,126,0,129,129,32,129,64,1,79,1,21,129,21,128,151,3,126,131,97,129,14,64,62,192,62,128,189,0,209,0,96
        .byte 128,31,64,32,32,72,32,80,192,83,64,69,96,69,224,37,128,223,96,216,160,67,144,15,176,15,96,47,64,52,0,24
        .byte 7,224,8,16,16,72,16,40,15,40,10,136,26,136,30,144,7,224,0,64,7,160,23,64,22,96,1,224,14,0,1,192
        .byte 1,248,2,4,4,18,4,10,3,202,2,162,6,162,7,164,1,248,0,16,1,232,5,208,5,152,0,120,3,128,0,112
        .byte 0,126,0,129,129,4,129,2,128,242,128,168,129,168,1,233,0,126,0,4,0,122,1,116,1,102,0,30,0,224,0,28
        .byte 128,31,64,32,32,65,160,64,160,60,32,42,32,106,64,122,128,31,0,1,128,30,0,93,128,89,128,7,0,56,0,7
        .byte 7,224,8,16,16,72,16,40,15,40,10,136,26,136,30,144,7,224,48,48,55,144,15,192,6,216,6,104,0,0,7,0
        .byte 1,248,2,4,4,18,4,10,3,202,2,162,6,162,7,164,1,248,12,12,13,228,3,240,1,182,1,154,0,0,1,192
        .byte 0,126,0,129,129,4,129,2,128,242,128,168,129,168,1,233,0,126,3,3,3,121,0,252,128,109,128,102,0,0,0,112
        .byte 128,31,64,32,32,65,160,64,160,60,32,42,32,106,64,122,128,31,192,192,64,222,0,63,96,27,160,25,0,0,0,28
        .byte 7,224,8,16,16,72,16,40,15,40,10,136,26,136,30,144,7,236,24,108,23,8,39,192,55,192,27,208,8,176,0,96
        .byte 1,248,2,4,4,18,4,10,3,202,2,162,6,162,7,164,1,251,6,27,5,194,9,240,13,240,6,244,2,44,0,24
        .byte 0,126,0,129,129,4,129,2,128,242,128,168,129,168,1,233,192,126,193,134,129,112,2,124,3,124,1,189,0,139,0,6
        .byte 128,31,64,32,32,65,160,64,160,60,32,42,32,106,64,122,176,31,176,97,32,92,0,159,0,223,64,111,192,34,128,1
        .byte 0,0,32,252,49,2,50,65,110,129,36,190,18,42,9,171,4,175,6,92,7,192,7,96,3,252,0,242,0,100,0,24
        .byte 0,0,8,63,140,64,76,144,91,160,137,47,132,138,194,106,193,43,1,151,1,240,1,216,0,255,128,60,0,25,0,6
        .byte 0,0,194,15,35,16,19,36,22,232,226,75,161,34,176,154,240,74,192,101,0,124,0,118,192,63,32,15,64,6,128,1
        .byte 0,0,240,131,8,196,4,201,5,186,248,146,168,72,172,38,188,18,112,25,0,31,128,29,240,15,200,3,144,1,96,0
        .byte 1,248,2,4,4,130,5,2,5,60,4,84,4,86,7,222,2,120,4,192,9,64,19,232,63,244,115,228,0,200,0,112
        .byte 0,126,0,129,129,32,129,64,1,79,1,21,129,21,129,247,0,158,1,48,2,80,4,250,15,253,28,249,0,50,0,28
        .byte 128,31,64,32,32,72,32,80,192,83,64,69,96,69,224,125,128,39,0,76,0,148,129,62,67,255,71,62,128,12,0,7
        .byte 224,7,16,8,8,18,8,20,240,20,80,17,88,17,120,31,224,9,0,19,0,37,160,79,208,255,145,207,32,3,192,1
        .byte 1,248,2,4,4,130,5,2,5,60,4,84,4,86,7,222,127,120,240,192,41,64,7,232,7,244,3,228,0,200,0,112
        .byte 0,126,0,129,129,32,129,64,1,79,1,21,129,21,129,247,31,222,60,48,10,80,1,250,1,253,0,249,0,50,0,28
        .byte 128,31,64,32,32,72,32,80,192,83,64,69,96,69,224,125,135,247,15,12,2,148,128,126,64,127,64,62,128,12,0,7
        .byte 224,7,16,8,8,18,8,20,240,20,80,17,88,17,120,31,225,253,3,195,0,165,160,31,208,31,144,15,32,3,192,1
        .byte 0,0,63,4,64,140,130,76,129,118,125,36,84,72,213,144,245,32,58,96,3,224,6,224,63,192,79,0,38,0,24,0
        .byte 0,0,15,193,16,35,32,147,160,93,31,73,21,18,53,100,61,72,14,152,0,248,1,184,15,240,19,192,9,128,6,0
        .byte 0,0,67,240,196,8,200,36,104,23,71,210,133,68,13,89,15,82,3,166,0,62,0,110,3,252,4,240,2,96,1,128
        .byte 0,0,16,252,49,2,50,9,218,5,145,244,33,81,67,86,131,212,128,233,128,15,128,27,0,255,1,60,0,152,0,96
        .byte 31,128,32,64,65,32,64,160,60,160,42,32,106,32,123,224,30,64,3,32,2,144,23,200,47,252,39,206,19,0,14,0
        .byte 7,224,8,16,16,72,16,40,15,40,10,136,26,136,30,248,7,144,0,200,0,164,5,242,11,255,137,243,4,192,3,128
        .byte 1,248,2,4,4,18,4,10,3,202,2,162,6,162,7,190,1,228,0,50,0,41,129,124,194,255,226,124,1,48,0,224
        .byte 0,126,0,129,129,4,129,2,128,242,128,168,129,168,129,239,0,121,128,12,64,10,32,95,240,191,56,159,0,76,0,56
        .byte 31,128,32,64,65,32,64,160,60,160,42,32,106,32,123,224,30,254,3,15,2,148,23,224,47,224,39,192,19,0,14,0
        .byte 7,224,8,16,16,72,16,40,15,40,10,136,26,136,30,248,135,191,192,195,0,165,5,248,11,248,9,240,4,192,3,128
        .byte 1,248,2,4,4,18,4,10,3,202,2,162,6,162,7,190,225,239,240,48,64,41,1,126,2,254,2,124,1,48,0,224
        .byte 0,126,0,129,129,4,129,2,128,242,128,168,129,168,129,239,248,123,60,12,80,10,128,95,128,191,0,159,0,76,0,56
        .byte 3,224,4,16,8,40,8,8,7,240,2,160,2,160,7,240,11,232,17,196,18,36,52,22,46,58,19,228,10,40,6,48
        .byte 0,248,1,4,2,10,2,2,1,252,0,168,0,168,1,252,2,250,4,113,4,137,141,5,139,142,4,249,2,138,1,140
        .byte 0,62,0,65,128,130,128,128,0,127,0,42,0,42,0,127,128,190,65,28,65,34,99,65,162,227,65,62,128,162,0,99
        .byte 128,15,64,16,160,32,32,32,192,31,128,10,128,10,192,31,160,47,16,71,144,72,88,208,232,184,144,79,160,40,192,24
        .byte 0,0,3,224,4,16,8,40,8,8,7,240,122,175,42,170,19,228,9,200,28,28,38,50,36,18,18,36,11,232,6,48
        .byte 0,0,0,248,1,4,2,10,2,2,1,252,222,171,138,170,4,249,2,114,7,7,137,140,137,4,4,137,2,250,1,140
        .byte 0,0,0,62,0,65,128,130,128,128,0,127,247,170,162,170,65,62,128,156,193,193,34,99,34,65,65,34,128,190,0,99
        .byte 0,0,128,15,64,16,160,32,32,32,192,31,189,234,168,170,144,79,32,39,112,112,200,152,72,144,144,72,160,47,192,24
        .byte 0,0,0,0,3,224,4,16,8,40,8,8,7,240,14,184,18,164,35,226,77,217,118,55,36,18,18,36,11,232,6,48
        .byte 0,0,0,0,0,248,1,4,2,10,2,2,1,252,3,174,4,169,136,248,83,118,221,141,137,4,4,137,2,250,1,140
        .byte 0,0,0,0,0,62,0,65,128,130,128,128,0,127,128,235,65,42,34,62,148,221,119,99,34,65,65,34,128,190,0,99
        .byte 0,0,0,0,128,15,64,16,160,32,32,32,192,31,224,58,144,74,136,143,101,55,221,216,72,144,144,72,160,47,192,24
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,192,7,240,11,152,45,172,54,236,58,158,29,126
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,112,1,252,2,230,11,107,13,187,142,167,135,95
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,28,0,127,128,185,194,218,195,110,227,169,225,215
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,192,31,96,46,176,182,176,219,120,234,248,117
        .byte 0,0,0,0,0,0,0,0,0,0,5,224,14,248,38,204,195,86,119,118,161,79,179,127,81,111,43,111,54,247,29,254
        .byte 0,0,0,0,0,0,0,0,0,0,1,120,3,190,9,179,176,213,157,221,232,83,236,223,212,91,202,219,205,189,135,127
        .byte 0,0,0,0,0,0,0,0,0,0,0,94,128,239,194,108,108,53,103,119,250,20,251,55,245,22,242,182,115,111,225,223
        .byte 0,0,0,0,0,0,0,0,0,0,128,23,224,59,48,155,91,13,217,221,62,133,254,205,189,69,188,173,220,219,248,119
        .byte 0,128,2,176,0,188,2,190,0,83,1,85,64,93,129,83,160,95,200,109,242,237,119,219,31,62,0,254,31,204,53,228
        .byte 0,32,0,172,0,47,128,175,192,20,64,85,80,23,224,84,232,23,114,27,124,187,221,246,135,207,128,63,7,243,13,121
        .byte 0,8,0,43,192,11,224,43,48,5,80,21,212,5,56,21,250,5,220,134,223,46,183,125,225,243,224,15,193,252,67,94
        .byte 0,2,192,10,240,2,248,10,76,1,84,5,117,1,78,5,126,129,183,33,183,203,109,223,248,124,248,3,48,127,144,215
        .byte 1,8,1,8,3,216,7,176,8,194,53,230,254,54,32,43,24,92,7,48,3,104,1,152,9,240,14,224,7,0,0,0
        .byte 0,66,0,66,0,246,1,236,130,48,141,121,191,141,200,10,6,23,1,204,0,218,0,102,2,124,3,184,1,192,0,0
        .byte 128,16,128,16,128,61,0,123,32,140,99,94,111,227,178,2,193,133,0,115,128,54,128,25,0,159,0,238,0,112,0,0
        .byte 32,4,32,4,96,15,192,30,8,35,152,215,219,248,172,128,112,97,192,28,160,13,96,6,192,39,128,59,0,28,0,0
        .byte 1,8,1,8,3,216,7,176,8,192,53,224,254,48,32,32,24,86,7,55,3,100,1,152,9,240,14,224,7,0,0,0
        .byte 0,66,0,66,0,246,1,236,2,48,13,120,63,140,8,8,134,21,193,205,0,217,0,102,2,124,3,184,1,192,0,0
        .byte 128,16,128,16,128,61,0,123,0,140,3,94,15,227,2,2,97,133,112,115,64,54,128,25,0,159,0,238,0,112,0,0
        .byte 32,4,32,4,96,15,192,30,0,35,128,215,195,248,128,128,88,97,220,28,144,13,96,6,192,39,128,59,0,28,0,0
        .byte 1,8,1,8,3,216,7,176,8,192,53,224,254,48,32,32,24,112,7,56,3,157,1,203,9,246,14,228,7,0,0,0
        .byte 0,66,0,66,0,246,1,236,2,48,13,120,63,140,8,8,6,28,1,206,64,231,192,114,130,125,3,185,1,192,0,0
        .byte 128,16,128,16,128,61,0,123,0,140,3,94,15,227,2,2,1,135,128,115,208,57,176,28,96,159,64,238,0,112,0,0
        .byte 32,4,32,4,96,15,192,30,0,35,128,215,195,248,128,128,192,97,224,28,116,14,44,7,216,39,144,59,0,28,0,0
        .byte 16,128,16,128,27,192,13,224,67,16,103,172,108,127,212,4,58,24,12,224,22,192,25,128,15,144,7,112,0,224,0,0
        .byte 4,32,4,32,6,240,3,120,16,196,25,235,219,31,53,1,14,134,3,56,5,176,6,96,3,228,1,220,0,56,0,0
        .byte 1,8,1,8,1,188,0,222,4,49,198,122,246,199,77,64,131,161,0,206,1,108,1,152,0,249,0,119,0,14,0,0
        .byte 0,66,0,66,0,111,128,55,65,12,177,158,253,177,19,80,96,232,128,51,0,91,0,102,64,62,192,29,128,3,0,0
        .byte 16,128,16,128,27,192,13,224,3,16,7,172,12,127,4,4,106,24,236,224,38,192,25,128,15,144,7,112,0,224,0,0
        .byte 4,32,4,32,6,240,3,120,0,196,1,235,195,31,1,1,26,134,59,56,9,176,6,96,3,228,1,220,0,56,0,0
        .byte 1,8,1,8,1,188,0,222,0,49,192,122,240,199,64,64,134,161,14,206,2,108,1,152,0,249,0,119,0,14,0,0
        .byte 0,66,0,66,0,111,128,55,64,12,176,30,252,49,16,16,97,168,131,179,0,155,0,102,64,62,192,29,128,3,0,0
        .byte 16,128,16,128,27,192,13,224,3,16,7,172,12,127,4,4,14,24,28,224,185,192,211,128,111,144,39,112,0,224,0,0
        .byte 4,32,4,32,6,240,3,120,0,196,1,235,195,31,1,1,3,134,7,56,46,112,52,224,27,228,9,220,0,56,0,0
        .byte 1,8,1,8,1,188,0,222,0,49,192,122,240,199,64,64,128,225,1,206,11,156,13,56,6,249,2,119,0,14,0,0
        .byte 0,66,0,66,0,111,128,55,64,12,176,30,252,49,16,16,96,56,128,115,2,231,3,78,65,190,192,157,128,3,0,0
        .byte 0,1,0,1,0,123,0,246,1,24,6,188,31,198,4,4,3,12,0,246,0,66,0,132,1,30,1,60,1,226,0,124
        .byte 64,0,64,0,192,30,128,61,0,70,1,175,135,241,1,1,0,195,128,61,128,16,0,33,128,71,0,79,128,120,0,31
        .byte 16,0,16,0,176,7,96,15,128,17,192,107,97,252,64,64,192,48,96,15,32,4,64,8,224,17,192,19,32,30,192,7
        .byte 4,0,4,0,236,1,216,3,96,4,240,26,24,127,16,16,48,12,216,3,8,1,16,2,120,4,240,4,136,7,240,1
        .byte 0,0,0,1,0,1,0,123,0,246,1,24,6,188,31,198,4,4,3,12,0,246,56,66,116,142,125,28,125,226,56,124
        .byte 0,0,64,0,64,0,192,30,128,61,0,70,1,175,135,241,1,1,0,195,128,61,142,16,157,35,31,71,159,120,14,31
        .byte 0,0,16,0,16,0,176,7,96,15,128,17,192,107,97,252,64,64,192,48,96,15,35,132,231,72,199,209,39,222,195,135
        .byte 0,0,4,0,4,0,236,1,216,3,96,4,240,26,24,127,16,16,48,12,216,3,8,225,57,210,113,244,137,247,240,225
        .byte 0,1,0,1,0,123,0,246,1,24,6,188,31,198,4,4,3,12,61,246,122,66,253,132,253,30,255,60,126,226,60,124
        .byte 64,0,64,0,192,30,128,61,0,70,1,175,135,241,1,1,0,195,143,125,158,144,63,97,191,71,63,207,159,184,15,31
        .byte 16,0,16,0,176,7,96,15,128,17,192,107,97,252,64,64,192,48,99,223,39,164,79,216,239,209,207,243,39,238,195,199
        .byte 4,0,4,0,236,1,216,3,96,4,240,26,24,127,16,16,48,12,216,247,9,233,19,246,123,244,243,252,137,251,240,241
        .byte 0,0,0,32,15,160,31,96,34,216,104,156,124,126,225,254,141,62,130,198,1,216,1,156,2,12,3,0,2,0,0,0
        .byte 0,0,0,8,3,232,7,216,8,182,26,39,159,31,184,127,163,79,160,177,0,118,0,103,0,131,0,192,0,128,0,0
        .byte 0,0,0,2,0,250,1,246,130,45,198,137,231,199,238,31,232,211,104,44,128,29,192,25,192,32,0,48,0,32,0,0
        .byte 0,0,128,0,128,62,128,125,96,139,113,162,249,241,251,135,250,52,26,11,96,7,112,6,48,8,0,12,0,8,0,0
        .byte 0,0,16,0,25,192,15,32,6,180,11,52,105,204,68,112,210,32,218,32,191,208,31,216,31,188,15,20,6,4,0,0
        .byte 0,0,4,0,6,112,3,200,1,173,2,205,26,115,17,28,52,136,54,136,47,244,7,246,7,239,3,197,1,129,0,0
        .byte 0,0,1,0,1,156,0,242,64,107,64,179,198,156,4,71,13,34,13,162,11,253,129,253,193,251,64,241,64,96,0,0
        .byte 0,0,0,64,0,103,128,60,208,26,208,44,49,167,193,17,131,72,131,104,66,255,96,127,240,126,80,60,16,24,0,0
        .byte 0,0,16,0,25,192,15,32,6,180,11,52,105,204,68,112,210,34,218,70,191,187,31,184,31,208,15,0,6,0,0,0
        .byte 0,0,4,0,6,112,3,200,1,173,2,205,26,115,17,28,180,136,182,145,239,238,7,238,7,244,3,192,1,128,0,0
        .byte 0,0,1,0,1,156,0,242,64,107,64,179,198,156,4,71,45,34,109,164,187,251,129,251,1,253,0,240,0,96,0,0
        .byte 0,0,0,64,0,103,128,60,208,26,208,44,49,167,193,17,139,72,27,105,238,254,224,126,64,127,0,60,0,24,0,0
        .byte 0,0,0,0,0,0,3,224,6,176,6,176,11,104,25,204,9,72,4,144,3,96,0,128,11,232,4,16,0,0,0,0
        .byte 0,0,0,0,0,0,0,248,1,172,1,172,2,218,6,115,2,82,1,36,0,216,0,32,2,250,1,4,0,0,0,0
        .byte 0,0,0,0,0,0,0,62,0,107,0,107,128,182,193,156,128,148,0,73,0,54,0,8,128,190,0,65,0,0,0,0
        .byte 0,0,0,0,0,0,128,15,192,26,192,26,160,45,48,103,32,37,64,18,128,13,0,2,160,47,64,16,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,5,64,4,0,8,32,9,64,4,64,0,0,4,64,5,64,5,64,4,64
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,80,1,0,2,8,2,80,1,16,0,0,1,16,1,80,1,80,1,16
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,84,0,64,0,130,0,148,0,68,0,0,0,68,0,84,0,84,0,68
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,21,0,16,128,32,0,37,0,17,0,0,0,17,0,21,0,21,0,17
        .byte 0,16,24,56,24,100,36,64,6,64,0,0,5,80,20,0,8,32,9,64,4,64,0,0,5,64,4,64,0,0,0,0
        .byte 0,4,6,14,6,25,9,16,1,144,0,0,1,84,5,0,2,8,2,80,1,16,0,0,1,80,1,16,0,0,0,0
        .byte 0,1,129,131,65,134,2,68,0,100,0,0,0,85,1,64,0,130,0,148,0,68,0,0,0,84,0,68,0,0,0,0
        .byte 64,0,224,96,144,97,0,145,0,25,0,0,64,21,0,80,128,32,0,37,0,17,0,0,0,21,0,17,0,0,0,0
        .byte 96,22,144,1,192,13,40,34,0,0,0,0,96,6,144,9,136,17,64,6,0,2,0,0,0,0,0,0,0,0,0,0
        .byte 152,5,100,0,112,3,138,8,0,0,0,0,152,1,100,2,98,4,144,1,128,0,0,0,0,0,0,0,0,0,0,0
        .byte 102,1,25,0,220,0,34,130,0,0,0,0,102,0,153,0,24,129,100,0,32,0,0,0,0,0,0,0,0,0,0,0
        .byte 89,128,6,64,55,0,136,160,0,0,0,0,25,128,38,64,70,32,25,0,8,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,7,192,24,48,32,8,64,100,64,20,128,10,128,10,128,2,144,2,80,4,72,4,32,8,24,48,7,192,0,0
        .byte 0,0,1,240,6,12,8,2,16,25,16,5,160,2,160,2,160,0,164,0,20,1,18,1,8,2,6,12,1,240,0,0
        .byte 0,0,0,124,1,131,130,0,68,6,68,1,168,0,168,0,40,0,41,0,69,0,68,128,130,0,1,131,0,124,0,0
        .byte 0,0,0,31,192,96,32,128,145,1,81,0,42,0,42,0,10,0,10,64,17,64,17,32,32,128,192,96,0,31,0,0
        .byte 15,224,16,16,32,8,64,100,64,20,128,10,128,10,128,10,160,2,160,2,144,2,80,4,72,4,32,8,24,48,7,192
        .byte 3,248,4,4,8,2,16,25,16,5,160,2,160,2,160,2,168,0,168,0,164,0,20,1,18,1,8,2,6,12,1,240
        .byte 0,254,1,1,130,0,68,6,68,1,168,0,168,0,168,0,42,0,42,0,41,0,69,0,68,128,130,0,1,131,0,124
        .byte 128,63,64,64,32,128,145,1,81,0,42,0,42,0,42,0,10,128,10,128,10,64,17,64,17,32,32,128,192,96,0,31
        .byte 4,32,6,96,114,78,16,8,0,0,192,3,96,6,0,0,0,0,96,6,192,3,0,0,16,8,114,78,6,96,4,32
        .byte 1,8,1,152,156,147,4,2,0,0,240,0,152,1,0,0,0,0,152,1,240,0,0,0,4,2,156,147,1,152,1,8
        .byte 0,66,0,102,231,36,129,0,0,0,60,0,102,0,0,0,0,0,102,0,60,0,0,0,129,0,231,36,0,102,0,66
        .byte 128,16,128,25,57,201,32,64,0,0,15,0,25,128,0,0,0,0,25,128,15,0,0,0,32,64,57,201,128,25,128,16
frmlst:
        .byte 0,3
        .byte 3,3
        .byte 6,3
        .byte 9,3
        .byte 12,3
        .byte 15,3
        .byte 18,3
        .byte 21,3
        .byte 24,3
        .byte 27,1
        .byte 28,2
        .byte 30,1
        .byte 31,3
        .byte 34,3,37,0
.out .sprintf("SPRITES SIZE = %d", (* - sprgfx))

scdat:
        .word 123,198
        .byte 255,0,80,23,255,0,5,23,255,0,6,23,255,0,5,23,255,0,4,7,8,9,10,255,0,26,11,12,13,14,255,0,26
        .byte 15,16,17,18,255,0,26,19,20,21,22,255,0,5,23,255,0,8,23,255,0,34,23,255,0,12,23,255,0,5,23,255,0,102
        .byte 255,1,14,255,0,10,23,255,0,5,6,255,5,12,6,255,0,4,23,255,0,70,23,255,0,8,23,255,0,6,23,255,0,48
        .byte 23,255,0,76,255,1,8,255,0,16,255,1,6,255,2,8,255,4,16,255,2,6
        .byte 255,0,80,23,255,0,5,23,255,0,6,23,255,0,5,23,255,0,29,1,1,255,0,11,1,1,1,255,0,7,7,8,9
        .byte 10,0,0,0,5,5,255,0,11,5,41,5,255,0,7,11,12,13,14,255,0,17,42,255,0,8,15,16,17,18,255,0,17
        .byte 43,255,0,8,19,20,21,22,0,23,255,0,12,23,255,0,5,23,255,0,18,1,1,1,255,0,27,5,41,5,255,0,28
        .byte 42,255,0,29,43,255,0,16,23,255,0,4,1,1,1,255,0,10,7,8,9,10,0,0,23,255,0,10,5,41,5,255,0,10
        .byte 11,12,13,14,255,0,14,42,255,0,11,15,16,17,18,255,0,13,23,43,255,0,7,23,0,0,0,19,20,21,22,255,0,48
        .byte 23,255,1,4,255,0,26,255,5,4,255,0,42,255,1,8,255,0,16,255,1,6,255,2,8,255,4,16,255,2,6
numsc:
        .byte 2
.out .sprintf("SCREENS SIZE = %d", (* - scdat))

nmedat:
        .byte 0,0,152,8,8,8,80,120,8,8,80,72,8,8,80,168,8,8,152,208,8,8,152,40,255
        .byte 0,0,152,8,8,8,16,72,8,8,16,176,8,8,56,120,8,8,88,64,8,8,128,120,255
.out .sprintf("POSITIONS SIZE = %d", (* - nmedat))

NUMOBJ = 34
objdta:
        .byte 0,0,0,0,30,56,59,124,125,174,125,182,255,214,255,222,255,222,255,220,127,188,63,120,30,240,8,32,4,64,2,128,255,80,88,255,80,88
        .byte 0,0,30,0,59,0,125,184,125,188,255,204,255,214,255,214,255,222,127,158,63,60,30,124,9,248,8,240,4,64,2,128,255,8,8,255,8,8
        .byte 0,0,30,0,59,0,125,128,125,128,255,192,255,192,255,192,255,192,127,128,63,0,30,0,8,0,8,0,4,0,2,0,255,0,0,255,0,0
        .byte 0,0,0,240,1,216,3,188,3,188,7,254,7,254,7,254,7,254,3,252,1,248,0,240,0,32,0,64,0,128,0,128,255,64,112,255,64,112
        .byte 0,0,3,192,7,224,15,176,15,208,31,216,31,248,31,248,31,248,15,240,7,224,7,224,3,192,1,0,0,128,1,0,255,64,144,255,64,144
        .byte 0,0,3,192,7,224,15,176,15,208,31,216,31,248,31,248,31,248,15,240,7,224,7,224,3,192,1,0,0,128,1,0,255,0,0,255,0,0
        .byte 0,0,3,192,7,224,15,176,15,208,31,216,31,248,31,248,31,248,15,240,7,224,7,224,3,192,1,0,0,128,1,0,255,0,0,255,0,0
        .byte 0,0,3,192,7,224,15,176,15,208,31,216,31,248,31,248,31,248,15,240,7,224,7,224,3,192,1,0,0,128,1,0,255,0,0,255,0,0
        .byte 0,0,3,192,7,224,15,176,15,208,31,216,31,248,31,248,31,248,15,240,7,224,7,224,3,192,1,0,0,128,1,0,255,0,0,255,0,0
        .byte 0,0,3,192,7,224,15,176,15,208,31,216,31,248,31,248,31,248,15,240,7,224,7,224,3,192,1,0,0,128,1,0,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,5,64,27,176,55,216,119,220,111,236,239,238,144,18,110,236,17,16,9,32,5,64,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,5,64,27,176,55,216,119,220,111,236,239,238,144,18,110,236,17,16,9,32,5,64,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,5,64,27,176,55,216,119,220,111,236,239,238,144,18,110,236,17,16,9,32,5,64,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,5,64,27,176,55,216,119,220,111,236,239,238,144,18,110,236,17,16,9,32,5,64,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,5,64,27,176,55,216,119,220,111,236,239,238,144,18,110,236,17,16,9,32,5,64,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,5,64,27,176,55,216,119,220,111,236,239,238,144,18,110,236,17,16,9,32,5,64,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,3,224,6,176,6,176,11,104,25,204,9,72,4,144,3,96,0,128,11,232,4,16,0,0,0,0,255,0,0,255,0,0
        .byte 0,0,3,192,5,96,10,176,13,80,26,168,21,88,26,168,21,88,26,168,13,80,10,176,5,96,2,192,1,128,1,0,255,32,24,255,32,24
        .byte 0,0,3,192,4,32,8,16,8,80,16,40,16,40,16,8,16,8,16,8,8,16,8,16,4,32,2,64,1,128,1,0,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,59,184,34,168,58,168,10,168,59,184,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,59,184,10,40,11,168,8,168,11,184,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,46,238,42,170,42,170,42,170,46,238,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,46,238,40,170,46,170,34,170,46,238,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,238,238,42,170,234,170,138,170,238,238,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,255,0,0
.out .sprintf("OBJECTS SIZE = %d", (* - objdta))

font:
        .byte 135,62,2,205,153,128,205,158
        .byte 135,201,201,201,201,201,0,24
        .byte 108,108,108,0,0,0,0,0
        .byte 108,108,254,108,254,108,108,0
        .byte 48,124,192,120,12,248,48,0
        .byte 0,198,204,24,48,102,198,0
        .byte 56,108,56,118,220,204,118,0
        .byte 96,96,192,0,0,0,0,0
        .byte 24,48,96,96,96,48,24,0
        .byte 96,48,24,24,24,48,96,0
        .byte 0,102,60,255,60,102,0,0
        .byte 0,48,48,252,48,48,0,0
        .byte 0,0,0,0,0,48,48,96
        .byte 0,0,0,252,0,0,0,0
        .byte 0,0,0,0,0,48,48,0
        .byte 6,12,24,48,96,192,128,0
        .byte 56,76,198,198,198,100,56,0
        .byte 24,56,24,24,24,24,126,0
        .byte 124,198,14,60,120,224,254,0
        .byte 126,12,24,60,6,198,124,0
        .byte 28,60,108,204,254,12,12,0
        .byte 252,192,252,6,6,198,124,0
        .byte 60,96,192,252,198,198,124,0
        .byte 254,198,12,24,48,48,48,0
        .byte 124,198,198,124,198,198,124,0
        .byte 124,198,198,126,6,12,120,0
        .byte 0,48,48,0,0,48,48,0
        .byte 0,48,48,0,0,48,48,96
        .byte 24,48,96,192,96,48,24,0
        .byte 0,0,252,0,0,252,0,0
        .byte 96,48,24,12,24,48,96,0
        .byte 120,204,12,24,48,0,48,0
        .byte 124,198,222,222,222,192,120,0
        .byte 56,108,198,198,254,198,198,0
        .byte 252,198,198,252,198,198,252,0
        .byte 60,102,192,192,192,102,60,0
        .byte 248,204,198,198,198,204,248,0
        .byte 254,192,192,252,192,192,254,0
        .byte 254,192,192,252,192,192,192,0
        .byte 62,96,192,206,198,102,62,0
        .byte 198,198,198,254,198,198,198,0
        .byte 126,24,24,24,24,24,126,0
        .byte 30,6,6,6,198,198,124,0
        .byte 198,204,216,240,248,220,206,0
        .byte 96,96,96,96,96,96,126,0
        .byte 198,238,254,254,214,198,198,0
        .byte 198,230,246,254,222,206,198,0
        .byte 124,198,198,198,198,198,124,0
        .byte 252,198,198,198,252,192,192,0
        .byte 124,198,198,198,222,204,122,0
        .byte 252,198,198,206,248,220,206,0
        .byte 120,204,192,124,6,198,124,0
        .byte 126,24,24,24,24,24,24,0
        .byte 198,198,198,198,198,198,124,0
        .byte 198,198,198,238,124,56,16,0
        .byte 198,198,214,254,254,238,198,0
        .byte 198,238,124,56,124,238,198,0
        .byte 102,102,102,60,24,24,24,0
        .byte 254,14,28,56,112,224,254,0
        .byte 120,96,96,96,96,96,120,0
        .byte 192,96,48,24,12,6,2,0
        .byte 120,24,24,24,24,24,120,0
        .byte 16,56,108,198,0,0,0,0
        .byte 0,0,0,0,0,0,0,255
        .byte 48,48,24,0,0,0,0,0
        .byte 0,0,60,102,102,102,59,0
        .byte 96,96,124,102,102,102,124,0
        .byte 0,0,62,96,96,96,62,0
        .byte 6,6,62,102,102,102,62,0
        .byte 0,0,60,102,126,96,62,0
        .byte 14,24,24,126,24,24,24,0
        .byte 0,0,62,102,102,62,6,60
        .byte 96,96,96,124,102,102,102,0
        .byte 0,24,0,24,24,24,24,0
        .byte 0,6,0,6,6,6,102,60
        .byte 96,96,98,100,104,124,102,0
        .byte 24,24,24,24,24,24,24,0
        .byte 0,0,118,107,107,107,107,0
        .byte 0,0,124,102,102,102,102,0
        .byte 0,0,60,102,102,102,60,0
        .byte 0,0,124,102,102,124,96,96
        .byte 0,0,62,102,102,62,6,6
        .byte 0,0,110,112,96,96,96,0
        .byte 0,0,60,64,60,6,124,0
        .byte 48,48,252,48,48,48,28,0
        .byte 0,0,102,102,102,102,60,0
        .byte 0,0,102,102,102,36,24,0
        .byte 0,0,99,107,107,107,54,0
        .byte 0,0,99,54,28,54,99,0
        .byte 0,0,102,102,44,24,48,96
        .byte 0,0,126,12,24,48,126,0
        .byte 28,48,48,224,48,48,28,0
        .byte 24,24,24,0,24,24,24,0
        .byte 224,48,48,28,48,48,224,0
        .byte 118,220,0,0,0,0,0,0
        .byte 0,16,56,108,198,198,254,0
.out .sprintf("FONT SIZE = %d", (* - font))

keys:   .byte 66,97,104,72,98,16,55,48,49,17,18
.out .sprintf("KEYS SIZE = %d", (* - keys))

data_end:
.out .sprintf("TOTAL DATA SIZE = %d", (* - data_start))
