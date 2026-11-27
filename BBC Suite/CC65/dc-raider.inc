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
WINDOWWID = 30 ;面面面面面面面面面面面面
MAPWID = 7
        .byte 255,255,255,255,255,255,255
mapdat:
        .byte 255,0,8,16,24,32,255,255,1,9,17,25,33,255,255,2,10,18,26,34,255,255,3,11,19,27,35,255,255,4,12,20,28,36,255,255,5,13,21,29,37,255,255,6,14,22,30,38,255,255,7,15,23,31,39,255
        .byte 255,255,255,255,255,255,255
stmap:  .byte 1

evnt00:
        lda #0
        cmp varl
        beq *+5
        jmp a01040
        ; lda #0
        sta vard
        jsr laddd
        beq :+
        jmp a00078
:
        lda #29
        cmp varh
        bcs *+5
        jmp a00078
        lda #200
        asl a
        sta sndtyp
        lda #30
        sta varh
a00078: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a00144
:
        lda #0
        cmp varc
        beq *+5
        jmp a00144
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a00144: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a00210
:
        lda #0
        cmp varc
        beq *+5
        jmp a00210
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a00210: jsr skobj
        sta varobj
        lda #255
        cmp varobj
        bne *+5
        jmp a00268
        lda varobj
        jsr getob
        lda #<10
        sta z80_l
        lda #>10
        sta z80_h
        jsr addsc
a00268: lda #164
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a00572
        lda #0
        sta vark
        lda #7
        cmp scno
        beq *+5
        jmp a00359
        lda #16
        ; ldy #8
        sta (z80_ix),y
        lda #2
        sta varp
        tya; lda #8
        sta scno
        jsr nwscr
        lda #1
        sta vark
a00359: lda #15
        cmp scno
        beq *+5
        jmp a00419
        lda #16
        ldy #8
        sta (z80_ix),y
        lda #3
        sta varp
        lda #16
        sta scno
        jsr nwscr
        lda #1
        sta vark
a00419: lda #23
        cmp scno
        beq *+5
        jmp a00478
        lda #16
        ldy #8
        sta (z80_ix),y
        lda #4
        sta varp
        lda #24
        sta scno
        jsr nwscr
        lda #1
        sta vark
a00478: lda #31
        cmp scno
        beq *+5
        jmp a00537
        lda #16
        ldy #8
        sta (z80_ix),y
        lda #5
        sta varp
        lda #32
        sta scno
        jsr nwscr
        lda #1
        sta vark
a00537: lda #16
        ldy #8
        sta (z80_ix),y
        lda #0
        cmp vark
        beq *+5
        jmp a00572
        jsr scrd
a00572: jsr cangd
        beq :+
        jmp a00629
:
        lda #0
        cmp varc
        beq *+5
        jmp a00624
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a00624: jmp a00653
a00629: lda #0
        cmp varh
        beq *+5
        jmp a00653
        lda #1
        sta varl
a00653: lda joyval
        and #1
        beq :+
        jmp a00720
:
        jsr cangr
        beq :+
        jmp a00720
:
        jsr cangd
        beq :+
        jmp a00720
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a00720: lda joyval
        and #2
        beq :+
        jmp a00786
:
        jsr cangl
        beq :+
        jmp a00786
:
        jsr cangd
        beq :+
        jmp a00786
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a00786: lda joyval
        and #8
        beq :+
        jmp a00926
:
        lda #0
        cmp varh
        bcc *+5
        jmp a00926
        lda #1
        sta vard
        jsr cangu
        beq :+
        jmp a00926
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #0
        cmp varg
        beq *+5
        jmp a00926
        ; lda #0
        cmp varh
        bne *+5
        jmp a00926
        lda varh
        sec
        sbc #1
        sta varh
        lda #20
        asl a
        sta sndtyp
a00926: lda joyval
        and #16
        beq :+
        jmp a00990
:
        lda #0
        cmp vare
        beq *+5
        jmp a00990
        lda #1
        sta vare
        lda #2
        sta z80_c
        ; lda #2
        sta z80_b
        jsr spawn
a00990: ldy #8
        lda (z80_ix),y
        sta vara
        iny; ldy #9
        lda (z80_ix),y
        sta varb
        jsr lcol
        bcs :+
        jmp a01040
:
        lda #1
        sta varl
a01040: lda #1
        cmp varl
        beq *+5
        jmp a01144
        lda #4
        sta loopa
a01065: lda #30
        asl a
        sta sndtyp
        lda #6
        jsr explod
        lda #20
        jsr delay
        lda vara
        ldy #8
        sta (z80_ix),y
        lda varb
        iny; ldy #9
        sta (z80_ix),y
        dec loopa
        beq :+
        jmp a01065
:
        lda #1
        sta deadf
a01144: lda #2
        cmp varl
        beq *+5
        jmp a01366
        lda #0
        cmp varc
        beq *+5
        jmp a01325
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01227
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a01227: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01276
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a01276: jsr cangd
        beq :+
        jmp a01317
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp a01325
a01317: lda #1
        sta gamwon
a01325: lda #164
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a01366
        lda #20
        ; ldy #8
        sta (z80_ix),y
        jsr scrd
a01366: rts
.out .sprintf("EVENT 0 SIZE = %d", (* - evnt00))

evnt01:
        lda #1
        cmp vard
        beq *+5
        jmp b00097
        lda vara
        ldy #8
        sta (z80_ix),y
        lda varb
        iny; ldy #9
        sta (z80_ix),y
        lda #14
        sta z80_c
        dey; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #0
        jsr animsp
        jmp b00125
b00097: lda #240
        ldy #8
        sta (z80_ix),y
        lda #0
        iny; ldy #9
        sta (z80_ix),y
b00125: rts
.out .sprintf("EVENT 1 SIZE = %d", (* - evnt01))

evnt02:
        jsr cangd
        beq :+
        jmp c00124
:
        ldy #8
        lda (z80_ix),y
        sta varf
        lda varf
        sta z80_d
        lda #10
        sta z80_e
        jsr idiv
        lda z80_d
        sta varf
        lda #0
        cmp varc
        beq *+5
        jmp c00097
        lda varf
        asl a
        sta sndtyp
        lda #0
        jsr animsp
c00097: ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp c00154
c00124: lda #2
        jsr explod
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #0
        sta vare
c00154: jsr skobj
        sta varobj
        lda #255
        cmp varobj
        bne *+5
        jmp c00199
        lda varobj
        jsr getob
        lda #6
        jsr explod
c00199: rts
.out .sprintf("EVENT 2 SIZE = %d", (* - evnt02))

evnt03:
        lda #0
        cmp varl
        beq *+5
        jmp d00248
        lda #10
        cmp scno
        bcc *+5
        jmp d00108
        jsr cangu
        beq :+
        jmp d00076
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp d00103
d00076: lda #2
        jsr explod
        ldy #11
        lda (z80_ix),y
        ldy #8
        sta (z80_ix),y
d00103: jmp d00219
d00108: jsr cangu
        beq :+
        jmp d00172
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        dey; ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp d00219
d00172: lda #2
        jsr explod
        ldy #11
        lda (z80_ix),y
        ldy #8
        sta (z80_ix),y
        ldy #12
        lda (z80_ix),y
        ldy #9
        sta (z80_ix),y
d00219: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00248
:
        lda #1
        sta varl
d00248: rts
.out .sprintf("EVENT 3 SIZE = %d", (* - evnt03))

evnt04:
        lda #0
        cmp varl
        beq *+5
        jmp e00527
        ; lda #0
        cmp varc
        beq *+5
        jmp e00409
        ; lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp e00129
        jsr cangl
        beq :+
        jmp e00099
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp e00124
e00099: lda #6
        asl a
        sta sndtyp
        lda #1
        ldy #11
        sta (z80_ix),y
e00124: jmp e00195
e00129: jsr cangr
        beq :+
        jmp e00169
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp e00195
e00169: lda #6
        asl a
        sta sndtyp
        lda #0
        ldy #11
        sta (z80_ix),y
e00195: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e00287
        jsr cangu
        beq :+
        jmp e00257
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp e00283
e00257: lda #1
        ldy #10
        sta (z80_ix),y
        lda #6
        asl a
        sta sndtyp
e00283: jmp e00353
e00287: jsr cangd
        beq :+
        jmp e00328
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp e00353
e00328: lda #6
        asl a
        sta sndtyp
        lda #0
        ldy #10
        sta (z80_ix),y
e00353: lda #0
        cmp varm
        beq *+5
        jmp e00379
        ; lda #0
        jsr animsp
e00379: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00409
:
        lda #1
        sta varl
e00409: lda #2
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00527
:
        lda #6
        jsr explod
        lda #255
        ldy #5
        sta (z80_ix),y
        lda vari
        sec
        sbc #1
        sta vari
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ; ldy #5
        sta (z80_ix),y
        lda #0
        sta vare
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
e00527: rts
.out .sprintf("EVENT 4 SIZE = %d", (* - evnt04))

evnt05:
        lda #0
        cmp varg
        beq *+5
        jmp f00115
        lda varj
        cmp vari
        bcs *+5
        jmp f00115
        lda #26
        cmp scno
        bcc *+5
        jmp f00078
        lda #4
        sta z80_c
        lda #9
        sta z80_b
        jsr spawn
        jmp f00099
f00078: lda #4
        sta z80_c
        lda #6
        sta z80_b
        jsr spawn
f00099: lda vari
        clc
        adc #1
        sta vari
f00115: rts
.out .sprintf("EVENT 5 SIZE = %d", (* - evnt05))

evnt06:
        lda #0
        cmp varl
        beq *+5
        jmp g00248
        lda #21
        cmp scno
        bcs *+5
        jmp g00143
        lda #2
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00139
:
        lda #6
        jsr explod
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
        sta vare
g00139: jmp g00218
g00143: jsr cangd
        beq :+
        jmp g00184
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp g00218
g00184: lda #20
        asl a
        sta sndtyp
        lda #2
        jsr explod
        lda #40
        ldy #8
        sta (z80_ix),y
g00218: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00248
:
        lda #1
        sta varl
g00248: rts
.out .sprintf("EVENT 6 SIZE = %d", (* - evnt06))

evnt07:
        lda #0
        cmp varg
        beq *+5
        jmp h00039
        lda #1
        jsr shoot
        lda #40
        asl a
        sta sndtyp
h00039: rts
.out .sprintf("EVENT 7 SIZE = %d", (* - evnt07))

evnt08:
        lda #0
        cmp varc
        beq *+5
        jmp i00366
        ; lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp i00206
        jsr cangl
        beq :+
        jmp i00188
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #0
        jsr animsp
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp i00148
:
        lda #1
        ldy #11
        sta (z80_ix),y
i00148: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp i00201
i00188: lda #1
        ldy #11
        sta (z80_ix),y
i00201: jmp i00366
i00206: jsr cangr
        beq :+
        jmp i00353
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #0
        jsr animbk
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp i00313
:
        lda #0
        ldy #11
        sta (z80_ix),y
i00313: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp i00366
i00353: lda #0
        ldy #11
        sta (z80_ix),y
i00366: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i00396
:
        lda #1
        sta varl
i00396: rts
.out .sprintf("EVENT 8 SIZE = %d", (* - evnt08))

evnt09:
        lda #3
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00195
        lda #150
        ldy #11
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y
        ldy #12
        sta (z80_ix),y
        lda #1
        cmp scno
        beq *+5
        jmp j00104
        lda #118
        dey; ldy #11
        sta (z80_ix),y
        lda #74
        iny; ldy #12
        sta (z80_ix),y
j00104: lda #2
        cmp scno
        beq *+5
        jmp j00150
        lda #140
        ldy #11
        sta (z80_ix),y
        lda #20
        iny; ldy #12
        sta (z80_ix),y
j00150: lda #3
        cmp scno
        beq *+5
        jmp j00195
        lda #100
        ldy #11
        sta (z80_ix),y
        lda #50
        iny; ldy #12
        sta (z80_ix),y
j00195: rts
.out .sprintf("EVENT 9 SIZE = %d", (* - evnt09))

evnt10:
        lda #1
        cmp varc
        beq *+5
        jmp k00031
        lda #0
        sta varc
        jmp k00046
k00031: lda varc
        clc
        adc #1
        sta varc
k00046: lda #1
        cmp varm
        beq *+5
        jmp k00076
        lda #0
        sta varm
        jmp k00091
k00076: lda varm
        clc
        adc #1
        sta varm
k00091: rts
.out .sprintf("EVENT 10 SIZE = %d", (* - evnt10))

evnt11:
        lda #1
        sta chary
        ; lda #1
        sta charx
        ; lda #1
        jsr dmsg
        lda #1
        sta chary
        lda #4
        sta charx
        jsr dscor
        lda #1
        sta chary
        lda #11
        sta charx
        lda #2
        jsr dmsg
        lda #1
        sta chary
        lda #18
        sta charx
        lda varp
        jsr disply
        lda #1
        sta chary
        lda #20
        sta charx
        lda #3
        jsr dmsg
        lda #1
        sta chary
        lda #9
        cmp varh
        bcc *+5
        jmp l00162
        lda #25
        sta charx
        jmp l00195
l00162: lda #25
        sta charx
        lda #5
        jsr dmsg
        lda #1
        sta chary
        lda #26
        sta charx
l00195: lda varh
        jsr disply
        lda #1
        sta chary
        lda #28
        sta charx
        lda #4
        jsr dmsg
        lda #1
        sta chary
        lda #30
        sta charx
        lda numlif
        jsr disply
        lda #30
        cmp varg
        beq *+5
        jmp l00285
        lda #0
        sta varg
        jmp l00300
l00285: lda varg
        clc
        adc #1
        sta varg
l00300: rts
.out .sprintf("EVENT 11 SIZE = %d", (* - evnt11))

evnt12:
        jsr cls
        lda #4
        sta chary
        lda #8
        sta charx
        lda #6
        jsr dmsg
        lda #5
        sta chary
        ; lda #5
        sta charx
        lda #15
        jsr dmsg
        lda #8
        sta chary
        lda #6
        sta charx
        lda #7
        jsr dmsg
        lda #10
        sta chary
        lda #4
        sta charx
        lda #8
        jsr dmsg
        lda #11
        sta chary
        lda #4
        sta charx
        lda #9
        jsr dmsg
        lda #12
        sta chary
        lda #4
        sta charx
        lda #10
        jsr dmsg
        lda #18
        sta chary
        lda #9
        sta charx
        lda #11
        jsr dmsg
        jsr prskey
        rts
.out .sprintf("EVENT 12 SIZE = %d", (* - evnt12))

evnt13:
        lda #5
        sta numlif
        jsr cls
        lda #0
        sta vara
        ; lda #0
        sta varb
        ; lda #0
        sta varc
        ; lda #0
        sta vard
        ; lda #0
        sta vare
        ; lda #0
        sta varf
        ; lda #0
        sta varg
        lda #30
        sta varh
        lda #0
        sta vari
        lda #2
        sta varj
        lda #0
        sta vark
        ; lda #0
        sta varl
        ; lda #0
        sta varm
        lda #1
        sta varp
        rts
.out .sprintf("EVENT 13 SIZE = %d", (* - evnt13))

evnt14:
        lda #0
        sta vare
        ; lda #0
        sta vari
        lda #2
        sta varj
        lda #5
        cmp scno
        beq *+5
        jmp o00051
        lda #3
        sta varj
o00051: lda #8
        cmp scno
        beq *+5
        jmp o00076
        lda #1
        sta varj
o00076: lda #2
        sta chary
        lda #1
        sta charx
        ; lda #1
        jsr dmsg
        lda #38
        cmp scno
        beq *+5
        jmp o00134
        lda #0
        sta varh
        lda #2
        sta varl
o00134: rts
.out .sprintf("EVENT 14 SIZE = %d", (* - evnt14))

evnt15:
        rts
.out .sprintf("EVENT 15 SIZE = %d", (* - evnt15))

evnt16:
        lda #50
        asl a
        sta sndtyp
        lda #0
        sta varl
        lda #30
        sta varh
        lda numlif
        sec
        sbc #1
        sta numlif
        rts
.out .sprintf("EVENT 16 SIZE = %d", (* - evnt16))

evnt17:
        lda #50
        jsr delay
        lda #10
        sta chary
        lda #12
        sta charx
        lda #0
        jsr dmsg
        lda #250
        jsr delay
        rts
.out .sprintf("EVENT 17 SIZE = %d", (* - evnt17))

evnt18:
        lda #5
        sta chary
        lda #1
        sta charx
        lda #12
        jsr dmsg
        lda #9
        sta chary
        lda #1
        sta charx
        lda #13
        jsr dmsg
        lda #12
        sta chary
        lda #1
        sta charx
        lda #14
        jsr dmsg
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
        .byte "SC:",141
        .byte "PLANET:",141
        .byte "FUEL:",141
        .byte "L:",141
        .byte "0",141
        .byte "DEEP CORE RAIDER",141
        .byte "SELECT CONTROL METHOD",141
        .byte "1.....KEYBOARD (Q,O,P,SP)",141
        .byte "2................KEMPSTON",141
        .byte "3.......SINCLAIR JOYSTICK",141
        .byte "c  P.JENKINSON",141
        .byte "THE PROBLEMS OF VICTORY ARE",13
        .byte " MORE AGREEABLE THAN THOSE OF",13
        .byte " DEFEAT, BUT THEY ARE NO LESS",13
        .byte " DIFFICULT.",141
        .byte "WINSTON CHURCHILL",141
        .byte "CONGRATULATIONS",13
        .byte " PRESS ANY KEY",141
        .byte "----------------------",141
nummsg:
        .byte 16
.out .sprintf("MESSAGE SIZE = %d", (* - msgdat))

chgfx:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,3,15,6,2,6,11,6,3,6,15,3,15,0,0
        .byte 0,4,15,2,1,10,11,10,11,10,3,10,15,2,0,4
        .byte 15,15,8,1,10,11,9,5,10,11,9,5,10,11,15,15
        .byte 90,90,37,36,18,0,32,0,0,0,0,0,0,0,0,0
        .byte 88,26,36,164,82,0,32,0,0,0,0,0,0,0,0,0
        .byte 187,221,51,68,17,136,0,0,0,0,0,0,0,0,0,0
        .byte 128,0,64,0,160,0,80,0,160,128,16,64,32,160,16,80
        .byte 0,1,0,2,0,5,0,10,1,5,2,8,5,4,10,8
        .byte 165,165,10,10,5,5,2,2,0,0,0,64,0,0,0,0
        .byte 0,0,0,0,32,0,0,8,0,1,80,64,160,160,90,90
        .byte 0,161,0,80,0,161,64,80,0,33,0,80,32,161,0,80
        .byte 160,0,88,0,160,0,72,64,160,0,88,0,160,128,88,0
        .byte 0,0,0,0,0,0,0,0,8,3,4,4,4,4,4,4
        .byte 112,240,112,240,112,240,112,240,112,240,112,240,112,240,0,0
        .byte 0,0,0,0,102,34,119,119,85,170,102,204,119,238,102,51
        .byte 144,80,176,16,112,240,0,240,16,112,32,0,0,0,112,0
        .byte 176,112,144,48,240,224,240,0,224,128,0,64,0,0,0,224
        .byte 0,0,0,0,16,240,112,240,240,240,144,80,176,80,176,80
        .byte 0,0,0,0,240,128,240,96,240,176,144,80,176,80,144,112
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 100,177,98,50,117,245,64,80,68,117,80,80,0,0,250,250
        .byte 128,128,216,200,228,128,186,0,145,128,250,200,0,0,250,250
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 128,1,80,2,32,132,64,2,4,32,10,16,32,36,16,24
        .byte 164,0,10,0,4,0,82,72,0,4,0,64,0,160,0,74
        .byte 0,164,0,10,0,164,0,0,0,4,66,64,5,132,66,0
        .byte 128,5,2,10,1,4,10,8,5,0,10,0,4,0,8,0
        .byte 160,1,80,64,32,128,16,80,0,160,0,80,0,32,0,16
        .byte 64,0,0,0,64,0,132,0,8,0,0,0,0,0,0,0
        .byte 0,2,0,0,0,2,0,33,0,16,0,0,0,0,0,0
        .byte 85,170,85,204,85,170,85,204,85,170,85,204,85,170,85,204
        .byte 17,102,34,238,17,102,34,238,17,102,34,238,17,102,34,238
        .byte 0,0,74,74,160,128,10,72,160,128,10,74,0,0,0,0
        .byte 0,0,10,74,160,4,64,64,160,4,10,64,0,0,0,0
        .byte 0,0,64,82,4,4,64,66,4,4,82,82,0,0,0,0
        .byte 0,0,80,82,5,4,80,66,32,4,2,82,0,0,0,0
        .byte 0,0,64,0,4,0,64,0,4,0,82,0,0,0,0,0
        .byte 1,0,16,0,1,0,16,0,129,32,80,64,33,128,16,0
        .byte 0,0,255,255,0,0,85,85,170,170,85,85,255,255,0,0
        .byte 0,0,0,0,0,0,0,8,160,0,64,0,160,128,80,0
        .byte 0,0,0,0,0,0,0,8,0,1,0,10,0,5,0,10
        .byte 5,0,10,0,5,0,8,0,4,0,0,0,0,0,0,0
        .byte 0,160,0,80,0,160,0,16,1,32,0,0,0,0,0,0
        .byte 240,240,240,224,112,192,176,128,208,0,224,0,192,0,128,0
        .byte 240,224,112,208,48,176,16,112,0,240,0,112,0,48,0,16
        .byte 240,240,128,16,160,176,144,80,160,176,144,80,160,176,240,240
        .byte 208,0,160,64,208,0,160,64,208,0,160,64,208,0,160,64
        .byte 0,0,0,0,80,80,0,0,160,160,80,80,160,160,240,240
        .byte 240,240,80,80,160,160,80,80,0,0,160,160,0,0,0,0
        .byte 128,0,192,0,224,0,240,0,224,128,208,192,176,224,112,240
        .byte 0,16,0,48,0,112,0,240,16,112,48,176,112,208,240,224
        .byte 32,80,0,176,32,80,0,176,32,80,0,176,32,80,0,176
        .byte 0,160,0,80,16,32,0,16,0,64,0,0,0,0,0,0
        .byte 240,224,112,208,176,176,80,112,160,240,80,112,32,176,144,80
        .byte 112,240,176,224,208,208,224,160,240,80,224,160,208,64,160,144
        .byte 80,0,160,0,64,128,128,0,32,0,0,0,0,0,0,0
        .byte 144,80,32,176,80,112,160,240,80,112,176,176,112,208,240,224
        .byte 0,0,0,0,0,0,0,64,0,16,16,32,0,80,0,160
        .byte 160,144,208,64,224,160,240,80,224,160,208,208,176,224,112,240
        .byte 0,0,0,0,0,0,32,0,128,0,64,128,160,0,80,0
        .byte 48,48,192,192,48,48,192,192,48,48,192,192,48,48,192,192
        .byte 74,72,33,164,90,10,5,164,88,10,37,160,82,66,133,160
        .byte 26,90,165,161,82,10,133,165,90,90,161,133,26,90,165,33
        .byte 18,90,164,161,90,26,37,165,74,74,161,165,90,24,133,165
        .byte 26,90,164,37,8,16,0,0,66,0,129,129,74,82,165,133
        .byte 90,18,37,164,72,82,128,161,18,26,128,37,90,88,161,133
        .byte 80,90,37,133,82,90,36,161,82,26,37,164,64,90,37,133
        .byte 0,0,165,161,18,90,164,165,90,74,161,161,26,82,165,133
        .byte 90,26,37,160,88,90,165,165,74,82,37,133,90,90,0,0
        .byte 8,0,128,0,72,0,37,0,74,8,161,132,90,2,133,165
        .byte 0,16,0,33,0,2,0,36,16,90,33,133,80,82,133,165
        .byte 74,90,161,160,90,72,133,128,80,0,164,0,8,0,0,0
        .byte 88,74,37,161,16,90,0,164,0,80,0,33,0,18,0,1
        .byte 90,90,164,33,72,88,132,5,72,16,164,33,90,90,165,165
        .byte 26,90,161,161,74,90,165,164,88,90,165,165,90,24,37,165
        .byte 68,2,4,5,68,2,4,5,68,2,4,5,68,2,4,5
        .byte 187,170,85,170,187,170,85,170,187,170,85,170,187,170,85,170
        .byte 0,0,114,170,117,196,114,170,117,196,114,170,117,196,0,0
        .byte 0,0,34,170,85,85,34,170,85,85,34,170,85,85,0,0
        .byte 0,0,170,170,85,85,170,170,85,85,170,170,85,85,0,0
        .byte 0,0,26,74,5,164,26,74,5,164,26,74,5,164,0,0
        .byte 0,0,0,0,136,0,68,0,170,0,85,0,187,136,85,170
        .byte 128,1,16,2,4,132,0,66,0,36,0,64,0,132,80,10
        .byte 0,0,0,0,0,0,0,4,0,78,34,4,87,0,34,0
        .byte 10,10,170,170,0,0,39,175,0,85,35,38,19,19,35,0
        .byte 170,170,170,170,0,0,255,238,238,0,170,204,204,204,0,204
        .byte 90,90,165,37,90,90,161,165,90,88,165,165,66,90,165,165
        .byte 4,14,5,6,4,14,5,6,4,14,5,6,4,14,5,6
        .byte 0,0,15,15,0,0,5,5,10,10,15,15,15,15,0,0
        .byte 0,0,7,14,4,2,7,10,7,10,5,10,7,14,0,0
        .byte 4,4,4,4,4,4,0,0,2,15,0,0,4,4,4,4
        .byte 4,4,4,4,3,8,0,0,8,3,4,4,4,4,4,4
        .byte 0,0,114,250,117,245,98,0,100,85,98,170,100,85,98,170
        .byte 0,0,175,175,95,95,0,0,85,85,170,170,85,85,170,170
        .byte 0,0,255,255,255,255,0,0,85,85,170,170,85,85,170,170
        .byte 0,0,90,74,165,164,0,66,4,36,10,66,4,36,10,66
        .byte 100,85,98,170,100,85,98,0,100,0,114,250,117,245,0,0
        .byte 85,85,170,170,85,85,0,0,0,0,175,175,95,95,0,0
        .byte 5,0,4,8,5,4,4,14,5,6,4,14,5,6,4,14
        .byte 4,36,10,66,4,36,0,66,0,36,90,74,165,164,0,0
        .byte 100,85,98,170,100,85,98,170,100,85,98,170,100,85,98,170
        .byte 85,85,170,170,85,85,170,170,85,85,170,170,85,85,170,170
        .byte 85,85,170,170,85,85,170,170,85,85,170,170,85,85,170,170
        .byte 4,36,10,66,4,36,10,66,4,36,10,66,4,36,10,66
        .byte 0,0,175,175,0,0,0,0,0,0,5,5,10,10,5,5
        .byte 170,170,85,85,170,170,255,255,255,255,0,0,255,255,0,0
        .byte 19,0,0,0,19,0,35,35,0,87,35,141,0,0,175,175
        .byte 0,204,0,0,0,204,204,204,238,0,187,204,0,0,255,255
        .byte 80,240,80,240,64,0,64,0,64,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,64,0,64,0,64,0,80,240,80,240
        .byte 0,0,0,0,0,0,0,32,0,32,0,32,240,160,240,160
        .byte 240,160,240,160,0,32,0,32,0,32,0,0,0,0,0,0
        .byte 0,17,0,0,0,0,0,0,0,17,0,0,0,0,0,0
        .byte 32,80,80,32,128,128,128,128,80,32,32,80,128,128,128,128
        .byte 32,80,80,16,128,144,128,144,80,16,32,80,128,144,128,144
        .byte 128,80,144,32,128,128,128,128,144,32,128,80,128,128,128,128
        .byte 32,80,80,32,128,128,128,128,80,32,32,80,0,0,240,240
        .byte 240,240,0,0,0,0,128,128,80,32,32,80,128,128,128,128
        .byte 128,0,64,0,32,0,144,0,64,128,32,64,128,32,128,144
        .byte 0,16,0,32,0,64,0,128,16,0,32,16,64,0,128,128
        .byte 128,80,64,32,32,0,16,0,0,128,0,64,0,32,0,16
        .byte 32,16,80,32,128,64,128,128,16,0,32,0,64,0,128,0
        .byte 0,0,0,0,240,240,0,0,0,0,240,240,0,0,0,0
        .byte 32,64,32,64,32,64,32,64,32,64,32,64,32,64,32,64
        .byte 32,64,32,64,32,64,32,64,32,64,32,64,32,64,0,0
        .byte 0,0,2,0,0,32,0,0,0,0,0,0,4,0,0,0
bprop:
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 6
        .byte 5
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 3
        .byte 3
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 0
        .byte 2
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 3
        .byte 3
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 4
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
        .byte 5
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 2
        .byte 0
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
        .byte 6
        .byte 5
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
        .byte 2
        .byte 2
        .byte 0
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 6
        .byte 5
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 2
.out .sprintf("BLOCKS SIZE = %d", (* - chgfx))

sprgfx:
        .byte 7,224,15,144,31,200,31,200,63,196,63,196,0,0,127,230,0,0,63,204,47,148,23,136,160,5,128,1,128,1,128,1
        .byte 4,160,1,64,2,0,0,128,2,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 10,160,4,64,2,192,5,128,2,128,9,0,2,128,1,0,2,64,1,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 4,160,0,64,1,0,2,128,0,0,1,0,0,0,0,0,0,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 6,192,0,0,5,64,2,128,10,160,5,64,4,64,1,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,48,12,0,0,48,12,48,12,48,12,48,12,0,0,48,12,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,6,96,0,0,6,96,6,96,6,96,6,96,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,1,128,0,0,1,128,1,128,1,128,1,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,6,96,0,0,6,96,6,96,6,96,6,96,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 7,224,15,176,15,208,0,0,31,216,31,216,0,0,31,248,27,248,21,248,11,248,16,0,32,4,0,0,112,14,248,31
        .byte 7,224,15,176,15,208,0,0,31,216,31,216,0,0,31,248,30,248,29,120,29,120,2,0,4,0,0,48,14,120,31,0
        .byte 7,224,15,176,15,208,0,0,31,216,31,216,0,0,31,248,31,184,31,88,31,88,0,64,0,128,24,0,61,192,3,224
        .byte 7,224,15,176,15,208,0,0,31,216,31,216,0,0,31,248,31,232,31,208,31,208,0,16,0,16,48,0,120,56,0,124
        .byte 0,0,0,0,0,0,2,0,7,0,14,0,4,0,32,0,16,8,0,28,0,56,0,16,0,128,0,64,0,0,0,0
        .byte 0,0,48,12,48,12,48,12,0,0,48,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 14,0,25,0,56,128,104,64,111,192,111,176,63,119,30,247,30,240,0,0,30,128,29,64,30,224,0,0,58,188,124,126
        .byte 96,6,3,64,215,173,215,173,215,173,215,173,0,0,103,166,15,208,15,208,39,196,112,14,224,7,82,74,58,92,18,72
        .byte 12,48,3,0,23,104,23,104,23,104,23,104,0,0,7,176,15,192,15,208,39,196,112,14,224,7,82,74,58,92,18,72
        .byte 1,128,0,0,3,64,3,64,3,64,3,64,0,0,5,160,14,80,15,208,39,196,112,14,224,7,82,74,58,92,18,72
        .byte 48,48,3,64,107,168,107,168,107,168,107,168,0,0,55,160,15,208,15,208,39,196,112,14,224,7,82,74,58,92,18,72
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,160,5,144,9,160,5,144,9
        .byte 0,128,0,128,16,132,10,168,5,80,11,232,15,240,247,111,15,240,11,232,5,80,10,168,16,132,0,128,0,128,0,0
        .byte 0,0,0,0,3,192,32,4,7,224,31,56,63,140,63,196,127,230,127,230,0,0,127,242,127,242,0,0,15,216,0,0
        .byte 0,0,0,0,3,192,32,4,7,224,31,56,63,140,63,196,127,230,127,230,0,0,127,62,127,62,0,0,15,216,0,0
        .byte 0,0,0,0,3,192,32,4,7,224,31,56,63,140,63,196,127,230,127,230,0,0,115,254,115,254,0,0,15,216,0,0
        .byte 0,0,0,0,3,192,32,4,7,224,31,56,63,140,63,196,127,230,127,230,0,0,127,254,127,254,0,0,15,216,0,0
frmlst:
        .byte 0,1
        .byte 1,4
        .byte 5,4
        .byte 9,4
        .byte 13,2
        .byte 15,1
        .byte 16,4
        .byte 20,1
        .byte 21,1
        .byte 22,4,26,0
.out .sprintf("SPRITES SIZE = %d", (* - sprgfx))

scdat:
        .word 254,304,353,346,384,285,320,287,396,340,413,415,322,357,419,425,307,373,416,513,323,329,416,431,370,411
        .word 416,358,360,332,400,369,281,246,356,356,427,361,527,79
        .byte 255,0,30,255,6,30,255,0,63,8,7,255,0,27,8,43,11,255,0,26,8,43,8,9,9,7,255,0,17,8,9,7,0,0,0
        .byte 9,7,8,43,25,0,44,7,8,9,7,255,0,13,12,0,11,255,0,4,44,43,255,0,4,44,43,0,11,255,0,13,12
        .byte 8,255,9,4,255,0,10,11,255,0,12,8,9,43,255,0,14,11,255,0,12,12,43,255,0,13,26,0,11,255,0,11,8
        .byte 43,255,0,4,26,255,0,4,42,255,10,6,28,255,0,10,8,43,127,25,255,0,6,42,0,28,0,42,28,255,0,10,31
        .byte 18,19,30,12,127,255,0,12,11,255,0,12,16,17,8,43,255,0,11,20,0,11,255,0,11,8,9,9,7,255,0,12,21,21
        .byte 11,0,0,20,255,0,8,12,0,0,11,255,0,4,27,255,0,9,44,7,0,21,21,255,0,5,8,9,12,255,0,18,44
        .byte 9,9,9,7,0,0,0,8,43,0,0,0,41,255,0,14,27,255,0,5,11,0,0,0,12,255,0,13,42,255,0,9,25
        .byte 0,11,0,0,0,12,0,0,26,255,0,22,11,0,0,0,12,255,0,15
        .byte 255,0,30,255,6,30,255,0,10,11,0,0,0,12,255,0,5,42,255,10,6,41,255,0,7,42,10,10,41,27,11,0,0,0
        .byte 29,255,10,5,28,255,0,6,29,41,255,0,4,25,0,28,0,0,29,10,28,255,0,17,29,41,0,42,10,10,28,255,0,21
        .byte 20,0,0,12,42,28,255,0,24,21,21,0,12,11,255,0,23,8,255,9,4,43,11,255,0,23,12,255,0,5,11,24,255,0,17
        .byte 8,9,7,0,8,9,9,0,0,26,0,11,21,21,255,0,16,12,27,44,9,43,255,0,6,44,9,9,7,255,0,15,12
        .byte 255,0,13,11,255,0,15,29,255,10,7,0,42,255,0,4,44,7,255,0,20,29,41,255,0,5,26,0,11,0,0,0,15
        .byte 255,0,17,29,41,255,0,6,44,9,7,0,22,23,255,0,17,29,41,255,0,6,8,255,9,4,7,255,0,17,12,255,0,5
        .byte 8,43,0,27,0,42,28,255,0,5,8,7,255,0,10,12,0,0,41,0,0,12,0,0,0,42,28,255,0,6,12,44,7
        .byte 255,0,9,12,255,0,8,42,28,255,0,5,8,9,12,0,11,9,9,9,7,0,20,0,0,0,12,255,0,8,11,255,0,5
        .byte 8,43,127,0,0,11,0,27,0,44,7,21,21,0,8,43,255,0,8,11,255,0,5,12,255,0,9,44,9,9,9,43,0,0
        .byte 255,0,30,255,6,30,0,42,10,10,10,41,0,11,255,0,5,12,255,0,6,42,255,10,7,41,0,42,28,0,0,0,29
        .byte 10,28,255,0,5,29,41,0,26,0,0,42,28,255,0,7,12,0,11,255,0,13,29,41,0,0,42,28,255,0,4,20,0,0,0
        .byte 12,0,11,255,0,14,29,41,42,28,255,0,5,21,21,0,8,43,0,11,255,0,15,29,28,255,0,5,8,9,9,9,43
        .byte 0,0,11,0,20,255,0,20,29,255,10,4,41,0,44,7,21,21,8,9,7,255,0,21,29,41,0,44,9,9,43,10,28
        .byte 255,0,22,12,25,0,24,0,28,255,0,24,12,0,0,42,28,255,0,24,8,43,0,0,44,7,255,0,21,8,9,9,43
        .byte 0,0,27,0,28,255,0,6,24,255,0,14,12,0,0,27,0,0,0,28,255,0,7,24,255,0,14,29,41,0,0,0,42
        .byte 28,255,0,6,8,9,9,7,255,0,14,29,41,0,0,11,255,0,7,12,43,0,11,0,20,255,0,13,29,41,0,11,15
        .byte 0,0,0,8,7,8,12,0,0,11,7,21,21,0,0,0,8,9,9,9,7,255,0,5,12,0,11,22,23,0,8,43,44
        .byte 7,12,0,0,11,44,9,9,7,0,0,12,0,26,127,44,7,255,0,4,12,0,44,9,9,9,43,127,0,44,255,9,5
        .byte 7,0,44,7,0,12,0,0,0,127,44,7,0,0,0,12,255,0,11,27,0,0,44,7,21,11,8,43,0,0,0,25,0
        .byte 11,0,0,0,12,255,0,15,44,9,9,43,255,0,6,11,0,0,0,12,0
        .byte 255,0,30,255,6,30,255,0,11,25,255,0,5,43,255,0,4,10,10,28,255,0,4,12,0,0,0,42,255,10,12,255,0,5
        .byte 28,255,0,7,12,0,0,42,28,255,0,12,29,10,10,10,28,255,0,8,12,0,0,11,255,0,25,8,43,0,26,11,255,0,24
        .byte 8,43,0,0,0,11,255,0,23,8,43,127,25,0,0,28,255,0,22,8,43,127,0,0,42,28,255,0,10,8,9,9,9
        .byte 7,255,0,8,12,0,26,0,0,11,255,0,8,8,7,8,43,0,42,10,28,255,0,8,29,41,0,0,0,11,255,0,8
        .byte 12,0,7,0,0,28,255,0,11,29,41,0,0,11,0,20,0,0,15,0,0,0,12,26,0,7,28,255,0,13,29,41,0
        .byte 44,7,21,21,0,22,23,0,8,43,0,0,11,255,0,7,8,9,7,255,0,5,12,0,0,44,255,9,6,43,0,0,42
        .byte 28,255,0,6,8,43,0,11,255,0,5,12,255,0,9,25,0,42,28,255,0,7,12,0,0,11,255,0,5,12,255,0,10
        .byte 42,28,255,0,6,8,9,9,9,7,28,255,0,5,12,0,0,44,255,0,6,42,28,255,0,6,8,43,0,0,0,11,255,0,6
        .byte 12,25,255,0,7,42,28,255,0,6,8,43,127,27,0,0,11,0,0,20,0,0,0,12,255,0,8,11,255,0,6,8,43
        .byte 127,255,0,4,44,7,0,21,21,0,8,43,255,0,7,25,11,255,0,5,8,127,255,0,4,44,0,0,44,255,9,4,43
        .byte 255,0,9,11,255,0,5,12,255,0,16
        .byte 255,0,30,255,6,30,0,42,10,10,41,0,0,11,255,0,5,12,255,0,4,42,255,10,6,41,25,0,0,0,42,28,0,0
        .byte 29,41,0,44,7,255,0,4,29,255,10,4,28,255,0,6,29,41,0,0,0,11,255,0,4,29,41,0,44,7,255,0,16
        .byte 29,41,0,0,11,255,0,5,29,41,27,11,255,0,17,29,41,0,11,255,0,6,29,10,28,255,0,18,29,41,11,255,0,20
        .byte 8,9,7,255,0,5,12,11,255,0,20,29,41,44,7,0,20,0,0,12,11,0,0,24,255,0,18,29,41,44,7,21,21
        .byte 8,43,44,7,0,24,255,0,19,29,41,44,9,9,43,0,0,44,9,9,7,255,0,5,8,7,255,0,12,29,41,0,0
        .byte 44,255,0,5,11,255,0,5,12,11,255,0,12,8,43,255,0,5,42,10,10,28,255,0,5,12,11,255,0,11,8,43,255,0,5
        .byte 42,28,255,0,8,12,11,255,0,10,8,43,127,26,0,0,25,0,11,255,0,9,12,11,255,0,10,29,41,255,0,6,11
        .byte 255,0,6,20,0,0,12,44,7,255,0,10,29,41,255,0,5,11,255,0,6,21,21,8,43,25,11,255,0,5,8,9,7
        .byte 0,0,0,29,41,255,0,4,11,255,0,5,8,9,9,12,0,0,11,7,255,0,4,12,0,44,7,0,0,0,29,41,0,0,0
        .byte 11,31,18,19,30,8,43,0,0,12,27,0,11,11,255,0,4,12,0,127,44,7,0,0,0,29,41,0,0,44,7,16,17
        .byte 8,43,25,0,0,12,0,0,0,44,7,0,0,8,43,43,0,127,44,7,0,0,0,12,0,0,0,44,9,9,43,255,0,9
        .byte 44,9,9,43,255,0,5,11,0,0,0,12,0,0
        .byte 255,0,30,255,6,30,0,0,42,255,10,4,41,0,0,0,42,255,10,6,41,255,0,4,11,0,0,0,12,0,0,0,42
        .byte 28,255,0,4,29,10,10,10,28,0,0,11,0,42,28,29,255,10,4,28,0,0,0,29,41,0,0,11,255,0,6,12,255,0,5
        .byte 11,10,28,255,0,11,12,0,0,11,255,0,6,29,10,10,41,0,0,11,255,0,13,12,0,26,11,255,0,9,29,41,42
        .byte 28,255,0,13,12,27,0,11,255,0,10,29,28,255,0,14,12,0,42,28,255,0,22,8,9,9,9,12,0,11,255,0,23
        .byte 12,0,0,0,12,0,11,0,20,255,0,21,29,10,10,10,12,0,44,7,21,21,255,0,24,12,0,0,44,9,9,7,255,0,23
        .byte 29,41,0,42,10,10,28,255,0,24,12,42,28,255,0,27,12,11,255,0,28,12,11,255,0,28,12,11,255,0,5,8,9,9
        .byte 7,255,0,19,12,11,0,20,0,0,8,43,27,0,44,7,255,0,18,12,44,7,21,21,8,43,127,0,0,127,44,7,255,0,17
        .byte 12,0,44,9,9,43,127,255,0,4,25,44,7,0,0,8,9,9,32,33,255,9,5,7,0,0,0,12,255,0,12,44,9,9
        .byte 43,0,0,32,33,255,0,5,11,0,0,0,12
        .byte 255,0,30,255,6,30,0,0,42,255,10,18,41,0,27,0,11,0,0,0,12,0,42,28,255,0,10,29,28,0,11,255,0,4
        .byte 29,10,10,10,28,0,0,0,12,0,11,255,0,12,12,0,11,255,0,12,12,0,11,20,255,0,11,29,0,11,255,0,12
        .byte 12,0,11,21,21,255,0,11,29,28,255,0,12,12,0,44,9,9,7,255,0,23,8,43,25,0,26,0,11,255,0,19,8
        .byte 9,9,9,43,255,0,4,42,28,255,0,19,29,10,10,10,41,0,0,0,42,28,255,0,7,8,9,9,9,7,255,0,12
        .byte 29,41,0,42,28,255,0,8,29,10,10,10,28,255,0,13,12,0,11,255,0,27,12,0,11,255,0,5,24,255,0,21,12
        .byte 0,11,0,20,0,0,0,24,255,0,15,8,7,255,0,4,12,0,44,7,21,21,8,9,9,7,255,0,14,12,11,255,0,4
        .byte 12,0,0,44,9,9,12,0,0,44,7,255,0,13,12,11,255,0,4,12,255,0,5,12,27,0,127,11,255,0,12,8,43
        .byte 11,255,0,4,12,0,0,43,0,0,12,0,0,0,11,9,7,255,0,10,12,0,11,0,20,0,0,12,255,0,5,12,0,0,0
        .byte 11,0,44,7,255,0,7,8,9,12,0,11,7,21,21,8,43,255,0,8,25,11,0,0,44,9,9,9,7,0,0,0,12
        .byte 0,12,0,25,44,9,9,43,255,0,17,11,0,0,0,12,255,0,9
        .byte 255,0,30,255,6,30,255,0,16,11,0,0,0,12,255,0,14,25,255,0,9,42,28,0,0,0,29,41,255,0,16,12,255,0,6
        .byte 11,255,0,5,12,255,0,11,42,255,10,4,12,255,0,4,27,0,11,255,0,5,12,255,0,6,43,0,0,0,42,28,255,0,4
        .byte 29,255,10,6,28,255,0,5,12,25,255,0,9,11,255,0,18,12,255,0,10,11,255,0,18,12,255,0,10,11,255,0,18
        .byte 12,255,0,10,11,31,18,19,30,255,0,14,29,41,0,8,9,255,0,7,7,16,17,0,8,255,9,7,7,255,0,5,8
        .byte 9,9,43,255,0,8,44,9,9,9,12,0,0,0,26,0,0,0,11,255,0,5,12,0,27,255,0,13,12,255,0,6,25
        .byte 11,255,0,5,29,10,41,255,0,21,11,255,0,5,8,43,29,255,10,4,255,0,4,42,255,0,12,11,255,0,5,12,255,0,20
        .byte 42,10,10,28,255,0,5,12,255,0,4,43,255,0,13,10,10,28,0,11,255,0,6,12,255,0,22,11,255,0,6,12,34
        .byte 35,36,37,38,255,0,17,94,97,112,0,0,111,94,97,0,0,39,255,0,11,25,255,0,6,59,98,101,113,0,0,110,98
        .byte 101,61,255,0,19,59,58,47,255,0,6,47,60,61,255,0,6
        .byte 255,0,30,255,6,30,255,0,6,59,255,49,6,61,0,53,47,0,0,47,48,0,0,59,49,49,49,61,255,0,7,59,58
        .byte 255,47,6,60,49,58,47,0,0,47,60,49,49,58,47,47,47,60,61,255,0,5,59,58,45,255,0,6,46,47,47,45,0,0
        .byte 46,47,47,47,45,0,0,0,46,60,61,0,0,0,59,58,45,255,0,8,46,45,255,0,12,47,48,0,0,59,58,45,255,0,23
        .byte 47,48,0,59,58,45,255,0,24,47,48,59,58,45,255,0,4,52,47,51,255,0,18,47,48,53,47,255,0,4,52,56,55
        .byte 47,255,0,18,47,48,53,47,255,0,4,47,48,53,47,255,0,18,47,48,53,47,255,0,4,47,48,53,47,0,21,21,0
        .byte 47,51,255,0,12,47,48,53,47,255,0,4,46,60,54,55,51,20,20,52,56,47,255,0,12,47,48,54,55,51,255,0,4
        .byte 46,60,54,55,47,47,56,53,47,255,0,12,47,48,0,54,55,51,255,0,4,46,60,61,50,50,57,53,47,255,0,12,47
        .byte 48,0,0,54,55,51,255,0,4,47,48,0,0,59,58,45,255,0,5,52,51,0,21,21,0,52,56,57,0,0,0,54,55
        .byte 51,0,0,0,47,48,0,59,58,45,255,0,5,52,47,47,51,20,20,52,56,57,255,0,5,53,47,0,0,0,47,48,0
        .byte 53,47,255,0,6,47,56,50,55,47,47,56,57,0,0,255,49,4,53,47,0,0,0,47,48,0,53,47,255,0,6,47,48
        .byte 0,54,50,50,57,0,0,0,255,62,4,53,47,0,0,0,47,48,0,53,255,47,8,48,255,0,8,255,50,4,53,47,0,0,0
        .byte 47,48,0,54,255,50,8,57,255,0,12,53,47,0,0,0,47,48,0,0,53,255,62,5,48,255,0,10
        .byte 255,0,30,255,6,30,255,0,4,53,47,0,0,0,255,47,11,60,255,49,7,61,255,0,4,59,58,47,255,0,13,46,255,47,8
        .byte 48,0,0,0,59,58,47,45,255,0,21,47,48,0,59,49,58,47,45,255,0,20,21,21,47,48,0,53,47,47,45,255,0,21
        .byte 20,20,47,48,0,53,47,45,255,0,21,52,47,47,47,48,0,53,47,255,0,22,47,56,50,50,57,0,53,47,51,0,0
        .byte 24,255,0,18,47,48,255,0,4,54,55,47,51,0,24,255,0,18,47,60,49,49,61,0,0,54,55,47,47,47,51,255,0,17
        .byte 46,47,47,47,48,0,0,0,54,50,50,50,47,255,0,20,47,48,49,0,0,59,49,49,49,47,255,0,20,47,48,62,0
        .byte 59,58,47,47,47,45,255,0,20,47,48,50,0,53,47,255,0,13,52,255,47,4,112,0,0,111,47,47,47,48,0,49,53
        .byte 47,255,0,13,47,56,50,55,47,113,0,0,110,60,50,50,57,0,62,53,47,0,21,21,255,0,9,52,47,48,0,53,47
        .byte 255,0,4,46,60,61,0,0,50,53,47,0,20,20,0,52,51,255,0,5,52,47,56,57,0,53,47,0,21,21,0,0,47
        .byte 48,0,0,0,54,55,255,47,6,51,255,0,4,47,56,57,0,0,53,47,0,20,20,0,0,47,48,255,0,4,54,255,50,5
        .byte 55,47,51,0,0,0,47,48,0,0,0,54,55,255,47,6,48,255,0,10,54,55,47,0,0,0,47,48,255,0,4,54,255,50,6
        .byte 57,0,0
        .byte 255,0,30,255,6,30,0,0,59,255,49,5,61,53,47,0,0,0,47,48,255,0,15,59,58,255,47,5,60,58,47,0,0,0
        .byte 47,60,255,49,9,61,255,0,4,59,58,47,45,0,0,0,46,47,47,45,0,0,0,46,255,47,10,60,61,0,0,0,53
        .byte 47,45,255,0,22,46,60,61,0,0,53,47,255,0,24,46,60,61,0,53,47,255,0,25,47,48,0,53,47,255,0,25,47
        .byte 48,0,53,47,255,0,4,52,255,47,5,51,255,0,14,47,48,0,53,47,255,0,4,47,56,50,50,50,55,47,255,0,4
        .byte 52,47,51,255,0,7,47,48,0,53,47,255,0,4,47,48,0,0,0,53,47,255,0,4,47,48,55,51,255,0,6,47,48
        .byte 0,53,47,112,0,0,111,47,48,0,0,0,53,47,255,0,4,47,48,54,55,51,255,0,5,47,48,0,53,47,113,0,0
        .byte 110,47,48,0,0,0,53,47,0,21,21,0,47,48,0,54,55,51,255,0,4,47,48,0,53,47,255,0,4,47,48,0,0,0
        .byte 53,47,0,20,20,0,47,48,0,0,53,47,112,0,0,111,47,48,49,53,47,255,0,4,47,48,0,0,0,54,55,255,47,4
        .byte 56,57,0,0,53,47,113,0,0,110,47,48,62,53,47,31,18,19,30,47,48,255,0,4,54,255,50,4,57,0,0,0,53
        .byte 47,255,0,4,47,48,62,53,47,0,16,17,0,47,48,255,0,5,53,47,48,255,0,5,53,47,255,0,4,47,48,62,54
        .byte 55,255,47,4,56,57,255,0,5,53,47,48,255,0,5,53,47,112,0,0,111,47,48,50,0,54,255,50,4,57,255,0,6
        .byte 53,47,48,255,0,5,53,47,113,0,0,110,47,48,255,0,14,53,47,48,255,0,5,53,47,255,0,4,47,48,255,0,14
        .byte 53,47,48,255,0,5,53,47,255,0,4,47,48,0
        .byte 255,0,30,255,6,30,0,59,49,49,49,61,255,0,7,53,47,48,255,0,5,53,47,255,0,4,47,48,0,59,58,47,47,47
        .byte 60,61,255,0,6,53,47,48,255,49,5,58,47,255,0,4,47,48,0,53,47,0,0,0,46,60,61,255,0,5,53,47,58
        .byte 255,47,6,45,255,0,4,46,60,61,53,47,255,0,4,46,60,61,255,0,4,53,47,45,255,0,12,47,48,53,47,255,0,5
        .byte 46,60,61,0,0,59,58,45,255,0,13,47,48,53,47,255,0,6,46,60,49,49,58,45,255,0,14,47,48,53,47,21,21
        .byte 255,0,5,46,47,47,45,255,0,15,47,48,53,47,20,20,255,0,24,47,48,54,55,47,47,47,51,255,0,22,47,48,0
        .byte 54,50,50,50,55,51,255,0,11,52,47,47,47,51,255,0,4,52,47,48,255,0,5,54,55,51,255,0,10,47,56,50,55
        .byte 47,255,0,4,47,56,57,255,0,6,54,55,51,255,0,9,47,48,0,53,47,255,0,4,47,48,255,0,7,59,58,45,255,0,9
        .byte 47,48,0,53,47,112,0,0,111,47,48,255,0,6,59,58,45,255,0,9,52,56,57,0,53,47,113,0,0,110,47,48,255,0,5
        .byte 59,58,45,255,0,9,52,56,57,0,0,53,47,255,0,4,47,48,255,0,4,59,58,45,255,0,6,52,51,0,0,47,48
        .byte 0,0,0,53,47,0,21,21,0,47,48,0,0,0,59,58,45,255,0,6,52,56,47,0,0,47,48,0,0,0,53,47,0
        .byte 20,20,0,47,48,0,0,59,58,45,255,0,4,52,47,47,56,53,47,108,109,47,48,0,0,0,54,55,255,47,4,56,57
        .byte 0,0,53,47,255,0,4,52,56,50,50,57,54,55,47,47,56,57,255,0,4,54,255,50,4,57,0,0,0,53,47,255,0,4
        .byte 47,48,255,0,4,54,50,50,57,255,0,13
        .byte 255,0,30,255,6,30,0,53,47,255,0,4,47,48,255,0,4,59,255,49,14,61,0,0,53,47,255,0,4,47,60,255,49,4
        .byte 58,255,47,14,60,61,0,53,47,255,0,4,46,255,47,6,45,255,0,12,46,47,48,0,53,47,255,0,5,47,255,0,19
        .byte 47,48,0,53,47,255,0,5,47,255,0,17,21,21,47,48,59,58,47,255,0,5,47,255,0,17,20,20,47,48,53,47,45
        .byte 255,0,5,47,255,0,15,52,47,47,47,56,57,53,47,47,112,255,0,4,47,255,0,15,47,56,50,50,57,0,53,47,47
        .byte 113,255,0,4,47,255,0,15,47,48,255,0,4,53,47,0,24,255,0,20,47,48,255,0,4,53,47,0,24,255,0,20,47
        .byte 60,49,61,0,0,53,47,255,0,22,46,47,47,60,61,0,53,47,255,0,24,46,47,48,0,53,47,0,20,255,0,4,47
        .byte 255,0,18,47,48,49,53,47,112,20,255,0,4,47,255,0,18,47,48,62,53,47,113,20,255,0,4,47,255,0,15,21,21
        .byte 0,47,48,50,53,47,51,20,255,0,4,47,0,108,109,0,0,0,52,47,108,109,47,51,0,0,0,20,20,0,47,48,0
        .byte 54,55,47,47,0,0,255,47,10,56,50,50,55,255,47,7,56,57,0,0,53,47,255,0,4,47,56,255,50,7,57,0,0
        .byte 54,255,50,7,57,0,0,0,53,47,255,0,4,47,48,255,0,4,53,62,48,255,0,14
        .byte 255,0,30,255,6,30,59,58,47,255,0,4,47,60,61,0,0,0,53,62,48,255,0,4,59,255,49,7,61,0,53,47,255,0,5
        .byte 46,47,60,49,61,0,53,62,48,0,0,59,49,58,255,47,7,60,61,53,47,255,0,6,46,47,47,60,255,49,6,58,47,47
        .byte 45,255,0,6,47,48,53,47,255,0,8,46,255,47,8,45,255,0,8,47,48,53,47,255,0,10,46,255,47,4,45,255,0,10
        .byte 47,48,53,47,255,0,26,47,48,53,47,255,0,21,31,18,19,30,0,47,48,53,47,255,0,17,52,47,47,47,51,16,17
        .byte 52,47,56,57,53,47,255,0,15,52,47,47,56,255,50,7,57,0,53,47,0,24,255,0,13,47,56,50,57,255,0,5,53
        .byte 62,48,0,53,47,0,24,255,0,13,47,60,61,255,0,6,53,62,48,0,53,47,47,47,255,0,13,46,47,60,61,255,0,5
        .byte 53,62,48,0,53,47,45,255,0,15,46,47,60,61,255,0,4,53,62,48,0,53,47,255,0,17,46,47,60,61,0,0,0
        .byte 53,62,48,0,53,47,255,0,18,46,47,60,61,0,0,53,62,48,0,53,47,255,0,19,46,47,60,61,0,53,62,48,0
        .byte 53,47,0,0,21,21,255,0,16,46,47,60,61,53,62,48,0,53,47,0,0,20,20,0,52,255,47,4,51,255,0,10,46
        .byte 47,48,255,49,4,54,55,255,47,6,56,50,50,55,47,47,47,32,33,47,47,51,255,0,4,47,48,255,62,4,0,54,255,50,6
        .byte 57,0,0,54,50,50,50,32,33,50,55,47,255,0,4,47,48,255,50,4
        .byte 255,0,30,255,6,30,0,59,255,49,16,58,47,255,0,4,46,60,61,0,0,0,59,58,255,47,17,45,255,0,5,46,60
        .byte 61,0,0,53,47,255,0,24,46,60,61,0,53,47,255,0,25,46,60,61,53,47,255,0,26,47,48,53,47,255,0,26,47
        .byte 48,53,47,255,0,26,47,48,53,47,255,0,4,52,47,47,51,255,0,4,52,255,47,4,51,255,0,8,47,48,53,47,255,0,4
        .byte 47,56,55,47,255,0,4,47,56,50,50,55,47,255,0,4,52,47,47,47,56,57,53,47,255,0,4,47,48,53,47,255,0,4
        .byte 47,48,0,0,53,47,255,0,4,47,56,50,50,57,0,53,47,255,0,4,47,48,53,47,112,0,0,111,47,48,0,0,53
        .byte 47,255,0,4,47,48,255,0,4,53,47,255,0,4,47,48,53,47,113,0,0,110,47,48,0,0,53,47,255,0,4,47,48
        .byte 255,0,4,53,47,112,0,0,111,47,48,53,47,255,0,4,47,48,0,0,53,47,255,0,4,47,48,255,0,4,53,47,113
        .byte 0,0,110,47,48,53,47,255,0,4,47,48,0,0,53,47,255,0,4,47,48,255,49,4,53,47,255,0,4,47,48,53,47
        .byte 255,0,4,47,48,0,0,53,47,255,0,4,47,48,255,62,4,53,47,255,0,4,47,48,53,47,255,0,4,47,48,0,0
        .byte 53,47,112,0,0,111,47,48,255,62,4,53,47,0,21,21,0,47,48,53,47,31,18,19,30,47,48,0,0,53,47,113,0,0
        .byte 110,47,48,255,50,4,53,47,0,20,20,0,47,48,53,47,0,16,17,0,47,48,0,0,53,47,255,0,4,47,48,255,0,4
        .byte 54,55,255,47,4,56,57,54,55,255,47,4,56,57,0,0,53,47,255,0,4,47,48,255,0,5,54,255,50,4,57,0,0
        .byte 54,255,50,4,57,0,0,0,53,47,255,0,4,47,48,255,0,4
        .byte 255,0,30,255,6,30,255,0,12,59,255,49,5,58,47,255,0,4,46,60,61,255,0,11,59,49,49,49,58,255,47,6,45
        .byte 255,0,5,46,60,61,255,0,6,59,49,49,49,58,255,47,4,45,255,0,12,46,60,61,255,0,4,59,58,255,47,4,45
        .byte 255,0,17,46,60,61,0,0,59,58,47,45,255,0,22,47,48,0,0,53,47,45,255,0,23,47,48,0,0,53,47,255,0,15
        .byte 52,47,47,47,51,255,0,4,47,48,0,59,58,47,255,0,14,52,56,50,50,55,47,255,0,4,47,48,59,58,47,45,255,0,14
        .byte 47,48,0,0,53,47,255,0,4,47,48,53,47,45,255,0,15,47,48,0,0,53,47,112,0,0,111,47,48,53,47,255,0,13
        .byte 24,0,0,47,48,0,0,53,47,113,0,0,110,47,48,53,47,0,0,0,52,255,47,9,51,0,0,47,48,0,0,53,47
        .byte 255,0,4,47,48,53,47,0,0,0,47,56,255,50,7,55,47,0,0,47,48,0,0,53,47,255,0,4,47,48,53,47,0,0,0
        .byte 47,48,255,24,4,0,0,0,53,47,0,0,47,48,0,0,53,47,0,21,21,0,47,48,53,47,0,0,0,47,48,24,24,24
        .byte 255,0,4,53,47,0,0,47,48,0,0,53,47,0,20,20,0,47,48,53,47,0,0,0,47,48,24,24,255,0,5,53,47
        .byte 0,0,47,48,0,0,54,55,255,47,4,56,57,53,47,0,0,0,47,48,0,34,35,36,37,38,0,53,47,0,0,47,48
        .byte 0,0,0,54,255,50,4,57,0,53,47,0,0,0,47,48,0,0,0,39,0,0,0,53,47,108,109,47,48,255,0,5,53
        .byte 62,48,0,0,53,47,47,0,0,47,48,24,0,0,24,0,0,0,54,55,47,47,56,57,255,0,5,53,62,48,0,0,53
        .byte 47,0,0,0,47,48,24,255,0,7,54,50,50,57,255,0,6,53,62,48,0,0
        .byte 255,0,30,255,6,30,64,63,255,0,4,64,64,65,76,76,76,64,76,75,76,64,64,65,65,69,64,67,76,76,76,75,76,76
        .byte 64,66,63,255,0,4,74,65,76,65,66,76,75,76,76,67,64,65,65,63,75,76,65,63,64,66,65,76,65,76,64,64,255,0,5
        .byte 74,68,70,65,70,73,74,64,76,70,70,68,63,70,65,70,70,68,65,70,70,67,65,64,63,255,0,13,4,255,0,8,5
        .byte 0,0,0,65,65,70,63,255,0,24,24,0,65,65,64,69,255,0,24,20,20,70,67,76,70,255,0,24,76,65,65,76,69
        .byte 65,65,64,255,0,24,69,76,75,63,4,255,0,23,24,0,65,65,65,63,255,0,24,20,20,70,65,76,63,24,255,0,23
        .byte 65,64,65,64,65,64,20,20,255,0,23,5,68,76,67,65,69,64,255,0,24,69,65,76,69,255,0,26,70,65,64,64,255,0,26
        .byte 68,69,64,64,255,0,26,68,76,64,65,71,255,0,25,65,76,65,76,76,69,67,68,63,69,69,76,65,0,0,65,69,69
        .byte 68,63,69,69,69,68,63,69,69,71,0,0,68,76,65,64,66,76,76,76,63,65,76,64,65,0,0,65,76,63,76,76,75
        .byte 255,76,4,66,76,65,0,0,66,76,75,76,76,65,76,76,76,66,76,76,76,66,76,64,76,65,255,76,4,64,255,76,4
        .byte 64,0,0,63,65
        .byte 255,0,30,255,6,30,76,76,64,76,66,76,76,76,70,70,68,64,63,70,76,76,76,67,64,76,70,76,76,70,70,63,0,0
        .byte 68,64,64,67,255,76,4,73,255,0,7,74,76,76,64,76,73,255,0,8,76,76,76,64,76,63,76,73,255,0,9,76,64
        .byte 76,76,255,0,9,76,66,76,76,63,63,73,255,0,10,74,75,64,73,255,0,9,68,76,66,64,73,255,0,13,4,255,0,11
        .byte 69,69,76,76,255,0,6,24,255,0,19,70,70,64,76,255,0,6,24,255,0,19,68,76,76,76,31,18,19,30,72,76,64
        .byte 71,255,0,18,74,64,64,76,20,16,17,72,76,75,76,76,255,0,19,76,76,76,64,255,76,6,73,255,0,19,64,76,64
        .byte 76,66,64,76,63,76,73,255,0,20,64,76,76,76,64,76,68,63,73,255,0,14,72,68,63,71,0,0,0,66,76,76,76
        .byte 73,255,0,18,76,64,63,76,0,0,0,76,76,76,64,255,0,19,64,76,66,76,0,24,0,64,67,76,63,255,0,19,255,76,4
        .byte 20,20,20,64,76,69,69,255,0,4,72,76,69,68,63,71,255,0,9,76,64,64,76,69,76,69,76,76,70,70,0,0,0
        .byte 72,76,76,64,68,63,76,76,71,255,0,7,76,76,76,67,76,64,76,64,76,76,63,0,0,72,76,76,66,76,76,63,76,76,76
        .byte 69,68,63,71,0,108,109,76,64,76,76,64,64,75,76,76,64,63,0,0,76,76,64,76,76,76,75,64,76,76,64,68,76
        .byte 64,76,64,255,76,7,64,76,64,76,64,0,0,76,76,76,67,64,255,76,4,64,76,76,67,76,76,76,64,76,66,76,76
        .byte 64,76,76,64
        .byte 255,0,30,255,6,30,76,65,76,0,0,76,64,76,64,76,64,76,64,76,64,67,76,64,64,76,76,64,64,76,76,63,76,76
        .byte 64,76,66,76,73,0,0,74,76,64,67,64,76,73,255,0,11,74,68,63,76,76,66,76,76,73,255,0,4,76,76,64,76
        .byte 73,255,0,15,74,76,64,76,64,255,0,5,68,76,76,73,255,0,17,74,76,64,76,255,0,5,69,69,69,255,0,19,74
        .byte 76,64,255,0,5,70,70,70,255,0,4,72,76,64,76,71,255,0,11,64,76,255,0,5,68,76,63,255,0,4,76,64,76
        .byte 64,76,76,76,71,255,0,8,67,63,255,0,5,66,76,73,255,0,4,76,64,76,64,76,64,75,76,255,0,8,64,63,255,0,5
        .byte 76,64,255,0,5,76,66,76,68,64,76,76,73,255,0,8,64,69,255,0,5,74,76,255,0,5,76,76,70,68,63,70,73
        .byte 255,0,9,76,76,255,0,6,73,255,0,4,72,76,73,255,0,14,76,70,255,0,10,72,76,76,255,0,10,71,255,0,4
        .byte 64,63,255,0,10,76,76,73,255,0,9,72,76,112,0,0,111,64,63,255,0,9,72,76,73,255,0,10,76,64,113,0,0
        .byte 110,76,76,71,255,0,7,76,76,73,255,0,11,76,76,255,0,4,64,76,76,255,0,6,72,76,76,255,0,9,72,0,0
        .byte 76,64,255,0,4,76,64,76,255,0,6,76,66,76,71,255,0,7,72,68,0,0,66,76,71,24,0,72,76,76,64,71,255,0,5
        .byte 76,76,76,64,67,71,0,0,72,76,76,76,68,108,109,76,64,76,20,20,76,64,67,76,76,64,69,68,63,69,76,64,75
        .byte 76,76,76,0,0,76,64,76,64,76,76,76,64,76,64,64,76,76,66,64,76,64,76,76,64,76,76,64,76,64,76,76,255,0,4
        .byte 76,67,76,64,76,64,76,76,75,76,64,64,76
        .byte 255,0,30,255,6,30,76,65,76,65,65,76,65,76,65,76,65,76,76,255,0,4,64,76,64,76,64,76,64,76,64,76,64
        .byte 66,76,76,65,76,65,76,65,76,76,76,75,76,65,76,255,0,4,76,64,76,64,76,76,63,76,76,75,76,76,64,65,67
        .byte 76,76,73,5,74,76,65,76,76,76,65,255,0,4,76,76,64,76,76,68,63,70,76,64,76,64,76,76,65,76,73,255,0,4
        .byte 74,65,76,65,76,255,0,4,76,64,75,76,73,0,4,0,74,76,76,64,64,65,76,65,255,0,6,74,76,76,76,255,0,4
        .byte 64,76,76,73,255,0,4,85,0,74,76,76,76,65,76,0,0,0,24,255,0,18,84,0,0,76,64,65,76,66,71,0,0
        .byte 24,255,0,15,72,76,76,66,76,64,255,76,4,65,76,65,76,65,76,65,76,65,65,76,255,0,4,69,68,76,63,69,76,76,76
        .byte 64,76,76,75,64,76,76,65,76,65,76,65,76,75,76,65,76,65,255,0,4,76,68,76,64,76,67,76,64,76,64,76,64,64
        .byte 76,65,76,65,65,76,65,76,76,68,76,65,76,255,0,4,64,76,64,76,76,76,73,5,4,74,76,76,64,75,76,73,0
        .byte 5,74,67,76,70,68,63,70,65,255,0,4,76,64,76,76,73,255,0,5,74,76,64,76,76,0,0,0,24,255,0,17,85
        .byte 255,0,4,76,64,65,76,71,0,0,24,255,0,17,84,0,0,0,72,76,64,76,76,65,72,76,65,85,0,72,76,76,76
        .byte 63,255,0,4,68,76,76,71,0,72,76,71,0,72,76,67,64,65,65,65,76,76,76,84,72,65,76,65,76,69,255,0,4
        .byte 69,69,76,76,71,255,76,11,75,76,65,76,65,76,67,76,70,70,255,0,4,70,76,64,76,64,76,64,76,64,76,64,76
        .byte 64,65,65,79,80,81,82,65,76,65,76,65,76,63,255,0,4,68,76,76,64,76,64,76,75,76,76,76,64,255,76,4,77
        .byte 83,76,66,65,76,65,76,65,76,71,0,0,72,76,76,64,76,66,76,64,76,64,76,64,76,76,65,67,76,77,78,255,76,5
        .byte 75,76,65,76,0,0,255,76,8,65,79,80,81,82,67,76,76,76,77,78,65,76,65,76,65,76,65,76,76,0,0,76,75
        .byte 65,64,65,64,65,64,76,64,77,83,76,76
        .byte 255,0,30,255,6,30,76,76,76,77,78,255,76,4,63,255,76,4,0,0,255,76,5,63,255,76,4,77,78,255,76,4,79
        .byte 80,81,82,76,76,68,63,70,73,5,255,0,4,74,70,68,76,63,70,73,0,0,77,78,74,255,76,8,73,255,0,11,4
        .byte 255,0,5,79,80,81,82,255,76,4,73,255,0,24,85,76,76,73,255,0,26,84,76,76,255,0,25,24,0,76,76,76,255,0,25
        .byte 20,20,76,76,76,71,0,24,0,72,69,68,63,69,71,255,0,6,69,68,63,69,255,76,11,71,20,20,68,76,68,63,76,76
        .byte 255,0,6,74,76,63,70,73,255,0,6,255,76,7,68,70,68,76,76,76,255,0,17,74,68,255,76,5,73,0,0,74,76
        .byte 73,255,0,18,68,76,0,24,255,0,26,76,76,0,24,255,0,26,76,76,76,73,255,0,15,72,68,63,69,71,255,0,5
        .byte 72,76,76,76,255,0,16,76,76,63,76,76,76,71,0,0,0,74,76,76,76,255,0,8,76,71,255,0,6,70,68,76,70
        .byte 76,76,76,0,24,0,0,76,76,76,71,255,0,6,72,76,63,255,0,10,74,76,76,0,20,20,0,255,76,4,71,31,18
        .byte 19,30,72,76,76,63,71,255,0,6,71,0,85,0,74,76,79,80,81,82,255,76,5,0,16,17,0,255,76,5,71,255,0,5
        .byte 76,71,84,72,76,76,76,77,83,255,76,17,255,0,4,255,76,7,77,78,76,76
        .byte 255,0,30,255,6,30,64,64,64,67,64,76,63,76,64,76,64,64,76,64,76,255,0,4,76,76,64,64,76,64,76,77,78
        .byte 76,64,76,76,76,70,70,68,63,70,73,255,0,4,4,255,0,9,74,67,64,77,78,64,76,64,76,73,255,0,23,77,78
        .byte 76,64,76,76,255,0,23,79,80,81,82,76,64,76,24,255,0,25,76,64,64,65,20,20,255,0,22,76,76,76,66,76,69
        .byte 76,76,255,0,23,74,76,64,64,76,64,73,255,0,15,85,255,0,8,74,76,76,64,76,255,0,11,72,65,65,63,71,84
        .byte 255,0,9,64,75,70,73,255,0,11,76,64,76,64,66,69,255,0,9,76,76,76,255,0,12,76,75,64,76,64,76,255,0,9
        .byte 64,64,73,255,0,12,64,76,76,64,70,70,255,0,8,72,76,76,255,0,13,74,68,67,65,73,255,0,9,68,75,64,255,0,15
        .byte 4,255,0,11,68,76,76,71,0,0,24,0,0,85,255,0,20,69,69,64,76,0,0,20,20,0,84,255,0,16,76,0,0,0
        .byte 76,64,64,76,71,79,80,81,82,68,63,255,0,14,72,76,0,24,0,70,70,76,64,64,65,77,83,76,68,63,255,0,11
        .byte 85,0,0,76,76,0,20,20,68,76,64,66,76,65,77,78,65,76,63,255,0,11,84,0,72,76,76,64,76,64,76,64,76
        .byte 64,76,64,77,78,76,64,76,64,76,64,71,255,0,4,72,76,64,76,64,76,64,76,64,64,76,67,76
        .byte 255,0,30,255,6,30,76,75,76,64,77,78,76,88,88,88,76,64,88,255,0,4,76,76,64,76,88,88,76,64,88,64,67
        .byte 76,76,64,76,88,79,80,81,82,76,88,76,76,67,76,255,0,4,76,75,76,88,76,76,68,64,76,88,76,64,64,88,64
        .byte 76,76,76,64,76,64,86,87,76,76,73,255,0,4,74,76,76,86,87,70,68,63,70,76,64,88,64,64,76,88,73,255,0,7
        .byte 5,255,0,6,4,255,0,4,5,0,0,74,76,76,76,88,64,73,255,0,24,74,76,88,76,76,0,24,255,0,24,64,76
        .byte 88,76,0,20,20,255,0,23,76,75,76,76,68,88,76,71,255,0,22,64,76,75,88,64,76,64,88,76,71,255,0,14,63
        .byte 71,255,0,4,76,88,76,88,76,66,76,64,76,73,255,0,14,76,76,255,0,4,68,64,64,76,64,76,73,0,5,255,0,15
        .byte 76,76,255,0,4,68,76,88,88,64,73,255,0,18,75,76,255,0,4,69,69,76,76,73,255,0,11,76,71,255,0,6,76,76
        .byte 255,0,4,64,76,88,76,255,0,12,76,76,255,0,6,76,76,255,0,4,70,70,76,63,255,0,10,85,72,76,76,71,255,0,5
        .byte 76,76,255,0,4,68,76,64,69,255,0,10,84,76,75,64,76,255,0,4,72,76,76,71,24,0,72,76,76,70,70,24,255,0,4
        .byte 79,80,81,82,72,76,64,76,76,76,71,0,0,0,255,76,4,20,20,76,76,64,64,63,20,20,0,72,76,76,77,83,76,76
        .byte 64,88,76,88,76,76,0,0,0,74,67,76,76,76,88,76,64,76,76,64,64,76,76,88,88,76,77,78,75,76,88,76,76
        .byte 66,88,64,71,0,0,0,74,76,88,64,65,88,76,67,88,76,88,75,76,88,76,76,77,78,76,64,76,88,64,76,64,76
        .byte 67,71,0,0,0,74,64,76,88,64,76,76
        .byte 255,0,30,255,6,30,88,76,65,76,65,66,76,79,80,81,82,65,76,65,76,65,65,75,65,255,0,5,65,88,76,88,76
        .byte 88,65,76,73,74,76,76,73,0,86,87,0,76,65,76,76,86,87,65,65,255,0,5,74,76,76,88,76,66,76,73,255,0,9
        .byte 74,76,67,65,0,0,76,76,255,0,6,74,76,65,65,88,65,255,0,11,74,76,76,0,0,65,73,255,0,7,76,88,76,76
        .byte 66,255,0,12,74,65,0,0,76,255,0,8,76,67,65,65,65,255,0,24,72,76,65,76,88,76,255,0,4,74,76,76,255,0,15
        .byte 72,76,76,88,88,65,88,88,255,0,5,74,76,0,0,71,255,0,10,85,72,76,76,88,65,76,76,65,88,255,0,6,4
        .byte 0,0,88,71,85,255,0,8,84,65,67,65,76,65,65,65,76,76,255,0,9,65,76,84,255,0,7,72,76,76,88,76,65
        .byte 88,88,65,88,65,0,0,0,24,255,0,5,88,65,69,73,255,0,6,76,76,65,76,76,65,76,66,76,76,76,71,0,0
        .byte 20,20,0,0,0,72,65,67,73,255,0,9,74,76,88,88,65,76,65,88,88,65,76,65,88,75,88,65,88,65,88,73,255,0,15
        .byte 74,76,65,76,65,65,65,76,88,76,65,76,65,73,255,0,17,74,88,79,80,81,82,65,73,4,5,5,255,0,20,76,76
        .byte 77,83,5,4,255,0,9,72,88,76,88,76,76,76,71,255,0,4,24,0,0,88,65,77,78,255,0,9,72,255,76,4,66
        .byte 76,88,76,88,76,71,0,0,20,20,72,75,65,77,78,255,0,6,72,76,76,88,76,88,76,88,88,76,88,76,88,76,88
        .byte 76,76,79,80,81,82,65,77,78,255,0,5,72,67,88,73,4,5,0,0,4,5,79,80,81,82,255,76,5,77,83,88,79
        .byte 80,81,82,0,0,0,75,88,76,73,39,34,35,36,37,38,0,0,77,83,0,76,88,67,88,76,77,78,76
        .byte 255,0,30,255,6,30,92,77,78,255,0,4,89,93,92,93,92,93,92,93,91,90,90,79,80,81,82,255,90,5,77,78,90
        .byte 93,77,78,255,0,4,91,86,87,255,90,5,91,86,87,255,0,9,77,78,92,94,95,96,97,255,0,23,77,78,93,102,103
        .byte 104,105,255,0,23,77,78,92,102,103,103,105,255,0,20,24,0,0,77,78,93,98,99,99,101,255,0,16,94,95,96,97,21,21
        .byte 94,95,96,97,13,77,83,255,0,17,102,103,104,105,106,106,102,103,104,105,92,77,78,255,0,17,102,103,103,105,107,107,102
        .byte 103,103,105,93,77,78,255,0,17,98,99,99,101,0,0,98,99,99,101,92,77,78,255,0,24,77,83,13,79,80,81,82,255,0,23
        .byte 77,78,93,13,77,83,255,0,23,79,80,81,82,93,77,78,255,0,24,77,83,13,94,95,96,97,255,0,4,94,95,96,97
        .byte 0,0,0,94,95,96,97,255,0,8,77,78,92,102,103,104,105,255,106,4,102,103,104,105,0,0,0,102,103,104,105,255,106,8
        .byte 77,78,106,102,103,103,105,255,107,4,102,103,103,105,0,0,0,102,103,103,105,255,107,8,77,78,107,98,99,99,101,255,13,4
        .byte 98,99,99,101,0,0,0,98,99,99,101,255,13,8,77,78,13,13,13,100,13,92,93,92,93,13,77,83,255,0,5,77,83
        .byte 13,93,92,93,92,93,92,93,92,77,78,92,93,92,91,90,90,91,93,92,93,77,78,255,0,5,77,78,93,92,3,92,3
        .byte 92,91,90,90,77,78,90,92,93,92,93,92,100,92,93,92,77,78,255,0,5,77,78,92,93,92,93,92,93,100,93,92,77
        .byte 78,92
        .byte 255,0,30,255,6,30,92,93,92,93,92,89,92,93,92,77,78,255,0,5,77,78,92,93,92,93,92,93,89,93,92,77,78
        .byte 93,93,91,90,90,90,91,90,90,90,77,78,255,0,5,77,78,255,90,6,91,90,90,77,78,90,92,100,255,0,7,77,78
        .byte 255,0,5,77,78,255,0,4,86,87,0,0,0,77,78,93,93,89,255,0,7,77,78,255,0,4,79,80,81,82,255,0,8
        .byte 77,78,92,92,89,255,0,4,24,0,94,95,96,97,255,0,15,77,78,93,93,89,255,0,4,21,21,102,103,104,105,255,0,14
        .byte 94,95,96,97,92,89,255,0,4,91,90,102,103,103,105,255,0,14,102,103,104,105,93,89,255,0,6,98,99,99,101,0,0,0
        .byte 79,80,81,82,255,0,7,102,103,103,105,92,89,255,0,14,77,83,255,0,8,98,99,99,101,93,89,255,0,14,77,78,255,0,10
        .byte 100,13,92,89,255,0,14,77,78,255,0,10,89,93,93,89,255,0,14,77,78,255,0,10,89,92,92,89,79,80,81,82,255,0,10
        .byte 77,78,0,0,0,79,80,81,82,0,0,0,89,93,93,89,0,77,83,255,0,11,77,78,255,0,4,77,83,255,0,4,89
        .byte 92,92,89,0,77,78,255,0,11,77,78,255,0,4,77,78,255,0,4,89,93,93,89,0,77,78,255,0,11,77,78,255,0,4
        .byte 77,78,255,0,4,89,92,92,89,0,77,78,255,0,11,77,78,255,0,4,77,78,255,0,4,89,93,93,89,0,77,78,255,0,4
        .byte 108,109,255,0,4,94,95,96,97,0,0,0,77,78,255,0,4,89,92,92,91,90,77,78,255,90,10,102,103,104,105,90,90,90
        .byte 77,78,90,91,0,0,89,93,93,92,93,77,78,92,93,92,93,92,93,92,93,92,93,102,103,103,105,92,93,92,77,78,93
        .byte 100,0,0,89,92
        .byte 255,0,30,255,6,30,93,91,90,77,78,255,90,7,91,92,91,98,99,99,101,90,91,92,77,78,93,89,0,0,89,92,92
        .byte 100,79,80,81,82,255,0,6,100,93,100,255,0,5,91,90,77,78,90,91,0,0,89,93,93,89,0,86,87,255,0,7,91
        .byte 90,91,255,0,6,79,80,81,82,0,0,0,89,92,92,89,255,0,26,89,93,93,89,255,0,26,89,92,92,89,255,0,26
        .byte 89,93,93,89,255,0,6,91,255,90,12,91,0,0,0,91,90,90,91,92,92,89,255,0,6,100,93,92,93,92,93,92,93
        .byte 92,93,92,93,92,100,0,0,0,100,92,93,92,93,93,89,255,0,6,89,92,93,92,93,92,93,92,93,92,91,90,90,91
        .byte 0,0,0,91,94,95,96,97,92,89,0,0,0,91,90,90,91,93,92,3,92,93,3,93,92,93,100,255,0,7,102,103,104
        .byte 105,93,89,0,0,0,100,93,92,93,92,93,92,93,92,93,92,93,92,89,255,0,7,102,103,103,105,92,89,0,0,0,91
        .byte 255,90,10,91,93,91,255,0,7,98,99,99,101,93,89,255,0,14,100,92,100,255,0,8,77,83,13,92,89,255,0,14,89
        .byte 93,89,255,0,4,91,90,90,90,77,78,93,93,89,255,0,14,89,92,89,255,0,4,100,93,92,93,77,78,92,92,89,255,0,14
        .byte 89,93,89,255,0,4,89,92,93,92,77,78,93,93,89,255,0,14,89,92,89,31,18,19,30,89,93,92,79,80,81,82,92
        .byte 89,94,95,96,97,0,0,0,79,80,81,82,0,0,0,89,93,89,0,16,17,0,89,92,93,13,77,83,13,93,89,102,103
        .byte 104,105,255,106,4,77,83,255,0,4,89,92,91,255,90,4,91,93,3,93,77,78,92,92,89,102,103,103,105,255,107,4,77
        .byte 78,255,0,4,89,93,92,93,92,93,92,93,92,93,92,77,78,93
        .byte 255,0,30,255,6,30,93,89,98,99,99,101,93,92,93,92,77,78,255,0,4,89,92,93,92,93,92,93,92,93,92,93,77
        .byte 78,92,92,89,13,77,78,13,92,93,92,79,80,81,82,0,0,0,91,255,90,10,77,78,93,93,89,94,95,96,97,93,92
        .byte 93,13,100,255,0,16,77,78,92,106,106,102,103,104,105,255,90,4,91,255,0,15,79,80,81,82,107,107,98,99,99,101,255,0,21
        .byte 77,83,13,13,89,255,0,25,77,78,93,93,89,255,0,25,77,78,92,92,89,255,0,25,77,78,93,93,89,24,255,0,5
        .byte 79,80,81,82,255,0,14,94,95,96,97,92,89,21,21,255,0,5,77,83,255,0,15,102,103,104,105,93,91,90,90,91,0,0
        .byte 91,90,77,78,255,0,15,102,103,103,105,92,93,92,93,100,0,0,100,92,77,78,255,0,15,98,99,99,101,93,92,93,92
        .byte 89,0,0,89,93,77,78,255,0,16,100,13,13,92,3,3,93,89,0,0,89,92,77,78,255,0,16,89,92,93,93,3,3
        .byte 92,89,0,0,89,93,77,78,90,90,90,91,255,0,12,89,93,92,92,3,3,93,89,0,0,89,92,77,78,93,92,93,100
        .byte 255,0,12,89,92,93,93,92,93,92,89,108,109,89,94,95,96,97,93,92,89,255,0,12,89,93,92,255,106,8,102,103,104
        .byte 105,92,93,89,255,0,12,89,92,93,255,107,8,102,103,103,105,93,92,91,255,90,5,91,0,0,0,91,90,90,91,93,92
        .byte 255,13,8,98,99,99,101,92,93,92,93,92,93,92,93,100,0,0,0,100,93,92,93,92,93
        .byte 255,0,30,255,6,30,93,92,93,92,93,92,93,92,77,78,93,92,93,92,93,92,93,92,93,92,89,0,0,0,89,92,93
        .byte 92,93,92,92,91,255,90,5,79,80,81,82,255,90,9,91,0,0,0,91,90,90,90,91,93,93,100,255,0,26,100,92,92
        .byte 89,255,0,26,89,93,93,89,0,24,255,0,24,89,92,92,89,0,24,255,0,22,24,0,89,93,93,91,90,90,91,255,0,16
        .byte 94,95,96,97,0,21,21,89,92,92,100,255,0,19,102,103,104,105,255,106,5,93,89,255,0,19,102,103,103,105,255,107,5
        .byte 92,89,0,24,255,0,17,98,99,99,101,255,13,5,93,89,0,24,255,0,18,77,83,92,1,2,93,93,92,92,91,90,90
        .byte 91,255,0,7,94,95,96,97,255,0,6,77,78,93,93,92,93,3,93,93,100,255,0,10,102,103,104,105,255,0,5,94,95
        .byte 96,97,92,93,92,93,92,92,89,255,0,10,102,103,103,105,255,0,5,102,103,104,105,255,106,5,93,89,255,0,10,98,99,99
        .byte 101,255,0,5,102,103,103,105,255,107,5,92,89,255,0,11,77,83,255,0,6,98,99,99,101,255,13,5,93,89,255,0,10
        .byte 79,80,81,82,0,0,24,0,0,0,77,83,89,79,80,81,82,92,92,89,255,0,11,77,83,0,0,0,21,21,0,0,77
        .byte 78,89,13,77,83,13,93,93,91,90,91,0,0,0,91,255,90,5,77,78,255,90,7,77,78,91,92,77,78,93,92,92,93
        .byte 92,100,0,0,0,100,92,93,92,93,92,77,78,93,92,93,92,93,92,93,77,78,92,93,77,78,92,93
        .byte 255,0,30,255,6,30,92,93,92,89,0,0,0,89,92,93,92,93,92,77,78,93,92,93,92,93,92,93,77,78,92,93,77
        .byte 78,92,93,93,91,90,91,0,0,0,91,255,90,5,77,78,255,90,6,79,80,81,82,90,77,78,93,92,92,100,255,0,11
        .byte 77,78,255,0,5,86,87,255,0,4,77,78,92,93,93,89,255,0,10,79,80,81,82,255,0,10,77,78,93,92,92,89,255,0,23
        .byte 79,80,81,82,93,93,89,255,0,25,89,13,92,92,89,255,0,25,89,92,93,93,89,94,95,96,97,255,0,19,24,0,89
        .byte 93,92,106,106,102,103,104,105,255,0,19,21,21,89,92,93,107,107,102,103,103,105,255,0,18,94,95,96,97,93,92,13,89
        .byte 98,99,99,101,255,0,18,102,103,104,105,106,106,93,89,255,0,22,102,103,103,105,107,107,92,89,255,0,22,98,99,99,101
        .byte 13,13,93,89,255,0,25,89,93,92,92,89,24,255,0,24,89,92,93,93,89,21,21,255,0,23,89,93,92,92,91,90,90,90
        .byte 91,0,0,91,90,90,91,0,0,79,80,81,82,255,0,9,89,92,93,93,92,93,92,93,100,108,109,100,92,93,100,108,109
        .byte 0,77,83,255,0,10,89,93,92,92,3,92,93,92,91,90,90,91,93,92,91,90,90,90,77,78,255,90,7,91,0,0,89
        .byte 92,93,93,92,93,92,93,92,93,92,93,92,93,92,93,92,93,77,78,92,93,92,93,92,93,92,100,0,0,89,93,92
        .byte 255,0,30,255,6,30,92,93,92,93,92,93,92,93,92,93,92,93,92,93,92,77,78,93,92,93,92,93,92,93,89,0,0
        .byte 89,92,93,93,91,255,90,9,91,93,92,79,80,81,82,91,255,90,5,91,0,0,89,93,92,92,89,255,0,9,89,92,93
        .byte 92,93,92,93,89,86,87,255,0,6,89,92,93,93,89,24,255,0,8,89,93,92,91,90,90,90,91,255,0,8,89,93,92,92
        .byte 89,21,21,255,0,7,89,92,93,89,255,0,12,89,92,93,93,91,90,90,91,255,0,6,91,90,90,91,255,0,12,89,93
        .byte 92,92,93,92,93,89,255,0,22,89,92,93,93,92,93,92,89,255,0,19,94,95,96,97,93,92,92,93,92,93,94,95,96
        .byte 97,0,24,255,0,14,102,103,104,105,255,106,6,102,103,104,105,0,24,255,0,14,102,103,103,105,255,107,6,102,103,103,105
        .byte 90,90,91,255,0,13,98,99,99,101,92,93,93,92,93,92,98,99,99,101,93,92,89,255,0,13,91,90,90,90,91,92,92
        .byte 91,255,90,8,91,255,0,5,91,90,91,255,0,9,89,93,93,89,255,0,14,89,92,89,255,0,9,89,92,92,89,255,0,14
        .byte 89,93,91,90,91,255,0,7,89,93,93,89,255,0,12,91,90,91,92,93,92,89,255,0,7,89,92,92,89,255,0,12,89
        .byte 93,92,93,92,93,79,80,81,82,31,18,19,30,89,93,93,89,0,0,91,79,80,81,82,255,90,5,91,92,93,3,93,92
        .byte 89,77,83,0,0,16,17,0,89,92,92,89,0,0,89,93,77,83,92,93,92,93,92,93,92,93,92,93,92,93,91,77,78
        .byte 255,90,5,91,93,93,89,0,0,89,92,77,78,93,92,93,92,93,92,93,92,93,3,93,92,93,77,78,92,93,92,93,92
        .byte 93,92
        .byte 255,0,30,255,6,30,93,89,0,0,89,92,77,78,93,92,93,92,93,92,93,92,93,92,93,92,93,77,78,92,93,92,93
        .byte 92,93,92,92,89,0,0,91,90,77,78,255,90,13,77,78,255,90,5,91,93,93,89,0,0,0,79,80,81,82,255,0,11
        .byte 79,80,81,82,255,0,4,100,92,92,89,255,0,26,89,93,93,89,255,0,16,91,90,91,255,0,5,24,0,89,92,92,89
        .byte 255,0,16,100,93,100,255,0,5,21,21,89,93,93,91,90,91,255,0,14,91,90,91,255,0,4,91,90,90,91,92,92,93
        .byte 92,100,255,0,21,100,92,93,92,93,93,91,90,91,255,0,7,91,90,91,255,0,11,91,90,90,91,92,92,100,255,0,9
        .byte 100,92,100,255,0,14,89,93,93,89,255,0,9,91,90,91,255,0,14,89,92,92,89,255,0,26,89,93,93,89,0,0,0
        .byte 91,90,91,255,0,12,91,90,91,255,0,5,89,92,92,89,0,0,0,100,93,100,255,0,12,100,92,100,255,0,5,89,93,93
        .byte 89,0,0,0,91,90,91,0,0,0,91,32,33,91,255,0,5,91,90,91,255,0,5,89,92,92,89,255,0,9,100,32,33
        .byte 100,255,0,13,89,93,93,89,0,0,0,24,255,0,5,89,32,33,89,255,0,13,89,92,92,89,0,0,0,21,21,255,0,4
        .byte 89,32,33,89,20,20,0,0,0,91,255,90,4,91,0,0,89,93,93,91,255,90,9,91,32,33,91,255,90,5,91,34,35
        .byte 92,93,100,0,0,89,92,92,93,92,93,92,93,92,93,92,93,92,93,32,33,92,93,92,93,92,93,92,36,37,38,39,89
        .byte 0,0,89,93
        .byte 255,0,30,255,6,30,115,115,255,118,8,115,115,115,255,118,9,115,115,115,116,0,0,117,115,115,116,255,0,5,86,87,0
        .byte 117,115,116,255,0,9,117,115,115,116,0,0,117,115,115,116,255,0,8,122,118,123,255,0,9,122,118,118,123,0,0,117,115,115
        .byte 116,255,0,26,117,115,115,116,255,0,26,117,115,115,116,255,0,26,117,115,115,116,255,0,8,121,255,119,9,120,255,0,7
        .byte 117,115,115,115,120,255,0,7,117,255,115,10,255,119,7,255,115,5,120,255,0,6,122,255,115,5,255,118,10,255,115,7,116
        .byte 112,255,0,6,122,118,118,118,123,255,0,10,122,255,115,6,116,113,255,0,22,122,255,115,5,123,255,0,20,94,95,96,97
        .byte 117,115,115,115,123,255,0,21,102,103,104,105,106,106,115,115,120,79,80,81,82,255,0,17,102,103,103,105,107,107,115,115,115
        .byte 120,77,83,255,0,18,98,99,99,101,117,255,115,5,77,78,255,0,22,117,255,115,5,77,78,255,0,22,117,255,115,5,77
        .byte 78,120,255,0,10,108,109,255,0,9,117,255,115,5,77,78,115,255,119,9,79,80,81,82,255,119,5,120,0,0,117,255,115,5
        .byte 77,78,255,115,11,77,83,255,115,6,116,0,0,117,115
        .byte 255,0,30,255,6,30,115,115,118,118,77,78,255,118,11,77,78,118,118,255,115,4,116,0,0,117,115,115,123,0,79,80,81
        .byte 82,255,0,9,79,80,81,82,0,122,118,118,118,123,0,0,117,115,116,255,0,27,117,115,116,255,0,27,117,115,116,255,0,27
        .byte 117,115,116,0,21,21,255,0,23,111,117,115,115,120,20,20,255,0,23,110,117,115,115,115,119,119,120,255,0,23,117,115,115
        .byte 118,118,118,123,255,0,15,121,119,119,120,255,0,4,117,115,116,255,0,19,117,115,115,116,255,124,6,116,255,0,19,122,118,118
        .byte 123,255,0,4,117,115,116,112,255,0,26,117,115,116,113,255,0,26,117,115,116,255,0,27,117,115,116,255,0,27,117,115,116
        .byte 255,0,27,117,115,116,255,0,17,79,80,81,82,255,0,6,117,115,116,255,0,14,21,21,0,0,77,83,255,0,7,117,115,115
        .byte 120,0,0,0,121,255,119,4,120,0,108,109,0,20,20,0,0,77,78,255,0,6,121,115,115,115,116,0,0,0,117,255,115,5
        .byte 255,119,8,77,78,255,119,6,115,115,115
        .byte 255,0,30,255,6,30,115,116,0,0,0,117,255,115,8,255,118,14,115,115,115,123,0,0,0,122,255,115,7,123,0,0,0
        .byte 86,87,255,0,9,122,115,116,255,0,5,122,118,118,115,115,115,116,255,0,14,21,21,117,116,255,0,8,122,115,115,116,112
        .byte 255,0,13,20,20,117,116,255,0,9,117,115,116,113,255,0,12,121,119,119,115,116,255,0,9,117,115,116,255,0,13,122,118,118
        .byte 115,116,112,255,0,7,121,115,115,116,255,0,16,117,116,113,255,0,5,121,119,115,115,115,116,255,0,16,117,116,255,0,6
        .byte 117,255,115,4,116,255,0,12,121,120,0,0,117,116,255,0,6,122,118,118,115,115,116,255,0,12,117,116,124,124,124,116,255,0,9
        .byte 122,115,116,0,0,0,121,119,119,120,255,0,5,122,123,0,111,117,116,255,0,10,117,116,0,0,0,117,115,115,116,255,0,8
        .byte 110,117,115,120,255,0,9,117,116,0,0,0,122,118,118,123,255,0,9,117,115,115,119,120,255,0,6,121,115,116,255,0,4
        .byte 125,125,255,0,9,121,115,115,118,118,123,255,0,4,121,119,115,115,123,255,0,4,125,125,255,0,8,121,115,115,116,255,0,7
        .byte 122,118,118,123,255,0,4,121,125,125,120,255,0,6,121,115,115,115,116,255,0,15,117,125,125,116,0,0,0,108,109,121,255,115,4
        .byte 116,255,0,14,121,115,125,125,116,0,0,121,119,119,255,115,6,120,0,0,108,109,255,0,8,121,115,115,125,125,116,0,0
        .byte 117,255,115,9,255,119,12,115,115,115,125,125,116,0,0,117,255,115,7
        .byte 255,0,30,255,6,30,115,115,125,255,118,13,115,115,115,116,0,0,117,115,115,118,118,118,115,115,115,123,125,255,0,9,86
        .byte 87,0,0,122,118,118,123,0,0,122,118,123,0,0,0,122,115,116,121,119,120,255,0,25,117,116,122,118,123,255,0,25,117
        .byte 116,255,0,28,117,116,255,0,5,121,119,119,120,255,0,15,31,18,19,30,117,116,255,0,4,121,115,115,118,123,255,0,16
        .byte 16,17,121,115,116,255,0,4,117,115,123,255,0,16,121,119,119,119,115,115,116,255,0,4,117,116,255,0,17,122,255,115,5
        .byte 116,255,0,4,117,116,255,0,18,122,255,115,4,116,255,0,4,117,116,255,0,19,122,115,115,115,116,255,0,4,117,116,255,0,20
        .byte 122,115,115,116,255,0,4,117,116,0,0,121,255,119,4,120,255,0,4,121,119,119,120,255,0,5,122,115,116,255,0,4,117
        .byte 116,0,0,122,255,115,5,120,0,0,0,117,115,115,115,120,255,0,5,117,116,255,0,4,117,116,0,0,0,122,255,115,5
        .byte 120,0,0,117,115,115,115,116,255,0,5,117,116,255,0,4,117,116,255,0,4,122,255,115,4,116,0,0,117,115,115,115,116
        .byte 255,0,5,117,116,0,21,21,0,117,115,120,255,0,4,122,115,115,115,116,0,0,117,255,115,4,120,0,21,21,0,117,115
        .byte 120,20,20,121,115,115,115,120,255,0,4,117,115,115,116,108,109,117,255,115,5,120,20,20,121,115,115,115,119,119,255,115,5
        .byte 120,0,0,121,255,115,4,119,119,255,115,7,119,119,255,115,11,116,0,0,117,255,115,17
        .byte 255,0,30,255,6,30,115,115,255,118,7,123,0,0,122,255,115,4,118,125,255,118,9,115,115,115,123,255,0,11,117,115,115
        .byte 123,0,125,255,0,9,122,115,116,255,0,5,14,14,255,0,5,117,115,116,0,121,119,120,255,0,7,21,21,117,116,0,0
        .byte 255,14,5,255,0,5,117,115,116,0,117,115,116,255,0,7,20,20,117,116,0,255,14,5,255,0,6,117,115,116,0,122,118
        .byte 123,255,0,6,121,119,119,115,116,0,0,14,14,0,14,255,0,6,117,115,116,255,0,10,122,118,115,115,116,0,0,255,14,5
        .byte 255,0,5,117,115,116,255,0,12,122,115,116,0,0,255,14,4,255,0,6,117,115,116,255,0,13,117,116,0,0,0,14,14
        .byte 255,0,7,117,115,116,255,0,13,117,116,0,0,255,14,4,255,0,6,117,115,116,255,0,4,121,255,119,5,120,0,0,117
        .byte 116,0,255,14,6,255,0,5,122,118,123,255,0,4,117,115,115,118,115,115,116,0,0,117,116,0,255,14,6,0,24,255,0,10
        .byte 117,115,123,125,122,115,116,0,0,117,116,0,255,14,6,24,24,0,0,0,121,119,120,255,0,4,117,116,0,125,0,117,116
        .byte 0,0,117,116,255,14,4,0,255,14,4,0,0,0,117,115,116,255,0,4,117,116,124,124,124,117,116,0,0,117,116,255,14,5
        .byte 0,14,14,255,0,4,122,118,123,255,0,4,117,116,0,125,0,117,116,0,0,117,116,0,0,255,14,4,255,0,13,117,116
        .byte 0,125,0,117,116,0,0,117,116,0,14,14,14,0,14,14,255,0,11,121,115,116,124,124,124,117,116,0,0,117,116,0,14,14
        .byte 0,14,14,14,255,0,10,121,115,115,116,0,125,0,117,116,0,0,117,116,0,14,14,14,0,14,14,14,255,0,8,121,115,115,115
        .byte 116,0,125,0,117,116,0,0,117,115,255,119,16,255,115,4,116,0,125,0,117,116,0,0,117
        .byte 255,0,30,255,6,30,115,115,255,118,6,255,115,4,118,118,118,255,115,4,118,118,118,115,120,0,121,116,0,0,117,115,123
        .byte 86,87,255,0,4,122,115,115,123,0,0,0,122,115,115,123,0,0,0,122,115,119,115,123,0,0,117,116,255,0,8,122,123
        .byte 255,0,5,122,123,255,0,5,122,118,123,0,0,0,117,116,255,0,27,121,115,116,255,0,26,121,115,115,116,255,0,26,117
        .byte 115,115,116,255,0,4,121,255,119,21,115,115,115,116,255,0,4,117,255,115,7,255,118,10,115,115,255,118,5,116,255,0,4
        .byte 122,118,118,118,115,115,115,123,255,0,10,122,115,120,255,0,4,116,255,0,8,122,115,116,255,0,12,122,115,120,0,0,0
        .byte 116,255,0,6,21,21,0,117,116,255,0,13,122,118,119,119,119,116,255,0,6,20,20,0,117,116,255,0,15,122,115,115,116
        .byte 255,0,4,121,255,119,4,115,123,255,0,7,121,119,120,255,0,6,122,115,116,255,0,4,117,255,115,4,123,255,0,8,117
        .byte 115,116,255,0,7,117,116,255,0,4,122,118,118,118,123,255,0,9,122,118,123,0,121,119,120,0,0,0,117,116,255,0,12
        .byte 121,119,120,255,0,4,125,0,0,117,115,116,0,0,0,117,116,255,0,12,117,115,116,255,0,4,125,0,0,122,118,123,0,0,0
        .byte 117,116,255,0,12,122,118,123,255,0,4,125,0,0,0,125,255,0,4,117,115,120,255,0,12,125,0,108,109,121,119,125,119,119,119
        .byte 125,119,120,0,0,117,115,115,255,119,12,125,119,119,119,115,115,125,115,115,115,125,115,116,0,0,117
        .byte 255,0,30,255,6,30,255,0,7,125,0,0,125,255,0,6,125,255,0,5,117,115,115,116,0,0,117,255,0,7,125,0,0
        .byte 125,255,0,6,125,255,0,5,117,115,115,123,0,0,117,92,255,0,6,126,0,0,126,255,0,6,125,255,0,5,117,115,123
        .byte 0,0,121,115,93,92,0,0,0,92,0,14,0,14,14,14,0,14,0,0,0,126,255,0,4,121,115,116,0,0,121,115,115
        .byte 92,93,92,14,0,93,0,14,0,14,0,0,0,14,0,0,0,14,255,0,4,117,115,116,0,0,117,115,115,124,124,124,14
        .byte 93,92,0,14,0,14,14,0,0,14,0,85,0,14,255,0,4,117,115,116,0,0,117,115,115,92,93,92,14,92,14,0,14
        .byte 0,14,0,0,0,14,0,84,0,14,255,0,4,117,115,116,0,0,117,115,123,93,92,93,255,14,5,0,14,14,14,0,14,14,14
        .byte 0,14,14,14,0,0,117,115,123,0,0,117,123,0,92,93,92,93,92,93,0,0,0,126,0,0,0,126,0,126,0,0,0
        .byte 125,0,121,115,123,0,0,121,123,0,0,93,92,93,14,14,92,0,14,14,14,0,14,14,14,0,14,14,0,0,125,0,117
        .byte 116,0,0,121,116,0,0,0,92,93,92,14,92,14,0,14,0,14,0,14,0,14,0,14,0,0,0,125,0,117,116,0,0
        .byte 117,116,255,124,6,14,93,14,0,14,0,14,0,14,0,14,0,14,14,0,0,125,0,117,116,0,0,117,116,0,0,0,92
        .byte 93,92,14,92,14,0,14,0,14,0,14,0,14,0,14,0,0,0,125,0,117,116,0,0,122,115,120,0,0,0,92,93,14,14
        .byte 0,0,14,14,14,0,14,0,14,0,255,14,4,125,0,117,115,120,0,0,122,115,120,0,0,0,92,255,0,5,125,255,0,4
        .byte 125,0,0,125,0,0,125,0,122,115,115,120,0,0,122,115,120,0,92,255,0,6,125,255,124,7,125,0,0,125,0,0,122
        .byte 115,115,120,0,0,122,115,255,0,5,85,0,0,125,255,0,7,125,0,0,125,0,0,0,122,115,115,120,0,0,117,255,0,5
        .byte 84,0,0,125,255,0,7,125,0,0,125,255,0,4,117,115,116,0,0,117,255,124,8,125,255,0,7,125,0,0,125,255,0,4
        .byte 117,115,116,0,0,117,255,0,8,125,255,0,7,125,0,0,125,255,0,4,117,115,116,0,0,117
        .byte 255,0,0,255,0,100,79,80,81,82,255,0,27,77,83,255,0,28,77,78,255,0,28,77,78,255,0,28,77,78,255,0,28
        .byte 77,78,255,0,28,77,78,255,0,28,77,78,255,0,22,31,18,19,30,0,121,119,119,119,255,0,22,16,17,0,0,117,1
        .byte 2,115,255,0,14,121,255,119,11,117,115,115,115
numsc:
        .byte 40
.out .sprintf("SCREENS SIZE = %d", (* - scdat))

nmedat:
        .byte 0,0,32,104,1,1,24,8,255
        .byte 0,0,24,104,1,1,24,8,3,4,120,72,255
        .byte 7,5,112,80,0,0,24,80,1,1,24,8,3,4,136,24,255
        .byte 0,0,24,216,1,1,24,8,3,4,72,72,255
        .byte 7,5,80,24,0,0,24,88,1,1,24,8,255
        .byte 0,0,24,200,1,1,24,8,5,7,152,152,255
        .byte 0,0,24,224,1,1,24,8,7,5,112,56,255
        .byte 0,0,24,152,1,1,16,8,6,8,160,144,255
        .byte 0,0,32,144,1,1,16,8,5,7,144,152,255
        .byte 0,0,24,56,1,1,16,8,7,5,80,40,6,8,128,184,255
        .byte 0,0,24,96,1,1,16,8,6,8,104,32,6,8,120,200,6,8,152,200,255
        .byte 0,0,24,200,1,1,16,8,6,8,120,200,3,2,144,120,255
        .byte 0,0,24,40,1,1,16,8,6,8,80,40,3,2,136,88,3,2,80,144,255
        .byte 7,5,96,24,5,7,160,128,0,0,24,40,1,1,16,8,255
        .byte 6,8,120,32,6,8,104,96,6,8,144,176,0,0,24,176,1,1,16,8,255
        .byte 0,0,24,176,1,1,16,8,6,8,96,208,3,2,144,136,8,3,96,56,255
        .byte 0,0,24,32,1,1,16,8,5,7,160,96,8,3,144,144,255
        .byte 0,0,24,216,1,1,24,8,7,5,64,64,3,2,136,168,255
        .byte 0,0,24,32,1,1,16,8,3,2,120,176,6,8,120,216,6,8,168,120,255
        .byte 0,0,24,120,1,1,16,8,7,5,64,48,7,5,112,40,255
        .byte 0,0,24,120,1,1,16,8,7,5,112,16,8,3,64,160,255
        .byte 0,0,24,136,1,1,16,8,5,7,160,80,255
        .byte 0,0,24,120,1,1,16,8,6,2,120,72,6,2,56,168,255
        .byte 0,0,24,176,1,1,16,8,6,2,40,72,6,2,64,128,255
        .byte 0,0,24,40,1,1,16,8,6,2,48,136,6,2,104,72,255
        .byte 0,0,24,104,1,1,16,8,3,2,128,80,6,2,48,184,255
        .byte 0,0,24,216,1,1,16,8,6,2,48,32,8,3,56,112,255
        .byte 0,0,24,120,1,1,16,8,3,2,120,48,5,7,120,96,255
        .byte 0,0,24,184,1,1,16,8,7,5,56,24,7,5,96,24,255
        .byte 0,0,24,40,1,1,16,8,3,2,136,56,3,2,88,104,6,2,72,168,255
        .byte 0,0,24,208,1,1,16,8,6,2,48,160,7,5,88,72,255
        .byte 0,0,24,24,1,1,16,8,5,7,128,104,255
        .byte 0,0,24,216,1,1,16,8,8,3,56,128,6,2,40,64,3,2,136,144,255
        .byte 0,0,24,216,1,1,24,8,3,2,144,104,5,7,152,64,255
        .byte 0,0,24,24,1,1,24,8,3,2,152,40,3,2,64,192,6,2,72,144,255
        .byte 0,0,24,168,1,1,16,8,3,2,136,144,6,2,40,104,255
        .byte 7,5,112,72,0,0,24,88,1,1,24,8,8,3,80,192,255
        .byte 0,0,24,224,1,1,24,8,6,2,56,24,8,3,56,64,3,2,136,136,255
        .byte 0,0,24,224,255
.out .sprintf("POSITIONS SIZE = %d", (* - nmedat))

NUMOBJ = 61
objdta:
        .byte 14,32,10,80,14,232,32,124,87,184,235,182,117,171,0,0,255,255,213,85,170,171,255,255,0,0,64,2,0,0,224,7,0,136,72,0,136,72
        .byte 14,32,10,80,14,232,32,124,87,184,235,182,117,171,0,0,255,255,213,85,170,171,255,255,0,0,64,2,0,0,224,7,1,80,16,1,80,16
        .byte 14,32,10,80,14,232,32,124,87,184,235,182,117,171,0,0,255,255,213,85,170,171,255,255,0,0,64,2,0,0,224,7,1,48,216,1,48,216
        .byte 14,32,10,80,14,232,32,124,87,184,235,182,117,171,0,0,255,255,213,85,170,171,255,255,0,0,64,2,0,0,224,7,1,160,200,1,160,200
        .byte 14,32,10,80,14,232,32,124,87,184,235,182,117,171,0,0,255,255,213,85,170,171,255,255,0,0,64,2,0,0,224,7,2,64,24,2,64,24
        .byte 14,32,10,80,14,232,32,124,87,184,235,182,117,171,0,0,255,255,213,85,170,171,255,255,0,0,64,2,0,0,224,7,2,136,112,2,136,112
        .byte 14,32,10,80,14,232,32,124,87,184,235,182,117,171,0,0,255,255,213,85,170,171,255,255,0,0,64,2,0,0,224,7,2,40,200,2,40,200
        .byte 14,32,10,80,14,232,32,124,87,184,235,182,117,171,0,0,255,255,213,85,170,171,255,255,0,0,64,2,0,0,224,7,3,104,24,3,104,24
        .byte 14,32,10,80,14,232,32,124,87,184,235,182,117,171,0,0,255,255,213,85,170,171,255,255,0,0,64,2,0,0,224,7,3,152,200,3,152,200
        .byte 14,32,10,80,14,232,32,124,87,184,235,182,117,171,0,0,255,255,213,85,170,171,255,255,0,0,64,2,0,0,224,7,4,136,64,4,136,64
        .byte 14,32,10,80,14,232,32,124,87,184,235,182,117,171,0,0,255,255,213,85,170,171,255,255,0,0,64,2,0,0,224,7,4,72,216,4,72,216
        .byte 14,32,10,80,14,232,32,124,87,184,235,182,117,171,0,0,255,255,213,85,170,171,255,255,0,0,64,2,0,0,224,7,5,88,24,5,88,24
        .byte 14,32,10,80,14,232,32,124,87,184,235,182,117,171,0,0,255,255,213,85,170,171,255,255,0,0,64,2,0,0,224,7,5,152,24,5,152,24
        .byte 14,32,10,80,14,232,32,124,87,184,235,182,117,171,0,0,255,255,213,85,170,171,255,255,0,0,64,2,0,0,224,7,6,48,24,6,48,24
        .byte 14,32,10,80,14,232,32,124,87,184,235,182,117,171,0,0,255,255,213,85,170,171,255,255,0,0,64,2,0,0,224,7,6,120,32,6,120,32
        .byte 14,32,10,80,14,232,32,124,87,184,235,182,117,171,0,0,255,255,213,85,170,171,255,255,0,0,64,2,0,0,224,7,6,152,216,6,152,216
        .byte 21,168,64,2,30,232,93,218,27,184,87,122,0,0,95,250,0,0,255,255,0,0,221,187,221,187,193,131,0,0,193,131,33,64,24,33,64,24
        .byte 0,0,0,0,0,0,0,192,5,32,10,208,53,124,107,172,0,0,123,222,59,220,12,48,23,232,32,4,0,0,112,14,9,144,40,9,144,40
        .byte 0,0,0,0,0,0,0,192,5,32,10,208,53,124,107,172,0,0,123,222,59,220,12,48,23,232,32,4,0,0,112,14,9,152,184,9,152,184
        .byte 0,0,0,0,0,0,0,192,5,32,10,208,53,124,107,172,0,0,123,222,59,220,12,48,23,232,32,4,0,0,112,14,9,48,208,9,48,208
        .byte 0,0,0,0,0,0,0,192,5,32,10,208,53,124,107,172,0,0,123,222,59,220,12,48,23,232,32,4,0,0,112,14,10,112,120,10,112,120
        .byte 0,0,0,0,0,0,0,192,5,32,10,208,53,124,107,172,0,0,123,222,59,220,12,48,23,232,32,4,0,0,112,14,11,72,24,11,72,24
        .byte 0,0,0,0,0,0,0,192,5,32,10,208,53,124,107,172,0,0,123,222,59,220,12,48,23,232,32,4,0,0,112,14,11,144,200,11,144,200
        .byte 0,0,0,0,0,0,0,192,5,32,10,208,53,124,107,172,0,0,123,222,59,220,12,48,23,232,32,4,0,0,112,14,12,56,216,12,56,216
        .byte 0,0,0,0,0,0,0,192,5,32,10,208,53,124,107,172,0,0,123,222,59,220,12,48,23,232,32,4,0,0,112,14,12,144,200,12,144,200
        .byte 0,0,0,0,0,0,0,192,5,32,10,208,53,124,107,172,0,0,123,222,59,220,12,48,23,232,32,4,0,0,112,14,13,152,40,13,152,40
        .byte 0,0,0,0,0,0,0,192,5,32,10,208,53,124,107,172,0,0,123,222,59,220,12,48,23,232,32,4,0,0,112,14,14,152,32,14,152,32
        .byte 0,0,0,0,0,0,0,192,5,32,10,208,53,124,107,172,0,0,123,222,59,220,12,48,23,232,32,4,0,0,112,14,15,128,208,15,128,208
        .byte 0,0,0,0,15,240,31,152,31,200,31,200,31,200,0,0,127,254,127,254,0,0,61,188,13,176,54,108,119,238,224,7,16,56,216,16,56,216
        .byte 0,0,0,0,15,240,31,152,31,200,31,200,31,200,0,0,127,254,127,254,0,0,61,188,13,176,54,108,119,238,224,7,16,88,216,16,88,216
        .byte 0,0,0,0,15,240,31,152,31,200,31,200,31,200,0,0,127,254,127,254,0,0,61,188,13,176,54,108,119,238,224,7,16,104,24,16,104,24
        .byte 0,0,0,0,15,240,31,152,31,200,31,200,31,200,0,0,127,254,127,254,0,0,61,188,13,176,54,108,119,238,224,7,17,128,224,17,128,224
        .byte 0,0,0,0,15,240,31,152,31,200,31,200,31,200,0,0,127,254,127,254,0,0,61,188,13,176,54,108,119,238,224,7,18,152,216,18,152,216
        .byte 0,0,0,0,15,240,31,152,31,200,31,200,31,200,0,0,127,254,127,254,0,0,61,188,13,176,54,108,119,238,224,7,20,80,32,20,80,32
        .byte 0,0,0,0,15,240,31,152,31,200,31,200,31,200,0,0,127,254,127,254,0,0,61,188,13,176,54,108,119,238,224,7,20,144,216,20,144,216
        .byte 0,0,0,0,15,240,31,152,31,200,31,200,31,200,0,0,127,254,127,254,0,0,61,188,13,176,54,108,119,238,224,7,20,64,216,20,64,216
        .byte 0,0,0,0,15,240,31,152,31,200,31,200,31,200,0,0,127,254,127,254,0,0,61,188,13,176,54,108,119,238,224,7,21,56,24,21,56,24
        .byte 0,0,0,0,15,240,31,152,31,200,31,200,31,200,0,0,127,254,127,254,0,0,61,188,13,176,54,108,119,238,224,7,21,152,216,21,152,216
        .byte 0,0,0,0,15,240,31,152,31,200,31,200,31,200,0,0,127,254,127,254,0,0,61,188,13,176,54,108,119,238,224,7,21,136,40,21,136,40
        .byte 0,0,0,0,15,240,31,152,31,200,31,200,31,200,0,0,127,254,127,254,0,0,61,188,13,176,54,108,119,238,224,7,22,64,32,22,64,32
        .byte 0,0,0,0,15,240,31,152,31,200,31,200,31,200,0,0,127,254,127,254,0,0,61,188,13,176,54,108,119,238,224,7,22,152,24,22,152,24
        .byte 0,0,0,0,15,240,31,152,31,200,31,200,31,200,0,0,127,254,127,254,0,0,61,188,13,176,54,108,119,238,224,7,22,144,208,22,144,208
        .byte 0,0,0,0,15,240,31,152,31,200,31,200,31,200,0,0,127,254,127,254,0,0,61,188,13,176,54,108,119,238,224,7,23,104,40,23,104,40
        .byte 0,0,0,0,15,240,31,152,31,200,31,200,31,200,0,0,127,254,127,254,0,0,61,188,13,176,54,108,119,238,224,7,23,144,216,23,144,216
        .byte 3,192,12,48,31,136,63,196,63,164,125,66,122,162,117,66,0,0,123,222,59,220,89,154,106,86,115,206,0,0,115,206,24,56,200,24,56,200
        .byte 3,192,12,48,31,136,63,196,63,164,125,66,122,162,117,66,0,0,123,222,59,220,89,154,106,86,115,206,0,0,115,206,25,56,56,25,56,56
        .byte 3,192,12,48,31,136,63,196,63,164,125,66,122,162,117,66,0,0,123,222,59,220,89,154,106,86,115,206,0,0,115,206,27,88,24,27,88,24
        .byte 3,192,12,48,31,136,63,196,63,164,125,66,122,162,117,66,0,0,123,222,59,220,89,154,106,86,115,206,0,0,115,206,28,64,216,28,64,216
        .byte 3,192,12,48,31,136,63,196,63,164,125,66,122,162,117,66,0,0,123,222,59,220,89,154,106,86,115,206,0,0,115,206,28,152,152,28,152,152
        .byte 3,192,12,48,31,136,63,196,63,164,125,66,122,162,117,66,0,0,123,222,59,220,89,154,106,86,115,206,0,0,115,206,29,136,24,29,136,24
        .byte 3,192,12,48,31,136,63,196,63,164,125,66,122,162,117,66,0,0,123,222,59,220,89,154,106,86,115,206,0,0,115,206,29,80,208,29,80,208
        .byte 3,192,12,48,31,136,63,196,63,164,125,66,122,162,117,66,0,0,123,222,59,220,89,154,106,86,115,206,0,0,115,206,30,48,24,30,48,24
        .byte 3,192,12,48,31,136,63,196,63,164,125,66,122,162,117,66,0,0,123,222,59,220,89,154,106,86,115,206,0,0,115,206,31,152,48,31,152,48
        .byte 3,192,12,48,31,136,63,196,63,164,125,66,122,162,117,66,0,0,123,222,59,220,89,154,106,86,115,206,0,0,115,206,31,56,216,31,56,216
        .byte 0,0,0,0,0,0,0,192,5,32,10,208,53,124,107,172,0,0,123,222,59,220,12,48,23,232,32,4,0,0,112,14,8,96,96,8,96,96
        .byte 0,0,0,0,0,0,0,192,5,32,10,208,53,124,107,172,0,0,123,222,59,220,12,48,23,232,32,4,0,0,112,14,8,128,200,8,128,200
        .byte 21,168,64,2,30,232,93,218,27,184,87,122,0,0,95,250,0,0,255,255,0,0,221,187,221,187,193,131,0,0,193,131,33,160,128,33,160,128
        .byte 21,168,64,2,30,232,93,218,27,184,87,122,0,0,95,250,0,0,255,255,0,0,221,187,221,187,193,131,0,0,193,131,34,40,224,34,40,224
        .byte 21,168,64,2,30,232,93,218,27,184,87,122,0,0,95,250,0,0,255,255,0,0,221,187,221,187,193,131,0,0,193,131,35,152,24,35,152,24
        .byte 21,168,64,2,30,232,93,218,27,184,87,122,0,0,95,250,0,0,255,255,0,0,221,187,221,187,193,131,0,0,193,131,35,152,216,35,152,216
        .byte 21,168,64,2,30,232,93,218,27,184,87,122,0,0,95,250,0,0,255,255,0,0,221,187,221,187,193,131,0,0,193,131,36,40,224,36,40,224
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
        .byte 0,0,126,66,98,98,126,0
        .byte 0,0,56,8,12,12,126,0
        .byte 0,0,124,12,124,64,124,0
        .byte 0,0,126,2,30,6,126,0
        .byte 0,0,98,98,126,2,2,0
        .byte 0,0,126,96,126,2,126,0
        .byte 0,0,64,64,126,98,126,0
        .byte 0,0,126,6,6,14,14,0
        .byte 0,0,60,36,126,66,126,0
        .byte 0,0,126,66,126,6,6,0
        .byte 0,0,0,48,0,0,48,0
        .byte 0,0,48,0,0,48,48,96
        .byte 0,0,12,24,48,24,12,0
        .byte 0,0,0,126,0,126,0,0
        .byte 0,0,48,24,12,24,48,0
        .byte 0,124,198,12,24,0,24,0
        .byte 0,124,222,254,254,192,124,0
        .byte 0,0,126,66,126,98,98,0
        .byte 0,0,124,68,126,98,126,0
        .byte 0,0,126,66,96,98,126,0
        .byte 0,0,124,66,70,70,126,0
        .byte 0,0,126,64,120,96,126,0
        .byte 0,0,126,64,120,96,96,0
        .byte 0,0,126,64,110,98,126,0
        .byte 0,0,66,66,126,98,98,0
        .byte 0,0,126,16,24,24,126,0
        .byte 0,0,2,2,6,70,126,0
        .byte 0,0,68,68,126,98,98,0
        .byte 0,0,64,64,96,96,126,0
        .byte 0,0,126,74,66,98,98,0
        .byte 0,0,122,74,74,106,110,0
        .byte 0,0,126,66,98,98,126,0
        .byte 0,0,126,66,126,96,96,0
        .byte 0,0,126,66,82,78,126,0
        .byte 0,0,124,68,126,98,98,0
        .byte 0,0,126,96,126,2,126,0
        .byte 0,0,126,16,16,24,24,0
        .byte 0,0,66,98,98,98,126,0
        .byte 0,0,66,66,100,104,112,0
        .byte 0,0,74,74,106,106,126,0
        .byte 0,0,66,36,24,36,66,0
        .byte 0,0,98,98,126,2,126,0
        .byte 0,0,126,4,8,16,126,0
        .byte 0,30,24,24,24,24,30,0
        .byte 0,0,192,96,48,24,12,0
        .byte 0,240,48,48,48,48,240,0
        .byte 0,48,120,252,48,48,48,0
        .byte 0,0,0,0,0,0,0,255
        .byte 0,60,102,248,96,96,254,0
        .byte 0,0,120,12,124,204,124,0
        .byte 0,96,96,124,102,102,124,0
        .byte 0,124,130,186,162,186,130,124
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

jtab:
        .byte 99
keys:   .byte 66,97,104,72,73,16,55,48,49,17,18
.out .sprintf("KEYS SIZE = %d", (* - keys))

data_end:
.out .sprintf("TOTAL DATA SIZE = %d", (* - data_start))
