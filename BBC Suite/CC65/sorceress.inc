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
WINDOWHGT = 22
WINDOWWID = 28 ;面面面面面面面面面面面面
MAPWID = 7
        .byte 255,255,255,255,255,255,255
mapdat:
        .byte 255,2,1,0,7,8,255,255,3,6,15,12,9,255,255,4,5,14,13,10,255,255,255,255,255,16,11,255
        .byte 255,255,255,255,255,255,255
stmap:  .byte 3

evnt00:
        lda #0
        cmp varg
        beq *+5
        jmp a00021
        rts
a00021: lda #13
        cmp varg
        beq *+5
        jmp a00176
        lda #8
        ldy #8
        sta (z80_ix),y
        lda #112
        iny; ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #23
        jsr drpob
        lda #96
        ldy #9
        sta (z80_ix),y
        lda #13
        ldy #6
        sta (z80_ix),y
        lda #14
        sta varg
        lda #5
        sta chary
        lda #4
        sta charx
        lda #12
        jsr dmsg
        lda #50
        jsr delay
        rts
a00176: lda #22
        jsr gotob
        cmp #255
        beq :+
        jmp a00251
:
        lda #14
        cmp varg
        bne *+5
        jmp a00251
        lda #13
        sta varg
        ; lda #13
        ldy #6
        sta (z80_ix),y
        lda #96
        ldy #9
        sta (z80_ix),y
a00251: lda #1
        cmp varg
        beq *+5
        jmp a00326
        lda joyval
        and #16
        beq :+
        jmp a00323
:
        lda #17
        sta chary
        lda #3
        sta charx
        lda #10
        jsr dmsg
        lda #2
        sta varg
        jmp a00326
a00323: rts
a00326: lda #0
        cmp vard
        beq *+5
        jmp a00348
        jmp a00377
a00348: lda #1
        cmp vara
        beq *+5
        jmp a00377
        lda #250
        asl a
        sta sndtyp
a00377: lda #15
        cmp scno
        beq *+5
        jmp a00431
        lda #152
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a00431
        ; lda #152
        ; ldy #8
        sta (z80_ix),y
a00431: lda #0
        ldy #11
        sta (z80_ix),y
        jsr skobj
        sta varobj
        lda #255
        cmp varobj
        bne *+5
        jmp a00481
        lda varobj
        jsr getob
a00481: lda varobj
        cmp #18
        bcc *+5
        jmp a00694
        lda #6
        sta varh
        lda #1
        sta varl
        lda #5
        sta loopa
a00523: lda varl
        sta chary
        lda #29
        sta charx
        lda #13
        jsr dmsg
        lda varl
        clc
        adc #1
        sta varl
        lda varl
        sta chary
        lda #29
        sta charx
        lda #14
        jsr dmsg
        lda #2
        sta z80_c
        lda varl
        clc
        adc z80_c
        sta varl
        dec loopa
        beq :+
        jmp a00523
:
        lda #70
        sta varl
        lda #6
        sta loopa
a00643: lda varl
        asl a
        sta sndtyp
        lda #10
        sta z80_c
        lda varl
        clc
        adc z80_c
        sta varl
        dec loopa
        beq :+
        jmp a00643
:
a00694: lda #18
        cmp varobj
        beq *+5
        jmp a00719
        lda #5
        sta varg
a00719: lda #19
        cmp varobj
        beq *+5
        jmp a00745
        lda #7
        sta varg
a00745: lda joyval
        and #1
        beq :+
        jmp a00941
:
        lda #2
        ldy #10
        cmp (z80_ix),y
        bne *+5
        jmp a00812
        lda #13
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
a00812: lda #2
        ldy #10
        sta (z80_ix),y
        jsr cangr
        beq :+
        jmp a00897
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
        lda #1
        ldy #11
        sta (z80_ix),y
a00897: lda #204
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp a00941
        lda #4
        ; ldy #9
        sta (z80_ix),y
        jsr scrr
        rts
a00941: lda joyval
        and #2
        beq :+
        jmp a01137
:
        lda #4
        ldy #10
        cmp (z80_ix),y
        bne *+5
        jmp a01009
        lda #14
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
a01009: lda #4
        ldy #10
        sta (z80_ix),y
        jsr cangl
        beq :+
        jmp a01094
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
        lda #1
        ldy #11
        sta (z80_ix),y
a01094: ldy #9
        lda (z80_ix),y
        cmp #4
        bcc *+5
        jmp a01137
        lda #204
        ; ldy #9
        sta (z80_ix),y
        jsr scrl
        rts
a01137: lda joyval
        and #4
        beq :+
        jmp a01334
:
        lda #3
        ldy #10
        cmp (z80_ix),y
        bne *+5
        jmp a01205
        lda #0
        ldy #6
        sta (z80_ix),y
        ; lda #0
        iny; ldy #7
        sta (z80_ix),y
a01205: lda #3
        ldy #10
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp a01290
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
        lda #1
        ldy #11
        sta (z80_ix),y
a01290: lda #156
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a01334
        lda #4
        ; ldy #8
        sta (z80_ix),y
        jsr scrd
        rts
a01334: lda joyval
        and #8
        beq :+
        jmp a01602
:
        lda #1
        ldy #10
        cmp (z80_ix),y
        bne *+5
        jmp a01401
        lda #12
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
a01401: lda #1
        ldy #10
        sta (z80_ix),y
        jsr cangu
        beq :+
        jmp a01558
:
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
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01545
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
a01545: lda #1
        ldy #11
        sta (z80_ix),y
a01558: ldy #8
        lda (z80_ix),y
        cmp #4
        bcc *+5
        jmp a01602
        lda #156
        ; ldy #8
        sta (z80_ix),y
        jsr scru
        rts
a01602: lda joyval
        and #16
        beq :+
        jmp a01672
:
        lda #0
        cmp varb
        beq *+5
        jmp a01672
        ldy #10
        lda (z80_ix),y
        sta varb
        lda #1
        sta z80_c
        ; lda #1
        sta z80_b
        jsr spawn
a01672: lda #1
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp a01769
        ; lda #1
        cmp vara
        beq *+5
        jmp a01732
        lda #0
        jsr animsp
        lda #2
        asl a
        sta sndtyp
a01732: lda #3
        cmp vara
        beq *+5
        jmp a01769
        lda #0
        jsr animsp
        lda #4
        asl a
        sta sndtyp
a01769: ldy #8
        lda (z80_ix),y
        sta varo
        iny; ldy #9
        lda (z80_ix),y
        sta varp
        rts
.out .sprintf("EVENT 0 SIZE = %d", (* - evnt00))

evnt01:
        lda varb
        ldy #10
        sta (z80_ix),y
        lda #0
        ; ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00052
        lda #1
        ; ldy #10
        sta (z80_ix),y
b00052: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00159
        lda #3
        sta loopa
b00083: jsr cangu
        beq :+
        jmp b00123
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp b00145
b00123: lda #255
        ldy #5
        sta (z80_ix),y
        lda #240
        sta varb
b00145: dec loopa
        beq :+
        jmp b00083
:
b00159: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00265
        lda #3
        sta loopa
b00189: jsr cangr
        beq :+
        jmp b00230
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp b00252
b00230: lda #255
        ldy #5
        sta (z80_ix),y
        lda #240
        sta varb
b00252: dec loopa
        beq :+
        jmp b00189
:
b00265: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00372
        ; lda #3
        sta loopa
b00296: jsr cangd
        beq :+
        jmp b00337
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp b00359
b00337: lda #255
        ldy #5
        sta (z80_ix),y
        lda #240
        sta varb
b00359: dec loopa
        beq :+
        jmp b00296
:
b00372: lda #4
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00479
        lda #3
        sta loopa
b00403: jsr cangl
        beq :+
        jmp b00443
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp b00465
b00443: lda #255
        ldy #5
        sta (z80_ix),y
        lda #240
        sta varb
b00465: dec loopa
        beq :+
        jmp b00403
:
b00479: lda #204
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp b00524
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #240
        sta varb
b00524: ldy #9
        lda (z80_ix),y
        cmp #4
        bcc *+5
        jmp b00568
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #240
        sta varb
b00568: lda #152
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp b00613
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #240
        sta varb
b00613: ldy #8
        lda (z80_ix),y
        cmp #8
        bcc *+5
        jmp b00657
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #240
        sta varb
b00657: lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp b00696
        lda #230
        asl a
        sta sndtyp
        jmp b00709
b00696: lda #250
        asl a
        sta sndtyp
b00709: lda #0
        jsr animsp
        rts
.out .sprintf("EVENT 1 SIZE = %d", (* - evnt01))

evnt02:
        lda #14
        cmp scno
        beq *+5
        jmp c00023
        jmp c00023
c00023: lda #255
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp c00060
        lda #0
        ; ldy #11
        sta (z80_ix),y
c00060: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00134
:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp c00130
        ; lda #0
        cmp vard
        beq *+5
        jmp c00130
        lda #30
        sta vard
c00130: jmp c00134
c00134: lda #0
        ldy #12
        cmp (z80_ix),y
        bcc *+5
        jmp c00184
        ; ldy #12
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #12
        sta (z80_ix),y
c00184: lda #1
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp c02142
        lda #49
        iny; ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c00273
        lda #4
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        lda #130
        asl a
        sta sndtyp
        rts
c00273: lda #46
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c00324
        lda #1
        ldy #7
        sta (z80_ix),y
        lda #150
        asl a
        sta sndtyp
        rts
c00324: lda #43
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c00376
        lda #2
        ldy #7
        sta (z80_ix),y
        lda #170
        asl a
        sta sndtyp
        rts
c00376: lda #40
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c00427
        lda #3
        ldy #7
        sta (z80_ix),y
        lda #190
        asl a
        sta sndtyp
        rts
c00427: lda #37
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c00466
        lda #200
        ldy #8
        sta (z80_ix),y
        rts
c00466: lda #8
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c01909
        lda #3
        ldy #7
        sta (z80_ix),y
        lda #1
        cmp scno
        beq *+5
        jmp c00689
        lda #3
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp c00597
        lda #40
        iny; ldy #8
        sta (z80_ix),y
        tya; lda #8
        iny; ldy #9
        sta (z80_ix),y
c00597: lda #1
        cmp varrnd
        beq *+5
        jmp c00643
        lda #88
        ldy #8
        sta (z80_ix),y
        lda #104
        iny; ldy #9
        sta (z80_ix),y
c00643: lda #2
        cmp varrnd
        beq *+5
        jmp c00689
        lda #152
        ldy #8
        sta (z80_ix),y
        lda #200
        iny; ldy #9
        sta (z80_ix),y
c00689: lda #2
        cmp scno
        beq *+5
        jmp c00920
        lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp c00782
        lda #8
        ldy #8
        sta (z80_ix),y
        lda #104
        iny; ldy #9
        sta (z80_ix),y
c00782: lda #1
        cmp varrnd
        beq *+5
        jmp c00829
        lda #152
        ldy #8
        sta (z80_ix),y
        ; lda #152
        iny; ldy #9
        sta (z80_ix),y
c00829: lda #2
        cmp varrnd
        beq *+5
        jmp c00875
        lda #40
        ldy #8
        sta (z80_ix),y
        ; lda #40
        iny; ldy #9
        sta (z80_ix),y
c00875: lda #3
        cmp varrnd
        beq *+5
        jmp c00920
        lda #152
        ldy #8
        sta (z80_ix),y
        lda #56
        iny; ldy #9
        sta (z80_ix),y
c00920: lda #3
        cmp scno
        beq *+5
        jmp c01152
        lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp c01015
        lda #112
        ldy #8
        sta (z80_ix),y
        tya; lda #8
        iny; ldy #9
        sta (z80_ix),y
c01015: lda #1
        cmp varrnd
        beq *+5
        jmp c01060
        lda #64
        ldy #8
        sta (z80_ix),y
        lda #56
        iny; ldy #9
        sta (z80_ix),y
c01060: lda #2
        cmp varrnd
        beq *+5
        jmp c01106
        lda #152
        ldy #8
        sta (z80_ix),y
        lda #56
        iny; ldy #9
        sta (z80_ix),y
c01106: lda #3
        cmp varrnd
        beq *+5
        jmp c01152
        lda #120
        ldy #8
        sta (z80_ix),y
        lda #184
        iny; ldy #9
        sta (z80_ix),y
c01152: lda #7
        cmp scno
        beq *+5
        jmp c01338
        lda #3
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp c01246
        lda #104
        ldy #8
        sta (z80_ix),y
        lda #24
        iny; ldy #9
        sta (z80_ix),y
c01246: lda #1
        cmp varrnd
        beq *+5
        jmp c01292
        lda #104
        ldy #8
        sta (z80_ix),y
        lda #120
        iny; ldy #9
        sta (z80_ix),y
c01292: lda #2
        cmp varrnd
        beq *+5
        jmp c01338
        lda #152
        ldy #8
        sta (z80_ix),y
        lda #184
        iny; ldy #9
        sta (z80_ix),y
c01338: lda #9
        cmp scno
        beq *+5
        jmp c01523
        lda #3
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp c01431
        lda #32
        ldy #8
        sta (z80_ix),y
        lda #112
        iny; ldy #9
        sta (z80_ix),y
c01431: lda #1
        cmp varrnd
        beq *+5
        jmp c01477
        lda #128
        ldy #8
        sta (z80_ix),y
        lda #56
        iny; ldy #9
        sta (z80_ix),y
c01477: lda #2
        cmp varrnd
        beq *+5
        jmp c01523
        lda #120
        ldy #8
        sta (z80_ix),y
        lda #152
        iny; ldy #9
        sta (z80_ix),y
c01523: lda #13
        cmp scno
        beq *+5
        jmp c01708
        lda #3
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp c01617
        lda #32
        ldy #8
        sta (z80_ix),y
        ; lda #32
        iny; ldy #9
        sta (z80_ix),y
c01617: lda #1
        cmp varrnd
        beq *+5
        jmp c01663
        lda #104
        ldy #8
        sta (z80_ix),y
        lda #80
        iny; ldy #9
        sta (z80_ix),y
c01663: lda #2
        cmp varrnd
        beq *+5
        jmp c01708
        lda #80
        ldy #8
        sta (z80_ix),y
        lda #176
        iny; ldy #9
        sta (z80_ix),y
c01708: lda #14
        cmp scno
        beq *+5
        jmp c01893
        lda #3
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp c01802
        lda #56
        ldy #8
        sta (z80_ix),y
        lda #88
        iny; ldy #9
        sta (z80_ix),y
c01802: lda #1
        cmp varrnd
        beq *+5
        jmp c01848
        lda #80
        ldy #8
        sta (z80_ix),y
        lda #144
        iny; ldy #9
        sta (z80_ix),y
c01848: lda #2
        cmp varrnd
        beq *+5
        jmp c01893
        lda #104
        ldy #8
        sta (z80_ix),y
        lda #40
        iny; ldy #9
        sta (z80_ix),y
c01893: lda #190
        asl a
        sta sndtyp
        rts
c01909: lda #6
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c01960
        lda #2
        ldy #7
        sta (z80_ix),y
        lda #170
        asl a
        sta sndtyp
        rts
c01960: lda #4
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c02011
        lda #1
        ldy #7
        sta (z80_ix),y
        lda #150
        asl a
        sta sndtyp
        rts
c02011: lda #2
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c02062
        lda #0
        ldy #7
        sta (z80_ix),y
        lda #130
        asl a
        sta sndtyp
        rts
c02062: lda #1
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c02142
        lda #0
        dey; ldy #11
        sta (z80_ix),y
        ; lda #0
        iny; ldy #12
        sta (z80_ix),y
        lda #2
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
c02142: lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c02265
:
        lda #0
        ldy #10
        sta (z80_ix),y
        lda #1
        iny; ldy #11
        sta (z80_ix),y
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #240
        sta varb
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #50
        ldy #12
        sta (z80_ix),y
c02265: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c02300
        lda #2
        ldy #6
        sta (z80_ix),y
c02300: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c02336
        ; lda #3
        ldy #6
        sta (z80_ix),y
c02336: lda #3
        cmp vara
        bne *+5
        jmp c02361
        rts
        jmp c03231
c02361: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp c03148
        ; lda #0
        dey; ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c02556
        jsr cangu
        beq :+
        jmp c02510
:
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
        jmp c02505
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #2
        ldy #10
        sta (z80_ix),y
c02505: jmp c02556
c02510: lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        ldy #10
        sta (z80_ix),y
c02556: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c02767
        jsr cangr
        beq :+
        jmp c02721
:
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
        jmp c02677
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #3
        iny; ldy #10
        sta (z80_ix),y
c02677: lda #14
        cmp scno
        beq *+5
        jmp c02716
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
c02716: jmp c02767
c02721: lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        ldy #10
        sta (z80_ix),y
c02767: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c02937
        jsr cangd
        beq :+
        jmp c02892
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp c02887
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #0
        ldy #10
        sta (z80_ix),y
c02887: jmp c02937
c02892: lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        ldy #10
        sta (z80_ix),y
c02937: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c03148
        jsr cangl
        beq :+
        jmp c03103
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp c03059
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #1
        iny; ldy #10
        sta (z80_ix),y
c03059: lda #14
        cmp scno
        beq *+5
        jmp c03098
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
c03098: jmp c03148
c03103: lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        ldy #10
        sta (z80_ix),y
c03148: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp c03231
        ; lda #0
        jsr animsp
        lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp c03219
        lda #20
        asl a
        sta sndtyp
        jmp c03231
c03219: lda #40
        asl a
        sta sndtyp
c03231: ldy #8
        lda (z80_ix),y
        cmp #8
        bcc *+5
        jmp c03266
        lda #2
        ldy #10
        sta (z80_ix),y
c03266: lda #152
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp c03302
        lda #0
        ldy #10
        sta (z80_ix),y
c03302: ldy #9
        lda (z80_ix),y
        cmp #8
        bcc *+5
        jmp c03339
        lda #1
        iny; ldy #10
        sta (z80_ix),y
c03339: lda #200
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp c03376
        lda #3
        iny; ldy #10
        sta (z80_ix),y
c03376: rts
.out .sprintf("EVENT 2 SIZE = %d", (* - evnt02))

evnt03:
        lda #1
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp d00029
        jmp d00029
d00029: lda #255
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp d00065
        lda #0
        ; ldy #11
        sta (z80_ix),y
d00065: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp d00157
        lda varp
        dey; ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp d00131
        tya; lda #9
        ldy #6
        sta (z80_ix),y
        jmp d00144
d00131: lda #8
        ldy #6
        sta (z80_ix),y
d00144: lda #0
        ldy #7
        sta (z80_ix),y
d00157: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp d00249
        lda varp
        dey; ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp d00222
        tya; lda #9
        ldy #6
        sta (z80_ix),y
        jmp d00236
d00222: lda #8
        ldy #6
        sta (z80_ix),y
d00236: lda #0
        ldy #7
        sta (z80_ix),y
d00249: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp d00299
        lda #8
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
d00299: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp d00349
        lda #9
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
d00349: lda #1
        cmp vara
        beq *+5
        jmp d00409
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp d00409
        ; lda #0
        jsr animsp
        lda #237
        asl a
        sta sndtyp
d00409: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00484
:
        lda #0
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp d00479
        ; lda #0
        cmp vard
        beq *+5
        jmp d00479
        lda #30
        sta vard
d00479: jmp d00484
d00484: lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00593
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #240
        sta varb
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #35
        ldy #11
        sta (z80_ix),y
        lda #1
        iny; ldy #12
        sta (z80_ix),y
d00593: lda #0
        ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp d00643
        ; ldy #11
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #11
        sta (z80_ix),y
d00643: lda #1
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp d00710
        lda #0
        dey; ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp d00696
        rts
        jmp d00710
d00696: lda #0
        ldy #12
        sta (z80_ix),y
d00710: lda #1
        cmp vara
        beq *+5
        jmp d00751
        lda #0
        cmp vars
        bcc *+5
        jmp d00746
        rts
d00746: jmp d00787
d00751: lda #3
        cmp vara
        beq *+5
        jmp d00787
        lda #0
        cmp vars
        bcc *+5
        jmp d00787
        rts
d00787: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp d01063
        jsr cangu
        beq :+
        jmp d00913
:
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
        jmp d00908
:
        lda #2
        ldy #10
        sta (z80_ix),y
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
d00908: jmp d00958
d00913: lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        ldy #10
        sta (z80_ix),y
d00958: ldy #9
        lda (z80_ix),y
        cmp varp
        bcc *+5
        jmp d01011
        jsr cangr
        beq :+
        jmp d01011
:
        lda #1
        ldy #10
        sta (z80_ix),y
        rts
d01011: lda varp
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp d01063
        jsr cangl
        beq :+
        jmp d01063
:
        lda #3
        ldy #10
        sta (z80_ix),y
        rts
d01063: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp d01340
        jsr cangr
        beq :+
        jmp d01190
:
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
        jmp d01185
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #3
        iny; ldy #10
        sta (z80_ix),y
d01185: jmp d01235
d01190: lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        ldy #10
        sta (z80_ix),y
d01235: lda varo
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp d01288
        jsr cangu
        beq :+
        jmp d01288
:
        lda #0
        ldy #10
        sta (z80_ix),y
        rts
d01288: ldy #8
        lda (z80_ix),y
        cmp varo
        bcc *+5
        jmp d01340
        jsr cangd
        beq :+
        jmp d01340
:
        lda #2
        ldy #10
        sta (z80_ix),y
        rts
d01340: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp d01616
        jsr cangd
        beq :+
        jmp d01465
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp d01461
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #0
        ldy #10
        sta (z80_ix),y
d01461: jmp d01511
d01465: lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        ldy #10
        sta (z80_ix),y
d01511: lda varp
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp d01563
        jsr cangl
        beq :+
        jmp d01563
:
        lda #3
        ldy #10
        sta (z80_ix),y
        rts
d01563: ldy #9
        lda (z80_ix),y
        cmp varp
        bcc *+5
        jmp d01616
        jsr cangr
        beq :+
        jmp d01616
:
        lda #1
        ldy #10
        sta (z80_ix),y
        rts
d01616: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp d01893
        jsr cangl
        beq :+
        jmp d01742
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp d01738
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #1
        iny; ldy #10
        sta (z80_ix),y
d01738: jmp d01788
d01742: lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        ldy #10
        sta (z80_ix),y
d01788: lda varo
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp d01840
        jsr cangu
        beq :+
        jmp d01840
:
        lda #0
        ldy #10
        sta (z80_ix),y
        rts
d01840: ldy #8
        lda (z80_ix),y
        cmp varo
        bcc *+5
        jmp d01893
        jsr cangd
        beq :+
        jmp d01893
:
        lda #2
        ldy #10
        sta (z80_ix),y
        rts
d01893: ldy #8
        lda (z80_ix),y
        cmp #8
        bcc *+5
        jmp d01928
        lda #2
        ldy #10
        sta (z80_ix),y
d01928: lda #152
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp d01964
        lda #0
        ldy #10
        sta (z80_ix),y
d01964: ldy #9
        lda (z80_ix),y
        cmp #8
        bcc *+5
        jmp d02001
        lda #1
        iny; ldy #10
        sta (z80_ix),y
d02001: lda #200
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp d02038
        lda #3
        iny; ldy #10
        sta (z80_ix),y
d02038: rts
.out .sprintf("EVENT 3 SIZE = %d", (* - evnt03))

evnt04:
        lda #255
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp e00038
        lda #0
        ; ldy #11
        sta (z80_ix),y
e00038: lda #255
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp e00075
        lda #0
        ; ldy #12
        sta (z80_ix),y
e00075: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00149
:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp e00145
        ; lda #0
        cmp vard
        beq *+5
        jmp e00145
        lda #30
        sta vard
e00145: jmp e00149
e00149: lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00258
:
        lda #1
        ldy #11
        sta (z80_ix),y
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #240
        sta varb
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #40
        ldy #12
        sta (z80_ix),y
e00258: lda #0
        ldy #12
        cmp (z80_ix),y
        bcc *+5
        jmp e00307
        ; ldy #12
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #12
        sta (z80_ix),y
e00307: lda #1
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp e02348
        lda #39
        iny; ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp e00393
        lda #7
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        lda #5
        asl a
        sta sndtyp
e00393: lda #36
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp e00441
        lda #1
        ldy #7
        sta (z80_ix),y
        lda #10
        asl a
        sta sndtyp
e00441: lda #33
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp e00489
        lda #2
        ldy #7
        sta (z80_ix),y
        lda #15
        asl a
        sta sndtyp
e00489: lda #30
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp e00537
        lda #3
        ldy #7
        sta (z80_ix),y
        lda #20
        asl a
        sta sndtyp
e00537: lda #27
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp e00573
        lda #200
        ldy #8
        sta (z80_ix),y
e00573: lda #8
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp e02113
        lda #3
        ldy #7
        sta (z80_ix),y
        ; lda #3
        cmp scno
        beq *+5
        jmp e00842
        lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp e00705
        lda #112
        iny; ldy #8
        sta (z80_ix),y
        tya; lda #8
        iny; ldy #9
        sta (z80_ix),y
e00705: lda #1
        cmp varrnd
        beq *+5
        jmp e00750
        lda #64
        ldy #8
        sta (z80_ix),y
        lda #56
        iny; ldy #9
        sta (z80_ix),y
e00750: lda #2
        cmp varrnd
        beq *+5
        jmp e00796
        lda #152
        ldy #8
        sta (z80_ix),y
        lda #56
        iny; ldy #9
        sta (z80_ix),y
e00796: lda #3
        cmp varrnd
        beq *+5
        jmp e00842
        lda #120
        ldy #8
        sta (z80_ix),y
        lda #184
        iny; ldy #9
        sta (z80_ix),y
e00842: lda #4
        cmp scno
        beq *+5
        jmp e00969
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
        jmp e00941
        lda #128
        ldy #8
        sta (z80_ix),y
        lda #176
        iny; ldy #9
        sta (z80_ix),y
        jmp e00969
e00941: lda #32
        ldy #8
        sta (z80_ix),y
        ; lda #32
        iny; ldy #9
        sta (z80_ix),y
e00969: lda #5
        cmp scno
        beq *+5
        jmp e01200
        lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp e01063
        lda #128
        ldy #8
        sta (z80_ix),y
        lda #72
        iny; ldy #9
        sta (z80_ix),y
e01063: lda #1
        cmp varrnd
        beq *+5
        jmp e01109
        lda #120
        ldy #8
        sta (z80_ix),y
        lda #176
        iny; ldy #9
        sta (z80_ix),y
e01109: lda #2
        cmp varrnd
        beq *+5
        jmp e01154
        lda #40
        ldy #8
        sta (z80_ix),y
        lda #32
        iny; ldy #9
        sta (z80_ix),y
e01154: lda #3
        cmp varrnd
        beq *+5
        jmp e01200
        lda #64
        ldy #8
        sta (z80_ix),y
        lda #176
        iny; ldy #9
        sta (z80_ix),y
e01200: lda #6
        cmp scno
        beq *+5
        jmp e01385
        lda #3
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp e01294
        lda #104
        ldy #8
        sta (z80_ix),y
        lda #80
        iny; ldy #9
        sta (z80_ix),y
e01294: lda #1
        cmp varrnd
        beq *+5
        jmp e01340
        lda #152
        ldy #8
        sta (z80_ix),y
        lda #176
        iny; ldy #9
        sta (z80_ix),y
e01340: lda #2
        cmp varrnd
        beq *+5
        jmp e01385
        lda #8
        ldy #8
        sta (z80_ix),y
        lda #128
        iny; ldy #9
        sta (z80_ix),y
e01385: lda #8
        cmp scno
        beq *+5
        jmp e01512
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
        jmp e01483
        lda #48
        ldy #8
        sta (z80_ix),y
        lda #176
        iny; ldy #9
        sta (z80_ix),y
        jmp e01512
e01483: lda #112
        ldy #8
        sta (z80_ix),y
        lda #32
        iny; ldy #9
        sta (z80_ix),y
e01512: lda #10
        cmp scno
        beq *+5
        jmp e01697
        lda #3
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp e01605
        lda #32
        ldy #8
        sta (z80_ix),y
        lda #56
        iny; ldy #9
        sta (z80_ix),y
e01605: lda #1
        cmp varrnd
        beq *+5
        jmp e01651
        lda #104
        ldy #8
        sta (z80_ix),y
        lda #200
        iny; ldy #9
        sta (z80_ix),y
e01651: lda #2
        cmp varrnd
        beq *+5
        jmp e01697
        lda #152
        ldy #8
        sta (z80_ix),y
        lda #24
        iny; ldy #9
        sta (z80_ix),y
e01697: lda #11
        cmp scno
        beq *+5
        jmp e01883
        lda #3
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp e01791
        lda #128
        ldy #8
        sta (z80_ix),y
        lda #176
        iny; ldy #9
        sta (z80_ix),y
e01791: lda #1
        cmp varrnd
        beq *+5
        jmp e01837
        lda #56
        ldy #8
        sta (z80_ix),y
        lda #112
        iny; ldy #9
        sta (z80_ix),y
e01837: lda #2
        cmp varrnd
        beq *+5
        jmp e01883
        lda #152
        ldy #8
        sta (z80_ix),y
        lda #32
        iny; ldy #9
        sta (z80_ix),y
e01883: lda #12
        cmp scno
        beq *+5
        jmp e02113
        lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp e01976
        lda #40
        ldy #8
        sta (z80_ix),y
        lda #96
        iny; ldy #9
        sta (z80_ix),y
e01976: lda #1
        cmp varrnd
        beq *+5
        jmp e02022
        lda #72
        ldy #8
        sta (z80_ix),y
        lda #40
        iny; ldy #9
        sta (z80_ix),y
e02022: lda #2
        cmp varrnd
        beq *+5
        jmp e02067
        lda #152
        ldy #8
        sta (z80_ix),y
        lda #88
        iny; ldy #9
        sta (z80_ix),y
e02067: lda #3
        cmp varrnd
        beq *+5
        jmp e02113
        lda #72
        ldy #8
        sta (z80_ix),y
        lda #152
        iny; ldy #9
        sta (z80_ix),y
e02113: lda #6
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp e02161
        lda #2
        ldy #7
        sta (z80_ix),y
        lda #20
        asl a
        sta sndtyp
e02161: lda #4
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp e02209
        lda #1
        ldy #7
        sta (z80_ix),y
        lda #15
        asl a
        sta sndtyp
e02209: lda #2
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp e02256
        lda #0
        ldy #7
        sta (z80_ix),y
        lda #10
        asl a
        sta sndtyp
e02256: lda #1
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp e02348
        lda #0
        dey; ldy #11
        sta (z80_ix),y
        ; lda #0
        iny; ldy #12
        sta (z80_ix),y
        lda #6
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        lda #5
        asl a
        sta sndtyp
e02348: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp e03855
        lda #3
        cmp vara
        bne *+5
        jmp e03855
        lda #1
        dey; ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e02424
        lda #5
        ldy #6
        sta (z80_ix),y
e02424: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e02460
        lda #6
        ldy #6
        sta (z80_ix),y
e02460: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e02678
        jsr cangu
        beq :+
        jmp e02585
:
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
        jmp e02580
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #2
        ldy #10
        sta (z80_ix),y
e02580: jmp e02678
e02585: lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp e02665
        jsr cangr
        beq :+
        jmp e02660
:
        lda #1
        ldy #10
        sta (z80_ix),y
e02660: jmp e02678
e02665: lda #3
        ldy #10
        sta (z80_ix),y
e02678: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e02808
        lda #3
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp e02808
        jsr cangr
        beq :+
        jmp e02781
:
        lda #1
        ldy #10
        sta (z80_ix),y
        jmp e02808
e02781: jsr cangl
        beq :+
        jmp e02808
:
        lda #3
        ldy #10
        sta (z80_ix),y
e02808: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e03028
        jsr cangr
        beq :+
        jmp e02934
:
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
        jmp e02929
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #3
        iny; ldy #10
        sta (z80_ix),y
e02929: jmp e03028
e02934: lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp e03014
        jsr cangd
        beq :+
        jmp e03009
:
        lda #2
        ldy #10
        sta (z80_ix),y
e03009: jmp e03028
e03014: lda #0
        ldy #10
        sta (z80_ix),y
e03028: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e03157
        lda #3
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp e03157
        jsr cangu
        beq :+
        jmp e03130
:
        lda #0
        ldy #10
        sta (z80_ix),y
        jmp e03157
e03130: jsr cangd
        beq :+
        jmp e03157
:
        lda #2
        ldy #10
        sta (z80_ix),y
e03157: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e03376
        jsr cangd
        beq :+
        jmp e03282
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp e03277
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #0
        ldy #10
        sta (z80_ix),y
e03277: jmp e03376
e03282: lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp e03362
        jsr cangl
        beq :+
        jmp e03357
:
        lda #3
        ldy #10
        sta (z80_ix),y
e03357: jmp e03376
e03362: lda #1
        ldy #10
        sta (z80_ix),y
e03376: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e03505
        lda #3
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp e03505
        jsr cangr
        beq :+
        jmp e03478
:
        lda #1
        ldy #10
        sta (z80_ix),y
        jmp e03505
e03478: jsr cangl
        beq :+
        jmp e03505
:
        lda #3
        ldy #10
        sta (z80_ix),y
e03505: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e03725
        jsr cangl
        beq :+
        jmp e03631
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp e03627
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #1
        iny; ldy #10
        sta (z80_ix),y
e03627: jmp e03725
e03631: lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp e03711
        jsr cangu
        beq :+
        jmp e03707
:
        lda #0
        ldy #10
        sta (z80_ix),y
e03707: jmp e03725
e03711: lda #2
        ldy #10
        sta (z80_ix),y
e03725: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e03855
        ; lda #3
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp e03855
        jsr cangu
        beq :+
        jmp e03828
:
        lda #0
        ldy #10
        sta (z80_ix),y
        jmp e03855
e03828: jsr cangd
        beq :+
        jmp e03855
:
        lda #2
        ldy #10
        sta (z80_ix),y
e03855: lda #2
        cmp vara
        beq *+5
        jmp e03954
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp e03954
        ; lda #0
        jsr animsp
        lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp e03942
        lda #127
        asl a
        sta sndtyp
        jmp e03954
e03942: lda #133
        asl a
        sta sndtyp
e03954: ldy #8
        lda (z80_ix),y
        cmp #8
        bcc *+5
        jmp e03990
        lda #2
        ldy #10
        sta (z80_ix),y
e03990: lda #152
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp e04026
        lda #0
        ldy #10
        sta (z80_ix),y
e04026: ldy #9
        lda (z80_ix),y
        cmp #8
        bcc *+5
        jmp e04063
        lda #1
        iny; ldy #10
        sta (z80_ix),y
e04063: lda #200
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp e04100
        lda #3
        iny; ldy #10
        sta (z80_ix),y
e04100: rts
.out .sprintf("EVENT 4 SIZE = %d", (* - evnt04))

evnt05:
        lda #0
        ldy #12
        cmp (z80_ix),y
        bcc *+5
        jmp f00029
        jmp f00029
f00029: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00080
:
        lda #0
        cmp vard
        beq *+5
        jmp f00075
        lda #30
        sta vard
f00075: jmp f00080
f00080: lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00357
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #240
        sta varb
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #10
        sta varc
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00210
        lda #2
        ; ldy #10
        sta (z80_ix),y
        jmp f00329
f00210: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00251
        lda #3
        ; ldy #10
        sta (z80_ix),y
        jmp f00329
f00251: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00292
        lda #0
        ; ldy #10
        sta (z80_ix),y
        jmp f00329
f00292: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00329
        lda #1
        ; ldy #10
        sta (z80_ix),y
f00329: lda #0
        ldy #11
        sta (z80_ix),y
        ; lda #0
        iny; ldy #12
        sta (z80_ix),y
f00357: lda varo
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp f00394
        lda #254
        ldy #12
        sta (z80_ix),y
f00394: lda varp
        ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp f00430
        lda #1
        ldy #12
        sta (z80_ix),y
f00430: lda #0
        cmp varc
        bcc *+5
        jmp f00450
        rts
f00450: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00487
        tya; lda #10
        ldy #6
        sta (z80_ix),y
f00487: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00522
        lda #11
        ldy #6
        sta (z80_ix),y
f00522: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00843
        jsr cangu
        beq :+
        jmp f00721
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #1
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp f00716
        lda varo
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp f00688
        jsr cangu
        beq :+
        jmp f00674
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #255
        ldy #11
        sta (z80_ix),y
f00674: lda #0
        jsr animsp
        jmp f00716
f00688: lda #0
        ldy #11
        sta (z80_ix),y
        ; lda #0
        iny; ldy #12
        sta (z80_ix),y
f00716: jmp f00843
f00721: lda #0
        ldy #11
        sta (z80_ix),y
        ; lda #0
        iny; ldy #12
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
        jmp f00830
        jsr cangr
        beq :+
        jmp f00825
:
        lda #1
        ldy #10
        sta (z80_ix),y
f00825: jmp f00843
f00830: lda #3
        ldy #10
        sta (z80_ix),y
f00843: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00997
        ; lda #0
        iny; ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp f00997
        lda #3
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp f00997
        jsr cangr
        beq :+
        jmp f00970
:
        lda #1
        ldy #10
        sta (z80_ix),y
        jmp f00997
f00970: jsr cangl
        beq :+
        jmp f00997
:
        lda #3
        ldy #10
        sta (z80_ix),y
f00997: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f01318
        jsr cangr
        beq :+
        jmp f01196
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #254
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp f01191
        ldy #9
        lda (z80_ix),y
        cmp varp
        bcc *+5
        jmp f01162
        jsr cangr
        beq :+
        jmp f01149
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #255
        ldy #11
        sta (z80_ix),y
f01149: lda #0
        jsr animsp
        jmp f01191
f01162: lda #0
        ldy #11
        sta (z80_ix),y
        ; lda #0
        iny; ldy #12
        sta (z80_ix),y
f01191: jmp f01318
f01196: lda #0
        ldy #11
        sta (z80_ix),y
        ; lda #0
        iny; ldy #12
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
        jmp f01305
        jsr cangd
        beq :+
        jmp f01300
:
        lda #2
        ldy #10
        sta (z80_ix),y
f01300: jmp f01318
f01305: lda #0
        ldy #10
        sta (z80_ix),y
f01318: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f01471
        lda #0
        iny; ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp f01471
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
        jmp f01471
        jsr cangu
        beq :+
        jmp f01444
:
        lda #0
        ldy #10
        sta (z80_ix),y
        jmp f01471
f01444: jsr cangd
        beq :+
        jmp f01471
:
        lda #2
        ldy #10
        sta (z80_ix),y
f01471: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f01792
        jsr cangd
        beq :+
        jmp f01670
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #1
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp f01665
        ldy #8
        lda (z80_ix),y
        cmp varo
        bcc *+5
        jmp f01665
        jsr cangd
        beq :+
        jmp f01636
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #255
        ldy #11
        sta (z80_ix),y
        lda #0
        jsr animsp
        jmp f01665
f01636: lda #0
        ldy #11
        sta (z80_ix),y
        ; lda #0
        iny; ldy #12
        sta (z80_ix),y
f01665: jmp f01792
f01670: lda #0
        ldy #11
        sta (z80_ix),y
        ; lda #0
        iny; ldy #12
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
        jmp f01778
        jsr cangr
        beq :+
        jmp f01774
:
        lda #3
        ldy #10
        sta (z80_ix),y
f01774: jmp f01792
f01778: lda #1
        ldy #10
        sta (z80_ix),y
f01792: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f01945
        lda #0
        iny; ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp f01945
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
        jmp f01945
        jsr cangr
        beq :+
        jmp f01918
:
        lda #1
        ldy #10
        sta (z80_ix),y
        jmp f01945
f01918: jsr cangl
        beq :+
        jmp f01945
:
        lda #3
        ldy #10
        sta (z80_ix),y
f01945: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f02266
        jsr cangl
        beq :+
        jmp f02144
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #254
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp f02139
        lda varp
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp f02110
        jsr cangl
        beq :+
        jmp f02106
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #255
        ldy #11
        sta (z80_ix),y
        lda #0
        jsr animsp
f02106: jmp f02139
f02110: lda #0
        ldy #11
        sta (z80_ix),y
        ; lda #0
        iny; ldy #12
        sta (z80_ix),y
f02139: jmp f02266
f02144: lda #0
        ldy #11
        sta (z80_ix),y
        ; lda #0
        iny; ldy #12
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
        jmp f02253
        jsr cangu
        beq :+
        jmp f02248
:
        lda #0
        ldy #10
        sta (z80_ix),y
f02248: jmp f02266
f02253: lda #2
        ldy #10
        sta (z80_ix),y
f02266: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f02419
        lda #0
        iny; ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp f02419
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
        jmp f02419
        jsr cangu
        beq :+
        jmp f02392
:
        lda #0
        ldy #10
        sta (z80_ix),y
        jmp f02419
f02392: jsr cangd
        beq :+
        jmp f02419
:
        lda #2
        ldy #10
        sta (z80_ix),y
f02419: lda #0
        ldy #12
        cmp (z80_ix),y
        bne *+5
        jmp f02480
        ; lda #0
        dey; ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp f02480
        ; lda #0
        iny; ldy #12
        sta (z80_ix),y
f02480: ldy #8
        lda (z80_ix),y
        cmp #8
        bcc *+5
        jmp f02516
        lda #2
        ldy #10
        sta (z80_ix),y
f02516: lda #152
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp f02552
        lda #0
        ldy #10
        sta (z80_ix),y
f02552: ldy #9
        lda (z80_ix),y
        cmp #8
        bcc *+5
        jmp f02589
        lda #1
        iny; ldy #10
        sta (z80_ix),y
f02589: lda #200
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp f02626
        lda #3
        iny; ldy #10
        sta (z80_ix),y
f02626: lda #0
        cmp vara
        beq *+5
        jmp f02652
        ; lda #0
        jsr animsp
f02652: rts
.out .sprintf("EVENT 5 SIZE = %d", (* - evnt05))

evnt06:
        rts
.out .sprintf("EVENT 6 SIZE = %d", (* - evnt06))

evnt07:
        lda #10
        cmp varg
        beq *+5
        jmp h00032
        lda #255
        ldy #5
        sta (z80_ix),y
h00032: lda #255
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp h00069
        lda #0
        ; ldy #11
        sta (z80_ix),y
h00069: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp h00121
:
        lda #0
        cmp vard
        beq *+5
        jmp h00116
        lda #30
        sta vard
h00116: jmp h00121
h00121: lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp h00216
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #240
        sta varb
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #255
        ; ldy #5
        sta (z80_ix),y
h00216: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp h00300
        lda #3
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        dey; ldy #10
        sta (z80_ix),y
        lda #1
        iny; ldy #11
        sta (z80_ix),y
h00300: lda #2
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #152
        ; ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp h00372
        lda #255
        ldy #5
        sta (z80_ix),y
h00372: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp h00467
        lda #2
        sta z80_c
        dey; ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        cmp #8
        bcc *+5
        jmp h00467
        lda #255
        ldy #5
        sta (z80_ix),y
h00467: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp h00563
        ; lda #2
        sta z80_c
        dey; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #200
        ; ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp h00563
        lda #255
        ldy #5
        sta (z80_ix),y
h00563: lda #3
        cmp vara
        beq *+5
        jmp h00588
        lda #0
        jsr animsp
h00588: ldy #8
        lda (z80_ix),y
        ldy #11
        sta (z80_ix),y
        lda #100
        sta z80_c
        iny; ldy #12
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #12
        sta (z80_ix),y
        ; ldy #12
        lda (z80_ix),y
        asl a
        sta sndtyp
        rts
.out .sprintf("EVENT 7 SIZE = %d", (* - evnt07))

evnt08:
        lda #0
        cmp scno
        beq *+5
        jmp i00120
        lda #4
        cmp varg
        beq *+5
        jmp i00117
        lda #8
        ldy #8
        sta (z80_ix),y
        lda #104
        iny; ldy #9
        sta (z80_ix),y
        lda #0
        sta z80_c
        ; lda #0
        sta z80_b
        jsr spawn
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
i00117: rts
i00120: lda #15
        cmp scno
        beq *+5
        jmp i00184
        lda #8
        cmp varg
        beq *+5
        jmp i00184
        lda #120
        sta varn
        lda #7
        ldy #11
        sta (z80_ix),y
        lda #9
        sta varg
i00184: lda #9
        cmp varg
        beq *+5
        jmp i01286
        lda #17
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i00235
        lda #190
        asl a
        sta sndtyp
i00235: lda varn
        sec
        sbc #1
        sta varn
        lda #1
        cmp varn
        beq *+5
        jmp i00693
        ldy #9
        lda (z80_ix),y
        ldy #12
        sta (z80_ix),y
        lda #7
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp i00348
        lda #8
        ldy #9
        sta (z80_ix),y
i00348: lda #1
        cmp varrnd
        beq *+5
        jmp i00379
        lda #40
        ldy #9
        sta (z80_ix),y
i00379: lda #2
        cmp varrnd
        beq *+5
        jmp i00410
        lda #72
        ldy #9
        sta (z80_ix),y
i00410: lda #3
        cmp varrnd
        beq *+5
        jmp i00441
        lda #104
        ldy #9
        sta (z80_ix),y
i00441: lda #4
        cmp varrnd
        beq *+5
        jmp i00472
        lda #136
        ldy #9
        sta (z80_ix),y
i00472: lda #5
        cmp varrnd
        beq *+5
        jmp i00503
        lda #168
        ldy #9
        sta (z80_ix),y
i00503: lda #6
        cmp varrnd
        beq *+5
        jmp i00534
        lda #200
        ldy #9
        sta (z80_ix),y
i00534: ldy #12
        lda (z80_ix),y
        ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp i00684
        lda #104
        ; ldy #9
        cmp (z80_ix),y
        bne *+5
        jmp i00604
        ; lda #104
        ; ldy #9
        sta (z80_ix),y
        jmp i00684
i00604: lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp i00670
        lda #8
        ldy #9
        sta (z80_ix),y
        jmp i00684
i00670: lda #200
        ldy #9
        sta (z80_ix),y
i00684: lda #120
        sta varn
i00693: lda #110
        cmp varn
        beq *+5
        jmp i00826
        lda #24
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #7
        sta z80_c
        lda #15
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #0
        jsr animsp
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #64
        ldy #8
        sta (z80_ix),y
i00826: lda #104
        cmp varn
        beq *+5
        jmp i00923
        lda #7
        sta z80_c
        lda #15
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #0
        jsr animsp
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #64
        ldy #8
        sta (z80_ix),y
i00923: lda #55
        cmp varn
        beq *+5
        jmp i01056
        lda #24
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #7
        sta z80_c
        lda #15
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #0
        jsr animsp
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #64
        ldy #8
        sta (z80_ix),y
i01056: lda #52
        cmp varn
        beq *+5
        jmp i01189
        lda #24
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #7
        sta z80_c
        lda #15
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #0
        jsr animsp
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #64
        ldy #8
        sta (z80_ix),y
i01189: lda #49
        cmp varn
        beq *+5
        jmp i01286
        lda #7
        sta z80_c
        lda #15
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #0
        jsr animsp
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #64
        ldy #8
        sta (z80_ix),y
i01286: lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i01416
:
        lda #17
        ldy #6
        sta (z80_ix),y
        ldy #11
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #11
        sta (z80_ix),y
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #240
        sta varb
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        tya; lda #5
        sta varn
i01416: lda #2
        cmp varn
        beq *+5
        jmp i01447
        lda #16
        ldy #6
        sta (z80_ix),y
i01447: lda #10
        cmp varg
        beq *+5
        jmp i01548
        lda #11
        sta varg
        lda #15
        sta chary
        lda #5
        sta charx
        lda #4
        jsr dmsg
        lda #150
        jsr delay
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #15
        sta chary
        tya; lda #5
        sta charx
        lda #10
        jsr dmsg
        rts
i01548: lda #15
        cmp scno
        beq *+5
        jmp i01596
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp i01596
        lda #10
        sta varg
i01596: rts
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
        bcc *+5
        jmp k00042
        lda #0
        sta vara
k00042: lda #255
        cmp varb
        beq *+5
        jmp k00067
        lda #0
        sta varb
k00067: lda #200
        cmp varb
        bcc *+5
        jmp k00099
        lda varb
        clc
        adc #1
        sta varb
k00099: lda #0
        cmp varc
        bcc *+5
        jmp k00131
        lda varc
        sec
        sbc #1
        sta varc
k00131: lda #0
        cmp vars
        bcc *+5
        jmp k00163
        lda vars
        sec
        sbc #1
        sta vars
k00163: lda #0
        cmp varg
        beq *+5
        jmp k00213
        lda #12
        sta chary
        lda #5
        sta charx
        lda #9
        jsr dmsg
        lda #1
        sta varg
k00213: lda #1
        cmp varg
        beq *+5
        jmp k00311
        lda #17
        sta chary
        lda #4
        sta charx
        lda #7
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #65
        sta z80_c
        lda varrnd
        clc
        adc z80_c
        sta varrnd
        lda #11
        jsr dmsg
k00311: lda #2
        cmp varg
        beq *+5
        jmp k00653
        lda #17
        sta chary
        lda #3
        sta charx
        lda #10
        jsr dmsg
        lda #12
        sta chary
        lda #3
        sta charx
        lda #10
        jsr dmsg
        lda #112
        ldy #8
        sta (z80_ix),y
        lda #104
        iny; ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        lda #16
        sta z80_b
        jsr spawn
        lda #50
        jsr delay
        lda #11
        sta chary
        lda #3
        sta charx
        lda #0
        jsr dmsg
        lda #12
        sta chary
        lda #5
        sta charx
        lda #1
        jsr dmsg
        lda #100
        jsr delay
        lda #11
        sta chary
        lda #3
        sta charx
        lda #2
        jsr dmsg
        lda #12
        sta chary
        lda #5
        sta charx
        lda #3
        jsr dmsg
        lda #100
        jsr delay
        lda #11
        sta chary
        lda #3
        sta charx
        lda #10
        jsr dmsg
        lda #12
        sta chary
        lda #3
        sta charx
        lda #10
        jsr dmsg
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #8
        ldy #8
        sta (z80_ix),y
        lda #104
        iny; ldy #9
        sta (z80_ix),y
        lda #3
        sta varg
k00653: lda #3
        cmp varg
        beq *+5
        jmp k00678
        lda #4
        sta varg
k00678: lda #7
        cmp scno
        beq *+5
        jmp k00891
        lda #5
        cmp varg
        beq *+5
        jmp k00771
        lda #20
        jsr getob
        lda #1
        sta chary
        lda #20
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #11
        jsr pattr
        lda #6
        sta varg
k00771: lda #7
        cmp varg
        beq *+5
        jmp k00891
        lda #21
        jsr getob
        lda #4
        sta chary
        lda #13
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #12
        jsr pattr
        lda #5
        sta chary
        lda #13
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #12
        jsr pattr
        lda #8
        sta varg
k00891: lda #11
        cmp varg
        beq *+5
        jmp k00981
        lda #18
        sta chary
        lda #1
        sta charx
        lda #26
        sta loopa
k00933: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        dec loopa
        beq :+
        jmp k00933
:
        lda #12
        sta varg
k00981: lda #14
        cmp varg
        beq *+5
        jmp k01006
        lda #1
        sta gamwon
k01006: rts
.out .sprintf("EVENT 10 SIZE = %d", (* - evnt10))

evnt11:
        lda #30
        cmp vard
        beq *+5
        jmp l00369
        lda varh
        sec
        sbc #1
        sta varh
        lda #5
        cmp varh
        beq *+5
        jmp l00101
        lda #1
        sta chary
        lda #29
        sta charx
        lda #15
        jsr dmsg
        lda #2
        sta chary
        lda #29
        sta charx
        lda #16
        jsr dmsg
l00101: lda #4
        cmp varh
        beq *+5
        jmp l00168
        ; lda #4
        sta chary
        lda #29
        sta charx
        lda #15
        jsr dmsg
        lda #5
        sta chary
        lda #29
        sta charx
        lda #16
        jsr dmsg
l00168: lda #3
        cmp varh
        beq *+5
        jmp l00235
        lda #7
        sta chary
        lda #29
        sta charx
        lda #15
        jsr dmsg
        lda #8
        sta chary
        lda #29
        sta charx
        lda #16
        jsr dmsg
l00235: lda #2
        cmp varh
        beq *+5
        jmp l00302
        lda #10
        sta chary
        lda #29
        sta charx
        lda #15
        jsr dmsg
        lda #11
        sta chary
        lda #29
        sta charx
        lda #16
        jsr dmsg
l00302: lda #1
        cmp varh
        beq *+5
        jmp l00369
        lda #13
        sta chary
        lda #29
        sta charx
        lda #15
        jsr dmsg
        lda #14
        sta chary
        lda #29
        sta charx
        lda #16
        jsr dmsg
l00369: lda #0
        cmp vard
        bcc *+5
        jmp l00401
        lda vard
        sec
        sbc #1
        sta vard
l00401: lda #0
        cmp varh
        beq *+5
        jmp l00426
        lda #1
        sta deadf
l00426: rts
.out .sprintf("EVENT 11 SIZE = %d", (* - evnt11))

evnt12:
        rts
.out .sprintf("EVENT 12 SIZE = %d", (* - evnt12))

evnt13:
        lda #1
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
        sta varg
        lda #6
        sta varh
        lda #0
        sta varl
        ; lda #0
        sta varn
        rts
.out .sprintf("EVENT 13 SIZE = %d", (* - evnt13))

evnt14:
        lda #0
        sta vara
        ; lda #0
        sta varb
        ; lda #0
        sta varc
        ; lda #0
        sta vard
        ; lda #0
        sta varl
        ldy #8
        lda (z80_ix),y
        sta varo
        iny; ldy #9
        lda (z80_ix),y
        sta varp
        lda #255
        sta vars
        lda #17
        cmp scno
        bne *+5
        jmp o01138
        lda #0
        sta chary
        lda #29
        sta charx
        lda #19
        jsr dmsg
        lda #0
        sta chary
        lda #30
        sta charx
        lda #20
        jsr dmsg
        lda #1
        sta chary
        lda #29
        sta charx
        lda #6
        cmp varh
        beq *+5
        jmp o00219
        lda #13
        jsr dmsg
        lda #2
        sta chary
        lda #29
        sta charx
        lda #14
        jsr dmsg
        jmp o00253
o00219: lda #15
        jsr dmsg
        lda #2
        sta chary
        lda #29
        sta charx
        lda #16
        jsr dmsg
o00253: lda #3
        sta chary
        lda #29
        sta charx
        lda #17
        jsr dmsg
        lda #3
        sta chary
        lda #30
        sta charx
        lda #18
        jsr dmsg
        lda #4
        sta chary
        lda #29
        sta charx
        lda #4
        cmp varh
        bcc *+5
        jmp o00374
        lda #13
        jsr dmsg
        lda #5
        sta chary
        lda #29
        sta charx
        lda #14
        jsr dmsg
        jmp o00407
o00374: lda #15
        jsr dmsg
        lda #5
        sta chary
        lda #29
        sta charx
        lda #16
        jsr dmsg
o00407: lda #6
        sta chary
        lda #29
        sta charx
        lda #17
        jsr dmsg
        lda #6
        sta chary
        lda #30
        sta charx
        lda #18
        jsr dmsg
        lda #7
        sta chary
        lda #29
        sta charx
        lda #3
        cmp varh
        bcc *+5
        jmp o00529
        lda #13
        jsr dmsg
        lda #8
        sta chary
        lda #29
        sta charx
        lda #14
        jsr dmsg
        jmp o00562
o00529: lda #15
        jsr dmsg
        lda #8
        sta chary
        lda #29
        sta charx
        lda #16
        jsr dmsg
o00562: lda #9
        sta chary
        lda #29
        sta charx
        lda #17
        jsr dmsg
        lda #9
        sta chary
        lda #30
        sta charx
        lda #18
        jsr dmsg
        lda #10
        sta chary
        lda #29
        sta charx
        lda #2
        cmp varh
        bcc *+5
        jmp o00684
        lda #13
        jsr dmsg
        lda #11
        sta chary
        lda #29
        sta charx
        lda #14
        jsr dmsg
        jmp o00718
o00684: lda #15
        jsr dmsg
        lda #11
        sta chary
        lda #29
        sta charx
        lda #16
        jsr dmsg
o00718: lda #12
        sta chary
        lda #29
        sta charx
        lda #17
        jsr dmsg
        lda #12
        sta chary
        lda #30
        sta charx
        lda #18
        jsr dmsg
        lda #13
        sta chary
        lda #29
        sta charx
        lda #1
        cmp varh
        bcc *+5
        jmp o00840
        lda #13
        jsr dmsg
        lda #14
        sta chary
        lda #29
        sta charx
        lda #14
        jsr dmsg
        jmp o00874
o00840: lda #15
        jsr dmsg
        lda #14
        sta chary
        lda #29
        sta charx
        lda #16
        jsr dmsg
o00874: lda #15
        sta chary
        lda #29
        sta charx
        lda #17
        jsr dmsg
        lda #15
        sta chary
        lda #30
        sta charx
        lda #18
        jsr dmsg
        lda #16
        sta chary
        lda #29
        sta charx
        lda #0
        cmp varh
        bcc *+5
        jmp o00991
        lda #13
        jsr dmsg
        lda #17
        sta chary
        lda #29
        sta charx
        lda #14
        jsr dmsg
o00991: lda #18
        sta chary
        lda #29
        sta charx
        lda #17
        jsr dmsg
        lda #18
        sta chary
        lda #30
        sta charx
        lda #18
        jsr dmsg
        lda #19
        sta varl
        lda #3
        sta loopa
o01058: lda varl
        sta chary
        lda #29
        sta charx
        lda #21
        jsr dmsg
        lda varl
        sta chary
        lda #30
        sta charx
        lda #22
        jsr dmsg
        lda varl
        clc
        adc #1
        sta varl
        dec loopa
        beq :+
        jmp o01058
:
o01138: rts
.out .sprintf("EVENT 14 SIZE = %d", (* - evnt14))

evnt15:
        rts
.out .sprintf("EVENT 15 SIZE = %d", (* - evnt15))

evnt16:
        lda numlif
        sec
        sbc #1
        sta numlif
        rts
.out .sprintf("EVENT 16 SIZE = %d", (* - evnt16))

evnt17:
        lda #0
        cmp numlif
        beq *+5
        jmp r00264
        lda #17
        sta scno
        jsr nwscr
        jsr cls
        lda #1
        sta restfl
        lda #0
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp r00080
        lda #255
        ; ldy #5
        sta (z80_ix),y
r00080: lda #8
        sta chary
        lda #3
        sta charx
        lda #5
        jsr dmsg
        lda #10
        sta chary
        lda #5
        sta charx
        lda #6
        jsr dmsg
        lda #12
        sta chary
        lda #5
        sta charx
        lda #7
        jsr dmsg
        lda #14
        sta chary
        lda #4
        sta charx
        lda #8
        jsr dmsg
        lda #120
        sta varl
        lda #20
        sta loopa
r00196: lda varl
        asl a
        sta sndtyp
        lda #5
        sta z80_c
        lda varl
        sec
        sbc z80_c
        sta varl
        dec loopa
        beq :+
        jmp r00196
:
        lda #200
        jsr delay
        lda #1
        sta gamwon
r00264: rts
.out .sprintf("EVENT 17 SIZE = %d", (* - evnt17))

evnt18:
        lda #5
        jsr delay
        jsr prskey
        lda #5
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
        .byte 39
        .byte "I HAVE KIDNAPPED YOUR",141
        .byte "LOVE, SORCERESS...",39
        .byte 141
        .byte 39
        .byte "I WILL TAKE HER LIFE*",141
        .byte "- AND THEN YOURS!",39
        .byte "*",141
        .byte 39
        .byte "NO! NO! NOOOOO!",39
        .byte 141
        .byte "THE SORCERESS IS DEFEATED.",141
        .byte "SHE AND HER LOVE WILL",141
        .byte "SPEND ETERNITY AS THE",141
        .byte "NECROMANCER",39
        .byte "S SLAVES...",141
        .byte "S O R C E R E S S",141
        .byte "**********************",141
        .byte "PRESS SPACE TO START",141
        .byte "LOVE CONQUERS HATE...",141
        .byte "ab",141
        .byte "cd",141
        .byte "ef",141
        .byte "gh",141
        .byte "i",141
        .byte "j",141
        .byte "k",141
        .byte "l",141
        .byte "m",141
        .byte "n",141
nummsg:
        .byte 23
.out .sprintf("MESSAGE SIZE = %d", (* - msgdat))

chgfx:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,48,128,112,192,112,224,240,224,240,240,240,240,112,240
        .byte 0,0,48,128,112,192,240,192,240,224,240,224,240,224,240,192
        .byte 112,240,48,240,48,240,16,240,0,240,0,48,0,16,0,0
        .byte 240,192,240,128,240,128,240,0,224,0,128,0,0,0,0,0
        .byte 0,0,51,136,102,204,68,102,204,34,136,51,204,17,68,34
        .byte 0,0,51,136,102,204,204,68,136,102,136,34,0,102,0,68
        .byte 102,34,34,17,51,0,17,153,0,238,0,51,0,17,0,0
        .byte 0,204,0,136,153,136,51,0,238,0,136,0,0,0,0,0
        .byte 32,245,34,250,32,245,114,250,117,245,0,0,32,245,34,250
        .byte 228,128,216,136,228,128,216,200,228,196,0,0,228,128,216,136
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 208,192,176,32,128,224,80,160,128,224,80,192,176,160,0,0
        .byte 80,96,176,16,80,80,224,96,112,80,176,32,96,112,0,0
        .byte 117,245,200,16,145,196,170,170,177,100,152,200,128,17,250,250
        .byte 245,228,136,50,49,145,80,80,100,213,50,152,128,17,250,250
        .byte 245,245,136,16,145,196,170,170,177,100,152,200,196,17,114,250
        .byte 245,245,136,16,49,145,80,80,100,213,50,152,128,17,250,234
        .byte 90,90,165,165,0,0,37,165,0,0,1,165,16,80,1,5
        .byte 90,74,165,164,0,0,165,132,0,0,165,0,80,0,5,0
        .byte 16,80,1,5,16,80,1,5,16,80,1,5,16,80,1,5
        .byte 80,0,5,0,80,0,5,0,80,0,5,0,80,0,5,0
        .byte 16,90,0,0,82,90,164,33,88,80,164,161,74,18,165,165
        .byte 90,0,0,0,90,72,128,164,80,66,160,164,8,74,165,164
        .byte 10,10,170,170,95,95,34,170,0,8,0,136,17,76,0,136
        .byte 10,10,170,170,95,78,170,136,2,0,34,0,87,0,34,0
        .byte 88,2,165,37,82,82,0,0,88,74,37,164,74,10,0,0
        .byte 0,0,7,9,15,12,5,10,0,0,1,3,12,7,8,2
        .byte 0,0,4,15,9,15,4,13,4,0,1,12,15,14,7,4
        .byte 95,95,175,175,15,15,15,15,15,15,175,175,15,15,175,175
        .byte 79,95,143,15,79,95,143,15,79,95,143,15,79,95,143,15
        .byte 95,95,175,143,95,31,175,15,79,31,143,15,15,31,175,175
        .byte 51,0,119,136,51,255,17,255,17,221,51,170,119,136,51,0
        .byte 0,0,0,0,187,255,255,187,85,85,170,170,0,0,0,0
        .byte 0,204,17,238,255,204,255,136,85,136,170,204,17,238,0,204
        .byte 0,0,68,34,238,119,255,255,119,102,51,136,51,68,51,136
        .byte 51,68,17,136,51,68,51,136,51,68,34,136,51,68,51,136
        .byte 51,68,51,136,51,68,119,170,255,119,238,119,68,34,0,0
        .byte 17,136,187,204,17,68,34,221,17,0,68,17,51,238,68,17
        .byte 0,0,17,68,238,51,17,68,0,0,68,17,51,238,68,17
        .byte 0,0,0,68,0,51,0,68,0,0,68,17,51,238,68,17
        .byte 0,0,17,0,238,0,17,0,0,0,68,17,51,238,68,17
        .byte 0,0,51,136,119,204,187,170,153,34,102,204,119,204,85,68
        .byte 0,0,51,204,119,238,102,68,119,170,51,204,17,68,0,0
        .byte 0,0,51,136,119,204,85,68,34,136,51,136,0,0,0,0
        .byte 176,208,176,240,160,240,0,0,208,176,240,48,240,176,0,0
        .byte 0,16,0,48,0,112,0,0,16,176,48,48,112,176,0,0
        .byte 128,0,192,0,224,0,0,0,176,128,240,0,240,160,0,0
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 90,90,0,0,90,90,128,1,26,88,161,133,8,16,165,165
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,16,0,117,0,250,17,177,16,114,49,117,34,250
        .byte 0,0,136,0,228,0,250,0,245,128,216,136,228,196,216,200
        .byte 32,245,34,250,32,245,34,250,32,245,34,250,32,245,34,250
        .byte 228,128,216,136,228,128,216,136,228,128,216,136,228,128,216,136
        .byte 176,176,80,112,16,80,80,176,224,112,224,176,224,176,32,64
        .byte 165,165,90,75,165,165,15,15,165,165,75,90,165,165,15,15
        .byte 240,240,15,90,45,180,90,90,240,240,90,15,180,45,90,90
bprop:
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
        .byte 5
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
        .byte 6
        .byte 2
        .byte 5
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 2
.out .sprintf("BLOCKS SIZE = %d", (* - chgfx))

sprgfx:
        .byte 2,128,5,64,11,160,5,64,11,160,49,16,76,72,83,164,82,148,73,18,131,130,134,66,148,81,228,137,2,135,6,0
        .byte 2,128,5,64,11,160,5,64,11,160,17,24,36,100,75,148,82,146,81,34,131,130,132,194,148,82,162,76,194,128,0,192
        .byte 0,0,0,0,0,0,0,0,8,16,3,192,4,32,5,160,5,160,4,32,3,192,8,16,0,0,0,0,0,0,0,0
        .byte 128,1,0,128,32,4,19,200,4,32,9,144,19,200,86,104,22,106,19,200,9,144,4,32,19,200,32,4,1,0,128,1
        .byte 0,0,0,112,0,248,0,208,0,232,14,208,17,112,36,130,35,237,5,1,35,192,82,240,34,8,12,4,16,13,116,0
        .byte 0,112,0,248,0,208,0,232,0,144,14,192,49,112,68,128,3,228,101,10,131,193,66,98,13,16,16,8,58,16,0,58
        .byte 0,0,14,0,31,0,11,0,23,0,11,112,14,136,65,36,183,196,128,160,3,196,15,74,16,68,32,48,176,8,0,46
        .byte 14,0,31,0,11,0,23,0,9,0,3,112,14,140,1,34,39,192,80,166,131,193,70,66,8,176,16,8,8,92,92,0
        .byte 3,128,7,192,6,129,55,64,64,130,8,4,18,88,9,128,136,0,160,152,66,32,38,0,0,8,44,73,48,4,40,7
        .byte 0,0,3,128,7,192,6,128,39,64,64,129,128,0,18,66,41,144,132,12,160,0,198,176,34,0,8,73,52,20,104,71
        .byte 0,0,0,0,0,0,0,0,3,128,7,192,6,128,7,64,64,128,128,1,144,0,42,68,1,148,168,2,52,136,106,87
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,128,7,192,70,136,175,87
        .byte 1,128,3,192,6,128,11,224,3,0,45,192,28,27,7,66,27,216,29,16,46,192,83,224,44,80,48,168,192,18,112,28
        .byte 1,128,11,192,6,128,3,224,3,65,13,2,28,218,38,0,27,152,93,200,46,64,19,160,10,104,22,208,8,32,30,120
        .byte 1,128,3,192,1,96,7,208,0,192,3,180,216,56,66,224,27,216,8,184,3,116,7,202,10,52,21,12,72,3,56,14
        .byte 1,128,3,208,1,96,7,192,130,192,64,176,91,56,0,100,25,216,19,186,2,116,5,200,22,80,11,104,4,16,30,120
        .byte 5,128,3,192,1,96,7,208,0,192,3,160,100,32,47,240,11,208,27,208,32,8,15,96,26,60,20,84,8,8,60,30
        .byte 3,0,0,32,32,64,16,24,0,64,97,8,0,160,206,112,91,208,18,208,32,8,15,96,26,60,20,84,8,8,60,30
        .byte 0,0,0,0,8,0,0,16,64,8,128,0,16,4,32,2,10,128,215,50,163,202,6,224,27,88,20,44,12,80,60,30
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,18,2,65,152,59,109,221,219
        .byte 12,240,57,8,68,184,133,80,5,104,2,48,6,4,10,49,16,6,33,131,35,0,64,10,64,148,64,144,128,8,170,168
        .byte 0,240,13,8,56,184,69,80,133,104,2,48,6,52,10,1,17,6,32,3,33,128,67,10,64,4,64,136,128,4,213,84
        .byte 15,48,16,156,29,34,10,161,22,160,12,64,32,96,140,80,96,8,193,132,0,196,80,2,41,2,9,2,16,1,21,85
        .byte 15,0,16,176,29,28,10,162,22,161,12,64,44,96,128,80,96,136,192,4,1,132,80,194,32,2,17,2,32,1,42,171
        .byte 0,33,0,66,8,98,4,62,36,92,34,234,22,62,141,85,86,64,58,170,85,28,73,0,148,144,32,142,40,97,80,152
        .byte 0,0,0,33,4,66,4,98,2,62,2,92,6,234,141,62,86,85,58,170,85,28,136,128,148,72,16,132,17,115,44,5
        .byte 132,0,66,0,70,16,124,32,58,36,87,68,124,104,170,177,2,106,85,92,56,170,0,146,9,41,113,4,134,20,25,10
        .byte 0,0,132,0,66,32,70,32,124,64,58,64,87,96,124,177,170,106,85,92,56,170,1,17,18,41,33,8,206,136,160,52
        .byte 2,128,5,64,10,160,5,64,10,160,53,80,66,136,69,68,66,132,65,66,128,2,128,2,128,1,103,201,26,54,2,0
        .byte 2,128,5,64,10,160,5,64,10,160,21,88,34,132,69,68,66,132,69,2,64,2,128,2,128,2,155,228,100,88,0,64
        .byte 0,160,1,80,2,224,5,192,10,240,21,96,42,0,84,64,8,168,16,160,33,16,194,128,130,160,156,160,97,0,1,128
        .byte 0,160,1,80,2,224,5,192,10,240,85,96,42,0,20,64,8,164,49,24,65,64,70,48,137,8,178,48,196,64,2,0
        .byte 5,0,10,128,7,64,3,160,15,80,6,168,0,84,2,42,21,16,5,8,8,132,1,67,5,65,5,57,0,134,1,128
        .byte 5,0,10,128,7,64,3,160,15,80,6,170,0,84,2,40,37,16,24,140,2,130,12,98,16,145,12,77,2,35,0,64
        .byte 0,0,32,8,80,20,72,36,139,162,151,210,165,74,198,198,131,130,66,132,0,0,0,0,2,128,1,0,0,0,0,0
        .byte 0,0,0,0,0,0,48,24,75,164,151,210,149,82,166,202,163,138,146,146,80,20,50,152,9,32,0,0,0,0,0,0
        .byte 5,64,3,128,27,176,33,8,82,148,17,16,32,8,32,136,33,72,68,68,66,4,68,4,64,4,128,2,128,2,170,170
        .byte 5,64,4,64,28,112,62,248,93,116,30,240,63,248,63,120,62,184,123,188,125,252,123,252,127,252,255,254,255,254,213,86
frmlst:
        .byte 0,2
        .byte 2,2
        .byte 4,2
        .byte 6,2
        .byte 8,4
        .byte 12,2
        .byte 14,2
        .byte 16,4
        .byte 20,2
        .byte 22,2
        .byte 24,2
        .byte 26,2
        .byte 28,2
        .byte 30,2
        .byte 32,2
        .byte 34,2
        .byte 36,1
        .byte 37,1,38,0
.out .sprintf("SPRITES SIZE = %d", (* - sprgfx))

scdat:
        .word 180,362,411,350,197,294,322,342,323,351,169,401,393,348,251,207,274,9
        .byte 255,15,11,21,22,27,28,21,22,255,15,10,58,255,16,11,23,24,0,0,23,24,255,16,11,255,15,11,23,24,0,0,23
        .byte 24,255,15,11,255,16,11,25,26,0,0,25,26,255,16,11,255,0,56,255,15,28,255,16,28,15,15,15,255,0,4,255,15,18
        .byte 0,15,15,16,16,16,255,0,14,255,16,5,255,0,4,16,16,15,15,15,255,0,23,15,15,16,16,255,0,24,16,16,15,15
        .byte 255,0,24,15,15,16,16,255,0,23,16,16,16,15,15,255,0,23,15,15,15,16,16,255,0,24,16,16,15,15,15,255,0,23
        .byte 15,15,16,16,16,255,0,22,16,16,16,15,15,15,255,0,22,15,15,15,16,16,255,0,4,255,16,8,255,0,6,16,16
        .byte 0,0,0,16,16,16,255,15,28,255,16,28
        .byte 255,15,28,255,16,4,255,0,10,16,16,255,0,11,16,255,15,4,255,0,10,15,15,255,0,11,15,255,16,4,0,0,16,16
        .byte 0,0,16,16,0,0,16,16,0,0,255,16,7,0,0,16,15,17,18,15,0,0,15,15,53,53,15,15,0,0,15,15,0,0
        .byte 255,15,7,0,0,0,16,255,0,5,16,16,53,53,16,16,0,0,16,16,0,0,255,16,7,0,0,0,15,255,0,5,255,15,6
        .byte 0,0,15,15,0,0,255,15,7,0,0,15,16,19,20,16,0,0,255,16,4,255,0,13,16,16,0,0,16,255,15,4,0,0
        .byte 255,15,4,255,0,13,15,15,0,0,15,255,16,4,0,0,255,16,4,0,0,255,16,4,0,0,16,16,16,0,0,16,16
        .byte 0,0,16,255,0,8,15,15,0,0,15,17,18,15,0,0,15,15,15,255,0,6,15,255,0,8,16,16,255,0,8,16,16,16
        .byte 255,0,6,16,255,15,6,0,0,15,15,255,0,8,255,15,7,0,0,15,255,16,6,0,0,16,16,0,0,16,19,20,16
        .byte 255,0,5,255,16,4,0,0,16,15,255,0,7,15,15,0,0,255,15,4,255,0,5,255,15,4,0,0,15,16,255,0,7
        .byte 16,16,0,0,255,16,7,255,0,8,16,15,0,0,255,15,7,0,0,255,15,7,255,0,8,15,16,0,0,255,16,7,0,0
        .byte 255,16,7,0,0,16,16,0,0,16,16,16,15,0,0,255,15,7,0,0,255,15,7,0,0,15,15,0,0,17,18,15,16
        .byte 255,0,20,16,16,255,0,4,16,15,255,0,20,15,15,255,0,4,15,255,16,25,19,20,16
        .byte 255,16,13,17,18,255,16,13,15,15,15,255,0,6,15,15,255,0,6,255,15,4,255,0,6,15,16,16,16,255,0,6,16,16
        .byte 255,0,6,16,16,255,0,8,16,15,15,15,0,0,15,15,0,0,15,15,0,0,19,20,0,0,15,15,255,0,4,15,15
        .byte 0,0,15,16,16,16,0,0,17,18,0,0,16,16,0,0,16,16,255,0,6,255,16,4,0,0,16,15,255,0,8,15,15
        .byte 0,0,15,15,255,0,6,255,15,4,0,0,15,16,255,0,8,16,16,0,0,16,16,16,0,0,255,16,4,53,53,0,0,0
        .byte 16,15,0,0,15,15,19,20,0,0,15,15,0,0,15,15,15,0,0,255,15,4,53,53,0,0,0,15,16,0,0,255,16,4
        .byte 0,0,16,16,255,0,8,255,16,9,15,0,0,0,15,15,15,0,0,15,15,255,0,8,255,15,9,16,0,0,0,16,16,16
        .byte 0,0,255,16,8,255,0,11,15,15,0,0,15,15,0,0,0,255,15,8,255,0,11,16,16,0,0,16,16,0,0,0,16,16
        .byte 255,0,8,255,16,6,0,0,16,15,15,0,0,15,15,0,0,15,15,15,255,0,8,255,15,6,0,0,15,16,0,0,0
        .byte 16,16,0,0,16,16,16,0,0,16,16,0,0,255,16,8,0,0,16,15,0,0,0,15,15,255,0,7,15,15,0,0,255,15,8
        .byte 0,0,15,16,0,0,16,16,16,255,0,7,16,16,255,0,12,16,15,0,0,255,15,8,0,0,15,15,255,0,12,15,16
        .byte 0,0,255,16,4,17,18,16,16,0,0,16,16,0,0,16,16,17,18,0,0,255,16,5,15,255,0,8,15,15,255,0,6
        .byte 15,15,255,0,4,255,15,5,16,255,0,8,16,16,255,0,6,16,16,255,0,4,255,16,5,255,15,4,0,0,15,19,20
        .byte 255,15,10,19,20,255,15,7
        .byte 255,15,4,0,0,255,15,22,16,255,0,16,16,16,255,0,6,53,53,16,15,255,0,16,15,15,255,0,6,53,53,15,16
        .byte 0,0,255,16,4,0,0,255,16,6,0,0,16,16,0,0,255,16,7,15,0,0,255,15,4,0,0,255,15,6,0,0,15,15
        .byte 0,0,15,255,29,6,16,255,0,16,16,16,0,0,16,29,11,11,0,0,0,15,255,0,16,15,15,0,0,15,29,11,11
        .byte 0,0,0,16,0,0,16,16,0,0,17,18,0,0,16,16,0,0,255,16,4,0,0,16,255,29,6,15,0,0,15,15,255,0,6
        .byte 15,15,0,0,255,15,4,0,0,255,15,7,16,0,0,16,16,255,0,6,16,16,0,0,16,16,255,0,10,16,15,0,0
        .byte 15,15,0,0,19,20,0,0,15,15,255,0,14,15,16,255,0,18,16,16,0,0,16,16,0,0,16,15,255,0,14,15,15
        .byte 0,0,15,15,0,0,15,15,0,0,15,16,17,18,0,0,255,16,12,255,0,6,16,16,0,0,16,15,255,0,4,255,15,12
        .byte 255,0,6,17,18,0,0,15,16,255,0,4,255,16,4,255,0,6,255,16,6,255,0,6,16,15,19,20,255,15,6,255,0,6
        .byte 255,15,6,255,0,6,15,255,16,9,0,0,16,16,0,0,255,16,6,0,0,19,20,0,0,16,15,15,255,0,5,17,18
        .byte 0,0,15,15,0,0,255,15,6,0,0,15,15,0,0,15,16,16,255,0,25,16,15,15,0,0,15,255,0,22,15,16,16
        .byte 0,0,16,16,16,19,20,255,16,13,0,0,255,16,4
        .byte 59,59,0,0,255,59,18,0,0,255,59,5,255,0,20,59,0,0,0,53,53,59,59,255,0,20,59,0,0,0,53,53,59,59
        .byte 0,0,59,17,18,255,59,9,0,0,59,59,0,0,255,59,8,255,0,26,59,59,255,0,26,59,59,0,0,59,19,20,255,59,9
        .byte 0,0,255,59,8,0,0,59,59,255,0,26,59,59,255,0,26,255,59,16,0,0,255,59,8,0,0,59,59,255,0,26,59,59
        .byte 255,0,26,59,59,0,0,255,59,4,0,0,255,59,13,0,0,255,59,5,255,0,23,59,0,0,0,59,255,0,23,59,0,0,0
        .byte 59,0,0,255,59,4,0,0,255,59,13,17,18,59,0,0,59,59,255,0,23,59,0,0,59,59,255,0,23,59,0,0,59,59
        .byte 0,0,255,59,17,0,0,19,20,59,0,0,59,59,255,0,26,59,59,255,0,26,255,59,29
        .byte 255,29,19,0,0,255,29,8,255,0,17,29,255,0,6,53,53,29,29,255,0,17,29,255,0,6,53,53,29,29,0,0,255,29,4
        .byte 0,0,255,29,4,0,0,29,0,0,255,29,4,0,0,255,29,5,0,0,29,17,18,29,255,0,8,29,0,0,29,255,0,8
        .byte 29,29,255,0,14,29,0,0,29,255,0,8,29,29,255,0,8,255,29,7,0,0,29,0,0,255,29,4,0,0,29,29,0,0
        .byte 29,19,20,29,0,0,255,29,7,0,0,29,0,0,29,17,18,29,0,0,29,29,0,0,255,29,4,0,0,255,29,7,255,0,11
        .byte 29,29,0,0,255,29,4,0,0,255,29,7,255,0,11,29,29,255,0,14,255,29,7,19,20,255,29,5,255,0,26,255,29,14
        .byte 255,0,14,29,255,0,13,255,29,6,0,0,255,29,4,0,0,29,255,0,18,29,0,0,29,17,18,29,0,0,255,29,8
        .byte 0,0,17,18,255,0,7,29,255,0,8,255,29,8,255,0,6,29,29,29,0,0,29,255,0,8,29,29,53,53,255,29,4
        .byte 255,0,6,29,29,29,0,0,29,0,0,29,19,20,29,0,0,29,29,53,53,255,29,4,0,0,19,20,0,0,29,29,29
        .byte 0,0,29,0,0,255,29,4,0,0,29,29,255,0,26,29,29,255,0,26,255,29,29
        .byte 255,29,16,17,18,255,29,11,255,0,5,29,255,0,20,29,29,255,0,5,29,255,0,20,29,29,0,0,29,0,0,29,0,0
        .byte 29,0,0,255,29,4,19,20,255,29,7,0,0,29,29,0,0,29,255,0,5,29,53,53,29,255,0,14,29,0,0,0,29
        .byte 255,0,5,29,53,53,29,255,0,14,29,0,0,0,255,29,10,0,0,255,29,18,255,0,4,29,255,0,5,29,255,0,5
        .byte 29,255,0,5,255,29,6,255,0,4,29,255,0,5,29,255,0,5,29,255,0,5,255,29,8,0,0,29,0,0,29,0,0
        .byte 29,0,0,29,0,0,29,0,0,29,0,0,29,29,255,0,11,29,255,0,5,29,0,0,29,0,0,29,0,0,29,29,255,0,11
        .byte 29,255,0,5,29,0,0,29,0,0,29,0,0,29,29,0,0,255,29,7,17,18,255,29,7,0,0,29,0,0,29,0,0
        .byte 29,29,255,0,15,53,53,29,255,0,5,29,0,0,29,29,255,0,15,53,53,29,255,0,5,29,0,0,255,29,8,0,0
        .byte 29,19,20,255,29,13,0,0,29,29,255,0,8,29,255,0,11,255,29,4,0,0,29,29,255,0,8,29,255,0,11,255,29,4
        .byte 0,0,29,29,0,0,255,29,7,0,0,255,29,4,0,0,29,0,0,29,17,18,29,0,0,29,29,255,0,17,29,255,0,8
        .byte 29,29,255,0,17,29,255,0,8,255,29,20,0,0,29,19,20,255,29,4
        .byte 255,15,28,16,16,255,0,18,13,11,255,0,5,16,15,15,255,0,18,11,11,255,0,5,15,16,16,0,0,255,16,7,21
        .byte 22,0,0,21,22,255,16,8,0,0,16,255,0,4,255,15,7,23,24,14,12,23,24,255,15,8,0,0,15,255,0,4,255,16,7
        .byte 23,24,14,12,23,24,255,16,8,0,0,16,255,15,11,25,26,0,0,25,26,255,15,8,0,0,15,255,16,5,255,0,16
        .byte 53,53,16,16,0,0,16,255,15,5,255,0,16,53,53,15,15,0,0,15,16,255,0,6,255,16,18,0,0,0,15,255,0,6
        .byte 255,15,18,0,0,0,16,0,0,16,16,0,0,16,16,255,0,6,16,16,255,0,6,255,16,5,15,0,0,17,18,0,0
        .byte 15,15,255,0,6,17,18,255,0,6,255,15,5,16,255,0,6,16,16,0,0,16,16,255,0,6,16,16,255,0,6,16,15
        .byte 255,0,6,15,15,0,0,15,15,255,0,6,15,15,255,0,6,15,16,0,0,19,20,255,16,4,255,0,6,19,20,0,0
        .byte 16,16,0,0,16,16,0,0,16,15,0,0,255,15,6,255,0,6,15,15,0,0,15,15,0,0,15,15,0,0,15,16,255,0,6
        .byte 16,16,0,0,255,16,10,0,0,16,16,0,0,16,15,255,0,6,15,15,0,0,255,15,10,0,0,17,18,0,0,15,16
        .byte 0,0,16,16,255,0,6,16,16,255,0,14,16,15,0,0,15,15,255,0,6,15,15,255,0,14,15,255,16,13,0,0,255,16,8
        .byte 19,20,16,16,16
        .byte 255,30,28,31,255,0,9,53,53,0,31,31,255,0,12,31,30,255,0,9,53,53,30,255,0,14,30,31,0,0,255,31,10
        .byte 255,0,4,31,31,0,0,255,31,4,0,0,31,30,255,0,14,255,30,4,53,53,255,30,4,0,0,30,31,255,0,15,31,31,31
        .byte 53,53,31,17,18,31,0,0,31,30,0,0,255,30,5,0,0,30,30,30,0,0,255,30,7,255,0,5,30,31,0,0,255,31,5
        .byte 0,0,31,31,255,0,6,31,31,31,255,0,6,31,30,0,0,255,30,5,0,0,30,30,30,255,0,7,30,30,19,20,30
        .byte 0,0,30,0,0,0,255,31,5,0,0,255,31,6,255,0,6,31,31,31,0,0,31,0,0,0,255,30,4,255,0,8,30,30,30
        .byte 255,0,9,30,31,0,0,31,31,31,255,0,10,255,31,4,255,0,7,31,30,0,0,255,30,4,0,0,255,30,4,0,0
        .byte 255,30,10,0,0,30,31,0,0,31,17,18,31,0,0,31,31,31,255,0,6,255,31,7,0,0,31,30,255,0,9,30,30,30
        .byte 255,0,7,255,30,5,0,0,30,31,255,0,8,255,31,7,255,0,6,31,31,31,0,0,31,255,30,4,19,20,255,30,12
        .byte 255,0,9,30,255,31,20,255,0,7,31,255,30,9,255,0,9,255,30,7,0,0,30,31,255,0,26,31,30,255,0,10,255,31,5
        .byte 255,0,11,30,31,31,31,0,0,255,31,17,0,0,255,31,4
        .byte 31,31,31,0,0,255,31,17,0,0,255,31,4,30,255,0,4,255,30,7,255,0,15,30,31,255,0,5,255,31,4,255,0,8
        .byte 31,31,255,0,6,31,31,30,0,0,255,30,5,255,0,6,17,18,0,0,30,0,0,0,30,30,0,0,0,30,31,0,0
        .byte 31,31,31,255,0,6,30,30,255,0,4,31,255,0,4,31,0,0,0,31,30,255,0,9,30,30,30,255,0,8,255,30,4
        .byte 0,0,30,31,255,0,7,30,30,255,31,4,19,20,0,0,31,0,0,255,31,4,0,0,31,30,0,0,255,30,9,255,0,6
        .byte 30,0,0,0,30,30,0,0,0,30,31,255,0,5,31,31,31,53,53,255,0,11,31,255,0,4,31,30,255,0,4,255,30,4
        .byte 53,53,30,0,0,255,30,6,0,0,30,30,0,0,30,30,31,31,31,255,0,16,31,0,0,31,31,0,0,31,31,30,30
        .byte 255,0,9,30,255,0,15,30,255,31,9,0,0,255,31,6,0,0,31,255,0,7,31,30,255,0,15,30,0,0,30,30,30
        .byte 0,0,255,30,4,31,255,0,18,17,18,255,0,6,31,30,0,0,255,30,4,17,18,255,30,8,255,0,10,30,31,255,0,15
        .byte 31,255,0,6,31,31,0,0,31,30,0,0,30,255,0,13,30,30,19,20,0,0,30,0,0,0,30,31,0,0,31,0,0
        .byte 31,19,20,255,31,5,0,0,255,31,5,0,0,31,31,0,0,31,30,255,0,15,30,30,255,0,9,30,31,255,0,5,31
        .byte 255,0,20,31,255,30,7,0,0,255,30,10,0,0,255,30,7
        .byte 255,30,7,0,0,255,30,10,0,0,255,30,7,31,53,53,255,0,6,31,255,0,17,30,30,53,53,255,0,6,30,255,0,17
        .byte 30,255,31,7,17,18,31,0,0,255,31,13,0,0,30,30,255,0,26,30,31,255,0,26,31,30,0,0,255,30,4,19,20
        .byte 255,30,16,0,0,30,31,255,0,26,31,30,255,0,26,30,255,31,12,0,0,255,31,14,30,255,0,26,30,31,255,0,26
        .byte 31,30,0,0,255,30,22,17,18,30,31,255,0,26,31,30,255,0,26,30,255,31,23,0,0,19,20,31,30,255,0,26,30
        .byte 31,255,0,26,31,30,0,0,17,18,255,30,23,31,255,0,26,31,30,255,0,26,30,31,31,31,19,20,255,31,14,0,0
        .byte 255,31,7
        .byte 255,60,19,0,0,255,60,8,255,0,5,60,255,0,11,60,0,0,60,255,0,5,60,60,255,0,5,60,255,0,11,60,0,0
        .byte 60,255,0,5,60,60,0,0,60,0,0,60,0,0,255,60,7,0,0,60,0,0,60,0,0,60,0,0,60,60,0,0,60
        .byte 255,0,5,60,255,0,8,60,255,0,5,60,0,0,60,60,0,0,60,255,0,5,60,255,0,8,60,255,0,5,60,0,0
        .byte 60,60,0,0,60,0,0,60,0,0,60,0,0,60,60,17,18,255,60,9,0,0,60,60,0,0,60,0,0,60,53,53,60
        .byte 255,0,11,60,255,0,5,60,60,0,0,60,0,0,60,53,53,60,255,0,11,60,255,0,5,60,60,0,0,60,0,0,255,60,8
        .byte 19,20,60,60,60,0,0,60,0,0,60,0,0,60,60,0,0,60,255,0,5,60,255,0,8,60,255,0,5,60,0,0,60,60
        .byte 0,0,60,255,0,5,60,255,0,8,60,255,0,5,60,0,0,60,60,0,0,255,60,4,0,0,60,0,0,60,0,0,60
        .byte 0,0,255,60,4,0,0,60,0,0,60,60,255,0,5,60,255,0,5,60,0,0,60,0,0,60,255,0,8,60,60,255,0,5
        .byte 60,255,0,5,60,0,0,60,0,0,60,255,0,8,60,60,0,0,60,0,0,60,0,0,60,0,0,60,0,0,60,0,0
        .byte 60,0,0,60,17,18,255,60,5,0,0,60,255,0,5,60,0,0,60,53,53,60,0,0,60,255,0,8,60,60,0,0,60
        .byte 255,0,5,60,0,0,60,53,53,60,0,0,60,255,0,8,60,60,0,0,60,17,18,60,0,0,60,0,0,255,60,4,0,0
        .byte 255,60,4,19,20,60,0,0,60,255,0,9,60,255,0,17,60,255,0,9,60,255,0,17,255,60,5,19,20,255,60,22
        .byte 35,255,36,11,37,0,0,35,255,36,10,37,38,38,255,0,14,43,44,53,53,255,0,8,39,39,255,0,14,41,44,53,53
        .byte 255,0,8,39,39,0,0,43,42,42,44,0,0,43,42,41,255,42,6,44,0,0,43,42,42,41,0,0,39,39,0,0,41
        .byte 42,42,42,0,0,41,42,42,17,18,255,42,4,41,0,0,42,42,42,44,0,0,39,39,255,0,4,43,44,255,0,17,42
        .byte 0,0,39,39,255,0,4,43,255,0,17,41,44,0,0,39,39,42,41,0,0,255,42,7,19,20,255,42,4,41,42,44,0,0
        .byte 42,44,0,0,39,39,42,44,0,0,17,18,41,255,42,11,17,18,0,0,43,42,0,0,39,39,255,0,8,43,255,0,13
        .byte 42,42,0,0,39,39,255,0,8,42,41,255,0,12,43,41,0,0,39,39,43,42,42,42,19,20,0,0,43,42,0,0,43
        .byte 42,42,44,0,0,19,20,255,0,6,39,39,255,42,4,41,44,0,0,41,42,0,0,42,42,42,41,0,0,43,44,255,0,6
        .byte 39,39,255,0,8,42,44,255,0,4,43,42,0,0,0,255,42,6,41,39,39,255,0,8,43,255,0,5,41,44,0,0,43
        .byte 41,255,42,6,39,39,0,0,43,42,42,42,0,255,42,4,44,0,0,42,255,0,11,39,39,0,0,41,255,42,9,0,0
        .byte 43,42,255,0,10,39,39,0,0,42,255,0,7,42,41,0,0,42,42,41,255,42,6,44,0,0,39,39,0,0,43,42,255,0,6
        .byte 17,18,0,0,255,42,7,41,42,41,0,0,39,39,255,0,6,43,44,255,0,7,44,255,0,10,39,39,255,0,6,42,41
        .byte 255,0,6,43,41,255,0,10,40,40,35,255,36,9,19,20,255,36,8,37,0,0,35,36,36,37
        .byte 35,255,36,20,37,0,0,35,36,37,38,38,255,0,15,45,255,0,4,45,0,0,0,53,53,39,39,255,0,7,45,255,0,15
        .byte 47,53,53,39,39,0,0,0,17,18,45,255,0,5,47,255,0,7,45,46,255,0,5,39,39,255,0,8,47,0,0,0,45
        .byte 255,0,4,45,255,0,5,45,0,0,39,39,0,46,255,0,7,46,255,0,16,39,39,0,0,0,19,20,45,0,0,45,47
        .byte 0,0,0,46,0,0,47,46,0,0,45,46,255,0,4,39,39,0,0,45,255,0,19,47,0,0,46,39,39,47,255,0,14
        .byte 46,255,0,10,39,39,0,0,46,0,0,46,0,47,45,0,0,46,0,0,45,0,46,0,0,47,0,17,18,0,0,45,39,39
        .byte 255,0,14,47,255,0,11,39,39,255,0,6,47,0,0,47,255,0,16,39,39,0,47,0,0,45,0,0,46,0,17,18,0,0
        .byte 46,45,0,0,47,0,0,45,19,20,46,0,0,39,39,255,0,17,46,255,0,8,39,39,45,255,0,7,45,255,0,5,47
        .byte 255,0,11,39,40,0,0,47,0,0,45,0,0,47,19,20,47,0,0,45,0,0,47,0,0,46,0,47,45,0,0,39,255,0,27
        .byte 39,255,0,4,47,255,0,5,45,255,0,5,45,0,0,47,0,45,0,0,46,0,0,39,38,0,0,45,0,0,46,0,0
        .byte 47,0,0,46,0,0,46,47,255,0,5,46,47,0,0,0,39,39,255,0,9,46,255,0,16,39,39,255,0,11,47,255,0,6
        .byte 47,255,0,6,46,40,40,35,255,36,25,37
        .byte 255,0,12,49,0,0,50,255,0,23,49,48,0,0,48,50,255,0,21,49,48,48,0,0,48,48,50,255,0,19,49,48,48,48
        .byte 0,0,48,48,48,50,255,0,17,49,255,48,4,0,0,255,48,4,50,255,0,15,49,255,48,4,47,0,0,47,255,48,4
        .byte 50,255,0,13,49,255,48,4,17,18,0,0,46,46,255,48,4,50,255,0,11,49,255,48,4,255,0,8,255,48,4,50,255,0,9
        .byte 49,255,48,4,255,0,10,255,48,4,50,255,0,7,49,255,48,4,45,45,45,19,20,45,45,45,0,0,17,18,255,48,4
        .byte 50,255,0,5,49,255,48,4,255,0,14,255,48,4,50,0,0,0,49,255,48,4,255,0,16,255,48,4,50,0,49,255,48,4
        .byte 17,18,0,0,255,47,9,19,20,47,47,47,255,48,4,50,255,48,4,255,0,20,255,48,7,255,0,22,255,48,5,46,46,46
        .byte 19,20,255,46,15,0,0,45,47,48,48,48,255,0,23,47,0,0,0,48,255,0,23,46,0,0,0,48,0,0,255,45,22
        .byte 0,0,48,48,255,0,26,48,48,255,0,26,255,48,29
        .byte 255,48,29,255,0,26,48,48,255,0,26,48,48,255,0,11,255,52,4,255,0,11,48,48,255,0,26,48,48,255,0,26,48,48
        .byte 255,0,26,48,48,17,18,48,48,17,18,48,48,17,18,48,48,0,0,48,48,17,18,48,48,17,18,48,48,17,18,48,48
        .byte 0,0,48,48,0,0,48,48,0,0,48,48,0,0,48,48,0,0,48,48,0,0,48,48,0,0,48,48,0,0,48,48,0,0
        .byte 48,48,0,0,48,48,0,0,48,48,0,0,48,48,0,0,48,48,0,0,48,48,0,0,19,20,0,0,19,20,0,0,19
        .byte 20,0,0,19,20,0,0,19,20,0,0,19,20,0,0,48,48,255,0,26,48,48,255,0,26,48,48,255,0,26,48,48,255,0,26
        .byte 48,48,255,0,26,48,48,255,0,26,48,48,255,0,26,48,48,255,51,26,48,48,255,0,26,48,48,255,0,26,255,48,14
        .byte 45,45,255,48,13
        .byte 34,255,32,26,34,33,255,0,26,33,33,255,0,26,33,33,0,0,34,255,32,20,34,0,0,33,33,255,0,23,33,0,0
        .byte 33,33,255,0,23,33,0,0,33,33,0,0,34,0,0,34,255,32,14,34,0,0,33,0,0,33,33,0,0,33,0,0,33
        .byte 255,0,14,33,0,0,33,0,0,33,33,0,0,33,0,0,33,255,0,14,33,0,0,33,0,0,33,33,0,0,33,0,0
        .byte 33,0,0,34,255,32,8,34,0,0,33,0,0,33,0,0,33,33,0,0,33,0,0,33,255,0,9,12,12,33,0,0,33
        .byte 0,0,33,0,0,33,33,0,0,33,0,0,33,255,0,9,12,12,33,0,0,33,0,0,33,0,0,33,33,0,0,33,0,0
        .byte 33,0,0,34,255,32,8,34,0,0,34,0,0,33,0,0,33,33,0,0,33,0,0,33,255,0,17,33,0,0,33,33,0,0
        .byte 33,0,0,33,255,0,17,33,0,0,33,33,0,0,33,0,0,34,255,32,14,34,0,0,33,0,0,33,33,0,0,33,255,0,20
        .byte 33,0,0,33,33,0,0,33,255,0,20,33,0,0,33,33,0,0,34,255,32,20,34,0,0,34,33,255,0,27,33,255,0,27
        .byte 34,255,32,26,34
        .byte 255,0,0,255,0,0,255,0,104
numsc:
        .byte 18
.out .sprintf("SCREENS SIZE = %d", (* - scdat))

nmedat:
        .byte 0,0,8,104,255
        .byte 2,2,8,144,2,2,56,168,2,2,88,48,2,2,152,112,2,2,8,64,255
        .byte 2,2,112,8,2,2,88,48,2,2,16,88,2,2,96,128,2,2,152,104,2,2,136,168,255
        .byte 2,2,24,56,2,2,64,56,4,5,88,104,2,2,120,72,2,2,152,112,4,5,72,184,2,2,120,184,255
        .byte 5,10,80,16,4,5,8,152,4,5,40,120,4,5,104,96,4,5,136,160,4,5,144,8,255
        .byte 3,8,8,8,5,10,152,200,4,5,152,48,4,5,64,144,4,5,32,80,4,5,88,112,255
        .byte 4,5,136,8,4,5,152,96,4,5,56,40,4,5,80,200,4,5,80,104,4,5,32,56,3,8,8,152,4,5,32,200,255
        .byte 3,8,120,136,2,2,64,40,2,2,152,8,2,2,152,200,255
        .byte 4,5,144,96,4,5,80,200,4,5,112,56,4,5,104,128,4,5,56,104,4,5,40,64,3,8,8,136,255
        .byte 2,2,152,32,2,2,128,200,2,2,56,200,2,2,48,152,2,2,16,96,3,8,104,112,3,8,56,96,2,2,64,8,255
        .byte 4,5,152,8,4,5,120,184,4,5,88,8,4,5,32,120,5,10,72,96,3,8,80,200,4,5,56,48,255
        .byte 4,5,152,200,4,5,128,32,3,8,152,104,3,8,8,96,4,8,80,128,4,5,8,200,5,10,80,56,4,5,8,8,255
        .byte 4,5,40,8,4,5,8,184,4,5,152,40,4,5,120,136,3,8,72,96,5,10,40,120,255
        .byte 2,2,144,160,2,2,24,200,2,2,80,128,2,2,48,56,2,2,152,64,2,2,80,40,2,2,104,80,3,8,152,120,255
        .byte 2,2,152,64,2,2,128,136,2,2,96,56,2,2,56,128,255
        .byte 8,16,64,104,255
        .byte 3,8,152,8,3,8,128,32,3,8,8,200,3,8,32,176,3,8,104,56,3,8,56,152,255
.out .sprintf("POSITIONS SIZE = %d", (* - nmedat))

NUMOBJ = 24
objdta:
        .byte 0,0,8,32,28,112,62,248,62,248,127,252,127,252,63,248,63,248,31,240,15,224,7,192,3,128,3,128,1,0,0,0,1,32,64,1,32,64
        .byte 0,0,8,32,28,112,62,248,62,248,127,252,127,252,63,248,63,248,31,240,15,224,7,192,3,128,3,128,1,0,0,0,2,48,176,2,48,176
        .byte 0,0,8,32,28,112,62,248,62,248,127,252,127,252,63,248,63,248,31,240,15,224,7,192,3,128,3,128,1,0,0,0,3,8,200,3,8,200
        .byte 0,0,8,32,28,112,62,248,62,248,127,252,127,252,63,248,63,248,31,240,15,224,7,192,3,128,3,128,1,0,0,0,4,8,200,4,8,200
        .byte 0,0,8,32,28,112,62,248,62,248,127,252,127,252,63,248,63,248,31,240,15,224,7,192,3,128,3,128,1,0,0,0,5,136,8,5,136,8
        .byte 0,0,8,32,28,112,62,248,62,248,127,252,127,252,63,248,63,248,31,240,15,224,7,192,3,128,3,128,1,0,0,0,5,8,200,5,8,200
        .byte 0,0,8,32,28,112,62,248,62,248,127,252,127,252,63,248,63,248,31,240,15,224,7,192,3,128,3,128,1,0,0,0,6,104,128,6,104,128
        .byte 0,0,8,32,28,112,62,248,62,248,127,252,127,252,63,248,63,248,31,240,15,224,7,192,3,128,3,128,1,0,0,0,6,32,80,6,32,80
        .byte 0,0,8,32,28,112,62,248,62,248,127,252,127,252,63,248,63,248,31,240,15,224,7,192,3,128,3,128,1,0,0,0,7,56,168,7,56,168
        .byte 0,0,8,32,28,112,62,248,62,248,127,252,127,252,63,248,63,248,31,240,15,224,7,192,3,128,3,128,1,0,0,0,8,8,80,8,8,80
        .byte 0,0,8,32,28,112,62,248,62,248,127,252,127,252,63,248,63,248,31,240,15,224,7,192,3,128,3,128,1,0,0,0,8,32,152,8,32,152
        .byte 0,0,8,32,28,112,62,248,62,248,127,252,127,252,63,248,63,248,31,240,15,224,7,192,3,128,3,128,1,0,0,0,9,64,72,9,64,72
        .byte 0,0,8,32,28,112,62,248,62,248,127,252,127,252,63,248,63,248,31,240,15,224,7,192,3,128,3,128,1,0,0,0,10,8,8,10,8,8
        .byte 0,0,8,32,28,112,62,248,62,248,127,252,127,252,63,248,63,248,31,240,15,224,7,192,3,128,3,128,1,0,0,0,11,56,56,11,56,56
        .byte 0,0,8,32,28,112,62,248,62,248,127,252,127,252,63,248,63,248,31,240,15,224,7,192,3,128,3,128,1,0,0,0,11,128,104,11,128,104
        .byte 0,0,8,32,28,112,62,248,62,248,127,252,127,252,63,248,63,248,31,240,15,224,7,192,3,128,3,128,1,0,0,0,12,8,136,12,8,136
        .byte 0,0,8,32,28,112,62,248,62,248,127,252,127,252,63,248,63,248,31,240,15,224,7,192,3,128,3,128,1,0,0,0,13,8,200,13,8,200
        .byte 0,0,8,32,28,112,62,248,62,248,127,252,127,252,63,248,63,248,31,240,15,224,7,192,3,128,3,128,1,0,0,0,255,0,72,255,0,72
        .byte 0,0,0,32,0,112,0,216,1,140,0,198,0,236,1,248,3,144,7,0,14,0,28,0,62,0,29,0,10,0,4,0,3,40,184,3,40,184
        .byte 0,0,0,32,0,112,0,216,1,140,0,198,0,236,1,248,3,144,7,0,14,0,28,0,62,0,29,0,10,0,4,0,16,80,128,16,80,128
        .byte 1,0,6,192,24,48,34,136,74,164,74,164,170,170,170,170,255,254,170,170,170,170,170,170,170,170,255,254,170,170,170,170,7,8,160,7,8,160
        .byte 1,0,6,192,24,48,34,136,74,164,74,164,170,170,170,170,255,254,170,170,170,170,170,170,170,170,255,254,170,170,170,170,7,32,104,7,32,104
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,34,0,183,192,88,113,175,187,0,0,15,8,104,15,8,104
        .byte 80,160,249,208,112,160,35,208,1,168,64,208,35,232,14,208,1,136,0,96,1,224,1,224,1,240,3,240,3,248,5,84,255,0,0,255,0,0
.out .sprintf("OBJECTS SIZE = %d", (* - objdta))

font:
        .byte 0,0,0,0,0,0,0,0
        .byte 0,112,32,32,32,0,32,0
        .byte 0,108,36,0,0,0,0,0
        .byte 0,108,254,108,108,254,108,0
        .byte 0,24,126,120,126,30,126,24
        .byte 0,230,236,24,48,110,206,0
        .byte 0,48,120,48,126,204,126,0
        .byte 0,48,32,0,0,0,0,0
        .byte 0,12,24,24,24,24,12,0
        .byte 0,96,48,48,48,48,96,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,24,24,126,24,24,0
        .byte 0,0,0,0,0,24,16,32
        .byte 0,0,0,0,126,0,0,0
        .byte 0,0,0,0,0,48,48,0
        .byte 0,0,6,12,24,48,96,0
        .byte 0,124,206,222,246,230,124,0
        .byte 0,56,120,24,24,24,126,0
        .byte 0,124,198,6,124,192,254,0
        .byte 0,124,198,28,6,198,124,0
        .byte 0,24,56,120,216,254,24,0
        .byte 0,254,192,252,6,198,124,0
        .byte 0,124,192,252,198,198,124,0
        .byte 0,254,6,12,24,48,48,0
        .byte 0,124,198,124,198,198,124,0
        .byte 0,124,198,198,126,6,124,0
        .byte 0,0,0,48,0,0,48,0
        .byte 0,0,48,0,0,48,48,96
        .byte 0,0,12,24,48,24,12,0
        .byte 0,0,0,126,0,126,0,0
        .byte 0,0,48,24,12,24,48,0
        .byte 0,124,198,12,24,0,24,0
        .byte 0,124,222,254,254,192,124,0
        .byte 0,56,68,68,124,68,198,0
        .byte 0,252,66,124,66,66,252,0
        .byte 0,120,132,128,128,132,120,0
        .byte 0,248,68,66,66,68,248,0
        .byte 0,252,66,120,64,66,252,0
        .byte 0,252,66,120,64,64,192,0
        .byte 0,124,130,128,134,130,124,0
        .byte 0,198,68,124,68,68,198,0
        .byte 0,56,16,16,16,16,56,0
        .byte 0,6,2,2,130,130,124,0
        .byte 0,196,72,112,72,68,194,0
        .byte 0,192,64,64,64,66,252,0
        .byte 0,198,108,84,68,68,198,0
        .byte 0,198,100,84,76,68,198,0
        .byte 0,120,132,132,132,132,120,0
        .byte 0,252,66,66,124,64,224,0
        .byte 0,124,130,130,162,146,124,8
        .byte 0,252,66,66,124,68,198,0
        .byte 0,124,128,124,2,130,124,0
        .byte 0,124,146,16,16,16,56,0
        .byte 0,198,68,68,68,68,56,0
        .byte 0,198,68,68,68,40,16,0
        .byte 0,198,68,84,84,124,40,0
        .byte 0,198,40,16,16,40,198,0
        .byte 0,130,68,56,16,16,56,0
        .byte 0,124,136,16,32,66,252,0
        .byte 0,30,24,24,24,24,30,0
        .byte 0,0,192,96,48,24,12,0
        .byte 0,240,48,48,48,48,240,0
        .byte 0,48,120,252,48,48,48,0
        .byte 0,0,0,0,0,0,0,255
        .byte 0,60,102,248,96,96,254,0
        .byte 0,56,124,126,254,255,255,127
        .byte 0,56,124,252,254,254,254,252
        .byte 127,63,63,31,15,3,1,0
        .byte 252,248,248,240,224,128,0,0
        .byte 0,56,108,70,194,131,193,66
        .byte 0,56,108,196,134,130,6,4
        .byte 98,33,48,25,14,3,1,0
        .byte 12,8,152,48,224,128,0,0
        .byte 47,47,47,127,127,0,47,47
        .byte 232,216,232,220,236,0,232,216
        .byte 0,1,7,15,27,23,55,47
        .byte 0,128,224,240,248,216,236,220
        .byte 47,47,47,47,47,47,47,47
        .byte 232,216,232,216,232,216,232,216
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
