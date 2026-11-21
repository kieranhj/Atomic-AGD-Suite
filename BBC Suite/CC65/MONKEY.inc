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

WINDOWTOP = 2
WINDOWLFT = 1
WINDOWHGT = 20
WINDOWWID = 30 ;面面面面面面面面面面面面
MAPWID = 13
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255
mapdat:
        .byte 255,255,255,255,255,255,255,255,255,3,5,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,14,255,255,255,255,255,255,255,255,255,255,255,255,0,1,6,7,8,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,2,4,9,10,11,12,13,16,17,18,19,255
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255
stmap:  .byte 45

evnt00:
        lda #0
        cmp scno
        beq *+5
        jmp a00059
        lda #16
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp a00059
        jsr scru
        lda #144
        ldy #8
        sta (z80_ix),y
a00059: lda #11
        cmp scno
        beq *+5
        jmp a00108
        lda #148
        ldy #8
        sta (z80_ix),y
        lda #16
        iny; ldy #9
        sta (z80_ix),y
        jsr scrr
a00108: lda #16
        cmp scno
        beq *+5
        jmp a00139
        lda #255
        ldy #5
        sta (z80_ix),y
a00139: lda #13
        cmp scno
        beq *+5
        jmp a00533
        lda #228
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp a00285
        lda #14
        cmp vare
        bcs *+5
        jmp a00273
        lda #2
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        dey; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp a00285
a00273: lda #15
        sta scno
        jsr nwscr
a00285: lda #7
        sta z80_b
        jsr sktyp
        bcs :+
        jmp a00454
:
        lda #0
        cmp varh
        beq *+5
        jmp a00454
        lda #255
        ldy #5
        sta (z80_ix),y
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ; ldy #5
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a00446
        lda #7
        sta z80_c
        lda #25
        sta z80_b
        jsr spawn
        lda #0
        ldy #11
        sta (z80_ix),y
a00446: lda #1
        sta varh
a00454: lda #60
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp a00533
        jsr cangd
        beq :+
        jmp a00533
:
        lda #0
        sta varh
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
a00533: lda #10
        cmp scno
        beq *+5
        jmp a00618
        lda #1
        cmp vare
        beq *+5
        jmp a00618
        lda #255
        ldy #5
        sta (z80_ix),y
        tya; lda #5
        sta z80_c
        lda #4
        sta z80_b
        jsr spawn
        lda vare
        clc
        adc #1
        sta vare
a00618: lda #152
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a00780
        lda #2
        cmp scno
        beq *+5
        jmp a00698
        lda #4
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp a00780
a00698: lda #14
        cmp scno
        beq *+5
        jmp a00737
        jsr scrd
        lda #24
        ldy #8
        sta (z80_ix),y
        jmp a00780
a00737: lda #255
        ldy #5
        sta (z80_ix),y
        lda #8
        sta z80_c
        lda #20
        sta z80_b
        jsr spawn
        lda #1
        sta varp
a00780: ldy #8
        lda (z80_ix),y
        sta varb
        iny; ldy #9
        lda (z80_ix),y
        sta vard
        jsr skobj
        sta varobj
        lda #0
        cmp varobj
        beq *+5
        jmp a00865
        lda varobj
        jsr getob
        lda #50
        asl a
        sta sndtyp
        lda #1
        sta varg
a00865: lda joyval
        and #16
        beq :+
        jmp a00887
:
        jsr jump
a00887: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a00932
:
        lda #1
        ldy #11
        sta (z80_ix),y
        jmp a01110
a00932: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a00978
:
        lda #1
        ldy #11
        sta (z80_ix),y
        jmp a01110
a00978: lda #2
        cmp scno
        beq *+5
        jmp a01035
        lda #0
        ldy #11
        sta (z80_ix),y
        lda #64
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a01030
a01030: jmp a01110
a01035: lda #3
        cmp scno
        beq *+5
        jmp a01092
        lda #0
        ldy #11
        sta (z80_ix),y
        lda #64
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a01088
a01088: jmp a01110
a01092: lda #0
        ldy #11
        sta (z80_ix),y
        jsr ifall
a01110: lda #1
        cmp scno
        bcc *+5
        jmp a01277
        lda #3
        cmp scno
        bcs *+5
        jmp a01277
        lda #64
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a01259
        lda joyval
        and #4
        beq :+
        jmp a01188
:
        jmp a01254
a01188: jsr cangu
        beq :+
        jmp a01254
:
        lda #0
        cmp vara
        beq *+5
        jmp a01254
        lda #4
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
a01254: jmp a01277
a01259: jsr cangd
        beq :+
        jmp a01277
:
        jsr ifall
a01277: lda #13
        cmp scno
        beq *+5
        jmp a01298
        jmp a01873
a01298: lda joyval
        and #2
        beq :+
        jmp a01586
:
        lda #1
        ldy #10
        sta (z80_ix),y
        lda #12
        dey; ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp a01371
        jsr scrl
        lda #224
        ldy #9
        sta (z80_ix),y
a01371: jsr cangl
        beq :+
        jmp a01586
:
        lda #1
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp a01464
        lda #0
        cmp varc
        beq *+5
        jmp a01459
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
a01459: jmp a01586
a01464: jsr cangd
        beq :+
        jmp a01534
:
        lda #0
        cmp varc
        beq *+5
        jmp a01529
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
a01529: jmp a01586
a01534: lda #0
        cmp vara
        beq *+5
        jmp a01586
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
a01586: lda joyval
        and #1
        beq :+
        jmp a01873
:
        lda #0
        ldy #10
        sta (z80_ix),y
        lda #228
        dey; ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp a01658
        jsr scrr
        lda #16
        ldy #9
        sta (z80_ix),y
a01658: jsr cangr
        beq :+
        jmp a01873
:
        lda #1
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp a01751
        lda #0
        cmp varc
        beq *+5
        jmp a01746
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
a01746: jmp a01873
a01751: jsr cangd
        beq :+
        jmp a01821
:
        lda #0
        cmp varc
        beq *+5
        jmp a01817
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
a01817: jmp a01873
a01821: lda #0
        cmp vara
        beq *+5
        jmp a01873
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
a01873: lda joyval
        and #8
        beq :+
        jmp a02081
:
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01988
:
        lda #3
        ldy #10
        sta (z80_ix),y
        lda #0
        cmp varc
        beq *+5
        jmp a01984
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
a01984: jmp a02081
a01988: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a02081
:
        lda #3
        ldy #10
        sta (z80_ix),y
        lda #0
        cmp varc
        beq *+5
        jmp a02081
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
a02081: lda joyval
        and #4
        beq :+
        jmp a02164
:
        jsr cangd
        beq :+
        jmp a02164
:
        lda #0
        cmp vara
        beq *+5
        jmp a02164
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
a02164: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a02308
:
        jsr cangd
        beq :+
        jmp a02237
:
        lda #1
        ldy #6
        sta (z80_ix),y
        ; lda #1
        ldy #10
        sta (z80_ix),y
        jmp a02308
a02237: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a02272
        lda #8
        ldy #6
        sta (z80_ix),y
a02272: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a02308
        lda #9
        ldy #6
        sta (z80_ix),y
a02308: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a02451
:
        jsr cangd
        beq :+
        jmp a02380
:
        lda #2
        ldy #6
        sta (z80_ix),y
        lda #0
        ldy #10
        sta (z80_ix),y
        jmp a02451
a02380: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a02416
        lda #9
        ldy #6
        sta (z80_ix),y
a02416: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a02451
        lda #8
        ldy #6
        sta (z80_ix),y
a02451: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp a02671
        lda #1
        dey; ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a02586
        lda joyval
        and #16
        beq :+
        jmp a02533
:
        lda #1
        ldy #6
        sta (z80_ix),y
        jmp a02581
a02533: lda joyval
        and #2
        beq :+
        jmp a02568
:
        lda #3
        ldy #6
        sta (z80_ix),y
        jmp a02581
a02568: lda #9
        ldy #6
        sta (z80_ix),y
a02581: jmp a02671
a02586: lda joyval
        and #16
        beq :+
        jmp a02622
:
        lda #2
        ldy #6
        sta (z80_ix),y
        jmp a02671
a02622: lda joyval
        and #1
        beq :+
        jmp a02657
:
        lda #4
        ldy #6
        sta (z80_ix),y
        jmp a02671
a02657: lda #8
        ldy #6
        sta (z80_ix),y
a02671: lda #13
        cmp scno
        beq *+5
        jmp a02692
        jmp a02938
a02692: lda #1
        cmp scno
        bcc *+5
        jmp a02938
        lda #3
        cmp scno
        bcs *+5
        jmp a02866
        lda #64
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a02753
        jmp a02861
a02753: lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        tya; lda #8
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #16
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
a02861: jmp a02938
a02866: lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        tya; lda #8
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
a02938: lda #1
        cmp scno
        bcs *+5
        jmp a03027
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        tya; lda #8
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
a03027: lda #0
        cmp vara
        beq *+5
        jmp a03053
        ; lda #0
        jsr animsp
a03053: jmp grav
.out .sprintf("EVENT 0 SIZE = %d", (* - evnt00))

evnt01:
        lda #7
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp b00149
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp b00086
:
        jsr cangl
        beq :+
        jmp b00086
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
b00086: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp b00149
:
        jsr cangr
        beq :+
        jmp b00149
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
b00149: lda #1
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b00176
        jmp b00181
b00176: jsr ifall
b00181: jsr laddd
        beq :+
        jmp b00212
:
        lda #1
        ldy #11
        sta (z80_ix),y
        jmp b00389
b00212: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp b00271
:
        jsr cangu
        beq :+
        jmp b00267
:
        lda #1
        ldy #11
        sta (z80_ix),y
b00267: jmp b00389
b00271: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp b00330
:
        jsr cangu
        beq :+
        jmp b00326
:
        lda #1
        ldy #11
        sta (z80_ix),y
b00326: jmp b00389
b00330: jsr laddu
        beq :+
        jmp b00376
:
        jsr cangu
        beq :+
        jmp b00371
:
        lda #1
        ldy #11
        sta (z80_ix),y
b00371: jmp b00389
b00376: lda #0
        ldy #11
        sta (z80_ix),y
b00389: lda #7
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp b00588
        lda #1
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b00490
        lda #0
        cmp varc
        beq *+5
        jmp b00485
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
b00485: jmp b00588
b00490: lda vard
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp b00552
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #1
        sta z80_c
        lda #6
        sta z80_b
        jsr spawn
        jmp b00588
b00552: lda #255
        ldy #5
        sta (z80_ix),y
        lda #1
        sta z80_c
        tya; lda #5
        sta z80_b
        jsr spawn
b00588: lda #4
        ldy #6
        cmp (z80_ix),y
        bcc *+5
        jmp b00762
        tya; lda #6
        ; ldy #6
        cmp (z80_ix),y
        bcs *+5
        jmp b00762
        lda varb
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp b00762
        lda #1
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b00762
        lda #20
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #2
        cmp varrnd
        bcs *+5
        jmp b00762
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #1
        sta z80_c
        lda #7
        sta z80_b
        jsr spawn
b00762: lda #5
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp b00868
        lda #224
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp b00834
        ; ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp b00868
b00834: lda #255
        ldy #5
        sta (z80_ix),y
        lda #1
        sta z80_c
        lda #6
        sta z80_b
        jsr spawn
b00868: lda #6
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp b00976
        lda #8
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp b00940
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp b00976
b00940: lda #255
        ldy #5
        sta (z80_ix),y
        lda #1
        sta z80_c
        tya; lda #5
        sta z80_b
        jsr spawn
b00976: jsr cangr
        beq :+
        jmp b01075
:
        jsr cangl
        beq :+
        jmp b01075
:
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        tya; lda #8
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
b01075: lda #6
        ldy #6
        cmp (z80_ix),y
        bcc *+5
        jmp b01228
        lda #8
        ; ldy #6
        cmp (z80_ix),y
        bcs *+5
        jmp b01228
        ; lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        tya; lda #8
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #16
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
b01228: lda #0
        cmp vara
        beq *+5
        jmp b01254
        ; lda #0
        jsr animsp
b01254: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b01351
:
        lda #255
        ldy #5
        sta (z80_ix),y
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ; ldy #5
        sta (z80_ix),y
        lda #8
        sta z80_c
        lda #17
        sta z80_b
        jsr spawn
        lda #1
        sta varp
b01351: jmp grav
.out .sprintf("EVENT 1 SIZE = %d", (* - evnt01))

evnt02:
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00077
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #8
        sta z80_c
        lda #18
        sta z80_b
        jsr spawn
c00077: lda #0
        cmp vara
        beq *+5
        jmp c00103
        ; lda #0
        jsr animsp
c00103: lda #10
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00428
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp c00157
:
        jmp c00336
c00157: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp c00189
:
        jmp c00336
c00189: lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        tya; lda #8
        sta z80_c
        iny; ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        dey; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        tya; lda #8
        sta z80_c
        iny; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
c00336: jsr cangl
        beq :+
        jmp c00406
:
        lda #0
        cmp varc
        beq *+5
        jmp c00402
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
c00402: jmp c00428
c00406: lda #11
        ldy #6
        sta (z80_ix),y
        lda #0
        sta vari
c00428: lda #8
        cmp scno
        bcc *+5
        jmp c00780
        lda varb
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp c00780
        lda #0
        cmp varc
        beq *+5
        jmp c00780
        lda #10
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #2
        cmp varrnd
        bcs *+5
        jmp c00780
        lda #0
        cmp vari
        beq *+5
        jmp c00780
        lda #10
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00615
        lda vard
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp c00615
        lda #6
        sta z80_c
        lda #23
        sta z80_b
        jsr spawn
c00615: lda #11
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00681
        ldy #9
        lda (z80_ix),y
        cmp vard
        bcc *+5
        jmp c00681
        lda #6
        sta z80_c
        lda #23
        sta z80_b
        jsr spawn
c00681: lda #1
        sta vari
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda vard
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp c00744
        lda #1
        ldy #11
        sta (z80_ix),y
c00744: ldy #9
        lda (z80_ix),y
        cmp vard
        bcc *+5
        jmp c00780
        lda #0
        ldy #11
        sta (z80_ix),y
c00780: lda #11
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c01103
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp c00834
:
        jmp c01011
c00834: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp c00866
:
        jmp c01011
c00866: lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        tya; lda #8
        sta z80_c
        iny; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        dey; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
c01011: jsr cangr
        beq :+
        jmp c01081
:
        lda #0
        cmp varc
        beq *+5
        jmp c01077
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
c01077: jmp c01103
c01081: lda #10
        ldy #6
        sta (z80_ix),y
        lda #0
        sta vari
c01103: rts
.out .sprintf("EVENT 2 SIZE = %d", (* - evnt02))

evnt03:
        lda joyval
        and #64
        beq :+
        jmp d00028
:
        lda #1
        sta restfl
d00028: lda #29
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00654
        lda #120
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp d00087
        lda #2
        dey; ldy #7
        sta (z80_ix),y
d00087: lda #120
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp d00147
        lda #140
        ; ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp d00147
        lda #1
        dey; ldy #7
        sta (z80_ix),y
d00147: lda #140
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp d00184
        lda #0
        dey; ldy #7
        sta (z80_ix),y
d00184: lda #220
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp d00220
        lda #8
        ; ldy #9
        sta (z80_ix),y
d00220: lda #7
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp d00257
        lda #220
        ; ldy #9
        sta (z80_ix),y
d00257: lda #152
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp d00320
        lda #4
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp d00432
d00320: lda #88
        ldy #8
        sta (z80_ix),y
        lda #200
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        tya; lda #8
        sta z80_c
        iny; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda varrnd
        ; ldy #9
        sta (z80_ix),y
        lda vare
        clc
        adc #1
        sta vare
d00432: lda #140
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp d00491
        lda #2
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
d00491: lda #72
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp d00549
        lda #2
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
d00549: lda joyval
        and #2
        beq :+
        jmp d00601
:
        lda #3
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
d00601: lda joyval
        and #1
        beq :+
        jmp d00654
:
        lda #3
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
d00654: lda #28
        ldy #6
        cmp (z80_ix),y
        bcs *+5
        jmp d00702
        lda #0
        cmp vara
        beq *+5
        jmp d00702
        ; lda #0
        jsr animsp
d00702: lda #14
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00818
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00818
:
        lda #1
        sta scno
        jsr nwscr
        lda #50
        asl a
        sta sndtyp
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #72
        ldy #8
        sta (z80_ix),y
        lda #64
        iny; ldy #9
        sta (z80_ix),y
d00818: lda #13
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d01203
        lda #1
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp d00898
        lda #150
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp d00898
        lda #255
        ldy #5
        sta (z80_ix),y
d00898: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d01203
:
        lda #100
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp d01162
        lda #0
        cmp vare
        bcc *+5
        jmp d01050
        lda #2
        sta scno
        jsr nwscr
        lda #50
        asl a
        sta sndtyp
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #24
        ; ldy #8
        sta (z80_ix),y
        ; lda #24
        iny; ldy #9
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
d01050: lda #0
        cmp vare
        beq *+5
        jmp d01158
        lda #3
        sta scno
        jsr nwscr
        lda #50
        asl a
        sta sndtyp
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #24
        ldy #8
        sta (z80_ix),y
        ; lda #24
        iny; ldy #9
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
d01158: jmp d01203
d01162: lda #1
        ldy #12
        cmp (z80_ix),y
        bne *+5
        jmp d01199
        ; lda #1
        ; ldy #12
        sta (z80_ix),y
d01199: jsr scrr
d01203: lda #12
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d01425
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        tya; lda #8
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d01378
:
        lda #2
        cmp scno
        bne *+5
        jmp d01378
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #50
        asl a
        sta sndtyp
        lda vare
        clc
        adc #1
        sta vare
d01378: lda #0
        cmp vare
        bcc *+5
        jmp d01425
        lda #2
        cmp scno
        bne *+5
        jmp d01425
        lda #255
        ldy #5
        sta (z80_ix),y
d01425: rts
.out .sprintf("EVENT 3 SIZE = %d", (* - evnt03))

evnt04:
        lda joyval
        and #64
        beq :+
        jmp e00028
:
        lda #1
        sta restfl
e00028: lda #19
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00873
        lda #8
        cmp scno
        beq *+5
        jmp e00326
        lda #1
        cmp varg
        beq *+5
        jmp e00326
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00178
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #10
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
e00178: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp e00268
        lda #24
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp e00250
        ; ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp e00263
e00250: lda #1
        ldy #11
        sta (z80_ix),y
e00263: jmp e00322
e00268: jsr cangd
        beq :+
        jmp e00308
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp e00322
e00308: lda #0
        ldy #11
        sta (z80_ix),y
e00322: jmp e00326
e00326: lda #8
        cmp scno
        bne *+5
        jmp e00873
        lda #11
        cmp scno
        beq *+5
        jmp e00559
        lda #80
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp e00559
        lda #0
        cmp varc
        beq *+5
        jmp e00404
        jsr vapour
e00404: lda #5
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00559
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #152
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp e00516
        ; ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp e00559
e00516: lda #255
        ldy #5
        sta (z80_ix),y
        lda #8
        sta z80_c
        lda #20
        sta z80_b
        jsr spawn
        lda #1
        sta varp
e00559: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00685
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        jsr cangd
        beq :+
        jmp e00667
:
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        asl a
        sta sndtyp
e00667: lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
e00685: lda #150
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp e00721
        jsr cangd
        beq :+
        jmp e00721
:
e00721: jsr cangd
        beq :+
        jmp e00775
:
        lda #4
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp e00873
e00775: lda #50
        asl a
        sta sndtyp
        lda #16
        ldy #8
        sta (z80_ix),y
        lda #180
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #32
        sta z80_c
        lda varrnd
        clc
        adc z80_c
        sta varrnd
        lda varrnd
        iny; ldy #9
        sta (z80_ix),y
e00873: lda #0
        cmp varc
        beq *+5
        jmp e00899
        ; lda #0
        jsr animsp
e00899: lda #16
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e01341
        lda #13
        cmp scno
        bne *+5
        jmp e01041
        lda #150
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp e01019
        lda #4
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        asl a
        sta sndtyp
        jmp e01041
e01019: lda #255
        ldy #5
        sta (z80_ix),y
        lda #0
        sta varf
e01041: lda #13
        cmp scno
        beq *+5
        jmp e01162
        lda #16
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp e01140
        tya; lda #8
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        asl a
        sta sndtyp
        jmp e01162
e01140: lda #255
        ldy #5
        sta (z80_ix),y
        lda #0
        sta varf
e01162: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e01341
:
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #0
        sta varf
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ; ldy #5
        sta (z80_ix),y
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e01289
        lda #8
        sta z80_c
        lda #4
        sta z80_b
        jsr spawn
        lda #1
        sta varp
e01289: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e01341
        lda #8
        sta z80_c
        lda #3
        sta z80_b
        jsr spawn
        lda #1
        sta varp
e01341: lda #15
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e01712
        lda vard
        ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp e01431
        lda #0
        cmp varf
        beq *+5
        jmp e01431
        lda #4
        sta z80_c
        lda #16
        sta z80_b
        jsr spawn
        lda #1
        sta varf
e01431: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e01512
        jsr cangl
        beq :+
        jmp e01494
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp e01507
e01494: lda #1
        ldy #10
        sta (z80_ix),y
e01507: jmp e01566
e01512: jsr cangr
        beq :+
        jmp e01552
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp e01566
e01552: lda #0
        ldy #10
        sta (z80_ix),y
e01566: lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        dey; ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        tya; lda #8
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        tya; lda #8
        sta z80_c
        iny; ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
e01712: lda #0
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e02291
        lda #5
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e01839
:
        lda #10
        cmp vare
        beq *+5
        jmp e01778
        jmp e01839
e01778: lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #8
        sta z80_c
        lda #4
        sta z80_b
        jsr spawn
        lda #1
        sta varp
e01839: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e02017
:
        lda #11
        cmp scno
        beq *+5
        jmp e01883
        jmp e02017
e01883: lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e01966
        lda #8
        sta z80_c
        lda #4
        sta z80_b
        jsr spawn
        lda #1
        sta varp
e01966: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e02017
        lda #8
        sta z80_c
        lda #3
        sta z80_b
        jsr spawn
        lda #1
        sta varp
e02017: lda #2
        cmp scno
        beq *+5
        jmp e02069
        jsr cangl
        beq :+
        jmp e02069
:
        lda #0
        cmp varc
        beq *+5
        jmp e02069
        jsr vapour
e02069: lda #3
        cmp scno
        beq *+5
        jmp e02086
e02086: lda #11
        cmp scno
        beq *+5
        jmp e02125
        lda #152
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp e02125
e02125: jsr cangl
        beq :+
        jmp e02205
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #11
        cmp scno
        beq *+5
        jmp e02200
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
e02200: jmp e02291
e02205: lda #100
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #64
        sta z80_c
        lda varrnd
        clc
        adc z80_c
        sta varrnd
        lda #224
        ldy #9
        sta (z80_ix),y
        lda varrnd
        dey; ldy #8
        sta (z80_ix),y
e02291: rts
.out .sprintf("EVENT 4 SIZE = %d", (* - evnt04))

evnt05:
        lda joyval
        and #64
        beq :+
        jmp f00036
:
        lda #1
        sta restfl
        ; lda #1
        sta vare
f00036: lda #22
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f00240
        lda #8
        cmp vare
        bne *+5
        jmp f00080
        jmp f00080
f00080: lda joyval
        and #1
        beq :+
        jmp f00124
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #9
        sta (z80_ix),y
f00124: lda #1
        cmp vare
        bcc *+5
        jmp f00240
        lda #9
        cmp vare
        bcs *+5
        jmp f00240
        lda #16
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp f00211
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #9
        sta (z80_ix),y
        jmp f00240
f00211: lda #232
        ldy #9
        sta (z80_ix),y
        lda vare
        clc
        adc #1
        sta vare
f00240: lda #4
        ldy #6
        cmp (z80_ix),y
        bcc *+5
        jmp f00749
        tya; lda #6
        ; ldy #6
        cmp (z80_ix),y
        bcs *+5
        jmp f00749
        lda #5
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00444
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #4
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f00444
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #255
        dey; ldy #5
        sta (z80_ix),y
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ; ldy #5
        sta (z80_ix),y
        lda #8
        sta z80_c
        lda #17
        sta z80_b
        jsr spawn
        lda #1
        sta varp
f00444: lda joyval
        and #2
        beq :+
        jmp f00510
:
        lda #230
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp f00510
        ; ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
f00510: lda joyval
        and #1
        beq :+
        jmp f00577
:
        lda #8
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp f00577
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #9
        sta (z80_ix),y
f00577: jsr ifall
        lda #15
        cmp varp
        beq *+5
        jmp f00651
        lda #10
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #1
        cmp varrnd
        bcs *+5
        jmp f00651
        jsr jump
f00651: lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        tya; lda #8
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp f00749
        ; lda #0
        jsr animsp
f00749: lda #5
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f00830
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #232
        ; ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp f00830
        lda #16
        ; ldy #9
        sta (z80_ix),y
f00830: lda #6
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f00911
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #8
        ; ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp f00911
        lda #230
        ; ldy #9
        sta (z80_ix),y
f00911: lda #4
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f01786
        ldy #8
        lda (z80_ix),y
        sta varb
        iny; ldy #9
        lda (z80_ix),y
        sta vard
        tya; lda #9
        cmp vare
        bcs *+5
        jmp f01684
        lda joyval
        and #1
        beq :+
        jmp f01047
:
        lda #180
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp f01042
        ; ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
f01042: jmp f01091
f01047: lda #16
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp f01091
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
f01091: lda #11
        cmp scno
        beq *+5
        jmp f01347
        lda #158
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp f01166
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #8
        sta z80_c
        lda #20
        sta z80_b
        jsr spawn
f01166: lda joyval
        and #8
        beq :+
        jmp f01233
:
        lda #72
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp f01228
        ; ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
f01228: jmp f01255
f01233: ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
f01255: lda #21
        sta chary
        lda #1
        sta charx
        lda #0
        cmp vara
        beq *+5
        jmp f01297
        lda #17
        jsr dmsg
f01297: lda #1
        cmp vara
        beq *+5
        jmp f01322
        lda #18
        jsr dmsg
f01322: lda #2
        cmp vara
        beq *+5
        jmp f01347
        lda #19
        jsr dmsg
f01347: lda #10
        cmp scno
        beq *+5
        jmp f01587
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        tya; lda #8
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jsr ifall
        lda #5
        cmp vare
        beq *+5
        jmp f01474
        lda #1
        sta nexlev
        lda #6
        sta vare
f01474: lda joyval
        and #16
        beq :+
        jmp f01496
:
        jsr jump
f01496: lda #13
        sta chary
        lda #1
        sta charx
        lda #0
        cmp vara
        beq *+5
        jmp f01537
        lda #14
        jsr dmsg
f01537: lda #1
        cmp vara
        beq *+5
        jmp f01562
        lda #15
        jsr dmsg
f01562: lda #2
        cmp vara
        beq *+5
        jmp f01587
        lda #16
        jsr dmsg
f01587: lda #3
        sta chary
        lda #1
        sta charx
        lda #0
        cmp vara
        beq *+5
        jmp f01629
        lda #14
        jsr dmsg
f01629: lda #1
        cmp vara
        beq *+5
        jmp f01654
        lda #15
        jsr dmsg
f01654: lda #2
        cmp vara
        beq *+5
        jmp f01679
        lda #16
        jsr dmsg
f01679: jmp f01760
f01684: lda #200
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp f01747
        lda #8
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp f01760
f01747: lda #0
        ldy #5
        sta (z80_ix),y
f01760: lda #0
        cmp vara
        beq *+5
        jmp f01786
        ; lda #0
        jsr animsp
f01786: jmp grav
.out .sprintf("EVENT 5 SIZE = %d", (* - evnt05))

evnt06:
        lda #23
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp g00361
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00121
:
        lda #255
        ldy #5
        sta (z80_ix),y
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ; ldy #5
        sta (z80_ix),y
        lda #8
        sta z80_c
        lda #18
        sta z80_b
        jsr spawn
        lda #1
        sta varp
g00121: lda #0
        cmp vara
        beq *+5
        jmp g00147
        ; lda #0
        jsr animsp
g00147: lda #1
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp g00254
        lda #8
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp g00232
        ; lda #8
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp g00254
g00232: lda #255
        ldy #5
        sta (z80_ix),y
        lda #0
        sta vari
g00254: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp g00361
        lda #220
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp g00339
        lda #8
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp g00361
g00339: lda #255
        ldy #5
        sta (z80_ix),y
        lda #0
        sta vari
g00361: rts
.out .sprintf("EVENT 6 SIZE = %d", (* - evnt06))

evnt07:
        lda #0
        cmp vara
        beq *+5
        jmp h00027
        ; lda #0
        jsr animsp
h00027: lda #14
        cmp vare
        bcc *+5
        jmp h00091
        lda #12
        sta chary
        lda #26
        sta charx
        lda #1
        cmp vara
        bcs *+5
        jmp h00083
        jmp h00083
h00083: lda #31
        jsr dmsg
h00091: lda #25
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h00262
        ldy #9
        lda (z80_ix),y
        sta vard
        lda #120
        ; ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp h00176
        lda joyval
        and #1
        beq :+
        jmp h00176
:
        lda #120
        sta vark
h00176: lda #4
        sta z80_b
        jsr sktyp
        bcs :+
        jmp h00262
:
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #7
        sta z80_c
        lda #24
        sta z80_b
        jsr spawn
        lda #8
        sta z80_c
        lda #4
        sta z80_b
        jsr spawn
        lda #1
        sta varp
h00262: lda #27
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h00384
        ldy #9
        lda (z80_ix),y
        sta vard
        lda #4
        sta z80_b
        jsr sktyp
        bcs :+
        jmp h00384
:
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #7
        sta z80_c
        lda #26
        sta z80_b
        jsr spawn
        lda #8
        sta z80_c
        lda #4
        sta z80_b
        jsr spawn
        lda #1
        sta varp
h00384: lda #25
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h00492
        lda varrnd
        ldy #11
        cmp (z80_ix),y
        bcs *+5
        jmp h00492
        lda joyval
        and #1
        beq :+
        jmp h00492
:
        lda #200
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp h00492
        ; ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
h00492: lda #27
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h00599
        lda varrnd
        ldy #11
        cmp (z80_ix),y
        bcs *+5
        jmp h00599
        lda joyval
        and #2
        beq :+
        jmp h00599
:
        lda #16
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp h00599
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
h00599: lda #0
        cmp vark
        bcc *+5
        jmp h01043
        lda #24
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h00643
        jmp h00908
h00643: lda #26
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h00670
        jmp h00908
h00670: lda #3
        sta varl
        lda #2
        sta loopa
h00686: lda varl
        sta chary
        lda #1
        sta charx
        lda #0
        cmp vara
        beq *+5
        jmp h00728
        lda #20
        jsr dmsg
h00728: lda #1
        cmp vara
        beq *+5
        jmp h00753
        lda #21
        jsr dmsg
h00753: lda #2
        cmp vara
        beq *+5
        jmp h00778
        lda #22
        jsr dmsg
h00778: lda #6
        sta z80_c
        lda varl
        clc
        adc z80_c
        sta varl
        dec loopa
        beq :+
        jmp h00686
:
        lda #21
        sta chary
        lda #1
        sta charx
        lda #0
        cmp vara
        beq *+5
        jmp h00858
        lda #23
        jsr dmsg
h00858: lda #1
        cmp vara
        beq *+5
        jmp h00883
        lda #24
        jsr dmsg
h00883: lda #2
        cmp vara
        beq *+5
        jmp h00908
        lda #25
        jsr dmsg
h00908: lda #8
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp h01027
        lda #25
        ldy #6
        cmp (z80_ix),y
        bcs *+5
        jmp h00992
        lda #4
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp h01027
h00992: lda #2
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
h01027: lda vark
        sec
        sbc #1
        sta vark
h01043: lda #32
        ldy #8
        sta (z80_ix),y
        lda #24
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h01083
        jmp h01182
h01083: lda #26
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h01110
        jmp h01182
h01110: lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        tya; lda #8
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
h01182: lda #25
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h01209
        jmp h01309
h01209: lda #27
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h01236
        jmp h01309
h01236: lda #220
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp h01273
        lda #16
        ; ldy #9
        sta (z80_ix),y
h01273: lda #8
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp h01309
        lda #216
        ; ldy #9
        sta (z80_ix),y
h01309: lda joyval
        and #16
        beq :+
        jmp h01747
:
        lda #0
        cmp varj
        beq *+5
        jmp h01743
        lda #220
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp h01743
        lda #25
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h01555
        lda #24
        ; ldy #6
        sta (z80_ix),y
        ; lda #24
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #0
        sta z80_c
        lda #2
        sta z80_b
        jsr spawn
        lda #1
        sta varj
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #0
        ldy #10
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #24
        sta z80_c
        dey; ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
h01555: lda #27
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h01743
        lda #26
        ; ldy #6
        sta (z80_ix),y
        lda #24
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #0
        sta z80_c
        lda #2
        sta z80_b
        jsr spawn
        lda #1
        sta varj
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #0
        ldy #10
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #24
        sta z80_c
        dey; ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
h01743: jmp h01755
h01747: lda #0
        sta varj
h01755: lda #25
        ldy #6
        cmp (z80_ix),y
        bcs *+5
        jmp h02141
        lda #1
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp h01857
        lda #30
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #10
        sta z80_c
        lda varrnd
        clc
        adc z80_c
        sta varrnd
h01857: lda varrnd
        ldy #11
        cmp (z80_ix),y
        bcs *+5
        jmp h02055
        lda #224
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp h01943
        lda #2
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp h02023
h01943: lda #24
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h01979
        lda #26
        ; ldy #6
        sta (z80_ix),y
h01979: lda #25
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h02023
        lda #27
        ; ldy #6
        sta (z80_ix),y
        lda #0
        sta vark
h02023: ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
        jmp h02141
h02055: lda #24
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h02091
        lda #26
        ; ldy #6
        sta (z80_ix),y
h02091: lda #25
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h02127
        lda #27
        ; ldy #6
        sta (z80_ix),y
h02127: lda #0
        ldy #11
        sta (z80_ix),y
h02141: lda #25
        ldy #6
        cmp (z80_ix),y
        bcc *+5
        jmp h02505
        lda #1
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp h02242
        lda #30
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #10
        sta z80_c
        lda varrnd
        clc
        adc z80_c
        sta varrnd
h02242: lda varrnd
        ldy #11
        cmp (z80_ix),y
        bcs *+5
        jmp h02419
        lda #2
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp h02315
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp h02387
h02315: lda #26
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h02351
        lda #24
        ; ldy #6
        sta (z80_ix),y
h02351: lda #27
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h02387
        lda #25
        ; ldy #6
        sta (z80_ix),y
h02387: ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
        jmp h02505
h02419: lda #0
        ldy #11
        sta (z80_ix),y
        lda #26
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h02468
        lda #24
        ; ldy #6
        sta (z80_ix),y
h02468: lda #27
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h02505
        lda #25
        ; ldy #6
        sta (z80_ix),y
h02505: rts
.out .sprintf("EVENT 7 SIZE = %d", (* - evnt07))

evnt08:
        lda #14
        cmp scno
        bcc *+5
        jmp i00127
        lda #18
        cmp scno
        bcs *+5
        jmp i00127
        lda #10
        sta chary
        lda #1
        sta charx
        lda #0
        cmp vara
        beq *+5
        jmp i00077
        lda #26
        jsr dmsg
i00077: lda #1
        cmp vara
        beq *+5
        jmp i00102
        lda #27
        jsr dmsg
i00102: lda #2
        cmp vara
        beq *+5
        jmp i00127
        lda #28
        jsr dmsg
i00127: lda #28
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i00840
        lda #40
        cmp vare
        beq *+5
        jmp i00186
        jsr scrr
        lda vare
        clc
        adc #1
        sta vare
i00186: lda #80
        cmp vare
        beq *+5
        jmp i00223
        jsr scrr
        lda vare
        clc
        adc #1
        sta vare
i00223: lda #140
        cmp vare
        beq *+5
        jmp i00259
        jsr scrr
        lda vare
        clc
        adc #1
        sta vare
i00259: lda #19
        cmp scno
        beq *+5
        jmp i00290
        lda #255
        ldy #5
        sta (z80_ix),y
i00290: lda joyval
        and #1
        beq :+
        jmp i00357
:
        lda #150
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp i00353
        ; ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
i00353: jmp i00514
i00357: lda joyval
        and #2
        beq :+
        jmp i00424
:
        lda #90
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp i00420
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
i00420: jmp i00514
i00424: lda #120
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp i00469
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
i00469: lda #119
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp i00514
        ; ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
i00514: lda #3
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i00579
:
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #8
        sta z80_c
        lda #4
        sta z80_b
        jsr spawn
        lda #2
        sta varp
i00579: lda #10
        sta chary
        lda #1
        sta charx
        ; lda #1
        cmp vara
        beq *+5
        jmp i00669
        lda joyval
        and #1
        beq :+
        jmp i00643
:
        lda #29
        jsr dmsg
        jmp i00669
i00643: lda joyval
        and #2
        beq :+
        jmp i00669
:
        lda #30
        jsr dmsg
i00669: lda #2
        cmp vara
        beq *+5
        jmp i00742
        lda joyval
        and #1
        beq :+
        jmp i00716
:
        lda #30
        jsr dmsg
        jmp i00742
i00716: lda joyval
        and #2
        beq :+
        jmp i00742
:
        lda #29
        jsr dmsg
i00742: lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        tya; lda #8
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp i00840
        ; lda #0
        jsr animsp
i00840: lda #13
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i01018
        ; lda #13
        cmp scno
        beq *+5
        jmp i00975
        lda #0
        cmp vark
        bcc *+5
        jmp i00975
        lda #8
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp i00946
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp i00975
i00946: lda #230
        ldy #9
        sta (z80_ix),y
        lda vare
        clc
        adc #1
        sta vare
i00975: lda #19
        cmp scno
        beq *+5
        jmp i01018
        lda joyval
        and #16
        beq :+
        jmp i01018
:
        lda #1
        sta gamwon
i01018: lda #2
        ldy #6
        cmp (z80_ix),y
        bcc *+5
        jmp i01283
        lda #4
        ; ldy #6
        cmp (z80_ix),y
        bcs *+5
        jmp i01283
        lda #0
        sta vark
        lda #11
        cmp scno
        bne *+5
        jmp i01097
        jsr ifall
        jmp i01172
i01097: lda #152
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp i01129
        jsr ifall
        jmp i01172
i01129: lda #255
        ldy #5
        sta (z80_ix),y
        lda #8
        sta z80_c
        lda #20
        sta z80_b
        jsr spawn
        lda #1
        sta varp
i01172: lda #13
        cmp scno
        bne *+5
        jmp i01237
        lda #3
        sta z80_c
        lda varc
        clc
        adc z80_c
        sta varc
        lda #3
        sta z80_c
        lda varc
        sec
        sbc z80_c
        sta varc
i01237: lda #3
        asl a
        sta sndtyp
        lda #0
        jsr animsp
        lda #30
        cmp varp
        beq *+5
        jmp i01283
        lda #1
        sta deadf
i01283: lda #20
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i01688
        lda #0
        sta vark
        ldy #9
        lda (z80_ix),y
        lsr a
        lsr a
        lsr a
        ; ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        asl a
        asl a
        asl a
        ; ldy #9
        sta (z80_ix),y
        jsr ifall
        lda #3
        asl a
        sta sndtyp
        lda #11
        cmp scno
        beq *+5
        jmp i01484
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        tya; lda #8
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp i01629
i01484: lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        tya; lda #8
        sta z80_c
        iny; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        dey; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
i01629: lda #30
        cmp varp
        beq *+5
        jmp i01680
        lda #1
        sta deadf
        lda #11
        cmp scno
        beq *+5
        jmp i01680
        lda #6
        sta vare
i01680: lda #0
        jsr animsp
i01688: lda #18
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i01915
        lda #3
        asl a
        sta sndtyp
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        dey; ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        tya; lda #8
        sta z80_c
        iny; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        dey; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jsr ifall
        lda #0
        jsr animsp
        lda #30
        cmp varp
        beq *+5
        jmp i01915
        lda #1
        sta deadf
        lda #0
        sta vari
i01915: lda #17
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i02294
        ldy #9
        lda (z80_ix),y
        lsr a
        lsr a
        lsr a
        ; ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        asl a
        asl a
        asl a
        ; ldy #9
        sta (z80_ix),y
        jsr ifall
        lda #3
        asl a
        sta sndtyp
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp i02076
:
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
i02076: lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        tya; lda #8
        sta z80_c
        iny; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        dey; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #0
        jsr animsp
        lda #30
        cmp varp
        beq *+5
        jmp i02294
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #1
        sta deadf
        lda #10
        cmp scno
        beq *+5
        jmp i02294
        lda #1
        sta vare
i02294: jmp grav
.out .sprintf("EVENT 8 SIZE = %d", (* - evnt08))

evnt09:
        rts
.out .sprintf("EVENT 9 SIZE = %d", (* - evnt09))

evnt10:
        lda #30
        cmp varp
        beq *+5
        jmp k00031
        lda #0
        sta varp
        jmp k00047
k00031: lda varp
        clc
        adc #1
        sta varp
k00047: lda #3
        cmp varc
        beq *+5
        jmp k00076
        lda #0
        sta varc
        jmp k00091
k00076: lda varc
        clc
        adc #1
        sta varc
k00091: lda #2
        cmp vara
        beq *+5
        jmp k00121
        lda #0
        sta vara
        jmp k00136
k00121: lda vara
        clc
        adc #1
        sta vara
k00136: rts
.out .sprintf("EVENT 10 SIZE = %d", (* - evnt10))

evnt11:
        rts
.out .sprintf("EVENT 11 SIZE = %d", (* - evnt11))

evnt12:
        jsr cls
        lda #15
        sta chary
        ; lda #15
        sta charx
        lda #11
        jsr dmsg
        lda #16
        sta chary
        lda #14
        sta charx
        lda #12
        jsr dmsg
        lda #14
        sta chary
        lda #17
        sta charx
        lda #9
        jsr dmsg
        lda #15
        sta chary
        lda #17
        sta charx
        lda #10
        jsr dmsg
        lda #17
        sta chary
        lda #9
        sta charx
        lda #13
        jsr dmsg
        lda #3
        sta chary
        lda #10
        sta charx
        lda #5
        jsr dmsg
        lda #7
        sta chary
        lda #1
        sta charx
        lda #6
        jsr dmsg
        lda #9
        sta chary
        lda #3
        sta charx
        lda #7
        jsr dmsg
        lda #11
        sta chary
        lda #3
        sta charx
        lda #8
        jsr dmsg
        lda #20
        sta chary
        lda #0
        sta charx
        lda #4
        jsr dmsg
        jsr prskey
        rts
.out .sprintf("EVENT 12 SIZE = %d", (* - evnt12))

evnt13:
        lda #0
        sta varh
        ; lda #0
        sta varg
        ; lda #0
        sta varf
        ; lda #0
        sta vare
        ; lda #0
        sta numlif
        jsr cls
        rts
.out .sprintf("EVENT 13 SIZE = %d", (* - evnt13))

evnt14:
        lda #19
        cmp scno
        beq *+5
        jmp o00120
        lda #0
        sta chary
        ; lda #0
        sta charx
        ; lda #0
        jsr dmsg
        lda #22
        sta chary
        lda #3
        sta charx
        lda #1
        jsr dmsg
        lda #23
        sta chary
        lda #1
        sta charx
        lda #2
        jsr dmsg
        lda #22
        sta chary
        lda #19
        sta charx
        lda numlif
        jsr disply
o00120: lda #13
        cmp scno
        beq *+5
        jmp o00145
        lda #10
        sta vare
o00145: lda #16
        cmp scno
        beq *+5
        jmp o00171
        lda #20
        sta vare
o00171: lda #17
        cmp scno
        beq *+5
        jmp o00196
        lda #50
        sta vare
o00196: lda #18
        cmp scno
        beq *+5
        jmp o00221
        lda #90
        sta vare
o00221: lda #0
        sta vark
        ; lda #0
        sta vari
        ; lda #0
        sta varq
        ; lda #0
        sta varf
        ; lda #0
        sta vara
        lda #19
        cmp scno
        beq *+5
        jmp o00285
        jmp o00289
o00285: jsr cls
o00289: rts
.out .sprintf("EVENT 14 SIZE = %d", (* - evnt14))

evnt15:
        rts
.out .sprintf("EVENT 15 SIZE = %d", (* - evnt15))

evnt16:
        lda numlif
        clc
        adc #1
        sta numlif
        rts
.out .sprintf("EVENT 16 SIZE = %d", (* - evnt16))

evnt17:
        lda #50
        jsr delay
        rts
.out .sprintf("EVENT 17 SIZE = %d", (* - evnt17))

evnt18:
        lda #50
        jsr delay
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
        .byte "YOU HAVE FOUND THE LOST TEMPLE",141
        .byte "YOU HAVE USED      LIVES",141
        .byte "TRY TO DO BETTER NEXT TIME!",141
        .byte "PRESS M FOR MENU",141
        .byte "1.QAOPM   2.KEMPSTON  3.SINCLAIR",141
        .byte "MONKEY J. IN:",13
        .byte 141
        .byte "THE TREASURE OF THE GOLD TEMPLE",141
        .byte "THAT WAS LOST IN THE FOREST",141
        .byte "UNTIL HE (OR SHE?) FOUND IT",141
        .byte "a",141
        .byte "bc",141
        .byte "d",141
        .byte "ef",141
        .byte "gggggggggggggg",141
        .byte "gggggggggggggggggggggggggggggg",13
        .byte 141
        .byte "hhhhhhhhhhhhhhhhhhhhhhhhhhhhhh",13
        .byte 141
        .byte "iiiiiiiiiiiiiiiiiiiiiiiiiiiiii",13
        .byte 141
        .byte "jjjjjjjjjjjjjjjjjjjjjjjjjjjjjj",13
        .byte 141
        .byte "kkkkkkkkkkkkkkkkkkkkkkkkkkkkkk",141
        .byte "llllllllllllllllllllllllllllll",141
        .byte "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmm",13
        .byte 141
        .byte "nnnnnnnnnnnnnnnnnnnnnnnnnnnnnn",13
        .byte 141
        .byte "oooooooooooooooooooooooooooooo",13
        .byte 141
        .byte "pppppppppppppppppppppppppppppp",141
        .byte "qqqqqqqqqqqqqqqqqqqqqqqqqqqqqq",141
        .byte "rrrrrrrrrrrrrrrrrrrrrrrrrrrrrr",141
        .byte "ssssssssssssssssssssssssssssss",141
        .byte "tttttttttttttttuuuuuuuuuuuuuuu",141
        .byte "uuuuuuuuuuuuuuuttttttttttttttt",141
        .byte "tttttttttttttttttttttttttttttt",141
        .byte "uuuuuuuuuuuuuuuuuuuuuuuuuuuuuu",141
        .byte "JUMP!",141
nummsg:
        .byte 32
.out .sprintf("MESSAGE SIZE = %d", (* - msgdat))

chgfx:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 245,245,248,242,240,245,248,248,240,241,248,240,244,241,250,250
        .byte 0,0,0,72,33,33,66,90,36,165,82,74,33,132,16,0
        .byte 33,164,66,18,160,132,88,90,133,36,88,80,5,5,24,26
        .byte 8,0,0,0,8,0,0,0,8,0,0,0,8,0,0,0
        .byte 0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 240,240,240,248,245,244,242,248,241,244,242,250,244,244,240,240
        .byte 128,0,128,0,0,0,0,0,128,0,128,0,0,0,0,0
        .byte 0,0,0,16,0,16,0,0,0,0,0,16,0,16,0,0
        .byte 15,15,47,15,15,15,15,15,15,143,15,15,15,15,15,15
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 245,245,0,114,0,117,0,16,0,17,0,16,0,17,0,0
        .byte 0,0,250,186,245,177,250,186,128,49,50,136,177,245,186,250
        .byte 245,228,250,234,245,228,0,0,245,117,250,114,245,117,250,114
        .byte 245,245,250,250,245,245,250,250,245,245,250,250,245,245,250,250
        .byte 245,245,250,250,245,245,250,250,245,245,250,250,245,245,250,250
        .byte 245,245,250,250,245,245,250,250,245,245,250,250,245,245,250,250
        .byte 245,245,248,242,240,245,248,248,240,241,248,240,244,241,250,250
        .byte 241,240,242,248,245,245,250,250,245,244,242,250,244,245,250,242
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,161,128,88,74
        .byte 164,133,88,26,5,160,10,8,32,0,0,0,0,0,0,0
        .byte 112,240,0,0,112,240,80,176,112,240,112,112,112,240,0,0
        .byte 0,0,255,238,255,238,119,136,136,119,255,119,255,119,255,102
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 165,165,90,90,165,165,90,90,165,165,90,90,165,165,90,90
        .byte 133,132,2,66,161,160,88,80,5,5,24,82,36,160,74,10
        .byte 95,95,143,15,15,79,47,15,15,15,15,15,15,15,15,15
        .byte 0,18,1,37,18,82,33,161,16,8,0,37,0,82,0,33
        .byte 245,245,250,0,196,0,200,0,128,0,136,0,0,0,0,0
        .byte 96,240,240,96,240,0,96,128,16,192,48,192,16,128,0,0
        .byte 31,95,15,143,79,15,175,47,31,95,15,143,79,15,175,47
        .byte 160,132,80,80,5,36,82,10,161,36,72,72,36,160,2,66
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 33,164,66,18,160,132,88,90,133,36,88,80,5,5,24,26
        .byte 160,132,80,80,5,36,82,26,161,37,72,88,36,161,2,66
        .byte 33,0,66,8,5,128,82,0,164,128,16,8,33,128,66,0
        .byte 0,0,15,11,15,11,15,11,8,3,3,8,11,15,11,15
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 240,240,250,250,245,241,250,250,240,241,242,248,241,245,250,250
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 90,90,250,190,245,241,250,190,210,121,182,173,241,245,190,250
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 95,95,143,15,15,79,47,15,15,15,15,15,15,15,15,15
        .byte 45,165,30,90,45,165,30,90,15,45,15,15,15,15,15,15
        .byte 0,0,0,16,0,16,0,0,0,0,0,0,0,16,0,16
        .byte 128,0,128,0,0,0,0,0,0,0,128,0,128,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 15,15,165,165,75,90,165,165,15,15,45,165,90,90,45,165
        .byte 240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240
        .byte 240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240
        .byte 0,0,175,174,95,78,39,136,8,87,175,39,95,87,175,39
        .byte 165,165,82,90,37,165,82,74,37,164,18,72,1,128,0,0
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 0,0,8,0,8,0,8,1,8,1,12,3,14,7,15,15
        .byte 245,245,250,250,0,0,250,250,245,245,200,50,128,17,136,16
        .byte 245,245,250,250,0,0,250,250,213,177,216,186,213,177,216,186
        .byte 90,90,165,165,90,90,165,165,90,90,165,165,90,90,165,165
        .byte 95,95,175,175,95,95,175,175,95,95,175,175,95,95,175,175
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
        .byte 90,90,165,165,90,90,165,165,90,90,165,165,90,90,165,165
bprop:
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 3
        .byte 3
        .byte 2
        .byte 2
        .byte 5
        .byte 6
        .byte 7
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
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 0
        .byte 4
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 2
        .byte 5
        .byte 6
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 0
        .byte 1
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
.out .sprintf("BLOCKS SIZE = %d", (* - chgfx))

sprgfx:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,28,239,176,239,176,64,28,0,0,0,0
        .byte 0,0,0,0,64,28,207,188,207,188,64,28,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,64,0,239,188,239,188,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 44,0,20,0,45,0,81,0,124,0,2,0,28,0,0,0,222,0,211,0,15,128,223,192,219,192,107,128,0,72,0,48
        .byte 0,0,44,0,20,0,45,0,81,0,124,0,30,0,0,0,0,0,223,128,211,200,15,200,31,144,107,96,104,0,48,0
        .byte 0,52,0,40,0,180,0,138,0,62,0,64,0,56,0,0,0,123,0,203,1,240,3,251,3,219,1,214,18,0,12,0
        .byte 0,0,0,52,0,40,0,180,0,138,0,62,0,120,0,0,0,0,1,251,19,203,19,240,9,248,6,214,0,22,0,12
        .byte 0,0,0,0,88,0,40,0,88,0,162,0,242,0,120,0,1,129,31,226,38,236,92,240,128,118,216,2,28,0,0,0
        .byte 0,0,88,0,40,0,88,0,162,0,242,0,120,0,0,0,7,0,15,128,14,192,13,192,5,208,1,144,6,96,7,0
        .byte 0,0,0,0,0,26,0,20,0,26,0,69,0,79,0,30,129,128,71,248,55,100,15,58,110,1,64,27,0,56,0,0
        .byte 0,0,0,26,0,20,0,26,0,69,0,79,0,30,0,0,0,224,1,240,3,112,3,176,11,160,9,128,6,96,0,224
        .byte 0,0,0,0,0,0,0,0,0,26,0,53,0,53,0,26,56,101,124,255,110,239,78,230,94,224,93,224,223,192,15,128
        .byte 0,0,0,0,0,0,0,0,0,52,0,106,0,106,0,180,1,202,1,254,128,222,128,108,158,112,191,248,247,248,99,48
        .byte 0,0,0,0,0,0,0,0,88,0,172,0,172,0,88,0,166,28,255,62,247,118,103,114,7,122,7,186,3,251,1,240
        .byte 0,0,0,0,0,0,0,0,44,0,86,0,86,0,45,0,83,128,127,128,123,1,54,1,14,121,31,253,31,239,12,198
        .byte 0,0,0,0,0,0,0,0,6,192,13,96,13,96,6,192,9,32,15,224,71,192,8,32,143,224,231,192,127,192,63,128
        .byte 0,0,0,0,0,0,0,0,3,96,6,176,6,176,3,96,4,144,7,240,67,224,68,0,71,224,113,224,63,240,31,224
        .byte 0,0,0,0,0,0,0,0,3,96,6,176,6,176,3,96,4,144,7,246,3,239,0,31,1,253,0,253,0,243,0,14
        .byte 0,0,0,0,0,0,0,0,6,192,13,96,13,96,6,192,9,32,111,224,199,192,200,32,207,224,239,192,127,128,31,0
        .byte 0,0,0,26,0,20,0,26,0,197,0,207,0,30,0,0,0,224,1,240,3,104,3,160,11,172,9,156,6,96,0,224
        .byte 0,0,0,0,0,26,0,20,0,26,0,197,0,207,0,30,0,96,0,248,1,244,1,212,3,132,11,176,9,116,6,12
        .byte 0,0,88,0,40,0,88,0,163,0,243,0,120,0,0,0,7,0,15,128,22,192,5,192,53,208,57,144,6,96,7,0
        .byte 0,0,0,0,88,0,40,0,88,0,163,0,243,0,120,0,6,0,31,0,47,128,43,128,33,192,13,208,46,144,48,96
        .byte 0,3,0,218,11,108,5,28,11,12,20,16,31,112,14,102,0,15,1,224,3,174,0,78,31,64,63,92,31,76,0,88
        .byte 0,218,22,109,10,29,22,12,40,112,62,108,28,30,0,0,3,220,6,220,1,0,125,112,253,55,125,103,1,1,28,0
        .byte 192,0,91,0,54,208,56,160,48,208,8,40,14,248,102,112,240,0,7,128,117,192,114,0,2,248,58,252,50,248,26,0
        .byte 91,0,182,104,184,80,48,104,14,20,54,124,120,56,0,0,59,192,59,96,0,128,14,190,236,191,230,190,128,128,0,56
        .byte 8,32,13,0,7,128,6,240,2,112,3,56,6,24,4,48,1,128,3,192,3,192,1,128,0,0,1,128,0,0,1,128
        .byte 0,32,2,104,0,248,0,240,3,184,3,144,6,144,4,32,1,128,3,192,3,192,1,128,0,0,1,128,0,0,1,128
        .byte 0,0,0,16,17,0,3,128,6,208,12,120,14,240,6,112,0,32,2,192,3,192,1,128,0,0,1,128,0,0,1,128
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,128,63,248
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 14,184,110,187,52,23,51,230,207,249,94,61,57,206,23,244,8,136,10,184,6,240,46,250,119,247,115,231,35,226,1,192
        .byte 0,0,57,188,12,16,115,231,238,59,25,204,183,245,16,132,10,184,14,186,78,247,231,247,230,50,66,32,2,32,1,192
        .byte 1,0,1,0,3,0,3,0,3,0,3,0,3,0,7,64,7,192,5,192,1,128,1,128,1,128,1,128,1,0,1,0
        .byte 0,128,0,128,0,192,0,192,0,192,0,192,0,192,2,224,3,224,3,160,1,128,1,128,1,128,1,128,0,128,0,128
        .byte 110,88,223,32,216,90,104,162,152,248,248,8,248,8,120,48,63,0,15,254,3,254,0,0,1,252,121,252,30,0,15,248
        .byte 0,44,110,16,223,45,216,81,104,124,152,56,248,0,255,0,127,252,25,252,0,0,1,254,57,254,94,0,15,252,7,248
        .byte 0,0,0,26,0,4,0,26,0,69,0,79,0,30,0,0,32,56,47,124,238,218,46,232,32,235,0,71,0,24,0,56
        .byte 0,26,0,4,0,26,0,197,0,207,0,16,0,16,0,14,0,48,32,124,46,249,238,219,46,227,32,236,0,92,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,127,255,127,255,127,60,60,195,195,251,247,251,247
        .byte 0,88,80,32,32,90,64,162,16,248,8,8,4,8,2,48,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 10,0,4,88,6,32,0,91,4,163,4,248,2,48,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 248,248,248,248,248,248,248,248,248,248,0,0,0,0,0,0,248,248,248,248,248,248,248,248,248,248,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,4,1,244,3,245,1,244,0,4,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,7,192,0,0,3,128,3,128,3,128,3,128,3,128,1,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,32,0,47,128,175,192,47,128,32,0,0,0,0,0,0,0,0,0,0,0
        .byte 1,0,3,128,3,128,3,128,3,128,3,128,0,0,7,192,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,6,0,6,0,6,0,14,0,6,0,6,0,14,0,14,0,28,0,12,0,24,0,56,0,56,0,112,0,32,0,192
        .byte 0,14,0,6,0,14,0,30,0,12,0,12,0,60,0,56,0,248,0,56,0,112,0,240,1,224,0,192,3,0,0,0
        .byte 0,211,0,163,2,215,2,43,0,251,1,7,0,230,0,1,1,236,3,44,7,192,15,236,47,92,39,24,24,96,0,192
        .byte 1,166,1,71,5,167,4,83,1,243,2,7,1,206,0,5,3,216,14,88,31,128,159,216,158,184,110,48,0,192,3,0
        .byte 96,0,96,0,96,0,112,0,96,0,96,0,112,0,112,0,56,0,48,0,24,0,28,0,28,0,14,0,4,0,3,0
        .byte 112,0,96,0,112,0,120,0,48,0,48,0,60,0,28,0,31,0,28,0,14,0,15,0,7,128,3,0,0,192,0,0
        .byte 13,3,10,1,77,15,66,135,31,142,16,28,14,26,0,0,31,248,30,24,29,192,29,216,31,88,15,56,6,50,0,0
        .byte 26,7,20,3,154,15,133,6,63,28,32,56,28,52,0,0,63,240,60,0,59,176,59,176,63,112,30,102,12,0,0,0
        .byte 0,0,0,0,2,0,0,0,0,0,48,24,48,24,0,0,60,120,207,230,223,246,7,192,26,176,61,120,57,56,0,192
        .byte 0,0,0,0,2,0,0,0,0,0,48,24,48,24,0,0,4,64,15,224,19,152,60,188,57,28,0,192,0,0,0,0
        .byte 0,0,0,0,0,0,2,0,0,0,0,0,48,24,48,24,0,0,4,64,23,208,47,232,50,152,44,104,14,224,14,224
        .byte 0,8,0,24,0,56,0,112,0,240,1,248,3,254,0,31,0,62,0,56,0,112,0,96,0,192,0,192,0,128,0,128
        .byte 0,0,0,0,2,0,1,0,1,128,0,192,7,224,3,0,1,128,0,128,0,64,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,128,0,128,1,0,3,192,0,192,1,128,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0
frmlst:
        .byte 0,3
        .byte 3,2
        .byte 5,2
        .byte 7,2
        .byte 9,2
        .byte 11,2
        .byte 13,2
        .byte 15,4
        .byte 19,2
        .byte 21,2
        .byte 23,2
        .byte 25,2
        .byte 27,3
        .byte 30,1
        .byte 31,1
        .byte 32,2
        .byte 34,2
        .byte 36,2
        .byte 38,2
        .byte 40,1
        .byte 41,2
        .byte 43,1
        .byte 44,1
        .byte 45,4
        .byte 49,2
        .byte 51,2
        .byte 53,2
        .byte 55,2
        .byte 57,3
        .byte 60,3,63,0
.out .sprintf("SPRITES SIZE = %d", (* - sprgfx))

scdat:
        .word 309,360,134,88,407,388,298,157,221,214,12,9,225,21,225,15,15,21,39,417
        .byte 15,27,44,0,0,0,255,34,24,15,27,44,255,0,4,255,34,12,0,0,0,255,34,5,0,0,0,15,33,44,255,0,4
        .byte 255,34,4,0,0,0,34,0,0,34,255,0,5,34,34,34,255,0,4,23,33,44,255,0,4,34,34,34,255,0,13,34,34,34
        .byte 255,0,4,17,33,44,255,0,27,14,33,44,255,0,10,255,34,5,255,0,12,18,33,44,255,0,9,255,34,7,255,0,5
        .byte 34,34,255,0,4,15,33,44,255,0,9,255,25,7,255,0,4,255,34,4,0,0,0,15,33,44,255,0,20,255,25,4,0,0,0
        .byte 15,22,44,255,0,25,23,23,15,15,44,255,0,24,35,27,35,255,23,4,27,255,0,21,23,23,33,24,14,14,14,33,3
        .byte 27,255,0,19,45,3,24,27,24,14,14,14,15,3,3,27,44,255,0,17,45,3,24,33,24,6,13,17,15,27,33,27,44
        .byte 255,0,16,45,3,17,16,27,24,6,39,15,15,22,22,33,44,255,0,16,45,17,17,30,33,24,6,39,24,15,39,0,33
        .byte 14,44,255,0,15,45,17,16,39,27,24,35,27,24,24,21,21,27,17,17,44,255,0,14,45,17,16,32,32,24,33,33,24,24,24
        .byte 40,33,17,17,44,255,0,15,17,16,32,32,24,3,15,15,15,24,24,33,3,255,24,4,255,3,5,15,14,24,24,24,3,3,3
        .byte 14,14,32,32,15
        .byte 3,27,15,14,31,22,0,0,255,34,18,14,3,3,33,33,15,14,255,0,6,34,34,34,255,0,8,255,34,7,29,33,14
        .byte 31,255,0,21,255,34,4,0,0,22,14,255,0,4,34,34,255,0,16,255,34,4,0,0,0,29,0,0,0,255,34,4,255,0,5
        .byte 3,15,14,44,255,0,13,3,0,0,0,255,25,4,255,0,4,3,3,14,18,44,255,0,11,15,15,33,255,0,8,45,17
        .byte 3,27,33,14,16,44,255,0,11,13,14,3,255,0,8,45,36,27,3,3,13,16,44,255,0,12,13,33,255,0,8,45,37
        .byte 22,33,3,6,16,44,255,0,13,3,23,3,255,0,6,45,27,14,22,33,18,14,44,0,0,1,7,1,7,1,7,255,0,5
        .byte 14,14,33,255,0,6,45,33,14,14,27,13,18,44,0,0,0,255,14,4,255,0,6,14,16,22,27,44,255,0,4,45,3
        .byte 14,15,22,6,18,44,255,0,14,14,33,3,44,255,0,4,45,33,18,16,14,18,18,44,255,0,14,13,15,33,44,255,0,4
        .byte 45,27,16,15,14,18,43,44,255,0,14,21,15,33,44,255,0,4,45,33,255,14,4,15,44,255,0,14,33,14,3,44,255,0,4
        .byte 45,3,14,18,15,14,18,44,255,0,14,3,14,16,44,255,0,4,45,16,16,0,13,18,18,44,255,0,14,14,17,17,44
        .byte 255,0,4,45,18,16,0,0,16,18,44,255,0,14,13,17,16,255,0,6,17,17,32,32,16,16,255,0,15,32,32,24,33
        .byte 3,27,255,24,4,255,32,4,24,3,33,3,33,24,24,24,32,3,33,27,24,24,3,33
        .byte 36,27,6,255,0,12,34,34,255,0,9,3,33,3,33,24,24,6,255,0,11,255,34,4,255,0,8,22,24,24,24,30,6,6
        .byte 255,0,4,39,255,0,6,255,25,4,255,0,10,13,15,6,255,0,28,15,255,0,29,15,255,0,29,15,255,0,29,15,255,54,22
        .byte 255,53,7,15,14,255,53,21,255,52,7,15,14,255,52,28,15,14,255,52,28,15,14,255,52,28,15,14,255,52,28,15,14
        .byte 255,52,4,255,54,25,14,255,52,4,255,53,25,14,255,52,29,14,255,52,29,14,255,52,29,14,255,52,29,255,54,30
        .byte 255,0,29,39,255,0,29,39,255,0,29,39,255,0,29,39,255,0,29,39,255,0,29,39,255,0,29,255,39,26,255,0,4
        .byte 39,39,255,0,28,39,39,255,0,28,39,39,255,0,28,39,39,255,0,28,39,39,255,0,28,39,39,255,0,4,255,39,26
        .byte 255,0,29,39,255,0,29,39,255,0,29,39,255,0,29,39,255,0,29,255,39,30
        .byte 15,33,37,0,0,9,8,0,0,9,8,0,0,9,8,0,0,9,8,0,0,9,8,255,0,4,35,24,24,1,37,0,0,0
        .byte 9,8,0,0,9,8,0,0,9,8,0,0,9,8,0,0,9,8,255,0,4,36,22,15,15,255,0,4,9,8,0,0,9
        .byte 8,0,0,9,8,0,0,9,8,0,0,9,8,0,34,34,0,0,0,13,7,255,0,4,9,8,0,0,9,8,0,0,9
        .byte 8,0,0,9,8,0,0,9,8,255,34,4,0,0,0,33,21,0,0,0,9,8,0,0,9,8,0,0,9,8,0,0,9
        .byte 8,0,0,9,8,255,34,4,0,0,0,27,33,0,0,0,9,8,0,0,9,8,0,0,9,8,0,0,9,8,0,0,9
        .byte 8,255,25,4,0,0,0,1,27,0,0,0,9,8,0,0,9,8,0,0,9,8,0,0,9,8,0,0,9,8,255,0,7
        .byte 15,33,0,0,0,9,8,0,0,9,8,0,0,9,8,0,0,9,8,0,0,9,8,255,0,7,15,27,0,0,0,9,8
        .byte 0,0,9,8,0,0,9,8,0,0,9,8,0,0,9,8,255,0,7,1,33,0,0,0,9,8,0,0,9,8,255,0,6
        .byte 9,8,255,0,11,7,22,0,0,0,9,8,0,0,9,8,255,0,6,9,8,255,0,11,15,255,0,4,9,8,0,0,9
        .byte 8,255,0,6,9,8,255,0,11,15,255,0,4,9,8,0,0,9,8,255,0,6,9,8,255,0,11,15,255,0,4,9,8
        .byte 0,0,9,8,255,0,6,9,8,255,0,11,15,255,0,4,9,8,0,0,9,8,255,0,6,9,8,255,0,20,9,8,255,0,6
        .byte 9,8,255,0,20,9,8,255,0,6,9,8,255,0,20,9,8,255,0,6,9,8,255,0,41,255,15,5,46,46,46,15,15,15
        .byte 255,46,15,15,15,1,15
        .byte 255,0,5,45,44,0,0,45,44,0,0,45,44,0,0,45,44,0,0,45,44,255,0,4,39,39,39,255,0,5,45,44,0,0
        .byte 45,44,0,0,45,44,0,0,45,44,0,0,45,44,255,0,4,39,39,39,255,0,5,45,44,0,0,45,44,0,0,45,44
        .byte 0,0,45,44,0,0,45,44,255,0,6,39,255,0,5,45,44,0,0,45,44,0,0,45,44,0,0,45,44,0,0,45,44
        .byte 255,0,7,39,39,0,0,0,45,44,0,0,45,44,0,0,45,44,0,0,45,44,0,0,45,44,255,0,7,39,39,0,0,0
        .byte 45,44,0,0,45,44,0,0,45,44,0,0,45,44,0,0,45,44,255,0,7,39,39,0,0,0,45,44,0,0,45,44,0,0
        .byte 45,44,0,0,45,44,0,0,45,44,255,0,7,39,39,0,0,0,45,44,0,0,45,44,0,0,45,44,0,0,45,44,0,0
        .byte 45,44,255,0,7,39,39,0,0,0,45,44,0,0,45,44,0,0,45,44,0,0,45,44,0,0,45,44,255,0,7,39,39
        .byte 0,0,0,45,44,0,0,45,44,255,0,6,45,44,255,0,11,39,39,0,0,0,45,44,0,0,45,44,255,0,6,45,44
        .byte 255,0,11,39,255,0,4,45,44,0,0,45,44,255,0,6,45,44,255,0,11,39,255,0,4,45,44,0,0,45,44,255,0,6
        .byte 45,44,255,0,11,39,255,0,4,45,44,0,0,45,44,255,0,6,45,44,255,0,11,39,255,0,4,45,44,0,0,45,44
        .byte 255,0,6,45,44,255,0,20,45,44,255,0,6,45,44,255,0,20,45,44,255,0,6,45,44,255,0,20,45,44,255,0,6
        .byte 45,44,255,0,41,255,39,5,0,0,0,39,39,39,255,0,15,255,39,4
        .byte 34,34,34,255,0,5,255,34,24,255,0,7,255,34,7,255,0,4,255,34,7,0,0,0,34,255,0,4,34,255,0,6,34,34
        .byte 255,0,8,255,34,4,255,0,8,34,34,34,255,0,10,34,34,0,0,0,255,34,4,255,0,7,255,34,5,255,0,8,255,34,4
        .byte 255,0,13,255,25,5,255,0,8,255,25,4,255,0,150,3,15,255,0,13,3,27,27,14,24,24,24,44,255,0,5,45,3
        .byte 24,15,27,44,255,0,9,45,3,3,22,27,3,27,24,24,44,255,0,5,45,24,14,15,33,44,255,0,9,45,3,14,14
        .byte 27,24,24,24,27,24,44,255,0,4,45,24,14,16,22,44,255,0,9,45,3,14,30,3,15,14,16,27,27,44,0,0,0
        .byte 45,3,17,14,27,15,27,44,255,0,8,45,33,14,39,27,17,14,16,24,27,44,0,0,0,45,33,24,17,33,15,33,44
        .byte 255,0,8,45,3,17,27,27,13,17,16,24,27,255,0,4,45,24,24,30,27,16,33,44,255,0,8,45,33,17,33,22,39
        .byte 17,17,13,27,40,255,0,4,3,17,39,22,16,27,255,0,10,33,17,22,39,39,17,17,39,40,30,6,40,40,255,24,8
        .byte 255,0,5,40,40,40,255,15,4,3,3,3,15,15,255,32,4,40,14,14,24,24,24,15,15,15,40,3,3,3,40
        .byte 23,23,23,22,22,22,255,34,19,255,0,4,3,255,0,7,255,34,12,0,0,0,34,34,255,0,5,22,255,0,7,255,34,4
        .byte 0,0,0,34,0,0,34,255,0,5,34,255,0,5,39,255,0,7,34,34,34,255,0,19,39,24,24,24,255,0,26,39,14
        .byte 30,255,0,11,34,0,34,0,34,0,34,255,0,7,24,24,24,14,255,0,27,13,15,14,255,0,10,34,0,34,0,34,0
        .byte 34,0,34,0,34,255,0,7,14,255,0,41,34,0,34,0,34,0,34,0,34,0,34,255,0,0,255,0,22,24,40,40,1
        .byte 15,15,15,255,46,4,24,24,24,40,1,7,1,255,46,4,15,15,15,46,46,1,7,1
        .byte 0,0,255,34,4,255,0,18,255,34,4,255,0,5,34,34,255,0,20,34,34,255,0,63,11,11,11,255,0,26,39,255,24,24
        .byte 0,0,0,24,24,24,15,30,22,255,0,4,9,8,255,0,9,22,22,255,0,8,13,15,14,255,0,6,9,8,255,0,20
        .byte 14,22,255,0,6,9,8,255,0,12,34,0,34,255,0,5,39,255,0,7,9,8,255,0,20,39,255,0,7,9,8,255,0,7
        .byte 11,0,0,34,0,34,0,34,0,34,0,0,0,39,255,0,7,9,8,255,0,20,39,255,0,7,9,8,255,0,10,34,0
        .byte 34,0,34,0,34,0,0,0,39,255,0,7,9,8,0,1,7,1,7,1,7,1,255,0,12,39,255,0,7,9,8,0,0
        .byte 14,14,14,24,24,255,0,13,39,255,0,7,9,8,255,0,20,39,255,0,7,9,8,255,0,20,39,255,0,7,9,8,255,0,20
        .byte 39,255,0,29,39,255,15,9,255,46,13,15,15,15,41,41,15,15,15
        .byte 255,0,32,34,0,34,255,0,55,34,0,34,0,34,0,34,255,0,45,45,255,24,7,34,0,34,0,34,0,34,255,0,15
        .byte 45,16,16,17,255,14,4,255,0,22,45,16,16,16,17,17,14,14,255,0,20,45,255,24,9,255,0,20,45,16,16,17,14,14,14
        .byte 30,13,14,255,0,20,45,16,16,16,17,14,14,39,39,14,255,0,18,45,255,24,11,255,0,18,45,16,16,255,17,4,255,14,5
        .byte 255,0,18,45,16,16,16,255,17,5,14,14,14,255,0,14,45,255,24,15,255,0,14,45,16,16,255,17,4,255,14,6,30
        .byte 13,14,255,0,14,45,16,16,16,255,17,6,14,14,14,39,39,14,255,0,10,45,255,24,19,255,0,10,45,16,16,255,17,8
        .byte 255,14,9,255,0,11,16,16,16,255,17,5,30,39,39,39,13,255,14,6,15,1,255,15,5,7,15,15,15,7,255,15,5
        .byte 1,15,255,28,5,15,15,15,1,15,15
        .byte 255,0,0,255,0,74,255,39,30,255,50,240
        .byte 255,0,210,255,34,0,255,34,134
        .byte 0,0,0,255,34,6,255,0,4,255,34,7,255,0,4,255,34,5,0,6,6,0,0,255,34,4,255,0,6,255,34,5,255,0,6
        .byte 255,34,4,0,6,255,0,4,34,34,0,0,0,34,34,0,0,0,34,34,34,255,0,8,34,34,0,0,6,0,9,8,255,0,5
        .byte 255,34,4,255,0,17,6,0,9,8,255,0,26,6,0,9,8,255,0,26,51,0,9,8,0,255,51,25,6,0,9,8,255,0,25
        .byte 6,6,0,9,8,255,0,25,6,6,0,9,8,255,0,22,9,8,0,6,6,0,9,8,255,0,22,9,8,0,6,6,0
        .byte 9,8,255,0,22,9,8,0,6,6,0,9,8,255,0,22,9,8,0,6,255,51,25,0,9,8,0,51,39,255,0,25,9
        .byte 8,0,6,39,255,0,25,9,8,0,6,39,255,0,25,9,8,0,6,39,255,0,25,9,8,0,6,39,255,0,25,9,8
        .byte 0,6,255,15,9,35,35,35,255,15,7,35,35,255,15,4,35,35,15,15,15
        .byte 255,26,30,255,55,30,255,0,150,255,57,30,255,56,210,255,0,120,255,46,30
        .byte 33,255,0,7,255,34,21,33,33,255,0,7,255,34,11,0,0,0,255,34,5,0,0,33,33,0,0,34,34,255,0,5,34
        .byte 0,0,0,34,0,0,34,255,0,5,34,34,34,0,0,0,33,33,0,255,34,4,255,0,17,34,34,34,0,0,0,33,33
        .byte 0,255,56,4,255,0,22,6,33,33,255,0,27,6,33,33,255,0,27,6,33,33,255,0,28,33,33,255,0,28,33,33,255,0,28
        .byte 33,33,255,0,28,33,22,255,0,28,22,255,0,25,45,0,0,0,42,6,255,0,6,44,255,0,10,45,255,23,11,6,255,0,17
        .byte 45,17,17,24,255,14,4,255,24,4,6,255,0,17,45,17,255,24,7,30,11,11,6,255,0,17,45,17,17,17,14,24,16
        .byte 17,17,11,11,11,23,27,44,255,0,6,44,255,0,8,45,255,17,4,16,16,16,17,11,11,11,14,33,44,255,0,15,45
        .byte 17,17,17,255,16,4,17,32,32,32,14,33,44,0,0,0,35,255,24,20,32,32,32
        .byte 255,25,60,255,34,210,255,0,0,255,0,44,255,39,30
        .byte 255,25,60,255,34,210,255,0,0,255,0,44,255,39,30
        .byte 255,25,60,255,34,163,59,59,59,255,34,14,255,0,0,255,0,74,255,39,30
        .byte 255,25,60,255,34,102,255,59,4,255,34,25,59,255,58,4,59,255,34,23,59,58,255,59,4,58,59,255,34,12,255,0,0
        .byte 255,0,74,255,39,30
        .byte 36,255,25,26,36,0,0,36,255,25,26,36,0,0,36,255,34,26,3,0,0,36,255,34,11,255,59,4,255,34,11,36,0,0
        .byte 33,255,34,10,59,255,58,4,59,255,34,10,33,0,0,33,255,34,9,59,58,59,58,58,59,58,59,255,34,9,36,0,0
        .byte 33,34,34,34,60,60,34,34,34,59,58,59,58,59,59,58,59,58,59,34,34,34,60,60,34,34,34,33,0,0,36,34,34,34
        .byte 60,60,34,34,59,58,59,255,58,6,59,58,59,34,34,60,60,34,34,34,36,0,0,36,34,63,63,63,60,34,59,58,59
        .byte 58,58,58,59,59,58,58,58,59,58,59,34,60,63,63,63,34,33,0,0,33,0,63,63,63,60,255,0,16,60,63,63,63
        .byte 0,33,0,0,33,0,63,63,63,60,255,0,16,60,63,63,63,0,36,0,0,33,0,63,63,63,255,0,6,255,53,5,255,0,7
        .byte 63,63,63,0,33,0,0,33,255,0,4,53,0,0,0,53,53,61,62,53,62,61,53,53,0,0,0,53,255,0,5,36,0,0
        .byte 36,0,0,0,52,53,0,0,63,63,53,61,0,62,0,61,53,63,63,0,0,53,52,255,0,4,33,0,0,36,0,0,0
        .byte 52,53,0,0,60,60,53,61,62,52,62,61,53,60,60,0,0,53,52,255,0,4,33,0,0,33,255,0,4,52,53,0,0
        .byte 53,53,60,53,53,53,60,53,53,0,0,53,52,255,0,5,36,0,0,33,255,0,4,52,53,0,0,53,60,60,60,63,60,60,60
        .byte 53,0,0,53,52,255,0,5,33,0,0,33,255,0,5,52,53,0,53,61,255,62,5,61,53,0,53,52,255,0,6,36,0,0
        .byte 33,255,0,5,52,53,0,0,63,255,62,5,63,0,0,53,52,255,0,6,33,0,0,36,0,255,39,4,0,52,53,0,0
        .byte 255,63,5,39,39,52,52,255,39,7,33,0,0
numsc:
        .byte 20
.out .sprintf("SCREENS SIZE = %d", (* - scdat))

nmedat:
        .byte 0,1,104,192,1,5,88,72,255
        .byte 0,1,120,32,1,6,56,176,2,11,152,176,3,13,72,80,255
        .byte 0,8,56,40,3,14,56,8,3,12,16,40,4,0,88,224,4,0,144,128,255
        .byte 0,8,48,24,3,14,56,8,4,0,88,224,4,0,144,128,255
        .byte 0,2,136,8,4,19,152,184,4,19,48,128,4,19,24,192,255
        .byte 0,0,144,16,255
        .byte 0,8,96,16,4,15,32,224,255
        .byte 0,8,152,24,3,12,56,200,2,10,152,160,2,11,152,112,255
        .byte 0,2,144,32,4,19,160,208,4,15,16,96,255
        .byte 0,2,128,8,4,15,16,216,4,15,16,88,255
        .byte 0,2,40,24,5,22,40,200,5,6,80,128,5,5,80,184,255
        .byte 5,4,56,24,5,22,40,200,4,19,24,128,4,0,96,160,4,0,136,32,255
        .byte 0,4,152,16,2,11,152,200,2,10,104,48,2,11,48,208,255
        .byte 7,26,32,32,7,26,32,160,0,0,32,32,8,13,120,24,4,15,144,16,4,15,144,216,255
        .byte 0,0,128,16,4,15,48,216,1,5,152,144,255
        .byte 8,28,160,120,3,29,88,40,3,29,112,176,255
        .byte 8,28,160,120,3,29,120,192,3,29,144,64,255
        .byte 8,28,160,120,3,29,88,32,3,29,112,176,3,29,152,208,255
        .byte 3,29,96,24,3,29,120,192,3,29,152,24,3,29,80,104,8,28,160,120,255
.out .sprintf("POSITIONS SIZE = %d", (* - nmedat))

NUMOBJ = 1
objdta:
        .byte 0,0,0,0,0,0,0,0,0,0,6,96,15,240,15,176,15,176,7,96,3,192,3,64,3,64,3,64,3,64,3,192,14,104,224,14,104,224
.out .sprintf("OBJECTS SIZE = %d", (* - objdta))

font:
        .byte 0,0,0,0,0,0,0,0
        .byte 0,112,112,48,48,0,48,48
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
        .byte 0,124,206,222,246,230,124,0
        .byte 0,56,120,88,24,24,126,0
        .byte 0,124,230,6,124,192,254,0
        .byte 0,124,230,28,22,198,124,0
        .byte 0,28,60,108,200,254,24,0
        .byte 0,254,198,248,14,230,124,0
        .byte 0,126,194,252,230,198,124,0
        .byte 0,254,198,12,24,56,56,0
        .byte 0,124,230,124,230,198,124,0
        .byte 0,124,230,198,126,6,124,0
        .byte 0,0,0,48,0,0,48,0
        .byte 0,0,48,0,0,48,48,96
        .byte 0,0,12,24,48,24,12,0
        .byte 0,0,0,126,0,126,0,0
        .byte 0,0,48,24,12,24,48,0
        .byte 0,124,198,12,24,0,24,0
        .byte 0,124,222,254,254,192,124,0
        .byte 0,124,198,230,254,230,238,0
        .byte 0,252,198,252,238,230,252,0
        .byte 0,124,246,224,224,198,124,0
        .byte 0,248,220,198,230,236,248,0
        .byte 0,254,242,248,192,198,254,0
        .byte 0,254,198,240,224,192,192,0
        .byte 0,124,238,224,206,198,124,0
        .byte 0,206,206,254,206,230,230,0
        .byte 0,126,92,28,24,24,126,0
        .byte 0,14,14,14,198,198,124,0
        .byte 0,220,220,240,216,206,206,0
        .byte 0,224,224,224,192,192,254,0
        .byte 0,198,238,254,214,206,206,0
        .byte 0,206,238,246,222,206,198,0
        .byte 0,124,238,238,198,198,124,0
        .byte 0,252,238,230,252,192,192,0
        .byte 0,124,206,206,246,222,124,0
        .byte 0,252,246,230,252,204,198,0
        .byte 0,124,240,124,14,230,124,0
        .byte 0,254,186,56,48,48,48,0
        .byte 0,238,238,230,198,198,124,0
        .byte 0,238,206,206,198,108,56,0
        .byte 0,230,230,230,198,254,108,0
        .byte 0,198,238,56,56,238,198,0
        .byte 0,206,206,120,56,48,48,0
        .byte 0,254,206,26,48,102,254,0
        .byte 0,30,24,24,24,24,30,0
        .byte 0,0,192,96,48,24,12,0
        .byte 0,240,48,48,48,48,240,0
        .byte 16,56,108,198,254,124,124,124
        .byte 0,0,0,0,0,0,0,255
        .byte 0,60,102,248,96,96,254,0
        .byte 88,40,91,163,248,8,48,0
        .byte 222,215,15,223,219,107,0,0
        .byte 0,0,128,192,192,128,72,48
        .byte 0,0,0,0,26,53,53,26
        .byte 0,56,124,110,78,93,223,15
        .byte 101,255,239,230,224,224,192,128
        .byte 0,251,251,251,0,191,191,191
        .byte 0,223,223,223,0,253,253,253
        .byte 0,127,127,127,0,247,247,247
        .byte 16,16,56,124,124,0,56,56
        .byte 64,64,224,241,241,0,224,224
        .byte 1,1,131,199,199,0,131,131
        .byte 255,127,127,126,126,60,24,0
        .byte 255,253,253,249,249,240,96,0
        .byte 255,247,247,231,231,195,129,0
        .byte 255,145,4,32,0,1,0,0
        .byte 255,70,16,128,0,4,0,0
        .byte 255,25,64,2,0,16,0,0
        .byte 227,255,255,255,255,255,255,255
        .byte 143,255,255,255,255,255,255,255
        .byte 62,255,255,255,255,255,255,255
        .byte 32,112,120,124,120,112,32,0
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

jtab:
        .byte 248,250,252,254,254,255,255,255,0,0,0,1,1,1,2,2,4,6,8,8,8,99
keys:   .byte 66,97,104,72,73,16,55,48,49,17,18
.out .sprintf("KEYS SIZE = %d", (* - keys))

data_end:
.out .sprintf("TOTAL DATA SIZE = %d", (* - data_start))
