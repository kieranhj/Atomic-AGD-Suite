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
WINDOWWID = 20 ;面面面面面面面面面面面面
MAPWID = 13
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255
mapdat:
        .byte 255,255,255,255,255,255,255,255,255,34,255,255,255,255,32,255,35,255,255,255,255,18,25,255,255,255,255,0,6,5,14,13,255,17,16,24,255,255,255,255,1,2,4,8,9,10,33,11,12,29,30,255,255,255,3,7,255,15,255,255,19,26,255,31,255,255,255,255,255,255,255,255,255,20,27,255,255,255,255,255,255,255,255,255,255,22,21,28,255,255,255,255,255,255,255,255,255,255,255,23,255,255,255,255
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255
stmap:  .byte 27

evnt00:
        lda #1
        cmp vard
        beq *+5
        jmp a00084
        lda #100
        asl a
        sta sndtyp
        lda #7
        sta z80_c
        lda #10
        sta z80_b
        jsr spawn
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
a00084: lda joyval
        and #1
        beq :+
        jmp a00190
:
        lda #0
        ldy #10
        sta (z80_ix),y
        ; lda #0
        ldy #6
        sta (z80_ix),y
        jsr cangr
        beq :+
        jmp a00190
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp a00190
        ; lda #0
        jsr animsp
a00190: lda joyval
        and #2
        beq :+
        jmp a00296
:
        lda #1
        ldy #10
        sta (z80_ix),y
        ; lda #1
        ldy #6
        sta (z80_ix),y
        jsr cangl
        beq :+
        jmp a00296
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp a00296
        ; lda #0
        jsr animsp
a00296: lda joyval
        and #4
        beq :+
        jmp a00402
:
        lda #2
        ldy #10
        sta (z80_ix),y
        ; lda #2
        ldy #6
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp a00402
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp a00402
        ; lda #0
        jsr animsp
a00402: lda joyval
        and #8
        beq :+
        jmp a00508
:
        lda #3
        ldy #10
        sta (z80_ix),y
        ; lda #3
        ldy #6
        sta (z80_ix),y
        jsr cangu
        beq :+
        jmp a00508
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp a00508
        ; lda #0
        jsr animsp
a00508: lda #12
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp a00549
        lda #148
        ; ldy #8
        sta (z80_ix),y
        jsr scru
a00549: lda #150
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp a00590
        lda #14
        ; ldy #8
        sta (z80_ix),y
        jsr scrd
a00590: lda #12
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp a00631
        lda #148
        ; ldy #9
        sta (z80_ix),y
        jsr scrl
a00631: lda #150
        ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp a00671
        lda #14
        ; ldy #9
        sta (z80_ix),y
        jsr scrr
a00671: lda joyval
        and #16
        beq :+
        jmp a00992
:
        lda #0
        cmp varg
        bcc *+5
        jmp a00979
        ; lda #0
        cmp varb
        beq *+5
        jmp a00979
        lda #20
        asl a
        sta sndtyp
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a00792
        lda #23
        ldy #6
        sta (z80_ix),y
        lda #1
        sta z80_c
        lda #4
        sta z80_b
        jsr spawn
a00792: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a00849
        lda #24
        ldy #6
        sta (z80_ix),y
        lda #1
        sta z80_c
        lda #5
        sta z80_b
        jsr spawn
a00849: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a00907
        lda #25
        ldy #6
        sta (z80_ix),y
        lda #1
        sta z80_c
        tya; lda #6
        sta z80_b
        jsr spawn
a00907: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a00964
        lda #26
        ldy #6
        sta (z80_ix),y
        lda #1
        sta z80_c
        lda #7
        sta z80_b
        jsr spawn
a00964: lda varg
        sec
        sbc #1
        sta varg
a00979: lda #1
        sta varb
        jmp a01000
a00992: lda #0
        sta varb
a01000: jsr skobj
        sta varobj
        lda #255
        cmp varobj
        bne *+5
        jmp a01194
        lda #2
        cmp vare
        bcs *+5
        jmp a01194
        lda #1
        sta varb
        lda #0
        cmp varobj
        beq *+5
        jmp a01098
        lda #1
        jsr gotob
        cmp #255
        beq :+
        jmp a01093
:
        rts
a01093: jmp a01140
a01098: lda #1
        cmp varobj
        beq *+5
        jmp a01140
        lda #0
        jsr gotob
        cmp #255
        beq :+
        jmp a01140
:
        rts
a01140: lda joyval
        and #16
        beq :+
        jmp a01194
:
        lda #30
        asl a
        sta sndtyp
        lda varobj
        jsr getob
        lda vare
        clc
        adc #1
        sta vare
a01194: lda joyval
        and #32
        beq :+
        jmp a01340
:
        lda #0
        cmp vare
        bcc *+5
        jmp a01328
        lda #2
        tax
        jsr minve
        lda #13
        cmp varopt
        bne *+5
        jmp a01323
        lda #30
        asl a
        sta sndtyp
        ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda varopt
        jsr drpob
        lda vare
        sec
        sbc #1
        sta vare
a01323: jmp a01331
a01328: rts
a01331: lda #10
        jsr delay
a01340: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01522
:
        lda #32
        cmp scno
        bne *+5
        jmp a01469
        lda #7
        jsr gotob
        cmp #255
        beq :+
        jmp a01410
:
        jmp a01464
a01410: lda #1
        cmp varh
        bcc *+5
        jmp a01456
        lda #2
        sta z80_c
        lda varh
        sec
        sbc z80_c
        sta varh
        jmp a01464
a01456: lda #0
        sta varh
a01464: jmp a01522
a01469: lda #1
        cmp varh
        bcc *+5
        jmp a01514
        lda #2
        sta z80_c
        lda varh
        sec
        sbc z80_c
        sta varh
        jmp a01522
a01514: lda #0
        sta varh
a01522: lda #0
        cmp varh
        beq *+5
        jmp a01547
        lda #1
        sta vard
a01547: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a02023
:
        lda #0
        cmp scno
        beq *+5
        jmp a01744
        lda #9
        jsr gotob
        cmp #255
        beq :+
        jmp a01744
:
        lda #10
        jsr gotob
        cmp #255
        beq :+
        jmp a01744
:
        lda #99
        cmp varc
        bcs *+5
        jmp a01744
        lda #100
        sta varc
        lda #16
        sta chary
        lda #27
        sta charx
        lda #1
        jsr dmsg
        lda #15
        cmp varm
        beq *+5
        jmp a01715
        lda #50
        sta varl
        jmp a01723
a01715: lda #30
        sta varl
a01723: lda #32
        sta scno
        jsr nwscr
        lda #1
        sta restfl
a01744: lda #14
        cmp scno
        beq *+5
        jmp a01800
        lda #15
        cmp varm
        beq *+5
        jmp a01791
        lda #35
        sta varh
        jmp a01800
a01791: lda #50
        sta varh
a01800: lda #15
        cmp scno
        beq *+5
        jmp a01855
        ; lda #15
        cmp varm
        beq *+5
        jmp a01847
        lda #65
        sta varg
        jmp a01855
a01847: lda #99
        sta varg
a01855: lda #34
        cmp scno
        beq *+5
        jmp a02023
        lda #0
        cmp varl
        beq *+5
        jmp a02023
        ldy #8
        lda (z80_ix),y
        sta varo
        lda varp
        iny; ldy #9
        sta (z80_ix),y
        lda #128
        dey; ldy #8
        sta (z80_ix),y
        lda #112
        iny; ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #12
        jsr drpob
        lda varo
        ldy #8
        sta (z80_ix),y
        lda varp
        iny; ldy #9
        sta (z80_ix),y
        lda #1
        sta varl
a02023: ldy #8
        lda (z80_ix),y
        sta varo
        iny; ldy #9
        lda (z80_ix),y
        sta varp
        rts
.out .sprintf("EVENT 0 SIZE = %d", (* - evnt00))

evnt01:
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00100
        jsr cangr
        beq :+
        jmp b00087
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
        jmp b00100
b00087: lda #255
        ldy #5
        sta (z80_ix),y
b00100: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00199
        jsr cangl
        beq :+
        jmp b00185
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
        jmp b00199
b00185: lda #255
        ldy #5
        sta (z80_ix),y
b00199: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00298
        jsr cangd
        beq :+
        jmp b00284
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp b00298
b00284: lda #255
        ldy #5
        sta (z80_ix),y
b00298: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00397
        jsr cangu
        beq :+
        jmp b00383
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp b00397
b00383: lda #255
        ldy #5
        sta (z80_ix),y
b00397: ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
        lda #10
        sta varf
        lda #0
        jsr gotob
        cmp #255
        beq :+
        jmp b00461
:
        lda #18
        sta varf
b00461: lda #1
        jsr gotob
        cmp #255
        beq :+
        jmp b00491
:
        lda #12
        sta varf
b00491: lda varf
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b00528
        lda #255
        ldy #5
        sta (z80_ix),y
b00528: lda #7
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00563
:
        lda #255
        ldy #5
        sta (z80_ix),y
b00563: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp b00785
:
        lda #32
        cmp scno
        beq *+5
        jmp b00785
        lda #0
        cmp varl
        bcc *+5
        jmp b00656
        lda #30
        asl a
        sta sndtyp
        lda varl
        sec
        sbc #1
        sta varl
        jmp b00771
b00656: lda #20
        sta loopa
b00665: lda #<250
        sta z80_l
        lda #>250
        sta z80_h
        jsr addsc
        dec loopa
        beq :+
        jmp b00665
:
        lda #15
        cmp varm
        beq *+5
        jmp b00762
        lda #20
        sta loopa
b00726: lda #<250
        sta z80_l
        lda #>250
        sta z80_h
        jsr addsc
        dec loopa
        beq :+
        jmp b00726
:
b00762: lda #1
        sta gamwon
b00771: lda #255
        ldy #5
        sta (z80_ix),y
b00785: rts
.out .sprintf("EVENT 1 SIZE = %d", (* - evnt01))

evnt02:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp c00082
        jsr cangl
        beq :+
        jmp c00064
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp c00078
c00064: lda #1
        ldy #11
        sta (z80_ix),y
c00078: jmp c00136
c00082: jsr cangr
        beq :+
        jmp c00123
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp c00136
c00123: lda #0
        ldy #11
        sta (z80_ix),y
c00136: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c00217
        jsr cangu
        beq :+
        jmp c00199
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp c00212
c00199: lda #1
        ldy #10
        sta (z80_ix),y
c00212: jmp c00271
c00217: jsr cangd
        beq :+
        jmp c00257
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp c00271
c00257: lda #0
        ldy #10
        sta (z80_ix),y
c00271: lda #0
        cmp vara
        beq *+5
        jmp c00296
        ; lda #0
        jsr animsp
c00296: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00434
:
        lda #0
        cmp vara
        beq *+5
        jmp c00434
        ; lda #0
        cmp varh
        bcc *+5
        jmp c00434
        lda #30
        asl a
        sta sndtyp
        lda varh
        sec
        sbc #1
        sta varh
        lda #9
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00434
        lda #0
        cmp varh
        bcc *+5
        jmp c00434
        lda varh
        sec
        sbc #1
        sta varh
c00434: lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00750
:
        lda #9
        ldy #6
        cmp (z80_ix),y
        bne *+5
        jmp c00750
        lda #1
        cmp vari
        bcs *+5
        jmp c00638
        lda #5
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #2
        cmp varrnd
        bcc *+5
        jmp c00568
        lda #7
        sta z80_c
        lda #22
        sta z80_b
        jsr spawn
        jmp c00598
c00568: lda #8
        sta z80_c
        ; lda #8
        sta z80_b
        jsr spawn
        lda #1
        sta varq
c00598: lda #255
        ldy #5
        sta (z80_ix),y
        lda #<50
        sta z80_l
        lda #>50
        sta z80_h
        jsr addsc
        jmp c00750
c00638: lda vari
        sec
        sbc #1
        sta vari
        lda #1
        jsr gotob
        cmp #255
        beq :+
        jmp c00699
:
        lda #2
        sta z80_c
        lda vari
        sec
        sbc z80_c
        sta vari
c00699: lda skptr
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
c00750: rts
.out .sprintf("EVENT 2 SIZE = %d", (* - evnt02))

evnt03:
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp d00034
:
        jmp d00034
d00034: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp d00114
        jsr cangl
        beq :+
        jmp d00096
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp d00110
d00096: lda #1
        ldy #11
        sta (z80_ix),y
d00110: jmp d00168
d00114: jsr cangr
        beq :+
        jmp d00155
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp d00168
d00155: lda #0
        ldy #11
        sta (z80_ix),y
d00168: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp d00249
        jsr cangu
        beq :+
        jmp d00231
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp d00244
d00231: lda #1
        ldy #10
        sta (z80_ix),y
d00244: jmp d00303
d00249: jsr cangd
        beq :+
        jmp d00289
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp d00303
d00289: lda #0
        ldy #10
        sta (z80_ix),y
d00303: lda #0
        cmp vara
        beq *+5
        jmp d00328
        ; lda #0
        jsr animsp
d00328: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00412
:
        lda #0
        cmp vara
        beq *+5
        jmp d00412
        ; lda #0
        cmp varh
        bcc *+5
        jmp d00412
        lda #30
        asl a
        sta sndtyp
        lda varh
        sec
        sbc #1
        sta varh
d00412: lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00754
:
        lda #1
        cmp varj
        bcs *+5
        jmp d00651
        lda #12
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00581
        lda #5
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #2
        cmp varrnd
        bcc *+5
        jmp d00547
        lda #7
        sta z80_c
        lda #22
        sta z80_b
        jsr spawn
        jmp d00576
d00547: lda #8
        sta z80_c
        ; lda #8
        sta z80_b
        jsr spawn
        lda #1
        sta varq
d00576: jmp d00610
d00581: lda #8
        sta z80_c
        ; lda #8
        sta z80_b
        jsr spawn
        lda #1
        sta varq
d00610: lda #255
        ldy #5
        sta (z80_ix),y
        lda #<50
        sta z80_l
        lda #>50
        sta z80_h
        jsr addsc
        jmp d00754
d00651: lda varj
        sec
        sbc #1
        sta varj
        lda #1
        jsr gotob
        cmp #255
        beq :+
        jmp d00703
:
        lda varj
        sec
        sbc #1
        sta varj
d00703: lda skptr
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
d00754: lda #12
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00781
        jmp d00781
d00781: rts
.out .sprintf("EVENT 3 SIZE = %d", (* - evnt03))

evnt04:
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp e00034
:
        jmp e00034
e00034: ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
        lda #1
        ; ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp e00339
        lda #0
        ; ldy #11
        sta (z80_ix),y
        lda varp
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp e00160
        jsr cangl
        beq :+
        jmp e00155
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
e00155: jmp e00218
e00160: lda varp
        ldy #9
        cmp (z80_ix),y
        bne *+5
        jmp e00218
        jsr cangr
        beq :+
        jmp e00218
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
e00218: lda varo
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp e00281
        jsr cangu
        beq :+
        jmp e00276
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
e00276: jmp e00339
e00281: lda varo
        ldy #8
        cmp (z80_ix),y
        bne *+5
        jmp e00339
        jsr cangd
        beq :+
        jmp e00339
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
e00339: lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00550
:
        lda #1
        cmp vark
        bcs *+5
        jmp e00448
        lda #8
        sta z80_c
        ; lda #8
        sta z80_b
        jsr spawn
        lda #1
        sta varq
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #<150
        sta z80_l
        lda #>150
        sta z80_h
        jsr addsc
        jmp e00463
e00448: lda vark
        sec
        sbc #1
        sta vark
e00463: lda #1
        jsr gotob
        cmp #255
        beq :+
        jmp e00500
:
        lda vark
        sec
        sbc #1
        sta vark
e00500: lda skptr
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
e00550: lda #0
        cmp vara
        beq *+5
        jmp e00576
        ; lda #0
        jsr animsp
e00576: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00659
:
        lda #0
        cmp vara
        beq *+5
        jmp e00659
        ; lda #0
        cmp varh
        bcc *+5
        jmp e00659
        lda #30
        asl a
        sta sndtyp
        lda varh
        sec
        sbc #1
        sta varh
e00659: rts
.out .sprintf("EVENT 4 SIZE = %d", (* - evnt04))

evnt05:
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp f00034
:
        jmp f00034
f00034: ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
        lda #1
        ; ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp f00339
        lda #0
        ; ldy #11
        sta (z80_ix),y
        lda varp
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp f00160
        jsr cangl
        beq :+
        jmp f00155
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
f00155: jmp f00218
f00160: lda varp
        ldy #9
        cmp (z80_ix),y
        bne *+5
        jmp f00218
        jsr cangr
        beq :+
        jmp f00218
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
f00218: lda varo
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp f00281
        jsr cangu
        beq :+
        jmp f00276
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
f00276: jmp f00339
f00281: lda varo
        ldy #8
        cmp (z80_ix),y
        bne *+5
        jmp f00339
        jsr cangd
        beq :+
        jmp f00339
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
f00339: lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00550
:
        lda #1
        cmp vark
        bcs *+5
        jmp f00447
        lda #8
        sta z80_c
        lda #9
        sta z80_b
        jsr spawn
        lda #1
        sta varq
        lda #<100
        sta z80_l
        lda #>100
        sta z80_h
        jsr addsc
        lda #255
        ldy #5
        sta (z80_ix),y
        jmp f00462
f00447: lda vark
        sec
        sbc #1
        sta vark
f00462: lda #1
        jsr gotob
        cmp #255
        beq :+
        jmp f00499
:
        lda vark
        sec
        sbc #1
        sta vark
f00499: lda skptr
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
f00550: lda #0
        cmp vara
        beq *+5
        jmp f00576
        ; lda #0
        jsr animsp
f00576: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00687
:
        lda #8
        sta z80_c
        lda #9
        sta z80_b
        jsr spawn
        lda varm
        cmp varh
        bcc *+5
        jmp f00665
        lda varm
        sta z80_c
        lda varh
        sec
        sbc z80_c
        sta varh
        jmp f00673
f00665: lda #0
        sta varh
f00673: lda #255
        ldy #5
        sta (z80_ix),y
f00687: rts
.out .sprintf("EVENT 5 SIZE = %d", (* - evnt05))

evnt06:
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00403
:
        lda #21
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp g00272
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #8
        jsr gotob
        cmp #255
        beq :+
        jmp g00218
:
        lda #16
        ldy #8
        sta (z80_ix),y
        ; lda #16
        iny; ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        tya; lda #8
        jsr drpob
        lda #40
        ldy #8
        sta (z80_ix),y
        lda #80
        iny; ldy #9
        sta (z80_ix),y
        lda #99
        sta varc
        lda #1
        sta varn
        lda vare
        sec
        sbc #1
        sta vare
        jmp g00253
g00218: lda #2
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
g00253: lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
g00272: lda #17
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp g00403
        lda #11
        jsr gotob
        cmp #255
        beq :+
        jmp g00355
:
        lda #30
        asl a
        sta sndtyp
        lda #6
        sta varr
        lda #255
        ldy #5
        sta (z80_ix),y
        jmp g00403
g00355: lda #0
        cmp varh
        bcc *+5
        jmp g00403
        lda #1
        cmp vara
        bcs *+5
        jmp g00403
        lda varh
        sec
        sbc #1
        sta varh
g00403: lda #0
        cmp vara
        beq *+5
        jmp g00429
        ; lda #0
        jsr animsp
g00429: rts
.out .sprintf("EVENT 6 SIZE = %d", (* - evnt06))

evnt07:
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp h00843
:
        lda #22
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h00301
        lda #30
        asl a
        sta sndtyp
        lda #3
        cmp varrnd
        beq *+5
        jmp h00173
        lda #5
        sta z80_c
        lda varh
        clc
        adc z80_c
        sta varh
        lda #15
        cmp varm
        beq *+5
        jmp h00147
        lda #35
        cmp varh
        bcc *+5
        jmp h00142
        ; lda #35
        sta varh
h00142: jmp h00173
h00147: lda #50
        cmp varh
        bcc *+5
        jmp h00173
        ; lda #50
        sta varh
h00173: lda #4
        cmp varrnd
        beq *+5
        jmp h00288
        lda #10
        sta z80_c
        lda varg
        clc
        adc z80_c
        sta varg
        lda #15
        cmp varm
        beq *+5
        jmp h00262
        lda #65
        cmp varg
        bcc *+5
        jmp h00257
        ; lda #65
        sta varg
h00257: jmp h00288
h00262: lda #99
        cmp varg
        bcc *+5
        jmp h00288
        ; lda #99
        sta varg
h00288: lda #255
        ldy #5
        sta (z80_ix),y
h00301: lda #18
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h00824
        lda #10
        cmp scno
        beq *+5
        jmp h00376
        lda #2
        jsr gotob
        cmp #255
        beq :+
        jmp h00376
:
        lda #255
        ldy #5
        sta (z80_ix),y
h00376: lda #12
        cmp scno
        beq *+5
        jmp h00428
        lda #5
        jsr gotob
        cmp #255
        beq :+
        jmp h00428
:
        lda #255
        ldy #5
        sta (z80_ix),y
h00428: lda #13
        cmp scno
        beq *+5
        jmp h00480
        lda #3
        jsr gotob
        cmp #255
        beq :+
        jmp h00480
:
        lda #255
        ldy #5
        sta (z80_ix),y
h00480: lda #15
        cmp scno
        beq *+5
        jmp h00533
        lda #4
        jsr gotob
        cmp #255
        beq :+
        jmp h00533
:
        lda #255
        ldy #5
        sta (z80_ix),y
h00533: lda #21
        cmp scno
        beq *+5
        jmp h00585
        lda #6
        jsr gotob
        cmp #255
        beq :+
        jmp h00585
:
        lda #255
        ldy #5
        sta (z80_ix),y
h00585: lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda joyval
        and #1
        beq :+
        jmp h00656
:
        lda #2
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
h00656: lda joyval
        and #2
        beq :+
        jmp h00709
:
        lda #2
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
h00709: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp h00767
        ; lda #2
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
h00767: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp h00824
        lda #2
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
h00824: lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
h00843: lda #10
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h01065
        lda vard
        clc
        adc #1
        sta vard
        lda #41
        cmp vard
        bcc *+5
        jmp h01065
        lda #255
        dey; ldy #5
        sta (z80_ix),y
        lda numlif
        sec
        sbc #1
        sta numlif
        lda #255
        cmp numlif
        beq *+5
        jmp h00960
        lda #1
        sta gamwon
        jmp h01065
h00960: lda varo
        ldy #8
        sta (z80_ix),y
        lda varp
        iny; ldy #9
        sta (z80_ix),y
        lda #0
        sta z80_c
        ; lda #0
        sta z80_b
        jsr spawn
        lda #15
        cmp varm
        beq *+5
        jmp h01048
        lda #35
        sta varh
        lda #65
        sta varg
        jmp h01065
h01048: lda #50
        sta varh
        lda #99
        sta varg
h01065: lda #0
        cmp vara
        beq *+5
        jmp h01090
        ; lda #0
        jsr animsp
h01090: rts
.out .sprintf("EVENT 7 SIZE = %d", (* - evnt07))

evnt08:
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp i00034
:
        jmp i00034
i00034: lda #8
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i00158
        lda varq
        clc
        adc #1
        sta varq
        lda #21
        cmp varq
        bcc *+5
        jmp i00158
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp i00128
:
        lda #7
        sta varr
        jmp i00136
i00128: lda #6
        sta varr
i00136: lda #255
        ldy #5
        sta (z80_ix),y
        lda #0
        sta varq
i00158: lda #9
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i00282
        lda varq
        clc
        adc #1
        sta varq
        lda #21
        cmp varq
        bcc *+5
        jmp i00282
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp i00252
:
        lda #7
        sta varr
        jmp i00260
i00252: lda #6
        sta varr
i00260: lda #255
        ldy #5
        sta (z80_ix),y
        lda #0
        sta varq
i00282: lda #0
        cmp vara
        beq *+5
        jmp i00307
        ; lda #0
        jsr animsp
i00307: rts
.out .sprintf("EVENT 8 SIZE = %d", (* - evnt08))

evnt09:
        rts
.out .sprintf("EVENT 9 SIZE = %d", (* - evnt09))

evnt10:
        lda #3
        cmp vara
        beq *+5
        jmp k00031
        lda #0
        sta vara
        jmp k00046
k00031: lda vara
        clc
        adc #1
        sta vara
k00046: lda #7
        sta chary
        lda #28
        sta charx
        lda numlif
        jsr disply
        lda #10
        sta chary
        lda #27
        sta charx
        lda #9
        cmp varh
        bcs *+5
        jmp k00145
        lda #0
        jsr dmsg
        lda #10
        sta chary
        lda #28
        sta charx
        lda varh
        jsr disply
        jmp k00154
k00145: lda varh
        jsr disply
k00154: lda #13
        sta chary
        lda #27
        sta charx
        lda #9
        cmp varg
        bcs *+5
        jmp k00227
        lda #0
        jsr dmsg
        lda #13
        sta chary
        lda #28
        sta charx
        lda varg
        jsr disply
        jmp k00236
k00227: lda varg
        jsr disply
k00236: lda #0
        cmp varq
        bcc *+5
        jmp k00293
        lda varq
        clc
        adc #1
        sta varq
        lda #19
        cmp varq
        beq *+5
        jmp k00293
        lda #0
        sta varq
k00293: lda #19
        sta chary
        lda #24
        sta charx
        jsr dscor
        rts
.out .sprintf("EVENT 10 SIZE = %d", (* - evnt10))

evnt11:
        lda #99
        cmp varc
        bcs *+5
        jmp l00158
        lda #16
        sta chary
        lda #27
        sta charx
        lda #9
        cmp varc
        bcs *+5
        jmp l00091
        lda #0
        jsr dmsg
        lda #16
        sta chary
        lda #28
        sta charx
        lda varc
        jsr disply
        jmp l00100
l00091: lda varc
        jsr disply
l00100: lda #15
        cmp varn
        beq *+5
        jmp l00132
        lda varc
        sec
        sbc #1
        sta varc
l00132: lda #255
        cmp varc
        beq *+5
        jmp l00158
        lda #1
        sta gamwon
l00158: lda #0
        cmp varn
        bcc *+5
        jmp l00215
        lda varn
        clc
        adc #1
        sta varn
        lda #16
        cmp varn
        beq *+5
        jmp l00215
        lda #1
        sta varn
l00215: lda #8
        cmp varr
        beq *+5
        jmp l00245
        lda #1
        sta varr
        jmp l00260
l00245: lda varr
        clc
        adc #1
        sta varr
l00260: rts
.out .sprintf("EVENT 11 SIZE = %d", (* - evnt11))

evnt12:
        rts
.out .sprintf("EVENT 12 SIZE = %d", (* - evnt12))

evnt13:
        lda #100
        sta varc
        lda #1
        sta vare
        lda #0
        sta varl
        ; lda #0
        sta varn
        lda #1
        sta varr
        lda #2
        sta numlif
        lda #15
        cmp varm
        beq *+5
        jmp n00089
        lda #65
        sta varg
        lda #35
        sta varh
        jmp n00106
n00089: lda #99
        sta varg
        lda #50
        sta varh
n00106: lda #0
        cmp vara
        beq *+5
        jmp n00570
        lda #3
        sta scno
        jsr nwscr
        lda #48
        ldy #8
        sta (z80_ix),y
        lda #144
        iny; ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #4
        jsr drpob
        lda #30
        sta scno
        jsr nwscr
        lda #72
        ldy #8
        sta (z80_ix),y
        lda #120
        iny; ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #1
        jsr drpob
        lda #6
        sta scno
        jsr nwscr
        lda #40
        ldy #8
        sta (z80_ix),y
        lda #96
        iny; ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #2
        jsr drpob
        lda #27
        sta scno
        jsr nwscr
        lda #112
        ldy #8
        sta (z80_ix),y
        iny; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #11
        jsr drpob
        lda #20
        sta scno
        jsr nwscr
        lda #136
        ldy #8
        sta (z80_ix),y
        lda #16
        iny; ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #3
        jsr drpob
        lda #15
        sta scno
        jsr nwscr
        lda #144
        ldy #8
        sta (z80_ix),y
        iny; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #0
        jsr drpob
n00570: lda #1
        cmp vara
        beq *+5
        jmp n01033
        lda #16
        sta scno
        jsr nwscr
        lda #120
        ldy #8
        sta (z80_ix),y
        lda #16
        iny; ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #0
        jsr drpob
        lda #7
        sta scno
        jsr nwscr
        lda #144
        ldy #8
        sta (z80_ix),y
        lda #128
        iny; ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #2
        jsr drpob
        lda #19
        sta scno
        jsr nwscr
        lda #32
        ldy #8
        sta (z80_ix),y
        iny; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #3
        jsr drpob
        lda #23
        sta scno
        jsr nwscr
        lda #16
        ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #11
        jsr drpob
        lda #27
        sta scno
        jsr nwscr
        lda #112
        ldy #8
        sta (z80_ix),y
        lda #96
        iny; ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #1
        jsr drpob
        lda #35
        sta scno
        jsr nwscr
        lda #88
        ldy #8
        sta (z80_ix),y
        lda #72
        iny; ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #4
        jsr drpob
n01033: lda #2
        cmp vara
        beq *+5
        jmp n01481
        lda #14
        sta scno
        jsr nwscr
        lda #32
        ldy #8
        sta (z80_ix),y
        lda #24
        iny; ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #0
        jsr drpob
        lda #25
        sta scno
        jsr nwscr
        ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #4
        jsr drpob
        lda #27
        sta scno
        jsr nwscr
        lda #112
        ldy #8
        sta (z80_ix),y
        lda #96
        iny; ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #1
        jsr drpob
        lda #6
        sta scno
        jsr nwscr
        lda #40
        ldy #8
        sta (z80_ix),y
        iny; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #3
        jsr drpob
        lda #35
        sta scno
        jsr nwscr
        lda #88
        ldy #8
        sta (z80_ix),y
        lda #72
        iny; ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #2
        jsr drpob
        lda #3
        sta scno
        jsr nwscr
        lda #48
        ldy #8
        sta (z80_ix),y
        lda #144
        iny; ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #11
        jsr drpob
n01481: lda #3
        cmp vara
        beq *+5
        jmp n01914
        lda #35
        sta scno
        jsr nwscr
        lda #88
        ldy #8
        sta (z80_ix),y
        lda #72
        iny; ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #3
        jsr drpob
        lda #25
        sta scno
        jsr nwscr
        lda #32
        ldy #8
        sta (z80_ix),y
        lda #24
        iny; ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #0
        jsr drpob
        lda #19
        sta scno
        jsr nwscr
        lda #128
        ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #4
        jsr drpob
        lda #23
        sta scno
        jsr nwscr
        lda #16
        ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #1
        jsr drpob
        lda #13
        sta scno
        jsr nwscr
        lda #144
        ldy #8
        sta (z80_ix),y
        iny; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #2
        jsr drpob
        lda #7
        sta scno
        jsr nwscr
        lda #128
        ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #11
        jsr drpob
n01914: lda #0
        sta scno
        jsr nwscr
        rts
.out .sprintf("EVENT 13 SIZE = %d", (* - evnt13))

evnt14:
        lda #15
        cmp varm
        beq *+5
        jmp o00048
        lda #4
        sta vari
        ; lda #4
        sta varj
        lda #6
        sta vark
        jmp o00072
o00048: lda #3
        sta vari
        ; lda #3
        sta varj
        lda #5
        sta vark
o00072: lda #0
        sta varq
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
        lda #12
        jsr gotob
        cmp #255
        beq :+
        jmp s00032
:
        lda #255
        sta varn
s00032: rts
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
        .byte "#",141
        .byte "--",141
        .byte "#M5000#RIFLE",13
        .byte "#AG-73#RIFLE",13
        .byte "#EAST#WING#KEYCARD",13
        .byte "#INFIRMARY#KEYCARD",13
        .byte "#ARMORY#KEYCARD",13
        .byte "#LABORATORY#KEYCARD",13
        .byte "#COMMANDER",39
        .byte "S#KEYCARD",13
        .byte "#ANTI-RAD#SHIELD",13
        .byte "#TIME#BOMB",13
        .byte "#DATA#DISK",13
        .byte "#NANOREACTOR",13
        .byte "#FIRE#EXTINGUISHER",13
        .byte "#ZX#SPECTRUM",13
        .byte "##*EXIT*",141
nummsg:
        .byte 3
.out .sprintf("MESSAGE SIZE = %d", (* - msgdat))

chgfx:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 34,136,34,136,34,136,34,136,34,136,34,136,34,136,34,136
        .byte 49,196,64,34,68,160,64,170,68,32,186,216,68,32,50,200
        .byte 68,0,0,136,136,0,0,51,68,204,17,0,34,0,68,0
        .byte 0,68,17,0,0,0,204,34,51,0,68,136,51,68,17,170
        .byte 68,0,170,0,51,0,0,136,68,102,17,0,51,68,68,34
        .byte 17,170,51,68,238,136,17,34,221,0,136,136,170,102,17,0
        .byte 0,204,51,238,119,221,255,238,187,85,102,170,51,68,17,136
        .byte 0,0,0,68,68,0,0,0,0,0,0,34,17,0,0,0
        .byte 0,0,0,68,68,0,0,0,0,136,0,0,0,34,34,0
        .byte 51,204,51,204,51,204,51,204,51,204,51,204,51,204,51,204
        .byte 0,0,0,0,90,26,0,0,90,26,165,133,82,10,0,0
        .byte 87,2,39,170,87,2,39,170,87,2,39,170,0,0,39,174
        .byte 78,25,175,1,78,25,175,1,78,25,175,1,78,25,175,1
        .byte 0,0,50,64,0,0,50,64,17,128,0,0,0,0,0,0
        .byte 245,245,152,80,128,177,136,80,128,49,200,16,228,17,80,16
        .byte 49,245,0,0,49,245,50,16,17,213,0,114,0,49,0,0
        .byte 0,95,0,175,0,95,0,175,0,95,0,175,0,95,0,0
        .byte 68,87,34,39,85,87,34,39,85,87,34,39,85,87,34,0
        .byte 17,95,0,0,19,95,0,0,87,95,39,175,87,95,0,0
        .byte 68,170,68,170,34,170,34,136,17,170,17,170,0,136,0,255
        .byte 93,0,137,0,93,0,137,0,93,0,137,0,93,0,0,0
        .byte 10,2,34,4,10,2,34,4,10,2,34,4,10,2,0,4
        .byte 95,0,0,0,95,8,0,0,85,2,170,34,85,2,0,0
        .byte 17,34,17,34,17,68,17,68,17,136,17,136,17,0,255,0
        .byte 245,245,170,152,213,17,170,16,196,17,136,50,128,85,136,170
        .byte 245,196,0,0,245,196,136,200,177,128,234,0,196,0,0,0
        .byte 243,252,247,248,255,240,254,241,252,243,248,247,240,255,241,254
        .byte 87,0,39,39,87,87,0,0,87,2,39,5,87,2,39,5
        .byte 0,78,174,174,78,78,0,0,10,78,4,174,10,78,4,174
        .byte 0,95,174,175,78,95,0,175,10,0,4,175,25,95,35,140
        .byte 95,0,175,39,95,87,175,0,0,2,175,1,95,8,35,140
        .byte 87,2,39,5,87,2,39,5,87,2,39,5,87,2,0,0
        .byte 10,78,4,174,10,78,4,174,10,78,4,174,10,78,0,0
        .byte 27,0,35,0,27,87,35,0,27,95,35,170,27,95,35,175
        .byte 0,76,0,140,78,76,0,140,95,76,5,140,95,76,175,140
        .byte 49,196,114,136,245,0,234,16,196,49,136,114,0,245,16,234
        .byte 95,95,175,175,95,95,175,175,95,95,175,175,95,95,175,175
        .byte 15,15,175,175,31,31,143,143,95,95,143,143,95,95,175,175
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 0,0,15,15,5,5,10,10,5,5,10,10,5,5,10,10
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
        .byte 143,15,127,255,127,223,127,223,127,223,127,223,79,31,127,255
        .byte 245,245,250,250,245,245,250,250,245,245,250,250,245,245,250,250
        .byte 90,90,242,250,94,91,240,250,95,215,240,248,95,91,242,250
        .byte 0,0,255,255,255,255,255,255,255,255,255,255,0,0,255,255
        .byte 241,244,240,242,240,245,248,248,240,241,248,240,244,240,242,248
        .byte 0,0,37,165,82,72,37,133,80,90,5,132,82,88,37,165
        .byte 0,0,165,164,16,10,165,164,90,10,1,164,90,10,165,164
        .byte 82,88,36,165,82,72,36,133,66,88,37,133,0,16,0,0
        .byte 90,10,165,164,16,74,165,160,88,74,133,164,72,0,0,0
        .byte 0,0,119,238,119,238,119,238,119,238,119,238,119,238,0,0
        .byte 119,238,119,238,68,34,85,238,119,238,68,34,85,238,119,238
        .byte 119,238,119,170,119,170,119,238,119,238,119,170,68,34,119,238
        .byte 0,0,102,0,85,255,34,0,68,221,85,136,85,0,85,0
        .byte 0,0,0,102,255,170,0,68,0,34,0,34,0,34,0,34
        .byte 68,0,85,0,68,0,102,0,51,0,68,255,102,255,0,0
        .byte 0,34,0,170,17,34,68,102,0,204,255,34,255,102,0,0
        .byte 0,0,16,250,17,245,16,250,68,0,80,250,85,0,16,50
        .byte 0,0,250,136,245,128,250,136,0,32,250,170,0,160,200,136
        .byte 85,117,80,0,85,245,64,0,85,245,80,0,85,0,16,250
        .byte 228,160,0,170,245,160,0,34,245,160,0,170,0,160,250,136
        .byte 0,0,0,17,0,119,17,255,51,255,119,204,119,221,119,170
        .byte 0,0,255,255,255,255,255,255,0,0,255,255,0,0,0,170
        .byte 0,0,136,0,238,0,255,136,255,204,51,238,187,238,85,238
        .byte 119,170,119,204,119,204,51,255,85,255,34,119,102,153,34,170
        .byte 85,85,170,170,85,85,0,0,255,255,255,255,255,255,0,0
        .byte 85,238,187,238,51,238,255,204,255,0,238,68,204,136,17,0
        .byte 102,170,34,170,102,170,34,170,102,170,34,170,102,170,34,170
        .byte 120,120,180,180,210,210,225,225,120,120,180,180,210,210,225,225
        .byte 68,0,68,68,68,136,85,0,68,0,68,68,68,136,85,0
        .byte 102,170,34,170,102,170,34,170,102,170,34,170,0,170,0,34
        .byte 0,0,119,255,68,0,68,255,85,255,68,0,68,255,85,255
        .byte 0,0,255,238,0,34,255,34,255,170,0,34,255,34,255,170
        .byte 68,0,68,255,85,255,68,0,68,255,85,255,68,0,119,255
        .byte 0,34,255,34,255,170,0,34,255,34,255,170,0,34,255,238
        .byte 245,245,250,250,245,245,250,250,245,245,250,250,0,0,250,250
        .byte 0,0,16,0,17,128,16,136,17,128,16,136,0,0,0,0
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,0,0,255,255
        .byte 0,0,17,0,17,136,17,136,17,136,17,136,0,0,0,0
        .byte 16,192,32,32,32,224,32,224,48,224,0,128,48,160,112,240
        .byte 128,0,128,0,0,0,128,0,128,0,176,192,128,224,112,224
        .byte 16,192,48,224,48,224,48,96,48,96,0,128,32,160,112,176
        .byte 0,16,0,16,0,0,0,16,0,16,48,208,112,16,112,224
        .byte 51,238,0,0,34,170,68,153,51,238,0,0,0,0,0,0
        .byte 0,0,0,16,0,33,0,18,1,133,18,74,4,32,0,10
        .byte 0,0,8,0,4,0,18,8,37,132,64,64,133,0,26,8
        .byte 33,128,66,64,4,128,64,90,0,37,0,8,0,37,0,0
        .byte 160,132,64,64,32,4,74,0,132,0,2,0,132,0,0,0
        .byte 95,95,136,0,27,95,170,0,8,0,171,175,10,0,136,0
        .byte 95,78,0,0,95,76,0,4,0,0,175,140,0,68,0,0
        .byte 27,95,170,0,8,0,171,175,27,78,171,175,8,0,175,175
        .byte 95,76,0,4,0,0,175,140,87,76,175,140,0,0,175,174
        .byte 0,0,80,80,5,37,16,0,5,37,80,2,1,0,80,82
        .byte 0,0,80,80,161,5,0,64,5,5,26,80,0,4,82,80
        .byte 0,0,82,90,32,160,80,80,0,0,82,90,32,160,80,80
        .byte 0,0,90,90,160,160,80,80,0,0,90,90,160,160,80,80
        .byte 0,0,5,5,80,82,1,0,80,82,5,32,16,0,5,37
        .byte 0,0,5,5,26,80,0,4,80,80,161,5,0,64,37,5
        .byte 0,0,37,165,2,10,5,5,0,0,37,165,2,10,5,5
        .byte 0,0,165,165,10,10,5,5,0,0,165,165,10,10,5,5
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
        .byte 0,0,170,170,85,85,170,170,0,0,0,0,0,0,0,0
        .byte 0,0,170,170,85,85,170,170,0,0,17,0,17,136,0,0
        .byte 0,102,102,238,102,238,102,238,102,238,102,238,102,136,102,102
        .byte 102,0,119,102,119,102,119,102,119,102,119,102,17,102,102,102
        .byte 0,0,175,175,95,95,175,175,95,95,175,175,95,95,175,175
        .byte 255,187,0,17,119,238,34,170,85,102,34,170,0,0,255,255
        .byte 136,34,119,221,136,34,187,34,170,34,136,34,119,221,136,34
        .byte 240,240,242,248,245,241,242,240,244,241,240,240,244,241,240,240
        .byte 240,240,250,250,240,240,248,242,240,240,248,242,240,240,250,242
        .byte 244,241,240,240,244,241,242,240,245,241,242,248,240,240,240,240
        .byte 240,240,248,242,240,240,248,242,240,240,250,250,240,240,240,240
        .byte 240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240
        .byte 240,240,248,250,240,240,248,250,240,240,248,250,240,240,248,250
        .byte 241,244,248,250,241,244,248,250,241,244,248,250,241,244,248,250
        .byte 0,0,119,238,0,0,119,238,102,102,119,238,51,204,0,0
        .byte 51,204,102,102,68,34,102,102,51,204,68,34,51,204,17,136
        .byte 102,102,238,221,187,221,204,51,119,238,0,0,17,0,17,136
        .byte 0,0,119,238,68,34,68,34,68,34,68,34,119,238,0,0
        .byte 0,0,119,119,0,0,119,255,119,255,119,255,119,255,68,255
        .byte 0,0,238,204,17,102,221,102,221,102,221,102,221,102,221,102
        .byte 68,255,119,255,119,255,119,255,119,255,119,255,0,0,119,119
        .byte 221,102,221,102,221,102,221,102,221,102,221,102,17,102,238,204
        .byte 95,95,175,175,95,95,175,175,95,95,175,175,0,0,175,175
        .byte 0,0,1,0,17,8,1,136,17,8,1,136,0,0,0,0
        .byte 17,95,35,175,87,95,39,175,87,95,39,175,19,95,4,175
        .byte 95,8,175,140,95,78,175,174,95,78,175,174,95,76,175,170
        .byte 87,0,39,174,19,78,4,174,87,0,39,174,19,78,4,174
        .byte 241,241,242,242,244,244,248,248,241,241,242,242,244,244,248,248
        .byte 85,68,34,170,85,68,34,170,85,68,34,170,85,68,34,170
        .byte 87,0,39,174,19,78,4,174,87,0,39,174,19,78,0,174
        .byte 85,68,34,170,85,68,34,170,85,68,34,170,85,68,34,0
        .byte 0,90,32,165,66,90,36,165,66,0,5,165,18,90,37,1
        .byte 0,0,0,0,64,0,96,0,80,0,80,0,176,0,208,64
        .byte 82,66,37,1,82,66,33,37,18,82,33,37,0,82,0,0
        .byte 224,96,112,32,176,64,80,96,144,96,208,0,192,0,0,0
        .byte 0,0,0,0,17,196,16,200,85,213,80,216,0,0,80,216
        .byte 0,0,0,0,33,0,18,64,161,36,26,66,0,36,26,10
        .byte 85,196,80,152,85,177,80,136,85,177,16,186,0,49,0,0
        .byte 33,132,24,74,133,164,16,8,133,128,88,8,132,0,0,0
        .byte 165,165,90,90,165,165,90,90,165,165,90,90,165,165,90,90
        .byte 0,0,0,0,0,0,51,204,119,238,68,34,102,102,34,68
        .byte 0,119,17,238,51,17,0,0,0,34,0,136,0,17,0,51
        .byte 243,248,254,251,252,253,248,245,245,242,246,244,244,248,243,252
        .byte 238,0,119,136,136,204,0,0,0,0,34,0,136,0,204,0
        .byte 0,17,0,34,0,119,0,102,0,68,0,204,0,0,0,0
        .byte 136,0,68,0,238,0,102,0,34,0,51,0,0,0,0,0
        .byte 240,246,242,253,245,251,245,251,252,251,252,241,249,248,240,240
        .byte 246,240,251,240,253,252,253,254,253,247,248,243,242,241,240,248
        .byte 0,0,0,0,0,0,51,136,85,68,119,204,102,204,51,136
        .byte 251,251,254,243,248,241,248,249,242,241,240,241,248,249,248,241
        .byte 248,245,250,241,248,241,248,241,248,241,248,241,252,243,255,255
        .byte 0,17,0,34,0,119,0,102,0,204,0,136,0,68,0,0
        .byte 0,80,16,96,0,80,0,176,0,48,32,96,0,144,0,32
        .byte 244,241,248,242,240,245,240,250,241,244,242,248,245,240,250,240
        .byte 0,0,50,250,68,160,80,98,117,245,80,34,117,245,64,170
        .byte 0,0,250,200,160,100,80,34,245,228,64,34,245,228,152,34
        .byte 117,245,80,16,117,245,64,34,117,245,64,136,49,245,0,0
        .byte 245,228,64,170,245,228,170,34,245,228,80,34,245,196,0,0
        .byte 160,245,136,216,213,245,216,250,213,177,216,216,128,245,170,250
        .byte 15,15,127,239,127,239,15,15,95,79,47,175,95,79,15,15
        .byte 252,243,251,253,246,246,245,250,245,250,246,246,251,253,252,243
        .byte 0,0,0,0,0,0,0,0,0,0,0,18,0,36,0,90
        .byte 0,0,0,0,0,0,16,136,245,245,186,216,117,228,250,250
        .byte 0,0,0,0,0,0,0,0,0,0,72,0,36,0,90,0
        .byte 0,161,16,82,1,37,16,82,32,165,2,90,32,165,2,26
        .byte 245,245,152,152,228,117,250,250,145,145,234,114,245,245,250,250
        .byte 133,0,74,8,164,128,74,8,165,4,90,64,165,4,88,64
        .byte 33,4,16,2,1,129,18,2,33,132,16,90,0,0,64,90
        .byte 245,245,114,234,49,196,186,216,17,128,250,250,0,0,250,250
        .byte 32,132,64,8,129,128,64,64,33,132,90,8,0,0,90,2
        .byte 0,0,0,0,0,0,0,1,0,87,1,175,17,78,35,171
        .byte 0,87,1,175,87,95,175,175,95,85,170,136,95,93,5,38
        .byte 36,160,10,10,160,5,88,64,133,4,88,80,128,36,64,2
        .byte 221,187,221,187,85,170,85,170,0,0,0,0,0,0,170,85
        .byte 5,36,80,80,160,5,2,26,32,161,10,26,36,1,64,2
        .byte 78,0,175,136,95,78,175,175,10,95,1,5,27,95,38,170
        .byte 0,0,0,0,0,0,136,0,78,0,175,136,87,8,141,140
        .byte 0,0,0,0,0,0,0,0,0,17,0,35,0,95,35,175
        .byte 85,78,175,1,78,95,141,4,78,8,141,1,8,19,5,38
        .byte 8,93,0,0,0,0,0,0,0,17,170,170,85,85,0,35
        .byte 0,32,64,18,161,1,8,24,0,0,90,0,1,128,0,72
        .byte 170,85,34,68,34,68,204,51,255,255,187,221,102,102,0,0
        .byte 4,0,72,2,128,133,24,16,0,0,0,90,1,128,18,0
        .byte 27,17,0,0,0,0,0,0,8,0,5,5,10,10,140,0
        .byte 87,10,136,175,95,87,34,171,17,87,136,171,76,17,38,170
        .byte 0,0,0,0,0,0,0,0,8,0,140,0,95,0,175,140
        .byte 0,17,0,51,0,51,0,102,0,85,0,221,0,153,0,170
        .byte 245,245,114,186,245,85,216,136,177,117,98,200,145,128,186,50
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 87,87,38,0,70,8,39,4,19,10,1,141,17,10,1,141
        .byte 18,72,0,0,16,8,4,32,0,0,132,33,2,64,0,0
        .byte 78,78,0,34,17,2,34,170,85,76,35,136,85,8,171,136
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 245,245,216,234,160,245,16,186,228,213,50,98,17,145,200,216
        .byte 128,0,200,0,196,0,98,0,160,0,186,0,145,0,80,0
        .byte 0,136,0,221,0,68,0,51,0,17,0,0,0,0,0,0
        .byte 238,102,102,68,51,136,0,136,136,204,238,102,51,187,0,119
        .byte 17,70,34,141,85,95,0,170,68,85,136,171,76,17,136,38
        .byte 8,17,136,1,70,70,136,1,68,2,170,5,17,8,39,174
        .byte 70,8,171,4,95,10,5,0,10,2,141,1,8,19,38,1
        .byte 100,117,34,98,17,196,16,0,49,17,98,114,213,196,234,0
        .byte 17,0,186,0,32,0,200,0,128,0,0,0,0,0,0,0
        .byte 0,19,0,39,0,95,1,170,19,85,35,136,70,0,38,4
        .byte 8,95,34,174,17,93,34,170,19,76,5,140,27,8,38,0
        .byte 95,17,39,4,27,8,5,4,19,76,35,170,17,93,0,38
        .byte 76,0,174,0,95,0,5,136,10,76,1,140,0,78,34,38
        .byte 2,8,1,137,17,0,0,170,0,76,0,34,0,17,0,1
        .byte 68,0,38,0,68,0,171,0,19,0,5,136,68,8,34,136
        .byte 0,2,0,38,0,2,0,140,0,76,1,170,17,2,1,4
        .byte 17,68,137,136,0,8,5,0,19,0,4,0,8,0,136,0
        .byte 0,32,48,192,96,0,144,0,64,32,144,48,192,112,240,128
        .byte 80,64,16,64,32,192,192,192,16,128,112,0,224,0,192,0
        .byte 32,160,32,128,48,64,48,48,16,128,0,224,0,112,0,48
        .byte 64,0,48,192,0,96,0,144,64,32,192,144,224,16,16,240
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 245,245,250,250,245,245,250,250,245,245,250,250,245,245,250,250
        .byte 90,90,242,250,94,91,240,250,95,215,240,248,95,91,242,250
        .byte 0,119,0,136,17,51,17,85,17,85,0,255,0,119,0,0
        .byte 238,0,17,0,204,136,170,136,170,136,255,0,204,0,0,0
        .byte 16,208,48,96,48,112,0,0,48,112,0,96,0,0,16,192
        .byte 176,128,96,192,224,192,0,0,224,192,96,0,0,0,48,128
        .byte 240,240,255,255,240,240,245,245,245,245,245,245,245,245,240,240
        .byte 15,15,175,175,95,95,175,175,15,15,175,175,95,95,175,175
bprop:
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
        .byte 2
        .byte 2
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
        .byte 2
        .byte 5
        .byte 6
        .byte 6
        .byte 6
        .byte 6
        .byte 6
        .byte 6
        .byte 2
        .byte 2
        .byte 6
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 6
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 0
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 0
        .byte 0
        .byte 5
        .byte 5
        .byte 5
        .byte 6
        .byte 5
        .byte 5
        .byte 5
        .byte 0
        .byte 0
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 0
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 6
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 6
        .byte 6
        .byte 2
        .byte 2
.out .sprintf("BLOCKS SIZE = %d", (* - chgfx))

sprgfx:
        .byte 3,128,7,192,7,0,99,128,89,196,60,8,37,144,91,176,89,104,46,192,3,0,12,128,13,128,0,0,13,128,14,192
        .byte 3,128,7,192,7,0,99,128,89,196,60,8,37,144,91,176,89,104,46,192,3,0,12,128,29,208,40,176,48,96,24,0
        .byte 3,128,7,192,7,0,99,128,89,196,60,8,37,144,91,176,89,104,46,192,3,0,24,128,109,192,96,0,64,192,0,224
        .byte 1,192,3,224,0,224,1,198,3,154,0,60,1,164,5,218,10,26,21,244,32,0,1,176,1,176,0,0,1,176,3,112
        .byte 1,192,3,224,0,224,1,198,3,154,0,60,1,164,5,218,10,58,21,244,32,0,1,112,11,184,13,20,6,12,0,24
        .byte 1,192,3,224,0,224,1,198,3,154,0,60,1,164,5,218,10,58,21,244,32,0,1,216,3,182,0,6,3,2,7,0
        .byte 3,192,7,224,4,32,54,96,122,88,120,60,67,180,48,8,55,252,27,128,4,32,0,192,12,96,28,0,0,96,0,112
        .byte 3,192,7,224,4,32,54,96,122,88,120,60,67,180,48,8,55,252,27,128,0,32,14,0,12,96,0,112,12,0,28,0
        .byte 3,192,7,224,4,32,54,96,122,88,120,60,67,180,48,8,55,252,27,128,0,32,12,64,0,0,12,96,28,112,0,0
        .byte 3,192,7,224,4,32,3,192,26,88,54,108,22,104,6,100,19,196,1,136,4,32,3,192,6,48,0,56,12,0,28,0
        .byte 3,192,7,224,4,32,3,192,26,88,54,108,22,104,6,100,19,196,1,136,4,32,3,208,12,0,28,48,0,56,0,0
        .byte 3,192,7,224,4,32,3,192,26,88,54,108,22,104,6,100,19,196,1,136,4,32,11,208,0,48,12,0,28,48,0,56
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,0,8,0
        .byte 0,16,0,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,32,8,0,28,5,8,2,0,5,0,0,0,0,0,0,0,16,32,56,0,16,0,0,0,0,0
        .byte 0,0,0,0,32,0,112,0,32,8,2,0,5,0,2,0,0,0,0,0,0,32,40,80,16,32,40,0,0,0,0,0
        .byte 0,0,0,0,80,0,32,0,80,0,0,0,2,0,0,0,0,0,0,0,0,80,0,32,16,80,0,0,0,0,0,0
        .byte 0,0,0,0,0,128,0,128,9,144,5,96,6,64,8,60,60,16,2,96,6,160,9,144,1,0,1,0,0,0,0,0
        .byte 0,128,64,130,49,140,61,188,18,72,21,168,10,80,244,44,52,47,10,80,21,168,18,72,61,188,49,140,65,2,1,0
        .byte 3,192,7,224,4,32,6,96,2,64,188,61,171,213,7,224,7,224,0,0,3,192,13,176,30,120,184,29,208,11,96,6
        .byte 96,96,192,0,160,96,112,32,56,96,25,174,21,219,13,195,13,195,21,219,25,174,56,96,112,32,160,96,192,0,96,96
        .byte 6,6,0,3,6,5,4,14,6,28,117,152,219,168,195,176,195,176,219,168,117,152,6,28,4,14,6,5,0,3,6,6
        .byte 0,0,1,192,26,224,59,168,122,108,97,206,100,22,55,242,38,54,18,34,5,84,6,176,14,48,12,96,6,64,28,96
        .byte 0,0,3,128,7,88,21,220,54,94,115,134,104,38,79,236,108,100,68,72,42,160,13,96,12,112,6,48,2,96,6,56
        .byte 1,192,2,160,3,224,10,40,25,204,60,30,119,246,102,50,50,38,37,82,22,180,7,112,6,48,12,48,24,24,28,56
        .byte 0,0,99,128,103,192,68,92,114,170,44,114,27,230,4,34,2,68,2,64,5,160,10,80,12,80,12,224,24,128,48,192
        .byte 0,0,3,140,7,196,4,66,2,134,60,90,91,252,100,32,20,32,50,64,7,192,6,96,12,80,24,80,0,32,0,56
        .byte 3,128,7,192,4,78,114,151,188,105,139,179,196,32,130,32,2,64,1,192,2,160,6,96,6,192,4,96,2,0,14,0
        .byte 0,0,0,0,0,0,0,0,7,224,63,252,115,254,237,253,243,249,127,226,56,28,7,224,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,3,192,7,224,13,240,26,232,29,232,31,232,31,200,15,144,4,32,3,192,0,0,0,0,0,0
        .byte 1,128,3,192,7,160,7,160,5,160,10,208,10,208,13,208,15,208,15,208,15,208,7,160,7,160,7,32,2,64,1,128
        .byte 0,0,0,0,0,0,3,192,7,224,13,240,26,232,29,232,31,232,31,200,15,144,4,32,3,192,0,0,0,0,0,0
        .byte 3,128,6,192,3,128,9,32,111,236,244,94,210,150,137,34,143,226,228,78,131,130,102,204,12,96,8,32,4,64,28,112
        .byte 3,128,6,192,3,128,105,44,239,238,212,86,210,150,137,34,207,230,164,74,67,132,6,192,12,96,28,32,0,64,0,112
        .byte 3,128,6,192,3,128,105,44,239,238,212,86,211,150,72,36,175,234,164,74,7,128,14,192,8,96,4,112,28,0,0,0
        .byte 3,128,6,192,3,128,9,32,111,236,244,94,211,150,136,34,207,230,164,74,67,132,6,192,12,32,8,64,4,112,28,0
        .byte 0,0,32,4,99,198,23,232,15,176,31,152,31,8,26,8,31,248,0,0,63,252,36,36,63,252,26,104,12,48,0,0
        .byte 0,0,4,32,13,208,1,224,15,176,31,152,31,8,26,8,31,248,0,0,63,252,34,20,63,252,18,72,12,48,0,0
        .byte 0,0,8,16,27,216,7,192,15,176,31,152,31,8,26,8,31,248,0,0,63,252,40,68,63,252,22,88,12,48,0,0
        .byte 0,0,103,6,71,2,7,128,3,128,2,12,1,190,26,222,114,88,113,128,112,64,0,192,1,224,64,226,96,230,0,0
        .byte 0,0,96,6,88,18,60,56,60,60,30,120,5,160,2,192,2,64,1,160,16,120,56,60,60,60,88,26,96,6,0,0
        .byte 0,0,96,70,64,114,0,240,48,224,120,64,125,128,26,192,2,88,1,190,0,28,0,12,15,0,78,2,102,6,0,0
        .byte 0,0,0,64,0,0,36,64,0,64,32,160,25,162,12,213,10,85,26,234,52,85,104,233,117,114,123,250,61,253,91,206
        .byte 0,0,72,64,28,200,24,160,53,162,56,208,144,106,40,210,53,148,26,226,44,84,104,234,116,218,53,235,59,246,30,238
        .byte 64,32,8,2,16,0,0,4,33,8,0,68,0,164,17,202,43,52,106,82,209,170,104,229,52,248,121,250,95,250,47,93
        .byte 0,0,127,126,0,0,127,126,67,66,123,122,122,122,122,250,126,158,126,190,66,242,122,58,123,122,123,122,127,126,0,0
        .byte 0,117,0,233,1,242,7,224,15,159,13,112,30,192,25,159,23,112,20,192,61,128,53,0,50,0,14,0,6,0,0,0
        .byte 1,255,3,123,3,245,6,216,5,183,13,108,9,152,10,179,8,230,13,100,4,56,3,8,1,140,0,230,0,59,0,7
        .byte 174,0,151,0,79,128,7,224,249,240,14,176,3,120,249,152,14,232,3,40,1,188,0,172,0,76,0,112,0,96,0,0
        .byte 255,128,222,192,175,192,27,96,237,160,54,176,25,144,205,80,103,16,38,176,28,32,16,192,49,128,103,0,220,0,224,0
        .byte 0,0,63,252,64,2,78,114,81,138,82,74,84,42,73,146,73,146,84,42,82,74,81,138,78,114,64,2,63,252,0,0
        .byte 0,0,63,252,64,2,67,194,68,34,75,210,86,106,84,42,84,42,86,106,75,210,68,34,67,194,64,2,63,252,0,0
        .byte 0,0,0,0,0,0,31,248,21,80,26,168,0,0,31,248,16,8,18,168,21,248,19,248,31,248,0,0,0,0,0,0
        .byte 7,0,15,128,14,0,103,0,89,128,60,0,37,200,91,124,88,160,47,64,0,0,29,128,25,128,0,0,48,192,56,224
        .byte 7,0,15,128,14,0,103,0,89,128,60,0,37,200,91,124,88,160,47,64,0,0,29,128,25,128,0,0,48,192,56,224
        .byte 7,0,15,128,14,0,103,0,89,128,60,0,37,200,91,124,88,160,47,64,0,0,29,128,25,128,0,0,48,192,56,224
        .byte 0,224,1,240,0,112,0,230,1,154,0,60,16,164,62,218,4,26,3,244,0,0,1,184,1,152,0,0,3,12,7,28
        .byte 0,224,1,240,0,112,0,230,1,154,0,60,16,164,62,218,4,26,3,244,0,0,1,184,1,152,0,0,3,12,7,28
        .byte 0,224,1,240,0,112,0,230,1,154,0,60,16,164,62,218,4,26,3,244,0,0,1,184,1,152,0,0,3,12,7,28
        .byte 3,192,7,224,4,32,54,96,122,88,120,60,67,188,55,132,35,48,16,96,11,128,8,96,40,96,8,0,0,96,0,112
        .byte 3,192,7,224,4,32,54,96,122,88,120,60,67,188,55,132,35,48,16,96,11,128,8,96,40,96,8,0,0,96,0,112
        .byte 3,192,7,224,4,32,54,96,122,88,120,60,67,188,55,132,35,48,16,96,11,128,8,96,40,96,8,0,0,96,0,112
        .byte 3,192,7,224,4,32,3,192,26,88,54,108,22,104,6,100,19,196,1,136,4,32,3,208,12,48,28,0,0,48,0,56
        .byte 3,192,7,224,4,32,3,192,26,88,54,108,22,104,6,100,19,196,1,136,4,32,3,208,12,48,28,0,0,48,0,56
        .byte 3,192,7,224,4,32,3,192,26,88,54,108,22,104,6,100,19,196,1,136,4,32,3,208,12,48,28,0,0,48,0,56
frmlst:
        .byte 0,3
        .byte 3,3
        .byte 6,3
        .byte 9,3
        .byte 12,1
        .byte 13,1
        .byte 14,1
        .byte 15,1
        .byte 16,3
        .byte 19,2
        .byte 21,3
        .byte 24,3
        .byte 27,3
        .byte 30,4
        .byte 34,4
        .byte 38,3
        .byte 41,3
        .byte 44,3
        .byte 47,1
        .byte 48,2
        .byte 50,2
        .byte 52,2
        .byte 54,1
        .byte 55,3
        .byte 58,3
        .byte 61,3
        .byte 64,3,67,0
.out .sprintf("SPRITES SIZE = %d", (* - sprgfx))

scdat:
        .word 330,306,290,238,217,267,245,233,161,181,228,150,178,272,264,226,270,247,246,266,281,241,256,267,251,260
        .word 298,197,255,214,284,242,259,171,79,257
        .byte 8,9,8,9,8,9,8,9,8,9,8,9,8,9,8,9,8,9,8,9,9,1,2,0,2,0,2,0,2,0,2,1,255,27,5
        .byte 8,9,8,8,1,255,0,9,1,255,0,4,27,9,8,9,9,1,255,0,4,10,255,0,4,1,255,0,4,27,8,3,4
        .byte 8,1,255,0,4,10,255,0,4,1,255,0,4,27,9,5,6,9,1,255,0,9,1,255,0,4,27,8,9,8,8,1,255,0,9
        .byte 1,255,0,4,27,9,8,9,9,1,255,0,4,10,255,0,4,1,255,0,4,27,8,7,8,8,1,255,0,4,10,255,0,4
        .byte 1,255,0,4,27,9,8,9,9,1,255,0,9,1,255,0,4,27,8,9,8,8,1,0,0,0,11,0,17,21,0,11,1
        .byte 255,0,4,27,9,8,9,9,1,0,0,0,12,15,18,22,25,12,1,216,0,0,0,27,3,4,8,8,1,0,0,0,13
        .byte 16,19,23,26,13,1,255,0,4,27,5,6,9,9,1,0,0,0,13,0,20,24,0,13,1,255,0,4,27,8,9,8,8
        .byte 1,0,0,0,14,255,0,4,14,1,255,0,4,27,9,8,9,9,1,255,0,4,10,255,0,4,1,255,0,4,27,8,9
        .byte 8,8,1,255,0,4,10,255,0,4,1,255,0,4,27,9,8,7,9,1,255,0,9,1,255,0,4,27,8,9,8,8,1
        .byte 255,0,9,1,255,0,4,27,9,8,9,9,1,255,0,4,10,255,0,4,1,255,0,4,27,8,7,8
        .byte 8,1,255,0,4,10,255,0,4,1,255,0,4,27,9,8,9,9,1,255,0,9,1,255,0,4,27,8,7,8,8,1,255,0,9
        .byte 1,255,0,4,27,9,8,9,9,1,255,0,4,10,255,0,4,1,255,0,4,27,8,9,8,8,1,255,0,4,10,255,0,4
        .byte 1,255,0,4,27,9,8,9,9,1,255,0,9,1,255,0,4,27,8,3,4,8,1,255,0,9,1,255,0,4,27,9,5
        .byte 6,9,1,255,0,4,10,255,0,4,1,255,0,4,27,8,9,8,8,1,255,0,4,10,255,0,4,1,255,0,4,255,27,4
        .byte 9,1,255,0,9,1,255,0,8,8,1,255,0,9,1,255,0,8,9,1,0,0,41,0,0,41,41,41,0,1,255,0,8
        .byte 8,1,0,0,41,0,0,41,0,41,0,1,255,0,8,9,1,0,0,41,0,0,41,0,41,0,1,255,0,4,255,27,4
        .byte 8,1,0,0,41,0,0,41,41,41,0,1,255,0,4,27,9,8,7,9,1,255,0,9,1,255,0,4,27,8,9,8,8
        .byte 1,255,10,9,1,255,27,5,9,8,9,9,8,9,8,9,8,9,8,9,8,9,8,9,8,9,8,9,3,4,8,8,7
        .byte 8,9,8,3,4,9,8,9,8,9,8,7,8,9,8,5,6,9,9,8,9,8,9,5,6,8,9,8,7,8,9,8,9
        .byte 8,9,8,9,8
        .byte 255,37,20,28,29,30,31,28,29,30,31,28,29,30,31,28,29,30,31,28,29,37,37,32,33,34,35,32,33,34,35,32,33
        .byte 34,35,32,33,34,35,32,33,37,37,8,9,8,9,8,9,8,9,8,9,8,9,8,9,8,9,8,9,37,37,9,8,9
        .byte 3,4,8,9,8,9,8,9,8,9,8,9,8,9,8,37,37,8,9,8,5,6,9,8,9,3,4,8,255,36,7,37,37
        .byte 9,7,9,8,9,8,9,8,5,6,9,36,255,0,5,36,37,37,8,9,8,9,8,9,8,9,8,9,8,36,255,0,5
        .byte 36,106,46,255,27,11,36,255,0,5,36,106,106,255,0,11,36,255,0,5,36,36,36,255,0,11,36,255,0,19,36,255,0,19
        .byte 36,255,0,8,255,27,4,255,0,4,27,27,27,36,255,0,5,36,37,37,8,9,8,27,255,0,4,27,9,8,36,255,0,5
        .byte 36,37,37,9,8,9,27,255,0,4,27,8,9,36,255,0,5,36,37,37,3,4,8,27,255,0,4,27,9,8,36,255,0,5
        .byte 36,37,37,5,6,9,27,255,0,4,27,8,9,255,36,7,37,37,8,9,8,27,255,0,4,27,9,8,9,8,9,8,9
        .byte 8,9,37,37,9,8,9,27,255,0,4,27,8,7,8,9,8,9,8,9,8,37,37
        .byte 8,9,8,27,255,0,4,27,9,8,9,8,255,39,7,9,27,27,27,255,0,4,27,27,27,8,9,255,39,7,8,27,255,0,8
        .byte 27,9,8,39,40,40,37,37,40,39,9,27,255,0,8,27,8,9,39,40,40,89,90,40,39,39,39,255,0,8,255,39,4
        .byte 0,0,91,92,0,39,39,39,255,0,8,255,39,4,255,0,5,39,39,39,255,0,8,40,156,157,40,255,0,5,39,39,39
        .byte 255,36,8,40,158,159,40,255,0,5,39,39,39,255,0,17,39,39,39,255,0,9,11,255,0,7,39,39,39,101,255,0,8
        .byte 12,15,18,22,25,0,0,0,39,39,39,101,255,0,8,13,16,19,23,26,0,0,0,39,39,39,101,255,0,8,13,0,20
        .byte 24,0,0,0,11,39,39,39,103,255,0,8,14,255,0,6,12,39,39,39,103,255,0,11,17,21,0,0,13,39,39,39,255,0,16
        .byte 13,39,39,39,255,0,16,14,39,39,39,93,94,97,98,255,0,13,39,39,39,95,96,99,100,0,0,255,76,4,255,0,7
        .byte 255,39,21
        .byte 255,37,8,255,0,4,255,37,9,38,38,38,72,73,38,38,255,0,4,38,38,72,73,38,38,38,37,37,38,38,38,74,75
        .byte 38,38,255,0,4,38,38,74,75,38,38,38,37,37,255,36,7,255,0,4,255,36,7,37,37,36,255,0,16,36,37,37,36
        .byte 255,0,16,36,37,37,36,255,0,16,36,37,106,36,255,0,16,36,106,106,36,255,0,4,255,37,8,255,0,4,36,106,36,36
        .byte 255,0,4,37,194,0,37,37,194,0,37,255,0,12,37,0,190,37,37,0,190,37,255,0,12,255,37,8,255,0,12,38,38,38
        .byte 47,48,38,38,38,255,0,6,37,36,255,0,4,38,38,38,49,50,38,38,38,255,0,4,36,37,37,36,255,0,16,36,37,37
        .byte 36,255,0,12,142,0,0,0,36,37,37,36,255,0,11,143,144,145,0,0,36,37,37,36,255,0,11,146,0,147,0,0,36
        .byte 37,37,36,255,0,16,36,255,37,9,255,0,4,255,37,8
        .byte 37,37,37,0,0,255,37,16,38,38,0,0,38,38,37,38,108,108,38,37,113,113,38,108,108,108,37,37,58,59,0,0,58
        .byte 59,37,78,107,107,78,37,109,110,38,76,107,43,37,37,60,61,0,0,60,61,37,79,82,82,79,37,111,112,0,77,81,43
        .byte 37,37,255,0,6,37,0,84,84,0,37,255,0,4,84,76,37,37,255,0,6,106,255,0,4,106,255,0,5,77,37,37,255,0,6
        .byte 106,255,0,4,106,255,0,6,37,37,255,0,18,37,37,255,0,18,37,37,255,0,6,37,255,0,4,37,255,0,6,37,37
        .byte 255,0,6,37,255,0,4,37,255,0,6,255,37,6,0,0,255,37,8,0,0,255,37,4,38,54,55,38,0,0,37,38,38,38
        .byte 46,38,38,38,0,0,38,38,37,37,38,56,57,38,0,0,37,255,38,7,0,0,38,38,37,37,255,0,6,37,0,0,80
        .byte 255,0,8,37,106,138,83,255,0,4,37,0,76,148,149,76,255,0,4,41,83,37,106,140,84,255,0,4,37,0,77,0,0
        .byte 77,255,0,4,78,84,37,255,0,7,37,255,0,9,79,0,37,255,0,7,37,255,0,11,255,37,9,255,0,4,255,37,8
        .byte 255,37,21,255,38,6,54,55,38,37,51,38,38,47,48,38,38,38,37,37,38,38,58,59,38,38,56,57,38,37,52,38,38
        .byte 49,50,38,38,38,37,37,0,0,60,61,0,255,76,4,37,53,255,0,7,37,37,138,83,0,0,0,77,82,82,77,37,255,0,8
        .byte 37,37,140,84,255,0,4,84,84,0,37,255,0,8,37,37,255,0,9,255,37,7,0,0,37,37,255,0,9,38,38,38,156
        .byte 157,38,38,0,0,37,37,255,0,9,38,38,38,158,159,38,38,0,0,37,37,255,0,4,37,255,0,13,37,37,0,150,0,0
        .byte 37,255,0,13,37,37,143,144,145,0,255,37,16,146,0,147,0,37,38,65,66,67,38,38,46,38,38,38,108,38,38,37,37
        .byte 255,0,4,37,38,68,69,70,255,38,5,78,107,78,38,37,37,255,0,4,37,0,71,69,70,255,0,5,79,80,79,0,37,37
        .byte 255,0,4,106,255,0,10,84,0,0,106,37,255,0,4,106,255,0,6,37,255,0,6,106,37,255,0,11,37,255,0,7,37
        .byte 255,0,11,37,255,0,7,255,37,20
        .byte 255,37,8,255,0,4,255,37,9,255,42,4,119,42,37,255,0,4,38,108,38,38,38,113,113,37,37,42,116,42,42,118,42
        .byte 37,255,0,4,43,76,38,38,38,109,110,37,37,0,117,255,0,4,106,255,0,4,43,83,0,0,0,111,112,37,37,255,0,6
        .byte 106,255,0,4,76,84,255,0,5,37,37,255,0,11,77,255,0,6,37,37,255,0,18,255,37,9,255,0,9,85,86,37,37
        .byte 38,38,38,46,38,38,38,255,0,9,87,88,37,37,255,38,7,255,0,11,37,37,85,86,255,0,10,80,255,0,5,37,37
        .byte 87,88,255,0,8,81,76,76,76,83,0,0,0,37,37,255,0,10,84,77,82,77,84,0,0,0,37,37,255,0,12,84,255,0,5
        .byte 37,37,0,0,0,81,78,78,83,255,0,11,37,37,0,0,0,84,79,79,84,255,0,9,124,83,37,37,255,0,16,125,84
        .byte 37,37,85,86,255,0,9,82,255,0,4,85,86,37,37,87,88,255,0,7,81,39,39,39,83,0,0,87,88,255,37,21
        .byte 255,37,21,255,38,4,54,55,255,38,7,54,55,255,38,4,37,255,38,4,56,57,255,38,7,56,57,255,38,4,37,85,86
        .byte 255,0,17,37,87,88,255,0,17,37,255,0,19,37,255,0,19,106,255,0,8,62,63,64,255,0,8,106,255,0,8,65,66
        .byte 67,255,0,14,62,63,64,68,69,70,62,63,64,255,0,11,65,66,67,71,69,70,65,66,67,255,0,11,68,69,70,62,63
        .byte 64,68,69,70,255,0,11,71,69,70,65,66,67,71,69,70,255,0,5,37,255,0,8,68,69,70,255,0,8,37,255,0,8
        .byte 71,69,70,255,0,8,37,255,0,19,37,255,0,19,37,85,86,255,0,17,37,87,88,255,0,17,255,37,20
        .byte 255,37,8,255,0,4,255,37,8,255,38,4,156,157,38,38,255,0,4,38,38,47,48,38,38,37,37,255,38,4,158,159,38,38
        .byte 255,0,4,38,38,49,50,38,38,37,37,255,0,16,85,86,37,37,255,0,16,87,88,38,38,255,0,18,38,38,255,0,44
        .byte 255,39,4,255,0,4,255,39,4,255,0,8,39,194,0,39,255,0,4,39,194,0,39,255,0,8,39,0,190,39,255,0,4
        .byte 39,0,190,39,255,0,8,255,39,4,255,0,4,255,39,4,255,0,8,255,40,4,255,0,4,255,40,4,255,0,8,255,40,4
        .byte 255,0,4,255,40,4,255,0,62,37,37,255,0,4,85,86,255,0,6,85,86,255,0,4,37,37,255,0,4,87,88,255,0,6
        .byte 87,88,255,0,4,255,37,16,255,0,4,37,37
        .byte 8,9,8,9,7,9,8,9,8,9,8,9,8,9,3,4,8,9,8,9,9,8,9,8,9,8,9,8,9,8,9,8,9
        .byte 8,5,6,9,8,7,8,255,37,40,38,38,72,73,38,38,38,46,38,37,37,38,46,38,38,38,72,73,255,38,4,74,75
        .byte 255,38,5,37,37,255,38,5,74,75,38,38,255,0,6,36,36,36,37,37,36,36,36,255,0,12,36,0,0,37,37,0,0
        .byte 36,255,0,12,36,0,0,106,106,0,0,36,255,0,12,36,0,0,106,106,0,0,36,255,0,12,36,255,0,6,36,255,0,12
        .byte 36,255,0,6,36,255,0,12,36,0,0,37,37,0,0,36,255,0,12,36,0,0,37,37,0,0,36,255,0,12,36,0,0
        .byte 37,37,0,0,36,255,0,12,36,0,0,37,37,0,0,36,255,0,6,255,37,40,28,30,31,28,29,30,31,28,29,30,31
        .byte 28,29,30,31,28,29,30,31,28,32,34,35,32,33,34,35,32,33,34,35,32,33,34,35,32,33,34,35,32
        .byte 255,37,8,255,0,4,255,37,8,38,38,54,55,255,38,4,255,0,4,255,38,4,156,157,255,38,4,56,57,255,38,4,255,0,4
        .byte 255,38,4,158,159,38,38,255,0,65,62,63,64,255,0,4,62,63,64,255,0,10,65,66,67,255,0,4,65,66,67,255,0,10
        .byte 68,69,70,255,0,4,68,69,70,255,0,10,71,69,70,255,0,4,71,69,70,255,0,50,62,63,64,255,0,4,62,63,64
        .byte 255,0,10,65,66,67,255,0,4,65,66,67,255,0,10,68,69,70,255,0,4,68,69,70,255,0,10,71,69,70,255,0,4
        .byte 71,69,70,255,0,65,255,37,8,255,0,4,255,37,8
        .byte 37,37,255,0,4,255,37,14,38,38,255,0,4,255,38,8,37,37,255,38,6,255,0,4,38,38,38,58,59,38,38,38,37,37
        .byte 255,38,4,255,0,9,60,61,0,85,86,37,37,255,0,16,87,88,37,37,255,0,18,37,37,255,0,10,62,63,64,255,0,5
        .byte 37,37,255,0,10,65,66,67,255,0,5,37,37,255,0,10,68,69,70,255,0,5,106,106,255,0,10,71,69,70,255,0,5
        .byte 106,106,255,0,50,62,63,64,255,0,5,37,37,255,0,10,65,66,67,255,0,5,37,37,255,0,10,68,69,70,255,0,5
        .byte 37,37,255,0,10,71,69,70,255,0,5,37,37,255,0,18,37,37,255,0,16,85,86,37,37,255,0,16,87,88,37,37,255,0,4
        .byte 255,37,8,255,0,4,255,37,8
        .byte 255,41,21,45,45,54,55,45,45,41,45,156,157,45,45,47,48,45,51,51,51,41,41,45,45,56,57,45,45,41,45,158,159
        .byte 45,45,49,50,45,52,52,52,41,41,255,0,6,45,255,0,8,53,53,53,41,41,138,83,255,0,4,45,255,0,11,41,41
        .byte 140,84,255,0,16,41,41,255,0,18,41,41,255,0,6,255,41,8,0,0,41,41,41,45,255,0,6,41,45,45,45,46,45,45,45
        .byte 0,0,45,45,41,45,0,0,0,106,106,104,41,255,45,7,0,0,45,45,41,255,0,4,103,102,103,41,255,0,9,85,86
        .byte 41,255,0,7,41,255,0,9,87,88,41,41,0,0,0,106,106,104,41,255,0,11,41,41,0,0,0,103,102,103,41,255,0,6
        .byte 80,0,43,0,0,41,41,255,0,6,41,41,0,0,41,41,0,84,0,43,83,0,41,41,0,0,0,106,106,104,41,45,0,0
        .byte 45,41,0,81,0,76,84,0,41,41,0,0,0,103,102,103,41,45,0,0,45,41,0,84,0,77,0,0,41,41,255,0,6
        .byte 41,255,0,4,41,255,0,4,85,86,41,41,0,0,0,106,106,104,41,255,0,4,41,255,0,4,87,88,255,41,9,255,0,4
        .byte 255,41,8
        .byte 255,41,21,42,42,42,119,42,41,42,42,42,119,42,41,45,45,45,51,51,51,41,41,116,42,42,118,42,41,116,42,42,118
        .byte 42,41,45,45,45,52,52,52,41,41,117,255,0,4,41,117,255,0,4,41,0,0,0,53,53,53,41,41,0,0,0,150,0
        .byte 41,255,0,5,41,255,0,6,41,41,120,121,143,144,145,41,120,121,0,0,0,41,255,0,6,41,41,122,123,146,0,147,45
        .byte 122,123,0,0,0,45,0,0,0,106,106,104,41,41,255,0,5,45,255,0,5,45,0,0,0,103,102,103,41,41,255,0,18
        .byte 45,41,81,43,255,0,16,45,41,84,76,255,0,17,41,0,77,255,0,17,255,41,4,0,0,255,41,7,0,0,0,106,106
        .byte 104,41,41,45,45,45,0,0,255,45,4,2,45,41,0,0,0,103,102,103,41,41,45,108,45,0,0,255,45,6,41,255,0,6
        .byte 41,41,124,107,124,255,0,5,155,133,134,41,0,0,0,106,106,104,41,41,125,82,125,255,0,5,155,135,136,41,0,0,0
        .byte 103,102,103,41,41,0,84,255,0,6,155,155,155,41,255,0,6,41,41,255,0,11,41,0,0,0,106,106,104,255,41,21
        .byte 255,39,14,255,0,4,39,39,39,40,37,37,40,72,73,40,46,40,40,51,51,39,255,0,4,40,39,39,40,89,90,40,74
        .byte 75,255,40,4,52,52,39,255,0,4,40,39,39,0,91,92,255,0,7,53,53,39,39,0,0,255,39,4,255,0,12,40,40
        .byte 0,0,40,40,39,39,255,0,12,40,40,0,0,40,40,39,39,255,0,18,39,39,255,0,4,101,101,161,101,255,0,10,39,39
        .byte 255,0,4,103,102,102,103,255,0,10,39,39,255,0,4,103,102,102,103,255,0,10,39,39,255,0,18,39,39,255,0,7,255,101,4
        .byte 0,0,0,97,98,93,94,39,39,97,98,255,0,5,103,102,102,103,0,0,0,99,100,95,96,39,39,99,100,255,0,5,103
        .byte 102,102,103,0,97,98,93,94,97,98,39,39,255,0,12,99,100,95,96,99,100,39,39,255,0,16,93,94,39,39,255,0,16
        .byte 95,96,39,39,0,0,93,94,255,0,13,155,39,39,0,0,95,96,255,0,13,155,255,39,21
        .byte 255,39,5,255,0,4,255,39,4,255,141,6,39,39,255,40,4,255,0,4,40,40,39,39,255,141,6,39,39,40,40,108,40
        .byte 255,0,4,40,40,39,39,255,141,6,39,39,41,78,107,78,255,0,4,133,134,39,39,255,141,6,39,39,41,0,82,79,255,0,4
        .byte 135,136,39,39,255,141,6,39,39,41,83,84,255,0,7,39,39,255,141,6,39,39,78,84,255,0,6,133,134,39,39,255,141,6
        .byte 39,39,79,255,0,7,135,136,39,39,255,141,6,39,39,255,0,10,39,39,255,141,6,39,39,255,0,8,133,134,39,39,255,141,4
        .byte 0,141,39,39,255,0,8,135,136,39,39,141,0,141,141,0,0,255,39,4,0,0,255,39,8,0,0,141,0,0,0,39,39
        .byte 40,40,0,0,40,40,39,40,40,46,40,40,255,0,6,39,39,40,40,0,0,40,40,39,255,40,5,255,36,6,39,40,255,0,6
        .byte 39,255,0,11,39,40,0,0,0,142,0,0,39,255,0,11,39,40,0,0,143,144,145,0,39,255,0,8,255,39,4,0,0,0
        .byte 146,0,147,0,39,255,0,8,39,194,0,39,255,0,7,39,255,0,8,39,0,190,255,39,9,255,0,4,255,39,8
        .byte 8,9,8,3,4,9,8,9,8,9,7,9,8,9,8,9,8,9,8,39,9,8,9,5,6,8,9,8,9,8,9,8,3
        .byte 4,9,8,7,8,9,39,8,9,8,9,8,9,8,7,8,9,8,9,5,6,8,9,8,9,8,255,39,43,40,40,255,101,4
        .byte 40,40,39,40,40,37,37,255,40,4,39,39,39,40,40,103,102,102,103,40,40,39,40,40,89,90,40,58,59,40,39,39,39
        .byte 0,0,103,102,102,103,0,0,40,0,0,91,92,0,60,61,0,39,39,39,255,0,8,40,255,0,8,39,39,39,255,0,17
        .byte 39,39,39,255,0,17,39,39,39,93,94,255,0,6,39,255,0,8,39,39,39,95,96,255,0,6,39,255,0,8,39,39,39
        .byte 255,0,8,39,255,0,8,39,39,39,255,0,8,39,255,0,8,39,39,39,255,0,4,97,98,93,94,39,255,0,8,40,39,39
        .byte 255,0,4,99,100,95,96,39,43,255,0,7,40,39,39,255,0,6,97,98,39,160,83,0,80,255,0,5,39,39,255,0,6
        .byte 99,100,39,76,76,148,149,76,255,0,4,255,39,20
        .byte 255,39,23,40,40,72,73,40,255,39,4,40,72,73,40,40,255,39,6,40,40,74,75,40,12,0,0,12,40,74,75,40,40
        .byte 255,39,6,255,0,4,36,12,0,0,12,255,141,5,255,39,4,40,40,255,0,4,36,36,36,255,141,7,40,40,39,39,40,40
        .byte 255,0,6,255,141,8,40,40,39,39,255,0,10,255,141,8,39,39,255,0,11,255,141,7,39,39,137,138,255,0,6,133,134
        .byte 39,39,255,141,6,39,39,139,140,255,0,6,135,136,39,39,255,141,6,39,39,255,0,10,39,39,255,141,6,39,39,137,138
        .byte 255,0,6,133,134,39,39,255,141,6,39,39,139,140,255,0,6,135,136,39,39,255,141,6,39,39,255,0,10,39,39,255,141,6
        .byte 39,39,137,138,255,0,6,133,134,39,39,255,141,6,39,39,139,140,255,0,6,135,136,39,39,255,141,6,39,39,255,0,10
        .byte 39,39,255,141,6,39,39,137,138,255,0,6,133,134,39,39,255,141,6,39,39,139,140,255,0,6,135,136,39,39,255,141,6
        .byte 255,39,6,255,0,4,255,39,4,255,141,6,39
        .byte 255,43,8,255,0,4,255,43,9,44,44,44,37,37,44,44,255,0,4,255,44,6,101,43,43,44,44,44,89,90,44,44,255,0,4
        .byte 44,37,37,44,44,41,101,43,43,85,86,0,91,92,255,0,7,37,37,0,0,162,101,43,43,87,88,255,0,10,37,37,0,0
        .byte 41,101,43,43,255,0,12,37,37,0,0,162,101,43,43,0,0,255,80,4,255,0,6,37,37,0,0,41,101,43,43,0,255,78,6
        .byte 255,0,5,37,37,0,0,162,101,43,43,0,79,255,82,4,79,255,0,5,37,37,0,0,41,101,43,43,0,0,255,84,4
        .byte 255,0,6,37,37,0,0,162,101,43,43,255,0,12,37,37,0,0,41,101,43,43,255,0,12,37,37,0,0,162,101,43,43
        .byte 0,0,255,80,4,255,0,6,37,37,0,0,41,101,43,43,0,255,78,6,255,0,5,37,37,0,0,162,101,43,43,0,79
        .byte 255,82,4,79,255,0,5,37,37,0,0,41,101,43,43,0,0,255,84,4,255,0,6,124,124,0,0,41,40,43,43,255,0,12
        .byte 125,125,0,0,40,40,43,43,85,86,255,0,16,43,43,87,88,255,0,16,255,43,9,255,0,4,255,43,8
        .byte 255,43,8,255,0,4,255,43,9,255,44,4,51,51,43,255,0,4,43,51,51,255,44,4,43,43,105,40,40,44,52,52,43
        .byte 255,0,4,43,52,52,44,40,40,104,43,43,103,102,103,0,53,53,43,255,0,4,43,53,53,0,103,102,103,43,43,103,102
        .byte 103,0,142,0,43,255,0,4,43,0,0,0,103,102,103,43,43,0,0,0,143,144,145,44,255,0,4,44,255,0,6,43,43
        .byte 0,0,0,146,0,147,44,255,0,4,44,255,0,6,43,43,0,80,255,0,14,80,0,43,43,41,41,41,83,255,0,10,81
        .byte 41,41,41,255,43,9,255,0,4,255,43,16,255,0,4,255,43,9,255,44,4,54,55,44,255,0,4,44,54,55,255,44,4
        .byte 43,43,41,41,41,44,56,57,44,255,0,4,44,56,57,44,41,41,41,43,43,78,78,78,83,255,0,10,81,78,78,78,43,43
        .byte 79,82,79,84,255,0,10,84,79,82,79,43,43,0,84,255,0,4,43,255,0,4,43,255,0,4,84,0,43,43,255,0,6
        .byte 43,255,0,4,43,255,0,6,43,43,255,0,6,43,255,0,4,43,255,0,6,43,43,105,40,40,0,51,51,43,255,0,4
        .byte 43,51,51,0,40,40,104,255,43,9,255,0,4,255,43,8
        .byte 255,43,8,255,0,4,255,43,9,44,44,156,157,44,46,44,255,0,4,43,51,51,255,44,4,43,43,44,44,158,159,44,44,44
        .byte 255,0,4,43,52,52,44,40,40,104,43,43,85,86,255,0,9,43,53,53,0,103,102,103,43,43,87,88,255,0,9,43,0,0,0
        .byte 103,102,103,43,43,255,0,11,44,255,0,6,43,43,0,81,37,37,83,255,0,6,44,255,0,6,43,43,0,84,37,37,84
        .byte 255,0,11,80,0,43,43,0,81,37,37,83,255,0,9,81,41,148,149,43,43,0,84,124,124,84,255,0,6,255,43,9,0,0
        .byte 125,125,255,0,7,255,43,9,85,86,255,0,9,44,54,55,255,44,4,43,43,87,88,255,0,9,44,56,57,44,41,41,41
        .byte 255,43,9,255,0,7,81,78,78,78,255,43,9,255,0,7,84,79,82,79,43,255,44,8,255,0,4,43,255,0,4,84,0
        .byte 43,255,44,8,255,0,4,43,255,0,6,43,255,0,12,43,255,0,6,43,255,0,12,43,51,51,0,40,40,104,255,43,9
        .byte 255,0,4,255,43,8
        .byte 8,7,8,9,8,9,8,9,8,9,8,9,7,9,8,9,3,4,8,9,9,8,9,8,9,3,4,8,9,7,9,8,9
        .byte 8,9,8,5,6,9,8,8,9,8,9,8,5,6,9,8,9,8,9,8,9,8,9,8,9,8,9,255,43,42,224,224,113,113
        .byte 224,54,55,224,113,113,224,224,43,42,119,42,42,43,43,43,223,223,109,110,223,56,57,223,109,110,223,223,43,42,118,42,116
        .byte 43,43,43,0,0,111,112,255,0,4,111,112,0,0,43,0,0,0,117,43,43,43,255,0,12,224,255,0,4,43,43,43,138
        .byte 255,0,11,223,255,0,4,43,43,43,140,255,0,4,80,255,0,11,43,43,43,0,0,114,113,37,37,37,113,115,255,0,7
        .byte 150,43,43,43,0,0,114,113,106,106,106,113,115,0,0,0,43,0,0,153,151,43,43,43,0,0,114,255,113,5,115,0,0,0
        .byte 43,0,0,154,152,43,43,43,255,0,4,82,0,82,255,0,5,255,43,8,255,0,4,84,0,84,255,0,5,255,224,6,43,43
        .byte 255,0,12,255,223,6,43,43,255,0,18,43,43,105,40,40,255,0,15,255,43,20
        .byte 255,41,8,255,0,4,255,41,9,255,42,4,41,42,42,255,0,4,41,42,119,41,42,119,41,41,41,255,42,4,41,42,42
        .byte 255,0,4,41,116,118,41,116,118,41,41,41,255,0,4,41,255,0,6,41,117,0,41,117,0,41,41,41,255,0,4,41,255,0,6
        .byte 42,0,0,42,0,0,41,41,41,120,121,0,0,41,255,0,6,42,0,0,42,0,0,41,41,41,122,123,0,0,41,255,0,12
        .byte 41,41,41,255,0,4,41,255,0,12,41,41,41,255,0,4,41,255,0,12,41,41,41,120,121,0,0,41,255,0,6,255,41,9
        .byte 122,123,0,0,41,255,0,6,41,42,119,41,42,119,41,41,41,255,0,4,41,41,41,255,0,4,41,116,118,41,116,118,41,41,41
        .byte 255,0,4,51,51,51,255,0,4,41,117,0,41,117,0,41,41,41,120,121,0,0,52,52,52,255,0,4,42,0,0,42,0,0
        .byte 41,41,41,122,123,0,0,53,53,53,255,0,4,42,0,0,42,0,0,41,41,41,255,0,9,142,255,0,7,41,41,41,255,0,8
        .byte 143,144,145,255,0,6,41,41,41,255,0,8,146,0,147,255,0,6,41,41,41,255,0,17,255,41,22
        .byte 43,255,0,4,255,43,5,255,0,4,255,43,7,255,0,4,43,255,44,4,255,0,4,255,44,5,43,43,255,0,4,43,255,44,4
        .byte 255,0,4,255,44,5,43,43,255,0,4,43,85,86,255,0,9,85,86,43,43,255,0,4,43,87,88,255,0,9,87,88,43,43
        .byte 255,0,4,43,255,0,13,43,43,255,0,4,43,255,0,7,80,255,0,5,43,43,255,0,4,43,255,0,5,81,37,37,37
        .byte 83,0,0,0,43,43,0,0,97,98,43,255,0,5,84,37,37,37,84,0,0,0,43,43,0,0,99,100,43,255,0,5,81
        .byte 37,37,37,83,0,0,0,43,43,0,0,93,94,43,255,0,5,84,37,37,37,84,0,0,0,43,43,0,0,95,96,43,255,0,5
        .byte 81,37,37,37,83,0,0,0,255,43,7,255,0,5,84,124,124,124,84,0,0,0,43,43,255,44,5,255,0,6,125,82,125
        .byte 255,0,4,43,43,255,44,5,255,0,7,84,255,0,5,43,43,255,0,18,43,43,255,0,18,43,43,255,0,16,85,86,43,43
        .byte 255,0,16,87,88,43,43,43,255,0,4,255,43,14
        .byte 255,43,15,217,217,255,43,4,255,44,4,54,55,44,44,101,101,101,44,44,44,218,218,44,44,43,43,255,44,4,56,57,44,44
        .byte 41,41,41,44,44,44,218,218,44,44,43,43,255,0,7,80,0,80,0,80,255,0,4,85,86,43,43,255,0,4,114,113,255,37,7
        .byte 113,115,0,87,88,43,43,255,0,4,114,113,255,106,7,113,115,0,0,0,43,43,255,0,4,114,255,113,9,115,0,0,0
        .byte 43,43,255,0,18,43,43,255,0,18,43,43,255,0,4,82,0,82,0,82,0,82,0,82,0,82,0,0,0,43,43,255,0,4
        .byte 84,0,84,0,84,0,84,0,84,0,84,0,0,0,43,43,255,0,4,82,0,82,0,82,0,82,0,82,0,82,0,0,0
        .byte 43,43,255,0,4,84,0,84,0,84,0,84,0,84,0,84,0,0,0,43,43,255,0,4,82,0,82,0,82,0,82,0,82
        .byte 0,82,0,0,0,43,43,255,0,4,84,0,84,0,84,0,84,0,84,0,84,0,0,0,43,43,255,0,18,43,43,255,0,18
        .byte 43,43,255,0,18,43,43,255,0,18,43,43,255,0,4,255,43,5,255,0,4,255,43,6
        .byte 255,39,8,255,0,4,255,39,9,40,37,37,40,51,51,40,255,0,4,40,255,101,4,40,40,39,39,40,89,90,40,52,52
        .byte 40,255,0,4,40,103,102,102,103,40,40,39,39,0,91,92,0,53,53,36,255,0,4,36,103,102,102,103,0,0,39,39,255,0,6
        .byte 36,255,0,4,36,255,0,6,39,39,255,0,6,36,255,0,4,36,255,0,6,39,39,255,0,6,36,255,0,4,36,255,0,6
        .byte 39,39,0,0,0,43,43,0,36,255,0,4,36,0,43,43,0,0,0,39,39,0,0,0,43,161,0,36,255,0,4,36,0
        .byte 43,43,0,0,0,39,39,0,0,0,43,43,0,93,94,0,0,0,36,0,43,160,0,0,0,39,39,0,0,0,43,43,0
        .byte 95,96,0,0,0,36,0,43,43,0,0,0,39,39,0,0,0,160,43,0,36,255,0,4,36,0,161,43,0,0,0,39,39
        .byte 0,0,0,43,43,0,36,97,98,0,0,36,0,43,161,0,0,0,39,39,0,0,0,76,76,0,36,99,100,0,0,36,0
        .byte 76,76,0,0,0,39,39,0,0,0,77,77,0,36,255,0,4,36,0,77,77,0,0,0,39,39,255,0,6,36,255,0,4
        .byte 36,255,0,6,39,39,255,36,7,255,0,4,255,36,7,39,39,255,0,18,39,39,255,0,18,39,39,255,0,18,39
        .byte 39,255,0,18,39,39,255,0,18,39,39,255,0,18,39,39,255,101,4,255,36,14,39,39,103,102,102,103,255,0,14,39,39
        .byte 103,102,102,103,255,0,10,93,94,97,98,39,39,255,0,14,95,96,99,100,39,39,255,0,16,93,94,39,39,255,0,16,95
        .byte 96,39,39,0,0,0,93,94,255,0,5,97,98,255,0,6,39,39,0,0,0,95,96,255,0,5,99,100,255,0,6,39,39
        .byte 255,0,8,97,98,93,94,97,98,255,0,4,39,39,255,0,8,99,100,95,96,99,100,255,0,4,39,39,255,0,8,93,94
        .byte 255,0,8,39,39,255,0,8,95,96,255,0,8,39,39,97,98,255,0,16,39,39,99,100,255,0,12,255,101,4,39,39,93
        .byte 94,97,98,255,0,10,103,102,102,103,39,39,95,96,99,100,255,0,10,103,102,102,103,255,39,9,255,0,4,255,39,8
        .byte 255,39,8,255,0,4,255,39,9,40,40,37,37,40,40,40,255,0,4,40,40,40,37,37,40,40,39,39,40,40,89,90,40,40,40
        .byte 255,0,4,40,40,40,89,90,40,40,39,39,0,0,91,92,255,0,10,91,92,0,0,39,39,255,0,5,97,98,255,0,11
        .byte 39,39,255,0,5,99,100,255,0,11,39,39,255,0,8,101,101,161,101,255,0,6,39,39,0,0,0,97,98,93,94,0,103
        .byte 102,102,103,255,0,6,39,39,0,0,0,99,100,95,96,0,103,102,102,103,255,0,6,39,39,0,0,0,93,94,255,0,11
        .byte 97,98,39,39,0,0,0,95,96,255,0,11,99,100,39,39,255,0,6,101,255,0,7,97,98,93,94,39,39,255,0,6,101
        .byte 255,0,7,99,100,95,96,39,39,255,0,6,101,0,0,0,150,255,0,5,97,98,39,39,255,0,6,103,0,0,143,144,145
        .byte 255,0,4,99,100,39,39,97,98,255,0,4,103,0,0,146,0,147,255,0,6,39,39,99,100,255,0,16,39,39,93,94,255,0,9
        .byte 93,94,255,0,5,39,39,95,96,255,0,9,95,96,255,0,5,255,39,21
        .byte 255,41,20,45,46,45,45,10,255,45,10,10,45,45,45,41,45,45,45,126,37,127,45,45,108,45,45,108,45,45,126,37,127
        .byte 45,45,41,0,0,0,128,129,130,0,78,107,78,78,107,78,0,128,129,130,0,0,45,0,0,0,131,129,132,0,79,82,0,0
        .byte 82,79,0,131,129,132,0,0,45,255,0,8,84,0,0,84,255,0,22,43,255,0,18,81,43,255,0,4,41,255,0,12,76,76,76
        .byte 255,0,4,41,255,0,12,77,0,77,255,0,4,41,255,0,6,62,63,64,255,0,10,41,255,0,6,65,66,67,255,0,10
        .byte 41,255,0,6,68,69,70,255,0,10,41,255,0,6,71,69,70,255,0,10,41,255,0,19,41,255,0,17,81,133,41,255,0,17
        .byte 84,135,41,0,0,0,126,37,127,0,0,80,0,0,80,0,0,126,37,127,0,0,41,0,0,0,128,10,130,0,45,148,149
        .byte 45,45,45,0,128,10,130,0,0,255,41,21
        .byte 255,41,21,45,45,45,10,45,45,41,45,45,10,45,45,41,45,45,10,45,45,41,41,45,45,126,37,127,45,41,45,126,37
        .byte 127,45,41,45,126,37,127,45,41,45,0,0,128,129,130,0,41,0,128,129,130,0,41,0,128,129,130,0,41,45,0,0,131
        .byte 129,132,0,41,0,131,129,132,0,41,0,131,129,132,0,41,255,0,7,41,255,0,11,41,255,0,7,41,255,0,11,41,41
        .byte 255,0,6,41,255,0,5,41,255,0,5,41,41,255,0,6,41,255,0,5,41,255,0,5,41,41,137,138,255,0,4,41,255,0,4
        .byte 133,41,255,0,5,41,41,139,140,255,0,4,41,255,0,4,135,41,137,138,0,0,0,41,41,255,0,6,41,255,0,5,41
        .byte 139,140,0,0,0,41,41,255,0,6,41,255,0,5,255,41,4,0,0,41,41,137,138,255,0,4,45,255,0,4,133,41,45,45,45
        .byte 0,0,41,41,139,140,255,0,4,45,255,0,4,135,41,45,45,45,0,0,41,41,255,0,12,41,255,0,5,41,41,255,0,12
        .byte 41,255,0,5,41,41,0,0,126,37,127,0,41,0,126,37,127,0,41,255,0,5,41,41,0,0,128,10,130,0,41,0,128
        .byte 10,130,0,41,255,0,5,255,41,16,0,0,41,41,41
        .byte 255,41,15,0,0,255,41,4,45,45,65,66,67,255,45,4,65,66,67,45,45,0,0,45,45,41,41,45,45,68,69,70,45
        .byte 58,59,45,68,69,70,45,45,0,0,45,45,41,41,0,0,71,69,70,0,60,61,0,71,69,70,255,0,4,133,134,41,41
        .byte 255,0,16,135,136,41,41,255,0,16,133,134,41,41,255,0,16,135,136,41,41,0,0,255,41,18,0,0,41,45,45,45,10
        .byte 255,45,4,108,45,45,45,41,194,0,41,41,0,0,41,45,45,126,37,127,45,45,76,107,76,45,45,41,0,190,41,41,0,0
        .byte 45,0,0,128,129,130,0,0,77,82,77,0,0,255,41,5,0,0,45,0,0,131,129,132,0,0,0,84,0,0,0,45,2
        .byte 45,41,41,255,0,15,45,45,45,41,41,255,0,18,41,41,255,0,18,41,41,37,255,0,17,41,41,37,83,255,0,13,41
        .byte 36,36,41,41,124,84,255,0,4,80,0,39,0,137,138,0,137,138,41,0,0,41,41,125,255,0,4,39,148,149,39,0,139
        .byte 140,0,139,140,41,0,0,255,41,21
        .byte 0,0,0,255,39,13,255,0,7,39,62,63,64,255,39,5,62,63,64,39,255,0,7,39,65,66,67,40,40,2,40,40,65
        .byte 66,67,39,255,0,7,39,68,69,70,255,40,5,68,69,70,39,255,0,7,39,71,69,70,255,0,5,71,69,70,39,255,0,7
        .byte 39,255,0,4,163,164,165,255,0,4,39,255,0,7,39,255,0,4,166,167,168,255,0,4,39,255,0,7,39,255,0,4,169
        .byte 170,171,255,0,4,39,255,0,7,39,0,0,172,173,174,175,176,177,178,0,0,39,255,0,7,39,0,179,180,181,182,183,184
        .byte 185,186,187,0,39,255,0,7,39,188,189,0,190,191,192,193,194,0,195,196,39,255,0,7,39,197,198,190,190,199,200,201,194,194
        .byte 202,203,39,255,0,7,39,0,0,0,204,205,0,206,207,0,0,0,39,255,0,7,39,0,0,0,208,209,0,210,211,0,0,0
        .byte 39,255,0,7,39,0,0,0,212,213,0,214,215,0,0,0,39,255,0,7,39,255,0,11,39,255,0,7,39,255,0,11,39
        .byte 255,0,7,39,255,0,11,39,255,0,7,39,255,0,11,39,255,0,7,255,39,13,255,0,4
        .byte 255,37,22,255,38,8,47,48,255,38,8,37,37,255,38,4,58,59,38,38,49,50,38,38,58,59,255,38,4,37,37,85,86
        .byte 0,0,60,61,255,0,6,60,61,255,0,4,38,38,87,88,255,0,16,38,38,255,0,63,255,39,4,255,0,4,255,39,4
        .byte 255,0,8,39,194,0,39,255,0,4,39,194,0,39,255,0,8,39,0,190,39,255,0,4,39,0,190,39,255,0,8,255,39,4
        .byte 255,0,4,255,39,4,255,0,8,255,40,4,255,0,4,255,40,4,255,0,8,255,40,4,255,0,4,255,40,4,255,0,32
        .byte 142,255,0,10,37,37,255,0,6,143,144,145,255,0,9,37,37,85,86,255,0,4,146,0,147,255,0,9,37,37,87,88,255,0,16
        .byte 255,37,20
        .byte 255,0,232,255,39,8,255,0,12,39,255,40,6,39,255,0,12,39,255,40,6,39,255,0,12,39,0,0,219,220,0,0,39
        .byte 255,0,12,39,0,0,221,222,0,0,39,255,0,12,39,255,0,6,39,255,0,12,39,255,0,6,39,255,0,12,39,255,0,6
        .byte 39,255,0,12,39,39,39,0,0,39,39,39
        .byte 255,37,21,38,108,38,38,38,54,55,38,38,47,48,38,38,46,255,38,4,37,37,43,107,76,38,38,56,57,38,38,49,50
        .byte 255,38,5,58,59,37,37,43,82,77,255,0,13,60,61,37,37,77,84,255,0,16,37,37,255,0,5,62,63,64,0,0,0
        .byte 37,255,0,6,37,37,255,0,5,65,66,67,0,0,0,37,255,0,6,37,37,255,0,5,68,129,70,0,0,0,37,138,83
        .byte 255,0,4,37,37,255,0,5,71,129,70,0,0,0,37,140,84,255,0,4,37,37,255,0,9,255,37,5,0,0,255,37,4
        .byte 39,0,80,255,0,6,37,38,108,38,38,0,0,38,38,37,37,39,148,149,39,255,0,5,37,78,107,78,38,0,0,38,38
        .byte 255,37,12,79,82,79,255,0,5,37,37,51,51,38,38,38,113,113,38,38,38,0,84,255,0,6,37,37,52,52,38,38,38
        .byte 109,110,38,38,38,255,0,6,81,133,37,37,53,53,0,0,0,111,112,255,0,9,84,135,37,37,255,0,18,37,37,255,0,9
        .byte 37,255,0,6,81,133,37,37,255,0,9,37,255,0,6,84,135,255,37,4,0,0,255,37,15
numsc:
        .byte 36
.out .sprintf("SCREENS SIZE = %d", (* - scdat))

nmedat:
        .byte 0,0,96,104,255
        .byte 3,12,104,120,255
        .byte 2,11,48,136,255
        .byte 3,13,128,80,6,17,144,136,255
        .byte 3,12,32,144,7,16,80,64,7,16,80,96,2,11,144,16,255
        .byte 2,11,80,32,3,12,32,112,255
        .byte 4,14,144,40,3,13,32,104,2,11,40,40,255
        .byte 5,15,48,16,3,13,88,48,2,11,136,128,255
        .byte 3,13,48,80,2,11,128,80,255
        .byte 7,16,80,48,7,16,80,112,5,15,88,80,3,12,88,80,6,17,88,80,255
        .byte 7,18,88,80,255
        .byte 5,15,88,80,3,12,88,80,255
        .byte 7,18,88,120,2,11,48,104,3,13,128,104,255
        .byte 7,18,128,80,3,13,48,64,255
        .byte 3,12,80,32,255
        .byte 7,18,40,128,3,13,120,88,255
        .byte 7,16,144,144,3,13,120,144,6,17,72,48,2,11,80,16,255
        .byte 4,14,64,24,3,12,128,104,255
        .byte 6,21,24,80,5,15,48,80,3,12,120,32,255
        .byte 5,15,88,32,255
        .byte 4,14,80,80,255
        .byte 7,18,144,56,3,12,96,32,6,17,80,80,255
        .byte 4,14,64,104,3,13,144,48,255
        .byte 2,11,64,136,3,13,144,16,255
        .byte 6,17,40,16,4,14,80,80,3,12,64,144,255
        .byte 6,17,136,56,5,15,64,80,2,11,48,24,255
        .byte 3,12,120,144,6,17,120,16,5,15,88,80,255
        .byte 4,14,96,40,6,17,96,136,2,11,80,80,255
        .byte 3,12,32,16,5,15,144,144,6,17,96,40,255
        .byte 6,17,64,72,5,15,120,72,3,12,88,112,255
        .byte 2,11,96,96,6,17,80,48,3,13,120,48,255
        .byte 7,16,72,144,3,13,48,32,4,14,88,40,2,11,112,144,255
        .byte 7,19,88,96,7,20,88,56,2,9,48,40,2,9,48,112,0,1,144,80,255
        .byte 7,16,80,56,7,16,80,120,2,11,144,88,3,12,32,88,255
        .byte 255,5,15,64,88,3,13,120,104,2,11,72,16,255
.out .sprintf("POSITIONS SIZE = %d", (* - nmedat))

NUMOBJ = 14
objdta:
        .byte 0,0,7,130,8,127,253,176,150,64,228,64,0,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,254,56,120,254,56,120
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,196,8,62,253,224,246,64,196,64,0,32,254,8,8,254,8,8
        .byte 58,0,122,0,250,0,138,0,154,0,186,0,138,0,250,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,10,72,136,10,72,136
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,58,0,122,0,250,0,138,0,218,0,218,0,138,0,250,0,0,40,120,0,40,120
        .byte 58,0,122,0,250,0,218,0,170,0,138,0,170,0,250,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,254,8,8,254,8,8
        .byte 58,0,122,0,250,0,186,0,186,0,186,0,138,0,250,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,22,112,136,22,112,136
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,29,0,61,0,125,0,101,0,93,0,93,0,101,0,125,24,88,16,24,88,16
        .byte 0,66,0,255,0,36,0,165,0,153,0,66,0,90,0,36,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,28,48,40,28,48,40
        .byte 0,0,0,0,0,0,3,192,3,192,11,208,8,16,3,192,42,84,43,212,11,208,11,208,3,192,0,0,0,0,0,0,255,8,8,255,8,8
        .byte 0,0,0,0,3,224,15,248,28,252,27,252,54,62,53,222,61,94,61,214,62,54,31,236,31,156,15,248,3,224,0,0,31,144,144,31,144,144
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,24,0,36,0,78,0,90,0,52,0,24,0,0,0,24,0,17,144,64,17,144,64
        .byte 0,0,0,0,15,192,14,160,1,16,2,144,5,208,5,208,5,192,5,192,5,192,5,192,5,192,3,128,0,0,0,0,254,8,8,254,8,8
        .byte 0,0,0,0,255,255,169,255,147,255,255,255,0,0,255,255,170,171,255,254,213,86,255,253,170,169,255,251,0,0,0,0,254,8,8,254,8,8
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,8,8,255,8,8
.out .sprintf("OBJECTS SIZE = %d", (* - objdta))

font:
        .byte 0,0,0,0,0,0,0,0
        .byte 24,24,24,24,24,0,24,24
        .byte 0,108,108,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
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
        .byte 0,0,0,0,0,0,24,24
        .byte 0,0,6,12,24,48,96,0
        .byte 28,62,119,99,99,119,62,28
        .byte 12,28,12,12,12,12,12,12
        .byte 126,127,7,63,126,0,127,127
        .byte 127,126,0,30,31,3,127,126
        .byte 24,56,118,127,127,0,6,6
        .byte 127,127,0,126,127,3,127,126
        .byte 28,60,112,110,111,99,127,62
        .byte 127,126,0,12,28,24,56,48
        .byte 62,127,99,62,127,99,127,62
        .byte 62,127,99,123,59,7,30,28
        .byte 0,0,48,48,0,0,48,48
        .byte 0,0,48,48,0,48,48,96
        .byte 0,0,12,24,48,24,12,0
        .byte 0,0,0,126,0,126,0,0
        .byte 0,0,48,24,12,24,48,0
        .byte 60,126,6,14,28,0,24,24
        .byte 0,124,222,254,254,192,124,0
        .byte 12,12,6,54,51,99,127,127
        .byte 124,126,6,60,60,6,126,124
        .byte 30,63,113,96,96,113,63,30
        .byte 124,126,7,99,99,103,126,124
        .byte 126,126,0,126,126,64,126,126
        .byte 126,126,0,126,126,96,96,96
        .byte 31,63,113,96,99,115,63,31
        .byte 99,99,99,123,123,99,99,99
        .byte 24,24,24,24,24,24,24,24
        .byte 126,126,0,6,6,6,126,124
        .byte 6,14,28,56,56,28,14,6
        .byte 96,96,96,96,96,96,126,126
        .byte 126,127,99,107,107,107,107,107
        .byte 126,127,99,99,99,99,99,99
        .byte 28,62,119,99,99,119,62,28
        .byte 124,126,6,126,124,96,96,96
        .byte 28,62,119,99,99,118,56,28
        .byte 124,126,6,126,124,102,102,102
        .byte 48,48,56,28,14,6,126,124
        .byte 126,126,0,96,96,112,62,30
        .byte 99,99,99,99,99,99,127,63
        .byte 99,99,99,119,54,62,28,28
        .byte 99,99,99,107,62,62,54,54
        .byte 99,119,62,28,28,62,119,99
        .byte 99,99,119,62,28,0,28,28
        .byte 127,127,0,14,28,56,127,127
        .byte 0,30,24,24,24,24,30,0
        .byte 0,0,192,96,48,24,12,0
        .byte 0,240,48,48,48,48,240,0
        .byte 0,48,120,252,48,48,48,0
        .byte 0,0,0,0,0,0,0,255
        .byte 0,60,102,248,96,96,254,0
        .byte 54,0,62,119,99,99,119,62
        .byte 4,8,99,99,99,99,127,63
        .byte 54,0,99,99,99,99,127,63
        .byte 16,8,28,6,51,99,127,127
        .byte 4,8,28,6,51,99,127,127
        .byte 16,8,126,0,126,64,126,126
        .byte 4,8,126,0,126,64,126,126
        .byte 4,8,24,24,24,24,24,24
        .byte 4,8,62,119,99,99,119,62
        .byte 24,24,0,24,24,24,24,24
        .byte 54,0,28,6,51,99,127,127
        .byte 16,8,99,99,99,99,127,63
        .byte 25,38,28,6,51,99,127,127
        .byte 62,114,224,224,114,62,24,56
        .byte 8,20,126,0,126,64,126,126
        .byte 8,20,62,119,99,99,119,62
        .byte 25,38,62,119,99,99,119,62
        .byte 0,0,0,0,3,21,10,28
        .byte 0,0,32,64,128,0,0,0
        .byte 104,249,201,13,14,6,0,1
        .byte 240,248,8,152,151,15,246,248
        .byte 0,0,0,1,1,0,1,3
        .byte 242,6,246,176,176,0,152,156
        .byte 0,0,0,193,237,247,20,8
        .byte 0,0,0,0,128,252,0,240
        .byte 9,9,13,6,6,2,1,1
        .byte 248,8,152,151,15,246,224,218
        .byte 0,0,1,3,0,3,7,0
        .byte 222,0,240,176,48,0,24,28
        .byte 0,60,120,0,0,0,0,0
        .byte 124,198,187,227,227,187,198,124
.out .sprintf("FONT SIZE = %d", (* - font))

jtab:
        .byte 99
keys:   .byte 66,97,104,72,73,16,55,48,49,17,18
.out .sprintf("KEYS SIZE = %d", (* - keys))

data_end:
.out .sprintf("TOTAL DATA SIZE = %d", (* - data_start))
