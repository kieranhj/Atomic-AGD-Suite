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
MAPWID = 7
        .byte 255,255,255,255,255,255,255
mapdat:
        .byte 255,255,24,25,26,27,255,255,255,20,19,18,17,255,255,255,21,16,15,14,255,255,255,22,13,12,11,255,255,255,23,8,9,10,255,255,0,1,7,6,255,255,255,255,2,3,4,255,255,255,255,255,255,5,255,255
        .byte 255,255,255,255,255,255,255
stmap:  .byte 36

evnt00:
        jsr skobj
        sta varobj
        lda #255
        cmp varobj
        bne *+5
        jmp a00107
        lda varobj
        jsr getob
        lda #4
        cmp varobj
        bcc *+5
        jmp a00107
        lda #3
        sta z80_c
        lda numlif
        clc
        adc z80_c
        sta numlif
        lda #22
        sta chary
        lda #7
        sta charx
        lda numlif
        jsr disply
a00107: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a00258
:
        lda #2
        cmp varc
        beq *+5
        jmp a00181
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #1
        sta varo
a00181: lda #0
        cmp varc
        beq *+5
        jmp a00219
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a00219: lda #1
        cmp varc
        beq *+5
        jmp a00258
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a00258: ldy #8
        lda (z80_ix),y
        sta varg
        iny; ldy #9
        lda (z80_ix),y
        sta varh
        lda #0
        sta vark
        lda #99
        cmp varc
        beq *+5
        jmp a00929
        jsr vapour
        lda joyval
        and #8
        beq :+
        jmp a00473
:
        lda #1
        sta vark
        jsr cangu
        beq :+
        jmp a00468
:
        lda #12
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp a00404
        jsr scru
        lda #140
        ldy #8
        sta (z80_ix),y
        rts
        jmp a00468
a00404: lda #0
        jsr animsp
        lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a00446
        lda #2
        asl a
        sta sndtyp
a00446: ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a00468: jmp a00565
a00473: jsr cangd
        beq :+
        jmp a00565
:
        lda #143
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a00534
        jsr scrd
        lda #14
        ldy #8
        sta (z80_ix),y
        rts
        jmp a00565
a00534: lda #0
        jsr animsp
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a00565: lda joyval
        and #1
        beq :+
        jmp a00745
:
        lda #1
        sta vark
        ; lda #1
        ldy #10
        sta (z80_ix),y
        lda #230
        dey; ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp a00653
        jsr scrr
        lda #8
        ldy #9
        sta (z80_ix),y
        rts
        jmp a00745
a00653: jsr cangr
        beq :+
        jmp a00745
:
        lda #2
        ldy #6
        sta (z80_ix),y
        lda #0
        jsr animsp
        lda #1
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a00723
        lda #4
        asl a
        sta sndtyp
a00723: ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a00745: lda joyval
        and #2
        beq :+
        jmp a00924
:
        lda #1
        sta vark
        lda #0
        ldy #10
        sta (z80_ix),y
        lda #8
        dey; ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp a00833
        jsr scrl
        lda #228
        ldy #9
        sta (z80_ix),y
        rts
        jmp a00924
a00833: jsr cangl
        beq :+
        jmp a00924
:
        lda #3
        ldy #6
        sta (z80_ix),y
        lda #0
        jsr animsp
        lda #1
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a00902
        lda #4
        asl a
        sta sndtyp
a00902: ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a00924: jmp a01344
a00929: lda joyval
        and #1
        beq :+
        jmp a01111
:
        lda #1
        sta vark
        lda #230
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp a01002
        jsr scrr
        lda #8
        ldy #9
        sta (z80_ix),y
        rts
        jmp a01111
a01002: jsr cangr
        beq :+
        jmp a01111
:
        lda #0
        ldy #6
        sta (z80_ix),y
        ; lda #0
        cmp vara
        beq *+5
        jmp a01055
        ; lda #0
        jsr animsp
a01055: lda #1
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a01089
        lda #3
        asl a
        sta sndtyp
a01089: ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a01111: lda joyval
        and #2
        beq :+
        jmp a01293
:
        lda #1
        sta vark
        lda #8
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp a01184
        jsr scrl
        lda #228
        ldy #9
        sta (z80_ix),y
        rts
        jmp a01293
a01184: jsr cangl
        beq :+
        jmp a01293
:
        lda #1
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp a01237
        ; lda #0
        jsr animsp
a01237: lda #1
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a01271
        lda #3
        asl a
        sta sndtyp
a01271: ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a01293: lda joyval
        and #8
        beq :+
        jmp a01344
:
        jsr jump
        lda #2
        ldy #7
        sta (z80_ix),y
        ; lda #2
        sta vark
        lda #1
        sta varq
a01344: lda joyval
        and #16
        beq :+
        jmp a01420
:
        lda #0
        cmp varl
        beq *+5
        jmp a01420
        lda #7
        sta z80_c
        lda #6
        sta z80_b
        jsr spawn
        lda #30
        asl a
        sta sndtyp
        lda #50
        sta varl
a01420: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01456
:
        lda #1
        sta deadf
a01456: lda #0
        cmp vark
        beq *+5
        jmp a01486
        lda #1
        ldy #7
        sta (z80_ix),y
a01486: lda #0
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a01521
        lda #1
        ldy #10
        sta (z80_ix),y
a01521: lda #1
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a01557
        lda #0
        ldy #10
        sta (z80_ix),y
a01557: lda #90
        cmp varc
        bcs *+5
        jmp a01745
        jsr cangu
        beq :+
        jmp a01630
:
        lda #12
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp a01630
        jsr scru
        lda #140
        ldy #8
        sta (z80_ix),y
        rts
a01630: jsr cangd
        beq :+
        jmp a01745
:
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01676
:
        jmp a01745
a01676: lda #2
        ldy #7
        sta (z80_ix),y
        jsr ifall
        lda #0
        sta varq
        lda #143
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a01745
        jsr scrd
        lda #14
        ldy #8
        sta (z80_ix),y
        rts
a01745: lda #7
        sta z80_b
        jsr sktyp
        bcs :+
        jmp a01908
:
        lda #1
        cmp vark
        bcs *+5
        jmp a01865
        jsr gravst
        lda #0
        cmp vark
        beq *+5
        jmp a01818
        lda #1
        ldy #7
        sta (z80_ix),y
a01818: lda #1
        cmp vark
        beq *+5
        jmp a01861
        lda #0
        cmp vara
        beq *+5
        jmp a01861
        ; lda #0
        jsr animsp
a01861: jmp a01878
a01865: jsr ifall
        lda #0
        sta varq
a01878: lda joyval
        and #8
        beq :+
        jmp a01908
:
        lda #25
        asl a
        sta sndtyp
a01908: jsr lcol
        bcs :+
        jmp a01929
:
        lda #1
        sta deadf
a01929: lda #1
        cmp varq
        beq *+5
        jmp a01997
        lda #0
        cmp varo
        beq *+5
        jmp a01997
        lda #2
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a01997
        lda #20
        asl a
        sta sndtyp
a01997: lda #23
        cmp scno
        beq *+5
        jmp a02028
        lda #255
        ldy #5
        sta (z80_ix),y
a02028: jmp grav
.out .sprintf("EVENT 0 SIZE = %d", (* - evnt00))

evnt01:
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp b00083
:
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00070
        lda #1
        ; ldy #10
        sta (z80_ix),y
        jmp b00083
b00070: lda #0
        ldy #10
        sta (z80_ix),y
b00083: lda #6
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00302
:
        lda #19
        ldy #6
        cmp (z80_ix),y
        bcc *+5
        jmp b00257
        lda #50
        asl a
        sta sndtyp
        lda #255
        dey; ldy #5
        sta (z80_ix),y
        lda #<100
        sta z80_l
        lda #>100
        sta z80_h
        jsr addsc
        lda #22
        sta chary
        lda #25
        sta charx
        jsr dscor
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #4
        sta z80_c
        ; lda #4
        sta z80_b
        jsr spawn
        jmp b00302
b00257: lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #100
        asl a
        sta sndtyp
b00302: lda #19
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp b00355
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00355
        ; lda #0
        jsr animbk
b00355: lda #25
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp b00413
        lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00413
        lda #29
        ldy #6
        sta (z80_ix),y
b00413: lda #29
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp b00471
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00471
        lda #25
        ldy #6
        sta (z80_ix),y
b00471: lda #26
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp b00529
        lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00529
        lda #30
        ldy #6
        sta (z80_ix),y
b00529: lda #30
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp b00587
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00587
        lda #26
        ldy #6
        sta (z80_ix),y
b00587: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00767
        jsr cangl
        beq :+
        jmp b00748
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
        jmp b00708
:
        lda #1
        ldy #10
        sta (z80_ix),y
b00708: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp b00762
b00748: lda #1
        ldy #10
        sta (z80_ix),y
b00762: jmp b00919
b00767: jsr cangr
        beq :+
        jmp b00905
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
        jmp b00865
:
        lda #0
        ldy #10
        sta (z80_ix),y
b00865: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp b00919
b00905: lda #0
        ldy #10
        sta (z80_ix),y
b00919: lda #0
        cmp vart
        beq *+5
        jmp b00945
        ; lda #0
        jsr animsp
b00945: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00975
:
        lda #1
        sta deadf
b00975: lda varg
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp b01223
        lda #29
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp b01110
        lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b01110
        dey; ldy #9
        lda (z80_ix),y
        cmp varh
        bcc *+5
        jmp b01110
        lda #13
        cmp varf
        beq *+5
        jmp b01110
        lda #60
        asl a
        sta sndtyp
        iny; ldy #10
        lda (z80_ix),y
        jsr shoot
b01110: lda #25
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp b01223
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b01223
        lda varh
        dey; ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp b01223
        lda #13
        cmp varf
        beq *+5
        jmp b01223
        lda #60
        asl a
        sta sndtyp
        iny; ldy #10
        lda (z80_ix),y
        jsr shoot
b01223: rts
.out .sprintf("EVENT 1 SIZE = %d", (* - evnt01))

evnt02:
        lda #0
        cmp vard
        beq *+5
        jmp c00190
        ; lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c00063
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
c00063: ldy #12
        lda (z80_ix),y
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp c00105
        lda #1
        ldy #10
        sta (z80_ix),y
c00105: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c00149
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
c00149: ldy #11
        lda (z80_ix),y
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp c00190
        lda #0
        ldy #10
        sta (z80_ix),y
c00190: lda #6
        cmp vard
        beq *+5
        jmp c00342
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c00288
        jsr cangu
        beq :+
        jmp c00270
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp c00283
c00270: lda #1
        ldy #10
        sta (z80_ix),y
c00283: jmp c00342
c00288: jsr cangd
        beq :+
        jmp c00328
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp c00342
c00328: lda #0
        ldy #10
        sta (z80_ix),y
c00342: lda #0
        cmp vart
        beq *+5
        jmp c00367
        ; lda #0
        jsr animsp
c00367: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00397
:
        lda #1
        sta deadf
c00397: lda #6
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00464
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #100
        asl a
        sta sndtyp
c00464: rts
.out .sprintf("EVENT 2 SIZE = %d", (* - evnt02))

evnt03:
        lda #6
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00417
:
        lda #27
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00198
        lda #50
        asl a
        sta sndtyp
        lda #255
        dey; ldy #5
        sta (z80_ix),y
        lda #<250
        sta z80_l
        lda #>250
        sta z80_h
        jsr addsc
        lda #<250
        sta z80_l
        lda #>250
        sta z80_h
        jsr addsc
        lda #22
        sta chary
        lda #25
        sta charx
        jsr dscor
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #4
        sta z80_c
        ; lda #4
        sta z80_b
        jsr spawn
        jmp d00417
d00198: lda #31
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00372
        lda #50
        asl a
        sta sndtyp
        lda #255
        dey; ldy #5
        sta (z80_ix),y
        lda #<250
        sta z80_l
        lda #>250
        sta z80_h
        jsr addsc
        lda #<250
        sta z80_l
        lda #>250
        sta z80_h
        jsr addsc
        lda #22
        sta chary
        lda #25
        sta charx
        jsr dscor
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #4
        sta z80_c
        ; lda #4
        sta z80_b
        jsr spawn
        jmp d00417
d00372: lda #100
        asl a
        sta sndtyp
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
d00417: lda #17
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00475
        lda #0
        cmp vari
        beq *+5
        jmp d00475
        jsr ifall
        ldy #12
        lda (z80_ix),y
        sta vari
d00475: lda #24
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00532
        lda #0
        cmp varb
        beq *+5
        jmp d00532
        jsr ifall
        ldy #12
        lda (z80_ix),y
        sta varb
d00532: lda #28
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00590
        lda #0
        cmp varb
        beq *+5
        jmp d00590
        jsr ifall
        ldy #12
        lda (z80_ix),y
        sta varb
d00590: lda #27
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00647
        lda #0
        cmp varn
        beq *+5
        jmp d00647
        jsr ifall
        ldy #12
        lda (z80_ix),y
        sta varn
d00647: lda #31
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00705
        lda #0
        cmp varn
        beq *+5
        jmp d00705
        jsr ifall
        ldy #12
        lda (z80_ix),y
        sta varn
d00705: lda #24
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00763
        lda #1
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp d00763
        lda #28
        ldy #6
        sta (z80_ix),y
d00763: lda #28
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00821
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp d00821
        lda #24
        ldy #6
        sta (z80_ix),y
d00821: lda #27
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00879
        lda #1
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp d00879
        lda #31
        ldy #6
        sta (z80_ix),y
d00879: lda #31
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00937
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp d00937
        lda #27
        ldy #6
        sta (z80_ix),y
d00937: lda #0
        cmp vara
        beq *+5
        jmp d01289
        lda #13
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00984
        lda #3
        sta loopa
d00984: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp d01065
        jsr cangl
        beq :+
        jmp d01046
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp d01060
d01046: lda #1
        ldy #11
        sta (z80_ix),y
d01060: jmp d01118
d01065: jsr cangr
        beq :+
        jmp d01105
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp d01118
d01105: lda #0
        ldy #11
        sta (z80_ix),y
d01118: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp d01199
        jsr cangu
        beq :+
        jmp d01181
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp d01194
d01181: lda #1
        ldy #10
        sta (z80_ix),y
d01194: jmp d01253
d01199: jsr cangd
        beq :+
        jmp d01239
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp d01253
d01239: lda #0
        ldy #10
        sta (z80_ix),y
d01253: lda #13
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d01289
        dec loopa
        beq :+
        jmp d00984
:
d01289: lda #0
        cmp vart
        beq *+5
        jmp d01314
        ; lda #0
        jsr animsp
d01314: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d01344
:
        lda #1
        sta deadf
d01344: lda #13
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d01388
        lda #0
        cmp vara
        beq *+5
        jmp d01388
        jmp d01388
d01388: jmp grav
.out .sprintf("EVENT 3 SIZE = %d", (* - evnt03))

evnt04:
        lda #12
        ldy #6
        cmp (z80_ix),y
        bcs *+5
        jmp e00024
e00024: lda #5
        ldy #6
        cmp (z80_ix),y
        bcs *+5
        jmp e00046
e00046: lda #20
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00120
        lda #0
        cmp vara
        beq *+5
        jmp e00090
        jmp e00090
e00090: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00120
:
        lda #1
        sta deadf
e00120: lda #27
        cmp scno
        beq *+5
        jmp e00615
        lda #24
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00246
        lda #0
        cmp vard
        beq *+5
        jmp e00246
        lda #19
        cmp varf
        bcs *+5
        jmp e00246
        lda #1
        cmp vart
        beq *+5
        jmp e00246
        lda #4
        sta z80_c
        ; lda #4
        sta z80_b
        jsr spawn
        lda varf
        clc
        adc #1
        sta varf
e00246: lda #0
        cmp vard
        bcc *+5
        jmp e00390
        lda #24
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00386
        tya; lda #6
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00356
:
        lda vard
        sec
        sbc #1
        sta vard
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
e00356: lda #0
        cmp vara
        beq *+5
        jmp e00372
e00372: lda #1
        ldy #7
        sta (z80_ix),y
e00386: jmp e00427
e00390: lda #24
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00427
        lda #0
        iny; ldy #7
        sta (z80_ix),y
e00427: lda #4
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00555
        lda #0
        cmp vart
        beq *+5
        jmp e00555
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #1
        cmp vart
        beq *+5
        jmp e00533
        dey; ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp e00555
e00533: ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
e00555: lda #20
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00615
        lda #40
        cmp vars
        beq *+5
        jmp e00615
        lda #5
        sta z80_c
        lda #15
        sta z80_b
        jsr spawn
e00615: lda #5
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00704
        lda #2
        iny; ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp e00674
        lda #255
        ldy #5
        sta (z80_ix),y
e00674: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00704
:
        lda #1
        sta deadf
e00704: lda #4
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00848
        lda #0
        cmp vart
        beq *+5
        jmp e00783
        lda #1
        iny; ldy #7
        cmp (z80_ix),y
        bcs *+5
        jmp e00783
        lda #0
        jsr animsp
        lda #5
        sta vare
e00783: lda #2
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp e00848
        lda #1
        cmp vare
        beq *+5
        jmp e00848
        lda #126
        asl a
        sta sndtyp
        lda #255
        ldy #5
        sta (z80_ix),y
e00848: lda #10
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00920
        lda #0
        cmp varf
        beq *+5
        jmp e00920
        lda #200
        asl a
        sta sndtyp
        lda #5
        sta z80_c
        lda #12
        sta z80_b
        jsr spawn
e00920: lda #9
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00993
        lda #0
        cmp vars
        beq *+5
        jmp e00993
        lda #200
        asl a
        sta sndtyp
        lda #5
        sta z80_c
        lda #32
        sta z80_b
        jsr spawn
e00993: lda #8
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e01065
        lda #0
        cmp vars
        beq *+5
        jmp e01065
        lda #200
        asl a
        sta sndtyp
        lda #5
        sta z80_c
        lda #21
        sta z80_b
        jsr spawn
e01065: lda #16
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e01345
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e01168
        jsr cangl
        beq :+
        jmp e01150
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp e01164
e01150: lda #1
        ldy #10
        sta (z80_ix),y
e01164: jmp e01222
e01168: jsr cangr
        beq :+
        jmp e01209
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp e01222
e01209: lda #0
        ldy #10
        sta (z80_ix),y
e01222: lda #50
        cmp varm
        bcs *+5
        jmp e01294
        lda #14
        cmp varf
        beq *+5
        jmp e01290
        lda #30
        asl a
        sta sndtyp
        lda #5
        sta z80_c
        lda #15
        sta z80_b
        jsr spawn
e01290: jmp e01345
e01294: lda #0
        cmp vars
        beq *+5
        jmp e01345
        lda #40
        asl a
        sta sndtyp
        lda #5
        sta z80_c
        lda #22
        sta z80_b
        jsr spawn
e01345: lda #22
        ldy #6
        cmp (z80_ix),y
        bcs *+5
        jmp e01393
        lda #0
        cmp vart
        beq *+5
        jmp e01393
        ; lda #0
        jsr animsp
e01393: rts
.out .sprintf("EVENT 4 SIZE = %d", (* - evnt04))

evnt05:
        lda #22
        ldy #6
        cmp (z80_ix),y
        bcc *+5
        jmp f00037
        lda #0
        ldy #10
        sta (z80_ix),y
f00037: lda #21
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f00078
        lda #3
        ldy #10
        sta (z80_ix),y
        jmp f00114
f00078: lda #22
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f00114
        lda #3
        ldy #10
        sta (z80_ix),y
f00114: lda #15
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f00149
        lda #2
        ldy #10
        sta (z80_ix),y
f00149: lda #14
        ldy #6
        cmp (z80_ix),y
        bcs *+5
        jmp f00185
        lda #1
        ldy #10
        sta (z80_ix),y
f00185: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00305
        jsr cangl
        beq :+
        jmp f00270
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
        jmp f00305
f00270: lda #255
        ldy #5
        sta (z80_ix),y
        lda #4
        sta z80_c
        ; lda #4
        sta z80_b
        jsr spawn
f00305: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00623
        jsr cangr
        beq :+
        jmp f00390
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
        jmp f00623
f00390: lda #32
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f00623
        lda #19
        cmp scno
        bcc *+5
        jmp f00588
        lda #255
        dey; ldy #5
        sta (z80_ix),y
        lda #4
        sta z80_c
        ; lda #4
        sta z80_b
        jsr spawn
        lda #80
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #8
        ldy #9
        sta (z80_ix),y
        lda varrnd
        dey; ldy #8
        sta (z80_ix),y
        lda #40
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #5
        sta z80_c
        lda #32
        sta z80_b
        jsr spawn
        jmp f00623
f00588: lda #255
        ldy #5
        sta (z80_ix),y
        lda #4
        sta z80_c
        ; lda #4
        sta z80_b
        jsr spawn
f00623: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00720
        jsr cangu
        beq :+
        jmp f00686
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp f00720
f00686: lda #8
        asl a
        sta sndtyp
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #4
        jsr explod
f00720: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00881
        jsr cangd
        beq :+
        jmp f00783
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp f00881
f00783: lda #21
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f00846
        lda #255
        dey; ldy #5
        sta (z80_ix),y
        lda #4
        sta z80_c
        ; lda #4
        sta z80_b
        jsr spawn
        jmp f00881
f00846: lda #12
        asl a
        sta sndtyp
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #3
        jsr explod
f00881: lda #0
        cmp vart
        beq *+5
        jmp f00906
        ; lda #0
        jsr animsp
f00906: lda #22
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f00929
f00929: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00959
:
        lda #1
        sta deadf
f00959: rts
.out .sprintf("EVENT 5 SIZE = %d", (* - evnt05))

evnt06:
        lda #27
        ldy #6
        cmp (z80_ix),y
        bcs *+5
        jmp g00042
        lda #1
        ldy #10
        sta (z80_ix),y
        jmp g00056
g00042: lda #0
        ldy #10
        sta (z80_ix),y
g00056: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp g00154
        jsr cangr
        beq :+
        jmp g00141
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
        jmp g00154
g00141: lda #255
        ldy #5
        sta (z80_ix),y
g00154: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp g00253
        jsr cangl
        beq :+
        jmp g00239
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
        jmp g00253
g00239: lda #255
        ldy #5
        sta (z80_ix),y
g00253: lda #0
        cmp varj
        beq *+5
        jmp g00284
        lda #255
        ldy #5
        sta (z80_ix),y
g00284: rts
.out .sprintf("EVENT 6 SIZE = %d", (* - evnt06))

evnt07:
        lda #6
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h00128
        lda #1
        cmp varl
        beq *+5
        jmp h00128
        lda #255
        dey; ldy #5
        sta (z80_ix),y
        lda #4
        sta z80_c
        tya; lda #5
        sta z80_b
        jsr spawn
        lda #6
        sta z80_c
        lda #23
        sta z80_b
        jsr spawn
        lda #6
        sta z80_c
        lda #33
        sta z80_b
        jsr spawn
        lda #12
        sta varj
h00128: jsr ifall
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp h00396
:
        ldy #9
        lda (z80_ix),y
        sta varr
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        ; ldy #9
        lda (z80_ix),y
        sta varp
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda varr
        cmp varp
        bcc *+5
        jmp h00319
        lda varr
        sta z80_c
        lda varp
        sec
        sbc z80_c
        sta varp
        lda #13
        cmp varp
        bcc *+5
        jmp h00314
        jsr cangl
        beq :+
        jmp h00314
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
h00314: jmp h00396
h00319: lda varp
        sta z80_c
        lda varr
        sec
        sbc z80_c
        sta varr
        lda #13
        cmp varr
        bcc *+5
        jmp h00396
        jsr cangr
        beq :+
        jmp h00396
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
h00396: lda #148
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp h00482
        lda #6
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h00468
        lda #255
        dey; ldy #5
        sta (z80_ix),y
        lda #1
        sta varm
        jmp h00482
h00468: lda #255
        ldy #5
        sta (z80_ix),y
h00482: lda #6
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h00567
        ; lda #6
        cmp varl
        bcs *+5
        jmp h00538
        lda #20
        asl a
        sta sndtyp
        jmp h00567
h00538: lda #4
        cmp varl
        bcs *+5
        jmp h00567
        lda #25
        asl a
        sta sndtyp
h00567: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp h00672
:
        lda #0
        cmp varc
        beq *+5
        jmp h00633
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
h00633: lda #1
        cmp varc
        beq *+5
        jmp h00672
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
h00672: jmp grav
.out .sprintf("EVENT 7 SIZE = %d", (* - evnt07))

evnt08:
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp i00167
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        jsr cangu
        beq :+
        jmp i00113
:
        lda #15
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp i00162
i00113: lda #16
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
i00162: jmp i00225
i00167: jsr cangd
        beq :+
        jmp i00211
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
        jmp i00225
i00211: lda #0
        ldy #10
        sta (z80_ix),y
i00225: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i00454
:
        ldy #8
        lda (z80_ix),y
        sta varp
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        ; ldy #8
        lda (z80_ix),y
        cmp varp
        bcc *+5
        jmp i00454
        ; ldy #8
        lda (z80_ix),y
        sta z80_c
        lda varp
        sec
        sbc z80_c
        sta varp
        tya; lda #8
        cmp varp
        bcc *+5
        jmp i00436
        ; ldy #8
        lda (z80_ix),y
        sta z80_c
        lda varp
        clc
        adc z80_c
        sta varp
        lda varp
        ; ldy #8
        sta (z80_ix),y
        lda #16
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        jsr gravst
i00436: lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
i00454: rts
.out .sprintf("EVENT 8 SIZE = %d", (* - evnt08))

evnt09:
        lda #0
        jsr gotob
        cmp #255
        beq :+
        jmp j00077
:
        lda #6
        cmp scno
        bcs *+5
        jmp j00077
        lda #20
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp j00077
        lda #255
        dey; ldy #5
        sta (z80_ix),y
j00077: lda #1
        jsr gotob
        cmp #255
        beq :+
        jmp j00153
:
        lda #15
        cmp scno
        bcs *+5
        jmp j00153
        lda #20
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp j00153
        lda #255
        dey; ldy #5
        sta (z80_ix),y
j00153: lda #2
        jsr gotob
        cmp #255
        beq :+
        jmp j00229
:
        lda #17
        cmp scno
        beq *+5
        jmp j00229
        lda #20
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp j00229
        lda #255
        dey; ldy #5
        sta (z80_ix),y
j00229: lda #3
        jsr gotob
        cmp #255
        beq :+
        jmp j00304
:
        lda #19
        cmp scno
        beq *+5
        jmp j00304
        lda #20
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp j00304
        lda #255
        dey; ldy #5
        sta (z80_ix),y
j00304: lda #4
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp j00445
        lda #27
        cmp scno
        beq *+5
        jmp j00445
        lda #60
        ldy #8
        sta (z80_ix),y
        lda varrnd
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #180
        iny; ldy #9
        sta (z80_ix),y
        lda varrnd
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
j00445: lda #3
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00540
        lda #17
        iny; ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp j00540
        lda #16
        cmp scno
        beq *+5
        jmp j00526
        lda #60
        ldy #12
        sta (z80_ix),y
        jmp j00540
j00526: lda #80
        ldy #12
        sta (z80_ix),y
j00540: lda #27
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp j00576
        lda #120
        ldy #12
        sta (z80_ix),y
j00576: lda #31
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp j00613
        lda #120
        ldy #12
        sta (z80_ix),y
j00613: lda #24
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp j00649
        lda #16
        ldy #12
        sta (z80_ix),y
j00649: lda #28
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp j00685
        lda #16
        ldy #12
        sta (z80_ix),y
j00685: lda #2
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00936
        lda #3
        cmp scno
        beq *+5
        jmp j00752
        lda #42
        ldy #11
        sta (z80_ix),y
        lda #112
        iny; ldy #12
        sta (z80_ix),y
j00752: lda #5
        cmp scno
        beq *+5
        jmp j00798
        lda #100
        ldy #11
        sta (z80_ix),y
        lda #138
        iny; ldy #12
        sta (z80_ix),y
j00798: lda #15
        cmp scno
        beq *+5
        jmp j00844
        lda #70
        ldy #11
        sta (z80_ix),y
        lda #120
        iny; ldy #12
        sta (z80_ix),y
j00844: lda #16
        cmp scno
        beq *+5
        jmp j00890
        lda #50
        ldy #11
        sta (z80_ix),y
        lda #110
        iny; ldy #12
        sta (z80_ix),y
j00890: lda #19
        cmp scno
        beq *+5
        jmp j00936
        lda #90
        ldy #11
        sta (z80_ix),y
        lda #130
        iny; ldy #12
        sta (z80_ix),y
j00936: rts
.out .sprintf("EVENT 9 SIZE = %d", (* - evnt09))

evnt10:
        lda #1
        sta varc
        lda #0
        jsr gotob
        cmp #255
        beq :+
        jmp k00091
:
        lda #5
        cmp scno
        beq *+5
        jmp k00091
        lda #8
        sta charx
        lda #3
        sta chary
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #104
        jsr pattr
k00091: lda #1
        jsr gotob
        cmp #255
        beq :+
        jmp k00254
:
        lda #8
        cmp scno
        beq *+5
        jmp k00173
        lda #3
        sta charx
        lda #6
        sta chary
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #104
        jsr pattr
k00173: lda #9
        cmp scno
        beq *+5
        jmp k00254
        lda #24
        sta charx
        lda #5
        sta chary
        lda #2
        sta loopa
k00214: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #25
        jsr pattr
        dec loopa
        beq :+
        jmp k00214
:
k00254: lda #2
        jsr gotob
        cmp #255
        beq :+
        jmp k00336
:
        lda #19
        cmp scno
        beq *+5
        jmp k00336
        lda #7
        sta charx
        lda #5
        sta chary
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #104
        jsr pattr
k00336: lda #3
        jsr gotob
        cmp #255
        beq :+
        jmp k00418
:
        lda #17
        cmp scno
        beq *+5
        jmp k00418
        lda #3
        sta charx
        lda #18
        sta chary
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #104
        jsr pattr
k00418: lda #4
        jsr gotob
        cmp #255
        beq :+
        jmp k00552
:
        lda #99
        sta varc
        lda #20
        cmp scno
        beq *+5
        jmp k00552
        lda #29
        sta charx
        lda #9
        sta chary
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #20
        jsr pattr
        lda #29
        sta charx
        lda #10
        sta chary
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #27
        jsr pattr
k00552: lda #0
        sta varo
        lda #26
        cmp scno
        bcs *+5
        jmp k00585
        lda #6
        sta vard
k00585: lda #80
        sta varm
        lda #1
        cmp vara
        beq *+5
        jmp k00622
        lda #0
        sta vara
        jmp k00638
k00622: lda vara
        clc
        adc #1
        sta vara
k00638: lda #3
        cmp vart
        beq *+5
        jmp k00667
        lda #0
        sta vart
        jmp k00682
k00667: lda vart
        clc
        adc #1
        sta vart
k00682: lda #25
        cmp scno
        bcs *+5
        jmp k00744
        lda #35
        cmp varf
        beq *+5
        jmp k00729
        lda #0
        sta varf
        jmp k00744
k00729: lda varf
        clc
        adc #1
        sta varf
k00744: lda #80
        cmp vars
        beq *+5
        jmp k00774
        lda #0
        sta vars
        jmp k00789
k00774: lda vars
        clc
        adc #1
        sta vars
k00789: lda #4
        cmp scno
        beq *+5
        jmp k00814
        lda #2
        sta varc
k00814: lda #6
        cmp scno
        beq *+5
        jmp k00847
        lda #2
        sta varc
        lda #50
        sta varm
k00847: lda #14
        cmp scno
        beq *+5
        jmp k00872
        lda #2
        sta varc
k00872: lda #17
        cmp scno
        beq *+5
        jmp k00897
        lda #2
        sta varc
k00897: lda #10
        cmp scno
        beq *+5
        jmp k00930
        lda #0
        sta varc
        lda #50
        sta varm
k00930: lda #2
        cmp scno
        beq *+5
        jmp k00955
        lda #99
        sta varc
k00955: lda #3
        cmp scno
        beq *+5
        jmp k00988
        lda #99
        sta varc
        lda #0
        sta vard
k00988: lda #5
        cmp scno
        beq *+5
        jmp k01013
        lda #0
        sta vard
k01013: lda #16
        cmp scno
        beq *+5
        jmp k01047
        lda #0
        sta vard
        ; lda #0
        sta varc
k01047: lda #19
        cmp scno
        beq *+5
        jmp k01072
        lda #0
        sta vard
k01072: lda #26
        cmp scno
        beq *+5
        jmp k01097
        lda #0
        sta varf
k01097: lda #13
        cmp scno
        beq *+5
        jmp k01122
        lda #77
        sta varc
k01122: lda #12
        cmp scno
        beq *+5
        jmp k01147
        lda #77
        sta varc
k01147: rts
.out .sprintf("EVENT 10 SIZE = %d", (* - evnt10))

evnt11:
        lda #0
        cmp vare
        bcc *+5
        jmp l00034
        lda vare
        sec
        sbc #1
        sta vare
l00034: lda #0
        cmp varl
        bcc *+5
        jmp l00066
        lda varl
        sec
        sbc #1
        sta varl
l00066: lda #0
        cmp varb
        bcc *+5
        jmp l00098
        lda varb
        sec
        sbc #1
        sta varb
l00098: lda #0
        cmp varn
        bcc *+5
        jmp l00130
        lda varn
        sec
        sbc #1
        sta varn
l00130: lda #0
        cmp vari
        bcc *+5
        jmp l00162
        lda vari
        sec
        sbc #1
        sta vari
l00162: lda #0
        cmp varj
        bcc *+5
        jmp l00194
        lda varj
        sec
        sbc #1
        sta varj
l00194: lda #0
        cmp varq
        bcc *+5
        jmp l00226
        lda varq
        sec
        sbc #1
        sta varq
l00226: lda #27
        cmp scno
        beq *+5
        jmp l00412
        lda #0
        cmp vara
        beq *+5
        jmp l00291
        lda #50
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
l00291: lda #0
        cmp vard
        beq *+5
        jmp l00412
        ; lda #0
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp l00365
        lda #255
        ; ldy #5
        sta (z80_ix),y
        lda #4
        sta z80_c
        lda #34
        sta z80_b
        jsr spawn
l00365: lda #20
        cmp varf
        beq *+5
        jmp l00412
        lda #30
        jsr delay
        lda #23
        sta scno
        jsr nwscr
        lda #1
        sta restfl
l00412: lda #0
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp l00529
        lda #99
        cmp varc
        beq *+5
        jmp l00529
        lda #0
        iny; ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp l00493
        lda #2
        ; ldy #6
        sta (z80_ix),y
        jmp l00529
l00493: lda #1
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp l00529
        lda #3
        ; ldy #6
        sta (z80_ix),y
l00529: lda #23
        cmp scno
        beq *+5
        jmp l00671
        lda #1
        sta chary
        lda #7
        sta charx
        lda #3
        jsr dmsg
        lda #4
        sta chary
        lda #0
        sta charx
        lda #4
        jsr dmsg
        lda #15
        sta chary
        lda #0
        sta charx
        lda #5
        jsr dmsg
        lda #19
        sta chary
        lda #22
        sta charx
        lda #6
        jsr dmsg
        lda joyval
        and #16
        beq :+
        jmp l00671
:
        lda #1
        sta gamwon
l00671: rts
.out .sprintf("EVENT 11 SIZE = %d", (* - evnt11))

evnt12:
        jsr cls
        lda #1
        sta chary
        lda #10
        sta charx
        lda #11
        sta loopa
m00031: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #205
        jsr pattr
        dec loopa
        beq :+
        jmp m00031
:
        lda #1
        sta chary
        lda #9
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #208
        jsr pattr
        lda #1
        sta chary
        lda #21
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #206
        jsr pattr
        lda #1
        sta chary
        lda #22
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #207
        jsr pattr
        lda #2
        sta chary
        lda #9
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #209
        jsr pattr
        lda #2
        sta chary
        lda #22
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #210
        jsr pattr
        lda #3
        sta chary
        lda #9
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #214
        jsr pattr
        lda #3
        sta chary
        lda #10
        sta charx
        lda #11
        sta loopa
m00357: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #211
        jsr pattr
        dec loopa
        beq :+
        jmp m00357
:
        lda #3
        sta chary
        lda #21
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #212
        jsr pattr
        lda #3
        sta chary
        lda #22
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #213
        jsr pattr
        lda #4
        sta chary
        lda #15
        sta charx
        lda #2
        sta loopa
m00509: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #215
        jsr pattr
        dec loopa
        beq :+
        jmp m00509
:
        lda #2
        sta chary
        lda #10
        sta charx
        lda #7
        jsr dmsg
        lda #7
        sta chary
        lda #9
        sta charx
        lda #12
        jsr dmsg
        lda #17
        sta chary
        lda #9
        sta charx
        lda #12
        jsr dmsg
        lda #9
        sta chary
        lda #11
        sta charx
        lda #8
        jsr dmsg
        lda #12
        sta chary
        lda #11
        sta charx
        lda #9
        jsr dmsg
        lda #15
        sta chary
        lda #11
        sta charx
        lda #10
        jsr dmsg
        lda #23
        sta chary
        lda #6
        sta charx
        lda #11
        jsr dmsg
        jsr prskey
        rts
.out .sprintf("EVENT 12 SIZE = %d", (* - evnt12))

evnt13:
        lda #3
        sta numlif
        jsr cls
        lda #0
        sta vara
        lda #22
        sta chary
        lda #1
        sta charx
        ; lda #1
        jsr dmsg
        lda #22
        sta chary
        lda #19
        sta charx
        lda #2
        jsr dmsg
        lda #22
        sta chary
        lda #25
        sta charx
        jsr dscor
        lda #0
        sta vark
        ; lda #0
        sta varl
        lda #1
        sta varm
        lda #0
        sta vart
        ; lda #0
        sta vare
        ; lda #0
        sta vard
        ; lda #0
        sta varf
        ; lda #0
        sta vars
        lda #21
        sta chary
        lda #0
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #15
        jsr pattr
        lda #1
        sta charx
        lda #7
        sta loopa
n00220: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #24
        jsr pattr
        dec loopa
        beq :+
        jmp n00220
:
        lda #8
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #17
        jsr pattr
        lda #18
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #15
        jsr pattr
        lda #19
        sta charx
        lda #13
        sta loopa
n00347: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #24
        jsr pattr
        dec loopa
        beq :+
        jmp n00347
:
        lda #31
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #17
        jsr pattr
        lda #22
        sta chary
        lda #0
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #13
        jsr pattr
        lda #8
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #14
        jsr pattr
        lda #18
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #13
        jsr pattr
        lda #31
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #14
        jsr pattr
        lda #23
        sta chary
        lda #0
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #10
        jsr pattr
        lda #1
        sta charx
        lda #7
        sta loopa
n00630: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #33
        jsr pattr
        dec loopa
        beq :+
        jmp n00630
:
        lda #8
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #12
        jsr pattr
        lda #18
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #10
        jsr pattr
        lda #19
        sta charx
        lda #12
        sta loopa
n00756: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #33
        jsr pattr
        dec loopa
        beq :+
        jmp n00756
:
        lda #31
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #12
        jsr pattr
        rts
.out .sprintf("EVENT 13 SIZE = %d", (* - evnt13))

evnt14:
        lda #0
        sta vara
        lda #22
        sta chary
        lda #7
        sta charx
        lda numlif
        jsr disply
        lda #1
        sta varm
        lda #0
        sta varl
        rts
.out .sprintf("EVENT 14 SIZE = %d", (* - evnt14))

evnt15:
        rts
.out .sprintf("EVENT 15 SIZE = %d", (* - evnt15))

evnt16:
        lda #70
        asl a
        sta sndtyp
        lda numlif
        sec
        sbc #1
        sta numlif
        lda #25
        jsr delay
        lda #1
        sta varm
        rts
.out .sprintf("EVENT 16 SIZE = %d", (* - evnt16))

evnt17:
        lda #10
        sta chary
        lda #11
        sta charx
        lda #0
        jsr dmsg
        lda #70
        jsr delay
        rts
.out .sprintf("EVENT 17 SIZE = %d", (* - evnt17))

evnt18:
        lda #10
        sta chary
        lda #11
        sta charx
        lda #13
        jsr dmsg
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
        .byte "GAMEaOVER",141
        .byte "LIVES",141
        .byte "SCORE",141
        .byte "CONGRATULATIONSb",141
        .byte "WITHaHANSaVONaDECKaDEFEATED",13
        .byte "AGENTa99aRETIRESaTOaTHEaBEACH,",141
        .byte "COLLECTINGaAaLIFETIMEaOFaFREE",13
        .byte "ICEaCREAMaALONGaTHEaWAY.",141
        .byte "PRESSaFIRE",141
        .byte "BLIMPGEDDONb",141
        .byte "1.KEYBOARD",141
        .byte "2.KEMPSTON",141
        .byte "3.SINCLAIR",141
        .byte "a2017aULTRANARWHAL",141
        .byte "dcdcdcdcdcdcdc",141
        .byte "WELLaDONEb",141
nummsg:
        .byte 14
.out .sprintf("MESSAGE SIZE = %d", (* - msgdat))

chgfx:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 95,95,175,175,95,95,175,175,95,95,175,175,95,95,175,175
        .byte 15,95,143,175,79,31,15,15,79,79,47,15,15,15,15,15
        .byte 181,245,250,250,245,165,250,218,245,229,122,218,245,229,250,250
        .byte 245,245,250,250,245,245,250,250,245,245,250,250,245,245,250,250
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
        .byte 76,0,137,5,2,95,5,5,0,10,0,5,0,10,0,5
        .byte 0,0,175,175,95,95,175,175,95,95,175,175,95,95,175,175
        .byte 0,19,5,5,95,78,175,140,95,78,175,140,95,78,175,140
        .byte 0,10,0,5,0,10,0,5,0,10,0,0,8,0,140,39
        .byte 85,85,170,170,85,85,170,170,85,85,0,0,0,0,175,175
        .byte 10,78,5,4,10,78,5,4,10,10,0,38,0,93,175,171
        .byte 0,85,0,170,0,85,0,170,0,85,0,170,0,85,0,170
        .byte 95,93,175,141,95,93,175,141,95,93,175,141,95,93,175,141
        .byte 34,0,85,85,34,255,85,85,0,170,0,85,0,170,0,85
        .byte 255,255,0,0,255,255,255,255,255,255,255,255,255,255,255,255
        .byte 0,0,85,68,255,238,255,204,255,238,255,204,255,238,255,204
        .byte 0,170,0,85,0,170,0,85,136,170,136,0,68,0,51,0
        .byte 85,85,170,170,85,85,170,170,85,85,0,0,0,0,68,255
        .byte 170,238,85,68,170,238,85,68,170,170,0,68,0,136,0,0
        .byte 0,85,0,170,0,85,0,170,0,85,0,170,0,85,0,170
        .byte 255,221,255,221,255,221,255,221,255,221,255,221,255,221,255,221
        .byte 15,15,15,15,95,95,175,175,95,95,175,175,95,95,175,175
        .byte 15,15,15,15,95,95,255,255,255,255,255,255,255,255,175,175
        .byte 255,255,255,255,0,0,85,85,170,170,255,255,0,0,0,0
        .byte 255,255,255,255,0,0,85,85,170,170,255,255,0,0,0,0
        .byte 245,0,234,16,196,49,136,114,0,245,16,234,49,196,114,136
        .byte 95,95,175,175,95,95,239,239,255,255,239,239,255,255,239,239
        .byte 95,95,175,175,78,78,0,0,0,0,0,0,0,0,1,1
        .byte 95,95,255,255,255,255,255,255,255,255,255,255,255,255,255,255
        .byte 255,255,255,255,255,255,255,255,255,255,170,170,0,0,0,0
        .byte 255,255,175,175,95,95,175,175,95,95,175,175,95,95,175,175
        .byte 95,95,175,175,95,95,175,175,95,95,175,175,15,15,15,15
        .byte 0,87,0,39,0,87,136,35,0,87,0,39,0,87,136,35
        .byte 255,223,255,175,255,223,175,191,255,223,255,175,255,223,175,191
        .byte 15,15,105,15,225,135,30,90,105,210,210,165,15,120,15,210
        .byte 30,135,105,150,15,195,120,90,210,15,30,240,240,45,120,15
        .byte 30,135,150,105,15,15,165,225,150,180,60,135,195,240,15,225
        .byte 195,15,225,15,225,135,120,90,60,210,30,165,15,240,15,60
        .byte 30,150,225,240,15,240,120,225,210,195,90,195,240,135,225,15
        .byte 0,112,144,112,208,144,192,48,224,144,64,128,144,64,128,96
        .byte 208,160,240,96,176,144,112,80,224,32,192,128,16,160,48,48
        .byte 0,80,160,112,0,112,224,0,0,240,208,112,16,16,64,144
        .byte 64,0,32,0,64,64,32,128,0,0,32,0,0,64,128,128
        .byte 16,80,0,160,64,64,32,0,16,16,128,0,80,64,0,0
        .byte 0,0,170,170,85,85,170,170,0,0,0,0,0,0,255,255
        .byte 79,79,175,175,79,79,143,47,15,15,15,15,31,15,15,15
        .byte 34,68,34,68,34,68,34,68,51,68,17,136,0,0,17,136
        .byte 68,68,34,34,17,17,136,136,68,68,34,34,17,17,136,136
        .byte 187,187,221,221,238,238,119,119,187,187,221,221,238,238,119,119
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
        .byte 117,17,250,186,245,245,250,250,245,245,250,186,228,245,250,250
        .byte 196,245,250,186,245,245,186,250,245,245,250,250,245,245,250,186
        .byte 129,5,24,16,128,1,8,80,129,0,24,64,1,0,16,0
        .byte 213,245,250,250,245,245,250,250,245,117,186,186,85,85,170,170
        .byte 85,85,170,170,68,0,16,34,68,68,170,34,0,0,0,64
        .byte 160,5,8,80,128,1,10,80,0,5,2,80,0,4,0,64
        .byte 79,79,143,15,79,79,143,15,79,79,143,15,79,79,143,15
        .byte 207,207,223,79,207,207,223,79,207,207,223,79,207,207,223,79
        .byte 31,95,47,175,31,95,47,175,31,95,47,175,31,95,47,175
        .byte 31,255,63,239,31,255,63,239,31,255,63,239,31,255,63,239
        .byte 17,136,51,204,119,238,17,136,170,119,170,85,170,119,238,68
        .byte 95,95,175,175,95,95,175,175,95,95,175,175,95,95,175,175
        .byte 95,95,175,175,95,95,175,175,15,15,15,15,15,15,15,15
        .byte 204,0,170,170,85,85,34,255,85,255,51,255,51,255,51,255
        .byte 0,0,170,170,85,68,255,238,255,204,255,238,255,204,255,238
        .byte 0,0,85,85,34,170,119,255,51,255,119,255,51,255,119,255
        .byte 0,51,85,85,170,170,255,68,255,170,255,204,255,204,255,204
        .byte 51,255,51,255,51,255,85,255,34,255,85,85,170,170,204,0
        .byte 255,238,255,204,255,238,255,204,255,238,85,68,170,170,0,0
        .byte 119,255,51,255,119,255,51,255,119,255,34,170,85,85,0,0
        .byte 255,204,255,204,255,204,255,170,255,68,170,170,85,85,0,51
        .byte 51,255,51,255,51,255,51,255,51,255,51,255,0,0,51,238
        .byte 238,0,136,0,136,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,119,0,17,0,17,0,0,0,0,0,0,0,0,0,0
        .byte 255,204,255,204,255,204,255,204,255,204,255,204,0,0,119,204
        .byte 85,85,85,85,85,85,85,85,85,85,85,85,85,85,85,85
        .byte 119,238,0,0,119,255,119,255,119,255,119,255,119,255,119,255
        .byte 0,0,0,0,0,0,0,0,0,0,136,0,136,0,238,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,17,0,17,0,119
        .byte 119,204,0,0,119,204,119,204,119,204,119,204,119,204,119,204
        .byte 221,85,255,85,255,119,221,85,85,85,85,221,85,85,85,85
        .byte 0,0,34,0,85,0,34,0,0,0,0,0,0,2,0,0
        .byte 0,0,0,0,0,32,34,0,32,0,216,136,32,0,34,0
        .byte 0,0,0,187,0,119,0,187,0,119,0,187,0,119,0,187
        .byte 0,0,255,255,255,238,255,255,255,238,204,119,204,102,238,255
        .byte 0,119,0,187,0,119,0,187,0,119,0,187,0,119,0,0
        .byte 221,102,153,51,153,34,255,255,255,238,153,119,255,238,0,0
        .byte 0,0,0,186,0,117,0,186,0,117,0,186,0,117,0,186
        .byte 0,0,250,250,245,228,250,250,245,228,200,114,196,100,234,250
        .byte 0,117,0,186,0,117,0,186,0,117,0,186,0,117,0,0
        .byte 213,100,152,50,145,32,250,250,245,228,152,114,245,228,0,0
        .byte 0,0,0,171,0,87,0,171,0,87,0,171,0,87,0,171
        .byte 0,0,175,175,95,78,175,175,95,78,140,39,76,70,174,175
        .byte 0,87,0,171,0,87,0,171,0,87,0,171,0,87,0,0
        .byte 93,70,137,35,25,2,175,175,95,78,137,39,95,78,0,0
        .byte 0,0,255,255,238,119,204,51,136,17,0,0,0,0,0,0
        .byte 0,0,175,175,78,87,140,35,8,17,0,0,0,0,0,0
        .byte 165,165,90,90,165,181,30,90,79,151,175,175,215,31,190,175
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,17,204,34,0,51,0,0,204,0,68,51,136,0,0
        .byte 0,0,51,136,85,204,85,204,102,68,51,136,0,0,102,0
        .byte 0,0,0,0,51,255,68,0,68,187,136,119,136,187,136,85
        .byte 0,0,0,0,255,204,0,238,255,102,255,187,255,187,255,187
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 196,0,234,0,68,117,16,186,49,117,34,186,128,0,170,50
        .byte 0,49,0,114,228,32,216,136,228,196,250,200,0,17,234,216
        .byte 160,49,136,0,32,177,50,80,17,177,64,114,228,0,200,0
        .byte 228,213,0,16,245,196,234,200,213,128,234,34,0,117,0,50
        .byte 199,15,235,15,79,125,30,190,61,125,47,190,135,15,175,62
        .byte 15,61,15,122,229,45,218,143,229,199,250,203,15,31,235,218
        .byte 165,61,143,15,45,181,62,90,31,181,75,122,229,15,203,15
        .byte 229,215,15,30,245,199,235,203,215,135,235,47,15,125,15,62
        .byte 165,165,90,90,229,165,90,90,165,165,122,90,165,165,90,90
        .byte 2,10,5,4,0,0,165,165,90,90,164,37,72,18,129,129
        .byte 144,144,192,48,224,112,80,208,160,160,0,0,32,160,80,64
        .byte 0,10,5,5,2,0,4,165,24,90,5,33,8,16,4,132
        .byte 10,8,5,4,0,2,165,1,90,10,132,129,8,2,33,1
        .byte 128,192,96,16,176,48,80,208,128,160,64,0,32,160,16,80
        .byte 48,0,128,80,192,160,112,16,160,32,0,64,160,128,80,0
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
        .byte 0,0,0,0,17,255,255,255,255,255,255,255,255,255,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,35,19,95,175,175
        .byte 95,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
        .byte 0,17,0,39,0,95,1,175,19,95,35,175,87,95,39,175
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
        .byte 95,127,175,191,95,127,175,191,95,127,175,191,95,95,255,255
        .byte 17,95,35,175,19,95,35,175,19,95,1,175,2,0,35,175
        .byte 95,127,175,255,95,127,175,191,95,127,175,255,95,127,175,255
        .byte 19,10,35,5,19,10,35,5,17,10,137,141,93,10,141,141
        .byte 93,93,141,170,76,85,140,174,78,93,38,174,2,93,0,174
        .byte 17,93,1,174,19,93,39,174,95,93,175,170,93,85,170,170
        .byte 85,87,170,175,85,87,170,171,85,87,170,175,85,87,170,175
        .byte 85,85,170,171,68,17,0,171,85,17,0,171,17,17,0,0
        .byte 255,255,255,255,238,119,119,238,119,238,119,238,119,238,51,204
        .byte 85,85,170,170,0,85,170,136,85,68,170,136,85,68,170,136
        .byte 85,119,170,187,68,0,136,0,0,255,153,255,0,255,153,255
        .byte 255,221,255,170,119,221,17,170,17,221,153,238,153,221,153,238
        .byte 255,238,255,136,255,0,238,0,204,0,136,0,0,0,136,0
        .byte 76,95,175,141,8,95,34,39,85,87,170,141,85,87,170,175
        .byte 0,34,0,85,0,170,0,119,0,187,0,119,0,34,0,0
        .byte 0,0,0,17,0,119,0,255,17,51,17,221,119,238,255,238
        .byte 51,51,255,221,255,238,255,255,255,255,255,255,255,170,221,68
        .byte 170,136,85,119,170,255,255,255,255,238,255,0,0,119,0,119
        .byte 255,238,255,221,255,170,255,204,238,170,221,68,102,136,0,0
        .byte 255,255,255,255,255,255,223,207,175,15,143,63,15,63,15,223
        .byte 0,0,0,0,0,0,34,136,221,102,255,255,255,255,255,255
        .byte 85,85,170,170,85,85,170,170,119,85,255,187,255,255,255,255
        .byte 85,85,170,170,85,85,170,170,85,85,170,170,221,85,238,238
        .byte 85,85,170,170,85,85,170,170,85,85,170,170,85,85,170,170
        .byte 85,85,170,170,85,85,170,170,119,255,255,255,119,255,187,255
        .byte 85,85,170,170,85,85,170,170,85,85,170,170,85,119,170,255
        .byte 85,85,170,170,85,119,187,255,119,255,255,255,255,255,255,255
        .byte 85,85,255,255,255,255,255,255,255,255,255,255,255,255,255,255
        .byte 85,85,238,170,255,221,255,255,255,255,255,255,255,255,255,255
        .byte 85,85,170,170,85,85,170,170,221,85,238,170,255,221,255,238
        .byte 85,85,170,170,85,85,170,170,85,85,187,170,119,221,255,170
        .byte 85,255,187,255,85,255,187,255,119,255,255,255,255,255,255,255
        .byte 255,255,255,255,255,255,255,221,255,136,255,136,255,204,255,204
        .byte 255,255,238,255,238,119,238,187,238,153,221,204,85,238,51,204
        .byte 85,85,170,170,85,85,170,170,221,0,204,51,136,102,17,204
        .byte 85,85,170,170,85,85,170,170,17,85,136,34,0,17,0,0
        .byte 255,85,0,238,204,51,238,0,255,0,255,136,102,0,0,136
        .byte 85,85,170,170,85,85,170,170,85,85,34,170,51,255,0,0
        .byte 85,85,170,170,85,85,170,187,85,119,170,170,255,119,34,136
        .byte 255,255,170,170,255,255,255,255,255,255,170,170,255,255,34,204
        .byte 255,238,170,153,255,221,255,204,255,204,170,170,255,255,238,51
        .byte 221,255,238,51,255,119,255,238,255,204,102,0,0,68,17,0
        .byte 51,238,34,204,51,187,119,170,119,136,170,34,255,17,136,17
        .byte 238,102,255,136,17,238,68,255,0,255,0,255,17,255,255,136
        .byte 0,0,0,0,0,0,0,0,136,0,204,0,204,0,204,0
        .byte 85,85,34,170,17,85,34,170,17,85,0,170,0,255,0,136
        .byte 85,85,170,170,85,85,170,170,85,85,170,170,255,255,51,136
        .byte 17,255,153,255,153,255,136,238,0,0,170,102,68,17,170,0
        .byte 255,255,255,255,17,255,0,51,17,0,0,187,85,68,0,34
        .byte 255,255,255,255,255,136,136,17,51,204,34,255,17,85,34,170
        .byte 255,255,255,255,51,255,255,255,0,51,170,136,204,221,170,170
        .byte 255,255,255,255,119,204,255,255,255,255,0,0,68,85,238,170
        .byte 255,255,255,255,0,119,255,255,255,255,119,255,17,255,170,0
        .byte 187,0,187,136,153,170,136,68,204,0,204,0,204,0,204,34
        .byte 255,136,119,136,51,204,68,136,68,0,0,136,0,0,0,0
        .byte 255,34,119,0,51,0,17,136,0,255,0,119,0,0,0,0
        .byte 102,0,102,0,119,0,255,0,238,0,136,0,0,0,0,0
        .byte 0,255,0,255,0,255,0,255,17,255,51,204,51,255,119,255
        .byte 255,255,255,255,0,255,255,255,255,255,119,187,255,255,255,255
        .byte 68,17,136,51,85,0,170,170,119,119,221,255,255,238,255,255
        .byte 136,85,0,170,17,85,34,170,119,17,221,187,119,255,255,255
        .byte 17,85,170,170,85,85,170,170,85,85,255,170,255,255,187,255
        .byte 17,68,170,170,85,17,34,170,85,17,255,187,255,255,255,255
        .byte 85,85,170,34,85,85,170,170,119,51,255,255,255,255,255,255
        .byte 85,85,170,170,85,85,187,170,255,255,255,119,255,255,255,255
        .byte 0,0,170,170,85,85,170,170,119,119,221,255,255,255,255,255
        .byte 68,0,136,0,85,136,153,136,153,0,136,0,136,0,136,0
        .byte 0,0,0,0,0,0,0,17,0,17,0,17,0,0,0,0
        .byte 0,0,0,17,0,119,187,0,238,0,238,255,238,255,102,0
        .byte 255,255,255,255,255,255,136,0,51,255,187,255,187,255,0,0
        .byte 255,255,255,255,255,255,0,0,255,255,255,255,255,255,0,0
        .byte 34,0,34,0,34,17,0,17,34,17,0,17,0,17,136,17
        .byte 0,68,0,85,0,119,0,68,136,51,153,255,153,255,153,238
        .byte 119,119,119,68,204,51,51,255,255,255,255,238,238,17,17,221
        .byte 221,238,17,238,153,238,153,238,153,238,17,238,153,238,153,238
        .byte 238,238,238,221,238,187,238,119,238,255,221,255,187,238,119,221
        .byte 204,51,255,119,255,119,255,187,255,255,255,255,255,255,0,0
        .byte 204,0,255,0,255,0,255,136,255,204,255,238,255,255,0,0
        .byte 0,0,0,102,0,238,51,238,119,238,119,238,255,238,255,204
        .byte 0,0,0,119,0,255,51,255,119,255,119,255,255,255,255,238
        .byte 255,238,255,238,255,204,255,204,255,204,255,204,255,238,255,238
        .byte 238,0,238,0,255,0,255,0,255,0,85,0,170,0,0,0
        .byte 0,0,255,255,170,170,85,17,170,170,85,85,170,34,204,51
        .byte 0,0,255,255,170,170,85,0,170,136,85,0,170,0,204,0
        .byte 255,204,255,238,119,238,119,238,51,238,0,238,0,102,0,0
        .byte 187,204,85,119,34,170,85,85,34,170,0,85,0,34,0,0
        .byte 0,0,255,255,153,153,153,153,255,255,170,187,85,68,0,0
bprop:
        .byte 0
        .byte 2
        .byte 0
        .byte 5
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
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 2
        .byte 2
        .byte 5
        .byte 2
        .byte 2
        .byte 5
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
        .byte 7
        .byte 2
        .byte 6
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 2
        .byte 2
        .byte 0
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 5
        .byte 5
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 2
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
        .byte 1,244,2,10,4,122,4,250,4,178,4,250,2,116,12,8,16,8,19,16,12,176,3,64,28,152,99,38,38,68,24,24
        .byte 0,0,1,244,2,10,4,122,4,250,4,178,4,250,2,116,29,8,32,8,35,0,28,168,3,96,4,160,4,160,4,176
        .byte 1,244,2,10,4,122,4,250,4,178,4,250,2,116,30,8,48,8,76,20,112,44,11,32,28,152,99,70,38,100,24,56
        .byte 47,128,80,64,94,32,95,32,77,32,95,32,46,64,16,48,16,8,8,200,13,48,2,192,25,56,100,198,34,100,24,24
        .byte 0,0,47,128,80,64,94,32,95,32,77,32,95,32,46,64,16,184,16,4,0,196,21,56,6,192,5,32,5,32,13,32
        .byte 47,128,80,64,94,32,95,32,77,32,95,32,46,64,16,120,16,12,40,50,52,14,4,208,25,56,98,198,38,100,28,24
        .byte 1,244,2,10,4,122,4,250,4,178,4,250,24,116,36,8,35,54,32,100,24,24,23,224,18,64,36,128,75,0,118,0
        .byte 0,0,1,244,2,10,4,122,4,250,4,178,4,250,24,112,36,54,35,100,32,24,25,224,14,64,49,64,39,128,40,0
        .byte 1,244,2,10,4,122,4,250,4,178,4,250,24,116,36,8,35,54,32,100,24,24,15,224,51,64,70,128,77,0,56,0
        .byte 47,128,80,64,94,32,95,32,77,32,95,32,46,24,16,36,108,196,38,4,24,24,7,232,2,72,1,36,0,210,0,110
        .byte 0,0,47,128,80,64,94,32,95,32,77,32,95,32,14,24,108,36,38,196,24,4,7,152,2,112,2,140,1,228,0,20
        .byte 47,128,80,64,94,32,95,32,77,32,95,32,46,24,16,36,108,196,38,4,24,24,7,240,2,204,1,98,0,178,0,28
        .byte 0,0,0,0,0,16,14,56,31,124,31,252,14,120,4,124,8,56,28,48,63,120,63,248,25,240,0,128,0,0,0,0
        .byte 0,0,0,0,60,60,126,254,127,254,62,254,60,108,60,24,14,60,12,126,62,254,127,124,127,252,127,60,12,8,0,0
        .byte 0,0,7,24,15,188,15,254,127,254,251,254,253,252,252,238,120,30,56,63,119,63,127,222,127,252,63,248,24,120,0,48
        .byte 0,0,0,0,0,16,14,56,23,84,27,108,4,112,0,40,8,24,28,0,20,112,40,168,25,112,0,0,0,0,0,0
        .byte 0,0,0,0,20,20,46,170,95,246,46,234,52,100,12,24,14,60,12,122,30,244,63,40,21,212,10,40,0,0,0,0
        .byte 0,0,0,8,5,20,10,170,85,214,171,234,93,252,188,234,88,28,56,58,87,52,43,218,87,84,42,168,16,80,0,0
        .byte 0,0,0,0,3,192,7,236,10,92,6,88,11,224,5,224,195,240,114,124,191,220,254,110,61,246,115,246,121,236,20,24
        .byte 255,255,175,255,0,0,42,156,68,56,46,118,76,238,45,206,75,174,39,110,78,238,60,238,90,238,0,0,95,255,175,255
        .byte 247,15,238,7,239,7,238,7,223,3,216,3,215,227,207,243,223,251,223,251,223,251,223,251,223,251,207,243,231,231,240,15
        .byte 255,255,0,3,255,237,255,222,255,158,255,158,85,30,0,30,170,158,0,30,0,30,0,30,0,30,254,237,0,19,255,255
        .byte 255,255,192,0,183,255,123,255,121,255,121,255,120,170,120,0,121,85,120,0,120,0,120,0,120,0,183,127,200,0,255,255
        .byte 127,254,131,23,127,254,128,1,138,171,133,255,138,255,69,254,98,6,63,252,0,0,101,226,211,229,229,239,3,192,0,0
        .byte 0,0,15,255,34,17,79,247,191,247,191,247,191,247,143,247,128,17,128,17,128,17,128,17,64,17,48,17,15,255,0,0
        .byte 0,0,0,255,1,250,3,244,7,232,15,208,31,224,0,255,0,230,1,152,3,96,7,128,14,0,28,0,48,0,0,0
        .byte 3,192,12,176,16,40,32,116,64,50,64,10,80,2,38,100,25,152,0,0,27,24,49,176,32,160,17,16,10,8,1,0
        .byte 7,224,8,80,16,8,32,228,32,100,32,20,32,4,46,116,17,136,0,0,24,216,13,140,5,4,8,136,16,80,0,128
        .byte 7,224,24,24,32,4,64,34,64,18,128,57,128,57,128,25,128,1,128,1,128,1,64,2,68,2,35,4,24,24,7,224
        .byte 3,192,12,48,16,8,32,68,32,36,64,114,64,114,64,50,64,2,64,2,64,2,32,4,34,4,17,8,12,48,3,192
        .byte 0,0,7,224,24,24,32,4,64,34,64,18,128,57,128,25,128,1,128,1,64,2,68,2,35,4,24,24,7,224,0,0
        .byte 134,6,0,32,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 3,192,4,32,8,16,8,16,8,16,11,208,102,102,243,207,216,27,2,224,5,224,10,208,61,188,110,118,102,102,52,44
        .byte 3,192,4,32,8,16,8,16,8,16,11,208,6,96,3,192,216,27,242,239,101,230,10,208,61,188,110,118,102,102,52,44
        .byte 7,224,10,240,21,248,11,248,85,250,64,10,18,8,8,24,79,242,128,4,67,194,40,17,35,193,72,18,135,228,3,192
        .byte 7,224,10,240,21,248,11,248,85,250,64,10,16,72,24,16,79,242,32,1,67,194,136,20,131,196,72,18,39,225,3,192
        .byte 0,0,11,240,231,255,211,245,103,242,0,0,63,252,106,6,85,254,170,255,213,255,128,1,49,140,107,90,123,222,49,140
        .byte 0,0,10,240,237,255,218,245,109,242,0,0,63,252,106,6,85,254,170,255,213,255,128,1,49,140,123,222,107,90,49,140
        .byte 0,0,15,80,239,191,223,85,111,178,0,0,63,252,106,6,85,254,170,255,213,255,128,1,49,140,90,214,123,222,49,140
        .byte 4,0,4,0,4,128,8,64,25,32,48,32,0,72,16,144,24,32,4,32,2,16,17,16,1,8,2,64,6,96,4,64
        .byte 0,64,0,68,2,64,4,64,0,16,17,8,18,8,18,64,16,8,8,16,8,32,4,64,0,64,1,64,2,32,2,16
        .byte 63,252,33,244,33,244,32,4,63,252,33,244,33,244,33,244,33,244,33,252,33,244,33,244,16,240,16,232,8,112,7,224
        .byte 0,128,1,128,1,128,3,192,7,224,7,224,15,176,15,208,31,216,31,248,31,248,31,216,11,144,12,16,6,32,3,192
        .byte 0,32,1,80,3,160,7,64,14,160,29,64,26,128,61,0,58,128,29,64,26,128,13,64,6,160,3,80,1,168,0,80
        .byte 7,224,31,152,63,196,127,226,127,242,255,161,255,209,193,161,227,81,254,161,221,65,106,130,64,2,32,4,24,24,7,224
        .byte 7,224,31,152,63,196,127,226,119,242,227,161,235,209,235,161,227,81,226,161,213,65,106,130,64,2,32,4,24,24,7,224
        .byte 7,224,31,240,16,232,31,72,16,112,16,224,15,220,0,30,246,230,59,62,0,28,3,0,12,252,56,127,19,50,12,12
        .byte 0,0,15,224,31,240,16,232,31,72,16,112,16,224,15,220,246,254,59,38,0,30,0,28,4,192,3,208,3,208,15,208
        .byte 31,128,32,64,80,160,80,96,64,160,80,96,32,192,31,248,17,252,40,178,41,142,7,208,31,56,126,126,62,60,24,24
        .byte 0,0,31,128,32,64,80,160,80,96,64,160,80,96,33,248,31,252,25,252,1,204,21,56,6,128,7,96,7,96,15,96
        .byte 31,51,32,136,65,72,80,128,80,92,64,162,41,73,17,149,14,41,33,245,46,194,12,24,2,168,1,84,0,238,0,110
        .byte 31,0,32,136,65,72,80,128,80,92,64,162,41,73,17,149,14,41,33,245,46,194,12,24,2,168,1,84,0,238,0,110
        .byte 7,224,25,248,35,252,71,254,79,254,133,255,139,255,133,131,138,199,133,127,130,187,65,86,64,2,32,4,24,24,7,224
        .byte 7,224,25,248,35,252,71,254,79,238,133,199,139,215,133,215,138,199,133,71,130,171,65,86,64,2,32,4,24,24,7,224
        .byte 7,224,15,248,23,8,18,248,14,8,7,8,59,240,120,0,103,111,124,220,56,0,0,192,63,48,254,28,76,200,48,48
        .byte 0,0,7,240,15,248,23,8,18,248,14,8,7,8,59,240,127,111,100,220,120,0,56,0,3,32,11,192,11,192,11,240
        .byte 1,248,2,4,5,10,6,10,5,2,6,10,3,4,31,248,63,136,77,20,113,148,11,224,28,248,126,126,60,124,24,24
        .byte 0,0,1,248,2,4,5,10,6,10,5,2,6,10,31,132,63,248,63,152,51,128,28,168,1,96,6,224,6,224,6,240
        .byte 204,248,17,4,18,130,1,10,58,10,69,2,146,148,169,136,148,112,175,132,67,116,24,48,21,64,42,128,119,0,118,0
        .byte 0,248,17,4,18,130,1,10,58,10,69,2,146,148,169,136,148,112,175,132,67,116,24,48,21,64,42,128,119,0,118,0
        .byte 0,0,255,240,136,68,239,242,239,253,239,253,239,253,239,241,136,1,136,1,136,1,136,1,136,2,136,12,255,240,0,0
        .byte 4,0,10,128,5,192,2,224,5,112,2,184,1,88,0,188,1,92,2,184,1,88,2,176,5,96,10,192,21,128,10,0
        .byte 1,244,2,10,4,122,4,250,52,179,76,251,74,117,36,9,16,14,14,16,4,16,2,0,4,144,4,144,9,32,14,192
frmlst:
        .byte 0,3
        .byte 3,3
        .byte 6,3
        .byte 9,3
        .byte 12,3
        .byte 15,3
        .byte 18,1
        .byte 19,1
        .byte 20,1
        .byte 21,1
        .byte 22,1
        .byte 23,1
        .byte 24,1
        .byte 25,1
        .byte 26,2
        .byte 28,3
        .byte 31,1
        .byte 32,2
        .byte 34,2
        .byte 36,3
        .byte 39,2
        .byte 41,1
        .byte 42,1
        .byte 43,1
        .byte 44,2
        .byte 46,2
        .byte 48,2
        .byte 50,2
        .byte 52,2
        .byte 54,2
        .byte 56,2
        .byte 58,2
        .byte 60,1
        .byte 61,1
        .byte 62,1,63,0
.out .sprintf("SPRITES SIZE = %d", (* - sprgfx))

scdat:
        .word 162,219,206,266,366,352,406,322,303,250,282,277,225,280,271,188,280,341,249,260,131,137,224,109,142,214
        .word 266,312
        .byte 107,255,0,29,107,255,0,7,85,255,0,16,86,255,0,4,107,255,0,29,107,255,0,29,107,255,0,29,107,255,0,29,107
        .byte 255,0,29,107,0,0,0,86,255,0,15,85,255,0,9,107,255,0,29,107,255,0,29,255,27,5,63,255,0,24,255,6,5
        .byte 63,255,0,24,255,6,5,63,255,0,24,6,6,88,6,6,63,255,0,24,6,6,90,6,6,63,255,0,17,15,255,16,6
        .byte 255,6,5,63,255,0,17,21,255,6,6,255,31,5,63,255,0,7,15,17,255,0,8,18,255,19,6,54,4,4,4,54,54,54
        .byte 55,4,55,54,55,55,4,4,54,55,54,4,55,54,55,4,4,55,4,4,54,55,54,255,57,30,255,58,30
        .byte 255,0,25,52,1,65,65,65,255,0,25,52,1,65,65,65,255,0,22,85,0,0,52,1,47,65,65,255,0,17,86,255,0,7
        .byte 52,1,47,65,65,255,0,25,52,1,65,65,65,0,0,0,85,255,0,21,52,1,65,65,65,255,0,25,52,1,65,65,65
        .byte 255,0,25,52,1,65,65,65,255,0,22,67,68,28,52,1,65,65,65,255,0,22,75,76,0,51,1,65,65,65,255,0,7
        .byte 86,255,0,9,85,255,0,4,27,0,0,51,1,65,65,65,255,0,22,27,0,0,51,1,65,65,65,255,0,22,27,0,0
        .byte 51,1,65,65,65,255,0,22,80,81,0,51,1,65,65,65,26,26,26,255,0,19,71,72,29,51,1,65,65,65,79,49,49
        .byte 255,0,18,58,255,57,8,79,49,49,255,0,18,44,255,58,8,79,255,0,20,43,42,42,42,41,255,42,4,79,255,2,19
        .byte 40,43,255,42,6,41,42,79,255,5,19,37,38,38,38,37,255,38,4,37
        .byte 44,255,5,19,39,41,42,43,42,43,42,42,43,42,45,255,5,20,255,37,9,44,255,5,20,39,40,255,5,7,41,255,5,29
        .byte 41,255,5,29,42,255,5,29,42,255,5,29,42,255,5,29,41,255,5,12,112,113,255,5,15,41,255,5,12,114,115,255,5,15
        .byte 41,255,5,29,32,60,28,255,5,27,53,60,53,255,5,27,30,61,53,255,5,16,87,88,255,5,9,31,61,29,255,5,16
        .byte 89,90,255,5,9,91,92,95,96,255,5,14,95,96,91,92,255,5,8,93,94,97,98,255,5,14,97,98,93,94,255,5,8
        .byte 255,37,5,38,38,38,255,37,10,255,38,7,255,37,5,42,43,41,41,42,41,41,43,42,42,41,41,41,43,42,255,41,4
        .byte 42,255,41,4,42,41,41,43,42,41,255,44,4,41,44,44,45,44,45,41,44,44,44,45,45,45,41,45,44,44,45,41,45
        .byte 41,45,45,44,45,44
        .byte 41,43,42,42,42,43,43,42,42,41,45,41,42,43,44,42,42,255,41,5,42,255,41,4,45,43,41,255,37,4,255,38,6
        .byte 255,37,7,255,38,7,255,37,6,255,5,6,112,113,255,5,18,13,1,1,1,255,5,6,114,115,255,5,18,13,1,1,1
        .byte 255,5,14,112,113,255,5,10,13,1,1,1,255,5,14,114,115,255,5,10,13,1,1,27,255,5,26,80,81,33,33,255,5,26
        .byte 71,72,0,0,255,5,26,255,0,4,255,5,26,255,0,4,255,5,26,255,0,4,255,5,26,67,68,0,0,255,5,26,75
        .byte 76,32,32,255,5,14,112,113,255,5,10,13,1,1,27,255,5,14,114,115,255,5,10,13,1,1,1,255,5,6,112,113,255,5,11
        .byte 87,88,5,5,91,92,5,13,1,1,1,255,5,6,114,115,255,5,11,89,90,5,5,93,94,5,10,1,1,1,38,36,36
        .byte 38,255,36,4,38,37,255,36,4,37,255,36,4,37,36,36,36,37,36,36,37,36,37,36,255,42,4,41,43,255,42,7,43
        .byte 42,41,43,42,43,43,41,43,42,41,44,43,43,42,45,45,255,44,9,45,45,44,45,255,44,10,45,45,45,255,44,4
        .byte 14,65,13,255,1,9,14,49,47,255,1,7,52,0,0,102,0,0,62,1,14,65,13,51,1,13,1,51,1,13,1,13,14
        .byte 49,65,255,1,6,13,52,0,0,102,0,0,62,1,14,65,13,255,1,9,14,49,65,255,1,6,13,52,0,0,102,0,0
        .byte 62,1,14,65,10,255,11,6,66,66,66,12,49,47,255,1,7,52,0,0,102,0,0,62,1,14,66,72,66,72,66,72,66
        .byte 72,66,72,66,72,66,72,255,11,5,66,66,12,0,0,102,0,0,62,1,27,255,0,5,48,255,0,4,48,0,0,48,255,0,10
        .byte 102,0,0,48,62,74,255,0,5,48,255,0,7,48,255,0,8,64,255,0,5,62,255,0,6,48,255,0,7,48,255,0,13
        .byte 67,68,255,0,6,48,255,0,17,25,25,26,26,75,76,255,0,24,255,27,4,80,81,255,0,20,25,26,255,0,6,71,75
        .byte 255,0,29,62,70,255,0,28,62,27,8,68,8,68,8,68,8,68,8,68,8,68,8,68,8,9,255,0,5,32,32,255,0,5
        .byte 62,7,8,8,8,83,9,255,1,10,14,255,0,5,66,66,25,26,0,0,0,62,51,1,1,1,8,14,11,11,11,66,66
        .byte 1,14,13,13,1,14,255,0,12,62,44,44,45,51,1,14,255,65,4,13,1,14,51,51,1,14,255,0,12,62,41,41,44
        .byte 51,1,14,65,47,65,47,10,13,255,1,4,14,255,0,4,255,32,8,62,41,41,44,51,1,14,255,65,5,10,255,11,4
        .byte 12,0,0,26,25,255,1,8,96,41,41,41,51,1,14,65,47,65,47,65,65,65,13,1,1,14,255,0,4,255,66,8,96
        .byte 44,44,44,255,33,6,51,95,52,95,255,33,4,255,0,4,255,33,9,34,35,255,0,7,51,97,52,97,255,0,6,25,26
        .byte 255,0,8,34,34,35,255,0,7,51,95,52,95,255,0,16,34,34,35,255,0,4,105,106,0,51,97,52,97,255,0,16,34,34
        .byte 35,255,0,4,27,27,27,255,1,4,255,28,10,70,255,0,5,34,34,35,255,0,4,255,33,7,255,29,10,78,255,0,5
        .byte 34,34,35,255,0,5,48,0,48,0,96,96,255,28,10,78,0,0,0,25,26,34,34,35,25,26,0,0,0,48,0,0,0
        .byte 95,95,29,29,29,59,56,56,59,56,29,29,74,255,0,5,34,34,35,255,0,9,97,97,0,0,48,255,0,13,34,34,35
        .byte 255,0,9,34,35,255,0,14,1,1,1,34,35,25,26,255,0,7,34,35,255,0,14,51,65,1,34,35,0,0,0,25,25,25
        .byte 26,32,60,95,95,255,0,14,51,65,96,34,35,255,0,7,33,60,97,97,255,0,14,51,65,96,23,23,72,32,255,0,6
        .byte 60,34,35,25,26,0,0,0,32,32,255,0,4,32,72,32,1,1,1,65,1,66,66,32,32,255,0,5,66,66,255,0,5
        .byte 33,33,255,0,4,255,66,6,65,96,255,0,27,34,65,1,255,0,11,25,26,255,0,14,34,65,1,0,0,0,91,95,96
        .byte 255,0,9,91,92,255,0,6,95,87,88,0,34,65,96,3,3,3,93,97,98,255,0,9,93,94,255,3,6,97,89,90,0
        .byte 34,65,1,32,32,101,101,101,255,32,11,101,101,255,32,6,101,32,32,32
        .byte 31,255,29,9,19,19,31,31,31,49,255,33,6,78,255,27,6,34,60,255,0,14,51,52,255,0,4,77,78,112,113,112,113
        .byte 112,113,34,1,1,255,0,20,78,110,111,110,111,110,111,34,1,1,255,0,20,60,255,0,6,71,255,0,13,32,32,27,27
        .byte 1,255,0,4,60,0,0,102,0,0,0,87,255,0,13,33,33,66,33,33,25,26,0,0,61,0,0,102,0,0,0,67,255,0,7
        .byte 51,32,32,255,0,12,61,0,0,102,0,0,0,34,71,71,71,75,0,0,0,51,1,96,255,0,12,61,0,0,102,0,0,0
        .byte 34,1,31,31,31,0,0,0,51,1,96,0,0,0,15,255,32,4,25,255,0,6,102,0,0,0,34,1,0,0,48,0,0,0
        .byte 51,1,96,0,0,0,18,19,95,65,96,255,0,7,102,0,0,0,34,1,255,0,6,11,11,11,0,0,0,48,0,97,65
        .byte 1,255,0,7,102,0,0,0,34,1,255,0,7,51,93,0,0,0,48,0,93,65,1,25,26,255,0,5,102,0,0,0,34
        .byte 1,255,0,7,51,93,255,0,5,93,65,96,255,0,7,102,0,0,0,71,66,255,0,7,51,93,255,0,5,93,65,1,255,0,7
        .byte 102,0,0,0,1,255,0,8,51,93,255,3,5,93,66,66,25,26,255,0,5,102,0,0,0,1,255,0,8,51,93,255,116,5
        .byte 93,87,87,255,0,7,102,0,0,0,1,255,0,8,51,93,255,116,5,93,89,89,255,0,7,102,0,0,0,1,32,11,32,32,32
        .byte 19,32,32,1,1,255,27,5,1,1,1,255,32,5,0,0,102,0,0,32,255,1,4,255,65,7,52,49,67,255,28,5,70
        .byte 49,51,1,1,0,0,102,0,0,1,1,33,33,255,66,8,52,49,71,255,29,5,74,49,51,33,33,0,0,102,0,0,33,33
        .byte 89,255,0,5,51,255,1,10,0,0,25,26,78,51,14,11,19,1,1,1,96,78,255,0,7,51,255,49,4,1,255,0,7
        .byte 87,51,14,255,0,4,51,96,89,25,26,255,0,5,51,49,1,1,49,1,255,0,7,78,51,27,0,0,0,51,1,96,78
        .byte 255,0,7,11,11,1,1,11,11,255,0,5,25,26,87,51,27,112,113,0,51,1,96,89,255,0,8,48,255,0,11,78,51
        .byte 27,110,111,255,0,4,32,32,32,255,0,6,48,255,0,11,87,51,27,108,109,255,0,4,14,33,33,255,0,16,25,26,78
        .byte 51,27,110,111,255,0,4,30,255,0,13,1,19,19,19,255,32,4,1,27,1,1,32,71,71,71,19,255,0,4,73,73,73
        .byte 255,0,5,14,14,95,89,95,97,91,97,95,95,89,95,65,255,11,4,19,255,0,4,51,33,33,255,0,9,48,0,0,48
        .byte 255,0,8,34,19,255,0,16,48,255,0,11,34,19,255,0,9,25,26,255,0,17,34,19,255,0,28,34,23,255,0,9,15
        .byte 17,255,0,17,23,30,255,0,5,255,32,4,21,22,255,19,4,15,17,73,73,73,32,32,32,255,0,6,51,32,255,0,4
        .byte 33,33,33,65,18,20,65,33,33,65,21,22,65,255,33,5,255,0,6,91,92,255,0,28,96,87,88,255,0,25,32,32,98
        .byte 89,90,255,0,25,96,1,33,33,33,255,24,5,75,31,255,73,10,75,31,255,24,6,33,33
        .byte 65,47,1,255,0,4,69,21,21,21,69,68,69,68,255,95,6,73,69,68,255,1,5,47,66,66,66,255,0,4,71,66,66,66
        .byte 73,72,73,72,255,66,6,73,73,72,255,33,5,66,91,255,0,9,48,255,0,4,48,255,0,8,48,255,0,5,93,255,0,14
        .byte 48,255,0,14,89,255,0,29,91,255,0,29,93,105,106,255,0,27,1,32,17,17,63,32,32,63,17,17,63,16,17,0,0,0
        .byte 32,32,63,16,63,17,255,0,8,51,33,31,31,63,33,33,63,31,31,63,31,20,0,0,0,66,66,63,31,63,20,255,0,5
        .byte 32,32,32,51,1,1,255,0,24,66,66,66,89,1,1,255,0,25,48,0,91,255,0,27,48,0,91,255,0,7,102,102,102
        .byte 255,0,4,102,102,102,255,0,10,48,0,89,255,0,5,67,119,117,117,117,120,70,67,119,117,117,117,120,70,255,0,10,91
        .byte 255,0,5,10,121,118,118,118,122,12,10,121,118,118,118,122,12,73,73,255,0,8,51,255,0,20,73,255,0,8,21,25,26
        .byte 255,0,18,13,73,73,73,255,0,5,51,255,0,20,13,1,255,32,7,51,255,0,5,255,32,8,96,255,0,6,13,1,255,49,4
        .byte 34,1,1,21,25,26,0,0,0,13,95,95,49,49,49,47,1,96,32,9,0,0,25,26,13,47,49,49,95,95,95,1,1
        .byte 63,255,16,4,34,35,255,0,10,34,35,13,14,16,16,89,255,0,4,27,27,63,255,31,4,34,255,28,12,35,13,14,31,31
        .byte 89,255,0,4,24,24,255,0,5,71,29,29,56,255,59,4,56,59,59,29,29,74,255,0,4,48,255,0,5,93,255,0,29
        .byte 93,255,0,29,95,255,0,29,95,255,0,27,25,26,20,255,0,29,95,32,32,32,96,80,83,69,70,255,0,21,95,66,66
        .byte 1,96,27,27,24,24,255,0,8,7,17,17,16,17,17,9,255,0,4,25,26,20,0,0,255,66,4,73,74,255,0,8,10
        .byte 255,11,5,12,255,0,6,95,255,0,11,19,19,255,0,16,95,255,0,27,25,26,20,255,0,29,34,255,0,29,34,255,0,27
        .byte 68,68,68,255,0,16,91,92,255,0,12,255,32,6,91,88,0,0,0,95,96,0,0,0,93,94,255,32,5,255,0,7,47
        .byte 21,87,21,87,47,89,90,3,3,3,97,98,3,3,3,95,96,255,11,4,1,16,16,255,17,5,255,66,5,95,96,91,87
        .byte 91,92,87,88,95,96,91,97,98,47,87,87,47,1,255,23,7
        .byte 96,255,66,4,255,0,4,34,35,255,31,5,0,0,0,255,31,11,96,95,255,0,7,34,35,255,0,18,91,96,95,255,0,7
        .byte 34,35,255,0,18,91,96,95,255,0,6,32,52,91,255,0,18,91,1,95,255,0,6,1,52,1,1,255,0,17,91,1,95
        .byte 0,0,0,32,32,32,47,52,1,1,255,0,17,91,1,95,0,0,0,33,33,33,66,33,33,255,0,18,91,1,89,255,0,27
        .byte 87,1,89,255,0,18,255,32,5,255,0,4,87,96,89,255,0,10,73,19,73,72,255,3,4,33,33,11,33,33,25,26,0,0
        .byte 87,24,89,255,0,10,255,31,4,255,24,4,255,0,9,87,6,89,255,0,6,25,26,255,0,19,87,6,89,102,102,102,0,0,0
        .byte 31,31,255,0,17,72,72,72,6,255,117,4,120,255,0,21,51,8,52,6,255,118,4,122,255,0,21,51,1,52,19,255,0,13
        .byte 91,92,0,0,0,95,96,255,0,4,73,72,51,66,66,255,0,14,93,94,0,0,0,97,98,255,0,4,51,1,51,49,49
        .byte 255,0,14,51,27,255,3,9,27,1,51,49,49,255,32,14,1,27,255,116,9,27,1,51,49,49,255,33,5,47,66,13,13
        .byte 66,47,255,33,4,255,27,11,1,51,49,49
        .byte 51,255,1,8,52,255,50,4,51,255,1,8,52,0,0,0,25,26,60,255,0,10,48,0,0,48,255,0,15,60,255,0,10
        .byte 48,255,0,18,60,255,0,26,17,17,32,60,255,0,18,17,16,17,16,16,17,16,16,1,1,1,60,0,0,0,95,91,92
        .byte 255,0,13,1,1,91,27,62,65,47,65,65,47,60,0,0,0,97,93,94,0,0,255,102,10,0,1,1,74,27,62,255,95,5
        .byte 60,17,17,32,32,101,101,67,119,255,117,10,120,70,1,1,1,62,65,47,65,65,47,60,255,33,4,47,33,71,121,255,118,10
        .byte 122,74,33,33,31,31,255,28,5,61,0,0,97,97,59,59,59,56,255,59,7,56,56,255,59,4,56,56,56,255,29,5,61
        .byte 255,0,29,61,255,0,29,61,255,0,14,255,102,9,255,0,6,60,11,61,0,0,255,32,8,67,119,255,117,9,120,70,0,0
        .byte 25,26,60,1,60,17,17,33,33,47,33,33,47,33,33,71,121,255,118,9,122,74,255,0,4,60,33,60,31,31,255,0,25
        .byte 61,34,35,255,0,25,25,26,60,34,35,255,0,27,61,34,35,255,0,27,61,1,255,32,4,255,0,4,51,255,32,6,0,0,0
        .byte 51,32,32,32,255,17,6,61
        .byte 255,72,4,79,79,255,72,6,13,11,11,255,72,9,79,79,255,72,4,255,0,12,48,255,0,56,108,109,255,0,28,110,111
        .byte 255,0,68,102,255,0,5,255,102,5,16,16,80,17,16,17,1,255,0,6,25,26,69,1,1,15,16,16,16,69,68,17,16,16
        .byte 17,1,1,255,33,7,255,0,8,96,255,33,14,65,0,0,48,0,48,255,0,9,1,93,255,0,13,66,255,0,4,48,255,0,7
        .byte 25,26,1,93,255,0,13,35,255,0,4,48,255,0,9,1,93,255,0,13,35,255,0,14,96,93,255,0,13,14,255,0,10
        .byte 67,17,17,17,1,93,255,0,4,63,30,30,30,34,255,11,4,12,255,0,10,10,255,11,4,12,255,0,4,63,31,31,31
        .byte 34,255,33,4,255,0,20,1,1,95,255,0,6,97,255,0,20,1,1,74,255,0,6,97,255,0,29,97,255,0,8,102,255,0,11
        .byte 102,255,0,8,97,255,19,5,255,32,9,11,19,19,255,32,7,255,0,4,30,30
        .byte 1,97,19,255,18,4,19,255,33,5,255,58,9,17,17,58,58,255,33,4,1,97,255,0,28,1,97,255,0,28,1,96,255,0,28
        .byte 1,14,255,0,28,1,14,255,0,28,18,20,255,0,28,19,19,255,0,15,15,28,28,28,27,0,0,15,255,28,5,1,97
        .byte 255,0,12,15,28,28,13,23,23,13,27,81,82,13,255,23,5,1,97,255,0,9,15,28,28,13,23,23,13,1,1,13,27
        .byte 96,96,13,255,1,4,14,1,97,25,26,0,0,15,27,28,28,27,13,23,23,13,1,1,13,1,1,13,27,1,1,13,255,1,4
        .byte 14,1,96,24,24,0,0,18,27,29,29,27,13,14,14,13,1,1,13,1,1,13,27,1,1,13,255,1,4,14,1,1,255,0,9
        .byte 18,29,29,13,14,14,13,1,1,13,27,96,96,13,255,1,4,14,1,14,255,0,13,29,29,13,14,14,13,27,0,0,13
        .byte 255,14,5,19,19,255,0,8,108,109,255,0,5,10,29,29,29,27,0,0,10,255,29,5,18,20,255,0,8,110,111,255,0,18
        .byte 18,20,255,0,28,18,20,255,0,28,1,14,75,255,0,12,102,255,0,14,1,1,9,255,0,4,19,71,67,19,19,19,255,58,9
        .byte 17,67,58,58,255,19,4
        .byte 8,11,8,8,8,11,255,8,4,96,11,11,11,21,255,19,4,35,0,0,102,0,34,255,19,4,22,0,48,0,0,0,48
        .byte 255,0,7,22,21,17,255,0,6,102,255,0,6,93,0,48,255,0,11,22,21,20,255,0,6,102,0,0,0,67,68,28,22
        .byte 0,48,255,0,11,18,19,255,0,6,64,102,0,0,0,75,76,0,22,255,0,26,80,81,0,22,255,0,22,25,25,26,26
        .byte 71,72,29,22,255,0,12,73,72,75,255,0,7,255,27,4,32,32,32,1,255,0,13,27,27,32,32,96,32,32,32,1,255,100,8
        .byte 255,0,4,30,30,30,255,32,4,92,30,80,52,49,47,49,49,49,47,49,49,49,47,49,49,49,47,51,0,0,0,82,1
        .byte 100,13,255,33,4,92,31,31,255,33,15,93,30,30,32,32,1,99,13,255,0,5,48,255,0,5,48,255,0,9,48,93,31
        .byte 19,11,11,31,31,31,255,0,5,48,255,0,15,48,95,255,0,28,48,97,255,0,28,48,97,255,0,28,48,97,255,0,29
        .byte 97,255,0,29,96,17,255,0,28,1,11,255,0,28,1,1,9,17,9,255,11,5,255,24,5,255,11,5,9,17,9,8,9
        .byte 255,0,4,26,1
        .byte 255,31,10,19,18,18,18,255,31,10,19,18,18,18,31,31,255,0,7,48,0,48,255,0,6,48,255,0,20,48,255,0,8
        .byte 48,0,108,109,255,0,26,48,0,110,111,255,0,179,255,32,5,255,0,6,255,11,5,96,9,255,0,4,7,72,72,72,8
        .byte 72,72,72,1,1,47,96,1,75,72,75,72,8,30,33,33,66,33,33,33,12,255,0,4,97,255,11,7,33,33,66,33,33
        .byte 255,66,5,19,255,0,11,71,29,29,56,59,56,59,29,74,255,0,109,102,102,102,255,0,26,119,117,117,117,120,73,255,0,4
        .byte 78,78,78,11,11,11,78,78,78,17,17,17,255,0,8,121,15,16,70,122,73,255,0,4,11,11,49,47,49,47,49,49,49
        .byte 11,11,11,32,32,32,96,95,32,32,32,255,8,5,96,32,101,101,32,255,33,12
        .byte 1,1,74,19,19,18,18,18,31,6,255,31,4,255,18,4,255,31,6,19,18,18,31,31,31,1,108,109,0,0,108,109,0,0
        .byte 48,0,0,48,255,0,7,48,255,0,9,96,110,111,0,0,110,111,0,0,48,0,0,48,255,0,7,48,255,0,9,35,255,0,8
        .byte 48,255,0,10,48,255,0,9,35,255,0,8,48,255,0,20,35,89,1,1,255,0,26,35,71,1,1,255,0,26,95,108,109
        .byte 255,0,21,67,70,255,0,4,95,110,111,255,0,8,102,102,102,255,0,4,102,102,102,0,0,0,24,87,108,109,0,0,95
        .byte 255,0,9,119,117,117,117,120,0,0,119,117,117,117,120,0,0,96,87,110,111,0,0,95,255,0,9,121,118,118,118,122,0,0
        .byte 121,118,118,118,122,0,0,1,97,19,11,11,11,93,255,0,23,51,33,66,66,33,66,93,73,73,255,0,27,31,31,31,25
        .byte 26,255,0,25,28,28,22,255,0,27,29,29,22,255,0,27,23,23,23,25,26,255,0,25,47,65,96,255,0,15,17,19,19
        .byte 17,255,0,8,49,65,1,255,0,15,51,47,47,52,255,0,8,49,65,1,95,32,75,72,75,72,75,72,75,72,75,72,32,32,32
        .byte 255,23,4,95,255,32,4,96,32,32
        .byte 61,31,31,19,18,18,18,255,31,6,19,18,18,255,31,4,255,27,4,255,31,5,19,61,255,0,9,48,255,0,18,21,74
        .byte 255,0,9,48,255,0,9,108,109,108,109,255,0,5,21,1,1,255,0,18,110,111,110,111,255,0,5,22,1,1,255,0,27
        .byte 22,97,255,0,28,22,12,255,0,4,15,17,255,0,22,22,255,0,5,62,72,25,26,255,0,13,102,255,0,6,22,0,0,0
        .byte 25,26,62,87,255,0,15,102,255,0,6,22,255,0,5,62,89,255,0,10,15,16,17,0,0,102,0,7,17,17,17,32,22
        .byte 255,0,5,62,72,25,26,0,0,0,32,255,17,4,32,33,33,0,0,102,0,10,11,11,11,96,31,255,0,5,62,91,255,0,5
        .byte 51,255,49,5,52,95,0,0,102,255,0,6,21,32,32,17,17,17,62,91,255,0,5,51,65,47,65,65,47,52,97,0,0
        .byte 102,255,0,6,21,255,33,5,62,72,25,26,0,0,0,51,24,24,24,31,31,1,96,0,0,102,255,0,6,22,60,0,0
        .byte 19,19,62,87,255,0,11,33,33,0,0,102,255,0,6,22,60,255,0,4,62,89,255,0,11,34,35,0,0,102,255,0,4
        .byte 25,26,22,61,255,0,4,27,27,25,26,255,0,9,34,35,0,0,102,255,0,6,22,61,255,0,17,1,96,0,0,102,255,0,6
        .byte 22,61,105,106,255,0,15,1,1,0,0,102,255,0,6,22,1,255,17,6,255,19,5,255,17,5,32,1,1,0,0,102,0
        .byte 15,16,11,11,16,22
        .byte 99,99,31,31,21,21,255,31,5,21,18,18,18,19,21,31,31,18,18,18,19,21,255,31,4,21,7,21,22,255,0,12,48
        .byte 0,0,0,48,0,0,0,48,0,0,0,48,0,0,10,21,22,255,0,16,48,0,0,0,48,0,0,0,48,0,0,91,21
        .byte 22,255,0,16,48,0,0,0,48,0,0,0,48,0,0,97,21,22,255,0,24,48,0,0,97,21,22,255,0,27,1,21,22
        .byte 255,0,27,96,21,22,255,0,28,21,22,255,0,28,19,20,255,0,9,255,102,5,255,0,25,119,117,117,117,120,255,0,22
        .byte 108,109,0,121,118,118,118,122,0,0,0,255,102,4,255,0,15,110,111,255,0,9,119,117,117,120,0,0,0,25,26,32,32
        .byte 92,80,80,80,255,0,12,108,109,0,121,118,118,122,255,0,5,34,35,60,33,255,0,14,110,111,255,0,10,34,35,60,255,0,25
        .byte 25,26,1,1,60,255,0,27,34,35,61,255,0,27,34,35,61,255,0,24,32,32,66,49,49,61,15,50,31,80,80,31,31
        .byte 80,80,31,50,255,31,8,50,31,80,80,31,17,255,33,4
        .byte 21,255,6,4,255,49,21,21,6,6,6,21,6,95,95,95,255,28,21,95,95,95,21,21,6,33,33,33,255,29,6,56,56
        .byte 59,59,59,56,59,56,255,29,7,33,33,33,21,21,6,255,0,22,48,0,0,48,0,21,21,6,255,0,25,48,0,21,21
        .byte 6,0,0,0,105,106,255,0,22,21,21,6,0,0,0,21,30,31,72,31,72,31,72,31,30,25,25,25,26,26,26,255,0,8
        .byte 21,21,6,0,0,0,18,255,19,6,31,31,31,255,0,14,21,21,20,255,0,27,21,255,0,18,25,26,26,255,0,8,18
        .byte 255,0,30,75,75,80,255,0,27,95,91,255,0,23,15,17,0,0,0,92,27,92,27,255,3,4,30,30,95,255,32,6,96
        .byte 21,30,30,255,3,4,27,6,92,92,92,66,66,66,27,255,116,4,71,31,97,33,33,66,66,66,33,33,21,31,74,255,116,4
        .byte 27,6,66,66,66,61,99,61,27,255,116,21,27,6,61,99,61,61,99,61,27,255,116,21,27,6,61,99,61,61,99,61,27
        .byte 255,116,21,27,6,61,99,61,60,65,60,255,27,23,6,60,65,60,60,47,60,255,33,24,60,47,60
        .byte 107,255,0,28,52,107,255,0,28,52,107,255,0,28,52,107,255,0,28,52,107,255,0,28,52,107,255,0,28,52,107,255,0,26
        .byte 67,1,65,107,255,0,26,73,33,65,107,255,0,29,107,255,0,29,107,255,0,29,107,255,0,6,108,109,255,0,10,108,109
        .byte 0,0,0,255,31,4,32,32,107,255,0,6,110,111,255,0,10,110,111,255,0,4,71,19,11,1,65,107,255,0,27,1,65
        .byte 107,255,0,27,33,65,107,255,0,28,52,107,255,0,28,52,107,255,0,28,52,107,255,0,28,52,107,255,0,28,52
        .byte 107,255,0,28,52,107,255,0,28,52,107,255,0,28,52,107,255,0,28,52,107,255,0,28,52,107,255,0,28,52,107,255,0,28
        .byte 52,107,255,0,28,52,107,255,0,27,52,1,107,255,0,23,67,69,32,32,1,49,107,255,0,12,108,109,255,0,9,34,76
        .byte 1,1,65,65,107,255,0,12,110,111,255,0,9,34,81,1,1,65,65,107,255,0,23,71,72,33,33,65,65,107,255,0,27
        .byte 52,47,107,255,0,26,1,1,49,107,255,0,26,1,1,49,107,255,0,27,52,47,107,255,0,28,52,107,255,0,28,52,107
        .byte 255,0,28,52
        .byte 107,255,0,28,52,107,255,0,28,52,107,255,0,28,52,107,255,0,26,62,32,52,107,255,0,26,34,1,52,107,255,0,25
        .byte 1,1,1,52,107,255,0,25,1,1,1,52,107,255,0,26,34,1,52,107,255,0,26,62,33,65,107,255,0,4,255,46,19
        .byte 255,0,5,47,107,255,0,11,77,83,79,255,0,6,140,255,0,7,65,107,255,0,8,21,30,255,53,5,70,81,0,82,53
        .byte 6,255,0,7,65,107,255,0,8,21,22,141,77,143,135,137,255,53,4,88,141,255,0,7,47,107,255,0,4,108,109,0,0
        .byte 21,22,81,82,125,135,129,53,6,84,84,76,0,108,109,255,0,5,65,107,255,0,4,110,111,0,0,89,255,6,6,84,72
        .byte 255,0,4,110,111,255,0,5,52,107,255,0,28,52,107,255,0,9,255,46,7,255,0,12,52,107,80,83,255,80,5,83,255,80,5
        .byte 83,255,80,5,83,255,80,5,83,31,31,31,107,255,0,27,97,47,107,255,0,27,52,65
        .byte 255,107,189,255,152,7,150,151,152,152,152,255,107,18,152,153,152,154,155,156,157,158,152,152,159,152,255,107,18,152,152,152,160
        .byte 53,161,162,53,163,164,152,152,255,107,18,165,166,167,168,168,169,170,171,172,173,174,175,255,107,18,176,177,178,179,180,181,182
        .byte 183,184,185,186,187,255,107,18,188,189,190,191,192,193,194,195,196,197,198,199,255,107,18,255,53,7,200,201,202,203,204,255,0,5
        .byte 255,107,8,255,0,22,255,107,184
        .byte 255,107,31,255,0,29,107,255,0,25,86,0,0,0,107,255,0,29,107,255,0,29,107,255,0,6,108,109,255,0,21,107,255,0,6
        .byte 110,111,255,0,11,108,109,255,0,8,107,255,0,19,110,111,255,0,8,107,255,0,29,107,255,0,29,107,255,0,29,107,0,0,0
        .byte 86,255,0,25,107,255,0,29,107,255,0,24,67,79,80,80,80,107,255,0,11,108,109,255,0,8,108,109,0,62,33,33,33
        .byte 1,107,255,0,11,110,111,255,0,8,110,111,255,0,5,1,107,255,0,28,1,107,255,0,27,52,1,107,255,0,27,52,1
        .byte 107,255,0,27,34,1
        .byte 255,107,26,84,50,6,6,255,0,26,79,50,6,6,255,0,26,18,50,50,6,0,0,86,255,0,24,84,50,50,255,0,28
        .byte 79,84,255,0,29,18,255,0,68,108,109,255,0,6,67,78,70,255,0,19,110,111,255,0,6,78,47,78,255,0,27,78,47
        .byte 78,0,0,86,255,0,24,27,27,27,255,0,5,108,109,255,0,20,61,83,61,255,0,5,110,111,255,0,4,80,80,80,70
        .byte 255,0,12,61,83,61,255,0,11,83,83,83,61,255,0,12,61,83,61,255,0,11,31,31,31,61,255,0,12,61,83,61,255,0,11
        .byte 1,1,1,32,32,32,79,79,70,255,0,7,60,83,60,255,0,11,7,8,8,8,9,13,1,96,32,32,255,0,6,60,83
        .byte 60,255,0,11,13,1,1,1,14,13,27,1,1,255,80,4,88,87,87,1,1,1,87,87,88,255,79,6,80,80,13,1,1,1
        .byte 96,13,27,1,7,255,8,15,9,13,255,1,4
        .byte 6,21,6,255,53,4,6,21,6,255,53,9,6,21,6,6,53,53,6,21,53,53,6,50,21,6,6,53,53,53,6,21,6
        .byte 255,53,9,6,21,6,6,53,53,6,21,53,6,6,50,21,255,6,6,21,6,6,255,53,7,6,6,21,6,53,53,6,6
        .byte 21,6,6,84,84,50,21,50,255,6,4,21,6,6,6,255,53,5,6,6,6,21,255,6,4,21,6,84,84,84,18,79,21
        .byte 84,50,50,6,6,21,255,6,11,21,255,6,4,21,50,79,79,20,0,77,21,79,79,84,84,50,6,83,255,6,9,83,6,6
        .byte 84,84,84,21,20,76,255,0,5,77,18,255,79,4,21,255,84,9,21,84,79,79,79,20,76,255,0,11,77,18,21,255,19,9
        .byte 21,20,76,255,0,33,86,255,0,75,15,16,17,255,87,6,255,0,21,21,6,22,255,89,6,255,0,21,18,19,20,255,6,5
        .byte 76,0,108,109,255,0,28,110,111,255,0,97,255,80,5,79,79,67,0,0,67,80,255,31,7,80,70,0,0,67,79,79,255,80,4
        .byte 89,91,89,95,89,89,61,88,32,32,31,61,89,91,89,95,89,91,89,61,88,32,32,31,61,95,89,89,91,89
        .byte 6,6,6,21,6,53,53,6,255,107,22,6,6,6,21,53,53,6,6,255,0,21,107,6,6,6,21,6,6,6,20,255,0,20
        .byte 86,107,84,84,21,84,6,84,20,76,255,0,9,108,109,255,0,10,107,79,79,21,79,79,20,76,255,0,10,110,111,255,0,6
        .byte 125,124,6,6,107,19,20,76,255,0,21,127,128,123,123,123,107,107,255,0,22,127,128,126,123,123,123,107,107,255,0,22,130
        .byte 129,255,123,4,107,107,255,0,22,132,131,123,141,0,123,107,107,255,0,22,133,139,140,0,0,135,107,107,255,0,16,143,147
        .byte 148,145,149,107,134,6,6,138,136,135,107,107,255,0,17,143,140,146,146,0,135,128,123,135,136,138,107,107,255,0,18,143,146
        .byte 145,0,136,137,137,138,0,0,107,107,255,0,7,108,109,255,0,9,143,145,255,107,10,255,0,7,110,111,0,0,0,15,16
        .byte 22,255,0,5,107,143,146,149,149,146,145,0,107,107,255,0,12,21,6,22,255,0,5,107,144,145,148,149,145,147,0,107,107
        .byte 255,0,12,21,16,16,255,0,5,107,147,0,145,147,145,0,0,107,107,255,0,12,18,31,31,255,0,5,107,0,0,147,0
        .byte 147,0,0,107,80,80,79,80,80,80,79,80,80,80,255,31,4,13,255,32,14,96,255,99,4,87,93,97,99,99,99,97,87
        .byte 93,63,13,11,11,11,7,255,8,8,9,1,1
numsc:
        .byte 28
.out .sprintf("SCREENS SIZE = %d", (* - scdat))

nmedat:
        .byte 0,0,72,24,1,25,128,88,1,19,128,152,255
        .byte 0,0,104,16,3,27,48,136,255
        .byte 0,2,112,32,2,14,56,200,2,14,112,80,255
        .byte 0,2,80,32,2,14,48,168,2,14,104,72,255
        .byte 0,0,96,32,1,19,96,128,1,25,128,224,255
        .byte 0,1,96,208,2,18,112,176,1,25,80,80,4,16,64,136,255
        .byte 0,1,128,176,3,17,80,24,4,20,24,128,4,9,24,8,1,29,56,128,4,20,56,112,4,16,120,88,255
        .byte 0,1,128,224,1,29,144,168,1,25,144,80,2,28,80,24,4,8,24,88,1,25,104,128,255
        .byte 0,1,136,88,1,19,48,96,2,17,72,192,4,9,80,16,7,7,96,80,255
        .byte 0,0,128,32,8,11,80,96,1,25,64,136,4,16,24,96,4,20,32,40,4,20,56,40,2,24,80,208,1,26,128,160,255
        .byte 0,0,136,32,8,11,64,24,2,18,48,224,4,9,40,88,4,16,144,152,255
        .byte 0,0,144,80,4,10,48,160,1,26,96,56,4,16,80,96,7,7,96,176,1,26,144,200,255
        .byte 0,1,144,192,2,18,88,16,4,10,128,168,1,26,48,120,1,30,48,184,1,26,144,80,255
        .byte 0,1,144,216,2,17,112,184,3,13,48,144,8,11,88,40,1,26,144,80,255
        .byte 0,1,144,168,1,25,48,136,2,18,112,40,4,20,40,112,3,15,88,200,3,15,128,104,255
        .byte 0,1,128,208,1,19,72,104,2,18,88,72,4,16,104,136,255
        .byte 0,1,144,216,1,25,144,136,3,27,40,176,2,18,24,128,4,9,48,24,8,11,112,64,255
        .byte 0,1,144,216,1,25,144,120,1,29,72,112,3,17,16,48,4,9,32,8,4,20,144,48,1,26,144,32,255
        .byte 0,1,88,216,8,11,80,56,3,17,32,40,1,26,144,40,3,28,144,160,7,7,72,120,255
        .byte 0,1,96,224,2,14,120,48,2,14,96,184,4,20,80,8,4,16,24,136,255
        .byte 0,1,80,208,5,32,32,40,255
        .byte 0,0,120,24,4,10,120,224,3,31,56,112,5,32,64,16,3,27,56,216,255
        .byte 0,0,128,24,4,10,48,216,3,17,32,72,255
        .byte 255,0,2,136,32,3,13,16,160,3,27,32,200,3,31,88,64,255
        .byte 0,2,72,24,2,17,40,144,3,15,128,104,3,15,80,168,1,29,120,48,1,26,136,200,255
        .byte 0,2,136,24,1,26,80,152,3,27,96,200,1,30,136,104,2,18,96,72,255
        .byte 0,2,136,24,4,24,72,216,4,20,72,160,3,13,72,144,255
.out .sprintf("POSITIONS SIZE = %d", (* - nmedat))

NUMOBJ = 7
objdta:
        .byte 0,0,28,0,46,0,46,0,50,0,28,0,0,0,6,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,24,56,5,24,56
        .byte 0,0,28,0,46,0,46,0,50,0,28,0,0,0,6,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,48,16,8,48,16
        .byte 0,0,28,0,46,0,46,0,50,0,28,0,0,0,6,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,19,40,48,19,40,48
        .byte 0,0,28,0,46,0,46,0,50,0,28,0,0,0,6,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,17,144,16,17,144,16
        .byte 7,0,15,194,31,230,23,244,75,240,229,248,226,132,181,124,90,224,45,192,82,132,38,104,12,0,9,0,2,0,0,0,22,128,224,22,128,224
        .byte 0,0,0,0,0,0,0,0,124,102,254,255,198,253,28,253,6,255,198,122,254,52,124,24,0,0,0,0,0,0,0,0,9,48,96,9,48,96
        .byte 0,0,0,0,0,0,0,0,124,102,254,255,198,253,28,253,6,255,198,122,254,52,124,24,0,0,0,0,0,0,0,0,17,104,216,17,104,216
.out .sprintf("OBJECTS SIZE = %d", (* - objdta))

font:
        .byte 10,50,49,125,62,11,50,50
        .byte 125,62,13,205,154,138,62,50
        .byte 71,205,236,126,201,0,0,0
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
        .byte 124,254,206,222,246,230,254,124
        .byte 56,120,120,24,24,24,126,126
        .byte 124,254,198,6,124,192,254,254
        .byte 124,254,198,28,6,198,254,124
        .byte 24,56,120,120,216,254,254,24
        .byte 254,254,192,252,6,198,254,124
        .byte 124,252,192,252,198,198,254,124
        .byte 254,254,6,12,24,48,48,48
        .byte 124,254,198,124,198,198,254,124
        .byte 124,254,198,198,126,6,126,124
        .byte 0,0,0,48,0,0,48,0
        .byte 0,0,48,0,0,48,48,96
        .byte 0,0,12,24,48,24,12,0
        .byte 0,0,0,126,0,126,0,0
        .byte 0,0,48,24,12,24,48,0
        .byte 0,124,198,12,24,0,24,0
        .byte 0,124,222,254,254,192,124,0
        .byte 124,254,198,198,254,198,198,198
        .byte 252,254,198,252,198,198,254,252
        .byte 124,254,198,192,192,198,254,124
        .byte 248,252,206,198,198,206,252,248
        .byte 254,254,192,252,252,192,254,254
        .byte 254,254,192,252,252,192,192,192
        .byte 124,254,192,222,222,198,254,124
        .byte 198,198,198,254,254,198,198,198
        .byte 126,126,24,24,24,24,126,126
        .byte 6,6,6,6,198,198,254,124
        .byte 206,220,248,240,248,220,206,199
        .byte 192,192,192,192,192,192,254,254
        .byte 198,238,254,254,198,198,198,198
        .byte 198,230,246,254,254,222,206,198
        .byte 124,254,198,198,198,198,254,124
        .byte 252,254,198,198,254,252,192,192
        .byte 124,254,198,198,246,222,254,124
        .byte 252,254,198,198,254,252,206,199
        .byte 124,254,192,252,126,6,254,124
        .byte 254,254,48,48,48,48,48,48
        .byte 198,198,198,198,198,198,254,124
        .byte 198,198,198,198,198,238,124,56
        .byte 198,198,198,198,214,254,254,108
        .byte 198,198,238,124,124,238,198,198
        .byte 134,206,252,120,48,48,48,48
        .byte 254,254,14,28,56,112,254,254
        .byte 0,30,24,24,24,24,30,0
        .byte 0,0,192,96,48,24,12,0
        .byte 0,240,48,48,48,48,240,0
        .byte 0,48,120,252,48,48,48,0
        .byte 0,0,0,0,0,0,0,255
        .byte 0,60,102,248,96,96,254,0
        .byte 0,0,0,0,0,0,0,0
        .byte 48,120,120,120,48,0,48,0
        .byte 28,52,62,120,158,239,247,126
        .byte 0,125,254,234,85,0,24,0
        .byte 0,6,60,108,242,222,227,126
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

jtab:
        .byte 248,250,252,254,254,255,255,255,0,0,0,1,1,1,2,2,4,6,8,8,8,99
keys:   .byte 66,97,104,72,73,16,55,48,49,17,18
.out .sprintf("KEYS SIZE = %d", (* - keys))

data_end:
.out .sprintf("TOTAL DATA SIZE = %d", (* - data_start))
