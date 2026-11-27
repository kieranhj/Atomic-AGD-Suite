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
MAPWID = 13
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255
mapdat:
        .byte 255,0,1,2,3,4,5,6,7,8,9,10,255,255,11,12,13,255,255,255,255,255,255,255,255,255
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255
stmap:  .byte 16

evnt00:
        lda #5
        cmp scno
        beq *+5
        jmp a00496
        lda #40
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp a00066
        lda #5
        sta chary
        lda #15
        sta charx
        lda #28
        jsr dmsg
a00066: lda #64
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp a00114
        tya; lda #8
        sta chary
        ; lda #8
        sta charx
        lda #28
        jsr dmsg
a00114: lda #72
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp a00163
        tya; lda #8
        sta chary
        ; lda #8
        sta charx
        lda #29
        jsr dmsg
a00163: lda #48
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp a00210
        lda #5
        sta chary
        lda #15
        sta charx
        lda #29
        jsr dmsg
a00210: lda #72
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp a00258
        lda #9
        sta chary
        lda #21
        sta charx
        lda #28
        jsr dmsg
a00258: lda #80
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp a00305
        lda #9
        sta chary
        lda #21
        sta charx
        lda #29
        jsr dmsg
a00305: lda #104
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp a00353
        lda #13
        sta chary
        lda #11
        sta charx
        lda #28
        jsr dmsg
a00353: lda #112
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp a00400
        lda #13
        sta chary
        lda #11
        sta charx
        lda #29
        jsr dmsg
a00400: lda #112
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp a00448
        lda #14
        sta chary
        lda #20
        sta charx
        lda #28
        jsr dmsg
a00448: lda #120
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp a00496
        lda #14
        sta chary
        lda #20
        sta charx
        lda #29
        jsr dmsg
a00496: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a00545
:
        jsr vapour
        lda #220
        asl a
        sta sndtyp
        jmp a00545
a00545: lda #10
        cmp scno
        bcs *+5
        jmp a00677
        lda #4
        sta z80_b
        jsr sktyp
        bcs :+
        jmp a00620
:
        lda #3
        sta z80_c
        ldy #12
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #12
        sta (z80_ix),y
a00620: lda #5
        sta z80_b
        jsr sktyp
        bcs :+
        jmp a00677
:
        lda #4
        sta z80_c
        ldy #12
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #12
        sta (z80_ix),y
a00677: lda #3
        sta z80_b
        jsr sktyp
        bcs :+
        jmp a00953
:
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jsr vapour
        lda #8
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
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
        jsr vapour
        lda #8
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda vari
        cmp varf
        bcc *+5
        jmp a00917
        lda #15
        sta z80_c
        ldy #12
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #12
        sta (z80_ix),y
        lda #1
        sta vark
        jmp a00953
a00917: lda #4
        sta z80_c
        ldy #12
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #12
        sta (z80_ix),y
a00953: jsr cangu
        beq :+
        jmp a01029
:
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01029
:
        lda #15
        sta z80_c
        ldy #12
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #12
        sta (z80_ix),y
a01029: lda varj
        clc
        adc #1
        sta varj
        lda #4
        cmp varj
        bcc *+5
        jmp a01078
        lda #0
        jsr animsp
        lda #0
        sta varj
a01078: lda #13
        cmp scno
        bne *+5
        jmp a01336
        lda joyval
        and #1
        beq :+
        jmp a01163
:
        lda #2
        ldy #6
        sta (z80_ix),y
        tya; lda #6
        sta z80_c
        ldy #11
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #11
        sta (z80_ix),y
a01163: lda joyval
        and #2
        beq :+
        jmp a01229
:
        lda #6
        sta z80_c
        ldy #11
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #11
        sta (z80_ix),y
        lda #3
        ldy #6
        sta (z80_ix),y
a01229: lda joyval
        and #4
        beq :+
        jmp a01282
:
        lda #5
        sta z80_c
        ldy #12
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #12
        sta (z80_ix),y
a01282: lda joyval
        and #8
        beq :+
        jmp a01336
:
        lda #5
        sta z80_c
        ldy #12
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #12
        sta (z80_ix),y
a01336: lda #63
        ldy #11
        cmp (z80_ix),y
        bcs *+5
        jmp a01377
        jsr vapour
        lda #63
        ; ldy #11
        sta (z80_ix),y
a01377: lda #192
        ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp a01419
        jsr vapour
        lda #192
        ; ldy #11
        sta (z80_ix),y
a01419: lda #48
        ldy #12
        cmp (z80_ix),y
        bcs *+5
        jmp a01461
        jsr vapour
        lda #48
        ; ldy #12
        sta (z80_ix),y
a01461: lda #160
        ldy #12
        cmp (z80_ix),y
        bcc *+5
        jmp a01503
        jsr vapour
        lda #150
        ; ldy #12
        sta (z80_ix),y
a01503: lda #128
        ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp a01599
        lda #2
        sta z80_c
        ; ldy #11
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #11
        sta (z80_ix),y
        lda #127
        ; ldy #11
        cmp (z80_ix),y
        bcs *+5
        jmp a01599
        lda #128
        ; ldy #11
        sta (z80_ix),y
a01599: lda #127
        ldy #11
        cmp (z80_ix),y
        bcs *+5
        jmp a01697
        lda #2
        sta z80_c
        ; ldy #11
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #11
        sta (z80_ix),y
        lda #128
        ; ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp a01697
        ; lda #128
        ; ldy #11
        sta (z80_ix),y
a01697: lda #96
        ldy #12
        cmp (z80_ix),y
        bcc *+5
        jmp a01793
        lda #2
        sta z80_c
        ; ldy #12
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #12
        sta (z80_ix),y
        lda #95
        ; ldy #12
        cmp (z80_ix),y
        bcs *+5
        jmp a01793
        lda #96
        ; ldy #12
        sta (z80_ix),y
a01793: lda #95
        ldy #12
        cmp (z80_ix),y
        bcs *+5
        jmp a01889
        lda #2
        sta z80_c
        ; ldy #12
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #12
        sta (z80_ix),y
        lda #96
        ; ldy #12
        cmp (z80_ix),y
        bcc *+5
        jmp a01889
        ; lda #96
        ; ldy #12
        sta (z80_ix),y
a01889: lda #3
        sta loopa
a01897: ldy #11
        lda (z80_ix),y
        sta varb
        lda #128
        cmp varb
        bcc *+5
        jmp a02079
        ; lda #128
        sta z80_c
        lda varb
        sec
        sbc z80_c
        sta varb
        lda varb
        sta z80_c
        lda varc
        clc
        adc z80_c
        sta varc
        lda #192
        cmp varc
        bcc *+5
        jmp a02075
        lda #64
        sta z80_c
        lda varc
        sec
        sbc z80_c
        sta varc
        jsr cangr
        beq :+
        jmp a02061
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp a02075
a02061: lda #128
        ldy #11
        sta (z80_ix),y
a02075: jmp a02256
a02079: lda #127
        cmp varb
        bcs *+5
        jmp a02256
        lda #128
        sta varb
        ldy #11
        lda (z80_ix),y
        sta z80_c
        lda varb
        sec
        sbc z80_c
        sta varb
        lda varb
        sta z80_c
        lda varc
        sec
        sbc z80_c
        sta varc
        lda #64
        cmp varc
        bcs *+5
        jmp a02256
        ; lda #64
        sta z80_c
        lda varc
        clc
        adc z80_c
        sta varc
        jsr cangl
        beq :+
        jmp a02242
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp a02256
a02242: lda #128
        ldy #11
        sta (z80_ix),y
a02256: ldy #12
        lda (z80_ix),y
        sta vard
        lda #96
        cmp vard
        bcc *+5
        jmp a02438
        ; lda #96
        sta z80_c
        lda vard
        sec
        sbc z80_c
        sta vard
        lda vard
        sta z80_c
        lda vare
        clc
        adc z80_c
        sta vare
        lda #192
        cmp vare
        bcc *+5
        jmp a02433
        lda #64
        sta z80_c
        lda vare
        sec
        sbc z80_c
        sta vare
        jsr cangd
        beq :+
        jmp a02419
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp a02433
a02419: lda #96
        ldy #12
        sta (z80_ix),y
a02433: jmp a02614
a02438: lda #95
        cmp vard
        bcs *+5
        jmp a02614
        lda #96
        sta vard
        ldy #12
        lda (z80_ix),y
        sta z80_c
        lda vard
        sec
        sbc z80_c
        sta vard
        lda vard
        sta z80_c
        lda vare
        sec
        sbc z80_c
        sta vare
        lda #48
        cmp vare
        bcs *+5
        jmp a02614
        ; lda #48
        sta z80_c
        lda vare
        clc
        adc z80_c
        sta vare
        jsr cangu
        beq :+
        jmp a02600
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp a02614
a02600: lda #96
        ldy #12
        sta (z80_ix),y
a02614: dec loopa
        beq :+
        jmp a01897
:
        lda #7
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp a02664
        tya; lda #8
        ; ldy #8
        sta (z80_ix),y
a02664: lda #160
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a02702
        ; lda #160
        ; ldy #8
        sta (z80_ix),y
a02702: lda #8
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp a02738
        ; lda #8
        ; ldy #9
        sta (z80_ix),y
a02738: lda #240
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp a02775
        ; lda #240
        ; ldy #9
        sta (z80_ix),y
a02775: ldy #8
        lda (z80_ix),y
        sta varf
        iny; ldy #9
        lda (z80_ix),y
        sta varg
        rts
.out .sprintf("EVENT 0 SIZE = %d", (* - evnt00))

evnt01:
        lda #15
        cmp varm
        bcs *+5
        jmp b00046
        lda #8
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
b00046: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00109
:
        lda #1
        sta varn
        lda #9
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        jmp b00436
b00109: lda #0
        sta varn
        lda #3
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00164
:
        lda #150
        asl a
        sta sndtyp
        lda #0
        sta varm
        jmp b00436
b00164: lda #15
        cmp varm
        bcc *+5
        jmp b00436
        ldy #9
        lda (z80_ix),y
        cmp varl
        bcs *+5
        jmp b00221
        lda #6
        ldy #6
        sta (z80_ix),y
        jmp b00234
b00221: lda #7
        ldy #6
        sta (z80_ix),y
b00234: ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
        lda #4
        ; ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp b00292
        lda #0
        jsr animsp
b00292: lda #5
        ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp b00328
        lda #2
        ldy #7
        sta (z80_ix),y
b00328: lda #25
        ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp b00364
        lda #3
        ldy #7
        sta (z80_ix),y
b00364: lda #35
        ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp b00399
        lda #0
        ldy #7
        sta (z80_ix),y
b00399: lda #45
        ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp b00436
        lda #0
        ; ldy #11
        sta (z80_ix),y
b00436: rts
.out .sprintf("EVENT 1 SIZE = %d", (* - evnt01))

evnt02:
        lda vari
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp c00120
        lda varl
        iny; ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp c00066
        lda #0
        ldy #11
        sta (z80_ix),y
        jmp c00080
c00066: lda #1
        ldy #11
        sta (z80_ix),y
c00080: lda #2
        sta z80_c
        ldy #12
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #12
        sta (z80_ix),y
        jmp c00232
c00120: lda #3
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00232
:
        lda varl
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp c00183
        lda #0
        ldy #11
        sta (z80_ix),y
        jmp c00196
c00183: lda #1
        ldy #11
        sta (z80_ix),y
c00196: lda #2
        sta z80_c
        ldy #12
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #12
        sta (z80_ix),y
c00232: ldy #12
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #12
        sta (z80_ix),y
        lda #10
        ; ldy #12
        cmp (z80_ix),y
        bcc *+5
        jmp c00304
        lda #0
        jsr animsp
        lda #1
        ldy #12
        sta (z80_ix),y
c00304: lda vari
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp c00619
        lda #1
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c00619
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp c00446
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp c00481
c00446: lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
c00481: jsr vapour
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp c00548
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp c00583
c00548: lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
c00583: lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
c00619: lda #4
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c00866
        lda #3
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00680
:
        lda #40
        asl a
        sta sndtyp
        jmp c00692
c00680: lda #3
        asl a
        sta sndtyp
c00692: lda #10
        ldy #6
        sta (z80_ix),y
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp c00799
        jsr cangl
        beq :+
        jmp c00781
:
        lda #2
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp c00794
c00781: lda #1
        ldy #11
        sta (z80_ix),y
c00794: jmp c00866
c00799: jsr cangr
        beq :+
        jmp c00853
:
        lda #2
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp c00866
c00853: lda #0
        ldy #11
        sta (z80_ix),y
c00866: lda #3
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00919
:
        lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp c00915
        jmp c00915
c00915: jmp c00970
c00919: lda vari
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp c00970
        lda #1
        dey; ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp c00970
        jmp c00970
c00970: rts
.out .sprintf("EVENT 2 SIZE = %d", (* - evnt02))

evnt03:
        lda #13
        cmp scno
        beq *+5
        jmp d00103
        lda #1
        ldy #11
        cmp (z80_ix),y
        bcs *+5
        jmp d00103
        ldy #9
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #9
        sta (z80_ix),y
        lda #72
        dey; ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp d00103
        lda #3
        asl a
        sta sndtyp
d00103: lda #2
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00129
:
        jsr vapour
d00129: lda #152
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp d00191
        jsr vapour
        lda #4
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp d00191
        lda #40
        asl a
        sta sndtyp
d00191: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp d00399
:
        lda #15
        jsr delay
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
        lda #80
        asl a
        sta sndtyp
        lda vars
        clc
        adc #1
        sta vars
        jsr cls
        lda #25
        jsr delay
        lda #0
        sta varo
        lda #3
        sta numlif
        lda #12
        sta scno
        jsr nwscr
        lda #1
        sta restfl
        jmp d02479
d00399: lda #0
        cmp numlif
        bcc *+5
        jmp d00537
        lda #50
        cmp varo
        bcc *+5
        jmp d00537
        lda #98
        cmp varo
        bcs *+5
        jmp d00537
        lda #4
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp d00485
        lda varo
        asl a
        sta sndtyp
d00485: lda #23
        sta chary
        lda #12
        sta charx
        lda #1
        ldy #11
        cmp (z80_ix),y
        bcs *+5
        jmp d00529
        jmp d00529
d00529: lda #1
        jsr dmsg
d00537: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp d00581
:
        lda #2
        sta varh
        lda #1
        sta nexlev
d00581: lda #100
        cmp varo
        beq *+5
        jmp d00697
        lda #13
        cmp scno
        bne *+5
        jmp d00637
        lda #23
        sta chary
        lda #13
        sta charx
        jsr dscor
d00637: lda #152
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp d00667
        lda #0
        sta varo
d00667: lda #2
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00697
:
        lda #0
        sta varo
d00697: lda #0
        cmp varo
        beq *+5
        jmp d00727
        lda #11
        ldy #6
        sta (z80_ix),y
d00727: lda #40
        cmp varo
        beq *+5
        jmp d00809
        lda #50
        asl a
        sta sndtyp
        lda numlif
        sec
        sbc #1
        sta numlif
        lda #9
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        lda #50
        sta varo
d00809: lda #50
        cmp varo
        beq *+5
        jmp d00875
        lda #2
        sta varh
        lda #1
        ldy #6
        sta (z80_ix),y
        lda #40
        ldy #8
        sta (z80_ix),y
        lda #24
        iny; ldy #9
        sta (z80_ix),y
d00875: lda #0
        cmp numlif
        bcs *+5
        jmp d01211
        jsr cls
        lda #60
        asl a
        sta sndtyp
        lda #48
        ldy #8
        sta (z80_ix),y
        lda #124
        iny; ldy #9
        sta (z80_ix),y
        lda #3
        sta z80_c
        lda #17
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #10
        sta chary
        lda #12
        sta charx
        lda #0
        jsr dmsg
        lda #12
        sta chary
        lda #10
        sta charx
        lda #20
        jsr dmsg
        lda #12
        sta chary
        lda #17
        sta charx
        jsr dscor
        lda #50
        jsr delay
        jsr prskey
        lda #50
        sta varo
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
        sta varj
        lda #2
        sta varh
        lda #25
        sta varm
        lda #0
        sta varn
        lda #50
        sta varo
        lda #0
        sta varp
        lda #5
        sta varq
        lda #0
        sta vars
        lda #3
        sta numlif
        jsr cls
        lda #0
        sta scno
        jsr nwscr
        lda #1
        sta restfl
d01211: lda #50
        cmp varo
        bcc *+5
        jmp d01245
        lda #99
        cmp varo
        bcs *+5
        jmp d01245
d01245: lda #13
        cmp scno
        beq *+5
        jmp d01384
        lda #5
        cmp varm
        beq *+5
        jmp d01303
        lda #19
        sta chary
        lda #2
        sta charx
        lda #22
        jsr dmsg
d01303: lda #100
        sta varo
        lda #210
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp d01384
        jsr cls
        lda #50
        jsr delay
        lda #0
        sta scno
        jsr nwscr
        lda #1
        sta restfl
        lda #50
        sta varo
        lda #25
        sta varm
d01384: ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
        lda #4
        ; ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp d01457
        lda #0
        ; ldy #11
        sta (z80_ix),y
        ; lda #0
        jsr animsp
d01457: ldy #8
        lda (z80_ix),y
        sta vari
        iny; ldy #9
        lda (z80_ix),y
        sta varl
        lda #100
        cmp varo
        beq *+5
        jmp d02479
        lda #13
        cmp scno
        bne *+5
        jmp d01772
        lda #4
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d01646
:
        lda #<3
        sta z80_l
        lda #>3
        sta z80_h
        jsr addsc
        lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp d01597
        lda #5
        asl a
        sta sndtyp
d01597: jsr cangu
        beq :+
        jmp d01646
:
        lda #2
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
d01646: lda #5
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d01772
:
        lda #<5
        sta z80_l
        lda #>5
        sta z80_h
        jsr addsc
        lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp d01723
        lda #5
        asl a
        sta sndtyp
d01723: jsr cangu
        beq :+
        jmp d01772
:
        lda #2
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
d01772: lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d01855
:
        lda #1
        cmp varn
        bne *+5
        jmp d01855
        jsr cangr
        beq :+
        jmp d01833
:
        jsr gravst
        jsr jump
d01833: jsr cangl
        beq :+
        jmp d01855
:
        jsr gravst
        jsr jump
d01855: lda #1
        cmp varh
        beq *+5
        jmp d01986
        jsr cangl
        beq :+
        jmp d01978
:
        lda #2
        cmp varm
        bcs *+5
        jmp d01947
        jsr vapour
        lda #2
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp d01973
d01947: ldy #9
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #9
        sta (z80_ix),y
d01973: jmp d01986
d01978: lda #2
        sta varh
d01986: lda #2
        cmp varh
        beq *+5
        jmp d02117
        jsr cangr
        beq :+
        jmp d02109
:
        lda #2
        cmp varm
        bcs *+5
        jmp d02078
        jsr vapour
        lda #2
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp d02104
d02078: ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
d02104: jmp d02117
d02109: lda #1
        sta varh
d02117: lda #1
        cmp vark
        beq *+5
        jmp d02151
        jsr gravst
        jsr jump
        lda #0
        sta vark
d02151: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d02281
:
        ldy #8
        lda (z80_ix),y
        cmp varf
        bcc *+5
        jmp d02264
        lda #7
        asl a
        sta sndtyp
        jsr gravst
        jsr jump
        lda varg
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp d02251
        lda #1
        sta varh
        jmp d02259
d02251: lda #2
        sta varh
d02259: jmp d02276
d02264: lda varj
        asl a
        sta sndtyp
d02276: jmp d02447
d02281: jsr cangu
        beq :+
        jmp d02447
:
        lda #1
        cmp varh
        beq *+5
        jmp d02329
        lda #5
        ldy #6
        sta (z80_ix),y
        jmp d02343
d02329: lda #1
        ldy #6
        sta (z80_ix),y
d02343: jsr cangd
        beq :+
        jmp d02447
:
        lda #1
        cmp varh
        beq *+5
        jmp d02386
        lda #4
        ldy #6
        sta (z80_ix),y
d02386: lda #2
        cmp varh
        beq *+5
        jmp d02416
        lda #0
        ldy #6
        sta (z80_ix),y
d02416: ldy #8
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #8
        sta (z80_ix),y
        jsr ifall
d02447: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp d02479
:
        jmp d02479
d02479: lda #8
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp d02528
        ; ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
d02528: jmp grav
.out .sprintf("EVENT 3 SIZE = %d", (* - evnt03))

evnt04:
        lda #1
        cmp scno
        beq *+5
        jmp e00285
        lda #3
        cmp numlif
        beq *+5
        jmp e00285
        lda #51
        cmp varo
        beq *+5
        jmp e00119
        lda #0
        cmp vars
        beq *+5
        jmp e00119
        lda #12
        ldy #8
        sta (z80_ix),y
        lda #72
        iny; ldy #9
        sta (z80_ix),y
        lda #6
        sta z80_c
        lda #1
        sta z80_b
        jsr spawn
e00119: lda #51
        cmp varo
        beq *+5
        jmp e00202
        lda #0
        cmp vars
        beq *+5
        jmp e00202
        lda #24
        ldy #8
        sta (z80_ix),y
        lda #56
        iny; ldy #9
        sta (z80_ix),y
        lda #6
        sta z80_c
        lda #1
        sta z80_b
        jsr spawn
e00202: lda #51
        cmp varo
        beq *+5
        jmp e00285
        lda #1
        cmp vars
        beq *+5
        jmp e00285
        lda #12
        ldy #8
        sta (z80_ix),y
        lda #72
        iny; ldy #9
        sta (z80_ix),y
        lda #6
        sta z80_c
        lda #1
        sta z80_b
        jsr spawn
e00285: lda #11
        cmp scno
        beq *+5
        jmp e00361
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
        lda #15
        ldy #6
        sta (z80_ix),y
        lda #1
        iny; ldy #7
        sta (z80_ix),y
        jmp e00389
e00361: lda #12
        ldy #6
        sta (z80_ix),y
        lda #1
        iny; ldy #7
        sta (z80_ix),y
e00389: ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        tya; lda #8
        ; ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp e00456
        lda #0
        sta varp
        lda #255
        ldy #5
        sta (z80_ix),y
e00456: lda #234
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp e00500
        lda #0
        sta varp
        lda #255
        ldy #5
        sta (z80_ix),y
e00500: rts
.out .sprintf("EVENT 4 SIZE = %d", (* - evnt04))

evnt05:
        lda #11
        cmp scno
        beq *+5
        jmp f00078
        lda #15
        ldy #6
        sta (z80_ix),y
        lda #1
        iny; ldy #7
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #9
        sta (z80_ix),y
        jmp f00369
f00078: lda #10
        cmp scno
        bcc *+5
        jmp f00099
        jmp f00143
f00099: lda #0
        cmp scno
        bne *+5
        jmp f00143
        lda #158
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp f00143
        jsr vapour
f00143: ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
        lda #4
        ; ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp f00207
        lda #0
        ; ldy #11
        sta (z80_ix),y
f00207: lda #1
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp f00255
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
f00255: lda #2
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp f00317
        ldy #9
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #9
        sta (z80_ix),y
        lda #4
        ldy #11
        sta (z80_ix),y
f00317: lda #13
        ldy #6
        sta (z80_ix),y
        lda #1
        iny; ldy #7
        sta (z80_ix),y
        iny; ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
f00369: ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        tya; lda #8
        ; ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp f00436
        lda #0
        sta varp
        lda #255
        ldy #5
        sta (z80_ix),y
f00436: lda #8
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp f00480
        lda #0
        sta varp
        lda #255
        ldy #5
        sta (z80_ix),y
f00480: rts
.out .sprintf("EVENT 5 SIZE = %d", (* - evnt05))

evnt06:
        lda #0
        cmp scno
        beq *+5
        jmp g00054
        lda #5
        cmp varq
        beq *+5
        jmp g00040
        jmp g00054
g00040: lda #255
        ldy #5
        sta (z80_ix),y
g00054: lda #10
        cmp scno
        bcs *+5
        jmp g00087
        lda #0
        cmp scno
        bcc *+5
        jmp g00087
g00087: ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
        lda #4
        ; ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp g00159
        lda #0
        jsr animbk
        lda #0
        ldy #11
        sta (z80_ix),y
g00159: lda #12
        cmp scno
        beq *+5
        jmp g00453
        ; lda #12
        sta chary
        lda #9
        sta charx
        lda #32
        jsr dmsg
        lda #14
        sta chary
        lda #9
        sta charx
        lda #20
        jsr dmsg
        lda #14
        sta chary
        lda #16
        sta charx
        lda vars
        jsr disply
        lda #14
        sta chary
        lda #18
        sta charx
        lda #21
        jsr dmsg
        lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp g00338
        lda #25
        asl a
        sta sndtyp
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
g00338: lda #90
        cmp varo
        beq *+5
        jmp g00453
        lda #3
        cmp vars
        beq *+5
        jmp g00419
        jsr cls
        lda #100
        jsr delay
        lda #11
        sta scno
        jsr nwscr
        lda #1
        sta restfl
        lda #0
        sta varo
        jmp g00453
g00419: jsr cls
        lda #1
        sta scno
        jsr nwscr
        ; lda #1
        sta restfl
        lda #50
        jsr delay
g00453: lda #11
        cmp scno
        beq *+5
        jmp g00975
        lda #2
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp g00538
        lda #3
        cmp vars
        beq *+5
        jmp g00529
        lda #11
        asl a
        sta sndtyp
        lda #0
        sta vars
g00529: jsr jump
        jmp g00542
g00538: jsr ifall
g00542: lda #80
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp g00572
        lda #3
        sta vars
g00572: lda #1
        cmp varo
        beq *+5
        jmp g00736
        lda #0
        sta chary
        ; lda #0
        sta charx
        lda #33
        jsr dmsg
        lda #8
        sta chary
        ; lda #8
        sta charx
        lda #35
        jsr dmsg
        lda #12
        sta chary
        lda #2
        sta charx
        lda #34
        jsr dmsg
        lda #17
        sta chary
        lda #6
        sta charx
        lda #12
        jsr dmsg
        lda #17
        sta chary
        lda #19
        sta charx
        jsr dscor
        lda #19
        sta chary
        lda #11
        sta charx
        lda #0
        jsr dmsg
g00736: lda #100
        cmp varo
        beq *+5
        jmp g00975
        lda #22
        sta chary
        lda #11
        sta charx
        lda #18
        jsr dmsg
        lda joyval
        and #2
        beq :+
        jmp g00975
:
        lda #45
        asl a
        sta sndtyp
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
        sta varj
        lda #2
        sta varh
        lda #0
        sta vark
        ; lda #0
        sta varm
        ; lda #0
        sta varn
        lda #100
        sta varo
        lda #0
        sta varp
        lda #5
        sta varq
        lda #0
        sta vars
        ; lda #0
        sta vart
        lda #3
        sta numlif
        jsr cls
        lda #120
        jsr delay
        lda #13
        sta scno
        jsr nwscr
        lda #1
        sta restfl
g00975: jmp grav
.out .sprintf("EVENT 6 SIZE = %d", (* - evnt06))

evnt07:
        lda #13
        cmp scno
        beq *+5
        jmp h00278
        lda #7
        cmp varm
        bcc *+5
        jmp h00155
        lda #168
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp h00155
        lda #4
        sta z80_c
        iny; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #1
        cmp varrnd
        beq *+5
        jmp h00155
        lda #5
        asl a
        sta sndtyp
h00155: lda #240
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp h00237
        lda #12
        sta z80_c
        dey; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #0
        iny; ldy #9
        sta (z80_ix),y
        lda #5
        jsr delay
h00237: lda #176
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp h00273
        lda #255
        ldy #5
        sta (z80_ix),y
h00273: jmp h00765
h00278: lda #3
        sta z80_b
        jsr sktyp
        bcs :+
        jmp h00312
:
        jsr jump
        lda #25
        sta varr
h00312: lda #25
        cmp varr
        beq *+5
        jmp h00477
        lda #154
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp h00378
        ; ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
h00378: lda #128
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp h00427
        ; ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
h00427: lda #128
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp h00477
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #9
        sta (z80_ix),y
h00477: lda #25
        cmp varr
        bne *+5
        jmp h00616
        ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
        lda #71
        ; ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp h00558
        lda #50
        ; ldy #11
        sta (z80_ix),y
h00558: lda #66
        ldy #11
        cmp (z80_ix),y
        bcs *+5
        jmp h00598
        lda #0
        ldy #7
        sta (z80_ix),y
        jmp h00611
h00598: lda #1
        ldy #7
        sta (z80_ix),y
h00611: jmp h00629
h00616: lda #1
        ldy #7
        sta (z80_ix),y
h00629: lda #154
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp h00765
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #98
        sta varo
        lda #40
        asl a
        sta sndtyp
        lda #23
        sta chary
        lda #13
        sta charx
        jsr dscor
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
        jsr vapour
        lda #0
        sta varr
h00765: jmp grav
.out .sprintf("EVENT 7 SIZE = %d", (* - evnt07))

evnt08:
        lda #0
        cmp scno
        beq *+5
        jmp i00023
        jmp i00023
i00023: ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
        lda #4
        ; ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp i00095
        lda #0
        jsr animsp
        lda #0
        ldy #11
        sta (z80_ix),y
i00095: lda #51
        cmp varo
        beq *+5
        jmp i00212
        lda #8
        sta chary
        lda #12
        sta charx
        lda #27
        jsr dmsg
        lda #10
        sta chary
        lda #11
        sta charx
        lda #3
        jsr dmsg
        lda #12
        sta chary
        lda #11
        sta charx
        lda #4
        jsr dmsg
        lda #14
        sta chary
        lda #11
        sta charx
        lda #5
        jsr dmsg
i00212: lda #54
        cmp varo
        beq *+5
        jmp i00246
        jsr prskey
        lda #200
        asl a
        sta sndtyp
i00246: lda #55
        cmp varo
        bcs *+5
        jmp i00446
        lda joyval
        and #16
        beq :+
        jmp i00324
:
        lda #72
        ldy #8
        sta (z80_ix),y
        lda #10
        sta chary
        lda #11
        sta charx
        lda #3
        jsr dmsg
        jmp i00441
i00324: lda joyval
        and #32
        beq :+
        jmp i00385
:
        lda #88
        ldy #8
        sta (z80_ix),y
        lda #12
        sta chary
        lda #11
        sta charx
        lda #4
        jsr dmsg
        jmp i00441
i00385: lda joyval
        and #64
        beq :+
        jmp i00441
:
        lda #106
        ldy #8
        sta (z80_ix),y
        lda #14
        sta chary
        lda #11
        sta charx
        lda #5
        jsr dmsg
i00441: jmp i01107
i00446: lda #5
        cmp varq
        beq *+5
        jmp i00720
        lda #70
        cmp varo
        beq *+5
        jmp i00720
        ; lda #70
        asl a
        sta sndtyp
        lda #88
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp i00545
        lda #10
        sta chary
        lda #11
        sta charx
        lda #30
        jsr dmsg
        jmp i00645
i00545: lda #106
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp i00597
        lda #10
        sta chary
        lda #11
        sta charx
        lda #31
        jsr dmsg
        jmp i00645
i00597: lda #72
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp i00645
        lda #10
        sta chary
        lda #11
        sta charx
        lda #23
        jsr dmsg
i00645: lda #8
        sta chary
        lda #11
        sta charx
        lda #26
        jsr dmsg
        lda #12
        sta chary
        lda #11
        sta charx
        lda #24
        jsr dmsg
        lda #14
        sta chary
        lda #11
        sta charx
        lda #25
        jsr dmsg
i00720: lda #70
        cmp varo
        bcc *+5
        jmp i01107
        lda #88
        ldy #8
        sta (z80_ix),y
        lda #5
        cmp varq
        beq *+5
        jmp i01009
        lda joyval
        and #16
        beq :+
        jmp i00847
:
        lda #200
        asl a
        sta sndtyp
        lda #1
        sta varq
        lda #99
        sta varo
        jsr cls
        lda #76
        ldy #8
        sta (z80_ix),y
        lda #120
        iny; ldy #9
        sta (z80_ix),y
i00847: lda joyval
        and #32
        beq :+
        jmp i00926
:
        lda #200
        asl a
        sta sndtyp
        lda #0
        sta varq
        lda #99
        sta varo
        jsr cls
        lda #76
        ldy #8
        sta (z80_ix),y
        lda #120
        iny; ldy #9
        sta (z80_ix),y
i00926: lda joyval
        and #64
        beq :+
        jmp i01005
:
        lda #200
        asl a
        sta sndtyp
        lda #2
        sta varq
        lda #99
        sta varo
        jsr cls
        lda #76
        ldy #8
        sta (z80_ix),y
        lda #120
        iny; ldy #9
        sta (z80_ix),y
i01005: jmp i01107
i01009: lda #100
        cmp varo
        beq *+5
        jmp i01107
        lda #9
        sta chary
        lda #14
        sta charx
        lda #37
        jsr dmsg
        lda #8
        sta chary
        lda #7
        sta charx
        lda #36
        jsr dmsg
        lda #110
        jsr delay
        lda #1
        sta scno
        jsr nwscr
        ; lda #1
        sta restfl
i01107: rts
.out .sprintf("EVENT 8 SIZE = %d", (* - evnt08))

evnt09:
        rts
.out .sprintf("EVENT 9 SIZE = %d", (* - evnt09))

evnt10:
        lda #2
        cmp varq
        beq *+5
        jmp k00027
        lda #50
        sta clock
k00027: lda #1
        cmp varq
        beq *+5
        jmp k00052
        ; lda #1
        jsr delay
k00052: lda #0
        cmp varp
        beq *+5
        jmp k00595
        lda #180
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #3
        cmp varrnd
        bcs *+5
        jmp k00332
        lda #4
        cmp scno
        bcc *+5
        jmp k00147
        lda #0
        sta varp
        jmp k00155
k00147: lda #1
        sta varp
k00155: lda #4
        sta z80_c
        lda #12
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #4
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp k00314
        lda #160
        ldy #8
        sta (z80_ix),y
        lda #224
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        iny; ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
k00314: lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
k00332: lda #180
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #3
        cmp varrnd
        bcs *+5
        jmp k00595
        lda #8
        cmp scno
        bcc *+5
        jmp k00410
        lda #0
        sta varp
        jmp k00418
k00410: lda #1
        sta varp
k00418: lda #5
        sta z80_c
        lda #13
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #5
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp k00577
        lda #160
        ldy #8
        sta (z80_ix),y
        lda #224
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        iny; ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
k00577: lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
k00595: lda #1
        cmp scno
        beq *+5
        jmp k01018
        lda #50
        cmp varo
        beq *+5
        jmp k01018
        lda #0
        sta chary
        lda #13
        sta charx
        lda #2
        jsr dmsg
        lda #1
        sta chary
        lda #23
        sta loopa
k00670: lda #0
        sta charx
        lda #13
        jsr dmsg
        dec loopa
        beq :+
        jmp k00670
:
        lda #1
        sta chary
        lda #23
        sta loopa
k00717: lda #31
        sta charx
        lda #19
        jsr dmsg
        dec loopa
        beq :+
        jmp k00717
:
        lda #0
        sta chary
        lda #1
        sta charx
        lda #14
        jsr dmsg
        lda #0
        sta chary
        lda #22
        sta charx
        lda #15
        jsr dmsg
        lda #23
        sta chary
        lda #1
        sta charx
        lda #16
        jsr dmsg
        lda #23
        sta chary
        lda #30
        sta charx
        lda #16
        jsr dmsg
        lda #0
        sta chary
        ; lda #0
        sta charx
        lda #17
        jsr dmsg
        lda #0
        sta chary
        lda #31
        sta charx
        lda #17
        jsr dmsg
        lda #23
        sta chary
        lda #0
        sta charx
        lda #17
        jsr dmsg
        lda #23
        sta chary
        lda #31
        sta charx
        lda #17
        jsr dmsg
        lda #10
        sta chary
        lda #0
        sta charx
        lda #17
        jsr dmsg
        rts
        lda #1
        sta gamwon
        lda #17
        jsr dmsg
        lda #0
        sta chary
        lda #22
        sta charx
        lda #17
        jsr dmsg
k01018: rts
.out .sprintf("EVENT 10 SIZE = %d", (* - evnt10))

evnt11:
        lda varm
        clc
        adc #1
        sta varm
        lda #0
        sta vars
        ; lda #0
        sta vart
        ; lda #0
        sta varr
        lda #100
        jsr delay
        lda #200
        asl a
        sta sndtyp
        rts
.out .sprintf("EVENT 11 SIZE = %d", (* - evnt11))

evnt12:
        lda #2
        sta varh
        lda #0
        sta varp
        lda #1
        cmp scno
        beq *+5
        jmp m00093
        lda #49
        sta varo
        lda #23
        sta chary
        lda #1
        sta charx
        lda #38
        jsr dmsg
        lda #0
        sta chary
        lda #12
        sta charx
        lda #39
        jsr dmsg
m00093: rts
.out .sprintf("EVENT 12 SIZE = %d", (* - evnt12))

evnt13:
        rts
.out .sprintf("EVENT 13 SIZE = %d", (* - evnt13))

evnt14:
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
        lda #0
        jsr animsp
        lda numlif
        sta varf
        lda #4
        sta varf
        lda #0
        jsr animsp
        jsr cangl
        beq :+
        jmp s00062
:
        jsr redraw
        lda #0
        jsr animsp
s00062: lda charx
        sta clock
        lda #4
        sta clock
        lda #0
        jsr animsp
        lda clock
        sta clock
        lda #4
        sta clock
        lda #0
        jsr animsp
        lda clock
        sta z80_c
        sta z80_b
        jsr spawn
        jsr ifall
        lda #0
        jsr animsp
        lda vara
        sta clock
        lda #0
        jsr animsp
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        lda #0
        jsr animsp
        jsr cangl
        beq :+
        jmp       
:
        lda charx
        sta clock
        lda #4
        sta clock
        lda #0
        jsr animsp
        jsr cangl
        beq :+
        jmp s08114
:
        lda clock
        sta clock
        lda #4
        sta clock
        lda #0
        jsr animsp
        lda clock
        sta z80_c
        sta z80_b
        jsr spawn
        jsr ifall
        lda #0
        jsr animsp
        lda vara
        sta clock
        lda #0
        jsr animsp
        sta z80_l
        lda #0
        sta z80_h
        jsr addsc
        lda #0
        jsr animsp
        jsr cangl
        beq :+
        jmp s08114
:
        lda charx
        sta clock
        lda #4
        sta clock
        lda #0
        jsr animsp
        jsr cangl
        beq :+
        jmp s08114
:
        lda clock
        sta clock
        lda #4
        sta clock
        lda #0
        jsr animsp
        lda clock
        sta z80_c
        sta z80_b
        jsr spawn
        jsr ifall
        lda #0
        jsr animsp
        lda varb
        sta clock
        lda #0
        jsr animsp
        lda chary
        sta clock
        jsr cangl
        beq :+
        jmp s00503
:
        lda #0
        jsr animsp
s00503: lda charx
        sta clock
        lda #4
        sta clock
        lda #0
        jsr animsp
        lda clock
        sta clock
        lda #4
        sta clock
        lda #0
        jsr animsp
        lda clock
        sta z80_c
        sta z80_b
        jsr spawn
        jsr ifall
        lda #0
        jsr animsp
        lda varb
        sta clock
        lda #0
        jsr animsp
        sta z80_l
        lda #0
        sta z80_h
        jsr addsc
        lda #0
        jsr animsp
        jsr cangr
        beq :+
        jmp s08114
:
        lda charx
        sta clock
        lda #4
        sta clock
        lda #0
        jsr animsp
        lda clock
        ldy #8
        sta (z80_ix),y
        lda clock
        ; ldy #8
        sta (z80_ix),y
        lda #4
        ; ldy #8
        sta (z80_ix),y
        lda #0
        jsr animsp
        lda clock
        sta z80_c
        sta z80_b
        jsr spawn
        jsr ifall
        lda #0
        jsr animsp
        lda vara
        sta clock
        lda #0
        jsr animsp
        tax
        jsr mmenu
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp s08114
:
        lda #0
        jsr animsp
        jsr cangr
        beq :+
        jmp s08114
:
        lda charx
        sta clock
        lda #4
        sta clock
        lda #0
        jsr animsp
        lda clock
        ldy #8
        sta (z80_ix),y
        lda clock
        ; ldy #8
        sta (z80_ix),y
        lda #4
        ; ldy #8
        sta (z80_ix),y
        lda #0
        jsr animsp
        lda clock
        sta z80_c
        sta z80_b
        jsr spawn
        jsr ifall
        lda #0
        jsr animsp
        lda varb
        sta clock
        lda #0
        jsr animsp
        lda varp
        sta clock
        lda #0
        jsr animsp
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp s08114
:
        lda charx
        sta clock
        lda #4
        sta clock
        lda #0
        jsr animsp
        jsr laddd
        beq :+
        jmp s08114
:
        lda clock
        sta clock
        lda #4
        sta clock
        lda #0
        jsr animsp
        lda clock
        sta z80_c
        sta z80_b
        jsr spawn
        jsr ifall
        lda #0
        jsr animsp
        lda vara
        sta clock
        lda #0
        jsr animsp
        lda #1
        sta prtmod
        jsr dmsg
        lda #0
        sta clock
        jsr cangl
        beq :+
        jmp s08114
:
        asl a
        sta sndtyp
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp s08114
:
        lda charx
        sta clock
        lda #4
        sta clock
        lda #0
        jsr animsp
        jsr laddd
        beq :+
        jmp s01775
:
        lda clock
        sta clock
        lda #4
        sta clock
        lda #0
        jsr animsp
        lda clock
        sta z80_c
        sta z80_b
        jsr spawn
        jsr ifall
        lda #0
        jsr animsp
        lda varb
        sta clock
        lda #0
        jsr animsp
        lda #1
        sta prtmod
        jsr dmsg
        lda #0
        sta clock
        jsr cangl
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        lda clock
        sta clock
        lda #4
        sta clock
        lda #0
        jsr animsp
        lda clock
        sta z80_c
        sta z80_b
        jsr spawn
        jsr ifall
        lda #0
        jsr animsp
        lda vara
        sta clock
        lda #0
        jsr animsp
        jsr cangl
        beq :+
        jmp       
:
        lda #50
        jsr animsp
        lda #4
        sta charx
        lda #0
        jsr animsp
        lda clock
        ldy #7
        sta (z80_ix),y
        lda clock
        ; ldy #7
        sta (z80_ix),y
        lda #4
        ; ldy #7
        sta (z80_ix),y
        lda #0
        jsr animsp
        lda clock
        sta z80_c
        sta z80_b
        jsr spawn
        jsr ifall
        lda #0
        jsr animsp
        lda varb
        sta clock
        jsr cangu
        beq :+
        jmp       
:
        jsr cangu
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        lda #0
        jsr animsp
        jsr dscor
        jsr dmsg
        lda #0
        jsr animsp
        jsr laddu
        beq :+
        jmp       
:
        lda numlif
        sta varf
        lda #4
        sta varf
        jsr cangu
        beq :+
        jmp       
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #0
        jsr animsp
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #5
        jsr dmsg
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        jsr gravst
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        jsr cangu
        beq :+
        jmp       
:
s01775: lda #0
        jsr animsp
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #5
        jsr dmsg
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp s02413
:
        jsr gravst
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        jsr cangu
        beq :+
        jmp       
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #0
        jsr animsp
        jsr cangl
        beq :+
        jmp       
:
        lda #5
        jsr dmsg
        jsr cangr
        beq :+
        jmp       
:
        jsr gravst
        jsr cangr
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr cangl
        beq :+
        jmp       
:
        lda #5
        jsr dmsg
        jsr cangr
        beq :+
        jmp       
:
        jsr gravst
        jsr cangr
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr cangl
        beq :+
        jmp       
:
        lda #5
        jsr dmsg
        jsr cangr
        beq :+
        jmp       
:
        jsr gravst
        jsr cangr
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr cangl
        beq :+
        jmp       
:
        lda #5
        jsr dmsg
        jsr cangr
        beq :+
        jmp       
:
        jsr gravst
        jsr cangr
        beq :+
        jmp       
:
        lda #4
        sta varj
        lda varg
        sta varf
        lda #4
        sta varf
        lda #0
        jsr animsp
        sec
        sbc z80_c
        lda #5
        sta varh
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        jsr gravst
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda varl
        sta clock
        lda #4
        sta clock
        lda #0
        jsr animsp
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #5
        jsr dmsg
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        jsr gravst
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        jmp s02444
s02413: jsr cangu
        beq :+
        jmp       
:
        jsr cangu
        beq :+
        jmp       
:
        jmp s03038
s02444: lda #0
        jsr animsp
        lda #5
        jsr dmsg
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        jsr gravst
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jsr dmsg
        lda #4
        sta vark
        lda vark
        sta clock
        lda #4
        sta clock
        lda #0
        jsr animsp
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda vark
        sta varf
        lda #4
        sta varf
        jsr getob
        lda vark
        cmp clock
        beq *+5
        jmp xxxxxx
        lda #0
        jsr animsp
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        lda numlif
        sta varf
        lda #4
        sta varf
        jsr cangu
        beq :+
        jmp       
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #0
        jsr animsp
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jsr gravst
        jsr cangu
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr cangu
        beq :+
        jmp       
:
        lda #5
        jsr dmsg
        jsr laddd
        beq :+
        jmp       
:
        jsr gravst
        jsr laddd
        beq :+
        jmp       
:
        jsr cangu
        beq :+
        jmp       
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #0
        jsr animsp
        jsr cangu
        beq :+
        jmp       
:
        lda #5
        jsr dmsg
        jsr laddd
        beq :+
        jmp       
:
        jsr gravst
        jsr laddd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jsr gravst
        jsr cangu
        beq :+
        jmp       
:
        jsr cangu
        beq :+
        jmp       
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #0
        jsr animsp
s03038: lda #5
        jsr dmsg
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp s03140
:
        jsr gravst
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        jsr cangu
        beq :+
        jmp       
:
        jsr cangl
        beq :+
        jmp       
:
        lda #0
        jsr animsp
s03140: lda #5
        jsr dmsg
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp s06601
:
        jsr gravst
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        lda #0
        jsr animbk
        jsr laddd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        lda #0
        jsr animbk
        jsr gravst
        jsr laddd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr laddd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr gravst
        jsr laddd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        lda chary
        ldy #9
        sta (z80_ix),y
        jsr gravst
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr gravst
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr laddd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #5
        jsr dmsg
        jsr laddd
        beq :+
        jmp       
:
        jsr gravst
        jsr laddd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr laddd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr gravst
        jsr laddd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr laddd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #5
        jsr dmsg
        jsr laddd
        beq :+
        jmp       
:
        jsr gravst
        jsr laddd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr laddd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr gravst
        jsr laddd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        lda #1
        sta gamwon
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        jsr scrr
        lda #0
        jsr animsp
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #5
        jsr dmsg
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        jsr gravst
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        jsr scrr
        lda #0
        jsr animsp
        lda #1
        sta gamwon
        jsr gravst
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #5
        jsr dmsg
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        jsr gravst
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        lda #1
        sta gamwon
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        lda #1
        sta gamwon
        jsr gravst
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jsr ifall
        lda #5
        sta clock
        jsr laddd
        beq :+
        jmp       
:
        lda varc
        sta clock
        lda #4
        sta clock
        lda #0
        jsr animsp
        lda varc
        sta varf
        lda #4
        sta varf
        lda #0
        jsr animsp
        jsr dmsg
        lda #4
        sta varc
        lda varc
        sta clock
        lda #4
        sta clock
        ; lda #4
        sta varc
        jsr dmsg
        lda #4
        sta vard
        lda vard
        sta clock
        lda #4
        sta clock
        lda #0
        jsr animsp
        lda vard
        sta varf
        lda #4
        sta varf
        lda #0
        jsr animsp
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        lda #4
        sta vard
        lda vard
        sta clock
        lda #4
        sta clock
        lda varobj
        sta varp
        jsr cangr
        beq :+
        jmp       
:
        jsr cangr
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr gravst
        jsr laddd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        lda varf
        ldy #9
        sta (z80_ix),y
        lda varc
        ; ldy #9
        sta (z80_ix),y
        lda #4
        ; ldy #9
        sta (z80_ix),y
        lda #0
        jsr animsp
        lda clock
        ldy #9
        sta (z80_ix),y
        lda varc
        ; ldy #9
        sta (z80_ix),y
        lda #4
        ; ldy #9
        sta (z80_ix),y
        jsr laddd
        beq :+
        jmp       
:
        jsr dmsg
        lda #4
        sta varc
        lda varc
        sta clock
        lda #4
        sta clock
        ; lda #4
        sta varc
        jsr dmsg
        lda #4
        sta vard
        lda vard
        sta clock
        lda #4
        sta clock
        lda #0
        jsr animsp
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda varf
        jsr pattr
        lda #4
        sta vard
        lda #0
        jsr animsp
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        lda vard
        ldy #9
        sta (z80_ix),y
        lda #4
        ; ldy #9
        sta (z80_ix),y
        lda vard
        sta clock
        lda #4
        sta clock
        jsr cangr
        beq :+
        jmp       
:
        jsr cangr
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr gravst
        jsr laddd
        beq :+
        jmp       
:
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        lda vare
        sta clock
        lda #4
        sta clock
        lda #0
        jsr animsp
        lda #1
        sta gamwon
        lda vare
        sta varf
        lda #4
        sta varf
        lda #0
        jsr animsp
        lda #1
        sta gamwon
        jsr dmsg
        lda #4
        sta vare
        lda vare
        sta clock
        lda #4
        sta clock
        ; lda #4
        sta vare
        jsr dmsg
        lda #4
        sta varf
        lda varf
        sta clock
        lda #4
        sta clock
        lda #0
        jsr animsp
        lda varf
        sta varf
        lda #4
        sta varf
        jsr getob
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        lda #4
        sta varf
        lda varf
        sta clock
        lda #4
        sta clock
        jsr ifall
        jsr cangl
        beq :+
        jmp       
:
        jsr cangl
        beq :+
        jmp       
:
        jsr getob
        lda #1
        sta gamwon
        jsr gravst
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda vare
        sta varf
        lda #4
        sta varf
        lda #0
        jsr animsp
        lda #1
        sta gamwon
        lda vare
        sta clock
        lda #4
        sta clock
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        jsr dmsg
        lda #4
        sta vare
        lda vare
        sta clock
        lda #4
        sta clock
        ; lda #4
        sta vare
        jsr dmsg
        lda #4
        sta varf
        lda varf
        sta clock
        lda #4
        sta clock
        lda #0
        jsr animsp
        lda varf
        sta chary
        lda varf
        sta chary
        lda #4
        sta chary
        lda #0
        jsr animsp
        jsr dmsg
        lda varf
        sta chary
        lda #4
        sta chary
        lda varf
        sta clock
        lda #4
        sta clock
        jsr cangl
        beq :+
        jmp       
:
        jsr cangl
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        lda #1
        sta gamwon
        jsr gravst
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        jsr ifall
        lda varopt
        sta varf
        lda #4
        sta varf
        lda #0
        jsr animsp
        jsr cangd
        beq :+
        jmp       
:
        jsr cangl
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr cangl
        beq :+
        jmp       
:
        jsr gravst
        jsr cangl
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr cangl
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr gravst
        jsr cangl
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr cangl
        beq :+
        jmp       
:
        jsr cangr
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr cangl
        beq :+
        jmp       
:
        jsr gravst
        jsr cangr
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr cangr
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr gravst
        jsr cangr
        beq :+
        jmp       
:
        jsr cangl
        beq :+
        jmp       
:
        lda varg
        sta clock
        lda #4
        sta clock
        jsr cangr
        beq :+
        jmp       
:
        lda varh
        sta clock
        lda #4
        sta clock
        lda #0
        jsr animsp
        lda varn
        sta varf
        lda #4
        sta varf
        lda #0
        jsr animsp
        jsr cangl
        beq :+
        jmp       
:
        jsr gravst
        jsr cangu
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr gravst
        jsr cangd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        lda #1
        sta restfl
        jsr dscor
        jsr dmsg
        jsr cangu
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda clock
        cmp varq
        beq *+5
        jmp xxxxxx
        lda #4
        sta varo
        lda #0
        jsr animsp
        jsr cangr
        beq :+
        jmp       
:
        jsr gravst
        jsr cangu
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr gravst
        jsr cangd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        lda varo
        cmp clock
        beq *+5
        jmp xxxxxx
        jsr cangu
        beq :+
        jmp       
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        sta z80_b
        jsr delay
        lda #1
        sta restfl
        jsr dscor
        jsr dmsg
        lda #0
        jsr animsp
        lda #0
        jsr animsp
        lda varn
        cmp clock
        beq *+5
        jmp xxxxxx
        lda #0
        jsr animsp
        lda #4
        sta varn
        lda #0
        jsr animsp
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        jsr dmsg
        jsr cangr
        beq :+
        jmp       
:
        lda varm
        sta varf
        lda #4
        sta varf
        jsr redraw
        lda #0
        jsr animsp
        jsr cangu
        beq :+
        jmp       
:
        jsr gravst
        jsr cangu
        beq :+
        jmp       
:
        lda #1
        sta prtmod
        lda #0
        sta prtmod
        ; lda #0
        jsr animsp
        jsr cangd
        beq :+
        jmp       
:
        jsr gravst
        jsr cangu
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #5
        jsr dmsg
        jsr laddd
        beq :+
        jmp       
:
        jsr gravst
        jsr laddd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jsr laddd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
s06601: jsr laddd
        beq :+
        jmp s08114
:
        lda #0
        jsr animsp
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jsr gravst
        jsr cangd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr laddd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jsr gravst
        jsr cangd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr laddd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr gravst
        jsr cangd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr laddd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr gravst
        jsr laddd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr dscor
        jsr dscor
        jsr dmsg
        lda #4
        sta varj
        jsr cangl
        beq :+
        jmp       
:
        lda #4
        sta varm
        jsr cangr
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr gravst
        jsr laddd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jsr gravst
        jsr laddd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #5
        jsr dmsg
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        jsr gravst
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        jsr cangu
        beq :+
        jmp       
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda varm
        sta vara
        lda #4
        sta vara
        jsr cangr
        beq :+
        jmp       
:
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr gravst
        jsr laddd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jsr gravst
        jsr laddd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #5
        jsr dmsg
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        jsr gravst
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #5
        jsr dmsg
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        jsr gravst
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr laddu
        beq :+
        jmp       
:
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        jsr gotob
        cmp #255
        beq :+
        jmp       
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jsr gravst
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        lda #4
        sta varj
        jsr cangl
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr cangl
        beq :+
        jmp       
:
        lda #5
        jsr dmsg
        jsr cangl
        beq :+
        jmp       
:
        jsr gravst
        jsr cangl
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr laddd
        beq :+
        jmp       
:
        sta loopa
s07721: jsr cangl
        beq :+
        jmp       
:
        lda #5
        jsr dmsg
        jsr cangr
        beq :+
        jmp       
:
        jsr gravst
        jsr cangr
        beq :+
        jmp       
:
        lda #1
        sta prtmod
        jsr dmsg
        lda #0
        jsr animsp
        jsr cangl
        beq :+
        jmp       
:
        lda #5
        jsr dmsg
        jsr cangr
        beq :+
        jmp       
:
        jsr gravst
        jsr cangr
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr laddd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr cangl
        beq :+
        jmp       
:
        lda #5
        jsr dmsg
        jsr cangr
        beq :+
        jmp       
:
        jsr gravst
        jsr cangr
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr cangl
        beq :+
        jmp       
:
        lda #5
        jsr dmsg
        jsr cangr
        beq :+
        jmp       
:
        jsr gravst
        jsr cangr
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr cangl
        beq :+
        jmp       
:
        lda #5
        jsr dmsg
        jsr cangl
        beq :+
        jmp       
:
        jsr gravst
        jsr cangl
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
s08114: jmp grav
.out .sprintf("EVENT 18 SIZE = %d", (* - evnt18))

evnt19:
        jsr cangu
        beq :+
        jmp       
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #0
        jsr animsp
        lda clock
        sta varm
        lda #5
        sta varm
        jsr cangd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda clock
        sta varg
        lda #0
        jsr animsp
        jsr laddu
        beq :+
        jmp       
:
        jsr gravst
        jsr cangu
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        jsr laddd
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #5
        jsr dmsg
        jsr cangr
        beq :+
        jmp       
:
        jsr gravst
        jsr cangr
        beq :+
        jmp       
:
        lda #0
        jsr animsp
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jsr gravst
        jsr laddd
        beq :+
        jmp t00296
:
t00296: rts
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
        .byte "HAPPILY!",141
        .byte 13
        .byte "s",141
        .byte "1..KEYBOARD",141
        .byte "2..KEMPSTON",141
        .byte "3..SINCLAIR",141
        .byte ") # #",141
        .byte "% ) #",141
        .byte "% % )",141
        .byte "$ $ _",141
        .byte "$ _ @",141
        .byte "_ @ @",141
        .byte "TOTAL SCORE:",141
        .byte "&",141
        .byte 39
        .byte 39
        .byte 39
        .byte 39
        .byte 39
        .byte 39
        .byte 39
        .byte 39
        .byte 39
        .byte 141
        .byte 39
        .byte 39
        .byte 39
        .byte 39
        .byte 39
        .byte 39
        .byte 39
        .byte 39
        .byte 39
        .byte 141
        .byte 39
        .byte 141
        .byte 39
        .byte 141
        .byte "EXIT....O",141
        .byte "+",141
        .byte "SCORE:",141
        .byte "(bfghi",141
        .byte "WHERE ARE YOU, MY CHILDREN?",13
        .byte 13
        .byte "  GRANDMA IS WAITING= COME SOON,",13
        .byte 13
        .byte "  THE JELLYFISH WILL HELP YOU!",141
        .byte "1.......LOW",141
        .byte "2....NORMAL",141
        .byte "3......HIGH",141
        .byte "GAME SPEED:",141
        .byte "CONTROLS:",141
        .byte ";",141
        .byte "-",141
        .byte "FIRE....LOW",141
        .byte "0.......LOW",141
        .byte "HELLO GRANDMA===",141
        .byte "I AM THE HAPPIEST GRANDMOTHER===",13
        .byte 141
        .byte " ALL OCTOPUSES HAVE SAFELY",13
        .byte 13
        .byte "   REACHED THE GRANDMOTHER!",141
        .byte "CONGRATULATIONS!",141
        .byte "I LOVE THESE KIDS===",141
        .byte "<",141
        .byte "      ",39
        .byte 39
        .byte 39
        .byte 39
        .byte 39
        .byte "        ",39
        .byte 39
        .byte 39
        .byte 39
        .byte 39
        .byte 141
        .byte "actopuses",141
nummsg:
        .byte 40
.out .sprintf("MESSAGE SIZE = %d", (* - msgdat))

chgfx:
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 0,0,0,0,128,128,160,80,192,192,176,112,0,0,0,0
        .byte 0,0,0,0,144,48,0,176,144,16,16,160,0,0,0,0
        .byte 0,0,82,82,33,5,64,80,37,37,0,0,0,0,0,0
        .byte 0,0,2,74,36,32,2,66,32,36,0,0,0,0,0,0
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,128,0,240,128,240,224,240,224,240,240,240,240,240,240
        .byte 224,208,240,160,176,208,224,160,240,0,160,128,208,0,0,0
        .byte 224,240,112,240,48,240,176,240,240,240,176,240,192,240,240,0
        .byte 0,0,0,112,48,240,240,240,240,240,240,112,208,176,224,176
        .byte 0,0,0,0,0,0,16,192,112,240,112,144,240,192,208,112
        .byte 192,240,240,240,240,240,112,80,112,48,176,240,240,240,112,240
        .byte 240,128,112,192,80,192,112,192,32,224,48,224,240,224,240,224
        .byte 48,240,240,240,240,240,80,240,240,240,240,240,240,128,192,0
        .byte 16,240,112,240,240,240,240,240,224,16,0,112,0,240,16,240
        .byte 240,32,240,208,240,224,240,224,240,240,240,240,192,112,0,0
        .byte 0,16,0,48,0,240,0,240,48,240,240,176,208,224,240,0
        .byte 16,176,32,224,48,128,96,0,96,0,64,0,192,0,128,0
        .byte 0,0,0,0,0,16,0,240,112,240,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,16,112,224,0,240
        .byte 0,0,0,112,16,192,0,0,0,0,0,0,0,0,0,0
        .byte 112,240,48,240,48,208,48,224,48,224,16,192,16,192,16,64
        .byte 240,0,240,240,240,240,240,208,240,112,240,128,112,192,112,224
        .byte 240,192,240,240,176,176,192,240,128,16,0,0,0,0,0,0
        .byte 112,240,48,240,16,208,0,112,0,0,0,0,0,0,0,0
        .byte 240,128,240,224,208,240,240,208,192,48,0,16,0,0,0,0
        .byte 16,192,0,192,0,128,0,192,0,128,0,128,16,0,16,0
        .byte 0,0,0,1,16,90,37,165,82,90,165,165,90,90,165,165
        .byte 26,82,5,165,26,88,5,37,0,90,1,5,0,26,0,0
        .byte 90,90,165,161,90,88,165,133,90,90,165,133,90,18,0,165
        .byte 0,0,164,0,90,72,165,165,90,90,165,165,90,90,165,165
        .byte 0,0,0,0,0,0,33,128,90,74,129,164,82,82,164,161
        .byte 74,82,165,37,88,90,160,164,24,74,133,133,90,90,165,164
        .byte 16,90,33,164,18,10,33,164,82,64,37,132,82,90,37,165
        .byte 90,72,165,165,90,90,165,160,90,90,165,165,16,90,0,33
        .byte 90,8,165,164,90,90,165,165,8,82,164,0,90,0,165,128
        .byte 64,90,161,165,82,90,37,165,90,90,165,165,74,18,0,0
        .byte 8,0,132,0,90,0,165,0,90,72,133,165,82,26,0,165
        .byte 88,8,37,4,16,72,0,36,0,66,0,32,0,18,0,1
        .byte 0,0,0,0,8,0,165,0,90,74,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,128,0,82,74,165,0
        .byte 0,0,164,0,18,8,0,0,0,0,0,0,0,0,0,0
        .byte 90,74,165,132,26,72,37,132,82,72,33,128,18,8,32,128
        .byte 0,90,165,165,90,90,165,161,74,90,1,165,18,74,37,164
        .byte 18,90,165,165,88,88,165,33,8,16,0,0,0,0,0,0
        .byte 90,74,165,132,26,8,164,0,0,0,0,0,0,0,0,0
        .byte 16,90,37,165,90,26,161,165,72,18,128,0,0,0,0,0
        .byte 18,8,33,0,16,0,33,0,16,0,1,0,0,8,0,128
        .byte 117,228,250,250,245,245,250,114,245,117,234,114,228,117,234,114
        .byte 238,119,238,119,238,119,238,119,238,119,238,119,238,119,68,34
        .byte 160,85,234,114,228,117,234,250,228,245,250,250,245,245,114,234
        .byte 117,228,250,250,245,245,250,114,245,117,234,114,228,117,234,114
        .byte 102,119,102,119,102,0,102,0,102,0,102,119,102,119,68,34
        .byte 102,119,102,119,102,119,102,255,102,255,119,238,119,0,34,0
        .byte 245,228,250,250,245,245,250,250,245,245,250,250,177,213,186,216
        .byte 51,204,51,204,51,204,51,204,51,204,51,204,51,204,17,68
        .byte 32,128,50,200,49,196,50,200,49,196,50,200,49,196,114,234
        .byte 85,0,250,0,245,0,250,136,245,128,250,136,245,128,250,136
        .byte 245,117,250,114,245,117,114,114,117,117,98,114,228,100,234,98
        .byte 238,102,238,102,238,102,238,102,238,102,238,102,238,102,68,34
        .byte 160,85,234,114,228,117,234,250,228,245,250,250,245,245,114,250
        .byte 117,228,250,250,245,117,250,114,228,117,234,114,228,0,234,234
        .byte 119,238,0,119,0,119,119,119,255,119,255,119,238,119,170,85
        .byte 68,32,234,114,228,117,234,250,228,245,250,250,245,245,114,234
        .byte 90,18,133,165,72,74,160,132,88,26,165,165,90,90,165,164
        .byte 0,0,0,0,0,0,33,128,90,74,129,36,82,18,164,161
        .byte 0,0,0,0,128,0,240,0,240,224,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 15,15,15,15,15,15,15,7,15,7,12,5,4,1,0,0
        .byte 7,15,0,15,1,15,0,7,1,15,3,15,1,15,0,7
        .byte 15,14,15,0,15,12,15,8,14,0,15,8,15,0,15,12
        .byte 0,0,0,0,5,1,7,5,15,7,15,7,15,15,15,15
        .byte 7,15,7,15,1,15,3,7,0,7,0,15,0,3,0,0
        .byte 15,14,15,14,15,8,12,12,14,0,15,0,12,0,0,0
        .byte 0,0,12,0,15,0,14,4,15,12,15,8,15,14,15,14
        .byte 0,0,0,7,3,15,0,15,6,7,3,15,7,15,7,15
        .byte 15,7,11,5,2,1,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,4,5,5,7,7
        .byte 0,0,12,0,14,0,8,0,12,0,0,0,14,0,12,0
        .byte 0,0,0,7,0,1,0,3,0,0,0,3,0,7,0,3
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 15,135,30,75,15,135,30,75,15,165,15,15,45,135,30,75
        .byte 15,45,30,15,15,135,30,15,15,135,90,15,15,135,30,15
        .byte 45,15,30,15,15,165,30,75,45,135,15,30,45,165,30,75
        .byte 15,15,30,15,15,15,30,15,15,15,30,75,15,15,15,75
        .byte 0,0,0,0,0,18,1,165,82,82,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,4,5,5,7,7
        .byte 0,0,12,0,15,0,14,4,15,12,15,8,15,14,15,14
        .byte 19,76,4,38,8,95,136,5,8,17,170,1,85,2,35,140
        .byte 0,0,0,0,0,0,39,0,25,8,0,136,10,8,5,0
        .byte 8,0,136,0,10,0,5,0,85,68,34,170,17,93,0,39
        .byte 0,17,0,1,0,17,0,34,0,2,136,4,17,8,174,0
        .byte 0,87,1,136,2,17,4,0,68,0,136,0,8,0,136,0
        .byte 78,0,175,136,95,76,35,34,17,2,0,1,0,17,0,1
        .byte 0,0,35,136,85,76,136,170,8,2,170,34,85,68,35,136
        .byte 15,15,15,75,75,195,105,195,60,180,30,105,120,120,45,165
        .byte 135,135,75,15,45,135,90,15,45,165,90,15,15,135,15,15
        .byte 64,2,160,5,74,82,4,32,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,64,2,164,37,10,80,4,32,0,0,0,0
        .byte 15,135,15,15,15,135,15,75,15,15,15,15,15,15,15,15
        .byte 0,0,0,0,8,0,0,0,0,0,4,0,9,0,2,2
        .byte 0,0,0,0,165,164,90,74,161,160,18,8,33,128,0,0
        .byte 0,0,0,0,0,0,74,74,164,164,90,74,164,164,74,74
        .byte 0,0,0,0,37,164,90,74,165,0,90,0,165,0,0,0
        .byte 0,0,0,0,0,0,90,72,164,0,90,72,165,0,90,74
        .byte 0,0,48,96,112,240,80,240,32,224,16,192,0,0,0,0
        .byte 15,15,3,15,12,15,15,3,15,12,15,14,15,15,15,15
        .byte 15,15,15,15,15,15,15,15,15,15,3,15,12,15,3,3
        .byte 0,7,12,0,15,0,15,12,15,14,15,15,15,15,15,15
        .byte 14,7,7,9,1,2,8,7,2,1,0,8,0,0,0,0
        .byte 15,14,15,15,3,13,12,11,9,3,3,12,0,9,12,3
        .byte 12,1,10,4,12,0,0,1,8,1,0,1,0,0,0,0
        .byte 15,12,15,12,15,8,15,8,15,0,15,0,14,0,14,0
        .byte 1,7,1,3,2,7,2,7,2,7,5,13,4,15,4,14
        .byte 8,4,8,5,0,4,0,9,0,11,0,9,1,3,1,3
        .byte 11,15,15,15,11,14,11,14,15,14,7,13,7,12,15,12
        .byte 7,8,15,8,7,1,15,1,6,1,14,2,4,2,8,2
        .byte 15,15,15,15,15,14,15,15,15,13,15,15,15,11,15,15
        .byte 15,15,15,15,15,3,15,12,15,15,3,15,8,15,7,8
        .byte 15,11,15,15,15,7,15,15,13,15,15,15,11,15,7,15
        .byte 14,15,15,15,13,15,15,15,7,15,15,15,15,15,15,15
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,14,15,13
        .byte 7,7,7,13,7,15,3,14,3,15,1,13,0,14,0,3
        .byte 9,15,11,15,9,15,13,15,7,15,9,15,12,15,3,15
        .byte 15,15,15,15,15,15,15,15,15,3,15,12,15,13,15,14
        .byte 117,228,250,250,245,245,234,250,228,245,234,250,228,117,234,0
        .byte 238,0,238,238,255,238,255,238,255,238,238,238,238,238,68,0
        .byte 160,0,234,114,228,245,234,250,228,245,250,250,245,245,114,234
        .byte 0,0,0,0,165,132,74,0,165,132,90,0,165,164,0,0
        .byte 32,128,24,0,4,4,16,8,132,4,16,0,4,128,0,0
        .byte 129,0,66,0,128,128,18,0,132,4,16,0,36,4,16,0
        .byte 0,17,0,34,0,27,0,38,17,2,0,140,0,2,0,141
        .byte 32,0,0,0,32,0,0,0,32,0,66,0,0,128,66,0
bprop:
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 5
        .byte 2
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
        .byte 6
        .byte 6
        .byte 6
        .byte 6
        .byte 6
        .byte 6
        .byte 6
        .byte 6
        .byte 6
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
        .byte 6
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
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 6
        .byte 6
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
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
.out .sprintf("BLOCKS SIZE = %d", (* - chgfx))

sprgfx:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,224,3,240,3,240,7,248,7,248,7,104
        .byte 128,100,0,127,3,243,199,255,230,118,100,102,204,204,129,24,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,120,0,252,0,252,1,254,1,254,1,218,1,146,3,252,7,204,15,254,14,247,9,179,17,34,16,68
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,240,1,248,1,248,3,252,3,252,3,180
        .byte 64,50,128,127,129,249,195,255,198,127,68,102,132,196,0,129,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,240,1,248,1,248,3,252,3,252,3,180,3,36,3,252,1,152,7,255,143,207,140,217,9,147,17,2
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,224,3,240,3,240,7,248,7,248,7,104
        .byte 128,100,128,127,0,51,192,255,225,255,101,189,55,30,3,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,240,1,248,1,248,3,252,3,252,3,180,3,36,3,252,1,152,7,254,87,255,221,153,129,56,0,112
        .byte 0,0,7,224,31,248,63,252,125,236,127,254,255,254,252,31,99,198,28,176,13,104,11,232,10,208,22,160,20,160,5,160
        .byte 0,82,0,148,0,148,0,148,0,16,0,16,0,0,0,0,0,0,128,31,224,127,240,255,177,247,249,255,251,255,127,240
        .byte 255,7,152,241,7,12,3,90,2,246,2,172,5,168,5,40,1,40,1,72,2,80,2,80,0,64,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,7,224,31,248,63,252,61,236,127,254,127,254,252,31,115,198,12,48,10,72,26,208,22,176,22,160
        .byte 0,74,0,82,0,148,0,132,0,20,0,16,0,0,0,0,0,0,0,0,0,0,128,31,224,127,240,255,176,247,249,255
        .byte 220,241,131,12,4,110,2,218,6,178,5,164,5,168,5,40,1,104,1,72,2,80,2,64,2,16,0,80,0,0,0,0
        .byte 0,0,7,224,31,248,63,252,55,190,127,254,127,255,248,63,99,198,13,56,22,176,23,208,11,80,5,104,5,40,5,160
        .byte 0,74,0,41,0,41,0,41,0,8,0,8,0,0,0,0,0,0,128,31,224,127,240,255,248,222,249,255,253,255,255,224
        .byte 254,15,152,241,3,14,5,172,6,244,3,84,1,90,1,74,1,72,1,40,0,164,0,164,0,32,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,7,224,31,248,63,252,55,188,127,254,127,254,248,63,99,206,12,48,18,80,11,88,13,104,5,104
        .byte 0,82,0,74,0,41,0,33,0,40,0,8,0,0,0,0,0,0,0,0,0,0,128,31,224,127,240,255,240,222,249,255
        .byte 184,243,19,12,7,98,5,180,4,214,2,90,1,90,1,74,1,104,1,40,0,164,0,36,0,132,0,160,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,128,15,192,15,192,31,224,31,224,22,224
        .byte 1,38,0,254,192,207,227,255,103,110,38,102,51,51,129,24,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,1,224,3,240,3,240,7,248,7,248,5,184,4,152,3,252,3,62,7,255,14,247,12,217,132,72,130,32
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,15,0,31,128,31,128,63,192,63,192,45,192
        .byte 2,76,1,254,129,159,195,255,99,254,34,102,33,35,0,129,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,240,1,248,1,248,3,252,3,252,2,220,2,76,3,252,1,152,15,254,31,63,25,179,12,153,132,8
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,128,15,192,15,192,31,224,31,224,22,224
        .byte 1,38,1,254,0,204,3,255,135,255,166,189,236,120,192,96,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,240,1,248,1,248,3,252,3,252,2,220,2,76,3,252,1,152,7,254,175,254,185,155,17,200,0,224
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,96,1,144,7,104,15,242,6,246,22,250,31,250,15,244,14,240,7,224
        .byte 0,24,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,64,6
        .byte 5,218,131,252,129,189,133,190,143,254,3,253,3,156,1,250,2,112,0,16,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,1,0,1,100,35,144,31,104,15,242,2,118,22,250,123,122,31,244,14,56,15,232
        .byte 64,156,1,4,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,144,5,64,142
        .byte 5,218,131,252,128,157,133,190,143,254,3,253,3,156,3,250,2,112,0,16,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,6,0,9,128,22,224,79,240,111,96,95,104,95,248,47,240,15,112,7,224
        .byte 0,24,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,24,0,38
        .byte 5,186,19,252,27,216,23,218,23,255,11,252,3,156,5,248,0,228,0,128,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,128,38,128,9,196,22,248,79,240,110,64,95,104,94,222,47,248,28,112,23,240
        .byte 2,57,128,32,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,154,16,39
        .byte 5,186,19,252,27,144,23,218,23,255,11,252,3,156,5,252,0,228,0,128,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,128,0,0,0,128,0,128,65,130,49,204,23,232,15,240,13,176,253,184,55,247,14,124,13,176
        .byte 129,126,195,62,36,24,0,16,0,16,0,0,0,16,0,0,0,0,0,0,0,0,0,2,0,0,0,2,0,2,9,6
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,32,0,64,0,80,0,144,0,176,0,240,0,224,0,198,50,77,107,102,51
        .byte 114,24,97,60,128,36,0,24,0,195,0,165,0,36,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,0
        .byte 64,2,64,3,195,25,213,172,179,24,184,97,56,241,25,154,5,152,0,240,6,102,8,1,8,145,1,8,1,8,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,0,0,0,0,1,224,3,240,3,240,7,248,7,248
        .byte 128,109,128,109,128,127,0,51,209,243,251,126,110,182,1,140,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,44,0,36,0,24,0,0,0,128,0,0,0,0,0,120,0,252,0,252,1,254,1,254,1,182,1,182,1,254,64,204
        .byte 0,0,0,0,0,0,0,0,0,224,5,48,1,16,1,144,0,224,0,0,0,0,4,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,30,0,63,0,63,128,127,128,127,128,109,128,109,0,0,0,0,0,0,0,0,128,3,192,20,64,4,64,6
        .byte 0,0,0,0,1,0,0,0,0,0,0,0,0,16,0,0,0,0,0,0,0,0,0,120,0,252,0,252,1,254,1,254
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 4,50,4,58,8,13,8,1,8,1,10,1,5,64,4,162,3,4,0,240,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 2,4,2,132,1,72,0,240,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
frmlst:
        .byte 0,3
        .byte 3,3
        .byte 6,4
        .byte 10,4
        .byte 14,3
        .byte 17,3
        .byte 20,4
        .byte 24,4
        .byte 28,1
        .byte 29,1
        .byte 30,2
        .byte 32,4
        .byte 36,2
        .byte 38,2
        .byte 40,5
        .byte 45,2
        .byte 47,2
        .byte 49,1
        .byte 50,5
        .byte 55,5,60,0
.out .sprintf("SPRITES SIZE = %d", (* - sprgfx))

scdat:
        .word 164,271,316,278,317,326,362,364,352,332,335,123,57,175
        .byte 255,6,36,68,103,104,105,106,130,68,255,6,9,68,255,6,13,76,49,79,49,68,55,68,49,68,52,68,59,68,62,68,127
        .byte 80,62,75,255,6,10,68,70,50,68,53,68,56,68,50,68,54,68,60,68,63,68,128,68,63,71,68,255,6,10,73,51,75
        .byte 51,72,57,78,51,76,58,75,61,78,64,72,129,76,64,74,255,6,12,70,71,77,73,69,74,77,73,69,74,77,73,69,74
        .byte 77,70,71,255,6,13,68,77,255,6,14,77,255,6,0,255,6,32,28,31,66,34,37,38,41,255,6,23,29,30,65,35,36
        .byte 40,42,255,6,23,47,44,43,39,39,255,6,24,87,45,46,48,255,6,27,48,6,1,2,255,6,28,3,4,255,6,14
        .byte 28,31,32,34,37,38,41,80,0,0,72,75,102,78,68,68,68,78,76,79,255,68,4,76,72,255,68,4,29,30,33,35,36
        .byte 40,42,70,255,0,4,72,0,75,78,76,0,0,75,72,75,78,72,0,0,75,76,75,68,45,44,43,42,39,76,255,0,22
        .byte 71,102,47,46,48,76,72,255,0,23,5,75,40,76,255,0,27,5,70,255,0,28,5,68,70,255,0,27,5,68,68,73,74
        .byte 77,70,255,0,20,71,77,77,73,255,68,5,70,255,0,21,79,255,68,6,76,255,0,22,71,102,68,68,76,72,72,255,0,24
        .byte 75,68,68,70,255,0,26,101,79,68,76,86,255,0,25,75,68,68,73,85,255,0,25,69,75,68,68,76,101,255,0,23,71
        .byte 68,77,76,72,0,84,255,0,23,75,102,68,70,0,84,98,255,0,24,75,68,68,73,83,85,84,255,0,23,74,68,68,134
        .byte 68,70,85,255,0,20,101,0,74,68,68,134,131,68,68,80,0,74,69,74,70,255,0,9,101,71,68,73,69,74,77,77,68,68,68
        .byte 132,132,134,68,68,77,68,68,68,70,255,0,9,85,74,255,68,9,133,131,131,255,68,5,78,76,255,0,9,75,102,255,68,6
        .byte 134,68,68
        .byte 255,68,4,76,72,0,75,78,102,255,68,6,134,68,68,68,78,76,72,0,0,72,75,78,68,73,68,68,72,78,255,0,5
        .byte 75,255,68,5,134,131,68,68,80,255,0,8,72,72,68,68,73,255,0,7,79,255,68,4,132,68,68,78,76,255,0,8,5
        .byte 0,78,76,255,0,8,75,78,76,79,68,68,68,80,255,0,11,5,255,0,13,75,68,68,68,80,255,0,11,5,255,0,14
        .byte 79,68,68,68,70,84,255,0,9,5,255,0,15,75,68,68,73,83,255,0,9,5,74,77,73,255,0,12,71,102,68,68,80
        .byte 255,0,6,74,77,77,77,68,68,68,77,70,255,0,11,79,68,68,68,73,255,0,5,79,255,68,7,77,73,74,77,77,70
        .byte 255,0,6,75,68,68,68,80,255,0,5,71,78,255,68,10,76,255,0,7,74,68,68,68,80,255,0,7,75,255,68,6,72
        .byte 78,76,255,0,8,75,78,68,68,76,255,0,7,79,102,134,68,68,68,72,255,0,13,72,72,255,0,8,75,68,131,68,68
        .byte 76,255,0,25,75,132,134,80,255,0,26,74,68,131,80,255,0,26,72,68,131,76,255,0,26,97,68,68,73,255,0,24,86
        .byte 0,79,68,68,80,101,255,0,23,85,74,255,68,4,76,255,0,23,75,6,68,68,68,80,255,0,19,71,68,73,74,73,79
        .byte 255,68,4,76,255,0,8,74,68,68,70,255,0,8,79,102,255,68,5
        .byte 0,75,102,255,68,5,78,68,76,75,255,68,7,72,76,79,68,78,72,76,79,68,68,68,0,0,72,72,78,72,75,76,0
        .byte 72,0,74,68,68,134,68,68,76,72,0,0,72,72,0,0,0,75,78,78,68,255,0,11,75,102,68,132,134,80,255,0,11
        .byte 5,72,255,0,12,79,78,68,68,68,70,255,0,11,5,255,0,13,71,102,68,80,255,0,12,5,255,0,14,79,80,255,0,13
        .byte 5,255,0,14,75,76,255,0,13,5,77,73,255,0,20,69,74,73,97,74,77,77,73,68,80,255,0,19,101,79,68,68,77
        .byte 68,68,68,80,68,72,255,0,19,79,255,68,9,70,255,0,19,79,68,80,72,76,75,78,68,68,68,73,255,0,18,71,102
        .byte 68,76,255,0,4,71,102,68,80,255,0,19,75,78,76,255,0,5,79,68,80,255,0,27,75,80,255,0,11,86,97,74,80
        .byte 255,0,14,76,255,0,11,85,79,68,80,255,0,12,101,74,101,255,0,11,75,102,68,80,255,0,12,79,68,80,255,0,12
        .byte 79,68,68,73,255,0,11,75,68,68,73,255,0,10,71,78,72,75,80,255,0,12,79,68,80,255,0,26,74,68,68,80,255,0,25
        .byte 74,68,68,76,72,255,0,25,79,102,78
        .byte 255,68,7,134,68,68,76,72,76,71,78,68,72,79,255,68,6,78,72,78,255,68,4,78,76,72,78,102,68,131,134,68,70
        .byte 255,0,4,72,71,102,255,68,5,78,0,0,0,75,78,72,78,255,0,4,79,68,68,68,80,255,0,7,72,79,68,68,68
        .byte 80,255,0,5,5,255,0,6,75,78,72,78,76,255,0,9,79,68,68,80,255,0,6,5,255,0,20,79,68,80,255,0,6
        .byte 5,255,0,20,71,68,68,73,255,0,5,5,255,0,20,71,102,68,80,255,0,5,5,0,77,69,255,0,18,79,68,76,255,0,4
        .byte 71,77,74,68,68,77,74,77,77,73,97,255,0,12,75,102,73,255,0,4,79,255,68,10,77,73,255,0,11,79,76,255,0,4
        .byte 72,79,255,68,10,80,255,0,18,75,255,68,6,78,68,68,68,80,255,0,18,71,255,68,4,78,76,0,75,78,80,255,0,20
        .byte 78,76,75,80,255,0,25,71,255,0,16,74,77,73,255,0,10,71,73,86,255,0,14,79,68,78,255,0,10,79,80,85,255,0,13
        .byte 74,68,80,255,0,11,75,68,76,255,0,13,79,68,80,255,0,11,97,68,80,255,0,12,74,68,68,68,70,84,255,0,9
        .byte 75,68,76,255,0,11,74,255,68,4,73,83,255,0,9,74,68,70,255,0,11,79,255,68,5,80,255,0,9,79,76,255,0,12
        .byte 71,78,102,68,68,68,70,255,0,7,75,102,68
        .byte 80,0,79,100,80,0,0,0,75,78,76,255,0,5,79,100,80,0,0,0,74,77,73,0,75,72,78,68,76,0,75,78,76
        .byte 255,0,11,75,78,76,0,0,0,79,100,80,0,0,0,5,75,255,0,13,123,255,0,8,75,78,76,255,0,4,5,255,0,13
        .byte 74,77,73,255,0,13,5,255,0,13,79,100,80,255,0,13,5,255,0,13,75,78,76,255,0,13,5,255,0,6,74,77,73
        .byte 255,0,20,5,77,73,255,0,4,79,100,80,255,0,10,74,77,73,255,0,5,77,73,77,68,80,255,0,4,75,78,76,255,0,10
        .byte 79,100,80,255,0,5,75,102,68,68,70,255,0,17,75,78,76,255,0,5,74,68,68,68,76,255,0,25,79,78,68,76,255,0,8
        .byte 74,77,73,255,0,15,75,0,79,0,101,255,0,7,79,99,80,255,0,6,74,77,73,255,0,8,75,73,85,255,0,7,75
        .byte 78,76,255,0,6,79,99,80,255,0,8,74,68,76,255,0,16,75,78,76,255,0,8,72,80,255,0,25,84,0,0,97,76
        .byte 255,0,25,98,84,74,77,73,0,84,255,0,23,75,77,68,99,80,101,98,0,0,74,77,73,255,0,19,79,102,68,68,77
        .byte 76,0,0,79,99,80,255,0,13,74,77,73,0,0,0,71,78,72,76,72,0,0,0,75,78,76,255,0,4,74,77,73,255,0,6
        .byte 79,99,80,255,0,6,70,255,0,11,79,99,80,255,0,6,75,78,76,255,0,6
        .byte 255,68,16,76,72,75,72,75,78,68,68,80,0,79,255,68,4,78,76,72,75,78,255,68,6,72,78,78,80,255,0,6,72,72
        .byte 76,0,0,75,78,68,72,255,0,5,75,102,78,68,68,80,255,0,16,5,72,255,0,7,72,0,79,68,76,255,0,17,5
        .byte 255,0,9,75,68,80,255,0,17,5,255,0,8,84,0,79,80,255,0,17,5,255,0,8,98,74,68,68,70,255,0,7,74
        .byte 73,74,77,70,255,0,4,5,69,69,255,0,6,79,134,80,72,255,0,8,79,255,68,4,77,73,113,74,77,68,68,73,255,0,5
        .byte 75,131,80,255,0,9,71,78,102,255,68,9,80,255,0,5,74,68,76,255,0,11,79,78,255,68,6,134,68,80,255,0,5
        .byte 79,68,80,255,0,13,72,79,68,68,134,68,131,68,70,255,0,5,75,102,68,77,73,255,0,10,86,71,68,68,68,131,255,68,4
        .byte 70,255,0,5,75,255,68,4,77,73,255,0,7,85,79,255,68,8,76,255,0,6,79,102,68,68,78,80,255,0,7,79,68
        .byte 134,255,68,4,79,102,68,73,255,0,7,75,78,76,255,0,8,74,68,68,131,134,68,68,68,0,79,68,80,255,0,17,71
        .byte 68,68,68,132,131,68,78,72,70,79,68,70,255,0,17,74,68,68,80,75,78,80,0,0,68,68,76,72,255,0,17,72,78
        .byte 80,255,0,6,68,68,73,101,255,0,23,101,0,0,75,78,68,80,255,0,23,83,97,101,0,71,102,68,77,255,0,22,79
        .byte 76,85,0,0,79,68,68,73,255,0,8,71,77,77,73,0,74,77,73,97,74,77,69,71,102,77,80
        .byte 68,80,72,75,102,255,68,19,73,72,79,68,68,78,68,76,0,0,79,68,134,68,68,68,72,78,72,78,72,72,76,75,102
        .byte 255,68,4,72,0,0,72,78,76,0,78,0,0,0,75,78,131,134,78,72,255,0,8,79,68,68,68,80,255,0,5,5,255,0,7
        .byte 79,132,73,255,0,10,79,68,68,80,255,0,6,5,255,0,5,71,68,68,80,255,0,10,75,68,134,80,255,0,6,5,255,0,6
        .byte 79,68,80,255,0,11,79,131,76,255,0,6,5,255,0,6,79,68,80,255,0,11,79,68,73,255,0,6,5,73,101,255,0,4
        .byte 79,68,76,255,0,11,75,102,80,255,0,5,71,77,68,80,255,0,4,75,68,70,255,0,12,79,80,255,0,6,79,68,76
        .byte 255,0,4,74,80,255,0,5,74,80,255,0,5,71,68,76,255,0,6,75,80,255,0,5,79,80,255,0,5,79,80,255,0,5
        .byte 71,68,70,255,0,6,71,70,255,0,5,71,80,255,0,5,79,68,73,255,0,5,79,73,255,0,6,74,80,255,0,12,71
        .byte 68,80,255,0,5,79,70,255,0,6,79,68,70,255,0,11,79,134,80,255,0,12,84,71,68,80,255,0,10,74,134,131,80
        .byte 255,0,12,98,79,76,84,255,0,10,79,132,68,70,255,0,12,75,102,0,98,255,0,10,71,78,72,255,0,13,79,68,84
        .byte 85,255,0,26,74,68,83,113,255,0,25,74,68,68,75,102,73,255,0,24,72,75,68,71,68,80,255,0,24,101,74,68,74
        .byte 68,68,77,73,97,74,77,77,77,73,74,255,77,4,73,97,69,74,77,77,73,74,77,73,97,79,68,68
        .byte 68,68,78,76,72,0,72,79,68,68,68,70,0,0,75,76,75,255,68,9,78,68,68,68,75,80,255,0,5,71,102,68,68
        .byte 80,255,0,5,79,255,68,7,70,0,72,78,72,255,0,8,75,76,75,76,255,0,5,79,102,68,68,68,134,68,68,70,0,0
        .byte 5,255,0,19,75,78,68,68,132,134,80,255,0,4,5,255,0,20,72,102,68,131,80,255,0,4,5,255,0,20,101,79,68,68
        .byte 70,255,0,4,5,255,0,20,75,68,68,80,255,0,5,5,77,77,73,97,74,77,73,255,0,13,79,68,68,80,255,0,5
        .byte 74,68,68,78,68,68,68,80,255,0,13,71,68,68,70,255,0,5,79,68,76,0,75,72,79,76,255,0,13,79,68,80,255,0,6
        .byte 79,80,255,0,18,71,102,68,80,255,0,5,71,68,70,255,0,19,79,78,76,255,0,5,79,68,73,255,0,11,69,255,0,15
        .byte 75,102,68,255,77,5,70,71,255,77,4,80,255,0,16,79,68,76,72,75,72,75,73,74,68,78,76,72,255,0,17,75,68
        .byte 70,101,0,0,0,79,68,68,70,255,0,19,101,68,80,83,0,97,71,68,68,80,255,0,20,85,68,68,80,0,79,77,68
        .byte 80,255,0,21,75,68,134,68,77,68,68,68,70,255,0,8,97,69,113,69,255,0,8,74,68,134,132,68,68,68,70,72,255,0,9
        .byte 75,102,68,80,97,101,255,0,6,79,68,132,131,68,78,80,255,0,12,79,68,68,76,83,255,0,5,79,68,68,133,132,76
        .byte 255,0,15,79,102,68,76,0,0,0,75,102,78,68,78
        .byte 68,68,72,75,76,72,72,78,78,76,0,75,76,72,72,75,102,255,68,5,134,68,68,70,0,75,102,68,75,78,70,255,0,13
        .byte 79,255,68,4,134,132,68,68,80,0,0,79,78,255,0,16,71,78,102,68,68,132,131,68,70,0,0,0,5,255,0,19,75
        .byte 68,68,70,75,76,255,0,5,5,255,0,17,101,79,68,80,255,0,8,5,255,0,8,71,77,73,255,0,6,75,102,80,255,0,9
        .byte 5,255,0,8,75,68,68,70,255,0,6,75,76,255,0,9,5,77,73,255,0,6,71,78,80,255,0,18,77,68,76,255,0,24
        .byte 69,73,74,68,76,255,0,25,79,68,68,68,70,255,0,25,79,68,68,68,70,255,0,25,75,68,68,68,80,255,0,16,79
        .byte 77,73,255,0,6,71,68,68,68,70,255,0,7,74,77,73,255,0,6,75,68,70,255,0,6,74,68,68,68,76,255,0,7
        .byte 75,68,80,255,0,6,79,68,80,255,0,7,72,102,68,73,255,0,7,74,68,80,255,0,6,71,68,80,255,0,8,72,79,79
        .byte 68,76,255,0,5,75,102,68,70,255,0,5,79,78,76,255,0,9,79,68,68,76,255,0,5,101,79,68,76,255,0,17,75
        .byte 68,80,255,0,6,83,79,80,84,255,0,16,97,74,68,73,97,255,0,5,75,68,80,83,255,0,15,71,255,68,4,76,255,0,5
        .byte 71,68,68,80,255,0,15,79,68,134,68,68,80,255,0,5,75,102,134,76,255,0,13,79,68,68,134,131
        .byte 68,68,78,76,72,75,78,78,102,255,68,12,76,75,20,17,16,13,11,10,7,78,72,255,0,6,72,79,102,255,68,9,80
        .byte 0,74,21,19,15,14,12,9,8,255,0,9,72,75,78,255,68,5,78,78,76,0,0,89,88,102,18,20,22,23,24,255,0,12
        .byte 79,68,68,68,76,255,0,7,89,88,102,27,25,26,255,0,12,71,102,68,80,101,255,0,9,89,88,88,21,255,0,13,79
        .byte 68,68,80,255,0,12,89,255,0,13,79,68,68,70,255,0,12,74,77,69,97,255,0,11,79,78,70,255,0,12,75,68,68
        .byte 80,255,0,24,84,101,79,68,68,70,255,0,24,98,75,68,68,80,255,0,25,79,77,68,68,76,86,255,0,23,71,68,76
        .byte 75,68,80,85,255,0,24,72,0,126,68,68,76,255,0,25,126,120,68,68,70,255,0,9,101,74,77,73,77,73,255,0,10
        .byte 119,117,68,68,80,255,0,9,75,102,68,68,68,76,255,0,9,123,122,118,68,68,76,255,0,9,79,68,68,68,80,97,255,0,5
        .byte 108,109,0,0,121,126,116,68,78,76,255,0,10,79,68,68,68,76,255,0,6,110,111,112,0,114,115,80,97,255,0,11,75
        .byte 68,134,68,70,255,0,7,75,102,111,113,125,68,76,255,0,11,79,134,132,80,101,255,0,7,84,102,91,68,124,80,255,0,11
        .byte 74,68,132,131,134,80,255,0,6,84,98,102,68,68,91,76,255,0,11,79,102,133,132,131,76,255,0,6,98,85,75,134,68,68
        .byte 255,0,4,74,77,255,82,14,92,93,255,82,8,73,69,74,77,255,82,20,90,255,82,31,91,255,82,24,20,11,17,16,13
        .byte 11,10,7,255,82,23,21,19,15,14,12,9,8,255,82,25,18,18,22,23,24,7,255,82,26,27,22,17,8,255,82,10,6
        .byte 82,6,82,6,82,6,82,6,255,82,7,21,27,255,82,15,6,82,82,82,92,93,255,82,23,90,82,82,82,91,255,82,23
        .byte 94,95,255,82,32,6,255,82,0,255,82,25,78,76,72,75,255,82,24,78,76,255,0,4
        .byte 255,81,80,107,255,81,26,68,107,255,81,30,107,255,81,55,20,17,16,13,11,10,7,255,81,24,18,19,15,12,9,8,255,81,26
        .byte 18,22,23,24,67,255,81,26,27,25,26,68,255,81,28,68,255,81,0,255,81,82
        .byte 255,0,0,255,0,49,74,77,69,77,77,73,69,77,74,73,0,0,74,73,255,0,14,74,77,68,68,81,28,31,66,34,37
        .byte 38,41,77,77,81,81,70,255,0,12,77,68,134,81,81,81,29,30,65,35,36,40,39,81,134,68,81,73,255,0,10,73,74
        .byte 81,134,131,81,81,91,28,44,43,81,39,81,81,81,68,81,81,81,73,69,74,77,70,255,0,5,81,81,78,131,81,90,81,81
        .byte 29,43,48,255,81,8,68,68,68,81,81,73,255,0,5,78,72,74,94,95,81,81,78,81,48,38,41,255,81,4,78,72,75
        .byte 255,81,6,77,73,69,0,0,90,96,96,92,93,255,96,24,90,6,255,81,28,6,68,255,81,28,68,6,255,81,28,6,68
        .byte 255,81,28,68,6,255,81,28,6
numsc:
        .byte 14
.out .sprintf("SCREENS SIZE = %d", (* - scdat))

nmedat:
        .byte 8,2,72,72,6,5,32,208,6,1,32,40,6,1,0,120,255
        .byte 0,2,96,40,6,19,136,168,3,1,40,24,7,16,88,216,4,9,128,24,255
        .byte 3,1,40,24,0,2,104,48,6,19,152,104,6,19,144,184,7,16,128,32,255
        .byte 3,1,40,24,0,2,88,24,1,6,88,80,6,19,96,120,7,16,120,208,255
        .byte 3,1,40,24,0,2,128,32,1,6,136,192,7,16,128,56,6,14,152,224,6,18,160,136,255
        .byte 3,1,40,24,0,2,104,24,7,16,112,104,255
        .byte 3,1,40,24,0,2,112,40,6,14,80,112,7,16,144,200,6,18,40,216,255
        .byte 3,1,40,24,0,2,128,24,2,9,152,208,6,19,56,112,7,16,88,224,255
        .byte 3,1,40,24,0,2,24,40,2,9,88,24,6,18,128,144,7,16,152,208,255
        .byte 3,1,40,24,0,2,72,24,6,19,152,208,7,16,112,16,6,14,80,144,255
        .byte 3,1,40,24,0,2,80,32,1,6,88,72,7,16,128,32,255
        .byte 6,1,24,120,6,1,40,88,6,5,8,152,0,2,72,120,7,16,144,40,7,16,104,232,7,16,32,16,255
        .byte 6,1,64,88,255
        .byte 3,9,56,24,3,9,16,8,3,9,32,40,0,2,56,184,7,9,144,8,255
.out .sprintf("POSITIONS SIZE = %d", (* - nmedat))

NUMOBJ = 1
objdta:
        .byte 95,32,97,109,101,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,254,56,120,254,56,120
.out .sprintf("OBJECTS SIZE = %d", (* - objdta))

font:
        .byte 0,0,0,0,0,0,0,0
        .byte 56,56,56,56,56,0,56,0
        .byte 60,78,133,129,129,161,82,60
        .byte 0,60,126,106,62,247,189,44
        .byte 0,102,251,255,254,125,58,20
        .byte 0,0,0,3,38,29,10,0
        .byte 34,28,56,69,152,177,66,36
        .byte 0,28,58,69,152,176,66,20
        .byte 0,32,32,248,32,32,0,0
        .byte 0,28,34,89,81,69,38,7
        .byte 0,0,20,8,62,8,20,0
        .byte 68,56,28,162,25,141,66,36
        .byte 0,0,0,0,0,48,48,96
        .byte 0,0,0,66,231,165,66,0
        .byte 0,0,0,0,0,0,34,0
        .byte 0,0,0,0,0,0,0,0
        .byte 62,119,118,119,55,119,62,0
        .byte 28,56,56,28,20,127,127,0
        .byte 62,127,14,60,115,63,127,0
        .byte 62,103,30,7,118,103,62,0
        .byte 14,30,58,122,127,14,31,0
        .byte 63,112,126,7,118,119,62,0
        .byte 62,112,126,119,54,119,62,0
        .byte 127,102,15,30,60,28,60,0
        .byte 62,119,54,127,55,119,62,0
        .byte 62,119,118,31,102,119,62,0
        .byte 0,24,24,0,0,24,24,0
        .byte 0,66,165,231,66,0,0,0
        .byte 48,88,72,48,12,22,12,1
        .byte 0,0,0,0,0,48,48,0
        .byte 0,0,16,8,4,8,16,0
        .byte 62,119,103,14,28,0,28,0
        .byte 0,102,137,129,130,69,42,20
        .byte 27,55,102,102,119,127,59,0
        .byte 110,119,62,55,119,127,110,0
        .byte 62,119,102,32,103,111,62,0
        .byte 108,118,103,39,55,127,110,0
        .byte 111,115,60,48,119,127,111,0
        .byte 119,123,48,60,48,112,112,0
        .byte 62,119,96,111,39,127,62,0
        .byte 118,119,127,63,51,119,119,0
        .byte 62,54,28,28,28,58,62,0
        .byte 118,108,12,14,78,126,124,0
        .byte 119,119,62,56,54,119,119,0
        .byte 120,48,48,48,55,125,127,0
        .byte 54,127,107,107,102,119,119,0
        .byte 110,127,55,39,102,111,111,0
        .byte 62,119,103,103,107,119,62,0
        .byte 110,119,55,63,46,112,112,0
        .byte 62,119,101,103,107,108,55,0
        .byte 110,119,55,62,44,111,103,0
        .byte 62,112,62,7,101,127,62,0
        .byte 119,127,93,28,28,28,54,0
        .byte 119,55,102,102,111,127,59,0
        .byte 239,103,103,103,118,60,24,0
        .byte 119,119,34,107,107,127,54,0
        .byte 119,119,58,28,38,119,119,0
        .byte 119,119,55,63,30,118,120,0
        .byte 63,55,14,28,59,127,119,0
        .byte 0,14,8,8,8,8,14,0
        .byte 0,0,64,32,16,8,4,0
        .byte 0,112,16,16,16,16,112,0
        .byte 0,16,56,84,16,16,16,0
        .byte 0,0,34,85,42,85,42,20
        .byte 0,28,34,120,32,32,126,0
        .byte 246,103,102,0,124,238,230,124
        .byte 252,126,114,254,0,255,127,127
        .byte 157,177,177,0,124,242,240,126
        .byte 0,4,4,60,68,68,60,0
        .byte 0,0,0,0,252,232,212,252
        .byte 124,246,246,124,0,191,188,191
        .byte 252,246,246,246,0,239,239,239
        .byte 222,222,222,126,0,251,59,251
        .byte 126,252,126,252,0,254,206,254
        .byte 0,4,0,4,4,4,36,24
        .byte 0,32,40,48,48,40,36,0
        .byte 0,16,16,16,16,16,12,0
        .byte 0,0,104,84,84,84,84,0
        .byte 0,0,120,68,68,68,68,0
        .byte 0,0,0,0,124,238,230,124
        .byte 0,0,0,0,254,246,230,248
        .byte 0,0,60,68,68,60,4,6
        .byte 0,0,28,32,32,32,32,0
        .byte 0,0,0,0,126,248,62,252
        .byte 140,206,239,0,254,186,56,124
        .byte 0,0,0,0,238,238,230,126
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
