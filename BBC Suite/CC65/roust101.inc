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
WINDOWHGT = 23
WINDOWWID = 32 ;面面面面面面面面面面面面
MAPWID = 3
        .byte 255,255,255
mapdat:
        .byte 255,0,255
        .byte 255,255,255
stmap:  .byte 2

evnt00:
        lda #3
        cmp vark
        beq *+5
        jmp a00023
        jmp a04531
a00023: lda #250
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp a00140
        lda #0
        sta varg
        lda vara
        ldy #8
        sta (z80_ix),y
        iny; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #12
        jsr drpob
        lda #255
        sta z80_c
        lda numlif
        clc
        adc z80_c
        sta numlif
        lda #1
        sta varj
a00140: lda #247
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp a00223
        lda vara
        ldy #8
        sta (z80_ix),y
        lda #12
        jsr getob
        ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #13
        jsr drpob
a00223: lda #243
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp a00306
        lda vara
        ldy #8
        sta (z80_ix),y
        lda #13
        jsr getob
        ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #14
        jsr drpob
a00306: lda #237
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp a00533
        lda #14
        jsr getob
        lda #0
        cmp numlif
        beq *+5
        jmp a00533
        lda varj
        cmp #5
        bcc *+5
        jmp a00430
        lda #22
        sta chary
        lda #10
        sta charx
        lda #0
        sta varm
        lda #130
        sta z80_c
        lda varm
        clc
        adc z80_c
        sta varm
        lda varm
        sta varopt
a00430: lda #5
        sta varj
        lda #0
        sta varrnd
        lda joyval
        and #16
        beq :+
        jmp a00473
:
        lda #1
        sta varrnd
a00473: lda joyval
        and #8
        beq :+
        jmp a00499
:
        lda #1
        sta varrnd
a00499: lda #1
        cmp varrnd
        beq *+5
        jmp a00530
        ; lda #1
        sta gamwon
        jmp a00533
a00530: rts
a00533: lda #150
        ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp a00569
        lda #200
        ldy #8
        sta (z80_ix),y
a00569: lda #99
        ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp a00628
        lda #255
        sta z80_c
        ; ldy #11
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #11
        sta (z80_ix),y
a00628: lda #150
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp a00717
        lda #168
        ldy #8
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        sta vara
        lda #120
        iny; ldy #9
        sta (z80_ix),y
        lda #2
        sta vark
        lda #0
        iny; ldy #10
        sta (z80_ix),y
a00717: lda #100
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp a00748
        lda #0
        sta vark
a00748: lda #2
        cmp vark
        beq *+5
        jmp a00874
        lda joyval
        and #1
        beq :+
        jmp a00790
:
        lda #0
        sta vark
a00790: lda joyval
        and #2
        beq :+
        jmp a00815
:
        lda #0
        sta vark
a00815: lda joyval
        and #4
        beq :+
        jmp a00841
:
        lda #0
        sta vark
a00841: lda #0
        cmp vark
        beq *+5
        jmp a00874
        lda #152
        ldy #8
        sta (z80_ix),y
        rts
a00874: ldy #8
        lda (z80_ix),y
        cmp #200
        bcc *+5
        jmp a04531
        lda #1
        cmp vark
        beq *+5
        jmp a00944
        lda #250
        ldy #11
        sta (z80_ix),y
        lda #200
        ldy #8
        sta (z80_ix),y
        rts
a00944: ldy #8
        lda (z80_ix),y
        sta vara
        iny; ldy #9
        lda (z80_ix),y
        sta varb
        lda #1
        cmp varl
        beq *+5
        jmp a01319
        lda #0
        sta varl
        lda #160
        dey; ldy #8
        sta (z80_ix),y
        lda #0
        iny; ldy #9
        sta (z80_ix),y
        lda #7
        sta z80_c
        lda #15
        sta z80_b
        jsr spawn
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda varr
        sta varrnd
        lda varrnd
        sta z80_d
        lda #5
        sta z80_e
        jsr idiv
        lda z80_d
        sta varrnd
        lda varrnd
        sta z80_d
        asl a
        asl a
        clc
        adc z80_d
        sta varrnd
        lda varr
        sta varobj
        lda varrnd
        sta z80_c
        lda varobj
        sec
        sbc z80_c
        sta varobj
        lda varr
        cmp varrnd
        beq *+5
        jmp a01222
        lda numlif
        clc
        adc #1
        sta numlif
        lda #11
        sta varopt
        lda #3
        sta varj
a01222: lda #3
        cmp varobj
        beq *+5
        jmp a01247
        lda #2
        sta varj
a01247: lda #2
        cmp varobj
        beq *+5
        jmp a01272
        lda #1
        sta varg
a01272: lda vara
        ldy #8
        sta (z80_ix),y
        lda varb
        iny; ldy #9
        sta (z80_ix),y
        lda varr
        clc
        adc #1
        sta varr
        rts
a01319: ldy #10
        lda (z80_ix),y
        sta varp
        ; ldy #10
        lda (z80_ix),y
        lsr a
        lsr a
        lsr a
        lsr a
        ; ldy #10
        sta (z80_ix),y
        ; ldy #10
        lda (z80_ix),y
        sta varq
        lda varq
        asl a
        asl a
        asl a
        asl a
        sta varq
        lda varq
        sta z80_c
        lda varp
        sec
        sbc z80_c
        sta varp
        lda #8
        cmp varp
        beq *+5
        jmp a01454
        lda #0
        sta varp
a01454: lda #6
        sta z80_b
        jsr sktyp
        bcs :+
        jmp a01539
:
        lda #2
        sta varo
        lda #9
        cmp varp
        bcc *+5
        jmp a01514
        ; lda #9
        sta varp
        jmp a01539
a01514: lda #1
        cmp varp
        bcc *+5
        jmp a01539
        ; lda #1
        sta varp
a01539: lda #8
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a01575
        lda #0
        ; ldy #10
        sta (z80_ix),y
a01575: ldy #9
        lda (z80_ix),y
        cmp #4
        bcc *+5
        jmp a01612
        lda #240
        ; ldy #9
        sta (z80_ix),y
a01612: ldy #8
        lda (z80_ix),y
        cmp #4
        bcc *+5
        jmp a01661
        lda #4
        ; ldy #8
        sta (z80_ix),y
        lda #1
        ldy #10
        sta (z80_ix),y
a01661: lda #240
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp a01697
        lda #4
        ; ldy #9
        sta (z80_ix),y
a01697: lda #160
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a01778
        lda #6
        sta z80_b
        jsr sktyp
        bcs :+
        jmp a01746
:
        jmp a01778
a01746: lda #12
        ldy #10
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
a01778: ldy #10
        lda (z80_ix),y
        cmp #8
        bcc *+5
        jmp a01913
        lda #0
        sta varo
        lda #3
        sta loopa
a01817: lda varo
        ; ldy #10
        cmp (z80_ix),y
        bcc *+5
        jmp a01880
        jsr cangd
        beq :+
        jmp a01880
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
a01880: lda varo
        clc
        adc #1
        sta varo
        dec loopa
        beq :+
        jmp a01817
:
        jmp a02030
a01913: lda #8
        sta varo
        lda #3
        sta loopa
a01930: lda varo
        ldy #10
        cmp (z80_ix),y
        bcc *+5
        jmp a02002
        jsr cangu
        beq :+
        jmp a02002
:
        lda #255
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
a02002: lda varo
        clc
        adc #1
        sta varo
        dec loopa
        beq :+
        jmp a01930
:
a02030: lda varp
        cmp #8
        bcc *+5
        jmp a02192
        lda #0
        sta varo
        lda #3
        sta loopa
a02063: lda varo
        cmp varp
        bcc *+5
        jmp a02159
        jsr cangr
        beq :+
        jmp a02125
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
        jmp a02159
a02125: lda #10
        sta varp
        lda #2
        cmp varo
        bcc *+5
        jmp a02159
        lda #10
        sta varopt
a02159: lda varo
        clc
        adc #1
        sta varo
        dec loopa
        beq :+
        jmp a02063
:
        jmp a02342
a02192: lda #8
        sta varo
        lda #3
        sta loopa
a02209: lda varo
        cmp varp
        bcc *+5
        jmp a02313
        jsr cangl
        beq :+
        jmp a02280
:
        lda #255
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp a02313
a02280: lda #9
        cmp varo
        bcc *+5
        jmp a02305
        lda #10
        sta varopt
a02305: lda #2
        sta varp
a02313: lda varo
        clc
        adc #1
        sta varo
        dec loopa
        beq :+
        jmp a02209
:
a02342: lda #0
        cmp varp
        bcc *+5
        jmp a02469
        jsr cangd
        beq :+
        jmp a02377
:
        jmp a02469
a02377: lda #0
        sta varq
        ; lda #0
        cmp varc
        beq *+5
        jmp a02410
        lda #1
        sta varq
a02410: lda #2
        cmp varc
        beq *+5
        jmp a02435
        lda #1
        sta varq
a02435: lda #1
        cmp varq
        beq *+5
        jmp a02469
        lda #6
        sta varopt
        lda #0
        jsr animsp
a02469: lda joyval
        and #1
        beq :+
        jmp a02718
:
        lda #8
        cmp varp
        bcc *+5
        jmp a02668
        jsr cangd
        beq :+
        jmp a02546
:
        lda #255
        sta z80_c
        lda varp
        clc
        adc z80_c
        sta varp
        jmp a02663
a02546: lda joyval
        and #2
        beq :+
        jmp a02568
:
        jmp a02663
a02568: lda #4
        ldy #6
        sta (z80_ix),y
        lda #1
        iny; ldy #7
        sta (z80_ix),y
        lda #2
        cmp varc
        beq *+5
        jmp a02621
        lda #5
        sta varopt
a02621: lda #0
        cmp varc
        beq *+5
        jmp a02663
        lda #255
        sta z80_c
        lda varp
        clc
        adc z80_c
        sta varp
a02663: jmp a02681
a02668: lda #0
        ldy #6
        sta (z80_ix),y
a02681: lda varp
        cmp #3
        bcc *+5
        jmp a02713
        lda varp
        clc
        adc #1
        sta varp
a02713: jmp a02888
a02718: lda joyval
        and #2
        beq :+
        jmp a02740
:
        jmp a02888
a02740: lda #0
        cmp varp
        bcc *+5
        jmp a02888
        jsr cangd
        beq :+
        jmp a02775
:
        jmp a02888
a02775: lda #0
        cmp varc
        beq *+5
        jmp a02825
        lda #5
        sta varopt
        lda #255
        sta z80_c
        lda varp
        clc
        adc z80_c
        sta varp
a02825: lda #4
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y
        cmp varb
        bcc *+5
        jmp a02888
        lda #1
        ldy #7
        sta (z80_ix),y
a02888: lda joyval
        and #2
        beq :+
        jmp a03167
:
        lda #8
        cmp varp
        bcc *+5
        jmp a02938
        lda varp
        clc
        adc #1
        sta varp
a02938: lda #11
        cmp varp
        bcc *+5
        jmp a02964
        ; lda #11
        sta varp
a02964: lda #0
        cmp varp
        beq *+5
        jmp a02988
        lda #9
        sta varp
a02988: lda varp
        cmp #5
        bcc *+5
        jmp a03153
        jsr cangd
        beq :+
        jmp a03048
:
        lda #255
        sta z80_c
        lda varp
        clc
        adc z80_c
        sta varp
        jmp a03149
a03048: lda joyval
        and #1
        beq :+
        jmp a03070
:
        jmp a03149
a03070: lda #4
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        ; lda #0
        cmp varc
        beq *+5
        jmp a03149
        lda #5
        sta varopt
        lda #255
        sta z80_c
        lda varp
        clc
        adc z80_c
        sta varp
a03149: jmp a03167
a03153: lda #1
        ldy #6
        sta (z80_ix),y
a03167: lda #0
        sta varrnd
        lda joyval
        and #16
        beq :+
        jmp a03201
:
        lda #1
        sta varrnd
a03201: lda joyval
        and #8
        beq :+
        jmp a03227
:
        lda #1
        sta varrnd
a03227: lda #1
        cmp varrnd
        beq *+5
        jmp a03493
        lda #6
        sta z80_b
        jsr sktyp
        bcs :+
        jmp a03330
:
        lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        ldy #12
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
a03330: lda #0
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp a03479
        lda #4
        sta varopt
        lda #9
        ldy #10
        cmp (z80_ix),y
        bcc *+5
        jmp a03443
        ; ldy #10
        lda (z80_ix),y
        cmp #13
        bcc *+5
        jmp a03443
        lda #2
        sta z80_c
        ; ldy #10
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #10
        sta (z80_ix),y
a03443: ldy #10
        lda (z80_ix),y
        cmp #5
        bcc *+5
        jmp a03479
        lda #11
        ; ldy #10
        sta (z80_ix),y
a03479: lda #2
        ldy #12
        sta (z80_ix),y
a03493: lda #0
        ldy #12
        cmp (z80_ix),y
        bcc *+5
        jmp a03657
        ; lda #0
        sta varrnd
        lda joyval
        and #16
        beq :+
        jmp a03550
:
        lda #1
        sta varrnd
a03550: lda joyval
        and #8
        beq :+
        jmp a03576
:
        lda #1
        sta varrnd
a03576: lda #1
        cmp varrnd
        beq *+5
        jmp a03598
        jmp a03657
a03598: ldy #12
        lda (z80_ix),y
        cmp #7
        bcc *+5
        jmp a03657
        lda #255
        sta z80_c
        ; ldy #12
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #12
        sta (z80_ix),y
a03657: jsr cangd
        beq :+
        jmp a04052
:
        ldy #6
        lda (z80_ix),y
        cmp #2
        bcc *+5
        jmp a03733
        lda #2
        sta z80_c
        ; ldy #6
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #6
        sta (z80_ix),y
        jmp a03812
a03733: lda #4
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a03812
        iny; ldy #7
        lda (z80_ix),y
        dey; ldy #6
        sta (z80_ix),y
        lda #2
        sta z80_c
        ; ldy #6
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #6
        sta (z80_ix),y
a03812: lda #0
        sta varrnd
        lda joyval
        and #16
        beq :+
        jmp a03847
:
        lda #1
        sta varrnd
a03847: lda joyval
        and #8
        beq :+
        jmp a03873
:
        lda #1
        sta varrnd
a03873: lda #1
        cmp varrnd
        beq *+5
        jmp a03909
        ; lda #1
        ldy #7
        sta (z80_ix),y
        jmp a03922
a03909: lda #0
        ldy #7
        sta (z80_ix),y
a03922: lda #0
        cmp varc
        beq *+5
        jmp a04047
        lda #8
        ldy #10
        cmp (z80_ix),y
        bcc *+5
        jmp a03998
        lda #255
        sta z80_c
        ; ldy #10
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #10
        sta (z80_ix),y
a03998: ldy #10
        lda (z80_ix),y
        cmp #4
        bcc *+5
        jmp a04047
        ; ldy #10
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #10
        sta (z80_ix),y
a04047: jmp a04226
a04052: lda #0
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp a04088
        ; lda #0
        ldy #10
        sta (z80_ix),y
a04088: lda #0
        cmp varp
        beq *+5
        jmp a04226
        lda #4
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a04168
        iny; ldy #7
        lda (z80_ix),y
        dey; ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        jmp a04226
a04168: lda #1
        ldy #6
        cmp (z80_ix),y
        bcc *+5
        jmp a04226
        lda #254
        sta z80_c
        ; ldy #6
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #6
        sta (z80_ix),y
a04226: ldy #10
        lda (z80_ix),y
        asl a
        asl a
        asl a
        asl a
        ; ldy #10
        sta (z80_ix),y
        lda varp
        sta z80_c
        ; ldy #10
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #10
        sta (z80_ix),y
        lda #0
        cmp vark
        beq *+5
        jmp a04314
a04314: lda #2
        cmp vark
        beq *+5
        jmp a04531
        lda #5
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
        lda #21
        sta chary
        lda #15
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #19
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #20
        jsr pattr
        lda #1
        sta varj
        lda #152
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a04531
        lda #0
        cmp varc
        beq *+5
        jmp a04531
        lda #255
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
a04531: rts
.out .sprintf("EVENT 0 SIZE = %d", (* - evnt00))

evnt01:
        lda #5
        cmp varj
        beq *+5
        jmp b00035
        lda #255
        ldy #5
        sta (z80_ix),y
        rts
b00035: ldy #10
        lda (z80_ix),y
        sta vart
        ; ldy #10
        lda (z80_ix),y
        sta vari
        lda vart
        lsr a
        lsr a
        lsr a
        lsr a
        lsr a
        sta vart
        lda vart
        sta varq
        lda varq
        asl a
        asl a
        asl a
        asl a
        asl a
        sta varq
        lda varq
        sta z80_c
        lda vari
        sec
        sbc z80_c
        sta vari
        lda vari
        sta varo
        lda vari
        lsr a
        lsr a
        sta vari
        lda vari
        sta varq
        lda varq
        asl a
        asl a
        sta varq
        lda varq
        sta z80_c
        lda varo
        sec
        sbc z80_c
        sta varo
        lda varo
        sta varp
        lda varo
        lsr a
        sta varo
        lda varo
        asl a
        sta varo
        lda varo
        sta z80_c
        lda varp
        sec
        sbc z80_c
        sta varp
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b01084
:
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        ldy #8
        lda (z80_ix),y
        cmp vara
        bcc *+5
        jmp b00383
        lda #0
        cmp vark
        beq *+5
        jmp b00378
        lda #1
        sta vark
        lda #7
        sta varopt
        rts
b00378: jmp b01084
b00383: lda #255
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda vara
        ; ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp b00802
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        ldy #10
        lda (z80_ix),y
        sta varq
        lda #176
        ; ldy #10
        sta (z80_ix),y
        lda varp
        cmp #8
        bcc *+5
        jmp b00547
        lda #255
        sta z80_c
        dey; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp b00582
b00547: lda #2
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
b00582: lda #2
        cmp varo
        beq *+5
        jmp b00639
        lda #3
        sta z80_c
        ldy #10
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #10
        sta (z80_ix),y
        jmp b00675
b00639: lda #11
        sta z80_c
        ldy #10
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #10
        sta (z80_ix),y
b00675: lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda varq
        ldy #10
        sta (z80_ix),y
        lda varp
        cmp #8
        bcc *+5
        jmp b00766
        lda #2
        sta z80_c
        dey; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp b00802
b00766: lda #254
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
b00802: ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
        lda vara
        ; ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp b01084
        lda #11
        ldy #6
        sta (z80_ix),y
        lda #250
        ldy #12
        sta (z80_ix),y
        lda #1
        cmp vart
        beq *+5
        jmp b00910
        lda #220
        ; ldy #12
        sta (z80_ix),y
b00910: lda #2
        cmp vart
        beq *+5
        jmp b00940
        lda #180
        ldy #12
        sta (z80_ix),y
b00940: lda #3
        cmp vart
        beq *+5
        jmp b00971
        lda #140
        ldy #12
        sta (z80_ix),y
b00971: lda vart
        asl a
        asl a
        asl a
        asl a
        sta vart
        lda vari
        asl a
        sta vari
        lda vari
        sta z80_c
        lda vart
        clc
        adc z80_c
        sta vart
        lda vart
        clc
        adc #1
        sta vart
        lda vart
        ldy #11
        sta (z80_ix),y
        lda #7
        sta varopt
        lda #2
        ldy #5
        sta (z80_ix),y
        rts
b01084: lda #3
        sta varq
        lda #0
        cmp varo
        beq *+5
        jmp b01149
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp b01185
b01149: lda #240
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
b01185: lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b01301
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        ldy #9
        lda (z80_ix),y
        sta varq
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        ; ldy #9
        lda (z80_ix),y
        cmp varq
        bcc *+5
        jmp b01293
        lda #0
        sta varq
        jmp b01301
b01293: lda #2
        sta varq
b01301: lda #0
        cmp varo
        beq *+5
        jmp b01359
        lda #240
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp b01394
b01359: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
b01394: lda varq
        cmp #3
        bcc *+5
        jmp b01419
        lda varq
        sta varo
b01419: lda #0
        cmp varo
        beq *+5
        jmp b01650
        jsr cangr
        beq :+
        jmp b01637
:
        lda #2
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #0
        cmp vart
        bcc *+5
        jmp b01554
        jsr cangr
        beq :+
        jmp b01546
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
        jmp b01554
b01546: lda #2
        sta varo
b01554: lda #2
        cmp vart
        bcc *+5
        jmp b01632
        jsr cangr
        beq :+
        jmp b01624
:
        lda #2
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp b01632
b01624: lda #2
        sta varo
b01632: jmp b01645
b01637: lda #2
        sta varo
b01645: jmp b01869
b01650: jsr cangl
        beq :+
        jmp b01861
:
        lda #254
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #0
        cmp vart
        bcc *+5
        jmp b01778
        jsr cangl
        beq :+
        jmp b01770
:
        lda #255
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp b01778
b01770: lda #0
        sta varo
b01778: lda #2
        cmp vart
        bcc *+5
        jmp b01857
        jsr cangl
        beq :+
        jmp b01849
:
        lda #254
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp b01857
b01849: lda #0
        sta varo
b01857: jmp b01869
b01861: lda #0
        sta varo
b01869: lda #6
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp varo
        beq *+5
        jmp b01913
        lda #5
        ; ldy #6
        sta (z80_ix),y
b01913: lda #0
        cmp varc
        beq *+5
        jmp b02081
        lda #100
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #75
        cmp varrnd
        bcc *+5
        jmp b02016
        lda #1
        cmp varp
        beq *+5
        jmp b02008
        lda #0
        sta varp
        jmp b02016
b02008: lda #1
        sta varp
b02016: lda #30
        cmp varrnd
        bcc *+5
        jmp b02081
        lda vara
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp b02081
        lda #0
        cmp varp
        beq *+5
        jmp b02081
        lda #1
        sta varp
b02081: ldy #8
        lda (z80_ix),y
        cmp #4
        bcc *+5
        jmp b02111
        lda #0
        sta varp
b02111: jsr cangd
        beq :+
        jmp b02419
:
        ldy #6
        lda (z80_ix),y
        cmp #7
        bcc *+5
        jmp b02182
        lda #2
        sta z80_c
        ; ldy #6
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #6
        sta (z80_ix),y
b02182: lda #1
        ldy #7
        cmp (z80_ix),y
        bcc *+5
        jmp b02218
        lda #0
        ; ldy #7
        sta (z80_ix),y
b02218: lda #0
        cmp varp
        beq *+5
        jmp b02249
        ; lda #0
        ldy #7
        sta (z80_ix),y
b02249: lda #1
        cmp varc
        beq *+5
        jmp b02331
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp b02331
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
b02331: lda #3
        cmp varc
        beq *+5
        jmp b02414
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp b02414
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
b02414: jmp b02470
b02419: lda #0
        cmp varc
        beq *+5
        jmp b02445
        ; lda #0
        jsr animsp
b02445: lda #3
        cmp varc
        beq *+5
        jmp b02470
        lda #0
        jsr animsp
b02470: lda #1
        cmp varp
        beq *+5
        jmp b02918
        lda #0
        cmp varc
        beq *+5
        jmp b02694
        jsr cangu
        beq :+
        jmp b02606
:
        lda #2
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        jsr cangu
        beq :+
        jmp b02601
:
        lda #2
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
b02601: jmp b02685
b02606: jsr cangd
        beq :+
        jmp b02685
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp b02685
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
b02685: lda #0
        jsr animsp
b02694: lda #1
        cmp varc
        beq *+5
        jmp b02893
        jsr cangu
        beq :+
        jmp b02814
:
        lda #254
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jsr cangu
        beq :+
        jmp b02809
:
        lda #254
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
b02809: jmp b02893
b02814: jsr cangd
        beq :+
        jmp b02893
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp b02893
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
b02893: lda #2
        cmp varc
        beq *+5
        jmp b02918
        lda #0
        jsr animsp
b02918: ldy #8
        lda (z80_ix),y
        cmp #4
        bcc *+5
        jmp b02976
        lda #4
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
b02976: lda #238
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp b03012
        lda #6
        ; ldy #9
        sta (z80_ix),y
b03012: ldy #9
        lda (z80_ix),y
        cmp #6
        bcc *+5
        jmp b03049
        lda #238
        ; ldy #9
        sta (z80_ix),y
b03049: lda varp
        sta z80_c
        lda varo
        clc
        adc z80_c
        sta varo
        lda vari
        asl a
        asl a
        sta vari
        lda varo
        sta z80_c
        lda vari
        clc
        adc z80_c
        sta vari
        lda #1
        cmp vart
        beq *+5
        jmp b03130
b03130: lda #2
        cmp vart
        beq *+5
        jmp b03147
b03147: lda #3
        cmp vart
        beq *+5
        jmp b03164
b03164: lda vart
        asl a
        asl a
        asl a
        asl a
        asl a
        sta vart
        lda vari
        sta z80_c
        lda vart
        clc
        adc z80_c
        sta vart
        lda vart
        ldy #10
        sta (z80_ix),y
        rts
.out .sprintf("EVENT 1 SIZE = %d", (* - evnt01))

evnt02:
        lda #5
        cmp varj
        beq *+5
        jmp c00035
        lda #255
        ldy #5
        sta (z80_ix),y
        rts
c00035: lda #2
        ldy #12
        cmp (z80_ix),y
        bcc *+5
        jmp c00164
        lda #0
        cmp varc
        beq *+5
        jmp c00111
        lda #255
        sta z80_c
        ; ldy #12
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #12
        sta (z80_ix),y
c00111: lda #1
        cmp varc
        beq *+5
        jmp c00164
        lda #255
        sta z80_c
        ldy #12
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #12
        sta (z80_ix),y
c00164: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c00790
        lda #80
        ldy #12
        cmp (z80_ix),y
        bcc *+5
        jmp c00330
        ; lda #80
        ; ldy #12
        sta (z80_ix),y
        lda #5
        cmp varr
        bcc *+5
        jmp c00277
        lda #246
        sta z80_c
        ; ldy #12
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #12
        sta (z80_ix),y
c00277: lda #10
        cmp varr
        bcc *+5
        jmp c00330
        lda #246
        sta z80_c
        ldy #12
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #12
        sta (z80_ix),y
c00330: lda #0
        ldy #7
        sta (z80_ix),y
        ldy #12
        lda (z80_ix),y
        cmp #30
        bcc *+5
        jmp c00379
        lda #1
        ldy #7
        sta (z80_ix),y
c00379: ldy #12
        lda (z80_ix),y
        cmp #26
        bcc *+5
        jmp c00415
        lda #2
        ldy #7
        sta (z80_ix),y
c00415: ldy #12
        lda (z80_ix),y
        cmp #16
        bcc *+5
        jmp c00451
        lda #3
        ldy #7
        sta (z80_ix),y
c00451: lda #13
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c00761
        dey; ldy #11
        lda (z80_ix),y
        sta vars
        ; ldy #11
        lda (z80_ix),y
        sta vari
        lda vars
        lsr a
        lsr a
        lsr a
        lsr a
        sta vars
        lda vars
        sta varq
        lda varq
        asl a
        asl a
        asl a
        asl a
        sta varq
        lda varq
        sta z80_c
        lda vari
        sec
        sbc z80_c
        sta vari
        lda vari
        lsr a
        sta vari
        ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda vari
        jsr drpob
        ldy #8
        lda (z80_ix),y
        ldy #11
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y
        ldy #12
        sta (z80_ix),y
        lda #200
        ldy #8
        sta (z80_ix),y
        lda vars
        asl a
        asl a
        asl a
        asl a
        sta vars
        lda vari
        sta z80_c
        lda vars
        clc
        adc z80_c
        sta vars
        lda vars
        ldy #10
        sta (z80_ix),y
        lda #3
        ldy #5
        sta (z80_ix),y
        rts
c00761: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00787
:
        jmp c00790
c00787: rts
c00790: ldy #11
        lda (z80_ix),y
        sta vars
        ; ldy #11
        lda (z80_ix),y
        sta vari
        lda vars
        lsr a
        lsr a
        lsr a
        lsr a
        sta vars
        lda vars
        sta varq
        lda varq
        asl a
        asl a
        asl a
        asl a
        sta varq
        lda varq
        sta z80_c
        lda vari
        sec
        sbc z80_c
        sta vari
        lda vari
        sta vart
        lda vari
        lsr a
        sta vari
        lda vari
        sta varq
        lda varq
        asl a
        sta varq
        lda varq
        sta z80_c
        lda vart
        sec
        sbc z80_c
        sta vart
        ldy #8
        lda (z80_ix),y
        cmp #200
        bcc *+5
        jmp c02022
        lda #11
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        ldy #10
        lda (z80_ix),y
        sta varp
        ; ldy #10
        lda (z80_ix),y
        lsr a
        lsr a
        lsr a
        lsr a
        ; ldy #10
        sta (z80_ix),y
        ; ldy #10
        lda (z80_ix),y
        sta varq
        lda varq
        asl a
        asl a
        asl a
        asl a
        sta varq
        lda varq
        sta z80_c
        lda varp
        sec
        sbc z80_c
        sta varp
        lda #0
        sta varrnd
        lda #168
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp c01171
        lda #1
        sta varrnd
        lda #0
        sta vart
c01171: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c01444
:
        lda vara
        sta varq
        ldy #8
        lda (z80_ix),y
        cmp varq
        bcc *+5
        jmp c01259
        ; ldy #8
        lda (z80_ix),y
        sta z80_c
        lda varq
        sec
        sbc z80_c
        sta varq
        jmp c01298
c01259: ldy #8
        lda (z80_ix),y
        sta varq
        lda vara
        sta z80_c
        lda varq
        sec
        sbc z80_c
        sta varq
c01298: lda varq
        cmp #8
        bcc *+5
        jmp c01444
        lda varb
        sta varq
        ldy #9
        lda (z80_ix),y
        cmp varq
        bcc *+5
        jmp c01381
        ; ldy #9
        lda (z80_ix),y
        sta z80_c
        lda varq
        sec
        sbc z80_c
        sta varq
        jmp c01419
c01381: ldy #9
        lda (z80_ix),y
        sta varq
        lda varb
        sta z80_c
        lda varq
        sec
        sbc z80_c
        sta varq
c01419: lda varq
        cmp #8
        bcc *+5
        jmp c01444
        lda #1
        sta varrnd
c01444: lda #1
        cmp varrnd
        beq *+5
        jmp c02022
        lda #9
        sta varopt
        lda #<25
        sta z80_l
        lda #>25
        sta z80_h
        jsr addsc
        lda #0
        cmp vart
        beq *+5
        jmp c01561
        lda #8
        jsr getob
        ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        tya; lda #8
        jsr drpob
        jmp c01815
c01561: lda #<25
        sta z80_l
        lda #>25
        sta z80_h
        jsr addsc
        lda #0
        cmp vars
        beq *+5
        jmp c01645
        lda #9
        jsr getob
        ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #9
        jsr drpob
c01645: lda #1
        cmp vars
        beq *+5
        jmp c01730
        lda #10
        jsr getob
        ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #10
        jsr drpob
        lda #<25
        sta z80_l
        lda #>25
        sta z80_h
        jsr addsc
c01730: lda #1
        cmp vars
        bcc *+5
        jmp c01815
        lda #<75
        sta z80_l
        lda #>75
        sta z80_h
        jsr addsc
        lda #11
        jsr getob
        ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #11
        jsr drpob
c01815: lda #1
        sta varj
        lda #9
        sta varopt
        lda #50
        sta varf
        lda #30
        ldy #12
        sta (z80_ix),y
        lda vars
        dey; ldy #11
        sta (z80_ix),y
        ; ldy #11
        lda (z80_ix),y
        asl a
        asl a
        asl a
        asl a
        ; ldy #11
        sta (z80_ix),y
        lda vari
        sta z80_c
        ; ldy #11
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #11
        sta (z80_ix),y
        lda #200
        ldy #8
        sta (z80_ix),y
        lda varl
        sec
        sbc #1
        sta varl
        lda varm
        cmp varl
        bcc *+5
        jmp c02006
        tya; lda #8
        ldy #5
        sta (z80_ix),y
        jmp c02019
c02006: lda #255
        ldy #5
        sta (z80_ix),y
c02019: rts
c02022: lda #0
        sta varrnd
        ; lda #0
        cmp varc
        beq *+5
        jmp c02057
        lda #1
        sta varrnd
c02057: lda #2
        cmp varc
        beq *+5
        jmp c02082
        lda #1
        sta varrnd
c02082: lda #1
        cmp varrnd
        beq *+5
        jmp c02517
        ldy #10
        lda (z80_ix),y
        cmp #8
        bcc *+5
        jmp c02359
        lda #0
        sta varo
        lda #4
        sta loopa
c02138: lda varo
        ; ldy #10
        cmp (z80_ix),y
        bcc *+5
        jmp c02326
        jsr cangd
        beq :+
        jmp c02205
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
        jmp c02326
c02205: lda #8
        sta z80_c
        ldy #10
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #10
        sta (z80_ix),y
        tya; lda #10
        ; ldy #10
        cmp (z80_ix),y
        bcc *+5
        jmp c02285
        lda #7
        sta varopt
        lda #0
        sta vart
        rts
c02285: lda #9
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c02326
        lda #0
        ; ldy #10
        sta (z80_ix),y
        jmp c02326
c02326: lda varo
        clc
        adc #1
        sta varo
        dec loopa
        beq :+
        jmp c02138
:
        jmp c02517
c02359: lda #8
        sta varo
        lda #4
        sta loopa
c02376: lda varo
        ldy #10
        cmp (z80_ix),y
        bcc *+5
        jmp c02489
        jsr cangu
        beq :+
        jmp c02452
:
        lda #255
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp c02489
c02452: lda #248
        sta z80_c
        ldy #10
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #10
        sta (z80_ix),y
c02489: lda varo
        clc
        adc #1
        sta varo
        dec loopa
        beq :+
        jmp c02376
:
c02517: lda varp
        cmp #8
        bcc *+5
        jmp c02695
        lda #0
        sta varo
        lda #4
        sta loopa
c02550: lda varo
        cmp varp
        bcc *+5
        jmp c02662
        jsr cangr
        beq :+
        jmp c02612
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
        jmp c02662
c02612: lda #7
        sta z80_c
        lda varp
        clc
        adc z80_c
        sta varp
        lda #1
        cmp varp
        bcc *+5
        jmp c02662
        lda #11
        sta varopt
c02662: lda varo
        clc
        adc #1
        sta varo
        dec loopa
        beq :+
        jmp c02550
:
        jmp c03001
c02695: lda #8
        sta varo
        lda #4
        sta loopa
c02712: lda varo
        cmp varp
        bcc *+5
        jmp c02854
        jsr cangl
        beq :+
        jmp c02805
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #9
        sta (z80_ix),y
        tya; lda #9
        cmp varp
        bcc *+5
        jmp c02800
        lda #2
        ldy #7
        sta (z80_ix),y
c02800: jmp c02854
c02805: lda #9
        sta z80_c
        lda varp
        sec
        sbc z80_c
        sta varp
        lda #9
        cmp varp
        bcc *+5
        jmp c02854
        lda #11
        sta varopt
c02854: lda varo
        clc
        adc #1
        sta varo
        dec loopa
        beq :+
        jmp c02712
:
        lda #248
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #2
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c02965
:
        lda #249
        sta z80_c
        lda varp
        clc
        adc z80_c
        sta varp
c02965: lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
c03001: lda #0
        ldy #7
        sta (z80_ix),y
        ; lda #0
        cmp varp
        bcc *+5
        jmp c03321
        ; ldy #7
        lda (z80_ix),y
        sta varq
        lda #1
        cmp varp
        beq *+5
        jmp c03076
        lda #2
        ; ldy #7
        sta (z80_ix),y
c03076: lda #3
        cmp varp
        beq *+5
        jmp c03106
        lda #1
        ldy #7
        sta (z80_ix),y
c03106: lda #9
        cmp varp
        beq *+5
        jmp c03136
        lda #1
        ldy #7
        sta (z80_ix),y
c03136: lda #11
        cmp varp
        beq *+5
        jmp c03166
        lda #2
        ldy #7
        sta (z80_ix),y
c03166: lda #0
        cmp varc
        beq *+5
        jmp c03317
        jsr cangd
        beq :+
        jmp c03201
:
        jmp c03317
c03201: lda varp
        cmp #8
        bcc *+5
        jmp c03270
        ldy #9
        lda (z80_ix),y
        cmp #235
        bcc *+5
        jmp c03265
        lda #255
        sta z80_c
        lda varp
        clc
        adc z80_c
        sta varp
c03265: jmp c03317
c03270: lda #8
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp c03317
        lda #255
        sta z80_c
        lda varp
        clc
        adc z80_c
        sta varp
c03317: jmp c03423
c03321: jsr cangd
        beq :+
        jmp c03423
:
        lda varp
        clc
        adc #1
        sta varp
        lda #10
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #5
        cmp varrnd
        bcc *+5
        jmp c03423
        lda #8
        sta z80_c
        lda varp
        clc
        adc z80_c
        sta varp
c03423: lda #2
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c03704
:
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        ldy #9
        lda (z80_ix),y
        sta varq
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda varq
        ; ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp c03614
        lda #2
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #254
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp c03704
c03614: lda #254
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #2
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
c03704: ldy #9
        lda (z80_ix),y
        cmp #4
        bcc *+5
        jmp c03740
        lda #240
        ; ldy #9
        sta (z80_ix),y
c03740: lda #240
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp c03776
        lda #4
        ; ldy #9
        sta (z80_ix),y
c03776: ldy #8
        lda (z80_ix),y
        cmp #4
        bcc *+5
        jmp c03841
        tya; lda #8
        cmp varp
        bcc *+5
        jmp c03841
        lda #249
        sta z80_c
        lda varp
        clc
        adc z80_c
        sta varp
c03841: jsr cangd
        beq :+
        jmp c03984
:
        ldy #10
        lda (z80_ix),y
        cmp #4
        bcc *+5
        jmp c03925
        lda #0
        cmp varc
        beq *+5
        jmp c03921
        ; ldy #10
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #10
        sta (z80_ix),y
c03921: jmp c03984
c03925: lda #8
        ldy #10
        cmp (z80_ix),y
        bcc *+5
        jmp c03984
        lda #255
        sta z80_c
        ; ldy #10
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #10
        sta (z80_ix),y
c03984: lda #8
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c04021
        lda #0
        ; ldy #10
        sta (z80_ix),y
c04021: lda #8
        cmp varp
        beq *+5
        jmp c04045
        lda #0
        sta varp
c04045: ldy #10
        lda (z80_ix),y
        asl a
        asl a
        asl a
        asl a
        ; ldy #10
        sta (z80_ix),y
        lda varp
        sta z80_c
        ; ldy #10
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #10
        sta (z80_ix),y
        lda vars
        asl a
        asl a
        asl a
        asl a
        sta vars
        lda vari
        asl a
        sta vari
        lda vart
        sta z80_c
        lda vari
        clc
        adc z80_c
        sta vari
        lda vari
        sta z80_c
        lda vars
        clc
        adc z80_c
        sta vars
        lda vars
        iny; ldy #11
        sta (z80_ix),y
        rts
.out .sprintf("EVENT 2 SIZE = %d", (* - evnt02))

evnt03:
        lda #5
        cmp varj
        beq *+5
        jmp d00035
        lda #255
        ldy #5
        sta (z80_ix),y
        rts
d00035: lda #255
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp d00406
        lda #120
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp d00135
        lda #4
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        tya; lda #9
        ldy #6
        sta (z80_ix),y
        jmp d00184
d00135: lda #4
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #10
        ldy #6
        sta (z80_ix),y
d00184: lda #0
        cmp varc
        beq *+5
        jmp d00210
        ; lda #0
        jsr animsp
d00210: lda #0
        sta varrnd
        lda #240
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp d00249
        lda #1
        sta varrnd
d00249: ldy #9
        lda (z80_ix),y
        cmp #8
        bcc *+5
        jmp d00280
        lda #1
        sta varrnd
d00280: lda #1
        cmp varrnd
        beq *+5
        jmp d00401
        lda #200
        ldy #8
        sta (z80_ix),y
        lda #255
        sta z80_c
        lda varl
        clc
        adc z80_c
        sta varl
        lda varm
        cmp varl
        bcc *+5
        jmp d00385
        lda #40
        ldy #12
        sta (z80_ix),y
        lda #8
        ldy #5
        sta (z80_ix),y
        jmp d00398
d00385: lda #255
        ldy #5
        sta (z80_ix),y
d00398: rts
d00401: jmp d03656
d00406: lda #0
        sta varo
        jsr skobj
        sta varobj
        lda #13
        cmp varobj
        bcc *+5
        jmp d00449
        lda #1
        sta varo
d00449: lda varobj
        cmp #19
        bcc *+5
        jmp d00482
        lda varo
        clc
        adc #1
        sta varo
d00482: lda #2
        cmp varo
        beq *+5
        jmp d00498
d00498: ldy #8
        lda (z80_ix),y
        lsr a
        ; ldy #8
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        asl a
        ; ldy #8
        sta (z80_ix),y
        lda #200
        ; ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp d00822
        lda #60
        ; ldy #8
        sta (z80_ix),y
        lda #10
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #6
        cmp varrnd
        bcc *+5
        jmp d00645
        lda #120
        ; ldy #8
        sta (z80_ix),y
d00645: lda #4
        ldy #9
        sta (z80_ix),y
        ldy #12
        lda (z80_ix),y
        cmp #120
        bcc *+5
        jmp d00694
        lda #240
        ldy #9
        sta (z80_ix),y
d00694: ldy #10
        lda (z80_ix),y
        sta vars
        ; ldy #10
        lda (z80_ix),y
        sta vari
        lda #0
        sta varo
        ; lda #0
        sta varp
        lda vars
        lsr a
        lsr a
        lsr a
        lsr a
        sta vars
        lda vars
        sta varq
        lda varq
        asl a
        asl a
        asl a
        asl a
        sta varq
        lda varq
        sta z80_c
        lda vari
        sec
        sbc z80_c
        sta vari
        jmp d03526
d00822: ldy #10
        lda (z80_ix),y
        sta vars
        ; ldy #10
        lda (z80_ix),y
        sta vari
        lda vars
        lsr a
        lsr a
        lsr a
        lsr a
        lsr a
        sta vars
        lda vars
        sta varq
        lda varq
        asl a
        asl a
        asl a
        asl a
        asl a
        sta varq
        lda varq
        sta z80_c
        lda vari
        sec
        sbc z80_c
        sta vari
        lda vari
        sta varo
        lda vari
        lsr a
        lsr a
        sta vari
        lda vari
        sta varq
        lda varq
        asl a
        asl a
        sta varq
        lda varq
        sta z80_c
        lda varo
        sec
        sbc z80_c
        sta varo
        lda varo
        sta varp
        lda varo
        lsr a
        sta varo
        lda varo
        asl a
        sta varo
        lda varo
        sta z80_c
        lda varp
        sec
        sbc z80_c
        sta varp
        lda vara
        sta varq
        iny; ldy #11
        lda (z80_ix),y
        cmp vara
        bcc *+5
        jmp d01134
        ; ldy #11
        lda (z80_ix),y
        sta z80_c
        lda varq
        sec
        sbc z80_c
        sta varq
        jmp d01173
d01134: ldy #11
        lda (z80_ix),y
        sta varq
        lda vara
        sta z80_c
        lda varq
        sec
        sbc z80_c
        sta varq
d01173: lda varq
        cmp #6
        bcc *+5
        jmp d01700
        lda varb
        sta varq
        ldy #12
        lda (z80_ix),y
        cmp varb
        bcc *+5
        jmp d01256
        ; ldy #12
        lda (z80_ix),y
        sta z80_c
        lda varq
        sec
        sbc z80_c
        sta varq
        jmp d01295
d01256: ldy #12
        lda (z80_ix),y
        sta varq
        lda varb
        sta z80_c
        lda varq
        sec
        sbc z80_c
        sta varq
d01295: lda varq
        cmp #8
        bcc *+5
        jmp d01700
        lda #9
        sta varopt
        ldy #8
        lda (z80_ix),y
        sta varrnd
        iny; ldy #9
        lda (z80_ix),y
        sta varobj
        ldy #11
        lda (z80_ix),y
        ldy #8
        sta (z80_ix),y
        ldy #12
        lda (z80_ix),y
        ldy #9
        sta (z80_ix),y
        lda #<25
        sta z80_l
        lda #>25
        sta z80_h
        jsr addsc
        lda #0
        cmp vars
        bcc *+5
        jmp d01472
        lda vars
        sta loopa
d01436: lda #<25
        sta z80_l
        lda #>25
        sta z80_h
        jsr addsc
        dec loopa
        beq :+
        jmp d01436
:
d01472: lda #8
        sta z80_c
        lda vars
        clc
        adc z80_c
        sta vars
        lda vars
        jsr getob
        ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda vars
        jsr drpob
        lda #8
        sta z80_c
        lda vars
        sec
        sbc z80_c
        sta vars
        lda #1
        sta varj
        lda vari
        jsr getob
        lda #50
        sta varf
        lda vars
        asl a
        asl a
        asl a
        asl a
        sta vars
        lda vari
        sta z80_c
        lda vars
        clc
        adc z80_c
        sta vars
        lda vars
        ldy #11
        sta (z80_ix),y
        lda varrnd
        ldy #8
        sta (z80_ix),y
        lda varobj
        iny; ldy #9
        sta (z80_ix),y
        lda #255
        ldy #12
        sta (z80_ix),y
        rts
d01700: ldy #8
        lda (z80_ix),y
        sta varq
        ldy #11
        lda (z80_ix),y
        sta varrnd
        iny; ldy #12
        lda (z80_ix),y
        sta varobj
        lda varrnd
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp d01797
        lda varrnd
        sta z80_c
        lda varq
        sec
        sbc z80_c
        sta varq
        jmp d01836
d01797: lda varrnd
        sta varq
        ldy #8
        lda (z80_ix),y
        sta z80_c
        lda varq
        sec
        sbc z80_c
        sta varq
d01836: lda varq
        cmp #6
        bcc *+5
        jmp d02121
        ldy #9
        lda (z80_ix),y
        sta varq
        lda varobj
        ; ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp d01920
        lda varobj
        sta z80_c
        lda varq
        sec
        sbc z80_c
        sta varq
        jmp d01959
d01920: lda varobj
        sta varq
        ldy #9
        lda (z80_ix),y
        sta z80_c
        lda varq
        sec
        sbc z80_c
        sta varq
d01959: lda varq
        cmp #8
        bcc *+5
        jmp d02121
        lda vari
        jsr getob
        lda vars
        clc
        adc #1
        sta vars
        lda #4
        cmp vars
        beq *+5
        jmp d02024
        lda #3
        sta vars
d02024: lda vars
        asl a
        asl a
        asl a
        asl a
        asl a
        sta vars
        lda vari
        asl a
        asl a
        sta vari
        lda vari
        sta z80_c
        lda vars
        clc
        adc z80_c
        sta vars
        lda vars
        ldy #10
        sta (z80_ix),y
        lda #1
        ldy #5
        sta (z80_ix),y
        rts
d02121: lda #2
        cmp varo
        beq *+5
        jmp d02297
        jsr cangr
        beq :+
        jmp d02178
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp d02297
d02178: lda #254
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jsr cangr
        beq :+
        jmp d02254
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp d02262
d02254: lda #0
        sta varo
d02262: lda #2
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
d02297: lda #0
        cmp varo
        beq *+5
        jmp d02618
        lda #2
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp d02418
:
        lda #10
        ldy #6
        sta (z80_ix),y
        lda #1
        iny; ldy #7
        cmp (z80_ix),y
        bcc *+5
        jmp d02413
        lda #0
        ; ldy #7
        sta (z80_ix),y
d02413: jmp d02431
d02418: lda #10
        ldy #6
        sta (z80_ix),y
d02431: lda #254
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jsr cangl
        beq :+
        jmp d02507
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp d02613
d02507: lda #254
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jsr cangl
        beq :+
        jmp d02583
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp d02591
d02583: lda #2
        sta varo
d02591: ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
d02613: jmp d02743
d02618: ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp d02672
:
        lda #9
        ldy #6
        sta (z80_ix),y
        jmp d02685
d02672: lda #9
        ldy #6
        sta (z80_ix),y
d02685: lda #1
        ldy #7
        cmp (z80_ix),y
        bcc *+5
        jmp d02721
        lda #0
        ; ldy #7
        sta (z80_ix),y
d02721: ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
d02743: ldy #8
        lda (z80_ix),y
        ldy #11
        cmp (z80_ix),y
        bne *+5
        jmp d03003
        ldy #8
        lda (z80_ix),y
        ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp d02839
        jsr cangd
        beq :+
        jmp d02834
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
d02834: jmp d03003
d02839: jsr cangu
        beq :+
        jmp d03003
:
        lda #0
        cmp varc
        beq *+5
        jmp d02928
        lda #8
        ldy #6
        cmp (z80_ix),y
        bcc *+5
        jmp d02906
        lda #1
        iny; ldy #7
        sta (z80_ix),y
d02906: ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
d02928: lda #2
        cmp varc
        beq *+5
        jmp d03003
        lda #8
        ldy #6
        cmp (z80_ix),y
        bcc *+5
        jmp d02981
        lda #0
        iny; ldy #7
        sta (z80_ix),y
d02981: ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
d03003: ldy #8
        lda (z80_ix),y
        cmp #150
        bcc *+5
        jmp d03250
        lda #0
        cmp varc
        beq *+5
        jmp d03250
        ; ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp d03228
:
        lda #2
        cmp varo
        beq *+5
        jmp d03150
        lda #9
        ldy #6
        cmp (z80_ix),y
        bne *+5
        jmp d03146
        ; lda #9
        ; ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
d03146: jmp d03202
d03150: lda #10
        ldy #6
        cmp (z80_ix),y
        bne *+5
        jmp d03202
        ; lda #10
        ; ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
d03202: ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
d03228: ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
d03250: ldy #6
        lda (z80_ix),y
        cmp #9
        bcc *+5
        jmp d03323
        lda #0
        cmp varc
        beq *+5
        jmp d03298
        ; lda #0
        jsr animsp
d03298: lda #2
        cmp varc
        beq *+5
        jmp d03323
        lda #0
        jsr animsp
d03323: lda #240
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp d03359
        lda #4
        ; ldy #9
        sta (z80_ix),y
d03359: ldy #9
        lda (z80_ix),y
        cmp #4
        bcc *+5
        jmp d03395
        lda #240
        ; ldy #9
        sta (z80_ix),y
d03395: ldy #8
        lda (z80_ix),y
        sta varq
        iny; ldy #9
        lda (z80_ix),y
        sta varrnd
        ldy #11
        lda (z80_ix),y
        ldy #8
        sta (z80_ix),y
        ldy #12
        lda (z80_ix),y
        ldy #9
        sta (z80_ix),y
        lda #1
        cmp vars
        beq *+5
        jmp d03480
d03480: lda #1
        cmp vars
        bcc *+5
        jmp d03497
d03497: lda varq
        ldy #8
        sta (z80_ix),y
        lda varrnd
        iny; ldy #9
        sta (z80_ix),y
d03526: lda vari
        asl a
        asl a
        sta vari
        lda varp
        sta z80_c
        lda varo
        clc
        adc z80_c
        sta varo
        lda varo
        sta z80_c
        lda vari
        clc
        adc z80_c
        sta vari
        lda vars
        asl a
        asl a
        asl a
        asl a
        asl a
        sta vars
        lda vari
        sta z80_c
        lda vars
        clc
        adc z80_c
        sta vars
        lda vars
        ldy #10
        sta (z80_ix),y
d03656: rts
.out .sprintf("EVENT 3 SIZE = %d", (* - evnt03))

evnt04:
        lda #5
        cmp varj
        beq *+5
        jmp e00069
        lda #15
        jsr getob
        lda #16
        jsr getob
        lda #17
        jsr getob
        lda #18
        jsr getob
        lda #255
        ldy #5
        sta (z80_ix),y
        rts
e00069: lda #13
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00360
        lda #15
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #15
        jsr getob
        lda #16
        jsr getob
        lda #17
        jsr getob
        lda #18
        jsr getob
        lda vara
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp e00286
        lda varc
        cmp #2
        bcc *+5
        jmp e00244
        iny; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #15
        jsr drpob
        jmp e00282
e00244: ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #16
        jsr drpob
e00282: jmp e00324
e00286: ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #16
        jsr drpob
e00324: lda #241
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
e00360: lda #14
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00651
        lda #241
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #15
        jsr getob
        lda #16
        jsr getob
        lda #17
        jsr getob
        lda #18
        jsr getob
        lda vara
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp e00577
        lda varc
        cmp #2
        bcc *+5
        jmp e00535
        iny; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #17
        jsr drpob
        jmp e00573
e00535: ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #18
        jsr drpob
e00573: jmp e00615
e00577: ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #18
        jsr drpob
e00615: lda #15
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
e00651: ldy #11
        lda (z80_ix),y
        cmp varr
        bcc *+5
        jmp e00887
        lda #13
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00792
        lda #252
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        cmp #4
        bcc *+5
        jmp e00788
        lda #15
        jsr getob
        lda #16
        jsr getob
        lda #255
        ldy #5
        sta (z80_ix),y
        rts
e00788: jmp e00884
e00792: lda #4
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #240
        ; ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp e00884
        lda #17
        jsr getob
        lda #18
        jsr getob
        lda #255
        ldy #5
        sta (z80_ix),y
        rts
e00884: rts
e00887: lda #1
        ldy #12
        cmp (z80_ix),y
        bcc *+5
        jmp e01182
        lda #255
        sta z80_c
        ; ldy #12
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #12
        sta (z80_ix),y
        ; ldy #12
        lda (z80_ix),y
        sta varo
        lda varo
        sta z80_d
        lda #10
        sta z80_e
        jsr idiv
        lda z80_d
        sta varo
        lda #13
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e01027
        lda #16
        sta varrnd
        jmp e01036
e01027: lda #240
        sta varrnd
e01036: lda varrnd
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda varrnd
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #1
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp e01179
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #15
        jsr getob
        lda #16
        jsr getob
        lda #17
        jsr getob
        lda #18
        jsr getob
e01179: rts
e01182: ldy #8
        lda (z80_ix),y
        sta varq
        lda vara
        ; ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp e01248
        lda vara
        sta z80_c
        lda varq
        sec
        sbc z80_c
        sta varq
        jmp e01287
e01248: lda vara
        sta varq
        ldy #8
        lda (z80_ix),y
        sta z80_c
        lda varq
        sec
        sbc z80_c
        sta varq
e01287: ldy #9
        lda (z80_ix),y
        sta vart
        lda varb
        ; ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp e01353
        lda varb
        sta z80_c
        lda vart
        sec
        sbc z80_c
        sta vart
        jmp e01391
e01353: lda varb
        sta vart
        ldy #9
        lda (z80_ix),y
        sta z80_c
        lda vart
        sec
        sbc z80_c
        sta vart
e01391: lda #0
        cmp vark
        beq *+5
        jmp e01510
        ; lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e01510
:
        lda #0
        cmp varq
        beq *+5
        jmp e01477
        lda #70
        ldy #12
        sta (z80_ix),y
        lda #8
        sta varopt
        rts
        jmp e01510
e01477: lda #1
        sta vark
        lda #0
        ldy #11
        sta (z80_ix),y
        ; lda #0
        sta vard
        rts
e01510: lda #0
        sta vart
        lda varq
        cmp #12
        bcc *+5
        jmp e01560
        lda vart
        cmp #80
        bcc *+5
        jmp e01560
        lda #1
        sta vart
e01560: lda #0
        ldy #7
        sta (z80_ix),y
        lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e01954
        jsr cangr
        beq :+
        jmp e01856
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
        lda #1
        cmp vart
        beq *+5
        jmp e01852
        ; ldy #9
        lda (z80_ix),y
        cmp varb
        bcc *+5
        jmp e01852
        jsr cangr
        beq :+
        jmp e01775
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
        lda #100
        sta varobj
        lda vart
        sta z80_c
        lda varobj
        sec
        sbc z80_c
        sta varobj
        lda #0
        cmp varc
        beq *+5
        jmp e01775
        lda #12
        sta varopt
e01775: jsr cangr
        beq :+
        jmp e01815
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
e01815: lda #14
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e01852
        lda #1
        iny; ldy #7
        sta (z80_ix),y
e01852: jmp e01950
e01856: lda #2
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jsr cangr
        beq :+
        jmp e01936
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
        jmp e01950
e01936: lda #0
        ldy #10
        sta (z80_ix),y
e01950: jmp e02333
e01954: jsr cangl
        beq :+
        jmp e02244
:
        lda #255
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #1
        cmp vart
        beq *+5
        jmp e02239
        lda varb
        ; ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp e02239
        jsr cangl
        beq :+
        jmp e02153
:
        lda #255
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #100
        sta varobj
        lda vart
        sta z80_c
        lda varobj
        sec
        sbc z80_c
        sta varobj
        lda #0
        cmp varc
        beq *+5
        jmp e02153
        lda #12
        sta varopt
e02153: jsr cangl
        beq :+
        jmp e02202
:
        lda #255
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
e02202: lda #13
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e02239
        lda #1
        iny; ldy #7
        sta (z80_ix),y
e02239: jmp e02333
e02244: ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jsr cangl
        beq :+
        jmp e02320
:
        lda #255
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp e02333
e02320: lda #1
        ldy #10
        sta (z80_ix),y
e02333: ldy #8
        lda (z80_ix),y
        cmp vara
        bcc *+5
        jmp e02714
        lda #0
        sta varobj
        jsr cangd
        beq :+
        jmp e02592
:
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e02499
        lda #240
        sta z80_c
        dey; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp e02459
:
        lda #1
        sta varobj
e02459: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp e02592
e02499: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp e02557
:
        lda #1
        sta varobj
e02557: lda #240
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
e02592: lda #1
        cmp varobj
        beq *+5
        jmp e02709
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
        lda #14
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e02673
        lda #0
        iny; ldy #7
        sta (z80_ix),y
e02673: lda #16
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e02709
        lda #0
        iny; ldy #7
        sta (z80_ix),y
e02709: jmp e03020
e02714: lda vara
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp e03020
        lda #0
        sta varobj
        jsr cangu
        beq :+
        jmp e02967
:
        lda #0
        cmp varp
        beq *+5
        jmp e02873
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jsr cangu
        beq :+
        jmp e02833
:
        lda #1
        sta varobj
e02833: lda #240
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp e02967
e02873: lda #240
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jsr cangu
        beq :+
        jmp e02931
:
        lda #1
        sta varobj
e02931: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
e02967: lda #1
        cmp varobj
        beq *+5
        jmp e03020
        lda #255
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
e03020: ldy #8
        lda (z80_ix),y
        cmp #4
        bcc *+5
        jmp e03068
        ; ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
e03068: lda #150
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp e03127
        lda #255
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
e03127: ldy #9
        lda (z80_ix),y
        cmp #4
        bcc *+5
        jmp e03163
        lda #220
        ; ldy #9
        sta (z80_ix),y
e03163: lda #232
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp e03200
        lda #16
        ; ldy #9
        sta (z80_ix),y
e03200: lda #100
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp e03304
        ldy #9
        lda (z80_ix),y
        cmp varb
        bcc *+5
        jmp e03291
        lda #1
        iny; ldy #10
        sta (z80_ix),y
        jmp e03304
e03291: lda #0
        ldy #10
        sta (z80_ix),y
e03304: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e03367
        lda #13
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e03363
        lda #14
        ; ldy #6
        sta (z80_ix),y
e03363: jmp e03404
e03367: lda #14
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e03404
        lda #13
        ; ldy #6
        sta (z80_ix),y
e03404: lda #14
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e03440
        lda #240
        sta varrnd
        jmp e03448
e03440: lda #16
        sta varrnd
e03448: lda varrnd
        sta z80_c
        ldy #6
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #6
        sta (z80_ix),y
        lda varrnd
        sta z80_c
        ; ldy #6
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #6
        sta (z80_ix),y
        rts
.out .sprintf("EVENT 4 SIZE = %d", (* - evnt04))

evnt05:
        rts
.out .sprintf("EVENT 5 SIZE = %d", (* - evnt05))

evnt06:
        lda #175
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp g00058
        lda #0
        dey; ldy #7
        sta (z80_ix),y
        lda #15
        dey; ldy #6
        sta (z80_ix),y
        jmp g00072
g00058: lda #12
        ldy #6
        sta (z80_ix),y
g00072: lda #12
        cmp vard
        beq *+5
        jmp g00243
        lda #0
        ldy #9
        sta (z80_ix),y
        lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp g00164
        lda #240
        ; ldy #9
        sta (z80_ix),y
g00164: lda #140
        ldy #8
        sta (z80_ix),y
        lda #4
        sta z80_c
        lda #13
        sta z80_b
        jsr spawn
        lda vard
        clc
        adc #1
        sta vard
        lda #176
        ldy #8
        sta (z80_ix),y
        lda #0
        iny; ldy #9
        sta (z80_ix),y
g00243: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp g00349
        ldy #8
        lda (z80_ix),y
        ldy #12
        sta (z80_ix),y
        lda #176
        ldy #8
        sta (z80_ix),y
        jsr cangu
        beq :+
        jmp g00325
:
        lda #1
        ldy #10
        sta (z80_ix),y
g00325: ldy #12
        lda (z80_ix),y
        ldy #8
        sta (z80_ix),y
        jmp g01482
g00349: lda #0
        cmp varc
        beq *+5
        jmp g00374
        lda #12
        jsr getob
g00374: lda #100
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #95
        cmp varrnd
        bcc *+5
        jmp g00728
        lda #12
        jsr getob
        lda #40
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        sta varo
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
        jmp g00545
        lda #186
        sta z80_c
        lda varo
        clc
        adc z80_c
        sta varo
g00545: lda #8
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #170
        sta z80_c
        lda varrnd
        clc
        adc z80_c
        sta varrnd
        ldy #8
        lda (z80_ix),y
        sta vars
        iny; ldy #9
        lda (z80_ix),y
        sta vart
        lda varo
        ; ldy #9
        sta (z80_ix),y
        lda varrnd
        dey; ldy #8
        sta (z80_ix),y
        iny; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #12
        jsr drpob
        lda vars
        ldy #8
        sta (z80_ix),y
        lda vart
        iny; ldy #9
        sta (z80_ix),y
g00728: lda #0
        sta varobj
        lda #130
        cmp vara
        bcc *+5
        jmp g00923
        lda #0
        sta varrnd
        lda varb
        cmp #52
        bcc *+5
        jmp g00788
        lda #1
        sta varrnd
g00788: lda #200
        cmp varb
        bcc *+5
        jmp g00814
        lda #1
        sta varrnd
g00814: lda #1
        cmp varrnd
        beq *+5
        jmp g00909
        lda varb
        ldy #9
        sta (z80_ix),y
        lda #160
        dey; ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp g00905
        lda #254
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
g00905: jmp g00918
g00909: lda #1
        sta varobj
g00918: jmp g00931
g00923: lda #1
        sta varobj
g00931: lda #0
        cmp vark
        bcc *+5
        jmp g00956
        lda #1
        sta varobj
g00956: lda #1
        cmp varobj
        beq *+5
        jmp g01032
        ldy #8
        lda (z80_ix),y
        cmp #190
        bcc *+5
        jmp g01032
        lda #2
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
g01032: ldy #8
        lda (z80_ix),y
        cmp #162
        bcc *+5
        jmp g01177
        lda #15
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #19
        jsr getob
        ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #19
        jsr drpob
        lda #15
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp g01185
g01177: lda #19
        jsr getob
g01185: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g01455
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #165
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp g01292
        lda #1
        sta vark
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #176
        ; ldy #8
        sta (z80_ix),y
        rts
g01292: lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #1
        ldy #7
        sta (z80_ix),y
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp g01415
        lda #1
        ; ldy #11
        sta (z80_ix),y
        lda vara
        ldy #8
        sta (z80_ix),y
        lda #248
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp g01450
g01415: lda #2
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
g01450: jmp g01482
g01455: lda #0
        ldy #7
        sta (z80_ix),y
        ; lda #0
        ldy #11
        sta (z80_ix),y
g01482: rts
.out .sprintf("EVENT 6 SIZE = %d", (* - evnt06))

evnt07:
        lda #0
        ldy #7
        sta (z80_ix),y
        ; lda #0
        cmp scno
        beq *+5
        jmp h03404
        lda varr
        sta varo
        lda varr
        sta varp
        lda varo
        sta z80_d
        lda #5
        sta z80_e
        jsr idiv
        lda z80_d
        sta varo
        lda varo
        sta z80_d
        asl a
        asl a
        clc
        adc z80_d
        sta varo
        lda varo
        sta z80_c
        lda varp
        sec
        sbc z80_c
        sta varp
        lda #0
        ldy #12
        cmp (z80_ix),y
        bcc *+5
        jmp h00190
        lda #255
        sta z80_c
        ; ldy #12
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #12
        sta (z80_ix),y
h00190: lda #249
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp h00235
        lda #200
        ldy #8
        sta (z80_ix),y
        lda #0
        sta vard
h00235: lda #21
        sta chary
        lda #0
        sta charx
        lda #3
        cmp varr
        beq *+5
        jmp h00574
        lda #255
        sta varrnd
        lda #240
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp h00309
        lda #22
        sta varrnd
h00309: lda #235
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp h00340
        lda #23
        sta varrnd
h00340: lda #230
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp h00372
        lda #24
        sta varrnd
h00372: lda #225
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp h00403
        lda #0
        sta varrnd
h00403: lda varrnd
        cmp #255
        bcc *+5
        jmp h00574
        lda #7
        sta loopa
h00429: lda charx
        sta dispx
        lda chary
        sta dispy
        lda varrnd
        jsr pattr
        lda #24
        sta z80_c
        lda charx
        clc
        adc z80_c
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda varrnd
        jsr pattr
        lda #25
        sta z80_c
        lda charx
        sec
        sbc z80_c
        sta charx
        dec loopa
        beq :+
        jmp h00429
:
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda varrnd
        jsr pattr
h00574: lda #5
        sta chary
        lda #10
        sta charx
        lda #250
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp h00797
        lda #0
        cmp varp
        beq *+5
        jmp h00797
        lda #9
        sta varl
        lda #8
        sta varm
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
        lda #9
        jsr pattr
        lda #3
        sta varq
        lda #6
        sta loopa
h00715: lda charx
        sta dispx
        lda chary
        sta dispy
        lda varq
        jsr pattr
        lda varq
        clc
        adc #1
        sta varq
        dec loopa
        beq :+
        jmp h00715
:
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #2
        jsr pattr
h00797: lda #100
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp h02151
        lda #1
        sta varj
        lda #0
        cmp varp
        beq *+5
        jmp h01704
        lda #24
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        sta vars
        lda vart
        sta varq
        lda varq
        lsr a
        lsr a
        lsr a
        lsr a
        sta varq
        lda varq
        asl a
        asl a
        asl a
        asl a
        sta varq
        lda varq
        sta z80_c
        lda vart
        sec
        sbc z80_c
        sta vart
        lda varq
        lsr a
        sta varq
        lda #120
        cmp varq
        beq *+5
        jmp h01001
        lda #152
        sta varq
h01001: lda vart
        asl a
        asl a
        asl a
        asl a
        sta vart
        ldy #8
        lda (z80_ix),y
        sta vars
        iny; ldy #9
        lda (z80_ix),y
        sta varrnd
        lda varq
        dey; ldy #8
        sta (z80_ix),y
        lda vart
        iny; ldy #9
        sta (z80_ix),y
        lda #2
        sta z80_b
        jsr sktyp
        bcs :+
        jmp h01182
:
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        ldy #12
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #12
        sta (z80_ix),y
        lda vars
        ldy #8
        sta (z80_ix),y
        lda varrnd
        iny; ldy #9
        sta (z80_ix),y
        rts
h01182: ldy #10
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #10
        sta (z80_ix),y
        lda varr
        sta varo
        lda #15
        cmp varo
        bcc *+5
        jmp h01243
        ; lda #15
        sta varo
h01243: lda varo
        sta z80_d
        lda #5
        sta z80_e
        jsr idiv
        lda z80_d
        sta varo
        lda varo
        sec
        sbc #1
        sta varo
        lda varo
        asl a
        asl a
        asl a
        asl a
        sta varo
        lda #15
        sta z80_c
        ldy #10
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #10
        sta (z80_ix),y
        ; ldy #10
        lda (z80_ix),y
        sta z80_c
        lda varo
        clc
        adc z80_c
        sta varo
        ; ldy #10
        lda (z80_ix),y
        sta z80_c
        lda varo
        clc
        adc z80_c
        sta varo
        lda #15
        sta z80_c
        ; ldy #10
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #10
        sta (z80_ix),y
        lda #2
        sta z80_c
        lda #11
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda varo
        ldy #11
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        dey; ldy #10
        lda (z80_ix),y
        cmp #22
        bcc *+5
        jmp h01567
        ldy #12
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #12
        sta (z80_ix),y
h01567: rts
        ldy #10
        lda (z80_ix),y
        cmp #24
        bcc *+5
        jmp h01657
        ldy #12
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #12
        sta (z80_ix),y
        lda vars
        ldy #8
        sta (z80_ix),y
        lda varrnd
        iny; ldy #9
        sta (z80_ix),y
        rts
        jmp h01670
h01657: lda #200
        ldy #8
        sta (z80_ix),y
h01670: lda vars
        ldy #8
        sta (z80_ix),y
        lda varrnd
        iny; ldy #9
        sta (z80_ix),y
        jmp h02151
h01704: lda #1
        sta vari
        lda #60
        sta varrnd
        lda varp
        sta varm
        lda varm
        clc
        adc #1
        sta varm
        lda #3
        sta varl
        lda varp
        sta z80_c
        lda varl
        clc
        adc z80_c
        sta varl
        lda varm
        sta loopa
h01786: lda #0
        sta vars
        lda #5
        cmp varr
        bcc *+5
        jmp h01819
        lda #1
        sta vars
h01819: lda #10
        cmp varr
        bcc *+5
        jmp h01844
        lda #2
        sta vars
h01844: lda #15
        cmp varr
        bcc *+5
        jmp h01874
        lda #3
        sta vars
        jmp h01906
h01874: lda #4
        cmp vari
        bcc *+5
        jmp h01906
        lda vars
        clc
        adc #1
        sta vars
h01906: lda #176
        ldy #8
        sta (z80_ix),y
        lda #0
        iny; ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        lda #15
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #80
        ldy #12
        sta (z80_ix),y
        lda #200
        ldy #8
        sta (z80_ix),y
        lda vars
        asl a
        asl a
        asl a
        asl a
        sta vars
        lda vari
        sta z80_c
        lda vars
        clc
        adc z80_c
        sta vars
        lda vars
        ldy #11
        sta (z80_ix),y
        lda vari
        clc
        adc #1
        sta vari
        lda varrnd
        iny; ldy #12
        sta (z80_ix),y
        lda #25
        sta z80_c
        lda varrnd
        clc
        adc z80_c
        sta varrnd
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        dec loopa
        beq :+
        jmp h01786
:
h02151: lda #3
        cmp varp
        beq *+5
        jmp h02676
        lda #220
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp h02292
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #24
        jsr pattr
        lda #7
        sta loopa
h02225: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #22
        jsr pattr
        dec loopa
        beq :+
        jmp h02225
:
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #24
        jsr pattr
h02292: lda #215
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp h02415
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        lda #7
        sta loopa
h02349: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #23
        jsr pattr
        dec loopa
        beq :+
        jmp h02349
:
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
h02415: lda #210
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp h02513
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        lda #7
        sta loopa
h02473: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #24
        jsr pattr
        dec loopa
        beq :+
        jmp h02473
:
h02513: lda #205
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp h02583
        lda #8
        sta loopa
h02544: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        dec loopa
        beq :+
        jmp h02544
:
h02583: lda #203
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp h02676
        lda #0
        sta chary
        ; lda #0
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
h02676: lda #200
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp h02774
        lda #22
        sta chary
        lda #10
        sta charx
        lda #7
        jsr dmsg
        lda #22
        sta chary
        lda #13
        sta charx
        lda #3
        jsr dmsg
        lda #22
        sta chary
        lda #18
        sta charx
        lda varr
        jsr disply
h02774: lda #150
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp h02922
        lda #22
        sta chary
        lda #10
        sta charx
        lda #0
        cmp varp
        beq *+5
        jmp h02872
        lda #7
        jsr dmsg
        lda #22
        sta chary
        lda #13
        sta charx
        lda #5
        jsr dmsg
        lda #1
        sta varq
h02872: lda #3
        cmp varp
        beq *+5
        jmp h02922
        lda #7
        jsr dmsg
        lda #22
        sta chary
        lda #10
        sta charx
        lda #6
        jsr dmsg
h02922: lda #1
        cmp varr
        beq *+5
        jmp h03404
        lda #12
        sta chary
        lda #254
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp h02994
        lda #8
        sta charx
        lda #1
        jsr dmsg
        lda #20
        jsr delay
h02994: lda #19
        sta charx
        lda #245
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp h03042
        lda #20
        jsr dmsg
        lda #20
        jsr delay
h03042: lda #240
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp h03082
        lda #21
        jsr dmsg
        lda #30
        jsr delay
h03082: lda #210
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp h03154
        lda #8
        sta charx
        lda #7
        jsr dmsg
        lda #12
        sta chary
        lda #16
        sta charx
        lda #7
        jsr dmsg
        lda #2
        sta vark
h03154: lda #180
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp h03218
        lda #6
        sta charx
        lda #7
        jsr dmsg
        lda #12
        sta chary
        ; lda #12
        sta charx
        lda #7
        jsr dmsg
h03218: lda #100
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp h03258
        lda #10
        sta charx
        lda #7
        jsr dmsg
h03258: lda #200
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp h03404
        lda #10
        sta charx
        lda #4
        jsr dmsg
        lda #20
        jsr delay
        lda #152
        ldy #8
        sta (z80_ix),y
        lda #120
        iny; ldy #9
        sta (z80_ix),y
        lda #1
        sta varopt
        lda #0
        sta z80_c
        ; lda #0
        sta z80_b
        jsr spawn
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #200
        ldy #8
        sta (z80_ix),y
        lda #2
        sta vark
h03404: lda #80
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp h03444
        lda #255
        ldy #5
        sta (z80_ix),y
        rts
h03444: lda #1
        cmp scno
        beq *+5
        jmp h04509
        lda #0
        sta vars
        lda joyval
        and #16
        beq :+
        jmp h03494
:
        lda #1
        sta vars
h03494: lda joyval
        and #8
        beq :+
        jmp h03520
:
        lda #1
        sta vars
h03520: lda #1
        cmp vars
        beq *+5
        jmp h03566
        lda #0
        sta scno
        jsr nwscr
        lda #1
        sta varopt
        ; lda #1
        sta restfl
h03566: lda #255
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp h03763
        lda #19
        sta chary
        lda #10
        sta charx
        lda #22
        jsr dmsg
        lda #19
        sta chary
        lda #15
        sta charx
        lda #22
        sta chary
        lda #10
        sta charx
        lda #23
        jsr dmsg
        lda #28
        ldy #12
        sta (z80_ix),y
        lda #11
        sta chary
        lda #10
        sta charx
        lda #9
        jsr dmsg
        lda #13
        sta chary
        lda #4
        sta charx
        lda #10
        jsr dmsg
        lda #15
        sta chary
        lda #3
        sta charx
        lda #11
        jsr dmsg
        lda #128
        sta varopt
        lda #20
        sta varrnd
h03763: lda #21
        sta chary
        lda #7
        sta charx
        lda #38
        sta z80_c
        ldy #12
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #12
        sta (z80_ix),y
        lda #38
        sta z80_c
        ; ldy #12
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #12
        sta (z80_ix),y
        tya; lda #12
        jsr dmsg
        lda #0
        sta chary
        ; lda #0
        sta charx
        lda #32
        sta loopa
h03887: lda charx
        sta dispx
        lda chary
        sta dispy
        ldy #12
        lda (z80_ix),y
        jsr pattr
        lda #23
        sta z80_c
        lda chary
        clc
        adc z80_c
        sta chary
        lda charx
        sec
        sbc #1
        sta charx
        lda #59
        sta varo
        ldy #12
        lda (z80_ix),y
        sta z80_c
        lda varo
        sec
        sbc z80_c
        sta varo
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda varo
        jsr pattr
        lda #23
        sta z80_c
        lda chary
        sec
        sbc z80_c
        sta chary
        dec loopa
        beq :+
        jmp h03887
:
        lda #1
        sta chary
        lda #22
        sta loopa
h04081: lda #0
        sta charx
        lda #59
        sta varo
        ldy #12
        lda (z80_ix),y
        sta z80_c
        lda varo
        sec
        sbc z80_c
        sta varo
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda varo
        jsr pattr
        lda #31
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        ldy #12
        lda (z80_ix),y
        jsr pattr
        lda chary
        clc
        adc #1
        sta chary
        dec loopa
        beq :+
        jmp h04081
:
        ldy #12
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #12
        sta (z80_ix),y
        lda #32
        ; ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp h04288
        lda #28
        ; ldy #12
        sta (z80_ix),y
h04288: lda joyval
        and #64
        beq :+
        jmp h04390
:
        lda #0
        cmp vart
        beq *+5
        jmp h04385
        lda #20
        asl a
        sta sndtyp
        lda varrnd
        clc
        adc #1
        sta varrnd
        lda #2
        cmp varrnd
        bcc *+5
        jmp h04377
        lda #0
        sta varrnd
h04377: lda #1
        sta vart
h04385: jmp h04398
h04390: lda #0
        sta vart
h04398: lda #17
        sta chary
        lda #8
        sta charx
        lda #13
        jsr dmsg
        lda #17
        sta chary
        lda #9
        sta charx
        lda #14
        jsr dmsg
        lda #15
        sta varobj
        lda varrnd
        sta z80_c
        lda varobj
        clc
        adc z80_c
        sta varobj
        lda #17
        sta chary
        lda #16
        sta charx
        lda varobj
        jsr dmsg
h04509: rts
.out .sprintf("EVENT 7 SIZE = %d", (* - evnt07))

evnt08:
        lda #5
        cmp varj
        beq *+5
        jmp i00035
        lda #255
        ldy #5
        sta (z80_ix),y
        rts
i00035: ldy #11
        lda (z80_ix),y
        sta vart
        ; ldy #11
        lda (z80_ix),y
        sta vari
        lda vart
        lsr a
        lsr a
        lsr a
        lsr a
        sta vart
        lda vart
        sta varq
        lda varq
        asl a
        asl a
        asl a
        asl a
        sta varq
        lda varq
        sta z80_c
        lda vari
        sec
        sbc z80_c
        sta vari
        lda #0
        cmp varc
        beq *+5
        jmp i00196
        lda #255
        sta z80_c
        iny; ldy #12
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #12
        sta (z80_ix),y
i00196: ldy #12
        lda (z80_ix),y
        cmp #12
        bcc *+5
        jmp i00864
        lda #200
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp i00452
        lda #11
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp i00452
        lda #5
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        lda #10
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #4
        cmp varrnd
        bcc *+5
        jmp i00374
        lda #76
        iny; ldy #8
        sta (z80_ix),y
        lda #196
        iny; ldy #9
        sta (z80_ix),y
        jmp i00403
i00374: lda #84
        ldy #8
        sta (z80_ix),y
        lda #16
        iny; ldy #9
        sta (z80_ix),y
i00403: lda #8
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i00452
:
        lda #12
        ldy #12
        sta (z80_ix),y
        lda #200
        ldy #8
        sta (z80_ix),y
i00452: lda #0
        cmp varc
        beq *+5
        jmp i00732
        lda #255
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #7
        sta varrnd
        ldy #12
        lda (z80_ix),y
        sta z80_c
        lda varrnd
        sec
        sbc z80_c
        sta varrnd
        lda #248
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        tya; lda #8
        sta z80_c
        ; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #16
        iny; ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp i00662
        lda #11
        sta chary
        lda #2
        sta charx
        jmp i00679
i00662: lda #10
        sta chary
        lda #23
        sta charx
i00679: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #19
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #20
        jsr pattr
i00732: lda #0
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp i00859
        lda vart
        asl a
        asl a
        asl a
        asl a
        asl a
        sta vart
        lda vari
        asl a
        asl a
        sta vari
        lda vari
        sta z80_c
        lda vart
        clc
        adc z80_c
        sta vart
        lda vart
        ldy #10
        sta (z80_ix),y
        lda #1
        ldy #5
        sta (z80_ix),y
        lda #3
        sta varopt
        rts
i00859: jmp i00878
i00864: lda #200
        ldy #8
        sta (z80_ix),y
i00878: lda vart
        asl a
        asl a
        asl a
        asl a
        sta vart
        lda vari
        sta z80_c
        lda vart
        clc
        adc z80_c
        sta vart
        lda vart
        ldy #11
        sta (z80_ix),y
        rts
.out .sprintf("EVENT 8 SIZE = %d", (* - evnt08))

evnt09:
        lda #0
        ldy #11
        sta (z80_ix),y
        ; lda #0
        iny; ldy #12
        sta (z80_ix),y
        ; lda #0
        ldy #10
        sta (z80_ix),y
        ; lda #0
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00095
        lda #230
        ldy #11
        sta (z80_ix),y
        lda #200
        ldy #8
        sta (z80_ix),y
j00095: lda #7
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00158
        lda #200
        ldy #8
        sta (z80_ix),y
        lda #255
        ldy #12
        sta (z80_ix),y
        lda #14
        ldy #10
        sta (z80_ix),y
j00158: lda #4
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00209
        lda varr
        ldy #11
        sta (z80_ix),y
        lda #0
        iny; ldy #12
        sta (z80_ix),y
j00209: lda #3
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00258
        lda #100
        ldy #12
        sta (z80_ix),y
        lda #0
        ldy #7
        sta (z80_ix),y
j00258: lda #6
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00307
        lda #180
        ldy #8
        sta (z80_ix),y
        lda #0
        ldy #10
        sta (z80_ix),y
j00307: lda #2
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00401
        lda #80
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #150
        sta z80_c
        lda varrnd
        clc
        adc z80_c
        sta varrnd
        lda varrnd
        ldy #12
        sta (z80_ix),y
j00401: rts
.out .sprintf("EVENT 9 SIZE = %d", (* - evnt09))

evnt10:
        lda joyval
        and #32
        beq :+
        jmp k00046
:
        lda joyval
        and #64
        beq :+
        jmp k00046
:
        lda #1
        sta gamwon
k00046: lda #0
        cmp scno
        beq *+5
        jmp k00244
        lda varc
        clc
        adc #1
        sta varc
        lda #4
        cmp varc
        beq *+5
        jmp k00102
        lda #0
        sta varc
k00102: lda #255
        cmp clock
        beq *+5
        jmp k00152
        lda vard
        cmp #12
        bcc *+5
        jmp k00152
        lda vard
        clc
        adc #1
        sta vard
k00152: lda #0
        cmp varf
        bcc *+5
        jmp k00244
        lda #255
        sta z80_c
        lda varf
        clc
        adc z80_c
        sta varf
        lda #1
        cmp varf
        beq *+5
        jmp k00244
        lda #8
        jsr getob
        lda #9
        jsr getob
        lda #10
        jsr getob
        lda #11
        jsr getob
k00244: rts
.out .sprintf("EVENT 10 SIZE = %d", (* - evnt10))

evnt11:
        lda #5
        cmp varj
        beq *+5
        jmp l00046
        lda #8
        sta chary
        ; lda #8
        sta charx
        lda #0
        jsr dmsg
        rts
l00046: lda #0
        cmp varj
        bcc *+5
        jmp l00432
        lda vara
        cmp #152
        bcc *+5
        jmp l00105
        lda #22
        sta chary
        lda #10
        sta charx
        lda #7
        jsr dmsg
l00105: lda #22
        sta chary
        lda #10
        sta charx
        lda #1
        cmp varj
        beq *+5
        jmp l00314
        jsr dscor
        lda #22
        sta chary
        lda #16
        sta charx
        lda #0
        jsr disply
        lda #22
        sta chary
        lda #18
        sta charx
        lda #0
        sta varo
        lda #5
        sta loopa
l00202: lda varo
        cmp numlif
        bcc *+5
        jmp l00251
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #21
        jsr pattr
        jmp l00278
l00251: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
l00278: lda varo
        clc
        adc #1
        sta varo
        dec loopa
        beq :+
        jmp l00202
:
        lda #0
        sta varj
l00314: lda #2
        cmp varj
        beq *+5
        jmp l00399
        lda #1
        cmp varg
        beq *+5
        jmp l00391
        lda #<100
        sta z80_l
        lda #>100
        sta z80_h
        jsr addsc
        lda #0
        sta varg
        lda #8
        jsr dmsg
        jmp l00399
l00391: lda #18
        jsr dmsg
l00399: lda #3
        cmp varj
        beq *+5
        jmp l00424
        lda #19
        jsr dmsg
l00424: lda #0
        sta varj
l00432: rts
.out .sprintf("EVENT 11 SIZE = %d", (* - evnt11))

evnt12:
        rts
.out .sprintf("EVENT 12 SIZE = %d", (* - evnt12))

evnt13:
        lda #3
        sta numlif
        lda #1
        sta scno
        jsr nwscr
        lda #0
        sta varj
        ; lda #0
        sta vars
        lda #3
        sta vark
        lda #0
        sta varl
        ; lda #0
        sta varg
        lda #1
        sta varc
        ; lda #1
        sta varr
        lda #0
        sta vara
        ; lda #0
        sta varb
        ; lda #0
        sta vard
        ; lda #0
        cmp scno
        beq *+5
        jmp n00289
        jsr cls
        lda #23
        sta chary
        lda #0
        sta charx
        lda #8
        sta loopa
n00151: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #11
        jsr pattr
        dec loopa
        beq :+
        jmp n00151
:
        lda #1
        sta varc
        lda #25
        sta charx
        lda #23
        sta chary
        lda #7
        sta loopa
n00225: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #11
        jsr pattr
        dec loopa
        beq :+
        jmp n00225
:
        lda #23
        sta chary
        lda #8
        sta charx
        lda #2
        jsr dmsg
n00289: rts
.out .sprintf("EVENT 13 SIZE = %d", (* - evnt13))

evnt14:
        lda #0
        cmp scno
        beq *+5
        jmp o00177
        jsr cls
        lda #23
        sta chary
        lda #0
        sta charx
        lda #8
        sta loopa
o00047: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #11
        jsr pattr
        dec loopa
        beq :+
        jmp o00047
:
        lda #25
        sta charx
        lda #23
        sta chary
        lda #7
        sta loopa
o00113: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #11
        jsr pattr
        dec loopa
        beq :+
        jmp o00113
:
        lda #23
        sta chary
        lda #8
        sta charx
        lda #2
        jsr dmsg
o00177: rts
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
        .byte "THY*GAME*IS*OVER",13
        .byte 141
        .byte "PREPARE*TO*JOUST",141
        .byte "(#",39
        .byte "#",39
        .byte "#",39
        .byte "$$#$#$$#&%",141
        .byte "WAVE",141
        .byte "BUZZARD*BAIT:",13
        .byte 141
        .byte "EGG*WAVE",141
        .byte "SURVIVAL*WAVE",141
        .byte "*************",141
        .byte "*BONUS*1000",141
        .byte "PRESENTED*BY;",141
        .byte "HIGHRISE*ELECTRONICS*INC.",141
        .byte "EXTRA*MOUNT*EVERY*EGG*WAVE",141
        .byte "PRESS*FIRE*TO*PLAY",141
        .byte "C",141
        .byte "ONTROL;",141
        .byte "KEYBOARD",141
        .byte "KEMPSTON",141
        .byte "SINCLAIR",141
        .byte "**NO*BONUS",141
        .byte "*BONUS*MOUNT",141
        .byte ">",141
        .byte "=",141
        .byte "HIGH;",141
        .byte "defghijklmnop",141
nummsg:
        .byte 24
.out .sprintf("MESSAGE SIZE = %d", (* - msgdat))

chgfx:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 49,245,0,186,0,100,0,16,0,0,0,0,0,0,0,0
        .byte 245,196,152,0,100,0,136,0,0,0,0,0,0,0,0,0
        .byte 245,245,50,234,17,128,80,98,117,245,234,50,17,196,0,0
        .byte 245,245,216,186,17,145,114,64,85,117,234,50,117,196,0,0
        .byte 245,245,250,234,17,68,234,250,32,145,216,250,0,0,0,0
        .byte 245,245,34,80,213,177,80,98,117,245,200,50,0,0,0,0
        .byte 245,245,170,98,245,145,80,98,117,145,234,98,0,0,0,0
        .byte 245,245,114,170,0,213,186,98,145,177,80,200,228,0,0,0
        .byte 245,245,80,16,245,49,200,234,117,85,16,170,0,117,0,0
        .byte 244,245,242,240,241,244,240,250,241,241,242,250,245,245,250,242
        .byte 240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240
        .byte 245,245,250,250,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 245,245,250,250,117,245,34,34,17,245,0,200,0,100,0,16
        .byte 245,245,250,250,245,228,50,170,196,196,186,0,196,0,136,0
        .byte 245,245,240,242,240,244,248,248,241,240,240,248,244,240,250,248
        .byte 245,245,250,250,0,0,250,250,0,228,98,170,128,117,114,136
        .byte 245,117,152,64,213,117,0,0,128,196,234,152,160,145,234,136
        .byte 245,245,136,80,85,117,0,16,117,68,170,152,68,117,250,200
        .byte 245,245,136,0,100,0,80,136,160,117,152,34,100,117,152,200
        .byte 245,245,0,0,0,0,0,0,245,245,80,152,117,32,216,216
        .byte 17,136,17,136,17,0,51,136,51,136,17,204,51,102,0,0
        .byte 0,0,186,80,128,177,64,64,117,85,0,50,196,0,0,136
        .byte 0,0,0,0,0,0,170,152,85,85,64,34,0,0,136,136
        .byte 0,0,0,0,0,0,0,0,0,0,0,64,68,17,16,0
        .byte 245,245,0,200,49,160,250,16,32,100,80,152,117,32,216,200
        .byte 245,245,50,0,145,196,98,250,145,17,80,234,145,49,98,64
        .byte 240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240
        .byte 128,37,0,90,1,164,18,72,37,128,90,0,164,1,72,18
        .byte 164,1,72,18,128,37,0,90,1,164,18,72,37,128,90,0
        .byte 37,128,90,0,164,1,72,18,128,37,0,90,1,164,18,72
        .byte 1,164,18,72,37,128,90,0,164,1,72,18,128,37,0,90
bprop:
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
        .byte 5
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 5
        .byte 2
        .byte 2
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
.out .sprintf("BLOCKS SIZE = %d", (* - chgfx))

sprgfx:
        .byte 0,48,3,44,2,56,3,32,2,0,115,127,251,128,92,112,47,224,51,224,223,64,66,128,2,128,4,64,4,32,2,16
        .byte 0,48,3,44,2,56,3,32,2,0,59,127,123,128,252,112,47,224,19,224,111,64,38,128,8,128,7,0,1,192,1,0
        .byte 0,24,3,22,2,28,3,16,2,0,59,127,251,128,124,112,47,224,19,224,111,64,37,0,3,0,2,192,2,32,4,0
        .byte 12,0,52,192,28,64,4,192,0,64,254,206,1,223,14,58,7,244,7,204,2,251,1,66,1,64,2,32,4,32,8,64
        .byte 12,0,52,192,28,64,4,192,0,64,254,220,1,222,14,63,7,244,7,200,2,246,1,100,1,16,0,224,3,128,0,128
        .byte 24,0,104,192,56,64,8,192,0,64,254,220,1,223,14,62,7,244,7,200,2,246,0,164,0,192,3,64,4,64,0,32
        .byte 192,48,35,44,210,56,107,32,234,0,43,127,119,128,236,112,63,224,255,192,83,0,15,128,0,64,0,0,0,0,0,0
        .byte 0,48,3,44,2,56,3,32,2,0,27,127,55,128,106,112,253,224,46,192,215,128,91,64,7,224,0,0,0,0,0,0
        .byte 12,3,52,196,28,75,4,214,0,87,254,212,1,238,14,55,7,252,3,255,0,202,1,240,2,0,0,0,0,0,0,0
        .byte 12,0,52,192,28,64,4,192,0,64,254,216,1,236,14,86,7,191,3,116,1,235,2,218,7,224,0,0,0,0,0,0
        .byte 0,48,195,44,34,56,211,32,82,0,187,127,107,128,252,112,55,224,57,224,223,64,77,0,4,128,2,64,1,32,0,216
        .byte 12,0,52,195,28,68,4,203,0,74,254,221,1,214,14,63,7,236,7,156,2,251,0,178,1,32,2,64,4,128,27,0
        .byte 3,6,2,12,3,11,50,9,104,0,219,127,181,128,108,88,219,240,183,192,43,0,213,0,69,0,8,128,8,64,4,32
        .byte 3,12,2,24,51,22,98,18,216,0,183,127,109,128,216,88,183,240,43,192,211,0,67,0,2,128,2,128,2,128,1,64
        .byte 96,192,48,64,208,192,144,76,0,22,254,219,1,173,14,54,7,219,1,237,0,212,0,171,0,162,1,16,2,16,4,32
        .byte 48,192,24,64,104,204,72,70,0,27,254,237,1,182,26,27,15,237,3,212,0,203,0,194,1,64,1,64,1,64,2,128
        .byte 131,0,66,0,163,0,210,0,104,127,183,0,221,204,108,88,55,214,219,210,110,112,35,224,0,0,0,0,0,0,0,0
        .byte 3,0,2,0,3,0,2,0,0,127,3,0,13,204,30,88,61,86,54,178,219,80,109,160,38,208,2,168,1,84,0,0
        .byte 0,193,0,66,0,197,0,75,254,22,0,237,51,187,26,54,107,236,75,219,14,118,7,196,0,0,0,0,0,0,0,0
        .byte 0,192,0,64,0,192,0,64,254,0,0,192,51,176,26,120,106,188,77,108,10,219,5,182,11,100,21,64,42,128,0,0
        .byte 128,0,64,0,160,0,208,0,104,0,180,0,90,12,175,24,87,214,41,242,223,240,99,224,32,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,12,12,30,24,61,22,54,178,219,80,109,160,38,208,3,104,0,180,0,0
        .byte 0,1,0,2,0,5,0,11,0,22,0,45,48,90,24,245,107,234,79,148,15,251,7,198,0,4,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,48,48,24,120,104,188,77,108,10,219,5,182,11,100,22,192,45,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,128,7,64,15,160,15,160,15,224,15,224,7,192,3,128
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,30,0,51,0,61,128,63,192,31,192,15,128,7,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,120,0,204,1,188,3,252,3,248,1,240,0,224
        .byte 0,0,0,0,0,0,0,0,1,0,2,0,0,16,1,128,1,0,1,132,8,0,1,16,7,96,6,224,3,64,1,128
        .byte 2,0,2,64,6,68,6,204,70,204,197,152,205,180,109,108,126,216,127,224,47,128,45,0,122,0,108,0,88,0,224,0
        .byte 6,216,110,216,245,176,237,172,109,172,127,248,127,240,46,192,45,128,123,0,110,0,124,0,124,0,120,0,240,0,224,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,240,124,129,31,255,7,186,0,15,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,8,0,5,192,3,32,1,248,0,224,0,113,1,191,0,77,0,131,1,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,15,224,129,62,255,248,93,224,240,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,16,3,160,4,192,31,128,7,0,142,0,253,128,178,0,193,0,0,128,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
frmlst:
        .byte 0,3
        .byte 3,3
        .byte 6,2
        .byte 8,2
        .byte 10,2
        .byte 12,2
        .byte 14,2
        .byte 16,2
        .byte 18,2
        .byte 20,2
        .byte 22,2
        .byte 24,4
        .byte 28,2
        .byte 30,2
        .byte 32,2
        .byte 34,1,35,0
.out .sprintf("SPRITES SIZE = %d", (* - sprgfx))

scdat:
        .word 101,9
        .byte 255,0,128,6,7,8,2,255,0,24,1,9,3,5,255,0,10,1,3,4,5,6,7,8,8,2,255,0,163,1,6,19,20
        .byte 25,2,255,0,4,6,18,19,20,25,8,2,255,0,20,1,9,6,7,5,255,0,108,1,9,3,4,5,6,8,2,255,0,172
        .byte 255,12,8,13,18,4,4,3,6,18,19,20,25,3,5,4,3,4,18,14,255,12,7,255,27,9,10,255,0,13,15,255,27,8
        .byte 255,0,0,255,0,0,255,0,224
numsc:
        .byte 2
.out .sprintf("SCREENS SIZE = %d", (* - scdat))

nmedat:
        .byte 7,14,176,240,6,12,176,8,255
        .byte 7,11,176,240,255
.out .sprintf("POSITIONS SIZE = %d", (* - nmedat))

NUMOBJ = 20
objdta:
        .byte 0,0,0,0,0,0,0,0,0,8,1,136,1,8,1,136,0,8,3,168,5,144,5,136,0,8,2,136,2,136,2,136,255,8,16,255,8,16
        .byte 0,0,0,0,0,0,0,0,8,0,9,128,8,128,9,128,8,0,11,192,5,160,9,160,8,0,9,64,9,64,9,64,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,0,16,1,144,1,16,1,144,0,16,3,208,5,160,5,144,0,16,2,144,2,144,2,144,255,168,0,255,168,0
        .byte 0,0,0,0,0,0,0,0,8,0,9,128,8,128,9,128,8,0,11,192,5,160,9,160,8,0,9,64,9,64,9,64,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,0,16,1,144,1,16,1,144,0,16,3,208,5,160,5,144,0,16,2,144,2,144,2,144,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,8,0,9,128,8,128,9,128,8,0,11,192,5,160,9,160,8,0,9,64,9,64,9,64,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,0,16,1,144,1,16,1,144,0,16,3,208,5,160,5,144,0,16,2,144,2,144,2,144,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,8,0,9,128,8,128,9,128,8,0,11,192,5,160,9,160,8,0,9,64,9,64,9,64,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,29,220,5,20,29,212,16,84,29,220,0,0,0,0,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,29,220,5,20,29,212,16,84,29,220,0,0,29,220,5,20,29,212,16,84,29,220,0,0,0,0,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,29,220,17,84,29,84,5,84,29,220,0,0,29,220,5,20,29,212,16,84,29,220,0,0,0,0,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,29,220,5,20,5,212,4,84,5,220,0,0,29,220,5,20,29,212,16,84,29,220,0,0,0,0,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,0,2,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,1,0,9,32,4,64,2,128,24,48,2,128,4,64,9,32,1,0,0,0,0,0,0,0,0,0,255,0,0,255,0,0
        .byte 0,0,0,0,4,64,1,0,16,16,4,64,0,0,40,40,0,0,4,64,16,16,1,0,4,64,0,0,0,0,0,0,255,0,0,255,0,0
        .byte 0,0,0,255,1,66,6,140,9,48,54,64,104,128,83,0,41,128,17,64,239,120,86,158,41,240,255,0,0,0,0,0,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,124,0,251,0,238,128,179,176,220,204,103,48,249,220,2,170,1,255,255,0,0,255,0,0
        .byte 0,0,255,0,66,128,49,96,12,144,2,108,1,22,0,202,1,148,2,136,30,247,121,106,15,148,0,255,0,0,0,0,255,0,0,255,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,62,0,223,1,119,13,205,51,59,12,230,59,159,85,64,255,128,255,0,0,255,0,0
        .byte 238,0,124,0,120,0,60,0,30,0,14,0,30,0,60,0,120,0,224,0,224,0,112,0,48,0,8,0,0,0,0,0,255,0,0,255,0,0
.out .sprintf("OBJECTS SIZE = %d", (* - objdta))

font:
        .byte 62,23,50,49,125,62,8,50
        .byte 50,125,62,2,205,154,138,62
        .byte 71,50,141,92,50,143,92,201
        .byte 201,201,201,201,109,242,255,0
        .byte 238,110,239,107,221,86,255,0
        .byte 128,128,64,160,208,108,255,0
        .byte 111,117,122,189,93,105,255,0
        .byte 0,24,48,0,0,0,0,0
        .byte 3,2,5,13,26,118,255,0
        .byte 0,96,48,48,48,48,96,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,24,24,48
        .byte 0,0,0,0,126,0,0,0
        .byte 0,0,0,0,0,0,64,0
        .byte 0,0,6,12,24,48,96,0
        .byte 56,68,76,84,100,68,56,0
        .byte 8,24,8,8,8,8,62,0
        .byte 56,68,4,24,32,68,124,0
        .byte 56,68,4,24,4,68,56,0
        .byte 12,20,36,68,124,4,4,0
        .byte 124,64,120,4,4,68,56,0
        .byte 56,64,88,100,68,68,56,0
        .byte 124,68,8,16,32,32,32,0
        .byte 56,68,68,56,68,68,56,0
        .byte 56,68,68,76,52,4,56,0
        .byte 16,16,16,16,16,0,16,0
        .byte 0,0,32,0,0,32,0,0
        .byte 0,0,12,24,48,24,12,0
        .byte 28,34,32,32,32,32,48,0
        .byte 0,0,252,130,2,2,4,0
        .byte 0,124,198,12,24,0,24,0
        .byte 0,124,222,254,254,192,124,0
        .byte 56,68,68,124,68,68,68,0
        .byte 120,68,68,88,68,68,120,0
        .byte 56,68,64,64,64,68,56,0
        .byte 112,72,68,68,68,68,120,0
        .byte 56,68,64,120,64,68,56,0
        .byte 56,68,64,120,64,64,64,0
        .byte 56,68,64,64,76,68,56,0
        .byte 36,68,68,124,68,68,68,0
        .byte 56,16,16,16,16,16,56,0
        .byte 12,4,4,4,4,68,56,0
        .byte 68,72,80,96,80,72,68,0
        .byte 32,64,64,64,64,68,124,0
        .byte 54,73,73,73,73,65,34,0
        .byte 68,100,84,84,84,76,68,0
        .byte 56,68,68,68,68,68,56,0
        .byte 120,68,68,120,64,64,64,0
        .byte 56,68,68,68,84,72,52,0
        .byte 56,68,68,120,80,72,68,0
        .byte 56,68,96,24,4,68,56,0
        .byte 56,84,16,16,16,16,56,0
        .byte 36,68,68,68,68,68,56,0
        .byte 36,68,68,68,40,40,16,0
        .byte 34,65,65,73,73,73,54,0
        .byte 68,68,40,16,40,68,68,0
        .byte 36,68,68,40,16,16,16,0
        .byte 60,4,8,16,32,64,124,0
        .byte 205,232,123,58,48,125,119,201
        .byte 205,232,123,126,50,48,125,201
        .byte 6,0,58,47,125,79,33,35
        .byte 248,9,201,252,48,48,48,0
        .byte 33,8,124,22,0,58,47,125
        .byte 95,25,126,50,48,125,201,0
        .byte 32,33,46,47,53,54,55,56
        .byte 139,140,141,144,145,146,158,159
        .byte 214,215,216,217,244,246,250,251
        .byte 0,0,0,119,84,117,87,0
        .byte 0,0,0,98,83,82,99,0
        .byte 0,0,0,50,165,166,163,0
        .byte 0,0,0,68,69,85,125,0
        .byte 0,0,4,132,92,148,220,0
        .byte 0,0,0,138,238,162,238,0
        .byte 0,0,0,114,82,114,86,0
        .byte 0,0,0,225,64,64,81,0
        .byte 0,0,0,93,137,137,73,0
        .byte 0,0,0,187,42,57,43,0
        .byte 0,0,0,34,59,40,59,0
        .byte 0,0,0,153,149,149,153,0
        .byte 0,0,0,182,84,18,22,0
        .byte 48,48,48,48,48,48,205,153
        .byte 124,201,205,153,124,205,183,124
        .byte 201,6,6,33,136,124,17,191
        .byte 134,175,26,190,202,171,124,210
        .byte 176,124,201,44,28,16,242,201
        .byte 119,44,28,26,16,250,201,62
        .byte 2,205,1,22,62,22,215,58
        .byte 49,125,215,58,50,125,215,6
        .byte 6,17,136,124,26,215,19,16
        .byte 251,62,48,215,201,96,124,0
        .byte 238,58,42,125,50,5,125,201
        .byte 0,8,8,8,8,8,8,0
        .byte 0,112,16,28,16,16,112,0
        .byte 0,20,40,0,0,0,0,0
        .byte 126,129,189,161,161,189,129,126
.out .sprintf("FONT SIZE = %d", (* - font))

jtab:
        .byte 99
keys:   .byte 66,97,104,72,73,16,55,48,49,17,18
.out .sprintf("KEYS SIZE = %d", (* - keys))

data_end:
.out .sprintf("TOTAL DATA SIZE = %d", (* - data_start))
