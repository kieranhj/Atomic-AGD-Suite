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
WINDOWHGT = 19
WINDOWWID = 30 ;面面面面面面面面面面面面
MAPWID = 11
        .byte 255,255,255,255,255,255,255,255,255,255,255
mapdat:
        .byte 255,255,255,255,255,8,9,13,255,255,255,255,255,255,255,255,7,10,12,255,255,255,255,255,2,3,4,5,6,11,15,16,255,255,0,1,255,255,14,20,19,18,17,255,255,21,22,255,255,255,255,255,255,255,255
        .byte 255,255,255,255,255,255,255,255,255,255,255
stmap:  .byte 34

evnt00:
        jsr skobj
        sta varobj
        lda #2
        cmp varobj
        bcs *+5
        jmp a00104
        lda #0
        cmp vark
        beq *+5
        jmp a00075
        lda #21
        sta chary
        lda #1
        sta charx
        lda #14
        jsr dmsg
        jmp a00100
a00075: lda #21
        sta chary
        lda #1
        sta charx
        lda #25
        jsr dmsg
a00100: jmp a00517
a00104: lda #4
        cmp varobj
        beq *+5
        jmp a00159
        lda #1
        sta varg
        lda #21
        sta chary
        lda #1
        sta charx
        lda #23
        jsr dmsg
        jmp a00517
a00159: lda #3
        cmp varobj
        beq *+5
        jmp a00245
        lda #1
        cmp varg
        beq *+5
        jmp a00215
        lda varobj
        jsr getob
        lda #0
        sta varg
        jmp a00240
a00215: lda #21
        sta chary
        lda #1
        sta charx
        lda #22
        jsr dmsg
a00240: jmp a00517
a00245: lda #6
        cmp varobj
        beq *+5
        jmp a00389
        lda #1
        cmp vark
        beq *+5
        jmp a00338
        lda #0
        cmp vard
        beq *+5
        jmp a00333
        lda #21
        sta chary
        lda #1
        sta charx
        lda #7
        jsr dmsg
        lda #1
        sta vard
        jmp a00333
a00333: jmp a00385
a00338: lda #0
        cmp vark
        beq *+5
        jmp a00385
        lda #21
        sta chary
        lda #1
        sta charx
        lda #14
        jsr dmsg
        jmp a00385
a00385: jmp a00517
a00389: lda #5
        cmp varobj
        beq *+5
        jmp a00466
        lda #0
        cmp vark
        beq *+5
        jmp a00461
        lda #1
        sta vark
        lda #21
        sta chary
        lda #1
        sta charx
        lda #15
        jsr dmsg
        jmp a00461
a00461: jmp a00517
a00466: lda #255
        cmp varobj
        bne *+5
        jmp a00517
        lda varobj
        jsr getob
        lda #5
        sta z80_c
        lda vare
        clc
        adc z80_c
        sta vare
a00517: lda #14
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp a00587
        jsr scru
        lda #128
        ldy #8
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        sta varo
        iny; ldy #9
        lda (z80_ix),y
        sta varp
a00587: lda #130
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a00656
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
a00656: lda joyval
        and #1
        beq :+
        jmp a01196
:
        lda #230
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp a00748
        jsr scrr
        lda #8
        ldy #9
        sta (z80_ix),y
        dey; ldy #8
        lda (z80_ix),y
        sta varo
        iny; ldy #9
        lda (z80_ix),y
        sta varp
        jmp a01196
a00748: jsr cangr
        beq :+
        jmp a01111
:
        lda joyval
        and #8
        beq :+
        jmp a00977
:
        jsr laddu
        beq :+
        jmp a00798
:
        jmp a00972
a00798: jsr laddd
        beq :+
        jmp a00842
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
        jmp a00972
a00842: lda #0
        cmp vari
        beq *+5
        jmp a00877
        lda #1
        ldy #6
        sta (z80_ix),y
        jmp a00890
a00877: lda #4
        ldy #6
        sta (z80_ix),y
a00890: lda #0
        cmp vara
        beq *+5
        jmp a00916
        ; lda #0
        jsr animsp
a00916: lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a00950
        lda #9
        asl a
        sta sndtyp
a00950: ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a00972: jmp a01106
a00977: lda #0
        cmp vari
        beq *+5
        jmp a01011
        lda #1
        ldy #6
        sta (z80_ix),y
        jmp a01025
a01011: lda #4
        ldy #6
        sta (z80_ix),y
a01025: lda #0
        cmp vara
        beq *+5
        jmp a01050
        ; lda #0
        jsr animsp
a01050: lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a01084
        lda #9
        asl a
        sta sndtyp
a01084: ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a01106: jmp a01196
a01111: jsr cangu
        beq :+
        jmp a01196
:
        lda #0
        cmp varb
        beq *+5
        jmp a01150
        ; lda #0
        jsr animsp
a01150: ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        iny; ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a01196: lda joyval
        and #2
        beq :+
        jmp a01736
:
        lda #8
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp a01288
        jsr scrl
        lda #230
        ldy #9
        sta (z80_ix),y
        dey; ldy #8
        lda (z80_ix),y
        sta varo
        iny; ldy #9
        lda (z80_ix),y
        sta varp
        jmp a01736
a01288: jsr cangl
        beq :+
        jmp a01652
:
        lda joyval
        and #8
        beq :+
        jmp a01517
:
        jsr laddu
        beq :+
        jmp a01337
:
        jmp a01512
a01337: jsr laddd
        beq :+
        jmp a01382
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
        jmp a01512
a01382: lda #0
        cmp vari
        beq *+5
        jmp a01417
        ; lda #0
        ldy #6
        sta (z80_ix),y
        jmp a01430
a01417: lda #3
        ldy #6
        sta (z80_ix),y
a01430: lda #0
        cmp vara
        beq *+5
        jmp a01456
        ; lda #0
        jsr animsp
a01456: lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a01490
        lda #9
        asl a
        sta sndtyp
a01490: ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a01512: jmp a01647
a01517: lda #0
        cmp vari
        beq *+5
        jmp a01552
        ; lda #0
        ldy #6
        sta (z80_ix),y
        jmp a01565
a01552: lda #3
        ldy #6
        sta (z80_ix),y
a01565: lda #0
        cmp vara
        beq *+5
        jmp a01591
        ; lda #0
        jsr animsp
a01591: lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a01625
        lda #9
        asl a
        sta sndtyp
a01625: ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a01647: jmp a01736
a01652: jsr cangu
        beq :+
        jmp a01736
:
        lda #0
        cmp varb
        beq *+5
        jmp a01691
        ; lda #0
        jsr animsp
a01691: ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        iny; ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a01736: lda joyval
        and #8
        beq :+
        jmp a01897
:
        jsr laddu
        beq :+
        jmp a01897
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #0
        cmp vari
        beq *+5
        jmp a01824
        lda #2
        ldy #6
        sta (z80_ix),y
        jmp a01837
a01824: lda #5
        ldy #6
        sta (z80_ix),y
a01837: lda #0
        cmp varb
        beq *+5
        jmp a01863
        ; lda #0
        jsr animsp
a01863: lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a01897
        lda #9
        asl a
        sta sndtyp
a01897: lda joyval
        and #4
        beq :+
        jmp a02071
:
        jsr laddd
        beq :+
        jmp a02071
:
        lda #2
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #0
        cmp vari
        beq *+5
        jmp a01998
        lda #2
        ldy #6
        sta (z80_ix),y
        jmp a02011
a01998: lda #5
        ldy #6
        sta (z80_ix),y
a02011: lda #0
        cmp varb
        beq *+5
        jmp a02037
        ; lda #0
        jsr animsp
a02037: lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a02071
        lda #9
        asl a
        sta sndtyp
a02071: lda joyval
        and #16
        beq :+
        jmp a02105
:
        jsr jump
        lda #122
        asl a
        sta sndtyp
a02105: lda joyval
        and #64
        beq :+
        jmp a02178
:
        lda #21
        sta chary
        lda #10
        sta charx
        lda #2
        jsr dmsg
        jsr prskey
        lda #21
        sta chary
        lda #1
        sta charx
        lda #18
        jsr dmsg
a02178: jsr cangd
        beq :+
        jmp a02246
:
        jsr laddu
        beq :+
        jmp a02210
:
        jmp a02246
a02210: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a02242
:
        jmp a02246
a02242: jsr ifall
a02246: ldy #8
        lda (z80_ix),y
        sta varf
        iny; ldy #9
        lda (z80_ix),y
        sta varn
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a02322
:
        lda #50
        asl a
        sta sndtyp
        lda #0
        sta vare
a02322: lda #0
        cmp vare
        beq *+5
        jmp a02367
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #1
        ; ldy #5
        sta (z80_ix),y
a02367: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a02513
:
        lda #0
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a02443
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp a02513
a02443: lda #3
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a02491
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp a02513
a02491: ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a02513: jmp grav
.out .sprintf("EVENT 0 SIZE = %d", (* - evnt00))

evnt01:
        lda #17
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp varc
        beq *+5
        jmp b00041
        ; lda #0
        jsr animsp
b00041: lda #4
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp b00080
        lda #1
        sta deadf
        lda #50
        jsr delay
b00080: rts
.out .sprintf("EVENT 1 SIZE = %d", (* - evnt01))

evnt02:
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c00144
        jsr laddu
        beq :+
        jmp c00064
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp c00144
c00064: lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp c00131
        lda #2
        ldy #10
        sta (z80_ix),y
        jmp c00144
c00131: lda #3
        ldy #10
        sta (z80_ix),y
c00144: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c00287
        jsr laddd
        beq :+
        jmp c00207
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp c00287
c00207: lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp c00273
        lda #2
        ldy #10
        sta (z80_ix),y
        jmp c00287
c00273: lda #3
        ldy #10
        sta (z80_ix),y
c00287: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c00547
        jsr cangl
        beq :+
        jmp c00363
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #10
        ldy #6
        sta (z80_ix),y
        jmp c00440
c00363: jsr cangu
        beq :+
        jmp c00426
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        iny; ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp c00440
c00426: lda #3
        ldy #10
        sta (z80_ix),y
c00440: lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp c00547
        jsr laddu
        beq :+
        jmp c00520
:
        lda #0
        ldy #10
        sta (z80_ix),y
        jmp c00547
c00520: jsr laddd
        beq :+
        jmp c00547
:
        lda #1
        ldy #10
        sta (z80_ix),y
c00547: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c00808
        jsr cangr
        beq :+
        jmp c00624
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        tya; lda #9
        ldy #6
        sta (z80_ix),y
        jmp c00701
c00624: jsr cangu
        beq :+
        jmp c00687
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        iny; ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp c00701
c00687: lda #2
        ldy #10
        sta (z80_ix),y
c00701: lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp c00808
        jsr laddu
        beq :+
        jmp c00781
:
        lda #0
        ldy #10
        sta (z80_ix),y
        jmp c00808
c00781: jsr laddd
        beq :+
        jmp c00808
:
        lda #1
        ldy #10
        sta (z80_ix),y
c00808: lda #16
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp c00843
        lda #1
        ldy #10
        sta (z80_ix),y
c00843: lda #130
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp c00879
        lda #0
        ldy #10
        sta (z80_ix),y
c00879: lda #228
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp c00917
        lda #2
        iny; ldy #10
        sta (z80_ix),y
c00917: lda #10
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp c00954
        lda #3
        iny; ldy #10
        sta (z80_ix),y
c00954: jsr cangd
        beq :+
        jmp c00972
:
        jsr ifall
c00972: lda #0
        cmp varb
        beq *+5
        jmp c00997
        ; lda #0
        jsr animsp
c00997: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c01358
:
        lda #0
        cmp vart
        bcs *+5
        jmp c01314
        lda #90
        asl a
        sta sndtyp
        lda #21
        sta chary
        lda #1
        sta charx
        lda #19
        jsr dmsg
        lda #50
        jsr delay
        jsr cls
        lda varj
        clc
        adc #1
        sta varj
        lda #0
        sta vard
        ; lda #0
        sta varg
        ; lda #0
        sta vari
        ; lda #0
        sta vark
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #3
        cmp varj
        beq *+5
        jmp c01209
        lda #4
        sta varm
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #22
        sta scno
        jsr nwscr
        jmp c01291
c01209: lda #3
        sta varm
        lda #8
        sta scno
        jsr nwscr
        lda #120
        ldy #8
        sta (z80_ix),y
        lda #32
        iny; ldy #9
        sta (z80_ix),y
        lda #5
        sta vart
        ; lda #5
        sta z80_c
        lda vare
        clc
        adc z80_c
        sta vare
c01291: lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        jmp c01358
c01314: lda #0
        cmp vara
        beq *+5
        jmp c01358
        lda #5
        asl a
        sta sndtyp
        lda vart
        sec
        sbc #1
        sta vart
c01358: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp c01407
:
        lda #2
        jsr explod
        lda #255
        ldy #5
        sta (z80_ix),y
c01407: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp c01448
:
        lda #3
        ldy #10
        sta (z80_ix),y
c01448: jmp grav
.out .sprintf("EVENT 2 SIZE = %d", (* - evnt02))

evnt03:
        ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
        lda #1
        ; ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp d00307
        lda #0
        ; ldy #11
        sta (z80_ix),y
        lda varn
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp d00128
        jsr cangl
        beq :+
        jmp d00123
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
d00123: jmp d00186
d00128: lda varn
        ldy #9
        cmp (z80_ix),y
        bne *+5
        jmp d00186
        jsr cangr
        beq :+
        jmp d00186
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
d00186: lda varf
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp d00249
        jsr cangu
        beq :+
        jmp d00244
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
d00244: jmp d00307
d00249: lda varf
        ldy #8
        cmp (z80_ix),y
        bne *+5
        jmp d00307
        jsr cangd
        beq :+
        jmp d00307
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
d00307: lda #0
        cmp vara
        beq *+5
        jmp d00332
        ; lda #0
        jsr animsp
d00332: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00407
:
        lda #0
        cmp varb
        beq *+5
        jmp d00407
        lda #50
        asl a
        sta sndtyp
        lda #1
        jsr delay
        lda vare
        sec
        sbc #1
        sta vare
d00407: rts
.out .sprintf("EVENT 3 SIZE = %d", (* - evnt03))

evnt04:
        lda #12
        cmp scno
        beq *+5
        jmp e00212
        jsr cangd
        beq :+
        jmp e00081
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
        jmp e00208
e00081: jsr cangl
        beq :+
        jmp e00144
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
        jmp e00208
e00144: lda #255
        ldy #5
        sta (z80_ix),y
        lda #16
        ldy #8
        sta (z80_ix),y
        lda #192
        iny; ldy #9
        sta (z80_ix),y
        lda #4
        sta z80_c
        lda #16
        sta z80_b
        jsr spawn
e00208: jmp e00401
e00212: jsr cangd
        beq :+
        jmp e00275
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
        jmp e00401
e00275: jsr cangr
        beq :+
        jmp e00338
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
        jmp e00401
e00338: lda #255
        ldy #5
        sta (z80_ix),y
        lda #16
        ldy #8
        sta (z80_ix),y
        lda #24
        iny; ldy #9
        sta (z80_ix),y
        lda #4
        sta z80_c
        lda #16
        sta z80_b
        jsr spawn
e00401: lda #0
        cmp vara
        beq *+5
        jmp e00427
        ; lda #0
        jsr animsp
e00427: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00469
:
        lda #50
        asl a
        sta sndtyp
        lda #0
        sta vare
e00469: rts
.out .sprintf("EVENT 4 SIZE = %d", (* - evnt04))

evnt05:
        lda #40
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp f00138
        jsr cangd
        beq :+
        jmp f00095
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp f00133
f00095: lda #1
        ldy #11
        sta (z80_ix),y
        lda #10
        asl a
        sta sndtyp
        lda #10
        asl a
        sta sndtyp
f00133: jmp f00210
f00138: jsr cangu
        beq :+
        jmp f00178
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp f00210
f00178: lda varrnd
        cmp varh
        beq *+5
        jmp f00210
        lda #0
        ldy #11
        sta (z80_ix),y
f00210: lda varrnd
        cmp varh
        beq *+5
        jmp f00241
        lda #0
        ldy #11
        sta (z80_ix),y
f00241: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00357
:
        lda #50
        asl a
        sta sndtyp
        lda vare
        sec
        sbc #1
        sta vare
        lda #1
        jsr delay
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
f00357: rts
.out .sprintf("EVENT 5 SIZE = %d", (* - evnt05))

evnt06:
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00119
:
        lda #0
        cmp vari
        beq *+5
        jmp g00119
        lda #21
        sta chary
        lda #1
        sta charx
        lda #13
        jsr dmsg
        lda #1
        sta vari
        lda #5
        sta z80_c
        lda vare
        clc
        adc z80_c
        sta vare
        lda #50
        jsr delay
        lda #2
        sta varm
        jmp g00119
g00119: rts
.out .sprintf("EVENT 6 SIZE = %d", (* - evnt06))

evnt07:
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp h00192
:
        lda #1
        cmp vard
        beq *+5
        jmp h00167
        ; lda #1
        cmp vari
        beq *+5
        jmp h00138
        lda #90
        asl a
        sta sndtyp
        lda skptr
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
        lda #21
        sta scno
        jsr nwscr
        jmp h00163
h00138: lda #21
        sta chary
        lda #1
        sta charx
        lda #3
        jsr dmsg
h00163: jmp h00192
h00167: lda #21
        sta chary
        lda #1
        sta charx
        lda #3
        jsr dmsg
h00192: lda #21
        cmp scno
        beq *+5
        jmp h00263
        jsr cangl
        beq :+
        jmp h00249
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp h00263
h00249: lda #255
        ldy #5
        sta (z80_ix),y
h00263: rts
.out .sprintf("EVENT 7 SIZE = %d", (* - evnt07))

evnt08:
        lda #0
        jsr animsp
        rts
.out .sprintf("EVENT 8 SIZE = %d", (* - evnt08))

evnt09:
        lda #0
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00199
        lda #8
        cmp scno
        beq *+5
        jmp j00122
        lda #112
        ldy #8
        sta (z80_ix),y
        lda #140
        iny; ldy #9
        sta (z80_ix),y
        lda #0
        cmp vari
        beq *+5
        jmp j00104
        lda #1
        ldy #6
        sta (z80_ix),y
        jmp j00117
j00104: lda #3
        ldy #6
        sta (z80_ix),y
j00117: jmp j00199
j00122: lda varo
        ldy #8
        sta (z80_ix),y
        lda varp
        iny; ldy #9
        sta (z80_ix),y
        lda #0
        cmp vari
        beq *+5
        jmp j00185
        lda #1
        ldy #6
        sta (z80_ix),y
        jmp j00199
j00185: lda #3
        ldy #6
        sta (z80_ix),y
j00199: rts
.out .sprintf("EVENT 9 SIZE = %d", (* - evnt09))

evnt10:
        lda #22
        sta chary
        lda #29
        sta charx
        lda #9
        cmp vart
        bcs *+5
        jmp k00062
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
k00062: lda vart
        jsr disply
        lda #22
        sta chary
        lda #1
        sta charx
        lda #26
        jsr dmsg
        lda #21
        cmp scno
        beq *+5
        jmp k00164
        lda #19
        sta chary
        lda #3
        sta charx
        lda #21
        jsr dmsg
        lda joyval
        and #16
        beq :+
        jmp k00164
:
        lda #1
        sta gamwon
k00164: lda #22
        cmp scno
        beq *+5
        jmp k00282
        lda #5
        sta chary
        lda #1
        sta charx
        lda #12
        jsr dmsg
        lda #7
        sta chary
        lda #4
        sta charx
        lda #20
        jsr dmsg
        lda #18
        sta chary
        lda #3
        sta charx
        lda #21
        jsr dmsg
        lda joyval
        and #16
        beq :+
        jmp k00282
:
        lda #1
        sta gamwon
k00282: rts
.out .sprintf("EVENT 10 SIZE = %d", (* - evnt10))

evnt11:
        lda #2
        cmp vara
        beq *+5
        jmp l00031
        lda #0
        sta vara
        jmp l00046
l00031: lda vara
        clc
        adc #1
        sta vara
l00046: lda #2
        cmp varb
        beq *+5
        jmp l00076
        lda #0
        sta varb
        jmp l00091
l00076: lda varb
        clc
        adc #1
        sta varb
l00091: lda #15
        cmp varc
        beq *+5
        jmp l00121
        lda #0
        sta varc
        jmp l00136
l00121: lda varc
        clc
        adc #1
        sta varc
l00136: lda #40
        cmp varh
        beq *+5
        jmp l00166
        lda #0
        sta varh
        jmp l00181
l00166: lda varh
        clc
        adc #1
        sta varh
l00181: lda #23
        sta chary
        lda #28
        sta charx
        lda #99
        cmp vare
        bcs *+5
        jmp l00241
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
l00241: lda #9
        cmp vare
        bcs *+5
        jmp l00284
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
l00284: lda vare
        jsr disply
        lda #100
        cmp vare
        bcc *+5
        jmp l00320
        ; lda #100
        sta vare
l00320: lda #22
        sta chary
        lda #7
        sta charx
        lda #1
        cmp varj
        beq *+5
        jmp l00380
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #25
        jsr pattr
l00380: lda #2
        cmp varj
        beq *+5
        jmp l00449
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #25
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #25
        jsr pattr
l00449: lda #3
        cmp varj
        beq *+5
        jmp l00515
        ; lda #3
        sta loopa
l00475: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #25
        jsr pattr
        dec loopa
        beq :+
        jmp l00475
:
l00515: lda #1
        cmp varg
        beq *+5
        jmp l00580
        lda #23
        sta chary
        lda #18
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #57
        jsr pattr
        jmp l00623
l00580: lda #23
        sta chary
        lda #18
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
l00623: lda #1
        cmp vark
        beq *+5
        jmp l00688
        lda #23
        sta chary
        lda #16
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #56
        jsr pattr
        jmp l00731
l00688: lda #23
        sta chary
        lda #16
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
l00731: lda #1
        cmp vard
        beq *+5
        jmp l00796
        lda #23
        sta chary
        lda #14
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #18
        jsr pattr
        jmp l00840
l00796: lda #20
        sta chary
        lda #14
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
l00840: lda #1
        cmp vari
        beq *+5
        jmp l00904
        lda #23
        sta chary
        lda #12
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #8
        jsr pattr
        jmp l00948
l00904: lda #23
        sta chary
        lda #12
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
l00948: lda #20
        cmp scno
        beq *+5
        jmp l01390
        lda #3
        jsr gotob
        cmp #255
        beq :+
        jmp l01390
:
        lda #9
        sta chary
        lda #7
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #39
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #40
        jsr pattr
        lda #10
        sta chary
        lda #7
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #42
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #41
        jsr pattr
        lda #12
        sta chary
        lda #9
        sta charx
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
        lda #12
        sta chary
        lda #11
        sta charx
        lda #10
        sta loopa
l01220: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #63
        jsr pattr
        dec loopa
        beq :+
        jmp l01220
:
        lda #15
        sta chary
        lda #4
        sta charx
        lda #2
        sta loopa
l01285: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #58
        jsr pattr
        dec loopa
        beq :+
        jmp l01285
:
        lda #15
        sta chary
        lda #26
        sta charx
        lda #2
        sta loopa
l01350: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #58
        jsr pattr
        dec loopa
        beq :+
        jmp l01350
:
l01390: rts
.out .sprintf("EVENT 11 SIZE = %d", (* - evnt11))

evnt12:
        jsr cls
        lda #1
        sta varm
        ; lda #1
        sta chary
        ; lda #1
        sta charx
        lda #8
        jsr dmsg
        lda #3
        sta chary
        lda #5
        sta charx
        lda #16
        jsr dmsg
        lda #5
        sta chary
        lda #0
        sta charx
        lda #6
        jsr dmsg
        lda #14
        sta chary
        lda #0
        sta charx
        lda #17
        jsr dmsg
        lda #23
        sta chary
        lda #3
        sta charx
        lda #10
        jsr dmsg
        jsr prskey
        jsr cls
        lda #3
        sta chary
        lda #5
        sta charx
        lda #24
        jsr dmsg
        lda #13
        sta chary
        lda #5
        sta charx
        lda #9
        jsr dmsg
        jsr prskey
        rts
.out .sprintf("EVENT 12 SIZE = %d", (* - evnt12))

evnt13:
        jsr cls
        lda #120
        sta varo
        lda #112
        sta varp
        lda #0
        sta vara
        ; lda #0
        sta varb
        ; lda #0
        sta vard
        lda #100
        sta vare
        lda #0
        sta varg
        ; lda #0
        sta varh
        lda #1
        sta vari
        lda #0
        sta varj
        ; lda #0
        sta vark
        lda #3
        sta varl
        lda #5
        sta vart
        lda #2
        sta varm
        jsr cls
        rts
.out .sprintf("EVENT 13 SIZE = %d", (* - evnt13))

evnt14:
        lda #23
        sta chary
        lda #21
        sta charx
        lda #4
        jsr dmsg
        lda #21
        sta chary
        lda #1
        sta charx
        lda #18
        jsr dmsg
        lda #23
        sta chary
        lda #1
        sta charx
        ; lda #1
        jsr dmsg
        lda #22
        sta chary
        lda #11
        sta charx
        lda #27
        jsr dmsg
        lda #22
        sta chary
        lda #21
        sta charx
        lda #11
        jsr dmsg
        lda #23
        sta chary
        lda #7
        sta charx
        lda #3
        sta loopa
o00152: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #23
        jsr pattr
        dec loopa
        beq :+
        jmp o00152
:
        lda #2
        cmp varl
        bcs *+5
        jmp o00252
        lda #23
        sta chary
        lda #9
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #24
        jsr pattr
o00252: lda #1
        cmp varl
        bcs *+5
        jmp o00312
        lda #23
        sta chary
        lda #8
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #24
        jsr pattr
o00312: lda #0
        cmp varl
        bcs *+5
        jmp o00372
        lda #23
        sta chary
        lda #7
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #24
        jsr pattr
o00372: rts
.out .sprintf("EVENT 14 SIZE = %d", (* - evnt14))

evnt15:
        rts
.out .sprintf("EVENT 15 SIZE = %d", (* - evnt15))

evnt16:
        lda varl
        sec
        sbc #1
        sta varl
        lda #100
        sta vare
        lda #0
        cmp varl
        beq *+5
        jmp q00063
        lda #50
        jsr delay
        jsr cls
        lda #1
        sta gamwon
q00063: rts
.out .sprintf("EVENT 16 SIZE = %d", (* - evnt16))

evnt17:
        rts
.out .sprintf("EVENT 17 SIZE = %d", (* - evnt17))

evnt18:
        jsr cls
        lda #3
        cmp varj
        beq *+5
        jmp s00749
        lda #9
        sta chary
        lda #11
        sta charx
        lda #0
        jsr dmsg
        lda #12
        sta chary
        lda #13
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #83
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #84
        jsr pattr
        lda #13
        sta chary
        ; lda #13
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #68
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #69
        jsr pattr
        lda #13
        sta chary
        lda #17
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #68
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #69
        jsr pattr
        lda #14
        sta chary
        lda #13
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #70
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #71
        jsr pattr
        lda #14
        sta chary
        lda #17
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #70
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #71
        jsr pattr
        lda #12
        sta chary
        lda #17
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #99
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #100
        jsr pattr
        lda #15
        sta chary
        lda #10
        sta charx
        lda #12
        sta loopa
s00493: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        dec loopa
        beq :+
        jmp s00493
:
        lda #13
        sta chary
        lda #15
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #101
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #102
        jsr pattr
        lda #14
        sta chary
        lda #15
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #103
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #104
        jsr pattr
        lda #12
        sta chary
        lda #15
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #105
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #106
        jsr pattr
        jmp s01857
s00749: lda #0
        cmp varl
        beq *+5
        jmp s01244
        lda #5
        sta varm
        lda #6
        sta chary
        lda #11
        sta charx
        lda #0
        jsr dmsg
        lda #14
        sta chary
        lda #16
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #83
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #84
        jsr pattr
        lda #11
        sta chary
        lda #14
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #93
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #94
        jsr pattr
        lda #12
        sta chary
        lda #14
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #87
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #88
        jsr pattr
        lda #13
        sta chary
        lda #14
        sta charx
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
        lda #90
        jsr pattr
        lda #14
        sta chary
        ; lda #14
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #91
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #92
        jsr pattr
        lda #15
        sta chary
        lda #10
        sta charx
        lda #12
        sta loopa
s01175: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        dec loopa
        beq :+
        jmp s01175
:
        lda #9
        sta chary
        lda #7
        sta charx
        lda #20
        jsr dmsg
        jmp s01857
s01244: lda #1
        sta varm
        lda #4
        sta chary
        lda #8
        sta charx
        lda #5
        jsr dmsg
        lda #10
        sta chary
        lda #14
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #83
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #84
        jsr pattr
        lda #11
        sta chary
        lda #14
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #85
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #86
        jsr pattr
        lda #12
        sta chary
        lda #14
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #87
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #88
        jsr pattr
        lda #13
        sta chary
        lda #14
        sta charx
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
        lda #90
        jsr pattr
        lda #14
        sta chary
        ; lda #14
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #91
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #92
        jsr pattr
        lda #13
        sta chary
        lda #16
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #95
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #96
        jsr pattr
        lda #14
        sta chary
        lda #16
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #97
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #98
        jsr pattr
        lda #15
        sta chary
        lda #10
        sta charx
        lda #12
        sta loopa
s01793: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        dec loopa
        beq :+
        jmp s01793
:
        lda #9
        sta chary
        lda #3
        sta charx
        lda #28
        jsr dmsg
s01857: lda #18
        sta chary
        lda #3
        sta charx
        lda #10
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
        .byte "GAME OVER!",141
        .byte "LIVES:",141
        .byte "GAME PAUSED",141
        .byte "HUM... SOMETHING",39
        .byte "S MISSING...",141
        .byte "ENERGY:",141
        .byte "CONGRATULATIONS!",13
        .byte "    YOU",39
        .byte "VE FINISHED THE GAME",13
        .byte "          WITH SUCCESS!",141
        .byte " Jane Jelly is on Badass Guys",13
        .byte " Island to find the EGG DIAMOND",13
        .byte " and then leave the island with",13
        .byte " her clothes on.",13
        .byte 13
        .byte " More information in",13
        .byte " jg-spectrum.webnode.pt",13
        .byte 141
        .byte "YOU",39
        .byte "VE FOUND THE EGG DIAMOND!",141
        .byte "THE ADVENTURES OF JANE JELLY ",13
        .byte "        The Egg Diamond ",141
        .byte "The KEYBOARD KEYS are:",13
        .byte 13
        .byte "      Q for UP",13
        .byte "      A for DOWN",13
        .byte "      O for LEFT",13
        .byte "      P for RIGHT",13
        .byte "      SPACE for JUMP",13
        .byte "      H to PAUSE",13
        .byte 13
        .byte 141
        .byte "PRESS ANY KEY TO CONTINUE!",141
        .byte "CAUGHT:",13
        .byte 13
        .byte 13
        .byte 13
        .byte 141
        .byte "AH! AH! AH! YOU",39
        .byte "RE MINE, B*TCH!",13
        .byte 141
        .byte "YOU HAVE FOUND YOUR CLOTHES!",141
        .byte "LOCKED! YOU NEED THE KEY!",141
        .byte "YOU HAVE FOUND THE CHEST KEY!",141
        .byte "By Jaime Grilo (2018)",141
        .byte "CREDITS:",13
        .byte "Game made by Jaime Grilo using:",13
        .byte "AGD by Jonathan Cauldwell",13
        .byte "AGDMusicizer II by David Saphier",13
        .byte "Loading screen by Andy Green and",13
        .byte "                  Jamie Ball",13
        .byte "Music By Yerzmyey",13
        .byte "Font by Paul Van Der Laan",13
        .byte 13
        .byte 13
        .byte 141
        .byte "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@",141
        .byte "GOT YA, B*TCH!",141
        .byte "OH NOOO... F*CK!!!",141
        .byte "PRESS JUMP KEY TO CONTINUE!",141
        .byte "YOU NEED THE WHEEL TO ACTIVATE",141
        .byte "YOU",39
        .byte "VE FOUND THE LEVER WHEEL",141
        .byte "PRESS A NUMBER KEY FOR:",13
        .byte 13
        .byte "      1. KEYBOARD",13
        .byte 13
        .byte "      2. KEMPSTON JOYSTICK",13
        .byte 13
        .byte "      3. SINCLAIR",141
        .byte "IT",39
        .byte "S EMPTY!...",141
        .byte "JAIL:",141
        .byte "INVENTORY",141
        .byte "I",39
        .byte "VE GOT THE EGG DIAMOND!!",141
nummsg:
        .byte 29
.out .sprintf("MESSAGE SIZE = %d", (* - msgdat))

chgfx:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 117,213,170,234,117,85,234,250,117,213,170,234,245,117,186,170
        .byte 0,0,0,0,221,187,34,68,221,187,0,0,0,0,0,0
        .byte 0,37,0,24,1,164,2,82,33,161,64,2,0,5,0,66
        .byte 164,0,24,0,37,128,74,64,133,132,64,2,160,0,66,0
        .byte 36,133,90,66,5,5,10,10,160,160,24,24,5,5,2,18
        .byte 161,36,66,90,160,160,80,80,5,5,24,24,160,160,72,64
        .byte 25,25,4,34,19,76,0,0,0,0,0,0,0,0,0,0
        .byte 0,32,216,186,117,0,114,16,0,49,114,0,85,32,80,114
        .byte 192,96,224,160,112,208,96,112,176,160,112,240,192,176,48,128
        .byte 0,48,0,224,16,160,32,208,80,80,96,240,144,96,96,240
        .byte 192,0,112,0,80,128,176,64,160,160,240,96,96,144,240,96
        .byte 95,95,175,15,79,95,175,15,79,95,175,15,79,95,175,15
        .byte 95,85,175,174,95,27,175,140,95,70,175,171,95,85,0,0
        .byte 95,95,175,175,95,95,39,171,87,27,175,175,78,95,175,137
        .byte 87,78,175,174,95,78,175,175,95,95,175,174,93,10,39,174
        .byte 87,78,39,175,87,95,175,175,95,95,39,175,85,27,39,174
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,255,255,0,0
        .byte 17,196,34,34,85,85,64,152,68,145,34,170,49,100,16,200
        .byte 0,0,80,8,133,1,80,66,36,128,88,8,164,5,88,10
        .byte 0,0,102,0,153,102,102,153,0,102,0,0,0,0,0,0
        .byte 2,0,165,165,80,80,0,0,2,0,37,0,2,0,37,0
        .byte 0,64,165,165,10,10,0,0,0,64,0,164,0,64,0,164
        .byte 0,0,0,0,32,132,18,88,4,33,90,74,128,0,0,0
        .byte 0,0,0,0,32,192,48,208,64,48,240,224,128,0,0,0
        .byte 0,0,34,4,87,78,34,4,2,68,39,174,2,68,0,0
        .byte 255,255,0,0,255,255,255,255,255,255,0,0,255,255,255,255
        .byte 15,15,143,15,15,31,15,143,79,15,47,47,95,79,143,15
        .byte 160,129,10,24,160,160,72,74,36,160,80,82,36,164,18,72
        .byte 4,160,26,80,133,5,74,10,5,4,64,2,37,4,10,10
        .byte 15,15,47,47,95,79,15,15,15,79,143,15,15,15,175,15
        .byte 17,17,174,174,17,17,1,1,17,17,1,1,17,17,1,1
        .byte 119,255,136,0,187,255,187,51,187,255,136,0,187,255,187,51
        .byte 255,238,0,17,255,221,204,221,255,221,0,17,255,221,204,221
        .byte 0,136,0,136,0,136,17,204,17,68,51,170,51,68,102,170
        .byte 17,0,0,136,17,8,34,136,2,8,35,0,2,0,1,0
        .byte 17,0,0,0,17,8,34,136,2,8,35,0,0,0,1,0
        .byte 15,31,15,31,15,239,31,223,63,239,127,175,255,95,255,175
        .byte 143,15,207,15,255,15,223,143,255,207,207,239,255,111,239,175
        .byte 34,0,34,0,34,17,34,102,0,136,17,102,17,119,34,51
        .byte 0,68,0,68,136,68,102,68,17,0,102,136,238,136,204,68
        .byte 204,68,238,136,102,136,17,0,102,68,136,68,0,68,0,68
        .byte 34,51,17,119,17,102,0,136,34,102,34,17,34,0,34,0
        .byte 34,0,34,0,34,255,34,0,34,119,34,0,34,255,34,0
        .byte 0,68,0,68,255,68,0,68,238,68,0,68,255,68,0,68
        .byte 0,68,0,68,0,68,0,68,0,68,0,68,0,68,0,68
        .byte 0,0,5,141,70,27,39,39,0,8,39,39,70,27,5,141
        .byte 34,0,34,0,34,0,34,0,34,0,34,0,34,0,34,0
        .byte 0,17,0,114,0,213,16,34,49,85,80,186,213,213,186,170
        .byte 128,0,234,0,117,0,170,136,117,196,234,98,213,85,186,186
        .byte 15,15,15,47,79,95,47,143,15,31,143,15,31,15,15,15
        .byte 119,187,51,238,51,102,51,170,17,68,17,204,0,136,0,136
        .byte 85,213,234,34,196,177,152,34,68,117,186,250,213,85,170,234
        .byte 17,17,174,174,19,19,1,1,17,17,1,1,17,17,1,1
        .byte 32,32,152,152,49,68,98,80,177,128,64,0,32,160,80,16
        .byte 48,192,64,32,144,144,160,80,160,80,144,144,64,32,48,192
        .byte 0,228,0,170,0,228,16,0,32,0,64,0,160,0,64,0
        .byte 49,196,98,98,228,117,152,152,145,145,234,114,100,100,50,200
        .byte 145,0,80,16,17,32,34,34,32,68,64,136,145,32,152,16
        .byte 0,0,16,10,128,161,66,10,1,36,16,26,160,37,80,26
        .byte 228,117,136,16,0,0,34,64,49,196,16,136,128,17,64,50
        .byte 145,0,80,16,17,32,34,34,32,68,64,136,145,32,152,16
        .byte 221,238,255,187,119,238,255,221,238,187,255,238,153,85,238,187
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 32,32,152,152,32,68,98,80,177,128,64,34,32,160,80,16
        .byte 244,244,248,250,240,244,240,242,240,245,240,250,245,244,242,248
        .byte 72,66,165,164,64,64,33,128,2,8,33,128,2,8,1,0
        .byte 160,129,80,10,128,165,66,10,133,36,18,26,160,37,80,10
        .byte 90,90,165,161,0,0,133,129,88,72,128,164,72,82,133,33
        .byte 240,240,208,240,0,0,144,176,48,176,112,16,32,48,128,176
        .byte 88,16,132,4,8,74,133,132,88,24,0,0,90,26,165,165
        .byte 192,176,224,48,112,16,48,176,144,176,0,0,208,240,240,240
        .byte 119,255,136,0,187,255,204,0,153,17,170,170,170,170,153,17
        .byte 255,238,0,17,255,221,0,51,17,17,170,153,170,153,17,17
        .byte 153,17,153,153,153,17,153,153,204,0,187,255,136,0,119,255
        .byte 17,17,153,153,17,17,153,153,0,51,255,221,0,17,255,238
        .byte 187,255,136,0,187,255,187,51,187,255,136,0,255,255,119,255
        .byte 255,221,0,17,255,221,204,221,255,221,0,17,255,255,255,238
        .byte 196,0,200,0,196,0,64,0,68,0,64,0,68,0,200,0
        .byte 0,49,0,50,0,49,0,34,0,32,0,34,0,32,0,50
        .byte 255,255,119,238,0,0,34,68,119,238,68,34,0,0,136,17
        .byte 0,0,175,175,95,95,38,34,17,95,0,170,0,68,0,4
        .byte 0,0,175,175,95,95,4,38,95,8,5,0,2,0,34,0
        .byte 0,0,0,0,0,0,0,112,0,112,128,80,80,240,48,240
        .byte 0,0,0,0,0,0,192,0,224,0,80,0,240,224,240,144
        .byte 17,228,34,200,49,128,114,136,32,196,16,98,32,228,114,170
        .byte 17,0,34,136,0,128,16,136,49,128,16,136,117,0,80,136
        .byte 17,25,1,34,0,70,0,38,0,70,0,34,0,2,0,1
        .byte 27,0,136,140,76,2,174,35,95,95,171,174,10,76,35,0
        .byte 0,4,16,72,36,36,90,16,165,132,90,90,165,164,82,88
        .byte 4,0,8,0,36,0,16,0,0,0,8,8,165,132,88,74
        .byte 1,165,74,18,128,0,64,0,129,132,10,18,4,0,0,0
        .byte 164,165,88,90,33,164,88,72,165,128,74,0,0,0,0,0
        .byte 0,117,0,216,17,100,50,200,32,128,16,64,32,228,114,136
        .byte 245,0,186,136,85,128,0,136,17,128,16,136,49,0,80,136
        .byte 0,0,0,0,0,87,0,171,17,85,35,170,19,93,1,174
        .byte 0,0,0,0,78,0,141,0,10,8,5,140,27,76,39,136
        .byte 0,93,0,170,0,87,0,5,0,2,0,137,17,76,1,39
        .byte 27,0,5,0,78,0,170,0,68,0,137,0,19,8,174,136
        .byte 0,164,0,10,0,160,0,10,1,32,74,64,0,160,90,66
        .byte 164,0,10,0,160,0,10,0,129,0,64,74,160,1,88,74
        .byte 105,210,150,240,90,165,150,75,105,180,150,105,105,150,150,105
        .byte 225,180,90,120,165,240,210,120,165,180,210,120,225,180,210,120
        .byte 105,150,150,105,105,150,150,105,75,180,180,75,75,180,180,75
        .byte 225,180,210,120,225,180,210,120,165,240,210,120,165,240,210,120
        .byte 0,0,0,0,0,70,0,137,0,87,35,136,68,87,35,171
        .byte 0,0,0,0,68,0,170,0,27,8,39,38,76,25,39,174
        .byte 0,0,0,34,0,0,0,0,34,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,136,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,49,0,64
        .byte 0,0,0,0,0,0,0,0,49,128,200,98,49,145,200,80
        .byte 0,85,0,170,17,100,0,170,0,160,0,80,0,85,0,216
        .byte 0,49,0,98,0,32,0,0,0,0,0,0,0,0,0,0
        .byte 51,170,102,153,255,221,255,153,119,34,51,153,34,34,119,119
bprop:
        .byte 0
        .byte 2
        .byte 0
        .byte 1
        .byte 1
        .byte 1
        .byte 1
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
        .byte 3
        .byte 3
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 7
        .byte 4
        .byte 0
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 5
        .byte 0
        .byte 3
        .byte 5
        .byte 5
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 0
        .byte 2
        .byte 2
        .byte 7
        .byte 5
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 3
        .byte 3
        .byte 5
        .byte 6
        .byte 6
        .byte 5
        .byte 0
        .byte 0
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
.out .sprintf("BLOCKS SIZE = %d", (* - chgfx))

sprgfx:
        .byte 0,0,0,224,1,240,0,120,1,216,0,216,0,108,0,30,1,164,7,80,15,80,7,80,1,80,0,92,1,158,1,62
        .byte 192,7,128,3,0,3,0,3,0,3,0,3,0,7,0,15,0,0,128,3,192,7,224,1,96,7,96,3,176,1,120,0
        .byte 1,232,3,244,1,230,0,7,129,204,1,159,128,63,128,15,0,111,0,110,0,236,0,206,0,199,128,195,129,193,131,128
        .byte 0,0,0,224,1,240,0,120,1,220,0,222,0,102,0,16,1,160,7,80,15,80,7,80,1,80,0,92,1,158,1,62
        .byte 192,7,128,3,0,3,0,3,0,3,0,3,0,7,0,15,0,0,128,3,192,7,224,1,112,7,120,3,152,1,64,0
        .byte 1,232,3,220,1,230,0,179,129,248,1,159,128,47,128,31,0,123,0,102,0,236,0,206,0,199,128,195,129,193,131,128
        .byte 0,0,7,0,15,128,30,0,27,128,27,0,54,0,120,0,37,128,10,224,10,240,10,224,10,128,58,0,121,128,124,128
        .byte 3,224,1,192,0,192,0,192,0,192,0,192,0,224,0,240,0,0,0,28,0,62,0,120,0,110,0,108,0,216,1,224
        .byte 1,120,2,252,6,120,14,0,19,56,15,152,31,192,31,0,15,96,7,96,3,112,7,48,14,48,28,48,24,56,16,28
        .byte 0,0,7,0,15,128,30,0,59,128,123,0,102,0,8,0,5,128,10,224,10,240,10,224,10,128,58,0,121,128,124,128
        .byte 3,224,1,192,0,192,0,192,0,192,0,192,0,224,0,240,0,0,0,28,0,62,0,120,0,238,1,236,1,152,0,32
        .byte 1,120,3,188,6,120,12,208,17,248,15,152,31,64,31,128,13,224,6,96,3,112,7,48,14,48,28,48,24,56,16,28
        .byte 0,0,129,192,131,224,194,33,194,161,193,67,102,67,120,255,61,254,7,240,7,240,3,224,3,224,3,240,7,248,7,252
        .byte 192,118,192,56,192,48,240,48,224,48,0,48,0,112,0,208,0,0,2,7,130,15,135,8,135,10,15,5,13,153,253,227
        .byte 143,175,1,252,1,252,0,248,0,248,0,248,1,252,1,252,1,220,0,216,0,216,0,216,0,216,0,216,1,220,3,222
        .byte 0,0,3,129,7,193,132,67,133,67,194,131,194,102,255,30,127,188,15,224,15,224,7,192,7,192,15,192,31,224,63,224
        .byte 3,110,3,28,3,12,15,12,7,12,0,12,0,14,0,11,0,0,4,14,4,31,14,17,14,21,15,10,155,9,123,252
        .byte 143,175,1,252,1,252,0,248,0,248,0,248,1,252,1,252,1,220,0,216,0,216,0,216,0,216,0,216,1,220,3,222
        .byte 0,240,0,146,3,252,4,112,1,216,0,216,0,108,0,30,1,164,7,80,15,80,7,80,1,80,0,88,1,156,1,60
        .byte 128,15,0,7,0,7,0,2,0,3,0,7,0,15,0,13,192,3,72,2,240,15,192,17,96,7,96,3,176,1,120,0
        .byte 1,232,3,244,1,230,0,39,129,205,129,158,0,63,0,79,0,238,1,236,0,236,0,138,128,199,0,195,129,193,131,64
        .byte 0,240,0,146,3,252,4,112,1,216,0,222,0,110,0,16,1,160,7,80,15,80,7,80,1,80,0,88,1,156,1,60
        .byte 128,15,0,7,0,7,0,2,0,3,0,7,0,13,0,13,192,3,72,2,240,15,192,17,96,7,120,3,184,1,64,0
        .byte 1,232,3,220,1,230,0,179,129,248,129,158,0,47,0,95,0,250,1,228,0,236,0,138,128,199,0,195,129,193,131,64
        .byte 15,0,73,0,63,192,14,32,27,128,27,0,54,0,120,0,37,128,10,224,10,240,10,224,10,128,26,0,57,128,60,128
        .byte 1,240,0,224,0,224,0,64,0,192,0,224,0,240,0,176,0,60,1,36,0,255,128,56,0,110,0,108,0,216,1,224
        .byte 1,120,2,252,6,120,14,64,27,56,23,152,15,192,15,32,7,112,3,120,3,112,5,16,30,48,12,48,24,56,16,44
        .byte 15,0,73,0,63,192,14,32,27,128,123,0,118,0,8,0,5,128,10,224,10,240,10,224,10,128,26,0,57,128,60,128
        .byte 1,240,0,224,0,224,0,64,0,192,0,224,0,176,0,176,0,60,1,36,0,255,128,56,0,110,1,236,1,216,0,32
        .byte 1,120,3,188,6,120,12,208,17,248,23,152,15,64,15,160,5,240,2,120,3,112,5,16,30,48,12,48,24,56,16,44
        .byte 3,224,146,36,143,248,194,33,194,161,193,67,102,67,120,255,61,254,7,240,7,240,3,224,2,160,3,240,7,248,7,252
        .byte 192,54,64,56,192,48,240,32,224,48,64,48,0,112,0,208,128,15,146,72,226,63,135,8,135,10,15,5,13,153,253,227
        .byte 143,175,1,252,1,252,0,248,0,248,0,168,1,252,1,252,0,248,0,216,0,216,0,136,0,216,0,216,1,220,3,86
        .byte 7,192,36,73,31,241,132,67,133,67,194,131,194,102,251,30,127,188,15,224,15,224,7,192,5,64,15,192,31,224,63,224
        .byte 3,108,2,28,3,12,15,4,7,12,2,12,0,14,0,11,0,31,36,145,196,127,14,17,14,21,15,10,155,9,123,236
        .byte 143,175,1,252,1,252,0,248,0,248,0,168,1,252,1,252,0,248,0,216,0,216,0,136,0,216,0,216,1,220,3,86
        .byte 56,28,16,8,0,0,16,8,16,8,198,99,186,93,186,93,198,99,16,8,16,8,198,99,186,93,186,93,198,99,16,8
        .byte 129,0,195,129,36,66,153,36,90,189,153,36,36,66,195,129,112,224,32,64,0,0,32,64,32,64,143,25,118,233,118,233
        .byte 96,96,96,96,97,104,99,108,99,108,97,104,96,96,96,96,111,111,96,96,111,111,96,96,111,111,96,96,255,159,159,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,48,12,122,94,106,86,199,227,133,161,131,193,102,102,5,160,12,48,12,48
        .byte 129,231,129,36,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,48,192,121,233
        .byte 6,150,1,104,1,104,0,144,1,152,1,104,3,12,3,12,7,158,4,146,4,146,0,0,0,0,0,0,0,0,0,0
        .byte 32,4,32,4,32,4,48,12,48,12,48,12,48,12,58,92,26,88,5,160,5,160,2,64,6,96,6,96,5,160,4,32
        .byte 0,231,0,165,0,0,0,0,0,0,0,0,0,0,0,0,16,128,16,128,16,128,48,192,48,192,48,192,48,192,112,233
        .byte 6,150,1,104,1,104,0,144,1,152,1,104,3,12,3,12,7,158,4,146,4,146,0,0,0,0,0,0,0,0,0,0
        .byte 15,0,80,128,63,192,22,32,55,128,31,0,14,0,0,0,12,0,10,0,11,0,11,0,11,0,17,0,28,0,29,0
        .byte 0,224,0,224,0,224,0,0,0,224,0,192,0,224,0,240,0,60,1,66,0,255,128,88,0,222,0,124,0,56,0,0
        .byte 3,128,6,192,14,224,31,112,59,188,53,216,38,64,7,0,3,96,3,112,3,112,5,0,30,48,26,32,16,48,0,56
        .byte 15,0,80,128,63,192,22,32,55,128,31,0,14,0,0,0,12,0,10,0,11,0,11,0,11,0,19,0,28,0,29,0
        .byte 0,224,0,224,0,224,0,0,0,224,0,192,0,224,0,176,0,60,1,66,0,255,128,88,0,222,0,124,0,56,0,0
        .byte 2,128,7,64,15,96,28,240,57,252,51,216,37,64,6,0,7,224,3,240,3,240,5,0,30,48,26,32,16,48,0,56
        .byte 0,240,1,10,3,252,4,104,1,236,0,248,0,112,0,0,0,48,0,80,0,208,0,208,0,208,0,136,0,56,0,184
        .byte 0,7,0,7,0,7,0,0,0,7,0,3,0,7,0,15,192,3,40,4,240,15,160,17,176,7,224,3,192,1,0,0
        .byte 0,28,0,54,0,119,128,239,195,221,193,186,64,38,0,14,0,108,0,236,0,236,0,10,128,199,128,69,128,192,1,192
        .byte 0,240,1,10,3,252,4,104,1,236,0,248,0,112,0,0,0,48,0,80,0,208,0,208,0,208,0,200,0,56,0,184
        .byte 0,7,0,7,0,7,0,0,0,7,0,3,0,7,0,13,192,3,40,4,240,15,160,17,176,7,224,3,192,1,0,0
frmlst:
        .byte 0,4
        .byte 4,4
        .byte 8,4
        .byte 12,4
        .byte 16,4
        .byte 20,4
        .byte 24,1
        .byte 25,1
        .byte 26,4
        .byte 30,4
        .byte 34,4
        .byte 38,4
        .byte 42,1
        .byte 43,1
        .byte 44,1
        .byte 45,1
        .byte 46,2
        .byte 48,5
        .byte 53,1
        .byte 54,1
        .byte 55,1,56,0
.out .sprintf("SPRITES SIZE = %d", (* - sprgfx))

scdat:
        .word 156,327,146,304,277,279,218,238,193,165,182,213,299,284,189,269,127,231,283,231,278,148,25
        .byte 0,108,255,0,26,108,255,0,16,107,0,0,108,255,0,19,108,255,0,19,108,0,0,0,107,255,0,20,107,255,0,35,107
        .byte 255,0,12,108,255,0,52,108,255,0,12,108,255,0,7,108,255,0,10,107,255,0,36,108,255,0,8,107,255,0,17,108,255,0,16
        .byte 108,0,0,108,255,0,23,108,255,0,10,63,255,0,8,108,255,0,20,108,255,0,59,63,255,0,17,48,1,49,255,0,20
        .byte 48,1,1,52,1,1,1,52,255,1,4,52,255,27,12,48,1,1,52,1,52,1,52,1,1,1,52,1,1,52,1,1,1
        .byte 255,30,9,48,1,1,52,255,1,5,52,1,52,52,1,1,52,1,1,1,52,1
        .byte 0,108,255,0,22,59,54,64,60,60,54,255,0,15,107,0,0,108,255,0,4,59,58,67,64,61,61,54,255,0,8,108,255,0,14
        .byte 59,58,67,64,61,61,54,0,0,107,255,0,19,59,58,67,64,67,60,60,54,255,0,22,59,58,67,67,64,61,61,54,255,0,12
        .byte 108,255,0,9,59,58,58,67,67,61,61,54,255,0,21,59,59,67,67,64,64,60,60,54,255,0,5,108,255,0,12,108,0
        .byte 59,255,58,4,67,64,61,61,54,255,0,7,107,255,0,12,59,58,67,58,58,67,64,61,61,54,255,0,14,108,255,0,4
        .byte 59,58,67,58,58,67,64,64,60,60,54,255,0,11,108,255,0,7,59,58,67,58,67,58,67,67,61,61,54,0,108,255,0,16
        .byte 59,58,67,58,58,58,67,58,58,61,61,54,255,0,8,48,1,1,49,0,0,0,108,0,0,59,58,67,67,67,58,58,67
        .byte 58,60,60,54,255,0,6,108,48,52,1,52,1,49,255,0,4,59,58,67,58,58,67,58,58,58,67,61,61,54,255,0,6
        .byte 48,255,1,6,49,0,59,59,59,67,67,58,67,58,67,58,67,67,61,61,54,255,0,4,48,1,1,52,1,1,1,52,255,1,5
        .byte 255,54,13,1,1,52,255,1,5,52,255,1,4,52,1,1,52,1,255,54,12,255,1,4,52,1,52,1,1,1,52,255,1,4
        .byte 52,1,1,52,255,54,11,1,52,255,1,8,52,1,1,52,255,1,6,255,54,10
        .byte 0,108,255,0,26,108,255,0,16,107,0,0,108,255,0,19,108,255,0,19,108,0,0,0,107,255,0,20,107,255,0,35,107
        .byte 255,0,12,108,255,0,52,108,255,0,12,108,255,0,7,34,34,34,255,0,8,107,255,0,17,48,54,54,54,255,0,15,108
        .byte 255,0,8,107,0,54,64,255,0,14,108,255,0,13,54,64,0,108,0,0,108,255,0,23,54,64,255,0,18,108,255,0,9
        .byte 54,64,255,0,9,108,255,0,18,54,64,255,0,28,54,64,255,0,28,54,64,255,0,28,54,64,60,60,54,255,0,24,59
        .byte 54,64,61,61,54,255,0,24,59,54,64,61,61,54
        .byte 0,108,255,0,26,108,255,0,16,107,0,0,108,255,0,19,108,255,0,19,108,59,0,0,107,255,0,14,3,6,4,0,0,0
        .byte 107,255,0,4,59,54,255,0,16,3,5,5,6,4,255,0,7,59,54,255,0,12,108,0,0,3,5,5,6,6,6,4,255,0,6
        .byte 59,54,255,0,15,3,5,255,6,4,4,255,0,6,59,54,255,0,5,108,255,0,9,3,255,5,4,6,4,255,0,4,108
        .byte 59,54,54,255,0,7,107,0,0,3,5,4,0,0,0,3,5,5,6,4,255,0,6,59,54,54,255,0,9,3,6,6,6
        .byte 4,108,0,0,3,9,4,0,0,0,107,0,0,0,59,54,54,255,0,8,3,5,255,6,4,4,0,0,0,9,255,0,7
        .byte 59,54,54,54,0,108,255,0,7,3,5,5,6,4,255,0,4,9,255,0,6,108,0,55,55,55,255,0,10,3,9,4,0,0
        .byte 108,0,0,9,255,0,8,55,55,55,255,0,6,108,255,0,4,9,255,0,6,9,255,0,8,7,7,7,255,0,11,9,255,0,6
        .byte 9,255,0,22,9,255,0,6,9,255,0,11,54,54,54,49,255,0,6,59,9,19,255,0,4,59,9,19,255,0,5,29,29
        .byte 0,0,0,255,54,5,255,29,5,10,9,11,255,29,4,10,9,11,29,29,29,59,59,29,29,255,54,8,255,28,22,54,54,54
        .byte 255,54,8,255,55,22,255,54,7,55,55,255,7,21,255,54,6,55,55,7,255,0,21,255,54,5,55,55,7,255,0,22,255,54,4
        .byte 55,55,7,0,72,73,0,0,0,255,55,13,255,0,4,255,54,4,55,55,0,0,74,75,0,0,55,255,7,13,55,0,0,0
        .byte 54,54,54,255,55,9,7,255,0,13,7,55,55,55,54,54,54,55,55,55,255,7,5,55,255,0,7,65,255,0,7,7,7,7
        .byte 54,54,54,55,55,7,255,0,5,55,255,0,7,66,255,0,10,54,54,55,55,7,255,0,6,55,255,0,18,54,54,55,55
        .byte 255,0,7,55,255,0,18,55,55,55,7,255,0,7,55,21,22,55,55,55,255,0,5,55,55,55,255,0,5,55,55,55,255,0,8
        .byte 55,21,22,55,7,7,55,0,0,0,55,7,7,7,55,255,0,4,7,7,7,255,0,8,7,21,22,7,0,0,7,55,55,55
        .byte 7,0,0,0,7,255,55,4,255,0,12,21,22,255,0,4,7,7,7,255,0,5,255,7,4,255,0,12,21,22,255,0,14
        .byte 55,55,255,0,12,21,22,255,0,9,79,0,32,26,33,68,69,54,54,54,49,255,0,8,21,22,255,0,9,80,0,76,26
        .byte 77,70,71,255,54,30
        .byte 255,55,18,21,22,255,55,10,255,7,17,55,21,22,55,255,7,5,60,60,7,55,255,0,17,7,21,22,55,255,0,5,35,35
        .byte 0,55,255,0,18,21,22,55,255,0,5,35,35,0,55,255,0,18,21,22,55,255,0,5,35,35,0,55,255,0,18,21,22
        .byte 55,255,0,5,36,36,0,255,55,12,21,22,255,55,8,0,0,0,55,55,36,36,55,55,255,7,11,21,22,255,7,8,0,0,0
        .byte 55,7,36,36,7,55,255,0,11,21,22,255,0,11,55,0,36,36,0,55,255,0,11,21,22,255,0,4,65,255,0,6,55
        .byte 0,36,36,0,55,255,0,11,21,22,255,0,4,66,0,0,255,55,5,0,36,36,0,55,255,0,11,21,22,255,0,7,255,7,4
        .byte 55,0,36,36,0,55,255,0,5,255,55,5,0,21,22,255,0,11,55,0,36,36,0,255,55,6,255,7,5,255,55,5,255,0,9
        .byte 55,0,36,36,0,55,255,7,5,255,0,5,255,7,5,255,0,9,55,0,36,36,0,55,55,55,255,0,22,55,0,36,36
        .byte 0,55,68,69,0,78,255,0,17,32,26,33,55,0,36,36,0,55,70,71,0,80,255,0,17,76,26,77,55,0,36,36,0
        .byte 55,255,54,26,36,36,54,54
        .byte 255,55,18,0,0,0,255,55,10,255,7,16,55,0,0,0,255,7,7,55,55,55,255,0,16,55,255,0,10,7,55,55,255,0,16
        .byte 55,255,0,11,7,55,255,0,10,65,255,0,5,55,255,0,12,55,255,0,10,66,255,0,5,255,55,4,255,0,9,255,55,7
        .byte 255,0,10,255,55,4,68,69,255,55,8,255,7,6,255,0,14,70,71,0,55,55,255,7,4,55,255,0,16,255,55,8,7
        .byte 255,0,4,55,0,0,72,73,255,0,12,255,7,8,255,0,5,55,0,0,74,75,255,0,24,255,55,12,255,0,17,55,7
        .byte 255,55,16,255,0,11,55,7,0,255,55,5,7,7,255,55,4,255,7,5,255,55,4,0,0,0,255,55,4,7,0,0,255,55,4
        .byte 7,0,0,255,7,4,255,0,5,255,7,4,0,0,0,255,7,4,0,0,0,55,55,55,7,255,0,26,55,55,7,255,0,27
        .byte 55,55,255,0,28,54,54,255,0,4,48,255,54,23
        .byte 255,55,26,21,22,55,55,55,255,7,25,21,22,7,7,55,255,0,25,21,22,0,0,55,255,0,16,65,255,0,8,21,22
        .byte 0,0,55,0,72,73,255,0,13,66,255,0,8,21,22,0,0,55,0,74,75,255,0,22,21,22,0,0,255,55,10,255,0,14
        .byte 255,55,7,255,7,9,55,255,0,12,55,255,7,6,55,255,0,9,7,55,255,0,10,55,7,255,0,6,55,255,0,10,7
        .byte 255,55,10,7,255,0,7,55,255,0,11,7,55,55,255,7,7,255,0,4,255,55,5,255,0,12,55,7,255,0,10,55,255,7,4
        .byte 55,255,0,11,55,7,255,0,10,55,7,255,0,4,55,255,0,10,55,7,255,0,5,255,55,6,7,255,0,5,55,255,0,4
        .byte 68,69,255,0,4,7,255,0,5,55,255,7,6,255,0,6,55,255,0,4,70,71,255,0,9,55,7,255,0,11,55,55,255,0,4
        .byte 68,69,255,0,8,55,7,255,0,11,55,55,55,255,0,4,70,71,255,0,7,55,7,255,0,11,255,55,21,21,22,255,55,10
        .byte 255,54,97,255,55,23,255,54,6,255,55,24,255,54,5,55,55,55,255,7,22,255,54,4,55,55,55,7,255,0,22,54,54,54
        .byte 55,55,55,7,255,0,10,65,255,0,12,54,54,55,55,55,7,255,0,11,66,255,0,12,54,55,55,55,7,255,0,25,55,55,55
        .byte 255,0,25,55,55,46,46,46,255,0,23,55,55,7,7,55,55,255,53,5,31,31,53,53,12,255,0,12,55,55,7,7,0,0
        .byte 55,55,255,31,9,12,255,0,10,55,55,7,7,255,0,4,55,55,255,31,9,12,255,0,8,55,55,7,7,255,0,6,55,55
        .byte 255,31,9,12,255,0,6,55,55,7,7,255,0,8,55,55,255,31,9,12,255,0,4,55,55,7,7,0,68,69,255,0,7
        .byte 55,55,255,46,13,55,55,55,0,0,0,70,71,255,0,6,255,55,27,21,22,55,55
        .byte 255,54,32,255,55,28,54,255,55,32,255,7,25,255,55,5,255,0,25,55,55,7,7,7,255,0,11,65,255,0,13,55,55
        .byte 255,0,14,66,255,0,8,68,69,0,0,0,55,55,255,0,23,70,71,0,0,0,55,55,255,0,23,68,69,0,72,73,55,55
        .byte 255,0,23,70,71,0,74,75,255,55,11,21,22,255,55,19,255,7,9,21,22,255,7,17,55,55,255,0,9,21,22,255,0,17
        .byte 55,55,255,0,9,21,22,255,0,17,55,55,255,0,9,21,22,255,0,17,55,55,255,0,9,21,22,255,0,17,55,55,255,0,9
        .byte 21,22,255,0,12,79,0,32,26,33,55,55,55,255,0,8,21,22,255,0,12,80,0,76,26,77,255,55,32
        .byte 255,55,30,255,7,28,55,55,255,0,28,55,55,255,0,14,65,255,0,13,55,55,255,0,14,66,255,0,13,55,55,255,0,28
        .byte 255,55,9,255,0,21,55,55,255,7,7,255,0,18,72,73,0,55,55,255,0,25,74,75,0,55,55,255,0,11,55,55,55
        .byte 255,0,9,255,55,18,7,7,7,255,0,9,255,7,5,55,55,7,7,7,55,55,255,7,6,255,0,17,55,55,0,0,0
        .byte 7,7,255,0,23,55,55,255,0,16,255,55,5,255,0,7,55,55,255,0,16,255,7,5,55,255,0,6,55,55,55,255,0,20
        .byte 7,55,255,0,5,7,7,55,55,255,0,10,55,55,255,0,8,7,55,255,0,6,55,55,55,255,0,8,255,55,4,255,0,8
        .byte 7,55,255,0,5,255,55,18,0,0,0,255,55,9
        .byte 255,55,26,255,54,4,255,55,7,255,7,16,55,55,55,255,54,4,255,55,7,255,0,16,7,55,55,55,54,54,54,255,7,6
        .byte 55,255,0,9,65,255,0,8,55,55,54,54,54,255,0,6,55,255,0,9,66,255,0,8,55,55,55,54,54,255,0,6,55
        .byte 255,0,18,55,55,55,54,54,55,55,55,0,0,0,55,255,0,18,55,55,55,54,54,7,7,7,0,0,0,55,255,0,18
        .byte 55,55,55,54,54,255,0,6,55,68,69,255,0,16,55,55,55,54,54,255,0,6,55,70,71,255,0,16,255,55,17,255,0,13
        .byte 255,55,5,255,7,12,55,255,0,12,255,7,5,255,0,12,7,55,55,55,255,0,4,68,69,255,0,21,7,7,7,255,0,4
        .byte 70,71,255,0,28,68,69,255,0,28,70,71,59,0,0,19,255,0,21,48,255,54,5,21,22,255,54,5,255,0,13,48,255,54,9
        .byte 21,22,255,54,28,21,22,255,54,5
        .byte 55,14,13,55,21,22,255,55,18,14,13,55,0,108,0,55,14,13,55,21,22,55,255,7,16,15,14,13,55,0,0,0,55
        .byte 14,13,55,21,22,255,0,7,65,255,0,9,15,14,13,55,0,108,0,55,14,13,55,21,22,255,0,7,66,255,0,9,15
        .byte 14,13,54,19,0,0,55,14,13,55,21,22,255,0,12,255,55,6,255,54,4,19,107,55,14,13,55,21,22,255,0,12,255,7,4
        .byte 55,55,255,54,4,19,0,55,14,13,55,21,22,255,0,13,32,26,33,55,55,255,54,4,19,0,55,14,13,55,21,22,255,0,13
        .byte 76,26,77,55,55,255,54,4,19,0,55,14,13,255,55,4,255,0,8,255,55,9,255,54,4,19,0,55,14,13,255,7,4
        .byte 255,0,5,255,55,4,255,7,6,55,55,55,255,54,4,19,55,14,13,255,0,9,255,7,4,255,0,6,55,55,55,255,54,4
        .byte 19,55,14,13,16,255,0,18,55,55,55,255,54,5,55,14,13,16,255,0,18,55,55,55,255,54,5,55,14,13,16,255,0,18
        .byte 55,55,55,255,54,5,255,55,16,255,0,4,68,69,55,55,55,255,54,5,255,7,16,255,0,4,70,71,255,55,4,255,54,4
        .byte 255,0,20,68,69,255,55,4,255,54,4,255,0,20,70,71,255,55,4,255,54,4,255,55,26,255,54,4
        .byte 255,54,14,19,255,0,13,108,0,255,55,12,54,54,54,255,55,5,255,0,10,255,55,21,255,0,7,108,0,55,14,13,16
        .byte 255,7,7,255,55,5,255,7,4,55,55,0,107,255,0,6,55,14,13,16,255,0,7,255,7,5,255,0,4,7,55,55,255,0,6
        .byte 107,55,14,13,15,255,0,17,7,55,55,255,0,6,55,14,13,255,55,5,255,0,10,65,0,0,0,7,55,55,255,0,5
        .byte 55,14,13,55,255,7,4,55,255,0,9,66,255,0,4,7,55,55,108,0,0,0,55,14,13,55,255,0,4,7,55,255,0,14
        .byte 7,55,55,0,0,0,55,14,13,55,255,0,5,7,255,0,15,55,55,0,0,0,55,14,13,55,255,0,21,55,55,0,108
        .byte 0,55,14,13,55,255,0,21,55,55,0,0,0,55,14,13,255,55,10,255,0,12,55,55,0,0,0,55,14,13,55,255,7,9
        .byte 55,255,0,11,55,55,0,0,0,55,14,13,55,255,0,9,7,255,55,4,255,0,7,7,55,0,0,0,55,14,13,55,255,0,10
        .byte 255,7,4,255,0,8,55,0,0,0,55,14,13,55,255,0,22,55,0,0,0,55,14,13,55,255,0,22,55,0,0,0,55
        .byte 14,13,55,21,22,255,55,18,14,13,55,0,0,0
        .byte 255,54,26,36,36,255,54,11,255,0,17,36,36,255,54,6,0,51,0,51,255,0,18,36,36,255,54,6,255,0,22,36,36
        .byte 255,54,4,255,0,24,36,36,255,54,4,255,0,9,255,55,17,255,54,4,255,0,9,255,7,17,255,54,4,255,0,26,255,54,4
        .byte 68,69,255,0,24,255,54,4,70,71,255,0,18,65,255,0,5,255,54,4,255,55,5,255,0,15,66,255,0,5,255,54,9
        .byte 55,255,0,20,255,54,10,55,255,0,19,255,54,11,55,55,55,255,0,4,55,55,55,255,0,11,54,54,255,0,7,7,7,7
        .byte 255,0,4,7,7,7,55,255,0,10,54,54,255,0,11,34,34,255,0,4,7,55,255,0,9,54,54,255,0,10,59,62,62
        .byte 19,255,0,4,7,55,55,55,255,0,6,255,54,24,55,255,0,5,255,54,30
        .byte 54,54,19,255,0,25,108,0,54,54,19,255,0,12,107,0,0,108,255,0,11,54,54,19,255,0,5,108,255,0,19,108,0
        .byte 54,54,54,19,255,0,19,107,255,0,6,54,54,54,19,255,0,25,107,54,54,54,19,255,0,8,108,255,0,17,255,54,4
        .byte 19,255,0,25,255,54,5,19,255,0,12,108,255,0,7,108,0,0,0,255,54,6,0,107,0,0,3,5,4,255,0,7,3
        .byte 6,4,255,0,7,255,55,6,0,0,0,3,6,6,6,4,108,255,0,4,3,5,6,6,4,255,0,6,255,55,6,0,0
        .byte 3,5,255,6,4,4,0,0,0,3,5,5,5,6,6,4,0,0,0,108,0,255,7,6,0,0,0,3,5,5,6,4,255,0,5
        .byte 3,5,5,6,4,0,108,255,0,14,3,9,4,0,0,108,255,0,4,3,9,4,255,0,13,108,255,0,4,9,255,0,9
        .byte 9,255,0,19,9,255,0,9,9,255,0,19,9,255,0,9,9,255,0,8,255,54,5,49,255,0,4,59,9,19,255,0,7
        .byte 59,9,19,0,0,29,29,48,255,54,8,255,29,4,10,9,11,255,29,7,10,9,11,59,59,29,29,255,54,9,255,28,21
        .byte 54,54,54
        .byte 0,108,255,0,26,108,255,0,16,107,0,0,108,255,0,19,108,255,0,19,108,0,0,0,107,255,0,20,107,255,0,35,107
        .byte 255,0,12,108,255,0,52,108,255,0,12,108,255,0,7,108,255,0,10,107,255,0,36,108,255,0,8,107,255,0,17,108,255,0,16
        .byte 108,0,0,108,255,0,23,108,255,0,19,108,255,0,20,108,255,0,28,72,73,255,0,28,74,75,255,0,23,54,60,60,54
        .byte 255,64,4,49,255,0,21,54,61,61,54,64,64,64,255,0,23,54,61,61,54,64,64,255,0,24
        .byte 54,60,60,54,54,255,0,23,108,0,54,61,61,54,54,255,0,10,107,0,0,108,255,0,11,54,61,61,54,54,0,0,0
        .byte 108,255,0,19,108,0,54,60,60,54,54,255,0,18,107,255,0,6,54,61,61,54,54,255,0,24,107,54,61,61,54,255,0,8
        .byte 108,255,0,17,54,60,60,54,255,0,26,54,61,61,54,0,108,255,0,12,108,255,0,7,108,0,0,0,54,61,61,54,0,0,0
        .byte 107,255,0,22,54,60,60,255,0,11,108,255,0,8,107,255,0,6,54,61,61,255,0,8,108,255,0,16,108,0,54,61,61
        .byte 255,0,22,108,255,0,4,255,54,4,49,255,0,10,108,255,0,14,255,1,4,54,0,108,255,0,23,1,60,60,1,54,255,0,25
        .byte 255,1,4,54,0,34,255,0,5,34,255,0,5,34,255,0,11,255,54,5,0,62,0,0,34,0,0,62,0,0,34,0,0
        .byte 62,255,0,11,255,54,5,37,62,38,37,62,38,37,62,38,37,62,38,37,62,38,255,50,10,255,30,30
        .byte 255,54,51,60,60,255,54,9,0,0,255,54,17,35,35,51,255,0,5,54,54,54,0,0,0,54,0,0,51,0,0,54,54,54
        .byte 0,0,54,54,54,0,51,35,35,255,0,6,54,54,54,0,0,0,51,255,0,6,51,255,0,4,51,0,0,0,35,35,255,0,6
        .byte 54,54,54,255,0,19,36,36,255,0,4,72,73,54,54,54,255,0,19,36,36,255,0,4,74,75,54,54,54,255,0,19,36,36
        .byte 0,0,0,255,54,4,255,0,21,36,36,255,0,4,54,54,54,255,0,15,65,255,0,5,36,36,255,0,4,54,54,54,255,0,15
        .byte 66,255,0,5,36,36,255,0,5,54,54,255,0,28,255,54,5,49,255,0,6,55,55,55,255,0,4,55,55,55,255,0,8
        .byte 54,54,255,1,4,54,255,0,5,7,7,7,255,0,4,7,7,7,255,0,8,54,54,1,60,60,1,54,0,0,34,255,0,19
        .byte 48,54,54,255,1,4,54,0,0,62,255,0,7,34,255,0,5,34,255,0,5,255,54,8,0,37,62,38,255,0,5,37,62
        .byte 38,255,0,4,62,0,0,0,34,0,0,255,54,7,37,62,62,62,38,50,50,50,37,62,62,62,38,50,50,37,62,38,50
        .byte 37,62,38,50,50,50,255,54,5,255,30,25
        .byte 255,54,23,21,22,255,54,6,255,55,8,255,54,8,255,55,6,21,22,55,255,54,4,12,255,7,8,255,55,8,255,7,6
        .byte 21,22,7,255,54,4,12,255,0,8,255,7,8,255,0,6,21,22,0,255,54,4,12,255,0,22,21,22,0,255,54,4,12
        .byte 255,0,22,21,22,0,255,54,5,255,55,8,255,0,12,255,55,6,255,54,4,255,7,8,55,255,0,11,255,7,6,255,54,4
        .byte 255,0,8,7,55,255,0,19,54,255,0,9,7,255,55,5,255,0,14,54,255,55,4,21,22,255,0,4,255,7,5,55,255,0,13
        .byte 255,54,5,21,22,255,0,9,7,55,255,0,12,255,54,4,0,21,22,255,0,6,65,0,0,0,7,255,55,4,255,0,5
        .byte 255,54,6,0,0,21,22,255,0,6,66,255,0,4,255,7,4,255,0,4,255,54,4,55,55,55,255,0,22,255,54,5,7,7,7
        .byte 255,0,21,255,54,6,255,0,9,34,255,0,13,255,54,7,255,0,8,59,62,19,255,0,11,255,54,38
        .byte 54,64,255,0,4,64,255,54,24,64,255,0,4,64,54,255,55,10,0,65,0,255,55,9,54,64,255,0,4,64,54,55,255,7,9
        .byte 0,66,0,255,7,9,54,64,255,0,4,64,54,55,255,0,21,54,64,255,0,4,64,54,55,0,0,0,78,81,82,81,82
        .byte 79,255,0,12,54,64,255,0,4,64,54,55,0,0,0,80,45,47,45,47,80,255,0,12,54,64,255,0,4,46,46,255,55,19
        .byte 21,22,54,54,64,255,0,4,43,44,255,7,19,21,22,54,54,64,255,0,4,47,45,255,0,19,21,22,54,54,64,255,0,4
        .byte 47,45,255,0,19,21,22,54,54,64,255,0,4,43,44,255,0,19,21,22,54,54,255,46,20,21,22,255,46,6,255,54,4
        .byte 58,58,54,54,54,255,0,12,54,21,22,54,54,58,58,255,54,6,58,58,54,54,54,255,0,12,54,21,22,54,54,58,58
        .byte 54,54,54,55,55,55,54,54,55,55,55,255,0,12,55,21,22,55,55,54,54,55,55,55,7,7,7,0,0,7,7,7,255,0,12
        .byte 7,21,22,7,7,0,0,7,7,7,255,0,11,255,34,6,63,0,0,0,21,22,255,0,17,59,255,62,6,19,0,0,0
        .byte 21,22,255,0,7,255,54,30
        .byte 0,108,255,0,26,108,255,0,16,107,0,0,108,255,0,19,108,255,0,19,108,0,0,0,107,255,0,20,107,255,0,35,107
        .byte 255,0,12,108,255,0,52,108,255,0,12,108,255,0,7,108,255,0,10,107,255,0,36,108,255,0,8,107,255,0,17,108,255,0,16
        .byte 108,0,0,108,255,0,23,108,255,0,19,108,255,0,20,108,255,0,77,48,1,49,255,0,20,48,1,1,52,1,1,1,52
        .byte 255,1,4,52,255,27,12,48,1,1,52,1,52,1,52,1,1,1,52,1,1,52,1,1,1,255,30,9,48,1,1,52,255,1,5
        .byte 52,1,52,52,1,1,52,1,1,1,52,1
        .byte 255,0,0,255,0,88,20,2,255,0,53,46,255,0,10,46,255,0,18,255,46,12,255,0,129
numsc:
        .byte 23
.out .sprintf("SCREENS SIZE = %d", (* - scdat))

nmedat:
        .byte 0,3,120,112,7,12,120,32,7,13,120,48,7,14,120,64,255
        .byte 0,4,112,8,3,11,32,64,255
        .byte 0,4,112,224,255
        .byte 0,4,112,8,3,11,40,80,2,10,128,168,255
        .byte 0,4,120,8,2,10,128,152,255
        .byte 0,4,128,24,2,9,88,112,255
        .byte 0,3,128,224,255
        .byte 0,3,32,232,3,8,80,72,2,9,32,64,255
        .byte 0,4,120,112,255
        .byte 0,4,64,8,2,10,128,88,3,8,48,184,255
        .byte 0,3,128,216,2,9,128,72,3,8,40,80,255
        .byte 0,3,112,232,3,8,40,80,2,9,128,72,255
        .byte 0,4,128,8,4,16,16,192,255
        .byte 0,4,128,40,4,16,32,32,6,7,80,40,255
        .byte 0,3,128,232,3,8,56,24,255
        .byte 0,4,112,8,3,11,16,80,2,10,120,208,255
        .byte 0,4,112,8,3,11,64,144,255
        .byte 0,3,40,24,255
        .byte 0,4,80,8,255
        .byte 0,3,32,176,3,8,56,104,255
        .byte 0,4,72,24,2,9,32,112,2,9,32,96,5,6,128,32,5,6,128,208,255
        .byte 7,12,120,32,7,13,120,48,7,14,120,64,255
        .byte 8,15,72,104,8,18,96,104,8,19,72,136,8,20,96,136,255
.out .sprintf("POSITIONS SIZE = %d", (* - nmedat))

NUMOBJ = 11
objdta:
        .byte 0,0,0,0,31,248,51,204,100,38,72,18,200,19,147,201,254,255,130,65,131,193,185,157,128,1,162,69,136,17,255,255,5,73,176,5,73,176
        .byte 0,0,0,0,31,248,51,204,100,38,72,18,200,19,147,201,254,255,130,65,131,193,185,157,128,1,162,69,136,17,255,255,7,136,24,7,136,24
        .byte 0,0,0,0,31,248,51,204,100,38,72,18,200,19,147,201,255,127,130,65,131,193,185,157,128,1,162,69,136,17,255,255,11,56,64,11,56,64
        .byte 0,0,0,0,0,0,0,0,3,192,4,32,10,80,9,144,9,144,10,80,4,32,3,192,0,0,0,0,0,0,0,0,20,72,56,20,72,56
        .byte 0,0,0,0,8,0,0,0,0,0,0,0,8,0,0,0,0,0,0,0,8,0,0,0,0,0,0,0,0,0,0,0,12,56,168,12,56,168
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,0,0,18,48,224,18,48,224
        .byte 0,0,0,0,31,248,51,204,100,38,72,18,200,19,147,201,255,127,130,65,131,193,185,157,128,1,162,69,136,17,255,255,20,40,80,20,40,80
        .byte 1,128,3,192,2,64,1,128,2,64,4,160,4,32,15,240,12,48,14,240,14,112,14,240,12,48,15,240,12,48,7,224,5,40,200,5,40,200
        .byte 1,128,3,192,2,64,1,128,2,64,4,160,4,32,15,240,12,48,14,240,14,112,14,240,12,48,15,240,12,48,7,224,6,40,32,6,40,32
        .byte 1,128,3,192,2,64,1,128,2,64,4,160,4,32,15,240,12,48,14,240,14,112,14,240,12,48,15,240,12,48,7,224,19,72,16,19,72,16
        .byte 1,128,3,192,2,64,1,128,2,64,4,160,4,32,15,240,12,48,14,240,14,112,14,240,12,48,15,240,12,48,7,224,15,120,232,15,120,232
.out .sprintf("OBJECTS SIZE = %d", (* - objdta))

font:
        .byte 0,0,0,0,0,0,0,0
        .byte 60,36,36,36,36,60,36,60
        .byte 127,73,201,147,254,0,0,0
        .byte 0,126,203,129,203,129,203,126
        .byte 124,196,172,198,234,170,198,124
        .byte 119,221,139,214,107,209,187,238
        .byte 126,194,159,193,155,154,198,124
        .byte 60,36,100,76,120,0,0,0
        .byte 62,98,78,72,72,78,98,62
        .byte 124,70,114,18,18,114,70,124
        .byte 0,126,90,231,129,231,90,126
        .byte 0,60,36,231,129,231,36,60
        .byte 0,0,0,60,36,100,76,120
        .byte 0,0,0,255,129,255,0,0
        .byte 0,0,0,0,60,36,36,60
        .byte 0,15,25,51,102,204,152,240
        .byte 126,195,153,149,141,157,195,126
        .byte 60,228,132,228,36,231,129,255
        .byte 126,195,185,249,195,159,129,255
        .byte 126,195,185,227,249,185,195,126
        .byte 30,50,98,210,179,129,243,30
        .byte 255,129,159,131,249,185,195,126
        .byte 126,194,158,131,157,157,195,126
        .byte 255,129,249,51,38,36,36,60
        .byte 126,195,157,195,157,157,195,126
        .byte 126,195,185,185,193,121,67,126
        .byte 0,60,36,36,60,36,36,60
        .byte 0,60,36,36,60,100,76,120
        .byte 0,30,50,102,76,102,50,30
        .byte 0,0,255,129,255,129,255,0
        .byte 0,120,76,102,50,102,76,120
        .byte 126,195,153,249,35,62,36,60
        .byte 0,0,0,0,0,0,0,0
        .byte 126,195,185,185,129,185,169,239
        .byte 254,131,157,131,157,157,131,254
        .byte 126,195,157,151,151,157,195,126
        .byte 254,131,157,149,149,157,131,254
        .byte 255,129,159,132,156,159,129,255
        .byte 255,129,159,132,156,144,144,240
        .byte 126,195,157,159,145,157,195,126
        .byte 247,149,157,129,157,149,149,247
        .byte 60,36,36,36,36,36,36,60
        .byte 15,9,9,9,233,185,195,126
        .byte 247,157,155,150,134,155,157,247
        .byte 240,144,144,144,144,159,129,255
        .byte 247,157,137,149,157,149,149,247
        .byte 247,157,141,149,153,157,149,247
        .byte 126,195,157,149,149,157,195,126
        .byte 254,131,157,157,131,158,144,240
        .byte 126,195,157,157,149,155,197,127
        .byte 254,131,157,157,131,155,157,247
        .byte 126,194,158,195,249,185,195,126
        .byte 255,129,231,36,36,36,36,60
        .byte 247,149,149,149,149,157,195,126
        .byte 247,149,149,149,157,203,102,60
        .byte 247,149,149,157,149,149,203,126
        .byte 247,157,203,102,102,211,185,239
        .byte 247,149,157,203,102,36,36,60
        .byte 255,129,243,102,204,159,129,255
        .byte 126,66,78,72,72,78,66,126
        .byte 0,240,152,204,102,51,25,15
        .byte 126,66,114,18,18,114,66,126
        .byte 60,102,211,185,239,0,0,0
        .byte 0,0,0,0,0,255,129,255
        .byte 62,98,206,130,207,205,131,254
        .byte 0,124,198,178,130,178,178,254
        .byte 0,252,134,154,134,154,134,252
        .byte 0,124,198,154,158,154,198,124
        .byte 0,252,134,154,154,154,134,252
        .byte 0,254,130,158,132,158,130,254
        .byte 0,254,130,158,132,156,144,240
        .byte 0,124,198,158,146,154,198,124
        .byte 0,254,154,154,130,154,154,254
        .byte 0,60,36,36,36,36,36,60
        .byte 0,30,18,18,242,178,198,124
        .byte 0,254,154,150,140,150,154,254
        .byte 0,240,144,144,144,158,130,254
        .byte 0,247,157,137,149,157,149,247
        .byte 0,254,154,138,146,154,154,254
        .byte 0,124,198,154,154,154,198,124
        .byte 0,252,134,154,134,156,144,240
        .byte 0,124,198,154,154,150,202,126
        .byte 0,252,134,154,134,150,154,254
        .byte 0,124,196,156,198,242,134,252
        .byte 0,252,132,204,72,72,72,120
        .byte 0,254,154,154,154,154,198,124
        .byte 0,254,154,154,154,198,108,56
        .byte 0,247,149,157,149,149,203,126
        .byte 0,254,154,154,198,154,154,254
        .byte 0,254,178,178,194,114,70,124
        .byte 0,254,130,242,198,158,130,254
        .byte 62,98,206,152,200,78,98,62
        .byte 56,40,40,40,40,40,40,56
        .byte 124,70,115,25,19,114,70,124
        .byte 0,120,207,165,243,30,0,0
        .byte 126,219,165,173,173,165,219,126
.out .sprintf("FONT SIZE = %d", (* - font))

jtab:
        .byte 248,250,252,254,254,255,255,255,0,0,0,1,1,1,2,2,4,6,8,8,8,99
keys:   .byte 66,97,104,72,73,16,55,48,49,17,18
.out .sprintf("KEYS SIZE = %d", (* - keys))

data_end:
.out .sprintf("TOTAL DATA SIZE = %d", (* - data_start))
