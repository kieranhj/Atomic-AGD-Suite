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
WINDOWWID = 26 ;面面面面面面面面面面面面
MAPWID = 3
        .byte 255,255,255
mapdat:
        .byte 255,0,255
        .byte 255,255,255
stmap:  .byte 1

evnt00:
        lda #4
        cmp varg
        beq *+5
        jmp a00095
        lda #0
        ; ldy #6
        ldy #6
        sta (z80_ix),y
        lda #0
        ; ldy #7
        iny
        sta (z80_ix),y
        lda #176
        ; ldy #8
        iny
        sta (z80_ix),y
        lda #0
        ; ldy #9
        iny
        sta (z80_ix),y
        lda #0
        sta varg
a00095: lda #3
        cmp varg
        beq *+5
        jmp a00146
        lda #19
        ; ldy #6
        ldy #6
        sta (z80_ix),y
        lda #1
        ; ldy #7
        iny
        sta (z80_ix),y
a00146: ; ldy #8
        ldy #8
        lda (z80_ix),y
        sta vara
        ; ldy #9
        iny
        lda (z80_ix),y
        sta varb
        lda #1
        cmp varl
        beq *+5
        jmp a00298
        lda #8
        sta z80_c
        lda #9
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #40
        ; ldy #8
        dey
        sta (z80_ix),y
        lda #96
        ; ldy #9
        iny
        sta (z80_ix),y
        lda #99
        sta varl
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
a00298: lda varg
        cmp #2
        bcc *+5
        jmp a01164
        jsr skobj
        sta varobj
        lda #8
        cmp varobj
        beq *+5
        jmp a00468
        lda varobj
        jsr getob
        lda #3
        sta z80_c
        lda vark
        clc
        adc z80_c
        sta vark
        lda #7
        sta chary
        lda #29
        sta charx
        lda #24
        jsr dmsg
        lda #7
        sta chary
        lda #29
        sta charx
        lda vark
        jsr disply
        lda #50
        asl a
        sta sndtyp
        lda #15
        cmp vark
        bcc *+5
        jmp a00463
        lda #15
        sta vark
a00463: jmp a00564
a00468: lda #88
        ; ldy #8
        ldy #8
        sta (z80_ix),y
        lda #96
        ; ldy #9
        iny
        sta (z80_ix),y
        jsr skobj
        sta varobj
        lda #8
        cmp varobj
        beq *+5
        jmp a00529
a00529: lda vara
        ; ldy #8
        ldy #8
        sta (z80_ix),y
        lda varb
        ; ldy #9
        iny
        sta (z80_ix),y
a00564: lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp a01164
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        sta varo
        ; ldy #9
        iny
        lda (z80_ix),y
        sta varp
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda varo
        sta varq
        ; ldy #8
        dey
        lda (z80_ix),y
        cmp varq
        bcc *+5
        jmp a00728
        ; ldy #8
        lda (z80_ix),y
        sta z80_c
        lda varq
        sec
        sbc z80_c
        sta varq
        jmp a00771
a00728: ; ldy #8
        ldy #8
        lda (z80_ix),y
        sta varq
        lda varo
        sta z80_c
        lda varq
        sec
        sbc z80_c
        sta varq
a00771: lda varq
        cmp #12
        bcc *+5
        jmp a00901
        lda varp
        sta varq
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        cmp varq
        bcc *+5
        jmp a00858
        ; ldy #9
        lda (z80_ix),y
        sta z80_c
        lda varq
        sec
        sbc z80_c
        sta varq
        jmp a00901
a00858: ; ldy #9
        ldy #9
        lda (z80_ix),y
        sta varq
        lda varp
        sta z80_c
        lda varq
        sec
        sbc z80_c
        sta varq
a00901: lda varq
        cmp #12
        bcc *+5
        jmp a01164
        lda #19
        ; ldy #6
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp varg
        beq *+5
        jmp a00961
        lda #3
        sta varg
a00961: lda #1
        cmp varg
        beq *+5
        jmp a00985
        lda #6
        sta varg
a00985: lda #6
        sta z80_c
        lda #0
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #200
        ; ldy #12
        ldy #12
        sta (z80_ix),y
        lda #0
        ; ldy #6
        ldy #6
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #19
        ; ldy #6
        sta (z80_ix),y
        lda #1
        ; ldy #7
        iny
        sta (z80_ix),y
        lda #0
        ; ldy #8
        iny
        sta (z80_ix),y
        lda #240
        ; ldy #9
        iny
        sta (z80_ix),y
        lda #3
        cmp varg
        beq *+5
        jmp a01164
        rts
a01164: lda varh
        sta vars
        lda varh
        sta vart
        lda vars
        lsr a
        lsr a
        lsr a
        lsr a
        sta vars
        lda vars
        sta varq
        lda varq
        asl a
        asl a
        asl a
        asl a
        sta varq
        lda varq
        sta z80_c
        lda vart
        sec
        sbc z80_c
        sta vart
        lda #3
        cmp scno
        beq *+5
        jmp a01284
        lda #99
        sta vart
a01284: lda #1
        cmp varg
        bcc *+5
        jmp a01321
        lda varg
        cmp #5
        bcc *+5
        jmp a01321
        rts
a01321: lda #255
        sta varobj
        lda #0
        cmp varg
        beq *+5
        jmp a01374
        jsr skobj
        sta varobj
        lda #255
        ; ldy #10
        ldy #10
        sta (z80_ix),y
a01374: lda #24
        ; ldy #9
        ldy #9
        sta (z80_ix),y
        lda #24
        ; ldy #8
        dey
        sta (z80_ix),y
        lda #1
        cmp scno
        bcc *+5
        jmp a01440
        lda #48
        ; ldy #8
        sta (z80_ix),y
a01440: lda #5
        cmp varg
        beq *+5
        jmp a01509
        lda #0
        jsr getob
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        sta dispx
        ; ldy #8
        dey
        lda (z80_ix),y
        sta dispy
        lda #0
        jsr drpob
a01509: lda #6
        cmp varg
        beq *+5
        jmp a01595
        lda #0
        ; ldy #10
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a01595
        ; ldy #9
        dey
        lda (z80_ix),y
        sta dispx
        ; ldy #8
        dey
        lda (z80_ix),y
        sta dispy
        lda #0
        jsr drpob
a01595: lda #0
        sta vard
        lda #0
        cmp varobj
        beq *+5
        jmp a01672
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        sta varo
        ; ldy #9
        iny
        lda (z80_ix),y
        sta varp
        lda #2
        sta varg
        lda #1
        sta vard
a01672: lda #168
        ; ldy #9
        ldy #9
        sta (z80_ix),y
        lda #1
        cmp varobj
        beq *+5
        jmp a01758
        ; ldy #8
        dey
        lda (z80_ix),y
        sta varo
        ; ldy #9
        iny
        lda (z80_ix),y
        sta varp
        lda #2
        sta varg
        lda #1
        sta vard
a01758: lda #5
        cmp varg
        beq *+5
        jmp a01827
        lda #1
        jsr getob
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        sta dispx
        ; ldy #8
        dey
        lda (z80_ix),y
        sta dispy
        lda #1
        jsr drpob
a01827: lda #6
        cmp varg
        beq *+5
        jmp a01913
        lda #1
        ; ldy #10
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a01913
        ; ldy #9
        dey
        lda (z80_ix),y
        sta dispx
        ; ldy #8
        dey
        lda (z80_ix),y
        sta dispy
        lda #1
        jsr drpob
a01913: lda #152
        ; ldy #8
        ldy #8
        sta (z80_ix),y
        lda #1
        cmp scno
        bcc *+5
        jmp a01962
        lda #128
        ; ldy #8
        sta (z80_ix),y
a01962: lda #2
        cmp varobj
        beq *+5
        jmp a02023
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        sta varo
        ; ldy #9
        iny
        lda (z80_ix),y
        sta varp
        lda #2
        sta varg
a02023: lda #5
        cmp varg
        beq *+5
        jmp a02092
        lda #2
        jsr getob
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        sta dispx
        ; ldy #8
        dey
        lda (z80_ix),y
        sta dispy
        lda #2
        jsr drpob
a02092: lda #6
        cmp varg
        beq *+5
        jmp a02179
        lda #2
        ; ldy #10
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a02179
        ; ldy #9
        dey
        lda (z80_ix),y
        sta dispx
        ; ldy #8
        dey
        lda (z80_ix),y
        sta dispy
        lda #2
        jsr drpob
a02179: lda #24
        ; ldy #9
        ldy #9
        sta (z80_ix),y
        lda #3
        cmp varobj
        beq *+5
        jmp a02256
        ; ldy #8
        dey
        lda (z80_ix),y
        sta varo
        ; ldy #9
        iny
        lda (z80_ix),y
        sta varp
        lda #2
        sta varg
a02256: lda #5
        cmp varg
        beq *+5
        jmp a02325
        lda #3
        jsr getob
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        sta dispx
        ; ldy #8
        dey
        lda (z80_ix),y
        sta dispy
        lda #3
        jsr drpob
a02325: lda #6
        cmp varg
        beq *+5
        jmp a02412
        lda #3
        ; ldy #10
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a02412
        ; ldy #9
        dey
        lda (z80_ix),y
        sta dispx
        ; ldy #8
        dey
        lda (z80_ix),y
        sta dispy
        lda #3
        jsr drpob
a02412: lda #96
        ; ldy #9
        ldy #9
        sta (z80_ix),y
        lda #16
        ; ldy #8
        dey
        sta (z80_ix),y
        lda #4
        cmp varobj
        beq *+5
        jmp a02503
        ; ldy #8
        lda (z80_ix),y
        sta varo
        ; ldy #9
        iny
        lda (z80_ix),y
        sta varp
        lda #2
        sta varg
a02503: lda #5
        cmp varg
        beq *+5
        jmp a02572
        lda #4
        jsr getob
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        sta dispx
        ; ldy #8
        dey
        lda (z80_ix),y
        sta dispy
        lda #4
        jsr drpob
a02572: lda #6
        cmp varg
        beq *+5
        jmp a02659
        lda #4
        ; ldy #10
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a02659
        ; ldy #9
        dey
        lda (z80_ix),y
        sta dispx
        ; ldy #8
        dey
        lda (z80_ix),y
        sta dispy
        lda #4
        jsr drpob
a02659: lda #160
        ; ldy #8
        ldy #8
        sta (z80_ix),y
        lda #5
        cmp varobj
        beq *+5
        jmp a02742
        ; ldy #8
        lda (z80_ix),y
        sta varo
        ; ldy #9
        iny
        lda (z80_ix),y
        sta varp
        lda #2
        sta varg
        lda #1
        sta vard
a02742: lda #5
        cmp varg
        beq *+5
        jmp a02811
        lda #5
        jsr getob
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        sta dispx
        ; ldy #8
        dey
        lda (z80_ix),y
        sta dispy
        lda #5
        jsr drpob
a02811: lda #6
        cmp varg
        beq *+5
        jmp a02898
        lda #5
        ; ldy #10
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a02898
        ; ldy #9
        dey
        lda (z80_ix),y
        sta dispx
        ; ldy #8
        dey
        lda (z80_ix),y
        sta dispy
        lda #5
        jsr drpob
a02898: lda #88
        ; ldy #8
        ldy #8
        sta (z80_ix),y
        lda #48
        ; ldy #9
        iny
        sta (z80_ix),y
        lda #1
        cmp scno
        bcc *+5
        jmp a02963
        lda #24
        ; ldy #9
        sta (z80_ix),y
a02963: lda #6
        cmp varobj
        beq *+5
        jmp a03032
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        sta varo
        ; ldy #9
        iny
        lda (z80_ix),y
        sta varp
        lda #2
        sta varg
        lda #3
        sta vard
a03032: lda #5
        cmp varg
        beq *+5
        jmp a03101
        lda #6
        jsr getob
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        sta dispx
        ; ldy #8
        dey
        lda (z80_ix),y
        sta dispy
        lda #6
        jsr drpob
a03101: lda #6
        cmp varg
        beq *+5
        jmp a03187
        lda #6
        ; ldy #10
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a03187
        ; ldy #9
        dey
        lda (z80_ix),y
        sta dispx
        ; ldy #8
        dey
        lda (z80_ix),y
        sta dispy
        lda #6
        jsr drpob
a03187: lda #144
        ; ldy #9
        ldy #9
        sta (z80_ix),y
        lda #1
        cmp scno
        bcc *+5
        jmp a03236
        lda #168
        ; ldy #9
        sta (z80_ix),y
a03236: lda #7
        cmp varobj
        beq *+5
        jmp a03305
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        sta varo
        ; ldy #9
        iny
        lda (z80_ix),y
        sta varp
        lda #2
        sta varg
        lda #2
        sta vard
a03305: lda #5
        cmp varg
        beq *+5
        jmp a03374
        lda #7
        jsr getob
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        sta dispx
        ; ldy #8
        dey
        lda (z80_ix),y
        sta dispy
        lda #7
        jsr drpob
a03374: lda #6
        cmp varg
        beq *+5
        jmp a03524
        lda #7
        ; ldy #10
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a03461
        ; ldy #9
        dey
        lda (z80_ix),y
        sta dispx
        ; ldy #8
        dey
        lda (z80_ix),y
        sta dispy
        lda #7
        jsr drpob
a03461: lda vara
        ; ldy #8
        ldy #8
        sta (z80_ix),y
        lda varb
        ; ldy #9
        iny
        sta (z80_ix),y
        lda #255
        ; ldy #10
        iny
        sta (z80_ix),y
        lda #3
        sta varg
        rts
a03524: lda #2
        cmp varg
        beq *+5
        jmp a04465
        lda varobj
        ; ldy #10
        ldy #10
        sta (z80_ix),y
        lda varobj
        jsr getob
        lda varo
        ; ldy #8
        ldy #8
        sta (z80_ix),y
        lda varp
        ; ldy #9
        iny
        sta (z80_ix),y
        lda vars
        cmp varobj
        beq *+5
        jmp a03950
        lda #<80
        sta z80_l
        lda #>80
        sta z80_h
        jsr addsc
        lda #1
        sta chary
        lda #27
        sta charx
        jsr dscor
        lda #4
        sta chary
        lda #27
        sta charx
        lda #8
        sta z80_c
        lda #9
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda vard
        ; ldy #10
        iny
        sta (z80_ix),y
        lda vard
        sta z80_c
        ; ldy #6
        ldy #6
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #6
        sta (z80_ix),y
        lda #50
        ; ldy #12
        ldy #12
        sta (z80_ix),y
        lda #0
        ; ldy #11
        dey
        sta (z80_ix),y
        lda #0
        cmp scno
        bcc *+5
        jmp a03845
        lda #1
        ; ldy #11
        sta (z80_ix),y
a03845: lda #2
        cmp scno
        bcc *+5
        jmp a03880
        lda #2
        ; ldy #11
        ldy #11
        sta (z80_ix),y
a03880: lda #0
        sta varg
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda vara
        ; ldy #8
        ldy #8
        sta (z80_ix),y
        lda varb
        ; ldy #9
        iny
        sta (z80_ix),y
        rts
        jmp a04465
a03950: lda vart
        cmp varobj
        beq *+5
        jmp a04271
        lda #1
        sta varq
        lda #<80
        sta z80_l
        lda #>80
        sta z80_h
        jsr addsc
        lda #1
        sta chary
        lda #27
        sta charx
        jsr dscor
        lda #4
        sta chary
        lda #27
        sta charx
        lda #8
        sta z80_c
        lda #9
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda vard
        sta z80_c
        ; ldy #6
        ldy #6
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #6
        sta (z80_ix),y
        lda #50
        ; ldy #12
        ldy #12
        sta (z80_ix),y
        lda #0
        cmp scno
        bcc *+5
        jmp a04167
        lda #1
        ; ldy #11
        dey
        sta (z80_ix),y
a04167: lda #2
        cmp scno
        bcc *+5
        jmp a04202
        lda #2
        ; ldy #11
        ldy #11
        sta (z80_ix),y
a04202: lda #0
        sta varg
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda vara
        ; ldy #8
        ldy #8
        sta (z80_ix),y
        lda varb
        ; ldy #9
        iny
        sta (z80_ix),y
        rts
        jmp a04465
a04271: lda #<100
        sta z80_l
        lda #>100
        sta z80_h
        jsr addsc
        lda #1
        sta chary
        lda #27
        sta charx
        jsr dscor
        lda #4
        sta chary
        lda #27
        sta charx
        lda #4
        sta z80_c
        lda #14
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda vard
        ; ldy #10
        ldy #10
        sta (z80_ix),y
        lda varobj
        ; ldy #11
        iny
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda vara
        ; ldy #8
        ldy #8
        sta (z80_ix),y
        lda varb
        ; ldy #9
        iny
        sta (z80_ix),y
        rts
a04465: lda vara
        ; ldy #8
        ldy #8
        sta (z80_ix),y
        lda varb
        ; ldy #9
        iny
        sta (z80_ix),y
        lda #5
        cmp varg
        beq *+5
        jmp a04525
        lda #0
        sta varg
a04525: lda #0
        sta varq
        lda joyval
        and #32
        beq :+
        jmp a04558
:
        lda #1
        sta varq
a04558: lda joyval
        and #16
        beq :+
        jmp a04584
:
        lda #1
        sta varq
a04584: lda #1
        cmp varq
        beq *+5
        jmp a04846
        lda #0
        ; ldy #11
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp a04846
        lda #0
        cmp vark
        bcc *+5
        jmp a04846
        lda #2
        sta z80_c
        lda #8
        sta z80_b
        jsr spawn
        lda #1
        ; ldy #11
        sta (z80_ix),y
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda clock
        ; ldy #12
        iny
        sta (z80_ix),y
        lda #255
        sta z80_c
        ; ldy #12
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #12
        sta (z80_ix),y
        lda #255
        sta z80_c
        lda vark
        clc
        adc z80_c
        sta vark
        lda #7
        sta chary
        lda #29
        sta charx
        lda #24
        jsr dmsg
        lda #7
        sta chary
        lda #29
        sta charx
        lda vark
        jsr disply
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
a04846: lda joyval
        and #8
        beq :+
        jmp a05123
:
        jsr cangu
        beq :+
        jmp a04922
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #0
        ; ldy #6
        ldy #6
        sta (z80_ix),y
        jmp a05046
a04922: ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jsr cangu
        beq :+
        jmp a04962
:
        jmp a05046
a04962: ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jsr cangu
        beq :+
        jmp a05024
:
        jmp a05046
a05024: ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a05046: ; ldy #8
        ldy #8
        lda (z80_ix),y
        cmp vara
        bcc *+5
        jmp a05123
        lda #0
        cmp varc
        beq *+5
        jmp a05098
        lda #0
        jsr animsp
a05098: lda #2
        cmp varc
        beq *+5
        jmp a05123
        lda #0
        jsr animsp
a05123: lda joyval
        and #4
        beq :+
        jmp a05400
:
        jsr cangd
        beq :+
        jmp a05199
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #1
        ; ldy #6
        ldy #6
        sta (z80_ix),y
        jmp a05323
a05199: ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp a05239
:
        jmp a05323
a05239: ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp a05301
:
        jmp a05323
a05301: ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a05323: lda vara
        ; ldy #8
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a05400
        lda #0
        cmp varc
        beq *+5
        jmp a05375
        lda #0
        jsr animsp
a05375: lda #2
        cmp varc
        beq *+5
        jmp a05400
        lda #0
        jsr animsp
a05400: lda joyval
        and #1
        beq :+
        jmp a05704
:
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        cmp #191
        bcc *+5
        jmp a05704
        jsr cangr
        beq :+
        jmp a05502
:
        lda #2
        ; ldy #6
        ldy #6
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp a05627
a05502: ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jsr cangr
        beq :+
        jmp a05543
:
        jmp a05627
a05543: ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jsr cangr
        beq :+
        jmp a05605
:
        jmp a05627
a05605: ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a05627: lda varb
        ; ldy #9
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp a05704
        lda #0
        cmp varc
        beq *+5
        jmp a05679
        lda #0
        jsr animsp
a05679: lda #2
        cmp varc
        beq *+5
        jmp a05704
        lda #0
        jsr animsp
a05704: lda joyval
        and #2
        beq :+
        jmp a06007
:
        lda #1
        ; ldy #9
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp a06007
        jsr cangl
        beq :+
        jmp a05806
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #3
        ; ldy #6
        ldy #6
        sta (z80_ix),y
        jmp a05930
a05806: ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jsr cangl
        beq :+
        jmp a05846
:
        jmp a05930
a05846: ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jsr cangl
        beq :+
        jmp a05908
:
        jmp a05930
a05908: ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a05930: ; ldy #9
        ldy #9
        lda (z80_ix),y
        cmp varb
        bcc *+5
        jmp a06007
        lda #0
        cmp varc
        beq *+5
        jmp a05982
        lda #0
        jsr animsp
a05982: lda #2
        cmp varc
        beq *+5
        jmp a06007
        lda #0
        jsr animsp
a06007: lda #250
        ; ldy #8
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a06048
        lda #0
        ; ldy #8
        sta (z80_ix),y
a06048: lda #176
        ; ldy #8
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a06089
        lda #176
        ; ldy #8
        sta (z80_ix),y
a06089: lda #1
        cmp varg
        beq *+5
        jmp a06168
        ; ldy #6
        ldy #6
        lda (z80_ix),y
        cmp #4
        bcc *+5
        jmp a06168
        lda #4
        sta z80_c
        ; ldy #6
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #6
        sta (z80_ix),y
a06168: rts
.out .sprintf("EVENT 0 SIZE = %d", (* - evnt00))

evnt01:
        lda #0
        cmp scno
        beq *+5
        jmp b00027
        lda #10
        sta varq
b00027: lda #1
        cmp scno
        beq *+5
        jmp b00051
        lda #9
        sta varq
b00051: lda #2
        cmp scno
        beq *+5
        jmp b00076
        lda #8
        sta varq
b00076: lda #3
        cmp scno
        beq *+5
        jmp b00101
        lda #7
        sta varq
b00101: ; ldy #11
        ldy #11
        lda (z80_ix),y
        cmp #2
        bcc *+5
        jmp b00242
        lda #0
        cmp clock
        beq *+5
        jmp b00242
        ; ldy #12
        iny
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #12
        sta (z80_ix),y
        lda varq
        ; ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp b00242
        lda #0
        ; ldy #12
        sta (z80_ix),y
        ; ldy #11
        dey
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
b00242: lda #2
        cmp varg
        beq *+5
        jmp b00262
        rts
b00262: ; ldy #11
        ldy #11
        lda (z80_ix),y
        cmp #2
        bcc *+5
        jmp b00344
        lda #0
        cmp varc
        beq *+5
        jmp b00314
        lda #0
        jsr animsp
b00314: lda #2
        cmp varc
        beq *+5
        jmp b00339
        lda #0
        jsr animsp
b00339: jmp b00353
b00344: lda #0
        jsr animsp
b00353: ; ldy #10
        ldy #10
        lda (z80_ix),y
        sta vard
        lda #0
        sta varo
        lda #0
        ; ldy #8
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp b00690
        jsr cangu
        beq :+
        jmp b00427
:
        lda #1
        sta varo
b00427: lda #0
        cmp varo
        beq *+5
        jmp b00558
        lda #0
        ; ldy #9
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp b00558
        lda #255
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jsr cangu
        beq :+
        jmp b00528
:
        lda #2
        sta varo
b00528: ; ldy #9
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
b00558: lda #0
        cmp varo
        beq *+5
        jmp b00690
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        cmp #192
        bcc *+5
        jmp b00690
        ; ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
        jsr cangu
        beq :+
        jmp b00650
:
        lda #3
        sta varo
b00650: lda #255
        sta z80_c
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
b00690: lda #0
        ; ldy #8
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp b00724
        lda #0
        sta varo
b00724: lda #0
        cmp varo
        bcc *+5
        jmp b00897
        lda #0
        cmp vard
        beq *+5
        jmp b00897
        lda #248
        sta z80_c
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00827
:
        lda #0
        sta varo
b00827: lda #8
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00857
:
        lda #0
        sta varo
b00857: lda #8
        sta z80_c
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
b00897: lda #0
        sta varp
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        cmp #176
        bcc *+5
        jmp b01216
        jsr cangd
        beq :+
        jmp b00953
:
        lda #1
        sta varp
b00953: lda #0
        cmp varp
        beq *+5
        jmp b01084
        lda #0
        ; ldy #9
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp b01084
        lda #255
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp b01054
:
        lda #2
        sta varp
b01054: ; ldy #9
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
b01084: lda #0
        cmp varp
        beq *+5
        jmp b01216
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        cmp #192
        bcc *+5
        jmp b01216
        ; ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp b01176
:
        lda #3
        sta varp
b01176: lda #255
        sta z80_c
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
b01216: lda #175
        ; ldy #8
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp b01265
        lda #176
        ; ldy #8
        sta (z80_ix),y
        lda #0
        sta varp
b01265: lda #0
        cmp varp
        bcc *+5
        jmp b01437
        lda #1
        cmp vard
        beq *+5
        jmp b01437
        lda #8
        sta z80_c
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b01368
:
        lda #0
        sta varp
b01368: lda #8
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b01397
:
        lda #0
        sta varp
b01397: lda #248
        sta z80_c
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
b01437: lda #0
        sta vars
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        cmp #192
        bcc *+5
        jmp b01757
        jsr cangr
        beq :+
        jmp b01494
:
        lda #1
        sta vars
b01494: lda #0
        cmp vars
        beq *+5
        jmp b01625
        lda #0
        ; ldy #8
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp b01625
        lda #255
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jsr cangr
        beq :+
        jmp b01594
:
        lda #2
        sta vars
b01594: ; ldy #8
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
b01625: lda #0
        cmp vars
        beq *+5
        jmp b01757
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        cmp #176
        bcc *+5
        jmp b01757
        ; ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
        jsr cangr
        beq :+
        jmp b01717
:
        lda #3
        sta vars
b01717: lda #255
        sta z80_c
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
b01757: lda #0
        cmp vars
        bcc *+5
        jmp b01929
        lda #2
        cmp vard
        beq *+5
        jmp b01929
        lda #8
        sta z80_c
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b01859
:
        lda #0
        sta vars
b01859: lda #8
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b01889
:
        lda #0
        sta vars
b01889: lda #248
        sta z80_c
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
b01929: lda #0
        sta vart
        lda #0
        ; ldy #9
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp b02248
        jsr cangl
        beq :+
        jmp b01985
:
        lda #1
        sta vart
b01985: lda #0
        cmp vart
        beq *+5
        jmp b02116
        lda #0
        ; ldy #8
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp b02116
        lda #255
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jsr cangl
        beq :+
        jmp b02086
:
        lda #2
        sta vart
b02086: ; ldy #8
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
b02116: lda #0
        cmp vart
        beq *+5
        jmp b02248
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        cmp #176
        bcc *+5
        jmp b02248
        ; ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
        jsr cangl
        beq :+
        jmp b02208
:
        lda #3
        sta vart
b02208: lda #255
        sta z80_c
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
b02248: lda #0
        cmp vart
        bcc *+5
        jmp b02420
        lda #3
        cmp vard
        beq *+5
        jmp b02420
        lda #248
        sta z80_c
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b02351
:
        lda #0
        sta vart
b02351: lda #8
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b02381
:
        lda #0
        sta vart
b02381: lda #8
        sta z80_c
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
b02420: lda #0
        sta varq
        lda #2
        ; ldy #11
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b02521
        lda #0
        cmp varc
        beq *+5
        jmp b02480
        lda #1
        sta varq
b02480: lda #3
        cmp scno
        beq *+5
        jmp b02521
        lda #2
        cmp varc
        beq *+5
        jmp b02521
        lda #1
        sta varq
b02521: lda #1
        ; ldy #11
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b02590
        lda #1
        cmp scno
        bcc *+5
        jmp b02590
        lda #0
        cmp varc
        beq *+5
        jmp b02590
        lda #1
        sta varq
b02590: lda vard
        cmp #2
        bcc *+5
        jmp b02965
        lda varb
        ; ldy #9
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp b02748
        lda #0
        cmp vart
        bcc *+5
        jmp b02663
        lda #3
        sta vard
        jmp b02743
b02663: lda #0
        cmp vard
        beq *+5
        jmp b02714
        lda #0
        cmp varo
        bcc *+5
        jmp b02701
        jmp b02709
b02701: lda #1
        sta vard
b02709: jmp b02743
b02714: lda #0
        cmp varp
        bcc *+5
        jmp b02735
        jmp b02743
b02735: lda #0
        sta vard
b02743: jmp b02960
b02748: lda varb
        ; ldy #9
        ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp b02850
        lda #0
        cmp vard
        beq *+5
        jmp b02821
        lda #0
        cmp varo
        beq *+5
        jmp b02816
        lda #1
        sta vard
b02816: jmp b02846
b02821: lda #0
        cmp varp
        beq *+5
        jmp b02846
        lda #0
        sta vard
b02846: jmp b02960
b02850: lda #0
        cmp vars
        bcc *+5
        jmp b02880
        lda #2
        sta vard
        jmp b02960
b02880: lda #0
        cmp vard
        beq *+5
        jmp b02931
        lda #0
        cmp varo
        bcc *+5
        jmp b02918
        jmp b02926
b02918: lda #1
        sta vard
b02926: jmp b02960
b02931: lda #0
        cmp varp
        bcc *+5
        jmp b02952
        jmp b02960
b02952: lda #0
        sta vard
b02960: jmp b03329
b02965: ; ldy #8
        ldy #8
        lda (z80_ix),y
        cmp vara
        bcc *+5
        jmp b03107
        lda #0
        cmp varp
        bcc *+5
        jmp b03021
        lda #1
        sta vard
        jmp b03102
b03021: lda #3
        cmp vard
        beq *+5
        jmp b03072
        lda #0
        cmp vart
        bcc *+5
        jmp b03060
        jmp b03068
b03060: lda #2
        sta vard
b03068: jmp b03102
b03072: lda #0
        cmp vars
        bcc *+5
        jmp b03094
        jmp b03102
b03094: lda #3
        sta vard
b03102: jmp b03329
b03107: lda vara
        ; ldy #8
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp b03219
        lda #2
        cmp vard
        beq *+5
        jmp b03184
        lda #0
        cmp vars
        bcc *+5
        jmp b03172
        jmp b03180
b03172: lda #3
        sta vard
b03180: jmp b03214
b03184: lda #0
        cmp vart
        bcc *+5
        jmp b03206
        jmp b03214
b03206: lda #2
        sta vard
b03214: jmp b03329
b03219: lda #0
        cmp varo
        bcc *+5
        jmp b03248
        lda #0
        sta vard
        jmp b03329
b03248: lda #2
        cmp vard
        beq *+5
        jmp b03299
        lda #0
        cmp vars
        bcc *+5
        jmp b03286
        jmp b03294
b03286: lda #3
        sta vard
b03294: jmp b03329
b03299: lda #0
        cmp vart
        bcc *+5
        jmp b03321
        jmp b03329
b03321: lda #2
        sta vard
b03329: lda vard
        ; ldy #6
        ldy #6
        sta (z80_ix),y
        lda #9
        sta z80_c
        ; ldy #6
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #6
        sta (z80_ix),y
        lda #0
        ; ldy #11
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b03523
        lda #0
        cmp varc
        beq *+5
        jmp b03458
        lda #0
        sta varo
        lda #0
        sta varp
        lda #0
        sta vars
        lda #0
        sta vart
b03458: lda #0
        cmp scno
        beq *+5
        jmp b03523
        lda #2
        cmp varc
        beq *+5
        jmp b03523
        lda #0
        sta varo
        lda #0
        sta varp
        lda #0
        sta vars
        lda #0
        sta vart
b03523: lda #0
        cmp vard
        beq *+5
        jmp b03879
        lda #0
        cmp varo
        bcc *+5
        jmp b03879
        lda #2
        cmp varo
        beq *+5
        jmp b03614
        lda #255
        sta z80_c
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
b03614: lda #3
        cmp varo
        beq *+5
        jmp b03661
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
b03661: lda #0
        ; ldy #8
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp b03723
        lda #255
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
b03723: lda #0
        ; ldy #8
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp b03786
        lda #255
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
b03786: lda #1
        cmp varq
        beq *+5
        jmp b03879
        lda #0
        ; ldy #8
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp b03879
        jsr cangu
        beq :+
        jmp b03879
:
        lda #255
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
b03879: lda #1
        cmp vard
        beq *+5
        jmp b04122
        lda #0
        cmp varp
        bcc *+5
        jmp b04122
        lda #2
        cmp varp
        beq *+5
        jmp b03969
        lda #255
        sta z80_c
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
b03969: lda #3
        cmp varp
        beq *+5
        jmp b04016
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
b04016: ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #1
        cmp varq
        beq *+5
        jmp b04122
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        cmp #176
        bcc *+5
        jmp b04122
        jsr cangd
        beq :+
        jmp b04122
:
        ; ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
b04122: lda #2
        cmp vard
        beq *+5
        jmp b04365
        lda #0
        cmp vars
        bcc *+5
        jmp b04365
        lda #2
        cmp vars
        beq *+5
        jmp b04212
        lda #255
        sta z80_c
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
b04212: lda #3
        cmp vars
        beq *+5
        jmp b04259
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
b04259: ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #1
        cmp varq
        beq *+5
        jmp b04365
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        cmp #192
        bcc *+5
        jmp b04365
        jsr cangr
        beq :+
        jmp b04365
:
        ; ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
b04365: lda #3
        cmp vard
        beq *+5
        jmp b04617
        lda #0
        cmp vart
        bcc *+5
        jmp b04617
        lda #2
        cmp vart
        beq *+5
        jmp b04455
        lda #255
        sta z80_c
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
b04455: lda #3
        cmp vart
        beq *+5
        jmp b04503
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
b04503: ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #1
        cmp varq
        beq *+5
        jmp b04617
        lda #0
        ; ldy #9
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp b04617
        jsr cangl
        beq :+
        jmp b04617
:
        lda #255
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
b04617: ; ldy #8
        ldy #8
        lda (z80_ix),y
        sta varo
        lda varo
        lsr a
        sta varo
        lda varo
        asl a
        sta varo
        lda varo
        ; ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp b04771
        lda #0
        cmp vard
        beq *+5
        jmp b04740
        lda #255
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp b04771
b04740: ; ldy #8
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
b04771: ; ldy #9
        ldy #9
        lda (z80_ix),y
        sta varp
        lda varp
        lsr a
        sta varp
        lda varp
        asl a
        sta varp
        lda varp
        ; ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp b04924
        lda #2
        cmp vard
        beq *+5
        jmp b04884
        ; ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
        jmp b04924
b04884: lda #255
        sta z80_c
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
b04924: lda vard
        ; ldy #10
        ldy #10
        sta (z80_ix),y
        lda #2
        ; ldy #11
        iny
        cmp (z80_ix),y
        beq *+5
        jmp b04968
b04968: lda #1
        ; ldy #11
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b04995
b04995: rts
.out .sprintf("EVENT 1 SIZE = %d", (* - evnt01))

evnt02:
        lda #0
        cmp clock
        beq *+5
        jmp c00199
        ; ldy #10
        ldy #10
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #10
        sta (z80_ix),y
        lda #250
        ; ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c00199
        lda vara
        ; ldy #8
        ldy #8
        sta (z80_ix),y
        lda varb
        ; ldy #9
        iny
        sta (z80_ix),y
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00199
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #0
        ; ldy #11
        ldy #11
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
c00199: lda #3
        cmp varg
        beq *+5
        jmp c00341
        lda vara
        ; ldy #8
        ldy #8
        sta (z80_ix),y
        lda varb
        ; ldy #9
        iny
        sta (z80_ix),y
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00341
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #0
        ; ldy #11
        ldy #11
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
c00341: lda #0
        cmp varc
        beq *+5
        jmp c00367
        lda #0
        jsr animsp
c00367: lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00960
:
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        sta varo
        ; ldy #9
        iny
        lda (z80_ix),y
        sta varp
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda varo
        sta varq
        ; ldy #8
        dey
        lda (z80_ix),y
        cmp varo
        bcc *+5
        jmp c00530
        ; ldy #8
        lda (z80_ix),y
        sta z80_c
        lda varq
        sec
        sbc z80_c
        sta varq
        jmp c00573
c00530: ; ldy #8
        ldy #8
        lda (z80_ix),y
        sta varq
        lda varo
        sta z80_c
        lda varq
        sec
        sbc z80_c
        sta varq
c00573: lda varq
        cmp #5
        bcc *+5
        jmp c00960
        lda varp
        sta varq
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        cmp varp
        bcc *+5
        jmp c00660
        ; ldy #9
        lda (z80_ix),y
        sta z80_c
        lda varq
        sec
        sbc z80_c
        sta varq
        jmp c00703
c00660: ; ldy #9
        ldy #9
        lda (z80_ix),y
        sta varq
        lda varp
        sta z80_c
        lda varq
        sec
        sbc z80_c
        sta varq
c00703: lda varq
        cmp #5
        bcc *+5
        jmp c00960
        lda #<50
        sta z80_l
        lda #>50
        sta z80_h
        jsr addsc
        lda #1
        sta chary
        lda #27
        sta charx
        jsr dscor
        lda #4
        sta chary
        lda #27
        sta charx
        lda #100
        ; ldy #12
        ldy #12
        sta (z80_ix),y
        lda #8
        ; ldy #5
        ldy #5
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda vara
        ; ldy #8
        ldy #8
        sta (z80_ix),y
        lda varb
        ; ldy #9
        iny
        sta (z80_ix),y
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00960
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #0
        ; ldy #11
        ldy #11
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
c00960: rts
.out .sprintf("EVENT 2 SIZE = %d", (* - evnt02))

evnt03:
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        cmp #200
        bcc *+5
        jmp d00672
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00672
:
        lda #<150
        sta z80_l
        lda #>150
        sta z80_h
        jsr addsc
        lda #1
        sta chary
        lda #27
        sta charx
        jsr dscor
        lda #4
        sta chary
        lda #27
        sta charx
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #0
        sta varg
        lda vari
        clc
        adc #1
        sta vari
        lda vari
        sta varo
        lda #25
        cmp vari
        beq *+5
        jmp d00194
        lda numlif
        clc
        adc #1
        sta numlif
d00194: lda #50
        cmp vari
        beq *+5
        jmp d00227
        lda numlif
        clc
        adc #1
        sta numlif
d00227: lda vari
        cmp varo
        bcc *+5
        jmp d00361
        lda #2
        sta chary
        lda #26
        sta charx
        lda #24
        jsr dmsg
        lda #2
        sta chary
        lda #29
        sta charx
        lda #24
        jsr dmsg
        lda #2
        sta chary
        lda #27
        sta charx
        lda numlif
        sta loopa
d00321: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #87
        jsr pattr
        dec loopa
        beq :+
        jmp d00321
:
d00361: lda varj
        clc
        adc #1
        sta varj
        lda #13
        sta chary
        lda #29
        sta charx
        lda #24
        jsr dmsg
        lda #13
        sta chary
        lda #29
        sta charx
        lda vari
        jsr disply
        lda scno
        cmp #2
        bcc *+5
        jmp d00494
        lda #6
        cmp varj
        beq *+5
        jmp d00490
        lda #0
        sta varj
        lda #0
        sta scno
        jsr nwscr
        lda #1
        sta restfl
d00490: jmp d00617
d00494: lda #7
        cmp varj
        beq *+5
        jmp d00617
        lda #7
        cmp varf
        beq *+5
        jmp d00588
        lda #0
        sta varf
        lda #99
        sta varj
        lda #4
        sta scno
        jsr nwscr
        lda varr
        clc
        adc #1
        sta varr
        lda #1
        sta restfl
        rts
        jmp d00617
d00588: lda #0
        sta varj
        lda #5
        sta scno
        jsr nwscr
        lda #1
        sta restfl
d00617: lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #200
        ; ldy #8
        ldy #8
        sta (z80_ix),y
        lda #30
        ; ldy #12
        ldy #12
        sta (z80_ix),y
d00672: lda #0
        ; ldy #12
        ldy #12
        cmp (z80_ix),y
        bcc *+5
        jmp d01616
        ; ldy #12
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #12
        sta (z80_ix),y
        lda varr
        sta varo
        lda #0
        sta varr
        lda #29
        ; ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp d00782
        lda #196
        sta varr
        lda #3
        sta varq
d00782: lda #26
        ; ldy #12
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp d00825
        lda #38
        sta varr
        lda #3
        sta varq
d00825: lda #23
        ; ldy #12
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp d01131
        lda #84
        sta varr
        lda #2
        sta varq
        lda #16
        sta chary
        lda #29
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #57
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #57
        jsr pattr
        lda #17
        sta chary
        lda #29
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #48
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #88
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #57
        jsr pattr
        lda #18
        sta chary
        lda #29
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #57
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #89
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #57
        jsr pattr
d01131: lda #20
        ; ldy #12
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp d01175
        lda #1
        sta varq
        lda #211
        sta varr
d01175: lda #14
        ; ldy #12
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp d01481
        lda #16
        sta chary
        lda #29
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #48
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #49
        jsr pattr
        lda #17
        sta chary
        lda #29
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #53
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #54
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #55
        jsr pattr
        lda #18
        sta chary
        lda #29
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #58
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #59
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #60
        jsr pattr
        lda #2
        sta varq
        lda #84
        sta varr
d01481: lda #5
        ; ldy #12
        ldy #12
        cmp (z80_ix),y
        bcc *+5
        jmp d01547
        ; ldy #12
        lda (z80_ix),y
        cmp #12
        bcc *+5
        jmp d01547
        lda #1
        sta varq
        lda #211
        sta varr
d01547: lda #0
        cmp varr
        bcc *+5
        jmp d01564
d01564: lda varo
        sta varr
        lda #0
        ; ldy #12
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp d01616
        lda #255
        ldy #5
        sta (z80_ix),y
        rts
d01616: lda #0
        ; ldy #8
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp d01719
        lda #8
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #8
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp d01794
d01719: lda #8
        sta z80_c
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #8
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
d01794: lda #0
        cmp varc
        beq *+5
        jmp d01819
        lda #0
        jsr animsp
d01819: lda #0
        ; ldy #12
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp d01907
        lda #0
        cmp varg
        beq *+5
        jmp d01876
        lda #255
        ldy #5
        sta (z80_ix),y
d01876: lda #3
        cmp varg
        beq *+5
        jmp d01907
        lda #255
        ldy #5
        sta (z80_ix),y
d01907: rts
.out .sprintf("EVENT 3 SIZE = %d", (* - evnt03))

evnt04:
        lda #0
        cmp varc
        beq *+5
        jmp e00027
        lda #0
        jsr animsp
e00027: lda #2
        cmp varg
        beq *+5
        jmp e00044
e00044: lda #1
        ; ldy #10
        ldy #10
        cmp (z80_ix),y
        bcc *+5
        jmp e00088
        lda #15
        ; ldy #6
        ldy #6
        sta (z80_ix),y
e00088: ; ldy #12
        ldy #12
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #12
        sta (z80_ix),y
        lda #151
        sta varo
        ; ldy #12
        lda (z80_ix),y
        sta z80_c
        lda varo
        sec
        sbc z80_c
        sta varo
        lda #0
        cmp varc
        beq *+5
        jmp e00205
        lda #120
        cmp varo
        bcc *+5
        jmp e00205
        lda varo
        asl a
        sta sndtyp
e00205: lda #150
        ; ldy #12
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp e00268
        lda vara
        ; ldy #8
        ldy #8
        sta (z80_ix),y
        lda varb
        ; ldy #9
        iny
        sta (z80_ix),y
e00268: lda #0
        cmp varc
        beq *+5
        jmp e00531
        lda #0
        ; ldy #10
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e00351
        lda #255
        sta z80_c
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
e00351: lda #1
        ; ldy #10
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e00408
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
e00408: lda #2
        ; ldy #10
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e00465
        ; ldy #9
        dey
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
e00465: lda #3
        ; ldy #10
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e00531
        lda #255
        sta z80_c
        ; ldy #9
        dey
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
e00531: lda vara
        ; ldy #8
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp e00813
        lda varb
        ; ldy #9
        iny
        cmp (z80_ix),y
        beq *+5
        jmp e00813
        lda #1
        sta varg
        lda #0
        ; ldy #8
        dey
        sta (z80_ix),y
        lda #0
        ; ldy #9
        iny
        sta (z80_ix),y
        lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #1
        cmp varrnd
        beq *+5
        jmp e00691
        lda #176
        ; ldy #8
        dey
        sta (z80_ix),y
e00691: lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #1
        cmp varrnd
        beq *+5
        jmp e00757
        lda #192
        ; ldy #9
        ldy #9
        sta (z80_ix),y
e00757: lda #3
        sta z80_c
        lda #13
        sta z80_b
        jsr spawn
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
        rts
e00813: rts
.out .sprintf("EVENT 4 SIZE = %d", (* - evnt04))

evnt05:
        lda #5
        cmp scno
        beq *+5
        jmp f00687
        lda varo
        cmp #255
        bcc *+5
        jmp f00217
        lda #2
        sta chary
        lda #8
        sta charx
        lda #7
        sta z80_c
        lda varf
        clc
        adc z80_c
        sta varf
        lda varf
        jsr dmsg
        lda #2
        sta chary
        lda #12
        sta charx
        lda #4
        jsr dmsg
        lda #255
        sta z80_c
        lda varf
        clc
        adc z80_c
        sta varf
        lda #21
        sta chary
        lda #8
        sta charx
        lda varf
        jsr dmsg
        lda #12
        sta charx
        lda #21
        sta chary
        lda #4
        jsr dmsg
        lda #6
        sta z80_c
        lda varf
        sec
        sbc z80_c
        sta varf
        lda #255
        sta varo
f00217: lda #0
        cmp varc
        beq *+5
        jmp f00283
        lda #255
        sta z80_c
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #0
        jsr animsp
f00283: lda #2
        cmp varc
        beq *+5
        jmp f00348
        lda #255
        sta z80_c
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #0
        jsr animsp
f00348: lda joyval
        and #16
        beq :+
        jmp f00383
:
        lda #32
        ; ldy #8
        ldy #8
        sta (z80_ix),y
f00383: lda joyval
        and #32
        beq :+
        jmp f00419
:
        lda #32
        ; ldy #8
        ldy #8
        sta (z80_ix),y
f00419: lda #32
        ; ldy #8
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp f00543
        lda #0
        cmp varl
        beq *+5
        jmp f00482
        lda varl
        clc
        adc #1
        sta varl
        jmp f00490
f00482: lda #0
        sta varl
f00490: lda varf
        clc
        adc #1
        sta varf
        lda varf
        sta scno
        jsr nwscr
        lda scno
        lsr a
        sta scno
        jsr nwscr
        lda #1
        sta restfl
f00543: lda #9
        ; ldy #6
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f00661
        lda #3
        cmp varf
        bcc *+5
        jmp f00613
        lda #96
        ; ldy #9
        ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp f00613
f00613: lda #6
        cmp varf
        bcc *+5
        jmp f00661
        lda #96
        ; ldy #9
        ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp f00661
        jmp f00661
f00661: lda #0
        ; ldy #6
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f00687
f00687: lda #6
        cmp scno
        beq *+5
        jmp f01789
        lda joyval
        and #16
        beq :+
        jmp f00743
:
        lda #4
        sta scno
        jsr nwscr
        lda #1
        sta restfl
f00743: lda joyval
        and #32
        beq :+
        jmp f00781
:
        lda #4
        sta scno
        jsr nwscr
        lda #1
        sta restfl
f00781: lda #0
        ; ldy #10
        ldy #10
        cmp (z80_ix),y
        bcc *+5
        jmp f00835
        ; ldy #10
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #10
        sta (z80_ix),y
f00835: lda joyval
        and #64
        beq :+
        jmp f00965
:
        lda #0
        ; ldy #10
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00965
        lda #99
        ; ldy #12
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp f00965
        lda varn
        clc
        adc #1
        sta varn
        lda #2
        cmp varn
        bcc *+5
        jmp f00947
        lda #0
        sta varn
f00947: lda #5
        ; ldy #10
        ldy #10
        sta (z80_ix),y
f00965: lda #15
        sta chary
        lda #5
        sta charx
        lda #30
        jsr dmsg
        lda #16
        sta chary
        lda #6
        sta charx
        lda #31
        jsr dmsg
        lda #15
        sta chary
        lda #13
        sta charx
        lda #27
        sta z80_c
        lda varn
        clc
        adc z80_c
        sta varn
        lda varn
        jsr dmsg
        lda #27
        sta z80_c
        lda varn
        sec
        sbc z80_c
        sta varn
        lda #12
        sta chary
        lda #4
        sta charx
        lda #180
        cmp clock
        bcc *+5
        jmp f01177
        lda #18
        sta loopa
f01132: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #23
        jsr pattr
        dec loopa
        beq :+
        jmp f01132
:
        jmp f01185
f01177: lda #21
        jsr dmsg
f01185: lda varo
        cmp #255
        bcc *+5
        jmp f01484
        lda #255
        sta varo
        lda #2
        sta chary
        lda #10
        sta charx
        lda #15
        jsr dmsg
        lda #3
        sta chary
        lda #7
        sta charx
        lda #16
        jsr dmsg
        lda #8
        sta chary
        lda #2
        sta charx
        lda #17
        jsr dmsg
        lda #9
        sta chary
        lda #4
        sta charx
        lda #18
        jsr dmsg
        lda #21
        sta chary
        lda #1
        sta charx
        lda #19
        jsr dmsg
        lda #22
        sta chary
        lda #1
        sta charx
        lda #20
        jsr dmsg
        lda #23
        sta chary
        lda #2
        sta charx
        lda #29
        sta varq
        lda #6
        sta loopa
f01393: lda charx
        sta dispx
        lda chary
        sta dispy
        lda varq
        jsr pattr
        lda varq
        clc
        adc #1
        sta varq
        dec loopa
        beq :+
        jmp f01393
:
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #23
        jsr pattr
        lda #22
        jsr dmsg
f01484: lda #16
        ; ldy #6
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f01586
        jsr cangl
        beq :+
        jmp f01568
:
        lda #255
        sta z80_c
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp f01586
f01568: lda #17
        ; ldy #6
        ldy #6
        sta (z80_ix),y
f01586: lda #17
        ; ldy #6
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f01679
        jsr cangr
        beq :+
        jmp f01661
:
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
        jmp f01679
f01661: lda #16
        ; ldy #6
        ldy #6
        sta (z80_ix),y
f01679: lda #0
        cmp varc
        beq *+5
        jmp f01704
        lda #0
        jsr animsp
f01704: lda #5
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f01789
:
        lda #16
        ; ldy #6
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f01771
        lda #17
        ; ldy #6
        sta (z80_ix),y
        jmp f01789
f01771: lda #16
        ; ldy #6
        ldy #6
        sta (z80_ix),y
f01789: rts
.out .sprintf("EVENT 5 SIZE = %d", (* - evnt05))

evnt06:
        lda #0
        ; ldy #12
        ldy #12
        cmp (z80_ix),y
        bcc *+5
        jmp g00745
        lda #255
        sta z80_c
        ; ldy #12
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #12
        sta (z80_ix),y
        lda #150
        ; ldy #12
        cmp (z80_ix),y
        bcc *+5
        jmp g00268
        lda #0
        cmp varc
        beq *+5
        jmp g00158
        ; ldy #6
        ldy #6
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #6
        sta (z80_ix),y
        ; ldy #12
        ldy #12
        lda (z80_ix),y
        asl a
        sta sndtyp
g00158: lda #2
        cmp varc
        beq *+5
        jmp g00228
        ; ldy #6
        ldy #6
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #6
        sta (z80_ix),y
        ; ldy #12
        ldy #12
        lda (z80_ix),y
        asl a
        sta sndtyp
g00228: lda #4
        ; ldy #6
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp g00268
        lda #0
        ; ldy #6
        sta (z80_ix),y
g00268: lda #190
        ; ldy #12
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp g00545
        lda #8
        sta varm
        lda #255
        sta z80_c
        lda numlif
        clc
        adc z80_c
        sta numlif
        lda #2
        sta chary
        lda #26
        sta charx
        lda #24
        jsr dmsg
        lda #2
        sta chary
        lda #29
        sta charx
        lda #24
        jsr dmsg
        lda #2
        sta chary
        lda #27
        sta charx
        lda #0
        cmp numlif
        bcc *+5
        jmp g00462
        lda numlif
        sta loopa
g00422: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #87
        jsr pattr
        dec loopa
        beq :+
        jmp g00422
:
g00462: lda #0
        cmp numlif
        beq *+5
        jmp g00545
        lda #2
        sta varg
        lda #12
        sta varm
        lda #6
        sta chary
        lda #11
        sta charx
        lda #0
        jsr dmsg
        lda #17
        sta chary
        lda #11
        sta charx
        lda #1
        jsr dmsg
g00545: lda #10
        ; ldy #12
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp g00598
        lda #0
        cmp numlif
        beq *+5
        jmp g00598
        lda #1
        sta gamwon
g00598: ; ldy #12
        ldy #12
        lda (z80_ix),y
        cmp #150
        bcc *+5
        jmp g00666
        lda #80
        ; ldy #12
        cmp (z80_ix),y
        bcc *+5
        jmp g00666
        lda #19
        ; ldy #6
        ldy #6
        sta (z80_ix),y
g00666: ; ldy #12
        ldy #12
        lda (z80_ix),y
        cmp #40
        bcc *+5
        jmp g00745
        lda #1
        ; ldy #7
        ldy #7
        sta (z80_ix),y
        lda #176
        ; ldy #8
        iny
        sta (z80_ix),y
        lda #0
        ; ldy #9
        iny
        sta (z80_ix),y
g00745: lda #0
        ; ldy #12
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp g01428
        lda #0
        ; ldy #11
        dey
        sta (z80_ix),y
        lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00824
:
        lda #1
        ; ldy #11
        sta (z80_ix),y
g00824: lda #8
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00864
:
        lda #1
        ; ldy #11
        ldy #11
        sta (z80_ix),y
g00864: lda #16
        sta z80_c
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00943
:
        lda #1
        ; ldy #11
        ldy #11
        sta (z80_ix),y
g00943: lda #8
        sta z80_c
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g01023
:
        lda #1
        ; ldy #11
        ldy #11
        sta (z80_ix),y
g01023: lda #24
        sta z80_c
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #16
        sta z80_c
        ; ldy #8
        dey
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g01141
:
        lda #1
        ; ldy #11
        ldy #11
        sta (z80_ix),y
g01141: lda #8
        sta z80_c
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g01220
:
        lda #1
        ; ldy #11
        ldy #11
        sta (z80_ix),y
g01220: lda #24
        sta z80_c
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda joyval
        and #16
        beq :+
        jmp g01296
:
        lda #0
        ; ldy #11
        ldy #11
        sta (z80_ix),y
g01296: lda joyval
        and #32
        beq :+
        jmp g01332
:
        lda #0
        ; ldy #11
        ldy #11
        sta (z80_ix),y
g01332: lda #0
        ; ldy #11
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp g01428
        lda #4
        sta varg
        lda scno
        clc
        adc #1
        sta scno
        jsr nwscr
        lda scno
        sta varm
        lda scno
        sec
        sbc #1
        sta scno
        jsr nwscr
        lda #255
        ldy #5
        sta (z80_ix),y
g01428: rts
.out .sprintf("EVENT 6 SIZE = %d", (* - evnt06))

evnt07:
        lda #0
        cmp varj
        bcc *+5
        jmp h00050
        ; ldy #11
        ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
h00050: lda #95
        cmp varj
        beq *+5
        jmp h00109
        lda #160
        ; ldy #8
        ldy #8
        sta (z80_ix),y
        lda #0
        sta chary
        lda #7
        sta charx
        lda #25
        jsr dmsg
h00109: lda #99
        cmp varj
        beq *+5
        jmp h00194
        lda #170
        ; ldy #11
        ldy #11
        sta (z80_ix),y
        lda #128
        ; ldy #9
        ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp h00186
        lda #104
        ; ldy #9
        sta (z80_ix),y
h00186: lda #11
        sta varm
h00194: lda #93
        cmp varj
        beq *+5
        jmp h00236
        lda #0
        sta chary
        lda #0
        sta charx
        lda #26
        jsr dmsg
h00236: lda #192
        ; ldy #9
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp h00280
        lda #200
        ; ldy #8
        dey
        sta (z80_ix),y
h00280: lda #95
        cmp varj
        bcc *+5
        jmp h00335
        lda #160
        ; ldy #8
        ldy #8
        sta (z80_ix),y
        lda varj
        sec
        sbc #1
        sta varj
        jmp h01730
h00335: lda #0
        ; ldy #11
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp h00377
        lda varj
        sec
        sbc #1
        sta varj
h00377: ; ldy #6
        ldy #6
        lda (z80_ix),y
        cmp #18
        bcc *+5
        jmp h00637
        lda #100
        ; ldy #8
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp h00557
        lda #94
        cmp varj
        bcc *+5
        jmp h00509
        lda #255
        sta z80_c
        ; ldy #9
        iny
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #16
        ; ldy #6
        ldy #6
        sta (z80_ix),y
        jmp h00557
h00509: lda #17
        ; ldy #6
        ldy #6
        sta (z80_ix),y
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
h00557: lda #75
        ; ldy #9
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp h00637
        lda #0
        cmp varc
        beq *+5
        jmp h00637
        lda #255
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
h00637: lda #0
        cmp varc
        beq *+5
        jmp h00688
        ; ldy #6
        ldy #6
        lda (z80_ix),y
        cmp #18
        bcc *+5
        jmp h00688
        lda #0
        jsr animsp
h00688: lda #7
        sta z80_b
        jsr sktyp
        bcs :+
        jmp h00767
:
        ; ldy #6
        ldy #6
        lda (z80_ix),y
        cmp #18
        bcc *+5
        jmp h00767
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
h00767: lda #16
        ; ldy #6
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h00855
        lda #0
        cmp varj
        bcc *+5
        jmp h00855
        lda #88
        ; ldy #9
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp h00855
        lda #17
        ; ldy #6
        ldy #6
        sta (z80_ix),y
h00855: lda #18
        ; ldy #6
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h01621
        lda #80
        cmp varj
        bcc *+5
        jmp h00946
        lda varj
        cmp #92
        bcc *+5
        jmp h00946
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
h00946: ; ldy #9
        ldy #9
        lda (z80_ix),y
        cmp #30
        bcc *+5
        jmp h01149
        lda #0
        ; ldy #12
        ldy #12
        sta (z80_ix),y
        lda #0
        cmp varc
        beq *+5
        jmp h01149
        ; ldy #7
        ldy #7
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #7
        sta (z80_ix),y
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
        lda #30
        ; ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp h01109
        lda #80
        ; ldy #12
        ldy #12
        sta (z80_ix),y
h01109: lda #3
        ; ldy #7
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp h01149
        lda #0
        ; ldy #7
        sta (z80_ix),y
h01149: lda #1
        ; ldy #12
        ldy #12
        cmp (z80_ix),y
        bcc *+5
        jmp h01290
        ; ldy #12
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #12
        sta (z80_ix),y
        lda #0
        cmp varc
        beq *+5
        jmp h01250
        ; ldy #7
        ldy #7
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #7
        sta (z80_ix),y
h01250: lda #5
        ; ldy #7
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp h01290
        lda #3
        ; ldy #7
        sta (z80_ix),y
h01290: lda #31
        ; ldy #9
        ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp h01334
        lda #0
        ; ldy #7
        ldy #7
        sta (z80_ix),y
h01334: lda #1
        ; ldy #12
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp h01621
        lda #0
        cmp varc
        beq *+5
        jmp h01439
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
        ; ldy #7
        ldy #7
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #7
        sta (z80_ix),y
h01439: lda #2
        cmp varc
        beq *+5
        jmp h01516
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
        ; ldy #7
        ldy #7
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #7
        sta (z80_ix),y
h01516: lda #2
        ; ldy #7
        ldy #7
        cmp (z80_ix),y
        bcc *+5
        jmp h01556
        lda #0
        ; ldy #7
        sta (z80_ix),y
h01556: lda #80
        ; ldy #9
        ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp h01621
        lda #0
        cmp varj
        beq *+5
        jmp h01621
        lda #0
        sta scno
        jsr nwscr
        lda #1
        sta restfl
h01621: lda joyval
        and #16
        beq :+
        jmp h01675
:
        lda #0
        sta varj
        lda #0
        sta varl
        lda #0
        sta scno
        jsr nwscr
        lda #1
        sta restfl
h01675: lda joyval
        and #32
        beq :+
        jmp h01730
:
        lda #0
        sta varj
        lda #0
        sta varl
        lda #0
        sta scno
        jsr nwscr
        lda #1
        sta restfl
h01730: lda #90
        cmp varj
        beq *+5
        jmp h01784
        lda #0
        sta varj
        lda #0
        sta varl
        lda #0
        sta scno
        jsr nwscr
        lda #1
        sta restfl
h01784: rts
.out .sprintf("EVENT 7 SIZE = %d", (* - evnt07))

evnt08:
        lda #0
        ; ldy #12
        ldy #12
        cmp (z80_ix),y
        bcc *+5
        jmp i00802
        lda #0
        jsr animsp
        ; ldy #12
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #12
        sta (z80_ix),y
        lda #1
        cmp varc
        bcc *+5
        jmp i00127
        lda #80
        ; ldy #12
        cmp (z80_ix),y
        bcc *+5
        jmp i00122
        ; ldy #12
        lda (z80_ix),y
        asl a
        sta sndtyp
i00122: jmp i00127
i00127: lda #0
        ; ldy #12
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp i00802
        lda #0
        sta varo
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp i00665
:
        lda #16
        sta z80_c
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i00259
:
        lda #1
        sta varo
i00259: lda #8
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i00288
:
        lda #1
        sta varo
i00288: lda #32
        sta z80_c
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i00358
:
        lda #1
        sta varo
i00358: lda #8
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i00388
:
        lda #1
        sta varo
i00388: lda #16
        sta z80_c
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #16
        sta z80_c
        ; ldy #9
        iny
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i00496
:
        lda #1
        sta varo
i00496: lda #8
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i00526
:
        lda #1
        sta varo
i00526: lda #32
        sta z80_c
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i00595
:
        lda #1
        sta varo
i00595: lda #8
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i00625
:
        lda #1
        sta varo
i00625: lda #16
        sta z80_c
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
i00665: lda #0
        cmp varo
        beq *+5
        jmp i00770
        lda #0
        ; ldy #11
        ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp i00745
        lda #255
        sta z80_c
        ; ldy #11
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #11
        sta (z80_ix),y
i00745: lda #1
        ; ldy #5
        ldy #5
        sta (z80_ix),y
        rts
        jmp i00802
i00770: ; ldy #12
        ldy #12
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #12
        sta (z80_ix),y
i00802: rts
.out .sprintf("EVENT 8 SIZE = %d", (* - evnt08))

evnt09:
        lda #5
        ; ldy #5
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00095
        lda #0
        ; ldy #9
        ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp j00077
        lda #99
        ; ldy #12
        ldy #12
        sta (z80_ix),y
        jmp j00095
j00077: lda #0
        ; ldy #12
        ldy #12
        sta (z80_ix),y
j00095: lda #2
        ; ldy #5
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00203
        lda #40
        asl a
        sta sndtyp
        lda #7
        sta chary
        lda #29
        sta charx
        lda #24
        jsr dmsg
        lda #7
        sta chary
        lda #29
        sta charx
        lda vark
        jsr disply
        lda #255
        ; ldy #10
        ldy #10
        sta (z80_ix),y
j00203: lda #0
        ; ldy #5
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00283
        lda #0
        ; ldy #11
        ldy #11
        sta (z80_ix),y
        lda #0
        ; ldy #12
        iny
        sta (z80_ix),y
        lda #255
        ; ldy #10
        ldy #10
        sta (z80_ix),y
j00283: lda #8
        ; ldy #5
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00485
        lda #20
        ; ldy #12
        ldy #12
        sta (z80_ix),y
        ; ldy #6
        ldy #6
        lda (z80_ix),y
        ; ldy #11
        ldy #11
        sta (z80_ix),y
        lda #8
        sta z80_c
        ; ldy #11
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #11
        sta (z80_ix),y
        lda #1
        ; ldy #10
        dey
        sta (z80_ix),y
        lda varr
        sta z80_c
        ; ldy #11
        iny
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #11
        sta (z80_ix),y
        lda #2
        ; ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp j00485
        lda #2
        ; ldy #11
        sta (z80_ix),y
j00485: rts
.out .sprintf("EVENT 9 SIZE = %d", (* - evnt09))

evnt10:
        lda joyval
        and #64
        beq :+
        jmp k00046
:
        lda joyval
        and #32
        beq :+
        jmp k00046
:
        lda #1
        sta gamwon
k00046: lda varc
        clc
        adc #1
        sta varc
        lda #3
        cmp varc
        bcc *+5
        jmp k00086
        lda #0
        sta varc
k00086: lda scno
        cmp #4
        bcc *+5
        jmp k00497
        lda #170
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        cmp clock
        beq *+5
        jmp k00349
        ; ldy #8
        ldy #8
        lda (z80_ix),y
        sta varo
        ; ldy #9
        iny
        lda (z80_ix),y
        sta varp
        lda #88
        ; ldy #8
        dey
        sta (z80_ix),y
        lda #96
        ; ldy #9
        iny
        sta (z80_ix),y
        jsr skobj
        sta varobj
        lda #8
        cmp varobj
        beq *+5
        jmp k00252
        jmp k00313
k00252: lda vark
        cmp #15
        bcc *+5
        jmp k00313
        ; ldy #9
        ldy #9
        lda (z80_ix),y
        sta dispx
        ; ldy #8
        dey
        lda (z80_ix),y
        sta dispy
        lda #8
        jsr drpob
k00313: lda varo
        ; ldy #8
        ldy #8
        sta (z80_ix),y
        lda varp
        ; ldy #9
        iny
        sta (z80_ix),y
k00349: lda #0
        cmp vark
        beq *+5
        jmp k00497
        lda #50
        cmp clock
        beq *+5
        jmp k00407
        lda #27
        sta charx
        lda #6
        sta chary
        lda #3
        jsr dmsg
k00407: lda #100
        cmp clock
        beq *+5
        jmp k00450
        lda #27
        sta charx
        lda #6
        sta chary
        lda #3
        jsr dmsg
k00450: lda #60
        cmp clock
        bcc *+5
        jmp k00497
        lda clock
        cmp #90
        bcc *+5
        jmp k00497
        lda clock
        asl a
        sta sndtyp
k00497: rts
.out .sprintf("EVENT 10 SIZE = %d", (* - evnt10))

evnt11:
        rts
.out .sprintf("EVENT 11 SIZE = %d", (* - evnt11))

evnt12:
        rts
.out .sprintf("EVENT 12 SIZE = %d", (* - evnt12))

evnt13:
        lda #0
        sta varq
        lda #0
        sta varr
        lda #0
        sta varm
        lda #3
        sta vark
        lda #6
        sta scno
        jsr nwscr
        lda #0
        sta varl
        lda #0
        sta vari
        lda #0
        sta varj
        lda #3
        sta numlif
        jsr cls
        lda #0
        sta varf
        rts
.out .sprintf("EVENT 13 SIZE = %d", (* - evnt13))

evnt14:
        lda #0
        sta varq
        lda #15
        sta vars
        lda #1
        cmp varl
        beq *+5
        jmp o00043
        lda #71
        sta vars
o00043: jsr cls
        lda scno
        cmp #6
        bcc *+5
        jmp o00450
        lda #0
        sta chary
        lda #27
        sta charx
        lda #2
        jsr dmsg
        lda #2
        sta chary
        lda #26
        sta charx
        lda #24
        jsr dmsg
        lda #2
        sta chary
        lda #29
        sta charx
        lda #24
        jsr dmsg
        lda #2
        sta chary
        lda #27
        sta charx
        lda numlif
        sta loopa
o00164: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #87
        jsr pattr
        dec loopa
        beq :+
        jmp o00164
:
        lda #3
        sta chary
        lda #27
        sta charx
        lda #14
        jsr dmsg
        lda #6
        sta chary
        lda #27
        sta charx
        lda #3
        jsr dmsg
        lda #9
        sta chary
        lda #27
        sta charx
        lda #4
        jsr dmsg
        lda #12
        sta chary
        lda #27
        sta charx
        lda #5
        jsr dmsg
        lda #1
        sta chary
        lda #27
        sta charx
        jsr dscor
        lda #4
        sta chary
        lda #27
        sta charx
        lda #29
        sta charx
        lda #7
        sta chary
        lda vark
        jsr disply
        lda #10
        sta chary
        lda #29
        sta charx
        lda varf
        clc
        adc #1
        sta varf
        lda varf
        jsr disply
        lda varf
        sec
        sbc #1
        sta varf
        lda #29
        sta charx
        lda #13
        sta chary
        lda vari
        jsr disply
o00450: lda #14
        sta varp
        lda #36
        sta varq
        lda #10
        sta loopa
o00475: lda #27
        sta charx
        lda varp
        sta chary
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda varq
        jsr pattr
        lda varq
        clc
        adc #1
        sta varq
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda varq
        jsr pattr
        lda varq
        clc
        adc #1
        sta varq
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda varq
        jsr pattr
        lda varq
        clc
        adc #1
        sta varq
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda varq
        jsr pattr
        lda varq
        clc
        adc #1
        sta varq
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda varq
        jsr pattr
        lda varq
        clc
        adc #1
        sta varq
        lda varp
        clc
        adc #1
        sta varp
        dec loopa
        beq :+
        jmp o00475
:
        lda scno
        clc
        adc #1
        sta scno
        jsr nwscr
        lda scno
        sta varm
        lda scno
        sec
        sbc #1
        sta scno
        jsr nwscr
        lda #4
        cmp scno
        beq *+5
        jmp o00861
        lda #23
        sta chary
        lda #0
        sta charx
        lda #26
        sta loopa
o00821: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #26
        jsr pattr
        dec loopa
        beq :+
        jmp o00821
:
o00861: lda #8
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        sta varo
        lda #8
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varo
        cmp varrnd
        beq *+5
        jmp o01111
        lda #8
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varo
        cmp varrnd
        beq *+5
        jmp o01097
        lda #8
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varo
        cmp varrnd
        beq *+5
        jmp o01083
        lda #8
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        jmp o01092
o01083: lda varrnd
        sta varh
o01092: jmp o01106
o01097: lda varrnd
        sta varh
o01106: jmp o01120
o01111: lda varrnd
        sta varh
o01120: lda varo
        cmp varh
        beq *+5
        jmp o01202
        lda #2
        sta z80_c
        lda varh
        clc
        adc z80_c
        sta varh
        lda #7
        cmp varh
        bcc *+5
        jmp o01202
        lda #7
        sta z80_c
        lda varh
        sec
        sbc z80_c
        sta varh
o01202: lda varo
        asl a
        asl a
        asl a
        asl a
        sta varo
        lda varo
        sta z80_c
        lda varh
        clc
        adc z80_c
        sta varh
        lda #5
        sta varg
        lda #0
        sta charx
        lda #23
        sta chary
        lda #0
        cmp scno
        beq *+5
        jmp o01418
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        lda #12
        sta loopa
o01326: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        dec loopa
        beq :+
        jmp o01326
:
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
o01418: lda #1
        cmp scno
        beq *+5
        jmp o01614
        lda #6
        sta loopa
o01443: lda charx
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
        dec loopa
        beq :+
        jmp o01443
:
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
o01614: lda #2
        cmp scno
        beq *+5
        jmp o01810
        lda #6
        sta loopa
o01639: lda charx
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
        dec loopa
        beq :+
        jmp o01639
:
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
o01810: lda #3
        cmp scno
        beq *+5
        jmp o02268
        lda #2
        sta loopa
o01835: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
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
        lda #1
        jsr pattr
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
        lda #1
        jsr pattr
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
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        dec loopa
        beq :+
        jmp o01835
:
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
        lda #0
        jsr pattr
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
        lda #0
        jsr pattr
o02268: rts
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
        .byte "GAME",141
        .byte "OVER",141
        .byte "SCORE",141
        .byte "BOMBS",13
        .byte 141
        .byte "FLOOR",141
        .byte "SAVED",141
        .byte "1ST",141
        .byte "2ND",141
        .byte "3RD",141
        .byte "4TH",141
        .byte "5TH",141
        .byte "6TH",141
        .byte "7TH",141
        .byte "8TH",141
        .byte "HIGH",141
        .byte "PLAY",141
        .byte "-TERRAPINS-",141
        .byte "HOW*MANY*KIDTERRAPINS",141
        .byte "CAN*YOU*RESCUE*?",141
        .byte "ORIGINAL*ARCADE*GAME*1981",141
        .byte "ZX*GAME*BY*HIGHRISE*2017",141
        .byte "PRESS*FIRE*TO*PLAY",141
        .byte "(bcdefghijklmn",141
        .byte "HIGH",141
        .byte "***",141
        .byte "CONGRATULATIONS",141
        .byte "A*TOUGHER*CHALLENGE*AWAITS",141
        .byte "KEYBOARD",141
        .byte "KEMPSTON",141
        .byte "SINCLAIR",141
        .byte "CONTROL:",141
        .byte "!",39
        .byte "#$%&",141
nummsg:
        .byte 32
.out .sprintf("MESSAGE SIZE = %d", (* - msgdat))

chgfx:
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,255,255,85,170,0,255,0
        .byte 106,114,106,114,106,114,106,114
        .byte 234,242,234,82,170,6,252,1
        .byte 106,117,106,117,106,112,63,128
        .byte 128,63,127,117,106,117,106,117
        .byte 1,252,254,86,170,82,170,82
        .byte 106,114,106,114,106,118,60,129
        .byte 129,60,106,114,106,114,106,114
        .byte 1,252,254,86,170,6,252,1
        .byte 128,63,127,85,106,64,63,128
        .byte 106,255,255,85,170,0,255,0
        .byte 0,255,255,85,170,0,255,118
        .byte 1,3,7,15,31,63,127,255
        .byte 0,0,126,64,126,2,126,0
        .byte 128,0,191,161,161,161,161,0
        .byte 0,0,126,64,64,64,126,0
        .byte 128,128,191,129,191,161,191,0
        .byte 64,0,95,80,80,80,80,0
        .byte 3,3,3,3,3,3,3,3
        .byte 96,96,96,96,96,96,96,96
        .byte 0,0,0,0,0,0,255,255
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 24,24,24,24,24,24,24,24
        .byte 0,72,0,32,2,128,0,36
        .byte 136,136,136,136,136,136,136,136
        .byte 96,96,96,96,96,96,224,224
        .byte 206,170,204,170,202,0,0,0
        .byte 209,145,213,149,223,0,0,0
        .byte 108,74,106,74,108,0,0,0
        .byte 94,82,82,82,82,0,0,0
        .byte 123,74,122,74,75,0,0,0
        .byte 220,18,18,82,220,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 255,135,2,2,201,201,200,201
        .byte 255,255,206,4,181,165,132,156
        .byte 255,255,115,33,173,41,33,225
        .byte 255,255,155,10,106,74,10,26
        .byte 255,252,40,40,11,73,77,76
        .byte 200,200,203,207,255,255,255,255
        .byte 4,148,181,255,255,255,255,255
        .byte 37,165,173,255,255,255,255,255
        .byte 50,50,118,255,255,255,255,255
        .byte 106,106,232,249,255,255,255,255
        .byte 255,255,255,254,254,252,248,252
        .byte 255,252,228,72,208,80,128,0
        .byte 0,0,0,1,3,6,12,16
        .byte 0,0,254,195,8,17,55,126
        .byte 255,15,3,1,21,11,0,28
        .byte 255,252,248,248,248,240,241,243
        .byte 0,0,0,0,0,0,0,0
        .byte 255,255,252,252,248,248,240,240
        .byte 255,7,7,1,0,48,120,232
        .byte 240,224,192,192,224,224,248,224
        .byte 243,243,243,240,240,248,252,255
        .byte 0,0,0,0,0,0,0,0
        .byte 224,224,224,224,224,224,240,248
        .byte 200,152,248,112,0,0,1,3
        .byte 252,224,224,192,128,0,0,128
        .byte 241,224,224,224,224,240,240,248
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,16,62
        .byte 129,0,0,0,0,0,0,0
        .byte 192,32,32,0,0,0,0,0
        .byte 252,254,255,255,255,255,255,255
        .byte 0,1,7,143,223,255,255,255
        .byte 255,255,255,209,129,193,255,255
        .byte 0,192,243,254,192,128,0,0
        .byte 32,224,128,0,3,7,13,26
        .byte 255,255,255,255,255,255,240,224
        .byte 159,15,15,7,135,131,65,33
        .byte 128,128,192,248,255,254,255,255
        .byte 1,71,4,24,224,0,0,7
        .byte 229,2,5,10,21,43,119,255
        .byte 225,223,193,193,225,223,193,193
        .byte 16,8,128,128,128,64,0,0
        .byte 234,65,0,1,34,37,107,255
        .byte 255,254,170,212,160,192,64,170
        .byte 0,0,0,0,0,0,0,0
        .byte 225,255,255,255,255,255,255,255
        .byte 255,253,234,212,168,212,168,212
        .byte 248,112,224,192,234,128,128,128
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,254,195,0,0,0,0,0
        .byte 8,28,65,28,54,42,54,73
        .byte 0,15,254,195,0,0,0,0
        .byte 0,0,8,17,55,126,254,255
bprop:
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
        .byte 6
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
.out .sprintf("BLOCKS SIZE = %d", (* - chgfx))

sprgfx:
        .byte 1,192,2,162,3,230,112,14,59,228,7,112,13,216,26,172,23,116,26,172,29,220,26,172,15,120,22,183,57,142,97,0
        .byte 0,112,128,168,128,249,156,3,14,249,1,220,3,118,6,171,5,221,6,171,7,119,6,171,3,222,197,173,142,99,24,64
        .byte 0,28,32,42,96,62,231,0,67,190,0,119,128,221,193,170,65,119,193,170,193,221,193,170,128,247,113,107,227,152,6,16
        .byte 0,7,136,10,152,15,57,192,144,239,192,29,96,55,176,106,208,93,176,106,112,119,176,106,224,61,220,90,56,230,1,132
        .byte 1,192,34,160,51,227,56,14,19,228,7,112,13,216,26,172,23,116,26,172,29,220,26,172,15,122,118,183,57,195,0,129
        .byte 0,112,8,168,204,248,142,3,4,249,1,220,3,118,6,171,5,221,6,171,7,119,6,171,131,222,221,173,206,112,64,32
        .byte 0,28,2,42,51,62,227,128,65,62,0,119,128,221,193,170,65,119,193,170,193,221,193,170,160,247,119,107,51,156,16,8
        .byte 0,7,128,138,140,207,56,224,144,79,192,29,96,55,176,106,208,93,176,106,112,119,176,106,232,61,221,218,12,231,4,2
        .byte 1,192,2,160,99,224,56,14,19,231,7,112,13,216,26,172,23,116,26,172,29,220,26,172,47,120,118,180,96,206,64,67
        .byte 0,112,0,168,24,248,142,3,196,249,1,220,3,118,6,171,5,221,6,171,7,119,6,171,11,222,29,173,152,51,208,16
        .byte 0,28,0,42,6,62,227,128,113,62,0,119,128,221,193,170,65,119,193,170,193,221,193,170,130,247,71,107,230,12,52,4
        .byte 0,7,128,10,129,143,56,224,156,79,192,29,96,55,176,106,208,93,176,106,112,119,176,106,224,189,209,218,57,131,13,1
        .byte 96,128,57,206,22,183,15,120,26,172,29,220,26,172,23,116,26,172,13,216,7,112,59,228,112,14,3,230,2,162,1,192
        .byte 24,32,142,115,197,173,3,222,6,171,7,119,6,171,5,221,6,171,3,118,1,220,14,249,156,3,128,249,128,168,0,112
        .byte 6,8,227,156,113,107,128,247,193,170,193,221,193,170,65,119,193,170,128,221,0,119,67,190,231,0,96,62,32,42,0,28
        .byte 1,130,56,231,220,90,224,61,176,106,112,119,176,106,208,93,176,106,96,55,192,29,144,239,57,192,152,15,136,10,0,7
        .byte 0,65,56,195,118,183,15,122,26,172,29,220,26,172,23,116,26,172,13,216,7,112,19,228,56,14,51,227,34,160,1,192
        .byte 64,16,206,48,221,173,131,222,6,171,7,119,6,171,5,221,6,171,3,118,1,220,4,249,142,3,204,248,8,168,0,112
        .byte 16,4,51,140,119,107,160,247,193,170,193,221,193,170,65,119,193,170,128,221,0,119,65,62,227,128,51,62,2,42,0,28
        .byte 4,1,12,227,221,218,232,61,176,106,112,119,176,106,208,93,176,106,96,55,192,29,144,79,56,224,140,207,128,138,0,7
        .byte 65,3,97,142,118,180,47,120,26,172,29,220,26,172,23,116,26,172,13,216,7,112,19,231,56,14,99,224,2,160,1,192
        .byte 208,64,152,99,29,173,11,222,6,171,7,119,6,171,5,221,6,171,3,118,1,220,196,249,142,3,24,248,0,168,0,112
        .byte 52,16,230,24,71,107,130,247,193,170,193,221,193,170,65,119,193,170,128,221,0,119,113,62,227,128,6,62,0,42,0,28
        .byte 13,4,57,134,209,218,224,189,176,106,112,119,176,106,208,93,176,106,96,55,192,29,156,79,56,224,129,143,128,10,0,7
        .byte 0,0,128,8,192,24,111,152,94,208,53,96,59,182,213,117,110,215,21,117,59,182,53,96,94,200,111,156,96,14,32,0
        .byte 0,0,32,2,48,6,27,230,23,180,13,88,142,237,117,93,219,181,69,93,142,237,13,88,23,178,27,231,152,3,8,0
        .byte 0,0,136,0,140,1,134,249,5,237,3,86,99,187,93,87,118,237,81,87,99,187,3,86,133,236,198,249,230,0,2,0
        .byte 0,0,34,0,99,0,97,190,65,123,128,213,216,238,215,85,93,187,212,85,216,238,128,213,33,123,113,190,57,128,0,128
        .byte 0,0,32,0,96,14,111,156,94,200,53,96,59,182,85,117,238,215,85,117,59,182,53,96,30,200,47,152,112,12,224,4
        .byte 0,0,8,0,152,3,27,231,23,178,13,88,142,237,85,93,251,181,85,93,142,237,13,88,7,178,11,230,28,3,56,1
        .byte 0,0,2,0,230,0,198,249,133,236,3,86,99,187,85,87,126,237,85,87,99,187,3,86,129,236,130,249,199,0,78,0
        .byte 0,0,0,128,57,128,113,190,33,123,128,213,216,238,213,85,95,187,213,85,216,238,128,213,32,123,96,190,49,192,19,128
        .byte 0,0,224,4,112,12,47,152,30,200,53,96,59,182,21,117,110,215,213,117,59,182,53,96,94,200,111,152,192,24,128,16
        .byte 0,0,56,1,28,3,11,230,7,178,13,88,142,237,69,93,219,181,117,93,142,237,13,88,23,178,27,230,48,6,32,4
        .byte 0,0,78,0,199,0,130,249,129,236,3,86,99,187,81,87,118,237,93,87,99,187,3,86,133,236,134,249,140,1,8,1
        .byte 0,0,19,128,49,192,96,190,32,123,128,213,216,238,212,85,93,187,215,85,216,238,128,213,33,123,97,190,99,0,66,0
        .byte 0,0,16,1,24,3,25,246,11,122,6,172,109,220,174,171,235,118,174,168,109,220,6,172,19,122,57,246,112,6,0,4
        .byte 0,0,68,0,198,0,134,125,130,222,1,171,27,119,235,170,186,221,43,170,27,119,1,171,132,222,142,125,156,1,0,1
        .byte 0,0,17,0,49,128,97,159,160,183,192,106,198,221,186,234,110,183,138,234,198,221,192,106,161,55,99,159,103,0,64,0
        .byte 0,0,4,64,12,96,216,103,232,45,176,26,113,183,174,186,219,173,162,186,113,183,176,26,232,77,216,231,25,192,16,0
        .byte 0,0,0,4,112,6,57,246,19,122,6,172,109,220,174,170,235,119,174,170,109,220,6,172,19,120,25,244,48,14,32,7
        .byte 0,0,0,1,156,1,142,125,132,222,1,171,27,119,171,170,250,221,171,170,27,119,1,171,4,222,6,125,140,3,200,1
        .byte 0,0,64,0,103,0,99,159,161,55,192,106,198,221,170,234,126,183,170,234,198,221,192,106,129,55,65,159,227,0,114,0
        .byte 0,0,16,0,25,192,216,231,232,77,176,26,113,183,170,186,223,173,170,186,113,183,176,26,224,77,208,103,56,192,28,128
        .byte 0,0,32,7,48,14,25,244,19,120,6,172,109,220,174,168,235,118,174,171,109,220,6,172,19,122,25,246,24,3,8,1
        .byte 0,0,200,1,140,3,6,125,4,222,1,171,27,119,43,170,186,221,235,170,27,119,1,171,132,222,134,125,198,0,66,0
        .byte 0,0,114,0,227,0,65,159,129,55,192,106,198,221,138,234,110,183,186,234,198,221,192,106,161,55,97,159,49,128,16,128
        .byte 0,0,28,128,56,192,208,103,224,77,176,26,113,183,162,186,219,173,174,186,113,183,176,26,232,77,216,103,12,96,4,32
        .byte 1,192,2,162,3,230,112,14,59,228,5,208,15,120,28,28,30,188,22,52,29,92,31,252,15,120,22,183,57,142,97,0
        .byte 0,112,128,168,128,249,156,3,14,249,1,116,3,222,7,7,7,175,5,141,7,87,7,255,3,222,197,173,142,99,24,64
        .byte 0,28,32,42,96,62,231,0,67,190,0,93,128,247,193,193,193,235,65,99,193,213,193,255,128,247,113,107,227,152,6,16
        .byte 0,7,136,10,152,15,57,192,144,239,64,23,224,61,112,112,240,122,208,88,112,117,240,127,224,61,220,90,56,230,1,132
        .byte 1,192,34,160,51,227,56,14,19,228,5,208,15,120,30,60,28,156,22,52,29,92,31,252,15,122,118,183,57,195,0,129
        .byte 0,112,8,168,204,248,142,3,4,249,1,116,3,222,7,143,7,39,5,141,7,87,7,255,131,222,221,173,206,112,64,32
        .byte 0,28,2,42,51,62,227,128,65,62,0,93,128,247,193,227,193,201,65,99,193,213,193,255,160,247,119,107,51,156,16,8
        .byte 0,7,128,138,140,207,56,224,144,79,64,23,224,61,240,120,112,114,208,88,112,117,240,127,232,61,221,218,12,231,4,2
        .byte 1,192,2,160,99,224,56,14,19,231,5,208,15,120,28,28,30,188,22,52,29,92,31,252,47,120,118,180,96,206,64,67
        .byte 0,112,0,168,24,248,142,3,196,249,1,116,3,222,7,7,7,175,5,141,7,87,7,255,11,222,29,173,152,51,208,16
        .byte 0,28,0,42,6,62,227,128,113,62,0,93,128,247,193,193,193,235,65,99,193,213,193,255,130,247,71,107,230,12,52,4
        .byte 0,7,128,10,129,143,56,224,156,79,64,23,224,61,112,112,240,122,208,88,112,117,240,127,224,189,209,218,57,131,13,1
        .byte 96,128,57,206,22,183,15,120,31,252,29,92,22,52,30,188,28,28,15,120,5,208,59,228,112,14,3,230,2,162,1,192
        .byte 24,32,142,115,197,173,3,222,7,255,7,87,5,141,7,175,7,7,3,222,1,116,14,249,156,3,128,249,128,168,0,112
        .byte 6,8,227,156,113,107,128,247,193,255,193,213,65,99,193,235,193,193,128,247,0,93,67,190,231,0,96,62,32,42,0,28
        .byte 1,130,56,231,220,90,224,61,240,127,112,117,208,88,240,122,112,112,224,61,64,23,144,239,57,192,152,15,136,10,0,7
        .byte 0,65,56,195,118,183,15,122,31,252,29,92,22,52,30,188,28,28,15,120,5,208,19,228,56,14,51,227,34,160,1,192
        .byte 64,16,206,48,221,173,131,222,7,255,7,87,5,141,7,175,7,7,3,222,1,116,4,249,142,3,204,248,8,168,0,112
        .byte 16,4,51,140,119,107,160,247,193,255,193,213,65,99,193,235,193,193,128,247,0,93,65,62,227,128,51,62,2,42,0,28
        .byte 4,1,12,227,221,218,232,61,240,127,112,117,208,88,240,122,112,112,224,61,64,23,144,79,56,224,140,207,128,138,0,7
        .byte 65,3,97,142,118,180,47,120,31,252,29,92,22,52,28,156,30,60,15,120,5,208,19,231,56,14,99,224,2,160,1,192
        .byte 208,64,152,99,29,173,11,222,7,255,7,87,5,141,7,39,7,143,3,222,1,116,196,249,142,3,24,248,0,168,0,112
        .byte 52,16,230,24,71,107,130,247,193,255,193,213,65,99,193,201,193,227,128,247,0,93,113,62,227,128,6,62,0,42,0,28
        .byte 13,4,57,134,209,218,224,189,240,127,112,117,208,88,112,114,240,120,224,61,64,23,156,79,56,224,129,143,128,10,0,7
        .byte 0,0,128,8,192,24,111,152,95,208,63,224,59,86,220,117,105,55,28,117,59,86,63,224,95,200,111,156,96,14,32,0
        .byte 0,0,32,2,48,6,27,230,23,244,15,248,142,213,119,29,218,77,71,29,142,213,15,248,23,242,27,231,152,3,8,0
        .byte 0,0,136,0,140,1,134,249,5,253,3,254,99,181,93,199,118,147,81,199,99,181,3,254,133,252,198,249,230,0,2,0
        .byte 0,0,34,0,99,0,97,190,65,127,128,255,88,237,215,113,221,164,212,113,88,237,128,255,33,127,113,190,57,128,0,128
        .byte 16,0,48,0,48,14,47,156,31,200,63,224,58,214,92,117,233,55,92,117,58,214,63,224,31,200,47,152,112,12,224,4
        .byte 4,0,12,0,140,3,11,231,7,242,15,248,142,181,87,29,250,77,87,29,142,181,15,248,7,242,11,230,28,3,56,1
        .byte 1,0,3,0,227,0,194,249,129,252,3,254,99,173,85,199,126,147,85,199,99,173,3,254,129,252,130,249,199,0,78,0
        .byte 0,64,0,192,56,192,112,190,32,127,128,255,88,235,213,113,223,164,213,113,88,235,128,255,32,127,96,190,49,192,19,128
        .byte 0,0,224,4,112,12,47,152,31,200,63,224,59,86,28,117,105,55,220,117,59,86,63,224,95,200,111,152,192,24,128,16
        .byte 0,0,56,1,28,3,11,230,7,242,15,248,142,213,71,29,218,77,119,29,142,213,15,248,23,242,27,230,48,6,32,4
        .byte 0,0,78,0,199,0,130,249,129,252,3,254,99,181,81,199,118,147,93,199,99,181,3,254,133,252,134,249,140,1,8,1
        .byte 0,0,19,128,49,192,96,190,32,127,128,255,88,237,212,113,221,164,215,113,88,237,128,255,33,127,97,190,99,0,66,0
        .byte 0,0,16,1,24,3,25,246,11,250,7,252,106,220,174,59,236,150,174,56,106,220,7,252,19,250,57,246,112,6,0,4
        .byte 0,0,68,0,198,0,134,125,130,254,1,255,26,183,235,142,187,37,43,142,26,183,1,255,132,254,142,125,156,1,0,1
        .byte 0,0,17,0,49,128,97,159,160,191,192,127,198,173,186,227,110,201,138,227,198,173,192,127,161,63,99,159,103,0,64,0
        .byte 0,0,4,64,12,96,216,103,232,47,240,31,113,171,238,184,91,178,226,184,113,171,240,31,232,79,216,231,25,192,16,0
        .byte 0,8,0,12,112,12,57,244,19,248,7,252,107,92,174,58,236,151,174,58,107,92,7,252,19,248,25,244,48,14,32,7
        .byte 0,2,0,3,28,3,14,125,4,254,1,255,26,215,171,142,251,37,171,142,26,215,1,255,4,254,6,125,140,3,200,1
        .byte 128,0,192,0,199,0,67,159,129,63,192,127,198,181,170,227,126,201,170,227,198,181,192,127,129,63,65,159,227,0,114,0
        .byte 32,0,48,0,49,192,208,231,224,79,240,31,113,173,234,184,95,178,234,184,113,173,240,31,224,79,208,103,56,192,28,128
        .byte 0,0,32,7,48,14,25,244,19,248,7,252,106,220,174,56,236,150,174,59,106,220,7,252,19,250,25,246,24,3,8,1
        .byte 0,0,200,1,140,3,6,125,4,254,1,255,26,183,43,142,187,37,235,142,26,183,1,255,132,254,134,125,198,0,66,0
        .byte 0,0,114,0,227,0,65,159,129,63,192,127,198,173,138,227,110,201,186,227,198,173,192,127,161,63,97,159,49,128,16,128
        .byte 0,0,28,128,56,192,208,103,224,79,240,31,113,171,226,184,91,178,238,184,113,171,240,31,232,79,216,103,12,96,4,32
        .byte 0,0,0,0,0,0,0,0,1,0,5,64,3,128,14,224,3,128,5,64,1,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,64,1,80,0,224,3,184,0,224,1,80,0,64,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,16,0,84,0,56,0,238,0,56,0,84,0,16,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,4,0,21,0,14,128,59,0,14,0,21,0,4,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,1,0,5,64,3,128,15,224,3,128,5,64,1,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,64,1,80,0,224,3,248,0,224,1,80,0,64,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,16,0,84,0,56,0,254,0,56,0,84,0,16,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,4,0,21,0,14,128,63,0,14,0,21,0,4,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,3,96,54,182,75,105,12,24,15,248,15,248,55,246,95,253,31,252,23,244,11,232,13,208,3,96,0,0
        .byte 0,0,0,0,0,216,141,173,82,218,3,6,3,254,3,254,141,253,87,255,7,255,5,253,2,250,3,116,0,216,0,0
        .byte 0,0,0,0,0,54,99,107,148,182,128,193,128,255,128,255,99,127,213,255,193,255,65,127,128,190,0,221,0,54,0,0
        .byte 0,0,0,0,128,13,216,218,165,45,96,48,224,63,224,63,216,223,245,127,240,127,208,95,160,47,64,55,128,13,0,0
        .byte 0,0,0,0,3,96,7,240,11,104,12,24,63,254,79,249,87,245,31,252,31,252,23,244,11,236,21,210,19,98,48,3
        .byte 0,0,0,0,0,216,1,252,2,218,3,6,143,255,83,254,85,253,7,255,7,255,5,253,2,251,133,116,132,216,204,0
        .byte 0,0,0,0,0,54,0,127,128,182,128,193,227,255,148,255,85,127,193,255,193,255,65,127,192,190,33,93,33,54,51,0
        .byte 0,0,0,0,128,13,192,31,160,45,96,48,248,255,229,63,213,95,240,127,240,127,208,95,176,47,72,87,136,77,12,192
        .byte 0,0,0,0,99,99,22,180,11,104,12,24,15,248,15,248,23,244,31,252,31,252,55,246,75,233,77,209,35,98,0,0
        .byte 0,0,0,0,216,216,5,173,2,218,3,6,3,254,3,254,5,253,7,255,7,255,141,253,82,250,83,116,136,216,0,0
        .byte 0,0,0,0,54,54,65,107,128,182,128,193,128,255,128,255,65,127,193,255,193,255,99,127,148,190,20,221,34,54,0,0
        .byte 0,0,0,0,141,141,208,90,160,45,96,48,224,63,224,63,208,95,240,127,240,127,216,223,165,47,69,55,136,141,0,0
        .byte 0,0,3,96,5,208,11,232,23,244,31,252,95,253,55,246,31,252,15,248,12,24,75,105,54,182,3,96,0,0,0,0
        .byte 0,0,0,216,1,116,2,250,5,253,7,255,87,255,141,253,7,255,3,254,3,6,82,218,141,173,0,216,0,0,0,0
        .byte 0,0,0,54,0,93,128,190,65,127,193,255,213,255,99,127,193,255,128,255,128,193,148,182,99,107,0,54,0,0,0,0
        .byte 0,0,128,13,64,23,160,47,208,95,240,127,245,127,216,223,240,127,224,63,96,48,165,45,216,218,128,13,0,0,0,0
        .byte 0,0,67,97,69,209,43,234,23,244,31,252,31,252,87,245,95,253,47,250,12,24,11,104,7,240,3,96,0,0,0,0
        .byte 0,0,80,216,81,116,138,250,5,253,7,255,7,255,85,253,87,255,139,254,3,6,2,218,1,252,0,216,0,0,0,0
        .byte 0,0,20,54,20,93,162,190,65,127,193,255,193,255,85,127,213,255,162,255,128,193,128,182,0,127,0,54,0,0,0,0
        .byte 0,0,133,13,69,23,168,175,208,95,240,127,240,127,213,95,245,127,232,191,96,48,160,45,192,31,128,13,0,0,0,0
        .byte 0,0,35,98,69,209,75,233,55,246,31,252,31,252,23,244,31,252,15,248,12,24,11,104,22,180,99,99,0,0,0,0
        .byte 0,0,136,216,81,116,82,250,141,253,7,255,7,255,5,253,7,255,3,254,3,6,2,218,5,173,216,216,0,0,0,0
        .byte 0,0,34,54,20,93,148,190,99,127,193,255,193,255,65,127,193,255,128,255,128,193,128,182,65,107,54,54,0,0,0,0
        .byte 0,0,136,141,69,23,165,47,216,223,240,127,240,127,208,95,240,127,224,63,96,48,160,45,208,90,141,141,0,0,0,0
        .byte 0,0,4,16,2,8,15,8,22,240,47,232,95,220,127,212,63,200,127,212,95,220,47,232,22,240,15,8,2,8,4,16
        .byte 0,0,1,4,0,130,3,194,5,188,11,250,23,247,31,245,15,242,31,245,23,247,11,250,5,188,3,194,0,130,1,4
        .byte 0,0,0,65,128,32,128,240,1,111,130,254,197,253,71,253,131,252,71,253,197,253,130,254,1,111,128,240,128,32,0,65
        .byte 0,0,64,16,32,8,32,60,192,91,160,191,113,127,81,255,32,255,81,255,113,127,160,191,192,91,32,60,32,8,64,16
        .byte 0,0,3,0,192,128,47,0,22,240,47,232,95,220,127,220,63,200,127,220,95,220,47,232,22,240,47,0,192,128,3,0
        .byte 0,0,0,192,48,32,11,192,5,188,11,250,23,247,31,247,15,242,31,247,23,247,11,250,5,188,11,192,48,32,0,192
        .byte 0,0,0,48,12,8,2,240,1,111,130,254,197,253,199,253,131,252,199,253,197,253,130,254,1,111,2,240,12,8,0,48
        .byte 0,0,0,12,3,2,0,188,192,91,160,191,113,127,113,255,32,255,113,255,113,127,160,191,192,91,0,188,3,2,0,12
        .byte 0,0,48,4,72,4,15,8,22,240,47,232,95,220,127,212,63,200,127,212,95,220,47,232,22,240,15,8,72,4,48,4
        .byte 0,0,12,1,18,1,3,194,5,188,11,250,23,247,31,245,15,242,31,245,23,247,11,250,5,188,3,194,18,1,12,1
        .byte 0,0,67,0,68,128,128,240,1,111,130,254,197,253,71,253,131,252,71,253,197,253,130,254,1,111,128,240,68,128,67,0
        .byte 0,0,16,192,17,32,32,60,192,91,160,191,113,127,81,255,32,255,81,255,113,127,160,191,192,91,32,60,17,32,16,192
        .byte 0,0,8,32,16,64,16,240,15,104,23,244,59,250,43,254,19,252,43,254,59,250,23,244,15,104,16,240,16,64,8,32
        .byte 0,0,2,8,4,16,4,60,3,218,5,253,142,254,138,255,4,255,138,255,142,254,5,253,3,218,4,60,4,16,2,8
        .byte 0,0,0,130,1,4,1,15,128,246,65,127,163,191,226,191,193,63,226,191,163,191,65,127,128,246,1,15,1,4,0,130
        .byte 0,0,128,32,0,65,192,67,160,61,208,95,232,239,248,175,240,79,248,175,232,239,208,95,160,61,192,67,0,65,128,32
        .byte 0,0,0,192,1,3,0,244,15,104,23,244,59,250,59,254,19,252,59,254,59,250,23,244,15,104,0,244,1,3,0,192
        .byte 0,0,0,48,192,64,0,61,3,218,5,253,142,254,142,255,4,255,142,255,142,254,5,253,3,218,0,61,192,64,0,48
        .byte 0,0,0,12,48,16,64,15,128,246,65,127,163,191,227,191,193,63,227,191,163,191,65,127,128,246,64,15,48,16,0,12
        .byte 0,0,0,3,12,4,208,3,160,61,208,95,232,239,248,239,240,79,248,239,232,239,208,95,160,61,208,3,12,4,0,3
        .byte 0,0,32,12,32,18,16,240,15,104,23,244,59,250,43,254,19,252,43,254,59,250,23,244,15,104,16,240,32,18,32,12
        .byte 0,0,8,3,136,4,4,60,3,218,5,253,142,254,138,255,4,255,138,255,142,254,5,253,3,218,4,60,136,4,8,3
        .byte 0,0,194,0,34,1,1,15,128,246,65,127,163,191,226,191,193,63,226,191,163,191,65,127,128,246,1,15,34,1,194,0
        .byte 0,0,48,128,72,128,192,67,160,61,208,95,232,239,248,175,240,79,248,175,232,239,208,95,160,61,192,67,72,128,48,128
        .byte 0,0,0,0,0,0,0,0,7,224,15,240,31,248,63,252,127,254,63,252,33,204,33,132,33,132,63,132,63,132,63,132
        .byte 0,0,0,0,0,0,0,0,1,248,3,252,7,254,15,255,159,255,15,255,8,115,8,97,8,97,15,225,15,225,15,225
        .byte 0,0,0,0,0,0,0,0,0,126,0,255,129,255,195,255,231,255,195,255,194,28,66,24,66,24,67,248,67,248,67,248
        .byte 0,0,0,0,0,0,0,0,128,31,192,63,224,127,240,255,249,255,240,255,48,135,16,134,16,134,16,254,16,254,16,254
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,160,3,192,1,64,1,224,2,128,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,40,0,240,0,80,0,120,0,160,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,10,0,60,0,20,0,30,0,40,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,128,2,0,15,0,5,128,7,0,10,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,2,128,1,224,1,64,3,192,0,160,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,160,0,120,0,80,0,240,0,40,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,40,0,30,0,20,0,60,0,10,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,10,128,7,0,5,0,15,128,2,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,1,32,1,192,3,96,1,192,2,64,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,72,0,112,0,216,0,112,0,144,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,18,0,28,0,54,0,28,0,36,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,128,4,0,7,128,13,0,7,0,9,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,2,64,1,192,3,96,1,192,1,32,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,144,0,112,0,216,0,112,0,72,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,36,0,28,0,54,0,28,0,18,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,9,0,7,128,13,0,7,128,4,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,19,192,5,160,1,128,8,16
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,240,1,104,0,96,2,4
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,60,0,90,0,24,0,129
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,79,128,22,0,6,64,32
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,192,21,160,1,128,4,32
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,240,5,104,0,96,1,8
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,60,1,90,0,24,0,66
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,15,128,86,0,6,128,16
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,200,5,160,1,128,8,16
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,242,1,104,0,96,2,4
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,60,0,90,0,24,0,129
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,15,128,22,0,6,64,32
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,192,5,168,1,128,4,32
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,240,1,106,0,96,1,8
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,60,128,90,0,24,0,66
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,15,160,22,0,6,128,16
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,192,26,160,55,112,106,168,221,211,74,165,32,70,48,96
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,112,6,168,13,220,26,170,247,116,82,169,136,17,12,24
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,92,1,170,3,119,134,170,61,221,84,170,98,4,3,6
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,23,128,106,192,221,161,170,79,119,149,42,24,129,128,193
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,192,26,160,55,112,106,168,93,211,170,165,80,134,24,192
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,112,6,168,13,220,26,170,215,116,106,169,148,33,6,48
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,92,1,170,3,119,134,170,53,221,90,170,101,8,1,140
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,23,128,106,192,221,161,170,77,119,150,170,25,66,0,99
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,192,26,160,55,112,106,168,93,211,162,133,72,38,12,48
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,112,6,168,13,220,26,170,215,116,104,161,146,9,3,12
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,92,1,170,3,119,134,170,53,221,90,40,100,130,0,195
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,23,128,106,192,221,161,170,77,119,22,138,153,32,192,48
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,3,64,12,176,23,104,58,187,21,229,43,6,116,48,32,16,144,0,24,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,208,3,44,5,218,206,174,69,121,138,193,29,12,8,4,36,0,6,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,52,0,203,129,118,179,171,81,94,98,176,7,67,2,1,9,0,1,128
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,13,192,50,160,93,236,234,148,87,24,172,193,208,64,128,2,64,0,96
        .byte 0,0,0,0,0,0,0,0,7,12,12,148,51,216,44,128,91,48,102,24,156,0,216,0,160,0,96,0,144,0,24,0
        .byte 0,0,0,0,0,0,0,0,1,195,3,37,12,246,11,32,22,204,25,134,39,0,54,0,40,0,24,0,36,0,6,0
        .byte 0,0,0,0,0,0,0,0,192,112,64,201,131,61,2,200,5,179,134,97,9,192,13,128,10,0,6,0,9,0,1,128
        .byte 0,0,0,0,0,0,0,0,48,28,80,50,96,207,0,178,193,108,97,152,2,112,3,96,2,128,1,128,2,64,0,96
        .byte 7,192,15,224,13,96,25,48,25,48,31,240,30,240,15,224,69,68,226,142,56,56,7,192,56,56,224,14,64,4,0,0
        .byte 1,240,3,248,3,88,6,76,6,76,7,252,7,188,3,248,17,81,184,163,14,14,1,240,14,14,184,3,16,1,0,0
        .byte 0,124,0,254,0,214,1,147,1,147,1,255,1,239,0,254,68,84,238,40,131,131,0,124,131,131,238,0,68,0,0,0
        .byte 0,31,128,63,128,53,192,100,192,100,192,127,192,123,128,63,17,21,59,138,224,224,0,31,224,224,59,128,17,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
frmlst:
        .byte 0,3
        .byte 3,3
        .byte 6,3
        .byte 9,3
        .byte 12,3
        .byte 15,3
        .byte 18,3
        .byte 21,3
        .byte 24,2
        .byte 26,3
        .byte 29,3
        .byte 32,3
        .byte 35,3
        .byte 38,2
        .byte 40,2
        .byte 42,2
        .byte 44,2
        .byte 46,2
        .byte 48,5
        .byte 53,2,55,0
.out .sprintf("SPRITES SIZE = %d", (* - sprgfx))

scdat:
        .word 590,9,9,9,198,307,9
        .byte 1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,1,0,1,0,1,0
        .byte 1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1,0,6,2,2,7,1,0,9,0,1,9,24,23
        .byte 9,0,1,9,1,0,6,2,2,7,1,0,0,1,3,24,23,3,0,1,3,1,0,3,23,23,3,1,0,3,0,1,3
        .byte 24,23,3,0,1,1,0,8,23,23,8,1,0,3,0,1,5,2,2,4,0,1,3,1,0,8,23,23,8,1,0,0,1
        .byte 0,1,0,1,0,1,3,1,0,1,0,1,0,1,0,3,0,1,0,1,0,1,0,1,1,0,1,0,1,0,1,0,3
        .byte 0,1,0,1,0,1,0,1,3,1,0,1,0,1,0,1,0,0,1,6,255,2,5,4,1,0,9,0,1,9,1,0,5
        .byte 255,2,5,7,0,1,1,0,3,0,1,0,1,0,1,0,1,3,1,0,3,0,1,0,1,0,1,0,1,3,1,0,0
        .byte 1,3,1,0,1,0,1,0,1,0,3,0,1,3,1,0,1,0,1,0,1,0,3,0,1,1,0,8,0,1,11,2,2
        .byte 7,0,1,3,1,0,3,0,1,6,2,2,10,0,1,8,1,0,0,1,0,1,0,1,24,23,3,1,0,3,0,1,3
        .byte 1,0,3,24,23,0,1,0,1,0,1,1,0,1,0,1,0,23,23,3,0,1,3,1,0,3,0,1,3,23,23,1,0
        .byte 1,0,1,0,0,1,9,1,0,11,2,2,4,1,0,3,0,1,3,1,0,5,2,2,10,1,0,9,0,1,1,0,3
        .byte 0,1,0,1,0,1,0,1,3,1,0,3,0,1,0,1,0,1,0,1,3,1,0,0,1,3,1,0,1,0,1,0,1
        .byte 0,3,0,1,3,1,0,1,0,1,0,1,0,3,0,1,1,0,5,255,2,5,7,0,1,8,1,0,8,0,1,6,255,2,5
        .byte 4,1,0,0,1,0,1,0,1,0,1,3,1,0,1,0,1,0,1,0,3,0,1,0,1,0,1,0,1,1,0,1,0
        .byte 1,0,1,0,3,0,1,0,1,0,1,0,1,3,1,0,1,0,1,0,1,0,0,1,9,24,23,9,0,1,3,1,0
        .byte 6,2,2,7,1,0,3,0,1,9,24,23,9,0,1,1,0,3,23,23,3,1,0,3,0,1,3,24,23,3,0,1,3
        .byte 1,0,3,23,23,3,1,0,0,1,5,2,2,4,0,1,8,1,0,8,23,23,8,1,0,8,0,1,5,2,2,4,0
        .byte 1,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0
        .byte 255,1,0,255,1,0,255,1,86
        .byte 255,1,0,255,1,0,255,1,86
        .byte 255,1,0,255,1,0,255,1,86
        .byte 255,23,61,14,255,27,10,255,23,14,14,1,255,27,10,255,23,13,14,1,1,255,27,10,255,23,13,1,1,1,255,27,10
        .byte 255,23,13,1,1,1,255,27,10,255,23,13,1,1,1,255,27,10,255,23,13,1,1,1,255,27,10,255,23,13,1,1,1
        .byte 255,27,10,255,23,13,1,1,1,255,27,10,255,23,13,1,1,1,255,27,10,255,23,13,1,1,1,255,27,10,255,23,13
        .byte 1,1,1,255,27,10,255,23,13,1,1,1,255,27,10,255,23,13,1,1,1,255,27,10,255,23,13,1,1,1,255,27,10
        .byte 255,23,13,1,1,1,255,27,10,255,23,13,1,1,1,255,27,10,255,23,13,1,1,1,15,16,17,18,19,255,27,5,255,23,13
        .byte 1,1,1,23,23,25,23,23,255,27,5,255,23,13,1,1,1,23,23,25,23,23,255,27,5,255,23,6,255,26,26
        .byte 255,23,109,20,22,22,28,23,23,20,22,22,28,23,23,20,22,22,28,255,23,10,20,23,23,21,23,23,20,23,23,21,23,23
        .byte 20,23,23,21,255,23,10,20,22,22,28,23,23,20,22,22,28,23,23,20,22,22,28,255,23,10,20,23,23,21,23,23,20
        .byte 23,23,21,23,23,20,23,23,21,255,23,10,20,22,22,28,23,23,20,22,22,28,23,23,20,22,22,28,255,23,10,20,23,23
        .byte 21,23,23,20,23,23,21,23,23,20,23,23,21,255,23,10,20,22,22,28,23,23,20,22,22,28,23,23,20,22,22,28,255,23,10
        .byte 20,23,23,21,23,23,20,23,23,21,23,23,20,23,23,21,255,23,10,20,22,22,28,23,23,20,22,22,28,23,23,20,22,22
        .byte 28,255,23,10,20,23,23,21,23,23,20,23,23,21,23,23,20,23,23,21,255,23,10,20,22,22,28,23,23,20,22,22,28
        .byte 23,23,20,22,22,28,255,23,10,20,23,23,21,23,23,20,23,23,21,23,23,20,23,23,21,255,23,10,20,22,22,28,23,23
        .byte 20,22,22,28,23,23,20,22,22,28,255,23,10,20,23,23,21,23,23,20,23,23,21,23,23,20,23,23,21,255,23,10,20
        .byte 22,22,28,23,23,20,22,22,28,23,23,20,22,22,28,255,23,10,20,23,23,21,23,23,20,23,23,21,23,23,20,23,23
        .byte 21,255,23,83
        .byte 255,23,0,255,23,0,255,23,86
numsc:
        .byte 7
.out .sprintf("SCREENS SIZE = %d", (* - scdat))

nmedat:
        .byte 0,0,176,0,8,9,0,192,8,9,0,0,255
        .byte 0,0,176,0,8,10,0,192,8,9,0,0,255
        .byte 0,0,168,0,8,10,0,0,8,10,0,192,255
        .byte 0,0,176,0,8,10,0,0,8,11,0,96,8,10,0,192,255
        .byte 7,16,0,128,7,16,0,88,7,18,160,0,255
        .byte 5,0,80,96,5,9,144,48,5,9,120,96,5,9,144,144,255
        .byte 5,16,40,72,5,16,40,0,5,16,40,144,255
.out .sprintf("POSITIONS SIZE = %d", (* - nmedat))

NUMOBJ = 9
objdta:
        .byte 0,0,63,252,95,250,120,30,112,14,113,142,115,206,127,206,127,142,126,30,126,126,127,254,126,126,94,122,63,252,0,0,0,24,24,0,24,24
        .byte 0,0,63,252,95,250,120,30,112,14,113,142,115,206,127,206,127,142,126,30,126,126,127,254,126,126,94,122,63,252,0,0,0,0,0,0,0,0
        .byte 0,0,63,252,95,250,120,30,112,14,113,142,115,206,127,206,127,142,126,30,126,126,127,254,126,126,94,122,63,252,0,0,0,0,0,0,0,0
        .byte 0,0,63,252,95,250,120,30,112,14,113,142,115,206,127,206,127,142,126,30,126,126,127,254,126,126,94,122,63,252,0,0,0,0,0,0,0,0
        .byte 0,0,63,252,95,250,120,30,112,14,113,142,115,206,127,206,127,142,126,30,126,126,127,254,126,126,94,122,63,252,0,0,0,0,0,0,0,0
        .byte 0,0,63,252,95,250,120,30,112,14,113,142,115,206,127,206,127,142,126,30,126,126,127,254,126,126,94,122,63,252,0,0,0,0,0,0,0,0
        .byte 0,0,63,252,95,250,120,30,112,14,113,142,115,206,127,206,127,142,126,30,126,126,127,254,126,126,94,122,63,252,0,0,0,0,0,0,0,0
        .byte 0,0,63,252,95,250,120,30,112,14,113,142,115,206,127,206,127,142,126,30,126,126,127,254,126,126,94,122,63,252,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,1,0,17,16,9,32,0,0,3,128,58,184,3,128,0,0,9,32,17,16,1,0,0,0,0,0,0,0,255,0,0,255,0,0
.out .sprintf("OBJECTS SIZE = %d", (* - objdta))

font:
        .byte 158,135,201,201,201,201,201,0
        .byte 64,156,132,136,144,156,64,0
        .byte 0,238,74,74,74,78,0,0
        .byte 0,117,69,71,69,117,0,0
        .byte 0,119,85,117,85,85,0,0
        .byte 0,119,68,71,84,119,0,0
        .byte 64,32,32,32,32,32,64,0
        .byte 0,8,16,0,0,0,0,0
        .byte 238,138,238,40,232,0,0,0
        .byte 32,16,16,16,16,16,16,32
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,8,8,62,8,8,0
        .byte 0,0,0,0,0,8,8,16
        .byte 0,0,0,0,62,0,0,0
        .byte 0,0,0,0,0,24,24,0
        .byte 0,0,2,4,8,16,32,0
        .byte 0,126,66,66,70,70,126,0
        .byte 0,24,8,8,24,24,24,0
        .byte 0,126,66,2,126,96,126,0
        .byte 0,124,68,30,6,70,126,0
        .byte 0,124,68,68,68,126,12,0
        .byte 0,126,64,126,6,70,126,0
        .byte 0,126,64,126,70,70,126,0
        .byte 0,124,4,4,12,12,12,0
        .byte 0,60,36,126,70,70,126,0
        .byte 0,126,66,66,126,6,6,0
        .byte 0,0,0,16,0,0,16,0
        .byte 0,0,16,0,0,16,16,32
        .byte 0,0,4,8,16,8,4,0
        .byte 0,0,0,62,0,62,0,0
        .byte 0,0,16,8,4,8,16,0
        .byte 0,126,2,62,48,0,48,0
        .byte 0,60,74,86,94,64,60,0
        .byte 0,60,36,36,126,98,98,0
        .byte 0,124,68,126,98,98,126,0
        .byte 0,126,66,64,96,98,126,0
        .byte 0,126,66,66,98,98,126,0
        .byte 0,126,64,124,96,96,126,0
        .byte 0,126,64,124,96,96,96,0
        .byte 0,126,66,64,102,98,126,0
        .byte 0,66,66,126,98,98,98,0
        .byte 0,8,8,8,24,24,24,0
        .byte 0,4,4,4,70,70,124,0
        .byte 0,68,68,126,98,98,98,0
        .byte 0,32,32,32,48,48,62,0
        .byte 0,126,74,74,106,106,106,0
        .byte 0,126,66,66,98,98,98,0
        .byte 0,126,70,66,66,66,126,0
        .byte 0,126,66,66,126,96,96,0
        .byte 0,126,66,66,66,78,126,0
        .byte 0,124,68,68,126,98,98,0
        .byte 0,126,64,126,6,70,126,0
        .byte 0,124,16,16,24,24,24,0
        .byte 0,66,66,66,98,98,126,0
        .byte 0,98,98,98,102,36,60,0
        .byte 0,74,74,74,106,106,126,0
        .byte 0,66,66,60,98,98,98,0
        .byte 0,66,66,126,24,24,24,0
        .byte 0,126,6,60,112,96,126,0
        .byte 205,232,123,58,48,125,119,201
        .byte 205,200,235,126,50,48,125,201
        .byte 6,0,58,47,125,71,33,35
        .byte 248,9,201,252,48,48,48,0
        .byte 0,0,0,0,0,0,0,255
        .byte 0,58,45,125,103,58,46,125
        .byte 111,17,1,0,205,181,3,201
        .byte 218,146,210,146,218,0,0,0
        .byte 232,168,232,168,172,0,0,0
        .byte 234,74,78,74,74,0,0,0
        .byte 167,66,66,66,162,0,0,0
        .byte 115,82,82,82,115,0,0,0
        .byte 14,136,142,130,46,0,0,0
        .byte 238,170,238,168,168,0,0,0
        .byte 149,149,245,149,149,0,0,0
        .byte 184,41,179,41,168,0,0,0
        .byte 49,33,161,33,53,0,0,0
        .byte 93,85,213,85,93,0,0,0
        .byte 23,21,87,85,245,0,0,0
        .byte 118,85,101,85,86,0,0,0
        .byte 238,58,47,125,50,73,175,201
        .byte 173,6,9,119,35,16,252,201
        .byte 48,48,48,48,48,48,205,153
        .byte 124,201,205,153,124,205,185,124
        .byte 201,6,5,33,176,91,17,192
        .byte 134,175,26,190,202,171,124,210
        .byte 176,124,201,44,28,16,242,201
        .byte 119,44,28,26,16,250,195,171
        .byte 124,62,2,205,1,22,62,22
        .byte 215,58,49,125,215,58,50,125
        .byte 215,6,5,17,176,91,26,215
        .byte 19,16,251,201,124,96,124,0
        .byte 238,58,42,125,50,5,125,201
        .byte 0,8,8,8,8,8,8,0
        .byte 0,112,16,28,16,16,112,0
        .byte 0,20,40,0,0,0,0,0
        .byte 126,129,189,161,161,189,129,126
.out .sprintf("FONT SIZE = %d", (* - font))

keys:   .byte 66,97,104,72,98,16,55,48,49,17,18
.out .sprintf("KEYS SIZE = %d", (* - keys))

data_end:
.out .sprintf("TOTAL DATA SIZE = %d", (* - data_start))
