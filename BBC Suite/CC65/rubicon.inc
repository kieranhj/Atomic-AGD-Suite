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
WINDOWHGT = 21
WINDOWWID = 26 ;面面面面面面面面面面面面
MAPWID = 8
        .byte 255,255,255,255,255,255,255,255
mapdat:
        .byte 255,2,3,6,7,17,21,255,255,1,4,8,14,18,22,255,255,5,10,12,15,19,23,255,255,9,11,13,16,20,24,255
        .byte 255,255,255,255,255,255,255,255
stmap:  .byte 9

evnt00:
        lda #0
        cmp scno
        beq *+5
        jmp a00035
        lda #255
        ldy #5
        sta (z80_ix),y
        rts
a00035: jsr skobj
        sta varobj
        lda #255
        cmp varobj
        bne *+5
        jmp a00092
        lda varobj
        jsr getob
        lda #102
        asl a
        sta sndtyp
        lda #1
        sta vare
a00092: lda #8
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp a00164
        jsr scru
        lda #158
        ldy #8
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        sta varo
        iny; ldy #9
        lda (z80_ix),y
        sta varp
        rts
a00164: lda #160
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp a00237
        jsr scrd
        lda #10
        ldy #8
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        sta varo
        iny; ldy #9
        lda (z80_ix),y
        sta varp
        rts
a00237: lda #8
        ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp a00310
        jsr scrl
        lda #198
        ldy #9
        sta (z80_ix),y
        dey; ldy #8
        lda (z80_ix),y
        sta varo
        iny; ldy #9
        lda (z80_ix),y
        sta varp
        rts
a00310: lda #200
        ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp a00383
        jsr scrr
        lda #10
        ldy #9
        sta (z80_ix),y
        dey; ldy #8
        lda (z80_ix),y
        sta varo
        iny; ldy #9
        lda (z80_ix),y
        sta varp
        rts
a00383: lda joyval
        and #2
        beq :+
        jmp a00474
:
        jsr cangl
        beq :+
        jmp a00474
:
        lda #4
        ldy #6
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #1
        cmp vara
        beq *+5
        jmp a00474
        lda #0
        jsr animsp
a00474: lda joyval
        and #1
        beq :+
        jmp a00566
:
        jsr cangr
        beq :+
        jmp a00566
:
        lda #3
        ldy #6
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #1
        cmp vara
        beq *+5
        jmp a00566
        lda #0
        jsr animsp
a00566: lda joyval
        and #8
        beq :+
        jmp a00657
:
        jsr cangu
        beq :+
        jmp a00657
:
        lda #1
        ldy #6
        sta (z80_ix),y
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #1
        cmp vara
        beq *+5
        jmp a00657
        lda #0
        jsr animsp
a00657: lda joyval
        and #4
        beq :+
        jmp a00749
:
        jsr cangd
        beq :+
        jmp a00749
:
        lda #0
        ldy #6
        sta (z80_ix),y
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #1
        cmp vara
        beq *+5
        jmp a00749
        lda #0
        jsr animsp
a00749: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a00784
:
        lda #1
        sta deadf
a00784: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a00864
:
        lda #1
        cmp varg
        bcs *+5
        jmp a00864
        lda varn
        sec
        sbc #1
        sta varn
        lda #1
        sta varopt
        lda #32
        asl a
        sta sndtyp
a00864: lda #0
        cmp varn
        beq *+5
        jmp a00898
        lda #100
        sta varn
        lda #1
        sta deadf
a00898: lda joyval
        and #16
        beq :+
        jmp a00973
:
        lda #1
        cmp varg
        beq *+5
        jmp a00973
        lda #2
        sta varg
        lda #0
        sta varq
        lda #30
        cmp varm
        beq *+5
        jmp a00973
        lda #5
        sta varm
a00973: lda joyval
        and #32
        beq :+
        jmp a01046
:
        lda #30
        cmp varm
        beq *+5
        jmp a01029
        lda #31
        sta varm
        lda #4
        jsr delay
        jmp a01046
a01029: lda #30
        sta varm
        lda #4
        jsr delay
a01046: rts
.out .sprintf("EVENT 0 SIZE = %d", (* - evnt00))

evnt01:
        lda #0
        cmp scno
        beq *+5
        jmp b00035
        lda #255
        ldy #5
        sta (z80_ix),y
        rts
b00035: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b00128
        jsr cangl
        beq :+
        jmp b00098
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp b00123
b00098: lda #1
        ldy #11
        sta (z80_ix),y
        lda #25
        asl a
        sta sndtyp
b00123: jmp b00194
b00128: jsr cangr
        beq :+
        jmp b00168
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp b00194
b00168: lda #0
        ldy #11
        sta (z80_ix),y
        lda #25
        asl a
        sta sndtyp
b00194: lda #1
        cmp vara
        beq *+5
        jmp b00219
        lda #0
        jsr animsp
b00219: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00249
:
        lda #1
        sta deadf
b00249: rts
.out .sprintf("EVENT 1 SIZE = %d", (* - evnt01))

evnt02:
        lda #30
        cmp varb
        bcs *+5
        jmp c00120
        lda #1
        cmp vara
        beq *+5
        jmp c00115
        lda #0
        jsr animsp
        lda #6
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00115
        lda #30
        asl a
        sta sndtyp
        lda #0
        iny; ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp c00115
        lda #1
        ; ldy #7
        sta (z80_ix),y
c00115: jmp c00133
c00120: lda #0
        ldy #7
        sta (z80_ix),y
c00133: lda #0
        ldy #7
        cmp (z80_ix),y
        bcc *+5
        jmp c00186
        ; lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00186
:
        lda #1
        sta deadf
c00186: lda #5
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00208
c00208: lda #5
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00265
        lda #2
        iny; ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp c00265
        lda #20
        asl a
        sta sndtyp
c00265: lda #11
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00287
c00287: lda #11
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00345
        lda #2
        iny; ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp c00345
        lda #12
        asl a
        sta sndtyp
c00345: rts
.out .sprintf("EVENT 2 SIZE = %d", (* - evnt02))

evnt03:
        lda #50
        cmp varb
        bcc *+5
        jmp d00150
        lda #80
        cmp varb
        bcs *+5
        jmp d00137
        lda #1
        cmp vara
        beq *+5
        jmp d00061
        lda #0
        jsr animsp
d00061: lda #6
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00132
        lda #40
        asl a
        sta sndtyp
        lda #0
        iny; ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp d00132
        lda #1
        ; ldy #7
        sta (z80_ix),y
d00132: jmp d00150
d00137: lda #0
        ldy #7
        sta (z80_ix),y
d00150: lda #0
        ldy #7
        cmp (z80_ix),y
        bcc *+5
        jmp d00203
        ; lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00203
:
        lda #1
        sta deadf
d00203: lda #5
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00225
d00225: lda #5
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00282
        lda #2
        iny; ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp d00282
        lda #30
        asl a
        sta sndtyp
d00282: lda #11
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00304
d00304: lda #11
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00362
        lda #2
        iny; ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp d00362
        lda #14
        asl a
        sta sndtyp
d00362: rts
.out .sprintf("EVENT 3 SIZE = %d", (* - evnt03))

evnt04:
        lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #8
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00080
        lda #1
        cmp vara
        beq *+5
        jmp e00080
        lda #0
        jsr animsp
e00080: lda #8
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00119
        lda #0
        cmp varrnd
        beq *+5
        jmp e00119
e00119: lda #8
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00158
        lda #1
        cmp varrnd
        beq *+5
        jmp e00158
e00158: lda #20
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #8
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00308
        lda #11
        cmp varrnd
        beq *+5
        jmp e00308
        lda #5
        cmp varc
        bcs *+5
        jmp e00308
        ; lda #5
        sta z80_c
        lda #9
        sta z80_b
        jsr spawn
        lda varc
        clc
        adc #1
        sta varc
        lda #6
        cmp varc
        bcc *+5
        jmp e00308
        ; lda #6
        sta varc
e00308: lda #14
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00411
        lda #4
        cmp varh
        beq *+5
        jmp e00355
        lda #0
        jsr animsp
e00355: lda #6
        cmp varh
        beq *+5
        jmp e00381
        lda #0
        jsr animsp
e00381: lda #10
        cmp vari
        beq *+5
        jmp e00411
        lda #255
        ldy #5
        sta (z80_ix),y
e00411: rts
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
f00271: lda #9
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f00301
        lda #0
        jsr animsp
f00301: lda #10
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f00349
        lda #1
        cmp vara
        beq *+5
        jmp f00349
        lda #0
        jsr animsp
f00349: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00423
:
        lda #1
        cmp varg
        bcs *+5
        jmp f00423
        lda varn
        sec
        sbc #1
        sta varn
        lda #1
        sta varopt
        lda #28
        asl a
        sta sndtyp
f00423: lda #0
        cmp varn
        beq *+5
        jmp f00457
        lda #100
        sta varn
        lda #1
        sta deadf
f00457: lda #10
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f00479
f00479: rts
.out .sprintf("EVENT 5 SIZE = %d", (* - evnt05))

evnt06:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp g00082
        jsr cangu
        beq :+
        jmp g00064
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp g00078
g00064: lda #1
        ldy #11
        sta (z80_ix),y
g00078: jmp g00136
g00082: jsr cangd
        beq :+
        jmp g00123
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp g00136
g00123: lda #0
        ldy #11
        sta (z80_ix),y
g00136: lda #1
        cmp vara
        beq *+5
        jmp g00161
        lda #0
        jsr animsp
g00161: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00191
:
        lda #1
        sta deadf
g00191: rts
.out .sprintf("EVENT 6 SIZE = %d", (* - evnt06))

evnt07:
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp h00110
:
        lda #0
        cmp varg
        beq *+5
        jmp h00110
        lda #1
        sta varg
        lda #18
        sta chary
        lda #29
        sta charx
        lda #52
        jsr dmsg
        lda #80
        asl a
        sta sndtyp
        lda #90
        asl a
        sta sndtyp
        lda #70
        asl a
        sta sndtyp
h00110: rts
.out .sprintf("EVENT 7 SIZE = %d", (* - evnt07))

evnt08:
        rts
.out .sprintf("EVENT 8 SIZE = %d", (* - evnt08))

evnt09:
        lda #1
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00072
        lda #4
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #0
        ldy #11
        sta (z80_ix),y
j00072: lda #0
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00123
        lda varo
        ldy #8
        sta (z80_ix),y
        lda varp
        iny; ldy #9
        sta (z80_ix),y
j00123: lda #6
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00182
        lda #12
        iny; ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp j00182
        lda #0
        ldy #11
        sta (z80_ix),y
j00182: rts
.out .sprintf("EVENT 9 SIZE = %d", (* - evnt09))

evnt10:
        lda #0
        cmp scno
        beq *+5
        jmp k00062
        lda #11
        cmp vari
        beq *+5
        jmp k00062
        lda joyval
        and #16
        beq :+
        jmp k00062
:
        lda #1
        sta gamwon
k00062: lda vara
        clc
        adc #1
        sta vara
        lda #2
        cmp vara
        beq *+5
        jmp k00102
        lda #0
        sta vara
k00102: lda varb
        clc
        adc #1
        sta varb
        lda #100
        cmp varb
        beq *+5
        jmp k00142
        lda #0
        sta varb
k00142: lda varh
        clc
        adc #1
        sta varh
        lda #25
        cmp varh
        beq *+5
        jmp k00182
        lda #0
        sta varh
k00182: lda #1
        cmp varopt
        beq *+5
        jmp k00242
        lda #6
        sta chary
        lda #28
        sta charx
        lda varn
        jsr disply
        lda #32
        jsr dmsg
        lda #0
        sta varopt
k00242: lda #1
        cmp varj
        beq *+5
        jmp k00399
        ; lda #1
        cmp scno
        beq *+5
        jmp k00306
        lda #13
        jsr gotob
        cmp #255
        beq :+
        jmp k00306
:
        lda #0
        sta varj
k00306: lda #16
        cmp scno
        beq *+5
        jmp k00353
        lda #14
        jsr gotob
        cmp #255
        beq :+
        jmp k00353
:
        lda #0
        sta varj
k00353: lda #18
        cmp scno
        beq *+5
        jmp k00399
        lda #12
        jsr gotob
        cmp #255
        beq :+
        jmp k00399
:
        lda #0
        sta varj
k00399: lda #0
        cmp varj
        beq *+5
        jmp k00930
        lda #1
        cmp scno
        beq *+5
        jmp k00585
        lda #6
        sta chary
        lda #13
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #72
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #73
        jsr pattr
        lda #7
        sta chary
        lda #13
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #75
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #74
        jsr pattr
        lda #121
        asl a
        sta sndtyp
k00585: lda #16
        cmp scno
        beq *+5
        jmp k00753
        lda #6
        sta chary
        lda #8
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #72
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #73
        jsr pattr
        lda #7
        sta chary
        lda #8
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #75
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #74
        jsr pattr
        lda #121
        asl a
        sta sndtyp
k00753: lda #18
        cmp scno
        beq *+5
        jmp k00922
        lda #6
        sta chary
        lda #18
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #72
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #73
        jsr pattr
        lda #7
        sta chary
        lda #18
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #75
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #74
        jsr pattr
        lda #121
        asl a
        sta sndtyp
k00922: lda #2
        sta varj
k00930: lda #1
        cmp vare
        beq *+5
        jmp k01476
        ; lda #1
        cmp scno
        beq *+5
        jmp k01030
        lda #0
        cmp varobj
        beq *+5
        jmp k01030
        lda #1
        sta chary
        lda #28
        sta charx
        lda #20
        jsr dmsg
        lda #2
        sta varopt
        lda vari
        clc
        adc #1
        sta vari
k01030: lda #6
        cmp scno
        beq *+5
        jmp k01114
        lda #1
        cmp varobj
        beq *+5
        jmp k01114
        ; lda #1
        sta chary
        lda #30
        sta charx
        lda #20
        jsr dmsg
        lda #2
        sta varopt
        lda vari
        clc
        adc #1
        sta vari
k01114: lda #7
        cmp scno
        beq *+5
        jmp k01197
        lda #2
        cmp varobj
        beq *+5
        jmp k01197
        ; lda #2
        sta chary
        lda #29
        sta charx
        lda #20
        jsr dmsg
        lda #2
        sta varopt
        lda vari
        clc
        adc #1
        sta vari
k01197: lda #16
        cmp scno
        beq *+5
        jmp k01280
        lda #3
        cmp varobj
        beq *+5
        jmp k01280
        ; lda #3
        sta chary
        lda #28
        sta charx
        lda #20
        jsr dmsg
        lda #2
        sta varopt
        lda vari
        clc
        adc #1
        sta vari
k01280: lda #19
        cmp scno
        beq *+5
        jmp k01363
        lda #4
        cmp varobj
        beq *+5
        jmp k01363
        lda #3
        sta chary
        lda #30
        sta charx
        lda #20
        jsr dmsg
        lda #2
        sta varopt
        lda vari
        clc
        adc #1
        sta vari
k01363: lda #9
        cmp scno
        beq *+5
        jmp k01476
        lda #15
        cmp varobj
        beq *+5
        jmp k01476
        lda #8
        sta chary
        lda #30
        sta charx
        lda #51
        jsr dmsg
        lda #0
        sta vare
        lda vari
        clc
        adc #1
        sta vari
        lda #5
        sta loopa
k01454: lda #2
        jsr delay
        dec loopa
        beq :+
        jmp k01454
:
k01476: lda #2
        cmp varopt
        beq *+5
        jmp k01813
        lda #8
        sta varrnd
        lda #7
        sta chary
        lda #3
        sta charx
        lda #38
        jsr dmsg
        lda #7
        sta loopa
k01535: lda varrnd
        sta chary
        lda #4
        sta charx
        lda #37
        jsr dmsg
        lda varrnd
        clc
        adc #1
        sta varrnd
        dec loopa
        beq :+
        jmp k01535
:
        lda #15
        sta chary
        lda #4
        sta charx
        lda #39
        jsr dmsg
        lda #8
        sta chary
        lda #5
        sta charx
        lda #40
        jsr dmsg
        lda #14
        sta chary
        lda #9
        sta charx
        lda #36
        jsr dmsg
        lda #10
        sta chary
        lda #5
        sta charx
        lda #41
        sta varrnd
        lda varobj
        sta z80_c
        lda varrnd
        clc
        adc z80_c
        sta varrnd
        lda varrnd
        jsr dmsg
        lda #46
        sta varrnd
        lda varobj
        sta z80_c
        lda varrnd
        clc
        adc z80_c
        sta varrnd
        lda #11
        sta chary
        lda #5
        sta charx
        lda varrnd
        jsr dmsg
        jsr prskey
        lda #0
        sta varopt
        ; lda #0
        sta vare
        jsr redraw
k01813: rts
.out .sprintf("EVENT 10 SIZE = %d", (* - evnt10))

evnt11:
        lda #16
        jsr gotob
        cmp #255
        beq :+
        jmp l00249
:
        lda #10
        cmp vari
        beq *+5
        jmp l00249
        lda #0
        sta scno
        jsr nwscr
        jsr redraw
        lda #11
        sta vari
        lda #5
        jsr delay
        lda #9
        sta chary
        lda #7
        sta charx
        lda #53
        jsr dmsg
        lda #11
        sta chary
        lda #5
        sta charx
        lda #54
        jsr dmsg
        lda #12
        sta chary
        lda #5
        sta charx
        lda #55
        jsr dmsg
        lda #13
        sta chary
        lda #5
        sta charx
        lda #56
        jsr dmsg
        lda #14
        sta chary
        lda #8
        sta charx
        lda #57
        jsr dmsg
        lda #16
        sta chary
        lda #9
        sta charx
        lda #58
        jsr dmsg
        lda #30
        cmp varm
        beq *+5
        jmp l00249
        lda #3
        sta varm
l00249: lda #5
        cmp scno
        beq *+5
        jmp l00535
        lda #10
        cmp vari
        beq *+5
        jmp l00535
        lda #2
        cmp varr
        bcs *+5
        jmp l00535
        lda #15
        sta chary
        lda #8
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
        lda #16
        sta chary
        lda #8
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
        lda #17
        sta chary
        lda #8
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
        lda varr
        clc
        adc #1
        sta varr
        lda #100
        asl a
        sta sndtyp
l00535: lda #1
        cmp vare
        beq *+5
        jmp l01510
        lda #4
        cmp scno
        beq *+5
        jmp l00685
        lda #9
        cmp varobj
        beq *+5
        jmp l00685
        ; lda #9
        sta chary
        lda #28
        sta charx
        lda #22
        jsr dmsg
        lda #10
        sta loopa
l00620: lda #2
        jsr delay
        lda #2
        jsr delay
        lda #55
        asl a
        sta sndtyp
        dec loopa
        beq :+
        jmp l00620
:
        lda #0
        sta vare
        lda vari
        clc
        adc #1
        sta vari
l00685: lda #5
        cmp scno
        beq *+5
        jmp l00818
        lda #10
        cmp varobj
        beq *+5
        jmp l00818
        lda #9
        sta chary
        lda #29
        sta charx
        lda #22
        jsr dmsg
        lda #10
        sta loopa
l00753: lda #2
        jsr delay
        lda #2
        jsr delay
        lda #55
        asl a
        sta sndtyp
        dec loopa
        beq :+
        jmp l00753
:
        lda #0
        sta vare
        lda vari
        clc
        adc #1
        sta vari
l00818: lda #24
        cmp scno
        beq *+5
        jmp l00952
        lda #11
        cmp varobj
        beq *+5
        jmp l00952
        lda #9
        sta chary
        lda #30
        sta charx
        lda #22
        jsr dmsg
        lda #10
        sta loopa
l00886: lda #2
        jsr delay
        lda #2
        jsr delay
        lda #55
        asl a
        sta sndtyp
        dec loopa
        beq :+
        jmp l00886
:
        lda #0
        sta vare
        lda vari
        clc
        adc #1
        sta vari
l00952: lda #22
        cmp scno
        beq *+5
        jmp l01086
        lda #5
        cmp varobj
        beq *+5
        jmp l01086
        lda #15
        sta chary
        lda #29
        sta charx
        ; lda #29
        jsr dmsg
        lda #10
        sta loopa
l01020: lda #2
        jsr delay
        lda #2
        jsr delay
        lda #40
        asl a
        sta sndtyp
        dec loopa
        beq :+
        jmp l01020
:
        lda #0
        sta vare
        lda vari
        clc
        adc #1
        sta vari
l01086: lda #12
        cmp scno
        beq *+5
        jmp l01166
        lda #13
        cmp varobj
        beq *+5
        jmp l01166
        lda #12
        sta chary
        lda #28
        sta charx
        lda #23
        jsr dmsg
        lda #88
        asl a
        sta sndtyp
        lda #0
        sta vare
l01166: lda #5
        cmp scno
        beq *+5
        jmp l01246
        lda #12
        cmp varobj
        beq *+5
        jmp l01246
        ; lda #12
        sta chary
        lda #29
        sta charx
        lda #23
        jsr dmsg
        lda #88
        asl a
        sta sndtyp
        lda #0
        sta vare
l01246: lda #22
        cmp scno
        beq *+5
        jmp l01326
        lda #14
        cmp varobj
        beq *+5
        jmp l01326
        lda #12
        sta chary
        lda #30
        sta charx
        lda #23
        jsr dmsg
        lda #99
        asl a
        sta sndtyp
        lda #0
        sta vare
l01326: lda #6
        cmp varobj
        beq *+5
        jmp l01387
        lda vard
        clc
        adc #1
        sta vard
        lda #96
        asl a
        sta sndtyp
        lda #0
        sta vare
        lda #1
        sta numlif
l01387: lda #7
        cmp varobj
        beq *+5
        jmp l01449
        lda vard
        clc
        adc #1
        sta vard
        lda #96
        asl a
        sta sndtyp
        lda #0
        sta vare
        lda #1
        sta numlif
l01449: lda #8
        cmp varobj
        beq *+5
        jmp l01510
        lda vard
        clc
        adc #1
        sta vard
        lda #96
        asl a
        sta sndtyp
        lda #0
        sta vare
        lda #1
        sta numlif
l01510: lda #2
        cmp varg
        beq *+5
        jmp l01598
        lda varq
        clc
        adc #1
        sta varq
        lda #7
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #18
        sta chary
        lda #29
        sta charx
        lda #52
        jsr dmsg
l01598: lda #254
        cmp varq
        beq *+5
        jmp l01682
        lda #0
        sta varg
        lda #18
        sta chary
        lda #29
        sta charx
        lda #32
        jsr dmsg
        lda #30
        cmp varm
        beq *+5
        jmp l01674
        lda #2
        sta varm
l01674: lda #0
        sta varq
l01682: lda #1
        cmp numlif
        beq *+5
        jmp l01806
        lda #23
        sta chary
        lda #21
        sta charx
        lda #24
        jsr dmsg
        lda #26
        sta varrnd
        lda vard
        sta loopa
l01742: lda #23
        sta chary
        lda varrnd
        sta charx
        lda #21
        jsr dmsg
        lda varrnd
        sec
        sbc #1
        sta varrnd
        dec loopa
        beq :+
        jmp l01742
:
        lda #2
        sta numlif
l01806: rts
.out .sprintf("EVENT 11 SIZE = %d", (* - evnt11))

evnt12:
        lda #150
        jsr delay
        lda #1
        sta varm
        jsr cls
        lda #0
        sta chary
        lda #6
        sta charx
        lda #2
        jsr dmsg
        lda #2
        sta chary
        lda #0
        sta charx
        lda #1
        jsr dmsg
        lda #8
        sta chary
        lda #0
        sta charx
        lda #3
        jsr dmsg
        lda #9
        sta chary
        lda #5
        sta charx
        lda #4
        jsr dmsg
        lda #11
        sta chary
        lda #0
        sta charx
        lda #5
        jsr dmsg
        lda #13
        sta chary
        lda #0
        sta charx
        lda #6
        jsr dmsg
        lda #18
        sta chary
        lda #0
        sta charx
        lda #7
        jsr dmsg
        lda #22
        sta chary
        lda #0
        sta charx
        lda #8
        jsr dmsg
        lda #16
        sta chary
        lda #15
        sta charx
        lda #5
        sta varobj
        lda #66
        sta varopt
        jsr cls
        lda #2
        sta varm
        rts
.out .sprintf("EVENT 12 SIZE = %d", (* - evnt12))

evnt13:
        lda #<0
        sta z80_l
        lda #>0
        sta z80_h
        jsr addsc
        lda #1
        sta numlif
        lda #0
        sta vara
        ; lda #0
        sta varb
        ; lda #0
        sta varc
        lda #3
        sta vard
        lda #0
        sta vare
        ; lda #0
        sta varf
        ; lda #0
        sta varg
        ; lda #0
        sta varh
        ; lda #0
        sta vari
        lda #1
        sta varj
        lda #0
        sta vark
        ; lda #0
        sta varl
        lda #2
        sta varm
        lda #100
        sta varn
        lda #102
        sta varo
        lda #24
        sta varp
        lda #0
        sta varq
        ; lda #0
        sta varr
        ; lda #0
        sta vars
        ; lda #0
        sta vart
        ; lda #0
        sta chary
        ; lda #0
        sta charx
        lda #9
        jsr dmsg
        lda #22
        sta chary
        lda #0
        sta charx
        lda #10
        jsr dmsg
        lda #21
        sta loopa
n00257: lda vari
        clc
        adc #1
        sta vari
        lda vari
        sta chary
        lda #0
        sta charx
        lda #11
        jsr dmsg
        lda vari
        sta chary
        lda #27
        sta charx
        lda #12
        jsr dmsg
        dec loopa
        beq :+
        jmp n00257
:
        lda #0
        sta chary
        ; lda #0
        sta charx
        lda #13
        jsr dmsg
        lda #0
        sta chary
        lda #27
        sta charx
        lda #14
        jsr dmsg
        lda #0
        sta chary
        lda #28
        sta charx
        lda #16
        jsr dmsg
        lda #5
        sta chary
        lda #28
        sta charx
        lda #17
        jsr dmsg
        lda #8
        sta chary
        lda #28
        sta charx
        lda #18
        jsr dmsg
        lda #11
        sta chary
        lda #28
        sta charx
        lda #19
        jsr dmsg
        lda #14
        sta chary
        lda #28
        sta charx
        lda #30
        jsr dmsg
        lda #1
        sta chary
        lda #28
        sta charx
        lda #27
        jsr dmsg
        lda #2
        sta chary
        lda #28
        sta charx
        ; lda #28
        jsr dmsg
        lda #3
        sta chary
        lda #28
        sta charx
        lda #27
        jsr dmsg
        lda #23
        sta chary
        lda #21
        sta charx
        lda #24
        jsr dmsg
        lda #6
        sta chary
        lda #28
        sta charx
        lda varn
        jsr disply
        lda #9
        sta chary
        lda #28
        sta charx
        lda #26
        jsr dmsg
        lda #8
        sta chary
        lda #30
        sta charx
        lda #51
        jsr dmsg
        lda #12
        sta chary
        lda #28
        sta charx
        lda #25
        jsr dmsg
        lda #15
        sta chary
        lda #29
        sta charx
        ; lda #29
        jsr dmsg
        lda #16
        sta chary
        lda #27
        sta charx
        lda #31
        jsr dmsg
        lda #4
        sta chary
        lda #27
        sta charx
        lda #31
        jsr dmsg
        lda #7
        sta chary
        lda #27
        sta charx
        lda #31
        jsr dmsg
        lda #10
        sta chary
        lda #27
        sta charx
        lda #31
        jsr dmsg
        lda #13
        sta chary
        lda #27
        sta charx
        lda #31
        jsr dmsg
        lda #0
        sta vari
        rts
.out .sprintf("EVENT 13 SIZE = %d", (* - evnt13))

evnt14:
        lda #0
        sta varc
        lda #1
        sta varj
        rts
.out .sprintf("EVENT 14 SIZE = %d", (* - evnt14))

evnt15:
        rts
.out .sprintf("EVENT 15 SIZE = %d", (* - evnt15))

evnt16:
        lda #33
        sta varf
        lda #9
        sta chary
        lda #4
        sta charx
        lda #3
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        sta z80_c
        lda varf
        clc
        adc z80_c
        sta varf
        lda varf
        jsr dmsg
        lda #80
        asl a
        sta sndtyp
        lda #100
        sta varn
        lda #6
        sta chary
        lda #28
        sta charx
        lda varn
        jsr disply
        lda #32
        jsr dmsg
        lda #100
        jsr delay
        lda vard
        sec
        sbc #1
        sta vard
        lda #1
        sta numlif
        lda #0
        cmp vard
        beq *+5
        jmp q00204
        ; lda #0
        sta numlif
q00204: rts
.out .sprintf("EVENT 16 SIZE = %d", (* - evnt16))

evnt17:
        jsr cls
        lda #6
        sta chary
        lda #0
        sta charx
        ; lda #0
        jsr dmsg
        lda #4
        sta varm
        lda #120
        asl a
        sta sndtyp
        jsr prskey
        rts
.out .sprintf("EVENT 17 SIZE = %d", (* - evnt17))

evnt18:
        lda #3
        sta varm
        jsr cls
        lda #10
        jsr delay
        lda #1
        sta chary
        lda #0
        sta charx
        lda #59
        jsr dmsg
        lda #10
        jsr delay
        lda #20
        sta chary
        lda #10
        sta charx
        lda #58
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
        .byte "aaaaaaaaPOORaNYMaCUTaOFF",13
        .byte "aaaaaaaaaaINaHISaPRIME",13
        .byte "a",13
        .byte "aa aaaaaaaaGAMEaOVER|",13
        .byte "a",13
        .byte "aaaaaaTHANKSaFORaPLAYING|",13
        .byte "a",13
        .byte "a",13
        .byte "aaaaaaaaaPRESSaANYaKEY",141
        .byte "aaabbbabababbaababbbabbbabbbaaa",13
        .byte "aaaaabababaaabaaaaabaaabaaabaaa",13
        .byte "aaabbbabababbaababaaababababaaa",13
        .byte "aaabbaabababababababababababaaa",13
        .byte "aaabababbbabbaababbbabbbababaaa",141
        .byte "AaJOHNaBLYTHEaGAME:",141
        .byte "aaaFROMaRUCKSACKaGAMESa2018",141
        .byte "POWEREDaBYaAGDaENGINE",141
        .byte "aaaaCHOOSEaYOURaCONTROLLER:",141
        .byte "aaaa1=KEYBOARDaa2=KEMPSTON",13
        .byte "aaaa3=SINCLAIRaaR=REDEFINE",141
        .byte "aaaaaaaaDEFAULTaKEYS:",13
        .byte "aaZ=LEFTaX=RIGHTaK=UPaM=DOWN",13
        .byte "aaL=SHIELDaSPACE=MUSICaON=OFF",141
        .byte "aaaaaaaaaaVERSIONa1_0aaaaaaaaaa",141
        .byte "cdddddddddddddddddddddddddde",141
        .byte "ihhhhhhhhhhhhhhhhhhhhhhhhhhg",141
        .byte "j",141
        .byte "f",141
        .byte "c",141
        .byte "e",141
        .byte "aTIMEaTAKEN:",141
        .byte "lmn",141
        .byte "qr",141
        .byte "vw",141
        .byte "xy",141
        .byte "o",141
        .byte "s",141
        .byte "t",141
        .byte "u",141
        .byte "ssssss",141
        .byte "uuu",141
        .byte "ttt",141
        .byte "oao",141
        .byte "aoa",141
        .byte "p",141
        .byte "`z{",141
        .byte "a<<>",141
        .byte "a",141
        .byte "NYM|aOHaNO|aNYYYYMMM|",141
        .byte "BEaMOREaCAREFULaNYM||",141
        .byte "OHaNO|aNOTaAGAINaNYM|",141
        .byte "PRESSaAaKEY",141
        .byte "[[[[[[[[[[[[[[[[[[[[",141
        .byte "\[[[[[[[[[[[[[[[[[[[}",141
        .byte "@^^^^^^^^^^^^^^^^^^^]",141
        .byte "WISDOM___",141
        .byte "PERFECTaTEA:aCUP__",141
        .byte "NEVERaEATaYELLOW",141
        .byte "SIRaCLIVEaISaAaGOD",141
        .byte "LOVEaALL_aTRUSTaA",141
        .byte "STRESSEDaISaJUST",141
        .byte "BAG_HOTaWATER_MILK",141
        .byte "SNOW||aURGH||",141
        .byte "WORSHIPaHIMaALWAYS",141
        .byte "FEW_aWRONGaNOaONE_",141
        .byte "DESSERTSaBACKWARDS",141
        .byte ";",141
        .byte "k",141
        .byte "WELLaDONEaNYM|",141
        .byte "YOUaHAVEaCONQUERED",141
        .byte "THEaRUBICON_aMAY",141
        .byte "YOURaFUTUREaBE____",141
        .byte "A=MAZE=ING||",141
        .byte "PRESSaFIRE",141
        .byte "aaaaaaTHANKSaFORaPLAYING|",13
        .byte "a",13
        .byte "aaaaaaaaaaaCREDITS__",13
        .byte "a",13
        .byte "aDESIGNaANDaCODE__JOHNaBLYTHE",13
        .byte "a",13
        .byte "aADDITIONALaCODE__ALANaTURVEY",13
        .byte "aaaaaaaaaaaaaaaaaaDAVIDaSAPHIER",13
        .byte "a",13
        .byte "aMUSICS________FERaPHANTOM",13
        .byte "aaaaaaaaaaaaaaaCJ_ECHO_TRIUMPH",13
        .byte "aaaaaaaaaaaaaaaADD257",13
        .byte "a",13
        .byte "aAGDaBY_____JONATHANaCAULDWELL",13
        .byte "a",13
        .byte "aADDITIONALaSUPPORTaANDYaJOHNS",141
nummsg:
        .byte 60
.out .sprintf("MESSAGE SIZE = %d", (* - msgdat))

chgfx:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 95,95,5,5,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,85,85,175,175
        .byte 0,19,0,1,0,19,0,1,0,19,0,1,0,19,0,1
        .byte 91,210,182,233,125,244,250,250,245,245,242,235,121,214,180,173
        .byte 76,0,136,0,76,0,136,0,76,0,136,0,76,0,136,0
        .byte 10,0,140,0,10,0,140,0,78,0,175,5,95,10,175,175
        .byte 0,85,0,35,0,85,0,35,0,85,170,171,85,87,175,175
        .byte 95,95,175,5,78,10,141,0,10,0,140,0,10,0,140,0
        .byte 95,95,5,175,10,27,0,39,0,19,0,5,0,19,0,5
        .byte 255,95,255,175,255,223,255,175,255,95,255,175,255,223,255,175
        .byte 95,0,174,136,95,68,174,136,93,0,174,136,95,68,174,136
        .byte 45,165,150,90,45,165,150,90,45,165,150,90,45,165,150,90
        .byte 165,150,90,75,165,150,90,75,165,150,90,75,165,150,90,75
        .byte 90,90,240,240,165,165,90,90,165,165,90,90,165,165,90,90
        .byte 165,165,90,90,165,165,90,90,165,165,90,90,240,240,165,165
        .byte 105,165,210,90,105,165,240,90,105,165,180,210,90,240,165,165
        .byte 165,240,90,105,165,240,90,105,165,240,240,225,240,210,165,165
        .byte 90,90,180,240,120,240,240,90,105,165,240,90,105,165,240,90
        .byte 90,90,15,45,15,30,90,15,165,150,90,15,165,150,90,15
        .byte 165,165,90,90,165,165,90,90,165,165,218,90,165,165,90,90
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 90,90,195,195,90,60,150,165,105,90,165,195,30,60,165,165
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 95,70,175,38,76,70,175,35,95,19,140,38,95,70,175,38
        .byte 70,93,38,141,70,76,140,140,76,76,38,140,70,76,38,140
        .byte 95,8,175,136,70,0,38,0,70,0,38,0,70,0,38,0
        .byte 34,0,17,238,51,255,102,51,85,17,68,17,68,17,51,238
        .byte 0,68,119,136,255,204,204,102,170,34,136,34,136,34,119,204
        .byte 8,16,165,161,24,26,133,132,66,90,161,0,80,90,0,0
        .byte 8,16,133,165,88,24,33,161,90,66,0,133,90,10,0,0
        .byte 82,64,0,0,66,88,36,133,0,0,0,0,0,0,0,0
        .byte 18,10,0,0,26,66,161,36,0,0,0,0,0,0,0,0
        .byte 165,165,90,90,165,225,90,90,225,165,90,90,180,165,90,90
        .byte 15,15,8,1,10,11,9,5,10,11,9,5,10,11,15,15
        .byte 245,245,250,250,245,244,250,248,245,244,250,248,244,240,248,240
        .byte 10,68,140,0,25,0,140,0,78,2,140,0,25,0,140,0
        .byte 240,240,160,160,64,64,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,32,32,80,80,240,240
        .byte 120,211,188,167,124,244,188,227,124,244,188,240,124,240,188,180
        .byte 124,244,188,240,124,240,188,180,124,215,188,167,124,215,248,227
        .byte 124,215,188,167,124,215,175,175,124,215,188,167,124,244,188,227
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,17,0,51,0,102,0,221,17,170,51,85,102,187
        .byte 70,27,35,5,17,10,0,141,0,70,0,35,0,17,0,0
        .byte 0,0,136,0,76,0,38,0,27,0,5,136,10,76,141,38
        .byte 13,6,10,12,5,8,11,0,6,0,12,0,8,0,0,0
        .byte 100,177,50,80,17,160,0,216,0,100,0,50,0,17,0,0
        .byte 0,0,136,0,196,0,98,0,177,0,80,136,160,196,216,98
        .byte 213,100,170,200,85,128,186,0,100,0,200,0,128,0,0,0
        .byte 66,26,33,5,16,10,0,133,0,66,0,33,0,16,0,0
        .byte 0,0,128,0,72,0,36,0,26,0,5,128,10,72,133,36
        .byte 208,96,160,192,80,128,176,0,96,0,192,0,128,0,0,0
        .byte 0,0,204,119,153,255,51,255,51,255,68,221,102,17,102,51
        .byte 0,0,234,50,245,145,250,200,245,196,186,34,128,100,200,98
        .byte 49,245,0,34,0,228,0,114,49,85,114,0,228,85,200,50
        .byte 165,132,64,0,37,0,74,0,160,132,0,74,160,37,72,18
        .byte 128,36,74,66,161,0,74,0,164,132,88,66,133,164,8,72
        .byte 0,5,18,16,33,33,0,80,36,37,26,82,165,33,66,16
        .byte 1,129,2,72,1,129,0,18,132,33,88,88,32,165,82,90
        .byte 165,164,26,90,165,36,74,8,133,132,72,8,128,1,2,0
        .byte 37,165,18,26,132,37,88,26,32,165,16,26,4,1,0,64
        .byte 128,0,26,0,165,36,74,66,132,0,10,72,133,164,90,90
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 17,8,1,140,95,78,175,175,95,95,175,174,17,76,1,136
        .byte 17,8,35,136,87,95,175,175,95,95,39,175,19,8,1,136
        .byte 51,204,51,204,51,204,255,255,255,255,119,238,51,204,17,136
        .byte 17,136,51,204,119,238,255,255,255,255,51,204,51,204,51,204
        .byte 112,224,240,112,224,112,192,112,224,112,224,112,192,48,112,224
        .byte 37,164,72,82,165,33,74,18,132,37,72,90,132,33,82,74
        .byte 0,0,0,1,0,19,0,38,0,76,1,136,19,0,38,1
        .byte 0,0,136,0,76,0,38,0,19,0,1,136,0,76,136,38
        .byte 8,70,0,140,17,8,35,0,70,0,140,0,8,0,0,0
        .byte 70,17,35,0,17,8,0,140,0,70,0,35,0,17,0,0
        .byte 8,17,175,171,25,27,140,4,70,8,171,39,85,8,0,0
        .byte 8,17,141,175,93,25,35,171,0,70,174,141,17,10,0,0
        .byte 19,10,0,0,27,70,171,38,0,0,0,0,0,0,0,0
        .byte 87,68,0,0,70,93,38,141,0,0,0,0,0,0,0,0
        .byte 128,17,250,186,145,177,216,200,100,245,186,0,85,245,0,0
        .byte 49,160,0,0,177,100,186,98,0,0,0,0,0,0,0,0
        .byte 0,0,50,0,100,17,98,80,85,100,16,136,0,0,0,0
        .byte 0,0,0,132,8,66,160,36,66,10,1,128,0,0,0,0
        .byte 0,0,96,96,176,176,240,240,240,240,112,224,48,192,16,128
bprop:
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
        .byte 5
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
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 2
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
        .byte 2
        .byte 2
        .byte 2
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
        .byte 28,56,2,64,12,48,26,104,18,72,18,72,13,176,113,142,254,127,31,248,27,216,44,52,115,206,120,30,48,12,0,0
        .byte 28,56,2,64,12,48,0,0,26,104,18,72,13,178,49,142,126,124,95,248,27,216,44,48,115,204,48,30,0,30,0,12
        .byte 0,0,30,120,12,48,30,120,0,0,18,72,13,176,113,142,254,127,31,248,25,152,44,52,115,206,120,30,48,12,0,0
        .byte 24,24,6,96,12,48,0,0,26,104,18,72,141,176,241,140,126,126,31,250,24,24,12,52,51,206,120,12,120,0,48,0
        .byte 28,56,2,64,12,48,30,120,30,120,16,8,15,240,127,254,255,255,29,184,30,120,47,244,115,206,120,30,48,12,0,0
        .byte 0,0,30,120,12,48,30,120,30,120,16,8,79,240,127,252,63,254,27,122,28,248,47,240,115,204,48,30,0,30,0,12
        .byte 6,96,24,24,12,48,30,120,30,120,16,8,15,240,255,255,127,254,29,184,30,120,47,244,115,206,120,30,48,12,0,0
        .byte 30,120,0,0,12,48,30,120,30,120,16,8,15,242,63,254,127,252,94,216,31,56,15,244,51,206,120,12,120,0,48,0
        .byte 253,191,86,106,1,128,25,152,37,164,82,82,66,66,66,66,61,188,67,194,252,63,239,247,247,239,102,102,7,224,3,192
        .byte 45,180,22,104,1,128,25,152,37,164,82,82,66,66,66,66,61,188,67,194,252,63,239,247,231,231,244,47,103,230,3,192
        .byte 1,128,1,128,1,128,25,152,61,188,126,126,126,126,66,66,61,188,67,194,252,63,239,247,196,35,228,39,199,227,3,192
        .byte 45,180,22,104,1,128,25,152,37,164,82,82,66,66,66,66,61,188,67,194,252,63,239,247,231,231,244,47,103,230,3,192
        .byte 0,116,0,128,0,104,0,212,0,148,6,148,15,104,95,144,127,252,63,108,30,48,13,152,3,192,3,192,1,128,0,0
        .byte 0,52,0,192,0,104,0,0,0,212,6,148,15,104,31,144,255,252,63,108,31,48,14,216,1,224,13,224,6,192,0,0
        .byte 0,0,0,244,0,104,0,252,0,0,6,148,15,104,31,144,63,252,127,108,94,48,13,152,3,192,3,192,1,128,0,0
        .byte 0,200,0,52,0,104,0,0,0,212,6,148,15,104,31,144,255,252,63,108,28,48,11,88,7,128,7,176,3,96,0,0
        .byte 46,0,1,0,22,0,43,0,41,0,41,96,22,240,9,250,63,254,54,252,12,120,25,176,3,192,3,192,1,128,0,0
        .byte 44,0,3,0,22,0,0,0,43,0,41,96,22,240,9,248,63,255,54,252,12,248,27,112,7,128,7,176,3,96,0,0
        .byte 0,0,47,0,22,0,63,0,0,0,41,96,22,240,9,248,63,252,54,254,12,122,25,176,3,192,3,192,1,128,0,0
        .byte 19,0,44,0,22,0,0,0,43,0,41,96,22,240,9,248,63,255,54,252,12,56,26,208,1,224,13,224,6,192,0,0
        .byte 0,1,0,0,0,0,0,0,128,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,128,0,0,0,0,0,0,1
        .byte 0,15,128,4,192,2,224,1,240,0,32,1,64,3,128,7,0,15,128,4,192,2,224,1,240,0,32,1,64,3,128,5
        .byte 0,255,248,64,252,42,254,21,255,0,2,31,84,63,168,127,0,255,248,64,252,42,254,21,255,0,2,31,84,63,168,85
        .byte 0,15,128,4,192,2,224,1,240,0,32,1,64,3,128,7,0,15,128,4,192,2,224,1,240,0,32,1,64,3,128,5
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,132,68,130,34,67,35,34,34,36,68,36,136,68,152,200,72,72,68,68,132,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,66,36,36,40,108,44,200,68,136,194,132,65,66,33,35,18,49,22,34,34,0,0,0,0,0,0
        .byte 8,16,20,40,46,92,0,0,79,242,211,203,89,154,25,152,25,152,95,250,212,43,77,178,0,0,46,92,20,40,8,16
        .byte 3,192,12,48,16,8,32,52,32,52,64,2,64,34,64,2,64,2,64,2,32,4,36,4,16,8,12,48,3,192,0,0
        .byte 0,0,3,192,12,48,16,8,32,52,32,52,64,66,64,2,64,2,32,4,36,4,16,8,12,48,3,192,0,0,0,0
        .byte 0,0,0,0,3,192,12,48,16,8,32,52,32,52,64,130,32,4,34,4,16,8,12,48,3,192,0,0,0,0,0,0
        .byte 0,0,3,192,12,48,16,8,32,52,32,52,64,66,64,2,64,2,32,4,36,4,16,8,12,48,3,192,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,3,192,4,32,8,208,8,80,8,16,10,16,4,32,3,192,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,3,192,4,224,4,96,4,32,6,32,3,192,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,3,128,5,192,103,220,191,238,255,254,127,252,63,248,127,252,95,254,191,94,191,171,253,117,106,154,53,12,30,0
        .byte 3,128,5,192,107,236,191,214,255,255,255,255,255,254,127,254,63,252,47,246,95,234,95,86,58,186,21,204,15,0,0,0
        .byte 0,0,1,128,3,192,54,108,44,52,24,24,48,12,97,134,97,6,48,12,24,24,44,52,54,108,3,192,1,128,0,0
        .byte 0,0,1,128,3,192,54,108,44,52,25,152,51,76,103,166,101,38,50,76,25,152,44,52,54,108,3,192,1,128,0,0
        .byte 0,0,1,128,3,192,54,108,45,148,27,72,55,164,111,82,106,2,53,36,18,64,41,4,52,12,2,64,1,128,0,0
        .byte 0,0,1,128,3,192,54,108,44,52,25,152,51,76,103,166,101,38,50,76,25,152,44,52,54,108,3,192,1,128,0,0
        .byte 0,128,65,130,51,204,59,220,23,232,12,48,48,12,246,110,118,111,48,12,12,48,23,232,58,92,51,204,65,130,1,0
        .byte 0,128,1,128,35,196,27,216,23,232,12,48,48,12,240,14,118,111,48,12,12,48,23,232,26,88,35,196,1,128,1,0
        .byte 0,128,1,128,3,192,19,200,15,240,12,48,48,12,240,14,114,79,48,12,12,48,14,112,19,200,3,192,1,128,1,0
        .byte 64,2,100,60,103,250,103,252,103,250,111,252,127,250,111,252,127,250,63,244,63,232,31,244,31,232,15,208,7,160,1,64
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,32,0,0,0,0,56,28,92,46,124,62,130,65,0,0
frmlst:
        .byte 0,4
        .byte 4,4
        .byte 8,4
        .byte 12,4
        .byte 16,4
        .byte 20,4
        .byte 24,3
        .byte 27,1
        .byte 28,4
        .byte 32,2
        .byte 34,2
        .byte 36,4
        .byte 40,3
        .byte 43,1
        .byte 44,2,46,0
.out .sprintf("SPRITES SIZE = %d", (* - sprgfx))

scdat:
        .word 205,366,471,348,414,407,243,394,430,436,434,137,376,131,460,449,304,403,377,388,407,393,501,386,375
        .byte 20,20,20,0,22,0,22,0,20,0,0,0,22,0,20,20,20,0,22,22,22,0,20,0,0,20,20,0,20,0,22,0,22
        .byte 0,20,20,20,0,0,0,20,255,0,5,22,0,20,20,0,20,20,20,0,0,22,0,22,0,20,0,20,0,22,0,20,0,0,0
        .byte 22,0,22,0,20,0,20,20,20,0,20,0,22,22,22,0,20,20,20,0,22,0,20,20,20,0,22,22,22,0,20,0,0
        .byte 20,255,0,12,27,28,255,0,15,255,2,9,29,30,255,2,9,255,0,5,3,8,255,1,8,32,31,255,1,8,9,5,255,0,4
        .byte 3,5,255,0,18,3,5,255,0,186,3,5,255,0,18,3,5,255,0,4,3,6,255,2,18,7,5,255,0,5,255,1,20
        .byte 255,0,12,28,27,0,0,82,27,28,83,255,0,18,76,77,0,84,0,80,30,0,84,255,0,17,78,79,0,0,0,81,31
        .byte 255,0,10
        .byte 4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,4,5,0,0,3,4,5
        .byte 0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0,0,1,4,5,0,0,3,4,5,0,0,3,4,5,0,0
        .byte 3,4,5,0,0,3,4,5,255,0,4,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,6
        .byte 255,2,4,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,255,4,7,5,0,0,3,4,5,0,0,0
        .byte 1,0,45,47,3,4,5,0,0,0,255,1,6,4,5,0,0,3,4,5,255,0,5,46,48,3,4,5,255,0,9,4,5
        .byte 0,0,3,4,6,255,2,7,7,4,6,255,2,9,4,5,0,0,3,255,4,21,39,36,0,0,0,255,1,21,41,36,255,0,24
        .byte 40,36,0,0,0,255,2,12,0,0,255,2,7,4,5,0,0,3,255,4,12,5,3,255,4,8,5,0,0,3,4,8,255,1,4
        .byte 9,4,18,14,19,4,5,3,4,8,255,1,5,4,5,0,0,3,4,5,255,0,4,3,4,12,20,13,4,55,56,4,5
        .byte 255,0,5,4,5,0,0,3,4,5,0,21,21,0,3,4,16,15,17,4,57,58,4,6,255,2,5,4,5,0,0,3,4
        .byte 5,0,21,21,0,3,255,4,15,5,0,0,3,4,5,55,56,0,0,0,1,9,4,4,8,255,1,9,4,5,0,0,3
        .byte 4,5,57,58,255,0,17,4,5,0,0,3,4,6,255,2,7,0,0,255,2,10,4,5,0,0,3,255,4,21
        .byte 255,4,19,22,4,4,22,4,22,22,4,8,255,1,7,9,4,8,255,1,14,4,5,255,0,7,3,4,5,255,0,14,4
        .byte 5,0,0,0,2,0,0,0,3,4,5,0,0,0,255,2,11,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,4,4
        .byte 22,4,4,4,22,4,22,22,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,8,255,1,9,4,5,0,0,3
        .byte 4,5,0,0,3,4,5,0,0,3,4,5,255,0,9,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,255,0,8
        .byte 2,4,5,0,0,3,4,6,0,0,7,4,6,0,0,7,4,5,255,0,7,3,22,4,5,0,0,3,4,4,0,0,4,4,4
        .byte 0,0,4,4,5,0,0,0,2,0,0,0,3,22,4,5,0,0,3,4,4,0,0,4,14,4,0,0,4,4,5,0,0
        .byte 3,4,5,0,0,3,22,4,5,0,0,3,4,4,0,0,4,15,4,0,0,4,4,5,0,0,3,22,5,0,0,3,4,4
        .byte 5,0,0,3,4,4,0,0,4,4,4,0,0,4,4,5,0,0,3,4,5,0,0,3,22,4,5,0,0,3,4,8,0,0
        .byte 9,4,8,0,0,9,4,5,0,0,3,4,5,0,0,3,4,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4
        .byte 5,0,0,3,4,5,0,0,3,22,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0
        .byte 3,4,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,4,5,0,0,3
        .byte 4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,22,4,5,0,0,3,4,5,0,0,3,4,5
        .byte 0,0,3,4,5,0,0,3,4,5,0,0,3,4,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0
        .byte 3,4,5,0,0,3,4,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4
        .byte 255,22,26,255,1,4,37,37,1,37,37,1,37,37,1,37,37,1,37,37,1,37,37,255,1,5,255,0,4,38,38,0,38,38
        .byte 0,38,38,0,38,38,0,38,38,0,38,38,255,0,5,255,2,4,22,22,2,22,22,2,22,22,2,22,22,2,22,22,2
        .byte 22,22,255,2,5,255,22,26,255,1,26,255,0,26,255,2,26,255,22,27,8,255,1,7,9,22,8,255,1,9,9,22,18
        .byte 19,22,22,5,255,0,7,3,22,5,55,56,255,0,7,3,22,12,13,22,22,5,0,0,0,2,0,0,0,3,22,5,57
        .byte 58,0,255,2,6,7,22,16,17,22,22,5,0,0,3,22,5,0,0,3,22,5,0,0,3,255,22,12,5,0,0,3,22
        .byte 5,0,0,3,22,5,0,0,3,22,8,255,1,9,4,5,0,0,3,22,5,0,0,3,22,5,0,0,3,22,5,255,0,9
        .byte 22,5,0,0,3,22,5,0,0,3,4,5,0,0,3,22,5,0,0,0,255,2,6,22,5,0,0,3,22,5,0,0,3
        .byte 22,5,0,0,3,22,5,0,0,3,255,22,6,4,5,0,0,3,22,5,0,0,3,4,5,0,0,3,22,5,0,0,3
        .byte 22,8,255,1,4,22,5,0,0,3,4,5,0,0,3,22,5,0,0,3,22,5,0,0,3,22,5,255,0,4,4,5,0,0
        .byte 3,22,5,0,0,3,4,5,0,0,3,22,5,0,0,3,22,5,0,0,0,2,4,5,0,0,3,4,5,0,0,3,4
        .byte 5,0,0,3,22,5,0,0,3,22,5,0,0,3,22
        .byte 22,5,0,0,3,22,5,0,0,3,22,5,0,0,3,22,5,0,0,3,22,5,0,0,3,22,1,0,0,0,3,4,5
        .byte 0,0,3,22,5,0,0,3,22,5,0,0,3,4,5,0,0,0,1,255,0,4,3,22,5,0,0,3,22,5,0,0,3
        .byte 4,5,0,0,3,22,5,255,0,4,255,2,4,7,4,5,43,21,3,4,5,0,0,3,22,5,0,0,3,4,6,255,2,4
        .byte 255,4,6,5,43,21,3,22,5,0,0,3,4,5,0,0,3,4,4,4,22,4,22,255,1,4,9,4,6,2,2,7,4
        .byte 5,0,0,3,4,5,0,0,0,255,1,6,255,0,4,3,255,4,6,5,0,0,3,4,5,255,0,9,2,0,0,0,3
        .byte 4,16,15,15,17,4,5,0,0,3,4,6,255,2,4,255,0,4,2,4,5,0,0,3,255,4,6,5,0,0,3,4,4,4
        .byte 22,4,22,5,0,0,3,22,1,255,0,4,255,1,6,255,0,4,255,1,4,9,4,5,0,0,3,22,255,0,19,3,22
        .byte 5,0,0,3,4,255,2,16,0,0,0,3,4,5,0,0,3,22,255,4,16,5,0,0,3,4,5,0,0,3,4,255,1,14
        .byte 9,4,5,0,0,3,4,5,0,0,3,22,255,0,14,3,4,5,0,0,3,4,5,0,0,3,4,255,2,11,0,0,0
        .byte 3,4,5,0,0,3,4,5,0,0,3,255,4,12,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,255,1,9,9
        .byte 4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,255,0,9,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0
        .byte 3,4,255,2,6,0,0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,255,4,7,5,0,0,3,4,5
        .byte 0,0,3,4,5,0,0,3,4,5,0,0,3,4
        .byte 4,5,0,0,3,255,4,22,5,0,0,0,255,1,14,9,4,18,14,14,19,4,4,5,255,0,17,3,4,16,15,15,17
        .byte 4,4,6,2,2,2,0,0,255,2,9,0,0,0,3,255,4,11,5,3,255,4,9,5,0,0,3,4,18,14,14,19,4,4
        .byte 8,1,1,1,0,0,255,1,9,0,0,0,3,4,12,20,20,13,4,4,5,255,0,17,3,4,16,15,15,17,4,4,6
        .byte 255,2,14,0,0,0,3,255,4,22,5,0,0,3,4,8,1,1,9,4,4,18,14,14,19,4,8,255,1,7,9,4,5
        .byte 0,0,3,4,5,21,21,3,4,4,12,20,20,13,4,5,255,0,5,42,23,3,4,5,0,0,3,4,5,21,21,3,4,4
        .byte 16,15,15,17,4,6,255,2,4,0,42,23,3,4,5,0,0,3,4,5,0,0,3,255,4,12,5,0,0,3,4,5,0,0
        .byte 3,4,5,0,0,3,4,10,11,255,1,7,9,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,33,10,11,0,24
        .byte 25,26,0,28,27,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,10,11,255,2,4,0,29,30,3,4,5
        .byte 0,0,3,4,5,0,0,3,33,5,0,0,3,255,4,7,5,32,31,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0
        .byte 3,33,4,8,1,1,9,4,5,0,0,3,33,5,0,0,3,4,5,0,0,3,33,5,0,0,3,4,4,5,0,0,3
        .byte 33,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,33,4,5,0,0,3,4,5,0,0,3,33,5
        .byte 0,0,3,33,5,0,0,3,33,5,0,0,3,33,33,5,0,0,3,33,5,0,0,3,33,5,0,0,3,4,5,0,0
        .byte 3,33,5,0,0,3,33
        .byte 255,22,26,255,1,26,255,0,26,255,2,26,255,22,26,255,1,20,9,22,18,14,19,22,255,0,20,3,22,12,20,13,22
        .byte 2,255,0,4,255,2,15,7,22,16,15,17,22,22,5,0,0,3,255,22,22,5,0,0,3,22,18,14,14,19,22,8,255,1,14
        .byte 22,5,0,0,3,22,12,20,20,13,22,5,255,0,14,22,5,0,0,3,22,16,15,15,17,22,5,0,0,0,255,2,11
        .byte 22,5,0,0,3,255,22,6,5,0,0,3,255,22,11,1,255,0,4,255,1,4,9,22,5,21,21,3,22,8,1,1,9
        .byte 22,18,14,14,19,22,255,0,9,3,22,5,21,21,3,22,5,0,0,3,22,16,15,15,17,22,255,2,6,0,0,0,3
        .byte 22,6,2,2,7,22,5,0,0,3,255,22,12,5,0,0,3,255,22,6,5,0,0,3,22,8,1,1,9,22,255,1,6
        .byte 255,0,4,255,1,6,0,0,0,3,22,5,0,0,3,22,255,0,19,3,22,5,0,0,3,22,255,2,19,7,22,5,0,0
        .byte 3,255,22,22,5,0,0,3,22
        .byte 255,22,18,35,22,22,22,35,22,35,35,255,1,24,9,35,255,0,24,3,35,255,2,6,255,0,4,255,2,11,0,0,0
        .byte 3,35,255,22,6,5,0,0,3,255,22,10,35,5,0,0,3,35,22,18,14,14,19,22,5,0,0,3,22,8,255,1,7
        .byte 9,22,5,0,0,3,35,22,12,20,20,13,22,5,0,0,3,22,5,255,0,7,3,22,5,0,0,3,35,22,16,15,15
        .byte 17,22,5,0,0,3,22,5,0,0,0,2,0,0,0,3,22,6,2,2,7,35,255,22,6,5,0,0,3,22,5,0,0
        .byte 3,22,5,0,0,3,35,22,22,35,22,35,255,1,6,0,0,0,3,22,5,0,0,3,22,5,0,0,3,22,18,14,14
        .byte 19,35,255,0,9,3,22,5,0,0,3,22,5,0,0,3,22,12,20,20,13,35,255,2,9,7,22,5,0,0,3,22,5
        .byte 0,0,3,22,16,15,15,17,35,255,22,11,5,0,0,3,22,5,0,0,3,22,22,35,22,35,35,22,8,255,1,9,0,0,0
        .byte 3,22,5,0,0,3,22,8,255,1,4,22,5,255,0,12,3,22,5,0,0,3,35,5,255,0,4,22,5,0,0,0,255,2,9
        .byte 7,22,5,0,0,3,22,5,0,0,0,2,22,5,0,0,3,255,22,11,5,0,0,3,22,5,0,0,3,35,22,5,0,0
        .byte 3,22,8,255,1,7,9,22,5,23,42,3,22,5,21,21,3,35,22,5,0,0,3,22,5,255,0,7,3,22,5,42,23
        .byte 3,22,5,21,21,3,35,22,5,0,0,3,22,5,0,0,0,2,0,0,0,3,22,6,2,2,7,35,6,2,2,7,22,22
        .byte 5,0,0,3,22,5,0,0,3,22,5,0,0,3,35,22,22,35,22,35,35,35,22,35,35
        .byte 255,22,21,5,0,0,3,22,255,1,13,9,22,18,14,14,14,19,22,5,0,0,3,22,255,0,13,3,22,12,20,20,20
        .byte 13,22,5,0,0,3,22,255,2,13,7,22,16,15,15,15,17,22,5,0,0,3,22,22,4,255,22,19,5,0,0,3,22
        .byte 255,1,19,9,22,5,0,0,3,22,255,0,19,3,22,5,0,0,3,22,2,255,0,4,255,2,11,0,0,0,3,22,5
        .byte 0,0,3,22,22,5,0,0,3,22,4,255,22,9,5,0,0,3,22,5,0,0,3,22,22,5,0,0,3,4,8,1,1
        .byte 9,22,18,14,14,19,22,5,0,0,3,22,5,0,0,3,22,4,5,0,0,3,22,5,42,23,3,22,12,20,20,13,4
        .byte 5,0,0,3,22,5,0,0,3,22,22,5,0,0,3,22,5,23,23,3,22,12,20,20,13,22,5,0,0,3,4,5,0,0
        .byte 3,22,4,5,0,0,3,4,5,0,0,3,22,16,15,15,17,22,5,0,0,3,22,5,0,0,3,22,4,5,0,0,3
        .byte 22,5,0,0,3,4,22,4,22,22,4,5,0,0,3,22,5,0,0,3,22,4,5,0,0,3,4,5,0,0,3,22,8
        .byte 255,1,4,0,0,0,3,4,5,0,0,3,22,4,5,0,0,3,22,5,0,0,3,22,5,255,0,7,3,22,5,0,0
        .byte 3,22,4,5,0,0,3,4,5,0,0,3,4,5,0,0,0,2,0,0,0,3,4,5,0,0,3,22,4,5,0,0,3
        .byte 4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,22,5,0,0,3,22,4,5,0,0,3,4,5,0,0,3,4,5
        .byte 0,0,3,4,5,0,0,3,4,5,0,0,3,22,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0
        .byte 3,4,5,0,0,3,4,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4
        .byte 33,5,0,0,3,33,5,0,0,3,33,5,0,0,3,33,5,0,0,3,33,5,0,0,3,33,33,5,0,0,3,33,5
        .byte 0,0,3,33,5,0,0,3,33,5,0,0,3,33,5,0,0,3,33,33,5,0,0,3,33,5,0,0,3,33,5,0,0
        .byte 3,33,5,0,0,3,33,5,0,0,3,33,33,5,0,0,3,33,5,0,0,3,33,5,0,0,3,33,5,0,0,3,33
        .byte 5,0,0,3,33,33,5,0,0,3,33,5,0,0,3,33,5,0,0,3,33,5,0,0,3,33,5,0,0,3,33,33,5
        .byte 0,0,3,33,5,0,0,3,33,5,0,0,3,33,5,43,0,3,33,5,0,0,0,1,33,5,0,0,3,33,5,0,0
        .byte 3,33,5,0,0,3,33,5,21,23,3,33,5,255,0,4,33,5,0,0,3,33,5,0,0,3,33,5,0,0,3,33,5
        .byte 0,0,3,33,5,0,0,0,2,33,5,0,0,3,33,5,0,0,3,33,5,0,0,3,33,5,55,56,3,33,5,0,0
        .byte 3,33,33,5,0,0,3,33,5,0,0,3,33,5,0,0,3,33,5,57,58,3,33,5,0,0,3,33,33,5,0,0,3
        .byte 33,5,0,0,3,33,5,0,0,3,33,6,2,2,7,33,5,0,0,3,33,33,5,0,0,3,33,5,0,0,3,33,5
        .byte 0,0,3,255,33,6,5,0,0,3,33,33,5,0,0,3,33,5,0,0,3,33,5,0,0,3,33,18,14,14,19,33,5
        .byte 0,0,3,33,33,5,0,0,0,1,255,0,4,1,0,0,0,3,33,12,20,20,13,33,5,0,0,3,33,33,5,255,0,12
        .byte 3,33,12,20,20,13,33,5,0,0,3,33,33,5,255,0,12,3,33,16,15,15,17,33,5,0,0,3,33,33,5,255,0,12
        .byte 3,255,33,6,5,0,0,3,33,33,5,255,0,13,255,1,6,0,0,0,3,33,33,5,255,0,22,3,33,33,6,255,2,22
        .byte 7,255,33,27
        .byte 255,4,6,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,4,8,255,1,4,0,0,0,3,4
        .byte 5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,4,5,255,0,7,3,4,5,0,0,3,4,5,0,0,3,4,5
        .byte 0,0,3,4,4,5,0,0,0,255,2,4,7,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,4,5,0,0
        .byte 3,255,4,6,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,4,5,0,0,3,4,8,255,1,4,0,0,0,3
        .byte 4,5,65,44,3,4,5,0,0,3,4,4,5,0,0,3,4,5,255,0,7,3,4,5,44,65,3,4,5,0,0,3,4,4
        .byte 5,0,0,3,4,5,0,0,0,2,0,0,0,3,4,6,2,2,7,4,5,0,0,3,4,4,5,0,0,3,4,5,0,0
        .byte 3,4,5,0,0,3,255,4,6,5,0,0,3,4,4,5,0,0,3,4,5,0,0,3,4,5,0,0,61,4,18,14,14
        .byte 19,33,5,0,0,3,4,4,5,0,0,3,4,5,0,0,3,4,5,0,60,255,20,5,13,4,5,0,0,3,33,4,5
        .byte 0,0,3,4,5,0,0,3,4,5,0,0,63,4,16,15,15,17,33,5,0,0,3,4,4,5,0,0,3,4,5,0,0
        .byte 3,4,5,0,0,3,33,4,33,4,4,33,5,0,0,3,33,4,5,0,0,3,4,5,0,0,3,33,5,0,0,0,255,1,6
        .byte 255,0,4,1,33,5,0,0,3,33,5,0,0,3,4,5,255,0,14,4,5,0,0,3,4,5,0,0,3,33,6,255,2,14
        .byte 33,5,0,0,3,33,5,0,0,3,33,33,4,33,255,4,4,33,4,4,33,4,4,33,33,4,5,0,0,0,1,255,0,4
        .byte 255,1,16,33,5,255,0,24,33,6,255,2,24,33,33,4,33,4,4,33,4,33,4,4,4,255,33,9,4,255,33,4
        .byte 255,33,27,18,255,14,5,20,62,255,1,17,33,12,20,20,20,15,20,59,255,0,18,33,16,15,15,17,33,63,62,255,2,18
        .byte 255,33,26,255,1,26,255,0,26,255,2,26,255,33,27,8,255,1,24,33,5,255,0,24,33,5,0,0,0,255,2,21,33
        .byte 5,0,0,3,255,33,22,5,0,0,0,255,1,21,33,5,255,0,24,33,5,255,0,10,255,2,14,33,5,255,0,9,3
        .byte 255,33,15,5,55,56,255,0,8,255,1,14,33,5,57,58,255,0,22,33,6,2,2,2,61,64,255,2,19,255,33,5,12
        .byte 13,255,33,19
        .byte 4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,4,5,0,0,3,4,5
        .byte 0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,4,5,0,0,3,4,5,0,0,3,4,5,0,0
        .byte 3,4,5,0,0,3,4,5,0,0,3,4,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4
        .byte 5,0,0,3,4,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,4,5
        .byte 0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,4,5,0,0,3,4,5,0,0
        .byte 3,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4
        .byte 5,0,0,3,4,5,0,0,3,4,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0,3,4,5,0,0
        .byte 3,33,4,5,0,0,0,1,0,0,0,3,4,5,0,0,3,4,5,44,44,3,4,6,0,0,7,4,4,5,255,0,7
        .byte 3,4,5,0,0,3,4,5,44,44,3,4,4,0,0,4,4,4,6,255,2,7,7,4,5,0,0,3,4,6,2,2,7
        .byte 4,4,0,0,33,255,4,9,33,4,4,5,0,0,3,255,4,4,33,4,8,0,0,9,33,255,1,11,255,0,4,255,1,6
        .byte 0,0,0,3,4,255,0,24,3,33,255,2,24,7,4,4,33,4,4,4,33,255,4,12,33,4,4,4,33,4,33,33,255,1,26
        .byte 255,0,26,255,2,26,255,33,26
        .byte 255,33,26,255,1,26,255,0,26,255,2,26,255,33,26,255,1,24,9,33,255,0,24,3,33,255,2,21,0,0,0,3,255,33,22
        .byte 5,0,0,3,33,255,1,21,0,0,0,3,33,255,0,24,3,33,255,2,9,255,0,7,255,2,8,7,255,33,10,5,255,0,5
        .byte 3,255,33,10,255,1,4,9,33,18,19,33,5,255,0,6,255,1,10,255,0,4,3,33,12,13,33,61,64,255,0,15,255,2,4
        .byte 7,33,16,15,15,20,20,64,255,2,14,255,33,26,255,1,26,255,0,26,255,2,26,255,33,26
        .byte 22,5,0,0,3,22,5,0,0,3,22,5,0,0,3,255,22,4,35,22,255,35,5,22,5,0,0,3,22,5,0,0,3
        .byte 22,5,0,0,0,255,1,11,22,5,0,0,3,22,5,0,0,3,22,5,255,0,14,22,5,0,0,3,22,5,0,0,3
        .byte 22,6,255,2,14,22,5,0,0,3,22,5,0,0,3,255,22,10,35,22,22,35,22,35,22,5,0,0,0,1,255,0,4
        .byte 1,255,37,4,255,1,9,9,35,22,5,255,0,9,255,38,4,255,0,9,3,35,22,5,0,0,0,255,2,5,7,255,22,4
        .byte 6,255,0,4,2,0,0,0,3,35,22,5,0,0,3,255,22,11,5,0,0,3,22,5,0,0,3,35,22,5,0,0,3
        .byte 22,8,255,1,7,9,22,5,0,0,3,22,5,0,0,0,1,22,5,0,0,3,22,5,255,0,7,3,22,5,0,0,3
        .byte 22,5,255,0,4,22,5,0,0,3,22,5,0,0,0,2,0,0,0,3,22,5,0,0,3,22,6,255,2,4,22,5,0,0
        .byte 3,22,5,0,0,3,22,5,0,0,3,22,5,0,0,61,22,22,35,22,22,35,22,5,0,0,3,22,5,0,0,3,22
        .byte 5,0,0,3,22,5,0,60,20,255,14,4,19,35,33,5,0,0,3,22,5,0,0,3,22,5,0,0,3,22,5,0,0
        .byte 63,255,22,5,35,22,5,0,0,3,33,5,0,0,3,33,5,0,0,3,22,5,0,0,3,22,8,1,1,9,22,22,5
        .byte 0,0,3,22,5,0,0,3,22,5,0,0,3,22,5,0,0,3,33,5,42,23,3,35,33,5,0,0,3,33,5,0,0
        .byte 3,22,5,0,0,3,33,5,0,0,3,22,5,23,23,3,35,22,5,0,0,3,33,5,0,0,3,33,5,0,0,3,22
        .byte 5,0,0,3,33,5,0,0,3,33,33,5,0,0,3,33,5,0,0,3,22,5,0,0,3,22,5,0,0,3,33,5,0,0
        .byte 3,35,33,5,0,0,3,33,5,0,0,3,33,5,0,0,3,33,5,0,0,3,33,5,0,0,3,33
        .byte 4,5,0,0,3,33,5,0,0,3,33,5,0,0,3,33,5,0,0,3,33,5,0,0,3,34,4,5,0,0,3,33,5
        .byte 0,0,3,33,5,0,0,3,33,5,0,0,3,33,5,0,0,0,1,33,5,0,0,3,33,6,0,0,7,33,5,0,0
        .byte 3,33,5,0,0,3,33,5,255,0,4,4,5,0,0,3,33,33,0,0,33,33,5,0,0,3,33,5,0,0,3,33,6
        .byte 255,2,4,4,5,0,0,3,33,33,0,0,33,33,5,0,0,3,33,5,0,0,3,33,33,34,33,34,34,4,5,0,0
        .byte 3,33,33,0,0,33,33,5,0,0,3,33,5,0,0,0,255,1,6,33,5,0,0,3,33,33,0,0,33,33,5,0,0
        .byte 3,33,5,255,0,9,4,5,0,0,3,33,8,0,0,9,33,5,0,0,3,33,6,255,2,9,33,5,0,0,3,33,6
        .byte 0,0,7,33,5,0,0,3,255,33,5,34,33,33,34,33,34,4,5,0,0,3,33,33,0,0,33,33,5,0,0,0,255,1,11
        .byte 33,5,0,0,3,33,33,0,0,33,33,5,255,0,14,33,5,0,0,3,33,33,0,0,33,33,6,255,2,14,33,5,0,0
        .byte 3,33,33,0,0,255,33,13,34,33,34,34,33,5,0,0,3,33,8,0,0,9,33,8,255,1,14,33,5,0,0,3,33
        .byte 5,0,0,3,33,5,255,0,14,33,5,0,0,3,33,5,0,0,3,33,64,0,0,0,2,60,20,64,255,2,7,33,5
        .byte 0,0,3,33,5,0,0,3,33,62,0,0,3,33,33,13,255,33,5,34,33,34,1,0,0,0,3,33,5,0,0,3,33
        .byte 5,0,0,61,33,18,20,19,33,8,255,1,5,255,0,4,3,33,5,0,0,3,33,5,0,0,20,14,20,20,13,33,5
        .byte 55,56,0,0,0,255,2,4,7,33,5,0,0,3,33,5,0,0,63,33,16,15,17,33,6,57,58,2,2,2,255,33,6
        .byte 5,0,0,3,33,5,0,0,3,255,33,6,34,33,34,34,34
        .byte 255,33,6,5,0,0,3,33,5,0,0,3,255,33,6,34,33,34,34,34,255,1,4,9,33,5,0,0,3,33,5,0,0,0
        .byte 255,1,11,255,0,4,3,33,5,0,0,3,33,5,255,0,14,2,0,0,0,3,33,5,0,0,3,33,6,2,2,2,61
        .byte 64,0,0,0,255,2,6,33,5,0,0,3,33,5,0,0,3,255,33,6,62,0,0,3,33,33,34,33,34,34,33,5,0,0
        .byte 3,33,5,45,53,0,255,1,6,0,0,0,61,33,18,14,14,19,33,33,5,0,0,3,33,5,52,54,255,0,10,20,14
        .byte 20,20,20,13,34,33,5,0,0,3,33,6,255,2,9,0,0,0,63,33,16,15,15,17,33,33,5,0,0,3,255,33,11
        .byte 5,0,0,3,33,33,34,33,34,34,33,5,0,0,0,255,1,11,0,0,0,3,33,8,1,1,9,34,33,5,255,0,17
        .byte 3,33,5,21,21,3,34,33,6,255,2,14,0,0,0,3,33,5,21,21,3,255,33,17,5,0,0,3,34,5,0,0,3
        .byte 34,255,1,16,0,0,0,3,33,5,0,0,3,33,255,0,19,3,33,5,0,0,3,33,255,2,19,7,33,5,0,0,3
        .byte 34,255,33,18,34,33,34,5,0,0,3,34,255,1,21,0,0,0,3,34,255,0,24,3,34,255,2,24,7,34,255,33,21
        .byte 34,33,34,34,34
        .byte 255,35,27,8,255,1,7,9,35,18,14,19,35,8,255,1,10,35,5,255,0,7,3,35,12,20,13,35,5,0,42,23,255,0,7
        .byte 35,5,0,0,0,2,0,0,0,3,35,16,15,17,35,6,2,23,23,255,2,7,35,5,0,0,3,35,5,0,0,3,255,35,17
        .byte 5,0,0,3,35,5,0,0,0,255,1,16,35,5,0,0,3,35,5,255,0,19,35,5,0,0,3,35,6,255,2,4,255,0,4
        .byte 255,2,11,35,5,0,0,3,255,35,6,5,0,0,3,255,35,12,5,0,0,0,255,1,4,9,35,5,0,0,3,35,8
        .byte 255,1,9,35,5,255,0,7,3,35,5,0,0,3,35,5,255,0,9,35,6,255,2,4,0,0,0,3,35,5,0,0,3
        .byte 35,5,0,0,0,255,2,6,255,35,6,5,0,0,3,35,5,0,0,3,35,5,0,0,3,255,35,6,255,1,4,9,35
        .byte 5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,8,255,1,4,255,0,4,3,35,5,0,0,3,35,5,0,0,3
        .byte 35,5,0,0,3,35,5,255,0,4,2,0,0,0,3,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,5,0,0,0
        .byte 2,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,35,5,0,0,3,35
        .byte 5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,35,5,0,0,3,35,5,0,0,3,35,5,0,0
        .byte 3,35,5,0,0,3,35,5,0,0,3,35,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35
        .byte 5,0,0,3,35,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35
        .byte 35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,1,0,0,0,3,35,5
        .byte 0,0,3,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,255,0,4,3,35,5,0,0,3,35,5,55,56,3
        .byte 35,5,0,0,3,35,5,0,0,3,35,255,2,4,7,35,5,0,0,3,35,6,57,58,7,35,5,0,0,3,35,5,0,0
        .byte 3,255,35,7,5,0,0,3,255,35,6,5,0,0,3,35,5,0,0,3,35,35,8,255,1,4,0,0,0,3,35,8,255,1,4
        .byte 0,45,50,3,35,5,0,0,3,35,35,5,255,0,7,3,35,5,255,0,5,49,51,3,35,5,0,0,3,35,35,5,0,0,0
        .byte 255,2,4,7,35,5,0,0,0,255,2,4,7,35,5,0,0,3,35,35,5,0,0,3,255,35,6,5,0,0,3,255,35,6
        .byte 5,0,0,3,35,1,255,0,4,255,1,6,255,0,4,255,1,6,0,0,0,3,35,255,0,24,3,35,2,255,0,4,255,2,19
        .byte 7,35,35,5,0,0,3,255,35,22,5,0,0,61,35,18,14,14,19,35,8,255,1,14,35,5,0,60,255,20,5,13,35
        .byte 5,55,56,255,0,12,35,5,0,0,63,35,16,20,15,17,35,6,57,58,255,2,12,35,5,0,0,3,35,35,20,255,35,7
        .byte 34,255,35,5,34,35,35,34,35,35,5,0,0,0,1,63,20,62,255,1,15,9,34,35,5,255,0,22,3,35,35,6,255,2,10
        .byte 61,64,255,2,7,0,0,0,3,34,35,34,35,35,34,35,34,34,34,35,34,35,34,34,35,35,34,35,34,34,34,5,0,0
        .byte 3,34
        .byte 255,34,21,5,0,0,3,34,255,1,19,9,34,5,0,0,3,34,255,0,19,3,34,5,0,0,60,20,255,2,16,0,0,0
        .byte 3,34,5,0,0,3,255,34,17,5,0,0,3,34,5,0,0,3,34,255,1,4,9,34,8,1,1,9,34,18,14,14,19
        .byte 34,5,0,0,3,34,5,0,0,61,14,255,0,4,3,34,5,21,21,3,34,12,20,20,13,34,5,0,0,3,34,5,0,0
        .byte 63,15,255,2,4,7,34,5,21,21,3,34,12,20,20,13,34,5,0,0,3,34,64,0,0,3,255,34,7,5,0,0,3
        .byte 34,12,20,20,13,34,5,0,0,3,34,62,0,0,3,34,255,1,6,0,0,0,3,34,12,20,20,13,34,5,0,0,3
        .byte 34,5,0,0,3,34,255,0,9,3,34,12,20,20,13,34,5,0,0,3,34,5,0,0,61,14,255,2,9,7,34,16,15,15
        .byte 17,34,5,0,0,3,34,5,0,0,63,15,255,34,16,5,0,0,3,34,5,0,0,3,34,255,1,9,9,34,18,14,14
        .byte 19,34,5,0,0,3,34,5,0,0,60,20,255,0,9,3,34,12,20,20,13,34,5,0,0,3,34,64,0,0,3,34,255,2,6
        .byte 0,0,0,3,34,16,15,15,17,34,5,0,0,3,34,62,0,0,3,255,34,7,5,0,0,3,255,34,6,5,0,0,3
        .byte 34,5,0,0,3,34,255,1,6,255,0,4,255,1,4,9,34,5,0,0,3,34,5,0,0,60,20,255,0,14,3,34,5
        .byte 0,0,3,34,5,0,0,3,34,255,2,6,255,0,4,2,0,0,0,3,34,5,0,0,3,34,5,0,0,3,255,34,7
        .byte 5,0,0,3,34,5,0,0,3,34,5,0,0,3,34,5,0,0,3,34
        .byte 255,34,6,5,0,0,3,34,5,0,0,3,34,5,0,0,3,34,5,0,0,3,34,255,1,4,9,34,5,0,0,3,34
        .byte 5,0,0,3,34,5,0,0,3,34,5,0,0,3,34,255,0,4,3,34,5,0,0,3,34,5,0,0,3,34,5,0,0
        .byte 3,34,5,0,0,60,20,2,0,0,0,3,34,5,0,0,3,34,5,0,0,3,34,5,0,0,3,34,5,0,0,3,34,34
        .byte 5,0,0,3,34,5,0,0,3,34,5,0,0,3,34,5,0,0,3,34,5,0,0,3,34,34,5,0,0,3,34,5,55
        .byte 56,3,34,5,23,21,3,34,5,0,0,3,34,5,0,0,60,20,34,5,0,0,3,34,5,57,58,3,34,5,21,23,3
        .byte 34,5,0,0,3,34,5,0,0,3,34,34,5,0,0,3,34,6,2,2,7,34,5,0,0,3,34,5,0,0,3,34,5
        .byte 0,0,60,20,34,5,0,0,3,255,34,6,5,55,56,3,34,5,0,0,3,34,5,0,0,3,34,34,5,0,0,3,34
        .byte 18,14,14,19,34,5,57,58,3,34,5,0,0,3,34,5,0,0,0,1,34,5,0,0,3,34,16,15,15,17,34,6,2,2
        .byte 7,34,5,0,0,3,34,5,255,0,4,34,5,0,0,3,255,34,11,5,0,0,3,34,61,64,2,2,2,34,5,0,0,0
        .byte 255,1,4,9,34,18,14,14,19,34,5,0,0,3,34,34,20,255,34,4,5,255,0,7,3,34,16,15,15,17,34,5,0,0
        .byte 3,34,20,62,1,1,1,34,5,255,0,7,3,255,34,6,5,0,0,3,34,20,55,56,0,0,34,5,255,0,8,255,1,6
        .byte 0,0,0,3,34,20,57,58,2,2,34,5,255,0,17,3,255,34,7,5,255,0,18,255,1,6,34,5,255,0,24,34,6
        .byte 255,2,24,255,34,26
        .byte 255,35,26,1,1,1,9,35,35,8,1,37,37,255,1,15,35,255,0,4,37,37,0,0,38,38,255,0,14,3,35,255,2,4
        .byte 38,38,2,7,35,35,6,255,0,4,2,255,0,4,2,0,0,0,3,255,35,12,5,0,0,3,35,5,0,0,3,35,5
        .byte 0,0,3,35,255,1,6,9,35,18,19,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,255,0,6,3,35,12
        .byte 13,35,5,0,0,3,35,5,0,0,3,35,6,0,0,7,35,255,2,6,7,35,16,17,35,5,0,0,3,35,5,0,0
        .byte 3,35,35,0,0,255,35,13,5,0,0,3,35,6,0,0,7,35,35,0,0,35,35,255,1,11,0,0,0,3,35,35,0,0
        .byte 35,35,8,0,0,9,35,255,0,14,3,35,35,0,0,35,35,6,0,0,7,35,255,2,14,7,35,8,0,0,9,35,35
        .byte 0,0,255,35,18,6,0,0,7,35,35,0,0,35,35,255,1,14,9,35,35,0,0,35,35,8,0,0,9,35,255,0,14
        .byte 3,35,35,0,0,35,35,5,0,0,3,35,255,2,6,255,0,4,2,0,0,0,3,35,8,0,0,9,35,5,0,0,3
        .byte 255,35,7,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,35,8,1,1,9,35,5,0,0,3
        .byte 35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,35,5,55,56,3,35,5,0,0,3,35,5,0,0,3,35,5
        .byte 0,0,3,35,5,0,0,3,35,35,5,57,58,3,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,5,0,0
        .byte 3,35,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35
        .byte 35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,35,5,0,0,3,35,5
        .byte 0,0,3,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,35,5,0,0,3,35,5,0,0,3,35,5,0,0
        .byte 3,35,5,0,0,3,35,5,0,0,3,35,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35
        .byte 5,21,65,3,35,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,5,65,65,3,35,35,5
        .byte 0,0,3,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,6,2,2,7,35,35,5,0,0,3,35,5,0,0
        .byte 3,35,5,0,0,3,35,5,0,0,3,255,35,7,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,5,0,0,0
        .byte 255,1,4,9,35,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,5,255,0,5,23,23,3,35,35,5,0,0
        .byte 3,35,5,0,0,3,35,5,0,0,3,35,5,255,0,5,23,23,3,35,35,5,0,0,3,35,5,0,0,3,35,5,0,0
        .byte 3,35,6,255,2,7,7,35,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,255,35,12,5,0,0,3,35,5,0,0
        .byte 3,35,5,0,0,3,35,8,255,1,7,9,35,1,0,0,0,3,35,5,0,0,3,35,5,0,0,3,35,5,255,0,7
        .byte 3,35,255,0,4,3,35,5,0,0,3,20,5,0,0,3,35,5,0,0,0,2,0,0,0,3,35,2,0,0,0,3,35
        .byte 5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,5,0,0,3,35,35,5,0,0,3,35,5,0,0,3,35,5,0,0
        .byte 3,20,5,0,0,3,35,5,0,0,3,20,35,5,0,0,0,1,0,0,0,3,20,5,0,0,3,35,5,0,0,3,34
        .byte 5,0,0,3,34,34,5,255,0,7,3,35,5,0,0,3,20,5,0,0,3,20,5,0,0,3,20,34,6,255,2,7,7
        .byte 34,5,0,0,3,34,5,0,0,3,34,5,0,0,3,255,34,9,20,34,34,5,0,0,3,34,5,0,0,3,34,5,0,0
        .byte 3,34
        .byte 255,34,11,5,0,0,3,34,5,0,0,3,34,5,0,0,3,34,34,8,255,1,9,255,0,4,20,255,0,4,20,0,0,0
        .byte 3,34,34,5,255,0,13,20,255,0,4,20,0,0,0,3,34,20,59,0,0,61,64,255,0,8,61,20,59,0,0,0,20
        .byte 64,0,0,3,34,34,5,0,0,63,20,0,0,0,61,255,20,6,0,0,0,60,20,62,0,0,3,34,34,5,0,0,0
        .byte 20,255,0,14,20,59,0,0,3,34,20,59,0,0,60,20,255,0,14,20,0,0,0,3,34,34,5,0,0,0,20,64,255,0,6
        .byte 61,64,255,0,4,61,20,0,0,0,61,34,34,5,0,0,61,255,20,16,0,0,0,20,34,20,59,0,0,63,20,62,255,0,17
        .byte 63,34,34,5,0,0,0,20,0,0,55,56,255,0,14,3,34,34,5,0,0,61,20,64,0,57,58,255,0,10,61,64,0,0
        .byte 3,34,34,5,0,0,63,255,20,16,62,0,0,3,34,34,5,0,0,60,20,62,255,0,12,63,20,0,0,0,3,34,34
        .byte 5,0,0,0,20,255,0,14,20,0,0,0,3,34,34,64,0,0,0,20,255,0,14,20,59,0,0,3,34,20,20,0,0,0
        .byte 20,0,0,0,61,64,255,0,4,61,64,0,0,0,20,0,0,0,3,34,34,62,0,0,0,20,0,0,0,63,20,59,0,0
        .byte 60,20,62,0,0,0,20,0,0,0,3,34,34,5,0,0,0,20,255,0,4,20,255,0,4,20,255,0,4,20,0,0,0
        .byte 3,34,20,59,0,0,0,20,255,0,4,20,255,0,4,20,255,0,4,20,0,0,0,3,34,34,5,0,0,0,20,255,0,4
        .byte 20,255,0,4,20,255,0,4,20,0,0,0,3,34
        .byte 34,5,0,0,3,34,5,0,0,0,20,255,0,4,20,255,0,4,20,0,0,0,3,34,34,5,0,0,0,1,255,0,4
        .byte 20,255,0,4,20,255,0,4,20,0,0,0,3,34,34,5,255,0,8,20,255,0,4,20,255,0,4,20,0,0,0,3,34,34
        .byte 64,255,0,7,61,20,64,0,0,0,20,64,0,0,0,20,0,0,0,3,34,34,34,255,20,9,62,0,0,0,20,62,0,0,0
        .byte 20,0,0,0,3,34,34,20,62,255,0,12,20,255,0,4,20,0,0,0,3,34,34,59,255,0,13,20,0,0,0,61,20
        .byte 0,0,0,3,34,34,20,64,0,0,0,61,64,0,0,61,64,0,0,0,20,0,0,0,20,20,64,0,0,3,34,34,34
        .byte 255,20,9,62,0,0,0,20,0,0,0,63,20,20,0,0,3,34,1,1,255,0,6,61,20,20,59,0,0,61,20,255,0,4
        .byte 20,62,0,0,3,34,255,0,5,55,56,60,20,20,20,59,0,0,63,20,255,0,4,20,0,0,0,3,34,2,2,0,0,0
        .byte 57,58,0,63,20,20,59,0,0,0,20,255,0,4,20,0,0,0,61,34,34,34,255,20,9,62,0,0,0,20,255,0,4
        .byte 20,0,0,0,20,34,1,1,255,0,13,20,255,0,4,20,0,0,0,63,34,255,0,15,20,255,0,4,20,0,0,0,3
        .byte 34,2,2,255,0,12,61,20,255,0,4,20,0,0,0,3,34,34,34,255,20,14,64,0,0,0,20,0,65,44,3,34,1,1
        .byte 255,0,5,63,62,255,0,6,63,62,0,0,0,20,0,65,44,3,34,255,0,20,20,0,0,0,3,34,255,2,20,20,64
        .byte 2,2,7,255,34,27
numsc:
        .byte 25
.out .sprintf("SCREENS SIZE = %d", (* - scdat))

nmedat:
        .byte 0,0,16,104,1,7,16,88,1,7,16,120,255
        .byte 0,0,104,24,1,2,80,128,3,11,152,120,255
        .byte 0,0,152,24,2,5,80,64,2,5,96,104,3,5,96,64,3,5,80,104,6,12,136,184,255
        .byte 0,0,24,8,2,6,16,40,2,6,16,88,2,6,16,136,3,6,16,64,3,6,16,112,3,6,16,160,255
        .byte 0,0,80,24,4,8,64,104,2,11,48,144,255
        .byte 0,0,16,24,1,7,16,128,1,7,48,24,4,14,112,64,255
        .byte 0,0,72,24,4,8,120,144,1,2,48,144,255
        .byte 0,0,24,8,4,8,48,184,7,13,144,144,255
        .byte 0,0,56,24,1,7,48,96,1,2,16,88,255
        .byte 0,0,8,24,1,2,112,16,1,2,128,112,4,8,144,24,255
        .byte 0,0,16,64,5,10,72,104,2,11,128,64,3,11,104,64,2,11,80,64,7,13,48,144,255
        .byte 0,0,56,8,5,10,24,72,5,10,144,40,5,10,144,56,255
        .byte 0,0,16,24,4,8,16,104,2,5,88,184,3,11,104,184,255
        .byte 0,0,24,8,5,10,104,80,5,10,120,104,255
        .byte 0,0,16,24,2,6,48,96,3,6,48,112,5,10,104,144,3,11,48,64,2,11,56,144,255
        .byte 0,0,16,24,2,5,32,64,2,5,96,64,3,5,48,64,3,5,80,64,255
        .byte 0,0,24,24,5,10,40,144,255
        .byte 0,0,152,24,4,8,56,104,255
        .byte 0,0,16,24,5,10,152,88,1,2,80,136,6,12,64,24,255
        .byte 0,0,152,144,4,8,144,64,5,10,88,184,255
        .byte 0,0,24,24,4,8,112,64,1,7,128,152,1,7,144,24,7,13,48,104,255
        .byte 0,0,88,16,3,6,24,40,2,6,16,72,2,5,80,144,2,5,96,184,3,5,64,184,3,5,112,144,1,2,16,184,2,11,128,144,3,11,112,184,255
        .byte 0,0,16,24,4,8,144,24,255
        .byte 0,0,16,104,2,11,120,104,255
        .byte 0,0,16,24,255
.out .sprintf("POSITIONS SIZE = %d", (* - nmedat))

NUMOBJ = 17
objdta:
        .byte 0,0,1,252,3,250,5,22,7,246,9,46,15,224,18,64,31,192,16,64,31,192,31,192,3,128,253,128,127,0,0,0,1,128,72,1,128,72
        .byte 0,0,1,252,3,250,5,22,7,246,9,46,15,224,18,64,31,192,16,64,31,192,31,192,3,128,253,128,127,0,0,0,6,112,104,6,112,104
        .byte 0,0,1,252,3,250,5,22,7,246,9,46,15,224,18,64,31,192,16,64,31,192,31,192,3,128,253,128,127,0,0,0,7,144,184,7,144,184
        .byte 0,0,1,252,3,250,5,22,7,246,9,46,15,224,18,64,31,192,16,64,31,192,31,192,3,128,253,128,127,0,0,0,16,88,184,16,88,184
        .byte 0,0,1,252,3,250,5,22,7,246,9,46,15,224,18,64,31,192,16,64,31,192,31,192,3,128,253,128,127,0,0,0,19,56,64,19,56,64
        .byte 56,28,113,142,248,31,253,187,187,213,22,104,13,176,90,218,91,218,13,176,22,104,187,213,245,171,232,23,113,142,56,28,22,32,184,22,32,184
        .byte 0,0,28,56,38,76,103,222,95,190,127,254,127,250,63,244,31,232,15,208,6,160,3,64,1,128,0,0,0,0,0,0,8,88,64,8,88,64
        .byte 0,0,28,56,38,76,103,222,95,190,127,254,127,250,63,244,31,232,15,208,6,160,3,64,1,128,0,0,0,0,0,0,14,136,184,14,136,184
        .byte 0,0,28,56,38,76,103,222,95,190,127,254,127,250,63,244,31,232,15,208,6,160,3,64,1,128,0,0,0,0,0,0,17,24,144,17,24,144
        .byte 0,0,0,0,32,4,81,138,34,68,97,134,97,134,41,148,107,214,111,246,124,62,59,220,116,46,11,16,122,94,60,60,4,32,64,4,32,64
        .byte 0,0,0,0,32,4,81,138,34,68,97,134,97,134,41,148,107,214,111,246,124,62,59,220,116,46,11,16,122,94,60,60,5,88,104,5,88,104
        .byte 0,0,0,0,32,4,81,138,34,68,97,134,97,134,41,148,107,214,111,246,124,62,59,220,116,46,11,16,122,94,60,60,24,136,184,24,136,184
        .byte 0,0,31,128,8,64,29,32,26,144,13,80,26,144,29,80,10,144,29,80,15,240,0,0,31,240,21,80,0,0,0,0,5,80,184,5,80,184
        .byte 0,0,31,128,8,64,29,32,26,144,13,80,26,144,29,80,10,144,29,80,15,240,0,0,31,240,21,80,0,0,0,0,12,80,144,12,80,144
        .byte 0,0,31,128,8,64,29,32,26,144,13,80,26,144,29,80,10,144,29,80,15,240,0,0,31,240,21,80,0,0,0,0,22,72,184,22,72,184
        .byte 0,0,0,0,3,128,7,192,11,160,12,96,3,128,5,64,30,240,56,56,48,24,32,8,48,24,28,112,15,224,3,128,9,48,144,9,48,144
        .byte 128,0,0,0,0,0,128,0,0,0,0,0,128,0,0,0,0,0,128,0,0,0,0,0,128,0,0,0,0,0,128,0,5,120,8,5,120,8
.out .sprintf("OBJECTS SIZE = %d", (* - objdta))

font:
        .byte 0,0,0,0,0,0,0,0
        .byte 205,226,126,205,33,127,205,142
        .byte 2,33,5,125,54,0,123,254
        .byte 13,40,13,254,36,200,52,254
        .byte 28,200,52,254,20,32,228,201
        .byte 33,11,138,58,53,125,230,7
        .byte 200,71,54,255,35,16,251,33
        .byte 11,138,71,14,0,229,197,58
        .byte 54,125,129,205,158,135,58,50
        .byte 125,61,50,50,125,205,226,126
        .byte 205,108,122,193,225,119,62,60
        .byte 50,140,127,35,12,16,222,175
        .byte 205,158,135,58,50,125,61,50
        .byte 50,125,24,159,205,142,2,123
        .byte 60,40,249,6,6,33,11,138
        .byte 123,190,40,240,35,16,250,201
        .byte 254,254,198,198,198,254,254,0
        .byte 248,248,56,56,56,56,56,0
        .byte 248,254,14,254,224,254,254,0
        .byte 254,254,6,254,6,254,254,0
        .byte 198,198,198,254,254,6,6,0
        .byte 248,248,224,254,14,254,254,0
        .byte 254,254,192,254,198,254,254,0
        .byte 254,254,6,6,6,6,6,0
        .byte 254,254,198,254,198,254,254,0
        .byte 254,254,198,254,6,254,254,0
        .byte 0,0,60,60,0,60,60,0
        .byte 60,126,60,90,131,131,70,60
        .byte 255,85,0,0,0,0,85,170
        .byte 0,0,0,0,126,0,0,0
        .byte 254,87,1,2,1,2,87,170
        .byte 254,254,14,254,224,0,224,0
        .byte 126,254,254,253,125,116,41,18
        .byte 254,254,14,254,238,238,238,0
        .byte 254,254,206,248,206,254,254,0
        .byte 254,254,254,224,254,254,254,0
        .byte 248,254,238,238,238,254,254,0
        .byte 254,254,224,254,224,254,254,0
        .byte 254,254,224,254,254,224,224,0
        .byte 254,254,224,238,238,254,254,0
        .byte 238,238,238,254,238,238,238,0
        .byte 254,254,56,56,56,254,254,0
        .byte 254,254,14,238,238,254,254,0
        .byte 238,238,238,248,238,238,238,0
        .byte 224,224,224,238,238,254,254,0
        .byte 238,238,254,254,198,238,238,0
        .byte 246,246,246,254,222,222,222,0
        .byte 254,254,238,238,238,254,254,0
        .byte 254,254,14,254,254,192,192,0
        .byte 254,254,238,238,224,238,238,0
        .byte 254,254,14,254,240,238,238,0
        .byte 254,254,224,254,14,254,254,0
        .byte 254,254,56,56,56,56,56,0
        .byte 238,238,238,238,238,254,254,0
        .byte 238,238,238,238,238,124,56,0
        .byte 238,238,238,198,254,238,238,0
        .byte 238,238,254,56,254,238,238,0
        .byte 238,238,238,254,14,254,254,0
        .byte 254,254,14,254,224,254,254,0
        .byte 255,255,255,255,255,255,255,255
        .byte 15,59,113,121,244,248,244,170
        .byte 0,255,255,254,246,172,88,160
        .byte 4,251,255,255,255,255,85,170
        .byte 0,0,0,0,0,56,56,0
        .byte 74,174,170,234,170,170,170,0
        .byte 0,0,0,0,0,0,0,0
        .byte 254,253,251,245,233,209,161,127
        .byte 255,213,160,192,128,192,128,192
        .byte 255,85,0,0,0,0,0,0
        .byte 255,85,3,1,1,2,1,2
        .byte 1,2,1,2,1,2,1,2
        .byte 1,2,1,2,1,3,87,174
        .byte 0,0,0,0,0,0,85,170
        .byte 128,192,128,192,128,192,213,234
        .byte 128,192,128,192,128,192,128,192
        .byte 255,177,241,241,143,78,78,60
        .byte 170,168,170,170,170,82,82,0
        .byte 76,162,138,74,42,170,76,0
        .byte 73,175,173,173,169,169,73,0
        .byte 30,61,57,60,8,236,232,112
        .byte 231,195,153,44,60,153,195,231
        .byte 66,160,34,34,42,42,122,0
        .byte 68,170,136,204,136,138,132,0
        .byte 108,182,254,254,124,56,16,0
        .byte 0,65,99,73,93,119,99,119
        .byte 120,76,86,106,82,106,82,126
        .byte 73,162,138,171,170,170,106,0
        .byte 114,165,36,34,33,37,34,0
        .byte 132,170,168,204,168,170,164,0
        .byte 164,170,168,68,66,74,68,0
        .byte 168,168,168,168,170,170,110,0
        .byte 238,132,132,228,132,132,228,0
        .byte 48,48,48,48,0,48,48,0
        .byte 224,248,252,254,254,255,255,254
        .byte 0,60,120,0,0,0,0,0
        .byte 0,124,68,92,68,124,0,0
.out .sprintf("FONT SIZE = %d", (* - font))

jtab:
        .byte 99
keys:   .byte 66,97,104,72,73,16,55,48,49,17,18
.out .sprintf("KEYS SIZE = %d", (* - keys))

data_end:
.out .sprintf("TOTAL DATA SIZE = %d", (* - data_start))
