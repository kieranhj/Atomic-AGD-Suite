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
MAPWID = 13
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255
mapdat:
        .byte 255,0,1,2,3,4,5,6,7,8,9,10,255,255,11,12,13,14,15,255,255,255,255,17,16,255,255,255,255,19,18,255,255,255,255,255,255,255,255,255,255,22,20,23,255,255,255,255,255,255,255,255,255,255,28,21,24,255,255,255,255,255,255,255,255,255,255,27,26,25,255,255,255,255,255,255,255,255
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255
stmap:  .byte 1

evnt00:
        lda #8
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp a00042
        jsr scru
        lda #136
        ldy #8
        sta (z80_ix),y
a00042: lda #142
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a00082
        jsr scrd
        lda #12
        ldy #8
        sta (z80_ix),y
a00082: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a00148
:
        lda #4
        cmp scno
        beq *+5
        jmp a00148
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a00148: jsr skobj
        sta varobj
        lda #5
        cmp varobj
        beq *+5
        jmp a00235
        lda varobj
        jsr getob
        lda #<20
        sta z80_l
        lda #>20
        sta z80_h
        jsr addsc
        lda #1
        sta chary
        lda #4
        sta charx
        lda #18
        jsr dmsg
        jmp a00631
a00235: lda #6
        cmp varobj
        beq *+5
        jmp a00313
        lda varobj
        jsr getob
        lda #<20
        sta z80_l
        lda #>20
        sta z80_h
        jsr addsc
        lda #1
        sta chary
        lda #4
        sta charx
        lda #20
        jsr dmsg
        jmp a00631
a00313: lda #52
        cmp varobj
        beq *+5
        jmp a00391
        lda varobj
        jsr getob
        lda #<20
        sta z80_l
        lda #>20
        sta z80_h
        jsr addsc
        lda #1
        sta chary
        lda #4
        sta charx
        lda #22
        jsr dmsg
        jmp a00631
a00391: lda #54
        cmp varobj
        beq *+5
        jmp a00470
        lda varobj
        jsr getob
        lda #<20
        sta z80_l
        lda #>20
        sta z80_h
        jsr addsc
        lda #1
        sta chary
        lda #4
        sta charx
        lda #26
        jsr dmsg
        jmp a00631
a00470: lda #26
        cmp varobj
        beq *+5
        jmp a00548
        lda varobj
        jsr getob
        lda #<20
        sta z80_l
        lda #>20
        sta z80_h
        jsr addsc
        lda #1
        sta chary
        ; lda #1
        sta charx
        lda #24
        jsr dmsg
        jmp a00631
a00548: lda #55
        cmp varobj
        beq *+5
        jmp a00592
        lda varobj
        jsr getob
        lda #90
        asl a
        sta sndtyp
        jmp a00631
a00592: lda #58
        cmp varobj
        beq *+5
        jmp a00631
        lda varobj
        jsr getob
        lda #90
        asl a
        sta sndtyp
a00631: jsr skobj
        sta varobj
        lda #21
        sta chary
        lda #25
        sta charx
        jsr dscor
        lda #21
        sta chary
        lda #16
        sta charx
        lda varo
        jsr disply
        lda #1
        cmp varobj
        beq *+5
        jmp a00783
        lda #5
        jsr gotob
        cmp #255
        beq :+
        jmp a00731
:
        jmp a00778
a00731: lda #1
        sta chary
        lda #2
        sta charx
        lda #19
        jsr dmsg
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a00778: jmp a01464
a00783: lda #2
        cmp varobj
        beq *+5
        jmp a00878
        lda #6
        jsr gotob
        cmp #255
        beq :+
        jmp a00826
:
        jmp a00873
a00826: lda #1
        sta chary
        lda #2
        sta charx
        lda #17
        jsr dmsg
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a00873: jmp a01464
a00878: lda #27
        cmp varobj
        beq *+5
        jmp a00974
        lda #26
        jsr gotob
        cmp #255
        beq :+
        jmp a00922
:
        jmp a00969
a00922: lda #1
        sta chary
        ; lda #1
        sta charx
        lda #23
        jsr dmsg
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a00969: jmp a01464
a00974: lda #51
        cmp varobj
        beq *+5
        jmp a01069
        lda #52
        jsr gotob
        cmp #255
        beq :+
        jmp a01018
:
        jmp a01065
a01018: lda #1
        sta chary
        lda #2
        sta charx
        lda #21
        jsr dmsg
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a01065: jmp a01464
a01069: lda #9
        cmp varobj
        beq *+5
        jmp a01193
        lda varobj
        jsr getob
        lda vart
        clc
        adc #1
        sta vart
        lda #<100
        sta z80_l
        lda #>100
        sta z80_h
        jsr addsc
        lda #50
        asl a
        sta sndtyp
        jsr cls
        lda #10
        sta chary
        lda #3
        sta charx
        lda #11
        jsr dmsg
        jsr prskey
        lda #1
        sta restfl
        jmp a01464
a01193: lda #53
        cmp varobj
        beq *+5
        jmp a01288
        lda #54
        jsr gotob
        cmp #255
        beq :+
        jmp a01237
:
        jmp a01284
a01237: lda #1
        sta chary
        lda #2
        sta charx
        lda #25
        jsr dmsg
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a01284: jmp a01464
a01288: lda #50
        cmp varobj
        beq *+5
        jmp a01399
        lda varobj
        jsr getob
        lda numlif
        clc
        adc #1
        sta numlif
        lda #21
        sta chary
        lda #7
        sta charx
        lda numlif
        jsr disply
        lda #10
        asl a
        sta sndtyp
        lda #1
        sta chary
        lda #2
        sta charx
        lda #12
        jsr dmsg
        jmp a01464
a01399: lda #255
        cmp varobj
        bne *+5
        jmp a01464
        lda varobj
        jsr getob
        lda #<10
        sta z80_l
        lda #>10
        sta z80_h
        jsr addsc
        lda varo
        clc
        adc #1
        sta varo
a01464: lda #14
        cmp scno
        beq *+5
        jmp a01589
        lda #55
        jsr gotob
        cmp #255
        beq :+
        jmp a01589
:
        lda #19
        sta chary
        lda #10
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #10
        jsr pattr
        lda #19
        sta chary
        lda #11
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #11
        jsr pattr
a01589: lda #18
        cmp scno
        beq *+5
        jmp a01715
        lda #58
        jsr gotob
        cmp #255
        beq :+
        jmp a01715
:
        lda #10
        sta chary
        lda #23
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        lda #11
        sta chary
        lda #23
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
a01715: lda joyval
        and #1
        beq :+
        jmp a01986
:
        lda #230
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp a01777
        jsr scrr
        lda #8
        ldy #9
        sta (z80_ix),y
        jmp a01986
a01777: jsr cangr
        beq :+
        jmp a01986
:
        lda joyval
        and #8
        beq :+
        jmp a01909
:
        jsr laddu
        beq :+
        jmp a01826
:
        jmp a01904
a01826: lda #1
        ldy #6
        sta (z80_ix),y
        lda #0
        jsr animsp
        lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a01882
        lda #3
        asl a
        sta sndtyp
a01882: ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a01904: jmp a01986
a01909: lda #1
        ldy #6
        sta (z80_ix),y
        lda #0
        jsr animsp
        lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a01964
        lda #3
        asl a
        sta sndtyp
a01964: ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a01986: lda joyval
        and #2
        beq :+
        jmp a02259
:
        lda #8
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp a02049
        jsr scrl
        lda #230
        ldy #9
        sta (z80_ix),y
        jmp a02259
a02049: jsr cangl
        beq :+
        jmp a02259
:
        lda joyval
        and #8
        beq :+
        jmp a02181
:
        jsr laddu
        beq :+
        jmp a02098
:
        jmp a02176
a02098: lda #0
        ldy #6
        sta (z80_ix),y
        ; lda #0
        jsr animsp
        lda #1
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a02154
        lda #3
        asl a
        sta sndtyp
a02154: ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a02176: jmp a02259
a02181: lda #0
        ldy #6
        sta (z80_ix),y
        ; lda #0
        jsr animsp
        lda #1
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a02237
        lda #3
        asl a
        sta sndtyp
a02237: ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a02259: lda joyval
        and #8
        beq :+
        jmp a02368
:
        jsr laddu
        beq :+
        jmp a02368
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #2
        ldy #6
        sta (z80_ix),y
        lda #0
        jsr animsp
        lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a02368
        lda #3
        asl a
        sta sndtyp
a02368: lda joyval
        and #4
        beq :+
        jmp a02477
:
        jsr laddd
        beq :+
        jmp a02477
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #2
        ldy #6
        sta (z80_ix),y
        lda #0
        jsr animsp
        lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a02477
        lda #3
        asl a
        sta sndtyp
a02477: lda joyval
        and #16
        beq :+
        jmp a02511
:
        jsr jump
        lda #30
        asl a
        sta sndtyp
a02511: jsr cangd
        beq :+
        jmp a02579
:
        jsr laddd
        beq :+
        jmp a02543
:
        jmp a02579
a02543: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a02575
:
        jmp a02579
a02575: jsr ifall
a02579: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a02623
:
        lda #1
        sta deadf
        lda #50
        jsr delay
a02623: jmp grav
.out .sprintf("EVENT 0 SIZE = %d", (* - evnt00))

evnt01:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b00082
        jsr cangu
        beq :+
        jmp b00064
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp b00078
b00064: lda #1
        ldy #11
        sta (z80_ix),y
b00078: jmp b00136
b00082: jsr cangd
        beq :+
        jmp b00123
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp b00136
b00123: lda #0
        ldy #11
        sta (z80_ix),y
b00136: lda #0
        jsr animsp
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00175
:
        lda #1
        sta deadf
b00175: rts
.out .sprintf("EVENT 1 SIZE = %d", (* - evnt01))

evnt02:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp c00181
        jsr cangl
        beq :+
        jmp c00163
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
        jmp c00122
:
        lda #1
        ldy #11
        sta (z80_ix),y
c00122: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp c00176
c00163: lda #1
        ldy #11
        sta (z80_ix),y
c00176: jmp c00333
c00181: jsr cangr
        beq :+
        jmp c00320
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
        jmp c00279
:
        lda #0
        ldy #11
        sta (z80_ix),y
c00279: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp c00333
c00320: lda #0
        ldy #11
        sta (z80_ix),y
c00333: lda #0
        jsr animsp
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00372
:
        lda #1
        sta deadf
c00372: rts
.out .sprintf("EVENT 2 SIZE = %d", (* - evnt02))

evnt03:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp d00082
        jsr cangl
        beq :+
        jmp d00064
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp d00078
d00064: lda #1
        ldy #11
        sta (z80_ix),y
d00078: jmp d00136
d00082: jsr cangr
        beq :+
        jmp d00123
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp d00136
d00123: lda #0
        ldy #11
        sta (z80_ix),y
d00136: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp d00217
        jsr cangu
        beq :+
        jmp d00199
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp d00212
d00199: lda #1
        ldy #10
        sta (z80_ix),y
d00212: jmp d00271
d00217: jsr cangd
        beq :+
        jmp d00257
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp d00271
d00257: lda #0
        ldy #10
        sta (z80_ix),y
d00271: lda #0
        jsr animsp
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00309
:
        lda #1
        sta deadf
d00309: rts
.out .sprintf("EVENT 3 SIZE = %d", (* - evnt03))

evnt04:
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp e00034
:
        jmp e00034
e00034: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e00199
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
        jmp e00145
:
        lda #15
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp e00194
e00145: lda #16
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
e00194: jmp e00257
e00199: jsr cangd
        beq :+
        jmp e00243
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
        jmp e00257
e00243: lda #0
        ldy #10
        sta (z80_ix),y
e00257: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00486
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
        jmp e00486
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
        jmp e00468
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
e00468: lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
e00486: lda #0
        jsr animsp
        lda #9
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00517
e00517: rts
.out .sprintf("EVENT 4 SIZE = %d", (* - evnt04))

evnt05:
        lda #0
        jsr animsp
        lda #5
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f00062
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00062
:
        lda #1
        sta deadf
f00062: lda #8
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f00216
        lda #26
        cmp scno
        beq *+5
        jmp f00153
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00148
:
        lda #25
        asl a
        sta sndtyp
        lda #28
        sta scno
        jsr nwscr
f00148: jmp f00216
f00153: lda #28
        cmp scno
        beq *+5
        jmp f00216
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00216
:
        lda #25
        asl a
        sta sndtyp
        lda #26
        sta scno
        jsr nwscr
f00216: rts
.out .sprintf("EVENT 5 SIZE = %d", (* - evnt05))

evnt06:
        jsr cangd
        beq :+
        jmp g00050
:
        lda #0
        jsr animsp
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp g00194
g00050: lda #255
        ldy #5
        sta (z80_ix),y
        lda #40
        asl a
        sta sndtyp
        lda #150
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        ldy #9
        sta (z80_ix),y
        lda #40
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #20
        dey; ldy #8
        sta (z80_ix),y
        lda #6
        sta z80_c
        lda #12
        sta z80_b
        jsr spawn
g00194: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00224
:
        lda #1
        sta deadf
g00224: rts
.out .sprintf("EVENT 6 SIZE = %d", (* - evnt06))

evnt07:
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp h00075
:
        lda #9
        jsr gotob
        cmp #255
        beq :+
        jmp h00075
:
        lda #100
        asl a
        sta sndtyp
        lda #100
        jsr delay
        lda #1
        sta gamwon
h00075: rts
.out .sprintf("EVENT 7 SIZE = %d", (* - evnt07))

evnt08:
        lda #11
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp varb
        beq *+5
        jmp i00041
        ; lda #0
        jsr animsp
i00041: lda #70
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        ldy #11
        sta (z80_ix),y
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i00122
:
        lda #255
        ldy #5
        sta (z80_ix),y
i00122: rts
.out .sprintf("EVENT 8 SIZE = %d", (* - evnt08))

evnt09:
        rts
.out .sprintf("EVENT 9 SIZE = %d", (* - evnt09))

evnt10:
        rts
.out .sprintf("EVENT 10 SIZE = %d", (* - evnt10))

evnt11:
        lda #0
        cmp scno
        beq *+5
        jmp l00043
        lda #2
        sta chary
        lda #7
        sta charx
        lda #16
        jsr dmsg
l00043: lda #27
        cmp scno
        beq *+5
        jmp l00085
        lda #5
        sta chary
        lda #6
        sta charx
        ; lda #6
        jsr dmsg
l00085: rts
.out .sprintf("EVENT 11 SIZE = %d", (* - evnt11))

evnt12:
        jsr cls
        lda #1
        sta chary
        lda #4
        sta charx
        lda #8
        jsr dmsg
        lda #2
        sta chary
        lda #5
        sta charx
        lda #15
        jsr dmsg
        lda #5
        sta chary
        lda #0
        sta charx
        lda #13
        jsr dmsg
        lda #23
        sta chary
        lda #3
        sta charx
        lda #10
        jsr dmsg
        jsr prskey
        jsr cls
        lda #1
        sta chary
        lda #4
        sta charx
        lda #8
        jsr dmsg
        lda #2
        sta chary
        lda #5
        sta charx
        lda #15
        jsr dmsg
        lda #5
        sta chary
        lda #0
        sta charx
        lda #9
        jsr dmsg
        lda #15
        sta chary
        lda #0
        sta charx
        lda #27
        jsr dmsg
        lda #23
        sta chary
        lda #4
        sta charx
        lda #28
        jsr dmsg
        jsr prskey
        lda joyval
        and #64
        beq :+
        jmp m00272
:
        lda #8
        sta varl
        jmp m00280
m00272: lda #5
        sta varl
m00280: lda #23
        sta chary
        lda #0
        sta charx
        lda #14
        jsr dmsg
        jsr prskey
        rts
.out .sprintf("EVENT 12 SIZE = %d", (* - evnt12))

evnt13:
        lda varl
        sta numlif
        jsr cls
        lda #0
        sta vara
        ; lda #0
        sta varb
        ; lda #0
        sta varc
        ; lda #0
        sta varo
        ; lda #0
        sta vart
        lda #21
        sta chary
        lda #1
        sta charx
        ; lda #1
        jsr dmsg
        lda #21
        sta chary
        lda #7
        sta charx
        lda numlif
        jsr disply
        lda #21
        sta chary
        lda #19
        sta charx
        lda #2
        jsr dmsg
        lda #21
        sta chary
        lda #25
        sta charx
        jsr dscor
        lda #21
        sta chary
        lda #9
        sta charx
        lda #4
        jsr dmsg
        lda #21
        sta chary
        lda #16
        sta charx
        lda varo
        jsr disply
        lda #23
        sta chary
        lda #9
        sta charx
        lda #7
        jsr dmsg
        lda #23
        sta chary
        lda #18
        sta charx
        lda vart
        jsr disply
        rts
.out .sprintf("EVENT 13 SIZE = %d", (* - evnt13))

evnt14:
        lda #21
        sta chary
        lda #1
        sta charx
        ; lda #1
        jsr dmsg
        lda #21
        sta chary
        lda #7
        sta charx
        lda numlif
        jsr disply
        lda #21
        sta chary
        lda #9
        sta charx
        lda #4
        jsr dmsg
        lda #21
        sta chary
        lda #16
        sta charx
        lda varo
        jsr disply
        lda #21
        sta chary
        lda #19
        sta charx
        lda #2
        jsr dmsg
        lda #21
        sta chary
        lda #25
        sta charx
        jsr dscor
        lda #23
        sta chary
        lda #9
        sta charx
        lda #7
        jsr dmsg
        lda #23
        sta chary
        lda #18
        sta charx
        lda vart
        jsr disply
        rts
.out .sprintf("EVENT 14 SIZE = %d", (* - evnt14))

evnt15:
        rts
.out .sprintf("EVENT 15 SIZE = %d", (* - evnt15))

evnt16:
        lda #50
        jsr delay
        lda numlif
        sec
        sbc #1
        sta numlif
        lda #21
        sta chary
        lda #7
        sta charx
        lda numlif
        jsr disply
        lda #0
        cmp numlif
        beq *+5
        jmp q00103
        lda #10
        sta chary
        lda #8
        sta charx
        lda #0
        jsr dmsg
        lda #90
        jsr delay
q00103: rts
.out .sprintf("EVENT 16 SIZE = %d", (* - evnt16))

evnt17:
        jsr cls
        lda #10
        sta chary
        lda #8
        sta charx
        lda #0
        jsr dmsg
        lda #12
        sta chary
        lda #10
        sta charx
        lda #3
        jsr dmsg
        lda #13
        sta chary
        ; lda #13
        sta charx
        jsr dscor
        lda #23
        sta chary
        lda #3
        sta charx
        lda #10
        jsr dmsg
        jsr prskey
        rts
.out .sprintf("EVENT 17 SIZE = %d", (* - evnt17))

evnt18:
        jsr cls
        lda #8
        sta chary
        ; lda #8
        sta charx
        lda #5
        jsr dmsg
        lda #12
        sta chary
        lda #10
        sta charx
        lda #3
        jsr dmsg
        lda #13
        sta chary
        ; lda #13
        sta charx
        jsr dscor
        lda #23
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
        .byte "END OF THE GAME!",141
        .byte "LIVES:",141
        .byte "SCORE:",141
        .byte "FINAL SCORE:",141
        .byte "RELICS:",141
        .byte "CONGRATULATIONS!",13
        .byte "    YOU",39
        .byte "VE FINISHED THE GAME",13
        .byte "          WITH SUCCESS!",141
        .byte "............",13
        .byte "      .GOLD COINS.",13
        .byte "      ............",13
        .byte "      .GEMSTONES .",13
        .byte "      ............",13
        .byte "      .WAY OUT!!!.",13
        .byte "      ............",141
        .byte "TREASURE:",141
        .byte "THE TREASURE OF LUMOS ",141
        .byte "USE THE KEYBOARD TO CONTROL JACK",13
        .byte 13
        .byte "          Q - UP",13
        .byte "          A - DOWN",13
        .byte "          O - LEFT",13
        .byte "          P - RIGHT",13
        .byte "          M - JUMP",13
        .byte 13
        .byte "         GOOD LUCK!!!",141
        .byte "PRESS ANY KEY TO CONTINUE!",141
        .byte "     CONGRATULATIONS!",13
        .byte "         YOU HAVE FOUND",13
        .byte "     THE TREASURE OF LUMOS!",141
        .byte "YOU",39
        .byte "VE EARNED AN EXTRA LIVE!",141
        .byte " Become JACK SCAVENGER, the new",13
        .byte " treasure hunter in the market.",13
        .byte 13
        .byte " Your mission is to explore the",13
        .byte " Cave of Lune Mountain and find",13
        .byte " the TREASURE OF LUMOS!",13
        .byte 13
        .byte " In the cave there",39
        .byte "s 45 relics.",13
        .byte " Pick them to earn points.",13
        .byte 13
        .byte " Also you have to find keys to",13
        .byte " open doors.",13
        .byte 13
        .byte " But the task is not easy...",13
        .byte " In your way you will find many",13
        .byte " cave hazards that kills you.",13
        .byte 141
        .byte "PRESS ANY KEY TO BEGIN THE GAME!",13
        .byte 141
        .byte "By Jaime Grilo (2017)",141
        .byte "THE TREASURE OF",141
        .byte "YOU NEED KEY #2 TO OPEN DOOR!",141
        .byte "YOU",39
        .byte "VE FOUND THE KEY #1!",141
        .byte "YOU NEED KEY #1 TO OPEN DOOR!",141
        .byte "YOU",39
        .byte "VE FOUND THE KEY #2!",141
        .byte "YOU NEED KEY #3 TO OPEN DOOR!",141
        .byte "YOU",39
        .byte "VE FOUND THE KEY#3!",141
        .byte "DOOR LOCKED!",13
        .byte " YOU NEED THE MAIN DOOR KEY!",141
        .byte "YOU",39
        .byte "VE FOUND THE MAIN DOOR KEY",141
        .byte "YOU NEED KEY #4 TO OPEN DOOR!",141
        .byte "YOU FOUND THE KEY #4!",141
        .byte "CREDITS:",13
        .byte "Game made by Jaime Grilo",13
        .byte "Using AGD by Jonathan Cauldwell",13
        .byte "Music by Riskej",13
        .byte "Additional code by David Saphier",13
        .byte 141
        .byte "BEGIN WITH 5 OR 8 LIVES?",141
nummsg:
        .byte 29
.out .sprintf("MESSAGE SIZE = %d", (* - msgdat))

chgfx:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,208,240,208,176,208,176,0,0,208,176,240,176,240,176
        .byte 0,0,255,255,255,255,0,0,255,255,85,85,170,170,85,85
        .byte 0,0,175,175,25,25,141,141,95,95,0,0,0,0,0,0
        .byte 244,241,248,250,241,245,250,248,240,245,242,250,245,240,240,240
        .byte 4,128,64,8,129,0,24,0,4,128,64,8,32,4,64,8
        .byte 0,0,10,10,5,5,80,80,160,160,10,10,5,5,10,10
        .byte 161,165,88,88,165,165,66,88,133,161,90,90,36,36,90,26
        .byte 15,31,15,175,15,79,143,15,95,15,47,15,15,31,15,175
        .byte 15,79,143,175,15,15,47,15,95,31,15,15,15,79,143,175
        .byte 196,0,200,0,196,0,136,0,213,85,136,0,196,0,200,0
        .byte 0,49,0,50,0,49,0,16,160,177,0,16,0,49,0,50
        .byte 0,0,0,0,85,85,80,80,160,160,170,170,0,0,0,0
        .byte 153,34,0,0,0,34,17,17,136,0,34,0,0,136,0,0
        .byte 0,0,255,255,0,0,0,0,0,0,238,238,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 95,95,143,15,95,95,143,47,15,95,143,143,31,31,175,175
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16
        .byte 17,0,0,0,0,0,0,0,0,0,0,0,0,0,136,34
        .byte 4,129,64,16,32,0,2,0,4,1,64,16,32,0,2,0
        .byte 129,32,8,2,0,4,0,64,128,32,8,2,0,4,0,64
        .byte 4,129,64,18,32,4,2,2,4,1,64,16,32,0,2,0
        .byte 129,32,72,2,32,4,64,64,128,32,8,2,0,4,0,64
        .byte 165,165,90,90,165,165,90,90,165,165,90,90,165,165,90,90
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 0,0,208,240,208,176,208,176,64,0,80,176,48,176,16,176
        .byte 0,0,208,240,208,176,208,176,0,32,208,224,240,192,240,128
        .byte 0,0,51,255,119,255,0,0,255,255,85,85,170,170,85,85
        .byte 0,0,255,204,255,238,0,0,255,255,85,85,170,170,85,85
        .byte 161,0,88,72,165,164,66,74,133,161,90,90,36,36,90,26
        .byte 0,0,13,15,13,11,13,11,0,0,13,11,15,11,15,11
        .byte 0,0,175,175,95,95,0,0,95,95,5,5,10,10,5,5
        .byte 0,0,13,15,13,11,13,11,4,0,5,11,3,11,1,11
        .byte 0,0,13,15,13,11,13,11,0,2,13,14,15,12,15,8
        .byte 0,0,35,175,87,95,0,0,95,95,5,5,10,10,5,5
        .byte 0,0,175,140,95,78,0,0,95,95,5,5,10,10,5,5
        .byte 76,0,140,0,76,0,136,0,93,85,136,0,76,0,140,0
        .byte 0,19,0,35,0,19,0,1,10,27,0,1,0,19,0,35
        .byte 0,0,90,90,37,164,64,2,165,165,10,80,160,5,88,26
        .byte 112,0,208,240,240,176,208,176,0,0,208,176,240,240,240,176
        .byte 0,0,0,170,17,245,50,0,100,49,200,64,128,145,152,34
        .byte 145,68,152,34,128,145,200,64,100,49,50,0,17,245,0,80
        .byte 0,0,170,170,245,245,0,0,128,0,64,0,32,0,152,0
        .byte 85,0,152,0,32,0,64,0,128,0,0,0,245,245,80,80
        .byte 0,0,170,170,245,245,0,0,0,49,0,64,0,145,16,34
        .byte 17,68,16,34,0,145,0,64,0,49,0,0,245,245,80,80
        .byte 0,0,170,170,245,245,0,0,128,0,64,0,32,0,152,0
        .byte 85,0,152,0,32,0,64,0,128,0,0,0,245,245,80,80
        .byte 0,0,80,80,245,245,0,0,0,17,0,34,0,68,0,152
        .byte 0,160,0,152,0,68,0,34,0,17,0,0,245,245,170,170
        .byte 0,0,80,0,245,128,0,200,196,100,34,50,145,17,64,152
        .byte 32,145,64,152,145,17,34,50,196,100,0,200,245,128,170,0
        .byte 7,0,13,15,15,11,13,11,0,0,13,11,15,15,15,11
        .byte 25,2,0,0,0,2,1,1,8,0,34,0,0,8,0,0
        .byte 112,0,208,240,240,176,208,176,0,0,208,160,240,192,240,128
        .byte 112,0,208,240,240,176,208,176,0,0,80,176,48,240,16,176
bprop:
        .byte 0
        .byte 2
        .byte 2
        .byte 1
        .byte 5
        .byte 0
        .byte 4
        .byte 2
        .byte 5
        .byte 7
        .byte 3
        .byte 3
        .byte 3
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 6
        .byte 6
        .byte 3
        .byte 3
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
        .byte 3
        .byte 3
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
        .byte 0
        .byte 0
        .byte 0
        .byte 0
.out .sprintf("BLOCKS SIZE = %d", (* - chgfx))

sprgfx:
        .byte 3,0,1,192,2,192,1,192,0,0,1,64,3,224,5,208,25,200,1,200,0,0,1,192,3,192,2,120,2,8,6,0
        .byte 3,0,1,192,2,192,1,192,0,0,1,64,1,192,1,64,1,64,1,64,0,128,1,192,1,192,1,64,1,64,3,192
        .byte 0,192,3,128,3,64,3,128,0,0,2,128,7,192,11,160,19,152,19,128,0,0,3,128,3,192,30,64,16,64,0,96
        .byte 0,192,3,128,3,64,3,128,0,0,2,128,3,128,2,128,2,128,2,128,1,0,3,128,3,128,2,128,2,128,3,192
        .byte 1,128,3,192,19,192,17,128,8,8,7,232,3,216,3,192,3,192,3,192,0,0,3,240,3,208,2,24,2,0,6,0
        .byte 1,128,3,192,3,192,1,128,16,8,23,232,27,216,3,192,3,192,3,192,0,0,3,192,3,192,2,64,2,64,6,96
        .byte 1,128,3,192,3,200,1,136,16,16,23,224,27,192,3,192,3,192,3,192,0,0,15,192,11,192,24,64,0,64,0,96
        .byte 1,128,3,192,3,192,1,128,16,8,23,232,27,216,3,192,3,192,3,192,0,0,3,192,3,192,2,64,2,64,6,96
        .byte 15,240,17,128,19,192,33,128,33,128,71,224,79,240,249,144,249,144,79,240,71,224,33,128,33,128,19,192,17,128,15,240
        .byte 15,240,1,128,3,192,1,128,1,128,7,224,15,240,9,144,9,144,15,240,7,224,1,128,1,128,3,192,1,128,15,240
        .byte 15,240,1,136,3,200,1,132,1,132,7,226,15,242,9,159,9,159,15,242,7,226,1,132,1,132,3,200,1,136,15,240
        .byte 15,240,1,128,3,192,1,128,1,128,7,224,15,240,9,144,9,144,15,240,7,224,1,128,1,128,3,192,1,128,15,240
        .byte 3,192,14,112,8,16,122,94,144,9,185,157,200,19,203,211,200,19,206,115,7,224,6,96,6,96,6,96,30,120,30,120
        .byte 3,192,14,112,8,16,122,94,144,9,185,157,200,19,201,147,104,22,110,118,15,240,14,112,12,48,12,48,60,60,60,60
        .byte 3,192,14,112,8,16,122,94,144,9,185,157,200,19,248,31,56,28,14,112,15,240,14,112,24,24,24,24,120,30,120,30
        .byte 3,192,14,112,8,16,122,94,144,9,185,157,200,19,201,147,104,22,110,118,15,240,14,112,12,48,12,48,60,60,60,60
        .byte 0,0,0,0,0,0,2,73,2,73,4,146,4,146,9,36,9,36,18,72,146,73,255,255,146,73,255,255,146,73,255,255
        .byte 0,0,0,0,0,0,18,72,18,72,18,72,18,72,18,72,18,72,18,72,146,73,255,255,146,73,255,255,146,73,255,255
        .byte 0,0,0,0,0,0,146,64,146,64,73,32,73,32,36,144,36,144,18,72,146,73,255,255,146,73,255,255,146,73,255,255
        .byte 0,0,0,0,0,0,18,72,18,72,18,72,18,72,18,72,18,72,18,72,146,73,255,255,146,73,255,255,146,73,255,255
        .byte 7,192,24,48,32,8,64,68,64,36,128,18,128,18,128,2,144,2,144,2,72,4,68,4,32,8,24,48,7,192,0,0
        .byte 7,192,24,48,32,8,76,4,80,4,144,2,128,2,128,2,128,2,128,18,64,20,64,100,32,8,24,48,7,192,0,0
        .byte 0,0,0,0,0,0,0,0,112,28,216,54,15,224,5,64,7,192,8,32,11,160,7,192,8,32,16,16,16,16,0,0
        .byte 0,0,0,0,0,0,64,4,112,28,24,48,15,224,5,64,7,192,8,32,8,32,7,192,8,32,16,16,32,8,0,0
        .byte 0,0,0,0,0,0,0,0,112,28,216,54,15,224,5,64,7,192,8,32,11,160,7,192,8,32,16,16,16,16,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,56,56,111,236,69,68,71,196,12,96,14,224,7,192,8,32,8,32,8,32,0,0
        .byte 255,255,159,249,8,16,128,1,64,2,66,18,128,1,0,128,130,1,66,18,8,16,64,2,128,1,8,16,159,249,255,255
        .byte 255,255,159,249,8,16,128,1,72,2,64,66,128,65,0,16,128,1,72,2,0,64,64,2,130,1,8,16,159,249,255,255
        .byte 255,255,159,249,8,16,130,1,66,2,64,18,128,17,0,0,128,1,64,18,0,0,72,66,130,1,8,16,159,249,255,255
        .byte 255,255,159,249,8,16,130,33,64,2,64,2,128,1,8,0,136,1,64,66,8,0,64,18,128,129,8,16,159,249,255,255
        .byte 251,223,132,33,161,9,144,133,132,33,107,214,84,42,77,178,69,162,36,33,35,193,36,33,11,208,28,56,15,240,0,0
        .byte 251,223,132,33,144,133,161,9,132,33,107,214,84,42,77,178,69,162,68,34,67,194,68,34,11,208,28,56,15,240,0,0
        .byte 251,223,132,33,161,9,144,133,132,33,107,214,84,42,77,178,69,162,132,36,131,196,132,36,11,208,28,56,15,240,0,0
        .byte 251,223,132,33,144,133,161,9,132,33,107,214,84,42,77,178,69,162,68,34,67,194,68,34,11,208,28,56,15,240,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,80,2,160,7,240,15,216,27,236,6,240,25,12,17,4,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,80,2,160,7,240,15,216,27,236,23,116,8,136,8,136,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,80,2,160,7,240,15,216,27,236,23,180,8,72,4,80,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,80,2,160,7,240,15,216,27,236,23,116,8,136,8,136,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 8,64,0,66,8,224,71,18,72,12,18,2,34,4,33,2,64,132,32,8,16,8,16,16,11,32,4,192,0,0,0,0
        .byte 5,8,36,4,0,224,39,18,8,12,17,2,34,4,34,2,65,4,32,8,16,8,16,16,11,32,4,192,0,0,0,0
        .byte 16,8,40,20,68,34,254,127,170,85,40,20,56,28,40,20,40,20,56,28,40,20,170,85,254,127,68,34,40,20,16,8
        .byte 0,119,0,139,0,114,0,138,0,115,0,139,0,119,0,139,0,112,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 154,0,253,255,1,31,253,255,254,255,193,127,128,191,42,94,62,32,235,191,85,32,227,191,85,0,235,128,62,0,42,0
        .byte 255,244,128,4,191,244,160,8,111,248,120,28,224,12,229,76,199,192,93,112,202,160,28,112,10,160,29,112,7,192,5,64
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,148,0,235,248
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,8,52,12,106,230,91,251,173
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,0,5,0,7,0,3
        .byte 0,0,170,170,255,255,128,1,131,129,132,65,136,33,144,17,148,17,146,17,137,33,132,65,131,129,128,1,255,255,85,85
        .byte 0,0,85,85,255,255,128,1,131,129,132,65,137,33,146,17,148,17,144,17,136,33,132,65,131,129,128,1,255,255,170,170
        .byte 0,0,170,170,255,255,128,1,131,129,132,65,137,33,144,145,144,81,144,17,136,33,132,65,131,129,128,1,255,255,85,85
        .byte 0,0,85,85,255,255,128,1,131,129,132,65,136,33,144,17,144,81,144,145,137,33,132,65,131,129,128,1,255,255,170,170
frmlst:
        .byte 0,2
        .byte 2,2
        .byte 4,4
        .byte 8,4
        .byte 12,4
        .byte 16,4
        .byte 20,2
        .byte 22,4
        .byte 26,4
        .byte 30,4
        .byte 34,4
        .byte 38,1
        .byte 39,2
        .byte 41,1
        .byte 42,1
        .byte 43,1
        .byte 44,1
        .byte 45,1
        .byte 46,1
        .byte 47,1
        .byte 48,4,52,0
.out .sprintf("SPRITES SIZE = %d", (* - sprgfx))

scdat:
        .word 254,185,169,255,261,241,293,234,246,184,281,173,159,240,230,236,288,197,250,172,441,371,144,151,87,163
        .word 222,153,251
        .byte 255,0,27,5,7,7,255,0,27,5,7,7,255,0,27,5,7,7,0,23,255,0,4,23,0,0,23,0,23,0,0,0,23
        .byte 0,255,23,4,0,255,23,4,0,5,7,7,0,23,255,0,4,23,0,0,23,0,23,23,0,23,23,0,23,0,0,23,0
        .byte 23,255,0,4,5,7,7,0,23,255,0,4,23,0,0,23,0,23,0,23,0,23,0,23,0,0,23,0,255,23,4,0,5
        .byte 7,7,0,23,255,0,4,23,0,0,23,0,23,0,0,0,23,0,23,0,0,23,255,0,4,23,0,5,7,7,0,255,23,4
        .byte 0,255,23,4,0,23,0,0,0,23,0,255,23,4,0,255,23,4,0,5,7,7,255,0,27,5,7,7,255,0,27,5,7,7
        .byte 255,0,27,5,7,7,255,0,27,5,7,7,255,15,6,255,0,21,5,7,7,255,15,6,255,0,21,5,7,7,255,15,6
        .byte 255,0,21,5,5,5,255,15,6,255,0,9,27,28,0,0,0,27,28,255,0,5,5,5,5,255,14,6,255,2,7,6,6
        .byte 1,1,9,9,9,1,1,255,6,6,2,2,255,7,16,255,1,5,255,7,39
        .byte 255,1,62,26,13,255,0,7,13,25,1,1,26,13,255,0,9,13,25,255,1,4,13,255,0,9,13,1,1,13,255,0,11
        .byte 13,255,1,4,255,0,11,25,26,255,0,13,255,1,4,255,0,26,255,1,4,255,0,26,255,1,4,255,0,26,255,1,4
        .byte 255,0,5,27,2,2,28,255,0,6,27,2,2,28,255,0,7,255,1,4,255,0,4,3,25,1,1,26,3,255,0,4,3
        .byte 25,1,1,26,3,255,0,5,3,255,1,4,255,0,6,1,1,255,0,8,1,1,255,0,7,3,25,1,1,1,3,3,255,0,4
        .byte 25,26,255,0,8,25,26,255,0,9,5,1,1,255,0,27,5,1,26,255,3,4,255,0,20,27,2,2,2,22,255,0,23
        .byte 27,2,255,1,4,22,255,0,23,255,1,6,255,2,24,255,1,66
        .byte 255,1,61,13,1,13,0,0,0,13,0,0,0,13,0,0,13,13,0,0,13,0,0,0,13,0,0,0,13,1,13,1,1
        .byte 255,0,28,1,1,255,0,28,1,1,255,0,28,1,1,255,0,28,5,1,255,0,28,5,1,255,0,26,27,2,2,1,255,0,4
        .byte 27,255,2,13,28,255,0,7,25,1,1,1,3,3,0,0,255,1,14,26,3,3,3,255,0,5,13,1,5,255,0,4,1,1
        .byte 13,255,0,11,13,255,0,9,1,5,255,0,4,1,1,255,0,22,1,255,2,5,1,1,255,0,18,27,255,2,4,255,1,6
        .byte 26,255,0,18,255,1,10,255,0,17,27,2,2,255,1,10,255,0,17,255,1,13,255,2,12,0,0,2,2,2,255,1,38
        .byte 255,1,31,13,1,13,1,13,1,13,1,13,1,13,1,13,25,1,1,26,13,1,13,1,13,1,13,1,13,1,13,1,1
        .byte 255,0,28,1,1,255,0,28,1,1,255,0,28,1,26,255,0,15,4,255,0,10,3,3,1,5,255,0,12,27,2,2,1
        .byte 255,0,12,1,5,255,0,12,255,1,4,255,0,12,1,2,2,2,28,0,0,27,2,2,10,11,2,2,1,1,1,26,255,0,10
        .byte 3,3,255,1,5,0,0,1,1,1,10,11,255,1,4,13,255,0,13,255,1,4,26,0,0,1,1,1,10,11,1,1,1
        .byte 26,255,0,14,1,1,255,0,5,1,1,13,10,11,13,0,0,13,255,0,11,3,3,3,1,1,255,0,5,1,1,0,10
        .byte 11,255,0,18,1,1,4,4,4,1,1,1,26,0,10,11,255,0,18,255,1,7,26,13,0,10,11,255,0,12,27,28,255,0,4
        .byte 255,1,6,255,0,4,10,11,255,0,12,1,1,255,0,4,5,255,1,5,255,0,4,10,11,255,0,12,1,1,255,0,4
        .byte 5,255,1,5,255,2,11,4,4,255,2,5,1,1,255,2,5,255,1,30
        .byte 255,1,31,13,1,13,255,0,17,1,255,0,4,13,1,13,1,1,255,0,20,1,255,0,7,1,1,1,255,12,18,1,26
        .byte 255,0,6,27,1,1,255,0,27,1,1,1,255,0,28,5,1,255,0,6,255,17,11,255,0,11,5,1,2,10,11,2,2
        .byte 28,40,42,44,46,44,46,44,46,44,46,48,50,27,2,2,28,0,0,0,10,11,2,2,1,1,10,11,1,1,1,41,43
        .byte 45,47,45,47,45,47,45,47,49,51,1,1,1,26,0,0,0,10,11,255,1,4,10,11,255,1,18,255,0,4,10,11,255,1,4
        .byte 10,11,1,1,1,13,255,1,10,13,1,1,1,255,0,4,10,11,255,1,4,10,11,1,1,1,0,4,255,0,8,4,0
        .byte 25,1,26,255,0,4,10,11,255,1,4,10,11,1,1,1,255,0,19,10,11,255,1,4,10,11,1,1,1,255,0,19,10
        .byte 11,255,1,4,10,11,1,1,1,255,0,19,10,11,25,1,5,0,10,11,1,1,1,255,0,19,10,11,5,39,5,0,10
        .byte 11,1,1,1,255,0,19,10,11,5,39,255,2,4,1,1,1,255,2,14,4,4,255,2,7,255,1,30
        .byte 255,1,32,13,255,0,4,13,13,255,0,4,13,1,13,255,0,5,13,255,0,5,13,255,1,4,255,0,12,1,255,0,13
        .byte 255,1,4,255,0,12,1,255,0,13,1,1,1,26,255,0,12,1,255,0,13,25,1,5,255,0,13,1,255,0,14,5,5
        .byte 255,0,13,1,2,28,255,0,12,5,255,2,5,28,255,0,8,1,1,26,255,0,10,27,2,2,255,1,6,255,0,8,1
        .byte 255,0,12,25,255,1,8,0,0,4,255,0,5,1,255,0,13,255,1,4,26,25,1,1,0,0,1,2,2,2,10,11,1
        .byte 255,2,5,28,255,0,7,255,1,4,0,0,1,1,0,0,25,1,1,1,10,11,255,1,6,26,255,0,7,255,1,4,0,0
        .byte 1,1,255,0,4,13,0,10,11,0,13,255,0,12,255,1,4,0,0,1,1,255,0,6,10,11,255,0,14,255,1,4,10
        .byte 11,1,26,255,0,6,10,11,255,0,14,1,1,39,5,10,11,255,0,24,1,1,39,5,10,11,255,0,24,1,1,255,2,8
        .byte 4,4,255,2,18,255,1,32
        .byte 255,1,32,13,255,0,12,13,1,13,255,0,10,13,1,1,255,0,15,1,255,0,12,1,1,255,0,15,1,255,0,12,1,1
        .byte 255,0,15,1,10,11,255,2,5,28,255,0,4,1,5,255,0,15,1,10,11,255,1,6,255,0,4,1,5,0,0,0,27
        .byte 255,2,9,10,11,1,10,11,0,0,5,5,1,1,255,0,4,1,2,2,3,3,25,255,1,9,10,11,1,10,11,255,0,4
        .byte 1,1,255,0,4,1,1,1,255,0,12,10,11,1,10,11,255,0,4,1,1,255,0,4,5,1,1,255,0,12,10,11,1
        .byte 10,11,255,0,4,1,1,255,0,4,5,1,1,255,0,12,2,2,1,255,2,4,10,11,1,1,0,0,0,27,2,1,1
        .byte 255,0,12,255,1,6,26,10,11,1,1,0,0,0,25,1,1,1,255,0,12,1,26,255,0,5,10,11,1,1,255,0,4
        .byte 1,1,1,10,11,2,2,28,255,0,4,27,2,2,1,255,0,6,10,11,1,1,255,0,4,1,1,1,10,11,1,1,26
        .byte 255,0,4,25,1,1,26,255,0,8,1,1,255,0,4,1,1,1,10,11,255,0,19,1,1,255,0,4,1,1,1,10,11
        .byte 255,0,19,1,1,255,0,4,1,1,1,255,2,16,4,4,2,2,2,1,1,255,0,4,255,1,31
        .byte 255,1,31,13,0,5,0,0,13,0,0,13,0,0,13,0,5,5,0,13,0,0,13,0,0,13,0,0,5,0,13,1,1
        .byte 255,0,28,1,1,255,0,28,1,1,255,0,28,1,1,255,0,28,1,1,2,2,28,255,0,4,27,28,255,0,4,27,28
        .byte 255,0,4,27,28,0,0,27,255,2,4,255,1,4,26,255,0,4,25,26,255,0,4,25,26,255,0,4,25,26,0,0,25
        .byte 255,1,5,5,255,0,27,13,1,5,255,0,28,1,255,2,6,10,11,2,28,255,0,19,255,1,7,10,11,1,1,2,28
        .byte 255,0,15,3,3,1,1,13,255,0,4,10,11,0,0,25,1,2,28,255,0,15,1,1,255,0,5,10,11,255,0,4,25
        .byte 1,2,28,255,0,11,3,3,1,1,255,0,5,10,11,255,0,6,25,26,255,0,4,27,28,255,0,7,1,1,255,0,19
        .byte 1,1,255,0,7,5,1,255,0,19,1,1,2,2,28,255,0,4,5,1,255,2,15,28,0,0,27,255,1,5,255,2,5
        .byte 255,1,30
        .byte 255,1,31,26,0,13,0,13,0,13,0,13,0,13,0,13,0,13,0,13,0,13,0,13,0,13,0,13,0,13,1,1,1
        .byte 255,0,26,21,22,1,1,255,0,26,21,22,1,1,255,0,26,19,20,1,1,255,0,26,19,20,1,1,10,11,28,0,0
        .byte 27,28,0,0,27,28,0,0,27,28,0,0,27,28,0,0,27,28,0,0,27,19,20,1,1,10,11,26,0,0,25,26,0,0
        .byte 25,26,0,0,25,26,0,0,25,26,0,0,25,26,0,0,25,19,20,1,1,10,11,255,0,24,19,20,1,1,10,11,255,0,24
        .byte 19,20,1,1,255,2,5,10,11,255,0,19,19,20,255,1,7,10,11,255,0,19,19,20,1,1,13,0,13,0,13,10,11
        .byte 255,0,19,19,20,1,1,255,0,5,10,11,255,0,19,19,20,1,1,255,0,5,10,11,255,0,19,19,20,1,5,255,0,28
        .byte 5,5,255,0,13,4,4,0,0,4,4,0,0,27,2,2,28,0,0,0,5,255,2,11,28,0,0,1,1,0,0,1,1
        .byte 0,0,255,1,4,255,2,4,255,1,30
        .byte 255,1,31,255,16,28,1,1,255,0,28,1,1,255,0,28,1,1,255,0,26,4,4,1,1,255,0,28,1,1,2,2,28
        .byte 255,0,25,255,1,4,26,255,0,23,3,3,1,1,255,0,28,1,1,255,0,28,1,1,2,2,2,10,11,2,28,255,0,19
        .byte 3,3,255,1,5,10,11,1,26,0,0,27,28,255,0,17,1,1,13,0,13,10,11,255,0,4,25,26,0,0,27,28,255,0,11
        .byte 10,11,1,1,0,0,0,10,11,255,0,8,25,26,0,0,27,28,255,0,7,10,11,1,1,0,0,0,10,11,255,0,12
        .byte 25,26,0,0,27,28,0,0,0,10,11,1,5,0,0,0,10,11,255,0,16,25,26,255,0,5,5,5,0,0,0,10,11
        .byte 255,0,23,5,255,2,10,255,4,12,255,2,8,255,1,30
        .byte 255,1,31,26,255,0,26,25,1,1,255,0,28,1,1,255,0,28,1,1,255,0,28,5,1,255,0,11,27,2,2,28,255,0,13
        .byte 5,1,10,11,2,2,28,255,0,4,27,2,255,1,4,2,2,10,11,255,0,5,10,11,2,2,2,1,10,11,1,1,26
        .byte 255,0,4,25,1,1,8,8,1,1,1,10,11,255,0,5,10,11,255,1,4,10,11,255,0,9,25,8,8,26,255,0,9
        .byte 10,11,13,13,1,1,10,11,255,0,10,8,8,255,0,10,10,11,0,0,1,1,2,2,2,10,11,255,0,7,8,8,255,0,10
        .byte 10,11,0,0,255,1,5,10,11,255,0,7,8,8,0,27,2,10,11,2,28,255,0,7,1,1,13,13,13,10,11,255,0,7
        .byte 8,8,0,25,1,10,11,1,26,255,0,7,1,1,0,0,0,10,11,255,0,7,8,8,0,0,0,10,11,255,0,9,1,1
        .byte 0,0,0,10,11,255,0,7,8,8,0,0,0,10,11,255,0,9,1,5,255,0,12,8,8,0,0,0,10,11,255,0,9
        .byte 1,5,255,0,12,8,8,0,0,0,10,11,255,0,7,3,3,1,255,2,13,8,8,2,2,2,10,11,255,2,10,255,1,13
        .byte 8,8,1,1,1,10,11,255,1,10
        .byte 255,1,31,255,16,28,1,1,255,0,28,1,1,255,0,28,1,5,255,0,28,1,5,255,0,28,1,2,2,2,10,11,255,0,24
        .byte 255,1,4,10,11,255,0,22,3,3,1,1,0,0,10,11,255,0,24,1,1,0,0,10,11,255,0,24,5,1,2,2,10
        .byte 11,255,0,24,5,1,1,1,10,11,255,0,21,27,2,2,2,1,13,0,10,11,255,0,17,27,28,0,0,25,255,1,4
        .byte 0,0,10,11,255,0,13,27,28,0,0,25,26,255,0,4,13,1,1,0,0,10,11,255,0,9,27,28,0,0,25,26,255,0,9
        .byte 1,1,255,0,13,25,26,255,0,13,1,1,255,0,6,27,2,2,2,28,255,0,17,1,1,255,2,6,255,1,5,255,4,17
        .byte 255,1,31
        .byte 255,1,31,255,16,28,1,1,255,0,28,1,1,255,0,28,1,1,255,0,28,1,1,255,0,28,5,1,255,0,28,5,1
        .byte 255,0,25,10,11,2,2,1,255,0,25,10,11,1,1,5,255,0,25,10,11,1,8,5,255,0,12,27,255,2,4,28,255,0,7
        .byte 10,11,1,8,255,2,8,28,255,0,4,25,255,1,4,26,255,0,4,10,11,2,2,2,1,8,255,1,8,26,255,0,14
        .byte 10,11,255,1,4,8,1,255,0,22,10,11,255,0,4,8,1,255,0,22,10,11,255,0,4,8,1,255,0,22,10,11,255,0,4
        .byte 8,1,2,2,2,28,255,0,24,8,255,1,5,255,2,16,28,255,8,8,255,1,30
        .byte 255,1,31,13,1,13,1,13,1,13,1,13,1,13,1,13,1,1,4,4,1,1,13,1,13,1,13,1,13,1,13,1,1
        .byte 255,0,7,16,255,0,16,16,0,0,0,1,1,255,0,7,16,255,0,16,16,0,0,0,1,1,255,0,7,16,255,0,16
        .byte 16,0,0,0,1,5,255,0,7,16,255,0,16,16,0,0,0,1,5,255,0,7,16,255,0,16,16,0,0,0,1,2,2
        .byte 10,11,255,0,4,16,0,0,0,27,28,255,0,6,27,28,0,0,0,16,0,0,0,1,1,1,10,11,255,0,4,16,0,0,0
        .byte 25,26,255,0,6,25,26,0,0,0,16,0,0,0,1,8,1,10,11,255,0,4,4,255,0,16,4,0,0,0,1,8,0
        .byte 10,11,255,0,25,1,8,0,10,11,255,0,25,1,8,0,10,11,255,0,25,1,8,0,10,11,255,0,25,1,8,255,0,28
        .byte 5,8,255,0,28,5,8,27,2,2,28,255,0,5,27,28,0,0,27,255,2,4,28,0,0,27,28,0,0,27,2,2,2
        .byte 255,8,26,255,1,34
        .byte 255,1,31,26,0,13,0,13,0,13,0,13,0,13,0,13,0,13,13,0,13,0,13,0,13,0,13,0,13,0,25,1,1
        .byte 255,0,28,1,1,255,0,28,1,1,255,0,28,1,1,255,0,28,1,1,0,0,10,11,255,2,18,28,255,0,5,1,1
        .byte 0,0,10,11,255,1,18,26,255,0,5,1,1,0,0,10,11,1,255,0,23,1,1,0,0,10,11,1,255,0,21,3,3
        .byte 1,1,3,3,10,11,1,0,0,0,10,11,255,2,18,1,1,0,0,10,11,1,0,0,0,10,11,255,1,20,0,0,10
        .byte 11,1,0,0,0,10,11,255,0,6,25,1,26,255,0,8,25,1,1,0,0,10,11,1,0,0,0,10,11,255,0,7,4
        .byte 255,0,10,5,5,0,0,10,11,1,255,0,23,5,5,0,0,10,11,1,255,0,23,5,255,2,5,1,255,0,8,4,255,0,7
        .byte 4,0,0,0,27,2,2,2,255,1,6,2,2,2,10,11,2,2,2,1,255,2,7,1,2,2,2,255,1,34
        .byte 7,7,29,255,0,27,1,1,255,7,8,29,255,0,19,255,1,10,7,29,255,0,18,1,26,255,0,8,25,7,29,255,0,17
        .byte 1,255,0,10,25,7,29,255,0,16,1,13,255,0,10,1,7,29,255,0,15,1,1,255,12,10,1,1,7,7,29,255,0,13
        .byte 1,26,255,0,10,25,1,1,1,7,255,0,13,1,255,0,14,1,7,255,0,13,1,13,0,0,0,4,4,4,255,0,7
        .byte 1,7,255,0,13,1,255,2,4,1,1,1,2,10,11,255,2,4,1,7,29,255,0,12,255,1,9,10,11,255,1,5,7,7
        .byte 29,255,0,11,1,26,255,0,7,10,11,0,0,0,25,1,1,7,7,255,0,11,5,255,0,8,10,11,255,0,5,25,1
        .byte 7,0,0,0,255,15,5,0,0,0,5,255,0,8,10,11,255,0,6,24,24,0,0,0,255,15,5,0,0,0,5,255,0,8
        .byte 10,11,255,0,6,24,24,0,0,0,255,15,5,0,0,0,255,2,21,255,14,9,255,1,19,255,7,11,255,1,19,255,7,11
        .byte 255,1,18,10,11,255,1,11,0,1,0,1,0,1,0,1,0,1,0,1,8,8,1,0,1,10,11,0,1,0,1,0,1
        .byte 0,1,0,1,1,255,0,12,8,8,0,0,0,10,11,255,0,9,1,1,255,0,12,8,8,0,0,0,10,11,255,0,9
        .byte 1,1,255,0,12,8,8,0,0,0,10,11,255,0,9,1,5,255,0,12,8,8,0,0,0,10,11,255,0,9,1,5,255,0,12
        .byte 8,8,0,0,0,10,11,255,0,9,1,2,2,28,255,0,10,8,8,0,0,0,10,11,255,0,9,1,1,1,26,3,3
        .byte 255,0,8,8,8,0,0,0,10,11,255,0,9,1,1,1,255,0,11,8,8,0,0,0,10,11,255,0,9,1,1,1,255,0,5
        .byte 27,2,2,28,0,0,8,8,0,0,27,2,2,2,28,255,0,7,1,1,1,0,0,0,3,3,255,1,4,255,8,6,255,1,5
        .byte 3,3,255,0,5,1,1,1,255,0,5,25,1,1,1,255,8,6,255,1,4,26,255,0,7,1,1,1,255,0,8,25,255,1,6
        .byte 26,255,0,9,27,2,1,1,26,3,3,3,255,0,6,25,0,1,1,0,26,255,0,8,3,3,1,1,1,13,255,0,26
        .byte 1,1,1,13,255,0,26,1,1,1,255,2,27,255,1,33
        .byte 255,0,11,255,1,19,255,0,11,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,255,0,11,1,255,0,17
        .byte 1,255,0,11,1,255,0,17,1,255,0,11,1,255,0,17,1,255,0,11,1,255,0,17,5,255,0,11,1,255,0,17,5
        .byte 255,0,11,1,255,0,15,27,2,2,255,0,11,1,255,0,15,25,1,1,255,0,7,255,1,5,255,0,12,255,3,4,1,1
        .byte 255,0,7,1,13,0,0,13,255,0,16,1,1,255,0,7,1,13,0,13,255,0,17,1,1,255,0,7,1,255,2,4,255,3,4
        .byte 255,2,4,255,3,4,255,2,4,255,1,34,13,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,13
        .byte 1,13,1,13,1,13,1,1,1,255,0,27,5,1,1,255,0,27,5,1,1,255,2,28,255,1,30
        .byte 255,1,9,10,11,255,1,20,26,13,255,0,6,10,11,0,13,25,1,26,13,0,0,0,13,25,1,26,13,0,0,13,25
        .byte 1,5,255,0,8,10,11,0,0,0,1,255,0,7,1,255,0,6,1,5,255,0,13,1,255,0,7,1,255,0,6,1,5
        .byte 255,0,13,1,13,255,0,6,1,255,0,5,13,1,255,2,14,1,13,0,13,13,0,0,13,1,13,255,0,4,3,255,1,16
        .byte 255,2,4,10,11,2,1,28,255,0,5,255,1,20,10,11,1,1,26,255,0,4,13,1,1,1,26,5,13,0,13,0,255,13,5
        .byte 0,13,0,0,0,13,10,11,13,1,255,0,5,3,1,1,1,13,0,0,0,13,255,0,15,1,255,0,6,1,1,1,0
        .byte 13,0,0,13,255,0,15,1,255,0,6,1,1,1,255,2,24,28,10,11,255,1,28,10,11,255,1,10,26,13,0,0,13
        .byte 0,0,13,25,255,1,9,10,11,1,5,255,0,18,13,13,24,24,13,13,0,0,10,11,1,5,255,0,19,13,24,24,13
        .byte 0,0,0,10,11,1,255,9,13,27,255,2,15,255,1,61
        .byte 255,1,31,26,255,0,26,25,1,1,255,0,28,5,1,255,0,28,5,1,255,0,28,5,1,3,3,255,0,23,3,3,3
        .byte 2,1,255,0,27,1,1,1,3,255,0,26,1,1,1,255,0,25,3,3,1,1,1,255,0,25,3,3,255,1,4,3,255,0,25
        .byte 255,1,4,255,0,26,255,1,4,255,0,24,3,3,255,1,4,255,2,6,10,11,255,2,10,28,255,0,7,255,1,10,10
        .byte 11,255,1,11,2,28,255,0,6,5,1,255,0,7,10,11,0,0,255,1,11,255,0,6,5,1,255,0,7,10,11,0,0
        .byte 1,1,8,8,255,1,7,2,2,28,255,9,4,1,255,2,7,10,11,2,2,1,1,8,8,255,1,22,10,11,255,1,4
        .byte 8,8,255,1,14
        .byte 255,1,8,10,11,255,1,4,8,8,255,1,15,26,255,0,6,10,11,255,1,4,8,8,255,1,15,255,0,7,10,11,1,1,1
        .byte 255,8,4,255,1,9,21,22,1,1,1,0,10,11,255,2,6,1,1,255,8,6,255,1,8,21,22,1,1,1,0,10,11
        .byte 255,1,7,8,8,1,8,8,1,8,8,255,1,6,0,19,20,0,5,1,0,10,11,0,1,1,1,255,8,4,1,1,8,8
        .byte 1,1,255,8,4,1,1,1,0,19,20,0,5,1,0,10,11,0,1,8,8,8,255,1,5,8,8,255,1,5,8,8,8
        .byte 1,0,19,20,0,5,1,0,10,11,0,1,8,255,1,6,255,8,4,255,1,6,8,1,0,19,20,2,2,1,0,10,11
        .byte 0,25,8,26,0,0,0,25,8,8,1,1,8,8,26,0,0,0,25,8,26,0,19,20,25,1,1,1,10,11,0,0,8
        .byte 255,0,5,8,255,1,4,8,255,0,4,15,8,0,0,19,20,0,1,13,13,255,0,4,8,255,0,5,8,25,1,1,26
        .byte 8,255,0,5,8,0,0,19,20,0,1,24,24,255,0,4,8,255,0,5,8,0,21,22,0,8,255,0,5,8,255,0,5
        .byte 1,24,24,255,0,4,8,255,0,5,8,0,21,22,0,8,255,0,5,8,255,0,5,1,2,2,10,11,2,2,8,2,28
        .byte 0,0,0,8,0,19,20,0,8,0,0,0,27,2,8,2,2,10,11,0,1,1,1,10,11,1,1,8,1,1,3,0,0
        .byte 8,0,19,20,0,8,0,0,3,1,1,8,1,1,10,11,0,1,1,0,10,11,255,1,4,26,3,0,0,8,0,19,20
        .byte 0,8,0,0,3,25,255,1,4,10,11,0,1,1,0,10,11,255,0,8,8,0,19,20,0,8,255,0,8,10,11,0,1,1
        .byte 0,10,11,255,0,8,8,0,19,20,0,8,255,0,8,10,11,0,1,1,0,10,11,255,2,7,28,8,0,19,20,0,8
        .byte 27,255,2,7,10,11,0,1
        .byte 1,0,10,11,255,1,8,8,0,19,20,0,8,255,1,8,10,11,1,1,1,0,10,11,255,0,6,1,1,8,0,19,20
        .byte 0,8,255,1,7,0,10,11,0,1,1,0,10,11,255,0,6,1,1,8,0,19,20,0,8,255,1,7,0,10,11,0,1,1
        .byte 0,10,11,255,2,6,1,1,8,0,19,20,0,8,255,1,8,10,11,1,1,1,0,10,11,255,1,8,8,1,19,20,1
        .byte 8,255,1,7,255,0,4,1,1,0,10,11,255,0,4,21,22,0,39,8,1,19,20,1,8,39,0,21,22,0,39,1,255,0,4
        .byte 1,1,0,10,11,255,0,4,21,22,0,55,1,54,19,20,55,1,54,0,21,22,0,55,54,0,10,11,1,1,1,0,10
        .byte 11,255,0,4,19,20,255,0,4,19,20,255,0,4,19,20,255,0,4,10,11,0,1,1,0,10,11,255,0,4,19,20,255,0,4
        .byte 19,20,255,0,4,19,20,255,0,4,10,11,0,1,1,0,10,11,255,0,4,19,20,255,0,4,19,20,255,0,4,19,20
        .byte 15,0,0,0,10,11,0,1,1,1,10,11,255,0,4,19,20,255,0,4,19,20,255,0,4,19,20,255,0,4,10,11,0
        .byte 1,1,255,0,7,19,20,255,0,4,19,20,255,0,4,19,20,255,0,4,10,11,0,1,1,255,0,7,19,20,255,0,4
        .byte 19,20,255,0,4,19,20,255,0,4,10,11,0,1,1,255,4,4,28,0,0,19,20,255,0,10,19,20,255,0,4,10,11
        .byte 255,1,7,26,255,0,22,24,24,255,1,5,255,0,23,24,24,255,1,5,4,27,255,2,6,28,255,4,10,27,255,2,5
        .byte 255,1,60
        .byte 255,0,15,255,1,15,255,0,15,1,39,39,39,26,255,0,8,25,1,255,0,15,1,39,39,39,255,0,10,1,255,0,15
        .byte 1,39,39,39,255,0,10,1,255,0,15,255,1,4,255,6,5,29,255,0,4,1,255,0,15,25,255,1,9,255,12,4,1
        .byte 255,0,23,25,1,255,0,4,1,255,0,24,1,255,0,4,1,255,0,24,1,0,0,0,3,1,255,0,24,1,255,0,4
        .byte 1,255,0,24,1,255,0,4,5,255,0,24,1,3,0,0,0,5,255,0,24,1,255,0,4,5,255,0,24,1,4,4,2,2,2
        .byte 255,0,24,25,255,1,5,255,0,120
        .byte 255,1,31,26,255,0,26,25,1,1,255,0,28,1,1,255,0,28,1,5,255,0,28,1,5,255,0,26,3,3,1,5,255,0,28
        .byte 1,2,2,28,255,0,26,255,1,4,255,0,24,3,3,255,1,4,2,2,28,255,0,23,255,1,7,2,2,28,255,0,4
        .byte 27,28,255,0,4,27,28,255,0,8,1,1,26,0,0,0,25,1,1,26,255,0,4,25,26,255,0,4,25,26,255,0,4
        .byte 27,2,10,11,1,1,255,0,24,25,1,10,11,1,1,255,0,26,10,11,1,1,255,0,26,10,11,1,1,2,2,28,255,0,25
        .byte 255,1,5,255,0,25,255,1,5,255,2,25,255,1,31
        .byte 255,0,0,0,255,30,4,255,0,25,30,33,0,0,32,30,255,0,8,255,30,16,33,255,0,4,32,255,30,11,33,255,0,24
        .byte 32,30,30,33,255,0,27,30,255,0,29,30,255,0,29,30,255,0,29,30,255,31,15,36,255,0,7,34,35,255,0,4,255,30,16
        .byte 36,37,255,31,6,30,30,255,31,4,255,30,16,36,37,255,30,13
        .byte 255,0,4,255,30,11,36,37,30,255,0,16,30,33,255,0,9,36,37,30,255,0,16,30,255,0,10,36,37,30,255,0,16
        .byte 30,255,0,10,36,37,255,30,6,255,0,11,30,255,0,5,34,31,30,255,31,4,30,255,0,4,30,255,0,11,30,255,0,5
        .byte 32,255,30,6,33,255,0,4,30,255,0,11,30,3,3,255,0,15,30,255,0,11,30,255,0,17,30,255,0,11,30,255,0,17
        .byte 30,255,0,7,255,30,5,255,31,4,30,255,31,4,30,255,31,4,30,36,37,30,255,0,7,30,33,0,0,32,255,30,15
        .byte 36,37,30,255,0,27,36,37,30,255,0,29,30,255,0,29,30,255,0,7,3,3,3,255,30,20,255,0,127
        .byte 30,255,0,27,30,30,30,255,0,27,30,30,30,255,0,27,30,30,30,255,0,27,30,30,30,255,0,27,30,30,30,255,0,27
        .byte 30,30,30,255,0,27,30,30,30,255,0,27,30,30,30,35,255,0,9,34,31,31,31,36,37,255,31,4,30,30,31,30,30
        .byte 31,31,255,30,4,255,0,9,32,30,30,30,36,37,255,30,14,33,0,255,38,4,255,0,7,30,36,37,0,32,32,30,33
        .byte 255,0,6,32,30,53,53,0,32,30,30,33,255,0,7,30,36,37,255,0,13,24,24,255,0,12,30,36,37,255,0,13,24,24
        .byte 255,0,12,30,36,37,255,0,13,3,3,255,30,6,36,37,255,30,5,36,37,255,0,9,34,3,3,3,30,30,33,255,0,5
        .byte 36,37,255,0,4,30,255,0,10,34,255,30,6,255,0,6,36,37,255,0,4,30,255,0,9,34,30,30,33,0,0,30,30
        .byte 255,31,12,30,255,31,9,30,30,30,0,0,0,255,30,25,33,33,0,0,0
        .byte 255,0,29,30,255,0,29,30,255,0,29,30,255,0,29,30,255,0,29,30,255,0,29,30,255,0,29,30,255,0,29,30,255,0,27
        .byte 34,30,30,255,0,22,34,255,30,7,255,0,20,34,30,33,255,53,4,32,30,30,255,0,18,34,30,33,255,0,6,53,53,53
        .byte 255,0,16,34,30,33,53,255,0,24,34,30,33,255,0,18,34,255,30,8,33,53,255,0,7,34,255,30,6,255,0,5,30
        .byte 255,52,4,255,53,4,255,0,7,34,30,33,255,0,11,30,255,52,4,53,255,0,8,34,30,33,255,0,13,30,255,52,4
        .byte 53,255,0,6,34,30,33,255,0,15,32,255,30,12,33,255,0,11
        .byte 34,255,30,22,35,255,0,6,30,33,255,0,8,32,30,30,33,255,0,8,32,30,255,0,6,30,255,0,22,30,255,0,6
        .byte 30,36,255,0,20,37,30,255,0,6,30,36,37,0,34,30,30,35,255,0,8,34,30,30,35,0,36,37,30,255,0,6,30
        .byte 36,37,0,32,30,30,33,255,0,8,32,30,30,33,0,36,37,30,255,0,6,30,36,37,255,0,18,36,37,30,255,0,6
        .byte 30,255,0,22,30,255,0,6,30,255,0,22,30,255,0,6,30,36,255,0,20,37,30,255,0,6,30,36,37,0,34,30,30
        .byte 35,255,0,8,34,30,30,35,0,36,37,30,255,0,6,30,36,37,0,32,30,30,33,255,0,8,32,30,30,33,0,36,37
        .byte 30,255,0,6,30,36,37,255,0,18,36,37,30,255,0,6,30,255,0,22,30,255,0,6,30,255,0,22,30,255,0,6,30
        .byte 31,31,255,30,4,35,255,0,8,34,255,30,4,31,31,30,255,0,6,32,255,30,7,35,255,0,6,34,255,30,7,33,255,0,14
        .byte 32,31,255,38,4,31,33,255,0,23,32,255,30,4,33,255,0,15
numsc:
        .byte 29
.out .sprintf("SCREENS SIZE = %d", (* - scdat))

nmedat:
        .byte 0,1,120,80,5,14,120,8,5,15,120,24,5,16,120,40,5,17,104,40,5,18,104,24,5,19,104,8,255
        .byte 0,1,120,24,8,11,56,72,8,11,56,152,1,3,64,112,255
        .byte 0,1,96,8,5,5,136,144,2,20,64,80,2,4,128,80,8,11,40,104,8,11,128,48,255
        .byte 0,1,56,8,1,6,80,40,3,6,48,176,8,11,40,120,8,11,32,224,255
        .byte 0,1,128,8,1,7,72,192,2,7,16,128,2,7,128,96,8,11,128,72,8,11,16,104,255
        .byte 0,1,48,8,1,13,96,56,2,20,128,128,4,9,72,160,4,9,48,192,8,11,72,128,8,11,104,24,8,11,40,128,255
        .byte 0,1,48,8,2,20,40,72,1,7,112,72,4,9,72,208,5,5,136,208,5,5,136,224,255
        .byte 0,1,72,8,2,20,128,88,1,3,88,144,1,6,56,104,4,9,40,184,5,5,136,144,8,11,128,16,8,11,40,16,8,11,80,224,255
        .byte 0,1,128,8,1,3,96,168,1,13,64,104,1,7,88,136,1,4,72,200,1,20,80,72,5,5,136,104,5,5,136,136,5,5,136,168,8,11,128,72,8,11,120,192,255
        .byte 0,1,128,8,6,12,24,88,6,12,24,152,8,11,72,16,8,11,48,224,255
        .byte 0,1,128,8,3,6,24,112,8,11,120,224,1,6,72,184,255
        .byte 0,1,40,8,6,12,24,72,6,12,24,128,6,12,24,184,8,11,48,224,8,11,72,16,255
        .byte 0,1,80,8,6,12,24,64,6,12,24,136,1,6,24,168,2,4,128,136,8,11,120,24,8,11,72,128,255
        .byte 0,1,48,8,4,9,88,136,1,7,64,88,1,7,104,184,8,11,48,104,8,11,48,168,255
        .byte 0,1,120,8,2,4,40,80,2,4,40,152,8,11,72,16,255
        .byte 0,1,120,8,7,15,120,192,7,14,120,176,7,16,120,208,7,17,104,208,7,18,104,192,7,19,104,176,8,11,72,112,8,11,32,48,255
        .byte 0,0,72,152,2,4,128,112,8,11,72,72,255
        .byte 0,0,128,232,1,3,24,144,3,6,56,112,8,11,128,24,255
        .byte 0,0,32,80,1,7,24,208,8,11,80,40,255
        .byte 0,0,32,232,2,4,96,120,3,7,56,16,8,11,128,16,8,11,32,16,255
        .byte 0,0,16,56,8,11,96,64,8,11,96,176,255
        .byte 0,0,16,40,2,10,120,96,8,11,16,72,255
        .byte 0,0,96,232,8,11,24,176,255
        .byte 0,1,48,8,2,4,128,56,1,7,88,136,1,13,48,184,6,12,16,88,6,12,16,160,8,11,112,16,255
        .byte 0,1,120,8,1,7,80,152,2,10,120,80,8,11,128,216,255
        .byte 0,0,24,112,2,20,64,120,255
        .byte 0,0,104,232,5,8,128,104,2,10,56,136,8,11,128,32,8,11,56,216,1,20,104,160,3,7,32,72,255
        .byte 0,0,136,112,8,11,136,96,255
        .byte 0,1,128,96,5,8,72,48,8,11,24,48,8,11,24,144,8,11,72,144,3,7,40,96,255
.out .sprintf("POSITIONS SIZE = %d", (* - nmedat))

NUMOBJ = 59
objdta:
        .byte 0,0,0,0,68,68,170,170,68,68,36,72,36,72,31,240,17,16,21,80,19,144,9,32,15,224,10,160,15,224,0,0,12,120,24,12,120,24
        .byte 255,255,130,65,164,33,168,21,168,21,160,133,161,133,35,132,33,132,161,133,160,5,168,21,168,21,132,33,130,65,255,255,20,96,8,20,96,8
        .byte 255,255,130,65,164,33,168,21,168,21,163,197,160,69,35,196,34,4,163,197,160,5,168,21,168,21,132,33,130,65,255,255,18,120,176,18,120,176
        .byte 0,0,0,0,0,0,0,0,68,68,170,170,68,68,68,68,63,248,32,8,18,144,8,32,10,160,8,32,15,224,0,0,12,72,128,12,72,128
        .byte 0,0,0,0,0,12,28,18,15,225,7,225,6,97,12,50,13,180,24,24,27,216,48,12,55,236,48,12,31,248,15,240,19,128,16,19,128,16
        .byte 0,0,255,255,213,85,128,1,152,1,164,1,167,253,164,21,152,149,129,129,128,129,128,129,128,129,128,1,170,171,255,255,17,88,80,17,88,80
        .byte 0,0,255,255,213,85,128,1,152,1,164,1,167,253,164,21,153,213,128,65,129,193,129,1,129,193,128,1,170,171,255,255,23,32,224,23,32,224
        .byte 0,0,127,254,53,172,36,36,36,36,39,228,32,4,39,228,37,164,39,228,32,4,39,228,36,36,36,36,53,172,127,254,13,48,104,13,48,104
        .byte 0,0,127,254,53,172,36,36,36,36,39,228,32,4,39,228,37,164,39,228,32,4,39,228,36,36,36,36,53,172,127,254,13,48,168,13,48,168
        .byte 0,128,31,248,32,4,16,2,13,85,72,1,114,242,61,126,43,106,85,82,255,226,142,34,132,34,128,36,128,40,255,240,27,136,96,27,136,96
        .byte 1,0,195,134,189,122,129,82,157,82,137,82,65,84,127,252,85,4,85,116,85,36,53,8,21,16,13,32,7,192,3,128,28,24,48,28,24,48
        .byte 0,0,0,0,0,0,31,248,16,8,27,216,15,224,10,80,31,248,20,40,30,120,14,112,4,32,4,32,7,224,0,0,9,48,224,9,48,224
        .byte 1,0,195,134,189,122,129,2,149,34,137,2,65,4,127,252,65,4,65,84,69,36,33,8,17,16,9,32,7,192,3,128,28,24,144,28,24,144
        .byte 1,0,195,134,189,254,129,254,157,142,145,238,73,252,127,252,127,4,127,116,99,68,59,40,31,16,15,32,7,192,3,128,28,72,144,28,72,144
        .byte 0,67,0,197,0,202,0,212,0,232,0,80,0,191,1,94,2,128,5,0,10,0,20,0,40,0,80,0,160,0,192,0,1,56,152,1,56,152
        .byte 194,0,163,0,83,0,43,0,23,0,10,0,253,0,122,128,1,64,0,160,0,80,0,40,0,20,0,10,0,5,0,3,1,56,72,1,56,72
        .byte 0,0,0,0,0,0,15,224,19,144,34,136,85,84,50,152,31,240,18,144,9,32,9,32,4,64,2,128,1,0,0,0,2,128,48,2,128,48
        .byte 0,0,0,0,0,0,54,108,20,40,20,40,31,248,2,96,13,176,9,144,6,96,3,192,2,64,1,128,0,0,0,0,2,40,104,2,40,104
        .byte 0,0,0,0,0,0,0,0,20,40,20,40,31,248,2,96,13,176,14,112,6,96,7,224,2,64,1,128,0,0,0,0,15,32,48,15,32,48
        .byte 0,0,0,0,1,0,2,128,13,96,18,144,31,240,31,240,32,8,47,232,32,8,23,208,16,16,8,32,7,192,0,0,3,40,120,3,40,120
        .byte 0,0,48,24,40,40,36,72,34,136,33,8,40,44,32,4,41,36,32,12,41,40,32,8,41,40,32,8,63,248,0,0,3,32,224,3,32,224
        .byte 0,0,3,128,7,192,8,32,19,144,36,72,72,36,209,22,210,150,209,22,72,36,36,72,19,144,8,32,7,192,3,128,4,128,72,4,128,72
        .byte 0,0,0,0,0,0,0,0,3,192,4,32,8,16,15,240,48,12,55,236,8,16,8,16,7,224,0,0,0,0,0,0,4,16,104,4,16,104
        .byte 1,128,1,128,2,64,1,128,1,128,1,128,2,64,4,32,9,16,18,8,20,40,16,72,8,144,8,16,4,32,7,224,5,104,24,5,104,24
        .byte 0,0,0,0,0,0,1,128,1,128,1,128,2,64,4,32,5,160,8,16,11,208,16,8,23,232,16,8,15,240,0,0,5,40,128,5,40,128
        .byte 0,0,0,0,1,128,2,64,5,32,4,160,2,64,4,32,5,160,8,16,11,208,16,8,23,232,16,8,15,240,0,0,5,72,128,5,72,128
        .byte 0,0,255,255,213,85,128,1,128,1,136,1,148,1,162,1,161,253,162,21,148,21,136,21,128,1,128,1,170,171,255,255,27,136,56,27,136,56
        .byte 255,255,130,65,164,33,169,149,169,149,164,37,162,69,113,142,113,142,162,69,164,37,169,149,169,149,132,33,130,65,255,255,15,120,144,15,120,144
        .byte 0,0,0,0,0,0,0,0,0,0,48,140,77,50,39,228,84,42,39,228,72,146,49,12,0,0,0,0,0,0,0,0,8,128,72,8,128,72
        .byte 96,6,144,9,175,245,80,10,23,8,18,8,19,104,16,8,21,88,16,8,21,104,16,8,95,250,160,5,144,9,96,6,26,128,32,26,128,32
        .byte 0,0,0,0,1,192,63,254,121,207,115,231,98,163,5,208,15,120,21,212,18,164,19,228,16,132,16,132,31,252,0,0,9,72,16,9,72,16
        .byte 0,0,0,0,1,128,2,64,5,160,24,24,19,200,10,80,9,144,10,80,18,72,11,208,8,16,9,144,6,96,0,0,8,120,192,8,120,192
        .byte 0,0,31,248,19,200,57,156,69,162,147,201,69,162,41,148,19,200,3,192,57,156,69,162,147,201,69,162,43,212,23,232,22,24,176,22,24,176
        .byte 0,0,0,0,1,128,15,240,9,144,1,128,2,64,4,32,5,160,8,16,9,144,16,8,17,136,16,8,15,240,0,0,14,72,16,14,72,16
        .byte 0,0,1,0,2,128,4,64,10,160,16,16,41,40,64,4,182,218,64,4,41,40,16,16,10,160,4,64,2,128,1,0,7,128,16,7,128,16
        .byte 0,0,5,160,5,160,7,224,3,192,2,64,4,32,9,16,18,8,20,8,16,40,16,72,8,144,4,32,3,192,0,0,7,80,224,7,80,224
        .byte 3,192,13,176,27,216,31,248,57,156,49,140,25,152,28,56,62,124,46,116,54,108,30,120,10,80,6,96,0,0,0,0,7,40,16,7,40,16
        .byte 0,0,0,0,0,0,27,236,22,180,16,4,22,180,20,20,28,156,9,200,8,8,25,76,19,228,30,60,0,0,0,0,10,120,224,10,120,224
        .byte 3,192,2,64,7,224,12,48,9,144,8,16,13,176,6,96,3,192,1,128,1,128,3,192,1,128,3,192,5,160,7,224,11,48,224,11,48,224
        .byte 0,0,0,0,30,120,33,132,40,20,35,196,25,152,5,160,5,160,5,160,4,32,2,64,2,64,4,32,9,144,14,112,11,72,16,11,72,16
        .byte 0,0,3,192,62,124,66,66,67,194,64,2,63,252,97,134,63,252,64,2,67,194,94,122,64,2,126,126,99,198,0,0,17,128,24,17,128,24
        .byte 0,0,0,0,0,0,0,0,96,3,90,173,74,169,106,171,63,254,24,12,8,8,10,168,8,8,8,8,15,248,0,0,24,128,216,24,128,216
        .byte 0,0,0,0,15,192,16,32,32,80,64,40,129,148,130,76,132,148,130,36,65,72,32,144,16,32,15,192,0,0,0,0,16,72,72,16,72,72
        .byte 0,0,0,0,0,0,0,24,0,56,1,208,1,160,1,96,2,224,13,0,10,0,22,0,40,0,48,0,0,0,0,0,21,16,72,21,16,72
        .byte 0,0,0,0,15,240,16,8,23,232,16,8,11,208,8,16,5,160,8,16,11,208,16,8,23,232,16,8,15,240,0,0,23,112,16,23,112,16
        .byte 0,0,0,0,31,248,16,8,31,248,16,8,23,232,9,144,9,144,8,16,6,96,1,128,1,128,3,192,3,192,15,240,15,72,112,15,72,112
        .byte 0,0,15,240,24,24,16,8,53,76,224,7,138,161,128,1,149,85,128,1,138,161,224,7,53,76,16,8,24,24,15,240,20,112,120,20,112,120
        .byte 0,0,0,0,3,192,4,32,5,160,2,64,2,64,5,160,4,32,8,16,10,80,16,8,16,8,11,208,12,48,0,0,20,96,64,20,96,64
        .byte 0,0,0,0,3,192,4,32,5,160,2,64,2,64,5,160,4,32,8,16,10,80,16,8,16,8,11,208,12,48,0,0,20,96,176,20,96,176
        .byte 0,0,0,0,0,0,15,240,8,16,15,240,7,224,12,48,31,248,28,56,31,248,12,48,15,240,4,32,7,224,0,0,26,56,216,26,56,216
        .byte 0,0,0,0,0,0,0,0,0,24,3,248,7,240,15,32,31,64,29,32,28,48,14,160,14,96,4,64,7,192,0,0,19,96,24,19,96,24
        .byte 255,255,130,65,164,33,168,21,168,21,163,197,160,69,35,196,32,68,163,197,160,5,168,21,168,21,132,33,130,65,255,255,21,120,232,21,120,232
        .byte 0,0,255,255,213,85,128,1,152,1,164,1,167,253,164,21,153,213,128,65,129,193,128,65,129,193,128,1,170,171,255,255,18,40,136,18,40,136
        .byte 255,255,130,65,164,33,168,21,168,21,162,69,162,69,35,196,32,68,160,69,160,5,168,21,168,21,132,33,130,65,255,255,26,104,8,26,104,8
        .byte 0,0,255,255,213,85,128,1,152,1,164,1,167,253,164,21,153,85,129,65,129,193,128,65,128,65,128,1,170,171,255,255,22,24,136,22,24,136
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,0,24,0,232,0,184,0,232,0,24,0,8,0,0,0,0,0,15,72,16,15,72,16
        .byte 7,252,8,2,9,194,10,34,12,146,10,34,17,68,33,74,127,242,71,18,66,18,64,18,64,20,64,24,127,240,0,0,18,80,40,18,80,40
        .byte 0,0,0,0,12,96,11,160,105,44,144,18,146,146,139,162,73,36,41,40,19,144,18,144,8,32,5,64,3,128,0,0,19,32,16,19,32,16
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,24,0,23,0,29,0,23,0,24,0,16,0,0,0,0,18,32,224,18,32,224
.out .sprintf("OBJECTS SIZE = %d", (* - objdta))

font:
        .byte 0,0,0,0,0,0,0,0
        .byte 0,16,16,16,16,0,16,0
        .byte 0,36,36,0,0,0,0,0
        .byte 0,36,126,36,36,126,36,0
        .byte 0,8,62,40,62,10,62,8
        .byte 0,98,100,8,16,38,70,0
        .byte 0,16,40,16,42,68,58,0
        .byte 0,8,16,0,0,0,0,0
        .byte 0,4,8,8,8,8,4,0
        .byte 0,32,16,16,16,16,32,0
        .byte 0,0,20,8,62,8,20,0
        .byte 0,0,8,8,62,8,8,0
        .byte 0,0,0,0,0,8,8,16
        .byte 0,0,0,0,62,0,0,0
        .byte 0,0,0,0,0,24,24,0
        .byte 0,0,2,4,8,16,32,0
        .byte 0,60,70,74,82,98,60,0
        .byte 0,24,40,8,8,8,62,0
        .byte 0,60,66,2,60,64,126,0
        .byte 0,60,66,12,2,66,60,0
        .byte 0,8,24,40,72,126,8,0
        .byte 0,126,64,124,2,66,60,0
        .byte 0,60,64,124,66,66,60,0
        .byte 0,126,2,4,8,16,16,0
        .byte 0,60,66,60,66,66,60,0
        .byte 0,60,66,66,62,2,60,0
        .byte 0,0,0,16,0,0,16,0
        .byte 0,0,16,0,0,16,16,32
        .byte 0,0,4,8,16,8,4,0
        .byte 0,0,0,62,0,62,0,0
        .byte 0,0,16,8,4,8,16,0
        .byte 0,60,66,4,8,0,8,0
        .byte 0,60,74,86,94,64,60,0
        .byte 0,60,66,66,126,66,66,0
        .byte 0,124,66,124,66,66,124,0
        .byte 0,60,66,64,64,66,60,0
        .byte 0,120,68,66,66,68,120,0
        .byte 0,126,64,124,64,64,126,0
        .byte 0,126,64,124,64,64,64,0
        .byte 0,60,66,64,78,66,60,0
        .byte 0,66,66,126,66,66,66,0
        .byte 0,62,8,8,8,8,62,0
        .byte 0,2,2,2,66,66,60,0
        .byte 0,68,72,112,72,68,66,0
        .byte 0,64,64,64,64,64,126,0
        .byte 0,66,102,90,66,66,66,0
        .byte 0,66,98,82,74,70,66,0
        .byte 0,60,66,66,66,66,60,0
        .byte 0,124,66,66,124,64,64,0
        .byte 0,60,66,66,82,74,60,0
        .byte 0,124,66,66,124,68,66,0
        .byte 0,60,64,60,2,66,60,0
        .byte 0,254,16,16,16,16,16,0
        .byte 0,66,66,66,66,66,60,0
        .byte 0,66,66,66,66,36,24,0
        .byte 0,66,66,66,66,90,36,0
        .byte 0,66,36,24,24,36,66,0
        .byte 0,130,68,40,16,16,16,0
        .byte 0,126,4,8,16,32,126,0
        .byte 0,14,8,8,8,8,14,0
        .byte 0,0,64,32,16,8,4,0
        .byte 0,112,16,16,16,16,112,0
        .byte 0,16,56,84,16,16,16,0
        .byte 0,0,0,0,0,0,0,255
        .byte 0,28,34,120,32,32,126,0
        .byte 0,0,56,4,60,68,60,0
        .byte 0,32,32,60,34,34,60,0
        .byte 0,0,28,32,32,32,28,0
        .byte 0,4,4,60,68,68,60,0
        .byte 0,0,56,68,120,64,60,0
        .byte 0,12,16,24,16,16,16,0
        .byte 0,0,60,68,68,60,4,56
        .byte 0,64,64,120,68,68,68,0
        .byte 0,16,0,48,16,16,56,0
        .byte 0,4,0,4,4,4,36,24
        .byte 0,32,40,48,48,40,36,0
        .byte 0,16,16,16,16,16,12,0
        .byte 0,0,104,84,84,84,84,0
        .byte 0,0,120,68,68,68,68,0
        .byte 0,0,56,68,68,68,56,0
        .byte 0,0,120,68,68,120,64,64
        .byte 0,0,60,68,68,60,4,6
        .byte 0,0,28,32,32,32,32,0
        .byte 0,0,56,64,56,4,120,0
        .byte 0,16,56,16,16,16,12,0
        .byte 0,0,68,68,68,68,56,0
        .byte 0,0,68,68,40,40,16,0
        .byte 0,0,68,84,84,84,40,0
        .byte 0,0,68,40,16,40,68,0
        .byte 0,0,68,68,68,60,4,56
        .byte 0,0,124,8,16,32,124,0
        .byte 0,14,8,48,8,8,14,0
        .byte 0,8,8,8,8,8,8,0
        .byte 0,112,16,12,16,16,112,0
        .byte 0,20,40,0,0,0,0,0
        .byte 60,66,153,161,161,153,66,60
.out .sprintf("FONT SIZE = %d", (* - font))

jtab:
        .byte 248,250,252,254,254,255,255,255,0,0,0,1,1,1,2,2,4,6,8,8,8,99
keys:   .byte 66,97,104,72,73,16,55,48,49,17,18
.out .sprintf("KEYS SIZE = %d", (* - keys))

data_end:
.out .sprintf("TOTAL DATA SIZE = %d", (* - data_start))
