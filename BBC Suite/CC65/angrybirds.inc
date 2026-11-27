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
WINDOWLFT = 0
WINDOWHGT = 23
WINDOWWID = 32 ;面面面面面面面面面面面面
MAPWID = 13
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255
mapdat:
        .byte 255,0,1,2,3,4,5,6,7,8,9,10,255,255,11,12,13,14,15,16,17,18,19,20,21,255,255,22,255,255,255,255,255,255,255,255,255,255,255
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255
stmap:  .byte 1

evnt00:
        lda #1
        cmp vard
        bcc *+5
        jmp a00032
        lda #255
        ldy #5
        sta (z80_ix),y
a00032: lda #8
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp a00054
a00054: lda #5
        cmp vara
        bcc *+5
        jmp a00085
        lda #255
        ldy #5
        sta (z80_ix),y
a00085: lda #0
        cmp vari
        beq *+5
        jmp a01418
        lda #236
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp a00159
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #0
        sta vara
        ; lda #0
        sta varj
        jmp a01418
a00159: lda #9
        cmp scno
        bcs *+5
        jmp a00379
        lda #2
        cmp scno
        bcs *+5
        jmp a00232
        lda #4
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp a00268
a00232: lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
a00268: lda #7
        cmp scno
        bcc *+5
        jmp a00374
        lda #32
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp a00374
        lda #56
        cmp varl
        bcc *+5
        jmp a00374
        lda #80
        dey; ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp a00374
        ; ldy #8
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #8
        sta (z80_ix),y
a00374: jmp a00414
a00379: lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
a00414: lda #9
        cmp scno
        bcc *+5
        jmp a00528
        lda #32
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp a00528
        lda #56
        cmp varl
        bcc *+5
        jmp a00502
        dey; ldy #8
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #8
        sta (z80_ix),y
        jmp a00528
a00502: ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
a00528: lda #0
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a00641
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a00596
:
        lda #1
        ldy #7
        sta (z80_ix),y
        jmp a00641
a00596: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a00628
:
        jmp a00641
a00628: lda #0
        ldy #7
        sta (z80_ix),y
a00641: lda #12
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a00825
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a00708
:
        lda #1
        ldy #7
        sta (z80_ix),y
        jmp a00825
a00708: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a00740
:
        jmp a00825
a00740: lda #0
        ldy #7
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #9
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
a00825: lda #13
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a00937
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a00892
:
        lda #1
        ldy #7
        sta (z80_ix),y
        jmp a00937
a00892: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a00924
:
        jmp a00937
a00924: lda #0
        ldy #7
        sta (z80_ix),y
a00937: lda #15
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a01121
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01005
:
        lda #1
        ldy #7
        sta (z80_ix),y
        jmp a01121
a01005: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01037
:
        jmp a01121
a01037: lda #0
        ldy #7
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #9
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
a01121: lda #16
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a01305
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01189
:
        lda #1
        ldy #7
        sta (z80_ix),y
        jmp a01305
a01189: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01221
:
        jmp a01305
a01221: lda #0
        ldy #7
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #9
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
a01305: lda #17
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a01418
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01372
:
        lda #1
        ldy #7
        sta (z80_ix),y
        jmp a01418
a01372: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01404
:
        jmp a01418
a01404: lda #0
        ldy #7
        sta (z80_ix),y
a01418: ldy #9
        lda (z80_ix),y
        sta varb
        dey; ldy #8
        lda (z80_ix),y
        sta varg
        lda #2
        cmp vari
        beq *+5
        jmp a01672
        lda #5
        cmp vara
        beq *+5
        jmp a01523
        lda #1
        sta chary
        lda #26
        sta charx
        lda #2
        cmp vari
        beq *+5
        jmp a01518
        jmp a01518
a01518: jsr dscor
a01523: lda #5
        ldy #6
        sta (z80_ix),y
        lda #120
        cmp varrnd
        bcc *+5
        jmp a01567
        lda #0
        jsr animsp
        jmp a01575
a01567: lda #0
        jsr animbk
a01575: lda #56
        cmp varl
        bcs *+5
        jmp a01632
        lda #2
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp a01668
a01632: lda #3
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
a01668: jsr ifall
a01672: jmp grav
.out .sprintf("EVENT 0 SIZE = %d", (* - evnt00))

evnt01:
        lda #1
        cmp vard
        bcc *+5
        jmp b00032
        lda #255
        ldy #5
        sta (z80_ix),y
b00032: lda #5
        cmp vara
        bcc *+5
        jmp b00063
        lda #255
        ldy #5
        sta (z80_ix),y
b00063: lda #2
        cmp vari
        beq *+5
        jmp b00601
        lda #8
        ldy #11
        cmp (z80_ix),y
        bcs *+5
        jmp b00257
        lda #25
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp b00191
        lda #1
        sta vark
        ; lda #1
        cmp vark
        beq *+5
        jmp b00186
        tya; lda #8
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        lda #3
        sta vark
b00186: jmp b00252
b00191: lda #2
        sta vark
        ; lda #2
        cmp vark
        beq *+5
        jmp b00252
        lda #9
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        lda #4
        sta vark
b00252: jmp b00265
b00257: lda #0
        jsr animbk
b00265: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp b00297
:
        jmp b00351
b00297: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp b00329
:
        jmp b00351
b00329: lda #4
        cmp scno
        bcc *+5
        jmp b00351
        jmp b00351
b00351: lda #5
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        jsr ifall
        ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
        lda #1
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp b00451
        lda #9
        asl a
        sta sndtyp
b00451: lda #11
        ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp b00601
        lda #3
        cmp vark
        beq *+5
        jmp b00513
        lda #<100
        sta z80_l
        lda #>100
        sta z80_h
        jsr addsc
b00513: lda #4
        cmp vark
        beq *+5
        jmp b00552
        lda #<50
        sta z80_l
        lda #>50
        sta z80_h
        jsr addsc
b00552: lda #0
        ldy #7
        sta (z80_ix),y
        ; lda #0
        ldy #11
        sta (z80_ix),y
        ; lda #0
        sta vark
        lda #255
        ldy #5
        sta (z80_ix),y
b00601: lda #0
        cmp vari
        beq *+5
        jmp b01171
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp b00678
:
        lda #11
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        jmp b00723
b00678: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp b00710
:
        jmp b00723
b00710: lda #1
        ldy #6
        sta (z80_ix),y
b00723: lda #24
        cmp varb
        bcs *+5
        jmp b00758
        lda #2
        ldy #7
        sta (z80_ix),y
        jmp b01171
b00758: lda #208
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp b00799
        lda #255
        ldy #5
        sta (z80_ix),y
        jmp b01171
b00799: lda #9
        cmp scno
        bcc *+5
        jmp b00907
        lda #32
        cmp varb
        bcc *+5
        jmp b00907
        lda #56
        cmp varl
        bcc *+5
        jmp b00881
        ldy #8
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #8
        sta (z80_ix),y
        jmp b00907
b00881: ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
b00907: lda #9
        cmp scno
        bcs *+5
        jmp b01127
        lda #2
        cmp scno
        bcs *+5
        jmp b00981
        lda #4
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp b01016
b00981: lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
b01016: lda #7
        cmp scno
        bcc *+5
        jmp b01122
        lda #32
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp b01122
        lda #56
        cmp varl
        bcc *+5
        jmp b01122
        lda #80
        dey; ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp b01122
        ; ldy #8
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #8
        sta (z80_ix),y
b01122: jmp b01162
b01127: lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
b01162: lda #0
        jsr animsp
b01171: jmp grav
.out .sprintf("EVENT 1 SIZE = %d", (* - evnt01))

evnt02:
        lda #9
        cmp vara
        beq *+5
        jmp c00044
        lda #66
        cmp varobj
        bcc *+5
        jmp c00044
        lda #10
        sta vara
c00044: lda #4
        cmp vara
        bcs *+5
        jmp c00095
        lda joyval
        and #4
        beq :+
        jmp c00095
:
        lda #65
        sta varobj
        lda #9
        sta vara
c00095: lda joyval
        and #1
        beq :+
        jmp c00256
:
        lda #4
        cmp vara
        bcs *+5
        jmp c00256
        lda #50
        asl a
        sta sndtyp
        lda #8
        sta chary
        lda #15
        sta charx
        lda #1
        sta prtmod
        lda #15
        jsr dmsg
        lda #0
        sta prtmod
        lda #30
        jsr delay
        jsr prskey
        jsr redraw
        lda #50
        asl a
        sta sndtyp
        lda #0
        sta chary
        lda #10
        sta charx
        lda #1
        sta prtmod
        lda #4
        jsr dmsg
        lda #0
        sta prtmod
c00256: lda #6
        cmp vara
        beq *+5
        jmp c00731
        lda #69
        cmp varobj
        bcc *+5
        jmp c00290
c00290: lda #1
        sta chary
        lda #10
        sta charx
        lda #1
        sta prtmod
        lda #14
        jsr dmsg
        lda #0
        sta prtmod
        lda #4
        sta chary
        lda #16
        sta charx
        lda #19
        jsr dmsg
        lda #5
        sta chary
        lda #16
        sta charx
        lda #20
        jsr dmsg
        lda #8
        sta chary
        lda #12
        sta charx
        lda #13
        jsr dmsg
        lda #8
        sta chary
        lda #20
        sta charx
        lda varm
        jsr disply
        lda #10
        sta chary
        lda #12
        sta charx
        ; lda #12
        jsr dmsg
        lda #10
        sta chary
        lda #20
        sta charx
        lda varn
        jsr disply
        lda #20
        sta chary
        lda #10
        sta charx
        lda #25
        jsr dmsg
        lda #22
        sta chary
        lda #20
        sta charx
        lda varf
        jsr disply
        lda #13
        sta chary
        lda #12
        sta charx
        lda #23
        jsr dmsg
        lda #15
        sta chary
        lda #14
        sta charx
        lda #1
        sta prtmod
        lda #0
        sta prtmod
        jsr dscor
        lda #22
        sta chary
        lda #13
        sta charx
        lda #24
        jsr dmsg
        lda #0
        cmp varf
        bcs *+5
        jmp c00697
        ; lda #0
        sta numlif
        ; lda #0
        sta varp
        lda #181
        sta varr
        lda #60
        jsr delay
        lda #0
        sta scno
        jsr nwscr
        lda #1
        sta restfl
c00697: lda joyval
        and #16
        beq :+
        jmp c00731
:
        lda #1
        sta restfl
        lda #5
        sta vark
c00731: lda #4
        cmp vara
        bcs *+5
        jmp c01799
        lda #24
        cmp varb
        bcc *+5
        jmp c00817
        lda #19
        sta chary
        lda #3
        sta charx
        lda varm
        jsr disply
        lda #19
        sta chary
        lda #27
        sta charx
        lda varn
        jsr disply
c00817: lda #0
        sta chary
        lda #27
        sta charx
        lda #2
        jsr dmsg
        lda #0
        sta chary
        ; lda #0
        sta charx
        lda #3
        jsr dmsg
        lda #9
        cmp scno
        bcs *+5
        jmp c00951
        lda #1
        sta chary
        lda #0
        sta charx
        lda #5
        cmp varb
        bcs *+5
        jmp c00921
        jmp c00921
c00921: lda #7
        jsr dmsg
        lda #1
        sta chary
        ; lda #1
        sta charx
        jmp c00967
c00951: lda #1
        sta chary
        lda #0
        sta charx
c00967: lda #5
        cmp varb
        bcs *+5
        jmp c00989
        jmp c00989
c00989: lda scno
        jsr disply
        lda #1
        cmp vara
        beq *+5
        jmp c01296
        lda #144
        ldy #8
        sta (z80_ix),y
        lda #200
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #132
        cmp varrnd
        beq *+5
        jmp c01087
        ; lda #132
        sta varrnd
c01087: lda #56
        cmp varrnd
        bcs *+5
        jmp c01114
        ; lda #56
        sta varrnd
c01114: lda #8
        cmp scno
        beq *+5
        jmp c01172
        lda #100
        cmp varrnd
        bcs *+5
        jmp c01158
        ; lda #100
        sta varrnd
c01158: lda varrnd
        ldy #9
        sta (z80_ix),y
c01172: lda #9
        cmp scno
        beq *+5
        jmp c01230
        lda #100
        cmp varrnd
        bcc *+5
        jmp c01216
        ; lda #100
        sta varrnd
c01216: lda varrnd
        ldy #9
        sta (z80_ix),y
c01230: lda #20
        cmp scno
        beq *+5
        jmp c01288
        lda #66
        cmp varrnd
        bcs *+5
        jmp c01274
        lda #120
        sta varrnd
c01274: lda varrnd
        ldy #9
        sta (z80_ix),y
c01288: lda #2
        sta vara
c01296: lda #2
        cmp vara
        beq *+5
        jmp c01616
        ; lda #2
        cmp scno
        bcc *+5
        jmp c01616
        lda varrnd
        ldy #9
        cmp (z80_ix),y
        bne *+5
        jmp c01383
        lda #68
        cmp varobj
        beq *+5
        jmp c01383
        lda #3
        asl a
        sta sndtyp
c01383: lda varrnd
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp c01499
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp c01473
:
        lda #132
        sta varrnd
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
        jmp c01499
c01473: ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
c01499: lda varrnd
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp c01616
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp c01589
:
        lda #100
        sta varrnd
        ldy #9
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #9
        sta (z80_ix),y
        jmp c01616
c01589: ldy #9
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #9
        sta (z80_ix),y
c01616: lda #0
        cmp vari
        beq *+5
        jmp c01715
        lda varb
        cmp varc
        bcs *+5
        jmp c01682
        lda #2
        ldy #6
        sta (z80_ix),y
        lda #1
        iny; ldy #7
        sta (z80_ix),y
        jmp c01710
c01682: lda #2
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
c01710: jmp c01742
c01715: lda #6
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
c01742: ldy #9
        lda (z80_ix),y
        sta varc
        lda #1
        sta chary
        lda #26
        sta charx
        lda #2
        cmp vari
        beq *+5
        jmp c01794
        jmp c01794
c01794: jsr dscor
c01799: rts
.out .sprintf("EVENT 2 SIZE = %d", (* - evnt02))

evnt03:
        lda #1
        cmp vare
        beq *+5
        jmp d00117
        lda #0
        cmp vark
        beq *+5
        jmp d00117
        lda joyval
        and #16
        beq :+
        jmp d00117
:
        lda #1
        cmp varr
        bcs *+5
        jmp d00074
        jmp d00117
d00074: lda #112
        sta varf
        lda #4
        sta z80_c
        ; lda #4
        sta z80_b
        jsr spawn
        lda #12
        asl a
        sta sndtyp
d00117: lda #9
        cmp vara
        bcc *+5
        jmp d00161
        lda #11
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
d00161: lda #4
        cmp vara
        bcs *+5
        jmp d00662
        lda #0
        cmp vara
        beq *+5
        jmp d00216
        ; lda #0
        sta vari
        lda #5
        sta vare
        jmp d00245
d00216: lda #127
        ldy #8
        sta (z80_ix),y
        lda varc
        iny; ldy #9
        sta (z80_ix),y
d00245: lda #5
        cmp vare
        beq *+5
        jmp d00287
        lda joyval
        and #16
        beq :+
        jmp d00287
:
        lda #0
        sta vare
d00287: lda #4
        cmp vare
        bcs *+5
        jmp d00450
        lda #0
        cmp vark
        beq *+5
        jmp d00330
        ; lda #0
        jsr animsp
d00330: lda #2
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp d00445
        lda #8
        sta z80_c
        iny; ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #4
        cmp scno
        bcs *+5
        jmp d00410
        jmp d00410
d00410: lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
d00445: jmp d00571
d00450: lda #4
        cmp scno
        bcc *+5
        jmp d00544
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
        jmp d00544
d00544: lda #3
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
d00571: lda #0
        cmp varj
        beq *+5
        jmp d00657
        lda #232
        cmp varb
        beq *+5
        jmp d00657
        lda varm
        clc
        adc #1
        sta varm
        lda #8
        sta z80_c
        lda varh
        sec
        sbc z80_c
        sta varh
        lda #19
        asl a
        sta sndtyp
d00657: jmp d00675
d00662: lda #0
        ldy #7
        sta (z80_ix),y
d00675: lda #10
        cmp vara
        beq *+5
        jmp d00717
        jsr cls
        lda #11
        sta vara
        lda #100
        jsr delay
        jsr cls
d00717: lda #11
        cmp vara
        beq *+5
        jmp d01226
        lda #4
        jsr delay
        lda #1
        cmp varr
        beq *+5
        jmp d00890
        lda #68
        cmp varobj
        bcc *+5
        jmp d00777
d00777: lda #5
        sta chary
        lda #14
        sta charx
        lda #43
        jsr dmsg
        lda #7
        sta chary
        lda #12
        sta charx
        lda #1
        sta prtmod
        lda #33
        jsr dmsg
        lda #0
        sta prtmod
        lda #7
        sta chary
        lda #17
        sta charx
        lda #1
        sta prtmod
        lda #36
        jsr dmsg
        lda #0
        sta prtmod
        jmp d00950
d00890: lda #67
        cmp varobj
        beq *+5
        jmp d00908
d00908: lda #7
        sta chary
        lda #12
        sta charx
        lda #1
        sta prtmod
        lda #0
        jsr dmsg
        lda #0
        sta prtmod
d00950: lda #10
        sta chary
        lda #11
        sta charx
        lda #1
        sta prtmod
        lda #23
        jsr dmsg
        lda #0
        sta prtmod
        lda #13
        sta chary
        lda #15
        sta charx
        lda #1
        sta prtmod
        lda #0
        sta prtmod
        jsr dscor
        lda #13
        sta chary
        lda #12
        sta charx
        lda #17
        jsr dmsg
        lda #14
        sta chary
        lda #12
        sta charx
        lda #18
        jsr dmsg
        lda #71
        cmp varobj
        beq *+5
        jmp d01226
        lda #23
        sta chary
        lda #11
        sta charx
        lda #10
        jsr dmsg
        lda joyval
        and #16
        beq :+
        jmp d01226
:
        lda #60
        jsr delay
        lda #1
        cmp varr
        beq *+5
        jmp d01180
        ; lda #1
        sta numlif
        jmp d01189
d01180: lda #0
        sta numlif
d01189: lda #0
        sta varp
        lda #181
        sta varr
        lda #0
        sta scno
        jsr nwscr
        lda #1
        sta restfl
d01226: rts
.out .sprintf("EVENT 3 SIZE = %d", (* - evnt03))

evnt04:
        lda #9
        cmp vara
        bcs *+5
        jmp e00411
        lda varf
        ldy #8
        sta (z80_ix),y
        lda #16
        cmp varf
        bcc *+5
        jmp e00246
        lda #1
        cmp vare
        beq *+5
        jmp e00081
        lda varc
        iny; ldy #9
        sta (z80_ix),y
e00081: lda #20
        sta z80_c
        lda varf
        sec
        sbc z80_c
        sta varf
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp e00165
:
        lda #11
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        jmp e00241
e00165: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp e00197
:
        jmp e00241
e00197: lda #4
        ldy #6
        sta (z80_ix),y
        ; lda #4
        cmp scno
        bcc *+5
        jmp e00233
        jmp e00233
e00233: lda #0
        jsr animsp
e00241: jmp e00272
e00246: lda #5
        asl a
        sta sndtyp
        lda #255
        ldy #5
        sta (z80_ix),y
e00272: lda #236
        cmp varb
        bcc *+5
        jmp e00303
        lda #255
        ldy #5
        sta (z80_ix),y
e00303: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00406
:
        lda #30
        asl a
        sta sndtyp
        lda varn
        clc
        adc #1
        sta varn
        lda #8
        sta z80_c
        lda varh
        clc
        adc z80_c
        sta varh
        lda #1
        sta varj
        lda #2
        sta vari
        lda #255
        ldy #5
        sta (z80_ix),y
e00406: jmp e00424
e00411: lda #255
        ldy #5
        sta (z80_ix),y
e00424: rts
.out .sprintf("EVENT 4 SIZE = %d", (* - evnt04))

evnt05:
        lda #4
        cmp vara
        bcs *+5
        jmp f00121
        lda #0
        cmp vark
        bne *+5
        jmp f00066
        ; lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp f00058
f00058: lda #0
        jsr animsp
f00066: lda #226
        cmp varb
        bcc *+5
        jmp f00092
        lda #0
        jsr animsp
f00092: lda #168
        ldy #8
        sta (z80_ix),y
        lda varh
        iny; ldy #9
        sta (z80_ix),y
f00121: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp f00221
:
        lda #20
        cmp scno
        beq *+5
        jmp f00199
        lda #6
        sta vara
        jsr cls
        lda #100
        jsr delay
        lda #1
        sta nexlev
        jmp f00207
f00199: lda #5
        sta vara
f00207: lda #255
        ldy #5
        sta (z80_ix),y
f00221: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp f00400
:
        lda #11
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        lda vard
        clc
        adc #1
        sta vard
        lda #5
        cmp vard
        bcc *+5
        jmp f00400
        lda #0
        sta chary
        ; lda #0
        sta charx
        lda #1
        sta prtmod
        lda #8
        jsr dmsg
        lda #0
        sta prtmod
        jsr cls
        lda #6
        sta vara
        lda #9
        sta varf
        lda #0
        sta vard
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #30
        jsr delay
f00400: lda #254
        cmp varm
        bcc *+5
        jmp f00450
        lda #19
        sta chary
        lda #4
        sta charx
        lda #5
        jsr dmsg
        lda #0
        sta varm
f00450: lda #254
        cmp varn
        bcc *+5
        jmp f00500
        lda #19
        sta chary
        lda #28
        sta charx
        lda #5
        jsr dmsg
        lda #0
        sta varn
f00500: lda #226
        cmp varb
        bcs *+5
        jmp f00534
        lda #0
        cmp vark
        beq *+5
        jmp f00534
f00534: rts
.out .sprintf("EVENT 5 SIZE = %d", (* - evnt05))

evnt06:
        lda #6
        cmp vara
        bne *+5
        jmp g00077
        lda #16
        cmp varb
        bcc *+5
        jmp g00077
        lda #11
        cmp vara
        bne *+5
        jmp g00077
        lda #23
        sta chary
        lda #15
        sta charx
        lda #6
        jsr dmsg
g00077: lda #4
        cmp vara
        bcs *+5
        jmp g00204
        lda varh
        ldy #9
        sta (z80_ix),y
        lda #176
        dey; ldy #8
        sta (z80_ix),y
        lda #224
        cmp varb
        bcc *+5
        jmp g00149
        lda #0
        jsr animsp
g00149: lda #3
        cmp vare
        beq *+5
        jmp g00174
        lda #0
        jsr animbk
g00174: lda #5
        cmp vara
        beq *+5
        jmp g00204
        lda #255
        ldy #5
        sta (z80_ix),y
g00204: lda #64
        cmp varb
        bcc *+5
        jmp g00557
        lda #4
        cmp vara
        bcs *+5
        jmp g00396
        lda #90
        cmp varrnd
        bcs *+5
        jmp g00396
        lda #13
        cmp varp
        bcc *+5
        jmp g00396
        lda #68
        cmp varobj
        bcc *+5
        jmp g00345
        lda #22
        sta chary
        lda #31
        sta charx
        lda #47
        jsr dmsg
        lda #22
        sta chary
        lda #28
        sta charx
        lda #49
        jsr dmsg
        jmp g00396
g00345: lda #22
        sta chary
        lda #31
        sta charx
        lda #40
        jsr dmsg
        lda #22
        sta chary
        lda #28
        sta charx
        lda #48
        jsr dmsg
g00396: lda #0
        cmp vara
        beq *+5
        jmp g00557
        lda #72
        cmp varh
        bcs *+5
        jmp g00460
        lda #22
        sta chary
        lda #31
        sta charx
        lda #39
        jsr dmsg
        jmp g00485
g00460: lda #22
        sta chary
        lda #31
        sta charx
        lda #40
        jsr dmsg
g00485: lda #160
        cmp varh
        bcc *+5
        jmp g00532
        lda #21
        sta chary
        lda #1
        sta charx
        lda #41
        jsr dmsg
        jmp g00557
g00532: lda #21
        sta chary
        lda #1
        sta charx
        lda #42
        jsr dmsg
g00557: rts
.out .sprintf("EVENT 6 SIZE = %d", (* - evnt06))

evnt07:
        lda #20
        cmp scno
        bcs *+5
        jmp h00138
        lda #0
        ldy #7
        sta (z80_ix),y
        lda #2
        sta z80_c
        lda varq
        clc
        adc z80_c
        sta varq
        lda #8
        cmp varq
        bne *+5
        jmp h00099
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
h00099: lda #10
        cmp varq
        bcc *+5
        jmp h00138
        lda #0
        sta varq
        lda #255
        ldy #5
        sta (z80_ix),y
h00138: lda #21
        cmp scno
        beq *+5
        jmp h00763
        lda #15
        sta chary
        lda #13
        sta charx
        lda #1
        sta prtmod
        lda #0
        sta prtmod
        jsr dscor
        lda #13
        sta chary
        lda #10
        sta charx
        lda #1
        sta prtmod
        lda #23
        jsr dmsg
        lda #0
        sta prtmod
        lda #3
        sta chary
        lda #13
        sta charx
        lda #43
        jsr dmsg
        lda #68
        cmp varobj
        beq *+5
        jmp h00326
        lda #3
        asl a
        sta sndtyp
        lda #21
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        jmp h00334
h00326: lda #1
        jsr delay
h00334: lda #10
        cmp varrnd
        bcc *+5
        jmp h00365
        lda #22
        sta varrnd
        jmp h00374
h00365: lda #0
        sta varrnd
h00374: lda varrnd
        sta chary
        lda #30
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        sta charx
        lda #44
        jsr dmsg
        lda #18
        sta chary
        lda #11
        sta charx
        lda #1
        sta prtmod
        lda #0
        jsr dmsg
        lda #0
        sta prtmod
        lda #5
        sta chary
        lda #11
        sta charx
        lda #1
        sta prtmod
        lda #31
        jsr dmsg
        lda #0
        sta prtmod
        lda #15
        cmp varrnd
        beq *+5
        jmp h00559
        lda #23
        sta chary
        lda #10
        sta charx
        ; lda #10
        jsr dmsg
h00559: lda #11
        sta chary
        lda #18
        sta charx
        lda #17
        jsr dmsg
        lda #12
        sta chary
        lda #18
        sta charx
        ; lda #18
        jsr dmsg
        lda #10
        sta chary
        lda #20
        sta charx
        lda #1
        sta prtmod
        lda #21
        jsr dmsg
        lda #0
        sta prtmod
        lda #12
        sta chary
        lda #20
        sta charx
        lda #22
        jsr dmsg
        lda joyval
        and #16
        beq :+
        jmp h00763
:
        lda #25
        sta varb
        lda #0
        sta numlif
        ; lda #0
        sta varp
        lda #181
        sta varr
        jsr cls
        lda #100
        jsr delay
        lda #0
        sta scno
        jsr nwscr
        lda #1
        sta restfl
h00763: rts
.out .sprintf("EVENT 7 SIZE = %d", (* - evnt07))

evnt08:
        lda #0
        cmp scno
        beq *+5
        jmp i00682
        lda #17
        sta chary
        lda #11
        sta charx
        lda #33
        jsr dmsg
        lda joyval
        and #2
        beq :+
        jmp i00124
:
        lda #30
        asl a
        sta sndtyp
        lda numlif
        clc
        adc #1
        sta numlif
        lda #12
        jsr delay
        lda #1
        cmp numlif
        bcc *+5
        jmp i00124
        lda #0
        sta numlif
i00124: lda #0
        cmp numlif
        beq *+5
        jmp i00171
        lda #17
        sta chary
        lda #16
        sta charx
        lda #34
        jsr dmsg
        jmp i00196
i00171: lda #17
        sta chary
        lda #16
        sta charx
        lda #35
        jsr dmsg
i00196: lda #1
        sta chary
        lda #11
        sta charx
        lda #1
        sta prtmod
        lda #4
        jsr dmsg
        lda #0
        sta prtmod
        lda #3
        sta chary
        lda #14
        sta charx
        lda #16
        jsr dmsg
        lda #8
        sta chary
        lda #15
        sta charx
        lda #6
        jsr dmsg
        lda #11
        sta chary
        lda #14
        sta charx
        lda #26
        jsr dmsg
        lda #13
        sta chary
        lda #11
        sta charx
        lda #27
        jsr dmsg
        lda #15
        sta chary
        lda #12
        sta charx
        lda #28
        jsr dmsg
        lda #0
        cmp vark
        beq *+5
        jmp i00406
        lda #22
        sta chary
        lda #11
        sta charx
        lda #10
        jsr dmsg
i00406: lda #7
        sta chary
        lda #11
        sta charx
        lda #19
        jsr dmsg
        lda #8
        sta chary
        lda #11
        sta charx
        lda #20
        jsr dmsg
        lda #7
        sta chary
        lda #19
        sta charx
        lda #17
        jsr dmsg
        lda #8
        sta chary
        lda #19
        sta charx
        lda #18
        jsr dmsg
        lda #6
        sta chary
        lda #21
        sta charx
        lda #1
        sta prtmod
        lda #21
        jsr dmsg
        lda #0
        sta prtmod
        lda #8
        sta chary
        lda #21
        sta charx
        lda #22
        jsr dmsg
        lda #19
        sta chary
        lda #13
        sta charx
        lda #38
        jsr dmsg
        lda #0
        cmp vark
        beq *+5
        jmp i00682
        lda joyval
        and #16
        beq :+
        jmp i00682
:
        lda #50
        asl a
        sta sndtyp
        lda #50
        jsr delay
        lda #22
        sta scno
        jsr nwscr
        lda #1
        sta restfl
        lda #0
        sta vark
i00682: lda #22
        cmp scno
        beq *+5
        jmp i01083
        lda #23
        sta chary
        lda #10
        sta charx
        lda #45
        jsr dmsg
        jsr ifall
        jsr jump
        lda #17
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i00756
i00756: lda #67
        cmp varobj
        beq *+5
        jmp i00773
i00773: lda #65
        cmp varobj
        bcs *+5
        jmp i00806
        lda vark
        clc
        adc #1
        sta vark
i00806: lda #60
        cmp vark
        bcc *+5
        jmp i00831
        lda #5
        sta vark
i00831: lda #5
        cmp vark
        beq *+5
        jmp i01083
        jsr cls
        lda #30
        jsr delay
        lda #45
        asl a
        sta sndtyp
        lda #7
        sta chary
        lda #15
        sta charx
        lda #17
        jsr dmsg
        lda #8
        sta chary
        lda #15
        sta charx
        lda #18
        jsr dmsg
        lda #6
        sta chary
        lda #17
        sta charx
        lda #1
        sta prtmod
        lda #21
        jsr dmsg
        lda #0
        sta prtmod
        lda #8
        sta chary
        lda #17
        sta charx
        lda #22
        jsr dmsg
        lda #10
        sta chary
        lda #11
        sta charx
        lda #1
        sta prtmod
        lda #11
        jsr dmsg
        lda #0
        sta prtmod
        lda #120
        jsr delay
        lda #50
        jsr delay
        lda #1
        sta scno
        jsr nwscr
        ; lda #1
        sta restfl
        lda #100
        asl a
        sta sndtyp
i01083: jmp grav
.out .sprintf("EVENT 8 SIZE = %d", (* - evnt08))

evnt09:
        rts
.out .sprintf("EVENT 9 SIZE = %d", (* - evnt09))

evnt10:
        lda #20
        cmp scno
        bcs *+5
        jmp k01138
        lda #6
        cmp varb
        beq *+5
        jmp k00456
        lda varo
        clc
        adc #1
        sta varo
        lda #24
        cmp varo
        bcs *+5
        jmp k00068
k00068: lda #30
        cmp varo
        bcc *+5
        jmp k00444
        lda #0
        sta vara
        ; lda #0
        sta varo
        lda #2
        sta chary
        lda #10
        sta charx
        lda #1
        sta prtmod
        lda #9
        jsr dmsg
        lda #0
        sta prtmod
        lda #6
        sta chary
        lda #16
        sta charx
        lda #17
        jsr dmsg
        lda #7
        sta chary
        lda #16
        sta charx
        lda #18
        jsr dmsg
        lda #5
        sta chary
        lda #18
        sta charx
        lda #1
        sta prtmod
        lda #21
        jsr dmsg
        lda #0
        sta prtmod
        lda #7
        sta chary
        lda #18
        sta charx
        lda #22
        jsr dmsg
        lda #10
        sta chary
        lda #12
        sta charx
        ; lda #12
        jsr dmsg
        lda #10
        sta chary
        lda #20
        sta charx
        lda varn
        jsr disply
        lda #12
        sta chary
        ; lda #12
        sta charx
        lda #13
        jsr dmsg
        lda #12
        sta chary
        lda #20
        sta charx
        lda varm
        jsr disply
        lda #17
        sta chary
        lda #12
        sta charx
        lda #29
        jsr dmsg
        lda #170
        jsr delay
        jsr cls
        lda #30
        jsr delay
        lda #0
        sta varb
        lda #1
        sta nexlev
        lda #100
        asl a
        sta sndtyp
        jmp k00456
k00444: lda varo
        asl a
        sta sndtyp
k00456: lda #0
        cmp scno
        bcc *+5
        jmp k01138
        lda #4
        cmp vark
        bcs *+5
        jmp k01079
        ; lda #4
        cmp vara
        bcs *+5
        jmp k01075
        lda #0
        cmp vari
        beq *+5
        jmp k01075
        ; lda #0
        cmp vara
        beq *+5
        jmp k01075
        lda #1
        cmp scno
        bcs *+5
        jmp k00571
        lda #32
        sta varrnd
        jmp k00629
k00571: lda #95
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #32
        cmp varrnd
        bcs *+5
        jmp k00629
        ; lda #32
        sta varrnd
k00629: lda varrnd
        sta varl
        lda varrnd
        ldy #8
        sta (z80_ix),y
        lda #0
        iny; ldy #9
        sta (z80_ix),y
        ; lda #0
        cmp scno
        bcc *+5
        jmp k00722
        lda #4
        cmp scno
        bcs *+5
        jmp k00722
        lda #0
        sta z80_c
        ; lda #0
        sta z80_b
        jsr spawn
k00722: lda #4
        cmp scno
        bcc *+5
        jmp k00777
        lda #9
        cmp scno
        bcs *+5
        jmp k00777
        lda #0
        sta z80_c
        lda #13
        sta z80_b
        jsr spawn
k00777: lda #9
        cmp scno
        bcc *+5
        jmp k00832
        lda #12
        cmp scno
        bcs *+5
        jmp k00832
        lda #0
        sta z80_c
        lda #12
        sta z80_b
        jsr spawn
k00832: lda #12
        cmp scno
        bcc *+5
        jmp k00887
        lda #15
        cmp scno
        bcs *+5
        jmp k00887
        lda #0
        sta z80_c
        lda #15
        sta z80_b
        jsr spawn
k00887: lda #15
        cmp scno
        bcc *+5
        jmp k00943
        lda #18
        cmp scno
        bcs *+5
        jmp k00943
        lda #0
        sta z80_c
        lda #16
        sta z80_b
        jsr spawn
k00943: lda #18
        cmp scno
        bcc *+5
        jmp k00998
        lda #20
        cmp scno
        bcs *+5
        jmp k00998
        lda #0
        sta z80_c
        lda #17
        sta z80_b
        jsr spawn
k00998: lda varrnd
        ldy #8
        sta (z80_ix),y
        lda #0
        iny; ldy #9
        sta (z80_ix),y
        lda #1
        sta z80_c
        ; lda #1
        sta z80_b
        jsr spawn
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #1
        sta vara
k01075: jmp k01138
k01079: lda #4
        cmp vara
        bcs *+5
        jmp k01138
        lda #0
        sta chary
        lda #10
        sta charx
        lda #1
        sta prtmod
        lda #4
        jsr dmsg
        lda #0
        sta prtmod
k01138: rts
.out .sprintf("EVENT 10 SIZE = %d", (* - evnt10))

evnt11:
        lda #0
        cmp scno
        bcc *+5
        jmp l00515
        ; lda #0
        cmp numlif
        beq *+5
        jmp l00076
        lda varp
        clc
        adc #1
        sta varp
        lda #25
        cmp varp
        bcc *+5
        jmp l00076
        lda #0
        sta varp
l00076: lda #20
        cmp scno
        bcs *+5
        jmp l00515
        lda #1
        cmp numlif
        beq *+5
        jmp l00515
        ; lda #1
        cmp varr
        bcs *+5
        jmp l00149
        lda #10
        sta vara
        lda #0
        sta numlif
        jmp l00208
l00149: lda #6
        cmp vara
        beq *+5
        jmp l00171
        jmp l00208
l00171: lda #6
        cmp varb
        beq *+5
        jmp l00192
        jmp l00208
l00192: lda varp
        clc
        adc #1
        sta varp
l00208: lda #25
        cmp varp
        bcc *+5
        jmp l00233
        lda #0
        sta varp
l00233: lda #25
        cmp varp
        beq *+5
        jmp l00265
        lda varr
        sec
        sbc #1
        sta varr
l00265: lda #1
        cmp varr
        bcs *+5
        jmp l00290
        ; lda #1
        sta varr
l00290: lda #4
        cmp vara
        bcs *+5
        jmp l00515
        lda #24
        cmp varb
        bcc *+5
        jmp l00515
        lda #20
        sta chary
        lda #14
        sta charx
        lda #50
        jsr dmsg
        lda #20
        sta chary
        lda #17
        sta charx
        lda #10
        cmp varr
        bcc *+5
        jmp l00388
        jmp l00388
l00388: lda varr
        jsr disply
        lda #1
        cmp varp
        bcs *+5
        jmp l00515
        lda #99
        cmp varr
        bcs *+5
        jmp l00515
        lda #9
        cmp varr
        bcc *+5
        jmp l00473
        lda #20
        sta chary
        lda #19
        sta charx
        lda #46
        jsr dmsg
l00473: lda #9
        cmp varr
        bcs *+5
        jmp l00515
        lda #20
        sta chary
        lda #18
        sta charx
        lda #46
        jsr dmsg
l00515: lda varobj
        clc
        adc #1
        sta varobj
        lda #71
        cmp varobj
        bcc *+5
        jmp l00557
        lda #65
        sta varobj
l00557: lda #9
        cmp vara
        bcs *+5
        jmp l00844
        lda #20
        cmp scno
        bcs *+5
        jmp l00844
        lda #4
        cmp vare
        bcs *+5
        jmp l00623
        lda vare
        clc
        adc #1
        sta vare
l00623: lda #4
        cmp vare
        beq *+5
        jmp l00677
        lda #0
        cmp vara
        beq *+5
        jmp l00669
        lda #5
        sta vare
        jmp l00677
l00669: lda #6
        sta vare
l00677: lda #4
        cmp vark
        bcc *+5
        jmp l00743
        lda vark
        clc
        adc #1
        sta vark
        lda #20
        cmp vark
        bcc *+5
        jmp l00743
        lda #0
        sta vark
        ; lda #0
        sta varb
l00743: lda #6
        cmp vara
        beq *+5
        jmp l00844
        lda vard
        clc
        adc #1
        sta vard
        lda #30
        cmp vard
        bcc *+5
        jmp l00800
        lda #0
        sta vard
l00800: lda #30
        cmp vard
        beq *+5
        jmp l00844
        lda #15
        asl a
        sta sndtyp
        lda varf
        sec
        sbc #1
        sta varf
l00844: rts
.out .sprintf("EVENT 11 SIZE = %d", (* - evnt11))

evnt12:
        rts
.out .sprintf("EVENT 12 SIZE = %d", (* - evnt12))

evnt13:
        jsr cls
        lda #50
        jsr delay
        lda #0
        sta vara
        ; lda #0
        sta varc
        lda #25
        sta varb
        lda #0
        sta vard
        lda #5
        sta vare
        lda #0
        sta vari
        lda #120
        sta varh
        lda #0
        sta varj
        lda #5
        sta vark
        lda #0
        sta varm
        ; lda #0
        sta varn
        ; lda #0
        sta varo
        lda #65
        sta varobj
        lda #0
        sta numlif
        ; lda #0
        sta varp
        lda #181
        sta varr
        rts
.out .sprintf("EVENT 13 SIZE = %d", (* - evnt13))

evnt14:
        lda #6
        cmp vara
        beq *+5
        jmp o00022
        jsr cls
o00022: lda #0
        sta vara
        ; lda #0
        sta varb
        ; lda #0
        sta varc
        ; lda #0
        sta vard
        lda #5
        sta vare
        lda #0
        sta vari
        lda #120
        sta varh
        lda #0
        sta varj
        lda #5
        sta vark
        lda #0
        sta varm
        ; lda #0
        sta varn
        ; lda #0
        sta varo
        lda #65
        sta varobj
        rts
.out .sprintf("EVENT 14 SIZE = %d", (* - evnt14))

evnt15:
        lda #0
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp p00101
        lda #255
        ; ldy #5
        sta (z80_ix),y
        tya; lda #5
        cmp vara
        beq *+5
        jmp p00069
        lda #6
        sta varb
        jmp p00101
p00069: lda #0
        sta vari
        lda #5
        sta vare
        lda #0
        sta vara
        ; lda #0
        sta varj
p00101: rts
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
        .byte "GAME OVER",141
        .byte "GET READY",141
        .byte "score",141
        .byte "level",141
        .byte "AnGrY BiRdS",141
        .byte "__",141
        .byte "*+",141
        .byte "0",141
        .byte "________________________________",141
        .byte "Pigs_have_won!",141
        .byte "Press_space",141
        .byte "Good luck!",141
        .byte "Pigs__-_",141
        .byte "Birds_-_",141
        .byte "won_the_birds!",141
        .byte ":;",141
        .byte 39
        .byte "#$%&",141
        .byte 39
        .byte "(",141
        .byte "),",141
        .byte "/<",141
        .byte "=>",141
        .byte "[",141
        .byte "]",141
        .byte "Total_score",141
        .byte "Exit_-_",141
        .byte "Restart_-_space",141
        .byte "KEYS\",141
        .byte "Fire .space",141
        .byte "Pause . p",141
        .byte "next_level",141
        .byte "@",141
        .byte "ExCeLlEnT!",141
        .byte "The_final_victory_of_the_pigs!",141
        .byte "Time\",141
        .byte "NO__.T",141
        .byte "YES_.T",141
        .byte "Out!",141
        .byte "_",141
        .byte "Exit_.E",141
        .byte "^",141
        .byte "|",141
        .byte "{",141
        .byte "}",141
        .byte "@_@_@",141
        .byte "~",141
        .byte "we_attack!!!",141
        .byte "`",141
        .byte "",141
        .byte "?",141
        .byte "j",141
        .byte "WZ\",141
nummsg:
        .byte 51
.out .sprintf("MESSAGE SIZE = %d", (* - msgdat))

chgfx:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 5,5,10,10,5,5,10,10,5,5,10,10,5,5,10,10
        .byte 32,32,24,66,5,5,80,80,160,160,64,24,32,1,0,8
        .byte 95,95,175,175,95,95,175,175,95,95,175,175,95,95,175,175
        .byte 165,164,90,90,165,165,26,82,133,5,72,26,160,4,0,0
        .byte 165,165,90,90,165,165,90,90,165,165,82,74,161,160,88,80
        .byte 95,95,175,175,95,95,175,175,255,95,255,255,95,95,175,175
        .byte 95,95,175,175,95,95,175,175,95,95,175,175,95,95,175,175
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 32,32,10,10,36,133,80,80,133,164,10,10,128,128,0,2
        .byte 0,128,64,24,36,4,80,80,160,164,0,10,32,32,0,0
        .byte 165,165,90,90,165,165,90,90,165,165,90,26,161,37,64,80
        .byte 128,0,64,64,1,0,80,88,160,160,8,64,1,0,0,0
        .byte 8,0,175,175,95,95,175,175,95,95,175,175,95,95,175,175
        .byte 8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 95,78,175,140,95,76,175,140,95,76,175,140,95,76,175,174
        .byte 2,2,171,5,87,78,5,5,93,93,175,175,95,95,175,175
        .byte 2,27,5,174,95,70,170,141,78,95,175,175,95,95,175,175
        .byte 95,95,39,175,87,95,39,175,87,95,39,175,87,95,175,175
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,68,0,5,175
        .byte 240,241,240,240,240,240,242,240,241,240,248,240,245,240,240,248
        .byte 192,0,192,0,192,0,128,0,128,0,0,0,0,0,0,0
        .byte 16,240,16,240,0,240,0,240,0,112,0,112,0,48,0,16
        .byte 0,0,0,0,0,0,0,36,0,66,0,36,0,66,0,0
        .byte 0,48,0,80,64,176,32,240,160,240,208,112,80,240,128,240
        .byte 0,0,0,0,0,48,0,112,0,0,0,48,0,112,0,240
        .byte 180,240,120,240,240,240,240,240,225,225,120,90,165,180,210,210
        .byte 240,252,241,254,241,253,241,255,240,254,240,241,240,243,240,246
        .byte 246,240,253,240,246,240,240,240,252,240,254,240,255,240,247,248
        .byte 16,192,48,240,128,240,240,48,240,240,144,240,0,112,224,16
        .byte 0,0,0,0,192,0,240,0,240,128,224,192,192,96,16,224
        .byte 128,32,90,72,165,164,90,64,164,128,26,0,132,0,0,0
        .byte 0,0,128,0,128,0,192,0,192,0,192,0,192,0,192,0
        .byte 0,160,0,82,0,37,16,90,1,165,18,8,33,37,82,90
        .byte 0,0,72,0,165,0,90,8,165,132,8,72,165,36,90,74
        .byte 4,1,80,90,33,165,2,90,1,37,0,88,0,33,0,0
        .byte 68,34,17,221,51,238,68,255,68,255,119,255,51,238,17,204
        .byte 136,34,51,136,119,204,255,34,255,34,255,238,119,204,51,136
        .byte 164,165,80,90,133,33,26,18,161,33,26,26,4,165,90,0
        .byte 165,45,75,90,45,45,30,15,45,45,75,15,135,45,30,90
        .byte 165,37,90,8,165,165,90,90,165,165,90,90,165,165,82,90
        .byte 129,165,88,90,33,165,90,90,165,165,90,90,165,165,90,74
        .byte 32,68,50,136,0,145,98,98,117,0,16,200,196,0,250,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,74,1,161,16,80
        .byte 0,0,0,0,0,0,16,72,33,36,2,10,33,36,16,72
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 165,165,90,74,165,165,90,90,165,165,90,90,165,132,90,18
        .byte 0,0,0,0,0,0,170,0,10,0,170,0,0,0,0,0
        .byte 0,0,0,0,0,0,170,0,10,0,170,0,0,2,35,170
        .byte 95,95,175,175,95,95,175,175,95,95,191,255,255,255,175,175
        .byte 95,95,175,175,95,95,175,255,127,255,255,175,95,95,175,175
        .byte 95,95,175,175,127,95,175,255,127,255,255,175,95,95,175,175
        .byte 127,223,239,191,95,255,239,239,255,95,255,239,95,223,175,175
        .byte 127,223,239,239,127,255,255,255,127,127,175,239,127,223,175,175
        .byte 95,127,191,239,127,95,191,191,255,223,191,239,127,95,191,255
        .byte 17,100,50,250,196,213,170,234,196,49,98,170,117,100,0,200
        .byte 49,100,34,216,228,160,186,98,213,213,170,250,160,117,200,200
        .byte 0,0,0,0,0,0,0,0,0,0,0,50,0,228,0,216
        .byte 95,8,38,0,76,8,137,4,8,78,0,5,0,2,0,1
        .byte 17,76,38,38,85,27,136,38,27,95,175,171,19,87,1,141
        .byte 87,76,175,38,10,27,170,171,76,78,39,174,87,8,1,0
        .byte 17,221,34,68,17,51,34,187,119,153,170,68,68,0,136,0
        .byte 0,0,0,0,8,0,136,0,78,0,137,0,70,8,137,140
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,2,175,170
        .byte 0,0,0,0,0,0,36,36,66,66,36,36,66,66,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,68,0,234
        .byte 0,0,136,0,128,0,200,0,128,0,0,0,0,0,0,0
        .byte 0,17,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,19,175,175,95,95,175,175,95,95,175,175,95,95,175,175
        .byte 0,82,33,161,66,90,5,165,88,90,165,164,90,26,37,165
        .byte 8,0,164,0,74,8,133,4,10,10,165,160,88,64,37,0
        .byte 0,0,0,0,0,0,0,64,4,128,2,10,33,4,80,10
        .byte 0,0,0,33,16,90,1,165,82,90,165,165,88,82,4,161
        .byte 74,0,133,4,90,10,165,164,90,88,165,165,90,90,164,32
        .byte 0,165,90,90,165,165,90,90,165,165,90,90,33,165,72,90
        .byte 1,1,0,10,132,4,2,26,129,160,88,64,37,5,80,10
        .byte 0,0,0,16,68,128,50,0,0,0,16,34,0,196,0,0
        .byte 0,0,0,0,16,240,96,112,96,16,96,16,96,16,16,240
        .byte 0,0,0,0,245,245,186,16,32,145,98,16,160,145,250,250
        .byte 0,0,0,0,165,165,0,8,33,164,72,24,0,128,90,90
        .byte 0,0,0,0,95,0,136,140,8,76,174,140,8,76,175,0
        .byte 0,0,0,0,245,245,250,200,68,196,136,200,17,196,250,250
        .byte 0,0,250,250,49,213,250,234,245,245,114,234,245,213,114,250
        .byte 0,17,250,234,85,196,170,136,213,68,170,136,117,85,250,234
        .byte 119,136,238,204,68,204,0,204,17,136,34,0,0,0,34,0
        .byte 51,136,119,204,34,102,0,238,17,204,34,0,0,0,68,0
        .byte 51,136,85,204,204,204,238,204,68,136,0,68,0,0,0,34
        .byte 1,15,7,15,15,15,15,15,15,15,15,15,7,15,2,10
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 15,8,15,14,15,15,15,15,15,15,15,15,15,14,10,8
        .byte 0,0,0,0,8,0,5,0,10,8,5,4,10,10,5,5
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 15,15,250,250,125,215,218,122,125,245,235,235,95,165,218,218
        .byte 61,245,250,250,245,215,122,62,245,245,250,190,229,109,250,143
        .byte 245,135,250,250,215,245,235,122,245,245,235,250,125,61,62,250
        .byte 245,245,80,80,160,160,80,80,160,160,80,80,17,17,200,64
        .byte 196,0,16,136,196,100,0,34,128,145,0,80,49,49,16,186
        .byte 0,49,16,136,100,49,64,136,145,0,136,0,213,0,200,136
        .byte 109,229,250,175,125,109,175,175,95,229,235,203,125,165,190,107
        .byte 245,245,250,250,245,245,250,250,245,245,250,250,245,245,170,216
        .byte 0,0,0,0,192,0,240,0,176,128,208,192,224,224,240,224
        .byte 16,192,48,240,128,240,240,112,240,240,240,112,224,240,240,240
        .byte 1,1,64,10,1,32,16,0,0,0,0,0,0,0,0,0
        .byte 128,160,80,0,4,32,16,0,0,0,0,0,0,0,0,0
        .byte 240,180,90,90,240,240,240,210,180,240,240,120,240,240,240,240
        .byte 1,128,2,72,33,128,18,72,161,129,18,64,161,129,18,64
        .byte 17,136,34,204,51,136,51,68,170,153,51,68,170,153,51,68
        .byte 80,80,160,160,80,80,160,160,80,80,160,160,80,80,160,160
        .byte 0,1,0,0,0,4,0,2,0,0,0,0,4,0,2,0
        .byte 8,0,0,0,0,0,0,0,0,8,1,0,0,2,0,4
        .byte 95,31,175,47,79,31,143,47,79,31,143,47,79,95,143,175
        .byte 95,95,180,242,210,244,225,248,240,120,242,180,244,240,175,175
        .byte 0,0,0,0,0,0,0,0,0,0,15,15,10,10,5,5
        .byte 2,10,3,0,2,10,3,0,2,10,3,0,2,10,3,0
        .byte 0,0,2,64,1,5,66,2,1,36,0,72,32,133,24,66
        .byte 0,0,0,8,33,36,88,72,36,161,16,66,33,128,16,64
        .byte 100,196,80,234,100,160,114,98,100,160,80,234,117,160,80,98
        .byte 0,0,255,255,255,255,255,255,170,170,85,85,170,170,255,255
        .byte 228,0,250,136,245,196,250,234,245,228,250,250,245,245,250,250
        .byte 177,85,250,250,245,245,250,234,245,228,216,0,245,128,234,0
        .byte 17,85,250,250,245,245,114,250,117,245,50,250,17,245,0,114
        .byte 0,117,16,250,0,160,114,250,117,245,250,250,245,245,250,250
        .byte 0,0,238,187,85,221,238,187,170,170,102,187,187,238,102,187
        .byte 0,0,119,255,221,68,51,255,170,170,85,85,170,170,85,85
        .byte 0,0,255,238,34,187,255,255,170,170,85,85,170,170,85,85
        .byte 0,0,0,0,0,0,0,0,0,0,136,4,68,25,5,170
        .byte 95,95,225,180,241,244,225,180,241,244,225,180,241,244,175,175
        .byte 0,17,0,34,0,85,0,186,17,117,34,250,85,245,186,80
        .byte 8,0,4,0,10,0,141,0,78,8,175,4,95,10,5,5
        .byte 128,0,64,0,32,0,16,0,0,128,0,64,0,32,0,16
        .byte 0,17,0,34,0,68,0,136,17,0,34,0,68,0,136,0
        .byte 95,95,248,242,90,244,233,248,120,120,250,180,120,210,175,175
        .byte 95,95,180,225,210,245,225,233,240,121,242,165,244,241,175,175
        .byte 0,0,0,0,0,0,0,0,4,0,2,64,32,129,88,64
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,68,2,0,136
        .byte 161,5,90,90,165,165,82,82,160,133,80,2,128,4,0,0
        .byte 245,245,250,250,245,245,250,250,245,245,250,250,245,245,250,250
        .byte 240,240,250,250,241,245,250,250,245,245,242,250,245,245,242,250
        .byte 240,241,250,250,245,244,250,248,245,244,250,248,245,245,250,250
        .byte 135,30,75,75,135,30,165,45,75,30,135,45,75,150,135,15
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 120,225,180,180,120,225,90,210,180,225,120,210,180,105,120,240
        .byte 0,0,0,0,0,16,0,48,0,64,16,144,96,160,144,112
        .byte 0,0,128,0,8,0,132,0,72,0,5,0,16,64,160,161
        .byte 120,225,180,60,120,105,90,90,180,165,120,150,90,195,195,240
        .byte 17,68,136,136,68,17,170,170,0,100,80,64,0,160,152,0
        .byte 32,32,26,80,37,164,80,80,133,133,74,66,129,1,64,64
        .byte 32,161,80,74,165,36,10,88,164,161,80,64,128,129,2,0
        .byte 128,0,26,80,37,164,80,80,133,133,74,66,129,1,64,64
        .byte 1,32,8,8,4,4,10,8,0,4,80,0,0,128,24,2
        .byte 128,32,64,8,4,1,2,10,0,36,80,64,0,160,24,0
        .byte 1,4,8,8,4,1,10,10,4,36,16,64,0,0,0,0
        .byte 128,0,8,8,4,1,10,10,0,36,80,64,0,160,24,0
        .byte 33,132,82,74,133,165,24,10,129,160,90,74,165,165,18,72
        .byte 0,0,66,66,0,161,0,66,0,1,16,8,0,4,0,0
        .byte 0,0,66,18,133,0,66,0,128,0,16,8,32,0,0,0
        .byte 240,165,105,240,210,45,105,180,210,45,105,120,240,225,105,240
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,82,72,33,128,0,0,0,0,0,0,128,0,16,64
        .byte 96,224,96,96,96,64,96,224,32,96,96,224,96,96,96,160
        .byte 0,0,171,5,0,0,4,34,93,27,141,171,93,27,0,0
        .byte 0,0,0,0,0,0,48,192,112,160,240,208,240,240,0,0
        .byte 68,119,187,187,119,85,119,170,255,170,255,238,255,238,119,204
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,51
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,136,0
        .byte 170,170,85,255,34,255,17,119,0,187,0,85,0,34,136,17
        .byte 161,165,82,90,165,165,90,90,165,165,90,90,165,165,90,74
        .byte 95,27,175,141,95,78,175,175,95,95,175,175,95,95,175,175
        .byte 93,95,171,175,87,95,175,175,95,95,175,175,95,95,175,175
        .byte 0,128,0,64,0,32,0,16,0,0,0,0,0,0,0,0
        .byte 17,0,34,0,68,0,136,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,187,136,170,0,170,136,187,136
        .byte 0,0,0,0,0,0,0,0,238,238,34,170,204,170,238,238
bprop:
        .byte 0
        .byte 6
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 6
        .byte 0
        .byte 5
        .byte 0
        .byte 0
        .byte 1
        .byte 1
        .byte 0
        .byte 1
        .byte 6
        .byte 6
        .byte 6
        .byte 5
        .byte 6
        .byte 6
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
        .byte 0
        .byte 0
        .byte 6
        .byte 0
        .byte 6
        .byte 6
        .byte 6
        .byte 5
        .byte 5
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
        .byte 5
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
        .byte 6
        .byte 0
        .byte 6
        .byte 6
        .byte 6
        .byte 6
        .byte 6
        .byte 0
        .byte 6
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
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
        .byte 6
        .byte 0
        .byte 5
        .byte 0
        .byte 0
        .byte 5
        .byte 6
        .byte 5
        .byte 0
        .byte 6
        .byte 6
        .byte 5
        .byte 2
        .byte 2
        .byte 2
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 6
        .byte 6
        .byte 5
        .byte 5
        .byte 6
        .byte 6
        .byte 6
        .byte 6
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
        .byte 0
        .byte 0
        .byte 5
        .byte 5
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
        .byte 5
        .byte 6
        .byte 6
        .byte 6
        .byte 6
        .byte 6
        .byte 5
        .byte 0
        .byte 6
        .byte 6
        .byte 6
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 0
        .byte 6
        .byte 6
        .byte 6
        .byte 6
        .byte 0
        .byte 0
.out .sprintf("BLOCKS SIZE = %d", (* - chgfx))

sprgfx:
        .byte 0,0,48,0,31,192,111,224,28,0,63,44,127,147,127,252,127,248,127,244,127,244,63,244,255,232,127,240,47,224,71,128
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,99,0,99,0,1,0,0,0,98,0,3,0,1,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,192,1,224,17,224,0,192,0,0,17,32,1,224,0,192,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,48,0,120,0,48,0,0,0,72,0,120,0,48,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,0,0,0,0,0,128,0,0,0,0,0,0,0,0,0
        .byte 15,240,63,212,127,170,127,214,255,255,239,247,247,239,124,62,139,209,103,230,181,235,245,175,103,230,11,208,188,61,159,249
        .byte 15,240,63,212,127,170,127,214,255,255,231,231,223,251,124,62,139,209,103,230,213,237,245,175,103,226,12,112,185,189,159,249
        .byte 0,0,14,112,17,136,26,216,31,120,26,216,31,120,30,248,31,120,30,248,31,120,18,152,31,120,18,152,31,120,18,152
        .byte 0,0,4,0,10,80,0,0,14,112,17,136,17,136,27,88,30,248,27,88,30,248,31,120,30,248,31,120,18,152,31,120
        .byte 18,20,18,64,0,72,34,0,8,80,6,48,17,136,17,136,27,88,30,248,27,88,30,248,31,120,30,248,31,120,30,248
        .byte 0,0,14,0,14,0,14,56,0,56,14,56,0,0,0,56,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,56,0,56,14,56,14,0,14,56,0,0,14,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,64,0,56,0,0,0,0,28,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,7,0,8,0,1,0,0,224,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,1,0,0,224,0,0,0,0,7,0,8,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,14,0,16,0,0,128,0,112,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 15,240,63,212,127,170,127,214,231,255,223,231,252,59,123,222,143,241,101,230,181,173,247,239,211,201,60,60,191,253,159,249
        .byte 0,0,0,0,0,0,3,128,7,64,27,176,61,104,62,116,126,244,126,252,61,120,230,100,153,152,153,152,102,102,127,254
        .byte 0,0,0,0,0,0,1,192,3,160,13,216,30,180,31,58,63,122,63,126,30,188,38,103,25,153,25,153,102,102,127,254
        .byte 0,0,0,0,0,0,0,0,0,0,28,198,77,173,237,239,77,239,12,198,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,28,198,77,173,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,15,56,76,116,239,124,67,124,15,56,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,15,56,76,116,3,56,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,121,158,236,51,181,173,180,45,204,55,120,30,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,192,121,158,205,183,180,45,180,45,236,51,120,30,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 1,0,2,0,10,128,7,0,31,128,56,0,62,142,126,31,127,127,191,135,95,240,191,127,126,230,63,192,15,128,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,4,0,2,0,30,0,7,0,15,128,15,192,31,224,31,240,63,240,191,0,95,212,191,239,95,240,127,246,63,248
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,3,128,7,64,27,176,61,104,62,116,126,244,126,252,126,252,61,120,128,2,255,254,255,254,255,254
        .byte 0,0,32,0,220,0,63,0,124,0,124,158,250,100,255,216,255,48,255,160,127,192,191,192,127,128,31,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 2,0,26,128,7,128,7,224,14,14,31,95,31,47,63,239,63,240,191,231,95,239,191,255,127,246,63,248,31,240,7,192
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 6,0,4,0,0,0,15,128,63,192,112,0,124,160,254,124,255,227,255,220,255,248,127,224,126,160,61,64,10,128,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
frmlst:
        .byte 0,2
        .byte 2,4
        .byte 6,2
        .byte 8,3
        .byte 11,2
        .byte 13,4
        .byte 17,1
        .byte 18,2
        .byte 20,3
        .byte 23,3
        .byte 26,2
        .byte 28,1
        .byte 29,2
        .byte 31,2
        .byte 33,1
        .byte 34,2
        .byte 36,2
        .byte 38,2,40,0
.out .sprintf("SPRITES SIZE = %d", (* - sprgfx))

scdat:
        .word 416,286,313,325,393,294,342,247,280,321,282,514,279,331,461,338,255,423,356,354,308,128,355
        .byte 255,0,9,110,111,110,0,111,0,0,111,0,0,110,0,111,110,111,255,0,17,111,0,111,110,0,111,110,0,111,110,0,111
        .byte 110,0,110,255,0,17,111,111,110,111,0,110,111,0,110,111,0,110,111,110,110,255,0,17,110,110,0,0,110,0,0,110,0,0
        .byte 110,0,0,111,111,255,0,17,111,110,111,110,0,111,110,0,111,110,0,111,110,111,110,255,0,17,110,0,110,111,0,110,111
        .byte 0,110,111,0,110,111,0,111,255,0,17,111,110,0,0,110,255,0,4,110,110,0,0,111,110,255,0,17,111,110,111,110,110
        .byte 111,110,0,111,110,0,111,110,111,110,255,0,17,110,0,110,111,0,110,111,0,110,111,0,110,111,0,111,255,0,17,111,110
        .byte 0,0,110,0,0,110,0,0,110,0,0,111,110,255,0,17,111,110,111,110,0,111,110,0,111,110,0,111,110,111,110,255,0,17
        .byte 110,0,110,111,0,110,111,0,110,111,0,110,111,0,111,255,0,17,111,110,0,0,110,0,0,110,0,0,110,0,0,111,110
        .byte 255,0,17,111,110,111,110,0,111,110,0,111,110,0,111,110,111,110,255,0,17,110,0,110,111,0,110,111,0,110,111,0,110
        .byte 111,0,111,255,0,17,111,110,0,111,110,0,111,110,111,110,111,110,0,111,110,255,0,17,111,110,111,110,0,111,110,0,111
        .byte 110,0,111,110,111,110,255,0,17,110,0,110,111,0,110,111,0,110,111,0,110,111,0,111,255,0,17,111,110,0,0,110,0,0
        .byte 110,0,0,110,0,0,111,110,255,0,17,111,110,111,110,0,111,110,0,111,110,0,111,110,111,110,255,0,17,110,0,110,111
        .byte 0,110,111,0,110,111,0,110,111,0,111,255,0,17,111,110,0,0,110,0,0,110,0,0,110,0,0,111,110,255,0,17,111
        .byte 110,111,110,174,79,83,80,81,82,173,111,110,111,110,255,0,8
        .byte 8,0,20,255,3,6,17,255,0,11,20,3,3,3,17,255,0,6,69,70,255,3,8,19,18,19,18,19,18,19,18,19,18
        .byte 19,255,3,5,255,15,5,16,8,255,3,20,52,53,54,55,6,255,3,5,8,8,255,3,30,8,8,255,3,30,8,8,255,3,30
        .byte 8,8,255,3,30,8,8,255,3,30,8,8,255,3,30,8,8,255,3,30,8,8,255,3,30,8,8,255,3,30,8,8,255,3,30
        .byte 8,8,255,3,30,8,8,255,3,30,8,8,255,7,23,3,255,7,6,8,8,137,5,137,4,13,5,137,137,13,5,13,137
        .byte 4,5,13,5,13,4,13,5,13,137,13,5,13,5,13,5,4,5,8,0,67,105,105,104,105,77,105,104,74,72,105,73,104
        .byte 105,9,9,105,105,104,73,104,71,72,105,104,105,105,105,104,61,64,59,58,68,255,0,4,71,75,77,0,0,10,0,0,10,10
        .byte 255,0,5,73,74,75,136,255,0,4,63,62,57,60,136,107,107,108,107,108,107,108,107,108,107,108,107,108,107,108,107,108,107
        .byte 108,107,108,107,108,107,107,45,46,44,0,27,31,32,255,0,25,35,48,76,36,26,29,30,34,25,255,66,22,47,38,40,41
        .byte 39,24,22,28,23,50,255,65,11,255,21,11,49,37,42,43,33
        .byte 8,0,20,255,3,6,17,255,0,11,20,3,3,3,17,255,0,6,69,70,255,3,8,19,18,19,18,19,18,19,18,19,18
        .byte 19,255,3,5,255,15,5,16,8,255,3,8,51,56,56,55,54,53,6,255,3,15,8,8,255,3,30,8,8,255,3,30,8,8
        .byte 255,3,30,8,8,255,3,30,8,8,255,3,30,8,8,255,3,30,8,8,255,3,30,8,8,255,3,30,8,8,255,3,30
        .byte 8,8,255,3,30,8,8,255,3,30,8,8,255,3,30,8,8,138,101,138,138,101,138,101,138,101,138,101,101,138,101,138,138,138
        .byte 101,101,138,138,101,138,138,101,138,101,138,138,101,8,8,137,5,13,4,13,5,13,4,13,5,13,5,4,5,13,5,13,5
        .byte 13,5,137,5,13,5,13,5,13,5,4,5,8,0,67,105,105,104,105,104,73,71,72,105,104,104,105,104,9,9,104,105,74
        .byte 72,117,105,104,104,105,105,77,104,104,61,64,59,58,68,0,135,0,136,255,0,4,73,74,75,0,10,10,0,0,0,116,0,0
        .byte 71,75,135,255,0,4,63,62,57,60,136,107,107,108,107,108,107,108,107,108,107,108,107,108,107,108,107,108,107,108,107,108,107
        .byte 108,107,107,45,46,44,0,27,31,32,255,0,25,35,48,76,36,26,29,30,34,25,255,66,22,47,38,40,41,39,24,22,28
        .byte 23,50,255,65,11,255,21,11,49,37,42,43,33
        .byte 8,0,20,255,3,6,17,255,0,11,20,3,3,3,17,255,0,6,69,70,255,3,8,19,18,19,18,19,18,19,18,19,18
        .byte 19,255,3,5,255,15,5,16,8,3,3,51,53,56,56,54,53,6,255,3,19,51,56,8,8,255,3,30,8,8,255,3,30
        .byte 8,8,255,3,28,106,106,8,8,255,3,28,169,100,8,8,255,3,29,100,8,8,255,3,29,100,8,8,255,3,26,255,106,4
        .byte 8,8,255,3,26,169,100,170,3,8,8,255,3,27,100,3,3,8,8,255,3,27,100,3,3,8,8,255,3,4,84,85,84
        .byte 85,255,3,16,255,106,6,8,8,255,3,4,84,85,84,85,255,3,16,169,100,170,3,3,3,8,8,255,7,4,84,85,84
        .byte 85,84,85,255,7,15,100,255,7,4,8,8,137,5,13,137,84,85,84,85,84,85,137,13,5,4,13,5,33,104,37,137,5
        .byte 137,5,5,4,100,13,5,4,137,8,0,135,105,84,85,84,77,84,85,84,85,105,255,9,11,105,105,104,74,75,105,104,61
        .byte 64,59,58,68,255,0,4,71,72,73,0,255,10,13,117,10,10,0,0,0,63,62,57,60,136,108,107,108,77,108,107,108,107
        .byte 108,107,108,107,108,107,108,107,108,107,73,107,108,135,108,107,108,45,46,44,0,27,31,32,255,0,25,35,48,76,36,26,29
        .byte 30,34,25,255,66,22,47,38,40,41,39,24,22,28,23,50,255,65,11,255,21,11,49,37,42,43,33
        .byte 8,0,20,255,3,6,17,255,0,11,20,3,3,3,17,255,0,6,69,70,52,55,6,255,3,5,19,18,19,18,19,18,19
        .byte 18,19,18,19,255,3,5,255,15,5,16,8,55,56,55,54,53,6,255,3,14,52,53,54,55,6,255,3,5,8,8,255,3,30
        .byte 8,8,255,3,30,8,8,255,3,30,8,8,255,3,30,8,8,255,3,23,133,113,134,255,3,4,8,8,255,3,24,112,255,3,5
        .byte 8,8,255,3,4,7,255,3,19,112,255,3,5,8,8,255,3,5,113,113,134,255,3,13,133,255,113,5,134,3,3,8,8
        .byte 255,3,6,112,255,3,15,112,3,3,3,112,3,3,3,8,8,255,3,6,112,255,3,15,112,3,3,3,112,3,3,3,8,8
        .byte 255,3,4,133,113,113,113,134,255,3,11,133,113,113,113,128,113,113,113,134,3,8,8,255,3,4,112,3,3,3,112,255,3,11
        .byte 112,3,3,3,112,3,3,3,112,3,8,8,3,7,3,3,112,84,85,7,112,3,255,7,10,112,7,84,85,112,7,7,3
        .byte 112,7,8,8,5,137,13,84,85,84,85,84,85,5,13,5,4,5,13,4,13,137,13,5,84,85,84,85,84,85,137,5,5
        .byte 4,8,0,104,84,85,84,85,84,85,84,85,105,255,9,8,105,104,84,77,84,85,84,85,84,85,105,61,64,59,58,68,255,0,5
        .byte 73,71,75,116,255,10,7,0,10,255,0,4,77,255,0,4,63,62,57,60,136,107,107,108,107,108,107,108,107,108,107,108,107
        .byte 108,107,108,107,108,107,108,107,108,107,108,107,107,45,46,44,0,27,31,32,255,0,25,35,48,76,36,26,29,30,34,25,255,66,22
        .byte 47,38,40,41,39,24,22,28,23,50,255,65,11,255,21,11,49,37,42,43,33
        .byte 255,0,6,16,255,0,15,16,255,0,16,69,255,0,14,123,120,255,0,7,8,8,0,16,255,0,4,16,255,0,12,69,0
        .byte 122,121,255,0,7,16,8,255,0,30,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30
        .byte 8,255,0,26,125,119,119,124,119,8,8,255,0,26,118,0,0,0,8,255,0,27,118,0,0,0,8,255,0,23,125,119,119
        .byte 124,255,119,4,8,255,0,24,118,255,0,4,118,0,8,0,125,119,126,255,0,20,118,255,0,4,118,0,8,0,0,118,255,0,18
        .byte 125,119,119,124,119,119,119,124,119,119,0,8,73,118,136,135,136,135,93,136,127,136,127,93,136,135,136,136,127,135,127,135,136
        .byte 118,73,11,12,118,104,77,127,118,0,8,67,71,72,116,0,0,255,9,12,74,72,117,118,0,0,0,118,73,0,77,61,64
        .byte 59,58,68,0,0,0,77,255,10,12,255,0,4,71,75,116,255,0,4,63,62,57,60,136,107,107,108,107,108,107,108,107,108
        .byte 107,108,107,108,107,108,107,108,107,108,107,108,107,108,107,107,45,46,44,0,27,31,32,255,0,25,35,48,76,36,26,29,30
        .byte 34,25,255,66,22,47,38,40,41,39,24,22,28,23,50,255,65,11,255,21,11,49,37,42,43,33
        .byte 8,0,0,0,123,120,0,0,0,16,255,0,13,69,255,0,8,8,0,0,16,122,121,255,0,17,16,255,0,7,8,8,255,0,7
        .byte 16,255,0,15,69,255,0,6,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30,8,8,255,0,8
        .byte 129,130,255,0,20,8,8,255,0,7,129,84,84,130,255,0,19,8,8,255,0,7,118,1,1,118,255,0,19,8,8,255,0,4
        .byte 129,130,0,118,1,1,118,255,0,19,8,8,0,0,0,129,84,84,130,84,84,84,85,255,0,19,8,8,0,0,0,118,131
        .byte 0,118,118,1,1,118,255,0,19,8,8,0,0,0,118,0,131,118,118,1,1,118,255,0,19,8,8,0,0,0,84,84,84
        .byte 85,84,84,84,85,255,0,19,8,8,0,0,0,118,1,1,118,118,0,132,118,255,0,19,8,8,0,0,136,118,1,1,118,118
        .byte 132,0,118,136,127,135,127,135,136,136,93,136,135,127,136,93,93,136,135,136,127,77,8,0,116,105,129,84,84,84,85,84,85
        .byte 71,72,117,255,9,11,73,74,75,77,73,105,61,64,59,58,68,0,0,0,74,72,116,0,0,0,255,10,13,0,73,0,10
        .byte 0,63,62,57,60,136,107,107,108,107,108,107,108,107,108,107,108,107,108,107,108,107,108,107,108,107,108,107,108,107,107,45,46
        .byte 44,0,27,31,32,255,0,25,35,48,76,36,26,29,30,34,25,255,66,22,47,38,40,41,39,24,22,28,23,50,255,65,11
        .byte 255,21,11,49,37,42,43,33
        .byte 8,255,0,7,16,255,0,15,16,255,0,7,8,120,255,0,20,16,255,0,8,8,8,121,0,0,0,16,255,0,4,16,255,0,16
        .byte 69,0,0,0,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30
        .byte 8,8,255,0,30,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30
        .byte 8,8,136,127,135,136,0,136,127,135,127,135,127,93,136,127,135,127,136,93,127,136,135,127,135,127,135,0,136,73,135,136,8
        .byte 0,67,0,74,75,73,0,0,255,9,16,0,0,135,74,72,105,61,64,59,58,68,0,0,0,71,72,77,255,10,14,116,71
        .byte 72,255,0,4,63,62,57,60,136,107,107,108,107,108,107,108,107,108,107,108,107,108,107,108,107,108,107,108,107,108,107,108,107,107
        .byte 45,46,44,0,27,31,32,255,0,25,35,48,76,36,26,29,30,34,25,255,66,22,47,38,40,41,39,24,22,28,23,50,255,65,11
        .byte 255,21,11,49,37,42,43,33
        .byte 8,255,0,21,16,255,0,9,8,0,0,0,16,255,0,20,123,120,16,0,0,0,8,8,255,0,7,16,255,0,15,16,122
        .byte 121,0,16,0,0,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30
        .byte 8,8,255,0,30,8,8,255,0,30,8,8,255,0,30,8,8,255,0,7,255,139,4,140,255,0,18,8,8,0,163,163,163
        .byte 255,0,4,131,100,132,255,0,19,8,8,0,171,118,172,255,0,5,100,255,0,20,8,8,0,0,118,255,0,6,100,255,0,20
        .byte 8,8,116,135,118,135,127,136,136,136,93,100,136,136,127,135,127,136,93,136,127,136,135,136,93,136,135,135,136,0,136,135,8,8
        .byte 67,104,116,154,105,0,105,104,73,97,104,255,9,15,116,0,105,61,64,59,58,68,0,0,0,77,0,71,75,77,73,255,10,14
        .byte 135,0,0,0,63,62,57,60,136,107,107,108,107,108,107,108,107,108,107,108,107,108,107,108,107,108,107,108,107,108,107,108,107,107
        .byte 45,46,44,0,27,31,32,255,0,25,35,48,76,36,26,29,30,34,25,255,66,22,47,38,40,41,39,24,22,28,23,50,255,65,11
        .byte 255,21,11,49,37,42,43,33
        .byte 8,0,0,0,69,255,0,16,69,255,0,10,8,255,0,5,69,0,123,120,255,0,13,69,255,0,7,8,8,0,0,69,255,0,4
        .byte 122,121,69,255,0,6,69,255,0,9,69,0,0,0,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30
        .byte 8,8,255,0,30,8,8,255,0,30,8,8,255,0,30,8,8,255,0,20,139,140,84,85,255,0,6,8,8,255,0,20,84
        .byte 85,139,140,255,0,6,8,8,255,0,20,139,140,84,85,255,0,6,8,8,255,0,18,139,140,84,85,139,140,255,0,6,8,8
        .byte 255,0,18,84,85,139,140,84,85,139,140,255,0,4,8,8,255,0,16,84,85,139,140,84,85,139,140,84,85,255,0,4,8,8
        .byte 0,136,127,127,135,135,136,93,135,127,136,135,136,127,127,136,139,140,84,85,139,140,84,85,139,140,135,136,127,136,8,0,67
        .byte 105,104,73,74,72,255,9,10,84,85,139,140,84,85,139,140,84,85,105,0,104,61,64,59,58,68,0,0,0,10,142,255,10,9
        .byte 0,117,0,0,0,71,75,73,255,0,5,63,62,57,60,136,107,107,108,107,108,107,108,107,108,107,108,107,108,107,108,73,108
        .byte 107,108,107,108,107,108,107,107,45,46,44,0,27,31,32,255,0,25,35,48,76,36,26,29,30,34,25,255,66,22,47,38,40
        .byte 41,39,24,22,28,23,50,255,65,11,255,21,11,49,37,42,43,33
        .byte 8,255,0,6,69,255,0,17,69,255,0,5,8,8,255,0,21,69,255,0,7,123,8,8,0,69,255,0,7,69,255,0,15
        .byte 69,0,0,0,122,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30
        .byte 8,8,255,0,30,8,8,255,0,30,8,8,255,0,30,8,0,0,255,114,4,255,0,24,114,8,8,0,119,124,119,119,126
        .byte 255,0,22,125,119,8,8,0,114,114,114,115,114,255,0,21,114,114,115,8,8,0,119,119,124,119,119,126,255,0,19,125,119,119
        .byte 124,8,8,8,136,115,136,127,115,136,135,127,135,136,93,93,136,127,136,135,127,135,136,135,127,93,135,127,135,127,115,135,136
        .byte 8,0,67,0,0,0,73,255,0,4,116,255,9,12,0,77,255,0,5,61,64,59,58,68,255,0,4,77,0,0,255,10,12
        .byte 2,0,0,0,117,0,0,0,63,62,57,60,0,12,108,107,12,12,107,108,107,11,12,107,108,107,11,12,107,108,107,14,12
        .byte 107,108,14,2,108,45,46,44,0,27,31,32,255,0,25,35,48,76,36,26,29,30,34,25,255,66,22,47,38,40,41,39,24
        .byte 22,28,23,50,255,65,11,255,21,11,49,37,42,43,33
        .byte 8,157,152,147,149,153,147,151,143,143,141,141,148,149,148,149,148,149,148,149,148,152,147,149,147,143,143,141,149,148,156,8,8
        .byte 150,147,149,153,148,154,151,143,143,141,141,149,148,154,148,149,154,149,148,149,148,147,148,151,143,143,141,152,150,154,8,8,122
        .byte 121,149,147,147,153,149,143,143,141,141,153,147,149,148,153,149,153,147,149,153,149,147,149,143,143,141,150,151,147,8,8,255,0,6
        .byte 142,143,143,141,141,255,0,5,142,153,147,150,147,150,147,153,143,143,141,153,147,0,8,8,255,0,7,143,143,141,141,255,0,7
        .byte 142,153,150,153,0,142,143,143,141,142,0,0,8,8,255,0,6,142,143,143,141,141,255,0,12,142,143,143,141,0,0,0,8,8
        .byte 255,0,7,143,143,141,141,255,0,12,142,143,158,141,0,0,0,8,8,255,0,6,142,158,143,141,141,255,0,12,142,143,143
        .byte 141,0,0,0,8,8,255,0,7,143,143,141,141,255,0,12,142,143,143,141,0,0,0,8,8,255,0,6,142,143,143,141,141
        .byte 255,0,12,142,143,143,141,0,0,0,8,8,255,0,7,143,143,141,141,255,0,12,142,143,143,141,0,0,0,8,8,255,0,6
        .byte 142,143,143,141,141,255,0,12,142,158,143,141,0,0,0,8,8,255,0,7,143,158,141,141,255,0,12,142,143,143,141,0,0,0
        .byte 8,8,255,0,6,142,143,143,141,141,255,0,12,142,143,143,141,0,0,0,8,8,255,0,7,143,143,141,141,255,0,12,142
        .byte 143,143,141,0,0,0,8,8,255,0,6,142,143,143,141,141,255,0,12,142,143,143,141,0,0,0,8,8,73,77,73,117,73
        .byte 136,135,143,143,141,141,136,127,136,93,136,136,135,136,135,127,136,135,144,143,146,141,145,127,73,8,0,67,104,71,72,105,104,104
        .byte 143,143,141,141,105,255,9,10,117,74,75,152,104,105,104,61,64,59,58,68,0,0,0,117,144,143,146,141,141,145,255,10,11
        .byte 0,117,104,0,0,0,63,62,57,60,104,12,108,107,108,12,2,14,107,108,107,2,108,107,108,12,107,108,107,14,12,2,108
        .byte 107,108,14,45,46,44,0,27,31,32,255,0,25,35,48,76,36,26,29,30,34,25,255,66,22,47,38,40,41,39,24,22,28
        .byte 23,50,255,65,11,255,21,11,49,37,42,43,33
        .byte 8,0,0,0,16,0,123,120,255,0,18,16,255,0,4,8,8,0,0,0,16,0,122,121,0,16,255,0,13,16,255,0,7
        .byte 8,8,0,16,0,0,16,255,0,8,16,255,0,7,16,0,0,0,16,255,0,5,8,255,0,30,8,8,255,0,30,8,8
        .byte 255,0,30,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30
        .byte 8,255,0,31,8,8,255,0,30,8,8,255,0,22,85,85,85,255,0,4,114,8,8,255,0,22,85,85,85,255,0,4,115
        .byte 8,8,8,136,0,136,127,0,136,135,136,136,136,135,127,136,93,136,135,127,135,136,85,85,85,136,85,85,85,136,135,115,8
        .byte 0,67,0,0,0,255,9,13,73,0,74,85,85,85,116,85,85,85,0,0,61,64,59,58,68,0,0,255,10,14,77,0,0
        .byte 73,71,72,0,117,0,0,0,63,62,57,60,0,12,108,107,108,12,107,108,107,116,107,107,108,107,108,12,107,108,107,14,12
        .byte 107,108,107,2,108,45,46,44,0,27,31,32,255,0,25,35,48,76,36,26,29,30,34,25,255,66,22,47,38,40,41,39,24
        .byte 22,28,23,50,255,65,11,255,21,11,49,37,42,43,33
        .byte 8,255,0,25,16,255,0,4,8,8,0,0,0,16,0,123,120,0,16,255,0,13,16,255,0,7,8,8,0,16,0,0,16
        .byte 122,121,255,0,6,16,255,0,7,16,0,0,0,16,0,0,0,8,0,8,84,85,255,0,27,8,8,8,109,85,255,0,27
        .byte 8,8,8,109,85,255,0,27,8,8,8,109,85,255,0,27,8,8,8,84,85,255,0,27,8,8,8,255,139,4,140,255,0,24
        .byte 8,8,8,139,131,0,0,140,255,0,24,8,8,8,139,0,131,0,140,255,0,24,8,8,8,139,0,0,131,140,255,0,24
        .byte 8,8,0,255,139,4,140,255,0,24,8,8,8,84,84,85,255,139,4,140,255,0,20,139,140,8,8,109,109,85,139,157,0
        .byte 156,140,255,0,20,84,85,8,8,109,109,85,139,39,155,39,140,255,0,18,139,140,139,140,8,8,109,109,85,139,0,160,129
        .byte 140,136,136,136,135,127,136,93,136,135,127,135,136,136,93,93,136,127,73,84,85,84,85,8,0,84,84,85,255,139,4,140,105
        .byte 255,9,14,104,71,72,77,104,116,61,64,59,58,68,255,0,5,74,75,77,255,10,15,255,0,4,63,62,57,60,104,12,108
        .byte 107,108,12,107,108,12,11,107,108,2,107,108,14,107,108,11,14,108,107,11,108,107,108,45,46,44,0,27,31,32,255,0,25
        .byte 35,48,76,36,26,29,30,34,25,255,66,22,47,38,40,41,39,24,22,28,23,50,255,65,11,255,21,11,49,37,42,43,33
        .byte 8,141,143,149,147,150,141,143,143,151,149,150,148,147,150,149,148,147,148,149,148,152,147,149,153,148,149,148,149,153,156,8,8
        .byte 141,143,148,149,150,141,143,143,150,150,147,149,148,147,148,149,150,149,147,149,148,147,148,151,150,147,149,152,148,149,8,8,141
        .byte 143,147,150,153,141,143,143,151,149,153,150,153,149,153,150,150,153,148,149,153,149,147,149,147,151,150,150,151,153,8,8,141,143
        .byte 150,153,149,141,143,143,149,153,0,0,0,153,150,152,153,0,153,154,153,0,122,121,153,149,147,153,142,143,8,8,141,143,0
        .byte 153,153,141,143,143,255,0,17,142,0,0,142,143,8,8,141,143,0,0,142,141,143,143,255,0,19,142,142,143,8,8,141,143
        .byte 0,0,0,141,143,143,255,0,20,142,143,8,8,141,143,0,0,142,141,143,143,255,0,21,158,8,8,141,158,0,0,0,141
        .byte 143,143,255,0,20,142,143,8,8,141,143,0,0,142,141,143,143,255,0,21,143,8,8,141,143,0,0,0,141,143,143,255,0,20
        .byte 142,143,8,8,141,143,0,0,142,141,143,158,255,0,21,143,8,8,141,143,0,0,0,141,143,143,255,0,20,142,143,8,8
        .byte 141,143,0,0,0,141,143,143,255,0,21,143,8,8,141,143,0,0,0,141,143,143,255,0,20,117,146,8,8,141,143,0,0
        .byte 142,141,143,143,255,0,17,135,0,104,136,73,8,8,141,146,145,135,117,141,143,143,77,135,136,136,127,136,127,136,93,136,136
        .byte 135,93,136,127,136,144,141,146,145,0,0,8,0,104,77,73,136,104,141,143,143,105,0,0,105,255,9,12,0,0,0,117,135
        .byte 61,64,59,58,68,77,104,144,141,158,146,145,136,117,0,255,10,11,73,71,75,0,136,105,63,62,57,60,0,73,108,117,108
        .byte 11,77,14,107,108,77,14,108,107,108,11,107,108,107,77,12,2,108,117,108,77,45,46,44,0,27,31,32,255,0,25,35,48
        .byte 76,36,26,29,30,34,25,255,66,22,47,38,40,41,39,24,22,28,23,50,255,65,11,255,21,11,49,37,42,43,33
        .byte 8,255,0,7,16,255,0,15,16,255,0,7,8,255,0,30,8,8,255,0,4,16,255,0,11,16,123,120,255,0,7,69,0,0,0
        .byte 8,8,255,0,17,122,121,0,16,255,0,9,8,8,255,0,29,159,8,8,255,0,30,8,8,255,0,29,159,8,8,255,0,30
        .byte 8,8,255,0,29,159,8,8,255,0,18,159,125,119,119,124,119,119,119,124,119,119,126,8,8,255,0,19,159,118,0,0,0
        .byte 118,0,0,0,118,159,8,8,255,0,17,47,0,132,118,131,0,132,118,131,0,132,118,131,8,8,255,0,18,159,71,72,71
        .byte 72,71,72,71,72,71,72,71,8,8,255,0,19,105,71,72,71,72,71,72,71,72,71,75,8,8,255,0,18,159,71,72,71
        .byte 84,85,72,71,75,74,72,74,8,8,255,0,19,104,71,72,84,85,84,85,71,72,71,75,8,8,136,127,135,136,93,136,127
        .byte 135,127,135,127,93,136,127,135,127,136,93,0,71,84,85,84,85,84,85,72,71,72,71,8,0,67,0,116,0,255,9,14,84
        .byte 85,84,85,84,85,84,85,105,77,0,61,64,59,58,68,0,0,255,10,14,84,85,84,85,84,85,84,85,0,0,0,63,62
        .byte 57,60,136,107,77,12,107,108,12,108,107,12,107,108,107,77,107,108,107,108,77,108,107,12,107,108,77,107,45,46,44,0,27
        .byte 31,32,255,0,25,35,48,76,36,26,29,30,34,25,255,66,22,47,38,40,41,39,24,22,28,23,50,255,65,11,255,21,11
        .byte 49,37,42,43,33
        .byte 8,255,0,30,8,8,0,0,0,16,255,0,17,16,255,0,8,8,8,0,16,255,0,9,16,0,0,123,120,0,16,255,0,6
        .byte 16,255,0,5,8,8,255,0,13,16,122,121,255,0,13,159,8,8,255,0,30,8,8,255,0,29,159,8,8,255,0,30,8,8
        .byte 255,0,29,159,8,8,255,0,30,8,8,255,0,29,159,8,8,255,0,30,8,8,255,0,29,159,8,255,0,31,8,8,255,0,29
        .byte 159,8,8,255,0,30,8,8,255,0,30,8,8,8,255,0,4,93,136,136,127,135,136,135,136,136,93,136,135,127,135,136,135
        .byte 127,136,136,0,0,0,135,0,136,8,0,67,0,136,135,136,0,0,0,255,9,15,0,0,144,143,146,145,61,64,59,58,68
        .byte 0,0,0,77,0,255,10,16,0,73,255,0,4,63,62,57,60,0,73,108,116,77,14,107,108,77,11,108,107,108,77,2,107
        .byte 108,2,77,108,108,12,77,116,108,73,45,46,44,0,27,31,32,255,0,25,35,48,76,36,26,29,30,34,25,255,66,22,47
        .byte 38,40,41,39,24,22,28,23,50,255,65,11,255,21,11,49,37,42,43,33
        .byte 8,255,0,6,69,255,0,23,8,8,0,0,123,120,0,69,255,0,16,69,255,0,7,8,8,0,16,122,121,255,0,5,69
        .byte 255,0,17,16,0,0,8,8,255,0,30,8,8,255,0,29,159,8,8,255,0,30,8,8,0,0,0,159,255,84,5,85,84,84,84
        .byte 85,84,84,84,85,84,84,84,85,255,84,4,85,0,0,159,8,8,255,0,4,171,118,131,131,131,118,157,0,156,118,157,0
        .byte 156,118,157,0,156,118,131,131,131,118,172,0,0,0,8,8,255,0,4,159,118,131,131,131,118,38,155,38,118,38,155,39,118
        .byte 39,155,39,118,131,131,131,118,0,0,0,159,8,8,255,0,4,159,118,131,131,131,118,130,160,0,118,0,160,0,118,0,160
        .byte 129,118,131,131,131,118,255,0,4,8,8,255,0,5,118,84,84,84,85,84,84,84,85,84,84,84,85,84,84,84,85,84,84,84
        .byte 118,0,0,0,159,8,8,255,0,4,159,118,172,255,0,17,171,118,255,0,4,8,255,0,6,118,255,0,19,118,0,0,0
        .byte 159,8,8,255,0,4,159,118,255,0,19,118,255,0,4,8,8,0,0,0,84,85,118,255,0,19,118,84,85,0,0,8,8
        .byte 0,0,0,139,140,118,255,0,19,118,139,140,0,0,8,8,8,136,0,84,85,118,139,140,127,135,136,93,93,136,127,136,135
        .byte 127,93,136,135,127,93,139,140,118,84,85,0,136,8,0,67,0,139,140,97,97,84,85,255,9,15,84,85,97,97,139,140,61
        .byte 64,59,58,68,0,0,77,0,0,255,10,16,0,73,255,0,4,63,62,57,60,0,73,108,107,108,117,107,108,107,11,77,107
        .byte 108,107,108,107,77,2,107,108,107,116,108,107,108,73,45,46,44,0,27,31,32,255,0,25,35,48,76,36,26,29,30,34,25
        .byte 255,66,22,47,38,40,41,39,24,22,28,23,50,255,65,11,255,21,11,49,37,42,43,33
        .byte 8,0,0,0,16,255,0,17,16,255,0,14,123,120,0,16,255,0,15,16,255,0,7,8,0,0,16,0,122,121,255,0,5
        .byte 16,255,0,10,16,255,0,4,16,0,0,8,8,255,0,30,8,8,255,0,25,114,114,114,0,114,8,8,255,0,24,159,0
        .byte 115,0,0,0,8,8,255,0,22,159,0,0,0,115,0,0,0,8,8,255,0,23,114,114,114,115,114,114,114,8,8,255,0,22
        .byte 159,0,115,0,0,0,115,0,8,8,255,0,20,159,0,0,0,115,0,0,0,115,0,8,8,255,0,21,114,114,114,115,114,114,114
        .byte 115,114,8,8,255,0,20,159,0,115,0,0,0,115,0,0,0,8,8,255,0,20,255,139,4,140,255,139,4,140,8,8,255,0,19
        .byte 114,114,114,97,114,114,114,115,97,114,114,8,8,255,0,20,115,0,100,0,115,0,0,100,115,0,8,8,255,0,20,115,0
        .byte 84,85,115,0,0,100,115,0,8,8,136,127,135,135,136,136,93,136,136,127,136,136,127,135,127,136,93,93,84,85,84,85,84
        .byte 85,115,0,136,100,115,136,8,0,67,0,104,255,9,15,84,85,84,85,84,85,105,9,9,97,73,61,64,59,58,68,0,0,0
        .byte 255,10,14,73,74,75,77,10,10,10,0,0,0,63,62,57,60,0,116,12,107,117,14,2,108,107,77,12,108,107,12,108,107
        .byte 108,107,108,116,12,107,108,14,77,14,45,46,44,0,27,31,32,255,0,25,35,48,76,36,26,29,30,34,25,255,66,22,47
        .byte 38,40,41,39,24,22,28,23,50,255,65,11,255,21,11,49,37,42,43,33
        .byte 8,0,0,69,0,0,0,69,255,0,15,69,255,0,7,8,8,0,0,0,69,255,0,17,123,120,255,0,7,8,8,0,16
        .byte 255,0,5,69,255,0,11,69,0,122,121,0,69,255,0,5,8,8,255,0,30,8,8,255,0,30,8,8,255,0,30,8,8
        .byte 255,0,30,8,8,255,0,30,8,8,255,0,5,163,255,0,18,163,255,0,5,8,8,255,0,4,162,162,162,255,0,16,162,162,162
        .byte 255,0,4,8,8,255,0,5,161,255,0,18,161,255,0,5,8,8,255,0,5,161,255,0,18,161,255,0,5,8,255,0,4
        .byte 163,0,161,0,163,255,0,14,163,0,161,0,163,0,0,0,8,8,0,0,255,162,7,255,0,12,255,162,7,0,0,8,8
        .byte 0,0,0,161,0,0,0,161,255,0,14,161,0,0,0,161,0,0,0,8,8,0,0,0,161,0,0,0,161,255,0,14,161
        .byte 0,0,0,161,0,0,0,8,8,127,0,135,161,127,0,136,161,127,135,136,93,136,135,136,136,135,127,135,136,93,127,161,135
        .byte 0,136,161,135,0,136,8,0,67,0,136,135,136,0,0,0,74,72,73,255,9,8,77,74,75,9,255,0,6,61,64,59,58
        .byte 68,0,0,0,77,0,255,10,16,0,73,0,0,116,0,63,62,57,60,0,73,108,116,107,14,107,108,77,11,108,107,108,107
        .byte 108,107,108,2,117,108,108,116,107,116,108,73,45,46,44,0,27,31,32,255,0,25,35,48,76,36,26,29,30,34,25,255,66,22
        .byte 47,38,40,41,39,24,22,28,23,50,255,65,11,255,21,11,49,37,42,43,33
        .byte 8,255,0,4,69,0,0,0,69,255,0,21,8,8,255,0,10,69,123,120,255,0,10,69,255,0,6,8,8,0,0,0,69
        .byte 255,0,5,69,0,122,121,0,69,255,0,6,69,0,0,0,69,0,0,165,166,8,8,255,0,28,162,162,8,8,255,0,29
        .byte 167,8,8,255,0,29,166,8,8,255,0,29,166,8,8,163,255,0,28,166,8,8,162,162,255,0,27,166,8,8,161,255,0,28
        .byte 166,8,8,161,0,163,255,0,24,47,0,166,8,8,255,162,4,255,0,25,166,8,0,0,0,161,255,0,26,166,8,8,163
        .byte 0,161,0,163,255,0,23,129,130,93,8,255,162,6,255,0,21,132,165,166,131,8,161,0,0,0,161,255,0,22,99,164,164
        .byte 98,8,161,136,127,136,161,93,136,136,127,135,136,135,136,93,136,136,135,127,135,136,93,93,136,127,136,136,0,96,94,94,95
        .byte 0,67,0,136,135,136,0,0,0,255,9,15,0,0,0,135,116,105,61,64,59,58,68,0,73,0,0,0,255,10,16,255,0,4
        .byte 105,0,63,62,57,60,0,73,108,116,77,108,107,108,77,11,108,107,108,107,108,107,108,2,77,108,107,108,12,116,108,73,45
        .byte 46,44,0,27,31,32,255,0,25,35,48,76,36,26,29,30,34,25,255,66,22,47,38,40,41,39,24,22,28,23,50,255,65,11
        .byte 255,21,11,49,37,42,43,33
        .byte 255,0,0,255,0,10,45,46,44,255,0,29,35,48,76,36,255,0,28,38,40,41,39,255,0,27,77,37,42,168,33,135,0,0
        .byte 136,0,0,0,116,255,0,18,114,114,255,0,11,114,114,255,0,17,115,115,255,0,11,115,115,255,0,17,115,115,115,255,0,9
        .byte 115,115,115,255,0,17,115,115,255,0,11,115,115,255,0,17,115,115,255,0,11,115,115,255,0,17,115,115,255,0,11,115,115
        .byte 255,0,17,115,255,0,13,115,255,0,17,115,255,0,13,115,255,0,17,115,255,0,13,115,255,0,73
        .byte 255,0,13,9,0,0,0,90,90,255,0,21,78,78,78,0,0,9,0,0,0,90,90,0,78,78,78,255,0,17,89,1,91
        .byte 0,0,9,0,0,0,90,90,0,89,1,91,255,0,17,89,1,91,0,0,9,0,0,0,90,90,0,89,1,91,255,0,17
        .byte 78,78,78,92,0,9,0,0,0,90,90,0,78,78,78,255,0,18,78,78,78,0,9,0,0,0,90,90,78,78,78,255,0,19
        .byte 255,78,4,92,255,0,4,255,78,4,255,0,20,255,78,4,92,90,90,255,78,4,255,0,22,255,78,9,255,0,24,255,78,7
        .byte 255,0,24,10,9,255,78,5,255,0,25,10,9,0,78,78,78,255,0,26,10,9,0,78,78,78,255,0,22,93,9,0,0
        .byte 10,9,0,78,78,78,255,0,22,93,9,0,0,10,9,0,78,78,78,255,0,19,86,0,0,93,9,0,0,10,9,0,78,78,78
        .byte 0,0,0,9,9,255,0,12,88,0,0,0,87,93,9,0,0,10,9,255,78,5,0,0,9,9,255,0,12,67,27,103,102
        .byte 0,93,9,0,0,109,1,109,1,109,1,109,1,109,9,9,255,0,11,59,58,26,29,30,34,93,9,0,0,1,109,1,109
        .byte 1,109,1,109,1,9,9,255,0,11,57,60,24,22,28,23,93,9,0,0,109,1,109,1,109,1,109,1,109,9,9,255,0,11
        .byte 135,73,2,99,97,98,73,135,0,0,1,109,1,109,1,109,1,109,1,9,9,255,0,10,255,11,4,96,94,95,11,11,2,2
        .byte 109,1,109,1,109,1,109,1,109,255,11,11,0,255,104,29,105,104,0
numsc:
        .byte 23
.out .sprintf("SCREENS SIZE = %d", (* - scdat))

nmedat:
        .byte 8,7,56,120,255
        .byte 5,7,168,120,2,2,144,120,3,3,128,120,6,10,176,120,255
        .byte 5,7,168,120,2,2,144,120,3,3,128,120,6,10,176,120,255
        .byte 5,7,168,120,2,2,144,120,3,3,128,120,6,10,176,120,255
        .byte 5,7,168,120,2,2,144,120,3,3,128,120,6,10,176,120,255
        .byte 5,7,168,120,2,2,144,120,3,3,128,120,6,10,176,120,255
        .byte 5,7,168,120,2,2,144,120,3,3,128,120,6,10,176,120,255
        .byte 5,7,168,120,2,2,144,120,3,3,128,120,6,10,176,120,255
        .byte 5,7,168,120,2,2,144,120,3,3,128,120,6,10,176,120,255
        .byte 5,7,168,120,2,2,144,120,3,3,128,120,6,10,176,120,255
        .byte 5,7,168,120,2,2,144,120,3,3,128,120,6,10,176,120,255
        .byte 5,7,168,120,2,2,144,120,3,3,128,120,6,10,176,120,255
        .byte 5,7,168,120,2,2,144,120,3,3,128,120,6,10,176,120,255
        .byte 5,7,168,120,2,2,144,120,3,3,128,120,6,10,176,120,255
        .byte 5,7,168,120,2,2,144,120,3,3,128,120,6,10,176,120,255
        .byte 5,7,168,120,2,2,144,120,3,3,128,120,6,10,176,120,255
        .byte 5,7,168,120,2,2,144,120,3,3,128,120,6,10,176,120,255
        .byte 5,7,168,120,2,2,144,120,3,3,128,120,6,10,176,120,255
        .byte 5,7,168,120,2,2,144,120,3,3,128,120,6,10,176,120,255
        .byte 5,7,168,120,2,2,144,120,3,3,128,120,6,10,176,120,255
        .byte 5,7,168,120,2,2,144,120,3,3,128,120,6,10,176,120,255
        .byte 7,14,88,120,255
        .byte 8,15,152,56,8,12,120,88,8,13,120,160,8,0,40,136,8,16,8,96,8,17,152,200,255
.out .sprintf("POSITIONS SIZE = %d", (* - nmedat))

NUMOBJ = 1
objdta:
        .byte 83,73,84,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,254,56,120,254,56,120
.out .sprintf("OBJECTS SIZE = %d", (* - objdta))

font:
        .byte 0,0,0,0,0,0,0,0
        .byte 0,56,124,124,56,0,56,0
        .byte 0,0,0,204,170,174,168,104
        .byte 0,0,0,204,170,234,138,134
        .byte 0,0,0,107,136,233,41,201
        .byte 0,0,0,172,42,42,42,38
        .byte 0,0,0,192,160,160,160,160
        .byte 15,63,127,127,255,243,239,252
        .byte 240,212,170,214,255,231,251,63
        .byte 139,103,181,245,103,11,188,159
        .byte 0,240,121,61,30,15,6,0
        .byte 0,251,230,248,252,62,252,0
        .byte 209,230,237,175,230,208,61,249
        .byte 0,0,0,62,124,0,0,0
        .byte 0,0,0,192,0,0,0,0
        .byte 0,48,31,111,28,63,63,127
        .byte 0,124,238,198,198,198,238,124
        .byte 0,24,56,56,24,24,62,124
        .byte 0,124,206,156,56,112,230,254
        .byte 0,126,204,152,60,14,206,124
        .byte 0,96,200,216,126,24,24,16
        .byte 0,252,102,96,124,14,206,124
        .byte 0,60,96,220,246,194,230,124
        .byte 0,126,230,14,28,56,48,48
        .byte 0,124,238,198,124,198,238,124
        .byte 0,124,206,134,222,118,12,120
        .byte 2,4,2,0,31,29,13,7
        .byte 64,128,64,0,246,116,96,192
        .byte 0,0,128,224,0,44,3,140
        .byte 127,95,95,47,223,111,47,71
        .byte 248,244,244,244,232,240,224,128
        .byte 66,29,62,79,79,127,62,28
        .byte 8,28,127,62,28,34,0,0
        .byte 0,60,110,70,219,254,230,66
        .byte 0,254,199,78,248,70,102,188
        .byte 0,56,108,198,192,194,238,124
        .byte 0,220,230,198,198,198,204,248
        .byte 0,220,230,96,124,96,230,220
        .byte 0,238,114,96,124,96,224,192
        .byte 0,28,112,224,207,230,60,0
        .byte 0,204,198,198,254,198,198,102
        .byte 0,126,152,48,48,48,26,252
        .byte 0,62,12,12,230,102,198,124
        .byte 0,192,102,108,120,120,236,198
        .byte 0,192,224,96,96,96,252,194
        .byte 0,102,254,214,214,198,230,102
        .byte 0,204,230,230,214,206,206,102
        .byte 0,56,108,198,198,198,238,124
        .byte 0,220,230,198,206,248,192,192
        .byte 0,56,108,198,214,204,236,118
        .byte 0,56,76,220,248,108,102,0
        .byte 0,56,204,240,28,206,252,120
        .byte 0,254,186,24,24,24,56,48
        .byte 0,230,102,198,198,198,198,124
        .byte 0,206,204,198,198,198,108,56
        .byte 0,27,22,12,12,12,6,11
        .byte 0,238,198,108,56,108,198,238
        .byte 0,128,198,108,48,48,96,192
        .byte 0,236,52,152,152,216,48,232
        .byte 24,36,50,46,46,46,46,94
        .byte 0,0,192,192,0,192,192,0
        .byte 46,29,29,46,46,23,19,15
        .byte 129,56,124,158,158,254,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 24,44,56,52,169,52,169,52
        .byte 0,0,120,140,204,204,118,0
        .byte 64,192,220,226,198,198,252,0
        .byte 0,0,124,230,192,198,124,0
        .byte 0,0,126,203,78,92,248,0
        .byte 0,0,124,198,252,192,124,0
        .byte 0,60,102,96,240,96,96,192
        .byte 0,0,120,198,206,118,6,60
        .byte 0,64,220,226,198,198,196,0
        .byte 0,0,52,61,44,28,20,0
        .byte 98,29,62,127,0,127,62,28
        .byte 0,64,200,208,224,216,204,0
        .byte 0,0,64,96,96,96,60,0
        .byte 0,0,168,252,210,214,212,0
        .byte 0,0,204,238,223,202,68,0
        .byte 0,0,124,142,198,226,124,0
        .byte 0,0,188,198,194,230,220,64
        .byte 0,0,116,204,140,220,108,6
        .byte 0,0,92,231,230,216,68,0
        .byte 0,0,60,114,56,158,124,0
        .byte 16,48,124,48,48,52,24,0
        .byte 0,0,66,198,198,238,122,0
        .byte 0,0,134,198,204,108,48,0
        .byte 0,204,206,198,214,214,254,108
        .byte 0,0,140,88,48,104,196,0
        .byte 0,0,68,198,238,122,6,124
        .byte 0,0,122,142,48,198,190,0
        .byte 28,63,143,243,255,249,227,207
        .byte 130,56,124,242,242,254,126,60
        .byte 28,63,143,243,255,159,7,225
        .byte 28,58,62,125,125,127,127,62
        .byte 134,56,252,254,0,254,126,60
.out .sprintf("FONT SIZE = %d", (* - font))

jtab:
        .byte 248,250,252,254,254,255,255,255,0,0,0,1,1,1,2,2,4,6,8,8,8,99
keys:   .byte 66,97,104,72,73,16,55,48,49,17,18
.out .sprintf("KEYS SIZE = %d", (* - keys))

data_end:
.out .sprintf("TOTAL DATA SIZE = %d", (* - data_start))
