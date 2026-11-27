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

WINDOWTOP = 0
WINDOWLFT = 0
WINDOWHGT = 20
WINDOWWID = 32 ;面面面面面面面面面面面面
MAPWID = 13
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255
mapdat:
        .byte 255,2,3,4,5,6,7,8,18,22,255,255,255,255,255,255,20,18,16,15,25,22,23,0,255,255,255,26,24,255,26,18,14,22,23,22,255,255,255,255,9,255,255,9,255,9,23,19,27,16,28,255,255,23,18,17,15,255,10,255,1,9,255,255,255,255,255,255,255,255,255,11,12,13,23,21,255,255
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255
stmap:  .byte 1

evnt00:
        lda #2
        cmp scno
        beq *+5
        jmp a00070
        lda #7
        sta z80_b
        jsr sktyp
        bcs :+
        jmp a00070
:
        lda #11
        jsr gotob
        cmp #255
        beq :+
        jmp a00070
:
        lda #1
        sta gamwon
a00070: jsr skobj
        sta varobj
        lda #255
        cmp varobj
        bne *+5
        jmp a00189
        lda varobj
        jsr getob
        lda varc
        clc
        adc #1
        sta varc
        lda #40
        asl a
        sta sndtyp
        lda #40
        asl a
        sta sndtyp
        lda #<1
        sta z80_l
        lda #>1
        sta z80_h
        jsr addsc
        lda #22
        sta chary
        lda #23
        sta charx
        jsr dscor
a00189: lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp a00216
:
        jmp a00234
a00216: jsr cangd
        beq :+
        jmp a00234
:
        jsr ifall
a00234: lda #238
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp a00304
        jsr scrr
        lda #2
        ldy #9
        sta (z80_ix),y
        dey; ldy #8
        lda (z80_ix),y
        sta varo
        iny; ldy #9
        lda (z80_ix),y
        sta varp
a00304: lda #0
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp a00374
        jsr scrl
        lda #236
        ldy #9
        sta (z80_ix),y
        dey; ldy #8
        lda (z80_ix),y
        sta varo
        iny; ldy #9
        lda (z80_ix),y
        sta varp
a00374: lda #141
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a00443
        jsr scrd
        lda #4
        ldy #8
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        sta varo
        iny; ldy #9
        lda (z80_ix),y
        sta varp
a00443: lda #2
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp a00512
        jsr scru
        lda #139
        ldy #8
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        sta varo
        iny; ldy #9
        lda (z80_ix),y
        sta varp
a00512: jsr laddu
        beq :+
        jmp a00539
:
        lda #1
        sta varl
        jmp a00573
a00539: jsr laddd
        beq :+
        jmp a00565
:
        lda #2
        sta varl
        jmp a00573
a00565: lda #0
        sta varl
a00573: lda joyval
        and #1
        beq :+
        jmp a00928
:
        jsr cangr
        beq :+
        jmp a00928
:
        lda joyval
        and #4
        beq :+
        jmp a00705
:
        jsr laddd
        beq :+
        jmp a00640
:
        jmp a00701
a00640: lda #0
        cmp vara
        beq *+5
        jmp a00665
        ; lda #0
        jsr animsp
a00665: lda #2
        ldy #6
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a00701: jmp a00928
a00705: lda joyval
        and #8
        beq :+
        jmp a00807
:
        jsr laddu
        beq :+
        jmp a00741
:
        jmp a00802
a00741: lda #2
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp a00780
        ; lda #0
        jsr animsp
a00780: ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a00802: jmp a00928
a00807: lda #1
        sta vard
        lda #2
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp a00906
        ; lda #0
        jsr animsp
        lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a00906
        jsr cangd
        beq :+
        jmp a00894
:
        jmp a00906
a00894: lda #5
        asl a
        sta sndtyp
a00906: ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a00928: lda joyval
        and #2
        beq :+
        jmp a01283
:
        jsr cangl
        beq :+
        jmp a01283
:
        lda joyval
        and #4
        beq :+
        jmp a01061
:
        jsr laddd
        beq :+
        jmp a00995
:
        jmp a01056
a00995: lda #0
        cmp vara
        beq *+5
        jmp a01021
        ; lda #0
        jsr animsp
a01021: lda #1
        ldy #6
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a01056: jmp a01283
a01061: lda joyval
        and #8
        beq :+
        jmp a01162
:
        jsr laddu
        beq :+
        jmp a01096
:
        jmp a01157
a01096: lda #1
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp a01135
        ; lda #0
        jsr animsp
a01135: ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a01157: jmp a01283
a01162: lda #2
        sta vard
        lda #1
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp a01261
        ; lda #0
        jsr animsp
        lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a01261
        jsr cangd
        beq :+
        jmp a01249
:
        jmp a01261
a01249: lda #5
        asl a
        sta sndtyp
a01261: ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a01283: lda joyval
        and #8
        beq :+
        jmp a01376
:
        jsr laddu
        beq :+
        jmp a01376
:
        lda #0
        ldy #6
        sta (z80_ix),y
        ; lda #0
        cmp vara
        beq *+5
        jmp a01354
        ; lda #0
        jsr animsp
a01354: ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a01376: lda joyval
        and #4
        beq :+
        jmp a01468
:
        jsr laddd
        beq :+
        jmp a01468
:
        lda #0
        ldy #6
        sta (z80_ix),y
        ; lda #0
        cmp vara
        beq *+5
        jmp a01446
        ; lda #0
        jsr animsp
a01446: ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a01468: jsr cangu
        beq :+
        jmp a01580
:
        lda joyval
        and #16
        beq :+
        jmp a01580
:
        jsr cangd
        beq :+
        jmp a01518
:
        jmp a01580
a01518: lda #1
        cmp varl
        beq *+5
        jmp a01539
        jmp a01580
a01539: jsr jump
        lda #5
        asl a
        sta sndtyp
        lda #10
        asl a
        sta sndtyp
        lda #20
        asl a
        sta sndtyp
a01580: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01644
:
        lda varo
        ldy #8
        sta (z80_ix),y
        lda varp
        iny; ldy #9
        sta (z80_ix),y
        lda #1
        sta deadf
a01644: jmp grav
.out .sprintf("EVENT 0 SIZE = %d", (* - evnt00))

evnt01:
        lda #12
        cmp scno
        beq *+5
        jmp b00023
        jmp b00023
b00023: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b00130
        jsr cangl
        beq :+
        jmp b00112
:
        lda #0
        cmp vara
        beq *+5
        jmp b00085
        ; lda #0
        jsr animsp
b00085: ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp b00125
b00112: lda #1
        ldy #11
        sta (z80_ix),y
b00125: jmp b00209
b00130: jsr cangr
        beq :+
        jmp b00196
:
        lda #0
        cmp vara
        beq *+5
        jmp b00169
        ; lda #0
        jsr animsp
b00169: ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp b00209
b00196: lda #0
        ldy #11
        sta (z80_ix),y
b00209: lda #12
        cmp scno
        beq *+5
        jmp b00231
        jmp b00231
b00231: rts
.out .sprintf("EVENT 1 SIZE = %d", (* - evnt01))

evnt02:
        ldy #6
        lda (z80_ix),y
        sta vari
        lda #3
        cmp vari
        beq *+5
        jmp c00048
        lda #4
        sta vark
        lda #3
        sta varj
c00048: lda #5
        cmp vari
        beq *+5
        jmp c00081
        lda #6
        sta vark
        lda #5
        sta varj
c00081: lda #15
        cmp vari
        beq *+5
        jmp c00114
        lda #16
        sta vark
        lda #15
        sta varj
c00114: lda #7
        cmp vari
        beq *+5
        jmp c00147
        lda #8
        sta vark
        lda #7
        sta varj
c00147: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp c00366
        jsr cangl
        beq :+
        jmp c00348
:
        lda varj
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp c00222
        ; lda #0
        jsr animsp
c00222: ldy #9
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
        jmp c00307
:
        lda #1
        ldy #11
        sta (z80_ix),y
c00307: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp c00361
c00348: lda #1
        ldy #11
        sta (z80_ix),y
c00361: jmp c00558
c00366: jsr cangr
        beq :+
        jmp c00544
:
        lda vark
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp c00419
        ; lda #0
        jsr animsp
c00419: ldy #9
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
        jmp c00504
:
        lda #0
        ldy #11
        sta (z80_ix),y
c00504: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp c00558
c00544: lda #0
        ldy #11
        sta (z80_ix),y
c00558: lda #0
        jsr animsp
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00596
:
        lda #1
        sta deadf
c00596: rts
.out .sprintf("EVENT 2 SIZE = %d", (* - evnt02))

evnt03:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp d00082
        jsr cangu
        beq :+
        jmp d00064
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp d00078
d00064: lda #1
        ldy #11
        sta (z80_ix),y
d00078: jmp d00136
d00082: jsr cangd
        beq :+
        jmp d00123
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp d00136
d00123: lda #0
        ldy #11
        sta (z80_ix),y
d00136: lda #0
        jsr animsp
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00175
:
        lda #1
        sta deadf
d00175: rts
.out .sprintf("EVENT 3 SIZE = %d", (* - evnt03))

evnt04:
        lda #5
        cmp scno
        beq *+5
        jmp e00018
e00018: lda #10
        cmp scno
        beq *+5
        jmp e00058
        lda #13
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00058
e00058: lda #0
        cmp vara
        beq *+5
        jmp e00119
        ; lda #0
        jsr animsp
        lda #10
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp e00119
        lda #255
        ldy #5
        sta (z80_ix),y
e00119: lda #5
        cmp scno
        beq *+5
        jmp e00136
e00136: lda #13
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00194
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00194
:
        lda #255
        ldy #5
        sta (z80_ix),y
e00194: rts
.out .sprintf("EVENT 4 SIZE = %d", (* - evnt04))

evnt05:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp f00082
        jsr cangl
        beq :+
        jmp f00064
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp f00078
f00064: lda #1
        ldy #11
        sta (z80_ix),y
f00078: jmp f00136
f00082: jsr cangr
        beq :+
        jmp f00123
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp f00136
f00123: lda #0
        ldy #11
        sta (z80_ix),y
f00136: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00217
        jsr cangu
        beq :+
        jmp f00199
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp f00212
f00199: lda #1
        ldy #10
        sta (z80_ix),y
f00212: jmp f00271
f00217: jsr cangd
        beq :+
        jmp f00257
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp f00271
f00257: lda #0
        ldy #10
        sta (z80_ix),y
f00271: lda #0
        jsr animsp
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00309
:
        lda #1
        sta deadf
f00309: rts
.out .sprintf("EVENT 5 SIZE = %d", (* - evnt05))

evnt06:
        lda #4
        sta z80_c
        ldy #11
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #11
        sta (z80_ix),y
        lda #23
        ; ldy #11
        cmp (z80_ix),y
        bcs *+5
        jmp g00174
        ; ldy #11
        lda (z80_ix),y
        ldy #7
        sta (z80_ix),y
        lda #16
        dey; ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp g00116
        lda #3
        asl a
        sta sndtyp
g00116: lda #23
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp g00143
        jmp g00143
g00143: ldy #7
        lda (z80_ix),y
        lsr a
        lsr a
        ; ldy #7
        sta (z80_ix),y
        jmp g00261
g00174: lda #0
        ldy #7
        cmp (z80_ix),y
        bcc *+5
        jmp g00261
        ; lda #0
        cmp vara
        beq *+5
        jmp g00256
        ; lda #0
        jsr animbk
        lda #16
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp g00256
        lda #3
        asl a
        sta sndtyp
g00256: jmp g00261
g00261: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00345
:
        lda #13
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp g00310
        jmp g00345
g00310: lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp g00337
        jmp g00345
g00337: lda #1
        sta deadf
g00345: lda #1
        cmp scno
        beq *+5
        jmp g00452
        lda #13
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp g00452
        lda #9
        jsr gotob
        cmp #255
        beq :+
        jmp g00424
:
        lda #255
        ldy #5
        sta (z80_ix),y
        jmp g00452
g00424: lda #17
        ldy #8
        sta (z80_ix),y
        lda #219
        iny; ldy #9
        sta (z80_ix),y
g00452: lda #0
        cmp scno
        beq *+5
        jmp g00560
        lda #13
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp g00560
        lda #11
        jsr gotob
        cmp #255
        beq :+
        jmp g00532
:
        lda #255
        ldy #5
        sta (z80_ix),y
        jmp g00560
g00532: lda #41
        ldy #8
        sta (z80_ix),y
        lda #140
        iny; ldy #9
        sta (z80_ix),y
g00560: lda #24
        cmp scno
        beq *+5
        jmp g00668
        lda #13
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp g00668
        lda #8
        jsr gotob
        cmp #255
        beq :+
        jmp g00639
:
        lda #255
        ldy #5
        sta (z80_ix),y
        jmp g00668
g00639: lda #96
        ldy #8
        sta (z80_ix),y
        lda #210
        iny; ldy #9
        sta (z80_ix),y
g00668: lda #21
        cmp scno
        beq *+5
        jmp g00776
        lda #13
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp g00776
        lda #10
        jsr gotob
        cmp #255
        beq :+
        jmp g00747
:
        lda #255
        ldy #5
        sta (z80_ix),y
        jmp g00776
g00747: lda #50
        ldy #8
        sta (z80_ix),y
        lda #60
        iny; ldy #9
        sta (z80_ix),y
g00776: lda #20
        cmp scno
        beq *+5
        jmp g00884
        lda #13
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp g00884
        lda #7
        jsr gotob
        cmp #255
        beq :+
        jmp g00855
:
        lda #255
        ldy #5
        sta (z80_ix),y
        jmp g00884
g00855: lda #32
        ldy #8
        sta (z80_ix),y
        ; lda #32
        iny; ldy #9
        sta (z80_ix),y
g00884: rts
.out .sprintf("EVENT 6 SIZE = %d", (* - evnt06))

evnt07:
        lda #2
        cmp scno
        beq *+5
        jmp h00023
        jmp h00120
h00023: lda #0
        cmp vara
        beq *+5
        jmp h00085
        lda #7
        cmp vare
        beq *+5
        jmp h00069
        lda #1
        sta vare
        jmp h00085
h00069: lda vare
        clc
        adc #1
        sta vare
h00085: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp h00120
:
        lda #255
        ldy #5
        sta (z80_ix),y
h00120: rts
.out .sprintf("EVENT 7 SIZE = %d", (* - evnt07))

evnt08:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp i00116
        jsr cangd
        beq :+
        jmp i00064
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp i00111
i00064: lda #1
        ldy #11
        sta (z80_ix),y
        lda #4
        sta z80_c
        lda #12
        sta z80_b
        jsr spawn
        lda #10
        asl a
        sta sndtyp
i00111: jmp i00209
i00116: jsr cangu
        beq :+
        jmp i00156
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp i00209
i00156: lda #10
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i00209
        lda #1
        cmp varh
        beq *+5
        jmp i00209
        lda #0
        ldy #11
        sta (z80_ix),y
i00209: lda #11
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i00261
        lda #25
        cmp varh
        beq *+5
        jmp i00261
        lda #0
        ldy #11
        sta (z80_ix),y
i00261: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i00291
:
        lda #1
        sta deadf
i00291: rts
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
        lda #11
        jsr gotob
        cmp #255
        beq :+
        jmp k00203
:
        lda #22
        sta chary
        lda #17
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #128
        jsr pattr
        lda #22
        sta chary
        lda #14
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #128
        jsr pattr
        lda #21
        sta chary
        lda #17
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #128
        jsr pattr
        lda #21
        sta chary
        lda #14
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #128
        jsr pattr
        jmp k00307
k00203: lda #2
        cmp scno
        beq *+5
        jmp k00307
        lda #17
        sta chary
        lda #0
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #129
        jsr pattr
        lda #18
        sta chary
        lda #0
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #129
        jsr pattr
k00307: lda #7
        jsr gotob
        cmp #255
        beq :+
        jmp k00403
:
        lda #11
        jsr gotob
        cmp #255
        beq :+
        jmp k00355
:
        jmp k00399
k00355: lda #22
        sta chary
        lda #14
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #106
        jsr pattr
k00399: jmp k00595
k00403: lda #14
        cmp scno
        beq *+5
        jmp k00595
        lda #16
        sta chary
        lda #26
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #104
        jsr pattr
        lda #16
        sta chary
        lda #27
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #104
        jsr pattr
        lda #16
        sta chary
        lda #28
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #104
        jsr pattr
        lda #16
        sta chary
        lda #29
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #104
        jsr pattr
k00595: lda #10
        jsr gotob
        cmp #255
        beq :+
        jmp k00692
:
        lda #11
        jsr gotob
        cmp #255
        beq :+
        jmp k00644
:
        jmp k00687
k00644: lda #21
        sta chary
        lda #14
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #108
        jsr pattr
k00687: jmp k00840
k00692: lda #6
        cmp scno
        beq *+5
        jmp k00840
        lda #16
        sta chary
        lda #11
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #110
        jsr pattr
        lda #16
        sta chary
        lda #12
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #110
        jsr pattr
        lda #16
        sta chary
        lda #13
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #110
        jsr pattr
k00840: lda #9
        jsr gotob
        cmp #255
        beq :+
        jmp k00937
:
        lda #11
        jsr gotob
        cmp #255
        beq :+
        jmp k00888
:
        jmp k00932
k00888: lda #21
        sta chary
        lda #17
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #107
        jsr pattr
k00932: jmp k01083
k00937: lda #14
        cmp scno
        beq *+5
        jmp k01083
        lda #7
        sta chary
        lda #1
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #111
        jsr pattr
        lda #8
        sta chary
        lda #1
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #111
        jsr pattr
        lda #9
        sta chary
        lda #1
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #111
        jsr pattr
k01083: lda #8
        jsr gotob
        cmp #255
        beq :+
        jmp k01180
:
        lda #11
        jsr gotob
        cmp #255
        beq :+
        jmp k01131
:
        jmp k01175
k01131: lda #22
        sta chary
        lda #17
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #105
        jsr pattr
k01175: jmp k01284
k01180: lda #8
        cmp scno
        beq *+5
        jmp k01284
        ; lda #8
        sta chary
        lda #30
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #109
        jsr pattr
        lda #9
        sta chary
        lda #30
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #109
        jsr pattr
k01284: lda #50
        cmp varh
        beq *+5
        jmp k01314
        lda #0
        sta varh
        jmp k01329
k01314: lda varh
        clc
        adc #1
        sta varh
k01329: lda #1
        cmp vara
        beq *+5
        jmp k01359
        lda #0
        sta vara
        jmp k01374
k01359: lda vara
        clc
        adc #1
        sta vara
k01374: rts
.out .sprintf("EVENT 10 SIZE = %d", (* - evnt10))

evnt11:
        rts
.out .sprintf("EVENT 11 SIZE = %d", (* - evnt11))

evnt12:
        jsr cls
        lda #2
        sta chary
        ; lda #2
        sta charx
        lda #17
        jsr dmsg
        lda #3
        sta chary
        lda #2
        sta charx
        lda #18
        jsr dmsg
        lda #4
        sta chary
        lda #2
        sta charx
        lda #18
        jsr dmsg
        lda #5
        sta chary
        lda #2
        sta charx
        lda #18
        jsr dmsg
        lda #6
        sta chary
        lda #2
        sta charx
        lda #17
        jsr dmsg
        lda #3
        sta chary
        lda #10
        sta charx
        lda #14
        jsr dmsg
        lda #4
        sta chary
        lda #12
        sta charx
        lda #15
        jsr dmsg
        lda #5
        sta chary
        ; lda #5
        sta charx
        lda #16
        jsr dmsg
        lda #9
        sta chary
        lda #10
        sta charx
        lda #8
        jsr dmsg
        lda #11
        sta chary
        lda #10
        sta charx
        lda #9
        jsr dmsg
        lda #13
        sta chary
        lda #10
        sta charx
        ; lda #10
        jsr dmsg
        lda #20
        sta chary
        lda #0
        sta charx
        lda #13
        jsr dmsg
        lda #20
        sta chary
        lda #13
        sta charx
        lda #12
        jsr dmsg
        lda #20
        sta chary
        ; lda #20
        sta charx
        lda #11
        jsr dmsg
        lda #16
        sta chary
        lda #2
        sta charx
        lda #19
        jsr dmsg
        lda #17
        sta chary
        lda #1
        sta charx
        lda #20
        jsr dmsg
        lda #18
        sta chary
        lda #8
        sta charx
        lda #21
        jsr dmsg
        jsr prskey
        rts
.out .sprintf("EVENT 12 SIZE = %d", (* - evnt12))

evnt13:
        lda #0
        sta varb
        ; lda #0
        sta varc
        ; lda #0
        sta varf
        ; lda #0
        sta varh
        lda #1
        sta vard
        lda #6
        sta numlif
        lda #130
        sta varo
        lda #24
        sta varp
        lda #20
        sta chary
        lda #0
        sta charx
        lda #2
        jsr dmsg
        lda #22
        sta chary
        lda #23
        sta charx
        jsr dscor
        lda #22
        sta chary
        lda #6
        sta charx
        lda numlif
        jsr disply
        lda #21
        sta chary
        lda #15
        sta charx
        lda #3
        jsr dmsg
        lda #22
        sta chary
        lda #15
        sta charx
        lda #4
        jsr dmsg
        lda #21
        sta chary
        lda #19
        sta charx
        lda #5
        jsr dmsg
        lda #22
        sta chary
        lda #19
        sta charx
        lda #5
        jsr dmsg
        lda #21
        sta chary
        lda #1
        sta charx
        lda #7
        jsr dmsg
        lda #22
        sta chary
        lda #1
        sta charx
        lda #7
        jsr dmsg
        lda #21
        sta chary
        lda #30
        sta charx
        lda #6
        jsr dmsg
        lda #22
        sta chary
        lda #30
        sta charx
        lda #6
        jsr dmsg
        lda #21
        sta chary
        lda #11
        sta charx
        lda #6
        jsr dmsg
        lda #22
        sta chary
        lda #11
        sta charx
        lda #6
        jsr dmsg
        rts
.out .sprintf("EVENT 13 SIZE = %d", (* - evnt13))

evnt14:
        lda #0
        sta vara
        rts
.out .sprintf("EVENT 14 SIZE = %d", (* - evnt14))

evnt15:
        lda #0
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp p00032
        lda #1
        sta deadf
p00032: rts
.out .sprintf("EVENT 15 SIZE = %d", (* - evnt15))

evnt16:
        lda #100
        asl a
        sta sndtyp
        lda numlif
        sec
        sbc #1
        sta numlif
        lda #22
        sta chary
        lda #6
        sta charx
        lda numlif
        jsr disply
        rts
.out .sprintf("EVENT 16 SIZE = %d", (* - evnt16))

evnt17:
        lda #50
        jsr delay
        rts
.out .sprintf("EVENT 17 SIZE = %d", (* - evnt17))

evnt18:
        jsr cls
        lda #50
        jsr delay
        lda #4
        sta chary
        lda #9
        sta charx
        lda #22
        jsr dmsg
        lda #50
        jsr delay
        lda #6
        sta chary
        lda #1
        sta charx
        lda #23
        jsr dmsg
        lda #50
        jsr delay
        lda #8
        sta chary
        lda #1
        sta charx
        lda #24
        jsr dmsg
        lda #50
        jsr delay
        lda #10
        sta chary
        lda #0
        sta charx
        lda #25
        jsr dmsg
        lda #50
        jsr delay
        lda #12
        sta chary
        lda #0
        sta charx
        lda #26
        jsr dmsg
        lda #150
        jsr delay
        lda #14
        sta chary
        lda #12
        sta charx
        lda #27
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
        .byte "GET READY",141
        .byte "abbbbbbbbbbbbbbbbbbbbbbbbbbbbbba",13
        .byte "ml  SOULS  kjn gh oil  RELICS km",13
        .byte "ml         kjn ef oil         km",13
        .byte "abbbbbbbbbbbbbbbbbbbbbbbbbbbbbba",13
        .byte 141
        .byte "g",141
        .byte "e",141
        .byte "i",141
        .byte "k",141
        .byte "l",141
        .byte "1 KEYBOARD",141
        .byte "2 KEMPSTON",141
        .byte "3 SINCLAIR",141
        .byte "BUBBLESOFT",141
        .byte "2017",141
        .byte "ANDY JOHNS",141
        .byte "MONTY MOLE",141
        .byte "AND THE",141
        .byte "TEMPLE OF LOST SOULS",141
        .byte "abbbbbbbbbbbbbbbbbbbbbbbba",141
        .byte "mppppppppppppppppppppppppm",141
        .byte "HELP MONTY ESCAPE THE TOMB",141
        .byte "FIND KEYS AND COLLECT RELICS",141
        .byte "ALONG THE WAY",141
        .byte "CONGRATULATIONS!!",141
        .byte "YOU HAVE GUIDED MONTY BACK TO",141
        .byte "THE ENTRANCE. MAYBE NEXT TIME",141
        .byte "HE WILL STAY WITH THE TOUR GUIDE",13
        .byte 141
        .byte "AND NOT GET INTO SO MUCH TROUBLE",13
        .byte 141
        .byte "AS IF......",141
nummsg:
        .byte 28
.out .sprintf("MESSAGE SIZE = %d", (* - msgdat))

chgfx:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 128,145,80,98,228,228,186,114,213,213,250,250,245,245,80,80
        .byte 0,0,0,0,0,0,0,136,0,0,0,0,0,0,0,0
        .byte 0,0,234,0,117,128,50,200,17,228,16,234,0,245,0,250
        .byte 0,245,0,250,17,228,16,234,49,196,114,136,228,0,0,0
        .byte 0,0,250,250,0,0,186,80,160,32,0,0,245,245,0,0
        .byte 0,0,114,234,0,32,80,34,17,160,64,34,32,160,0,0
        .byte 85,32,64,170,68,32,80,170,68,32,80,170,85,32,64,34
        .byte 0,0,0,0,32,128,64,0,0,0,64,0,0,0,0,0
        .byte 85,100,186,250,117,177,170,234,117,177,186,234,85,85,34,170
        .byte 177,245,114,186,160,245,114,250,177,100,80,250,160,160,80,80
        .byte 245,213,186,114,245,245,250,186,213,228,250,250,160,160,80,64
        .byte 228,228,186,250,228,213,250,250,213,160,250,234,245,117,186,250
        .byte 49,177,98,250,177,228,114,114,177,245,114,216,160,245,114,186
        .byte 85,100,186,250,117,177,170,234,117,177,186,234,85,85,34,170
        .byte 177,245,114,186,160,245,114,250,177,100,80,250,160,160,80,80
        .byte 245,213,186,114,245,245,250,186,213,228,250,250,160,160,80,64
        .byte 228,228,186,250,228,213,250,250,213,160,250,234,245,117,186,250
        .byte 49,177,98,250,177,228,114,114,177,245,114,216,160,245,114,186
        .byte 85,228,34,200,85,228,34,234,0,0,80,170,245,245,0,0
        .byte 177,85,250,250,85,160,0,0,85,196,34,234,85,196,34,234
        .byte 85,228,34,200,85,228,34,234,85,196,34,234,85,196,34,234
        .byte 0,0,0,0,17,0,34,136,17,68,34,234,85,196,34,234
        .byte 85,228,34,200,85,196,34,200,17,68,0,136,0,0,0,0
        .byte 0,0,5,170,0,0,170,171,93,85,0,0,85,10,0,0
        .byte 228,213,250,250,0,0,200,170,85,32,0,0,245,245,170,114
        .byte 32,0,64,0,36,164,24,64,164,0,80,72,164,160,64,0
        .byte 0,4,16,2,32,160,0,66,37,33,24,10,32,165,0,2
        .byte 0,0,0,0,0,0,0,0,0,4,64,8,32,129,2,10
        .byte 0,0,136,0,128,0,64,0,196,0,186,0,100,128,186,98
        .byte 0,0,0,16,0,17,0,50,0,85,0,250,17,160,98,216
        .byte 0,85,0,186,0,85,0,186,0,0,0,186,0,85,0,186
        .byte 196,0,234,0,196,0,234,0,0,0,234,0,196,0,234,0
        .byte 17,85,34,114,17,117,34,50,17,49,34,186,17,17,0,136
        .byte 17,0,16,0,32,128,16,136,32,196,16,114,100,245,16,186
        .byte 0,128,0,136,17,68,16,136,49,68,234,136,245,100,216,136
        .byte 160,128,234,64,228,128,234,64,196,128,216,64,128,128,16,0
        .byte 0,85,250,50,0,85,152,50,160,68,0,50,245,85,0,50
        .byte 196,0,234,250,196,0,234,170,0,145,234,0,196,245,234,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,68,0,170,152
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,85,80,50,0,85,16,50,0,68,0,50,85,85,0,50
        .byte 196,0,234,170,196,0,234,170,0,128,234,0,196,160,234,0
        .byte 117,245,34,50,17,17,0,136,68,68,98,34,49,245,0,0
        .byte 117,196,64,34,68,32,64,0,68,0,0,0,117,228,0,0
        .byte 49,228,64,34,68,32,64,34,68,32,0,34,117,196,0,0
        .byte 117,196,64,34,68,32,64,34,85,68,64,136,68,100,0,0
        .byte 17,0,16,0,17,0,16,0,17,0,16,136,32,68,0,0
        .byte 32,0,34,0,32,0,34,0,32,0,16,0,0,196,0,0
        .byte 117,228,34,34,17,0,0,200,17,0,34,0,117,228,0,0
        .byte 196,128,64,98,68,32,64,34,68,32,64,34,49,196,0,0
        .byte 128,145,80,98,228,228,186,114,213,213,250,250,245,245,80,80
        .byte 228,213,250,250,0,0,200,170,85,32,0,0,245,245,170,114
        .byte 0,0,170,170,0,0,186,80,160,32,0,0,85,85,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,128,64,34,0,0,152,0,0,32,64,0,17,17,136,64
        .byte 128,145,80,98,228,228,186,114,213,213,250,250,245,245,80,80
        .byte 245,213,186,50,196,228,98,250,228,177,234,234,177,117,80,64
        .byte 228,245,186,216,213,32,98,250,228,245,216,216,213,245,80,98
        .byte 1,1,2,64,1,1,0,2,0,5,0,64,0,1,0,64
        .byte 32,128,8,8,160,0,0,8,128,0,2,0,32,0,0,0
        .byte 0,4,0,80,0,4,0,16,0,5,0,64,0,0,0,64
        .byte 32,0,8,0,32,0,2,0,128,0,2,0,0,0,2,0
        .byte 0,0,0,0,2,8,4,0,0,0,4,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,68,0,170,152
        .byte 2,8,2,0,0,8,2,8,2,0,0,8,0,8,0,8
        .byte 0,0,0,136,68,0,0,16,0,0,136,64,0,0,16,0
        .byte 0,0,0,8,4,0,0,1,0,0,8,4,0,0,1,0
        .byte 0,0,15,15,0,0,11,5,10,2,0,0,15,15,0,0
        .byte 0,0,7,14,0,2,5,2,1,10,4,2,2,10,0,0
        .byte 5,2,4,10,4,2,5,10,4,2,5,10,5,2,4,2
        .byte 5,6,11,15,7,11,10,14,7,11,11,14,5,5,2,10
        .byte 11,15,7,11,10,15,7,15,11,6,5,15,10,10,5,5
        .byte 15,13,11,7,15,15,15,11,13,14,15,15,10,10,5,4
        .byte 14,14,11,15,14,13,15,15,13,10,15,14,15,7,11,15
        .byte 3,11,6,15,11,14,7,7,11,15,7,13,10,15,7,11
        .byte 5,14,2,12,5,14,2,14,0,0,5,10,15,15,0,0
        .byte 11,5,15,15,5,10,0,0,5,12,2,14,5,12,2,14
        .byte 5,14,2,12,5,14,2,14,5,12,2,14,5,12,2,14
        .byte 0,0,0,0,1,0,2,8,1,4,2,14,5,12,2,14
        .byte 5,14,2,12,5,12,2,12,1,4,0,8,0,0,0,0
        .byte 14,13,15,15,0,0,12,10,5,2,0,0,15,15,10,7
        .byte 8,9,5,6,14,14,11,7,13,13,15,15,15,15,5,5
        .byte 0,8,4,2,0,0,9,0,0,2,4,0,1,1,8,4
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,5,0,11,0,5,0,11,0,0,0,11,0,5,0,11
        .byte 12,0,14,0,12,0,14,0,0,0,14,0,12,0,14,0
        .byte 0,5,0,10,0,1,0,2,0,0,0,2,0,1,0,0
        .byte 12,0,10,0,4,0,10,0,0,0,8,0,0,0,0,0
        .byte 11,11,15,15,5,10,1,0,0,0,0,0,0,0,0,0
        .byte 11,11,15,15,5,6,11,13,6,10,2,8,0,8,0,0
        .byte 255,255,153,34,170,68,221,68,136,136,187,68,204,0,136,0
        .byte 255,255,68,153,34,85,34,187,17,17,34,221,0,51,0,17
        .byte 160,160,186,80,160,160,186,250,128,0,216,80,100,0,234,0
        .byte 160,160,80,80,160,160,250,250,0,0,80,80,0,0,0,0
        .byte 160,213,80,80,160,213,250,216,0,17,80,80,0,100,0,114
        .byte 245,245,0,0,245,245,170,170,0,0,250,250,245,245,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,216,216,177,177,0,0
        .byte 117,245,152,80,196,177,80,200,49,0,80,136,245,245,0,0
        .byte 245,245,80,200,228,0,0,0,0,0,0,0,0,0,0,0
        .byte 245,245,50,170,0,117,0,0,0,0,0,0,0,0,0,0
        .byte 245,196,80,34,228,117,50,98,0,196,16,98,245,245,0,0
        .byte 245,181,90,90,165,245,218,90,165,245,218,90,165,165,218,250
        .byte 0,0,48,192,64,96,176,208,64,96,32,192,16,128,0,0
        .byte 0,0,18,72,4,36,26,88,4,36,2,72,1,128,0,0
        .byte 0,0,33,132,64,66,161,133,64,66,32,132,16,8,0,0
        .byte 0,0,35,140,68,70,171,141,68,70,34,140,17,8,0,0
        .byte 241,245,248,248,240,240,250,248,240,241,242,248,241,245,248,240
        .byte 215,245,175,175,245,95,190,250,125,95,175,175,95,95,250,190
        .byte 240,95,173,180,121,214,173,180,241,211,165,233,240,91,188,188
        .byte 17,255,34,85,102,221,238,102,102,221,34,85,17,255,0,0
        .byte 255,255,85,17,85,187,221,187,85,187,85,187,255,255,0,0
        .byte 5,6,11,15,7,11,10,14,7,11,11,14,5,5,2,10
        .byte 11,15,7,11,10,15,7,15,11,6,5,15,10,10,5,5
        .byte 15,13,11,7,15,15,15,11,13,14,15,15,10,10,5,4
        .byte 14,14,11,15,14,13,15,15,13,10,15,14,15,7,11,15
        .byte 3,11,6,15,11,14,7,7,11,15,7,13,10,15,7,11
        .byte 0,5,15,3,0,5,9,3,10,4,0,3,15,5,0,3
        .byte 12,0,14,15,12,0,14,10,0,9,14,0,12,15,14,0
        .byte 6,10,11,15,7,1,14,14,9,7,7,6,15,9,5,6
        .byte 5,6,15,7,11,4,4,15,14,11,6,14,15,7,5,2
        .byte 0,0,0,0,0,0,0,0,48,128,64,64,48,128,16,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,50,200,68,100,186,216,68,100,34,200,17,128,0,0
        .byte 85,160,250,250,196,49,114,234,196,117,250,50,85,228,234,114
        .byte 128,17,64,34,228,100,170,50,196,85,170,114,228,49,64,80
        .byte 0,0,10,10,0,0,11,5,10,2,0,0,5,5,0,0
        .byte 14,13,15,15,0,0,12,10,5,2,0,0,15,15,10,7
        .byte 228,213,186,186,0,0,200,170,85,32,0,0,213,160,170,114
        .byte 0,0,112,224,0,32,80,32,16,160,64,32,32,160,0,0
        .byte 0,0,37,164,0,2,5,32,16,10,4,32,2,10,0,0
        .byte 0,0,82,74,0,32,80,2,1,160,64,2,32,160,0,0
        .byte 0,0,39,174,0,2,5,34,17,10,4,34,2,10,0,0
bprop:
        .byte 0
        .byte 0
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 1
        .byte 2
        .byte 2
        .byte 3
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
        .byte 0
        .byte 0
        .byte 1
        .byte 1
        .byte 0
        .byte 2
        .byte 1
        .byte 1
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 1
        .byte 1
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
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 1
        .byte 2
        .byte 0
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
        .byte 1
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 1
        .byte 1
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 1
        .byte 0
        .byte 0
        .byte 0
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
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 0
        .byte 1
        .byte 1
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
.out .sprintf("BLOCKS SIZE = %d", (* - chgfx))

sprgfx:
        .byte 0,0,3,192,7,224,15,240,15,244,7,236,31,252,63,248,63,248,47,252,31,252,63,252,63,252,63,248,30,48,12,0
        .byte 0,0,3,192,7,224,15,240,15,240,7,236,31,252,63,252,63,252,15,240,31,248,63,252,63,252,63,252,31,248,12,48
        .byte 0,0,3,192,7,224,15,240,47,240,55,224,63,248,31,252,31,252,63,244,63,248,63,252,63,252,31,252,12,120,0,48
        .byte 0,0,3,192,7,224,15,240,15,240,55,224,63,248,63,252,63,252,15,240,31,248,63,252,63,252,63,252,31,248,12,48
        .byte 0,0,7,128,9,192,27,224,127,208,55,56,0,216,11,92,27,92,22,252,22,220,15,60,7,248,1,248,0,240,7,224
        .byte 0,0,7,128,9,192,27,224,127,208,51,56,0,220,31,108,59,118,55,190,15,182,15,204,7,222,55,190,31,28,14,120
        .byte 0,0,7,128,9,192,27,224,127,208,55,56,0,216,11,92,27,92,22,252,22,220,15,60,7,248,1,248,0,240,7,224
        .byte 0,0,7,128,9,192,27,224,127,208,55,56,0,220,3,92,2,222,5,254,13,190,14,124,7,126,55,190,31,28,14,120
        .byte 0,0,1,224,3,144,7,216,11,254,28,236,27,0,58,208,58,216,63,104,59,104,60,240,31,224,31,128,15,0,7,224
        .byte 0,0,1,224,3,144,7,216,11,254,28,204,59,0,54,220,110,220,125,236,109,240,51,240,123,224,125,236,56,248,30,112
        .byte 0,0,1,224,3,144,7,216,11,254,28,204,27,0,58,208,58,216,63,104,59,104,60,240,31,224,31,128,15,0,7,224
        .byte 0,0,1,224,3,144,7,216,11,254,28,236,59,0,58,192,123,64,127,160,125,176,62,112,126,224,125,236,56,248,30,112
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,30,0,25,0,33,3,2,5,110,103,108,16,0,109,36,1,34
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,28,0,26,0,33,0,1,3,2,5,110,7,108,112,0,108,164,0,148
        .byte 0,0,0,0,0,0,0,0,0,0,0,24,0,28,0,34,0,1,3,1,5,66,103,110,16,44,110,0,0,164,0,104
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,28,0,26,0,33,0,1,3,2,5,110,103,108,112,0,12,164,0,196
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,120,0,152,0,132,0,64,192,118,160,54,230,0,8,36,182,68,128
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,56,0,88,0,132,0,128,0,64,192,118,160,54,224,0,14,37,54,41,0
        .byte 0,0,0,0,0,0,0,0,0,0,24,0,56,0,68,0,128,0,128,192,66,160,118,230,52,8,0,118,37,0,22,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,56,0,88,0,132,0,128,0,64,192,118,160,54,230,0,14,37,48,35,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,118,2,214,219,240,216,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,0,118,192,208,218,240,27,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,192,118,216,214,26,240,3,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,24,112,218,214,195,246,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,112,27,214,216,246,192,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,110,219,107,27,15,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,96,3,110,91,11,216,15,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,27,110,88,107,192,15,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,24,0,91,14,195,107,0,111,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,0,216,14,27,107,3,111,0,0
        .byte 0,0,11,32,21,144,118,192,126,224,29,244,0,2,1,224,239,180,111,186,14,108,9,250,0,177,0,104,0,36,0,16
        .byte 0,0,11,96,21,128,118,224,126,246,29,192,0,0,1,224,15,182,239,184,103,108,4,251,0,176,0,108,0,32,0,24
        .byte 0,0,11,64,21,129,118,250,126,240,29,192,0,0,1,226,15,180,239,185,103,110,2,248,0,190,0,96,0,48,0,0
        .byte 0,0,11,64,21,128,118,224,126,246,29,192,0,0,1,224,15,182,239,184,103,108,4,251,0,176,0,108,0,32,0,24
        .byte 0,0,4,208,9,168,3,110,7,126,47,184,64,0,7,128,45,247,93,246,54,112,95,144,141,0,22,0,36,0,8,0
        .byte 0,0,6,208,1,168,7,110,111,126,3,184,0,0,7,128,109,240,29,247,54,230,223,32,13,0,54,0,4,0,24,0
        .byte 0,0,2,208,129,168,95,110,15,126,3,184,0,0,71,128,45,240,157,247,118,230,31,64,125,0,6,0,12,0,0,0
        .byte 0,0,2,208,1,168,7,110,111,126,3,184,0,0,7,128,109,240,29,247,54,230,223,32,13,0,54,0,4,0,24,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,189,119,0,0,178,77,166,165,0,0,247,223,0,0
        .byte 109,220,255,247,119,127,187,254,122,247,187,207,125,187,190,127,127,110,175,191,126,191,191,187,119,222,191,223,85,85,42,138
        .byte 109,220,255,247,119,127,187,254,122,247,187,207,125,187,190,127,127,110,175,191,126,191,191,187,119,222,191,223,85,85,42,138
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,164,82
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,132,66,32,18,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,132,66,0,2,32,16,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,132,66,0,2,32,16,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,132,66,0,18,32,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,2,132,64,0,18,32,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,2,132,64,0,18,32,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,2,4,0,128,82,32,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,128,18
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,40,0,16,0,40,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,68,0,40,0,16,0,40,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,2,0,68,0,40,0,16,0,40,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 16,0,8,0,28,0,36,0,52,0,24,0,0,0,36,0,24,0,16,0,8,0,24,0,16,0,0,0,0,0,0,0
        .byte 8,0,8,0,24,0,52,0,36,0,24,0,0,0,36,0,24,0,16,0,8,0,24,0,16,0,0,0,0,0,0,0
        .byte 8,0,16,0,56,0,44,0,36,0,24,0,0,0,36,0,24,0,16,0,8,0,24,0,16,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,0,128,33,130
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,0,128,0,128,32,128,32,130,33,130,32,130,97,130
        .byte 0,0,0,0,0,0,0,0,0,128,0,128,0,128,32,128,32,130,33,130,32,130,97,130,32,195,97,194,32,195,97,195
        .byte 0,128,0,128,0,128,32,128,32,130,33,130,32,130,97,130,32,195,97,194,32,195,97,194,97,195,97,195,97,195,97,195
        .byte 0,128,0,128,0,128,32,128,32,130,33,130,32,130,97,130,32,195,97,194,32,195,97,194,97,195,97,195,97,195,97,195
frmlst:
        .byte 0,4
        .byte 4,4
        .byte 8,4
        .byte 12,4
        .byte 16,4
        .byte 20,5
        .byte 25,5
        .byte 30,4
        .byte 34,4
        .byte 38,1
        .byte 39,1
        .byte 40,1
        .byte 41,11
        .byte 52,6
        .byte 58,3
        .byte 61,6,67,0
.out .sprintf("SPRITES SIZE = %d", (* - sprgfx))

scdat:
        .word 495,485,216,247,232,306,338,443,475,462,496,459,375,434,478,188,465,491,378,521,431,351,258,375,470,177
        .word 231,487,156
        .byte 255,0,5,67,0,67,0,67,0,67,0,67,0,19,18,11,63,19,18,67,0,67,0,255,56,5,255,0,6,67,0,255,56,8
        .byte 15,16,12,0,63,11,17,15,56,56,56,19,18,19,18,56,67,255,0,4,67,0,56,19,19,59,10,19,18,19,18,19,12
        .byte 255,0,4,11,59,10,19,18,10,12,16,17,56,0,67,67,0,67,0,56,19,13,10,12,60,10,12,10,12,10,0,0,35
        .byte 36,0,63,12,61,10,12,0,60,10,19,18,56,0,0,56,56,56,15,11,12,0,0,62,255,0,4,61,0,0,34,37,0,0,0
        .byte 63,0,0,0,62,0,11,17,56,56,56,15,15,56,19,13,62,0,0,62,0,86,0,0,63,0,0,127,255,0,6,86,0
        .byte 62,0,10,106,19,59,19,18,19,18,15,12,255,0,5,85,255,0,5,127,127,255,0,5,85,0,0,0,11,19,59,17,10
        .byte 12,10,10,12,255,0,12,38,39,255,0,9,62,11,17,15,255,0,17,32,33,255,0,10,14,18,56,0,255,65,4,255,0,10
        .byte 7,6,38,39,6,7,255,0,8,11,17,56,14,13,14,13,14,58,255,0,8,7,38,39,38,39,38,39,7,255,0,8,58
        .byte 18,16,17,16,59,16,10,27,255,0,6,7,6,38,39,38,39,38,39,6,7,255,0,4,127,112,113,11,17,56,15,56,56
        .byte 105,19,13,0,0,38,39,0,7,38,39,38,39,38,39,38,39,38,39,7,0,38,39,0,0,14,18,56,0,56,0,67,56
        .byte 16,12,0,0,7,6,7,6,38,39,38,39,38,39,38,39,38,39,6,7,6,7,27,0,11,17,67,67,0,67,0,56,19
        .byte 13,14,13,14,58,10,14,58,14,13,14,58,13,15,14,13,15,14,58,14,13,10,10,10,56,255,0,4,67,56,16,17,16
        .byte 59,16,17,56,16,17,16,17,19,18,59,56,16,17,10,16,17,19,59,56,16,17,56,67,255,0,4,67,255,56,6,0,255,56,4
        .byte 16,17,17,56,56,56,15,56,56,16,17,56,56,56,67,255,0,6,67,0,67,0,67,0,67,0,67,0,67,56,56,56,67
        .byte 0,67,56,67,0,56,56,67,0,67,255,0,18,67,0,67,0,0,0,67,0,67,0,67,255,0,37
        .byte 41,114,118,117,72,121,117,73,74,120,73,74,118,121,72,73,118,121,41,73,74,72,119,120,72,41,73,122,72,74,72,84,118
        .byte 121,73,122,1,73,74,1,87,88,1,1,73,74,72,1,73,74,255,1,4,87,88,255,1,6,76,117,115,74,255,1,6,87
        .byte 88,255,1,12,87,88,1,1,1,124,0,1,122,116,84,70,255,1,6,87,88,255,1,12,87,88,1,1,1,124,124,1,72
        .byte 84,114,72,255,1,6,87,88,255,1,12,87,88,1,1,1,119,120,1,76,117,118,75,255,1,6,87,88,255,1,9,76,122
        .byte 1,87,88,1,1,1,87,88,1,73,121,115,74,70,64,64,70,69,70,69,69,70,122,72,76,75,76,76,75,72,115,116,255,69,5
        .byte 70,69,69,70,114,84,121,75,1,64,64,1,1,1,119,72,73,74,72,73,122,1,16,59,16,118,121,255,84,10,1,115,122
        .byte 1,64,64,1,1,1,87,88,255,1,5,0,60,0,63,16,116,114,84,68,1,68,1,68,1,68,1,68,114,71,1,64,64
        .byte 1,1,1,87,88,255,1,4,0,0,62,0,0,86,19,117,84,85,68,255,1,7,118,122,1,64,64,1,1,1,87,88,255,1,4
        .byte 255,0,5,85,59,116,84,68,255,1,8,115,74,1,64,64,1,1,1,87,88,255,1,4,255,0,5,19,121,84,68,85,68
        .byte 255,1,7,84,72,1,64,64,1,1,1,87,88,255,1,5,255,0,4,16,116,114,84,68,255,1,8,118,70,131,82,82,132,132
        .byte 76,75,76,122,72,131,132,82,9,9,26,53,82,114,84,68,85,255,1,8,115,121,118,117,114,118,121,115,121,115,116,118,117
        .byte 118,121,64,64,118,117,84,84,68,85,68,255,1,8,84,84,115,116,84,115,116,255,84,4,121,116,115,116,64,64,115,116,84
        .byte 68,1,68,85,255,1,9,68,84,84,68,84,84,68,85,68,1,255,84,4,64,64,84,84,68,1,68,85,68,255,1,8,68
        .byte 1,68,1,68,1,68,1,68,1,68,1,68,1,84,64,64,84,68,1,68,1,68,85,255,1,21,68,84,64,64,68,1,68
        .byte 255,1,12,255,86,12,68,86,84,64,64,84,68,86,68,255,86,11
        .byte 255,0,13,3,255,0,13,3,255,0,6,3,255,0,15,3,0,0,0,4,255,0,8,3,255,0,22,5,255,0,9,3,255,0,9
        .byte 3,255,0,38,3,0,0,3,255,0,15,3,255,0,20,3,255,0,16,3,255,0,19,3,255,0,21,3,255,0,11,10,255,0,27
        .byte 3,0,0,0,14,13,255,0,15,3,255,0,5,3,255,0,8,11,12,10,0,0,3,255,0,9,3,255,0,14,3,0,19
        .byte 18,14,13,0,0,0,41,255,0,24,16,17,11,12,10,0,0,3,255,0,24,18,19,18,15,14,13,0,41,255,0,20,3
        .byte 0,0,0,17,16,17,15,11,12,10,41,255,0,24,19,18,19,18,19,18,14,13,255,0,24,7,255,54,5,112,113,255,0,30
        .byte 42,33,255,0,22,14,13,255,0,6,42,33,0,0,0,28,10,10,255,0,12,10,30,0,29,11,12,7,130,255,52,5,255,2,4
        .byte 52,52,255,2,13,52,2,2,2,52,52
        .byte 255,0,13,3,255,0,13,3,255,0,6,3,255,0,15,3,0,0,0,4,255,0,8,3,255,0,22,5,255,0,9,3,255,0,9
        .byte 3,255,0,38,3,0,0,3,255,0,15,3,255,0,20,3,255,0,16,3,255,0,19,3,255,0,34,3,255,0,26,3,255,0,20
        .byte 3,255,0,5,3,255,0,13,3,255,0,24,3,255,0,40,3,255,0,5,133,26,26,53,255,26,4,53,255,0,10,3,255,0,13
        .byte 21,255,0,5,21,255,0,7,3,255,0,10,133,26,26,53,0,0,0,22,255,0,5,22,255,0,5,23,255,0,13,21,255,0,5
        .byte 22,255,0,5,22,255,0,5,22,255,0,13,22,255,0,5,22,255,0,5,22,255,0,5,22,255,0,5,14,13,0,14,13
        .byte 0,0,0,24,0,14,13,0,0,22,255,0,5,22,255,0,5,22,255,0,5,11,17,29,16,12,27,0,0,31,10,16,12
        .byte 0,0,20,255,0,5,20,255,0,5,20,0,26,0,0,0,52,2,52,52,255,2,5,52,52,2,2,2,52,255,2,5,52
        .byte 255,2,6,52,2,52,2,2
        .byte 255,0,13,3,255,0,13,3,255,0,6,3,255,0,15,3,0,0,0,4,255,0,8,3,255,0,22,5,255,0,9,3,255,0,9
        .byte 3,255,0,38,41,0,0,3,0,0,41,255,0,12,3,255,0,20,3,255,0,16,3,255,0,19,3,255,0,12,41,255,0,8
        .byte 3,255,0,12,3,255,0,26,3,255,0,40,3,255,0,9,3,255,0,14,3,255,0,40,3,255,0,11,133,53,26,0,0,0
        .byte 133,53,26,255,0,4,3,255,0,19,21,255,0,5,21,0,3,255,0,16,133,53,26,255,0,4,22,255,0,5,22,255,0,19
        .byte 21,255,0,5,22,255,0,5,22,255,0,14,10,255,0,4,22,255,0,5,22,255,0,5,22,255,0,13,14,13,27,0,0,0
        .byte 22,255,0,5,22,255,0,5,22,255,0,7,29,0,0,28,10,10,16,17,10,0,0,0,20,255,0,5,20,255,0,5,20
        .byte 0,10,29,0,0,255,2,5,255,52,5,255,2,4,52,255,2,5,52,255,2,5,52,2,2,52,2,2
        .byte 255,0,13,3,255,0,13,3,255,0,6,3,255,0,15,3,0,0,0,4,255,0,8,3,255,0,22,5,255,0,9,3,255,0,9
        .byte 3,255,0,38,3,0,0,3,255,0,15,3,255,0,20,3,255,0,16,3,255,0,19,3,255,0,21,3,255,0,12,3,255,0,26
        .byte 3,255,0,20,3,255,0,5,3,255,0,13,3,255,0,9,3,255,0,14,3,255,0,11,35,36,0,0,0,35,36,0,0,0
        .byte 35,36,255,0,17,3,0,0,34,37,0,0,0,34,37,0,0,0,34,37,255,0,10,3,255,0,9,32,33,0,0,0,32
        .byte 33,0,0,0,32,33,255,0,6,3,255,0,8,38,39,6,6,6,38,39,6,6,6,38,39,6,6,6,38,39,6,6,6
        .byte 38,39,255,0,10,32,33,0,0,0,32,33,0,0,0,32,33,0,0,0,32,33,0,0,0,32,33,255,0,10,32,33,0,0,0
        .byte 32,33,0,0,0,32,33,0,0,0,32,33,0,0,0,32,33,255,0,10,32,33,0,0,0,32,33,0,0,0,32,33,0,0,0
        .byte 32,33,0,0,0,32,33,255,0,6,29,28,10,0,32,33,0,0,0,32,33,0,0,0,32,33,0,0,0,32,33,0,0,0
        .byte 32,10,27,255,0,4,2,2,52,2,2,52,255,2,4,52,255,2,4,52,52,255,2,4,52,255,2,4,52,255,2,5
        .byte 255,0,13,3,255,0,13,3,14,13,15,15,0,0,3,255,0,15,3,0,0,0,4,255,0,5,11,12,19,18,255,0,18
        .byte 41,41,0,0,5,0,0,0,14,13,15,15,16,17,3,255,0,9,3,255,0,15,11,12,106,19,18,19,255,0,17,3,0,0
        .byte 3,0,0,0,14,58,128,0,15,16,59,16,255,0,4,3,255,0,18,31,11,12,19,18,19,18,19,18,255,0,10,3,255,0,11
        .byte 14,13,15,15,16,59,15,17,16,17,255,0,20,3,0,11,7,255,54,8,0,3,255,0,47,3,255,0,19,3,255,0,9
        .byte 3,0,0,14,13,7,6,6,255,54,9,255,0,18,11,17,16,17,16,17,16,17,16,17,16,17,16,17,255,0,7,3,255,0,8
        .byte 14,13,19,18,10,19,18,19,18,105,15,19,18,19,18,15,3,255,0,14,31,11,17,16,59,16,15,17,16,59,16,17,16
        .byte 17,16,17,15,255,0,14,14,13,15,18,0,0,15,18,19,18,0,0,19,18,15,18,19,18,255,0,8,40,255,0,5,11
        .byte 17,16,17,0,0,16,15,16,17,0,0,16,17,16,59,16,17,255,0,6,28,26,26,26,137,0,0,0,137,53,53,53,0,0
        .byte 255,53,4,0,0,255,53,6,255,0,5,26,10,18,19,59,8,255,0,4,32,33,255,0,4,32,33,255,0,4,32,33,255,0,5
        .byte 28,26,10,10,16,17,16,59,7,30,0,0,0,32,33,255,0,4,32,33,255,0,4,32,33,0,0,0,2,2,255,52,30
        .byte 255,0,8,67,0,0,0,67,255,0,24,67,0,67,0,67,0,0,0,67,0,0,0,67,255,0,5,67,0,0,0,67,255,0,4
        .byte 67,0,67,0,67,0,67,56,56,0,67,67,67,0,67,0,67,0,67,0,67,0,67,0,67,0,67,255,0,6,67,0,67
        .byte 0,56,56,19,18,255,56,4,67,0,56,56,67,255,56,4,0,67,56,56,0,67,0,67,0,67,56,56,0,56,56,19,18
        .byte 19,18,19,18,18,15,56,56,19,18,56,19,18,18,18,56,56,19,18,255,56,6,19,18,56,19,18,11,58,11,12,11,12
        .byte 58,10,19,18,11,58,15,11,12,12,12,19,18,11,12,10,19,18,19,18,56,16,59,15,16,12,0,0,0,61,255,0,4
        .byte 11,12,255,0,7,11,58,0,0,60,11,17,16,59,15,255,54,5,0,0,0,63,255,0,5,60,255,0,7,62,60,0,0
        .byte 62,255,54,5,255,0,14,62,255,0,8,62,255,0,40,255,54,4,7,255,0,22,7,255,54,4,19,18,56,19,13,30,40
        .byte 0,0,40,40,40,0,0,40,0,40,0,40,0,40,40,0,0,40,0,31,14,18,15,19,18,16,17,15,16,59,14,13,10
        .byte 14,13,13,10,14,13,10,14,13,14,13,10,14,13,13,14,13,14,13,16,17,56,16,17,255,56,5,16,17,56,16,17,17
        .byte 56,16,59,56,16,17,16,59,56,16,59,17,19,18,16,59,255,56,16,67,56,56,0,255,56,4,67,56,56,56,16,17,56,56
        .byte 67,0,67,0,67,19,18,19,18,15,15,18,19,18,56,67,0,67,0,67,0,67,0,67,0,67,0,0,56,56,0,67,0
        .byte 67,0,0,0,11,12,11,58,10,11,12,11,17,56,0,67,0,0,0,67,255,0,5,67,0,67,0,67,0,0,0,67,0,0,0
        .byte 61,63,62,0,0,60,14,18,56,67,255,0,11,67,255,0,15,62,11,17,56,0,67,255,0,20,14,13,14,58,0,0,0
        .byte 10,19,18,56,0,67,255,0,19
        .byte 0,67,56,56,19,18,56,19,18,18,10,19,18,19,18,10,19,18,19,18,19,18,0,10,19,18,10,19,18,56,15,56,67
        .byte 56,56,15,11,12,10,19,18,13,61,11,12,11,58,60,11,12,11,12,11,58,0,0,11,12,0,11,12,19,18,18,56,19
        .byte 18,19,13,0,61,11,58,12,255,0,5,62,255,0,5,60,255,0,7,14,18,18,56,19,18,19,13,255,0,16,62,255,0,7
        .byte 11,12,17,15,16,12,16,12,255,0,24,61,14,18,19,13,63,11,58,26,133,255,0,6,133,26,133,255,0,7,14,13,255,0,4
        .byte 63,11,17,16,12,0,0,61,255,0,8,63,21,255,0,5,40,40,40,11,12,30,40,0,0,0,14,18,54,6,0,0,63
        .byte 255,0,9,22,255,0,5,54,6,6,7,54,6,54,0,0,0,134,59,255,0,14,22,255,0,5,63,0,60,14,13,61,255,0,20
        .byte 20,0,40,255,0,5,62,14,13,63,255,0,4,40,0,54,6,255,0,10,133,26,26,26,53,255,0,6,11,12,255,0,4
        .byte 6,134,15,16,58,255,0,11,63,21,0,62,255,0,6,14,13,255,0,4,60,14,18,19,18,10,255,0,10,63,22,255,0,5
        .byte 40,40,0,11,12,255,0,4,62,11,17,16,12,133,26,133,255,0,9,22,255,0,4,53,26,53,0,28,10,10,255,0,5
        .byte 10,56,19,13,255,0,11,22,0,0,0,40,40,21,0,0,14,13,61,255,0,4,14,18,56,16,12,0,0,40,0,40,0,0
        .byte 40,40,0,0,20,0,0,14,13,13,20,0,28,11,12,255,0,5,11,59,56,19,13,14,13,14,14,13,14,13,58,14,13
        .byte 10,10,14,13,18,17,17,14,59,10,14,13,10,0,0,0,10,15,15,56,16,17,16,17,16,16,59,16,17,17,16,17,56,56
        .byte 16,17,56,56,56,16,59,56,16,17,10,0,0,54,6,7,56,67,255,56,13,67,56,56,56,0,56,56,56,67,56,56,10
        .byte 30,0,61,0,14,18,0,67,0,67,0,67,0,67,0,67,0,67,0,67,0,67,0,67,0,67,0,67,0,56,10,7,6
        .byte 54,0,0,11,17
        .byte 255,0,21,67,0,56,19,13,255,0,4,14,18,255,0,22,67,56,16,12,255,0,4,11,17,255,0,15,67,255,0,5,67
        .byte 0,56,15,7,54,6,0,0,0,10,255,0,4,67,0,67,0,67,0,67,0,0,0,67,0,67,0,67,0,67,0,67,56
        .byte 19,13,255,0,4,31,10,67,0,67,67,0,67,0,67,0,67,0,67,0,67,0,56,56,67,0,67,0,67,0,56,19,13
        .byte 0,0,0,54,7,10,0,67,0,255,56,8,0,67,0,56,19,18,56,67,0,56,56,67,56,16,58,0,0,0,60,14,18
        .byte 67,56,56,15,15,19,18,10,10,19,18,255,56,4,16,17,56,56,56,15,15,56,56,10,7,54,6,0,62,11,59,56,19
        .byte 18,18,10,11,12,61,60,11,12,0,0,19,18,0,0,19,18,18,0,0,19,18,63,255,0,6,28,56,16,12,58,0,0,0
        .byte 63,62,255,0,4,11,58,0,0,11,58,12,0,0,11,12,255,0,7,10,56,15,10,255,0,5,62,255,0,9,61,255,0,8
        .byte 54,6,7,14,18,19,13,10,255,0,15,63,255,0,10,14,18,17,16,12,255,0,24,40,40,31,11,59,56,56,10,255,0,5
        .byte 40,0,40,0,0,40,40,0,0,40,255,0,5,40,0,40,31,14,13,14,13,56,67,15,10,255,0,5,14,13,58,14,13
        .byte 14,13,58,14,13,30,14,13,10,14,13,14,13,10,16,59,16,17,56,0,10,63,0,0,0,54,6,7,56,56,16,17,19
        .byte 18,18,16,59,10,16,17,56,19,18,16,17,255,56,5,0,67,10,30,255,0,5,14,18,56,56,56,16,17,17,255,56,6
        .byte 16,59,56,56,0,67,0,67,0,67,0,56,19,13,255,0,4,11,17,56,0,67,56,56,56,67,0,67,0,67,0,56,56
        .byte 67,0,67,0,67,0,67,0,0,56,16,12,255,0,4,7,56,0,67,0,67,0,67,0,67,0,67,0,67,0,67,0,67
        .byte 255,0,8,56,10,255,0,4,8,10,56,0,67,255,0,20,67,56,7,6,54,0,0,8,10,56,67,255,0,21
        .byte 67,56,19,13,0,0,0,11,17,56,56,67,19,18,67,67,67,56,67,56,56,56,67,85,255,56,4,19,18,56,67,0,56
        .byte 19,13,0,0,0,10,19,59,85,85,16,17,56,56,19,59,85,85,19,18,56,56,15,19,59,54,16,17,27,56,67,56,16
        .byte 12,0,0,0,10,16,17,255,85,6,16,17,85,85,16,59,85,85,60,16,17,85,61,85,19,18,56,19,13,0,9,9,6
        .byte 7,15,255,85,15,62,85,85,85,63,85,16,59,56,19,13,0,9,9,0,255,85,23,7,54,56,16,17,10,9,9,0,65,65
        .byte 85,85,85,65,65,85,85,85,65,85,85,65,65,85,85,85,65,65,85,85,85,19,59,56,17,255,2,6,255,57,4,18,57,57
        .byte 18,255,57,5,18,255,57,4,15,9,9,85,16,17,0,56,16,17,19,18,18,19,18,15,19,18,18,15,19,18,19,59,19
        .byte 18,16,17,19,18,15,19,18,9,9,85,63,19,67,0,56,56,16,59,17,16,59,56,16,17,17,56,16,59,16,17,16,17
        .byte 56,56,16,17,56,16,59,9,9,85,85,16,85,67,85,67,255,56,5,67,56,56,56,67,255,56,6,85,67,56,56,85,56
        .byte 85,9,9,85,85,56,67,85,67,85,67,85,67,1,67,1,67,1,67,1,67,1,67,0,67,1,67,1,67,0,67,1,85
        .byte 9,9,85,85,15,84,56,255,0,4,255,1,9,68,1,68,1,68,255,1,6,85,9,9,85,85,67,76,13,0,86,0,0,0
        .byte 255,1,20,64,64,1,1,76,73,12,0,85,0,0,0,255,1,20,64,64,1,1,73,72,1,255,0,4,255,1,21,64,64
        .byte 1,76,75,76,70,54,9,9,54,70,75,255,1,19,64,64,1,73,74,73,74,1,64,64,1,73,74,76,75,76,75,72,72
        .byte 76,75,76,75,75,72,76,75,72,76,75,1,1,64,64,1,1,76,72,1,1,64,64,1,1,76,73,74,73,74,84,84,73
        .byte 74,73,74,74,84,73,74,84,73,74,255,82,6,73,76,75,1,64,64,1,1,73,74,255,84,15,73,74,255,84,4,76,75
        .byte 73,74,1,64,64,1,76,75,84,84,1,68,1,68,1,68,1,68,1,68,1,68,1,255,84,4,68,68,84,73,74
        .byte 115,1,1,64,64,255,41,4,73,74,68,80,68,73,122,73,74,80,68,73,118,121,84,84,68,255,1,6,68,1,1,64,64
        .byte 80,255,1,6,79,255,1,5,79,1,72,73,74,118,117,84,1,1,68,1,1,1,68,1,1,64,64,79,255,1,6,22,255,1,5
        .byte 79,1,1,1,68,73,73,1,1,84,1,68,1,1,75,1,1,64,64,79,255,1,5,0,24,0,255,1,4,79,255,1,8
        .byte 115,84,1,68,1,74,1,1,64,64,77,255,1,4,0,0,86,0,0,1,1,1,22,255,1,8,68,115,84,1,1,68,1,1
        .byte 131,132,82,132,131,1,1,0,0,85,0,0,1,1,0,24,0,255,1,12,72,1,1,1,68,78,68,255,1,4,40,23,40
        .byte 1,1,0,0,86,0,0,255,1,11,68,255,1,4,79,255,1,5,53,26,53,1,1,0,0,85,0,0,255,1,7,72,84
        .byte 68,1,1,72,1,1,1,79,255,1,5,68,78,68,1,1,1,40,23,40,255,1,8,76,121,84,68,55,72,1,1,1,79
        .byte 255,1,6,79,255,1,4,53,26,53,255,1,8,73,116,84,1,55,255,1,4,77,255,1,6,79,255,1,4,68,78,68,255,1,8
        .byte 68,76,117,84,72,55,1,131,132,82,132,131,255,1,4,79,255,1,5,79,255,1,4,131,132,64,64,132,131,73,122,72,84
        .byte 72,55,1,1,78,255,1,6,77,255,1,5,79,255,1,6,64,64,255,1,8,55,1,79,255,1,5,131,82,131,255,1,4
        .byte 79,255,1,6,64,64,255,1,5,84,76,122,55,1,81,255,1,6,78,255,1,5,77,255,1,6,64,64,255,1,5,84,115
        .byte 75,55,55,255,1,7,79,255,1,4,131,82,131,255,1,5,64,64,255,1,5,84,114,118,75,55,255,1,7,81,255,1,5
        .byte 78,255,1,4,131,132,82,82,132,131,132,82,82,1,84,121,73,255,55,14,79,255,1,4,55,255,68,9,1,84,84,76,75
        .byte 72,255,55,11,81,255,55,13,86,68,86,84,115,121,84,76,75,68,72,72,86,115,255,55,18
        .byte 68,1,68,255,84,4,118,117,84,118,121,117,73,74,121,75,68,72,73,74,68,68,73,74,76,117,84,123,84,1,68,84,84,84
        .byte 118,117,118,117,73,122,1,73,74,74,1,68,73,74,68,1,66,255,1,4,66,73,74,122,72,114,84,1,114,76,121,122,75
        .byte 73,74,68,1,1,66,66,1,1,68,87,88,1,1,66,255,1,5,122,66,68,66,76,117,84,84,73,74,66,66,255,1,5
        .byte 66,66,1,1,1,87,88,68,255,1,8,66,1,1,73,74,84,1,68,78,66,255,1,6,66,255,1,4,87,88,255,1,12
        .byte 78,68,68,1,68,79,255,1,7,66,255,1,4,87,88,255,1,12,79,68,1,1,1,79,255,1,12,87,88,255,1,12,79
        .byte 255,1,4,79,255,1,12,87,88,255,1,12,79,255,1,4,77,255,1,12,87,88,255,1,12,79,1,1,70,69,69,69,70
        .byte 255,1,8,70,69,119,120,69,70,255,1,8,70,69,69,69,70,68,68,78,68,68,255,1,8,68,68,87,88,68,68,255,1,8
        .byte 68,68,78,68,68,1,1,79,255,1,12,87,88,255,1,12,79,255,1,4,79,255,1,12,87,88,255,1,12,79,255,1,4
        .byte 79,255,1,12,87,88,255,1,12,79,255,1,4,79,255,1,12,87,88,255,1,12,79,255,1,4,77,255,1,12,87,88,255,1,12
        .byte 79,1,1,70,69,69,69,70,255,1,8,70,69,119,120,69,70,255,1,8,70,69,69,69,70,68,68,78,68,68,255,1,8
        .byte 68,68,87,88,68,68,255,1,8,68,68,78,68,68,1,1,79,255,1,12,87,88,255,1,12,79,255,1,4,81,255,55,12
        .byte 89,90,255,55,12,81,1,1
        .byte 1,1,79,255,1,7,68,84,121,75,1,64,64,1,76,117,84,68,255,1,12,79,255,1,8,84,115,74,1,64,64,1,122
        .byte 116,84,255,1,13,79,255,1,7,68,1,84,72,1,64,64,76,117,84,1,68,255,1,12,81,255,1,8,84,118,75,1,64,64
        .byte 73,116,84,68,255,1,21,68,84,121,122,1,64,64,1,122,117,84,68,255,1,12,80,255,1,6,68,1,84,72,85,1,64,64
        .byte 1,73,116,84,255,1,12,68,79,255,1,7,68,84,118,75,85,64,64,85,85,72,84,68,255,1,12,79,68,1,1,1,68
        .byte 1,68,1,84,121,74,1,64,64,1,76,117,84,255,1,12,68,77,1,68,84,84,1,68,1,68,84,118,75,1,64,64,1
        .byte 73,121,84,68,255,1,10,131,132,82,132,131,118,117,255,84,5,115,122,1,64,64,1,72,84,255,1,12,68,68,78,68,68
        .byte 73,74,118,117,10,118,121,72,1,1,64,64,1,76,117,84,68,255,1,12,79,255,1,4,73,12,0,11,74,1,1,1,64,64
        .byte 1,73,116,84,1,68,255,1,11,79,255,1,4,0,61,0,60,0,1,1,1,64,64,1,1,122,117,84,255,1,12,79,1,1,1
        .byte 0,0,0,86,62,0,0,1,1,64,64,1,1,73,116,84,68,255,1,8,85,1,1,79,1,1,1,0,0,0,85,0,0,0
        .byte 1,1,64,64,1,76,117,84,68,255,1,8,85,85,1,1,77,255,1,4,255,0,5,1,1,1,64,64,1,73,121,84,1
        .byte 68,255,1,7,85,85,131,132,82,132,131,132,82,131,7,19,59,76,75,122,76,75,76,122,116,84,85,68,255,1,10,68,68
        .byte 78,68,68,255,84,4,115,121,115,121,84,115,121,115,116,84,85,68,255,1,12,68,79,68,1,68,1,68,1,255,84,4,1
        .byte 255,84,4,1,68,255,1,14,81,1,255,86,4,68,86,68,1,68,1,68,1,68,1,68,1,1,255,86,7,255,1,4
        .byte 1,67,0,67,1,67,1,67,1,67,1,67,1,67,1,67,0,67,1,67,1,67,1,67,1,67,1,67,1,67,1,67,56,56
        .byte 67,56,56,1,255,56,17,1,67,255,56,6,1,19,59,56,19,18,56,15,0,0,19,18,19,18,19,18,10,19,18,18,1
        .byte 0,0,15,56,56,19,18,10,19,59,56,56,16,17,15,11,58,10,10,0,0,11,12,11,12,11,12,61,11,58,12,10,0,0
        .byte 94,14,18,11,12,61,11,12,14,18,15,19,13,86,255,0,7,60,86,14,13,63,255,0,7,11,12,0,0,63,0,86,11
        .byte 17,56,19,58,85,255,0,7,62,85,11,12,1,255,0,13,85,10,56,56,135,12,255,0,10,14,13,255,0,14,10,17,10
        .byte 15,255,0,12,11,12,255,0,14,7,54,10,255,0,13,7,7,6,54,255,0,28,11,12,86,255,0,4,14,7,6,54,255,0,8
        .byte 10,135,0,0,0,54,6,7,7,6,54,0,0,14,13,85,255,0,4,11,12,255,0,6,54,6,7,10,19,13,255,0,5
        .byte 14,18,255,0,4,11,12,0,0,0,54,6,7,10,255,0,8,19,18,16,12,255,0,5,11,17,255,0,4,19,18,255,0,5
        .byte 19,13,7,6,54,255,0,5,11,17,10,7,6,54,0,0,0,7,7,0,0,54,6,7,7,6,54,0,0,0,16,12,10
        .byte 255,0,6,14,13,56,59,13,255,0,5,19,18,255,0,11,19,13,255,0,5,54,6,7,17,56,16,12,30,40,40,0,0
        .byte 16,17,0,0,40,40,0,0,40,40,40,0,0,16,12,30,40,40,255,0,5,14,18,16,14,13,10,14,13,58,10,14,19,19
        .byte 18,19,18,15,19,18,15,19,59,19,59,14,13,10,136,255,0,4,136,17,56,16,59,56,16,17,56,15,16,59,16,17,19
        .byte 59,56,16,17,56,16,17,16,17,16,59,16,12,255,0,4,14,18,1,56,56,67,56,56,1,255,56,5,16,17,56,56,56
        .byte 67,255,56,6,19,7,6,54,0,0,11,17,67,1,67,0,67,1,67,1,67,1,67,1,56,56,67,1,67,1,67,1,67
        .byte 1,67,56,16,12,255,0,4,67,10
        .byte 67,56,19,58,0,0,0,14,18,56,67,255,0,22,56,16,12,0,0,0,11,59,56,255,0,22,56,19,13,30,255,0,4
        .byte 14,18,56,67,255,0,20,56,19,7,6,54,0,0,0,11,17,56,255,0,21,56,16,12,255,0,4,31,10,56,0,67,255,0,20
        .byte 56,107,10,0,0,0,54,6,7,59,56,255,0,21,19,59,10,30,0,0,0,60,11,17,56,67,255,0,20,11,12,7,6
        .byte 54,0,0,62,31,19,18,56,255,0,23,21,255,0,4,11,17,17,56,67,255,0,19,65,65,65,20,65,65,65,7,10,15
        .byte 56,67,255,0,20,14,13,11,14,58,14,10,13,56,56,67,67,255,0,20,16,17,56,16,17,56,16,59,56,67,255,0,22
        .byte 56,56,67,255,56,5,67,67,255,0,23,67,0,67,0,67,0,67,255,0,216
        .byte 255,0,5,67,0,67,0,67,255,56,19,67,0,67,255,0,4,67,0,255,56,4,19,18,19,59,19,18,10,15,19,18,19
        .byte 59,10,19,59,19,18,19,18,56,67,255,0,4,67,0,56,19,19,59,10,11,12,11,12,11,58,61,10,11,12,58,12,60
        .byte 11,12,10,12,16,59,56,0,67,67,0,67,0,56,19,13,10,12,61,255,0,4,63,0,63,0,60,0,0,0,62,255,0,4
        .byte 10,15,59,56,0,0,56,56,56,15,11,12,0,0,63,255,0,8,62,255,0,9,11,17,56,56,56,15,15,123,19,13,61
        .byte 0,0,63,255,0,18,58,18,19,18,19,18,19,18,15,12,63,255,0,18,53,26,26,11,10,11,12,10,12,10,10,12,255,0,21
        .byte 61,0,62,0,0,10,255,0,26,63,255,0,6,255,65,4,0,0,0,26,53,53,26,53,53,26,255,53,5,26,53,53,26
        .byte 53,255,0,5,65,65,58,13,14,13,14,58,14,13,0,63,60,21,0,0,21,0,62,0,0,60,22,63,0,22,63,255,0,4
        .byte 31,14,13,16,17,16,59,16,10,11,12,0,0,62,22,0,0,22,255,0,4,62,22,0,0,22,255,0,4,14,58,16,17
        .byte 56,15,56,56,15,19,58,255,0,4,22,0,0,22,0,0,23,0,0,22,0,0,22,0,0,0,14,16,17,56,56,0,56
        .byte 0,67,56,16,12,0,65,0,65,20,65,0,20,65,65,20,65,65,20,65,0,20,65,0,65,10,107,10,56,67,67,0,67
        .byte 0,56,19,13,14,13,14,58,10,14,58,14,13,14,58,13,10,14,13,31,14,58,14,13,10,10,10,56,255,0,4,67,56
        .byte 16,17,16,59,16,17,56,16,17,16,17,19,18,59,56,16,17,10,16,17,19,59,56,16,17,56,67,255,0,4,67,255,56,6
        .byte 0,255,56,4,16,17,17,56,56,56,15,56,56,16,17,56,56,56,67,255,0,6,67,0,67,0,67,0,67,0,67,0,67
        .byte 56,56,56,67,0,67,56,67,0,56,56,67,0,67,255,0,18,67,0,67,0,0,0,67,0,67,0,67,255,0,37
        .byte 84,84,118,75,73,74,118,117,87,88,118,117,84,73,122,87,88,73,74,118,117,66,87,88,118,117,118,117,72,1,84,1,84
        .byte 114,115,122,1,1,73,122,87,88,73,74,72,1,1,87,88,66,72,1,1,66,87,88,66,1,73,74,1,76,117,84,84,118
        .byte 75,66,255,1,4,87,88,66,255,1,4,87,88,66,255,1,4,87,88,66,255,1,4,73,116,84,84,115,74,66,255,1,4
        .byte 87,88,255,1,5,87,88,255,1,5,87,88,255,1,5,66,72,84,114,255,1,7,87,88,255,1,5,87,88,255,1,5,87
        .byte 88,255,1,5,66,76,117,118,117,255,1,6,131,69,255,1,5,131,69,255,1,5,131,69,255,1,6,73,116,115,117,255,1,7
        .byte 66,255,1,5,66,255,1,7,66,255,1,6,114,117,11,17,255,1,7,66,255,1,20,16,12,0,0,0,255,1,27,255,0,5
        .byte 255,1,26,0,0,0,10,7,6,69,1,1,1,0,131,69,0,255,1,4,131,69,255,1,5,131,69,255,1,4,69,6,7
        .byte 10,115,116,1,66,255,1,4,87,88,255,1,5,87,88,255,1,5,87,88,255,1,4,66,1,115,116,1,118,122,66,255,1,4
        .byte 87,88,255,1,5,87,88,255,1,5,87,88,255,1,4,66,118,117,84,1,115,74,255,1,5,87,88,255,1,5,87,88,255,1,4
        .byte 68,87,88,68,1,1,1,66,115,116,84,118,75,255,1,6,87,88,255,1,4,68,87,88,255,1,5,87,88,255,1,4,66
        .byte 114,84,1,116,74,123,55,1,1,1,55,87,88,1,1,1,55,55,87,88,255,55,4,1,87,88,255,55,4,123,118,121,84,84
        .byte 114,76,122,55,55,55,114,89,90,55,55,55,123,114,89,90,1,118,117,55,55,89,90,114,1,1,118,117,115,116,84,84,114
        .byte 115,116,121,117,123,118,121,1,68,1,114,118,121,1,68,1,115,121,1,1,1,123,114,118,121,121,116,84,84,1,1,84,84
        .byte 114,115,116,114,115,116,1,118,117,114,84,84,1,118,117,68,1,121,114,68,114,84,115,116,84,84,1,1,1,68,86,86,255,84,6
        .byte 86,84,84,84,86,86,86,84,84,86,86,84,84,86,84,86,84,84,255,86,5
        .byte 255,0,69,67,0,67,255,0,9,67,0,67,0,67,0,67,0,67,0,67,0,67,0,0,67,0,67,0,67,0,67,0,67
        .byte 0,67,0,67,0,67,0,67,0,67,0,67,0,67,0,67,0,67,0,67,0,67,0,0,67,0,67,0,255,56,4,67,85
        .byte 67,0,67,0,67,255,56,4,0,67,255,56,9,67,255,56,6,19,18,255,56,9,19,18,255,56,4,19,18,19,18,18,19
        .byte 18,56,56,19,59,19,18,19,18,11,12,19,18,19,18,18,18,0,0,10,11,12,19,18,19,59,11,12,11,58,12,11,12
        .byte 19,18,11,12,11,12,11,58,61,0,11,12,11,12,58,12,255,0,4,60,11,12,11,12,0,60,0,0,0,62,0,11,12
        .byte 255,0,5,63,63,255,0,11,62,255,0,5,62,255,0,5,63,0,0,40,40,255,0,4,40,0,40,40,255,0,5,40,40
        .byte 0,40,0,40,0,0,0,40,0,40,40,40,0,0,14,13,14,13,30,14,13,14,58,13,14,13,30,0,28,14,13,14,58
        .byte 10,14,13,30,31,14,13,14,13,58,10,14,13,16,17,16,59,10,16,17,16,17,17,16,17,14,13,13,16,17,16,17,56
        .byte 16,17,14,13,16,59,16,17,56,56,16,17,255,56,4,15,255,56,7,16,17,17,255,56,4,67,56,56,16,17,255,56,4
        .byte 67,85,56,56,0,67,0,56,56,56,0,67,0,67,0,255,56,5,0,67,0,67,0,255,56,4,67,0,67,85,67,0,67,67
        .byte 0,67,0,67,0,67,0,67,0,67,0,67,0,67,0,67,0,67,0,67,0,67,0,67,0,67,0,67,0,67,255,0,4
        .byte 67,0,67,255,0,7,67,255,0,9,67,255,0,136
        .byte 255,0,18,67,56,15,19,59,255,56,5,15,15,19,18,0,0,0,56,0,0,0,56,255,0,9,67,0,56,15,11,12,19
        .byte 18,10,19,18,10,15,16,17,67,0,67,0,67,0,67,0,67,0,67,255,0,7,56,19,13,61,0,11,12,0,11,12,0
        .byte 14,18,56,0,255,56,5,0,255,56,4,67,255,0,5,67,56,16,12,63,255,0,5,62,0,11,17,56,56,19,18,10,19
        .byte 59,56,19,18,19,59,56,67,55,0,0,67,0,56,10,255,0,9,14,18,56,19,19,12,0,11,12,10,11,12,10,10,56
        .byte 0,67,0,0,0,67,0,56,10,255,0,8,11,59,56,10,10,255,0,4,21,0,0,10,56,0,67,0,67,0,67,0,56
        .byte 19,13,255,0,6,26,26,26,19,18,54,54,255,0,4,22,0,0,14,18,255,56,8,16,12,65,255,0,5,32,33,0,11
        .byte 58,255,0,6,20,0,0,11,12,19,59,10,19,18,10,19,18,10,7,6,6,54,0,0,0,32,33,255,0,8,54,6,6,6
        .byte 54,7,11,12,61,11,12,0,11,12,19,58,32,33,255,0,4,32,33,0,65,0,54,7,255,0,4,21,255,0,4,61,0
        .byte 63,0,60,0,0,60,11,12,32,33,0,0,0,54,38,39,7,13,10,16,58,255,0,4,22,255,0,4,63,0,63,0,62
        .byte 0,0,62,0,0,32,33,255,0,4,32,33,11,17,15,19,7,6,54,0,0,22,255,0,4,63,0,0,0,62,0,0,62
        .byte 0,0,32,33,255,0,4,32,33,14,18,56,16,12,255,0,4,22,255,0,8,62,255,0,5,32,39,6,0,0,0,32,33
        .byte 58,59,56,56,10,255,0,4,20,255,0,14,32,33,255,0,4,32,33,14,18,56,56,15,14,13,10,19,58,65,65,255,0,12
        .byte 32,33,0,0,0,54,38,33,11,17,56,0,56,16,17,15,16,17,14,13,255,0,12,32,33,255,0,4,32,33,0,15,56
        .byte 67,0,255,56,4,15,16,17,2,2,14,58,2,2,14,58,2,2,14,58,2,2,14,13,13,65,32,33,14,59,56,0,67
        .byte 0,67,0,67,255,56,5,16,17,19,59,16,17,15,15,16,17,59,15,16,17,14,13,10,10,16,17,56,0,0,67,0,67
        .byte 0,67,0,67,0,67,56,56,16,17,255,56,5,16,17,56,56,56,16,17,255,56,4,0
        .byte 56,19,18,11,12,19,18,11,12,19,59,255,56,19,67,0,56,11,12,63,60,11,12,60,61,11,12,19,18,10,19,18,18
        .byte 10,19,59,10,19,18,19,18,59,18,11,19,18,56,67,59,13,0,63,62,0,63,0,63,0,0,11,12,0,11,58,12,60
        .byte 11,12,0,11,58,11,12,11,12,63,11,12,15,56,16,58,0,0,62,0,63,255,0,4,21,255,0,4,21,62,0,0,0
        .byte 21,0,0,0,61,21,63,0,62,14,18,10,27,0,0,125,255,0,6,22,255,0,4,22,255,0,4,22,0,0,0,63,22
        .byte 0,0,0,11,17,19,58,0,0,125,125,255,0,5,25,255,0,4,25,255,0,4,25,255,0,4,25,255,0,4,11,16,12
        .byte 0,0,38,39,255,0,5,22,255,0,4,22,255,0,4,22,255,0,4,22,0,0,0,14,59,56,10,0,0,32,33,255,0,5
        .byte 25,255,0,4,25,255,0,4,25,255,0,4,25,0,0,0,11,12,19,13,0,54,6,6,54,30,0,0,0,22,255,0,4
        .byte 22,255,0,4,22,255,0,4,22,255,0,5,16,58,54,255,6,4,54,30,0,0,20,0,65,0,65,20,0,0,65,0,20
        .byte 0,65,65,0,20,0,65,65,0,0,16,10,255,2,30,56,16,16,59,18,19,18,15,19,59,15,15,19,18,19,18,19,18
        .byte 15,15,19,59,19,18,59,19,18,19,18,15,19,59,0,56,56,16,17,59,17,56,16,17,56,56,19,59,16,17,16,59,15
        .byte 56,16,17,19,18,18,16,17,16,59,56,16,17,67,0,67,255,56,4,0,56,56,0,56,16,17,255,56,5,0,56,56,16
        .byte 17,17,255,56,4,0,56,56,0,0,0,67,0,67,0,67,0,67,0,67,56,56,0,67,0,67,0,67,0,67,56,56,56
        .byte 67,0,67,0,67,0,67,255,0,10,67,0,67,0,67,255,0,7,67,0,67,0,67,0,67,0,67,255,0,24,67,255,0,104
        .byte 255,0,41,67,255,0,26,67,0,67,0,67,0,67,255,0,8,67,0,67,0,67,255,0,11,67,0,67,56,67,0,56,0
        .byte 56,255,0,6,56,0,67,56,67,0,67,0,67,0,0,0,67,0,67,0,67,56,56,56,15,10,0,10,0,15,255,0,6
        .byte 15,0,15,10,56,56,0,67,0,67,0,67,0,67,56,56,56,19,13,10,10,0,0,62,0,60,255,0,6,62,0,61,10
        .byte 19,18,56,56,56,0,255,56,4,19,59,15,16,12,60,62,126,0,0,0,62,255,0,8,63,0,11,12,15,19,18,56,19
        .byte 59,14,18,11,12,7,6,54,62,0,126,126,255,0,15,62,11,7,6,54,12,58,12,0,61,0,21,0,0,0,38,39,255,0,13
        .byte 23,0,62,0,0,21,0,0,62,0,65,65,65,20,65,65,65,32,33,255,0,13,20,0,40,40,0,20,40,40,40,0,14
        .byte 13,11,14,58,14,10,13,14,18,56,67,255,0,8,67,56,19,10,10,58,10,14,13,10,14,58,16,17,56,16,17,56,16
        .byte 59,15,56,67,255,0,10,67,56,15,16,17,10,16,59,56,16,17,56,56,67,56,56,67,56,56,56,67,0,67,255,0,8
        .byte 67,0,67,255,56,6,0,56,56,67,0,67,0,67,0,67,0,67,0,67,255,0,10,67,0,67,0,67,0,67,0,67,0
        .byte 67,0,67,0,68,0,67,0,68,0,68,255,0,10,68,0,68,0,68,0,67,0,67,0,68,0,68,0,68,0,68,0,68
        .byte 255,0,18,68,0,68,0,68,0,68,255,0,36,255,55,26,255,0,66
        .byte 255,0,99,67,0,67,0,67,0,67,0,67,255,0,20,67,0,67,0,255,56,6,67,85,67,255,0,19,255,56,4,19,59
        .byte 19,18,15,59,56,67,255,0,20,19,59,19,15,11,12,11,12,11,17,56,67,67,255,0,19,11,58,11,12,63,0,61,62
        .byte 0,14,18,56,0,67,255,0,21,21,255,0,4,31,11,17,56,67,255,0,19,65,65,65,20,40,0,0,0,14,59,56,67
        .byte 255,0,20,14,13,11,14,58,0,0,40,11,17,56,0,67,255,0,19,16,17,19,18,12,0,0,54,6,7,18,56,0,67
        .byte 255,0,18,67,56,16,10,0,40,0,0,58,11,17,56,67,255,0,19,56,19,18,7,6,54,0,0,14,15,56,67,255,0,20
        .byte 56,16,12,30,60,0,0,0,11,27,56,0,67,255,0,20,56,19,58,62,0,0,40,40,14,18,56,0,67,255,0,18,67
        .byte 56,16,12,0,0,54,6,7,16,17,56,67,255,0,20,56,10,0,0,0,63,0,11,17,56,67,255,0,20,67,56,19,30
        .byte 40,0,0,14,59,56,67,0,67,255,0,20,56,16,7,54,0,0,11,17,56,0,67,255,0,20
        .byte 67,56,19,58,0,0,0,14,18,56,67,255,0,22,56,16,12,0,0,0,11,59,56,255,0,22,56,19,13,30,0,0,0
        .byte 61,14,18,56,67,255,0,7,67,0,67,255,0,5,67,0,67,0,0,56,19,7,6,54,0,0,0,11,17,56,0,67,0
        .byte 67,0,67,0,67,56,56,0,67,0,67,0,67,0,56,0,67,0,56,16,12,255,0,4,31,10,255,56,4,67,56,56,56
        .byte 67,56,15,15,255,56,7,15,56,56,67,56,56,10,0,0,0,54,6,7,59,10,0,0,56,19,59,18,56,19,59,10,19
        .byte 18,0,0,19,59,10,19,18,15,56,19,59,10,30,0,0,0,60,11,17,61,0,0,10,11,12,12,10,11,12,60,11,58
        .byte 0,0,11,12,62,11,12,14,18,11,12,7,6,54,0,0,62,0,0,63,255,0,9,62,255,0,7,63,62,10,12,19,13
        .byte 0,21,255,0,26,62,0,16,12,27,20,65,65,65,0,0,0,65,0,65,0,65,0,0,0,65,65,0,65,0,65,65,0,0,0
        .byte 40,40,40,0,56,56,11,14,58,14,10,13,56,14,58,10,14,58,13,0,14,13,13,14,13,10,14,13,10,58,28,14,13
        .byte 10,14,58,0,67,56,16,17,56,16,59,10,16,17,56,19,18,59,10,16,17,17,16,17,56,16,59,16,17,10,16,59,56
        .byte 16,17,67,0,67,255,56,9,16,17,17,255,56,4,15,56,0,255,56,7,0,56,56,0,67,0,67,0,67,0,67,0,67
        .byte 0,67,56,56,56,67,0,67,0,56,0,67,0,67,0,67,0,67,0,67,0,67,255,0,10,67,0,67,0,67,0,67,0
        .byte 67,0,67,255,0,24,67,255,0,146
        .byte 255,1,10,68,1,68,1,1,1,68,1,68,1,68,255,1,12,68,1,68,1,68,1,68,1,68,84,84,84,68,1,68,1
        .byte 84,84,84,1,68,1,68,1,68,1,68,1,68,1,68,68,1,84,1,255,84,6,114,118,117,255,84,4,118,117,117,84,84,84
        .byte 1,68,1,255,84,5,1,1,84,114,84,118,117,72,118,117,118,117,73,74,118,117,118,117,73,122,74,72,118,117,84,84,84
        .byte 118,117,118,121,114,84,114,118,117,72,73,74,1,73,74,73,122,1,1,73,74,73,74,255,1,4,73,74,118,117,122,73,74
        .byte 73,74,114,84,114,115,122,255,1,20,73,74,255,1,4,72,118,117,76,75,255,1,28,73,116,11,12,255,1,27,76,117,84
        .byte 0,0,0,255,1,26,73,116,84,65,65,0,0,255,1,5,0,0,0,255,1,9,0,0,0,255,1,6,72,84,10,14,18
        .byte 9,9,1,1,1,255,0,6,255,1,5,255,0,6,255,1,4,76,117,84,84,115,121,64,64,1,1,0,0,0,86,255,0,4
        .byte 1,1,1,255,0,4,86,0,0,0,1,1,1,73,116,84,84,118,122,64,64,1,1,0,0,0,85,0,0,0,100,101,1
        .byte 102,103,0,0,0,85,0,0,0,86,0,1,1,76,121,84,115,74,64,64,1,1,255,0,6,99,255,98,5,99,255,0,6
        .byte 86,86,1,1,73,116,118,75,1,64,64,1,1,255,0,7,95,96,96,96,97,255,0,7,119,120,1,76,121,84,115,74,1
        .byte 64,64,1,1,255,0,5,7,7,54,69,69,69,54,7,7,255,0,5,87,88,1,73,116,84,84,76,75,72,255,1,5,0,0
        .byte 7,54,70,69,69,70,69,69,70,54,7,0,0,1,1,76,122,76,117,84,1,84,115,116,76,75,1,76,122,72,76,75,76
        .byte 122,76,122,75,1,76,75,76,122,72,76,75,76,75,115,116,115,116,84,68,68,84,84,121,116,72,115,116,84,115,116,115,116
        .byte 115,121,116,114,115,116,115,116,84,115,116,115,121,255,84,4,68,1,1,68,1,255,84,5,1,255,84,12,1,255,84,4,1
        .byte 68,1,68,1,1
        .byte 255,0,22,67,56,19,58,255,0,4,14,18,255,0,21,67,0,56,16,12,255,0,4,11,17,255,0,22,56,19,13,60,255,0,4
        .byte 31,10,255,0,21,67,56,16,12,62,0,0,54,6,7,10,255,0,22,56,10,40,255,0,4,61,14,59,255,0,21,67,56
        .byte 19,7,6,54,0,0,0,58,56,255,0,22,56,16,12,255,0,4,40,14,18,255,0,21,67,0,56,10,0,0,0,54,6
        .byte 7,12,255,0,22,56,19,13,255,0,5,62,255,0,22,67,56,16,12,30,0,0,40,40,40,255,0,23,67,56,10,10,28
        .byte 14,13,10,14,58,255,0,23,56,16,17,10,16,59,56,16,17,255,0,22,67,0,255,56,5,0,56,56,255,0,23,67,0
        .byte 67,0,67,0,67,0,67,255,0,192
        .byte 255,0,88,67,0,67,0,67,0,67,255,0,24,67,0,67,0,67,0,67,0,67,255,0,22,67,0,67,56,56,56,15,56,56
        .byte 255,0,24,67,56,19,59,10,19,18,15,56,255,0,22,67,0,56,16,12,61,11,12,14,59,255,0,21,67,0,56,19,10
        .byte 0,63,0,0,10,12,255,0,20,67,0,67,56,16,10,255,0,4,62,255,0,22,67,56,15,10,63,255,0,4,40,255,0,21
        .byte 67,56,19,18,10,0,0,0,9,9,14,58,255,0,21,56,16,59,255,0,4,9,9,11,17,255,0,20,67,0,56,15,10
        .byte 27,0,0,9,9,14,18,255,0,21,67,0,56,19,13,0,0,9,9,11,17,255,0,22,67,56,16,58,0,0,9,9,0
        .byte 58,255,0,21,67,0,67,56,10,0,0,9,9,14,18,255,0,22,67,56,19,13,0,0,54,6,7,59,255,0,23,56,59
        .byte 12,0,0,0,60,0,10,255,0,22,67,0,56,7,6,54,0,62,14,18,255,0,23,67,56,10,255,0,4,11,17
        .byte 67,56,19,58,0,0,0,14,18,56,67,67,255,0,21,56,16,12,0,0,0,11,59,56,67,0,67,255,0,13,67,255,0,5
        .byte 56,19,13,30,0,0,0,63,14,18,56,67,0,67,0,67,0,67,255,0,5,67,0,67,56,67,0,67,0,67,56,19,7
        .byte 6,54,0,0,0,11,17,56,0,67,0,67,0,67,56,67,0,67,0,67,56,56,56,15,56,56,56,67,0,56,16,12,255,0,4
        .byte 31,10,56,0,67,255,56,5,15,255,56,5,15,19,18,19,59,19,18,56,56,56,108,10,0,0,0,54,6,7,59,56,15
        .byte 19,18,0,0,19,18,0,0,19,18,19,59,11,12,11,12,11,12,19,18,19,59,10,30,0,0,0,60,21,17,19,59,11
        .byte 12,0,0,11,58,0,0,11,12,11,12,21,61,0,0,60,0,11,17,11,12,7,6,54,0,0,62,22,10,11,12,61,255,0,8
        .byte 62,0,0,22,63,0,0,62,0,63,10,0,0,0,21,255,0,4,22,0,0,0,63,255,0,11,22,0,0,0,62,0,0,0
        .byte 65,65,65,20,65,65,65,0,20,0,0,0,65,65,0,0,65,65,65,0,0,0,65,65,20,255,0,6,65,14,13,11,14
        .byte 58,14,10,13,14,58,67,14,13,14,58,14,13,14,58,13,0,14,13,14,58,255,0,5,14,13,16,17,56,16,17,56,16
        .byte 59,16,17,15,16,17,16,59,16,17,16,17,59,10,16,17,16,7,6,54,0,0,0,11,17,56,56,67,255,56,12,15,19
        .byte 59,255,56,5,19,13,255,0,4,14,59,56,0,67,0,67,0,67,0,67,0,67,0,67,0,67,0,56,16,17,56,67,0
        .byte 67,56,16,12,65,0,0,0,11,17,56,67,0,67,0,67,0,0,0,67,0,0,0,67,0,67,0,56,56,67,0,67,0
        .byte 67,56,59,7,54,0,0,0,10,56,255,0,15,67,0,67,0,67,0,67,0,56,16,12,0,0,0,65,14,18,255,0,20
        .byte 67,0,56,19,13,0,0,0,54,6,7,17,255,0,21,67,56,16,12,0,0,0,61,60,0,10,255,0,22,67,56,19,7
        .byte 6,54,0,62,14,18,255,0,21,67,0,56,16,12,255,0,4,11,59
        .byte 255,0,99,67,0,67,255,0,26,67,0,67,56,56,0,67,255,0,25,56,56,56,19,59,56,56,67,255,0,24,19,59,15
        .byte 16,17,19,18,255,0,25,11,12,7,6,54,11,12,255,0,26,61,0,21,0,61,255,0,26,65,65,65,20,65,65,65,0
        .byte 40,255,0,23,14,13,11,14,58,14,10,13,14,18,56,67,255,0,20,16,17,56,16,17,56,16,59,15,56,67,255,0,21
        .byte 56,56,67,56,56,67,56,56,56,67,0,67,255,0,20,67,0,67,0,67,0,67,0,67,0,67,255,0,22,67,0,68,0
        .byte 67,0,68,0,68,255,0,22,68,0,68,0,68,0,68,255,0,59,255,55,27,255,0,67
numsc:
        .byte 29
.out .sprintf("SCREENS SIZE = %d", (* - scdat))

nmedat:
        .byte 0,1,64,24,3,7,80,56,7,12,80,200,7,12,80,72,6,13,40,136,4,14,40,88,4,14,40,192,255
        .byte 0,1,88,128,3,8,56,48,6,13,16,216,4,14,72,152,6,15,32,96,6,15,24,152,255
        .byte 0,2,136,24,2,5,136,160,7,12,136,0,255
        .byte 0,2,120,16,2,3,136,104,2,3,136,168,7,12,80,136,255
        .byte 0,2,136,16,3,7,96,88,3,7,128,136,3,7,80,184,7,12,80,208,255
        .byte 0,2,128,16,2,3,136,144,2,3,136,64,255
        .byte 0,2,136,8,3,7,64,144,8,10,112,144,8,11,112,192,255
        .byte 0,2,64,8,2,3,80,72,2,3,80,168,255
        .byte 0,2,64,8,1,9,32,80,3,7,88,136,3,7,32,216,7,12,24,40,2,5,112,56,255
        .byte 8,10,56,120,0,1,88,48,8,11,56,160,6,15,88,80,6,15,88,192,255
        .byte 4,14,96,24,8,10,8,80,8,11,16,112,8,10,8,144,8,11,16,176,2,3,112,80,2,3,112,160,0,2,32,48,255
        .byte 0,2,24,40,3,7,16,112,3,8,104,160,7,12,72,24,4,14,32,96,4,14,48,144,3,7,32,72,255
        .byte 1,9,120,40,3,7,56,64,3,7,72,184,0,2,112,24,1,9,120,200,255
        .byte 0,2,112,24,4,14,104,72,6,15,112,88,255
        .byte 0,2,88,16,3,7,80,200,4,14,32,24,4,14,32,96,4,14,32,232,4,14,72,120,6,15,64,56,6,15,56,160,6,15,112,104,8,10,16,56,8,11,16,160,255
        .byte 0,2,64,8,6,15,64,40,255
        .byte 0,2,64,16,2,3,56,104,2,3,56,176,7,12,32,208,255
        .byte 0,2,64,8,3,7,32,40,3,7,32,144,3,8,96,88,3,8,96,200,7,12,64,120,6,15,64,64,6,15,64,176,255
        .byte 0,2,64,16,6,15,64,64,6,15,64,160,8,10,48,112,255
        .byte 0,2,64,8,7,12,56,56,2,5,120,176,2,5,120,96,3,8,96,192,7,12,32,216,255
        .byte 0,1,64,232,3,8,24,104,3,8,56,184,3,8,40,144,6,13,32,64,255
        .byte 0,2,64,8,6,13,48,56,255
        .byte 0,1,64,8,255
        .byte 0,2,8,32,8,10,40,184,8,11,40,88,6,15,64,104,6,15,64,168,255
        .byte 0,1,64,8,5,7,72,104,5,7,72,144,7,12,120,48,6,13,96,208,4,14,88,80,4,14,88,176,255
        .byte 0,1,8,224,255
        .byte 0,1,64,224,255
        .byte 8,10,40,112,8,11,40,144,0,2,64,8,6,15,64,40,255
        .byte 0,2,64,48,7,12,64,72,255
.out .sprintf("POSITIONS SIZE = %d", (* - nmedat))

NUMOBJ = 17
objdta:
        .byte 76,73,67,200,121,158,4,32,25,28,5,160,8,144,1,0,1,128,0,128,1,0,0,128,1,0,0,128,1,0,0,128,3,80,136,3,80,136
        .byte 0,0,7,128,24,96,99,16,12,192,51,60,18,32,8,192,7,64,130,64,68,0,56,0,0,0,0,0,0,0,0,0,4,80,208,4,80,208
        .byte 3,128,4,64,4,64,4,64,2,128,2,128,17,16,21,80,17,16,0,0,1,0,0,0,1,0,0,0,3,128,0,0,8,24,40,8,24,40
        .byte 0,0,0,0,1,0,5,0,5,0,1,6,14,9,243,8,126,132,14,130,5,1,0,121,7,254,15,254,127,78,176,60,11,72,24,11,72,24
        .byte 0,0,7,128,24,96,99,16,12,192,51,60,18,32,8,192,7,64,130,64,68,0,56,0,0,0,0,0,0,0,0,0,16,32,208,16,32,208
        .byte 0,0,0,0,1,0,5,0,5,0,1,6,14,9,243,8,126,132,14,130,5,1,0,121,7,254,15,254,127,78,176,60,19,56,56,19,56,56
        .byte 32,4,80,10,40,20,24,24,44,52,23,232,111,246,27,216,21,168,39,228,23,232,39,228,19,200,42,84,17,136,8,16,28,64,72,28,64,72
        .byte 0,0,0,0,0,0,0,0,0,0,15,224,48,24,65,4,176,26,79,228,36,72,17,16,8,32,5,64,2,128,1,0,20,32,32,20,32,32
        .byte 0,0,0,0,0,0,0,0,0,0,15,224,48,24,65,4,176,26,79,228,36,72,17,16,8,32,5,64,2,128,1,0,24,96,208,24,96,208
        .byte 0,0,0,0,0,0,0,0,0,0,15,224,48,24,65,4,176,26,79,228,36,72,17,16,8,32,5,64,2,128,1,0,1,16,216,1,16,216
        .byte 0,0,0,0,0,0,0,0,0,0,15,224,48,24,65,4,176,26,79,228,36,72,17,16,8,32,5,64,2,128,1,0,21,48,56,21,48,56
        .byte 0,0,0,0,0,0,0,0,0,0,15,224,48,24,65,4,176,26,79,228,36,72,17,16,8,32,5,64,2,128,1,0,0,40,136,0,40,136
        .byte 76,73,67,200,121,158,4,32,25,28,5,160,8,144,1,0,1,128,0,128,1,0,0,128,1,0,0,128,1,0,0,128,17,64,120,17,64,120
        .byte 32,4,80,10,40,20,24,24,44,52,23,232,111,246,27,216,21,168,39,228,23,232,39,228,19,200,42,84,17,136,8,16,0,80,200,0,80,200
        .byte 32,4,80,10,40,20,24,24,44,52,23,232,111,246,27,216,21,168,39,228,23,232,39,228,19,200,42,84,17,136,8,16,0,80,72,0,80,72
        .byte 3,128,4,64,4,64,4,64,2,128,2,128,17,16,21,80,17,16,0,0,1,0,0,0,1,0,0,0,3,128,0,0,24,120,48,24,120,48
        .byte 3,128,4,64,4,64,4,64,2,128,2,128,17,16,21,80,17,16,0,0,1,0,0,0,1,0,0,0,3,128,0,0,19,32,216,19,32,216
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
        .byte 126,66,66,66,66,2,126,0
        .byte 24,8,8,8,8,8,12,0
        .byte 252,130,2,126,128,128,254,0
        .byte 126,194,4,24,4,194,126,0
        .byte 4,12,20,36,64,254,4,0
        .byte 254,130,128,124,2,130,252,0
        .byte 126,128,190,162,130,130,126,0
        .byte 254,4,2,4,8,16,16,0
        .byte 124,68,124,130,130,130,254,0
        .byte 252,130,130,138,122,2,252,0
        .byte 0,0,0,48,0,0,48,0
        .byte 0,0,48,0,0,48,48,96
        .byte 0,0,12,24,48,24,12,0
        .byte 0,0,0,126,0,126,0,0
        .byte 0,0,48,24,12,24,48,0
        .byte 0,124,198,12,24,0,24,0
        .byte 0,124,222,254,254,192,124,0
        .byte 8,20,20,34,34,65,127,0
        .byte 126,68,72,112,72,4,126,0
        .byte 124,66,66,64,64,0,126,0
        .byte 124,66,66,66,66,2,124,0
        .byte 126,34,16,12,16,34,126,0
        .byte 126,66,0,124,64,64,64,0
        .byte 124,66,64,94,66,2,124,0
        .byte 66,66,66,102,90,66,66,0
        .byte 8,8,8,8,8,12,26,0
        .byte 60,8,8,8,8,16,96,0
        .byte 76,68,72,112,72,68,66,0
        .byte 32,32,32,32,32,16,12,0
        .byte 198,170,146,146,146,146,130,0
        .byte 70,66,98,82,74,70,98,0
        .byte 62,66,66,66,66,2,124,0
        .byte 126,66,66,82,92,64,64,0
        .byte 126,66,66,66,66,68,104,6
        .byte 124,66,66,66,84,72,70,0
        .byte 252,68,32,16,8,68,126,0
        .byte 254,84,16,16,16,16,16,0
        .byte 198,66,66,66,66,66,60,0
        .byte 136,134,68,68,40,40,16,0
        .byte 130,146,146,146,146,170,198,0
        .byte 142,68,40,16,40,68,226,0
        .byte 136,134,68,40,16,16,16,0
        .byte 254,132,8,16,32,66,254,0
        .byte 0,30,24,24,24,24,30,0
        .byte 0,0,192,96,48,24,12,0
        .byte 0,240,48,48,48,48,240,0
        .byte 0,48,120,252,48,48,48,0
        .byte 0,0,0,0,0,0,0,255
        .byte 0,60,102,248,96,96,254,0
        .byte 0,126,2,82,26,66,42,0
        .byte 0,255,0,181,162,0,255,0
        .byte 5,11,5,11,0,11,5,11
        .byte 192,224,192,224,0,224,192,224
        .byte 21,39,23,35,19,41,17,8
        .byte 168,228,232,196,200,148,136,16
        .byte 16,16,40,24,44,23,111,27
        .byte 8,8,20,24,52,232,246,216
        .byte 5,243,5,147,160,3,245,3
        .byte 192,239,192,234,9,224,207,224
        .byte 5,83,5,19,4,3,85,3
        .byte 192,234,192,234,8,224,202,224
        .byte 74,82,66,90,66,90,82,66
        .byte 0,254,4,40,100,2,255,0
        .byte 0,127,32,20,38,64,255,0
        .byte 0,0,0,0,0,0,0,0
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
