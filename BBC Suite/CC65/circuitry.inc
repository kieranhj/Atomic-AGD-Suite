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
WINDOWHGT = 22
WINDOWWID = 22 ;面面面面面面面面面面面面
MAPWID = 13
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255
mapdat:
        .byte 255,0,1,2,3,4,5,6,7,8,9,10,255,255,11,12,13,14,15,16,17,18,19,20,21,255
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255
stmap:  .byte 24

evnt00:
        jsr ifall
        jsr skobj
        sta varobj
        lda #255
        cmp varobj
        bne *+5
        jmp a00123
        lda varobj
        jsr getob
        lda #3
        sta z80_c
        lda numlif
        clc
        adc z80_c
        sta numlif
        lda #6
        sta chary
        lda #24
        sta charx
        lda numlif
        jsr disply
        lda #30
        jsr dmsg
        lda #95
        asl a
        sta sndtyp
        lda #20
        jsr delay
a00123: lda #0
        cmp vark
        beq *+5
        jmp a00369
        lda joyval
        and #1
        beq :+
        jmp a00243
:
        jsr cangr
        beq :+
        jmp a00243
:
        lda #0
        ldy #6
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #2
        cmp vara
        beq *+5
        jmp a00243
        lda #0
        jsr animsp
        lda #7
        asl a
        sta sndtyp
a00243: lda joyval
        and #2
        beq :+
        jmp a00347
:
        jsr cangl
        beq :+
        jmp a00347
:
        lda #1
        ldy #6
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #2
        cmp vara
        beq *+5
        jmp a00347
        lda #0
        jsr animsp
        lda #7
        asl a
        sta sndtyp
a00347: lda joyval
        and #16
        beq :+
        jmp a00369
:
        jsr jump
a00369: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a00417
:
        lda #14
        ldy #6
        sta (z80_ix),y
        lda #1
        sta vark
a00417: lda #167
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a00461
        lda #14
        ldy #6
        sta (z80_ix),y
        lda #1
        sta vark
a00461: lda #1
        cmp vark
        beq *+5
        jmp a00516
        lda #2
        cmp vara
        beq *+5
        jmp a00516
        lda #0
        jsr animsp
        lda #50
        asl a
        sta sndtyp
a00516: jmp grav
.out .sprintf("EVENT 0 SIZE = %d", (* - evnt00))

evnt01:
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00167
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
        jmp b00113
:
        lda #15
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp b00162
b00113: lda #16
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
b00162: jmp b00225
b00167: jsr cangd
        beq :+
        jmp b00211
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
        jmp b00225
b00211: lda #0
        ldy #10
        sta (z80_ix),y
b00225: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00476
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
        jmp b00476
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
        jmp b00436
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
b00436: lda joyval
        and #16
        beq :+
        jmp b00458
:
        jsr jump
b00458: lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
b00476: lda #2
        cmp vara
        beq *+5
        jmp b00502
        lda #0
        jsr animsp
b00502: jmp grav
.out .sprintf("EVENT 1 SIZE = %d", (* - evnt01))

evnt02:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp c00094
        jsr cangu
        beq :+
        jmp c00064
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp c00090
c00064: lda #1
        ldy #11
        sta (z80_ix),y
        lda #5
        asl a
        sta sndtyp
c00090: jmp c00160
c00094: jsr cangd
        beq :+
        jmp c00135
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp c00160
c00135: lda #0
        ldy #11
        sta (z80_ix),y
        lda #3
        asl a
        sta sndtyp
c00160: lda #2
        cmp vara
        beq *+5
        jmp c00185
        lda #0
        jsr animsp
c00185: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00266
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #14
        ldy #6
        sta (z80_ix),y
        lda #1
        sta vark
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
c00266: rts
.out .sprintf("EVENT 2 SIZE = %d", (* - evnt02))

evnt03:
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00119
:
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #<16
        sta z80_l
        lda #>16
        sta z80_h
        jsr addsc
        lda #3
        sta chary
        lda #24
        sta charx
        jsr dscor
        lda #70
        asl a
        sta sndtyp
        lda #5
        jsr delay
        lda varc
        clc
        adc #1
        sta varc
        rts
d00119: lda #0
        cmp vara
        beq *+5
        jmp d00136
d00136: lda #1
        cmp vara
        beq *+5
        jmp d00153
d00153: lda #2
        cmp vara
        beq *+5
        jmp d00169
d00169: rts
.out .sprintf("EVENT 3 SIZE = %d", (* - evnt03))

evnt04:
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00229
:
        lda #2
        cmp varc
        beq *+5
        jmp e00229
        lda #70
        asl a
        sta sndtyp
        lda #5
        jsr delay
        lda #90
        asl a
        sta sndtyp
        lda #5
        jsr delay
        lda #40
        asl a
        sta sndtyp
        lda #5
        jsr delay
        lda #1
        sta nexlev
        lda #102
        asl a
        sta sndtyp
        lda #0
        sta varc
        lda varm
        clc
        adc #1
        sta varm
        lda #17
        cmp varm
        beq *+5
        jmp e00171
        lda #22
        sta varm
e00171: lda #30
        cmp varm
        beq *+5
        jmp e00204
        lda #31
        sta varm
        lda #0
        sta varf
e00204: lda #0
        sta chary
        lda #1
        sta charx
        lda varm
        jsr dmsg
e00229: rts
.out .sprintf("EVENT 4 SIZE = %d", (* - evnt04))

evnt05:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp f00166
        jsr cangl
        beq :+
        jmp f00136
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
        lda #16
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp f00162
f00136: lda #8
        asl a
        sta sndtyp
        lda #1
        ldy #11
        sta (z80_ix),y
f00162: jmp f00304
f00166: jsr cangr
        beq :+
        jmp f00279
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
        lda #16
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp f00304
f00279: lda #8
        asl a
        sta sndtyp
        lda #0
        ldy #11
        sta (z80_ix),y
f00304: lda #2
        cmp vara
        beq *+5
        jmp f00329
        lda #0
        jsr animsp
f00329: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00410
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #14
        ldy #6
        sta (z80_ix),y
        lda #1
        sta vark
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
f00410: rts
.out .sprintf("EVENT 5 SIZE = %d", (* - evnt05))

evnt06:
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00193
:
        lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp g00108
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #14
        dey; ldy #6
        sta (z80_ix),y
        lda #1
        sta vark
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        rts
g00108: lda #1
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp g00193
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #14
        dey; ldy #6
        sta (z80_ix),y
        lda #1
        sta vark
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        rts
g00193: lda #100
        cmp varb
        bcc *+5
        jmp g00231
        lda #0
        jsr animsp
        lda #75
        asl a
        sta sndtyp
g00231: lda varb
        clc
        adc #1
        sta varb
        lda #200
        cmp varb
        beq *+5
        jmp g00271
        lda #0
        sta varb
g00271: lda #100
        cmp varb
        bcs *+5
        jmp g00302
        lda #2
        ldy #7
        sta (z80_ix),y
g00302: rts
.out .sprintf("EVENT 6 SIZE = %d", (* - evnt06))

evnt07:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp h00082
        jsr cangl
        beq :+
        jmp h00064
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp h00078
h00064: lda #1
        ldy #11
        sta (z80_ix),y
h00078: jmp h00136
h00082: jsr cangr
        beq :+
        jmp h00123
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp h00136
h00123: lda #0
        ldy #11
        sta (z80_ix),y
h00136: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp h00217
        jsr cangu
        beq :+
        jmp h00199
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp h00212
h00199: lda #1
        ldy #10
        sta (z80_ix),y
h00212: jmp h00271
h00217: jsr cangd
        beq :+
        jmp h00257
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp h00271
h00257: lda #0
        ldy #10
        sta (z80_ix),y
h00271: lda #2
        cmp vara
        beq *+5
        jmp h00296
        lda #0
        jsr animsp
h00296: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp h00376
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #14
        ldy #6
        sta (z80_ix),y
        lda #1
        sta vark
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
h00376: rts
.out .sprintf("EVENT 7 SIZE = %d", (* - evnt07))

evnt08:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp i00194
        jsr cangl
        beq :+
        jmp i00176
:
        lda #1
        ldy #6
        sta (z80_ix),y
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
        jmp i00136
:
        lda #1
        ldy #11
        sta (z80_ix),y
i00136: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp i00189
i00176: lda #1
        ldy #11
        sta (z80_ix),y
i00189: jmp i00360
i00194: jsr cangr
        beq :+
        jmp i00346
:
        lda #0
        ldy #6
        sta (z80_ix),y
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
        jmp i00306
:
        lda #0
        ldy #11
        sta (z80_ix),y
i00306: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp i00360
i00346: lda #0
        ldy #11
        sta (z80_ix),y
i00360: lda #2
        cmp vara
        beq *+5
        jmp i00385
        lda #0
        jsr animsp
i00385: rts
.out .sprintf("EVENT 8 SIZE = %d", (* - evnt08))

evnt09:
        rts
.out .sprintf("EVENT 9 SIZE = %d", (* - evnt09))

evnt10:
        lda vara
        clc
        adc #1
        sta vara
        lda #3
        cmp vara
        beq *+5
        jmp k00042
        lda #0
        sta vara
k00042: lda vars
        clc
        adc #1
        sta vars
        lda #60
        cmp vars
        beq *+5
        jmp k00082
        lda #20
        sta vars
k00082: lda #0
        cmp varf
        beq *+5
        jmp k00240
        lda #20
        cmp scno
        beq *+5
        jmp k00232
        lda #6
        sta chary
        lda #2
        sta charx
        lda #32
        jsr dmsg
        lda #8
        sta chary
        lda #1
        sta charx
        lda #33
        jsr dmsg
        lda #9
        sta chary
        lda #1
        sta charx
        lda #34
        jsr dmsg
        lda #1
        sta charx
        lda #35
        jsr dmsg
        lda #13
        sta chary
        lda #7
        sta charx
        lda #36
        jsr dmsg
k00232: lda #1
        sta varf
k00240: lda #20
        cmp scno
        beq *+5
        jmp k00385
        lda vars
        sta varr
        lda #20
        sta z80_c
        lda varr
        clc
        adc z80_c
        sta varr
        lda #5
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        sta z80_c
        lda varr
        clc
        adc z80_c
        sta varr
        lda varr
        asl a
        sta sndtyp
        lda joyval
        and #64
        beq :+
        jmp k00385
:
        lda #1
        sta gamwon
k00385: lda #21
        cmp scno
        beq *+5
        jmp k00746
        lda #0
        cmp varn
        beq *+5
        jmp k00746
        lda #2
        sta chary
        lda #8
        sta charx
        lda #39
        jsr dmsg
        lda #4
        sta chary
        lda #2
        sta charx
        lda #37
        jsr dmsg
        lda #4
        sta chary
        lda #18
        sta charx
        lda #38
        jsr dmsg
        lda #20
        sta chary
        lda #5
        sta charx
        lda #44
        jsr dmsg
        lda #21
        sta chary
        lda #5
        sta charx
        lda #2
        jsr dmsg
        lda #14
        sta chary
        lda #7
        sta charx
        lda #40
        jsr dmsg
        lda #15
        sta chary
        lda #7
        sta charx
        lda #41
        jsr dmsg
        lda #16
        sta chary
        lda #7
        sta charx
        lda #42
        jsr dmsg
        lda #0
        sta chary
        lda #1
        sta charx
        lda #43
        jsr dmsg
        lda #18
        sta chary
        lda #9
        sta charx
        lda #36
        jsr dmsg
        lda #20
        sta chary
        lda #16
        sta charx
        lda #45
        jsr dmsg
        lda #21
        sta chary
        lda #15
        sta charx
        lda #46
        jsr dmsg
        lda #100
        asl a
        sta sndtyp
        lda varn
        clc
        adc #1
        sta varn
k00746: rts
.out .sprintf("EVENT 10 SIZE = %d", (* - evnt10))

evnt11:
        lda #21
        cmp scno
        beq *+5
        jmp l00078
        lda joyval
        and #64
        beq :+
        jmp l00078
:
        lda #5
        sta varm
        lda #0
        sta scno
        jsr nwscr
        lda #1
        sta restfl
        lda #115
        asl a
        sta sndtyp
l00078: lda #1
        cmp vark
        beq *+5
        jmp l00143
        lda #2
        cmp vara
        beq *+5
        jmp l00143
        lda #9
        cmp varj
        bcs *+5
        jmp l00143
        lda varj
        clc
        adc #1
        sta varj
l00143: lda #10
        cmp varj
        beq *+5
        jmp l00185
        lda #0
        sta varj
        ; lda #0
        sta vark
        lda #1
        sta deadf
l00185: rts
.out .sprintf("EVENT 11 SIZE = %d", (* - evnt11))

evnt12:
        lda #120
        jsr delay
        jsr cls
        lda #1
        sta chary
        lda #0
        sta charx
        lda #17
        jsr dmsg
        lda #7
        sta chary
        lda #0
        sta charx
        lda #18
        jsr dmsg
        lda #9
        sta chary
        lda #0
        sta charx
        lda #1
        jsr dmsg
        lda #100
        asl a
        sta sndtyp
        jsr prskey
        rts
.out .sprintf("EVENT 12 SIZE = %d", (* - evnt12))

evnt13:
        lda #10
        sta numlif
        lda #31
        sta varm
        lda #0
        sta varf
        ; lda #0
        sta vars
        ; lda #0
        sta varn
        ; lda #0
        sta vark
        jsr cls
        lda #23
        sta chary
        lda #0
        sta charx
        lda #19
        jsr dmsg
        lda #0
        sta chary
        ; lda #0
        sta charx
        lda #20
        jsr dmsg
        lda #1
        sta chary
        lda #0
        sta charx
        lda #22
        sta loopa
n00131: lda #21
        jsr dmsg
        dec loopa
        beq :+
        jmp n00131
:
        lda #0
        sta chary
        lda #1
        sta charx
        lda varm
        jsr dmsg
        lda #2
        sta chary
        lda #24
        sta charx
        lda #3
        jsr dmsg
        lda #3
        sta chary
        lda #24
        sta charx
        jsr dscor
        lda #3
        sta chary
        lda #30
        sta charx
        lda #4
        jsr dmsg
        lda #5
        sta chary
        lda #24
        sta charx
        lda #2
        jsr dmsg
        lda #0
        sta varc
        rts
.out .sprintf("EVENT 13 SIZE = %d", (* - evnt13))

evnt14:
        lda #0
        sta vara
        lda #6
        sta chary
        lda #24
        sta charx
        lda numlif
        jsr disply
        lda #30
        jsr dmsg
        rts
.out .sprintf("EVENT 14 SIZE = %d", (* - evnt14))

evnt15:
        rts
.out .sprintf("EVENT 15 SIZE = %d", (* - evnt15))

evnt16:
        lda #150
        asl a
        sta sndtyp
        lda numlif
        sec
        sbc #1
        sta numlif
        lda #0
        sta varc
        rts
.out .sprintf("EVENT 16 SIZE = %d", (* - evnt16))

evnt17:
        lda #50
        jsr delay
        rts
.out .sprintf("EVENT 17 SIZE = %d", (* - evnt17))

evnt18:
        lda #102
        asl a
        sta sndtyp
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
        .byte "ENDqOFqLINE..",141
        .byte "qqqqqHUMANqINTERFACEqDEVICE",13
        .byte 13
        .byte "qqqqqqqqqq1q:qKEYBOARD",13
        .byte "qqqqqqqqqq2q:qKEMPSTON",13
        .byte "qqqqqqqqqq3q:qSINCLAIR",13
        .byte 13
        .byte "qqqqqqqqqqqCTRLqKEYS",13
        .byte "qqZq:qLEFTqqXq:qRIGHTqqKq:qJUMP",13
        .byte 13
        .byte "qlmnnnnnnqVERSIONq1.1qnnnnnnnop",13
        .byte 13
        .byte "qqqqqq2017qRUCKSACKqGAMES",13
        .byte "qqqqqCODEDqANDqDESIGNEDqBY:",13
        .byte "qqqqqqqqqqqJOHNqBLYTHE",141
        .byte "CYCLES",141
        .byte "CODE:",141
        .byte "k",141
        .byte "IT",39
        .byte "SqTERMINALqq",141
        .byte "SEVEREDqDREAMSqq",141
        .byte "ANYqPORTqINqA..",141
        .byte "SIRqKITqBOREDqq",141
        .byte "REZqSISTER,qHI!",141
        .byte "CONTROLqFREAK!q",141
        .byte "CHIPSqAREqDOWNq",141
        .byte "BADqMEMORIESqqq",141
        .byte "CTRL+ALT+DELqqq",141
        .byte "RAMqRAIDERSqqqq",141
        .byte "ADAPTORqORqDIEq",141
        .byte "GATEqOFqXORqqqq",141
        .byte "jihqaqjihqjihqaqgqgqbceqjihqaqg",13
        .byte "aqqqaqaqgqaqqqaqgqgqqaqqaqgqaqg",13
        .byte "aqqqaqaceqaqqqaqgqfqqaqqaceqbce",13
        .byte "aqqqaqaqhqaqqqaqgqgqqaqqaqhqqqh",13
        .byte "bdeqaqaqgqbdeqbceqgqqaqqaqgqbde",141
        .byte "qlmnnnnnqPOWEREDqBYqAGDqnnnnnop",141
        .byte "bccdccdcccdcccCIRCUITRYe",141
        .byte "jqqqqqqqqqqqqqqqiccdicih",141
        .byte "aqqqqqqqqqqqqqqqqqqqqqqg",141
        .byte "DIEqODEqqqqqqqq",141
        .byte "SERIALqKILLERqq",141
        .byte "SAVEqMEqqqqqqqq",141
        .byte "SURFqBAUDq9600q",141
        .byte "THEqJOYqOFqHEXq",141
        .byte "PHISHqNqCHIPSqq",141
        .byte "LOAD",39
        .byte 39
        .byte "qRUNNERqq",141
        .byte "PRESSqESCAPEqqq",141
        .byte "q",141
        .byte "THEqVERYqEND...",141
        .byte "WELLqDONEqNAN",39
        .byte "O",39
        .byte "BOT",141
        .byte "qYOU",39
        .byte "VEqSAVEDqALLqTHE",13
        .byte 141
        .byte "qqqDATA.qMANKINDqCAN",141
        .byte "qqqSLEEPqSAFEqAGAIN.",141
        .byte "PRESSqSPACE",141
        .byte "ENEMY",141
        .byte "DATA",141
        .byte "NAN",39
        .byte "O",39
        .byte "BOT",141
        .byte "RESCUEqTHEqDATA",141
        .byte "GETqTOqTHEqEXIT",141
        .byte "SAVEqHUMANITY..",141
        .byte "GETqREADY...qqq",141
        .byte "MORE",141
        .byte "THE",141
        .byte "EXIT",141
nummsg:
        .byte 47
.out .sprintf("MESSAGE SIZE = %d", (* - msgdat))

chgfx:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 228,228,216,216,228,228,216,216,228,228,216,216,228,228,216,216
        .byte 45,165,15,75,165,45,15,75,45,15,75,75,135,165,30,15
        .byte 165,135,15,15,165,45,30,75,45,15,15,75,165,45,75,15
        .byte 165,135,15,15,165,165,15,15,45,45,15,75,45,15,30,15
        .byte 15,15,75,15,15,45,15,75,15,15,15,15,15,45,15,15
        .byte 15,135,30,15,15,135,30,15,45,15,15,75,15,135,15,15
        .byte 15,45,75,15,45,135,90,15,15,15,90,75,135,45,75,15
        .byte 129,32,24,2,129,32,24,2,129,32,24,2,129,32,24,2
        .byte 4,129,2,64,1,32,0,24,0,4,0,2,0,1,0,0
        .byte 129,32,2,64,4,128,24,0,32,0,64,0,128,0,0,0
        .byte 0,0,90,90,0,0,0,0,165,165,0,0,0,0,90,90
        .byte 112,0,240,176,240,176,240,176,112,0,0,16,160,160,80,0
        .byte 0,224,176,240,176,240,176,240,16,224,32,0,144,80,0,160
        .byte 0,0,0,0,0,17,0,187,0,187,0,17,0,0,0,0
        .byte 0,0,0,0,136,0,221,0,221,0,136,0,0,0,0,0
        .byte 255,95,239,175,207,15,207,15,143,15,79,15,143,15,79,15
        .byte 79,15,143,15,79,15,143,15,79,15,143,15,79,15,143,15
        .byte 79,15,15,15,79,15,15,15,15,15,15,15,15,15,15,15
        .byte 15,15,175,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 15,13,15,14,15,13,15,14,15,13,15,14,15,13,15,14
        .byte 0,0,250,0,245,0,0,0,0,0,250,0,245,0,0,0
        .byte 0,0,0,250,0,245,0,0,0,0,0,250,0,245,0,0
        .byte 0,0,8,0,4,0,2,0,129,0,64,8,32,4,24,2
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,10,10,5,5
        .byte 95,95,175,175,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 100,100,98,98,100,100,98,98,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,100,100,98,98,100,100,98,98
        .byte 4,129,10,64,5,32,2,24,129,4,64,10,32,5,24,2
        .byte 129,32,24,2,129,32,8,10,128,129,64,8,1,0,0,0
        .byte 87,2,175,137,95,10,175,137,87,2,0,0,87,10,0,0
        .byte 87,10,0,0,87,68,39,34,95,25,175,170,95,25,39,34
        .byte 0,0,0,119,17,153,51,0,34,51,68,119,68,255,68,255
        .byte 0,0,238,0,153,136,0,204,204,68,238,34,255,34,221,34
        .byte 10,2,5,0,10,2,4,4,0,8,137,4,10,0,0,0
        .byte 68,255,68,255,68,102,34,51,51,0,17,153,0,85,0,0
        .byte 0,0,0,0,0,0,0,0,17,136,0,0,17,136,51,204
        .byte 51,204,17,136,0,0,17,136,0,0,0,0,0,0,0,0
        .byte 94,90,190,250,95,90,227,250,95,94,240,190,95,95,240,227
        .byte 124,240,175,175,215,240,167,175,245,124,165,175,245,215,165,167
        .byte 90,120,250,248,90,240,250,227,120,240,248,175,240,240,227,175
        .byte 95,124,240,240,95,241,240,225,124,245,240,165,241,245,225,165
        .byte 0,32,64,64,128,4,10,2,129,32,24,2,129,32,24,2
        .byte 51,204,68,34,153,153,187,204,187,221,153,136,68,34,34,136
        .byte 32,0,16,26,0,4,18,0,4,165,0,0,1,0,0,90
        .byte 128,32,192,96,224,224,224,224,224,224,224,224,240,224,176,160
        .byte 85,228,80,234,85,228,186,250,32,68,98,98,160,85,64,34
        .byte 117,128,250,216,245,213,250,216,117,0,0,64,160,145,80,0
        .byte 17,228,186,250,177,245,186,250,0,228,170,0,17,85,0,170
        .byte 145,245,64,234,17,128,50,200,100,100,170,80,68,32,0,0
        .byte 90,210,180,165,120,210,180,225,120,210,180,225,90,210,180,165
        .byte 0,0,0,0,0,0,0,0,32,192,80,224,80,224,176,240
        .byte 181,165,90,218,181,229,122,218,181,229,122,218,181,165,90,218
        .byte 0,0,0,0,0,0,0,0,32,196,80,234,85,228,186,250
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 0,32,90,72,0,0,18,0,132,128,0,0,0,132,90,2
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
        .byte 68,68,68,136,119,0,68,136,68,68,68,34,68,17,0,0
        .byte 0,0,255,255,255,255,0,0,0,0,0,0,0,0,170,170
        .byte 0,102,136,102,0,102,136,102,0,102,136,102,0,102,136,102
        .byte 0,102,136,119,0,51,136,0,0,0,136,0,0,0,170,170
        .byte 0,0,119,221,187,187,221,119,238,255,0,0,170,170,255,255
        .byte 255,255,119,238,119,102,51,68,34,68,34,68,34,68,0,0
        .byte 34,68,0,0,68,34,238,119,238,255,255,119,238,255,255,255
        .byte 19,76,4,136,27,17,174,35,76,87,136,141,17,10,35,140
        .byte 51,136,102,204,238,170,136,0,238,170,68,68,34,136,0,0
        .byte 15,15,12,3,13,11,13,11,13,11,9,9,1,8,0,0
        .byte 0,0,1,8,9,9,13,11,13,11,13,11,12,3,15,15
        .byte 129,32,24,18,1,0,16,0,165,165,16,0,1,0,24,18
        .byte 245,245,152,34,145,49,250,234,145,49,152,34,245,245,170,170
        .byte 0,0,0,136,0,34,0,136,0,85,0,0,0,0,0,0
        .byte 0,0,0,136,34,0,0,136,85,0,0,0,0,0,0,0
        .byte 0,0,0,136,0,0,0,136,0,0,0,136,0,0,0,136
        .byte 0,0,0,0,0,0,0,0,85,85,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,85,0,136,0,34,0,136
        .byte 0,0,0,0,0,0,0,0,85,0,0,136,34,0,0,136
        .byte 15,15,15,14,15,12,15,8,15,0,14,0,12,0,8,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,255,255,255,255
        .byte 0,0,255,255,255,255,255,255,255,255,255,255,255,255,255,255
        .byte 127,255,191,255,207,15,175,15,239,223,175,159,239,223,175,143
        .byte 255,239,255,223,15,63,15,95,191,127,47,95,175,127,175,95
        .byte 239,143,175,223,239,15,175,15,223,255,175,15,207,15,127,255
        .byte 175,127,191,95,15,127,15,95,255,191,15,95,15,63,255,239
        .byte 70,2,38,4,70,2,38,38,70,2,38,4,70,2,38,4
        .byte 0,0,175,175,95,95,0,0,0,0,5,5,27,10,0,0
        .byte 0,0,32,0,112,224,240,224,112,224,32,0,0,0,0,0
        .byte 0,0,16,224,48,224,48,224,48,128,112,192,48,128,16,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
bprop:
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
        .byte 2
        .byte 2
        .byte 0
        .byte 0
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
        .byte 2
        .byte 0
        .byte 5
        .byte 1
        .byte 2
        .byte 2
        .byte 0
        .byte 2
        .byte 0
        .byte 2
        .byte 0
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 5
        .byte 1
        .byte 2
        .byte 2
        .byte 2
        .byte 2
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
.out .sprintf("BLOCKS SIZE = %d", (* - chgfx))

sprgfx:
        .byte 139,224,87,144,55,112,55,72,23,72,23,248,11,0,11,80,23,248,0,48,19,248,13,240,61,224,123,214,56,44,28,24
        .byte 0,0,11,224,55,144,247,112,23,72,23,72,23,248,11,80,23,248,0,16,23,248,10,112,5,160,1,128,3,192,3,224
        .byte 0,0,0,0,11,224,55,144,119,112,151,72,23,72,23,248,23,248,0,0,23,200,15,176,23,186,43,222,48,28,28,8
        .byte 0,0,11,224,55,144,247,112,23,72,23,72,23,248,11,80,23,248,0,16,23,248,10,112,5,160,1,128,3,192,3,224
        .byte 7,209,9,234,14,236,18,236,18,232,31,232,0,208,10,208,31,232,12,0,31,200,15,176,7,188,43,222,52,12,24,24
        .byte 0,0,7,208,9,236,14,239,18,232,18,232,31,232,10,208,31,232,8,0,31,232,14,80,5,160,1,128,3,192,7,192
        .byte 0,0,0,0,7,208,9,236,14,238,18,233,18,232,31,232,31,232,0,0,19,232,13,240,93,232,123,212,56,12,16,56
        .byte 0,0,7,208,9,236,14,239,18,232,18,232,31,232,10,208,31,232,8,0,31,232,14,80,5,160,1,128,3,192,7,192
        .byte 219,109,255,255,0,0,63,252,63,252,21,84,21,84,0,0,4,16,2,16,2,16,4,32,4,32,0,16,0,0,0,0
        .byte 219,109,255,255,0,0,63,252,63,252,21,84,21,84,0,0,4,16,4,8,8,8,8,8,16,16,0,32,0,0,0,0
        .byte 0,0,93,92,85,84,93,92,0,0,117,212,85,84,117,212,0,0,87,116,85,84,87,116,0,0,117,92,85,84,117,92
        .byte 0,0,58,186,42,170,58,186,0,0,43,174,42,170,43,174,0,0,46,234,42,170,46,234,0,0,58,174,42,170,58,174
        .byte 0,0,0,0,63,252,48,8,48,12,48,12,48,12,63,252,56,28,51,12,51,12,51,12,16,12,15,252,0,0,0,0
        .byte 0,0,0,0,63,252,63,252,63,252,63,252,62,124,60,60,62,124,63,220,62,124,62,124,62,124,63,252,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,63,252,63,252,63,252,57,156,57,156,63,252,56,28,55,236,0,0,0,0,0,0
        .byte 16,1,8,6,10,8,4,8,130,84,71,224,53,96,78,192,15,224,5,64,8,32,48,16,40,24,32,36,64,2,128,1
        .byte 128,8,96,16,16,80,16,32,42,65,7,226,5,108,2,242,7,240,2,160,132,16,136,8,88,54,36,4,2,8,0,16
        .byte 128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1
        .byte 0,0,0,0,0,64,64,128,37,2,30,4,49,4,225,195,16,102,8,156,8,34,16,192,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,9,2,16,164,32,120,36,140,195,199,102,8,57,16,36,20,74,8,1,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,102,102,255,255,159,255,191,255,255,255,255,255,255,255,255,255,128,1,230,103,238,239,102,102,102,102,68,68
        .byte 0,0,0,0,0,0,102,102,255,255,191,255,255,255,255,255,128,1,230,103,238,239,102,102,102,102,102,102,68,68,0,0
        .byte 0,0,0,0,0,0,0,0,102,102,255,255,255,255,128,1,230,103,238,239,102,102,102,102,102,102,68,68,0,0,0,0
        .byte 0,0,0,0,0,0,102,102,255,255,191,255,255,255,255,255,128,1,230,103,238,239,102,102,102,102,102,102,68,68,0,0
        .byte 0,0,7,230,7,230,27,222,25,158,127,254,127,254,120,24,120,24,103,224,103,224,25,152,25,152,96,6,96,6,0,0
        .byte 0,0,103,224,103,224,121,152,123,216,127,254,127,254,24,30,24,30,6,102,6,102,25,152,25,152,24,24,24,24,0,0
        .byte 3,224,15,248,19,156,45,110,45,110,101,47,101,47,115,159,127,255,127,255,127,255,127,255,127,255,94,247,76,99,0,0
        .byte 7,192,31,240,57,200,118,180,118,180,244,166,244,166,249,206,255,254,255,254,255,254,255,254,255,254,239,122,198,50,0,0
        .byte 1,0,1,64,0,128,8,128,5,32,3,80,3,64,17,128,10,128,5,194,8,172,17,16,1,32,4,128,3,192,1,160
        .byte 5,128,3,192,1,32,4,128,8,128,53,16,67,160,1,136,1,84,2,192,10,192,4,160,1,16,1,0,2,128,0,128
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 16,112,0,80,60,112,36,0,36,4,60,0,0,118,2,86,64,112,0,0,0,64,7,0,5,30,7,18,32,18,0,30
        .byte 14,8,10,0,14,60,0,36,32,36,0,60,110,0,106,0,14,34,0,0,2,0,0,224,120,160,72,224,72,4,120,0
        .byte 120,0,72,4,72,224,120,160,0,224,2,0,0,0,46,64,10,2,78,0,0,60,32,36,0,36,14,60,10,0,14,8
        .byte 0,30,32,18,7,18,5,30,7,0,0,64,4,0,0,116,64,80,0,114,60,0,36,4,36,0,60,112,0,80,16,112
frmlst:
        .byte 0,4
        .byte 4,4
        .byte 8,2
        .byte 10,2
        .byte 12,1
        .byte 13,1
        .byte 14,1
        .byte 15,2
        .byte 17,1
        .byte 18,3
        .byte 21,4
        .byte 25,2
        .byte 27,2
        .byte 29,3
        .byte 32,4,36,0
.out .sprintf("SPRITES SIZE = %d", (* - sprgfx))

scdat:
        .word 348,358,350,305,376,359,330,370,262,297,330,368,363,384,330,296,465,346,397,371,222,223
        .byte 16,25,25,25,19,11,11,28,23,0,8,8,255,0,6,22,16,19,21,18,24,24,24,20,0,0,9,28,11,10,8,255,0,6
        .byte 22,17,20,21,255,26,5,0,0,0,9,11,11,10,255,0,6,22,18,20,21,38,40,255,0,20,41,39,255,73,4,75,255,0,15
        .byte 8,8,255,0,4,72,0,74,255,73,9,75,0,0,0,47,48,15,0,0,0,72,0,72,14,12,13,15,0,14,47,48,15
        .byte 72,0,0,0,13,15,14,12,13,15,72,0,70,255,73,9,71,14,12,13,255,73,6,71,255,0,14,43,255,0,15,42,255,0,4
        .byte 32,33,255,0,14,3,4,3,3,4,3,35,34,255,0,7,1,1,0,0,0,46,0,65,255,8,4,255,0,7,3,3,4
        .byte 2,2,7,0,0,8,0,0,255,8,4,255,0,9,8,6,6,5,3,3,68,11,11,10,8,9,28,11,11,11,7,255,0,5
        .byte 9,23,8,0,0,0,29,0,0,0,9,23,9,11,11,11,7,255,0,6,8,8,255,0,8,8,255,0,4,5,3,4,4,4
        .byte 0,0,8,8,255,0,8,8,255,0,4,11,10,8,65,9,11,11,10,8,0,0,255,1,5,0,8,255,0,4,11,11,10
        .byte 0,22,16,19,21,8,4,4,255,2,5,4,3,4,3,4,3,11,11,11,23,22,17,20,21,9,23,0,255,6,5,0,8
        .byte 255,0,4,27,27,0,8,22,17,20,21,0,8,0,8,42,8,8,8,0,9,255,11,4,16,19,21,8,22,18,20,21,0
        .byte 8,0,8,8,8,9,28,23,255,0,5
        .byte 38,40,0,9,23,255,0,4,1,1,255,0,6,8,8,0,0,0,41,39,0,0,8,0,0,74,4,2,2,4,75,255,0,4
        .byte 8,8,0,0,0,8,8,0,0,8,0,0,72,65,6,6,65,72,255,0,4,8,8,0,0,0,3,3,0,0,8,0,0
        .byte 72,0,7,7,0,72,255,0,4,3,3,7,0,0,8,0,0,0,8,0,0,72,0,4,7,0,72,255,0,5,9,5,3,3
        .byte 8,0,0,0,8,0,0,70,255,73,4,71,255,0,9,10,0,0,0,2,255,0,17,36,0,0,4,3,255,0,4,42,42
        .byte 255,0,6,74,255,73,4,30,255,0,8,9,28,23,255,0,5,72,255,0,4,31,255,0,9,9,28,23,255,0,4,72,255,0,4
        .byte 37,255,0,10,9,28,23,0,0,0,72,255,0,4,14,12,13,15,255,0,6,32,33,9,68,55,0,0,70,255,73,4,23
        .byte 14,47,48,15,255,0,5,35,34,44,68,23,255,0,7,8,32,33,36,36,255,0,4,14,12,13,15,8,8,255,0,6,7
        .byte 8,35,34,30,30,255,0,8,8,8,255,0,4,5,4,7,9,11,11,31,31,255,0,8,8,8,255,0,5,8,255,0,4
        .byte 37,37,255,0,8,8,8,255,0,5,9,11,255,0,13,8,8,255,0,15,255,1,4,0,8,8,0,0,0,255,27,4,255,0,5
        .byte 3,3,4,4,3,4,3,0,8,8,0,0,0,16,25,25,20,255,0,6,8,64,255,6,4,0,8,8,0,0,0,18,24,24
        .byte 20,4,3,4,3,11,11,10,0,255,8,4,0,8,8,0,0,0,255,26,4
        .byte 38,40,11,10,0,0,9,23,9,23,0,32,33,43,0,43,31,31,36,14,47,48,41,39,255,0,5,9,23,9,23,35,34
        .byte 0,0,0,37,37,30,14,12,13,8,8,255,0,6,9,23,8,255,0,7,31,0,0,0,10,8,255,0,7,8,8,255,0,7
        .byte 37,0,42,0,11,10,255,0,7,8,8,255,0,9,8,36,255,0,9,8,8,255,0,9,8,30,255,0,4,14,12,13,15
        .byte 64,8,8,255,0,4,45,0,0,45,0,8,31,255,0,9,27,27,44,3,3,3,4,4,4,3,3,3,37,36,255,0,7
        .byte 22,16,19,21,44,11,11,11,6,6,11,23,9,23,30,255,0,7,22,18,20,21,44,11,11,11,10,9,11,28,23,9,31
        .byte 255,0,8,26,26,255,0,8,9,28,23,37,43,255,0,13,74,73,75,0,0,9,28,27,27,27,255,0,6,42,42,36,0,0,0
        .byte 72,36,72,0,0,0,9,25,25,20,255,0,4,44,4,4,4,30,0,0,0,72,30,72,255,0,4,24,24,20,255,0,6
        .byte 6,65,31,0,0,0,72,31,72,255,0,4,26,26,26,255,0,6,9,23,37,0,0,0,72,37,72,0,0,14,12,255,0,10
        .byte 8,255,0,4,70,73,71,255,0,14,8,255,0,11,3,4,3,255,0,5,3,3,4,3,4,3,255,0,5,3,3,3,6
        .byte 5,255,0,7,5,6,6,6,255,0,7,4,6,8,0,0,255,1,5,0,0,8,8,8,0,255,1,5,0,0,8,8,0,0
        .byte 255,2,5,11,11,10,8,8,0,255,2,5,0,0,9
        .byte 8,8,8,0,0,0,72,30,72,255,0,6,30,0,9,23,9,38,40,10,8,26,0,0,0,72,31,72,255,0,6,31,0,0
        .byte 9,11,41,39,11,10,255,0,4,72,37,72,255,0,6,37,255,0,4,9,28,255,0,6,70,73,71,255,0,12,9,73,73,73
        .byte 75,255,0,18,36,0,0,72,255,0,8,255,27,4,255,0,4,1,1,30,0,0,72,0,14,12,13,15,32,33,43,16,25,25
        .byte 20,0,0,44,3,3,3,31,0,0,72,255,0,5,35,34,37,18,24,24,20,0,0,44,5,4,4,37,0,0,72,255,0,8
        .byte 255,26,4,255,0,4,6,6,16,19,21,72,0,74,255,73,8,75,255,0,5,8,8,17,20,21,72,0,72,46,46,255,0,6
        .byte 72,255,0,5,8,9,18,20,21,72,0,72,8,8,255,0,6,72,255,0,5,9,23,64,0,0,72,0,72,65,65,14,12
        .byte 13,15,65,65,72,255,0,6,9,73,73,73,71,0,70,255,73,8,71,0,0,46,255,0,21,8,0,0,14,12,255,0,17
        .byte 9,255,11,4,255,0,27,43,14,47,48,15,43,14,12,13,15,0,0,0,14,12,13,15,255,0,5,37,255,0,4,37,255,0,11
        .byte 1,1,1,255,0,19,2,2,2,255,0,19,6,6,6,0,3,3,4,3,3,3,4,3,3,3,0,45,45,0,3,4,3,3
        .byte 12,13,15,0,0,53,0,0,0,22,16,19,21,255,8,4,0,9,11,38,40,47,48,15,0,0,52,0,0,51,22,18,20
        .byte 21,255,27,4,0,0,0,41,39,255,0,5,49,0,0,50,0,26,26,0,16,25,25,19,0,0,0,9,11,255,0,5,9
        .byte 23,0,49,14,12,13,15,18,24,24,24,0,0,0,14,47,255,0,6,9,11,10,255,0,4,255,26,4,0,0,0,14,47
        .byte 255,0,7,8,255,0,11,14,12,13,255,0,7,8,255,0,13,36,255,0,6,7,3,3,3,255,0,9,32,33,30,0,0,0
        .byte 5,4,4,4,8,32,33,255,0,8,36,35,34,31,255,0,6,42,8,35,34,0,0,0,46,0,42,42,0,30,0,0,37
        .byte 255,0,5,5,3,3,4,7,0,0,44,68,11,3,3,7,31,255,0,8,65,6,6,6,255,0,4,29,0,4,4,65,37
        .byte 0,0,0,36,36,36,0,0,0,8,8,0,0,46,255,0,4,9,23,255,0,5,30,30,30,11,11,11,10,8,11,11,10
        .byte 255,0,5,9,23,255,0,4,31,31,31,255,11,4,10,255,0,9,8,255,0,4,37,37,37,255,0,6,255,1,5,0,0,0
        .byte 8,255,0,11,36,4,255,2,5,3,44,7,4,3,255,0,10,30,0,255,6,4,8,0,0,0,8,255,0,11,31,0,8
        .byte 0,0,8,8,0,0,0,8,0,0,0,46,16,19,21,22,16,19,21,37,0,8,0,0,8,8,0,0,0,8,0,0,0
        .byte 9,17,20,21,22,17,20,21,11,11,10,42,42,9,68,55,0,0,8,255,0,4,18,20,21,22,18,20,21,11,11,11,2,2
        .byte 4,3,45,45,45,4,4,3,3,3
        .byte 255,0,4,5,7,38,40,10,8,0,0,0,5,4,7,59,255,56,5,255,0,4,8,7,41,39,11,10,0,0,0,9,5
        .byte 7,59,255,56,5,47,48,15,0,8,7,255,0,10,59,255,56,5,255,11,4,10,7,255,0,10,59,56,57,56,56,56,255,0,5
        .byte 7,1,1,255,0,8,59,255,56,5,0,0,14,12,13,7,2,2,255,0,6,27,27,60,255,58,5,255,0,5,5,3,3,3
        .byte 4,0,0,0,22,16,19,21,6,6,0,6,6,255,0,8,8,7,46,0,0,22,18,20,21,8,8,0,8,8,255,0,8
        .byte 8,7,29,0,0,0,26,26,0,9,28,23,9,28,255,0,5,7,11,55,7,7,255,73,4,75,0,0,0,9,28,23,9
        .byte 12,13,15,0,0,7,44,11,5,32,33,0,0,0,72,53,0,0,0,9,28,23,255,0,8,72,35,34,0,0,0,72,52
        .byte 0,51,0,0,8,9,255,0,8,72,64,14,12,13,15,72,49,0,50,0,0,8,255,0,4,27,27,0,0,0,70,255,73,5
        .byte 71,0,0,49,0,0,8,0,0,0,22,16,19,21,255,0,14,9,46,53,0,22,18,20,21,255,0,15,9,52,0,0,26,26
        .byte 255,0,16,53,49,36,36,65,0,0,0,43,255,27,4,255,0,9,52,11,30,30,55,0,0,44,22,16,25,25,19,21,255,0,8
        .byte 49,11,31,31,55,0,44,11,22,17,54,54,20,21,255,0,8,8,0,37,37,0,0,0,44,22,18,24,24,20,21,255,0,5
        .byte 46,0,0,8,32,33,255,45,4,0,0,255,26,4,255,0,6,8,0,0,8
        .byte 14,12,13,15,43,32,33,4,38,40,10,8,0,0,65,8,8,0,65,14,12,13,255,0,5,35,34,7,41,39,11,10,0,0
        .byte 36,26,26,0,36,255,0,10,7,8,10,255,0,4,30,0,0,0,30,0,0,0,1,1,1,255,0,4,7,10,255,0,5
        .byte 31,0,0,0,31,0,0,0,2,2,2,0,0,0,14,7,15,255,0,5,37,0,0,0,37,0,0,0,6,6,6,0,0,0
        .byte 14,5,15,0,27,27,255,0,10,8,8,8,255,0,6,22,16,19,21,2,2,3,3,3,4,4,0,0,10,8,8,255,0,6
        .byte 22,18,20,21,6,6,9,28,28,23,0,0,46,11,10,8,255,0,4,4,36,64,26,26,255,0,4,8,8,8,0,0,9
        .byte 11,11,10,255,0,4,5,30,255,0,7,26,8,8,255,0,11,31,255,0,8,9,8,0,0,46,255,0,8,37,255,0,9
        .byte 26,23,0,9,255,0,7,4,36,255,0,5,255,27,5,8,0,0,55,255,0,5,44,7,30,255,0,4,44,16,25,25,25
        .byte 19,9,55,44,11,55,0,0,0,44,11,5,31,51,0,0,44,11,18,24,24,24,20,255,0,11,37,50,255,0,4,255,26,5
        .byte 0,0,46,255,0,9,49,255,0,11,9,11,55,255,0,19,36,255,0,21,30,255,0,21,31,0,0,14,47,48,15,44,255,2,4
        .byte 255,0,5,4,4,0,0,0,37,45,45,45,8,8,45,45,255,6,4,255,45,5,6,6,255,45,4
        .byte 255,0,8,30,8,255,0,4,59,255,56,7,255,27,4,255,0,4,31,8,55,0,0,22,59,255,56,7,16,25,25,19,255,0,4
        .byte 37,9,55,0,0,22,59,255,56,7,18,24,24,20,255,0,10,59,56,57,255,56,5,255,26,4,255,0,4,36,255,0,4
        .byte 36,59,255,56,7,47,13,15,255,0,5,30,43,0,0,0,30,60,255,58,7,27,27,27,255,0,5,31,255,0,4,31,65
        .byte 43,38,40,255,26,4,16,25,19,0,0,0,36,65,37,255,0,4,37,0,0,41,39,255,1,4,18,24,20,0,0,0,30
        .byte 255,0,9,9,28,255,2,4,26,26,26,0,0,0,31,255,0,10,9,255,6,4,64,255,0,5,37,65,255,0,6,3,11
        .byte 4,4,8,42,8,8,32,33,255,0,6,51,0,0,0,46,46,0,0,0,9,8,8,9,28,35,34,255,0,6,50,36,0,0
        .byte 8,8,255,0,4,9,28,23,9,255,0,4,22,16,19,21,49,30,0,0,8,8,255,0,5,9,28,23,255,0,4,22,17
        .byte 20,21,43,31,7,0,8,8,255,0,6,9,28,27,27,27,0,22,17,20,21,0,37,5,4,3,3,3,255,0,6,9,16
        .byte 25,19,0,22,18,20,21,0,0,42,0,8,8,255,0,6,46,46,18,24,20,0,0,8,42,0,0,0,8,0,8,8,255,0,8
        .byte 26,26,26,0,0,8,8,0,0,0,9,11,10,9,23,255,0,7,42,42,0,0,0,8,8,255,0,7,9,23,0,0,0
        .byte 1,1,0,10,8,0,0,0,8,9,23,255,0,4,45,0,0,8,0,0,255,3,4,11,10,0,0,44,255,3,13,7,2,2,2
        .byte 59,255,56,21,59,255,56,21,59,255,56,21,59,255,56,21,59,56,56,63,56,56,56,63,56,56,56,63,56,56,56,61,56,56,56
        .byte 61,61,61,59,255,56,8,63,255,56,5,61,56,56,56,61,56,56,59,255,56,14,61,56,56,63,61,56,56,59,63,255,56,13
        .byte 61,56,56,56,61,56,56,59,255,56,6,62,255,56,7,61,56,56,56,61,61,61,59,255,56,5,62,61,61,62,255,56,5
        .byte 61,63,255,56,5,59,56,56,62,56,61,61,56,56,61,61,255,56,4,61,255,56,6,59,255,61,4,255,56,10,61,255,56,6
        .byte 59,255,56,14,61,61,56,56,61,61,61,59,255,56,13,63,61,61,56,56,61,56,56,59,255,56,14,61,56,56,56,61,61,61
        .byte 59,255,56,14,61,255,56,6,59,56,255,61,5,255,56,4,255,61,5,62,255,56,5,59,56,57,255,56,12,255,61,5,56,56
        .byte 59,255,56,5,255,62,6,56,62,62,56,255,62,4,56,56,60,255,58,19,38,40,0,0,255,26,7,0,36,14,12,13,15
        .byte 14,12,13,15,0,41,39,0,0,255,1,7,0,30,32,33,43,14,47,48,15,0,0,8,8
        .byte 255,0,4,14,43,32,33,5,255,3,4,7,0,14,12,13,15,255,0,8,37,35,34,0,255,26,4,255,0,9,46,0,0,0
        .byte 14,43,15,255,0,15,10,255,0,4,64,255,0,13,14,47,48,255,0,5,51,255,0,20,46,50,0,0,7,255,1,4,7
        .byte 255,0,8,255,11,4,10,49,0,0,5,255,2,4,4,3,3,3,4,255,3,4,255,0,11,8,8,255,0,4,5,255,2,4
        .byte 46,255,0,10,8,9,23,255,0,4,5,6,6,4,10,255,0,10,26,0,26,255,0,5,8,255,0,6,32,33,255,0,13
        .byte 8,255,0,6,35,34,64,0,255,27,6,0,36,0,255,27,5,255,0,4,14,47,48,15,16,255,25,4,19,11,30,11,16
        .byte 255,25,4,255,0,4,14,12,13,15,18,255,24,4,20,11,31,11,18,255,24,4,255,0,8,255,26,6,0,37,0,255,26,5
        .byte 255,0,17,9,255,28,4,255,0,18,9,11,38,40,255,0,20,41,39,53,22,16,19,21,22,16,19,21,255,0,7,65,22
        .byte 16,19,21,11,52,22,17,20,21,22,17,20,21,0,0,0,65,255,0,4,22,17,20,21,44,49,22,17,20,21,22,17,20
        .byte 21,45,255,0,6,45,22,17,20,21,44,45,22,18,20,21,22,18,20,21,7,255,45,6,7,22,18,20,21,45
        .byte 38,40,10,8,64,255,0,13,72,64,53,51,41,39,11,10,255,0,14,72,0,52,50,13,15,255,0,16,72,0,49,49,47
        .byte 48,15,255,0,15,70,73,73,73,14,47,48,15,255,0,18,27,27,65,0,46,255,0,4,46,0,0,0,46,0,0,46,255,0,5
        .byte 16,19,21,0,42,255,0,4,8,0,0,0,42,0,0,8,0,0,14,47,48,18,24,21,0,8,255,0,4,8,0,0,0
        .byte 8,0,0,8,255,0,5,26,26,0,0,8,255,0,4,8,0,0,0,8,0,0,8,255,0,9,46,255,0,4,46,0,0,0
        .byte 46,0,0,5,3,3,4,4,4,255,0,4,8,255,0,4,42,0,0,0,8,255,0,4,8,8,32,33,255,0,4,8,255,0,4
        .byte 8,0,0,0,9,255,11,4,68,68,35,34,255,6,5,255,0,4,8,255,0,8,29,9,11,11,255,26,5,0,0,0,46
        .byte 8,255,0,19,44,68,10,0,0,0,46,255,0,16,8,255,0,4,8,255,0,16,8,255,0,4,8,0,0,0,5,4,3
        .byte 4,4,255,0,8,8,255,0,4,9,23,0,46,0,9,28,28,23,255,1,5,0,0,0,8,255,0,5,9,23,42,0,0
        .byte 9,11,68,255,2,5,0,0,0,8,14,47,48,15,0,0,9,8,0,14,12,13,15,255,6,5,0,0,0,8,255,45,4
        .byte 0,0,0,8,23,0,65,36,36,255,8,5,0,0,0,8,255,2,4,23,0,0,8,8,0,0,30,30
        .byte 255,0,7,8,0,0,8,0,0,29,0,8,255,0,13,29,0,0,29,0,0,42,0,8,255,0,13,42,0,0,42,0,0
        .byte 8,0,8,255,0,12,65,255,66,9,255,0,13,255,67,9,65,45,255,0,11,27,27,27,38,40,255,11,5,10,255,0,6
        .byte 46,255,0,4,16,25,24,41,39,0,14,12,13,15,255,0,7,8,255,0,4,26,26,26,0,0,0,14,47,48,15,255,0,4
        .byte 45,0,0,8,255,0,4,64,0,64,255,0,5,36,255,0,4,45,2,0,0,8,46,0,0,0,43,255,0,7,30,255,0,4
        .byte 2,3,0,0,9,68,55,0,0,32,33,0,0,46,0,0,0,31,255,0,4,5,4,0,0,0,8,0,0,0,35,34,11,11
        .byte 10,0,0,0,37,255,0,4,8,9,0,0,0,8,0,14,12,13,15,255,0,4,27,27,65,0,0,0,45,9,11,48,15
        .byte 11,10,0,0,22,16,19,21,0,0,22,16,19,21,0,0,0,5,4,3,13,15,255,0,4,22,17,20,21,0,0,22,18
        .byte 20,21,255,0,4,9,28,0,36,255,0,4,22,17,20,21,51,0,0,26,26,0,0,0,45,32,33,9,36,30,255,0,4
        .byte 22,18,20,21,50,255,0,7,7,35,34,43,30,31,255,0,5,8,8,0,49,255,0,7,7,23,14,12,31,37,255,0,4
        .byte 65,255,66,9,0,45,7,28,23,0,255,27,5,0,0,255,67,9,65,5,3,9,28,23,16,25,25,25,19,0,0,8,0
        .byte 42,0,8,0,42,0,42,0,0,2,0,9,28,18,24,24,24,20,0,0,8,0,8,0,8,0,8,0,8,0,0,8,43
        .byte 0,8
        .byte 0,72,255,0,5,72,0,0,38,40,10,8,0,72,255,0,4,72,64,73,71,255,0,5,72,0,0,41,39,11,10,43,72
        .byte 255,0,4,70,73,255,0,7,72,0,0,8,8,32,33,0,72,255,0,9,5,4,4,4,3,3,3,2,2,35,34,0,72
        .byte 255,0,9,8,65,32,33,70,255,73,7,71,0,0,0,46,0,0,46,0,0,8,0,35,34,255,0,12,9,11,11,9,11,11
        .byte 10,255,0,15,51,255,0,9,74,73,75,255,0,9,50,0,0,46,255,0,6,72,46,72,255,0,9,49,0,0,10,255,0,6
        .byte 72,8,72,255,0,4,46,255,0,4,8,0,0,51,255,0,6,70,8,71,0,0,0,44,68,255,11,4,10,0,0,50,0,0,0
        .byte 51,0,0,0,8,0,53,0,0,0,8,255,0,4,74,73,73,49,0,0,0,50,0,0,0,8,0,52,0,0,0,8,255,0,4
        .byte 72,0,0,8,0,0,0,49,0,0,0,8,0,49,0,0,0,8,255,0,4,72,0,0,10,0,0,0,9,11,11,11,68
        .byte 11,68,55,0,0,8,0,46,0,0,72,255,0,10,29,0,9,11,11,11,68,11,10,0,0,70,73,73,255,0,14,8,255,0,6
        .byte 51,0,0,53,255,0,5,51,255,0,5,8,255,0,6,50,0,0,52,255,0,5,50,0,0,0,53,0,8,0,0,53,0,0,0
        .byte 49,53,0,49,255,11,5,49,23,0,0,52,0,8,0,0,52,0,0,0,8,52,11,43,255,0,5,36,9,11,11,49,11
        .byte 68,11,11,49,11,11,11,10,49,32,33,255,45,5,30,255,0,5,8,255,0,6,64
        .byte 65,255,66,4,65,0,0,0,72,0,43,0,72,0,0,14,12,13,15,32,33,0,255,67,4,255,0,4,70,73,73,73,71
        .byte 255,0,6,35,34,255,0,21,36,255,0,9,74,73,73,73,75,255,0,7,30,0,255,1,4,255,0,4,72,0,0,0,72
        .byte 255,0,7,31,65,255,2,4,65,0,0,0,72,0,0,0,72,0,0,0,43,0,0,0,37,0,8,8,6,6,36,0,0,0
        .byte 72,0,64,0,72,0,0,0,7,255,11,4,0,29,8,8,8,30,0,0,0,72,0,0,0,72,0,0,0,7,55,0,0
        .byte 44,0,42,8,8,29,31,0,0,0,70,73,73,73,71,0,0,0,7,55,0,0,44,0,8,8,8,42,37,255,0,11,7
        .byte 255,0,5,255,8,4,0,0,255,45,5,0,255,45,4,7,255,11,4,0,9,28,28,28,23,0,5,4,4,4,3,3,3
        .byte 4,4,4,7,255,0,6,9,28,28,28,23,0,0,43,14,47,48,15,0,8,8,255,0,8,9,28,28,28,23,255,0,7
        .byte 9,8,255,0,4,46,255,0,4,9,28,28,28,23,255,0,7,8,23,0,0,0,9,255,0,5,9,28,28,28,23,255,0,6
        .byte 8,29,255,0,4,7,0,0,44,5,4,3,3,3,4,4,4,7,11,5,255,4,4,3,3,3,10,255,0,4,29,0,9
        .byte 28,28,28,23,0,0,0,14,47,48,15,14,12,13,255,0,8,9,28,28,28,23,255,0,4,38,40,22,16,19,255,0,9
        .byte 255,8,4,255,0,4,41,39,22,17,20,255,0,5,42,0,0,0,8,29,8,8,255,0,4,8,8,22,18,20,45,0,0
        .byte 45,4,2,3,3,3,8,3,4,8,5,4,4,3,3,4,0,0,0
        .byte 11,23,0,0,0,64,59,54,255,56,6,54,59,64,8,8,0,8,0,255,11,6,59,54,255,56,6,54,59,11,8,10,0
        .byte 8,0,0,0,9,55,38,40,59,255,54,8,59,11,10,0,0,29,0,11,11,11,55,41,39,59,255,54,8,59,11,11,11
        .byte 55,255,0,6,51,74,59,54,16,255,78,4,54,54,59,75,53,255,0,8,50,72,65,54,17,56,0,56,56,54,54,65,72
        .byte 52,255,0,4,46,0,0,0,49,72,0,54,18,56,77,56,56,54,76,0,72,49,255,0,4,68,11,11,11,10,70,255,73,10
        .byte 71,9,11,11,11,23,10,255,0,14,43,255,0,5,9,0,0,0,46,255,0,17,46,11,11,11,10,255,0,11,36,255,0,5
        .byte 9,255,0,15,30,55,0,0,44,11,11,46,255,0,14,31,0,0,46,0,0,0,10,255,0,14,37,0,0,8,255,0,4
        .byte 27,27,255,0,15,8,0,0,0,22,16,19,21,255,0,14,8,0,0,46,22,18,20,21,0,0,0,255,66,8,0,0,0
        .byte 9,23,0,9,0,26,26,22,16,19,21,255,67,8,255,0,4,8,0,36,12,13,15,22,17,20,21,255,2,8,43,0,0,0
        .byte 8,0,30,16,19,21,22,18,20,21,255,6,8,255,0,4,8,0,31,17,20,21,36,43,32,33,29,8,8,8,29,9,8
        .byte 9,23,0,0,0,8,0,37,17,20,21,30,0,35,34,42,9,8,8,42,0,8,23,8,45,0,45,8,45,0
        .byte 32,33,53,255,0,7,72,65,0,65,72,255,0,4,9,28,69,35,34,52,255,0,7,72,0,64,0,72,255,0,5,9,69
        .byte 0,0,49,255,0,7,70,73,73,73,71,255,0,6,9,0,0,8,255,0,21,8,255,0,20,44,7,255,0,21,7,255,0,19
        .byte 55,0,5,4,3,3,4,3,3,3,4,4,3,3,4,4,3,3,4,3,46,46,255,0,13,43,14,12,13,15,6,6,9
        .byte 46,255,0,18,29,29,0,9,73,73,75,255,0,15,42,0,0,0,64,0,72,255,0,15,8,0,0,46,58,58,58,65,0
        .byte 64,255,0,12,8,42,0,9,16,25,25,25,22,21,7,55,0,0,44,7,3,255,0,5,8,8,42,0,17,54,54,54,22
        .byte 21,7,55,0,0,44,7,4,3,3,4,4,4,255,3,4,18,54,54,76,22,21,7,55,0,0,44,7,4,32,33,43,0,0
        .byte 9,28,28,23,58,58,58,65,0,64,255,0,7,35,34,0,0,0,44,68,28,28,64,0,72,255,0,16,8,9,28,73,73
        .byte 71,255,0,16,38,40,9,255,0,12,255,27,4,0,36,0,41,39,11,69,69,69,0,69,69,69,43,22,16,19,21,16,25,25
        .byte 19,0,30,0,14,47,48,8,8,8,0,14,47,48,15,22,17,20,21,18,24,24,20,0,31,14,12,13,15
        .byte 43,72,65,59,56,54,54,56,56,56,54,54,56,59,65,72,38,40,10,8,0,43,0,72,0,59,54,78,78,54,56,56,78,78
        .byte 54,59,0,72,41,39,11,10,0,0,0,72,0,59,255,54,4,56,56,54,54,54,59,0,72,8,255,0,6,72,65,59,54
        .byte 78,78,54,56,56,78,78,54,59,65,72,29,255,0,6,70,73,59,54,56,56,54,56,56,54,54,78,59,73,71,14,12,13
        .byte 15,0,46,0,0,0,29,56,68,68,68,56,68,56,68,56,29,255,0,4,36,0,0,9,46,0,0,0,44,68,56,255,68,4
        .byte 56,55,255,0,5,30,0,0,0,10,0,0,42,56,56,68,68,56,68,56,56,56,42,255,0,4,31,0,0,46,0,74,73
        .byte 59,56,54,68,56,56,56,54,54,56,59,73,75,0,0,37,0,0,9,0,72,46,59,54,78,56,54,56,56,78,78,54,59
        .byte 65,72,0,0,32,33,0,0,0,70,73,59,255,54,4,56,56,56,54,78,59,73,71,0,0,35,34,0,0,46,0,0,59
        .byte 54,78,78,54,56,56,54,78,56,59,255,0,4,36,43,0,46,10,74,73,59,54,56,56,54,56,56,54,54,54,59,73,75
        .byte 0,0,30,0,0,9,0,72,65,59,78,56,56,78,56,56,78,78,78,59,65,72,0,0,31,255,0,4,72,0,59,56,54,54
        .byte 255,56,4,54,56,59,0,72,0,0,37,0,0,46,46,72,64,59,54,78,78,54,56,56,54,54,56,59,64,72,0,0,64
        .byte 53,0,9,10,72,0,59,255,54,4,56,56,78,54,56,59,0,72,0,0,65,52,0,0,0,72,65,59,54,78,78,54,56,56,56
        .byte 54,56,59,65,72,0,0,0,49,0,46,46,70,73,59,54,56,56,54,56,56,54,54,54,59,73,71,0,0,0,8,0,9
        .byte 10,0,0,59,78,56,56,78,56,56,78,78,78,29,255,0,5,8,0,0,36,74,73,59,56,54,54,56,56,56,54,54,54
        .byte 42,73,75,0,0,0,8,1,1,30,72,65,59,54,78,78,54,56,56,54,78,54,59,65,72,45,45,5,4,4,3
        .byte 64,255,0,5,36,8,38,40,10,72,0,0,29,68,68,255,56,5,255,0,6,30,9,41,39,0,70,73,75,44,68,255,56,6
        .byte 255,0,6,31,0,29,29,51,0,0,72,42,255,56,7,255,0,6,37,0,0,0,50,0,43,72,59,63,255,56,6,255,0,10
        .byte 49,0,0,72,59,56,56,63,255,56,4,255,0,10,9,55,0,72,29,255,56,6,63,255,0,13,72,44,68,255,56,6,46
        .byte 0,0,27,27,27,255,0,7,72,42,255,56,7,10,0,0,16,25,19,0,0,0,14,12,13,15,72,59,61,57,61,56,56,56
        .byte 61,0,0,0,18,24,24,255,0,5,43,0,70,59,56,56,56,62,62,62,56,0,0,46,26,26,26,255,0,8,60,255,58,7
        .byte 0,0,29,255,0,17,69,69,255,0,6,43,0,43,255,0,11,9,68,46,0,0,0,43,11,10,0,9,11,43,0,0,0
        .byte 74,73,75,255,0,4,9,11,23,0,32,33,255,0,5,9,23,0,0,72,0,72,255,0,5,11,11,11,35,34,255,0,6
        .byte 9,5,7,72,0,72,255,0,5,22,16,19,21,255,0,8,9,5,4,4,72,46,255,0,4,22,17,20,21,255,0,9,8
        .byte 72,65,72,29,0,22,16,19,22,18,20,21,255,0,9,29,70,73,71,0,0,22,17,20,0,8,255,0,7,36,0,36,255,0,6
        .byte 43,22,18,20,0,29,255,0,7,30,0,30,32,33,43,0,14,47,48,15,0,0,64,14,12,13,15,14,12,13,15,31,0
        .byte 31,35,34,0,45,45,255,0,4,64
        .byte 38,40,11,10,255,0,5,59,54,54,54,59,12,13,15,255,0,5,41,39,10,32,33,255,0,4,59,54,54,54,59,0,27,27
        .byte 255,0,5,10,0,0,35,34,255,0,4,64,54,54,54,64,22,16,19,21,255,0,7,43,255,0,5,72,54,54,76,72,22
        .byte 18,20,21,46,0,0,0,11,11,11,7,255,0,5,70,26,26,26,71,0,26,26,0,8,0,0,0,46,0,9,7,255,0,6
        .byte 27,27,27,0,44,68,11,11,10,0,0,0,10,0,0,7,255,0,6,42,8,29,0,0,29,0,0,0,22,16,19,55,44
        .byte 11,7,255,0,6,8,8,42,255,0,6,22,17,20,46,0,14,47,48,15,0,0,0,5,4,4,3,7,255,0,4,46,22
        .byte 18,20,10,0,14,12,13,15,0,0,0,65,2,2,2,65,255,0,4,8,0,0,43,0,0,22,16,19,21,255,0,4,6,6,6
        .byte 36,0,27,27,0,9,11,11,11,46,0,22,17,20,21,0,0,44,11,69,69,69,30,22,16,19,21,255,0,4,10,74,22
        .byte 18,20,21,255,0,4,8,8,8,31,22,18,20,21,255,0,5,72,65,72,255,0,6,69,69,69,37,0,26,26,0,0,0
        .byte 27,27,46,72,0,72,255,0,6,26,26,26,255,0,6,22,16,19,10,72,65,72,255,0,15,22,18,20,0,70,73,71,255,0,12
        .byte 27,27,0,0,26,26,46,255,0,14,22,16,19,21,0,32,33,10,255,0,9,1,1,1,0,0,22,18,20,21,0,35,34
        .byte 13,15,22,16,19,21,0,0,0,5,2,2,2,7,0,43,26,26,0,0,0,43,0,0,22,17,20,21,0,0,0,65,6,6,6
        .byte 65,255,0,8,45,45,22,18,20,21,45,45,0,0,8,8,8,0,45,45,0,45,45,0,45,45
        .byte 73,73,73,75,255,0,4,22,16,19,21,255,0,13,72,255,0,4,22,17,20,21,86,0,85,0,85,0,85,255,0,6,72
        .byte 255,0,4,22,18,20,21,255,0,8,74,73,12,13,15,72,255,0,5,51,36,255,0,9,72,0,73,73,73,71,255,0,5
        .byte 50,30,0,0,0,69,11,11,11,23,46,72,255,0,10,49,31,0,0,0,69,11,55,0,9,68,55,44,255,0,9,43,37
        .byte 0,0,0,69,11,11,23,0,29,72,46,255,0,4,14,12,13,15,14,47,48,15,14,12,13,15,0,8,0,0,72,9,255,0,7
        .byte 45,45,74,255,73,4,75,53,0,8,0,0,72,64,255,0,7,2,2,72,65,0,0,65,72,52,0,29,0,0,70,255,73,4
        .byte 75,0,44,11,6,6,72,83,79,80,83,72,49,255,0,9,72,0,32,33,29,8,72,83,81,82,83,72,65,255,0,7,46
        .byte 0,72,0,35,34,43,29,72,65,84,84,65,72,22,16,19,21,0,0,0,11,10,0,72,0,0,27,27,0,70,255,73,4
        .byte 71,22,17,20,21,255,0,6,72,0,22,16,19,21,255,0,5,43,22,18,20,21,0,0,46,73,73,73,71,0,22,18,20
        .byte 21,74,255,73,4,75,255,0,6,9,255,0,5,43,8,26,0,72,255,0,4,72,255,0,12,29,29,0,0,72,255,0,4
        .byte 72,255,0,6,46,255,0,9,72,255,0,4,72,255,0,6,29,0,0,36,255,0,6,72,255,0,4,72,0,0,0,36,255,0,5
        .byte 30,0,0,32,33,0,44,255,66,6,23,0,0,30,0,0,42,45,45,31,45,45,35,34,44,11,255,67,6,28,45,45,31
        .byte 45,45,8
        .byte 0,0,70,65,58,65,255,54,10,65,58,65,71,255,0,6,72,59,54,54,16,255,78,5,54,54,59,72,255,0,8,72,64
        .byte 54,54,17,56,0,56,56,56,54,54,64,72,255,0,8,72,0,54,54,17,56,77,56,56,56,54,76,0,72,255,0,8,70
        .byte 255,73,12,71,255,0,136,11,23,255,0,20,28,28,23,255,0,16,27,27,27,28,28,28,23,255,0,14,22,16,25,19,255,27,4
        .byte 255,0,14,22,17,54,20,16,25,25,19,255,0,14,22,18,24,20,18,24,24,20,0,51,88,255,0,8,88,53,0,43,255,26,7
        .byte 0,50,0,255,66,8,0,52,0,0,9,28,28,69,69,88,88,36,49,36,255,67,8,0,49,0,36,0,9,28,36,87,87
        .byte 69,30,0,30,0,255,2,6,14,12,13,15,30,0,0,9,30,87,87,69,31,43,31,0,255,6,6,14,47,48,15,31,32
        .byte 33,43,31,32,33,69,37,43,37,0,255,8,6,255,0,4,37,35,34,0
        .byte 87,87,88,255,0,5,255,26,6,255,0,8,87,87,88,255,0,19,88,88,88,255,0,20,255,88,4,0,0,88,255,0,6
        .byte 88,255,0,8,88,0,0,88,0,0,0,255,1,6,255,0,9,88,0,0,88,0,0,5,255,2,6,7,255,0,8,255,88,4
        .byte 0,0,65,255,6,6,65,255,0,8,88,0,0,88,0,0,0,42,29,42,8,29,8,36,255,0,8,88,0,0,88,0,0,0
        .byte 255,27,6,30,255,0,8,255,88,4,0,0,36,16,255,25,4,19,31,255,0,8,88,0,0,88,0,0,30,17,255,54,4
        .byte 20,37,255,0,8,88,0,0,88,0,0,31,18,255,24,5,255,0,9,255,88,4,0,0,37,255,26,6,255,0,9,88,0,0
        .byte 88,255,0,18,88,0,0,88,255,0,18,255,88,4,255,0,18,88,0,0,88,255,0,18,88,0,0,88,255,0,18,255,88,4
        .byte 255,0,35,38,40,11,11,255,0,18,41,39,11,11,255,0,18,8,8,0,0
numsc:
        .byte 22
.out .sprintf("SCREENS SIZE = %d", (* - scdat))

nmedat:
        .byte 0,1,136,160,2,3,80,24,2,7,40,112,3,5,48,168,3,6,104,8,4,8,32,8,255
        .byte 0,0,160,8,1,2,160,128,2,3,112,64,3,4,24,168,3,5,80,16,5,7,48,128,4,8,8,8,255
        .byte 0,0,136,8,5,7,136,152,3,5,64,56,3,6,48,136,2,7,40,32,4,8,8,8,255
        .byte 0,0,160,168,3,4,56,136,3,6,112,8,5,7,120,32,5,7,88,152,4,8,8,168,5,3,40,40,255
        .byte 0,0,144,8,3,4,88,8,3,5,152,88,6,9,88,88,2,3,96,160,2,3,16,32,4,8,8,168,255
        .byte 0,0,8,8,3,5,136,16,5,7,64,40,2,7,72,56,6,9,64,96,3,6,40,72,4,8,8,56,255
        .byte 0,0,16,8,5,10,56,72,5,10,88,8,5,10,152,48,3,4,104,16,3,5,104,80,4,8,8,72,5,10,40,136,255
        .byte 0,0,8,8,2,7,48,32,6,9,24,96,5,10,160,72,5,7,72,112,4,8,56,136,3,6,112,8,3,5,8,96,255
        .byte 0,0,120,24,3,5,80,16,2,7,96,112,3,6,8,168,5,10,16,16,4,8,160,168,6,9,104,144,6,9,136,168,255
        .byte 0,1,16,168,2,3,168,40,5,3,64,128,3,5,80,168,4,8,136,168,2,3,8,24,3,6,8,8,255
        .byte 0,0,136,8,2,11,168,56,4,8,8,8,2,11,72,48,2,11,104,120,2,11,8,120,2,3,48,88,3,4,88,8,3,5,64,168,255
        .byte 0,0,152,24,2,7,8,8,2,11,136,40,6,9,16,72,6,9,8,96,3,5,16,24,3,6,88,112,4,8,48,88,255
        .byte 0,0,152,8,5,7,128,72,2,7,64,144,3,5,8,160,3,4,40,64,2,7,24,16,4,8,8,88,255
        .byte 0,0,24,16,4,8,152,144,5,11,16,48,6,9,64,160,5,10,120,160,6,9,136,16,3,4,104,160,3,5,160,56,2,3,120,16,255
        .byte 0,0,128,40,2,3,120,64,2,3,104,80,2,3,64,96,2,3,80,112,7,12,152,136,3,4,40,8,4,8,24,40,3,4,32,168,6,9,88,144,255
        .byte 0,0,88,24,7,11,8,80,7,11,8,120,5,10,152,40,6,9,120,72,3,6,24,8,3,5,136,128,4,8,152,160,255
        .byte 0,1,160,16,3,6,16,8,2,3,8,72,2,3,144,72,6,9,56,16,3,4,128,136,4,8,8,136,5,10,160,120,255
        .byte 0,0,160,8,7,12,120,48,5,7,56,160,4,8,8,72,3,4,24,24,3,6,8,168,255
        .byte 0,0,136,104,7,7,8,168,3,5,8,144,3,5,40,40,4,8,8,8,2,3,16,64,6,9,88,56,5,11,144,32,6,13,144,72,255
        .byte 0,0,16,8,5,10,72,40,5,10,48,8,5,11,152,8,3,5,32,64,3,6,48,96,6,9,112,160,6,13,88,136,4,8,88,96,2,12,8,144,255
        .byte 0,0,160,16,8,0,128,88,7,4,128,80,7,5,120,96,7,6,112,88,255
        .byte 0,0,8,8,8,0,24,88,7,3,64,24,7,7,88,24,7,10,112,24,7,11,136,24,7,12,40,24,3,5,40,152,3,4,64,152,3,6,88,152,255
.out .sprintf("POSITIONS SIZE = %d", (* - nmedat))

NUMOBJ = 3
objdta:
        .byte 1,128,15,240,31,88,22,168,31,88,22,168,25,248,25,248,25,248,25,248,25,248,25,248,25,248,25,248,12,240,0,0,6,91,124,6,91,124
        .byte 1,128,15,240,31,88,22,168,31,88,22,168,25,248,25,248,25,248,25,248,25,248,25,248,25,248,25,248,12,240,0,0,11,136,96,11,136,96
        .byte 1,128,15,240,31,88,22,168,31,88,22,168,25,248,25,248,25,248,25,248,25,248,25,248,25,248,25,248,12,240,0,0,21,160,24,21,160,24
.out .sprintf("OBJECTS SIZE = %d", (* - objdta))

font:
        .byte 62,50,71,205,236,126,201,0
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
        .byte 32,112,32,255,4,20,60,16
        .byte 0,0,0,0,0,56,56,0
        .byte 0,0,6,12,24,48,96,0
        .byte 254,254,206,214,230,254,254,0
        .byte 120,120,24,24,24,126,126,0
        .byte 252,254,6,124,192,254,254,0
        .byte 252,254,6,124,6,254,252,0
        .byte 192,216,216,216,254,254,24,0
        .byte 254,254,192,252,6,254,252,0
        .byte 126,254,192,252,198,254,124,0
        .byte 254,254,14,28,56,48,48,0
        .byte 124,254,198,124,198,254,124,0
        .byte 124,254,198,126,6,126,124,0
        .byte 0,0,48,48,0,48,48,0
        .byte 0,0,48,48,0,48,48,96
        .byte 0,0,12,24,48,24,12,0
        .byte 0,0,0,126,0,126,0,0
        .byte 0,0,48,24,12,24,48,0
        .byte 0,124,198,12,24,0,24,0
        .byte 0,124,222,254,254,192,124,0
        .byte 254,254,198,254,254,198,198,0
        .byte 252,254,198,252,198,254,252,0
        .byte 254,254,198,192,198,254,254,0
        .byte 252,254,198,198,198,254,252,0
        .byte 254,254,192,252,192,254,254,0
        .byte 254,254,192,252,192,192,192,0
        .byte 254,254,192,222,198,254,254,0
        .byte 198,198,198,254,254,198,198,0
        .byte 126,126,24,24,24,126,126,0
        .byte 62,62,6,6,198,254,254,0
        .byte 198,206,220,248,220,206,198,0
        .byte 192,192,192,192,192,254,254,0
        .byte 130,198,238,254,214,198,198,0
        .byte 134,198,230,246,222,206,198,0
        .byte 126,254,198,198,198,254,252,0
        .byte 252,254,198,254,252,192,192,0
        .byte 126,254,198,198,218,236,246,0
        .byte 252,254,198,254,252,206,198,0
        .byte 126,254,192,124,6,254,252,0
        .byte 254,254,48,48,48,48,48,0
        .byte 198,198,198,198,198,254,124,0
        .byte 198,198,198,198,238,124,56,0
        .byte 198,198,198,198,214,254,108,0
        .byte 198,238,124,56,124,238,198,0
        .byte 198,198,254,126,6,254,252,0
        .byte 254,254,28,56,112,254,254,0
        .byte 0,30,24,24,24,24,30,0
        .byte 0,0,192,96,48,24,12,0
        .byte 0,240,48,48,48,48,240,0
        .byte 0,48,120,252,48,48,48,0
        .byte 0,0,0,0,0,0,0,255
        .byte 0,60,102,248,96,96,254,0
        .byte 24,24,24,120,120,24,24,24
        .byte 24,24,28,15,7,0,0,0
        .byte 0,24,24,255,255,0,0,0
        .byte 0,60,102,195,195,102,60,0
        .byte 24,24,56,240,224,0,0,0
        .byte 24,60,102,90,90,102,60,24
        .byte 24,24,24,30,30,24,24,24
        .byte 0,0,0,224,240,56,24,24
        .byte 0,60,102,219,219,102,60,0
        .byte 0,0,0,7,15,28,24,24
        .byte 96,102,102,124,126,103,99,0
        .byte 146,146,146,146,146,146,146,0
        .byte 175,175,175,175,175,175,175,0
        .byte 255,255,255,255,255,255,255,0
        .byte 245,245,245,245,245,245,245,0
        .byte 73,73,73,73,73,73,73,0
        .byte 0,0,0,0,0,0,0,0
        .byte 232,210,232,210,232,210,232,210
        .byte 23,75,23,75,23,75,23,75
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
        .byte 254,130,186,162,186,130,254,0
.out .sprintf("FONT SIZE = %d", (* - font))

jtab:
        .byte 248,250,252,254,254,255,255,255,0,0,0,1,1,1,2,2,4,6,8,8,8,99
keys:   .byte 66,97,104,72,73,16,55,48,49,17,18
.out .sprintf("KEYS SIZE = %d", (* - keys))

data_end:
.out .sprintf("TOTAL DATA SIZE = %d", (* - data_start))
