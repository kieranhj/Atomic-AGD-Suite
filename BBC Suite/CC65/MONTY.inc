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
WINDOWHGT = 20
WINDOWWID = 30 ;面面面面面面面面面面面面
MAPWID = 10
        .byte 255,255,255,255,255,255,255,255,255,255
mapdat:
        .byte 255,255,255,255,255,255,255,255,25,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,21,20,19,18,23,3,255,255,255,255,24,16,15,17,22,2,7,255,255,255,255,11,4,5,6,1,255,255,255,12,13,10,9,8,255,0,255,255,255,255,255,14,255,255,255,255,255,255
        .byte 255,255,255,255,255,255,255,255,255,255
stmap:  .byte 67

evnt00:
        lda #6
        sta z80_b
        jsr sktyp
        bcs :+
        jmp a00028
:
        jmp a00046
a00028: jsr cangd
        beq :+
        jmp a00046
:
        jsr ifall
a00046: lda #25
        cmp scno
        beq *+5
        jmp a00068
        jmp a01583
a00068: jsr skobj
        sta varobj
        lda #255
        cmp varobj
        bne *+5
        jmp a00188
        lda varobj
        jsr getob
        lda #40
        asl a
        sta sndtyp
        lda #40
        asl a
        sta sndtyp
        lda varc
        clc
        adc #1
        sta varc
        lda #<100
        sta z80_l
        lda #>100
        sta z80_h
        jsr addsc
        lda #21
        sta chary
        lda #24
        sta charx
        jsr dscor
a00188: lda #230
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp a00258
        jsr scrr
        lda #16
        ldy #9
        sta (z80_ix),y
        dey; ldy #8
        lda (z80_ix),y
        sta varo
        iny; ldy #9
        lda (z80_ix),y
        sta varp
a00258: lda #10
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp a00328
        jsr scrl
        lda #228
        ldy #9
        sta (z80_ix),y
        dey; ldy #8
        lda (z80_ix),y
        sta varo
        iny; ldy #9
        lda (z80_ix),y
        sta varp
a00328: lda #10
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp a00398
        jsr scru
        lda #144
        ldy #8
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        sta varo
        iny; ldy #9
        lda (z80_ix),y
        sta varp
a00398: lda #150
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a00467
        jsr scrd
        lda #16
        ldy #8
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        sta varo
        iny; ldy #9
        lda (z80_ix),y
        sta varp
a00467: jsr laddd
        beq :+
        jmp a00515
:
        lda #2
        sta varl
        jsr laddu
        beq :+
        jmp a00510
:
        lda #1
        sta varl
a00510: jmp a00523
a00515: lda #0
        sta varl
a00523: lda joyval
        and #1
        beq :+
        jmp a00861
:
        jsr cangr
        beq :+
        jmp a00861
:
        lda joyval
        and #4
        beq :+
        jmp a00656
:
        jsr laddd
        beq :+
        jmp a00590
:
        jmp a00651
a00590: lda #0
        cmp vara
        beq *+5
        jmp a00616
        ; lda #0
        jsr animsp
a00616: lda #12
        ldy #6
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a00651: jmp a00861
a00656: lda joyval
        and #8
        beq :+
        jmp a00758
:
        jsr laddu
        beq :+
        jmp a00692
:
        jmp a00753
a00692: lda #12
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp a00731
        ; lda #0
        jsr animsp
a00731: ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a00753: jmp a00861
a00758: lda #1
        sta vard
        lda #12
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp a00839
        ; lda #0
        jsr animsp
        lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a00839
        lda #5
        asl a
        sta sndtyp
a00839: ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a00861: lda joyval
        and #2
        beq :+
        jmp a01199
:
        jsr cangl
        beq :+
        jmp a01199
:
        lda joyval
        and #4
        beq :+
        jmp a00994
:
        jsr laddd
        beq :+
        jmp a00928
:
        jmp a00989
a00928: lda #0
        cmp vara
        beq *+5
        jmp a00953
        ; lda #0
        jsr animsp
a00953: lda #11
        ldy #6
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a00989: jmp a01199
a00994: lda joyval
        and #8
        beq :+
        jmp a01095
:
        jsr laddu
        beq :+
        jmp a01029
:
        jmp a01091
a01029: lda #11
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp a01069
        ; lda #0
        jsr animsp
a01069: ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a01091: jmp a01199
a01095: lda #2
        sta vard
        lda #11
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp a01177
        ; lda #0
        jsr animsp
        lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a01177
        lda #5
        asl a
        sta sndtyp
a01177: ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a01199: lda joyval
        and #8
        beq :+
        jmp a01299
:
        lda #3
        sta vard
        jsr laddu
        beq :+
        jmp a01299
:
        lda #0
        ldy #6
        sta (z80_ix),y
        ; lda #0
        cmp vara
        beq *+5
        jmp a01277
        ; lda #0
        jsr animsp
a01277: ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a01299: lda joyval
        and #4
        beq :+
        jmp a01400
:
        lda #3
        sta vard
        jsr laddd
        beq :+
        jmp a01400
:
        lda #0
        ldy #6
        sta (z80_ix),y
        ; lda #0
        cmp vara
        beq *+5
        jmp a01378
        ; lda #0
        jsr animsp
a01378: ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a01400: jsr cangu
        beq :+
        jmp a01519
:
        lda joyval
        and #16
        beq :+
        jmp a01519
:
        jsr cangd
        beq :+
        jmp a01449
:
        jmp a01519
a01449: lda #0
        cmp varl
        beq *+5
        jmp a01519
        jsr jump
        lda #5
        asl a
        sta sndtyp
        lda #10
        asl a
        sta sndtyp
        lda #20
        asl a
        sta sndtyp
        lda #30
        asl a
        sta sndtyp
a01519: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01583
:
        lda varo
        ldy #8
        sta (z80_ix),y
        lda varp
        iny; ldy #9
        sta (z80_ix),y
        lda #1
        sta deadf
a01583: jmp grav
.out .sprintf("EVENT 0 SIZE = %d", (* - evnt00))

evnt01:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b00181
        jsr cangl
        beq :+
        jmp b00163
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #16
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp b00122
:
        lda #1
        ldy #11
        sta (z80_ix),y
b00122: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp b00176
b00163: lda #1
        ldy #11
        sta (z80_ix),y
b00176: jmp b00333
b00181: jsr cangr
        beq :+
        jmp b00320
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #16
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp b00279
:
        lda #0
        ldy #11
        sta (z80_ix),y
b00279: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp b00333
b00320: lda #0
        ldy #11
        sta (z80_ix),y
b00333: lda #0
        cmp vara
        beq *+5
        jmp b00359
        ; lda #0
        jsr animsp
b00359: lda #13
        cmp scno
        beq *+5
        jmp b00376
b00376: lda #10
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp b00398
b00398: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00428
:
        lda #1
        sta deadf
b00428: rts
.out .sprintf("EVENT 1 SIZE = %d", (* - evnt01))

evnt02:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp c00108
        jsr cangu
        beq :+
        jmp c00090
:
        lda #0
        cmp vara
        beq *+5
        jmp c00063
        ; lda #0
        jsr animsp
c00063: ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp c00103
c00090: lda #1
        ldy #11
        sta (z80_ix),y
c00103: jmp c00188
c00108: jsr cangd
        beq :+
        jmp c00174
:
        lda #0
        cmp vara
        beq *+5
        jmp c00147
        ; lda #0
        jsr animsp
c00147: ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp c00188
c00174: lda #0
        ldy #11
        sta (z80_ix),y
c00188: lda #0
        sta varb
        lda #13
        cmp scno
        beq *+5
        jmp c00255
        lda #26
        jsr gotob
        cmp #255
        beq :+
        jmp c00247
:
        lda #1
        sta varb
        jmp c00255
c00247: lda #0
        sta varb
c00255: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00399
:
        lda #19
        jsr gotob
        cmp #255
        beq :+
        jmp c00357
:
        lda #8
        sta z80_c
        lda #17
        sta z80_b
        jsr spawn
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
        jmp c00399
c00357: lda #1
        cmp varb
        beq *+5
        jmp c00378
        jmp c00399
c00378: lda #60
        asl a
        sta sndtyp
        lda #1
        sta deadf
c00399: lda #10
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00421
c00421: lda #7
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00465
        lda #0
        cmp varb
        beq *+5
        jmp c00465
        jmp c00465
c00465: lda #6
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00487
c00487: lda #8
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00509
c00509: lda #16
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00531
c00531: lda #2
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00553
c00553: lda #1
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00603
        ; lda #1
        iny; ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp c00603
        jmp c00603
c00603: rts
.out .sprintf("EVENT 2 SIZE = %d", (* - evnt02))

evnt03:
        ldy #6
        lda (z80_ix),y
        sta vari
        lda #11
        cmp vari
        beq *+5
        jmp d00057
        lda #12
        sta varj
        lda #11
        sta vark
        lda #71
        sta varm
d00057: lda #10
        cmp vari
        beq *+5
        jmp d00099
        lda #16
        sta varj
        lda #10
        sta vark
        lda #70
        sta varm
d00099: lda #6
        cmp vari
        beq *+5
        jmp d00140
        lda #5
        sta varj
        lda #6
        sta vark
        lda #67
        sta varm
d00140: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp d00359
        jsr cangl
        beq :+
        jmp d00340
:
        lda vark
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp d00215
        ; lda #0
        jsr animsp
d00215: ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #16
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp d00300
:
        lda #1
        ldy #11
        sta (z80_ix),y
d00300: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp d00354
d00340: lda #1
        ldy #11
        sta (z80_ix),y
d00354: jmp d00550
d00359: jsr cangr
        beq :+
        jmp d00537
:
        lda varj
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp d00412
        ; lda #0
        jsr animsp
d00412: ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #16
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp d00497
:
        lda #0
        ldy #11
        sta (z80_ix),y
d00497: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp d00550
d00537: lda #0
        ldy #11
        sta (z80_ix),y
d00550: lda #0
        jsr animsp
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00589
:
        lda #1
        sta deadf
d00589: rts
.out .sprintf("EVENT 3 SIZE = %d", (* - evnt03))

evnt04:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp e00107
        jsr cangd
        beq :+
        jmp e00064
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp e00102
e00064: lda #1
        ldy #11
        sta (z80_ix),y
        lda #10
        asl a
        sta sndtyp
        lda #10
        asl a
        sta sndtyp
e00102: jmp e00199
e00107: jsr cangu
        beq :+
        jmp e00147
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp e00199
e00147: lda #3
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00199
        lda #1
        cmp varh
        beq *+5
        jmp e00199
        lda #0
        ldy #11
        sta (z80_ix),y
e00199: lda #4
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00252
        lda #16
        cmp varh
        beq *+5
        jmp e00252
        lda #0
        ldy #11
        sta (z80_ix),y
e00252: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00282
:
        lda #1
        sta deadf
e00282: rts
.out .sprintf("EVENT 4 SIZE = %d", (* - evnt04))

evnt05:
        lda #0
        cmp vara
        beq *+5
        jmp f00027
        ; lda #0
        jsr animsp
f00027: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00196
        jsr laddu
        beq :+
        jmp f00116
:
        lda #0
        cmp vara
        beq *+5
        jmp f00089
        ; lda #0
        jsr animsp
f00089: ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp f00196
f00116: lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp f00182
        lda #2
        ldy #10
        sta (z80_ix),y
        jmp f00196
f00182: lda #3
        ldy #10
        sta (z80_ix),y
f00196: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00338
        jsr laddd
        beq :+
        jmp f00258
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp f00338
f00258: lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp f00325
        lda #2
        ldy #10
        sta (z80_ix),y
        jmp f00338
f00325: lda #3
        ldy #10
        sta (z80_ix),y
f00338: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00675
        jsr cangd
        beq :+
        jmp f00415
:
        lda #3
        ldy #10
        sta (z80_ix),y
        dey; ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp f00675
f00415: jsr cangl
        beq :+
        jmp f00554
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #16
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp f00514
:
        lda #3
        ldy #10
        sta (z80_ix),y
f00514: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp f00568
f00554: lda #3
        ldy #10
        sta (z80_ix),y
f00568: lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp f00675
        jsr laddu
        beq :+
        jmp f00648
:
        lda #0
        ldy #10
        sta (z80_ix),y
        jmp f00675
f00648: jsr laddd
        beq :+
        jmp f00675
:
        lda #1
        ldy #10
        sta (z80_ix),y
f00675: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f01011
        jsr cangd
        beq :+
        jmp f00752
:
        lda #2
        ldy #10
        sta (z80_ix),y
        dey; ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp f01011
f00752: jsr cangr
        beq :+
        jmp f00891
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #16
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp f00850
:
        lda #2
        ldy #10
        sta (z80_ix),y
f00850: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp f00904
f00891: lda #2
        ldy #10
        sta (z80_ix),y
f00904: lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp f01011
        jsr laddu
        beq :+
        jmp f00984
:
        lda #0
        ldy #10
        sta (z80_ix),y
        jmp f01011
f00984: jsr laddd
        beq :+
        jmp f01011
:
        lda #1
        ldy #10
        sta (z80_ix),y
f01011: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f01041
:
        lda #1
        sta deadf
f01041: rts
.out .sprintf("EVENT 5 SIZE = %d", (* - evnt05))

evnt06:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp g00199
        jsr cangl
        beq :+
        jmp g00181
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #16
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp g00127
:
        jmp g00141
g00127: lda #1
        ldy #11
        sta (z80_ix),y
g00141: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp g00195
g00181: lda #1
        ldy #11
        sta (z80_ix),y
g00195: jmp g00370
g00199: jsr cangr
        beq :+
        jmp g00357
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #16
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp g00303
:
        jmp g00316
g00303: lda #0
        ldy #11
        sta (z80_ix),y
g00316: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp g00370
g00357: lda #0
        ldy #11
        sta (z80_ix),y
g00370: lda #22
        cmp scno
        beq *+5
        jmp g00422
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00422
:
        lda #25
        sta scno
        jsr nwscr
g00422: rts
.out .sprintf("EVENT 6 SIZE = %d", (* - evnt06))

evnt07:
        lda #22
        cmp scno
        beq *+5
        jmp h00070
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp h00070
:
        lda #28
        cmp varc
        beq *+5
        jmp h00070
        lda #25
        sta scno
        jsr nwscr
h00070: rts
.out .sprintf("EVENT 7 SIZE = %d", (* - evnt07))

evnt08:
        lda #9
        cmp scno
        beq *+5
        jmp i00158
        lda #2
        jsr gotob
        cmp #255
        beq :+
        jmp i00158
:
        lda #0
        cmp varg
        beq *+5
        jmp i00158
        ; lda #0
        cmp vara
        beq *+5
        jmp i00158
        ; lda #0
        jsr animsp
        lda #6
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp i00158
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #20
        asl a
        sta sndtyp
        lda #20
        asl a
        sta sndtyp
        lda varg
        clc
        adc #1
        sta varg
i00158: lda #11
        cmp scno
        beq *+5
        jmp i00316
        lda #12
        jsr gotob
        cmp #255
        beq :+
        jmp i00316
:
        lda #0
        cmp varn
        beq *+5
        jmp i00316
        ; lda #0
        cmp vara
        beq *+5
        jmp i00316
        ; lda #0
        jsr animsp
        lda #6
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp i00316
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #20
        asl a
        sta sndtyp
        lda #20
        asl a
        sta sndtyp
        lda varn
        clc
        adc #1
        sta varn
i00316: lda #22
        cmp scno
        beq *+5
        jmp i00419
        lda #20
        asl a
        sta sndtyp
        lda #20
        asl a
        sta sndtyp
        lda #0
        cmp vara
        beq *+5
        jmp i00419
        ; lda #0
        jsr animsp
        lda #6
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp i00419
        lda #255
        ldy #5
        sta (z80_ix),y
i00419: lda #23
        cmp scno
        beq *+5
        jmp i00522
        lda #20
        asl a
        sta sndtyp
        lda #20
        asl a
        sta sndtyp
        lda #0
        cmp vara
        beq *+5
        jmp i00522
        ; lda #0
        jsr animsp
        lda #6
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp i00522
        lda #255
        ldy #5
        sta (z80_ix),y
i00522: rts
.out .sprintf("EVENT 8 SIZE = %d", (* - evnt08))

evnt09:
        lda #4
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00037
        lda #1
        ldy #11
        sta (z80_ix),y
j00037: lda #0
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00115
        lda varo
        ldy #8
        sta (z80_ix),y
        lda varp
        iny; ldy #9
        sta (z80_ix),y
        dey; ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
j00115: rts
.out .sprintf("EVENT 9 SIZE = %d", (* - evnt09))

evnt10:
        lda #0
        cmp scno
        beq *+5
        jmp k00043
        lda #19
        sta chary
        lda #0
        sta charx
        lda #41
        jsr dmsg
k00043: lda #1
        cmp vara
        beq *+5
        jmp k00073
        lda #0
        sta vara
        jmp k00088
k00073: lda vara
        clc
        adc #1
        sta vara
k00088: lda #11
        cmp scno
        beq *+5
        jmp k00261
        lda #12
        jsr gotob
        cmp #255
        beq :+
        jmp k00132
:
        jmp k00261
k00132: lda #1
        sta chary
        lda #7
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #26
        jsr pattr
        lda #1
        sta chary
        lda #5
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #26
        jsr pattr
        lda #1
        sta chary
        lda #6
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #26
        jsr pattr
k00261: lda #17
        cmp scno
        beq *+5
        jmp k00434
        lda #18
        jsr gotob
        cmp #255
        beq :+
        jmp k00304
:
        jmp k00434
k00304: lda #16
        sta chary
        lda #30
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda #15
        sta chary
        lda #30
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda #14
        sta chary
        lda #30
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
k00434: lda #9
        cmp scno
        beq *+5
        jmp k00607
        lda #2
        jsr gotob
        cmp #255
        beq :+
        jmp k00477
:
        jmp k00607
k00477: lda #19
        sta chary
        lda #30
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #2
        jsr pattr
        lda #18
        sta chary
        lda #30
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #2
        jsr pattr
        lda #29
        sta charx
        lda #19
        sta chary
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #2
        jsr pattr
k00607: lda #25
        cmp scno
        beq *+5
        jmp k00691
        lda #0
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp k00674
        lda #160
        ldy #8
        sta (z80_ix),y
        lda #116
        iny; ldy #9
        sta (z80_ix),y
k00674: lda #0
        jsr animsp
        lda #1
        sta gamwon
k00691: lda #50
        cmp varh
        beq *+5
        jmp k00721
        lda #0
        sta varh
        jmp k00736
k00721: lda varh
        clc
        adc #1
        sta varh
k00736: rts
.out .sprintf("EVENT 10 SIZE = %d", (* - evnt10))

evnt11:
        lda #0
        cmp scno
        beq *+5
        jmp l00148
        lda #1
        sta chary
        lda #11
        sta charx
        lda #5
        jsr dmsg
        lda #3
        sta chary
        lda #7
        sta charx
        lda #6
        jsr dmsg
        lda #22
        sta chary
        lda #1
        sta charx
        lda #32
        jsr dmsg
        lda joyval
        and #32
        beq :+
        jmp l00119
:
        jsr scru
        jsr cls
l00119: lda #20
        sta chary
        lda #8
        sta charx
        lda #33
        jsr dmsg
        jmp l00411
l00148: lda #25
        cmp scno
        beq *+5
        jmp l00314
        lda #1
        sta chary
        lda #6
        sta charx
        lda #34
        jsr dmsg
        lda #3
        sta chary
        lda #1
        sta charx
        lda #35
        jsr dmsg
        lda #4
        sta chary
        lda #3
        sta charx
        lda #36
        jsr dmsg
        lda #5
        sta chary
        lda #0
        sta charx
        lda #37
        jsr dmsg
        lda #6
        sta chary
        lda #1
        sta charx
        lda #38
        jsr dmsg
        lda #7
        sta chary
        lda #4
        sta charx
        lda #39
        jsr dmsg
l00314: lda #21
        sta chary
        lda #2
        sta charx
        lda #1
        jsr dmsg
        lda #21
        sta chary
        lda #8
        sta charx
        lda numlif
        jsr disply
        lda #21
        sta chary
        lda #18
        sta charx
        lda #2
        jsr dmsg
        lda #21
        sta chary
        lda #24
        sta charx
        jsr dscor
l00411: lda #22
        cmp scno
        beq *+5
        jmp l00453
        lda #18
        sta charx
        lda #2
        sta chary
        lda #11
        jsr dmsg
l00453: lda #22
        sta chary
        lda #0
        sta charx
        lda #1
        cmp scno
        beq *+5
        jmp l00495
        lda #10
        jsr dmsg
l00495: lda #2
        cmp scno
        beq *+5
        jmp l00520
        lda #12
        jsr dmsg
l00520: lda #3
        cmp scno
        beq *+5
        jmp l00545
        lda #14
        jsr dmsg
l00545: lda #4
        cmp scno
        beq *+5
        jmp l00570
        lda #15
        jsr dmsg
l00570: lda #5
        cmp scno
        beq *+5
        jmp l00595
        lda #16
        jsr dmsg
l00595: lda #6
        cmp scno
        beq *+5
        jmp l00620
        lda #17
        jsr dmsg
l00620: lda #7
        cmp scno
        beq *+5
        jmp l00645
        lda #18
        jsr dmsg
l00645: lda #8
        cmp scno
        beq *+5
        jmp l00670
        lda #19
        jsr dmsg
l00670: lda #9
        cmp scno
        beq *+5
        jmp l00695
        lda #20
        jsr dmsg
l00695: lda #10
        cmp scno
        beq *+5
        jmp l00720
        lda #21
        jsr dmsg
l00720: lda #11
        cmp scno
        beq *+5
        jmp l00746
        lda #22
        jsr dmsg
l00746: lda #12
        cmp scno
        beq *+5
        jmp l00771
        lda #23
        jsr dmsg
l00771: lda #13
        cmp scno
        beq *+5
        jmp l00796
        lda #23
        jsr dmsg
l00796: lda #14
        cmp scno
        beq *+5
        jmp l00821
        lda #24
        jsr dmsg
l00821: lda #15
        cmp scno
        beq *+5
        jmp l00847
        lda #25
        jsr dmsg
l00847: lda #16
        cmp scno
        beq *+5
        jmp l00872
        lda #26
        jsr dmsg
l00872: lda #17
        cmp scno
        beq *+5
        jmp l00897
        lda #27
        jsr dmsg
l00897: lda #18
        cmp scno
        beq *+5
        jmp l00922
        lda #3
        jsr dmsg
l00922: lda #19
        cmp scno
        beq *+5
        jmp l00947
        lda #4
        jsr dmsg
l00947: lda #20
        cmp scno
        beq *+5
        jmp l00972
        lda #28
        jsr dmsg
l00972: lda #21
        cmp scno
        beq *+5
        jmp l00998
        lda #29
        jsr dmsg
l00998: lda #22
        cmp scno
        beq *+5
        jmp l01023
        lda #30
        jsr dmsg
l01023: lda #23
        cmp scno
        beq *+5
        jmp l01048
        lda #31
        jsr dmsg
l01048: lda #9
        cmp scno
        beq *+5
        jmp l01216
        lda #0
        jsr gotob
        cmp #255
        beq :+
        jmp l01216
:
        lda #8
        sta chary
        lda #11
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #39
        jsr pattr
        lda #8
        sta chary
        lda #12
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #39
        jsr pattr
        lda #8
        sta chary
        lda #13
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #39
        jsr pattr
l01216: rts
.out .sprintf("EVENT 11 SIZE = %d", (* - evnt11))

evnt12:
        jsr cls
        lda #3
        sta chary
        lda #0
        sta charx
        lda #42
        jsr dmsg
        lda #5
        sta chary
        lda #0
        sta charx
        lda #40
        jsr dmsg
        lda #16
        sta chary
        lda #10
        sta charx
        lda #7
        jsr dmsg
        lda #17
        sta chary
        lda #10
        sta charx
        lda #8
        jsr dmsg
        lda #18
        sta chary
        lda #10
        sta charx
        lda #9
        jsr dmsg
        lda #14
        sta chary
        lda #0
        sta charx
        lda #44
        jsr dmsg
        lda #20
        sta chary
        lda #0
        sta charx
        lda #43
        jsr dmsg
        jsr prskey
        rts
.out .sprintf("EVENT 12 SIZE = %d", (* - evnt12))

evnt13:
        jsr cls
        lda #0
        sta varc
        ; lda #0
        sta varg
        lda #1
        sta vard
        lda #0
        sta varm
        ; lda #0
        sta varn
        lda #112
        sta varo
        lda #20
        sta varp
        lda #0
        sta varh
        lda #6
        sta numlif
        rts
.out .sprintf("EVENT 13 SIZE = %d", (* - evnt13))

evnt14:
        rts
.out .sprintf("EVENT 14 SIZE = %d", (* - evnt14))

evnt15:
        lda #1
        sta deadf
        lda varo
        ldy #8
        sta (z80_ix),y
        lda varp
        iny; ldy #9
        sta (z80_ix),y
        rts
.out .sprintf("EVENT 15 SIZE = %d", (* - evnt15))

evnt16:
        lda #50
        asl a
        sta sndtyp
        lda #40
        asl a
        sta sndtyp
        lda numlif
        sec
        sbc #1
        sta numlif
        lda #21
        sta chary
        lda #8
        sta charx
        lda numlif
        jsr disply
        lda varo
        ldy #8
        sta (z80_ix),y
        lda varp
        iny; ldy #9
        sta (z80_ix),y
        rts
.out .sprintf("EVENT 16 SIZE = %d", (* - evnt16))

evnt17:
        lda #50
        jsr delay
        jsr cls
        lda #10
        sta chary
        lda #11
        sta charx
        lda #0
        jsr dmsg
        lda #100
        jsr delay
        rts
.out .sprintf("EVENT 17 SIZE = %d", (* - evnt17))

evnt18:
        lda #200
        jsr delay
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
        .byte "=========Into the canopy========",141
        .byte "======When The Bough Breaks=====",13
        .byte 141
        .byte "MONTY MOLE",141
        .byte "MONTYS HONEY RUN !",141
        .byte "1 KEYBOARD",141
        .byte "2 KEMPSTON",141
        .byte "3 SINCLAIR",141
        .byte "=========The Bat Cave===========",141
        .byte "HONEY SHOP",141
        .byte "===========The Ascent===========",13
        .byte 141
        .byte "=======The End Of The Line======",13
        .byte 141
        .byte "=========Climb That Tree========",13
        .byte 141
        .byte "=======The Mine Entrance========",13
        .byte 141
        .byte "====Ore Processing Facility=====",13
        .byte 141
        .byte "=====The Underground Jungle=====",13
        .byte 13
        .byte 141
        .byte "======A Sting In The Tail=======",13
        .byte 141
        .byte "===========Piper Maru===========",13
        .byte 141
        .byte "======The Gemstone Cavern=======",13
        .byte 141
        .byte "=======The Shining Spires=======",13
        .byte 141
        .byte "==========The Way Out?==========",141
        .byte "====The Crystaline Enitities====",13
        .byte 141
        .byte "========The Crystal Maze========",141
        .byte "=Between A Rock And A Hard Place",13
        .byte 13
        .byte 141
        .byte "========Daylight At Last========",13
        .byte 141
        .byte "===========It Goes Up===========",13
        .byte 141
        .byte "=========Into the Hive==========",141
        .byte "============The Hive============",13
        .byte 141
        .byte "=========Honey Im Home==========",13
        .byte 141
        .byte "=========The Lost Attic=========",13
        .byte 141
        .byte "==Thanks to GREMLIN GRAPHICS==",13
        .byte 13
        .byte 141
        .byte "Press S to start",141
        .byte "CONGRATULATIONS!!!!",141
        .byte "YOU HAVE SUCCEEDED IN FINDING",141
        .byte "ALL OF MONTYS HONEY POTS",141
        .byte "NOW IF ONLY HE COULD SELL THEM",141
        .byte "HE COULD AFFORD THAT HOLIDAY",141
        .byte "TO VISIT THE PYRAMIDS!!!",141
        .byte 13
        .byte "=Monty has bought a honey shop.=",13
        .byte "=Everything was going well until",13
        .byte "==he was attacked by the Bees.==",13
        .byte 13
        .byte "=Help Monty find all the stolen=",13
        .byte "=Honey Pots",141
        .byte "===An AGD Game By Andy Johns",141
        .byte "========Montys Honey Run",141
        .byte "=======Keys: QAOP Space",141
        .byte "=======Control Options",141
nummsg:
        .byte 45
.out .sprintf("MESSAGE SIZE = %d", (* - msgdat))

chgfx:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 160,208,176,112,96,160,160,208,80,112,176,192,240,112,160,208
        .byte 0,32,112,112,240,160,112,128,16,96,96,112,112,112,96,32
        .byte 1,5,2,10,37,165,90,90,133,133,10,10,5,5,16,16
        .byte 1,4,2,8,165,164,90,90,133,133,10,10,5,4,16,0
        .byte 176,160,96,176,0,224,80,240,0,32,16,64,0,48,0,16
        .byte 176,160,224,160,160,64,208,0,160,128,144,0,64,0,128,0
        .byte 128,0,208,0,128,128,240,0,176,64,224,128,176,160,160,176
        .byte 0,0,0,48,0,160,16,112,48,160,64,160,80,80,160,176
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 0,0,0,0,0,0,2,0,1,1,72,10,32,160,16,72
        .byte 36,0,18,0,0,0,18,0,33,0,0,0,33,0,66,0
        .byte 0,36,0,72,0,0,0,72,0,132,0,0,0,132,0,66
        .byte 0,0,0,0,0,36,16,24,33,165,64,66,0,0,0,0
        .byte 0,0,82,0,129,128,90,72,36,32,0,0,0,0,0,0
        .byte 211,124,165,250,120,241,235,175,245,94,242,188,241,120,240,240
        .byte 180,165,165,180,75,225,90,180,45,165,15,165,15,90,15,30
        .byte 180,165,120,210,165,165,210,75,165,15,90,15,165,15,135,15
        .byte 0,0,90,0,133,0,66,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,90,0,133,0,66,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,16,0,16,32,64,160,32,192,176,208,112,224
        .byte 32,160,88,88,133,133,88,16,160,161,66,88,36,160,2,2
        .byte 32,0,82,66,37,37,2,18,128,160,88,72,5,132,0,8
        .byte 15,30,15,15,15,90,15,180,45,105,15,150,90,105,180,210
        .byte 135,15,75,15,135,15,210,15,165,135,90,135,225,165,90,210
        .byte 19,76,5,170,8,17,171,175,85,78,34,140,17,8,0,0
        .byte 1,4,2,8,165,164,90,90,133,133,10,10,5,5,16,16
        .byte 1,5,2,10,37,165,90,90,133,133,10,10,5,5,16,16
        .byte 36,0,18,0,0,0,18,0,33,0,0,0,33,0,66,0
        .byte 0,36,0,72,0,0,0,72,0,132,0,0,0,132,0,66
        .byte 10,0,0,0,93,0,141,0,93,0,141,0,0,0,170,0
        .byte 0,87,0,0,0,95,0,175,0,95,0,175,0,0,0,39
        .byte 119,255,119,255,119,255,0,0,255,119,255,119,255,119,0,0
        .byte 112,224,112,240,48,240,0,0,192,240,224,240,224,112,0,0
        .byte 0,0,0,0,82,74,133,5,82,74,132,33,16,8,1,128
        .byte 0,0,0,0,66,0,165,37,8,90,160,1,16,64,1,4
        .byte 128,64,48,112,96,48,128,192,16,112,48,0,224,112,128,192
        .byte 245,245,0,0,213,177,50,200,49,196,0,0,0,0,0,0
        .byte 245,245,0,0,213,177,0,0,0,0,0,0,0,0,0,0
        .byte 25,19,34,137,68,68,137,34,68,68,34,137,25,19,140,38
        .byte 17,0,0,0,17,136,34,136,34,136,51,0,0,0,17,0
        .byte 17,0,0,0,17,136,34,136,34,136,51,0,0,0,17,0
        .byte 4,4,2,8,4,128,2,64,32,4,64,8,4,160,0,64
        .byte 4,4,2,2,4,32,2,2,32,4,64,64,4,32,64,64
        .byte 0,87,0,0,0,95,0,175,0,95,0,175,0,0,0,39
        .byte 10,0,0,0,93,0,141,0,93,0,141,0,0,0,170,0
        .byte 0,87,0,39,0,87,0,39,0,87,0,39,0,87,0,39
        .byte 10,0,170,0,10,0,170,0,10,0,170,0,10,0,170,0
        .byte 87,95,39,175,87,95,0,0,95,87,175,39,95,87,0,0
        .byte 19,76,171,141,27,93,171,141,19,76,136,1,19,76,0,0
        .byte 0,0,175,175,95,95,175,175,0,0,175,175,0,0,0,0
        .byte 19,76,171,141,27,93,171,141,19,76,136,1,19,76,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 74,74,164,164,74,74,0,0,26,26,161,161,26,26,0,0
        .byte 90,74,165,132,90,10,165,4,74,10,133,4,10,10,0,0
        .byte 74,74,132,132,10,10,0,0,74,74,132,132,10,10,0,0
        .byte 72,72,132,132,2,2,0,0,72,72,132,132,2,2,0,0
        .byte 76,70,137,34,2,8,5,4,2,8,137,34,76,70,0,0
        .byte 10,0,170,0,10,0,170,0,10,0,170,0,10,0,170,0
        .byte 0,87,0,39,0,87,0,39,0,87,0,39,0,87,0,39
        .byte 176,80,112,160,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 144,192,240,224,208,160,0,0,64,176,224,240,160,96,0,0
        .byte 90,90,128,128,80,80,32,32,80,80,128,128,90,90,0,0
        .byte 26,88,128,128,16,80,32,32,80,16,128,128,88,88,0,0
        .byte 90,90,128,128,80,80,32,32,80,80,128,128,90,90,0,0
        .byte 0,136,17,68,17,68,221,119,187,85,85,102,34,204,85,170
        .byte 1,4,82,74,165,37,10,10,133,133,82,82,32,33,0,2
        .byte 0,64,32,64,64,128,64,0,128,64,0,128,0,128,16,32
        .byte 119,238,221,119,119,238,221,119,238,238,221,85,238,187,85,238
        .byte 161,36,66,88,0,4,0,0,0,0,0,0,0,0,0,0
        .byte 161,5,66,74,32,4,2,0,0,4,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 17,0,16,0,17,0,16,0,17,128,50,136,49,128,16,0
        .byte 128,17,64,16,68,32,50,34,68,196,64,34,128,32,136,16
        .byte 117,228,186,216,196,49,250,250,160,160,80,80,32,32,0,0
        .byte 117,228,186,200,196,49,250,250,160,160,80,80,32,32,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 117,196,50,136,49,128,16,0,17,0,16,0,17,0,16,0
        .byte 17,0,16,0,49,128,50,136,117,68,114,64,32,128,16,0
        .byte 0,0,250,250,68,32,250,250,160,160,80,80,32,32,0,0
        .byte 18,8,32,128,18,8,32,128,18,8,32,128,18,8,32,128
        .byte 17,95,174,174,95,17,175,175,10,10,5,5,10,10,5,5
        .byte 0,0,0,0,0,0,0,0,255,0,119,204,119,255,51,238
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 32,68,64,34,160,85,16,136,17,128,170,80,68,32,34,64
        .byte 128,17,64,16,68,32,50,34,0,196,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,50,0,68,196,64,34,128,32,136,16
        .byte 128,17,64,16,68,32,50,34,68,196,64,34,128,32,136,16
        .byte 165,165,0,0,165,165,24,24,165,165,0,0,165,165,0,0
        .byte 255,255,153,34,170,68,221,68,136,136,187,68,204,0,136,0
        .byte 255,255,68,153,34,85,34,187,17,17,34,221,0,51,0,17
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 95,87,136,136,85,85,34,34,93,93,0,0,0,0,0,0
        .byte 2,2,5,0,8,8,5,0,2,2,0,5,8,8,0,5
        .byte 95,87,136,136,85,85,34,34,93,93,0,0,17,8,0,136
        .byte 17,8,34,136,2,8,1,0,0,8,0,4,17,68,1,136
        .byte 17,8,34,136,2,8,1,0,0,8,0,4,17,68,1,136
        .byte 2,2,5,0,8,8,5,0,2,0,0,0,8,0,0,0
        .byte 0,0,0,0,8,0,4,0,2,0,0,4,8,8,0,4
        .byte 0,0,0,0,0,0,0,0,0,2,0,5,0,8,0,5
        .byte 2,10,4,4,2,8,1,0,0,2,0,5,0,2,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,32,0,80,16,240,176
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 17,8,1,136,17,8,1,136,17,8,1,136,17,8,1,136
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,17,95,34,136
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,95,8,1,4
        .byte 213,0,0,136,0,68,16,64,245,68,250,64,0,68,250,136
        .byte 0,177,16,0,32,68,34,250,32,245,34,250,32,0,16,250
        .byte 159,47,79,31,15,143,15,15,15,15,15,15,15,15,15,15
bprop:
        .byte 0
        .byte 2
        .byte 2
        .byte 1
        .byte 1
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 7
        .byte 0
        .byte 3
        .byte 3
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 3
        .byte 3
        .byte 5
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 0
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 0
        .byte 0
        .byte 2
        .byte 1
        .byte 1
        .byte 2
        .byte 0
        .byte 3
        .byte 0
        .byte 0
        .byte 3
        .byte 3
        .byte 3
        .byte 3
        .byte 2
        .byte 3
        .byte 1
        .byte 1
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 3
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 1
        .byte 1
        .byte 3
        .byte 5
        .byte 1
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 6
        .byte 0
        .byte 3
        .byte 1
        .byte 2
        .byte 6
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 1
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 1
        .byte 0
        .byte 3
        .byte 0
        .byte 3
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
        .byte 7
.out .sprintf("BLOCKS SIZE = %d", (* - chgfx))

sprgfx:
        .byte 0,0,3,192,7,224,15,240,15,244,7,236,31,252,63,248,63,248,47,252,31,252,63,252,63,252,63,248,30,48,12,0
        .byte 0,0,3,192,7,224,15,240,15,240,7,236,31,252,63,252,63,252,15,240,31,248,63,252,63,252,63,252,31,248,12,48
        .byte 0,0,3,192,7,224,15,240,47,240,55,224,63,248,31,252,31,252,63,244,63,248,63,252,63,252,31,252,12,120,0,48
        .byte 0,0,3,192,7,224,15,240,15,240,55,224,63,248,63,252,63,252,15,240,31,248,63,252,63,252,63,252,31,248,12,48
        .byte 0,0,1,0,1,0,16,16,11,160,4,64,9,32,107,172,9,32,4,64,11,160,16,16,1,0,1,0,0,0,0,0
        .byte 0,0,0,0,32,8,16,16,3,128,4,64,9,32,10,160,9,32,4,64,3,128,16,16,32,8,0,0,0,0,0,0
        .byte 0,0,0,0,1,0,1,0,11,160,4,64,8,32,57,56,8,32,4,64,11,160,1,0,1,0,0,0,0,0,0,0
        .byte 240,192,137,32,133,32,66,196,50,8,12,28,0,210,3,82,5,78,13,36,20,144,18,96,25,136,62,36,0,144,0,64
        .byte 0,0,24,192,37,32,66,196,50,8,12,28,0,210,3,82,5,78,13,36,20,144,18,96,25,136,30,32,32,128,1,0
        .byte 0,0,0,0,1,224,14,196,18,8,60,28,0,210,3,82,5,78,13,36,20,144,18,96,25,140,30,48,16,192,0,0
        .byte 5,224,5,224,5,224,5,224,5,224,5,224,5,224,5,224,5,224,5,224,0,0,47,252,80,2,191,255,191,255,191,255
        .byte 5,224,5,224,5,224,5,224,5,224,5,224,5,224,5,224,5,224,5,224,0,0,47,252,80,2,191,255,191,255,191,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,14,238,237,110,239,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,192,130,238,238,45,108,15,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,134,142,238,237,104,111,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,44,14,238,237,66,239,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,112,1,183,119,247,118,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,119,65,180,119,240,54,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,113,97,183,119,246,22,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,112,52,183,119,247,66,0,0
        .byte 0,0,0,0,0,0,224,7,240,15,248,31,56,28,58,92,13,176,6,96,3,192,2,64,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,128,1,224,7,248,31,250,95,61,188,14,112,3,192,2,64,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,242,79,253,191,62,124,11,208,2,64,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,64,61,188,126,126,243,207,194,67,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,242,79,253,191,62,124,11,208,2,64,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,255,255,128,1,128,1,255,255,0,0,127,254,127,254,191,252,128,0,180,52,44,44,24,24
        .byte 0,0,0,0,0,0,0,0,255,255,128,1,128,1,255,255,0,0,127,254,127,254,191,252,128,0,172,44,52,52,24,24
        .byte 0,0,1,128,2,64,3,192,4,32,7,224,4,32,3,192,0,0,19,200,37,164,37,164,74,82,81,138,84,42,16,8
        .byte 0,0,0,0,1,128,2,64,3,192,4,32,7,224,4,32,3,192,32,4,75,210,149,169,165,165,162,69,145,137,68,34
        .byte 0,0,0,0,0,0,1,128,2,64,3,192,4,32,7,224,4,32,35,196,72,18,83,202,149,169,165,165,34,68,9,144
        .byte 0,0,0,0,1,128,2,64,3,192,4,32,7,224,4,32,3,192,32,4,75,210,149,169,165,165,162,69,161,133,128,1
        .byte 16,96,8,144,120,136,200,136,218,144,115,96,7,128,15,128,23,32,32,80,4,184,8,104,0,24,0,4,0,4,0,0
        .byte 4,0,8,0,120,14,200,49,218,193,115,1,7,230,15,152,23,32,8,80,4,184,2,104,0,24,0,4,0,4,0,0
        .byte 8,30,8,34,120,66,200,132,218,136,115,112,7,128,15,128,23,32,16,80,4,184,4,104,0,24,0,4,0,4,0,0
        .byte 0,0,7,128,9,192,27,224,127,208,55,56,0,216,11,92,27,92,22,252,14,220,15,60,7,248,1,248,0,240,7,224
        .byte 0,0,7,128,9,192,27,224,127,208,51,56,0,220,27,108,59,110,55,182,15,182,15,204,7,222,55,190,31,28,14,120
        .byte 0,0,7,128,9,192,27,224,127,208,51,56,0,216,11,92,27,92,22,252,14,220,15,60,7,248,1,248,0,240,7,224
        .byte 0,0,7,128,9,192,27,224,127,208,55,56,0,220,3,92,2,222,5,254,13,190,14,124,7,126,55,190,31,28,14,120
        .byte 0,0,1,224,3,144,7,216,11,254,28,236,27,0,58,208,58,216,63,104,59,112,60,240,31,224,31,128,15,0,7,224
        .byte 0,0,1,224,3,144,7,216,11,254,28,204,59,0,54,216,118,220,109,236,109,240,51,240,123,224,125,236,56,248,30,112
        .byte 0,0,1,224,3,144,7,216,11,254,28,236,27,0,58,208,58,216,63,104,59,112,60,240,31,224,31,128,15,0,7,224
        .byte 0,0,1,224,3,144,7,216,11,254,28,236,59,0,58,192,123,64,127,160,125,176,62,112,126,224,125,236,56,248,30,112
        .byte 0,128,0,0,1,128,2,64,4,32,8,16,22,200,164,132,32,5,17,136,8,16,4,32,2,64,1,128,0,0,1,0
        .byte 0,0,0,0,9,144,18,72,36,36,8,16,22,200,36,132,32,4,17,136,8,16,36,36,18,72,9,144,0,0,0,0
        .byte 1,0,32,4,65,130,2,64,4,32,8,16,19,104,33,36,160,5,17,136,8,16,4,32,2,64,65,130,32,4,0,128
        .byte 128,1,0,0,1,128,2,64,4,32,8,16,19,104,33,36,32,4,17,136,8,16,4,32,2,64,1,128,0,0,128,1
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,255,170,85,255,255,0,0,91,218,104,22,48,12,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,255,170,85,255,255,0,0,107,214,88,26,48,12,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 6,8,9,16,17,30,17,19,9,91,6,206,1,224,1,240,4,232,10,4,29,32,22,16,24,0,32,0,32,0,0,0
        .byte 0,32,0,16,112,30,140,19,131,91,128,206,103,224,25,240,4,232,10,16,29,32,22,64,24,0,32,0,32,0,0,0
        .byte 120,16,68,16,66,30,33,19,17,91,14,206,1,224,1,240,4,232,10,8,29,32,22,32,24,0,32,0,32,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,1,64,3,128,6,224,5,64,3,192,0,128,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,7,48,13,144,24,72,19,168,21,216,22,0,27,208,6,176,2,96,1,128,0,0,0,0,0,0,0,0
        .byte 3,128,15,160,58,232,46,184,93,92,51,170,93,218,85,150,91,84,38,180,62,104,21,156,10,120,3,160,0,0,0,0
        .byte 2,0,7,128,10,224,44,176,92,92,112,42,24,24,16,22,88,16,38,52,30,104,21,148,2,96,1,0,0,0,0,0
        .byte 0,0,5,0,0,128,4,32,80,4,0,40,16,0,0,20,64,0,0,16,8,64,17,0,2,64,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
frmlst:
        .byte 0,4
        .byte 4,3
        .byte 7,3
        .byte 10,1
        .byte 11,1
        .byte 12,4
        .byte 16,4
        .byte 20,5
        .byte 25,2
        .byte 27,4
        .byte 31,3
        .byte 34,4
        .byte 38,4
        .byte 42,4
        .byte 46,2
        .byte 48,1
        .byte 49,3
        .byte 52,8,60,0
.out .sprintf("SPRITES SIZE = %d", (* - sprgfx))

scdat:
        .word 221,454,479,434,345,320,429,348,220,356,394,367,368,318,473,339,303,402,424,370,400,367,336,328,362,197
        .byte 255,0,163,74,74,74,75,255,0,25,86,87,0,0,87,86,255,0,23,74,74,74,0,0,74,74,74,255,0,11,255,52,4
        .byte 255,0,6,86,87,85,77,0,0,78,85,87,86,255,0,10,255,52,5,255,0,4,74,75,75,0,78,255,0,4,75,75,74
        .byte 255,0,9,255,52,5,0,0,0,86,87,83,83,255,0,6,83,83,87,86,255,0,8,255,52,5,0,0,74,74,74,83,83
        .byte 255,0,6,83,83,74,74,74,255,0,7,255,52,5,0,86,87,87,78,83,83,0,0,105,106,0,0,83,83,0,87,87,86
        .byte 0,92,255,0,4,255,52,5,0,74,74,74,0,83,83,0,0,108,107,0,0,83,83,0,74,74,74,255,0,6,255,52,5
        .byte 0,85,87,87,86,52,52,255,0,6,52,52,86,87,87,85,255,0,6,255,52,5,0,0,74,74,74,255,0,10,74,74,74
        .byte 255,0,14,72,85,87,255,0,10,87,78,85,255,0,16,85,255,74,10,85,255,0,69
        .byte 1,6,0,0,11,12,0,5,1,1,36,15,36,2,36,2,36,36,2,2,36,1,2,1,1,6,11,12,0,1,6,0,0,0
        .byte 11,12,0,0,0,5,1,36,36,255,2,4,15,36,36,1,2,1,1,6,13,11,12,0,1,255,0,4,11,12,255,0,4
        .byte 5,1,36,2,2,36,2,36,36,36,1,1,1,6,0,0,11,12,14,2,0,0,10,0,18,19,255,0,4,12,5,1,36
        .byte 255,1,5,6,5,6,255,0,4,11,12,0,1,255,3,7,4,0,0,12,0,5,1,6,0,0,5,6,255,0,7,11,12
        .byte 0,255,1,8,6,0,0,12,0,0,42,255,0,12,11,12,0,1,1,1,6,5,6,28,29,255,0,18,13,11,12,0,2
        .byte 1,6,0,0,0,28,29,255,0,19,11,12,0,1,2,255,0,4,11,12,14,0,0,10,255,0,15,11,12,14,1,1,0,0,0
        .byte 13,11,12,0,0,0,3,3,4,255,0,12,13,11,12,0,2,1,255,0,4,11,12,0,0,0,5,1,6,255,0,13,11
        .byte 12,0,1,2,255,0,4,11,12,14,0,0,0,1,255,0,13,10,18,19,10,1,1,255,0,4,11,12,255,0,4,2,0,0
        .byte 3,3,3,4,255,0,6,255,3,4,4,1,2,255,0,4,11,12,255,0,4,1,0,0,5,2,1,6,255,0,6,5,255,1,6
        .byte 0,0,0,10,11,12,255,0,4,5,7,0,0,1,6,0,0,10,255,0,6,5,2,2,2,1,3,3,3,4,4,4,255,0,5
        .byte 1,7,8,6,0,0,0,3,3,3,4,255,0,4,1,2,15,1,1,2,1,2,1,6,255,0,4,8,1,1,6,255,0,4
        .byte 5,1,1,6,255,0,4,5,1,2,15,1,17,109,9,109,9,109,109,9,109,16,1,1,9,109,9,109,109,9,1,1,109
        .byte 9,109,109,9,9,1,2,36,1,24,9,9,23,1,1,24,9,9,23,1,2,24,255,9,4,23,2,1,24,255,9,4,23
        .byte 1,36,36,36,255,1,10,2,2,255,1,7,2,255,1,8
        .byte 1,0,0,0,11,12,0,8,255,1,11,6,11,12,5,255,1,8,0,0,0,11,12,14,1,1,1,6,255,0,6,43,0,0
        .byte 11,12,0,5,1,1,6,43,5,1,1,7,0,0,11,12,0,1,1,6,0,0,35,255,0,4,42,0,13,11,12,0,0
        .byte 28,29,14,43,0,1,1,1,0,0,11,12,14,5,1,255,3,4,4,255,0,6,18,19,0,0,28,29,0,42,0,1,1
        .byte 6,0,13,11,12,0,0,255,1,5,6,255,0,6,3,3,4,0,11,12,0,0,8,1,1,0,0,0,11,12,0,0,1,1
        .byte 6,0,12,255,0,7,5,1,6,0,11,12,0,0,1,2,1,0,0,13,11,12,0,0,1,1,0,0,12,255,0,10,13
        .byte 11,12,0,0,5,1,1,7,0,0,11,12,0,0,1,6,0,0,12,255,0,11,11,12,14,0,0,0,1,6,0,0,11
        .byte 12,14,8,1,92,92,255,0,13,11,12,255,0,4,1,92,92,0,11,12,0,1,1,83,83,10,255,0,12,18,19,0,0
        .byte 10,0,1,83,83,0,11,12,0,5,1,3,3,3,4,255,0,9,255,3,8,1,7,0,0,11,12,0,0,1,1,1,6
        .byte 255,0,6,10,0,0,0,5,255,1,9,7,0,11,12,0,0,1,6,255,0,6,3,3,4,255,0,4,5,6,13,28,29
        .byte 5,1,1,1,6,13,11,12,14,0,1,92,92,255,0,5,5,1,6,255,0,7,28,29,3,1,1,1,0,0,11,12,14
        .byte 8,1,83,83,10,255,0,14,28,29,5,1,1,6,0,0,11,12,0,1,1,255,3,5,4,255,0,5,255,3,4,4,13
        .byte 11,12,0,1,1,0,0,0,11,12,0,255,1,6,17,109,9,109,109,9,109,16,1,1,1,7,0,11,12,14,1,1,7
        .byte 0,13,11,12,0,1,36,2,1,15,1,255,9,8,2,36,2,1,0,11,12,0,1,1,1,7,0,11,12,14,1,36,2,2
        .byte 36,1,2,255,9,5,2,2,2,15,1,1,13,11,12,0,1,1,1,6,0,11,12,0,5,1,36,2,2,36,1,2,2
        .byte 1,1,2,36,2,1,36,1,6,0,11,12,14,1
        .byte 255,1,31,21,22,21,21,22,21,21,22,22,255,21,4,22,22,22,21,22,255,21,5,255,0,5,1,1,0,21,21,43,22,22
        .byte 21,22,21,21,22,21,22,21,22,21,21,22,21,22,43,21,255,0,6,1,1,22,0,0,43,0,0,43,21,22,21,22,21
        .byte 42,0,0,0,42,0,42,0,42,255,0,7,1,1,0,0,0,42,4,4,42,43,0,42,0,42,42,255,3,7,4,4,255,0,6
        .byte 1,1,0,0,0,3,3,3,4,43,0,3,255,4,4,8,255,1,4,6,255,0,6,10,0,1,1,0,0,0,43,5,1
        .byte 7,42,21,21,3,4,8,1,6,21,0,0,43,255,0,6,3,3,3,1,1,0,0,0,43,0,5,255,1,7,6,0,43
        .byte 0,0,43,255,0,7,5,1,1,1,4,4,0,42,0,0,43,21,21,21,5,1,1,0,0,42,0,0,43,255,0,8,69
        .byte 1,1,6,255,0,5,43,0,21,21,21,1,1,255,0,5,43,3,4,4,255,0,6,1,1,255,0,6,42,0,0,43,0
        .byte 1,1,255,0,5,43,5,1,6,255,0,6,1,1,255,0,4,10,255,0,4,43,0,5,1,3,3,4,0,0,42,255,0,9
        .byte 1,1,0,0,0,3,3,3,4,0,0,42,0,0,1,1,6,255,0,6,10,255,0,6,1,1,255,0,4,5,6,255,0,4
        .byte 92,92,1,1,255,0,4,3,3,3,4,4,255,0,5,1,1,255,0,10,83,83,1,1,255,0,4,5,1,1,1,6,255,0,5
        .byte 1,1,255,0,10,3,4,1,1,7,255,0,4,69,70,255,0,5,3,4,1,1,3,4,255,0,9,5,1,36,1,255,0,11
        .byte 5,1,1,1,6,52,255,0,9,8,1,36,1,255,0,12,10,1,1,3,3,27,11,12,26,255,3,4,4,1,36,36,1
        .byte 3,3,3,27,11,12,26,255,3,5,4,255,1,5,11,12,255,1,7,36,36,36,255,1,4,11,12,255,1,8
        .byte 255,36,14,255,1,16,255,0,12,36,36,1,36,67,255,0,25,36,36,1,67,255,0,26,36,1,1,255,0,11,37,38,37
        .byte 38,36,36,93,93,255,0,6,34,0,36,1,67,0,0,0,255,93,8,36,1,1,1,2,36,255,0,8,36,36,36,1,255,0,12
        .byte 67,1,1,1,2,36,255,0,5,93,93,93,36,36,2,1,93,93,255,0,11,67,1,1,2,36,255,0,8,67,36,2,1
        .byte 255,0,14,36,1,2,36,93,93,255,0,7,36,1,1,255,0,14,36,1,2,36,255,0,9,36,1,1,38,37,38,37,38
        .byte 37,38,37,38,37,38,41,41,37,36,1,2,36,0,0,35,255,0,6,36,2,1,67,255,0,10,41,41,0,67,1,2,255,36,8
        .byte 41,41,36,2,1,1,35,255,0,9,41,41,0,0,1,36,36,36,255,0,6,41,41,36,36,2,1,36,255,0,9,41,41
        .byte 0,0,1,36,89,255,0,7,41,41,67,36,2,1,1,255,0,9,41,41,0,0,1,36,255,0,8,41,41,0,36,2,1,1
        .byte 38,41,41,37,38,37,38,37,38,37,38,37,38,1,36,255,0,8,41,41,0,67,2,2,1,67,41,41,255,0,10,1,36
        .byte 255,0,8,41,41,0,0,36,2,1,36,41,41,255,0,10,1,36,255,0,8,41,41,0,0,36,2,1,1,41,41,255,0,10
        .byte 1,36,38,37,0,0,0,38,37,38,37,38,37,38,67,2,2,1,41,41,255,0,10,1,36,36,36,44,45,0,255,36,9
        .byte 2,1,255,93,9,0,0,0,1
        .byte 255,33,6,61,255,33,5,61,255,33,13,61,33,33,33,255,0,29,33,255,0,29,61,38,37,38,37,38,37,38,37,38,38,38
        .byte 37,38,37,38,37,38,37,38,37,38,38,38,37,38,37,38,38,37,33,33,54,255,55,5,54,0,0,0,54,255,55,7,54
        .byte 92,92,0,54,55,54,41,41,54,33,61,89,255,0,5,55,0,0,0,55,255,0,6,90,55,83,83,0,55,0,0,41,41
        .byte 90,33,33,255,0,6,54,0,0,0,54,255,0,7,54,55,55,55,54,0,0,41,41,0,33,33,255,0,25,41,41,0,33,33
        .byte 255,0,25,41,41,0,61,61,49,49,50,51,50,51,50,51,50,51,50,51,50,49,49,50,51,50,51,50,51,50,51,50,51
        .byte 50,51,50,33,33,44,45,255,0,11,44,45,255,0,13,33,33,46,47,255,0,11,46,47,255,0,11,92,92,33,33,44,45
        .byte 255,0,11,44,45,255,0,11,83,83,33,61,46,47,255,0,11,46,47,255,0,11,54,54,33,33,44,45,255,54,26,33,33
        .byte 46,47,255,55,5,61,61,55,55,55,61,61,55,55,61,61,55,55,55,61,61,255,55,5,61,33,44,45,54,33,61,61,54
        .byte 61,61,54,55,54,61,61,54,54,61,61,54,55,54,61,61,54,33,33,61,54,33,33,46,47,255,0,27,61,44,45,255,0,27
        .byte 255,33,9,61,255,33,5,61,33,33,61,33,33,61,255,33,5,61,33,33
        .byte 36,36,36,2,2,255,1,10,2,36,36,36,2,255,1,10,36,36,2,1,1,6,69,69,42,70,28,29,5,255,1,8,6
        .byte 29,0,70,42,70,69,0,0,36,2,1,6,43,0,0,92,92,0,28,29,0,69,43,69,43,29,70,28,69,28,29,14,255,0,6
        .byte 2,1,6,0,43,0,35,83,83,0,28,29,0,13,43,0,43,29,14,28,0,28,29,0,0,0,34,0,0,0,1,1,0,0
        .byte 42,0,3,3,3,4,11,12,0,0,42,0,43,0,0,28,0,11,12,0,0,255,3,5,1,6,255,0,4,5,1,1,6
        .byte 11,12,14,0,0,13,43,0,0,28,0,11,12,14,0,0,5,255,1,4,3,3,3,26,0,0,69,69,13,11,12,255,0,4
        .byte 43,14,0,28,0,11,12,255,0,4,5,255,1,5,6,255,0,6,11,12,255,0,4,42,0,0,28,14,11,12,255,0,5
        .byte 5,1,1,1,6,255,0,7,11,12,14,255,0,5,13,28,0,11,12,14,255,0,5,1,1,1,255,0,7,13,11,12,255,0,7
        .byte 28,0,11,12,255,0,6,1,1,6,255,0,8,18,19,255,0,8,13,11,12,255,0,5,8,1,1,255,0,20,11,12,255,0,5
        .byte 5,1,1,255,0,20,18,19,255,0,6,1,1,255,0,6,255,3,9,4,0,0,0,3,3,3,4,255,0,4,8,1,1
        .byte 255,0,7,5,255,1,6,6,255,0,5,5,6,255,0,5,1,1,1,255,0,5,10,0,8,1,36,2,36,1,6,255,0,12
        .byte 8,1,2,6,0,0,0,3,27,3,4,1,36,2,36,1,6,255,0,12,8,1,1,2,255,0,5,5,1,1,36,2,2
        .byte 36,1,255,0,10,8,255,1,4,2,36,255,0,5,8,1,36,255,2,4,36,24,109,109,23,1,1,24,109,109,23,1,2,2
        .byte 1,2,36,36,255,1,6,36,36,255,2,4,36,255,1,10,2,36,36,2,36,36,36
        .byte 1,22,21,22,21,22,21,22,22,255,0,8,22,21,36,36,1,36,21,22,21,22,21,22,36,1,21,22,21,21,21,22,0
        .byte 21,22,255,0,6,21,52,52,21,22,21,22,36,36,22,21,22,21,36,1,22,21,21,22,29,22,0,29,0,22,255,0,5
        .byte 22,0,0,22,0,21,22,22,21,36,22,21,22,36,1,21,22,21,22,29,0,0,29,255,0,7,52,0,0,52,52,22,0
        .byte 21,22,1,36,22,21,36,1,21,22,29,0,29,255,0,15,255,52,4,21,36,1,36,36,1,22,0,29,255,0,22,22,21
        .byte 1,36,1,0,0,29,255,0,24,21,1,6,0,0,29,255,0,25,1,255,0,6,20,255,0,4,10,255,0,17,1,255,0,4
        .byte 27,3,3,27,3,3,26,255,3,4,4,255,0,11,10,0,1,27,3,3,4,8,255,1,10,6,255,0,10,27,3,4,255,1,5
        .byte 6,42,69,0,28,29,0,42,69,69,42,255,0,6,3,3,3,4,0,0,5,1,1,1,70,69,70,255,0,4,28,29,14
        .byte 255,0,17,5,1,1,0,0,10,0,0,0,13,11,12,0,0,0,10,255,0,4,3,3,4,255,0,8,1,1,255,3,6
        .byte 26,11,12,27,255,3,5,4,255,0,12,255,1,6,6,60,60,11,12,60,60,60,5,1,6,60,255,0,12,1,1,70,69
        .byte 70,69,0,0,0,11,12,255,0,17,92,92,1,1,255,0,4,10,0,0,18,19,0,10,255,0,4,10,0,0,0,10,255,0,4
        .byte 10,0,83,83,1,1,255,3,27,4,255,1,31
        .byte 54,255,55,28,54,55,255,0,28,55,55,255,0,26,92,92,55,55,255,0,26,92,92,55,55,0,0,0,51,50,50,51,50,50
        .byte 51,50,50,51,50,50,51,50,50,51,50,50,51,50,50,51,0,54,55,54,55,255,0,28,55,54,55,54,255,0,26,55,55
        .byte 57,55,255,0,26,55,54,55,54,0,51,50,50,51,50,50,51,50,50,51,50,50,51,50,50,51,50,50,51,50,50,51,0,0,0
        .byte 55,55,255,0,28,55,55,255,0,26,54,55,54,55,255,0,26,55,57,55,55,0,0,0,51,50,50,51,50,50,51,50,50
        .byte 51,50,50,128,50,50,51,50,50,51,50,50,51,0,54,55,54,55,255,0,28,55,54,55,54,255,0,26,55,55,57,55,255,0,26
        .byte 55,54,55,54,50,51,50,50,51,50,50,51,50,50,51,50,50,51,50,50,51,50,50,51,50,50,51,0,0,0,55,255,0,27
        .byte 54,55,54,255,0,27,55,57,55,54,255,55,26,54,55,54
        .byte 54,55,54,46,47,0,54,255,55,9,36,36,36,67,67,36,55,55,55,54,0,0,0,54,55,55,98,44,45,255,0,10,56,56
        .byte 36,36,36,67,36,36,59,58,255,0,4,55,55,98,0,46,47,255,0,11,56,40,67,255,36,4,59,58,255,0,4,55,55
        .byte 0,0,44,45,255,0,11,40,40,0,36,36,36,67,46,47,255,0,4,55,55,0,0,46,47,255,0,11,40,40,0,67,36,36
        .byte 0,44,45,0,93,93,93,55,54,255,93,6,255,0,9,41,41,0,67,36,36,0,46,47,255,0,4,55,55,98,101,94,94,94
        .byte 98,255,0,9,41,41,0,0,36,67,0,44,45,255,0,4,55,55,0,0,0,101,98,255,0,10,41,41,0,0,36,67,0
        .byte 46,47,255,0,4,55,55,92,92,255,0,13,41,41,0,0,67,0,0,44,45,255,0,4,55,55,83,83,255,0,13,41,41
        .byte 255,0,5,46,47,0,0,0,100,55,54,55,55,255,0,13,41,41,255,0,5,44,45,0,0,100,94,55,255,0,16,40,255,0,9
        .byte 100,94,54,54,255,0,25,100,94,94,55,57,54,93,93,93,255,0,6,93,93,93,0,0,0,51,50,51,50,51,50,51,50
        .byte 51,50,54,55,54,57,55,55,55,54,255,0,14,101,94,94,94,98,0,101,94,55,57,57,57,255,55,4,93,93,93,255,0,13
        .byte 101,98,0,0,0,101,55,255,57,6,55,55,55,54,255,0,19,54,55,55,54,255,57,6,55,255,0,23,55,255,57,5,55
        .byte 54,54,255,0,13,65,255,0,7,54,55,55,55,57,57,55,55,255,54,22
        .byte 54,255,55,28,54,94,94,94,98,0,0,0,101,94,94,25,98,255,0,5,101,94,99,255,0,8,101,55,94,25,98,255,0,5
        .byte 101,94,98,255,0,7,101,25,94,255,0,6,92,92,55,94,94,94,99,255,0,4,100,94,255,0,9,101,94,99,255,0,5
        .byte 83,83,55,54,94,94,98,0,0,0,100,94,98,255,0,10,94,94,255,0,4,93,93,93,54,55,94,94,255,0,5,94,25
        .byte 99,255,0,7,100,100,94,25,255,0,7,55,55,94,25,99,255,0,5,94,94,99,255,0,6,100,94,94,94,99,255,0,6
        .byte 55,54,95,95,93,255,0,4,255,93,5,255,0,4,255,93,6,255,0,5,100,55,55,97,97,255,0,6,94,94,98,255,0,6
        .byte 101,25,94,94,255,0,6,54,54,55,97,97,255,0,7,94,255,0,8,101,94,98,255,0,6,55,57,55,97,97,255,0,7
        .byte 94,94,255,0,7,100,94,255,0,7,54,54,55,97,97,255,0,8,94,255,0,6,100,94,255,0,10,55,97,97,255,0,5
        .byte 100,94,94,94,99,255,0,4,100,94,94,94,25,99,255,0,7,54,95,95,93,255,0,4,255,93,5,255,0,4,255,93,6
        .byte 0,0,0,54,55,55,54,55,97,97,255,0,6,94,94,25,255,0,6,101,94,25,94,98,0,0,0,55,96,96,55,55,97,97
        .byte 255,0,7,94,94,255,0,7,101,94,98,255,0,4,55,96,96,55,55,97,97,255,0,6,100,94,255,0,8,100,94,255,0,5
        .byte 54,96,96,55,55,97,97,255,0,5,94,25,94,255,0,7,100,94,94,99,255,0,5,96,96,55,55,97,97,255,0,4,65
        .byte 255,94,4,65,255,0,4,65,94,25,94,94,65,255,0,4,96,96,55,54,255,55,25,54,97,97,54
        .byte 61,33,33,61,0,0,0,33,61,33,33,61,2,36,36,2,2,2,255,36,12,53,89,69,70,255,0,4,69,70,70,69,40,40
        .byte 36,36,2,2,36,67,59,58,0,40,40,69,70,69,69,0,53,0,92,255,0,9,40,40,67,36,2,36,67,0,31,30,0
        .byte 40,40,255,0,5,53,0,92,255,0,9,41,41,0,36,36,36,67,0,59,58,0,40,40,255,0,5,54,255,62,10,54,41,41
        .byte 0,53,36,36,0,0,31,30,0,41,41,255,36,5,49,50,49,50,49,50,49,50,49,50,49,54,41,41,0,53,53,67,0,0
        .byte 59,58,0,41,41,0,5,1,2,2,51,50,49,50,51,50,51,50,51,50,51,54,41,41,0,53,53,0,92,92,31,30,0
        .byte 41,41,0,0,1,1,2,54,255,62,10,54,41,41,0,53,53,0,83,83,59,58,0,41,41,0,0,5,1,1,53,40,40
        .byte 0,255,71,8,41,41,0,53,53,50,49,50,49,30,255,0,6,5,1,53,40,40,0,255,71,8,41,41,0,53,53,0,0,0
        .byte 59,58,255,0,7,2,53,41,41,62,255,71,8,62,62,63,53,53,0,0,0,49,51,50,51,50,51,0,0,0,1,53,41,41
        .byte 0,255,71,8,255,0,17,2,53,41,41,255,0,26,1,53,41,41,255,0,15,51,50,51,50,51,50,51,50,51,0,0,1
        .byte 53,41,41,255,0,26,2,53,41,41,255,0,26,1,53,50,51,255,0,24,51,50,1,53,52,52,255,0,24,52,52,2,53
        .byte 38,37,38,37,38,37,38,37,38,37,38,37,38,37,38,37,38,37,38,37,38,37,38,37,38,37,38,37,1,255,64,30
        .byte 54,255,55,28,54,55,101,94,94,99,100,94,25,94,98,255,0,8,101,94,94,25,98,255,0,7,55,0,101,101,25,94,94
        .byte 98,255,0,11,101,94,94,99,255,0,7,55,0,0,101,94,98,255,0,14,101,94,94,99,255,0,6,39,93,100,94,94,255,0,15
        .byte 255,39,9,54,55,0,101,94,255,0,16,39,0,101,94,94,94,98,0,0,55,55,99,100,94,255,0,16,39,92,92,0,101
        .byte 98,0,0,100,55,55,25,94,0,93,95,95,255,93,13,39,83,83,255,0,4,100,94,55,55,98,98,0,0,97,97,255,0,5
        .byte 101,25,94,98,0,0,0,101,255,39,4,93,93,95,95,93,39,55,255,0,4,97,97,255,0,13,39,25,94,98,0,0,97,97
        .byte 0,55,55,255,0,4,97,97,255,0,13,39,94,98,0,0,0,97,97,0,55,55,255,0,4,97,97,255,0,12,100,39,94
        .byte 25,99,0,0,97,97,0,55,39,255,93,13,95,95,255,93,4,39,0,101,94,99,0,97,97,0,55,55,94,98,0,0,0
        .byte 101,98,255,0,6,97,97,255,0,4,39,0,0,101,94,99,97,97,0,55,55,94,255,0,12,97,97,0,0,0,100,39,0,0,0
        .byte 94,98,97,97,0,55,55,94,255,0,12,97,97,255,0,4,39,0,0,100,94,99,97,97,0,55,55,94,99,0,95,95,255,93,15
        .byte 0,100,94,54,255,55,4,54,55,25,94,99,97,97,255,0,9,101,94,98,0,101,25,99,101,94,25,98,0,0,103,55,55
        .byte 94,94,94,97,97,255,0,7,100,94,25,94,99,0,0,101,94,94,99,0,0,0,92,92,55,54,255,55,28,54
        .byte 57,255,39,28,57,57,0,0,101,94,25,94,98,255,0,14,101,94,94,98,98,0,0,57,0,0,0,101,101,94,94,255,0,15
        .byte 101,25,98,98,255,0,9,101,94,99,255,0,15,101,255,0,6,57,93,95,95,0,0,25,98,93,95,95,255,93,11,255,0,4
        .byte 95,95,93,57,55,98,97,97,0,100,94,0,0,97,97,101,98,0,0,101,25,94,98,255,0,7,97,97,101,55,55,0,97,97
        .byte 255,0,5,97,97,255,0,15,97,97,0,55,55,0,97,97,255,0,5,97,97,255,0,15,97,97,0,55,57,255,93,5,95,95
        .byte 255,93,11,95,95,255,93,8,57,55,98,0,0,0,101,97,97,98,255,0,4,101,94,98,101,94,98,97,97,255,0,5,101
        .byte 98,101,55,55,255,0,5,97,97,255,0,11,97,97,255,0,8,55,55,255,0,5,97,97,255,0,11,97,97,255,0,8,55
        .byte 57,255,93,8,95,95,255,93,18,57,55,101,101,25,94,94,94,98,0,97,97,255,0,10,101,101,255,94,4,25,98,55,55
        .byte 92,92,101,101,25,98,0,0,97,97,255,0,12,101,101,25,94,92,92,55,55,83,83,0,0,101,98,0,0,97,97,255,0,4
        .byte 65,255,0,9,101,98,83,83,55,57,93,93,255,0,5,255,93,11,95,95,93,255,0,5,93,93,57,55,255,0,18,97,97
        .byte 255,0,8,55,55,255,0,18,97,97,255,0,8,255,55,31
        .byte 54,255,55,25,54,97,97,54,55,255,0,25,55,97,97,55,55,255,0,25,55,97,97,55,55,95,95,54,255,55,12,54,95,95
        .byte 54,95,95,54,55,55,55,54,97,97,55,55,97,97,55,94,98,0,0,0,94,99,0,0,0,101,94,55,97,97,55,97,97
        .byte 55,101,94,25,98,97,97,55,55,97,97,55,98,0,0,100,94,25,94,94,99,0,0,101,55,97,97,55,97,97,55,0,101
        .byte 98,0,97,97,55,55,97,97,55,95,95,54,255,55,6,54,95,95,55,97,97,55,97,97,55,95,95,54,55,55,54,54,55
        .byte 97,97,55,97,97,55,92,92,255,0,4,55,97,97,55,97,97,55,97,97,55,97,97,55,51,50,50,51,55,97,97,55,97,97
        .byte 55,83,83,255,0,4,55,97,97,55,97,97,55,97,97,55,97,97,55,51,50,50,51,55,97,97,55,97,97,54,55,54,255,0,4
        .byte 55,97,97,55,97,97,55,97,97,55,97,97,54,55,55,54,54,55,97,97,55,97,97,55,25,98,0,0,0,100,55,97,97
        .byte 55,97,97,55,97,97,55,97,97,0,100,99,0,55,55,97,97,55,97,97,55,98,0,0,0,100,94,55,97,97,55,97,97
        .byte 55,97,97,55,97,97,100,25,94,99,55,55,97,97,55,97,97,55,255,0,4,54,55,54,97,97,55,97,97,55,97,97,54
        .byte 55,55,55,54,95,95,55,55,97,97,55,97,97,54,55,54,0,94,55,0,0,97,97,55,97,97,55,97,97,99,0,101,98
        .byte 55,97,97,55,55,97,97,55,97,97,0,0,0,100,94,55,0,0,97,97,55,97,97,55,97,97,25,0,92,92,55,97,97
        .byte 55,55,97,97,55,97,97,0,100,94,25,94,55,0,103,97,97,55,97,97,55,97,97,94,99,83,83,55,97,97,55,55,97,97
        .byte 54,55,55,54,0,101,98,54,54,255,55,4,54,97,97,54,255,55,6,54,97,97,55,55,97,97,255,0,13,55,97,97,0
        .byte 100,94,99,0,100,94,99,97,97,55,55,97,97,255,0,13,55,97,97,100,94,94,98,100,94,94,25,97,97,55,54,255,55,15
        .byte 54,255,55,12,54
        .byte 255,0,14,22,21,22,21,22,21,22,21,36,22,21,22,21,36,21,22,255,0,13,22,21,21,22,21,36,36,21,22,67,36
        .byte 22,21,36,21,22,21,255,0,15,22,21,36,22,21,36,36,21,22,36,36,67,36,21,22,255,0,15,28,29,29,21,22,22,22
        .byte 21,52,22,52,22,21,22,21,255,0,15,28,0,29,0,52,52,21,36,67,0,36,67,0,21,22,255,0,17,29,0,0,0
        .byte 22,67,255,36,4,67,0,21,0,0,10,255,0,20,29,67,67,67,36,36,36,27,27,27,26,255,0,19,29,0,28,0,67
        .byte 36,36,1,1,6,255,0,20,29,0,28,0,0,67,67,2,1,1,3,3,4,255,0,17,29,255,0,5,67,2,2,1,36
        .byte 67,70,255,0,23,67,36,36,2,1,255,36,6,255,0,19,67,36,36,2,2,1,36,67,69,70,69,255,0,19,67,36,36
        .byte 15,2,36,67,255,0,4,66,66,66,11,12,66,66,0,36,36,36,255,0,7,67,67,67,36,36,2,36,70,255,0,7,11
        .byte 12,14,0,0,69,5,2,36,255,0,5,67,67,67,69,67,36,36,36,255,0,7,13,11,12,255,0,4,92,92,2,2,3
        .byte 4,0,0,27,27,26,0,69,67,36,36,67,92,92,255,0,5,18,19,255,0,4,83,83,255,2,4,0,0,0,5,1,0,0,0
        .byte 36,2,36,83,83,255,0,12,255,36,4,2,2,0,0,67,36,0,0,0,67,36,2,36,36,255,0,12,67,36,36,15,36,36
        .byte 67,0,255,36,32
        .byte 255,0,201,10,255,0,4,10,255,0,20,10,0,27,255,3,9,4,255,0,16,27,3,4,8,255,1,10,0,10,255,0,7
        .byte 10,0,0,0,27,3,4,8,1,1,1,255,2,10,3,27,4,255,0,5,27,3,3,3,4,8,1,1,1,255,2,4,36,36
        .byte 2,2,36,2,36,15,2,1,6,255,0,4,3,4,8,255,1,5,255,2,5,255,36,8,67,36,36,2,255,0,6,5,1
        .byte 36,36,255,2,4,15,2,36,36,36,67,67,12,67,67,11,12,14,67,36,2,10,255,0,6,67,36,36,36,255,2,4,36,36
        .byte 67,67,0,0,12,14,69,11,12,0,69,67,36,66,66,66,255,0,4,70,0,67,36,36,36,2,2,36,11,0,12,0,0
        .byte 12,0,13,11,0,0,0,70,1,1,6,255,0,8,36,36,36,2,36,89,11,0,12,14,0,12,0,0,11,255,0,4,36
        .byte 255,0,7,10,0,0,36,2,36,36,36,13,11,0,12,0,0,12,14,0,11,0,0,67,0,36,67,255,0,5,66,66,66
        .byte 255,36,4,15,36,0,0,0,12,14,0,12,0,0,11,0,67,67,0,2,36,255,0,6,67,67,36,36,2,36,36,36,67
        .byte 0,0,12,255,0,5,67,67,67,0,67,2,1,36,36,0,0,0,255,36,4,2,2,36,36,36,67,255,0,6,67,67,0
        .byte 67,0,0,0
        .byte 21,22,21,22,21,21,22,21,22,21,0,0,0,11,12,255,0,4,21,22,21,36,21,22,36,67,21,22,21,22,21,22,36,36
        .byte 22,21,36,21,0,21,0,13,11,12,255,0,4,22,21,22,67,36,21,22,36,36,67,22,21,43,21,22,21,22,22,21,36
        .byte 21,22,0,0,11,12,14,0,0,21,0,22,21,22,67,36,21,22,70,36,67,22,43,36,21,22,21,36,22,21,43,21,22
        .byte 0,11,12,255,0,6,21,22,21,67,36,36,22,21,36,67,42,67,42,21,22,0,0,0,43,22,21,13,11,12,255,0,6
        .byte 21,0,22,21,22,67,36,67,36,67,0,67,36,0,21,0,0,0,43,0,22,0,11,12,255,0,5,52,52,52,21,0,0
        .byte 22,21,67,36,67,36,36,89,0,43,0,0,0,42,0,0,0,11,12,14,255,0,12,67,36,36,67,70,0,0,43,255,0,7
        .byte 11,12,0,20,255,0,11,67,67,67,70,0,0,0,42,255,0,6,255,66,6,255,0,10,67,67,67,255,0,12,70,69,70
        .byte 69,255,0,11,67,36,67,67,255,0,22,92,92,0,0,67,36,67,67,255,0,6,66,66,66,255,0,6,10,255,0,6,83,83
        .byte 0,0,255,67,4,255,0,7,70,255,0,7,66,66,66,255,0,4,66,66,0,0,67,36,67,67,255,0,11,10,255,0,4
        .byte 70,255,0,9,67,0,67,67,67,10,255,0,8,255,66,4,255,0,12,67,0,255,3,4,4,255,0,8,70,69,255,0,4
        .byte 10,255,0,8,67,0,1,1,1,6,255,0,14,255,66,4,255,0,5,8,1,1,2,2,1,1,1,7,255,0,19,8,255,1,4
        .byte 36,2,2,255,1,4,24,9,109,9,109,9,109,109,9,9,109,9,109,109,9,9,109,23,255,1,5,36,36,2,36,255,1,26
        .byte 22,22,21,22,21,67,66,67,21,22,21,22,21,22,21,255,26,6,27,27,26,27,27,36,67,26,5,0,21,22,21,22,21
        .byte 67,21,22,0,21,0,52,52,22,21,22,0,43,0,42,0,22,21,22,21,22,36,67,27,0,22,0,22,21,22,21,52,52,52
        .byte 0,22,0,0,11,0,43,0,43,0,92,92,0,0,11,42,0,21,36,67,0,0,0,21,0,21,22,0,0,0,21,0,0,0
        .byte 11,0,43,0,42,0,83,83,0,0,11,0,21,22,27,36,255,0,5,22,21,0,0,0,22,0,0,0,11,0,42,0,0,0
        .byte 66,66,66,255,0,4,21,27,36,255,0,6,22,255,0,14,72,255,0,6,22,36,255,0,19,20,0,0,0,66,66,0,0
        .byte 22,21,27,255,0,14,255,66,8,255,0,5,21,22,27,255,0,27,3,3,255,27,4,26,255,0,5,4,4,4,255,0,15
        .byte 21,22,36,22,21,22,66,26,255,0,4,69,70,69,0,0,255,66,10,0,0,0,22,21,67,21,22,67,22,66,26,255,0,10
        .byte 77,0,72,255,0,7,27,27,22,36,22,67,22,21,22,66,26,255,0,9,78,255,0,9,27,22,21,36,36,22,21,22,21
        .byte 67,66,255,0,18,27,22,21,36,67,36,21,67,36,36,22,21,255,66,6,255,0,9,66,66,66,22,21,22,36,67,36,36,36
        .byte 67,22,21,22,21,22,21,22,21,22,21,255,0,7,255,27,6,21,67,36,67,36,67,22,21,22,36,67,36,36,36,67,67
        .byte 255,0,10,21,36,36,36,22,21,36,67,36,22,21,36,36,22,21,22,67,22,21,22,255,0,5,3,3,4,0,0,0,21,21
        .byte 67,36,36,36,67,36,21,36,21,22,0,22,21,22,36,36,21,11,12,3,3,4,69,70,70,255,0,4,21,21,21,67,36
        .byte 67,36,36,21,0,0,22,21,0,21,22,21,22,11,12,70,70,69,255,0,11,67
        .byte 21,36,67,21,21,36,36,255,21,5,22,255,52,7,21,22,255,21,4,67,36,36,21,36,67,21,36,36,21,21,43,21,43
        .byte 21,22,78,255,0,8,77,22,21,21,21,43,21,67,36,67,42,22,21,22,21,22,43,21,21,22,255,0,10,78,22,21,43
        .byte 21,43,22,42,22,21,42,21,21,21,43,21,43,21,21,77,255,0,12,22,43,21,43,26,21,21,22,21,22,0,26,42,22
        .byte 43,21,22,72,255,0,12,78,43,21,22,26,22,0,21,0,42,0,22,26,21,21,22,255,0,15,42,22,26,22,255,0,7
        .byte 22,26,22,255,0,17,26,255,0,11,255,71,18,255,0,7,10,0,10,0,0,255,71,18,0,0,10,10,0,255,66,7,255,71,18
        .byte 5,255,1,4,22,36,36,21,22,21,22,255,71,18,0,21,255,36,5,21,22,0,43,255,0,20,22,36,67,22,36,21,0
        .byte 78,0,42,255,0,21,22,36,21,36,255,0,16,22,21,22,21,0,21,255,0,4,21,67,36,36,10,21,0,21,22,0,21
        .byte 0,22,255,0,6,22,21,22,36,36,21,22,21,0,0,22,21,21,255,36,4,21,22,21,22,21,22,21,22,255,0,6,77
        .byte 0,21,22,36,36,22,10,21,22,21,36,67,36,67,21,36,36,36,21,22,36,22,21,22,255,0,5,78,0,0,43,21,67
        .byte 255,36,6,21,22,21,22,21,22,21,22,21,36,36,22,21,22,52,52,52,255,0,4,43,0,21,22,21,22,21,21,67,36
        .byte 21,22,0,0,21,22,21,0,78,70,36,36,255,0,10,78,0,0,0,21,22,21,67,22,0,0,0,78,255,0,5,69,70
        .byte 255,0,16,22,21
        .byte 74,74,74,75,0,22,67,36,21,0,22,21,22,21,255,0,6,21,22,21,22,21,22,21,36,22,21,255,87,4,86,0,22
        .byte 67,36,21,22,21,36,36,21,22,255,0,5,21,0,52,52,52,22,21,36,22,255,74,5,75,0,22,22,36,36,36,52,52
        .byte 0,21,255,0,5,77,255,0,4,72,22,21,36,255,87,6,86,0,0,22,21,36,0,0,0,22,255,0,5,72,255,0,5
        .byte 29,22,21,255,74,7,75,0,72,22,36,0,0,0,77,255,0,11,29,78,22,255,87,8,0,0,0,22,0,0,0,78,255,0,11
        .byte 29,0,0,255,74,7,75,0,0,0,78,255,0,18,72,0,78,0,0,77,0,72,255,0,27,78,255,0,40,92,92,0,255,66,4
        .byte 0,0,0,255,66,4,255,74,7,75,255,0,8,83,83,0,0,72,255,0,6,72,0,0,255,87,8,0,66,66,66,0,0,0
        .byte 66,66,66,0,0,0,22,255,0,4,22,0,22,21,255,74,7,75,0,0,0,78,0,0,0,78,77,0,0,0,21,36,22,22
        .byte 0,0,21,22,36,36,255,87,7,72,255,0,8,78,0,0,0,72,22,36,21,22,52,22,36,22,21,255,74,6,75,0,0,0
        .byte 22,21,52,52,255,0,7,42,21,255,36,4,21,0,0,255,87,5,85,0,52,22,21,36,36,36,21,255,0,8,43,21,21,21
        .byte 36,22,0,0,74,74,74,75,75,21,22,67,36,36,67,21,22,28,21,255,0,7,43,72,43,77,21,36,21,22,21,36,36
        .byte 67,22,67,36,67,21,22,21,22,21,28,28,255,0,7,43,0,42,78,0,21,36,36,36,67,21,36,36,67,22,21,22,0
        .byte 22,0,22,28,255,0,8,42,255,0,4,78,22,36,21,22,21,22,21,22,21,22,0,21,255,0,18,21,36
        .byte 255,75,30,87,87,87,77,87,72,0,0,77,72,85,87,0,0,78,86,77,0,0,0,78,0,72,0,0,77,0,85,87,87
        .byte 74,75,87,78,85,0,0,0,78,0,0,85,0,0,86,87,78,255,0,8,78,0,0,75,74,87,87,87,255,0,11,87,87
        .byte 86,255,0,11,87,87,74,74,75,0,86,86,255,0,8,85,87,85,255,0,9,91,0,75,74,87,0,0,0,87,87,255,0,9
        .byte 85,255,0,10,91,91,87,87,74,75,0,0,87,87,255,0,8,74,74,74,0,0,0,74,74,74,0,0,0,255,75,4,87
        .byte 0,0,0,85,87,255,0,9,87,85,0,0,0,86,255,0,9,75,255,0,4,85,0,0,74,74,75,255,0,4,85,86,0,0,0
        .byte 87,86,255,0,8,87,86,86,255,0,5,86,0,0,86,0,0,0,86,87,0,0,0,85,87,255,0,8,75,73,73,75,75
        .byte 0,0,0,85,86,86,87,0,0,0,87,85,255,0,4,85,0,0,255,74,6,87,73,73,87,78,255,0,4,87,87,87,0,0,0
        .byte 85,255,0,8,72,77,78,87,87,75,75,73,73,75,255,0,4,86,87,87,85,255,0,6,75,75,74,255,0,4,78,255,87,5
        .byte 73,73,87,255,0,4,87,87,85,255,0,8,86,255,0,6,85,87,75,74,75,73,73,75,75,0,0,0,85,85,255,0,9
        .byte 87,86,255,0,6,255,87,4,73,73,87,77,255,0,7,75,75,75,255,0,4,85,87,255,0,6,75,74,74,75,85,85,75
        .byte 78,255,0,8,85,85,255,0,5,85,255,0,6,85,87,87,87,255,0,27,75,74,75,255,0,27,87,87,255,74,30
        .byte 21,22,21,22,21,22,21,21,0,22,0,0,0,44,45,255,53,15,22,21,22,21,67,36,67,21,21,21,22,0,0,46,47
        .byte 53,53,255,0,10,53,53,53,21,22,21,36,21,21,36,67,36,36,21,21,0,44,45,255,53,15,22,67,36,21,0,255,21,5
        .byte 0,22,0,46,47,255,33,15,36,22,21,22,21,36,21,22,0,21,21,0,0,44,45,33,255,0,12,90,33,36,21,22,36
        .byte 21,22,36,22,0,12,22,0,0,46,47,104,255,0,10,105,106,0,33,36,22,0,36,11,22,21,12,0,12,0,0,0,44
        .byte 45,104,255,0,10,108,107,0,33,36,0,36,22,11,0,22,0,0,12,0,0,0,46,47,33,255,38,13,33,36,36,22,0
        .byte 11,255,0,8,44,45,33,0,105,106,255,0,10,33,67,255,0,12,46,47,33,0,108,107,255,0,10,33,67,255,0,12,44
        .byte 45,33,255,38,13,33,67,255,0,12,46,47,104,255,0,13,33,67,255,0,11,49,49,45,104,255,0,13,33,255,0,13,33,33,33
        .byte 255,0,13,33,255,0,25,105,106,0,0,33,0,0,0,10,0,0,34,0,0,10,0,10,255,0,13,108,107,0,0,33,255,3,14
        .byte 4,255,48,15,255,1,30,36,2,2,36,2,36,2,36,2,36,36,2,2,36,36,2,36,2,36,36,2,36,36,2,2,36,36
        .byte 2,36,2,36,36,67,36,67,36,67,36,67,36,36,36,67,36,67,67,67,36,67,67,36,67,67,36,67,36,67,67,36,36
        .byte 255,26,5,22,255,0,18,255,48,6,26,21,21,0,22,26,22,255,0,17,255,48,6,26,0,0,0,21,22,26,21,255,0,16
        .byte 255,48,6,26,0,0,22,0,21,26,255,0,20,48,48,48,66,66,26,0,21,21,0,255,71,19,0,0,90,48,67,67,36
        .byte 26,0,0,0,255,71,19,0,0,0,48,21,36,21,255,26,4,255,71,19,88,33,33,33,36,69,22,67,67,70,69,255,71,19
        .byte 0,33,33,33,36,21,67,21,26,22,255,0,21,33,33,33,67,67,21,0,22,26,22,255,0,16,88,88,0,0,33,33,33
        .byte 67,21,22,22,0,21,26,22,255,0,7,88,88,255,0,10,33,33,33,36,21,21,22,67,36,36,26,22,255,0,18,33,33,33
        .byte 36,22,21,21,36,21,67,36,26,21,255,0,9,88,88,88,255,0,5,33,33,33,67,69,21,67,36,21,0,67,26,21,255,0,15
        .byte 92,92,33,33,33,67,0,36,36,67,70,21,21,21,67,22,255,0,11,20,0,0,83,83,33,33,33,21,36,67,21,22,21
        .byte 70,70,69,70,0,0,0,44,45,255,33,15,36,67,21,22,67,67,21,22,255,0,5,46,47,33,89,255,0,11,90,33,36
        .byte 21,22,21,22,21,67,70,255,0,5,44,45,33,255,0,9,105,106,0,0,33,21,70,21,21,70,22,69,22,255,0,5,46
        .byte 47,33,255,0,9,108,107,0,0,33,67,21,22,21,22,21,255,0,7,44,45,33,255,38,13,33
        .byte 21,22,21,22,67,36,36,36,21,21,22,21,22,255,0,17,22,21,21,67,36,21,21,67,36,255,21,6,255,0,15,21,67
        .byte 36,36,21,22,22,21,67,36,36,21,21,21,0,22,255,0,14,21,36,21,22,21,22,21,22,21,22,21,22,21,22,22,255,0,15
        .byte 21,36,21,22,21,255,36,4,21,21,22,21,22,21,255,0,15,67,36,67,36,36,67,67,21,67,67,52,52,52,22,21,21
        .byte 255,0,14,67,36,36,21,22,21,22,21,22,22,0,0,0,21,255,0,16,67,36,36,0,21,22,255,0,24,36,36,21,21
        .byte 0,22,255,0,24,1,36,21,255,0,16,66,66,66,255,0,4,10,0,0,0,1,36,255,0,5,10,255,0,5,10,255,0,11
        .byte 27,3,3,3,4,36,67,0,66,66,11,12,66,66,66,0,0,0,66,66,66,255,0,10,5,1,1,1,36,67,0,0,0
        .byte 11,12,14,255,0,17,92,92,5,1,1,1,36,0,0,13,11,12,255,0,18,83,83,0,1,1,1,36,0,0,0,11,12
        .byte 14,255,0,17,27,3,4,1,1,1,36,67,0,0,18,19,255,0,13,10,0,27,4,4,4,8,1,1,2,1,36,67,255,0,5
        .byte 27,4,4,26,0,0,10,0,0,0,27,4,4,4,8,255,1,5,2,2,1,36,67,0,0,10,27,4,8,1,1,7,255,4,6
        .byte 8,1,1,1,255,2,5,6,0,5,67,36,36,4,4,4,8,255,2,4,1,7,8,255,1,7,15,2,2,2,6,255,0,4
        .byte 67,67,36,36,1,1,1,2,15,2,2,1,1,1,255,2,6,1,67,67,67,255,0,6
        .byte 255,0,183,52,255,0,21,52,255,0,7,52,255,0,21,52,255,0,7,52,255,0,21,52,255,0,7,52,255,0,21,52,255,0,7
        .byte 33,255,0,21,33,255,0,7,104,0,105,106,0,105,106,0,105,106,0,0,0,105,106,0,105,106,0,105,106,0,104,255,0,7
        .byte 104,0,108,107,0,108,107,0,108,107,0,0,0,108,107,0,108,107,0,108,107,0,104,255,0,7,33,255,38,9,0,0,0
        .byte 255,38,9,33,255,0,7,104,0,105,106,0,105,106,0,105,106,0,0,0,105,106,0,105,106,0,105,106,0,104,255,0,7
        .byte 104,0,108,107,0,108,107,0,108,107,0,0,0,108,107,0,108,107,0,108,107,0,104,255,0,7,33,255,38,9,33,33,33
        .byte 255,38,9,33,255,0,7,80,255,0,21,80,255,0,7,80,255,0,21,80,255,0,7,255,33,23,255,0,4
numsc:
        .byte 26
.out .sprintf("SCREENS SIZE = %d", (* - scdat))

nmedat:
        .byte 3,11,128,120,2,16,104,88,2,10,88,152,0,12,104,24,255
        .byte 0,12,112,24,2,7,40,176,2,7,104,72,2,7,72,136,255
        .byte 2,7,56,120,2,7,96,168,0,12,88,136,255
        .byte 2,7,40,200,2,7,112,144,3,6,136,80,0,12,136,176,255
        .byte 2,7,32,48,1,8,64,128,1,8,104,184,1,8,136,88,0,11,16,224,255
        .byte 1,8,16,112,1,8,64,192,4,4,128,184,4,4,144,112,0,11,144,224,4,3,136,72,4,3,136,144,255
        .byte 0,11,24,224,3,5,96,88,2,7,72,40,255
        .byte 0,11,72,8,2,10,40,144,255
        .byte 1,1,56,120,1,1,88,200,1,1,88,40,1,1,120,120,0,11,40,16,255
        .byte 2,1,136,64,2,1,48,112,1,9,144,104,0,11,32,16,8,17,144,232,255
        .byte 2,1,40,120,2,1,104,48,2,1,136,192,0,11,96,216,255
        .byte 0,11,24,232,6,14,80,40,8,17,16,40,1,8,136,80,1,8,136,160,255
        .byte 1,1,48,152,1,1,120,88,1,13,88,88,0,11,24,224,255
        .byte 5,13,24,152,5,13,144,40,0,11,24,224,5,13,88,96,2,7,16,16,255
        .byte 1,13,16,104,1,13,144,112,0,11,144,192,255
        .byte 2,2,88,160,3,6,144,72,2,10,48,96,0,12,48,16,255
        .byte 2,2,32,192,2,2,8,112,0,11,144,32,255
        .byte 0,12,120,32,2,2,96,176,2,10,120,56,255
        .byte 0,12,64,16,3,6,72,120,2,16,32,64,2,10,96,104,255
        .byte 0,11,64,224,6,14,72,128,2,2,24,120,255
        .byte 2,2,24,200,2,2,80,104,2,16,32,152,0,11,64,224,255
        .byte 2,10,64,104,2,2,32,152,2,2,40,56,1,10,144,80,0,11,72,224,255
        .byte 0,12,120,8,2,10,120,112,7,15,120,128,255
        .byte 6,14,48,64,0,0,136,112,2,2,24,96,2,10,24,144,255
        .byte 0,11,72,224,2,16,80,136,2,2,112,88,2,16,88,184,255
        .byte 0,12,144,112,8,15,144,112,255
.out .sprintf("POSITIONS SIZE = %d", (* - nmedat))

NUMOBJ = 28
objdta:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,54,216,0,0,127,252,73,36,73,36,127,252,0,0,0,0,0,0,0,0,8,24,224,8,24,224
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,15,240,20,40,11,208,16,8,36,4,47,20,47,244,47,244,32,4,31,248,9,72,16,9,72,16
        .byte 0,0,0,0,0,0,0,252,1,224,0,64,3,64,11,0,24,64,30,160,24,112,16,40,16,28,16,10,0,7,0,2,14,120,104,14,120,104
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,15,240,20,40,11,208,16,8,36,4,47,20,47,244,47,244,32,4,31,248,14,64,64,14,64,64
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,15,240,20,40,11,208,16,8,36,4,47,20,47,244,47,244,32,4,31,248,14,120,200,14,120,200
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,15,240,20,40,11,208,16,8,36,4,47,20,47,244,47,244,32,4,31,248,11,56,152,11,56,152
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,15,240,20,40,11,208,16,8,36,4,47,20,47,244,47,244,32,4,31,248,17,88,200,17,88,200
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,15,240,20,40,11,208,16,8,36,4,47,20,47,244,47,244,32,4,31,248,15,128,160,15,128,160
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,15,240,20,40,11,208,16,8,36,4,47,20,47,244,47,244,32,4,31,248,3,112,96,3,112,96
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,15,240,20,40,11,208,16,8,36,4,47,20,47,244,47,244,32,4,31,248,7,136,224,7,136,224
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,15,240,20,40,11,208,16,8,36,4,47,20,47,244,47,244,32,4,31,248,2,72,80,2,72,80
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,15,240,20,40,11,208,16,8,36,4,47,20,47,244,47,244,32,4,31,248,24,104,208,24,104,208
        .byte 0,0,0,128,0,16,3,64,4,16,4,128,0,0,15,0,63,192,49,192,117,224,112,224,118,224,48,192,63,192,15,0,12,144,224,12,144,224
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,15,240,20,40,11,208,16,8,36,4,47,20,47,244,47,244,32,4,31,248,12,56,176,12,56,176
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,15,240,20,40,11,208,16,8,36,4,47,20,47,244,47,244,32,4,31,248,13,120,16,13,120,16
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,15,240,20,40,11,208,16,8,36,4,47,20,47,244,47,244,32,4,31,248,13,120,224,13,120,224
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,15,240,20,40,11,208,16,8,36,4,47,20,47,244,47,244,32,4,31,248,15,136,56,15,136,56
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,15,240,20,40,11,208,16,8,36,4,47,20,47,244,47,244,32,4,31,248,18,24,168,18,24,168
        .byte 0,128,0,192,0,224,12,224,13,240,2,240,6,48,1,16,3,0,0,128,1,128,0,64,0,192,0,32,0,96,0,0,21,40,216,21,40,216
        .byte 0,10,0,32,3,85,3,32,0,10,7,128,15,192,16,32,11,224,23,0,11,96,23,96,11,96,23,0,11,224,31,224,23,24,16,23,24,16
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,15,240,20,40,11,208,16,8,36,4,47,20,47,244,47,244,32,4,31,248,2,80,16,2,80,16
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,15,240,20,40,11,208,16,8,36,4,47,20,47,244,47,244,32,4,31,248,2,112,80,2,112,80
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,15,240,20,40,11,208,16,8,36,4,47,20,47,244,47,244,32,4,31,248,5,40,168,5,40,168
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,15,240,20,40,11,208,16,8,36,4,47,20,47,244,47,244,32,4,31,248,6,24,64,6,24,64
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,15,240,20,40,11,208,16,8,36,4,47,20,47,244,47,244,32,4,31,248,10,24,224,10,24,224
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,15,240,20,40,11,208,16,8,36,4,47,20,47,244,47,244,32,4,31,248,20,80,136,20,80,136
        .byte 0,0,0,0,32,130,51,230,62,190,34,34,49,70,26,44,31,252,9,200,10,168,7,112,2,160,1,192,0,128,0,0,11,24,16,11,24,16
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,15,240,20,40,11,208,16,8,36,4,47,20,47,244,47,244,32,4,31,248,5,96,224,5,96,224
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
        .byte 219,0,255,136,255,0,0,0
        .byte 0,0,0,0,126,0,0,0
        .byte 0,0,0,0,0,56,56,0
        .byte 0,0,6,12,24,48,96,0
        .byte 0,124,206,222,246,230,124,0
        .byte 0,56,120,24,24,24,126,0
        .byte 0,124,198,6,124,192,254,0
        .byte 0,124,198,28,6,198,124,0
        .byte 0,24,56,120,216,254,24,0
        .byte 0,254,192,252,6,198,124,0
        .byte 0,124,192,252,198,198,124,0
        .byte 0,254,6,12,24,48,48,0
        .byte 0,124,198,124,198,198,124,0
        .byte 0,124,198,198,126,6,124,0
        .byte 0,0,0,48,0,0,48,0
        .byte 0,0,48,0,0,48,48,96
        .byte 0,0,12,24,48,24,12,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,48,24,12,24,48,0
        .byte 0,124,198,12,24,0,24,0
        .byte 0,124,222,254,254,192,124,0
        .byte 24,60,102,102,126,102,102,0
        .byte 124,102,102,124,102,102,124,0
        .byte 60,102,96,96,96,102,60,0
        .byte 120,108,102,102,102,108,120,0
        .byte 126,96,96,124,96,96,126,0
        .byte 126,98,96,124,96,96,112,0
        .byte 60,102,96,96,110,102,60,0
        .byte 102,102,102,126,102,102,102,0
        .byte 126,90,24,24,24,90,126,0
        .byte 6,6,6,6,102,102,60,0
        .byte 96,102,108,120,120,108,102,0
        .byte 96,96,96,96,96,98,126,0
        .byte 99,119,127,107,99,99,99,0
        .byte 102,102,118,126,110,102,102,0
        .byte 60,102,102,102,102,102,60,0
        .byte 124,102,102,102,124,96,96,0
        .byte 60,126,102,102,118,126,60,0
        .byte 124,102,102,102,124,108,102,0
        .byte 60,102,96,60,6,102,60,0
        .byte 126,24,24,24,24,24,24,0
        .byte 102,102,102,102,102,102,60,0
        .byte 102,102,102,102,102,60,24,0
        .byte 99,99,99,99,107,127,54,0
        .byte 102,102,60,24,60,102,102,0
        .byte 102,102,102,60,24,24,24,0
        .byte 126,6,12,24,48,96,126,0
        .byte 30,12,12,12,12,12,12,30
        .byte 0,0,192,96,48,24,12,0
        .byte 0,240,48,48,48,48,240,0
        .byte 0,48,120,252,48,48,48,0
        .byte 0,0,0,0,0,0,0,255
        .byte 0,60,102,248,96,96,254,0
        .byte 0,0,60,6,62,102,62,0
        .byte 0,96,96,124,102,102,124,0
        .byte 0,0,60,102,96,102,60,0
        .byte 0,6,6,62,102,102,62,0
        .byte 0,0,60,102,124,96,60,0
        .byte 0,28,48,56,48,48,48,0
        .byte 0,0,56,108,108,60,12,120
        .byte 0,96,96,124,102,102,102,0
        .byte 0,24,0,56,24,24,60,0
        .byte 0,6,0,6,6,6,102,60
        .byte 0,96,108,120,120,108,102,0
        .byte 0,48,48,48,48,48,28,0
        .byte 0,0,54,127,127,107,99,0
        .byte 0,0,124,102,102,102,102,0
        .byte 0,0,60,102,102,102,60,0
        .byte 0,0,124,102,102,124,96,96
        .byte 0,0,62,102,102,62,6,7
        .byte 0,0,30,51,48,48,48,0
        .byte 0,0,62,96,60,6,124,0
        .byte 0,48,124,48,48,48,28,0
        .byte 0,0,102,102,102,102,60,0
        .byte 0,0,102,102,60,60,24,0
        .byte 0,0,99,99,107,127,54,0
        .byte 0,0,99,54,28,54,99,0
        .byte 0,0,102,102,102,62,6,124
        .byte 0,0,124,24,48,96,124,0
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
