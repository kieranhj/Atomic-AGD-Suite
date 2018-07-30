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

WINDOWTOP = 1
WINDOWLFT = 1
WINDOWHGT = 22
WINDOWWID = 30 ;面面面面面面面面面面面面
MAPWID = 13
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255
mapdat:
        .byte 255,0,8,16,24,255,255,255,255,255,255,46,255,255,1,9,17,25,255,255,255,255,255,255,255,255,255,2,10,18,26,255,255,255,255,255,255,255,255,255,3,11,19,27,255,255,255,255,255,255,255,255,255,4,12,20,28,255,255,255,255,255,255,255,255,255,5,13,21,29,255,255,255,255,255,255,255,255,255,6,14,22,30,255,255,255,255,255,255,255,255,255,7,15,23,39,40,41,42,43,44,45,255,255
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255
stmap:  .byte 1

evnt00:
        lda #0
        cmp scno
        bne a00027
        lda #150
        jsr delay
        jsr scrd
a00027: jsr skobj
        sta varobj
        lda #255
        cmp varobj
        bne a00055
        jmp a00108
a00055: lda varobj
        jsr getob
        lda #<100
        sta z80_l
        lda #>100
        sta z80_h
        jsr addsc
        lda #23
        sta chary
        lda #24
        sta charx
        jsr dscor
a00108: lda joyval
        and #1
        beq :+
        jmp a00232
:
        jsr cangr
        beq :+
        jmp a00232
:
        lda #1
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp vara
        bne a00174
        lda #0
        jsr animsp
a00174: ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #230
        ldy #9
        cmp (z80_ix),y
        bcs a00232
        jsr scrr
        lda #20
        ldy #9
        sta (z80_ix),y
a00232: lda joyval
        and #2
        beq :+
        jmp a00356
:
        jsr cangl
        beq :+
        jmp a00356
:
        lda #0
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp vara
        bne a00298
        lda #0
        jsr animsp
a00298: ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #10
        ldy #9
        cmp (z80_ix),y
        bcc a00356
        jsr scrl
        lda #220
        ldy #9
        sta (z80_ix),y
a00356: lda #7
        cmp scno
        bne a00471
        lda joyval
        and #16
        beq :+
        jmp a00391
:
        jsr jump
a00391: jsr cangd
        beq :+
        jmp a00467
:
        jsr ifall
        lda #151
        ldy #8
        cmp (z80_ix),y
        bcs a00467
        jsr cangd
        beq :+
        jmp a00467
:
        lda #8
        sta scno
        jsr nwscr
        lda #20
        ldy #8
        sta (z80_ix),y
a00467: jmp a00896
a00471: lda #15
        cmp scno
        bne a00587
        lda joyval
        and #16
        beq :+
        jmp a00506
:
        jsr jump
a00506: jsr cangd
        beq :+
        jmp a00583
:
        jsr ifall
        lda #151
        ldy #8
        cmp (z80_ix),y
        bcs a00583
        jsr cangd
        beq :+
        jmp a00583
:
        lda #16
        sta scno
        jsr nwscr
        lda #20
        ldy #8
        sta (z80_ix),y
a00583: jmp a00896
a00587: lda #23
        cmp scno
        bne a00703
        lda joyval
        and #16
        beq :+
        jmp a00622
:
        jsr jump
a00622: jsr cangd
        beq :+
        jmp a00699
:
        jsr ifall
        lda #151
        ldy #8
        cmp (z80_ix),y
        bcs a00699
        jsr cangd
        beq :+
        jmp a00699
:
        lda #24
        sta scno
        jsr nwscr
        lda #20
        ldy #8
        sta (z80_ix),y
a00699: jmp a00896
a00703: lda #157
        ldy #8
        cmp (z80_ix),y
        bcs a00753
        jsr cangd
        beq :+
        jmp a00753
:
        jsr scrd
        lda #20
        ldy #8
        sta (z80_ix),y
a00753: lda joyval
        and #16
        beq :+
        jmp a00775
:
        jsr jump
a00775: jsr cangd
        beq :+
        jmp a00793
:
        jsr ifall
a00793: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a00829
:
        lda #1
        sta deadf
a00829: lda #45
        cmp scno
        bne a00896
        lda #30
        ldy #8
        cmp (z80_ix),y
        bcs a00896
        lda #191
        ldy #9
        cmp (z80_ix),y
        bcs a00896
        lda #100
        jsr delay
        lda #1
        sta gamwon
a00896: jmp grav
.out .sprintf("EVENT 0 SIZE = %d", (* - evnt00))

evnt01:
        jsr cangd
        beq :+
        jmp b00076
:
        lda #0
        jsr animsp
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ldy #8
        sta (z80_ix),y
        jmp b00205
b00076: lda #255
        ldy #5
        sta (z80_ix),y
        lda #220
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        ldy #9
        sta (z80_ix),y
        lda #10
        ldy #8
        sta (z80_ix),y
        lda #10
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        lda #1
        sta z80_c
        lda #4
        sta z80_b
        jsr spawn
b00205: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00235
:
        lda #1
        sta deadf
b00235: rts
.out .sprintf("EVENT 1 SIZE = %d", (* - evnt01))

evnt02:
        lda #0
        ldy #11
        cmp (z80_ix),y
        bne c00175
        jsr cangl
        beq :+
        jmp c00157
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp c00117
:
        lda #1
        ldy #11
        sta (z80_ix),y
c00117: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        jmp c00171
c00157: lda #1
        ldy #11
        sta (z80_ix),y
c00171: jmp c00326
c00175: jsr cangr
        beq :+
        jmp c00313
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp c00273
:
        lda #0
        ldy #11
        sta (z80_ix),y
c00273: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        jmp c00326
c00313: lda #0
        ldy #11
        sta (z80_ix),y
c00326: lda #1
        ldy #11
        cmp (z80_ix),y
        bne c00370
        lda #0
        cmp vara
        bne c00366
        lda #0
        jsr animsp
c00366: jmp c00392
c00370: lda #0
        cmp vara
        bne c00392
        lda #0
        jsr animbk
c00392: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00422
:
        lda #1
        sta deadf
c00422: rts
.out .sprintf("EVENT 2 SIZE = %d", (* - evnt02))

evnt03:
        lda #45
        cmp scno
        bne d00239
        jsr cangl
        beq :+
        jmp d00089
:
        lda #0
        jsr animsp
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y
        sec
        sbc #1
        ldy #9
        sta (z80_ix),y
        jmp d00209
d00089: lda #255
        ldy #5
        sta (z80_ix),y
        lda #135
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #140
        ldy #9
        sta (z80_ix),y
        lda varrnd
        ldy #8
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y
        sec
        sbc #1
        ldy #9
        sta (z80_ix),y
        lda #3
        sta z80_c
        lda #6
        sta z80_b
        jsr spawn
d00209: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00239
:
        lda #1
        sta deadf
d00239: rts
.out .sprintf("EVENT 3 SIZE = %d", (* - evnt03))

evnt04:
        rts
.out .sprintf("EVENT 4 SIZE = %d", (* - evnt04))

evnt05:
        jsr cangl
        beq :+
        jmp f00102
:
        lda #0
        jsr animsp
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y
        sec
        sbc #1
        ldy #9
        sta (z80_ix),y
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ldy #8
        sta (z80_ix),y
        jmp f00248
f00102: lda #255
        ldy #5
        sta (z80_ix),y
        lda #135
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #220
        ldy #9
        sta (z80_ix),y
        lda varrnd
        ldy #8
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y
        sec
        sbc #1
        ldy #9
        sta (z80_ix),y
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ldy #8
        sta (z80_ix),y
        lda #5
        sta z80_c
        lda #6
        sta z80_b
        jsr spawn
f00248: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00278
:
        lda #1
        sta deadf
f00278: rts
.out .sprintf("EVENT 5 SIZE = %d", (* - evnt05))

evnt06:
        rts
.out .sprintf("EVENT 6 SIZE = %d", (* - evnt06))

evnt07:
        lda #0
        ldy #11
        cmp (z80_ix),y
        bne h00078
        jsr cangl
        beq :+
        jmp h00060
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp h00074
h00060: lda #1
        ldy #11
        sta (z80_ix),y
h00074: jmp h00132
h00078: jsr cangr
        beq :+
        jmp h00119
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp h00132
h00119: lda #0
        ldy #11
        sta (z80_ix),y
h00132: lda #0
        ldy #10
        cmp (z80_ix),y
        bne h00209
        jsr cangu
        beq :+
        jmp h00191
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp h00204
h00191: lda #1
        ldy #10
        sta (z80_ix),y
h00204: jmp h00271
h00209: jsr cangd
        beq :+
        jmp h00258
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #0
        jsr animsp
        jmp h00271
h00258: lda #0
        ldy #10
        sta (z80_ix),y
h00271: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp h00301
:
        lda #1
        sta deadf
h00301: rts
.out .sprintf("EVENT 7 SIZE = %d", (* - evnt07))

evnt08:
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i00080
:
        lda numlif
        clc
        adc #1
        sta numlif
        lda #23
        sta chary
        lda #8
        sta charx
        lda numlif
        jsr disply
        lda #255
        ldy #5
        sta (z80_ix),y
i00080: rts
.out .sprintf("EVENT 8 SIZE = %d", (* - evnt08))

evnt09:
        rts
.out .sprintf("EVENT 9 SIZE = %d", (* - evnt09))

evnt10:
        lda #1
        cmp vara
        bne k00027
        lda #0
        sta vara
        jmp k00042
k00027: lda vara
        clc
        adc #1
        sta vara
k00042: rts
.out .sprintf("EVENT 10 SIZE = %d", (* - evnt10))

evnt11:
        lda vare
        sec
        sbc #1
        sta vare
        rts
.out .sprintf("EVENT 11 SIZE = %d", (* - evnt11))

evnt12:
        jsr cls
        lda #3
        sta chary
        lda #12
        sta charx
        lda #5
        jsr dmsg
        lda #6
        sta chary
        lda #3
        sta charx
        lda #6
        jsr dmsg
        lda #8
        sta chary
        lda #10
        sta charx
        lda #7
        jsr dmsg
        lda #10
        sta chary
        lda #10
        sta charx
        lda #8
        jsr dmsg
        lda #15
        sta chary
        lda #0
        sta charx
        lda #3
        jsr dmsg
        lda #3
        cmp vare
        bne m00152
        jsr prskey
        jmp m00164
m00152: jsr prskey
        lda #3
        sta vare
m00164: rts
.out .sprintf("EVENT 12 SIZE = %d", (* - evnt12))

evnt13:
        lda #10
        sta numlif
        jsr cls
        lda #0
        sta vara
        lda #23
        sta chary
        lda #2
        sta charx
        lda #1
        jsr dmsg
        lda #23
        sta chary
        lda #8
        sta charx
        lda numlif
        jsr disply
        lda #23
        sta chary
        lda #18
        sta charx
        lda #2
        jsr dmsg
        lda #23
        sta chary
        lda #24
        sta charx
        jsr dscor
        rts
.out .sprintf("EVENT 13 SIZE = %d", (* - evnt13))

evnt14:
        rts
.out .sprintf("EVENT 14 SIZE = %d", (* - evnt14))

evnt15:
        rts
.out .sprintf("EVENT 15 SIZE = %d", (* - evnt15))

evnt16:
        lda #23
        sta chary
        lda #8
        sta charx
        jsr cls
        lda #23
        sta chary
        lda #2
        sta charx
        lda #1
        jsr dmsg
        lda numlif
        sec
        sbc #1
        sta numlif
        lda #23
        sta chary
        lda #8
        sta charx
        lda numlif
        jsr disply
        lda #23
        sta chary
        lda #18
        sta charx
        lda #2
        jsr dmsg
        lda #23
        sta chary
        lda #24
        sta charx
        jsr dscor
        rts
.out .sprintf("EVENT 16 SIZE = %d", (* - evnt16))

evnt17:
        jsr cls
        lda #0
        sta chary
        lda #0
        sta charx
        lda #12
        jsr dmsg
        lda #18
        sta chary
        lda #10
        sta charx
        lda #2
        jsr dmsg
        lda #18
        sta chary
        lda #16
        sta charx
        jsr dscor
        jsr prskey
        rts
.out .sprintf("EVENT 17 SIZE = %d", (* - evnt17))

evnt18:
        jsr cls
        lda #1
        sta chary
        lda #0
        sta charx
        lda #10
        jsr dmsg
        lda #19
        sta chary
        lda #9
        sta charx
        lda #2
        jsr dmsg
        lda #19
        sta chary
        lda #16
        sta charx
        jsr dscor
        jsr prskey
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
        .byte "GAME OVER",141
        .byte "LIVES",141
        .byte "SCORE",141
        .byte "           BY DAZMAN",13
        .byte "       ",13
        .byte "        HAPPY HALLOWEEN",13
        .byte 141
        .byte "SPARE",141
        .byte "OPTIONS",141
        .byte "1: KEYBOARD Q,A,O,P,SPACE",141
        .byte "2: KEMPSTON",141
        .byte "3: SINCLAIR",141
        .byte "0 PLAY GAME",141
        .byte 13
        .byte "           WELL DONE",13
        .byte 13
        .byte "        HELPING BRITTLE",13
        .byte 13
        .byte "       COMPLETE THE TASK",13
        .byte 13
        .byte "     PUMPKIN MASTER IS DEAD",13
        .byte 13
        .byte "              AND",13
        .byte 13
        .byte "  HALLOWEEN IS SAFE ONCE AGAIN",13
        .byte 13
        .byte 13
        .byte "        HAPPY HALLOWEEN",141
        .byte "TIME",141
        .byte 13
        .byte 13
        .byte 13
        .byte 13
        .byte 13
        .byte "            BAD LUCK",13
        .byte 13
        .byte "         BRITTLE FAILED",13
        .byte 13
        .byte "    THE PUMPKIN MASTER WINS",13
        .byte 13
        .byte "           THIS TIME",13
        .byte 13
        .byte 13
        .byte "             HA HA",13
        .byte 141
nummsg:
        .byte 13
.out .sprintf("MESSAGE SIZE = %d", (* - msgdat))

chgfx:
        .byte 0,0,0,0,0,0,0,0
        .byte 181,170,250,118,150,250,95,77
        .byte 0,255,213,255,155,147,1,17
        .byte 255,73,2,165,16,66,137,36
        .byte 252,74,2,161,21,66,138,37
        .byte 63,102,73,149,68,82,137,84
        .byte 181,107,90,109,106,91,236,169
        .byte 0,0,24,60,60,24,0,0
        .byte 0,255,213,255,187,147,129,255
        .byte 32,68,8,129,4,64,145,8
        .byte 85,170,85,170,85,170,85,170
        .byte 24,12,6,255,255,6,12,24
        .byte 118,109,235,167,205,161,24,118
        .byte 218,218,170,171,173,85,85,85
        .byte 0,4,132,148,134,87,95,222
        .byte 222,222,111,107,105,57,57,57
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,170,170,0,0,0
        .byte 0,0,0,170,170,0,0,0
        .byte 0,0,0,170,170,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 255,255,255,255,255,255,255,255
        .byte 127,127,63,63,31,15,7,1
        .byte 1,7,15,31,63,63,127,127
        .byte 0,0,16,0,0,0,0,0
        .byte 0,0,0,0,8,0,0,0
        .byte 54,13,1,5,0,3,1,0
        .byte 108,160,128,160,0,192,128,0
bprop:
        .byte 0
        .byte 2
        .byte 2
        .byte 1
        .byte 1
        .byte 1
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 5
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 5
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 2
.out .sprintf("BLOCKS SIZE = %d", (* - chgfx))

sprgfx:
        .byte 31,128,41,192,8,192,8,192,63,128,20,0,1,32,14,16,0,80,7,144,10,96,8,96,0,128,2,128,0,64,5,192
        .byte 7,224,10,112,2,48,2,48,15,224,5,0,0,72,3,132,0,20,1,228,2,152,2,24,0,32,0,160,0,16,1,112
        .byte 1,248,2,156,0,140,0,140,3,248,1,64,0,18,0,225,0,5,0,121,0,166,0,134,0,8,0,40,0,4,0,92
        .byte 0,126,0,167,0,35,0,35,0,254,0,80,128,4,64,56,64,1,64,30,128,41,128,33,0,2,0,10,0,1,0,23
        .byte 31,128,41,192,8,192,8,192,63,128,20,0,1,32,14,16,0,72,55,136,50,56,3,48,4,0,36,32,24,16,8,96
        .byte 7,224,10,112,2,48,2,48,15,224,5,0,0,72,3,132,0,18,13,226,12,142,0,204,1,0,9,8,6,4,2,24
        .byte 1,248,2,156,0,140,0,140,3,248,1,64,0,18,0,225,128,4,131,120,131,35,0,51,0,64,2,66,1,129,0,134
        .byte 0,126,0,167,0,35,0,35,0,254,0,80,128,4,64,56,32,1,32,222,224,200,192,12,0,16,128,144,64,96,128,33
        .byte 31,128,41,192,8,192,8,192,63,128,20,0,1,64,14,32,0,160,15,32,20,192,16,192,1,0,5,0,0,128,11,128
        .byte 7,224,10,112,2,48,2,48,15,224,5,0,0,80,3,136,0,40,3,200,5,48,4,48,0,64,1,64,0,32,2,224
        .byte 1,248,2,156,0,140,0,140,3,248,1,64,0,20,0,226,0,10,0,242,1,76,1,12,0,16,0,80,0,8,0,184
        .byte 0,126,0,167,0,35,0,35,0,254,0,80,0,5,128,56,128,2,128,60,0,83,0,67,0,4,0,20,0,2,0,46
        .byte 31,128,41,192,8,192,8,192,63,128,20,0,1,64,14,32,0,160,11,192,3,0,0,0,8,128,72,96,48,16,16,96
        .byte 7,224,10,112,2,48,2,48,15,224,5,0,0,80,3,136,0,40,2,240,0,192,0,0,2,32,18,24,12,4,4,24
        .byte 1,248,2,156,0,140,0,140,3,248,1,64,0,20,0,226,0,10,0,188,0,48,0,0,0,136,4,134,3,1,1,6
        .byte 0,126,0,167,0,35,0,35,0,254,0,80,0,5,128,56,128,2,0,47,0,12,0,0,0,34,129,33,64,192,128,65
        .byte 1,248,3,148,3,16,3,16,1,252,0,40,4,128,8,112,10,0,9,224,6,80,6,16,1,0,1,64,2,0,3,160
        .byte 0,126,0,229,0,196,0,196,0,127,0,10,1,32,2,28,2,128,2,120,1,148,1,132,0,64,0,80,0,128,0,232
        .byte 128,31,64,57,0,49,0,49,192,31,128,2,0,72,0,135,0,160,0,158,0,101,0,97,0,16,0,20,0,32,0,58
        .byte 224,7,80,14,64,12,64,12,240,7,160,0,0,18,192,33,0,40,128,39,64,25,64,24,0,4,0,5,0,8,128,14
        .byte 1,248,3,148,3,16,3,16,1,252,0,40,4,128,8,112,18,0,17,236,28,76,12,192,0,32,4,36,8,24,6,16
        .byte 0,126,0,229,0,196,0,196,0,127,0,10,1,32,2,28,4,128,4,123,7,19,3,48,0,8,1,9,2,6,1,132
        .byte 128,31,64,57,0,49,0,49,192,31,128,2,0,72,0,135,1,32,193,30,193,196,0,204,0,2,64,66,128,129,0,97
        .byte 224,7,80,14,64,12,64,12,240,7,160,0,0,18,192,33,0,72,176,71,48,113,0,51,128,0,144,16,96,32,64,24
        .byte 1,248,3,148,3,16,3,16,1,252,0,40,2,128,4,112,5,0,4,240,3,40,3,8,0,128,0,160,1,0,1,208
        .byte 0,126,0,229,0,196,0,196,0,127,0,10,0,160,1,28,1,64,1,60,0,202,0,194,0,32,0,40,0,64,0,116
        .byte 128,31,64,57,0,49,0,49,192,31,128,2,0,40,0,71,0,80,0,79,128,50,128,48,0,8,0,10,0,16,0,29
        .byte 224,7,80,14,64,12,64,12,240,7,160,0,0,10,192,17,0,20,192,19,160,12,32,12,0,2,128,2,0,4,64,7
        .byte 1,248,3,148,3,16,3,16,1,252,0,40,2,128,4,112,5,0,3,208,0,192,0,0,1,16,6,18,8,12,6,8
        .byte 0,126,0,229,0,196,0,196,0,127,0,10,0,160,1,28,1,64,0,244,0,48,0,0,0,68,129,132,2,3,1,130
        .byte 128,31,64,57,0,49,0,49,192,31,128,2,0,40,0,71,0,80,0,61,0,12,0,0,0,17,32,97,192,128,128,96
        .byte 224,7,80,14,64,12,64,12,240,7,160,0,0,10,192,17,0,20,64,15,0,3,0,0,64,4,72,24,48,32,32,24
        .byte 0,0,0,0,0,112,3,232,249,192,62,128,7,224,9,240,0,28,3,128,1,240,12,240,13,240,1,240,9,232,11,244
        .byte 0,0,0,0,0,28,0,250,62,112,15,160,1,248,2,124,0,7,0,224,0,124,3,60,3,124,0,124,2,122,2,253
        .byte 0,0,0,0,0,7,128,62,15,156,3,232,0,126,0,159,192,1,0,56,0,31,0,207,0,223,0,31,128,158,64,191
        .byte 0,0,0,0,192,1,160,15,3,231,0,250,128,31,192,39,112,0,0,14,192,7,192,51,192,55,192,7,160,39,208,47
        .byte 0,0,0,0,0,112,3,232,249,192,62,128,7,224,9,240,0,28,3,128,1,240,12,240,13,240,1,240,9,232,11,244
        .byte 0,0,0,0,0,28,0,250,62,112,15,160,1,248,2,124,0,7,0,224,0,124,3,60,3,124,0,124,2,122,2,253
        .byte 0,0,0,0,0,7,128,62,15,156,3,232,0,126,0,159,192,1,0,56,0,31,0,207,0,223,0,31,128,158,64,191
        .byte 0,0,0,0,192,1,160,15,3,231,0,250,128,31,192,39,112,0,0,14,192,7,192,51,192,55,192,7,160,39,208,47
        .byte 0,0,0,0,0,112,3,232,249,192,62,128,7,224,9,240,0,28,3,128,1,240,12,240,13,240,1,240,9,232,11,244
        .byte 0,0,0,0,0,28,0,250,62,112,15,160,1,248,2,124,0,7,0,224,0,124,3,60,3,124,0,124,2,122,2,253
        .byte 0,0,0,0,0,7,128,62,15,156,3,232,0,126,0,159,192,1,0,56,0,31,0,207,0,223,0,31,128,158,64,191
        .byte 0,0,0,0,192,1,160,15,3,231,0,250,128,31,192,39,112,0,0,14,192,7,192,51,192,55,192,7,160,39,208,47
        .byte 0,0,0,0,0,112,3,232,249,192,62,128,7,224,9,240,0,28,3,128,1,240,12,240,13,240,1,240,9,232,11,244
        .byte 0,0,0,0,0,28,0,250,62,112,15,160,1,248,2,124,0,7,0,224,0,124,3,60,3,124,0,124,2,122,2,253
        .byte 0,0,0,0,0,7,128,62,15,156,3,232,0,126,0,159,192,1,0,56,0,31,0,207,0,223,0,31,128,158,64,191
        .byte 0,0,0,0,192,1,160,15,3,231,0,250,128,31,192,39,112,0,0,14,192,7,192,51,192,55,192,7,160,39,208,47
        .byte 0,0,128,1,127,254,107,214,92,58,107,214,72,18,72,18,127,254,71,226,27,216,53,180,86,86,106,106,44,44,24,24
        .byte 0,0,96,0,159,255,154,245,151,14,154,245,146,4,146,4,159,255,145,248,6,246,13,109,149,149,154,154,11,11,6,6
        .byte 0,0,24,0,231,255,102,189,165,195,102,189,36,129,36,129,231,255,36,126,129,189,67,91,101,101,166,166,194,194,129,129
        .byte 0,0,6,0,249,255,89,175,233,112,89,175,73,32,73,32,249,255,137,31,96,111,208,214,89,89,169,169,176,176,96,96
        .byte 0,0,128,1,127,254,107,214,92,58,107,214,72,18,72,18,127,254,71,226,27,216,45,172,114,114,78,78,52,52,24,24
        .byte 0,0,96,0,159,255,154,245,151,14,154,245,146,4,146,4,159,255,145,248,6,246,11,107,156,156,147,147,13,13,6,6
        .byte 0,0,24,0,231,255,102,189,165,195,102,189,36,129,36,129,231,255,36,126,129,189,194,218,39,39,228,228,67,67,129,129
        .byte 0,0,6,0,249,255,89,175,233,112,89,175,73,32,73,32,249,255,137,31,96,111,176,182,201,201,57,57,208,208,96,96
        .byte 0,0,128,1,127,254,107,214,92,58,107,214,72,18,72,18,127,254,71,226,27,216,53,180,86,86,106,106,44,44,24,24
        .byte 0,0,96,0,159,255,154,245,151,14,154,245,146,4,146,4,159,255,145,248,6,246,13,109,149,149,154,154,11,11,6,6
        .byte 0,0,24,0,231,255,102,189,165,195,102,189,36,129,36,129,231,255,36,126,129,189,67,91,101,101,166,166,194,194,129,129
        .byte 0,0,6,0,249,255,89,175,233,112,89,175,73,32,73,32,249,255,137,31,96,111,208,214,89,89,169,169,176,176,96,96
        .byte 0,0,128,1,127,254,107,214,92,58,107,214,72,18,72,18,127,254,71,226,27,216,53,180,86,86,106,106,44,44,24,24
        .byte 0,0,96,0,159,255,154,245,151,14,154,245,146,4,146,4,159,255,145,248,6,246,13,109,149,149,154,154,11,11,6,6
        .byte 0,0,24,0,231,255,102,189,165,195,102,189,36,129,36,129,231,255,36,126,129,189,67,91,101,101,166,166,194,194,129,129
        .byte 0,0,6,0,249,255,89,175,233,112,89,175,73,32,73,32,249,255,137,31,96,111,208,214,89,89,169,169,176,176,96,96
        .byte 0,0,1,128,7,224,31,248,49,140,57,156,125,190,111,246,103,230,101,166,48,12,48,12,28,56,15,240,1,128,0,0
        .byte 0,0,0,96,1,248,7,254,12,99,14,103,159,111,155,253,153,249,153,105,12,3,12,3,7,14,3,252,0,96,0,0
        .byte 0,0,0,24,0,126,129,255,195,24,195,153,231,219,102,255,102,126,102,90,195,0,195,0,129,195,0,255,0,24,0,0
        .byte 0,0,0,6,128,31,224,127,48,198,112,230,249,246,217,191,153,159,153,150,48,192,48,192,224,112,192,63,0,6,0,0
        .byte 0,0,1,128,7,224,31,248,49,140,57,156,125,190,111,246,103,230,117,174,48,12,62,124,31,248,15,240,1,128,0,0
        .byte 0,0,0,96,1,248,7,254,12,99,14,103,159,111,155,253,153,249,157,107,12,3,15,159,7,254,3,252,0,96,0,0
        .byte 0,0,0,24,0,126,129,255,195,24,195,153,231,219,102,255,102,126,231,90,195,0,195,231,129,255,0,255,0,24,0,0
        .byte 0,0,0,6,128,31,224,127,48,198,112,230,249,246,217,191,153,159,185,214,48,192,240,249,224,127,192,63,0,6,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,198,7,12,31,219,63,158,56,159,74,140,72,192,127,192,63,240,21,128,0,16,2,176,1,224,0,0,0,0,0,0
        .byte 128,49,1,195,199,246,143,231,206,39,18,163,18,48,31,240,15,252,5,96,0,4,0,172,0,120,0,0,0,0,0,0
        .byte 96,12,192,112,177,253,227,249,243,137,196,168,4,140,7,252,3,255,1,88,0,1,0,43,0,30,0,0,0,0,0,0
        .byte 24,3,48,28,108,127,120,254,124,226,49,42,1,35,1,255,192,255,0,86,64,0,192,10,128,7,0,0,0,0,0,0
        .byte 0,192,7,14,31,220,63,155,56,158,74,159,72,204,127,192,63,240,21,128,0,0,0,0,0,16,2,176,1,224,0,0
        .byte 0,48,129,195,7,247,207,230,142,39,210,167,18,51,31,240,15,252,5,96,0,0,0,0,0,4,0,172,0,120,0,0
        .byte 0,12,224,112,193,253,179,249,227,137,244,169,196,140,7,252,3,255,1,88,0,0,0,0,0,1,0,43,0,30,0,0
        .byte 0,3,56,28,112,127,108,254,120,226,125,42,49,35,1,255,192,255,0,86,0,0,0,0,64,0,192,10,128,7,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,56,56,124,124,226,226,226,226,124,124,56,56,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,14,14,31,31,184,184,184,184,31,31,14,14,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,131,131,199,199,46,46,46,46,199,199,131,131,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,224,224,241,241,139,139,139,139,241,241,224,224,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,56,56,124,124,142,142,142,142,124,124,56,56,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,14,14,31,31,163,163,163,163,31,31,14,14,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,131,131,199,199,232,232,232,232,199,199,131,131,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,224,224,241,241,58,58,58,58,241,241,224,224,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,129,0,0,0,0,0,0,0,0,0,129,0,255,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,63,192,32,64,0,0,0,0,0,0,0,0,32,64,63,192
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,15,240,8,16,0,0,0,0,0,0,0,0,8,16,15,240
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,252,2,4,0,0,0,0,0,0,0,0,2,4,3,252
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,224,14,80,12,64,12,64,7,240,48,160,170,12
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,248,3,148,3,16,3,16,1,252,12,40,42,131
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,126,0,229,0,196,0,196,0,127,3,10,202,160
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,31,64,57,0,49,0,49,192,31,128,194,50,168
frmlst:
        .byte 0,4
        .byte 4,4
        .byte 8,4
        .byte 12,4
        .byte 16,2
        .byte 18,1
        .byte 19,2
        .byte 21,2
        .byte 23,1
        .byte 24,1,25,0
.out .sprintf("SPRITES SIZE = %d", (* - sprgfx))

scdat:
        .word 472,274,116,179,149,158,131,160,217,239,261,142,181,179,140,149,138,168,178,210,122,157,275,144,139,122
        .word 122,132,125,120,122,113,111,110,110,110,110,110,110,107,61,63,63,58,46,204,36
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,4,7,7,7,0,0,0,7,7,0,0,7,7,7,0,0,7,0,0,7
        .byte 255,0,5,6,1,255,0,4,7,0,0,7,0,7,0,0,7,0,7,0,0,7,0,7,0,0,7,255,0,5,6,1,255,0,4
        .byte 7,0,0,7,0,7,0,0,7,0,7,0,0,7,0,7,0,7,255,0,6,6,1,255,0,4,7,0,0,7,0,255,7,4
        .byte 0,7,7,7,0,0,7,7,255,0,7,6,1,255,0,4,7,0,0,7,0,7,0,0,7,0,7,0,0,7,0,7,0
        .byte 7,255,0,6,6,1,255,0,4,7,0,0,7,0,7,0,0,7,0,7,0,0,7,0,7,0,0,7,255,0,5,6,1
        .byte 255,0,4,7,0,0,7,0,7,0,0,7,0,7,0,0,7,0,7,0,0,7,255,0,5,6,1,255,0,4,7,7,7
        .byte 0,0,7,0,0,7,0,7,0,0,7,0,7,0,0,7,255,0,5,6,1,255,0,28,6,1,0,255,7,5,0,7,7,7
        .byte 0,0,7,0,0,7,7,7,0,7,0,0,7,0,7,7,7,0,6,1,0,0,0,7,0,0,0,7,0,0,7,0,0,0
        .byte 7,255,0,4,7,0,0,7,0,7,0,0,0,6,1,0,0,0,7,0,0,0,7,0,0,7,0,7,0,7,255,0,4
        .byte 7,0,7,0,0,7,0,0,0,6,1,0,0,0,7,0,0,0,7,7,7,0,0,7,0,7,255,0,4,7,7,0,0,0
        .byte 7,7,7,0,6,1,0,0,0,7,0,0,0,7,0,0,7,0,7,0,7,255,0,4,7,0,7,255,0,4,7,0,6
        .byte 1,0,0,0,7,0,0,0,7,0,0,7,0,7,0,7,255,0,4,7,0,0,7,0,0,0,7,0,6,1,0,0,0
        .byte 7,0,0,0,7,0,0,7,0,7,0,7,255,0,4,7,0,0,7,0,0,0,7,0,6,1,0,0,0,7,0,0,0
        .byte 7,0,0,7,0,7,0,0,7,7,7,0,7,0,0,7,0,7,7,7,0,6,1,255,0,28,6,1,255,0,28,6,1
        .byte 255,0,28,6
        .byte 255,0,59,23,255,0,7,25,255,0,5,25,255,0,4,25,255,0,8,20,0,22,0,0,25,255,0,14,25,255,0,7,25
        .byte 255,0,72,9,255,0,7,25,255,0,4,9,255,0,10,25,255,0,4,9,9,9,255,0,11,9,255,0,4,25,255,0,11
        .byte 9,255,0,11,9,9,255,0,13,25,0,0,9,9,255,0,10,9,10,9,255,0,5,25,255,0,8,9,9,9,255,0,9
        .byte 255,9,4,255,0,13,255,9,4,0,0,0,25,255,0,5,255,9,5,255,0,5,9,9,255,0,4,255,9,6,255,0,7
        .byte 9,9,9,10,9,9,9,0,0,0,255,9,4,0,255,9,8,255,0,5,255,9,4,10,10,10,255,9,16,10,9,9,255,0,4
        .byte 255,9,22,10,10,10,9,255,0,4,9,9,0,255,9,5,10,9,9,9,10,255,9,9,10,10,10,9,9,255,0,7,255,9,5
        .byte 10,9,10,10,255,9,14,255,0,8,9,10,255,9,4,10,255,9,4,10,10,255,9,4,10,10,9,9,255,0,9,9,10
        .byte 255,9,10,10,10,9,9,10,10,255,0,12,9,9,10,255,9,13,10,10,255,0,13,9,10,255,9,6,255,3,13,4,255,0,6
        .byte 5,255,3,9
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,10,255,2,8,255,0,10,6,1,255,0,28,6,1,255,0,28,6,1
        .byte 255,0,28,6,1,255,0,28,6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,26,8,2,6,1,255,0,4,255,2,4,255,0,17,8,2,0,6,1,255,0,24,8,2
        .byte 0,0,6,1,255,0,23,8,2,0,0,0,6,1,255,0,22,8,2,255,0,4,6,1,255,0,7,2,2,255,0,12,8
        .byte 2,255,0,5,6,1,255,0,9,2,2,255,0,9,8,2,255,0,6,6,1,255,0,11,2,2,255,0,6,2,2,255,0,7
        .byte 6,1,255,0,13,2,2,255,0,13,6,1,255,2,6,255,0,22,6,1,255,0,28,6,1,255,0,16,255,2,9,0,0,0
        .byte 6,1,255,0,28,6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,21,2,2,255,0,5,6,1,255,0,18,2,2,2,255,0,7,6,1,255,0,10,255,2,5,255,0,13,6
        .byte 1,255,0,28,6,1,255,0,6,2,2,255,0,20,6,1,255,0,28,6,1,2,2,2,255,0,25,6,1,255,0,28,6
        .byte 1,255,0,4,255,2,9,255,0,15,6,1,255,0,13,2,2,255,0,13,6,1,255,0,28,6,1,255,0,28,6,1,255,0,9
        .byte 255,2,19,6,1,255,0,28,6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,26,2,2,6,1,255,0,24,2,2,0,0,6,1,255,0,22,2,2
        .byte 255,0,4,6,1,255,0,28,6,1,255,0,16,255,2,4,255,0,8,6,1,255,0,28,6,1,255,0,10,2,2,255,0,16
        .byte 6,1,255,0,12,2,2,255,0,14,6,1,255,2,6,255,0,8,2,2,2,255,0,11,6,1,255,0,28,6,1,255,0,19
        .byte 255,2,6,0,0,0,6,1,255,0,6,255,2,5,255,0,17,6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,5,255,2,6,255,0,17,6,1,255,0,19,255,2,9,6,1,255,0,28,6,1,255,0,13,2,2,2,255,0,12
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,18,255,2,4,255,0,6,6,1,255,0,28,6,1,255,0,28,6,1
        .byte 255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,0,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,0
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,2,8,255,0,24,8,2,6,1,0,2,8,255,0,22,8
        .byte 2,0,6,1,0,0,2,8,255,0,20,8,2,0,0,6,1,0,0,0,2,8,255,0,18,8,2,0,0,0,6,1,255,0,4
        .byte 2,8,255,0,16,8,2,255,0,4,6,1,255,0,5,2,8,255,0,14,8,2,255,0,5,6,1,255,0,6,2,8,255,0,12
        .byte 8,2,255,0,6,6,1,255,0,7,2,8,255,0,10,8,2,255,0,7,6,1,255,0,8,2,8,255,0,8,8,2,255,0,8
        .byte 6,1,255,0,9,2,8,255,0,6,8,2,255,0,9,6,1,255,0,10,2,8,255,0,4,8,2,255,0,10,6,1,255,0,11
        .byte 2,2,0,0,2,2,255,0,11,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,8,255,0,5,8,255,0,5,8,255,0,5,8,255,0,5,8,0,0,0,6,1,8,255,0,5,8,255,0,5,8
        .byte 255,0,5,8,255,0,5,8,0,0,0,6,1,8,255,0,5,8,255,0,5,8,255,0,5,8,255,0,5,8,0,0,0
        .byte 6,1,8,0,0,8,0,0,8,0,0,8,0,0,8,0,0,8,0,0,8,0,0,8,0,0,8,0,0,0,6,1,8
        .byte 0,0,8,0,0,8,0,0,8,0,0,8,0,0,8,0,0,8,0,0,8,0,0,8,0,0,0,6,1,8,0,0,8
        .byte 0,0,8,0,0,8,0,0,8,0,0,8,0,0,8,0,0,8,0,0,8,0,0,0,6,1,255,2,25,0,0,0,6
        .byte 1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,5,8,2,2
        .byte 8,0,0,0,8,2,2,8,0,0,8,8,2,2,8,255,0,5,6,1,255,0,4,8,8,0,0,8,8,0,8,8,0,0
        .byte 2,2,0,8,8,0,0,8,8,255,0,4,6,1,255,0,4,8,8,0,0,8,8,0,8,8,255,0,5,8,8,0,0
        .byte 8,8,255,0,4,6,1,255,0,4,8,8,2,2,8,8,0,8,8,255,0,5,8,8,0,0,8,8,255,0,4,6,1
        .byte 255,0,4,8,8,0,0,8,8,0,8,8,0,2,8,8,0,8,8,0,0,8,8,255,0,4,6,1,255,0,4,8,8
        .byte 0,0,8,8,0,8,8,0,0,8,2,0,8,8,0,0,8,2,255,0,4,6,1,255,0,4,2,2,0,0,2,2,0,0
        .byte 255,2,4,0,0,255,2,5,255,0,5,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,2,2,255,0,7,2,2,255,0,17,6,1,255,0,16,2,2,255,0,10,6
        .byte 1,255,0,4,2,2,255,0,22,6,1,255,0,12,2,2,2,255,0,5,2,2,255,0,6,6,1,255,0,28,6,1,255,0,24
        .byte 2,2,0,0,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,0,255,2,5
        .byte 255,0,22,6,1,255,0,28,6,1,255,0,6,2,2,255,0,15,2,2,2,0,0,6,1,255,0,19,2,2,255,0,7
        .byte 6,1,255,0,8,2,255,0,7,2,255,0,11,6,1,255,0,28,6,1,255,0,9,2,255,0,4,8,2,255,0,12,6
        .byte 1,255,0,13,8,2,255,0,13,6,1,255,0,12,2,2,255,0,14,6,1,255,0,28,6,1,255,0,10,255,2,7,0,0
        .byte 255,2,7,0,0,6,1,255,0,6,2,2,255,0,20,6,1,255,0,28,6,1,2,2,0,0,2,2,255,0,22,6,1
        .byte 0,0,2,2,255,0,24,6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,13,8,8,255,0,13,6,1,255,0,12,255,8,4
        .byte 255,0,12,6,1,255,0,11,255,8,6,255,0,11,6,1,255,0,10,255,8,8,255,0,10,6,1,255,0,9,255,8,10
        .byte 255,0,9,6,1,255,0,8,255,8,12,255,0,8,6,1,255,0,7,255,8,14,255,0,7,6,1,255,0,6,255,8,16
        .byte 255,0,6,6,1,255,0,5,255,8,18,255,0,5,6,1,255,0,4,255,8,20,255,0,4,6,1,0,0,0,255,8,22
        .byte 0,0,0,6,1,0,0,255,8,24,0,0,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,24
        .byte 255,2,4,6,1,255,0,28,6,1,255,0,21,2,255,0,6,6,1,255,0,28,6,1,255,0,16,2,2,2,255,0,9
        .byte 6,1,255,0,28,6,1,255,0,13,2,255,0,14,6,1,255,0,28,6,1,255,0,8,2,2,2,255,0,17,6,1,255,0,28
        .byte 6,1,255,0,5,2,255,0,22,6,1,255,0,28,6,1,2,2,2,255,0,25,6,1,255,0,28,6,1,255,0,28,6
        .byte 1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,17
        .byte 2,255,0,10,6,1,255,0,21,2,255,0,6,6,1,255,0,7,2,2,255,0,4,2,2,255,0,13,6,1,255,0,24
        .byte 2,2,0,0,6,1,255,0,4,2,255,0,5,2,255,0,17,6,1,255,0,28,6,1,0,2,2,255,0,25,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,10,255,2,8,255,0,10,6,1,255,0,28,6,1,255,0,28,6,1
        .byte 255,0,28,6,1,255,0,28,6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,10,2,2,0,0
        .byte 255,2,4,255,0,8,2,2,6,1,255,0,28,6,1,0,0,255,2,5,255,0,13,2,2,255,0,6,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,2,19
        .byte 255,0,9,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,19,255,2,9,6,1
        .byte 255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,0,2,0,0,0,2,255,0,16
        .byte 2,0,0,0,2,2,6,1,255,0,28,6,1,255,0,8,2,255,0,9,2,255,0,9,6,1,255,0,28,6,1,255,0,11
        .byte 2,0,0,0,2,255,0,12,6,1,255,0,28,6,1,255,0,28,6,1,255,0,12,2,255,0,9,2,255,0,5,6,1
        .byte 255,0,18,2,255,0,9,6,1,255,0,15,2,255,0,9,2,0,0,6,1,255,0,28,6,1,255,0,28,6,1,0,0
        .byte 2,255,0,25,6,1,255,0,28,6,1,255,0,5,2,2,0,0,2,255,0,18,6,1,255,0,28,6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,2,4,255,0,11,2,2,2,255,0,10
        .byte 6,1,255,0,5,2,255,0,7,2,255,0,5,2,255,0,8,6,1,255,0,7,255,2,5,255,0,9,2,255,8,4,0,0
        .byte 6,1,255,0,22,255,8,4,0,0,6,1,255,0,22,2,8,8,8,0,0,6,1,255,0,23,8,8,8,0,0,6,1
        .byte 255,0,23,2,8,8,0,0,6,1,255,0,24,8,8,0,0,6,1,255,0,24,2,8,0,0,6,1,255,0,25,8,0,0
        .byte 6,1,255,0,25,2,0,0,6
        .byte 1,255,0,25,8,0,0,6,1,255,0,25,8,0,0,6,1,255,0,25,8,0,0,6,1,255,0,25,8,0,0,6,1
        .byte 255,0,25,8,0,0,6,1,255,0,25,8,0,0,6,1,255,0,25,8,0,0,6,1,255,0,25,8,0,0,6,1,255,0,25
        .byte 8,0,0,6,1,255,0,25,8,0,0,6,1,255,0,25,8,0,0,6,1,255,0,25,8,0,0,6,1,255,0,25,8
        .byte 0,0,6,1,255,0,25,8,0,0,6,1,255,0,6,2,2,2,8,255,0,15,8,0,0,6,1,255,0,9,2,2,8
        .byte 255,0,13,8,0,0,6,1,255,0,11,2,2,8,255,0,11,8,0,0,6,1,255,0,13,2,2,8,255,0,9,8,0,0
        .byte 6,1,255,0,15,2,2,8,255,0,7,2,0,0,6,1,255,0,17,255,2,4,255,0,7,6,1,255,2,6,255,0,22
        .byte 6,1,255,0,22,255,2,6,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,5,255,2,7,255,0,16,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1
        .byte 255,0,25,2,2,0,6,1,255,0,28,6,1,255,0,28,6,1,0,0,0,255,2,25,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,0,0,255,2,6,8,255,0,19,6,1,255,0,8,2,8
        .byte 255,0,18,6,1,255,0,9,2,8,255,0,17,6,1,255,0,10,2,8,255,0,16,6,1,255,0,11,2,8,255,0,15
        .byte 6,1,255,0,12,2,8,255,0,14,6,1,255,0,13,2,8,255,0,13,6,1,255,0,14,2,8,255,0,12,6,1,255,0,15
        .byte 255,2,5,255,0,8,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,12,2,8,16,255,0,13,6,1,255,0,13,8,8,255,0,13
        .byte 6,1,255,0,10,255,8,7,255,0,11,6,1,255,0,9,255,8,9,255,0,10,6,1,255,0,8,8,255,2,4,8,255,2,4
        .byte 8,255,0,9,6,1,255,0,7,8,8,255,16,4,8,255,16,4,8,8,255,0,8,6,1,255,0,7,8,8,8,16,16
        .byte 8,2,8,16,16,8,8,8,255,0,8,6,1,255,0,6,8,2,255,8,4,2,16,2,255,8,4,2,8,255,0,7,6
        .byte 1,255,0,6,8,16,255,8,4,16,16,16,255,8,4,16,8,255,0,7,6,1,255,0,6,8,16,2,255,8,9,2,16
        .byte 8,255,0,7,6,1,255,0,6,8,16,16,255,2,9,16,16,8,255,0,7,6,1,255,0,6,2,8,255,16,11,8,2
        .byte 255,0,7,6,1,255,0,7,8,255,16,11,8,255,0,8,6,1,255,0,7,2,8,255,16,9,8,2,255,0,8,6,1
        .byte 255,0,8,2,2,255,8,7,2,2,255,0,9,6,1,255,0,10,255,2,7,255,0,11,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,25,2,2,0,6,1,255,0,28
        .byte 6,1,255,0,20,2,0,0,2,255,0,4,6,1,255,0,28,6,1,255,0,18,2,255,0,9,6,1,255,0,16,2,255,0,11
        .byte 6,1,255,0,7,2,2,2,255,0,18,6,1,0,255,2,4,255,0,7,2,2,2,255,0,13,6,1,255,0,28,6,1
        .byte 255,0,28,6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,5,2,2,255,0,4,2,2,2,255,0,14,6,1,2,2,255,0,15,2,2,255,0,9,6,1,255,0,28
        .byte 6,1,255,0,22,2,2,255,0,4,6,1,255,0,28,6,1,255,0,28,6,1,255,2,19,255,0,9,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,14,255,2,14,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,2,19,255,0,9,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,6,255,2,22,6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,2,23,255,0,5,6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,25,2,2,2,6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,5,255,2,5,255,0,18,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1
        .byte 255,0,28,6,1,255,0,28,6,1,255,0,16,255,2,8,255,0,4,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,11
        .byte 255,2,6,255,0,11,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,0,0,255,2,6,255,0,12,255,2,6
        .byte 0,0,6,1,255,0,28,6,1,255,0,12,255,2,4,255,0,12,6,1,255,0,28,6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,4
        .byte 2,2,255,0,22,6,1,255,0,28,6,1,255,0,9,2,2,255,0,17,6,1,255,0,28,6,1,255,0,15,2,2,255,0,11
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,13,2,2,255,0,13,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,2,13,0,0,255,2,13
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,11
        .byte 255,2,4,255,0,13,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1
        .byte 255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,11,255,2,4,255,0,13,6,1,255,0,28,6,1,255,0,28,6
        .byte 1,255,0,22,255,16,6,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6
        .byte 1,255,0,27,16,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6
        .byte 1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28,6,1,255,0,28
        .byte 6,1,255,0,28,6
        .byte 1,255,0,28,1,1,255,0,28,1,1,255,0,28,1,1,255,0,28,1,1,255,0,28,1,1,255,0,29,1,255,0,29
        .byte 1,255,0,29,1,255,0,29,1,255,0,29,1,255,0,29,1,255,0,29,1,255,0,29,1,255,0,29,1,255,2,8,255,0,21
        .byte 1,255,0,22,255,2,5,0,0,1,255,0,10,255,2,9,255,0,10,1,255,0,29,1,255,0,29,1,255,14,29,1,255,15,29
        .byte 1,255,15,29
        .byte 255,1,75,27,255,0,14,255,1,10,27,255,0,19,255,1,5,27,255,0,252,255,2,4,255,0,22,2,2,255,0,6,2,2
        .byte 255,0,8,255,2,4,255,0,63,255,2,4,255,0,12,2,2,2,255,0,62,255,14,30,255,15,60
        .byte 255,1,60,255,0,7,26,255,1,6,27,255,0,7,26,1,1,1,27,255,0,0,0,0,2,2,2,255,0,48,2,2,255,0,24
        .byte 2,2,2,255,0,8,2,2,255,0,4,2,2,255,0,63,255,2,4,255,0,62,255,14,30,255,15,60
        .byte 255,1,60,255,0,7,26,255,1,7,27,255,0,6,26,255,1,7,255,0,0,255,0,39,255,2,4,255,0,23,2,2,255,0,38
        .byte 255,2,4,255,0,30,2,2,2,255,0,5,2,2,2,0,0,2,2,255,0,69,255,14,30,255,15,60
        .byte 255,1,64,27,255,0,5,26,255,1,14,27,255,0,221,255,2,5,255,0,49,255,2,4,255,0,10,2,2,255,0,30,2,2
        .byte 255,0,30,2,2,255,0,30,2,2,255,0,63,2,2,255,0,32,255,14,30,255,15,60
        .byte 255,1,60,255,0,5,26,255,1,6,27,255,0,0,255,0,73,255,2,5,255,0,75,2,2,2,0,0,2,2,2,255,0,13
        .byte 255,2,5,255,0,62,255,14,30,255,15,60
        .byte 255,1,60,255,0,5,26,1,1,1,27,255,0,14,26,255,1,5,255,0,27,26,1,1,255,0,25,16,16,0,26,1,255,0,25
        .byte 16,16,0,0,1,255,0,25,12,12,0,0,26,255,0,26,12,12,255,0,20,2,2,0,0,0,255,13,7,255,0,21,255,13,9
        .byte 255,0,20,255,13,10,255,0,14,2,2,0,0,0,13,13,13,0,0,0,255,13,5,255,0,10,2,2,255,0,7,13,13,13
        .byte 0,0,0,13,13,255,0,21,255,13,5,0,0,0,13,255,0,21,255,13,6,0,0,13,13,255,0,9,2,2,255,0,9
        .byte 13,13,0,255,13,9,255,0,18,13,13,0,0,255,13,8,0,0,255,2,4,255,0,12,13,13,0,0,0,255,13,7,255,0,19
        .byte 13,13,255,0,4,255,13,5,255,14,19,13,13,13,255,0,8,255,15,20,255,13,4,255,0,6,255,15,22,255,13,8
        .byte 255,0,0,255,0,144,255,20,12,255,0,12,255,20,18,255,0,12,255,20,19,255,0,12,255,20,18,255,0,12,255,20,18
        .byte 255,0,127
numsc:
        .byte 47
.out .sprintf("SCREENS SIZE = %d", (* - scdat))

nmedat:
        .byte 0,5,160,72,6,2,80,208,255
        .byte 0,1,160,96,255
        .byte 1,4,16,216,1,4,32,24,1,4,8,48,1,4,16,88,1,4,40,192,1,4,8,152,0,1,120,120,1,4,8,72,255
        .byte 1,4,136,24,0,1,112,104,1,4,64,224,1,4,72,48,255
        .byte 0,1,112,88,1,4,104,16,1,4,104,208,1,4,56,184,255
        .byte 0,0,96,152,1,4,136,24,1,4,72,224,1,4,120,104,1,4,88,192,255
        .byte 0,1,72,120,1,4,56,64,1,4,64,176,1,4,64,208,1,4,104,160,255
        .byte 0,1,104,120,1,4,112,88,1,4,112,152,1,4,112,216,1,4,136,104,8,9,128,40,255
        .byte 0,0,120,176,1,4,72,192,1,4,80,128,1,4,64,208,1,4,128,24,255
        .byte 0,1,104,56,1,4,88,80,1,4,64,32,1,4,64,208,1,4,72,152,255
        .byte 0,1,88,120,1,4,64,64,1,4,72,192,1,4,24,40,1,4,8,200,255
        .byte 0,1,88,112,1,4,136,208,1,4,112,144,1,4,104,88,1,4,128,40,255
        .byte 0,1,40,40,1,4,64,184,1,4,72,40,8,9,120,192,255
        .byte 0,1,72,120,1,4,120,144,1,4,168,152,1,4,160,72,1,4,104,112,255
        .byte 0,0,88,120,1,4,80,48,1,4,104,144,1,4,96,200,255
        .byte 0,1,56,120,1,4,80,120,1,4,96,208,1,4,112,48,8,9,120,120,255
        .byte 0,0,48,176,1,4,64,40,1,4,48,128,1,4,48,96,2,3,112,56,2,3,152,192,255
        .byte 0,1,64,96,1,4,88,152,1,4,160,56,255
        .byte 0,1,80,16,1,4,48,216,1,4,112,32,1,4,112,144,255
        .byte 0,0,160,224,1,4,48,40,1,4,32,176,1,4,16,88,255
        .byte 0,1,56,56,2,3,112,192,2,3,112,64,1,4,24,208,1,4,88,112,255
        .byte 0,1,136,152,1,4,128,200,1,4,104,56,1,4,80,216,255
        .byte 0,1,16,112,2,3,128,136,255
        .byte 0,1,136,112,1,4,104,96,1,4,72,160,1,4,80,80,8,9,136,24,255
        .byte 0,1,48,104,1,4,128,24,1,4,80,168,2,3,136,192,255
        .byte 0,1,32,24,2,3,32,56,2,3,72,168,2,3,120,56,255
        .byte 7,7,128,48,7,7,72,176,7,7,136,136,0,1,56,64,255
        .byte 0,1,96,120,2,3,128,40,2,3,128,208,7,7,40,40,255
        .byte 0,1,40,48,7,7,32,192,7,7,128,40,8,9,72,136,255
        .byte 0,1,104,120,2,3,136,32,2,3,136,80,2,3,136,224,2,3,136,144,255
        .byte 0,1,40,112,1,4,120,168,1,4,96,208,1,4,152,64,1,4,96,32,255
        .byte 255,255,255,255,255,255,255,255,0,1,120,120,5,6,32,216,1,4,56,32,1,4,72,208,255
        .byte 0,1,104,24,5,6,136,224,8,9,88,160,255
        .byte 0,1,104,24,5,6,80,208,255
        .byte 0,1,112,24,5,6,128,208,255
        .byte 0,1,88,24,5,6,72,232,255
        .byte 0,1,120,24,5,6,88,224,8,9,96,112,255
        .byte 0,1,128,24,7,7,56,32,3,6,40,136,255
        .byte 0,1,128,120,255
.out .sprintf("POSITIONS SIZE = %d", (* - nmedat))

NUMOBJ = 41
objdta:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,3,129,16,3,129,16
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,3,56,224,3,56,224
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,4,96,16,4,96,16
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,4,48,184,4,48,184
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,5,128,24,5,128,24
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,5,64,224,5,64,224
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,6,48,72,6,48,72
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,6,56,216,6,56,216
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,7,104,56,7,104,56
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,8,72,16,8,72,16
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,7,104,184,7,104,184
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,8,72,224,8,72,224
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,9,152,24,9,152,24
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,9,152,48,9,152,48
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,9,152,72,9,152,72
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,9,152,96,9,152,96
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,9,152,120,9,152,120
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,9,152,144,9,152,144
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,9,152,168,9,152,168
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,9,152,192,9,152,192
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,11,64,16,11,64,16
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,11,88,176,11,88,176
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,12,56,208,12,56,208
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,12,152,32,12,152,32
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,14,136,16,14,136,16
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,14,40,224,14,40,224
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,15,88,24,15,88,24
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,15,64,208,15,64,208
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,16,32,224,16,32,224
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,16,112,16,16,112,16
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,17,144,56,17,144,56
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,17,32,224,17,32,224
        .byte 0,0,32,0,0,8,1,28,16,8,56,0,17,128,0,193,7,240,37,208,15,120,9,200,4,16,70,48,1,194,0,0,45,40,208,45,40,208
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,20,88,216,20,88,216
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,21,72,40,21,72,40
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,23,80,216,23,80,216
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,24,56,16,24,56,16
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,26,104,184,26,104,184
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,30,144,112,30,144,112
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,28,136,200,28,136,200
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,0,192,7,240,5,208,15,120,9,200,4,16,6,48,1,192,0,0,39,104,24,39,104,24
.out .sprintf("OBJECTS SIZE = %d", (* - objdta))

font:
        .byte 0,0,0,0,0,0,0,0
        .byte 0,48,48,48,48,0,48,0
        .byte 0,108,108,0,0,0,0,0
        .byte 0,108,254,108,108,254,108,0
        .byte 0,24,126,120,126,30,126,24
        .byte 0,230,236,24,48,110,206,0
        .byte 0,48,120,48,126,204,126,0
        .byte 0,24,48,0,0,0,0,0
        .byte 0,12,24,24,24,24,12,0
        .byte 0,96,48,48,48,48,96,0
        .byte 0,0,60,24,126,24,60,0
        .byte 0,0,24,24,126,24,24,0
        .byte 0,0,0,0,0,24,24,48
        .byte 0,0,0,0,126,0,0,0
        .byte 0,0,0,0,0,56,56,0
        .byte 0,0,6,12,24,48,96,0
        .byte 0,124,206,222,246,230,124,80
        .byte 0,56,120,24,88,24,126,34
        .byte 0,124,214,6,124,192,254,70
        .byte 0,124,198,92,22,198,124,36
        .byte 0,24,56,120,216,254,88,72
        .byte 0,254,192,252,22,198,124,36
        .byte 0,124,192,252,206,198,124,72
        .byte 0,254,166,142,24,50,50,0
        .byte 0,124,198,124,214,198,124,36
        .byte 0,124,214,198,126,6,124,80
        .byte 0,0,0,48,0,0,48,0
        .byte 0,0,48,0,0,48,48,96
        .byte 0,0,12,24,48,24,12,0
        .byte 0,0,0,126,0,126,0,0
        .byte 0,0,48,24,12,24,48,0
        .byte 0,124,198,12,24,0,24,0
        .byte 0,124,222,254,254,192,124,0
        .byte 0,124,230,198,254,214,198,66
        .byte 0,252,198,252,214,198,254,132
        .byte 0,124,246,208,192,198,252,36
        .byte 0,248,236,198,198,206,248,160
        .byte 0,254,194,250,200,192,254,144
        .byte 0,254,194,252,196,192,196,64
        .byte 0,124,214,192,222,198,126,34
        .byte 0,198,198,254,214,198,214,82
        .byte 0,126,26,90,24,24,126,34
        .byte 0,6,6,6,198,198,124,36
        .byte 0,204,216,240,216,220,198,144
        .byte 0,192,192,192,192,192,254,66
        .byte 0,198,238,254,214,198,214,18
        .byte 0,198,230,246,222,206,206,136
        .byte 0,124,214,198,214,198,124,36
        .byte 0,252,198,198,252,212,192,64
        .byte 0,124,214,198,246,222,124,4
        .byte 0,252,230,198,252,204,198,66
        .byte 0,124,192,124,22,198,124,32
        .byte 0,254,182,50,176,50,48,16
        .byte 0,198,198,198,198,198,124,64
        .byte 0,198,198,198,198,236,120,72
        .byte 0,198,198,198,198,254,108,64
        .byte 0,198,110,58,56,108,198,128
        .byte 0,134,204,122,48,50,48,18
        .byte 0,254,140,152,48,96,254,66
        .byte 0,30,24,24,24,24,30,0
        .byte 0,0,192,96,48,24,12,0
        .byte 0,240,48,48,48,48,240,0
        .byte 0,48,120,252,48,48,48,0
        .byte 0,0,0,0,0,0,0,255
        .byte 0,60,102,248,96,96,254,0
        .byte 0,0,120,12,124,204,124,0
        .byte 0,96,96,124,102,102,124,0
        .byte 0,0,60,96,96,96,60,0
        .byte 0,12,12,124,204,204,124,0
        .byte 0,0,120,204,248,192,124,0
        .byte 0,28,48,56,48,48,48,0
        .byte 0,0,124,204,204,124,12,120
        .byte 0,192,192,248,204,204,204,0
        .byte 0,48,0,112,48,48,120,0
        .byte 0,12,0,12,12,12,108,56
        .byte 0,96,120,112,112,120,108,0
        .byte 0,48,48,48,48,48,28,0
        .byte 0,0,248,252,252,252,252,0
        .byte 0,0,248,204,204,204,204,0
        .byte 0,0,120,204,204,204,120,0
        .byte 0,0,248,204,204,248,192,192
        .byte 0,0,124,204,204,124,12,14
        .byte 0,0,60,96,96,96,96,0
        .byte 0,0,120,192,120,12,248,0
        .byte 0,48,120,48,48,48,28,0
        .byte 0,0,204,204,204,204,120,0
        .byte 0,0,204,204,120,120,48,0
        .byte 0,0,204,252,252,252,120,0
        .byte 0,0,204,120,48,120,204,0
        .byte 0,0,204,204,204,124,12,120
        .byte 0,0,252,24,48,96,252,0
        .byte 0,30,24,112,24,24,30,0
        .byte 0,24,24,24,24,24,24,0
        .byte 0,240,48,28,48,48,240,0
        .byte 0,60,120,0,0,0,0,0
        .byte 124,198,187,227,227,187,198,124
.out .sprintf("FONT SIZE = %d", (* - font))

keys:   .byte 66,97,104,72,98,16,55,48,49,17,18
.out .sprintf("KEYS SIZE = %d", (* - keys))

data_end:
.out .sprintf("TOTAL DATA SIZE = %d", (* - data_start))
