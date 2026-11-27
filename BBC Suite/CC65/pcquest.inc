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
        .byte 255,255,255,255,255,27,28,29,255,255,255,255,255,255,255,255,255,255,26,255,255,255,255,255,255,255,255,0,1,2,3,4,5,6,7,8,9,10,255,255,25,24,23,22,21,255,255,255,255,255,11,255,255,255,255,255,255,14,20,255,255,15,13,12,255,255,30,255,255,255,255,19,18,17,16,255,255,255
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255
stmap:  .byte 27

evnt00:
        lda #14
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp a00045
        jsr scru
        lda #128
        ldy #8
        sta (z80_ix),y
        rts
a00045: lda #144
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp a00088
        jsr scrd
        lda #16
        ldy #8
        sta (z80_ix),y
        rts
a00088: lda #8
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp a00131
        jsr scrl
        lda #228
        ldy #9
        sta (z80_ix),y
        rts
a00131: lda #230
        ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp a00174
        jsr scrr
        lda #10
        ldy #9
        sta (z80_ix),y
        rts
a00174: jsr skobj
        sta varobj
        lda #255
        cmp varobj
        beq *+5
        jmp a00206
        jmp a00997
a00206: lda #61
        cmp varobj
        beq *+5
        jmp a00321
        lda varobj
        jsr getob
        lda #100
        sta loopa
a00242: ; lda #100
        jsr explod
        lda #<2
        sta z80_l
        lda #>2
        sta z80_h
        jsr addsc
        lda #21
        sta chary
        lda #24
        sta charx
        jsr dscor
        dec loopa
        beq :+
        jmp a00242
:
        lda #1
        sta gamwon
        jmp a00997
a00321: lda #57
        cmp varobj
        beq *+5
        jmp a00469
        lda numlif
        clc
        adc #1
        sta numlif
        lda varobj
        jsr getob
        lda #30
        jsr explod
        lda #25
        sta loopa
a00381: lda #<10
        sta z80_l
        lda #>10
        sta z80_h
        jsr addsc
        lda #21
        sta chary
        lda #24
        sta charx
        jsr dscor
        dec loopa
        beq :+
        jmp a00381
:
        lda #21
        sta chary
        lda #8
        sta charx
        lda numlif
        jsr disply
        jmp a00997
a00469: lda #47
        cmp varobj
        beq *+5
        jmp a00617
        lda numlif
        clc
        adc #1
        sta numlif
        lda varobj
        jsr getob
        lda #30
        jsr explod
        lda #25
        sta loopa
a00529: lda #<10
        sta z80_l
        lda #>10
        sta z80_h
        jsr addsc
        lda #21
        sta chary
        lda #24
        sta charx
        jsr dscor
        dec loopa
        beq :+
        jmp a00529
:
        lda #21
        sta chary
        lda #8
        sta charx
        lda numlif
        jsr disply
        jmp a00997
a00617: lda #69
        cmp varobj
        beq *+5
        jmp a00766
        lda numlif
        clc
        adc #1
        sta numlif
        lda varobj
        jsr getob
        lda #30
        jsr explod
        lda #25
        sta loopa
a00678: lda #<10
        sta z80_l
        lda #>10
        sta z80_h
        jsr addsc
        lda #21
        sta chary
        lda #24
        sta charx
        jsr dscor
        dec loopa
        beq :+
        jmp a00678
:
        lda #21
        sta chary
        lda #8
        sta charx
        lda numlif
        jsr disply
        jmp a00997
a00766: lda #90
        cmp varobj
        beq *+5
        jmp a00914
        lda numlif
        clc
        adc #1
        sta numlif
        lda varobj
        jsr getob
        lda #30
        jsr explod
        lda #25
        sta loopa
a00826: lda #<10
        sta z80_l
        lda #>10
        sta z80_h
        jsr addsc
        lda #21
        sta chary
        lda #24
        sta charx
        jsr dscor
        dec loopa
        beq :+
        jmp a00826
:
        lda #21
        sta chary
        lda #8
        sta charx
        lda numlif
        jsr disply
        jmp a00997
a00914: lda varobj
        jsr getob
        lda #12
        jsr explod
        lda #50
        sta loopa
a00940: lda #<2
        sta z80_l
        lda #>2
        sta z80_h
        jsr addsc
        lda #21
        sta chary
        lda #24
        sta charx
        jsr dscor
        dec loopa
        beq :+
        jmp a00940
:
a00997: lda joyval
        and #1
        beq :+
        jmp a01111
:
        jsr cangr
        beq :+
        jmp a01111
:
        lda #1
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp a01089
        ; lda #0
        jsr animsp
        lda #3
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a01089
a01089: ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a01111: lda joyval
        and #2
        beq :+
        jmp a01225
:
        jsr cangl
        beq :+
        jmp a01225
:
        lda #0
        ldy #6
        sta (z80_ix),y
        ; lda #0
        cmp vara
        beq *+5
        jmp a01203
        ; lda #0
        jsr animsp
        lda #3
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a01203
a01203: ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a01225: lda joyval
        and #16
        beq :+
        jmp a01247
:
        jsr jump
a01247: jsr cangd
        beq :+
        jmp a01265
:
        jsr ifall
a01265: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01301
:
        lda #1
        sta deadf
a01301: jmp grav
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
        cmp vara
        beq *+5
        jmp b00162
        ; lda #0
        jsr animsp
b00162: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00192
:
        lda #1
        sta deadf
b00192: rts
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
c00333: lda #1
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp c00386
        lda #0
        cmp vara
        beq *+5
        jmp c00381
        ; lda #0
        jsr animsp
c00381: jmp c00412
c00386: lda #0
        cmp vara
        beq *+5
        jmp c00412
        ; lda #0
        jsr animbk
c00412: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00442
:
        lda #1
        sta deadf
c00442: rts
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
        cmp vara
        beq *+5
        jmp d00296
        ; lda #0
        jsr animsp
d00296: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00326
:
        lda #1
        sta deadf
d00326: rts
.out .sprintf("EVENT 3 SIZE = %d", (* - evnt03))

evnt04:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp e00082
        jsr cangu
        beq :+
        jmp e00064
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp e00078
e00064: lda #1
        ldy #11
        sta (z80_ix),y
e00078: jmp e00136
e00082: jsr cangd
        beq :+
        jmp e00123
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp e00136
e00123: lda #0
        ldy #11
        sta (z80_ix),y
e00136: lda #0
        jsr animsp
        rts
.out .sprintf("EVENT 4 SIZE = %d", (* - evnt04))

evnt05:
        rts
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
        rts
.out .sprintf("EVENT 9 SIZE = %d", (* - evnt09))

evnt10:
        lda #3
        cmp vara
        beq *+5
        jmp k00031
        lda #0
        sta vara
        jmp k00046
k00031: lda vara
        clc
        adc #1
        sta vara
k00046: rts
.out .sprintf("EVENT 10 SIZE = %d", (* - evnt10))

evnt11:
        rts
.out .sprintf("EVENT 11 SIZE = %d", (* - evnt11))

evnt12:
        jsr cls
        lda #0
        sta chary
        lda #3
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        ; lda #1
        sta chary
        lda #3
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda #2
        sta chary
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda #3
        sta chary
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda #2
        sta chary
        lda #26
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        ; lda #1
        sta chary
        lda #27
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #1
        jsr pattr
        lda #2
        sta chary
        lda #8
        sta charx
        lda #34
        jsr dmsg
        lda #5
        sta charx
        lda #6
        sta chary
        lda #35
        jsr dmsg
        lda #5
        sta charx
        lda #8
        sta chary
        lda #36
        jsr dmsg
        lda #5
        sta charx
        lda #10
        sta chary
        lda #37
        jsr dmsg
        lda #13
        sta chary
        lda #0
        sta charx
        lda #38
        jsr dmsg
        lda #21
        sta chary
        lda #0
        sta charx
        lda #39
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
        lda #21
        sta chary
        lda #2
        sta charx
        lda #1
        jsr dmsg
        lda #21
        sta chary
        lda #8
        sta charx
        lda numlif
        jsr disply
        lda #21
        sta chary
        lda #18
        sta charx
        lda #2
        jsr dmsg
        lda #21
        sta chary
        lda #24
        sta charx
        jsr dscor
        rts
.out .sprintf("EVENT 13 SIZE = %d", (* - evnt13))

evnt14:
        lda #0
        sta charx
        ; lda #0
        sta chary
        lda scno
        sta varm
        lda #4
        sta z80_c
        lda varm
        clc
        adc z80_c
        sta varm
        lda varm
        jsr dmsg
        rts
.out .sprintf("EVENT 14 SIZE = %d", (* - evnt14))

evnt15:
        lda #0
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp p00024
p00024: rts
.out .sprintf("EVENT 15 SIZE = %d", (* - evnt15))

evnt16:
        lda numlif
        sec
        sbc #1
        sta numlif
        lda #21
        sta chary
        lda #8
        sta charx
        lda numlif
        jsr disply
        lda #0
        cmp numlif
        beq *+5
        jmp q00175
        lda #100
        jsr delay
        jsr cls
        lda #6
        sta chary
        lda #11
        sta charx
        lda #0
        jsr dmsg
        lda #10
        sta chary
        lda #8
        sta charx
        lda #2
        jsr dmsg
        lda #10
        sta chary
        lda #17
        sta charx
        jsr dscor
        lda #3
        sta charx
        lda #21
        sta chary
        lda #40
        jsr dmsg
        jsr prskey
q00175: rts
.out .sprintf("EVENT 16 SIZE = %d", (* - evnt16))

evnt17:
        lda #50
        jsr delay
        rts
.out .sprintf("EVENT 17 SIZE = %d", (* - evnt17))

evnt18:
        lda #150
        jsr delay
        jsr cls
        lda #6
        sta chary
        lda #0
        sta charx
        lda #3
        jsr dmsg
        lda #8
        sta charx
        lda #12
        sta chary
        lda #2
        jsr dmsg
        lda #19
        sta charx
        lda #12
        sta chary
        jsr dscor
        lda #18
        sta chary
        lda #2
        sta charx
        lda #40
        jsr dmsg
        lda #50
        sta loopa
s00119: dec loopa
        beq :+
        jmp s00119
:
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
        .byte "LIVES",141
        .byte "SCORE",141
        .byte "The aliens are a little confused",13
        .byte " but decide it might be best if",13
        .byte "  they find another castle to",13
        .byte "      destroy and leave!",141
        .byte " ........The Main Hall.........",141
        .byte " ........The Court Yard........",141
        .byte " .........Ethan",39
        .byte "s Keep.........",141
        .byte " .........Atop The Wall........",13
        .byte 141
        .byte " ........The Gate House........",13
        .byte 141
        .byte " ........The Main Gate.........",13
        .byte 141
        .byte " ........Native Temple.........",13
        .byte 141
        .byte " ......The Cave Enterance......",141
        .byte " .........Into The Dank........",141
        .byte " ........Luke",39
        .byte "s Cavern.........",141
        .byte " .....The Only Way Is Down.....",141
        .byte " .......Miner",39
        .byte "s Delight........",13
        .byte 141
        .byte " .........Dug Too Deep.........",141
        .byte " .......Cavandale Cavern.......",141
        .byte " ......Castle Foundations......",141
        .byte " .....How Deep Does It Go......",13
        .byte 141
        .byte " .....Under The Underwurlde....",141
        .byte " .......None Shall Pass........",141
        .byte " ..........Hade",39
        .byte "s Hall.........",141
        .byte " ......Belzebubs Bathroom......",141
        .byte " .......Hell Hath No Fury......",141
        .byte " .......Into The Dungeons......",141
        .byte " .......Cell Block Ouch........",141
        .byte " .......Torture Chamber........",141
        .byte " ...By The Short And Curlies...",13
        .byte 141
        .byte " ..........A Way Out...........",141
        .byte " ........The Bergfried.........",141
        .byte " ...All Along The Watchtower...",141
        .byte " ...You Cant Park That There...",13
        .byte 141
        .byte " ......Destroy All Humans......",141
        .byte "Select To Start",141
        .byte "1............KEYBOARD",141
        .byte "2.............KEMPSON",141
        .byte "3...SINCLAIR JOYSTICK",141
        .byte "Page has returned to the castle",13
        .byte " to find a crashed alien space",13
        .byte " ship and monsters everywhere.",13
        .byte "Treasure is scattered throughout",13
        .byte "    the castle and beyond.",13
        .byte "Guide our hero through 30 levels",13
        .byte "and recover the valuables.......",13
        .byte 141
        .byte " Q-Left   W-Right   SPACE-Jump",141
        .byte "Press any key to try again",141
nummsg:
        .byte 41
.out .sprintf("MESSAGE SIZE = %d", (* - msgdat))

chgfx:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,141,174,93,95,141,175,0,0,141,137,95,27,39,171
        .byte 0,0,255,119,255,119,0,0,170,170,85,85,170,170,85,85
        .byte 0,0,240,240,0,0,208,176,176,208,0,0,0,0,0,0
        .byte 0,0,141,175,93,27,137,5,68,8,136,1,0,8,0,0
        .byte 32,4,80,10,37,164,64,2,37,32,16,2,1,32,2,64
        .byte 0,0,8,0,0,2,1,0,4,4,10,10,1,1,4,2
        .byte 0,0,82,10,133,37,74,80,33,160,72,88,33,164,24,24
        .byte 4,32,2,64,4,32,10,64,4,4,64,2,160,5,64,2
        .byte 4,0,2,0,1,0,2,0,5,0,2,0,0,0,0,0
        .byte 0,0,186,186,68,68,34,34,85,17,34,34,0,85,0,34
        .byte 0,0,88,26,160,1,8,0,4,36,90,80,160,129,80,74
        .byte 177,117,250,98,228,117,0,0,68,228,216,234,213,228,80,200
        .byte 153,153,17,153,153,136,221,221,153,153,153,187,17,187,136,153
        .byte 0,117,0,250,17,128,50,114,100,245,98,0,228,117,234,250
        .byte 228,0,250,0,145,128,98,200,213,160,170,234,68,177,170,250
        .byte 0,0,90,66,4,129,2,2,5,4,2,2,0,5,0,2
        .byte 32,64,80,160,112,224,64,32,112,32,16,32,16,32,32,64
        .byte 17,8,34,4,70,70,171,137,93,25,38,174,19,68,1,136
        .byte 16,128,32,64,96,96,144,208,144,176,112,96,32,192,16,128
        .byte 7,3,15,11,11,1,1,0,8,5,14,15,14,7,7,2
        .byte 0,11,3,11,7,1,5,0,0,5,6,15,14,7,7,2
        .byte 6,10,6,2,2,2,2,0,2,2,0,0,2,0,0,0
        .byte 15,15,13,11,4,10,6,2,6,2,4,2,6,0,4,2
        .byte 12,0,15,8,15,2,4,7,0,15,6,15,14,7,7,0
        .byte 85,10,136,137,68,10,5,34,68,8,136,170,85,2,4,137
        .byte 16,0,48,0,64,128,96,192,32,64,64,128,48,128,0,0
        .byte 68,68,34,136,68,68,50,136,32,128,16,0,17,0,0,0
        .byte 160,96,144,176,96,64,80,144,176,96,96,48,160,224,80,32
        .byte 160,144,208,80,64,160,112,32,32,0,32,0,0,0,0,0
        .byte 96,192,80,96,32,64,48,160,80,96,32,64,96,160,80,224
        .byte 0,0,16,64,32,68,50,170,68,228,34,64,32,160,80,200
        .byte 68,228,50,64,68,160,34,200,85,160,34,64,32,128,0,0
        .byte 0,96,16,176,96,64,80,144,176,96,96,48,160,224,80,32
        .byte 96,0,144,0,96,224,80,144,176,96,96,48,160,224,80,32
        .byte 0,0,0,0,170,170,0,0,170,170,0,0,0,0,0,0
        .byte 102,170,68,68,102,68,68,68,102,170,0,0,0,0,0,0
        .byte 238,238,68,68,68,68,68,68,238,68,0,0,0,0,0,0
        .byte 68,68,0,0,68,68,0,0,68,68,0,0,68,68,0,0
        .byte 0,68,68,0,0,68,68,238,0,170,68,68,0,0,68,68
        .byte 34,0,68,0,187,204,34,170,153,68,68,0,85,68,68,153
        .byte 68,153,51,85,34,34,17,68,0,0,17,204,0,0,17,68
        .byte 17,68,0,68,17,0,17,34,34,34,0,68,0,0,0,0
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 15,15,203,15,245,15,250,143,245,199,250,203,245,229,250,235
        .byte 245,229,250,235,245,199,250,203,245,135,250,15,199,15,15,15
        .byte 125,245,122,250,61,245,62,250,31,245,15,250,15,61,15,15
        .byte 15,15,15,62,15,245,30,250,61,245,62,250,125,245,122,250
        .byte 15,15,15,143,95,15,47,15,15,15,15,15,15,15,15,15
        .byte 15,31,15,62,15,95,15,250,31,245,62,250,95,245,250,250
        .byte 245,245,186,216,245,245,250,250,245,245,250,250,177,213,250,250
        .byte 245,245,250,170,245,196,250,136,245,0,170,0,196,0,136,0
        .byte 128,0,200,0,160,0,250,0,245,128,250,200,245,160,250,250
        .byte 245,245,80,250,49,245,16,250,0,245,0,80,0,49,0,16
        .byte 245,245,250,242,244,241,248,240,240,241,248,242,244,245,250,250
        .byte 245,245,250,122,229,181,218,90,165,181,218,122,229,245,250,250
        .byte 0,0,250,250,0,0,250,250,177,213,250,250,245,245,250,250
bprop:
        .byte 0
        .byte 2
        .byte 2
        .byte 1
        .byte 0
        .byte 5
        .byte 5
        .byte 2
        .byte 0
        .byte 0
        .byte 1
        .byte 2
        .byte 2
        .byte 0
        .byte 2
        .byte 2
        .byte 1
        .byte 5
        .byte 5
        .byte 5
        .byte 2
        .byte 2
        .byte 0
        .byte 1
        .byte 2
        .byte 0
        .byte 5
        .byte 5
        .byte 2
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 1
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
        .byte 2
        .byte 2
        .byte 0
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 2
.out .sprintf("BLOCKS SIZE = %d", (* - chgfx))

sprgfx:
        .byte 1,128,3,192,6,192,3,224,1,128,19,64,19,64,23,160,27,144,19,208,19,192,2,64,1,128,0,0,3,128,7,128
        .byte 1,128,3,192,6,192,3,224,1,128,19,64,19,64,23,176,27,144,19,200,19,192,2,64,21,224,26,0,12,112,0,224
        .byte 1,128,3,192,6,192,3,224,1,128,19,64,19,96,23,144,27,144,19,224,19,192,2,176,13,176,2,48,28,16,60,0
        .byte 1,128,3,192,6,192,3,224,1,128,19,64,19,96,23,144,27,144,19,208,19,192,2,64,1,184,1,24,14,16,30,0
        .byte 1,128,3,192,3,96,7,192,1,128,2,200,2,200,5,232,9,216,11,200,3,200,2,64,1,128,0,0,1,192,1,224
        .byte 1,128,3,192,3,96,7,192,1,128,2,200,2,200,13,232,9,216,19,200,3,200,2,64,7,168,0,88,14,48,7,0
        .byte 1,128,3,192,3,96,7,192,1,128,2,200,6,200,9,232,9,216,7,200,3,200,13,64,13,176,12,64,8,56,0,60
        .byte 1,128,3,192,3,96,7,192,1,128,2,200,6,200,9,232,9,216,11,200,3,200,2,64,29,128,24,128,8,112,0,120
        .byte 6,96,13,176,26,88,49,140,250,95,204,51,196,35,35,196,18,72,4,32,28,56,10,80,3,192,0,0,0,0,0,0
        .byte 7,224,12,48,26,88,113,142,250,95,140,49,68,34,35,196,19,200,4,32,14,112,54,108,19,200,1,128,0,0,0,0
        .byte 3,192,12,48,26,88,113,142,218,91,142,113,100,38,17,136,18,72,4,32,14,112,22,104,99,198,33,132,0,0,0,0
        .byte 7,224,12,48,26,88,113,142,250,95,140,49,68,34,35,196,19,200,4,32,14,112,54,108,19,200,1,128,0,0,0,0
        .byte 0,0,0,0,1,0,3,128,23,208,63,248,4,64,23,208,32,8,32,8,63,248,85,84,0,0,114,156,83,148,0,0
        .byte 0,0,0,0,0,128,1,192,11,232,31,252,2,96,19,240,32,8,32,8,63,248,85,84,0,0,83,148,114,156,0,0
        .byte 0,0,0,0,1,0,3,128,23,208,63,248,4,64,23,208,32,8,32,8,63,248,85,84,0,0,114,156,83,148,0,0
        .byte 0,0,0,0,2,0,7,0,47,160,127,240,8,128,31,144,32,8,32,8,63,248,85,84,0,0,83,148,114,156,0,0
        .byte 0,0,2,64,2,64,37,164,116,46,218,91,141,177,198,99,117,174,58,92,53,172,98,70,206,115,89,154,49,140,0,0
        .byte 2,64,2,64,37,164,52,44,74,82,109,182,70,98,37,164,26,88,37,164,50,76,34,68,21,168,9,144,0,0,0,0
        .byte 0,0,2,64,2,64,5,160,20,40,42,84,37,164,22,104,13,176,18,72,13,176,18,72,22,104,9,144,1,128,0,0
        .byte 2,64,2,64,37,164,52,44,74,82,109,182,70,98,37,164,26,88,37,164,50,76,34,68,21,168,9,144,0,0,0,0
        .byte 0,0,0,0,0,2,0,3,0,20,0,40,0,52,64,88,96,160,33,64,97,192,80,160,40,80,28,112,10,160,5,64
        .byte 0,0,0,20,0,8,0,8,0,28,0,34,0,54,128,28,96,20,96,28,56,20,40,40,20,112,14,160,5,64,2,128
        .byte 2,0,6,192,1,32,2,192,2,224,1,208,0,72,32,52,96,20,32,28,96,20,84,56,43,80,26,160,4,64,3,128
        .byte 0,0,0,20,0,8,0,8,0,28,0,34,0,54,128,28,96,20,96,28,56,20,40,40,20,112,14,160,5,64,2,128
        .byte 3,197,4,34,10,82,4,34,97,135,83,202,41,151,20,40,21,168,10,80,15,240,66,66,182,109,159,249,158,121,112,14
        .byte 3,192,4,32,10,85,4,34,1,130,67,194,177,151,76,42,37,167,26,88,79,242,162,69,156,57,95,250,62,124,0,0
        .byte 3,192,4,32,14,112,4,37,1,130,2,66,81,130,172,55,101,170,26,95,79,242,162,69,156,57,95,250,62,124,0,0
        .byte 3,192,4,32,14,117,4,34,1,130,66,66,177,151,76,42,37,167,26,88,79,242,162,69,156,57,95,250,62,124,0,0
        .byte 32,4,32,4,49,140,30,120,25,152,36,36,42,84,68,34,65,130,40,20,32,4,17,136,10,80,4,32,3,192,1,128
        .byte 32,4,32,4,49,140,30,120,25,152,36,36,42,84,68,34,69,162,40,20,16,8,9,144,6,96,2,64,3,192,1,128
        .byte 32,4,32,4,51,204,28,56,25,152,36,36,74,82,78,114,37,164,40,20,17,136,10,80,6,96,2,64,3,192,1,128
        .byte 32,4,32,4,49,140,30,120,25,152,36,36,42,84,68,34,69,162,40,20,16,8,9,144,6,96,2,64,3,192,1,128
        .byte 0,0,255,255,128,1,182,109,128,1,128,1,248,31,127,254,20,40,31,248,8,16,15,240,4,32,1,0,8,32,0,128
        .byte 0,0,255,255,128,1,236,217,128,1,128,1,248,31,127,254,20,40,31,248,8,16,15,240,1,0,4,144,0,0,10,64
        .byte 0,0,255,255,128,1,155,55,128,1,128,1,248,31,127,254,20,40,31,248,8,16,15,240,8,0,2,16,0,64,5,16
frmlst:
        .byte 0,4
        .byte 4,4
        .byte 8,4
        .byte 12,4
        .byte 16,4
        .byte 20,4
        .byte 24,4
        .byte 28,4
        .byte 32,3,35,0
.out .sprintf("SPRITES SIZE = %d", (* - sprgfx))

scdat:
        .word 295,258,232,304,282,379,412,460,325,349,308,311,315,315,308,280,325,318,271,382,287,246,350,276,303,195
        .word 309,303,261,304,451
        .byte 1,1,1,4,1,1,1,4,1,1,4,1,1,1,4,255,1,7,4,1,4,255,1,7,4,0,0,1,1,255,0,4,4
        .byte 1,4,0,0,0,4,4,4,0,0,0,4,255,0,4,4,1,1,26,0,0,0,4,4,255,0,5,4,255,0,16,4,1
        .byte 27,255,0,28,1,255,0,29,1,255,0,26,2,2,2,1,255,0,26,1,4,1,1,0,3,0,0,1,1,1,0,0,255,1,15
        .byte 0,0,1,0,1,1,255,0,4,1,1,1,0,0,1,4,255,1,7,4,1,1,1,4,1,0,0,255,1,4,0,3,0,0
        .byte 4,4,1,0,0,1,0,1,1,4,4,4,1,1,0,1,26,1,0,1,0,0,0,4,1,1,255,0,6,4,0,0,4
        .byte 1,1,4,0,0,0,4,1,1,1,27,1,1,4,0,0,0,26,1,1,1,255,0,9,4,1,255,0,5,4,4,1,1
        .byte 4,4,255,0,4,27,255,1,4,255,0,9,4,255,0,7,4,4,255,0,7,255,1,5,3,3,255,0,18,255,2,5,1,1
        .byte 4,36,37,1,255,0,19,1,1,4,4,1,1,1,0,0,0,4,1,1,255,0,18,1,13,13,1,1,1,255,0,6,1
        .byte 255,0,7,5,255,0,9,1,13,13,1,1,1,255,0,6,1,1,255,2,21,1,0,0,255,1,27
        .byte 255,1,31,4,255,0,6,4,4,255,0,10,4,4,255,0,4,4,1,1,1,4,255,0,26,4,4,1,255,0,60,255,2,5
        .byte 0,0,2,2,255,0,11,2,2,255,0,5,3,3,2,255,1,5,0,0,255,1,15,255,0,7,1,1,4,1,26,1,0,0
        .byte 1,26,1,1,1,4,1,1,1,4,1,1,4,255,1,4,255,0,5,1,1,0,1,27,1,0,0,1,27,1,1,1,0
        .byte 1,1,1,0,1,1,0,255,1,5,0,0,3,3,1,1,4,1,1,1,0,0,1,255,4,4,1,4,4,1,1,1,4,4,4
        .byte 1,4,1,1,255,0,4,1,1,0,0,4,1,0,0,4,255,0,4,4,0,0,4,4,4,0,0,0,4,0,4,1,255,0,4
        .byte 1,1,0,0,0,4,255,0,19,1,3,3,0,0,1,1,26,255,0,21,26,1,255,0,4,1,1,27,255,0,21,27,1
        .byte 255,0,4,1,1,255,0,23,4,0,0,3,3,1,1,255,0,28,1,1,255,0,7,2,2,255,6,5,2,2,255,0,10
        .byte 255,1,4,255,0,7,4,1,6,6,5,6,6,1,4,255,0,10,255,1,33
        .byte 255,1,18,4,1,1,4,255,1,10,4,255,0,8,4,1,1,4,0,0,0,26,1,4,255,0,7,4,1,1,4,255,0,10
        .byte 4,4,255,0,4,27,4,255,0,9,1,255,0,60,255,2,6,255,0,20,255,2,4,1,4,1,1,1,4,0,0,1,1
        .byte 255,0,16,4,4,1,1,1,0,1,26,255,0,4,4,4,255,0,14,3,3,0,0,4,1,1,4,4,27,255,0,25,1,1
        .byte 255,0,11,1,1,255,0,4,1,1,1,0,0,255,1,8,255,0,11,4,4,255,0,4,4,4,4,0,0,4,1,4,1,1
        .byte 4,1,1,255,0,23,1,0,1,1,0,1,1,3,3,3,255,0,4,26,1,1,255,0,13,4,255,1,6,255,0,7,27
        .byte 4,4,255,0,10,2,255,0,5,4,1,1,1,255,0,12,1,1,255,0,5,2,0,0,255,2,4,0,4,1,1,3,3,3
        .byte 255,0,9,4,4,255,0,6,2,255,0,7,1,1,255,0,28,1,1,255,0,8,1,1,255,0,18,255,1,31
        .byte 255,1,8,4,4,4,1,4,4,1,4,1,1,4,4,1,1,1,4,1,1,4,255,1,4,4,255,0,5,4,0,0,0
        .byte 4,255,0,12,4,4,0,0,4,1,1,255,0,28,1,255,0,60,255,2,7,255,0,21,2,2,1,4,1,4,1,4,1
        .byte 0,0,3,3,3,0,0,3,3,3,0,0,3,3,3,0,0,3,3,0,0,4,1,1,0,1,0,1,0,1,0,0,0
        .byte 5,255,0,4,5,255,0,4,5,255,0,7,26,255,1,7,4,0,0,1,4,1,0,0,1,4,1,0,0,1,4,1,255,0,6
        .byte 27,255,1,5,4,4,0,0,0,1,0,1,0,0,1,0,1,0,0,1,0,1,255,0,5,3,3,1,1,4,1,4,255,0,5
        .byte 4,1,1,0,0,1,1,1,0,0,1,1,4,255,0,7,1,1,255,0,8,26,1,1,0,0,1,1,1,0,0,1,1
        .byte 26,0,0,3,3,0,0,0,1,1,255,0,8,27,1,1,0,0,1,1,1,0,0,1,1,27,255,0,7,1,1,3,3
        .byte 255,0,6,1,4,4,0,0,1,4,4,0,0,1,1,1,255,0,5,3,3,1,1,255,0,8,4,255,0,4,4,255,0,4
        .byte 1,4,4,255,0,7,1,1,3,3,255,0,16,4,255,0,4,3,3,0,0,0,1,1,255,0,28,1,1,255,0,26,255,1,33
        .byte 255,1,20,4,1,1,0,0,4,255,1,5,4,255,0,7,4,1,1,1,4,255,1,4,4,4,0,4,255,0,6,4,1,1
        .byte 255,0,10,4,1,0,1,4,4,255,0,12,1,255,0,12,4,1,4,255,0,28,4,255,0,10,3,3,255,0,4,2,2,2
        .byte 255,0,25,2,2,255,1,4,255,0,22,2,2,1,1,1,4,1,4,1,1,255,0,18,3,3,1,1,4,1,1,0,1
        .byte 0,1,1,1,255,0,19,4,1,0,255,1,6,4,1,1,255,0,15,3,3,0,0,4,1,1,1,0,0,1,1,0,1,1,1
        .byte 0,0,5,255,0,16,4,1,1,0,0,4,4,1,1,1,4,0,0,2,2,2,255,0,5,2,2,255,0,7,26,1,1
        .byte 255,0,4,4,4,1,26,0,0,13,13,13,255,0,5,13,13,255,0,4,3,3,0,27,1,1,255,0,6,4,27,0,0
        .byte 13,13,13,255,0,5,13,13,255,0,8,1,1,255,0,10,13,13,13,255,0,5,13,13,255,0,8,1,1,0,0,255,1,6
        .byte 0,0,255,1,10,255,0,5,3,3,0,1,1,255,0,10,13,13,13,255,0,5,13,13,255,0,8,1,1,255,0,10,13,13,13
        .byte 255,0,5,13,13,255,0,8,255,1,31
        .byte 1,4,255,1,5,4,8,11,255,7,13,11,7,7,8,7,7,19,1,0,1,1,1,4,0,0,8,9,0,0,9,8,9
        .byte 255,0,4,8,0,0,0,9,8,8,0,9,8,7,1,4,4,4,255,0,4,9,255,0,4,8,255,0,5,9,255,0,5
        .byte 9,0,0,0,9,255,0,13,9,255,0,24,5,255,0,21,255,2,4,255,0,4,11,7,7,11,0,0,11,7,7,0,0
        .byte 7,7,11,255,0,4,255,2,4,26,1,1,4,255,0,4,9,8,8,9,0,0,8,9,8,0,0,8,9,8,255,0,4
        .byte 9,8,7,7,27,1,4,255,0,7,8,0,0,0,8,0,9,0,0,8,255,0,5,10,10,0,8,7,1,4,255,0,8
        .byte 8,0,0,0,9,255,0,4,8,255,0,8,8,7,1,255,0,9,9,255,0,8,8,255,0,8,9,11,1,255,0,18,9
        .byte 0,0,0,10,10,255,0,4,7,1,0,0,1,1,0,0,1,1,255,0,20,7,1,0,0,1,1,0,1,1,1,255,0,18
        .byte 10,10,7,1,4,1,4,1,4,1,4,1,10,10,255,0,18,19,1,0,1,0,1,0,1,0,1,255,0,13,5,10,10
        .byte 255,0,4,11,255,1,9,255,0,5,7,7,0,0,7,11,0,0,11,7,0,0,0,11,7,7,1,1,1,4,4,4,1,1
        .byte 4,255,6,5,9,7,6,6,18,8,6,6,7,9,6,6,6,9,11,7,1,1,1,13,13,13,1,1,6,6,5,6,6,6
        .byte 7,7,6,6,11,7,6,6,7,11,6,6,6,7,18,7,1,1,1,13,13,13,1,1,11,7,7,7,11,7,11,7,7
        .byte 11,11,7,7,11,7,7,11,7,7,11,7,7
        .byte 7,19,7,18,7,11,255,7,14,11,255,7,8,18,7,8,9,0,0,8,9,255,0,6,9,8,255,0,5,9,0,0,8
        .byte 0,9,8,8,19,7,9,9,0,0,0,8,255,0,8,8,255,0,8,8,0,0,9,8,8,7,255,0,5,9,255,0,8
        .byte 9,255,0,8,8,255,0,29,8,255,0,6,255,2,4,0,0,0,7,11,255,0,6,7,7,255,0,6,8,0,0,255,2,4
        .byte 11,0,0,8,0,0,0,8,255,0,8,8,255,0,6,9,0,0,9,8,11,7,7,0,0,8,0,0,0,8,0,0,0
        .byte 11,7,0,0,0,8,0,0,11,7,255,0,6,9,8,7,7,0,0,8,0,0,0,8,255,0,4,8,0,0,0,9,0,0
        .byte 8,255,0,8,9,11,7,0,0,8,0,0,0,9,255,0,15,7,7,255,0,4,18,18,0,0,9,255,0,5,14,12,15
        .byte 255,0,4,14,12,12,12,15,5,0,0,9,255,0,4,7,7,255,0,5,17,0,14,12,12,12,15,19,18,14,255,12,6
        .byte 15,255,0,4,7,11,7,7,0,0,0,14,255,12,12,8,8,8,9,10,12,15,255,0,4,8,7,7,0,0,0,12,8
        .byte 255,12,4,8,8,14,255,12,4,8,9,0,0,13,8,12,255,0,4,9,7,7,0,0,0,12,8,12,12,12,8,9,0
        .byte 255,12,4,8,13,0,0,0,13,8,12,10,10,0,0,0,19,7,0,0,0,13,9,8,9,13,0,0,0,13,9,8,8,8
        .byte 13,0,0,0,13,9,13,255,0,5,7,7,0,0,0,13,0,0,0,13,0,0,0,13,0,0,0,9,13,0,0,0,13
        .byte 0,13,0,0,0,10,10,7,11,0,0,0,13,0,0,0,13,0,0,0,13,255,0,4,13,0,5,0,13,0,13,255,0,5
        .byte 11,255,7,5,11,255,7,18,11,255,7,5
        .byte 11,7,7,18,7,8,7,8,7,7,7,8,11,7,8,7,8,11,7,7,8,7,7,8,11,7,8,7,19,7,7,19,7
        .byte 8,9,8,9,8,0,0,0,8,0,0,8,0,8,0,0,0,8,0,0,8,0,9,8,18,11,7,7,9,9,0,0,8
        .byte 0,8,0,0,0,8,0,0,8,0,8,0,0,0,8,0,0,8,0,0,8,9,7,7,255,0,5,9,0,8,0,0,0
        .byte 8,0,0,8,0,8,0,0,0,8,0,0,8,255,0,5,7,255,0,7,8,0,0,0,8,0,0,8,0,8,0,0,0
        .byte 8,0,0,8,255,0,5,7,255,2,4,0,0,0,7,7,7,11,7,0,0,8,0,8,0,0,0,8,0,0,8,255,0,5
        .byte 11,7,11,7,8,255,0,4,9,0,0,8,0,0,8,0,8,0,0,0,8,0,0,8,255,0,5,7,7,18,9,8,255,0,7
        .byte 9,0,0,7,7,7,0,0,0,8,0,0,8,10,10,0,0,0,11,19,8,0,8,255,0,10,8,0,9,0,0,0,8
        .byte 0,0,8,255,0,5,7,7,8,0,8,255,0,16,8,0,0,8,0,0,10,10,0,7,7,8,0,9,255,0,16,7,11
        .byte 7,7,255,0,5,7,7,255,0,19,9,0,0,8,255,0,5,16,7,255,0,4,5,255,0,15,17,0,9,10,10,0,0,0
        .byte 21,7,0,0,0,14,12,15,255,0,5,14,12,15,255,0,5,14,12,15,255,0,5,21,20,7,0,0,14,12,12,12,15
        .byte 0,0,0,14,12,12,12,15,0,0,0,14,12,12,12,15,0,0,0,21,20,20,11,0,0,13,0,0,9,13,0,0,0
        .byte 13,9,0,8,13,0,0,0,13,9,8,0,13,0,10,10,20,20,20,7,0,0,13,0,0,0,13,0,0,0,13,0,0,0
        .byte 13,0,0,0,13,0,0,0,13,0,0,0,22,0,0,7,0,0,13,0,0,0,13,0,0,0,13,0,0,0,13,0,0,0
        .byte 13,0,0,0,13,255,0,6,7,7,11,255,7,14,11,255,7,5,11,7,7,11,21,20,20
        .byte 255,20,31,22,0,0,0,22,20,20,22,255,0,5,22,255,20,5,22,255,0,5,22,255,20,4,17,255,0,4,22,22,255,0,7
        .byte 25,22,0,0,22,255,0,8,22,20,20,24,255,0,13,25,25,255,0,12,20,20,20,24,255,0,12,25,25,255,0,10,23
        .byte 255,20,6,24,0,0,0,21,20,20,24,0,0,0,25,25,255,0,10,22,22,255,20,5,22,0,0,0,22,255,20,4,24
        .byte 0,25,25,255,0,5,21,24,255,0,5,20,20,20,22,22,0,23,0,0,0,22,0,0,22,255,20,11,24,255,0,4,20,20,20
        .byte 0,0,0,22,255,0,7,22,255,20,4,22,255,20,5,22,255,0,4,255,20,4,24,255,0,11,25,25,255,0,6,22,255,0,4
        .byte 21,20,20,20,22,22,255,0,4,23,23,255,0,5,21,24,255,0,10,21,20,20,20,22,255,0,6,22,22,255,0,5,20,20
        .byte 255,0,10,255,20,4,255,0,13,21,20,20,255,0,5,20,24,0,0,0,255,20,5,24,255,0,8,255,20,4,25,25,255,0,4
        .byte 21,255,20,11,24,255,0,5,21,20,20,22,0,0,25,25,255,0,4,22,22,0,22,0,0,22,255,20,6,24,255,0,4
        .byte 20,20,20,0,0,0,25,25,255,0,11,20,20,255,0,4,22,0,0,0,21,20,22,255,0,4,25,25,21,24,255,0,18
        .byte 21,20,22,17,255,0,4,25,21,20,20,24,255,0,10,255,20,29,21
        .byte 255,20,34,22,255,0,11,25,25,22,20,22,22,255,20,5,22,23,20,20,20,22,22,255,0,12,25,25,0,20,0,0,20
        .byte 23,20,20,20,0,0,20,20,22,255,0,14,25,25,0,22,0,0,23,22,22,20,20,0,0,20,20,255,0,7,20,20,24
        .byte 255,0,5,25,25,255,0,6,20,22,0,0,0,20,20,255,0,6,20,22,255,0,7,25,25,255,0,6,22,255,0,4,20,20
        .byte 0,0,0,23,23,20,20,20,0,0,20,20,24,0,0,25,25,255,0,11,20,20,0,0,0,22,22,0,22,0,0,0,22
        .byte 0,23,24,0,25,25,255,0,8,22,23,23,20,20,24,255,0,12,22,20,25,25,20,24,255,0,10,20,23,24,255,0,12
        .byte 20,25,25,22,20,255,0,4,20,24,255,0,4,20,22,23,20,20,24,255,0,9,20,25,25,0,22,24,0,0,0,22,23
        .byte 0,255,20,4,0,22,20,20,20,24,255,0,8,20,25,25,0,0,22,24,255,0,5,23,23,20,20,0,0,0,23,22,20,20
        .byte 23,23,255,0,5,20,25,25,0,0,0,22,255,0,5,22,22,20,20,0,0,0,22,0,0,0,22,255,0,6,22,25,25
        .byte 255,0,11,20,20,255,0,10,21,20,24,0,20,20,20,23,24,0,0,0,23,23,255,0,4,20,20,255,0,9,20,20,22,22
        .byte 20,22,25,25,22,22,0,0,0,22,22,255,0,4,20,255,0,8,21,20,20,22,255,0,4,25,25,255,0,11,20,255,0,6
        .byte 21,255,20,4,255,0,5,25,25,255,0,10,255,20,32
        .byte 255,20,32,22,255,0,4,22,255,0,4,22,23,22,255,0,4,22,0,0,22,255,20,8,22,255,0,21,22,20,23,255,20,5
        .byte 24,255,0,22,20,0,20,23,22,255,20,4,24,255,0,20,22,0,22,0,0,20,20,23,22,20,0,0,0,23,23,255,0,7
        .byte 21,20,24,255,0,8,17,21,20,20,0,0,22,255,0,8,21,20,20,20,25,25,20,24,255,0,7,20,20,20,22,255,0,10
        .byte 21,20,20,20,22,25,25,22,20,24,255,0,6,22,20,20,255,0,10,21,20,20,20,22,0,25,25,0,22,20,255,0,8
        .byte 20,255,0,8,21,255,20,5,0,0,25,25,255,0,11,255,20,5,24,0,0,0,22,255,0,4,23,0,0,25,25,0,0,0
        .byte 20,20,24,255,0,4,23,20,20,20,22,0,22,255,0,11,25,25,0,20,20,22,22,20,24,255,0,4,20,20,20,255,0,14
        .byte 25,25,0,23,22,0,0,22,20,23,0,0,0,20,20,20,24,255,20,13,25,25,255,0,6,22,255,0,4,255,20,4,22
        .byte 0,22,20,20,22,255,0,4,22,20,20,25,25,255,0,11,20,20,20,22,255,0,4,22,255,0,7,23,25,25,23,23,255,0,8
        .byte 21,20,20,20,24,17,255,0,12,25,25,255,0,9,21,255,20,5,24,255,0,12,25,25,255,0,7,21,255,20,8,24,0,0
        .byte 255,20,23
        .byte 255,20,5,0,0,255,20,24,23,22,255,0,9,22,22,20,20,25,25,20,20,20,22,255,0,6,22,20,20,255,0,13,22
        .byte 20,25,25,20,20,22,255,0,8,20,20,255,0,14,22,25,25,20,22,255,0,9,20,20,255,0,4,21,24,21,20,24,255,0,6
        .byte 25,25,23,255,0,7,21,255,20,4,255,0,4,22,22,20,23,22,23,23,23,0,0,0,25,25,255,0,5,21,255,20,4
        .byte 22,20,20,255,0,6,23,255,0,8,25,25,255,0,5,22,22,23,255,20,5,24,255,0,11,23,23,23,25,25,255,0,8
        .byte 23,22,22,20,20,20,255,0,14,25,25,23,23,23,255,0,8,255,20,6,24,255,0,10,25,25,255,0,11,255,20,5,23
        .byte 22,255,0,10,25,25,255,0,11,20,20,22,255,0,8,21,20,24,17,21,20,25,25,0,0,0,23,23,23,255,0,5,20,20
        .byte 255,0,9,255,20,6,25,25,255,0,11,20,20,255,0,9,22,0,23,20,23,22,25,25,255,0,6,23,23,23,0,0,20,20
        .byte 24,255,0,11,22,0,0,25,25,23,23,23,255,0,8,20,20,20,24,255,0,13,25,25,255,0,11,20,20,22,20,255,0,13
        .byte 25,25,255,0,4,21,24,255,0,4,21,255,20,4,24,255,0,4,21,24,255,0,6,25,25,0,0,0,21,20,22,24,17
        .byte 21,255,20,8,0,0,255,20,23
        .byte 255,20,5,0,0,255,20,26,22,0,0,0,22,0,22,20,22,0,22,20,20,22,0,0,22,20,20,22,0,22,255,20,7
        .byte 22,255,0,7,20,0,0,0,22,20,255,0,4,22,20,0,0,0,22,255,20,5,22,255,0,8,22,255,0,4,22,255,0,5
        .byte 20,255,0,4,22,25,0,20,20,0,0,0,23,23,23,255,0,14,22,255,0,5,25,0,20,20,255,0,26,25,25,20,20
        .byte 24,255,0,25,25,25,20,20,20,24,255,0,24,25,21,255,20,4,24,255,0,23,25,22,20,20,20,25,25,255,0,21,23,23
        .byte 25,25,20,20,22,25,25,21,20,24,0,0,21,20,24,0,0,21,20,24,0,0,21,20,24,255,0,5,25,25,20,22,0
        .byte 25,25,20,0,20,0,0,20,0,20,0,0,20,0,20,0,0,20,0,20,255,0,5,25,25,20,0,0,25,25,20,26,20
        .byte 0,0,20,26,20,0,0,20,26,20,0,0,20,26,20,0,0,23,0,0,25,25,20,0,0,25,25,20,27,20,0,0,20
        .byte 27,20,0,0,20,27,20,0,0,20,27,20,255,0,5,25,25,20,23,23,25,25,22,0,22,0,0,22,0,22,0,0,22
        .byte 0,22,0,0,22,0,22,255,0,5,25,25,20,20,0,25,25,0,0,255,6,19,0,23,25,25,20,20,0,25,25,255,0,23
        .byte 25,25,20,20,20,24,25,255,0,23,25,21,255,20,31
        .byte 255,20,31,22,20,22,22,20,20,20,22,22,20,22,20,20,22,20,22,20,22,20,20,20,22,255,20,4,22,22,20,20,0
        .byte 22,0,0,22,20,20,0,0,20,0,22,20,0,22,0,20,0,22,20,20,0,20,22,20,22,0,0,20,20,255,0,5,22
        .byte 20,0,0,22,0,0,20,0,0,0,22,0,0,20,22,0,20,0,22,0,0,0,20,20,255,0,6,20,255,0,5,20,255,0,6
        .byte 20,0,0,20,255,0,5,20,20,255,0,6,20,255,0,5,22,255,0,6,20,0,0,20,255,0,5,20,20,255,0,6,20
        .byte 255,0,12,20,0,0,20,0,0,0,23,23,20,20,255,0,6,20,255,0,7,23,23,0,0,0,20,0,0,22,255,0,5
        .byte 20,20,26,255,0,5,22,255,0,12,22,255,0,7,26,20,20,27,255,0,10,23,23,255,0,11,23,23,0,27,20,255,0,29
        .byte 20,255,0,9,23,23,255,0,11,23,23,255,0,5,22,255,20,4,255,0,26,20,20,20,255,0,27,20,20,20,255,0,4
        .byte 255,20,4,0,0,255,20,4,0,0,255,20,4,0,0,255,20,7,255,0,6,20,20,255,0,4,20,20,255,0,4,20,20
        .byte 255,0,4,255,20,5,255,0,7,20,20,255,0,4,20,20,255,0,4,20,20,255,0,4,255,20,5,255,6,7,20,20,255,6,4
        .byte 20,20,255,6,4,20,20,255,6,4,255,20,34
        .byte 1,1,1,0,0,4,255,1,26,4,0,0,0,4,1,21,20,20,22,20,24,1,1,21,20,22,22,20,24,1,1,21,20
        .byte 22,24,1,1,1,255,0,6,1,20,22,20,0,22,20,1,1,20,22,0,0,22,20,1,1,20,22,0,20,1,1,20,255,0,6
        .byte 20,20,0,20,0,0,20,4,20,20,255,0,4,20,20,4,0,0,0,22,20,20,20,0,0,23,23,0,0,20,20,26,22
        .byte 0,0,20,0,22,20,255,0,4,22,20,255,0,5,22,20,20,23,23,255,0,4,20,20,27,0,0,0,22,0,0,22,255,0,5
        .byte 22,255,0,6,20,20,255,0,6,21,22,255,0,19,26,20,20,255,0,5,23,22,255,0,20,27,20,24,24,255,0,27,20,20,20
        .byte 24,255,0,26,20,20,22,20,20,24,255,0,25,20,20,21,20,20,255,0,25,20,20,20,22,22,20,24,255,0,6,21,24
        .byte 255,0,6,21,24,0,0,0,33,28,28,34,20,22,22,0,0,22,23,20,24,0,17,0,21,0,0,24,0,0,17,0,21
        .byte 20,22,0,0,0,29,0,29,30,20,255,0,6,23,22,255,20,4,0,0,255,20,5,22,255,0,8,30,20,255,0,8,22
        .byte 0,22,22,0,0,22,0,22,0,22,255,0,4,23,23,0,0,0,30,20,255,0,27,31,30,20,255,0,26,31,30,30,255,20,27
        .byte 30,30,30
        .byte 255,20,33,22,0,0,22,20,20,22,0,22,0,0,22,20,20,20,23,22,20,20,20,22,0,22,255,20,6,22,255,0,5
        .byte 22,255,0,6,22,22,20,0,0,20,20,22,0,0,0,22,255,20,4,22,255,0,15,22,0,0,0,20,255,0,5,22,20,20,20
        .byte 26,255,0,19,22,255,0,6,22,20,20,27,255,0,27,20,20,255,0,27,26,20,20,255,0,8,21,24,255,0,17,27,20,20
        .byte 255,0,8,20,20,26,0,0,0,21,20,24,255,0,11,20,20,0,0,21,24,255,0,4,20,20,27,0,0,0,22,20,20
        .byte 255,0,11,22,20,0,0,20,20,255,0,4,20,20,24,255,0,4,20,22,0,0,0,21,24,255,0,7,20,0,0,22,22
        .byte 0,0,0,23,20,20,22,0,0,0,20,20,255,0,4,22,20,255,0,7,20,255,0,8,22,22,255,0,4,22,255,0,6
        .byte 20,24,0,0,0,21,20,20,20,255,0,20,20,20,22,0,0,0,255,20,4,255,0,7,23,255,0,13,22,255,0,4,22,22
        .byte 20,20,24,255,0,27,20,20,20,255,0,6,21,24,255,0,19,20,20,20,24,0,0,0,21,20,20,20,24,255,0,5,17
        .byte 255,0,12,255,20,4,24,0,0,255,20,24
        .byte 20,20,22,20,0,0,20,20,20,22,22,23,255,20,4,23,22,255,20,10,21,20,20,255,0,5,22,22,23,0,0,0,23
        .byte 20,20,0,0,0,22,23,22,22,22,20,20,255,22,4,20,22,255,0,12,22,22,255,0,8,22,22,255,0,4,20,31,255,0,28
        .byte 23,30,0,0,0,3,3,255,0,23,31,30,255,0,28,30,30,255,0,7,3,3,255,0,19,30,30,255,0,28,30,30,0,0,0
        .byte 3,3,255,0,23,30,30,255,0,28,30,30,34,255,0,9,31,31,255,0,15,33,30,30,29,255,0,9,30,30,255,0,4
        .byte 31,31,255,0,9,29,30,30,0,0,0,31,31,255,0,5,30,30,255,0,4,30,30,255,0,10,30,30,0,0,0,30,30
        .byte 255,0,5,30,30,31,0,0,0,30,30,31,255,0,4,31,31,0,0,0,30,30,0,0,0,30,30,255,0,5,30,30,30
        .byte 0,0,0,30,30,30,255,0,4,30,30,0,0,0,30,32,0,0,31,30,30,31,255,0,4,30,30,30,31,0,0,30,30,30
        .byte 31,0,0,0,30,30,0,0,0,30,0,0,0,255,30,4,0,0,0,31,255,30,4,0,0,255,30,4,0,0,0,30,30
        .byte 31,0,0,30,0,0,0,255,30,4,0,0,17,255,30,5,0,0,255,30,4,0,0,0,30,30,30,0,0,30,33,28,34
        .byte 255,30,4,33,28,34,255,30,5,33,34,255,30,4,33,28,34,30,30,30,33,34,30
        .byte 20,20,24,255,20,4,24,20,20,20,22,255,30,4,32,255,30,13,255,20,6,22,20,20,24,22,0,30,32,30,30,0,32
        .byte 30,30,32,30,32,30,32,32,30,32,30,30,22,20,20,22,20,20,24,20,22,20,0,0,30,0,30,32,0,0,30,32,0
        .byte 32,0,32,0,0,32,0,30,30,31,22,20,20,20,22,20,20,0,22,0,0,32,0,32,0,0,0,30,255,0,9,32,30,30
        .byte 0,20,24,0,0,22,20,255,0,10,32,255,0,10,30,30,0,20,20,0,0,0,22,255,0,21,30,30,0,20,20,255,0,22
        .byte 3,3,0,30,30,0,22,20,255,0,22,29,29,0,30,30,0,0,20,255,0,18,3,3,255,0,5,30,30,26,0,22,255,0,14
        .byte 3,3,0,0,29,29,255,0,4,26,30,30,27,255,0,16,29,29,255,0,8,27,30,30,255,0,13,3,3,255,0,13,30,30
        .byte 255,0,9,3,3,0,0,29,29,255,0,13,30,30,255,0,9,29,29,255,0,17,30,30,255,0,5,3,3,255,0,21,30
        .byte 32,0,3,3,0,0,29,29,255,0,10,31,31,255,0,9,32,0,0,29,29,255,0,6,31,31,255,0,6,30,30,0,0,0
        .byte 31,31,255,0,15,30,30,0,0,31,31,0,0,30,30,0,0,0,30,30,255,0,5,33,255,28,8,34,30,30,33,34,30,30
        .byte 33,34,30,30,33,28,34,30,30,33,28,28,28,34
        .byte 255,30,5,32,30,30,30,32,30,30,23,20,20,20,22,21,20,20,22,21,24,20,20,22,21,20,20,23,255,30,5,0,32
        .byte 30,30,0,32,30,0,22,22,22,0,0,0,22,0,22,20,20,23,0,22,20,24,20,30,30,32,30,30,0,0,30,30,0,0
        .byte 30,255,0,10,20,20,0,0,0,22,22,20,30,30,0,32,30,0,0,32,30,0,0,32,255,0,17,22,30,32,0,0,30
        .byte 0,0,0,32,255,0,20,31,30,0,0,0,32,255,0,24,30,30,255,0,28,30,32,255,0,14,3,255,0,12,31,30,255,0,20
        .byte 3,255,0,7,30,30,255,0,12,3,255,0,11,3,0,0,0,30,30,33,28,28,34,255,0,24,30,30,30,28,28,29,255,0,4
        .byte 3,255,0,19,30,30,30,28,29,255,0,25,32,30,30,29,0,0,3,255,0,10,31,255,0,13,30,30,255,0,14,30,31
        .byte 255,0,12,30,30,255,0,5,3,255,0,5,31,31,0,30,30,255,0,12,32,30,255,0,11,30,30,0,30,30,0,31,255,0,11
        .byte 32,255,0,11,30,30,0,30,30,0,30,31,255,0,10,33,255,28,10,34,30,30,34,30,30,33,30,30,33,255,28,8,34
        .byte 255,30,5,21,20,24,255,6,13,20,24,30,30,30,32,0,0,255,30,6,22,20,20,24,255,6,7,21,20,24,6,6,20,20
        .byte 30,32,30,0,0,0,255,30,4,32,30,0,22,20,20,20,24,6,21,24,6,6,255,20,6,22,32,0,32,0,0,0,30,30
        .byte 32,30,0,32,0,0,22,255,20,11,22,20,22,255,0,6,3,30,30,0,30,255,0,5,255,20,4,22,255,20,4,25,25
        .byte 0,22,255,0,8,30,30,0,32,255,0,5,25,25,20,22,0,20,20,22,22,25,25,255,0,10,30,30,255,0,7,25,25
        .byte 22,0,0,20,22,0,0,25,25,255,0,7,3,0,0,32,30,255,0,7,25,25,0,0,0,22,0,0,0,25,25,255,0,11
        .byte 30,255,0,7,25,25,255,0,7,25,25,255,0,4,3,255,0,6,30,0,0,0,3,0,0,0,25,25,255,0,7,25,25
        .byte 255,0,11,30,255,0,7,25,25,255,0,7,25,25,0,3,255,0,7,33,28,30,3,255,0,5,3,25,25,255,0,5,3
        .byte 0,25,25,255,0,9,28,28,30,255,0,7,25,25,0,0,3,255,0,4,25,25,255,0,8,33,28,29,30,255,0,7,25,25
        .byte 255,0,7,25,25,255,0,7,33,28,28,28,30,3,255,0,6,25,25,255,0,7,25,25,255,0,6,33,28,28,29,28,30
        .byte 255,0,7,25,25,255,0,7,25,25,255,0,5,33,28,29,29,0,28,30,255,0,6,33,25,25,34,255,0,5,33,25,25
        .byte 34,255,0,4,29,255,0,4,28,32,0,0,0,26,0,33,28,25,25,28,34,0,26,0,33,28,25,25,28,26,255,0,8
        .byte 28,33,255,28,7,6,6,255,28,7,6,6,255,28,11
        .byte 20,20,20,22,255,20,9,22,255,20,6,22,255,20,10,21,20,20,24,22,20,22,20,20,22,0,22,0,22,22,20,22,20,20
        .byte 0,22,22,23,20,22,20,20,22,20,21,20,20,22,22,0,22,0,22,22,255,0,6,22,0,22,22,255,0,4,22,0,23
        .byte 255,20,5,22,255,0,24,22,20,20,20,22,255,0,26,20,20,20,255,0,4,23,255,0,16,23,255,0,5,20,20,20,26
        .byte 255,0,26,22,20,20,27,255,0,6,23,255,0,20,20,20,255,0,24,23,0,0,0,20,22,255,0,10,23,255,0,17,20
        .byte 255,0,21,23,255,0,6,26,20,255,0,28,27,20,33,29,34,255,0,6,21,255,0,8,24,255,0,10,20,30,31,30,255,0,6
        .byte 20,23,23,23,0,0,23,23,23,20,255,0,10,20,30,30,30,31,255,0,5,20,26,255,0,6,26,20,24,255,0,9,20
        .byte 255,30,4,255,0,4,21,20,27,255,0,6,27,20,20,255,0,9,20,255,30,4,31,0,0,0,20,20,255,0,8,24,20
        .byte 24,0,0,0,31,255,0,4,20,255,30,5,0,21,20,21,20,255,6,8,20,21,22,24,0,31,30,31,31,0,0,31,255,30,5
        .byte 21,20,22,20,20,255,25,8,255,20,4,24,255,30,4,0,0,30
        .byte 255,1,31,40,255,0,27,1,1,41,255,0,27,1,1,42,255,0,27,1,1,255,0,10,1,1,255,0,9,1,1,255,0,5
        .byte 1,4,255,0,8,1,1,1,255,0,8,255,1,4,255,0,5,1,255,0,7,1,1,1,4,255,0,7,1,1,1,4,4
        .byte 1,255,0,4,1,1,255,0,4,17,1,1,1,4,255,0,7,1,1,1,4,4,0,40,1,0,0,0,1,4,1,255,2,5
        .byte 1,4,255,0,9,4,4,255,0,4,41,1,0,0,1,1,0,1,1,1,4,1,1,1,255,0,16,42,255,1,12,4,0,0,0
        .byte 255,1,5,255,0,9,4,4,0,0,0,40,1,4,1,38,38,39,38,255,0,7,4,1,1,255,0,12,41,1,0,1,255,38,4
        .byte 255,0,8,4,4,1,1,255,0,10,42,1,1,1,4,4,255,1,12,4,4,1,1,255,0,9,1,1,4,255,0,17,4
        .byte 255,0,8,1,1,1,26,255,0,23,255,1,6,27,255,0,22,1,4,1,1,4,1,1,1,255,0,21,1,1,0,1,1
        .byte 0,255,1,4,0,0,255,1,25
        .byte 255,1,30,4,255,0,10,4,4,0,0,0,4,1,1,4,255,0,9,1,255,0,17,4,1,255,0,10,1,255,0,18,4
        .byte 255,0,9,26,1,2,2,2,255,0,8,1,1,255,0,15,27,1,1,4,0,0,0,3,255,0,5,1,1,26,255,0,15
        .byte 4,1,255,0,6,1,1,1,0,1,1,27,255,0,16,1,255,0,5,255,1,7,255,0,17,1,0,0,40,255,1,9,255,0,4
        .byte 255,2,13,1,0,0,41,1,1,1,4,255,1,4,4,255,0,4,1,4,4,255,1,4,4,1,4,255,1,4,3,0,42
        .byte 1,38,38,38,1,38,38,38,1,3,0,0,0,4,38,38,38,1,38,38,38,4,0,0,0,1,1,0,0,0,4,38,38
        .byte 39,1,38,38,39,1,255,0,5,38,38,39,4,38,38,39,255,0,4,1,1,255,0,4,38,38,38,1,38,38,38,1,0,0,0
        .byte 3,1,38,38,38,0,38,38,38,255,0,4,255,1,6,4,0,0,255,1,5,255,0,4,255,1,8,4,0,0,0,1,1,1
        .byte 4,1,26,0,0,0,1,1,1,4,1,3,0,0,0,1,1,1,4,255,1,4,0,0,0,3,1,4,38,38,38,27,0,0
        .byte 3,1,38,38,38,1,255,0,4,1,38,38,38,1,38,38,38,4,0,0,0,1,0,38,38,39,1,0,0,0,4,38,38
        .byte 39,4,0,0,0,3,1,38,38,39,4,38,38,39,0,0,0,3,1,0,38,38,38,1,255,0,4,38,38,38,255,0,5
        .byte 1,38,38,38,0,38,38,38,255,0,4,255,1,31
        .byte 255,1,31,38,38,1,26,255,0,9,1,1,40,0,40,0,40,255,0,8,1,1,38,39,1,27,255,0,9,4,1,41,0
        .byte 41,0,41,255,0,9,255,1,5,255,0,10,1,42,0,42,0,42,255,0,9,1,38,38,1,255,0,5,3,3,255,0,4
        .byte 1,255,0,10,255,2,4,1,38,39,1,255,0,9,1,1,1,26,255,0,9,255,1,10,255,0,7,4,1,4,27,255,0,8
        .byte 1,4,1,4,1,255,0,14,4,255,0,9,1,1,0,1,0,1,255,0,23,255,1,7,255,2,9,255,0,13,1,4,255,1,4
        .byte 4,1,1,38,38,1,38,38,1,38,4,255,0,16,40,0,40,0,1,1,38,39,1,38,39,1,255,0,12,1,255,0,5
        .byte 41,0,41,0,1,1,4,4,255,1,7,4,255,0,7,1,1,255,0,5,42,0,42,0,1,1,38,38,4,38,38,4,0,0
        .byte 1,255,0,7,1,1,4,1,1,255,0,7,1,1,38,0,0,38,39,0,0,0,1,0,0,1,1,0,0,1,1,1,0
        .byte 1,1,255,0,7,1,1,0,0,1,1,1,0,4,4,255,1,5,0,0,4,255,1,4,4,255,3,6,4,4,1,255,0,29
        .byte 1,255,0,29,255,1,30
        .byte 1,1,4,255,1,9,4,40,4,255,1,4,4,40,4,255,1,4,4,40,4,1,1,4,255,0,4,4,1,1,4,0,0,0
        .byte 41,255,0,6,41,255,0,6,41,0,1,4,255,0,6,4,4,255,0,4,42,255,0,6,42,255,0,6,42,0,1,255,0,29
        .byte 1,255,0,29,1,255,2,4,255,0,25,255,1,4,4,255,0,8,3,3,3,255,0,4,3,3,3,255,0,4,3,3,3
        .byte 4,1,1,4,255,0,27,1,4,255,0,5,3,3,255,0,21,1,255,0,25,255,2,4,1,0,0,3,3,255,0,20,26
        .byte 1,38,38,1,1,255,0,18,2,2,2,0,0,0,27,1,38,39,1,1,255,0,18,1,4,1,40,0,0,0,1,4,1,1,1
        .byte 3,255,0,10,2,2,2,0,0,0,40,1,38,1,41,0,0,0,1,38,38,1,1,255,0,5,2,2,2,0,0,40,1,1
        .byte 4,40,0,0,41,1,38,1,42,0,0,40,1,38,39,1,1,0,0,3,3,0,4,1,1,0,0,41,1,38,1,41,0,0
        .byte 42,1,38,1,0,0,0,41,255,1,5,255,0,5,38,38,1,0,0,42,1,38,1,42,0,0,0,1,38,1,0,0,0
        .byte 42,1,38,38,1,1,255,0,5,38,39,1,0,0,0,4,1,1,255,0,4,1,1,4,255,0,4,4,38,39,255,1,31
        .byte 1,0,0,255,1,4,4,255,1,17,4,255,1,5,255,0,28,1,1,255,0,28,1,1,255,0,29,1,1,1,4,1,255,0,25
        .byte 1,4,4,0,4,0,0,0,3,3,255,0,16,255,2,4,1,255,0,24,40,255,1,5,255,0,19,1,1,0,0,0,41
        .byte 4,1,4,1,1,255,0,4,3,3,255,0,13,4,40,0,0,0,42,0,4,0,1,1,3,255,0,19,41,255,0,7,1,1
        .byte 255,0,12,1,1,255,0,6,42,255,0,7,1,1,255,0,12,4,4,40,255,0,13,1,1,3,255,0,13,41,255,0,11
        .byte 255,1,4,255,0,6,1,1,40,255,0,5,42,255,0,11,38,38,1,1,255,0,6,4,4,41,255,0,17,38,39,1,1
        .byte 3,3,255,0,6,42,255,0,15,255,1,6,255,0,26,38,38,1,1,255,0,26,38,39,255,1,31
        .byte 255,43,6,1,4,255,1,6,4,255,1,5,4,255,1,5,0,0,1,1,43,47,44,43,48,43,1,1,255,0,7,4,1
        .byte 4,1,4,255,0,8,4,1,43,46,45,43,43,48,1,1,255,0,7,26,1,0,1,255,0,9,1,1,255,43,6,1,1
        .byte 255,0,7,27,4,1,4,255,0,9,1,1,255,43,6,1,1,0,0,3,255,0,14,255,1,5,255,43,6,1,4,255,0,15
        .byte 1,1,4,0,4,1,1,255,43,6,1,26,255,0,4,3,255,0,8,1,1,4,255,0,4,1,1,255,43,4,48,43,1
        .byte 27,255,0,11,1,1,4,255,0,6,1,1,43,43,48,48,43,43,4,1,255,0,11,4,255,0,8,1,1,255,43,6,1,1
        .byte 255,0,7,1,1,255,0,10,3,4,1,255,43,6,1,1,255,0,5,1,1,4,4,255,0,11,26,1,255,43,6,1,1
        .byte 0,0,0,4,1,4,255,0,14,27,1,255,43,6,1,1,255,0,16,3,0,0,0,1,1,255,43,6,1,4,255,0,20
        .byte 1,4,1,43,1,43,1,43,1,1,1,4,4,255,1,4,255,0,13,255,1,8,4,255,0,6,4,1,1,1,255,0,10
        .byte 3,1,1,1,4,1,1,4,1,26,255,0,9,4,255,0,11,1,1,1,0,1,1,0,1,27,255,0,12,1,1,255,0,7
        .byte 4,255,1,24,0,0,255,1,5
        .byte 255,43,31,47,44,43,43,43,1,43,1,43,1,43,1,255,43,10,1,43,1,43,1,43,1,43,46,45,43,43,43,1,4
        .byte 1,1,1,4,1,255,43,10,1,4,1,1,1,4,1,255,43,6,1,0,1,1,1,0,1,43,1,1,43,1,1,43,1,1
        .byte 43,1,0,1,1,1,0,1,255,43,6,1,0,1,1,1,0,1,43,1,1,43,1,1,43,1,1,43,1,0,1,1,1
        .byte 0,1,43,43,43,48,43,43,255,1,10,4,255,1,9,4,1,1,1,255,43,4,48,43,1,1,4,255,0,18,4,1,1
        .byte 43,43,43,48,43,43,1,1,255,0,20,4,1,255,43,6,1,4,255,0,21,4,255,43,6,1,26,255,0,22,255,43,6
        .byte 1,27,255,0,22,43,48,255,43,4,1,1,255,0,9,3,0,0,3,0,0,3,0,0,255,2,4,43,43,48,43,43,43
        .byte 1,1,3,255,0,17,4,1,4,1,255,43,6,1,1,255,0,5,4,1,1,255,0,11,1,0,1,255,43,6,4,1,255,0,7
        .byte 4,1,1,255,0,9,4,4,1,255,43,6,1,1,3,255,0,8,4,1,1,255,0,8,26,1,255,43,6,1,1,255,0,11
        .byte 4,1,1,255,0,6,27,1,255,43,6,1,4,255,0,12,4,1,1,1,255,0,4,1,1,255,43,6,255,1,20,0,0
        .byte 1,1
        .byte 255,43,36,48,255,43,12,48,255,43,17,48,255,43,10,48,255,43,23,47,44,255,43,9,49,255,50,6,255,43,12,46,45
        .byte 255,43,8,49,51,255,0,6,1,1,43,1,1,43,1,1,255,43,13,49,51,255,0,7,1,1,43,1,1,43,1,1,255,43,12
        .byte 49,51,255,0,8,255,1,4,4,1,1,1,255,43,7,48,43,43,43,49,51,255,0,6,35,35,35,4,255,0,4,4,1
        .byte 255,43,9,48,43,49,51,255,0,16,1,255,43,7,48,43,43,49,51,255,0,4,255,35,7,255,0,6,1,255,43,9,49
        .byte 50,255,0,12,2,2,2,0,0,0,1,255,43,5,49,255,50,6,255,0,11,4,1,1,0,0,3,4,49,255,50,10,51
        .byte 0,0,255,35,9,0,1,4,255,0,27,1,4,255,0,28,1,0,0,255,56,27,1,0,0,53,55,50,50,54,50,50,55
        .byte 50,50,54,50,50,55,50,50,54,50,50,55,50,50,54,50,50,55,50,1,255,0,5,4,1,8,8,9,16,8,9,53,255,50,15
        .byte 255,1,7,4,0,8,0,0,8,0,0,53,54,50,50,55,50,50,54,50,50,55,50,50,54,50
        .byte 43,43,43,49,255,50,14,52,8,9,255,7,9,43,43,49,51,255,0,14,50,8,7,19,11,8,10,8,8,7,9,7,43
        .byte 49,51,255,0,15,50,9,9,7,8,8,0,8,8,7,7,11,50,51,255,0,4,255,35,8,255,0,4,50,50,52,8,8,8
        .byte 0,9,8,7,7,7,255,0,19,53,50,52,8,8,0,0,8,8,7,18,255,0,20,53,50,52,9,0,0,8,8,7,7
        .byte 0,0,0,255,35,11,255,0,7,53,50,52,0,0,8,8,7,7,35,35,35,255,0,14,35,35,35,0,0,53,50,52,0
        .byte 8,8,7,7,255,0,23,53,50,52,9,9,11,7,255,35,14,0,0,0,35,35,35,255,0,4,53,50,52,0,7,7,255,0,25
        .byte 50,50,0,7,9,255,0,25,50,50,0,19,7,255,35,14,0,0,0,35,35,35,255,0,5,50,50,0,11,7,255,0,23
        .byte 50,50,50,51,0,8,7,255,0,23,50,50,51,9,0,8,7,255,56,23,50,51,9,0,0,8,7,50,54,50,50,55,50,50
        .byte 54,50,50,55,50,50,54,50,50,51,8,8,8,10,9,8,8,255,0,4,9,7,255,50,15,51,9,0,8,8,0,0,9
        .byte 8,0,0,0,7,11,7,50,55,50,50,54,50,50,55,50,50,54,51,9,0,10,0,0,0,8,9,0,0,0,9,0,11
        .byte 255,7,4
        .byte 255,43,11,48,255,43,4,11,19,9,7,8,9,9,8,10,7,7,8,13,8,43,47,44,255,43,8,48,48,43,43,43,16
        .byte 8,255,7,5,8,7,10,7,8,13,8,43,46,45,43,43,48,255,43,11,9,7,7,7,8,9,7,9,0,9,7,7,8
        .byte 43,1,43,43,48,255,43,6,1,255,43,5,7,11,7,8,7,7,9,17,14,15,9,18,7,1,4,1,255,43,7,1,4
        .byte 1,43,43,43,11,7,7,9,9,7,7,7,14,12,12,15,11,13,1,0,1,43,1,43,1,43,1,43,1,0,1,49,56,56
        .byte 52,7,18,7,7,8,8,7,7,11,7,7,7,13,255,1,5,4,4,4,255,1,4,255,50,6,7,8,10,8,8,10,13
        .byte 7,7,9,7,11,255,1,5,13,13,13,255,1,5,53,54,54,51,7,9,8,17,0,8,5,13,9,255,7,4,255,1,5
        .byte 13,13,13,255,1,5,7,11,11,255,7,5,11,11,255,7,4,11,18,7,255,6,14,7,16,7,7,11,16,7,7,7,21,21
        .byte 20,24,24,11,7,255,6,14,5,20,7,7,21,255,20,6,23,22,24,7,7,255,2,14,255,20,10,22,0,0,20,20,20
        .byte 255,1,14,22,255,20,4,22,22,20,20,22,0,0,26,22,20,23,1,38,38,1,4,40,1,38,38,1,4,1,4,1,0
        .byte 23,22,20,20,0,0,0,20,0,0,0,27,25,20,25,1,38,39,1,26,41,1,38,39,1,0,1,0,1,0,0,0,22
        .byte 20,0,33,34,22,33,34,26,0,25,20,25,255,1,4,27,42,1,1,1,4,255,1,4,33,34,0,0,22,33,29,28,28,28
        .byte 29,28,28,25,22,25,1,4,0,4,0,0,4,255,0,6,4,29,29,34,0,26,28,31,29,31,29,31,29,28,25,25,25
        .byte 4,255,0,7,21,20,20,24,0,0,31,0,29,28,28,29,30,0,30,31,30,0,28,25,25,25,255,35,7,21,21,22,20
        .byte 23,24,31,30,34,26,29,29,31,30,31,30,30,30,31,29,25,25,25
numsc:
        .byte 31
.out .sprintf("SCREENS SIZE = %d", (* - scdat))

nmedat:
        .byte 0,0,128,192,1,2,88,72,1,2,48,32,2,3,48,96,255
        .byte 0,1,32,16,1,2,56,48,2,3,40,96,255
        .byte 0,1,32,16,1,2,56,128,2,3,136,96,1,2,104,56,255
        .byte 1,2,32,64,1,2,48,104,1,2,72,144,1,2,96,184,2,3,136,32,0,1,32,16,255
        .byte 0,1,32,16,1,2,88,80,1,2,40,176,2,3,112,144,255
        .byte 0,1,32,16,1,2,72,56,1,2,16,104,1,2,48,144,255
        .byte 0,1,32,16,1,4,64,16,1,4,48,144,255
        .byte 3,4,64,64,3,4,24,168,0,1,32,16,2,5,136,16,255
        .byte 1,4,40,56,1,4,88,208,0,1,136,16,2,5,48,120,255
        .byte 0,1,136,16,1,4,56,80,1,4,24,168,255
        .byte 0,1,72,16,2,5,96,88,1,4,32,40,1,4,56,176,255
        .byte 0,1,16,48,1,4,40,176,1,4,16,24,2,5,136,96,255
        .byte 0,1,24,48,1,6,112,64,1,6,96,104,1,6,72,144,1,6,56,184,255
        .byte 0,0,104,224,1,6,104,48,3,4,56,136,255
        .byte 0,0,88,224,2,5,136,32,255
        .byte 0,0,88,224,1,6,104,56,1,6,96,200,1,4,80,160,1,4,40,112,255
        .byte 0,0,16,40,1,7,112,24,1,7,72,128,1,7,120,184,1,7,48,224,255
        .byte 0,0,136,224,1,7,48,40,1,7,64,72,1,7,80,104,1,7,104,136,1,7,88,168,1,7,56,200,255
        .byte 0,0,136,224,2,8,136,48,1,7,40,144,1,7,104,184,255
        .byte 0,0,72,224,1,7,96,48,1,7,120,88,1,7,72,176,255
        .byte 0,1,88,16,1,6,104,112,3,4,40,64,3,4,56,176,255
        .byte 0,1,136,40,2,8,96,88,1,2,24,200,1,2,136,168,255
        .byte 0,0,56,224,1,2,120,120,255
        .byte 0,0,24,224,2,8,136,24,255
        .byte 0,0,64,224,1,2,80,192,1,2,96,136,1,2,112,80,255
        .byte 0,0,32,224,1,2,80,192,1,2,40,136,1,2,64,88,1,2,96,32,2,8,136,160,255
        .byte 0,0,128,200,1,2,128,144,1,2,64,72,1,2,72,176,255
        .byte 0,0,128,208,1,2,104,88,1,2,64,152,255
        .byte 0,1,80,16,2,8,112,128,255
        .byte 0,1,112,16,2,3,88,88,2,8,40,64,1,2,88,168,1,2,48,120,255
        .byte 0,1,136,40,255
.out .sprintf("POSITIONS SIZE = %d", (* - nmedat))

NUMOBJ = 92
objdta:
        .byte 1,128,3,192,1,128,21,168,62,124,65,130,74,82,98,70,65,130,62,124,42,84,63,252,16,8,31,248,0,0,0,0,0,96,216,0,96,216
        .byte 1,128,3,192,9,144,29,184,62,124,65,130,66,66,106,86,65,130,62,124,106,86,63,252,16,8,15,240,0,0,0,0,1,48,192,1,48,192
        .byte 0,0,5,160,0,0,4,32,14,112,49,140,42,84,34,68,49,140,30,120,18,72,31,248,0,0,0,0,0,0,0,0,0,16,128,0,16,128
        .byte 4,32,14,112,4,32,14,112,27,216,48,12,97,134,106,86,49,140,48,12,18,72,31,248,33,132,42,84,31,248,0,0,1,104,224,1,104,224
        .byte 16,8,32,4,95,250,64,2,84,42,85,170,84,42,85,170,84,42,85,170,84,42,85,170,64,2,95,250,32,4,0,0,15,136,224,15,136,224
        .byte 0,0,0,0,1,0,2,128,4,64,8,160,16,80,16,144,20,16,18,16,9,32,36,72,50,152,57,112,6,128,1,0,6,136,56,6,136,56
        .byte 1,0,2,128,6,192,14,224,28,112,24,176,17,16,144,146,242,30,148,18,202,38,116,92,2,128,1,0,0,0,0,0,2,112,24,2,112,24
        .byte 1,128,3,192,1,128,20,40,62,124,64,2,92,58,72,18,65,130,62,124,42,84,63,252,16,8,31,248,0,0,0,0,2,64,216,2,64,216
        .byte 0,0,0,0,9,32,26,176,60,120,11,32,20,16,36,40,38,104,16,208,9,160,4,64,2,128,1,0,0,0,0,0,3,80,200,3,80,200
        .byte 0,0,4,32,20,40,20,40,62,124,64,2,102,102,98,70,64,2,62,124,54,108,63,252,18,72,31,248,0,0,0,0,3,16,120,3,16,120
        .byte 0,0,0,0,0,0,4,64,14,224,4,64,4,64,36,72,116,92,36,72,42,168,63,248,32,8,85,84,127,252,0,0,3,96,16,3,96,16
        .byte 0,0,7,128,20,160,4,128,252,252,128,4,128,4,252,252,4,128,20,160,4,128,4,128,4,128,4,128,4,128,7,128,4,88,16,4,88,16
        .byte 7,128,12,192,8,64,123,120,80,40,67,8,120,120,11,64,8,64,27,96,56,112,107,88,104,88,123,120,64,8,63,240,5,32,120,5,32,120
        .byte 7,240,28,28,48,6,102,51,108,27,97,3,48,6,54,110,17,140,36,34,50,70,25,140,24,24,14,56,7,240,0,0,7,136,104,7,136,104
        .byte 7,240,28,28,48,6,102,35,108,59,97,131,48,6,48,14,17,204,38,34,49,198,27,108,24,24,14,56,7,240,0,0,6,120,152,6,120,152
        .byte 0,0,1,0,2,128,4,64,9,32,16,144,24,48,20,80,18,144,21,16,21,80,17,80,9,32,5,64,3,128,1,0,8,16,24,8,16,24
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,254,8,6,16,10,63,242,32,20,32,24,63,240,0,0,8,24,224,8,24,224
        .byte 1,128,3,192,6,96,4,32,4,32,2,64,1,128,1,128,1,128,1,128,1,160,1,192,1,192,1,160,0,0,0,0,10,24,24,10,24,24
        .byte 60,0,70,0,90,0,90,0,99,0,61,128,6,192,3,96,1,176,3,216,6,108,10,124,5,216,2,128,1,0,0,0,10,80,160,10,80,160
        .byte 0,0,0,0,1,0,2,128,4,64,8,160,16,80,16,144,20,16,18,16,9,32,36,72,50,152,57,112,6,128,1,0,9,136,104,9,136,104
        .byte 1,128,3,192,1,128,21,168,62,124,65,130,74,82,98,70,65,130,62,124,42,84,63,252,16,8,31,248,0,0,0,0,1,136,24,1,136,24
        .byte 1,128,3,192,9,144,29,184,62,124,65,130,66,66,106,86,65,130,62,124,106,86,63,252,16,8,15,240,0,0,0,0,4,80,160,4,80,160
        .byte 4,32,14,112,4,32,14,112,27,216,48,12,97,134,106,86,49,140,48,12,18,72,31,248,33,132,42,84,31,248,0,0,5,96,16,5,96,16
        .byte 0,0,0,0,1,0,2,128,4,64,8,160,16,80,16,144,20,16,18,16,9,32,36,72,50,152,57,112,6,128,1,0,5,112,224,5,112,224
        .byte 3,128,13,112,49,40,82,156,36,68,74,32,20,16,18,16,16,144,17,16,8,160,4,64,2,128,1,0,0,0,0,0,6,16,128,6,16,128
        .byte 130,0,146,0,84,0,108,0,68,0,170,0,68,0,84,0,68,0,254,0,130,0,84,0,56,0,0,0,0,0,0,0,7,48,128,7,48,128
        .byte 3,128,13,112,49,40,82,156,36,68,74,32,20,16,18,16,16,144,17,16,8,160,4,64,2,128,1,0,0,0,0,0,9,16,120,9,16,120
        .byte 0,0,0,0,1,0,2,128,4,64,10,32,20,16,18,18,16,147,17,21,8,164,4,117,2,154,1,44,0,84,0,100,9,24,224,9,24,224
        .byte 0,0,16,8,56,28,16,8,60,60,67,194,32,4,43,212,64,2,28,56,38,100,127,254,54,108,63,252,0,0,0,0,2,104,160,2,104,160
        .byte 1,0,6,192,24,48,36,8,72,68,80,164,32,72,18,16,9,32,4,64,2,128,1,0,0,0,0,0,0,0,0,0,4,24,144,4,24,144
        .byte 1,128,3,192,6,96,4,32,4,32,6,96,3,192,1,128,1,128,1,128,5,128,3,128,3,128,7,128,1,128,0,0,14,48,192,14,48,192
        .byte 15,192,112,32,79,192,72,0,188,0,170,0,165,0,162,128,161,64,160,160,64,80,0,40,0,20,0,12,0,0,0,0,8,136,96,8,136,96
        .byte 0,7,0,9,0,17,0,34,48,68,40,136,21,16,10,32,6,64,7,128,11,64,20,160,104,80,80,48,112,0,0,0,11,104,16,11,104,16
        .byte 24,96,100,152,196,140,195,12,200,12,108,24,96,24,48,48,48,48,48,48,56,112,28,224,15,192,0,0,0,0,0,0,10,72,224,10,72,224
        .byte 0,0,0,0,0,0,0,0,15,192,16,32,44,16,40,16,39,144,40,80,40,80,47,208,24,96,0,0,0,0,0,0,11,24,224,11,24,224
        .byte 0,64,0,224,1,176,3,184,6,28,3,14,1,198,0,108,0,184,1,80,2,128,5,0,10,0,20,0,40,0,48,0,11,120,224,11,120,224
        .byte 0,7,0,25,0,18,0,22,0,44,0,80,0,160,1,64,2,128,5,0,26,0,52,0,12,0,8,0,0,0,0,0,16,40,192,16,40,192
        .byte 16,0,56,0,16,0,40,0,16,0,40,0,16,0,40,0,16,0,40,0,124,0,198,0,238,0,130,0,108,0,56,0,12,32,232,12,32,232
        .byte 224,0,144,0,136,0,68,0,34,12,17,20,8,168,4,80,2,96,1,224,2,208,5,40,10,22,12,10,0,14,0,0,12,40,120,12,40,120
        .byte 96,6,144,9,136,17,68,34,68,34,66,66,65,130,32,4,18,72,12,50,50,69,41,138,21,148,15,136,1,176,1,192,12,136,40,12,136,40
        .byte 224,7,177,141,208,11,121,158,29,184,11,208,5,160,3,192,5,160,11,208,21,168,41,148,49,140,65,130,2,64,5,160,13,24,224,13,24,224
        .byte 3,192,2,64,1,128,1,128,7,224,8,16,16,136,17,168,23,232,21,136,9,16,4,32,2,64,1,128,2,64,7,224,13,48,32,13,48,32
        .byte 48,24,72,36,135,194,128,2,129,2,67,36,67,228,79,132,41,136,17,16,8,32,4,64,4,64,2,128,1,0,0,0,13,32,152,13,32,152
        .byte 0,0,3,128,6,192,4,64,9,32,9,32,8,32,7,192,1,0,1,16,17,32,9,80,21,160,11,0,1,0,1,0,7,136,16,7,136,16
        .byte 0,0,16,0,16,0,16,0,16,0,16,0,16,0,16,0,84,0,186,0,84,0,16,0,16,0,56,0,16,0,0,0,16,136,176,16,136,176
        .byte 3,192,12,48,16,136,32,68,80,10,79,242,32,4,16,8,10,16,9,16,4,32,2,64,1,128,0,0,0,0,0,0,16,32,120,16,32,120
        .byte 0,0,3,0,4,128,5,128,3,0,7,128,9,192,11,192,15,192,4,128,31,224,17,224,23,224,55,240,63,240,0,0,15,136,120,15,136,120
        .byte 3,0,7,128,3,0,8,64,27,96,19,32,23,160,7,128,0,0,7,128,4,128,4,128,12,192,0,0,0,0,0,0,15,24,32,15,24,32
        .byte 1,128,7,224,30,120,120,30,225,135,193,131,97,134,102,102,54,108,49,140,25,152,25,152,13,176,12,48,6,96,1,128,17,40,216,17,40,216
        .byte 28,0,99,0,65,0,152,128,158,128,137,0,75,0,103,128,25,192,0,224,0,120,0,52,0,42,0,22,0,12,0,0,17,136,168,17,136,168
        .byte 0,0,7,224,8,16,4,32,2,64,3,192,2,64,6,96,24,24,32,4,34,20,65,10,80,2,40,4,32,4,31,248,17,112,24,17,112,24
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,120,0,28,0,7,2,253,253,7,2,28,0,120,0,18,16,200,18,16,200
        .byte 0,0,0,0,255,248,64,16,48,96,24,192,15,128,7,0,2,0,2,0,5,0,10,128,18,64,32,32,71,16,255,248,18,120,16,18,120,16
        .byte 1,0,3,128,1,0,2,128,12,96,17,16,17,16,33,8,32,200,16,16,16,16,12,96,3,128,0,0,0,0,0,0,18,24,80,18,24,80
        .byte 1,128,2,64,2,64,4,32,5,160,15,240,1,128,7,224,1,128,3,192,0,0,1,128,0,0,1,128,0,0,1,128,19,32,48,19,32,48
        .byte 0,0,0,0,0,0,1,128,130,65,255,255,80,10,64,2,127,254,32,4,63,252,106,170,85,86,106,170,127,254,64,2,19,136,224,19,136,224
        .byte 0,0,1,0,3,128,11,160,27,176,3,128,57,56,126,252,57,56,3,128,27,176,11,160,3,128,1,0,0,0,0,0,19,56,128,19,56,128
        .byte 3,0,7,128,3,0,8,64,27,96,19,32,23,160,7,128,0,0,7,128,4,128,4,128,12,192,0,0,0,0,0,0,7,16,176,7,16,176
        .byte 15,192,48,48,39,144,72,72,83,40,84,168,84,168,83,40,72,72,39,144,48,48,15,192,0,0,0,0,0,0,0,0,20,32,48,20,32,48
        .byte 0,0,64,2,48,12,56,28,31,248,15,240,12,48,13,176,13,176,12,48,15,240,31,248,56,28,48,12,64,2,0,0,20,24,176,20,24,176
        .byte 0,0,0,0,1,128,3,192,6,96,7,224,53,172,47,244,46,116,51,204,6,96,7,224,1,128,7,224,13,176,0,0,20,128,56,20,128,56
        .byte 64,8,17,32,34,144,4,64,9,32,17,16,18,144,8,32,8,32,36,72,4,64,82,148,129,2,0,0,0,0,0,0,0,136,48,0,136,48
        .byte 1,128,2,64,1,128,1,128,3,192,7,224,6,224,15,112,11,176,25,248,28,56,15,240,1,128,0,0,0,0,0,0,14,112,112,14,112,112
        .byte 0,0,4,0,12,0,10,0,4,0,4,0,14,0,14,0,15,0,13,0,31,0,14,8,26,20,142,40,127,224,63,192,14,112,32,14,112,32
        .byte 0,0,0,0,3,224,6,48,12,24,12,24,12,24,31,252,63,254,63,126,63,126,31,252,31,252,15,248,7,240,0,0,21,96,24,21,96,24
        .byte 0,0,0,0,0,6,0,10,0,20,0,40,0,80,0,160,1,64,6,128,15,0,31,0,62,0,124,0,88,0,48,0,21,88,216,21,88,216
        .byte 63,224,32,32,64,16,64,16,170,175,85,95,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,21,16,224,21,16,224
        .byte 56,0,124,0,204,0,204,0,252,0,126,0,7,0,3,128,1,200,0,252,0,120,0,120,0,252,0,76,0,0,0,0,22,40,72,22,40,72
        .byte 7,128,15,192,8,64,15,192,7,128,8,64,24,96,51,48,53,176,54,176,51,48,56,112,48,48,31,224,16,32,15,192,22,136,48,22,136,48
        .byte 3,0,7,128,3,0,8,64,27,96,19,32,23,160,7,128,0,0,7,128,4,128,4,128,12,192,0,0,0,0,0,0,22,136,16,22,136,16
        .byte 96,0,144,0,144,0,120,0,28,0,14,8,7,20,3,162,1,193,0,193,1,33,2,18,4,4,2,8,1,16,0,224,23,112,64,23,112,64
        .byte 0,0,1,128,31,248,44,52,44,52,56,28,57,156,44,52,44,52,28,56,20,40,12,48,14,112,15,240,3,192,0,0,23,32,112,23,32,112
        .byte 7,224,8,16,16,8,58,92,81,138,81,138,255,255,249,159,255,255,81,138,81,138,58,92,16,8,8,16,7,224,0,0,23,112,224,23,112,224
        .byte 3,128,7,192,15,224,9,32,31,240,15,224,12,96,10,160,7,192,15,224,15,224,7,192,0,0,0,0,0,0,0,0,24,136,56,24,136,56
        .byte 0,0,3,192,12,48,16,8,34,196,33,164,65,194,67,2,37,4,35,132,17,72,12,48,3,192,0,0,0,0,0,0,24,80,160,24,80,160
        .byte 2,0,5,0,13,128,8,128,8,128,24,192,24,192,13,128,5,0,5,0,2,0,2,0,15,128,2,0,5,0,2,0,24,16,88,24,16,88
        .byte 3,0,6,128,7,128,3,0,3,0,7,128,108,216,219,116,251,124,108,216,7,128,3,0,3,0,6,128,7,128,3,0,22,128,152,22,128,152
        .byte 7,128,7,128,4,128,3,0,3,0,3,0,3,0,7,160,8,64,4,64,24,96,19,32,24,96,15,128,0,0,0,0,26,16,88,26,16,88
        .byte 1,128,2,64,111,246,64,2,31,248,80,10,84,42,82,74,84,42,82,74,80,10,31,248,64,2,111,246,0,0,0,0,26,56,216,26,56,216
        .byte 0,0,0,0,0,0,1,128,3,192,4,32,4,32,2,64,1,128,1,128,1,128,63,252,85,86,106,170,63,252,0,0,26,136,64,26,136,64
        .byte 12,96,28,112,28,112,15,224,7,192,3,128,3,128,3,128,3,128,3,128,3,128,7,192,15,224,28,112,28,112,12,96,28,48,224,28,48,224
        .byte 0,0,3,0,7,128,15,192,31,224,16,32,8,64,4,128,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,28,120,24,28,120,24
        .byte 1,128,1,0,0,128,9,144,62,124,64,2,226,71,200,19,65,130,62,124,34,68,63,252,16,8,31,248,0,0,0,0,25,80,224,25,80,224
        .byte 1,0,3,128,1,0,1,0,2,128,4,64,8,32,10,160,8,32,4,64,7,192,8,32,24,48,0,0,0,0,0,0,25,56,16,25,56,16
        .byte 0,0,3,128,7,192,11,160,17,16,53,88,51,152,63,248,51,152,53,88,17,16,11,160,7,192,3,128,0,0,0,0,25,136,16,25,136,16
        .byte 3,128,2,128,3,128,1,0,1,0,2,128,50,152,4,64,63,248,127,12,127,236,127,252,48,24,0,0,0,0,0,0,29,96,192,29,96,192
        .byte 0,12,0,50,0,195,3,13,12,53,48,197,67,13,161,53,152,213,70,21,49,149,12,117,3,21,0,197,0,54,0,12,29,48,144,29,48,144
        .byte 1,128,2,64,2,64,1,128,6,96,12,48,24,24,96,6,144,9,159,249,103,230,0,0,0,0,0,0,0,0,0,0,29,16,104,29,16,104
        .byte 0,0,9,144,0,0,14,112,27,216,48,12,105,22,99,198,40,148,64,2,50,76,31,248,41,148,32,4,31,248,0,0,27,64,72,27,64,72
        .byte 0,0,0,0,127,254,255,255,128,1,147,201,184,29,147,201,128,1,191,253,80,10,63,252,0,0,0,0,0,0,0,0,27,136,144,27,136,144
        .byte 3,0,7,128,3,0,8,64,27,96,19,32,23,160,7,128,0,0,7,128,4,128,4,128,12,192,0,0,0,0,0,0,27,56,208,27,56,208
        .byte 0,0,0,0,0,0,0,0,64,0,32,0,127,240,64,24,127,240,42,128,41,128,46,0,40,0,40,0,56,0,0,0,28,80,152,28,80,152
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
        .byte 0,124,78,94,246,230,124,0
        .byte 0,56,120,8,8,24,126,0
        .byte 0,124,70,6,124,66,254,0
        .byte 0,124,70,28,6,70,124,0
        .byte 0,16,48,120,216,254,24,0
        .byte 0,126,64,252,6,198,124,0
        .byte 0,124,64,124,198,198,124,0
        .byte 0,254,4,12,24,48,32,0
        .byte 0,124,198,124,198,196,124,0
        .byte 0,124,70,198,124,4,124,0
        .byte 0,0,0,48,0,0,48,0
        .byte 0,0,48,0,0,48,48,96
        .byte 0,0,12,24,48,24,12,0
        .byte 0,0,0,126,0,126,0,0
        .byte 0,0,48,24,12,24,48,0
        .byte 0,124,198,12,24,0,24,0
        .byte 0,124,222,254,254,192,124,0
        .byte 0,124,70,70,254,198,198,0
        .byte 0,124,70,124,198,198,252,0
        .byte 0,126,66,192,192,198,124,0
        .byte 0,120,76,70,198,204,248,0
        .byte 0,126,64,124,192,192,254,0
        .byte 0,126,64,124,192,192,192,0
        .byte 0,126,66,192,222,198,124,0
        .byte 0,70,70,126,198,198,198,0
        .byte 0,62,8,8,24,24,126,0
        .byte 0,4,4,6,70,198,124,0
        .byte 0,76,88,112,216,204,198,0
        .byte 0,64,64,64,192,192,254,0
        .byte 0,70,110,126,70,198,198,0
        .byte 0,70,102,118,94,206,198,0
        .byte 0,124,70,198,198,198,124,0
        .byte 0,124,70,70,252,192,192,0
        .byte 0,124,70,70,246,222,124,0
        .byte 0,124,70,70,124,204,198,0
        .byte 0,124,192,124,6,70,252,0
        .byte 0,254,16,16,48,48,48,0
        .byte 0,70,70,70,198,198,124,0
        .byte 0,70,70,198,198,108,56,0
        .byte 0,70,70,70,198,254,108,0
        .byte 0,70,108,56,56,108,196,0
        .byte 0,134,204,120,48,16,16,0
        .byte 0,254,76,24,48,96,254,0
        .byte 0,30,24,24,24,24,30,0
        .byte 0,0,192,96,48,24,12,0
        .byte 0,240,48,48,48,48,240,0
        .byte 0,48,120,252,48,48,48,0
        .byte 0,0,0,0,0,0,0,255
        .byte 0,60,102,248,96,96,254,0
        .byte 0,0,120,12,124,204,124,0
        .byte 0,96,96,124,102,102,124,0
        .byte 0,0,60,96,96,96,60,0
        .byte 0,12,12,124,204,204,124,0
        .byte 0,0,120,204,248,192,124,0
        .byte 0,28,48,56,48,48,48,0
        .byte 0,0,124,204,204,124,12,120
        .byte 0,192,192,248,204,204,204,0
        .byte 0,48,0,112,48,48,120,0
        .byte 0,12,0,12,12,12,108,56
        .byte 0,96,120,112,112,120,108,0
        .byte 0,48,48,48,48,48,28,0
        .byte 0,0,248,172,172,172,172,0
        .byte 0,0,248,204,204,204,204,0
        .byte 0,0,120,204,204,204,120,0
        .byte 0,0,248,204,204,248,192,192
        .byte 0,0,124,204,204,124,12,14
        .byte 0,0,60,96,96,96,96,0
        .byte 0,0,120,192,120,12,248,0
        .byte 0,48,120,48,48,48,28,0
        .byte 0,0,204,204,204,204,120,0
        .byte 0,0,204,204,120,120,48,0
        .byte 0,0,84,212,212,252,120,0
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
