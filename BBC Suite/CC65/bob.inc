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
WINDOWLFT = 0
WINDOWHGT = 21
WINDOWWID = 32 ;面面面面面面面面面面面面
MAPWID = 5
        .byte 255,255,255,255,255
mapdat:
        .byte 255,1,255,0,255
        .byte 255,255,255,255,255
stmap:  .byte 1

evnt00:
        lda #1
        cmp scno
        beq *+5
        jmp a00021
        rts
a00021: lda #255
        cmp varj
        bne *+5
        jmp a00175
        lda #199
        cmp varj
        bcc *+5
        jmp a00170
        lda #253
        cmp varj
        beq *+5
        jmp a00155
        lda #88
        ldy #8
        sta (z80_ix),y
        lda #120
        iny; ldy #9
        sta (z80_ix),y
        lda #0
        ldy #6
        sta (z80_ix),y
        lda #1
        iny; ldy #7
        sta (z80_ix),y
        ; lda #1
        sta chary
        lda #24
        sta charx
        lda #8
        jsr dmsg
a00155: lda varj
        clc
        adc #1
        sta varj
a00170: jmp a01118
a00175: lda #0
        sta vare
        lda #2
        sta loopa
a00191: lda vare
        clc
        adc #1
        sta vare
        lda #28
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        clc
        adc #1
        sta varrnd
        lda varrnd
        sta varn
        lda #5
        cmp varn
        beq *+5
        jmp a00295
        lda varn
        sec
        sbc #1
        sta varn
a00295: lda #6
        cmp varn
        beq *+5
        jmp a00327
        lda varn
        clc
        adc #1
        sta varn
a00327: lda #11
        cmp varn
        beq *+5
        jmp a00359
        lda varn
        sec
        sbc #1
        sta varn
a00359: lda #12
        cmp varn
        beq *+5
        jmp a00391
        lda varn
        clc
        adc #1
        sta varn
a00391: lda #18
        cmp varn
        beq *+5
        jmp a00423
        lda varn
        sec
        sbc #1
        sta varn
a00423: lda #19
        cmp varn
        beq *+5
        jmp a00456
        lda varn
        clc
        adc #1
        sta varn
a00456: lda #24
        cmp varn
        beq *+5
        jmp a00488
        lda varn
        sec
        sbc #1
        sta varn
a00488: lda #25
        cmp varn
        beq *+5
        jmp a00520
        lda varn
        clc
        adc #1
        sta varn
a00520: lda #17
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #3
        sta z80_c
        lda varrnd
        clc
        adc z80_c
        sta varrnd
        lda varrnd
        sta varm
        lda #8
        cmp varm
        beq *+5
        jmp a00618
        lda varm
        sec
        sbc #1
        sta varm
a00618: lda #9
        cmp varm
        beq *+5
        jmp a00650
        lda varm
        clc
        adc #1
        sta varm
a00650: lda #14
        cmp varm
        beq *+5
        jmp a00682
        lda varm
        sec
        sbc #1
        sta varm
a00682: lda #15
        cmp varm
        beq *+5
        jmp a00714
        lda varm
        clc
        adc #1
        sta varm
a00714: lda varm
        asl a
        asl a
        asl a
        sta varm
        lda varn
        asl a
        asl a
        asl a
        sta varn
        lda #0
        sta varf
        lda #1
        cmp vare
        beq *+5
        jmp a00869
        ldy #11
        lda (z80_ix),y
        sta vark
        iny; ldy #12
        lda (z80_ix),y
        sta z80_c
        lda vark
        clc
        adc z80_c
        sta vark
        lda vark
        cmp varc
        bcc *+5
        jmp a00869
        lda #3
        sta z80_c
        lda #10
        sta z80_b
        jsr spawn
        lda #1
        sta varf
a00869: lda #2
        cmp vare
        beq *+5
        jmp a00997
        ; lda #2
        cmp numlif
        bcs *+5
        jmp a00997
        lda #4
        sta z80_c
        lda #8
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #0
        ldy #7
        sta (z80_ix),y
        ; lda #0
        ldy #12
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #1
        sta varf
a00997: lda #1
        cmp varf
        beq *+5
        jmp a01079
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda varm
        ldy #8
        sta (z80_ix),y
        lda varn
        iny; ldy #9
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
a01079: dec loopa
        beq :+
        jmp a00191
:
        lda #0
        sta vark
        ; lda #0
        sta varj
        lda varc
        sta vare
a01118: lda joyval
        and #32
        beq :+
        jmp a01190
:
        lda #0
        sta chary
        ; lda #0
        sta charx
        lda #7
        jsr dmsg
        jsr prskey
        lda #0
        sta chary
        ; lda #0
        sta charx
        lda #10
        jsr dmsg
a01190: lda joyval
        and #64
        beq :+
        jmp a01563
:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp a01563
        ; lda #0
        iny; ldy #12
        cmp (z80_ix),y
        bcc *+5
        jmp a01563
        lda #6
        ; ldy #12
        cmp (z80_ix),y
        bcc *+5
        jmp a01290
        ; lda #6
        sta vark
        jmp a01304
a01290: ldy #12
        lda (z80_ix),y
        sta vark
a01304: lda vark
        sta z80_c
        ldy #11
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #11
        sta (z80_ix),y
        lda vark
        sta z80_c
        iny; ldy #12
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #12
        sta (z80_ix),y
        lda #100
        asl a
        sta sndtyp
        lda #26
        sta varrnd
        dey; ldy #11
        lda (z80_ix),y
        sta loopa
a01415: lda varrnd
        sta charx
        lda #23
        sta chary
        lda #12
        jsr dmsg
        lda varrnd
        clc
        adc #1
        sta varrnd
        dec loopa
        beq :+
        jmp a01415
:
        lda #23
        sta chary
        lda #21
        sta charx
        lda #13
        jsr dmsg
        lda #9
        ldy #12
        cmp (z80_ix),y
        bcs *+5
        jmp a01531
        lda #24
        sta charx
        jmp a01540
a01531: lda #23
        sta charx
a01540: lda #23
        sta chary
        ldy #12
        lda (z80_ix),y
        jsr disply
a01563: lda #0
        cmp varj
        beq *+5
        jmp a03764
        lda #7
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a01726
        lda #0
        cmp vara
        beq *+5
        jmp a01639
        lda #2
        asl a
        sta sndtyp
        lda #0
        jsr animsp
a01639: lda #12
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a01721
        lda #0
        cmp numlif
        bcc *+5
        jmp a01713
        ; lda #0
        dey; ldy #6
        sta (z80_ix),y
        ; lda #0
        iny; ldy #7
        sta (z80_ix),y
        jmp a01721
a01713: lda #1
        sta deadf
a01721: jmp a03736
a01726: jsr skobj
        sta varobj
        lda #255
        cmp varobj
        bne *+5
        jmp a01996
        lda #60
        asl a
        sta sndtyp
        lda varobj
        jsr getob
        lda #0
        cmp varobj
        beq *+5
        jmp a01903
        lda #23
        sta chary
        lda #21
        sta charx
        lda #13
        jsr dmsg
        lda #26
        sta varrnd
        lda #6
        sta loopa
a01834: lda #23
        sta chary
        lda varrnd
        sta charx
        lda #12
        jsr dmsg
        lda varrnd
        clc
        adc #1
        sta varrnd
        dec loopa
        beq :+
        jmp a01834
:
        lda #6
        ldy #11
        sta (z80_ix),y
a01903: lda #23
        sta chary
        lda #21
        sta charx
        lda #13
        jsr dmsg
        lda #9
        ldy #12
        cmp (z80_ix),y
        bcs *+5
        jmp a01964
        lda #24
        sta charx
        jmp a01973
a01964: lda #23
        sta charx
a01973: lda #23
        sta chary
        ldy #12
        lda (z80_ix),y
        jsr disply
a01996: lda #1
        sta varr
        lda joyval
        and #1
        beq :+
        jmp a02169
:
        jsr cangr
        beq :+
        jmp a02084
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #2
        ldy #6
        sta (z80_ix),y
        ; lda #2
        ldy #10
        sta (z80_ix),y
a02084: lda #238
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp a02169
        lda varo
        clc
        adc #1
        sta varo
        lda #129
        cmp varo
        beq *+5
        jmp a02147
        lda #1
        sta varo
a02147: lda #10
        ldy #9
        sta (z80_ix),y
        ; lda #10
        sta varj
a02169: lda joyval
        and #2
        beq :+
        jmp a02333
:
        jsr cangl
        beq :+
        jmp a02249
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #1
        ldy #6
        sta (z80_ix),y
        lda #4
        ldy #10
        sta (z80_ix),y
a02249: lda #0
        ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp a02333
        lda varo
        sec
        sbc #1
        sta varo
        lda #0
        cmp varo
        beq *+5
        jmp a02311
        lda #128
        sta varo
a02311: lda #228
        ldy #9
        sta (z80_ix),y
        lda #10
        sta varj
a02333: lda joyval
        and #8
        beq :+
        jmp a02521
:
        jsr cangu
        beq :+
        jmp a02436
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #3
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        ; lda #0
        sta varr
        lda #1
        ldy #10
        sta (z80_ix),y
a02436: lda #16
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp a02521
        lda varp
        sec
        sbc #1
        sta varp
        lda #0
        cmp varp
        beq *+5
        jmp a02499
        lda #127
        sta varp
a02499: lda #160
        ldy #8
        sta (z80_ix),y
        lda #10
        sta varj
a02521: lda joyval
        and #4
        beq :+
        jmp a02708
:
        jsr cangd
        beq :+
        jmp a02623
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #0
        ldy #6
        sta (z80_ix),y
        lda #2
        iny; ldy #7
        sta (z80_ix),y
        lda #0
        sta varr
        lda #3
        ldy #10
        sta (z80_ix),y
a02623: lda #166
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a02708
        lda varp
        clc
        adc #1
        sta varp
        lda #128
        cmp varp
        beq *+5
        jmp a02686
        lda #1
        sta varp
a02686: lda #26
        ldy #8
        sta (z80_ix),y
        lda #10
        sta varj
a02708: lda joyval
        and #16
        beq :+
        jmp a02808
:
        lda #0
        ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp a02808
        ; lda #0
        cmp vard
        beq *+5
        jmp a02808
        tya; lda #11
        cmp varc
        bcs *+5
        jmp a02808
        lda #0
        cmp varb
        beq *+5
        jmp a02808
        lda #5
        sta varb
a02808: lda #5
        cmp varb
        beq *+5
        jmp a03623
        lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a03024
        jsr cangu
        beq :+
        jmp a03024
:
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #1
        sta z80_c
        lda #4
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #0
        ldy #7
        sta (z80_ix),y
        lda #1
        ldy #10
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #4
        sta varb
a03024: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a03224
        jsr cangr
        beq :+
        jmp a03224
:
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #1
        sta z80_c
        lda #4
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #1
        ldy #7
        sta (z80_ix),y
        lda #2
        ldy #10
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #8
        sta z80_c
        dey; ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #4
        sta varb
a03224: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a03423
        jsr cangd
        beq :+
        jmp a03423
:
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #1
        sta z80_c
        lda #4
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #2
        ldy #7
        sta (z80_ix),y
        lda #3
        ldy #10
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #4
        sta varb
a03423: lda #4
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a03623
        jsr cangl
        beq :+
        jmp a03623
:
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #1
        sta z80_c
        lda #4
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #3
        ldy #7
        sta (z80_ix),y
        lda #4
        ldy #10
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #8
        sta z80_c
        dey; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #4
        sta varb
a03623: lda #4
        cmp varb
        beq *+5
        jmp a03694
        lda #40
        asl a
        sta sndtyp
        ldy #11
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #11
        sta (z80_ix),y
        ; ldy #11
        lda (z80_ix),y
        sta vark
a03694: lda #0
        cmp vara
        beq *+5
        jmp a03736
        lda #1
        cmp varr
        beq *+5
        jmp a03736
        lda #0
        jsr animsp
a03736: ldy #8
        lda (z80_ix),y
        sta varm
        iny; ldy #9
        lda (z80_ix),y
        sta varn
a03764: rts
.out .sprintf("EVENT 0 SIZE = %d", (* - evnt00))

evnt01:
        lda #0
        ldy #12
        sta (z80_ix),y
        lda #2
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00227
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        ldy #6
        lda (z80_ix),y
        sta varrnd
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #9
        cmp varrnd
        bne *+5
        jmp b00227
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #9
        ; ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #<1
        sta z80_l
        lda #>1
        sta z80_h
        jsr addsc
        lda #1
        sta chary
        lda #24
        sta charx
        jsr dscor
        lda #1
        ldy #12
        sta (z80_ix),y
b00227: lda #0
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp b00781
        ; lda #0
        cmp varj
        bcc *+5
        jmp b00280
        lda #1
        ; ldy #12
        sta (z80_ix),y
b00280: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00370
        jsr cangu
        beq :+
        jmp b00356
:
        lda #4
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp b00370
b00356: lda #1
        ldy #12
        sta (z80_ix),y
b00370: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00459
        jsr cangr
        beq :+
        jmp b00445
:
        lda #4
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp b00459
b00445: lda #1
        ldy #12
        sta (z80_ix),y
b00459: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00548
        jsr cangd
        beq :+
        jmp b00535
:
        lda #4
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp b00548
b00535: lda #1
        ldy #12
        sta (z80_ix),y
b00548: lda #4
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00637
        jsr cangl
        beq :+
        jmp b00624
:
        lda #4
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp b00637
b00624: lda #1
        ldy #12
        sta (z80_ix),y
b00637: lda #238
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp b00673
        lda #1
        ldy #12
        sta (z80_ix),y
b00673: lda #0
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp b00709
        lda #1
        ldy #12
        sta (z80_ix),y
b00709: lda #16
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp b00745
        lda #1
        ldy #12
        sta (z80_ix),y
b00745: lda #176
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp b00781
        lda #1
        ldy #12
        sta (z80_ix),y
b00781: lda #1
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp b00847
        lda varc
        sec
        sbc #1
        sta varc
        lda vard
        sec
        sbc #1
        sta vard
        lda #255
        ldy #5
        sta (z80_ix),y
b00847: rts
.out .sprintf("EVENT 1 SIZE = %d", (* - evnt01))

evnt02:
        lda #9
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00945
        lda #3
        iny; ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp c00945
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp c00945
        tya; lda #11
        cmp varc
        bcs *+5
        jmp c00945
        lda #1
        ; ldy #11
        sta (z80_ix),y
        iny; ldy #12
        lda (z80_ix),y
        sta varrnd
        lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c00324
        jsr cangu
        beq :+
        jmp c00324
:
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #5
        sta z80_c
        lda #11
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #0
        ldy #7
        sta (z80_ix),y
        lda #1
        ldy #10
        sta (z80_ix),y
        lda varrnd
        iny; ldy #11
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
c00324: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c00531
        jsr cangr
        beq :+
        jmp c00531
:
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #5
        sta z80_c
        lda #11
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #1
        ldy #7
        sta (z80_ix),y
        lda #2
        ldy #10
        sta (z80_ix),y
        lda varrnd
        iny; ldy #11
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
c00531: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c00738
        jsr cangd
        beq :+
        jmp c00738
:
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #5
        sta z80_c
        lda #11
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #2
        ldy #7
        sta (z80_ix),y
        lda #3
        ldy #10
        sta (z80_ix),y
        lda varrnd
        iny; ldy #11
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
c00738: lda #4
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c00945
        jsr cangl
        beq :+
        jmp c00945
:
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #5
        sta z80_c
        lda #11
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #3
        ldy #7
        sta (z80_ix),y
        lda #4
        ldy #10
        sta (z80_ix),y
        lda varrnd
        iny; ldy #11
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
c00945: lda #0
        cmp varopt
        beq *+5
        jmp c01537
        lda #9
        ldy #6
        cmp (z80_ix),y
        bne *+5
        jmp c01402
        lda #0
        ldy #10
        sta (z80_ix),y
        ldy #8
        lda (z80_ix),y
        cmp varm
        bcc *+5
        jmp c01074
        jsr cangd
        beq :+
        jmp c01069
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #3
        ldy #10
        sta (z80_ix),y
c01069: jmp c01146
c01074: ldy #8
        lda (z80_ix),y
        cmp varm
        bne *+5
        jmp c01146
        jsr cangu
        beq :+
        jmp c01146
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #1
        ldy #10
        sta (z80_ix),y
c01146: ldy #9
        lda (z80_ix),y
        cmp varn
        bcc *+5
        jmp c01274
        jsr cangr
        beq :+
        jmp c01269
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #2
        iny; ldy #10
        sta (z80_ix),y
        lda #5
        ldy #6
        cmp (z80_ix),y
        bne *+5
        jmp c01269
        ; lda #5
        ; ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
c01269: jmp c01397
c01274: ldy #9
        lda (z80_ix),y
        cmp varn
        bne *+5
        jmp c01397
        jsr cangl
        beq :+
        jmp c01397
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #4
        iny; ldy #10
        sta (z80_ix),y
        lda #6
        ldy #6
        cmp (z80_ix),y
        bne *+5
        jmp c01397
        ; lda #6
        ; ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
c01397: jmp c01487
c01402: lda #2
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp c01436
        lda #30
        asl a
        sta sndtyp
c01436: lda #9
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp c01487
        lda #255
        ldy #5
        sta (z80_ix),y
        lda varc
        sec
        sbc #1
        sta varc
c01487: lda #0
        ldy #10
        cmp (z80_ix),y
        bcc *+5
        jmp c01523
        ; lda #0
        jsr animsp
        jmp c01537
c01523: lda #0
        ldy #7
        sta (z80_ix),y
c01537: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c01749
:
        lda #9
        ldy #6
        cmp (z80_ix),y
        bne *+5
        jmp c01749
        lda #60
        asl a
        sta sndtyp
        lda #9
        ; ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        dey; ldy #6
        lda (z80_ix),y
        sta varrnd
        lda #7
        ; ldy #6
        cmp (z80_ix),y
        bne *+5
        jmp c01730
        ; lda #7
        ; ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        lda numlif
        sec
        sbc #1
        sta numlif
        lda #3
        sta varb
c01730: lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
c01749: rts
.out .sprintf("EVENT 2 SIZE = %d", (* - evnt02))

evnt03:
        lda #0
        cmp varopt
        beq *+5
        jmp d00028
        ; lda #0
        jsr animsp
d00028: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00282
:
        lda #60
        asl a
        sta sndtyp
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #93
        ldy #12
        cmp (z80_ix),y
        bcs *+5
        jmp d00139
        lda #6
        sta z80_c
        ; ldy #12
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #12
        sta (z80_ix),y
d00139: ldy #12
        lda (z80_ix),y
        sta vark
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
        lda varc
        sec
        sbc #1
        sta varc
        lda #23
        sta chary
        lda #21
        sta charx
        lda #13
        jsr dmsg
        lda #9
        cmp vark
        bcs *+5
        jmp d00256
        lda #24
        sta charx
        jmp d00264
d00256: lda #23
        sta charx
d00264: lda #23
        sta chary
        lda vark
        jsr disply
d00282: rts
.out .sprintf("EVENT 3 SIZE = %d", (* - evnt03))

evnt04:
        lda #0
        cmp vara
        beq *+5
        jmp e00193
        ; lda #0
        ldy #7
        sta (z80_ix),y
        lda #10
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp e00080
        lda #1
        ldy #7
        sta (z80_ix),y
        lda #2
        asl a
        sta sndtyp
e00080: lda #12
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp e00128
        lda #1
        ldy #7
        sta (z80_ix),y
        lda #2
        asl a
        sta sndtyp
e00128: ldy #12
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #12
        sta (z80_ix),y
        tya; lda #12
        ; ldy #12
        cmp (z80_ix),y
        bcc *+5
        jmp e00193
        lda #0
        ; ldy #12
        sta (z80_ix),y
e00193: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00280
:
        lda #60
        asl a
        sta sndtyp
        lda numlif
        clc
        adc #1
        sta numlif
        lda #3
        sta varb
        lda #255
        ldy #5
        sta (z80_ix),y
        lda varc
        sec
        sbc #1
        sta varc
e00280: rts
.out .sprintf("EVENT 4 SIZE = %d", (* - evnt04))

evnt05:
        lda #0
        cmp vara
        beq *+5
        jmp f00027
        ; lda #0
        jsr animsp
f00027: lda #0
        ldy #12
        sta (z80_ix),y
        ; lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00201
:
        lda #60
        asl a
        sta sndtyp
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #7
        ldy #6
        cmp (z80_ix),y
        bne *+5
        jmp f00187
        ; lda #7
        ; ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        lda numlif
        sec
        sbc #1
        sta numlif
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #3
        sta varb
f00187: lda #1
        ldy #12
        sta (z80_ix),y
f00201: lda #2
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00399
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        ldy #6
        lda (z80_ix),y
        sta varrnd
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #9
        cmp varrnd
        bne *+5
        jmp f00399
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #9
        ; ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #<1
        sta z80_l
        lda #>1
        sta z80_h
        jsr addsc
        lda #1
        sta chary
        lda #24
        sta charx
        jsr dscor
f00399: lda #0
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp f00953
        ; lda #0
        cmp varj
        bcc *+5
        jmp f00453
        lda #1
        ; ldy #12
        sta (z80_ix),y
f00453: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00542
        jsr cangu
        beq :+
        jmp f00528
:
        lda #3
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp f00542
f00528: lda #1
        ldy #12
        sta (z80_ix),y
f00542: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00631
        jsr cangr
        beq :+
        jmp f00618
:
        lda #3
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp f00631
f00618: lda #1
        ldy #12
        sta (z80_ix),y
f00631: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00720
        jsr cangd
        beq :+
        jmp f00707
:
        lda #3
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp f00720
f00707: lda #1
        ldy #12
        sta (z80_ix),y
f00720: lda #4
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00810
        jsr cangl
        beq :+
        jmp f00796
:
        lda #3
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp f00810
f00796: lda #1
        ldy #12
        sta (z80_ix),y
f00810: lda #238
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp f00846
        lda #1
        ldy #12
        sta (z80_ix),y
f00846: lda #0
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp f00881
        lda #1
        ldy #12
        sta (z80_ix),y
f00881: lda #16
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp f00917
        lda #1
        ldy #12
        sta (z80_ix),y
f00917: lda #176
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp f00953
        lda #1
        ldy #12
        sta (z80_ix),y
f00953: lda #1
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp f01004
        lda varc
        sec
        sbc #1
        sta varc
        lda #255
        ldy #5
        sta (z80_ix),y
f01004: rts
.out .sprintf("EVENT 5 SIZE = %d", (* - evnt05))

evnt06:
        rts
.out .sprintf("EVENT 6 SIZE = %d", (* - evnt06))

evnt07:
        rts
.out .sprintf("EVENT 7 SIZE = %d", (* - evnt07))

evnt08:
        rts
.out .sprintf("EVENT 8 SIZE = %d", (* - evnt08))

evnt09:
        lda #0
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00058
        ; lda #0
        ldy #11
        sta (z80_ix),y
        ; lda #0
        iny; ldy #12
        sta (z80_ix),y
        jmp j00389
j00058: lda varc
        clc
        adc #1
        sta varc
        lda #1
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00110
        lda vard
        clc
        adc #1
        sta vard
j00110: lda #2
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00389
        ; lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #1
        cmp varrnd
        beq *+5
        jmp j00215
        lda #255
        ; ldy #5
        sta (z80_ix),y
        lda varc
        sec
        sbc #1
        sta varc
        jmp j00389
j00215: lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        ldy #7
        sta (z80_ix),y
        lda #6
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        clc
        adc #1
        sta varrnd
        lda #6
        cmp varrnd
        beq *+5
        jmp j00335
        tya; lda #7
        sta varrnd
j00335: lda #64
        sta z80_c
        lda varrnd
        clc
        adc z80_c
        sta varrnd
        lda varrnd
        ldy #12
        sta (z80_ix),y
        lda #0
        dey; ldy #11
        sta (z80_ix),y
j00389: rts
.out .sprintf("EVENT 9 SIZE = %d", (* - evnt09))

evnt10:
        lda #1
        cmp scno
        beq *+5
        jmp k00476
        lda #0
        cmp vara
        beq *+5
        jmp k00385
        ; lda #0
        cmp varopt
        beq *+5
        jmp k00385
        ; lda #0
        cmp varo
        beq *+5
        jmp k00178
        lda #1
        sta varo
        lda #32
        sta varc
        lda #3
        sta varr
        lda varc
        sta varq
        lda #18
        sta chary
        lda #1
        sta charx
        lda #33
        jsr dmsg
        lda #19
        sta chary
        lda #1
        sta charx
        lda #34
        jsr dmsg
        lda #20
        sta chary
        lda #5
        sta charx
        lda #35
        jsr dmsg
k00178: lda #12
        sta loopa
k00186: lda varr
        sta chary
        lda #14
        sta charx
        lda #31
        cmp varq
        bcs *+5
        jmp k00234
        lda varq
        jsr dmsg
        jmp k00242
k00234: lda #32
        jsr dmsg
k00242: lda varq
        clc
        adc #1
        sta varq
        lda #43
        cmp varq
        bcc *+5
        jmp k00283
        lda #17
        sta varq
k00283: lda varr
        clc
        adc #1
        sta varr
        lda #14
        cmp varr
        bcc *+5
        jmp k00372
        lda #3
        sta varr
        lda varc
        clc
        adc #1
        sta varc
        lda #43
        cmp varc
        bcc *+5
        jmp k00363
        lda #17
        sta varc
k00363: lda varc
        sta varq
k00372: dec loopa
        beq :+
        jmp k00186
:
k00385: lda #0
        sta varb
        lda joyval
        and #16
        beq :+
        jmp k00476
:
        lda #0
        sta scno
        jsr nwscr
        lda #23
        sta chary
        lda #0
        sta charx
        lda #9
        jsr dmsg
        lda #1
        sta varo
        ; lda #1
        sta varp
        lda #253
        sta varj
        rts
k00476: lda #0
        cmp vara
        beq *+5
        jmp k00513
        lda vara
        clc
        adc #1
        sta vara
        jmp k00521
k00513: lda #0
        sta vara
k00521: lda #3
        cmp varopt
        bcs *+5
        jmp k00559
        lda varopt
        clc
        adc #1
        sta varopt
        jmp k00568
k00559: lda #0
        sta varopt
k00568: lda #0
        cmp varb
        bcc *+5
        jmp k01086
        lda #4
        cmp varb
        beq *+5
        jmp k00707
        lda #6
        sta varrnd
        lda vark
        sta z80_c
        lda varrnd
        sec
        sbc z80_c
        sta varrnd
        lda #26
        sta varq
        lda varrnd
        sta loopa
k00653: lda #23
        sta chary
        lda varq
        sta charx
        lda #12
        jsr dmsg
        lda varq
        clc
        adc #1
        sta varq
        dec loopa
        beq :+
        jmp k00653
:
k00707: lda #3
        cmp varb
        beq *+5
        jmp k00848
        lda #1
        sta chary
        lda numlif
        sta charx
        lda #2
        jsr dmsg
        lda #0
        cmp numlif
        bcc *+5
        jmp k00840
        ; lda #0
        sta varrnd
        lda numlif
        sta loopa
k00785: lda varrnd
        sta charx
        lda #1
        sta chary
        lda #6
        jsr dmsg
        lda varrnd
        clc
        adc #1
        sta varrnd
        dec loopa
        beq :+
        jmp k00785
:
k00840: lda #1
        sta varb
k00848: lda #1
        cmp varb
        beq *+5
        jmp k01078
        lda #0
        cmp varo
        bcc *+5
        jmp k01078
        lda #23
        sta chary
        lda #0
        sta charx
        lda #5
        jsr dmsg
        lda #23
        sta chary
        lda #9
        cmp varo
        bcs *+5
        jmp k00945
        lda #2
        sta charx
        jmp k00983
k00945: lda #99
        cmp varo
        bcs *+5
        jmp k00975
        lda #1
        sta charx
        jmp k00983
k00975: lda #0
        sta charx
k00983: lda varo
        jsr disply
        lda #23
        sta chary
        lda #9
        cmp varp
        bcs *+5
        jmp k01030
        lda #6
        sta charx
        jmp k01069
k01030: lda #99
        cmp varp
        bcs *+5
        jmp k01060
        lda #5
        sta charx
        jmp k01069
k01060: lda #4
        sta charx
k01069: lda varp
        jsr disply
k01078: lda #0
        sta varb
k01086: rts
.out .sprintf("EVENT 10 SIZE = %d", (* - evnt10))

evnt11:
        lda #254
        cmp varj
        beq *+5
        jmp l00106
        lda #3
        sta numlif
        lda #0
        sta vard
        ; lda #0
        sta vare
        lda #4
        sta varopt
        lda #0
        sta varc
        ; lda #0
        sta vark
        ; lda #0
        sta varl
        ; lda #0
        sta varm
        ; lda #0
        sta varn
        lda #10
        sta varj
        rts
l00106: lda #200
        cmp varj
        bcc *+5
        jmp l00126
        rts
l00126: lda #0
        cmp varj
        bcc *+5
        jmp l04591
        lda #9
        cmp varj
        bcc *+5
        jmp l00496
        lda #10
        cmp varj
        beq *+5
        jmp l00451
        lda #0
        cmp vare
        bcc *+5
        jmp l00439
        ; lda #0
        cmp varc
        beq *+5
        jmp l00439
        lda #9
        sta chary
        lda #8
        sta charx
        lda #16
        jsr dmsg
        lda #10
        sta chary
        lda #8
        sta charx
        lda #14
        jsr dmsg
        lda #11
        sta chary
        lda #8
        sta charx
        lda #15
        jsr dmsg
        lda #11
        sta chary
        lda #13
        sta charx
        lda vare
        jsr disply
        lda #12
        sta chary
        lda #8
        sta charx
        lda #16
        jsr dmsg
        lda #40
        jsr delay
        lda vare
        sta loopa
l00353: lda #60
        asl a
        sta sndtyp
        lda #<1
        sta z80_l
        lda #>1
        sta z80_h
        jsr addsc
        lda #1
        sta chary
        lda #24
        sta charx
        jsr dscor
        lda #10
        jsr delay
        dec loopa
        beq :+
        jmp l00353
:
        lda #40
        jsr delay
l00439: lda #0
        sta scno
        jsr nwscr
l00451: lda varj
        sec
        sbc #1
        sta varj
        lda #9
        cmp varj
        beq *+5
        jmp l00491
        lda #1
        sta varb
l00491: jmp l04591
l00496: lda #26
        cmp varm
        beq *+5
        jmp l00636
        lda #2
        sta vare
        lda #13
        sta varf
        lda #6
        sta loopa
l00538: lda vare
        sta chary
        lda varf
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda vare
        sta chary
        lda varf
        sta charx
        lda #1
        jsr dmsg
        lda varf
        clc
        adc #1
        sta varf
        dec loopa
        beq :+
        jmp l00538
:
l00636: lda #160
        cmp varm
        beq *+5
        jmp l00776
        lda #22
        sta vare
        lda #13
        sta varf
        lda #6
        sta loopa
l00678: lda vare
        sta chary
        lda varf
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda vare
        sta chary
        lda varf
        sta charx
        lda #1
        jsr dmsg
        lda varf
        clc
        adc #1
        sta varf
        dec loopa
        beq :+
        jmp l00678
:
l00776: lda #10
        cmp varn
        beq *+5
        jmp l00916
        ; lda #10
        sta vare
        lda #0
        sta varf
        lda #5
        sta loopa
l00818: lda vare
        sta chary
        lda varf
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda vare
        sta chary
        lda varf
        sta charx
        lda #1
        jsr dmsg
        lda vare
        clc
        adc #1
        sta vare
        dec loopa
        beq :+
        jmp l00818
:
l00916: lda #228
        cmp varn
        beq *+5
        jmp l01056
        lda #10
        sta vare
        lda #31
        sta varf
        lda #5
        sta loopa
l00958: lda vare
        sta chary
        lda varf
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda vare
        sta chary
        lda varf
        sta charx
        lda #1
        jsr dmsg
        lda vare
        clc
        adc #1
        sta vare
        dec loopa
        beq :+
        jmp l00958
:
l01056: lda #1
        sta varj
        lda #8
        sta loopa
l01072: lda varo
        sta vark
        lda varp
        sta z80_c
        lda vark
        clc
        adc z80_c
        sta vark
        lda varo
        sta varl
        lda varp
        sta z80_c
        lda varl
        clc
        adc z80_c
        sta varl
        lda varl
        sta z80_d
        lda varj
        sta z80_h
        jsr imul
        lda z80_l
        sta varl
        lda varl
        sta z80_d
        asl a
        clc
        adc z80_d
        sta varl
        lda varl
        sta z80_c
        lda vark
        clc
        adc z80_c
        sta vark
        lda #4
        cmp vark
        bcc *+5
        jmp l01264
        lda vark
        sta z80_d
        lda #3
        sta z80_e
        jsr idiv
        lda z80_d
        sta vark
l01264: lda #4
        cmp vark
        bcc *+5
        jmp l01311
        lda vark
        sta z80_d
        lda #3
        sta z80_e
        jsr idiv
        lda z80_d
        sta vark
l01311: lda #4
        cmp vark
        bcc *+5
        jmp l01358
        lda vark
        sta z80_d
        lda #3
        sta z80_e
        jsr idiv
        lda z80_d
        sta vark
l01358: lda #4
        cmp vark
        bcc *+5
        jmp l01405
        lda vark
        sta z80_d
        lda #3
        sta z80_e
        jsr idiv
        lda z80_d
        sta vark
l01405: lda #4
        cmp vark
        bcc *+5
        jmp l01451
        lda vark
        sta z80_d
        lda #3
        sta z80_e
        jsr idiv
        lda z80_d
        sta vark
l01451: lda #1
        cmp varj
        beq *+5
        jmp l01477
        lda vark
        sta vare
l01477: lda #2
        cmp varj
        beq *+5
        jmp l01502
        lda vark
        sta varf
l01502: lda #3
        cmp varj
        beq *+5
        jmp l01527
        lda vark
        sta varg
l01527: lda #4
        cmp varj
        beq *+5
        jmp l01552
        lda vark
        sta varh
l01552: lda #5
        cmp varj
        beq *+5
        jmp l01578
        lda vark
        sta vari
l01578: lda #6
        cmp varj
        beq *+5
        jmp l01603
        lda vark
        sta varl
l01603: lda #7
        cmp varj
        beq *+5
        jmp l01628
        lda vark
        sta varq
l01628: lda #8
        cmp varj
        beq *+5
        jmp l01653
        lda vark
        sta varr
l01653: lda varj
        clc
        adc #1
        sta varj
        dec loopa
        beq :+
        jmp l01072
:
        lda varo
        sta vark
        lda vark
        sta varj
        lda vark
        lsr a
        lsr a
        lsr a
        sta vark
        lda vark
        asl a
        asl a
        asl a
        sta vark
        lda vark
        sta z80_c
        lda varj
        sec
        sbc z80_c
        sta varj
        lda varj
        sta loopa
l01771: lda vare
        sta vark
        lda varf
        sta vare
        lda varg
        sta varf
        lda varh
        sta varg
        lda vari
        sta varh
        lda varl
        sta vari
        lda varq
        sta varl
        lda varr
        sta varq
        lda vark
        sta varr
        dec loopa
        beq :+
        jmp l01771
:
        lda #1
        sta varj
        lda #8
        sta loopa
l01877: lda #4
        cmp varj
        bcs *+5
        jmp l01906
        lda #9
        sta varm
        jmp l01915
l01906: lda #15
        sta varm
l01915: lda #1
        cmp varj
        beq *+5
        jmp l01948
        lda #6
        sta varn
        lda vare
        sta vark
l01948: lda #2
        cmp varj
        beq *+5
        jmp l01981
        lda #12
        sta varn
        lda varf
        sta vark
l01981: lda #3
        cmp varj
        beq *+5
        jmp l02015
        lda #19
        sta varn
        lda varg
        sta vark
l02015: lda #4
        cmp varj
        beq *+5
        jmp l02048
        lda #25
        sta varn
        lda varh
        sta vark
l02048: lda #5
        cmp varj
        beq *+5
        jmp l02082
        lda #6
        sta varn
        lda vari
        sta vark
l02082: lda #6
        cmp varj
        beq *+5
        jmp l02115
        lda #12
        sta varn
        lda varl
        sta vark
l02115: lda #7
        cmp varj
        beq *+5
        jmp l02149
        lda #19
        sta varn
        lda varq
        sta vark
l02149: lda #8
        cmp varj
        beq *+5
        jmp l02182
        lda #25
        sta varn
        lda varr
        sta vark
l02182: lda #1
        cmp vark
        beq *+5
        jmp l02775
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
        lda varm
        sec
        sbc #1
        sta varm
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
        lda varm
        sec
        sbc #1
        sta varm
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
        lda varm
        sec
        sbc #1
        sta varm
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
        lda varm
        sec
        sbc #1
        sta varm
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
        lda varm
        sec
        sbc #1
        sta varm
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
        lda varm
        sec
        sbc #1
        sta varm
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
l02775: lda #2
        cmp vark
        beq *+5
        jmp l03368
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
        lda varn
        clc
        adc #1
        sta varn
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
        lda varn
        clc
        adc #1
        sta varn
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
        lda varn
        clc
        adc #1
        sta varn
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
        lda varn
        clc
        adc #1
        sta varn
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
        lda varn
        clc
        adc #1
        sta varn
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
        lda varn
        clc
        adc #1
        sta varn
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
l03368: lda #3
        cmp vark
        beq *+5
        jmp l03961
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
        lda varm
        clc
        adc #1
        sta varm
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
        lda varm
        clc
        adc #1
        sta varm
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
        lda varm
        clc
        adc #1
        sta varm
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
        lda varm
        clc
        adc #1
        sta varm
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
        lda varm
        clc
        adc #1
        sta varm
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
        lda varm
        clc
        adc #1
        sta varm
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
l03961: lda #4
        cmp vark
        beq *+5
        jmp l04554
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
        lda varn
        sec
        sbc #1
        sta varn
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
        lda varn
        sec
        sbc #1
        sta varn
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
        lda varn
        sec
        sbc #1
        sta varn
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
        lda varn
        sec
        sbc #1
        sta varn
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
        lda varn
        sec
        sbc #1
        sta varn
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
        lda varn
        sec
        sbc #1
        sta varn
        lda varm
        sta chary
        lda varn
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda varm
        sta chary
        lda varn
        sta charx
        lda #1
        jsr dmsg
l04554: lda varj
        clc
        adc #1
        sta varj
        dec loopa
        beq :+
        jmp l01877
:
        lda #255
        sta varj
l04591: rts
.out .sprintf("EVENT 11 SIZE = %d", (* - evnt11))

evnt12:
        rts
.out .sprintf("EVENT 12 SIZE = %d", (* - evnt12))

evnt13:
        lda #3
        sta numlif
        lda #0
        sta vard
        lda #4
        sta varopt
        lda #0
        sta varc
        ; lda #0
        sta varo
        ; lda #0
        sta varp
        lda #1
        sta varj
        lda #0
        sta vark
        ; lda #0
        sta varl
        ; lda #0
        sta varm
        ; lda #0
        sta varn
        jsr cls
        ; lda #0
        sta chary
        ; lda #0
        sta charx
        lda #10
        jsr dmsg
        lda #1
        sta chary
        lda #0
        sta charx
        lda #9
        jsr dmsg
        lda #1
        sta chary
        lda #9
        sta charx
        lda #3
        jsr dmsg
        lda #23
        sta chary
        lda #0
        sta charx
        lda #9
        jsr dmsg
        lda #1
        sta varb
        lda #23
        sta chary
        lda #6
        sta charx
        lda #4
        jsr dmsg
        rts
.out .sprintf("EVENT 13 SIZE = %d", (* - evnt13))

evnt14:
        lda #0
        sta varc
        ; lda #0
        sta vard
        lda #3
        sta varb
        lda #0
        sta vare
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
        .byte "GAME OVER",141
        .byte "[",141
        .byte "]]",141
        .byte "Lurching Dead",141
        .byte "  2015 retrific.com ]",141
        .byte "] ],] ]",141
        .byte "#",141
        .byte "     PAUSED - Press ANY key    ]",13
        .byte 141
        .byte "00000000",141
        .byte "                               ]",141
        .byte " The Adventures of Bouncing Bob]",13
        .byte 141
        .byte "]",141
        .byte "~",141
        .byte "|{00",141
        .byte " Clearance Bonus]",141
        .byte "     ? x 100    ]",141
        .byte "                ]",141
        .byte "Bob has been    ]",13
        .byte 141
        .byte "working hard to ]",13
        .byte 141
        .byte "expand his garden",13
        .byte 141
        .byte "maze. Sadly, due]",13
        .byte 141
        .byte "to an outbreak of",13
        .byte 141
        .byte "a mutated deadly]",13
        .byte 141
        .byte "pathogen, Bob",39
        .byte "s ]",13
        .byte 141
        .byte "beautiful garden]",13
        .byte 141
        .byte "maze has become ]",13
        .byte 141
        .byte "home to a horde ]",141
        .byte "of zombies. Help]",141
        .byte "Bob to clear his]",141
        .byte "garden maze of  ]",141
        .byte "this inhuman    ]",141
        .byte "infestation.    ]",141
        .byte "                ]",141
        .byte "Q=Up, A=Down, O=Left, P=Right",141
        .byte "SPACE=Fire, R=Reload, H=Pause",141
        .byte "Press SPACE to start",141
nummsg:
        .byte 36
.out .sprintf("MESSAGE SIZE = %d", (* - msgdat))

chgfx:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 33,36,66,88,165,33,80,74,161,161,72,10,36,37,64,10
        .byte 245,245,250,250,245,245,250,250,245,245,250,250,245,245,250,250
        .byte 0,0,200,0,245,0,250,136,245,196,250,200,245,228,250,234
        .byte 0,0,0,50,0,245,16,250,49,245,50,250,117,245,114,250
        .byte 117,245,114,250,49,245,50,250,17,245,0,250,0,49,0,0
        .byte 245,228,250,234,245,196,250,200,245,128,250,0,196,0,0,0
bprop:
        .byte 0
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
.out .sprintf("BLOCKS SIZE = %d", (* - chgfx))

sprgfx:
        .byte 31,248,63,252,63,252,57,156,57,156,63,252,63,252,55,236,55,236,56,28,63,252,31,248,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,31,248,63,252,63,252,57,156,57,156,63,252,63,252,55,236,55,236,56,28,63,252,31,248,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,31,248,63,252,63,252,57,156,57,156,63,252,63,252,55,236,55,236,56,28,63,252,31,248
        .byte 0,0,0,0,31,248,63,252,63,252,57,156,57,156,63,252,63,252,55,236,55,236,56,28,63,252,31,248,0,0,0,0
        .byte 31,248,63,252,63,252,51,252,51,252,63,252,63,252,59,252,59,252,39,252,63,252,31,248,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,31,248,63,252,63,252,51,252,51,252,63,252,63,252,59,252,59,252,39,252,63,252,31,248,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,31,248,63,252,63,252,51,252,51,252,63,252,63,252,59,252,59,252,39,252,63,252,31,248
        .byte 0,0,0,0,31,248,63,252,63,252,51,252,51,252,63,252,63,252,59,252,59,252,39,252,63,252,31,248,0,0,0,0
        .byte 31,248,63,252,63,252,63,204,63,204,63,252,63,252,63,220,63,220,63,228,63,252,31,248,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,31,248,63,252,63,252,63,204,63,204,63,252,63,252,63,220,63,220,63,228,63,252,31,248,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,31,248,63,252,63,252,63,204,63,204,63,252,63,252,63,220,63,220,63,228,63,252,31,248
        .byte 0,0,0,0,31,248,63,252,63,252,63,204,63,204,63,252,63,252,63,220,63,220,63,228,63,252,31,248,0,0,0,0
        .byte 31,248,63,252,63,252,63,252,63,252,63,252,63,252,63,252,63,252,63,252,63,252,31,248,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,31,248,63,252,63,252,63,252,63,252,63,252,63,252,63,252,63,252,63,252,63,252,31,248,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,31,248,63,252,63,252,63,252,63,252,63,252,63,252,63,252,63,252,63,252,63,252,31,248
        .byte 0,0,0,0,31,248,63,252,63,252,63,252,63,252,63,252,63,252,63,252,63,252,63,252,63,252,31,248,0,0,0,0
        .byte 0,0,0,0,0,0,1,128,3,192,3,192,3,192,3,192,3,192,3,192,3,192,3,192,3,192,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,31,240,31,248,31,248,31,240,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,3,192,3,192,3,192,3,192,3,192,3,192,3,192,3,192,3,192,1,128,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,15,248,31,248,31,248,15,248,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 1,128,3,192,7,160,7,224,3,192,1,128,3,254,3,248,3,0,3,192,0,0,3,192,1,128,1,128,1,128,1,192
        .byte 0,192,1,224,3,208,3,240,1,224,0,192,1,224,3,254,3,24,7,128,1,128,6,0,15,128,28,192,24,192,12,96
        .byte 1,128,3,192,5,224,7,224,3,192,1,128,127,192,31,192,0,192,3,192,0,0,3,192,1,128,1,128,1,128,3,128
        .byte 3,0,7,128,11,192,15,192,7,128,3,0,7,128,127,192,24,192,1,224,1,128,0,96,1,240,3,56,3,24,6,48
        .byte 31,248,63,252,63,252,57,156,57,156,63,252,63,252,56,28,55,236,63,252,63,252,31,248,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,31,248,63,252,63,252,57,156,57,156,63,252,63,252,56,28,55,236,63,252,63,252,31,248,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,31,248,63,252,63,252,57,156,57,156,63,252,63,252,56,28,55,236,63,252,63,252,31,248
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,31,248,63,252,63,252,57,156,57,156,63,252,63,252,56,28,55,236,63,252
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,31,248,63,252,63,252,57,156,57,156,63,252,63,252,56,28,55,236
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,31,248,63,252,63,252,57,156,57,156,63,252,63,252,120,30
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,31,248,63,252,63,252,57,156,57,156,63,252,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,31,248,63,252,63,252,57,156,57,156,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,31,248,63,252,63,252,57,156,249,159
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,31,248,63,252,63,252,249,159
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,31,248,63,252,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,31,248,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,255
        .byte 0,0,28,112,58,248,125,244,127,232,127,244,127,232,127,244,127,232,63,240,63,232,31,208,11,160,5,64,2,128,0,0
        .byte 60,112,122,248,125,244,255,250,255,252,255,250,255,252,255,250,255,244,127,232,127,244,63,232,31,208,11,160,5,64,2,128
        .byte 3,128,7,192,13,96,15,224,6,192,3,128,15,224,31,240,31,240,23,208,16,16,22,208,6,192,6,192,6,192,14,224
        .byte 7,192,15,224,29,112,31,240,14,224,7,192,15,224,31,240,31,240,23,208,16,16,22,208,6,192,6,192,6,192,14,224
        .byte 15,224,31,240,59,184,63,248,28,112,15,224,15,224,31,240,31,240,23,208,16,16,22,208,6,192,6,192,6,192,14,224
        .byte 17,16,9,32,0,0,88,52,0,0,8,32,16,16,3,128,15,224,31,240,23,208,23,208,22,208,6,192,6,192,14,224
        .byte 17,16,0,0,0,0,176,26,0,0,0,0,0,0,32,8,3,128,15,224,31,240,23,208,23,208,22,208,6,192,14,224
        .byte 0,0,0,0,0,0,192,6,0,0,0,0,0,0,0,0,64,4,3,128,15,224,31,240,23,208,23,208,22,208,14,224
        .byte 0,0,0,0,0,0,128,2,0,0,0,0,0,0,0,0,0,0,128,2,3,128,15,224,31,240,23,208,23,208,30,240
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,128,15,224,31,240,23,208,63,248
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,128,15,224,31,240,127,252
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,15,224,63,248,255,254
        .byte 255,255,128,1,128,1,140,49,146,49,146,253,146,253,146,49,146,49,146,1,158,1,146,1,158,1,128,1,128,1,255,255
        .byte 255,255,128,1,128,1,140,1,146,1,146,1,146,1,146,1,146,1,146,1,158,1,146,1,158,1,128,1,128,1,255,255
        .byte 56,252,71,2,144,41,132,129,64,1,64,18,38,42,47,18,79,130,159,129,142,1,128,5,72,17,64,2,33,140,30,112
        .byte 48,28,72,226,135,10,144,34,132,130,64,1,64,17,33,41,19,145,39,194,71,202,79,130,134,9,144,33,64,82,63,140
        .byte 120,112,134,136,145,4,132,82,129,1,64,1,68,33,142,82,158,36,159,4,142,2,64,2,72,41,32,1,16,226,15,28
        .byte 30,60,33,194,64,9,74,65,128,2,144,4,128,72,68,168,78,72,79,4,159,2,158,1,128,41,144,1,67,2,60,252
        .byte 128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1
frmlst:
        .byte 0,4
        .byte 4,4
        .byte 8,4
        .byte 12,4
        .byte 16,4
        .byte 20,2
        .byte 22,2
        .byte 24,13
        .byte 37,2
        .byte 39,10
        .byte 49,2
        .byte 51,4
        .byte 55,1,56,0
.out .sprintf("SPRITES SIZE = %d", (* - sprgfx))

scdat:
        .word 89,124
        .byte 255,1,13,255,0,6,255,1,14,255,0,30,1,1,255,0,30,1,1,255,0,30,1,1,255,0,30,1,1,255,0,30,1,1
        .byte 255,0,30,1,1,255,0,30,1,255,0,160,1,255,0,30,1,1,255,0,30,1,1,255,0,30,1,1,255,0,30,1,1
        .byte 255,0,30,1,1,255,0,30,1,1,255,0,30,255,1,14,255,0,6,255,1,13
        .byte 255,0,33,4,255,2,10,3,255,0,20,255,2,12,255,0,20,255,2,12,255,0,20,2,2,2,0,0,2,2,0,0,2,2,2
        .byte 255,0,20,2,2,2,0,0,2,2,0,0,2,2,2,255,0,20,255,2,12,255,0,20,255,2,12,255,0,20,2,2,0
        .byte 255,2,6,0,2,2,255,0,20,2,2,0,255,2,6,0,2,2,255,0,20,2,2,2,255,0,6,2,2,2,255,0,20
        .byte 255,2,12,255,0,20,5,255,2,10,6,255,0,51,255,1,32,255,0,160,255,1,32
numsc:
        .byte 2
.out .sprintf("SCREENS SIZE = %d", (* - scdat))

nmedat:
        .byte 0,0,88,120,2,5,32,24,2,5,32,64,2,6,32,216,2,6,32,176,2,6,88,176,2,5,152,24,2,5,152,64,2,6,152,176,2,6,152,216,255
        .byte 0,12,48,32,255
.out .sprintf("POSITIONS SIZE = %d", (* - nmedat))

NUMOBJ = 1
objdta:
        .byte 0,0,0,0,0,0,8,2,63,255,0,1,253,255,253,64,27,128,48,0,96,0,252,0,252,0,0,0,0,0,0,0,0,104,120,0,104,120
.out .sprintf("OBJECTS SIZE = %d", (* - objdta))

font:
        .byte 0,0,0,0,0,0,0,0
        .byte 0,16,16,16,16,0,16,0
        .byte 0,40,40,0,0,0,0,0
        .byte 62,127,107,127,93,99,62,0
        .byte 0,8,62,40,62,10,62,8
        .byte 0,97,98,4,8,16,35,67
        .byte 0,48,72,48,58,76,54,0
        .byte 0,16,32,0,0,0,0,0
        .byte 0,8,16,16,16,16,8,0
        .byte 0,32,16,16,16,16,32,0
        .byte 0,0,36,24,126,24,36,0
        .byte 0,0,8,8,62,8,8,0
        .byte 0,0,0,0,0,16,16,32
        .byte 0,0,0,0,60,0,0,0
        .byte 0,0,0,0,0,48,48,0
        .byte 0,0,2,4,8,16,32,0
        .byte 0,60,70,74,82,98,60,0
        .byte 0,8,24,8,8,8,62,0
        .byte 0,60,66,2,60,64,126,0
        .byte 0,60,66,12,2,66,60,0
        .byte 0,8,24,40,72,126,8,0
        .byte 0,126,64,124,2,66,60,0
        .byte 0,60,64,124,66,66,60,0
        .byte 0,126,2,4,8,16,16,0
        .byte 0,60,66,60,66,66,60,0
        .byte 0,60,66,66,62,2,60,0
        .byte 0,0,16,0,0,16,0,0
        .byte 0,0,16,0,0,16,16,32
        .byte 0,0,4,8,16,8,4,0
        .byte 0,0,0,60,0,60,0,0
        .byte 0,0,32,16,8,16,32,0
        .byte 0,60,66,4,8,0,8,0
        .byte 0,60,66,90,94,64,60,0
        .byte 0,124,130,130,254,130,130,0
        .byte 0,252,130,252,130,130,252,0
        .byte 0,124,130,128,128,130,124,0
        .byte 0,248,132,130,130,132,248,0
        .byte 0,254,128,248,128,128,254,0
        .byte 0,254,128,248,128,128,128,0
        .byte 0,124,130,128,142,130,124,0
        .byte 0,130,130,254,130,130,130,0
        .byte 0,124,16,16,16,16,124,0
        .byte 0,2,2,2,130,130,124,0
        .byte 0,132,136,240,136,132,130,0
        .byte 0,128,128,128,128,128,254,0
        .byte 0,198,170,146,130,130,130,0
        .byte 0,194,162,146,138,134,130,0
        .byte 0,124,130,130,130,130,124,0
        .byte 0,252,130,130,252,128,128,0
        .byte 0,124,130,130,138,132,122,0
        .byte 0,252,130,130,252,132,130,0
        .byte 0,124,128,124,2,130,124,0
        .byte 0,254,16,16,16,16,16,0
        .byte 0,130,130,130,130,130,124,0
        .byte 0,130,130,130,68,40,16,0
        .byte 0,130,130,130,146,146,108,0
        .byte 0,66,36,24,24,36,66,0
        .byte 0,130,68,56,16,16,16,0
        .byte 0,254,4,24,48,64,254,0
        .byte 54,109,243,94,187,202,39,74
        .byte 0,0,64,32,16,8,4,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,16,56,124,16,16,16,0
        .byte 0,0,0,0,0,0,0,255
        .byte 0,28,34,120,32,32,126,0
        .byte 0,0,60,2,62,66,62,0
        .byte 0,64,64,124,66,66,124,0
        .byte 0,0,60,64,64,64,60,0
        .byte 0,2,2,62,66,66,62,0
        .byte 0,0,60,66,124,64,62,0
        .byte 0,28,32,56,32,32,32,0
        .byte 0,0,62,66,66,62,2,60
        .byte 0,64,64,124,66,66,66,0
        .byte 0,16,0,48,16,16,56,0
        .byte 0,4,0,4,4,4,68,56
        .byte 0,64,64,72,112,72,68,0
        .byte 0,32,32,32,32,32,28,0
        .byte 0,0,236,146,146,146,146,0
        .byte 0,0,124,66,66,66,66,0
        .byte 0,0,60,66,66,66,60,0
        .byte 0,0,124,66,66,124,64,64
        .byte 0,0,62,66,66,62,2,3
        .byte 0,0,62,64,64,64,64,0
        .byte 0,0,60,64,60,2,124,0
        .byte 0,32,112,32,32,32,28,0
        .byte 0,0,66,66,66,66,60,0
        .byte 0,0,68,68,40,40,16,0
        .byte 0,0,130,130,146,146,108,0
        .byte 0,0,66,36,24,36,66,0
        .byte 0,0,66,66,66,62,2,60
        .byte 0,0,126,4,24,32,126,0
        .byte 0,0,0,0,36,24,36,0
        .byte 0,64,164,174,164,160,224,0
        .byte 0,96,16,12,16,16,96,0
        .byte 0,24,60,126,126,60,24,0
        .byte 124,130,186,162,162,186,130,124
.out .sprintf("FONT SIZE = %d", (* - font))

jtab:
        .byte 99
keys:   .byte 66,97,104,72,73,16,55,48,49,17,18
.out .sprintf("KEYS SIZE = %d", (* - keys))

data_end:
.out .sprintf("TOTAL DATA SIZE = %d", (* - data_start))
