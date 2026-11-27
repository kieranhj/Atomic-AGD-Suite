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
MAPWID = 7
        .byte 255,255,255,255,255,255,255
mapdat:
        .byte 255,0,8,16,24,32,255,255,1,9,17,25,33,255,255,2,10,18,26,34,255,255,3,11,19,27,35,255,255,4,12,20,28,255,255,255,5,13,21,29,255,255,255,6,14,22,30,255,255,255,7,15,23,31,255,255
        .byte 255,255,255,255,255,255,255
stmap:  .byte 1

evnt00:
        lda #23
        cmp scno
        beq *+5
        jmp a00032
        ldy #9
        lda (z80_ix),y
        sta vars
a00032: ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
        lda #7
        sta z80_b
        jsr sktyp
        bcs :+
        jmp a00102
:
        lda #3
        ldy #6
        sta (z80_ix),y
        lda #1
        sta vare
a00102: ldy #8
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #8
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp a00360
:
        lda #0
        ldy #6
        sta (z80_ix),y
        ; lda #0
        iny; ldy #7
        sta (z80_ix),y
        ; lda #0
        cmp varc
        beq *+5
        jmp a00252
        lda #<1
        sta z80_l
        lda #>1
        sta z80_h
        jsr addsc
        lda #5
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        asl a
        sta sndtyp
a00252: lda #0
        ldy #6
        sta (z80_ix),y
        ; lda #0
        cmp vara
        beq *+5
        jmp a00292
        ; lda #0
        jsr animsp
a00292: lda #164
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a00356
        lda #1
        sta nexlev
        lda varr
        iny; ldy #9
        sta (z80_ix),y
        lda #12
        dey; ldy #8
        sta (z80_ix),y
        rts
a00356: jmp a00399
a00360: lda #3
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp varc
        beq *+5
        jmp a00399
        ; lda #0
        jsr animsp
a00399: lda joyval
        and #8
        beq :+
        jmp a00474
:
        jsr cangu
        beq :+
        jmp a00469
:
        lda #0
        cmp vara
        beq *+5
        jmp a00469
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a00469: jmp a00872
a00474: jsr cangd
        beq :+
        jmp a00664
:
        lda #0
        ldy #6
        sta (z80_ix),y
        lda joyval
        and #4
        beq :+
        jmp a00576
:
        lda #0
        ldy #6
        sta (z80_ix),y
        lda vart
        cmp #1
        bcs *+5
        jmp a00576
        lda vart
        sec
        sbc #1
        sta vart
        lda #0
        sta vara
        jmp a00576
a00576: lda #0
        cmp vara
        beq *+5
        jmp a00659
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #10
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        asl a
        sta sndtyp
a00659: jmp a00872
a00664: lda joyval
        and #1
        beq :+
        jmp a00770
:
        jsr cangr
        beq :+
        jmp a00765
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #1
        ldy #6
        sta (z80_ix),y
        lda #0
        jsr animsp
        jsr cangd
        beq :+
        jmp a00765
:
        lda #0
        ldy #6
        sta (z80_ix),y
a00765: jmp a00872
a00770: lda joyval
        and #2
        beq :+
        jmp a00872
:
        jsr cangl
        beq :+
        jmp a00872
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #2
        ldy #6
        sta (z80_ix),y
        lda #0
        jsr animsp
        jsr cangd
        beq :+
        jmp a00872
:
        lda #0
        ldy #6
        sta (z80_ix),y
a00872: lda joyval
        and #1
        beq :+
        jmp a00980
:
        lda #168
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp a00931
        ; lda #168
        ; ldy #9
        sta (z80_ix),y
        jmp a00980
a00931: jsr cangr
        beq :+
        jmp a00980
:
        jsr cangd
        beq :+
        jmp a00980
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a00980: lda joyval
        and #2
        beq :+
        jmp a01088
:
        lda #8
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp a01039
        ; lda #8
        ; ldy #9
        sta (z80_ix),y
        jmp a01088
a01039: jsr cangl
        beq :+
        jmp a01088
:
        jsr cangd
        beq :+
        jmp a01088
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a01088: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01128
:
        lda #8
        ldy #5
        sta (z80_ix),y
a01128: ldy #8
        lda (z80_ix),y
        sta varq
        iny; ldy #9
        lda (z80_ix),y
        sta varr
        rts
.out .sprintf("EVENT 0 SIZE = %d", (* - evnt00))

evnt01:
        lda #0
        cmp varg
        beq *+5
        jmp b00064
        lda #10
        sta chary
        lda #9
        sta charx
        lda #1
        jsr dmsg
        lda #40
        jsr delay
        jsr redraw
        lda #1
        sta varg
b00064: jsr cangl
        beq :+
        jmp b00515
:
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #0
        cmp varc
        beq *+5
        jmp b00135
        jsr vapour
b00135: lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #9
        sta (z80_ix),y
        ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
        lda #20
        ; ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b00260
        lda #0
        ; ldy #11
        sta (z80_ix),y
b00260: lda #0
        cmp varc
        beq *+5
        jmp b00295
        ldy #11
        lda (z80_ix),y
        asl a
        sta sndtyp
b00295: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00399
        lda #150
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        tya; lda #10
        sta z80_c
        lda varrnd
        clc
        adc z80_c
        sta varrnd
        lda varrnd
        sta vard
        lda #1
        ; ldy #10
        sta (z80_ix),y
b00399: lda vard
        ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp b00510
        lda #8
        dey; ldy #8
        sta (z80_ix),y
        iny; ldy #9
        lda (z80_ix),y
        sta varr
        lda varr
        lsr a
        lsr a
        lsr a
        sta varr
        lda varr
        asl a
        asl a
        asl a
        sta varr
        lda #0
        sta z80_c
        ; lda #0
        sta z80_b
        jsr spawn
b00510: jmp b00542
b00515: lda #255
        ldy #5
        sta (z80_ix),y
        lda #0
        ldy #10
        sta (z80_ix),y
b00542: rts
.out .sprintf("EVENT 1 SIZE = %d", (* - evnt01))

evnt02:
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00074
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #8
        ldy #5
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
c00074: lda #14
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00278
        lda #0
        cmp varb
        beq *+5
        jmp c00126
        ; lda #0
        jsr animsp
        jmp c00126
c00126: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp c00202
        jsr cangu
        beq :+
        jmp c00189
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp c00202
c00189: lda #1
        ldy #11
        sta (z80_ix),y
c00202: lda #1
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp c00278
        jsr cangd
        beq :+
        jmp c00265
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp c00278
c00265: lda #0
        ldy #11
        sta (z80_ix),y
c00278: lda #13
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00470
        lda #0
        cmp varb
        beq *+5
        jmp c00326
        ; lda #0
        jsr animsp
c00326: lda #90
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #5
        cmp varrnd
        beq *+5
        jmp c00470
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #2
        sta z80_c
        lda #11
        sta z80_b
        jsr spawn
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        rts
c00470: lda #11
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00707
        lda #2
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00528
:
        lda #255
        ldy #5
        sta (z80_ix),y
c00528: jsr cangu
        beq :+
        jmp c00694
:
        lda #0
        cmp varb
        beq *+5
        jmp c00567
        ; lda #0
        jsr animsp
c00567: ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        iny; ldy #9
        lda (z80_ix),y
        cmp varr
        bcc *+5
        jmp c00640
        ; ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
c00640: lda varr
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp c00689
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #9
        sta (z80_ix),y
c00689: jmp c00707
c00694: lda #255
        ldy #5
        sta (z80_ix),y
c00707: lda #12
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c01097
        lda #8
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp c00883
        lda #110
        cmp varq
        bcs *+5
        jmp c00864
        lda #168
        ; ldy #8
        sta (z80_ix),y
        lda #156
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        tya; lda #8
        sta z80_c
        lda varrnd
        clc
        adc z80_c
        sta varrnd
        lda varrnd
        iny; ldy #9
        sta (z80_ix),y
        rts
        jmp c00878
c00864: lda #255
        ldy #5
        sta (z80_ix),y
c00878: jmp c01097
c00883: jsr cangu
        beq :+
        jmp c00971
:
        lda #0
        cmp varb
        beq *+5
        jmp c00967
        ; lda #0
        jsr animsp
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
c00967: jmp c01097
c00971: lda #110
        cmp varq
        bcs *+5
        jmp c01084
        lda #168
        ldy #8
        sta (z80_ix),y
        lda #156
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        tya; lda #8
        sta z80_c
        lda varrnd
        clc
        adc z80_c
        sta varrnd
        lda varrnd
        iny; ldy #9
        sta (z80_ix),y
        rts
        jmp c01097
c01084: lda #255
        ldy #5
        sta (z80_ix),y
c01097: lda #8
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c01225
        ; lda #8
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp c01164
        lda #168
        ; ldy #9
        sta (z80_ix),y
        rts
        jmp c01225
c01164: jsr cangl
        beq :+
        jmp c01225
:
        lda #0
        cmp varc
        beq *+5
        jmp c01225
        ; lda #0
        jsr animsp
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
c01225: lda #9
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c01345
        lda #0
        cmp varc
        beq *+5
        jmp c01273
        ; lda #0
        jsr animsp
c01273: lda #2
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #168
        ; ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp c01345
        lda #8
        ; ldy #9
        sta (z80_ix),y
c01345: rts
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
d00271: lda #1
        cmp varc
        beq *+5
        jmp d00296
        lda #0
        jsr animsp
d00296: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00368
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #8
        ldy #5
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
d00368: rts
.out .sprintf("EVENT 3 SIZE = %d", (* - evnt03))

evnt04:
        lda #18
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00206
        lda #0
        cmp varc
        beq *+5
        jmp e00050
        ; lda #0
        jsr animsp
e00050: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e00130
        jsr cangl
        beq :+
        jmp e00112
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp e00126
e00112: lda #1
        ldy #10
        sta (z80_ix),y
e00126: jmp e00206
e00130: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e00206
        jsr cangr
        beq :+
        jmp e00193
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp e00206
e00193: lda #0
        ldy #10
        sta (z80_ix),y
e00206: lda #17
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00724
        lda vars
        ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp e00366
        lda #20
        cmp varq
        bcc *+5
        jmp e00366
        lda #16
        sta z80_c
        dey; ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #2
        sta z80_c
        lda #11
        sta z80_b
        jsr spawn
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp e00366
e00366: lda #0
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp e00559
        jsr cangl
        beq :+
        jmp e00541
:
        lda #2
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
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
        jsr cangd
        beq :+
        jmp e00500
:
        lda #1
        ldy #12
        sta (z80_ix),y
e00500: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp e00554
e00541: lda #1
        ldy #12
        sta (z80_ix),y
e00554: jmp e00724
e00559: jsr cangr
        beq :+
        jmp e00711
:
        lda #2
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
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
        jsr cangd
        beq :+
        jmp e00671
:
        lda #0
        ldy #12
        sta (z80_ix),y
e00671: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp e00724
e00711: lda #0
        ldy #12
        sta (z80_ix),y
e00724: lda #16
        ldy #6
        cmp (z80_ix),y
        bcs *+5
        jmp e01157
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp e00965
        jsr cangl
        beq :+
        jmp e00947
:
        lda #0
        cmp varc
        beq *+5
        jmp e00808
        ; lda #0
        jsr animsp
e00808: lda #16
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
        jmp e00907
:
        lda #1
        ldy #11
        sta (z80_ix),y
e00907: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp e00960
e00947: lda #1
        ldy #11
        sta (z80_ix),y
e00960: jmp e01157
e00965: jsr cangr
        beq :+
        jmp e01143
:
        lda #0
        cmp varc
        beq *+5
        jmp e01004
        ; lda #0
        jsr animsp
e01004: lda #15
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
        jmp e01103
:
        lda #0
        ldy #11
        sta (z80_ix),y
e01103: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp e01157
e01143: lda #0
        ldy #11
        sta (z80_ix),y
e01157: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e01229
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #8
        ldy #5
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
e01229: rts
.out .sprintf("EVENT 4 SIZE = %d", (* - evnt04))

evnt05:
        jsr cangd
        beq :+
        jmp f00046
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
        jmp f00191
f00046: lda #6
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #2
        sta z80_c
        lda varrnd
        clc
        adc z80_c
        sta varrnd
        ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
        lda #100
        ; ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp f00191
        lda #8
        sta chary
        ; lda #8
        sta charx
        lda #27
        jsr dmsg
        jsr prskey
        lda #1
        sta gamwon
f00191: rts
.out .sprintf("EVENT 5 SIZE = %d", (* - evnt05))

evnt06:
        lda #21
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp g00111
        lda #0
        cmp varc
        beq *+5
        jmp g00050
        ; lda #0
        jsr animsp
g00050: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00106
:
        lda #100
        asl a
        sta sndtyp
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #99
        sta vart
g00106: jmp g00181
g00111: lda #30
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        asl a
        sta sndtyp
        lda #5
        cmp varrnd
        beq *+5
        jmp g00181
        lda #10
        jsr explod
g00181: rts
.out .sprintf("EVENT 6 SIZE = %d", (* - evnt06))

evnt07:
        lda #0
        cmp scno
        beq *+5
        jmp h00184
        lda #8
        sta chary
        lda #7
        sta charx
        lda #24
        jsr dmsg
        lda #9
        sta chary
        lda #7
        sta charx
        lda #25
        jsr dmsg
        lda #10
        sta chary
        lda #7
        sta charx
        lda #26
        jsr dmsg
        lda #8
        sta chary
        ; lda #8
        sta charx
        lda #21
        jsr dmsg
        lda #9
        sta chary
        lda #8
        sta charx
        lda #22
        jsr dmsg
        lda #10
        sta chary
        lda #8
        sta charx
        lda #23
        jsr dmsg
        jsr prskey
        lda #1
        sta nexlev
        rts
h00184: lda #0
        cmp varc
        beq *+5
        jmp h00210
        ; lda #0
        jsr animsp
h00210: lda #1
        cmp vare
        beq *+5
        jmp h00306
        lda #100
        asl a
        sta sndtyp
        lda #10
        sta chary
        lda #8
        sta charx
        lda #2
        jsr dmsg
        lda #<235
        sta z80_l
        lda #>235
        sta z80_h
        jsr addsc
        lda #60
        jsr delay
        lda #1
        sta nexlev
        rts
h00306: rts
.out .sprintf("EVENT 7 SIZE = %d", (* - evnt07))

evnt08:
        jsr cangd
        beq :+
        jmp i00153
:
        lda #4
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        iny; ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
        lda #90
        ; ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp i00130
        lda #0
        ; ldy #11
        sta (z80_ix),y
i00130: ldy #11
        lda (z80_ix),y
        asl a
        sta sndtyp
        jmp i00233
i00153: lda #5
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp i00192
        ; lda #0
        jsr animsp
i00192: lda #4
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp i00233
        lda #20
        jsr delay
        lda #1
        sta deadf
        rts
i00233: rts
.out .sprintf("EVENT 8 SIZE = %d", (* - evnt08))

evnt09:
        lda #0
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00037
        lda varr
        ldy #9
        sta (z80_ix),y
j00037: lda #1
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00068
        lda #99
        sta vart
j00068: lda #2
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00133
        lda vara
        clc
        adc #1
        sta vara
        lda #0
        ldy #11
        sta (z80_ix),y
        lda #69
        iny; ldy #12
        sta (z80_ix),y
j00133: rts
.out .sprintf("EVENT 9 SIZE = %d", (* - evnt09))

evnt10:
        lda #0
        cmp varc
        beq *+5
        jmp k01193
        lda #1
        cmp scno
        beq *+5
        jmp k00060
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k00060: lda #4
        cmp scno
        beq *+5
        jmp k00102
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k00102: lda #5
        cmp scno
        beq *+5
        jmp k00144
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k00144: lda #6
        cmp scno
        beq *+5
        jmp k00185
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k00185: lda #7
        cmp scno
        beq *+5
        jmp k00227
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k00227: lda #11
        cmp scno
        beq *+5
        jmp k00269
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k00269: lda #13
        cmp scno
        beq *+5
        jmp k00311
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k00311: lda #14
        cmp scno
        beq *+5
        jmp k00353
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k00353: lda #15
        cmp scno
        beq *+5
        jmp k00395
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k00395: lda #16
        cmp scno
        beq *+5
        jmp k00437
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k00437: lda #17
        cmp scno
        beq *+5
        jmp k00479
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k00479: lda #18
        cmp scno
        beq *+5
        jmp k00521
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k00521: lda #19
        cmp scno
        beq *+5
        jmp k00563
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k00563: lda #20
        cmp scno
        beq *+5
        jmp k00605
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k00605: lda #21
        cmp scno
        beq *+5
        jmp k00647
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k00647: lda #22
        cmp scno
        beq *+5
        jmp k00689
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k00689: lda #23
        cmp scno
        beq *+5
        jmp k00731
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k00731: lda #24
        cmp scno
        beq *+5
        jmp k00773
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k00773: lda #25
        cmp scno
        beq *+5
        jmp k00815
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k00815: lda #26
        cmp scno
        beq *+5
        jmp k00857
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k00857: lda #27
        cmp scno
        beq *+5
        jmp k00899
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k00899: lda #29
        cmp scno
        beq *+5
        jmp k00941
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k00941: lda #30
        cmp scno
        beq *+5
        jmp k00983
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k00983: lda #31
        cmp scno
        beq *+5
        jmp k01025
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k01025: lda #32
        cmp scno
        beq *+5
        jmp k01067
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k01067: lda #33
        cmp scno
        beq *+5
        jmp k01109
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k01109: lda #34
        cmp scno
        beq *+5
        jmp k01151
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k01151: lda #35
        cmp scno
        beq *+5
        jmp k01193
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k01193: lda #0
        cmp scno
        beq *+5
        jmp k01234
        lda #4
        sta chary
        lda #7
        sta charx
        lda #6
        jsr dmsg
k01234: lda varc
        clc
        adc #1
        sta varc
        lda #3
        cmp varc
        bcc *+5
        jmp k01274
        lda #0
        sta varc
k01274: lda #0
        cmp varb
        beq *+5
        jmp k01304
        lda #1
        sta varb
        jmp k01312
k01304: lda #0
        sta varb
k01312: lda vara
        clc
        adc #1
        sta vara
        lda #3
        cmp vara
        bcc *+5
        jmp k01352
        lda #0
        sta vara
k01352: rts
.out .sprintf("EVENT 10 SIZE = %d", (* - evnt10))

evnt11:
        lda #9
        cmp vart
        bcs *+5
        jmp l00096
        lda #16
        sta chary
        lda #30
        sta charx
        lda #0
        cmp vart
        beq *+5
        jmp l00057
        jmp l00057
l00057: lda vart
        jsr disply
        lda #16
        sta chary
        lda #29
        sta charx
        lda #14
        jsr dmsg
        jmp l00122
l00096: lda #16
        sta chary
        lda #29
        sta charx
        lda vart
        jsr disply
l00122: lda #9
        cmp scno
        bcs *+5
        jmp l00164
        lda #21
        sta chary
        lda #29
        sta charx
        lda #14
        jsr dmsg
l00164: lda #7
        sta chary
        lda #25
        sta charx
        jsr dscor
        rts
.out .sprintf("EVENT 11 SIZE = %d", (* - evnt11))

evnt12:
        rts
.out .sprintf("EVENT 12 SIZE = %d", (* - evnt12))

evnt13:
        lda #99
        sta vart
        lda #0
        sta vare
        ; lda #0
        sta vard
        lda #9
        sta numlif
        jsr cls
        lda #16
        sta chary
        lda #25
        sta charx
        lda #28
        jsr dmsg
        lda #15
        sta chary
        lda #24
        sta charx
        lda #8
        jsr dmsg
        lda #16
        sta chary
        lda #31
        sta charx
        lda #10
        jsr dmsg
        lda #16
        sta chary
        lda #24
        sta charx
        lda #18
        jsr dmsg
        lda #17
        sta chary
        lda #24
        sta charx
        lda #9
        jsr dmsg
        lda #1
        sta chary
        lda #25
        sta charx
        lda #15
        jsr dmsg
        lda #2
        sta chary
        lda #26
        sta charx
        lda #16
        jsr dmsg
        lda #0
        sta chary
        lda #24
        sta charx
        lda #8
        jsr dmsg
        lda #6
        sta chary
        lda #26
        sta charx
        lda #11
        jsr dmsg
        lda #1
        sta chary
        lda #2
        sta loopa
n00281: lda #31
        sta charx
        lda #10
        jsr dmsg
        dec loopa
        beq :+
        jmp n00281
:
        lda #1
        sta chary
        lda #2
        sta loopa
n00328: lda #24
        sta charx
        lda #18
        jsr dmsg
        dec loopa
        beq :+
        jmp n00328
:
        lda #3
        sta chary
        lda #24
        sta charx
        lda #9
        jsr dmsg
        lda #5
        sta chary
        lda #24
        sta charx
        lda #8
        jsr dmsg
        lda #11
        sta chary
        lda #26
        sta charx
        lda #12
        jsr dmsg
        lda #6
        sta chary
        lda #2
        sta loopa
n00449: lda #31
        sta charx
        lda #10
        jsr dmsg
        dec loopa
        beq :+
        jmp n00449
:
        lda #6
        sta chary
        lda #2
        sta loopa
n00496: lda #24
        sta charx
        lda #18
        jsr dmsg
        dec loopa
        beq :+
        jmp n00496
:
        lda #8
        sta chary
        lda #24
        sta charx
        lda #9
        jsr dmsg
        lda #10
        sta chary
        lda #24
        sta charx
        lda #8
        jsr dmsg
        lda #11
        sta chary
        lda #2
        sta loopa
n00593: lda #31
        sta charx
        lda #10
        jsr dmsg
        dec loopa
        beq :+
        jmp n00593
:
        lda #11
        sta chary
        lda #2
        sta loopa
n00640: lda #24
        sta charx
        lda #18
        jsr dmsg
        dec loopa
        beq :+
        jmp n00640
:
        lda #13
        sta chary
        lda #24
        sta charx
        lda #9
        jsr dmsg
        lda #19
        sta chary
        lda #24
        sta charx
        lda #8
        jsr dmsg
        lda #20
        sta chary
        lda #2
        sta loopa
n00737: lda #31
        sta charx
        lda #10
        jsr dmsg
        dec loopa
        beq :+
        jmp n00737
:
        lda #20
        sta chary
        lda #2
        sta loopa
n00784: lda #24
        sta charx
        lda #18
        jsr dmsg
        dec loopa
        beq :+
        jmp n00784
:
        lda #22
        sta chary
        lda #24
        sta charx
        lda #9
        jsr dmsg
        lda #0
        sta chary
        ; lda #0
        sta charx
        lda #4
        jsr dmsg
        lda #20
        sta chary
        lda #26
        sta charx
        lda #13
        jsr dmsg
        lda #1
        sta chary
        lda #22
        sta loopa
n00906: lda #0
        sta charx
        lda #5
        jsr dmsg
        dec loopa
        beq :+
        jmp n00906
:
        lda #1
        sta chary
        lda #22
        sta loopa
n00952: lda #23
        sta charx
        lda #5
        jsr dmsg
        dec loopa
        beq :+
        jmp n00952
:
        lda #23
        sta chary
        lda #0
        sta charx
        lda #4
        jsr dmsg
        lda #23
        sta chary
        lda #26
        sta charx
        lda #20
        jsr dmsg
        lda #4
        sta chary
        lda #25
        sta charx
        lda #29
        jsr dmsg
        lda #9
        sta chary
        lda #25
        sta charx
        lda #29
        jsr dmsg
        lda #14
        sta chary
        lda #25
        sta charx
        lda #29
        jsr dmsg
        lda #18
        sta chary
        lda #25
        sta charx
        lda #29
        jsr dmsg
        rts
.out .sprintf("EVENT 13 SIZE = %d", (* - evnt13))

evnt14:
        lda #0
        cmp scno
        beq *+5
        jmp o00026
        lda #1
        sta varm
o00026: lda #1
        cmp scno
        beq *+5
        jmp o00051
        lda #2
        sta varm
o00051: lda #2
        cmp scno
        beq *+5
        jmp o00076
        lda #3
        sta varm
o00076: lda #5
        cmp scno
        beq *+5
        jmp o00101
        lda #4
        sta varm
o00101: lda #9
        cmp scno
        beq *+5
        jmp o00125
        lda #2
        sta varm
o00125: lda #13
        cmp scno
        beq *+5
        jmp o00150
        lda #3
        sta varm
o00150: lda #18
        cmp scno
        beq *+5
        jmp o00175
        lda #4
        sta varm
o00175: lda #23
        cmp scno
        beq *+5
        jmp o00200
        lda #2
        sta varm
o00200: lda #28
        cmp scno
        beq *+5
        jmp o00225
        lda #3
        sta varm
o00225: lda #35
        cmp scno
        beq *+5
        jmp o00250
        lda #1
        sta varm
o00250: lda #0
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp o00348
        ldy #9
        lda (z80_ix),y
        lsr a
        lsr a
        lsr a
        ; ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        asl a
        asl a
        asl a
        ; ldy #9
        sta (z80_ix),y
        dey; ldy #8
        lda (z80_ix),y
        sta varq
o00348: lda #0
        sta varg
        ; lda #0
        sta vare
        lda #160
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        sta vard
        lda #0
        sta vara
        lda #12
        sta chary
        lda #30
        sta charx
        lda numlif
        jsr disply
        lda scno
        cmp #10
        bcs *+5
        jmp o00487
        lda #21
        sta chary
        lda #29
        sta charx
        lda scno
        jsr disply
        jmp o00513
o00487: lda #21
        sta chary
        lda #30
        sta charx
        lda scno
        jsr disply
o00513: rts
.out .sprintf("EVENT 14 SIZE = %d", (* - evnt14))

evnt15:
        rts
.out .sprintf("EVENT 15 SIZE = %d", (* - evnt15))

evnt16:
        lda #8
        sta varq
        lda numlif
        sec
        sbc #1
        sta numlif
        lda #10
        sta chary
        lda #9
        sta charx
        lda #3
        jsr dmsg
        lda #40
        jsr delay
        lda #0
        cmp numlif
        beq *+5
        jmp q00118
        lda #10
        sta chary
        lda #7
        sta charx
        lda #0
        jsr dmsg
        lda #40
        jsr delay
        lda #1
        sta gamwon
q00118: lda #3
        cmp scno
        beq *+5
        jmp q00147
        lda #2
        sta scno
        jsr nwscr
q00147: lda #4
        cmp scno
        beq *+5
        jmp q00177
        lda #2
        sta scno
        jsr nwscr
q00177: lda #6
        cmp scno
        beq *+5
        jmp q00206
        lda #5
        sta scno
        jsr nwscr
q00206: lda #7
        cmp scno
        beq *+5
        jmp q00235
        lda #5
        sta scno
        jsr nwscr
q00235: lda #8
        cmp scno
        beq *+5
        jmp q00264
        lda #5
        sta scno
        jsr nwscr
q00264: lda #10
        cmp scno
        beq *+5
        jmp q00294
        lda #9
        sta scno
        jsr nwscr
q00294: lda #11
        cmp scno
        beq *+5
        jmp q00323
        lda #9
        sta scno
        jsr nwscr
q00323: lda #12
        cmp scno
        beq *+5
        jmp q00353
        lda #9
        sta scno
        jsr nwscr
q00353: lda #14
        cmp scno
        beq *+5
        jmp q00383
        lda #13
        sta scno
        jsr nwscr
q00383: lda #15
        cmp scno
        beq *+5
        jmp q00412
        lda #13
        sta scno
        jsr nwscr
q00412: lda #16
        cmp scno
        beq *+5
        jmp q00442
        lda #13
        sta scno
        jsr nwscr
q00442: lda #17
        cmp scno
        beq *+5
        jmp q00472
        lda #13
        sta scno
        jsr nwscr
q00472: lda #19
        cmp scno
        beq *+5
        jmp q00502
        lda #18
        sta scno
        jsr nwscr
q00502: lda #20
        cmp scno
        beq *+5
        jmp q00531
        lda #18
        sta scno
        jsr nwscr
q00531: lda #21
        cmp scno
        beq *+5
        jmp q00561
        lda #18
        sta scno
        jsr nwscr
q00561: lda #22
        cmp scno
        beq *+5
        jmp q00591
        lda #18
        sta scno
        jsr nwscr
q00591: lda #24
        cmp scno
        beq *+5
        jmp q00621
        lda #23
        sta scno
        jsr nwscr
q00621: lda #25
        cmp scno
        beq *+5
        jmp q00650
        lda #23
        sta scno
        jsr nwscr
q00650: lda #26
        cmp scno
        beq *+5
        jmp q00680
        lda #23
        sta scno
        jsr nwscr
q00680: lda #27
        cmp scno
        beq *+5
        jmp q00710
        lda #23
        sta scno
        jsr nwscr
q00710: lda #29
        cmp scno
        beq *+5
        jmp q00740
        lda #28
        sta scno
        jsr nwscr
q00740: lda #30
        cmp scno
        beq *+5
        jmp q00769
        lda #28
        sta scno
        jsr nwscr
q00769: lda #31
        cmp scno
        beq *+5
        jmp q00799
        lda #28
        sta scno
        jsr nwscr
q00799: lda #32
        cmp scno
        beq *+5
        jmp q00829
        lda #28
        sta scno
        jsr nwscr
q00829: lda #33
        cmp scno
        beq *+5
        jmp q00859
        lda #28
        sta scno
        jsr nwscr
q00859: lda #34
        cmp scno
        beq *+5
        jmp q00888
        lda #28
        sta scno
        jsr nwscr
q00888: rts
.out .sprintf("EVENT 16 SIZE = %d", (* - evnt16))

evnt17:
        jsr prskey
        lda #0
        sta scno
        jsr nwscr
        rts
.out .sprintf("EVENT 17 SIZE = %d", (* - evnt17))

evnt18:
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
        .byte "GAME@OVER",141
        .byte "READY?",141
        .byte "GOOD@JOB",141
        .byte "OUCHHH",141
        .byte ",----------------------,",141
        .byte ".",141
        .byte "PARACHUTE",141
        .byte "abcde",141
        .byte "pggggggf",141
        .byte "qhhhhhhi",141
        .byte "j",141
        .byte "SCORE",141
        .byte "LIVES",141
        .byte "LEVEL",141
        .byte "0",141
        .byte "PARA",141
        .byte "CHUTE",141
        .byte "d",141
        .byte "r",141
        .byte ",",141
        .byte "stuvw",141
        .byte "OPQA",141
        .byte "KEMPSTON",141
        .byte "SINCLAIR",141
        .byte "1",141
        .byte "2",141
        .byte "3",141
        .byte "WELL@DONE",141
        .byte "FUEL",141
        .byte "xxxxxx",141
nummsg:
        .byte 30
.out .sprintf("MESSAGE SIZE = %d", (* - msgdat))

chgfx:
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 245,245,80,80,0,0,152,216,177,145,64,0,245,245,170,170
        .byte 15,15,15,15,15,15,15,143,31,95,175,175,31,95,15,15
        .byte 15,15,15,15,31,15,15,143,95,79,143,47,15,15,15,15
        .byte 90,90,45,165,15,75,15,15,15,15,15,15,15,15,15,15
        .byte 240,240,242,242,245,245,242,242,245,245,240,240,244,245,240,240
        .byte 240,240,182,190,244,120,182,188,210,120,242,248,214,94,240,240
        .byte 95,95,0,136,17,0,4,170,85,2,0,136,93,19,136,137
        .byte 176,240,144,144,160,144,144,112,128,176,128,80,128,32,128,16
        .byte 240,208,144,176,144,80,224,144,208,16,160,16,64,16,128,16
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,95,95,175,175
        .byte 31,31,143,15,31,31,15,143,31,95,143,143,95,31,143,175
        .byte 82,82,37,37,2,2,128,0,72,24,133,133,8,8,32,32
        .byte 15,15,15,15,15,15,31,15,15,15,15,15,15,15,15,15
        .byte 90,90,5,5,0,0,1,132,18,8,4,0,90,90,160,160
        .byte 159,31,159,31,143,175,159,159,159,31,223,159,207,191,223,191
        .byte 95,95,5,5,0,0,1,140,19,8,4,0,95,95,170,170
        .byte 15,15,15,15,63,15,79,143,255,207,223,127,47,175,15,15
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 15,15,15,15,15,15,15,15,15,79,143,15,15,15,15,15
        .byte 15,15,15,15,15,175,95,223,175,239,95,63,15,207,15,15
        .byte 239,255,223,15,143,15,63,143,31,255,191,143,207,47,127,223
        .byte 255,191,15,127,15,31,255,207,31,175,15,255,15,15,255,127
        .byte 239,255,223,239,207,223,223,207,159,159,159,31,159,159,159,31
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 15,143,31,15,15,15,15,143,31,15,15,15,15,143,15,15
        .byte 245,245,15,47,15,79,15,143,31,15,47,15,79,15,250,250
        .byte 143,31,79,47,47,79,31,143,31,143,47,79,79,47,143,31
        .byte 240,135,135,75,135,45,135,30,135,30,135,45,135,75,240,135
        .byte 95,95,15,15,15,15,175,15,15,15,143,15,15,15,175,175
        .byte 135,15,15,30,135,15,75,15,15,45,15,75,45,15,30,15
        .byte 63,143,15,143,15,143,15,143,15,143,15,143,15,143,63,239
        .byte 95,95,15,47,15,79,15,143,31,15,15,15,31,15,15,15
        .byte 15,30,135,45,15,90,135,135,15,30,135,15,15,30,135,15
        .byte 125,229,30,143,31,135,30,143,31,135,30,143,31,135,122,235
        .byte 95,79,143,15,15,15,143,15,15,79,143,47,15,15,175,175
        .byte 245,245,143,15,135,15,250,143,135,15,143,15,135,15,250,250
        .byte 135,45,15,15,135,45,15,15,135,45,15,15,135,45,90,75
        .byte 240,240,30,15,30,15,30,15,30,15,30,15,30,15,30,15
        .byte 79,15,79,15,79,15,79,15,79,15,79,15,79,15,127,255
        .byte 95,95,143,15,15,31,143,15,15,31,143,15,15,31,175,175
        .byte 112,240,192,64,80,80,192,80,80,80,208,64,112,240,240,240
        .byte 240,160,64,208,208,96,80,80,80,96,64,208,240,160,240,80
        .byte 0,2,2,7,5,2,2,0,0,4,4,10,14,4,4,0
bprop:
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
        .byte 0
        .byte 5
        .byte 2
        .byte 0
        .byte 2
        .byte 5
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 5
        .byte 5
        .byte 5
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
        .byte 15,240,59,220,113,142,239,247,248,31,192,3,131,193,197,163,101,166,39,236,48,8,31,248,2,64,3,192,48,12,28,56
        .byte 15,240,59,220,113,142,231,231,252,63,192,3,131,193,197,163,101,166,23,232,16,8,15,240,2,64,55,236,48,12,24,24
        .byte 3,192,7,224,3,160,7,160,211,224,55,224,176,0,115,160,167,160,86,44,176,236,102,224,0,0,7,225,113,143,60,62
        .byte 3,192,7,224,3,160,7,160,227,224,119,224,176,0,121,160,179,160,115,48,176,112,83,96,0,0,7,224,16,12,30,60
        .byte 3,192,7,224,3,160,7,160,227,224,119,224,176,0,117,160,181,160,117,128,181,208,100,208,0,0,7,224,0,0,7,176
        .byte 3,192,7,224,3,160,7,160,227,224,103,224,176,0,117,192,181,192,124,232,190,232,118,0,32,0,7,224,0,0,3,192
        .byte 3,192,7,224,3,160,7,160,99,224,119,224,176,0,117,160,181,160,117,128,181,208,116,208,240,0,39,224,0,0,7,176
        .byte 3,192,7,224,3,160,7,160,227,224,103,224,176,0,121,160,171,160,115,48,168,112,115,96,32,0,7,224,16,12,30,60
        .byte 3,192,7,224,5,192,5,224,7,203,7,236,0,13,5,206,5,229,52,106,55,13,7,102,0,0,135,224,241,142,124,60
        .byte 3,192,7,224,5,192,5,224,7,199,7,238,0,13,5,158,5,205,12,206,14,13,6,202,0,0,7,224,48,8,60,120
        .byte 3,192,7,224,5,192,5,224,7,199,7,238,0,13,5,174,5,173,1,174,11,173,11,38,0,0,7,224,0,0,13,224
        .byte 3,192,7,224,5,192,5,224,7,199,7,230,0,13,3,174,3,173,23,62,23,125,0,110,0,4,7,224,0,0,3,192
        .byte 3,192,7,224,5,192,5,224,7,198,7,238,0,13,5,174,5,173,1,174,11,173,11,46,0,15,7,228,0,0,13,224
        .byte 3,192,7,224,5,192,5,224,7,199,7,230,0,13,5,158,5,213,12,206,14,21,6,206,0,4,7,224,48,8,60,120
        .byte 3,192,7,224,5,160,5,160,7,224,6,96,0,0,19,200,53,172,54,108,48,12,7,224,49,140,5,160,12,56,28,48
        .byte 3,192,7,224,5,160,5,160,7,224,6,96,0,0,19,200,53,172,54,108,48,12,7,224,49,140,5,160,12,48,28,56
        .byte 3,192,7,224,13,176,15,240,8,16,8,16,19,200,7,224,23,232,21,168,16,8,15,240,2,64,3,192,0,0,2,64
        .byte 3,192,7,224,5,160,5,160,7,224,6,96,0,0,19,200,53,172,54,108,48,12,7,224,49,140,5,160,12,48,28,56
        .byte 0,0,0,0,3,192,7,224,5,160,5,160,7,224,6,96,0,0,23,232,52,44,48,12,7,224,49,140,4,32,28,56
        .byte 0,0,0,0,0,0,0,0,3,192,7,224,5,160,5,160,7,224,6,96,0,0,23,232,55,236,4,32,49,140,28,56
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,192,7,224,5,160,5,160,7,224,7,224,22,104,24,24,47,116
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,192,7,224,5,160,7,224,103,228,214,106
        .byte 0,0,0,0,0,0,0,0,0,227,1,37,2,73,31,242,96,1,149,7,128,24,127,16,0,136,0,72,0,36,0,28
        .byte 170,170,0,1,128,0,0,1,128,0,0,1,128,0,85,85,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,32,0,64,0,96,0,176,0,104,0,176,1,120,2,176,1,112,2,224,31,208,103,251,151,255,47,254,30,99,1,224
        .byte 0,0,0,0,0,64,0,160,0,96,0,176,1,96,30,176,101,113,150,226,47,215,31,255,2,255,1,97,0,192,0,0
        .byte 1,224,30,99,103,238,151,247,63,243,6,240,1,248,2,232,1,240,1,232,0,240,0,232,0,208,0,96,0,64,0,32
        .byte 0,0,0,0,0,64,0,160,0,96,0,176,1,96,30,176,101,113,150,226,47,215,31,255,2,255,1,97,0,192,0,0
        .byte 252,159,0,0,0,128,1,252,131,194,207,137,191,197,255,129,1,193,0,254,0,124,0,72,0,37,7,254,0,37,31,254
        .byte 28,156,0,0,0,128,1,252,131,194,207,137,127,197,255,129,1,193,0,254,0,124,0,72,0,37,7,254,0,37,31,254
        .byte 3,224,0,0,0,128,1,252,131,194,207,137,127,197,255,129,1,193,0,254,0,124,0,72,0,37,7,254,0,37,31,254
        .byte 48,12,72,18,156,49,170,89,170,85,40,20,131,193,39,228,5,160,5,160,3,192,1,128,1,128,4,32,4,32,10,80
        .byte 0,0,4,32,3,192,23,232,37,164,69,162,147,201,161,133,161,133,164,37,164,37,170,85,160,5,144,9,128,1,64,2
        .byte 1,128,2,192,5,224,10,240,13,112,6,224,9,144,4,32,43,212,102,102,246,111,0,0,2,32,0,0,16,128,2,8
        .byte 1,128,2,192,5,224,10,240,13,112,6,224,9,144,4,32,43,212,102,102,246,111,0,0,4,0,0,0,0,32,17,4
        .byte 3,192,15,240,15,48,31,152,31,216,31,248,31,248,15,176,7,224,3,192,1,128,0,0,1,0,0,128,0,128,1,0
        .byte 3,192,15,240,15,48,31,152,31,216,31,248,31,248,15,176,7,224,3,192,1,128,0,0,1,0,1,0,2,0,1,0
        .byte 3,192,4,32,2,96,7,224,3,160,7,160,115,174,119,174,115,174,119,174,115,174,7,160,117,174,114,78,1,128,112,14
        .byte 0,0,0,0,0,0,0,0,0,0,1,128,3,192,6,96,6,96,3,192,1,128,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,7,224,8,16,16,8,32,4,32,4,33,132,33,132,32,4,32,4,16,8,8,16,7,224,0,0,0,0
        .byte 15,240,16,8,32,4,64,2,128,1,128,1,128,1,129,129,129,129,128,1,128,1,128,1,64,2,32,4,16,8,15,240
        .byte 135,128,199,192,111,224,50,0,25,176,12,224,6,96,3,0,13,177,12,51,0,3,7,123,7,123,0,0,7,128,7,192
        .byte 7,192,143,224,196,0,97,176,49,224,24,224,14,0,3,0,1,128,12,48,14,48,0,0,7,0,0,0,7,0,7,128
        .byte 135,128,199,192,111,224,50,0,25,176,12,224,6,96,3,0,13,177,12,51,0,3,7,251,7,251,0,0,7,128,7,192
        .byte 7,192,143,224,196,0,97,176,49,224,24,224,14,0,3,0,1,128,12,48,14,48,0,0,7,0,0,0,7,0,7,128
        .byte 1,225,3,227,7,246,0,76,13,152,7,48,6,96,0,192,141,176,204,48,192,0,222,224,222,224,0,0,1,224,3,224
        .byte 3,224,7,241,0,35,13,134,7,140,7,24,0,112,0,192,1,128,12,48,12,112,0,0,0,224,0,0,0,224,1,224
        .byte 1,225,3,227,7,246,0,76,13,152,7,48,6,96,0,192,141,176,204,48,192,0,223,224,223,224,0,0,1,224,3,224
        .byte 3,224,7,241,0,35,13,134,7,140,7,24,0,112,0,192,1,128,12,48,12,112,0,0,0,224,0,0,0,224,1,224
        .byte 3,192,2,192,2,192,2,192,2,192,2,192,2,192,2,192,2,192,2,192,9,144,20,40,43,212,84,42,175,245,87,234
        .byte 7,224,31,248,63,252,63,252,127,254,92,62,184,31,81,15,176,142,208,13,56,26,92,60,111,250,55,244,26,168,5,96
        .byte 7,224,31,248,63,252,63,252,127,254,124,58,248,29,240,138,113,13,176,10,88,29,60,58,95,244,47,232,21,80,2,160
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 24,48,24,48,27,176,27,176,27,176,31,240,7,192,7,192,3,128,3,128,3,128,7,192,6,192,12,96,12,96,60,120
        .byte 4,128,12,224,24,48,27,176,27,176,15,176,15,224,15,224,3,128,3,196,3,196,7,252,6,252,12,0,12,0,60,0
        .byte 24,48,24,48,27,176,27,176,27,176,31,240,7,192,7,192,3,128,3,128,3,128,7,192,6,192,12,96,12,96,60,120
        .byte 1,32,7,48,12,24,13,216,13,216,13,240,7,240,7,240,1,192,35,192,35,192,63,224,63,96,0,48,0,48,0,60
        .byte 31,143,16,79,0,12,117,84,255,168,255,252,149,44,181,108,149,44,181,108,177,36,255,252,0,0,85,84,170,170,255,254
        .byte 31,143,16,79,0,12,117,84,255,168,255,252,255,252,255,252,255,252,255,252,255,252,255,252,0,0,85,84,170,170,255,254
frmlst:
        .byte 0,2
        .byte 2,6
        .byte 8,6
        .byte 14,2
        .byte 16,1
        .byte 17,5
        .byte 22,1
        .byte 23,2
        .byte 25,4
        .byte 29,3
        .byte 32,2
        .byte 34,2
        .byte 36,2
        .byte 38,1
        .byte 39,3
        .byte 42,4
        .byte 46,4
        .byte 50,1
        .byte 51,2
        .byte 53,1
        .byte 54,4
        .byte 58,2,60,0
.out .sprintf("SPRITES SIZE = %d", (* - sprgfx))

scdat:
        .word 345,101,67,72,86,57,48,6,87,115,227,181,136,175,169,273,296,108,99,51,185,206,128,119,221,228
        .word 167,68,56,46,112,248,407,314,324,165
        .byte 255,0,11,13,255,0,10,13,0,255,16,6,7,9,15,8,7,255,16,6,0,0,13,0,22,12,1,255,24,13,1,12,21
        .byte 0,0,0,22,12,1,255,24,13,1,12,21,0,0,0,22,12,1,255,24,13,1,12,21,13,0,0,18,7,8,14,14,9
        .byte 1,7,7,14,7,7,1,9,14,14,9,7,18,255,0,4,4,7,1,255,24,11,1,7,25,0,0,0,3,17,2,7,1
        .byte 255,24,11,1,7,25,0,2,17,0,0,0,22,9,255,24,11,8,21,25,255,0,6,22,9,255,24,11,8,21,25,0,0
        .byte 13,0,13,0,22,9,255,24,11,8,21,25,255,0,6,18,7,1,9,8,9,8,9,8,9,8,9,1,7,18,25,13,255,0,6
        .byte 25,16,1,1,12,7,16,7,12,1,1,16,4,0,25,255,0,4,2,17,3,25,255,4,11,0,0,25,255,0,7,25,13
        .byte 0,0,25,0,0,0,25,0,13,0,0,0,25,255,0,7,25,0,0,0,25,0,0,0,25,255,0,5,25,255,0,7,25
        .byte 0,0,26,27,28,29,30,31,32,0,0,0,2,25,3,17,2,2,17,3,0,25,13,0,4,25,4,4,4,25,4,255,0,4
        .byte 25,255,0,7,25,0,0,0,25,0,0,0,25,255,0,5,25,0,0,0,8,7,41,42,9,0,33,34,35,37,36,38,36
        .byte 39,40,0,13,8,7,5,6,9,25,255,18,4,0,255,4,9,0,0,25,255,18,4,25,0,0,2,17,3,19,20,19,19
        .byte 20,20,19,19,20,17,0,25,0,0,2,17
        .byte 255,0,79,2,3,2,3,255,0,27,3,255,0,49,1,7,14,7,1,255,0,17,255,4,5,255,0,31,2,3,255,0,71
        .byte 2,3,3,255,0,6,14,8,9,7,255,0,18,255,4,4,255,0,64,2,3,2,255,0,47,15,15,11,11,0,8,5,6
        .byte 9,0,11,11,15,15,15,255,11,4,15,15,15,7,1,14,7,18,4,8,9,4,18,7,14,7,1,7,1,7,1,7,1
        .byte 7,1
        .byte 255,0,69,13,255,0,29,2,17,2,3,0,0,0,13,255,0,26,2,17,2,255,0,4,13,255,0,68,13,0,0,0,2
        .byte 17,2,2,255,0,47,13,255,0,24,2,17,3,255,0,33,13,255,0,52,13,255,0,27,2,17,2,255,0,46,13,255,0,28
        .byte 255,0,101,13,0,0,0,2,17,2,3,0,0,13,255,0,4,2,17,2,255,0,79,3,2,17,255,0,14,13,0,0,2
        .byte 17,255,0,49,13,255,0,27,2,17,2,17,17,255,0,5,13,255,0,77,13,255,0,10,2,17,3,255,0,14,2,17,255,0,30
        .byte 13,255,0,38
        .byte 255,0,99,2,17,2,3,255,0,30,2,17,2,255,0,77,2,17,2,2,255,0,74,2,17,3,255,0,112,2,17,2,255,0,12
        .byte 10,0,0,10,255,0,15,15,11,0,8,5,6,9,0,11,11,15,15,11,11,15,15,11,11,15,11,11,15,1,14,18,4
        .byte 8,9,4,18,14,7,1,14,1,7,1,14,1,7,1,14,1,7
        .byte 255,18,66,20,20,19,20,255,18,17,19,255,18,55,19,19,20,19,20,255,18,53,19,20,19,255,18,13,20,20,19,255,18,88
        .byte 19,255,18,9,19,20,19,20,255,18,99,19,20,19,20,255,18,13,20,19,255,18,44
        .byte 255,18,66,20,255,18,15,20,19,255,18,57,19,20,255,18,53,20,19,19,20,255,18,92,19,20,255,18,58,19,255,18,11
        .byte 19,20,255,18,70,19,20,255,18,4,19,20,255,18,40
        .byte 255,18,0,255,18,228
        .byte 255,18,66,20,20,19,20,255,18,17,19,255,18,55,19,19,20,19,20,255,18,53,19,20,19,255,18,13,20,20,19,255,18,88
        .byte 19,255,18,9,19,20,19,20,255,18,118,15,15,11,255,18,4,11,255,18,4,11,15,18,8,5,6,9,18,18,18,8,16
        .byte 9,16,8,16,9,16,8,16,9,16,8,16,18,4,8,9,4,18,8,9
        .byte 255,0,68,13,255,0,30,2,17,2,3,255,0,4,13,255,0,25,2,17,2,255,0,71,13,255,0,5,2,17,2,2,255,0,72
        .byte 2,17,3,255,0,29,13,255,0,33,13,255,0,62,16,1,16,0,0,0,16,1,1,16,0,0,0,16,1,16,18,0,0
        .byte 16,1,16,16,12,16,21,0,22,16,12,12,16,21,0,22,16,12,16,18,0,0,16,12,16,16,12,16,21,0,22,16,12,12
        .byte 16,21,0,22,16,12,16,18,0,0,16,12,16
        .byte 16,12,16,21,0,22,16,12,12,16,21,0,22,16,12,16,18,0,0,16,12,16,16,12,16,21,0,22,16,12,12,16,21
        .byte 0,22,16,12,16,18,0,0,16,12,16,16,12,16,21,0,22,16,12,12,16,21,0,22,16,12,16,18,0,0,16,12,16,16
        .byte 12,16,21,0,22,16,12,12,16,21,0,22,16,12,16,18,0,0,16,12,16,16,12,16,21,0,22,16,12,12,16,21,0
        .byte 22,16,12,16,18,0,0,16,12,16,1,1,1,0,0,0,255,1,4,0,0,0,1,1,1,18,0,0,1,1,1,255,0,22
        .byte 13,255,0,7,13,255,0,78,13,255,0,36,13,255,0,9,13,255,0,71,13,255,0,9,13,255,0,47,1,1,1,18,0,0
        .byte 255,1,4,0,0,0,1,1,1,0,0,0,1,1,1,16,12,16,18,0,0,16,12,12,16,21,0,22,16,12,16,21,0
        .byte 22,16,12,16,16,12,16,18,0,0,16,12,12,16,21,0,22,16,12,16,21,0,22,16,12,16
        .byte 16,12,16,18,0,0,16,12,12,16,21,0,22,16,12,16,21,0,22,16,12,16,16,12,16,18,0,0,16,12,12,16,21
        .byte 0,22,16,12,16,21,0,22,16,12,16,16,12,16,18,0,0,16,12,12,16,21,0,22,16,12,16,21,0,22,16,12,16,16
        .byte 12,16,18,0,0,16,12,12,16,21,0,22,16,12,16,21,0,22,16,12,16,16,12,16,18,0,0,16,12,12,16,21,0
        .byte 22,16,12,16,21,0,22,16,12,16,1,1,1,18,0,0,255,1,4,0,0,0,1,1,1,0,0,0,1,1,1,255,18,4
        .byte 0,0,255,18,4,0,0,0,18,18,18,0,0,0,18,18,18,255,0,30,2,3,17,3,255,0,73,2,17,3,255,0,66
        .byte 3,2,17,3,255,0,75,2,3,17,3,255,0,71
        .byte 255,0,58,13,255,0,32,13,255,0,49,13,255,0,11,13,255,0,10,11,255,0,7,11,255,0,8,11,0,0,0,22,12
        .byte 21,255,0,5,22,12,21,255,0,6,22,12,21,0,0,18,18,18,255,0,5,18,18,18,0,0,0,11,0,0,18,18,18
        .byte 255,0,15,22,12,21,255,0,19,18,18,18,255,0,6,13,255,0,74,13,255,0,4,13,255,0,32,13,255,0,34,10,0,0
        .byte 10,255,0,15,15,11,0,8,5,6,9,255,0,15,1,14,18,4,8,9,4,18,14,7,1,14,1,7,1,14,1,7,1
        .byte 14,1,7
        .byte 255,0,85,2,17,3,255,0,6,2,17,3,255,0,36,11,0,0,0,11,0,0,0,11,0,0,0,11,255,0,8,22,12
        .byte 21,0,22,12,21,0,22,12,21,0,22,12,21,255,0,7,18,18,18,0,18,18,18,0,18,18,18,0,18,18,18,255,0,80
        .byte 2,17,3,255,0,39,11,0,0,0,11,0,0,0,11,0,0,0,11,0,0,0,11,255,0,4,22,12,21,0,22,12,21
        .byte 0,22,12,21,0,22,12,21,0,22,12,255,0,4,18,18,18,0,18,18,18,0,18,18,18,0,18,18,18,0,18,18,255,0,14
        .byte 2,17,3,255,0,50,11,0,0,0,11,0,0,0,11,0,0,0,11,255,0,8,22,12,21,0,22,12,21,0,22,12,21
        .byte 0,22,12,21,255,0,7
        .byte 255,0,44,2,17,3,255,0,38,2,17,3,255,0,49,11,0,0,0,11,0,0,0,11,0,0,0,11,0,0,0,11,255,0,4
        .byte 22,12,21,0,22,12,21,0,22,12,21,0,22,12,21,0,22,12,255,0,4,18,18,18,0,18,18,18,0,18,18,18,0
        .byte 18,18,18,0,18,18,255,0,81,2,17,3,255,0,27,11,0,0,0,11,0,0,0,11,0,0,0,11,0,0,0,11,255,0,4
        .byte 22,12,21,0,22,12,21,0,22,12,21,0,22,12,21,0,22,12,21,0,0,0,18,18,18,0,18,18,18,0,18,18,18
        .byte 0,18,18,18,0,18,18,18,255,0,69,255,1,8,18,0,0,0,255,1,10,255,12,7,16,18,0,0,0,16,255,12,9
        .byte 255,18,6,12,1,255,0,4,1,12,255,18,14,12,1,255,0,4,1,12,20,19,255,18,6,20,255,18,5,1,1,255,0,4
        .byte 1,1,255,18,14,12,16,255,0,4,16,12,255,18,14,12,16,255,0,4,16,12,255,18,14,12,16,255,0,4,16,12,255,18,10
        .byte 20,19,17,19,1,1,255,0,4,1,1,255,18,4,19,17,19,19,255,18,6,12,16,21,0,0,0,16,12,255,18,14,12
        .byte 16,21,0,0,0,16,12,255,18,14,12,16,21,0,0,0,16,12,17,19,20,255,18,11,1,1,18,0,0,0,1,1,255,18,14
        .byte 12,16,255,0,4,16,12,255,18,8,20,19,20,17,18,18,12,16,255,0,4,16,12,255,18,6,19,20,255,18,6,12,16
        .byte 255,0,4,16,12,255,18,14,1,1,0,0,0,22,1,1,255,18,14,12,16,0,0,0,22,16,12,255,18,14,12,16,0,0,0
        .byte 22,16,12,20,19,17,19,255,18,10,12,16,0,0,0,18,16,12,255,18,14,1,1,255,0,4,1,1,255,18,14,12,16
        .byte 255,0,4,16,12,255,18,9,19,17,19,20,18,12,16,21,0,0,0,16,12,255,18,14,12,16,21,0,0,0,16,12,255,18,8
        .byte 255,18,6,12,1,255,0,4,1,12,255,18,11,19,20,19,12,1,255,0,4,1,12,255,18,14,1,1,255,0,4,1,1
        .byte 255,18,14,12,16,255,0,4,16,12,255,18,14,12,16,255,0,4,16,12,255,18,14,12,16,255,0,4,16,12,255,18,8
        .byte 19,20,19,17,18,18,255,1,4,0,0,1,1,18,18,17,19,20,19,255,18,8,12,16,18,18,0,0,16,12,255,18,14
        .byte 12,16,255,0,4,16,12,255,18,14,12,16,255,0,4,16,12,255,18,14,1,1,0,0,255,1,4,255,18,14,12,16,0,0
        .byte 18,18,16,12,255,18,8,19,17,255,18,4,12,16,255,0,4,16,12,255,18,4,19,17,19,255,18,7,12,16,255,0,4
        .byte 16,12,255,18,14,1,1,0,0,255,1,4,255,18,14,12,16,0,0,18,18,16,12,255,18,9,0,18,0,18,0,12,16
        .byte 255,0,4,16,12,0,18,0,18,0,18,0,255,18,7,12,16,255,0,4,16,12,255,18,8,0,18,0,18,0,18,255,1,4
        .byte 0,0,1,1,18,0,18,0,18,0,18,0,18,0,18,0,18,0,12,16,18,18,0,0,16,12,0,18,0,18,0,18,0
        .byte 18,0,13,255,0,4,12,16,255,0,4,16,12,255,0,14,12,16,255,0,4,16,12,0,0,0,13,255,0,4
        .byte 255,18,6,12,1,255,0,4,1,12,255,18,9,0,18,0,18,0,12,1,255,0,4,1,12,0,18,0,18,0,18,0,18
        .byte 0,18,0,18,0,18,1,1,255,0,4,1,1,18,0,18,0,18,0,18,255,0,7,18,18,255,0,4,18,18,255,0,0
        .byte 255,0,104,11,15,0,0,16,1,1,7,8,5,6,9,7,1,1,16,1,0,0,11,15,15,1,16,18,18,255,4,5,8
        .byte 9,255,4,6,18,18,1,16,1
        .byte 255,0,134,11,255,0,6,11,255,0,6,11,255,0,6,22,12,21,255,0,4,22,12,21,255,0,4,22,12,21,255,0,5
        .byte 18,18,18,255,0,4,18,18,18,255,0,4,18,18,18,255,0,141,11,255,0,6,11,255,0,6,11,255,0,6,22,12,21
        .byte 255,0,4,22,12,21,255,0,4,22,12,21,255,0,5,18,18,18,255,0,4,18,18,18,255,0,4,18,18,18,255,0,89
        .byte 255,0,106,2,3,17,3,255,0,25,2,17,3,255,0,49,2,17,3,17,3,255,0,72,2,17,3,255,0,17,2,17,255,0,73
        .byte 2,17,3,17,3,255,0,71,2,17,3,17,3,255,0,44
        .byte 255,0,81,2,17,3,2,255,0,25,2,3,17,3,255,0,154,2,17,3,2,255,0,36,11,11,0,0,0,11,15,11,255,0,4
        .byte 11,15,11,255,0,4,11,15,11,1,1,21,0,22,1,1,1,21,0,0,22,1,1,1,18,0,0,0,1,1,1,12,16
        .byte 21,0,22,16,12,16,21,0,0,22,16,12,16,18,0,0,0,16,12,16,12,16,21,0,22,16,12,16,21,0,0,22,16
        .byte 12,16,18,0,0,0,16,12,16,1,1,21,0,22,1,1,1,21,0,0,22,1,1,1,18,0,0,0,1,1,1,12,16
        .byte 21,0,22,16,12,16,21,0,0,22,16,12,16,18,0,0,0,16,12,16,12,16,15,11,11,16,12,16,255,11,4,16,12
        .byte 16,18,0,0,0,16,12,16,255,1,15,18,0,0,0,1,1,1
        .byte 16,255,1,4,16,1,16,255,1,4,16,1,16,18,0,0,0,16,1,16,16,255,4,4,16,4,16,255,4,4,16,4,16
        .byte 18,0,0,0,16,4,16,255,1,15,18,0,0,0,1,1,1,16,18,18,18,255,23,4,255,18,8,255,0,4,18,16,16
        .byte 18,255,0,19,16,16,18,255,0,19,16,16,18,255,0,19,16,1,18,0,0,255,1,6,255,16,5,1,1,1,16,16,16
        .byte 1,16,18,0,0,255,18,17,16,16,18,255,0,19,16,16,18,255,0,19,16,16,18,255,0,11,11,11,255,0,6,16,255,1,10
        .byte 21,0,22,1,1,1,0,0,0,1,1,1,16,16,12,12,16,16,12,12,16,16,21,0,22,16,16,16,18,0,0,16,16,16
        .byte 255,18,11,0,0,255,18,4,0,0,18,18,18,255,0,132,12,16,1,18,0,0,0,1,16,1,12,12,1,16,16,1,12,12,12
        .byte 1,16,16
        .byte 16,1,1,255,0,4,1,1,16,1,12,16,1,12,16,1,12,16,1,12,16,4,12,16,255,0,4,16,12,255,4,13,0
        .byte 4,1,255,0,4,1,4,255,0,15,4,255,0,4,4,255,0,52,2,17,3,255,0,50,2,17,2,17,255,0,26,10,10
        .byte 255,0,20,16,16,255,0,20,12,12,255,0,20,4,4,255,0,31,2,17,255,0,17,2,3,17,255,0,50,2,17,3,255,0,50
        .byte 8,5,6,9,0,0,0,255,10,15,4,8,9,4,10,10,10,255,7,15,18,8,9,18,7,7,7
        .byte 255,18,45,0,18,0,18,0,18,0,18,0,18,0,18,0,18,0,18,0,18,0,18,0,0,18,0,18,0,18,0,18,0
        .byte 18,0,18,0,18,0,18,0,18,0,18,0,18,255,0,76,15,11,255,0,19,22,12,12,21,255,0,18,22,12,12,21,255,0,18
        .byte 18,23,23,18,255,0,19,18,18,255,0,142,1,255,0,20,255,1,10,18,0,0,255,1,10,16,255,12,7,16,18,0,0
        .byte 16,255,12,8,16,16,255,4,7,16,18,0,0,16,255,4,8,16
        .byte 255,12,8,1,18,0,0,1,255,12,17,16,18,0,0,16,12,12,12,27,27,27,255,12,11,16,18,0,0,16,12,12,12
        .byte 27,27,27,255,12,4,255,1,8,18,0,0,16,12,12,12,27,27,27,255,12,4,16,255,18,8,0,0,16,255,12,10,16
        .byte 18,255,0,9,16,12,27,27,27,12,27,27,27,12,12,16,18,0,0,11,255,0,6,16,12,27,27,27,12,27,27,27,12,12
        .byte 16,18,0,0,255,1,8,12,27,27,27,12,27,27,27,12,12,16,18,0,0,16,255,4,6,16,255,12,9,1,1,18,0,0
        .byte 255,1,17,18,18,18,0,0,0,23,23,23,255,18,11,23,23,255,0,88,11,11,255,0,10,11,11,11,255,0,5,11,11
        .byte 255,1,17,0,0,0,1,1,16,255,18,17,0,0,16,12,16,18,255,0,18,16,12,16,18,255,0,18,16,12,16,15,11
        .byte 255,0,4,11,11,11,255,0,10,16,12,1,1,1,255,0,4,255,1,14,12
        .byte 12,12,1,18,0,0,0,1,12,12,1,16,16,1,16,16,16,1,16,16,16,1,16,16,16,18,0,0,0,16,16,16,255,18,11
        .byte 4,4,12,1,18,0,0,0,1,12,4,255,0,11,16,1,18,16,18,0,0,0,16,4,255,0,12,1,4,18,4,255,0,4
        .byte 4,255,0,5,2,17,3,17,3,0,0,0,4,16,18,255,0,19,16,12,18,255,0,19,12,1,18,255,0,19,1,4,18
        .byte 255,0,19,4,16,18,255,0,19,16,12,18,255,0,19,12,1,18,255,0,19,1,4,18,255,0,19,4,16,18,255,0,19
        .byte 16,12,18,255,0,19,12,1,18,255,0,19,1,4,18,255,0,19,4,16,18,0,0,2,17,3,17,3,255,0,12,16,16
        .byte 18,255,0,11,15,16,18,0,0,0,1,1,16,12,18,255,0,10,15,12,1,18,0,0,0,4,4,4,1,18,255,0,10
        .byte 16,16,16,18,0,0,0,255,16,4,1,12,255,16,7,12,1,1,1,16,18,0,0,0,16,1,12
        .byte 8,7,4,8,7,9,8,7,9,8,7,9,8,7,9,255,0,4,8,7,9,255,1,15,255,0,4,8,7,9,8,7,4
        .byte 9,255,18,11,255,0,4,8,7,9,8,7,4,9,255,0,15,8,7,9,8,7,4,9,255,0,15,8,7,9,255,1,4
        .byte 255,0,9,255,1,9,255,18,4,255,0,9,255,18,9,21,255,0,21,21,255,0,21,21,255,0,43,1,1,255,0,8,1,1,1
        .byte 255,0,9,7,7,255,0,8,8,7,9,255,0,9,8,9,255,0,8,18,18,18,255,0,9,18,18,255,0,32,8,7,9
        .byte 255,0,4,8,7,9,255,0,12,1,1,1,255,0,4,1,1,1,255,0,12,18,18,18,255,0,4,18,18,18,255,0,88
        .byte 255,0,83,2,17,3,255,0,47,2,17,2,17,3,255,0,139,2,17,17,3,255,0,49,2,17,3,255,0,107,8,7,9
        .byte 8,1,9,8,7,9,8,5,6,9,8,7,9,8,1,9,8,7,9,255,24,4,7,255,24,5,8,9,255,24,5,7,255,24,4
        .byte 255,0,34,13,255,0,64,2,17,2,3,0,0,13,255,0,27,2,17,2,0,0,0,13,255,0,73,2,17,2,2,255,0,70
        .byte 13,0,2,17,3,255,0,32,13,255,0,75,13,255,0,5,2,17,2,255,0,75
        .byte 255,0,105,2,17,2,3,255,0,7,2,17,2,255,0,79,3,2,17,255,0,17,2,17,255,0,77,2,17,2,17,17,255,0,94
        .byte 2,17,3,255,0,14,2,17,255,0,69
        .byte 255,0,141,11,255,0,20,22,4,21,255,0,19,18,23,18,255,0,20,18,255,0,15,1,1,255,0,19,22,12,12,21,255,0,8
        .byte 11,255,0,10,18,18,255,0,8,22,4,21,255,0,19,18,23,18,0,1,1,255,0,17,18,0,0,12,12,21,255,0,19
        .byte 18,18,255,0,35,1,1,255,0,11,11,11,255,0,6,22,12,12,21,255,0,9,22,1,1,21,255,0,6,23,23,255,0,11
        .byte 23,23,18,255,0,20,18,255,0,19
        .byte 255,0,220,11,15,11,15,255,0,4,11,255,0,4,11,255,0,4,15,11,15,11,255,1,4,255,0,4,1,255,0,4,1
        .byte 255,0,4,255,1,4,43,43,43,12,18,0,0,0,12,18,0,0,0,12,18,0,0,0,12,255,24,4,43,43,12,18,0,0,0
        .byte 12,18,0,0,0,12,21,0,0,0,12,255,24,4,1,1,1,18,0,0,22,12,18,0,0,0,12,21,0,0,0,1,1,1
        .byte 24,24,24,43,12,18,0,0,22,12,18,0,0,22,12,21,0,0,0,12,24,24,24,43,24,43,12,18,0,0,18,12,18
        .byte 0,0,22,12,18,0,0,0,12,255,24,5,1,1,18,0,0,0,1,18,0,0,22,1,18,0,0,0,1,1,255,24,4
        .byte 43,12,18,0,0,0,12,18,0,0,22,12,18,0,0,0,12,255,24,5,43,12,18,0,0,0,12,18,0,0,18,12,18
        .byte 0,0,0,12,255,24,5,43,1,21,0,0,0,12,18,0,0,0,12,18,0,0,22,1,255,24,5,43,12,21,0,0,0
        .byte 12,18,0,0,0,12,18,0,0,22,12,24,24,24
        .byte 24,24,43,1,0,0,0,18,1,0,0,0,18,1,0,0,0,18,1,43,255,24,4,43,16,0,0,0,18,16,0,0,0
        .byte 18,16,0,0,0,18,16,43,255,24,4,43,16,0,0,0,18,16,0,0,0,18,16,0,0,0,18,16,43,255,24,4,43
        .byte 1,0,0,0,18,1,0,0,1,1,1,0,0,0,18,1,43,255,24,4,43,4,255,0,4,4,255,0,4,4,0,0,0
        .byte 18,4,43,24,24,24,43,43,16,0,0,8,12,16,255,0,8,22,16,43,43,24,43,43,43,16,0,0,4,8,16,255,0,8
        .byte 22,16,43,43,43,255,1,4,0,0,0,4,1,1,255,0,7,18,255,1,4,16,18,18,255,0,14,255,18,4,16,16,255,0,20
        .byte 16,255,1,4,0,0,0,1,1,1,0,0,0,1,1,0,0,0,255,1,4,43,43,43,4,0,0,0,4,4,4,0,0,0
        .byte 4,4,0,0,0,4,43,43,43,24,43,43,1,0,0,22,1,1,1,21,0,0,1,1,0,0,0,1,43,43,24,24,24
        .byte 43,16,0,0,22,16,43,16,21,0,18,16,12,0,0,0,16,43,255,24,4,43,16,0,0,22,16,43,16,21,0,18,16
        .byte 12,0,0,0,16,43,255,24,4,43,1,0,0,22,1,1,1,21,0,18,1,1,0,0,0,1,43,255,24,4,43,4,0,0
        .byte 18,4,4,4,18,0,18,4,4,0,0,0,4,43,255,24,4,43,1,21,0,0,22,1,21,0,0,18,1,18,0,0,0
        .byte 1,43,255,24,4,43,12,21,0,0,22,12,21,0,0,18,12,18,0,0,18,12,43,255,24,4,43,12,21,0,0,22,12
        .byte 21,0,0,18,12,18,0,0,18,12,43,255,24,4,43,255,1,11,18,0,0,22,1,43,255,24,4,255,43,11,16,18,0,0
        .byte 22,16,43,24,24
        .byte 255,24,12,43,1,0,0,0,18,1,43,255,24,13,43,43,4,0,0,0,18,4,43,255,24,11,255,43,4,16,0,0,0
        .byte 18,16,43,255,24,4,8,9,7,8,9,7,8,9,7,8,7,9,0,0,0,18,1,43,255,24,4,16,255,23,4,255,18,7
        .byte 0,0,0,18,4,43,24,24,24,43,16,255,0,14,18,12,43,24,24,24,43,12,255,0,14,18,7,43,24,24,24,43,7
        .byte 255,0,14,18,16,43,43,24,24,43,1,0,0,0,8,7,9,8,7,9,8,7,9,8,7,9,8,7,43,24,24,43,4
        .byte 0,0,0,255,18,6,255,23,7,4,43,24,24,43,16,255,0,15,18,7,43,24,24,43,7,255,0,15,18,16,43,24,24
        .byte 43,12,255,0,15,18,12,43,24,24,24,8,9,7,8,9,7,8,9,7,8,9,7,8,9,0,0,18,1,43,24,24,24
        .byte 43,4,255,23,6,255,18,6,0,0,18,4,43,24,24,24,43,16,255,0,14,18,16,43,24,24,24,43,16,255,0,14,18
        .byte 16,43,24,24,24,43,12,255,0,14,18,12,43,24,24,24,43,1,0,0,8,7,9,8,7,9,8,7,9,8,7,9,8
        .byte 1,43,24,24,24,43,4,0,0,255,18,12,4,43,43,24,24,24,43,16,255,0,13,18,16,43,255,24,4,43,1,9,7
        .byte 8,9,7,8,9,7,8,9,0,0,0,18,1,43,24,24
        .byte 255,24,6,43,43,12,255,16,4,1,18,0,0,18,1,16,16,12,255,24,5,43,43,1,18,18,18,12,16,1,18,0,0
        .byte 18,1,16,16,16,255,24,4,43,43,12,18,0,0,0,18,12,1,18,0,0,18,1,12,18,16,24,24,24,43,43,12,23
        .byte 255,0,5,18,1,18,0,0,18,1,18,18,16,24,24,24,43,16,18,255,0,7,18,18,0,0,18,18,0,18,16,24,24
        .byte 43,43,16,255,0,15,18,16,24,43,43,12,18,255,0,15,18,16,24,12,12,18,255,0,16,18,16,24,43,16,18,255,0,16
        .byte 18,12,24,43,16,18,255,0,15,18,1,43,24,43,16,18,255,0,15,18,16,43,24,43,16,18,255,0,15,18,16,43,24
        .byte 1,1,18,255,0,15,18,16,24,24,43,16,21,255,0,15,18,12,24,24,43,16,21,255,0,14,22,16,43,24,24,43,16
        .byte 21,255,0,14,22,16,43,24,24,12,12,18,255,0,14,22,16,43,24,24,43,16,18,255,0,4,18,18,0,0,18,18,255,0,4
        .byte 18,12,12,24,24,43,16,18,0,0,0,18,9,18,0,0,18,8,18,0,0,0,18,16,43,24,24,12,12,18,0,0,18
        .byte 8,9,18,0,0,18,8,9,18,0,0,18,12,43,24,24,24,43,12,18,18,12,8,9,18,0,0,18,8,9,1,18,18
        .byte 12,43,255,24,5,43,16,16,16,8,9,18,0,0,18,8,9,16,16,16,43,24,24,24
        .byte 255,18,7,12,12,255,0,4,12,12,255,18,15,16,255,0,4,16,255,18,8,0,18,0,18,0,18,0,18,1,255,0,4
        .byte 1,18,0,18,0,18,0,18,0,18,0,18,0,18,0,18,0,18,255,0,4,18,0,18,0,18,0,18,0,18,255,0,23
        .byte 18,0,18,0,18,255,0,10,18,0,18,0,18,255,0,59,13,0,0,2,17,3,17,3,255,0,66,2,17,3,3,0,0,0
        .byte 13,255,0,50,2,17,3,17,0,0,13,255,0,45,17,0,0,13,255,0,18,255,10,8,255,0,6,255,10,8,7,8,7
        .byte 9,7,8,7,9,24,8,5,6,9,24,8,7,9,7,8,7,9,7,255,24,10,8,9,255,24,20,12,12,255,24,10
numsc:
        .byte 36
.out .sprintf("SCREENS SIZE = %d", (* - scdat))

nmedat:
        .byte 7,7,160,160,255
        .byte 1,6,8,168,7,7,168,56,2,8,88,168,2,8,88,136,2,8,88,80,2,8,128,40,255
        .byte 1,6,8,168,2,8,64,168,2,8,64,128,2,8,64,88,2,8,128,8,2,8,128,48,2,8,128,88,255
        .byte 2,8,56,168,2,8,56,120,2,8,56,64,2,9,128,8,2,9,128,40,2,9,128,72,2,9,128,104,0,0,8,88,6,21,152,40,255
        .byte 7,7,168,40,3,10,136,144,3,10,152,72,0,0,8,88,255
        .byte 1,6,8,168,2,8,136,96,2,8,136,48,2,8,136,152,2,9,96,8,2,9,96,56,2,9,96,104,255
        .byte 0,0,8,96,2,12,168,48,2,12,128,136,2,12,88,168,255
        .byte 0,0,8,88,2,8,40,168,2,9,72,88,2,9,152,56,2,8,104,120,2,8,128,80,2,9,152,8,2,9,72,152,2,9,152,112,2,8,104,40,2,9,72,24,6,21,88,152,255
        .byte 2,13,160,168,0,0,8,96,2,13,160,40,2,13,160,80,7,7,168,136,255
        .byte 1,6,8,168,2,9,64,8,2,9,64,40,2,8,136,72,2,8,128,24,2,9,96,104,2,9,96,136,2,8,128,120,255
        .byte 0,0,8,144,2,14,128,64,2,14,88,120,2,14,80,8,2,14,128,168,2,8,96,168,2,9,64,8,2,9,136,88,255
        .byte 0,0,8,32,2,12,168,72,2,12,128,168,2,12,144,8,255
        .byte 0,0,8,8,7,7,168,40,3,10,120,96,3,10,144,168,4,15,160,136,6,21,40,168,255
        .byte 1,6,8,168,2,8,88,168,2,8,88,128,2,8,88,88,2,9,152,8,2,9,152,48,255
        .byte 0,0,8,96,2,9,88,8,2,9,88,48,2,9,88,88,2,8,144,168,255
        .byte 0,0,8,80,255
        .byte 0,0,8,72,2,14,136,72,6,21,160,72,255
        .byte 0,0,8,80,7,7,168,80,2,13,168,24,2,13,168,144,2,8,112,168,2,8,64,136,2,8,88,104,2,8,64,64,2,8,136,32,4,15,152,64,255
        .byte 1,6,8,168,2,8,80,168,2,8,104,128,2,8,88,80,2,9,152,40,2,9,152,128,255
        .byte 0,0,8,80,2,12,168,16,2,12,128,80,2,12,96,152,255
        .byte 0,0,8,80,2,8,64,120,2,8,96,144,2,8,88,88,2,8,80,24,2,8,48,64,2,8,56,160,255
        .byte 0,0,8,136,4,15,48,56,2,8,128,168,2,8,128,112,2,8,128,48,2,9,152,80,6,21,88,160,255
        .byte 0,0,8,40,4,15,160,40,3,10,136,168,2,14,72,96,2,14,104,96,7,7,160,136,2,14,24,8,2,14,24,168,255
        .byte 1,6,8,168,4,17,144,16,4,17,144,136,255
        .byte 0,0,8,80,2,8,96,168,2,8,112,80,4,18,144,24,6,21,160,112,255
        .byte 0,0,8,40,4,18,80,16,4,18,64,88,4,18,96,160,4,18,112,64,4,15,160,40,4,18,128,16,255
        .byte 0,0,8,144,4,18,40,48,2,14,40,88,4,18,96,32,2,13,112,112,2,13,112,160,2,9,152,8,2,9,152,48,2,9,152,88,6,21,64,24,255
        .byte 0,0,8,88,7,7,168,88,3,10,128,32,4,15,152,104,4,15,152,56,255
        .byte 1,6,8,168,2,8,48,168,2,8,48,136,2,8,48,104,2,9,80,8,2,9,80,40,2,9,80,72,2,8,112,72,2,8,112,104,2,8,112,136,2,9,144,8,255
        .byte 0,0,8,88,4,18,64,8,4,18,80,136,4,18,96,40,4,18,120,88,4,18,136,152,4,18,48,168,4,18,152,40,4,18,104,112,255
        .byte 0,0,8,48,2,13,96,144,2,13,128,88,6,21,112,32,255
        .byte 0,0,8,88,255
        .byte 0,0,8,48,4,18,72,24,2,14,136,96,6,21,48,80,255
        .byte 0,0,8,128,4,16,56,56,4,15,96,112,4,16,136,88,255
        .byte 0,0,8,128,3,10,80,32,255
        .byte 7,7,160,88,5,0,8,88,6,19,72,16,6,19,48,136,6,19,104,56,6,19,120,160,7,20,144,32,7,20,144,56,7,20,144,128,7,20,144,152,255
.out .sprintf("POSITIONS SIZE = %d", (* - nmedat))

NUMOBJ = 1
objdta:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,254,56,120,254,56,120
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
        .byte 255,129,173,173,161,189,129,255
        .byte 0,0,255,170,0,255,0,0
        .byte 36,44,36,44,36,44,36,44
        .byte 0,0,6,12,24,48,96,0
        .byte 0,108,206,206,230,230,108,0
        .byte 0,40,104,8,8,8,110,0
        .byte 0,108,198,6,108,192,238,0
        .byte 0,108,198,20,6,198,108,0
        .byte 0,24,24,88,216,222,24,0
        .byte 0,238,192,236,6,198,108,0
        .byte 0,108,192,236,198,198,108,0
        .byte 0,238,6,12,8,32,32,0
        .byte 0,108,198,108,198,198,108,0
        .byte 0,108,198,198,110,6,108,0
        .byte 0,0,0,48,0,0,48,0
        .byte 0,0,48,0,0,48,48,96
        .byte 0,0,12,24,48,24,12,0
        .byte 0,0,0,126,0,126,0,0
        .byte 0,0,48,24,12,24,48,0
        .byte 0,124,198,12,24,0,24,0
        .byte 0,0,0,24,24,0,0,0
        .byte 0,108,198,198,238,198,198,0
        .byte 0,236,198,236,198,198,236,0
        .byte 0,108,198,192,192,198,108,0
        .byte 0,216,204,198,198,204,216,0
        .byte 0,222,192,220,192,192,222,0
        .byte 0,222,192,220,192,192,192,0
        .byte 0,92,198,192,222,198,92,0
        .byte 0,198,198,238,198,198,198,0
        .byte 0,110,8,8,8,8,110,0
        .byte 0,6,6,6,198,198,108,0
        .byte 0,204,200,224,200,204,198,0
        .byte 0,192,192,192,192,192,222,0
        .byte 0,198,238,238,198,198,198,0
        .byte 0,198,230,230,206,206,198,0
        .byte 0,108,198,198,198,198,108,0
        .byte 0,236,198,198,236,192,192,0
        .byte 0,108,198,198,230,206,108,0
        .byte 0,236,198,198,236,204,198,0
        .byte 0,108,192,108,6,198,108,0
        .byte 0,238,40,40,40,40,40,0
        .byte 0,198,198,198,198,198,108,0
        .byte 0,198,198,198,198,108,40,0
        .byte 0,198,198,198,198,238,108,0
        .byte 0,198,108,40,40,108,198,0
        .byte 0,134,196,104,40,40,40,0
        .byte 0,238,12,8,32,96,238,0
        .byte 0,30,24,24,24,24,30,0
        .byte 0,0,192,96,48,24,12,0
        .byte 0,240,48,48,48,48,240,0
        .byte 0,48,120,252,48,48,48,0
        .byte 0,0,0,0,0,0,0,255
        .byte 0,60,102,248,96,96,254,0
        .byte 254,254,2,2,254,192,192,192
        .byte 250,250,130,130,254,194,194,194
        .byte 254,254,130,2,254,136,206,198
        .byte 60,126,255,211,145,74,74,44
        .byte 0,224,224,128,224,192,240,0
        .byte 252,174,7,243,9,11,9,11
        .byte 255,170,0,255,0,0,0,0
        .byte 0,0,0,0,255,0,170,255
        .byte 11,9,11,9,243,5,170,252
        .byte 9,11,9,11,11,9,11,9
        .byte 254,254,0,128,128,128,128,254
        .byte 134,134,130,250,250,130,194,194
        .byte 194,194,194,130,130,2,254,254
        .byte 254,254,0,16,16,48,48,48
        .byte 254,254,0,128,248,128,254,254
        .byte 63,106,192,143,208,144,208,144
        .byte 144,208,144,208,143,192,106,63
        .byte 208,144,208,144,208,208,144,208
        .byte 0,246,208,214,150,146,146,0
        .byte 0,234,138,138,170,238,238,0
        .byte 0,238,132,196,132,228,228,0
        .byte 0,232,136,200,136,238,238,0
        .byte 0,224,160,160,160,224,224,0
        .byte 0,16,0,32,0,16,0,32
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
