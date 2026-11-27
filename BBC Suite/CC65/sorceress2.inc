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
WINDOWWID = 29 ;面面面面面面面面面面面面
MAPWID = 8
        .byte 255,255,255,255,255,255,255,255
mapdat:
        .byte 255,3,1,11,8,12,11,255,255,7,6,10,13,11,9,255,255,15,16,0,12,14,4,255,255,10,10,15,16,17,11,255,255,13,14,13,5,14,2,255,255,255,255,255,18,255,255,255
        .byte 255,255,255,255,255,255,255,255
stmap:  .byte 19

evnt00:
        lda varg
        cmp #16
        bcc *+5
        jmp a00069
        lda #0
        cmp varc
        beq *+5
        jmp a00040
        jmp a00069
a00040: lda #1
        cmp vara
        beq *+5
        jmp a00069
        lda #200
        asl a
        sta sndtyp
a00069: lda #18
        cmp scno
        beq *+5
        jmp a00126
        ldy #8
        lda (z80_ix),y
        cmp #8
        bcc *+5
        jmp a00126
        tya; lda #8
        ; ldy #8
        sta (z80_ix),y
        rts
a00126: lda #254
        cmp varg
        beq *+5
        jmp a00205
        lda #12
        sta chary
        lda #5
        sta charx
        lda #25
        jsr dmsg
        lda #13
        sta chary
        lda #6
        sta charx
        lda #26
        jsr dmsg
        lda #255
        sta varg
        rts
a00205: lda #255
        cmp varg
        beq *+5
        jmp a00532
        lda joyval
        and #16
        beq :+
        jmp a00451
:
        lda #12
        sta chary
        lda #5
        sta charx
        lda #21
        sta loopa
a00265: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        dec loopa
        beq :+
        jmp a00265
:
        lda #13
        sta chary
        lda #6
        sta charx
        lda #17
        sta loopa
a00330: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        dec loopa
        beq :+
        jmp a00330
:
        lda #15
        sta chary
        lda #8
        sta charx
        lda #14
        sta loopa
a00395: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        dec loopa
        beq :+
        jmp a00395
:
        lda #0
        sta varg
        rts
        jmp a00532
a00451: lda #6
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
        lda #15
        sta chary
        lda #9
        sta charx
        lda #32
        jsr dmsg
a00532: lda #18
        cmp varg
        beq *+5
        jmp a00633
        lda #12
        sta chary
        lda #5
        sta charx
        lda #33
        jsr dmsg
        lda #13
        sta chary
        lda #7
        sta charx
        lda #34
        jsr dmsg
        lda #14
        sta chary
        lda #7
        sta charx
        lda #35
        jsr dmsg
        lda #19
        sta varg
a00633: lda #16
        cmp varg
        beq *+5
        jmp a00681
        lda #200
        ldy #8
        sta (z80_ix),y
        lda #104
        iny; ldy #9
        sta (z80_ix),y
        rts
a00681: lda #17
        cmp varg
        beq *+5
        jmp a00735
        lda #128
        ldy #8
        sta (z80_ix),y
        lda #104
        iny; ldy #9
        sta (z80_ix),y
        lda #18
        sta varg
a00735: lda #0
        cmp varg
        beq *+5
        jmp a01278
        lda #12
        sta chary
        lda #8
        sta charx
        lda #0
        jsr dmsg
        lda #13
        sta chary
        lda #7
        sta charx
        lda #1
        jsr dmsg
        lda #14
        sta chary
        lda #6
        sta charx
        lda #2
        jsr dmsg
        lda #50
        jsr delay
        lda #13
        sta chary
        lda #6
        sta charx
        lda #9
        jsr dmsg
        lda #14
        sta chary
        lda #6
        sta charx
        lda #9
        jsr dmsg
        lda #12
        sta chary
        lda #8
        sta charx
        lda #3
        jsr dmsg
        lda #13
        sta chary
        lda #7
        sta charx
        lda #4
        jsr dmsg
        lda #14
        sta chary
        lda #6
        sta charx
        lda #5
        jsr dmsg
        lda #50
        jsr delay
        lda #12
        sta chary
        lda #8
        sta charx
        lda #9
        jsr dmsg
        lda #12
        sta chary
        lda #8
        sta charx
        lda #6
        jsr dmsg
        lda #13
        sta chary
        lda #8
        sta charx
        lda #7
        jsr dmsg
        lda #14
        sta chary
        lda #7
        sta charx
        lda #8
        jsr dmsg
        lda #50
        jsr delay
        lda #12
        sta chary
        lda #6
        sta charx
        lda #19
        sta loopa
a01100: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        dec loopa
        beq :+
        jmp a01100
:
        lda #13
        sta chary
        lda #6
        sta charx
        lda #19
        sta loopa
a01165: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        dec loopa
        beq :+
        jmp a01165
:
        lda #14
        sta chary
        lda #6
        sta charx
        lda #17
        sta loopa
a01230: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        dec loopa
        beq :+
        jmp a01230
:
        lda #1
        sta varg
a01278: lda #0
        ldy #10
        sta (z80_ix),y
        lda joyval
        and #4
        beq :+
        jmp a01322
:
        lda #5
        ldy #10
        sta (z80_ix),y
a01322: lda joyval
        and #8
        beq :+
        jmp a01353
:
        lda #1
        ldy #10
        sta (z80_ix),y
a01353: lda joyval
        and #1
        beq :+
        jmp a01497
:
        lda #0
        ldy #6
        sta (z80_ix),y
        ; lda #0
        iny; ldy #7
        sta (z80_ix),y
        lda #3
        ldy #12
        sta (z80_ix),y
        lda joyval
        and #8
        beq :+
        jmp a01448
:
        lda #2
        ldy #10
        sta (z80_ix),y
        jmp a01497
a01448: lda joyval
        and #4
        beq :+
        jmp a01484
:
        lda #4
        ldy #10
        sta (z80_ix),y
        jmp a01497
a01484: lda #3
        ldy #10
        sta (z80_ix),y
a01497: lda joyval
        and #2
        beq :+
        jmp a01642
:
        lda #1
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        tya; lda #7
        ldy #12
        sta (z80_ix),y
        lda joyval
        and #8
        beq :+
        jmp a01593
:
        lda #8
        ldy #10
        sta (z80_ix),y
        jmp a01642
a01593: lda joyval
        and #4
        beq :+
        jmp a01629
:
        lda #6
        ldy #10
        sta (z80_ix),y
        jmp a01642
a01629: lda #7
        ldy #10
        sta (z80_ix),y
a01642: lda #0
        ldy #10
        cmp (z80_ix),y
        bne *+5
        jmp a01679
        ; ldy #10
        lda (z80_ix),y
        sta vard
a01679: lda joyval
        and #16
        beq :+
        jmp a01760
:
        lda #0
        cmp vard
        bne *+5
        jmp a01760
        ; lda #0
        cmp varb
        beq *+5
        jmp a01760
        lda #1
        sta varb
        ; lda #1
        sta z80_c
        lda #3
        sta z80_b
        jsr spawn
a01760: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a01995
        lda #3
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp a01838
        lda #0
        ldy #6
        sta (z80_ix),y
        ; lda #0
        iny; ldy #7
        sta (z80_ix),y
        jmp a01866
a01838: lda #1
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
a01866: jsr cangu
        beq :+
        jmp a01995
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
        jmp a01995
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
a01995: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a02277
        jsr cangu
        beq :+
        jmp a02147
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
        jmp a02147
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
a02147: jsr cangr
        beq :+
        jmp a02277
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
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a02277
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
a02277: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a02429
        jsr cangr
        beq :+
        jmp a02429
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
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a02429
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
a02429: lda #4
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a02711
        jsr cangd
        beq :+
        jmp a02581
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
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a02581
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
a02581: jsr cangr
        beq :+
        jmp a02711
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
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a02711
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
a02711: lda #5
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a02946
        lda #3
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp a02788
        lda #0
        ldy #6
        sta (z80_ix),y
        ; lda #0
        iny; ldy #7
        sta (z80_ix),y
        jmp a02816
a02788: lda #1
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
a02816: jsr cangd
        beq :+
        jmp a02946
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
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a02946
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
a02946: lda #6
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a03228
        jsr cangd
        beq :+
        jmp a03098
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
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a03098
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
a03098: jsr cangl
        beq :+
        jmp a03228
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
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a03228
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
a03228: lda #7
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a03380
        jsr cangl
        beq :+
        jmp a03380
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
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a03380
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
a03380: lda #8
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a03661
        jsr cangu
        beq :+
        jmp a03532
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
        jmp a03532
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
a03532: jsr cangl
        beq :+
        jmp a03661
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
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a03661
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
a03661: ldy #8
        lda (z80_ix),y
        cmp #4
        bcc *+5
        jmp a03705
        lda #160
        ; ldy #8
        sta (z80_ix),y
        jsr scru
        rts
a03705: lda #164
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a03750
        tya; lda #8
        ; ldy #8
        sta (z80_ix),y
        jsr scrd
        rts
a03750: ldy #9
        lda (z80_ix),y
        cmp #4
        bcc *+5
        jmp a03793
        lda #208
        ; ldy #9
        sta (z80_ix),y
        jsr scrl
        rts
a03793: lda #212
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp a03837
        lda #8
        ; ldy #9
        sta (z80_ix),y
        jsr scrr
        rts
a03837: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a03887
        lda #2
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
a03887: lda #2
        ldy #6
        cmp (z80_ix),y
        bne *+5
        jmp a03992
        lda #0
        cmp vara
        beq *+5
        jmp a03934
        ; lda #0
        jsr animsp
a03934: lda #1
        cmp vara
        beq *+5
        jmp a03963
        lda #2
        asl a
        sta sndtyp
a03963: lda #3
        cmp vara
        beq *+5
        jmp a03992
        lda #4
        asl a
        sta sndtyp
a03992: jsr skobj
        sta varobj
        lda #0
        cmp varobj
        beq *+5
        jmp a04027
        ; lda #0
        sta varr
a04027: lda #1
        cmp varobj
        beq *+5
        jmp a04053
        ; lda #1
        sta varr
a04053: lda #2
        cmp varobj
        beq *+5
        jmp a04078
        lda #0
        sta varr
a04078: lda #3
        cmp varobj
        beq *+5
        jmp a04103
        lda #2
        sta varr
a04103: lda #4
        cmp varobj
        beq *+5
        jmp a04128
        lda #0
        sta varr
a04128: lda #5
        cmp varobj
        beq *+5
        jmp a04154
        lda #3
        sta varr
a04154: ldy #8
        lda (z80_ix),y
        sta varo
        iny; ldy #9
        lda (z80_ix),y
        sta varp
        rts
.out .sprintf("EVENT 0 SIZE = %d", (* - evnt00))

evnt01:
        lda #14
        cmp varg
        beq *+5
        jmp b00032
        lda #255
        ldy #5
        sta (z80_ix),y
b00032: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp b00849
:
        lda #1
        cmp scno
        beq *+5
        jmp b00251
        lda varg
        cmp #2
        bcc *+5
        jmp b00251
        lda #2
        sta varg
        lda #9
        sta chary
        lda #1
        sta charx
        lda #10
        jsr dmsg
        lda #10
        sta chary
        lda #1
        sta charx
        lda #11
        jsr dmsg
        lda #11
        sta chary
        lda #1
        sta charx
        lda #12
        jsr dmsg
        lda #12
        sta chary
        lda #1
        sta charx
        lda #13
        jsr dmsg
        lda #13
        sta chary
        lda #1
        sta charx
        lda #14
        jsr dmsg
        lda #23
        sta chary
        lda #5
        sta charx
        lda #36
        jsr dmsg
b00251: lda #2
        cmp scno
        beq *+5
        jmp b00443
        lda varg
        cmp #3
        bcc *+5
        jmp b00443
        lda #3
        sta varg
        lda #9
        sta chary
        lda #25
        sta charx
        lda #10
        jsr dmsg
        lda #10
        sta chary
        lda #25
        sta charx
        lda #11
        jsr dmsg
        lda #11
        sta chary
        lda #25
        sta charx
        lda #12
        jsr dmsg
        lda #12
        sta chary
        lda #25
        sta charx
        lda #13
        jsr dmsg
        lda #13
        sta chary
        lda #25
        sta charx
        lda #14
        jsr dmsg
        lda #23
        sta chary
        lda #5
        sta charx
        lda #36
        jsr dmsg
b00443: lda #3
        cmp scno
        beq *+5
        jmp b00635
        lda varg
        cmp #4
        bcc *+5
        jmp b00635
        lda #4
        sta varg
        lda #1
        sta chary
        lda #25
        sta charx
        lda #10
        jsr dmsg
        lda #2
        sta chary
        lda #25
        sta charx
        lda #11
        jsr dmsg
        lda #3
        sta chary
        lda #25
        sta charx
        lda #12
        jsr dmsg
        lda #4
        sta chary
        lda #25
        sta charx
        lda #13
        jsr dmsg
        lda #5
        sta chary
        lda #25
        sta charx
        lda #14
        jsr dmsg
        lda #23
        sta chary
        lda #5
        sta charx
        lda #36
        jsr dmsg
b00635: lda #4
        cmp scno
        beq *+5
        jmp b00826
        lda varg
        cmp #5
        bcc *+5
        jmp b00826
        lda #5
        sta varg
        lda #17
        sta chary
        lda #1
        sta charx
        lda #10
        jsr dmsg
        lda #18
        sta chary
        lda #1
        sta charx
        lda #11
        jsr dmsg
        lda #19
        sta chary
        lda #1
        sta charx
        lda #12
        jsr dmsg
        lda #20
        sta chary
        lda #1
        sta charx
        lda #13
        jsr dmsg
        lda #21
        sta chary
        lda #1
        sta charx
        lda #14
        jsr dmsg
        lda #23
        sta chary
        lda #5
        sta charx
        lda #36
        jsr dmsg
b00826: lda #255
        ldy #5
        sta (z80_ix),y
        lda #240
        sta varb
b00849: lda #1
        cmp varb
        beq *+5
        jmp b00879
        lda vard
        ldy #10
        sta (z80_ix),y
b00879: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00986
        lda #4
        sta loopa
b00910: jsr cangu
        beq :+
        jmp b00950
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp b00972
b00950: lda #255
        ldy #5
        sta (z80_ix),y
        lda #240
        sta varb
b00972: dec loopa
        beq :+
        jmp b00910
:
b00986: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b01155
        lda #4
        sta loopa
b01016: jsr cangu
        beq :+
        jmp b01057
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp b01079
b01057: lda #255
        ldy #5
        sta (z80_ix),y
        lda #240
        sta varb
b01079: jsr cangr
        beq :+
        jmp b01119
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp b01141
b01119: lda #255
        ldy #5
        sta (z80_ix),y
        lda #240
        sta varb
b01141: dec loopa
        beq :+
        jmp b01016
:
b01155: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b01261
        lda #4
        sta loopa
b01185: jsr cangr
        beq :+
        jmp b01226
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp b01248
b01226: lda #255
        ldy #5
        sta (z80_ix),y
        lda #240
        sta varb
b01248: dec loopa
        beq :+
        jmp b01185
:
b01261: lda #4
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b01431
        ; lda #4
        sta loopa
b01292: jsr cangr
        beq :+
        jmp b01333
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp b01355
b01333: lda #255
        ldy #5
        sta (z80_ix),y
        lda #240
        sta varb
b01355: jsr cangd
        beq :+
        jmp b01395
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp b01417
b01395: lda #255
        ldy #5
        sta (z80_ix),y
        lda #240
        sta varb
b01417: dec loopa
        beq :+
        jmp b01292
:
b01431: lda #5
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b01537
        lda #4
        sta loopa
b01461: jsr cangd
        beq :+
        jmp b01502
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp b01524
b01502: lda #255
        ldy #5
        sta (z80_ix),y
        lda #240
        sta varb
b01524: dec loopa
        beq :+
        jmp b01461
:
b01537: lda #6
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b01706
        lda #4
        sta loopa
b01568: jsr cangd
        beq :+
        jmp b01608
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp b01630
b01608: lda #255
        ldy #5
        sta (z80_ix),y
        lda #240
        sta varb
b01630: jsr cangl
        beq :+
        jmp b01671
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp b01693
b01671: lda #255
        ldy #5
        sta (z80_ix),y
        lda #240
        sta varb
b01693: dec loopa
        beq :+
        jmp b01568
:
b01706: lda #7
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b01813
        lda #4
        sta loopa
b01737: jsr cangl
        beq :+
        jmp b01777
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp b01799
b01777: lda #255
        ldy #5
        sta (z80_ix),y
        lda #240
        sta varb
b01799: dec loopa
        beq :+
        jmp b01737
:
b01813: lda #8
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b01982
        lda #4
        sta loopa
b01843: jsr cangl
        beq :+
        jmp b01884
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp b01906
b01884: lda #255
        ldy #5
        sta (z80_ix),y
        lda #240
        sta varb
b01906: jsr cangu
        beq :+
        jmp b01946
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp b01968
b01946: lda #255
        ldy #5
        sta (z80_ix),y
        lda #240
        sta varb
b01968: dec loopa
        beq :+
        jmp b01843
:
b01982: lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp b02021
        lda #230
        asl a
        sta sndtyp
        jmp b02034
b02021: lda #250
        asl a
        sta sndtyp
b02034: lda #0
        jsr animsp
        rts
.out .sprintf("EVENT 1 SIZE = %d", (* - evnt01))

evnt02:
        lda #10
        ldy #12
        cmp (z80_ix),y
        bcc *+5
        jmp c00051
        ; ldy #12
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #12
        sta (z80_ix),y
c00051: lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00146
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
        lda #20
        ldy #12
        sta (z80_ix),y
c00146: lda #10
        ldy #12
        cmp (z80_ix),y
        bcc *+5
        jmp c01064
        lda #20
        ; ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c00232
        lda #6
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        lda #130
        asl a
        sta sndtyp
c00232: lda #23
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c00280
        lda #1
        ldy #7
        sta (z80_ix),y
        lda #150
        asl a
        sta sndtyp
c00280: lda #26
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c00328
        lda #2
        ldy #7
        sta (z80_ix),y
        lda #170
        asl a
        sta sndtyp
c00328: lda #29
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c00365
        lda #200
        ldy #8
        sta (z80_ix),y
c00365: lda #50
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c00792
        lda #5
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
        lda varrnd
        dey; ldy #11
        sta (z80_ix),y
        lda #2
        ldy #7
        sta (z80_ix),y
        lda #6
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp c00551
        lda #40
        iny; ldy #8
        sta (z80_ix),y
        ; lda #40
        iny; ldy #9
        sta (z80_ix),y
c00551: lda #1
        cmp varrnd
        beq *+5
        jmp c00596
        lda #128
        ldy #8
        sta (z80_ix),y
        lda #40
        iny; ldy #9
        sta (z80_ix),y
c00596: lda #2
        cmp varrnd
        beq *+5
        jmp c00642
        lda #40
        ldy #8
        sta (z80_ix),y
        lda #176
        iny; ldy #9
        sta (z80_ix),y
c00642: lda #3
        cmp varrnd
        beq *+5
        jmp c00688
        lda #128
        ldy #8
        sta (z80_ix),y
        lda #176
        iny; ldy #9
        sta (z80_ix),y
c00688: lda #4
        cmp varrnd
        beq *+5
        jmp c00734
        lda #40
        ldy #8
        sta (z80_ix),y
        lda #104
        iny; ldy #9
        sta (z80_ix),y
c00734: lda #5
        cmp varrnd
        beq *+5
        jmp c00780
        lda #128
        ldy #8
        sta (z80_ix),y
        lda #112
        iny; ldy #9
        sta (z80_ix),y
c00780: lda #190
        asl a
        sta sndtyp
c00792: lda #52
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c00841
        lda #1
        ldy #7
        sta (z80_ix),y
        lda #170
        asl a
        sta sndtyp
c00841: lda #54
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c00889
        lda #0
        ldy #7
        sta (z80_ix),y
        lda #150
        asl a
        sta sndtyp
c00889: lda #56
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c01061
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
        jmp c00973
        lda #4
        ldy #6
        sta (z80_ix),y
c00973: lda #1
        cmp varrnd
        beq *+5
        jmp c01003
        lda #7
        ldy #6
        sta (z80_ix),y
c01003: lda #2
        cmp varrnd
        beq *+5
        jmp c01034
        lda #12
        ldy #6
        sta (z80_ix),y
c01034: lda #0
        ldy #7
        sta (z80_ix),y
        ; lda #0
        ldy #12
        sta (z80_ix),y
c01061: rts
c01064: lda #57
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c01104
        lda #0
        ; ldy #12
        sta (z80_ix),y
        rts
c01104: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c01178
:
        ldy #12
        lda (z80_ix),y
        cmp #2
        bcc *+5
        jmp c01173
        lda #0
        cmp varc
        beq *+5
        jmp c01173
        lda #30
        sta varc
c01173: jmp c01178
c01178: lda #0
        ldy #12
        sta (z80_ix),y
        lda #1
        cmp vara
        bcc *+5
        jmp c01247
        lda #0
        cmp varr
        beq *+5
        jmp c01228
        rts
c01228: lda #1
        cmp varr
        beq *+5
        jmp c01247
        rts
c01247: lda #20
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp c01485
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
        jmp c01412
        ldy #10
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #10
        sta (z80_ix),y
        lda #9
        ; ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c01408
        lda #1
        ; ldy #10
        sta (z80_ix),y
c01408: jmp c01485
c01412: lda #255
        sta z80_c
        ldy #10
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #10
        sta (z80_ix),y
        lda #0
        ; ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c01485
        lda #8
        ; ldy #10
        sta (z80_ix),y
c01485: ldy #8
        lda (z80_ix),y
        cmp #4
        bcc *+5
        jmp c01521
        lda #5
        ldy #10
        sta (z80_ix),y
c01521: lda #212
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp c01558
        lda #7
        iny; ldy #10
        sta (z80_ix),y
c01558: ldy #9
        lda (z80_ix),y
        cmp #4
        bcc *+5
        jmp c01595
        lda #3
        iny; ldy #10
        sta (z80_ix),y
c01595: lda #164
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp c01631
        lda #1
        ldy #10
        sta (z80_ix),y
c01631: ldy #10
        lda (z80_ix),y
        cmp #5
        bcc *+5
        jmp c01875
        lda #4
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c01689
        ; lda #4
        ; ldy #6
        sta (z80_ix),y
c01689: lda #5
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c01725
        lda #4
        ; ldy #6
        sta (z80_ix),y
c01725: lda #7
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c01761
        ; lda #7
        ; ldy #6
        sta (z80_ix),y
c01761: lda #8
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c01797
        lda #7
        ; ldy #6
        sta (z80_ix),y
c01797: lda #12
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c01834
        ; lda #12
        ; ldy #6
        sta (z80_ix),y
c01834: lda #13
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c01870
        lda #12
        ; ldy #6
        sta (z80_ix),y
c01870: jmp c02105
c01875: lda #4
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c01911
        lda #5
        ; ldy #6
        sta (z80_ix),y
c01911: lda #5
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c01947
        ; lda #5
        ; ldy #6
        sta (z80_ix),y
c01947: lda #7
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c01983
        lda #8
        ; ldy #6
        sta (z80_ix),y
c01983: lda #8
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c02019
        ; lda #8
        ; ldy #6
        sta (z80_ix),y
c02019: lda #12
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c02055
        lda #13
        ; ldy #6
        sta (z80_ix),y
c02055: lda #13
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c02092
        ; lda #13
        ; ldy #6
        sta (z80_ix),y
c02092: lda #0
        ldy #7
        sta (z80_ix),y
c02105: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c02184
        jsr cangu
        beq :+
        jmp c02168
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp c02184
c02168: lda #5
        ldy #10
        sta (z80_ix),y
        rts
c02184: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c02353
        jsr cangu
        beq :+
        jmp c02247
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp c02260
c02247: lda #1
        ldy #12
        sta (z80_ix),y
c02260: jsr cangr
        beq :+
        jmp c02300
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp c02314
c02300: lda #1
        ldy #12
        sta (z80_ix),y
c02314: lda #1
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c02353
        lda #6
        ldy #10
        sta (z80_ix),y
        rts
c02353: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c02432
        jsr cangr
        beq :+
        jmp c02415
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp c02432
c02415: lda #7
        ldy #10
        sta (z80_ix),y
        rts
c02432: lda #4
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c02600
        jsr cangd
        beq :+
        jmp c02494
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp c02508
c02494: lda #1
        ldy #12
        sta (z80_ix),y
c02508: jsr cangr
        beq :+
        jmp c02548
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp c02561
c02548: lda #1
        ldy #12
        sta (z80_ix),y
c02561: lda #1
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c02600
        lda #8
        ldy #10
        sta (z80_ix),y
        rts
c02600: lda #5
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c02679
        jsr cangd
        beq :+
        jmp c02663
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp c02679
c02663: lda #1
        ldy #10
        sta (z80_ix),y
        rts
c02679: lda #6
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c02848
        jsr cangd
        beq :+
        jmp c02742
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp c02755
c02742: lda #1
        ldy #12
        sta (z80_ix),y
c02755: jsr cangl
        beq :+
        jmp c02795
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp c02809
c02795: lda #1
        ldy #12
        sta (z80_ix),y
c02809: lda #1
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c02848
        lda #2
        ldy #10
        sta (z80_ix),y
        rts
c02848: lda #7
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c02927
        jsr cangl
        beq :+
        jmp c02910
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp c02927
c02910: lda #3
        ldy #10
        sta (z80_ix),y
        rts
c02927: lda #8
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp c03095
        jsr cangu
        beq :+
        jmp c02989
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp c03003
c02989: lda #1
        ldy #12
        sta (z80_ix),y
c03003: jsr cangl
        beq :+
        jmp c03043
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp c03056
c03043: lda #1
        ldy #12
        sta (z80_ix),y
c03056: lda #1
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c03095
        lda #4
        ldy #10
        sta (z80_ix),y
        rts
c03095: lda #0
        cmp vara
        beq *+5
        jmp c03121
        ; lda #0
        jsr animsp
c03121: rts
.out .sprintf("EVENT 2 SIZE = %d", (* - evnt02))

evnt03:
        rts
.out .sprintf("EVENT 3 SIZE = %d", (* - evnt03))

evnt04:
        lda #0
        ldy #12
        cmp (z80_ix),y
        bcc *+5
        jmp e00051
        ; ldy #12
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #12
        sta (z80_ix),y
e00051: lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00383
:
        lda #120
        asl a
        sta sndtyp
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #140
        asl a
        sta sndtyp
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #160
        asl a
        sta sndtyp
        lda #240
        sta varb
        lda #120
        asl a
        sta sndtyp
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #140
        asl a
        sta sndtyp
        ldy #8
        lda (z80_ix),y
        cmp varo
        bcc *+5
        jmp e00279
        lda varp
        iny; ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp e00260
        lda #2
        iny; ldy #10
        sta (z80_ix),y
        jmp e00274
e00260: lda #8
        ldy #10
        sta (z80_ix),y
e00274: jmp e00334
e00279: lda varp
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp e00321
        lda #4
        iny; ldy #10
        sta (z80_ix),y
        jmp e00334
e00321: lda #6
        ldy #10
        sta (z80_ix),y
e00334: ldy #12
        lda (z80_ix),y
        cmp #20
        bcc *+5
        jmp e00371
        lda #20
        ; ldy #12
        sta (z80_ix),y
e00371: lda #160
        asl a
        sta sndtyp
e00383: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00435
:
        lda #0
        cmp varc
        beq *+5
        jmp e00430
        lda #30
        sta varc
e00430: jmp e00435
e00435: lda varr
        cmp #2
        bcc *+5
        jmp e00496
        lda #1
        cmp vara
        bcc *+5
        jmp e00476
        rts
        jmp e00496
e00476: lda #2
        cmp vara
        bcc *+5
        jmp e00496
        rts
e00496: ldy #8
        lda (z80_ix),y
        cmp #8
        bcc *+5
        jmp e00572
        lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e00559
        lda #4
        ; ldy #10
        sta (z80_ix),y
        jmp e00572
e00559: lda #6
        ldy #10
        sta (z80_ix),y
e00572: ldy #9
        lda (z80_ix),y
        cmp #8
        bcc *+5
        jmp e00650
        lda #6
        iny; ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e00637
        lda #4
        ; ldy #10
        sta (z80_ix),y
        jmp e00650
e00637: lda #2
        ldy #10
        sta (z80_ix),y
e00650: lda #160
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp e00727
        lda #4
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e00714
        lda #2
        ; ldy #10
        sta (z80_ix),y
        jmp e00727
e00714: lda #8
        ldy #10
        sta (z80_ix),y
e00727: lda #208
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp e00805
        lda #2
        iny; ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e00792
        lda #8
        ; ldy #10
        sta (z80_ix),y
        jmp e00805
e00792: lda #6
        ldy #10
        sta (z80_ix),y
e00805: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e00941
        jsr cangu
        beq :+
        jmp e00868
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp e00884
e00868: lda #4
        ldy #10
        sta (z80_ix),y
        rts
e00884: jsr cangr
        beq :+
        jmp e00925
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp e00941
e00925: lda #8
        ldy #10
        sta (z80_ix),y
        rts
e00941: lda #4
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e01077
        jsr cangd
        beq :+
        jmp e01004
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp e01020
e01004: lda #2
        ldy #10
        sta (z80_ix),y
        rts
e01020: jsr cangr
        beq :+
        jmp e01060
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp e01077
e01060: lda #6
        ldy #10
        sta (z80_ix),y
        rts
e01077: lda #6
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e01213
        jsr cangd
        beq :+
        jmp e01139
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp e01156
e01139: lda #8
        ldy #10
        sta (z80_ix),y
        rts
e01156: jsr cangl
        beq :+
        jmp e01196
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp e01213
e01196: lda #4
        ldy #10
        sta (z80_ix),y
        rts
e01213: lda #8
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e01348
        jsr cangu
        beq :+
        jmp e01275
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp e01292
e01275: lda #6
        ldy #10
        sta (z80_ix),y
        rts
e01292: jsr cangl
        beq :+
        jmp e01332
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp e01348
e01332: lda #2
        ldy #10
        sta (z80_ix),y
        rts
e01348: lda #0
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp e02146
        ldy #8
        lda (z80_ix),y
        cmp varo
        beq *+5
        jmp e01521
        lda #5
        sta z80_c
        lda #10
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        ldy #9
        lda (z80_ix),y
        cmp varp
        bcc *+5
        jmp e01475
        lda #3
        iny; ldy #10
        sta (z80_ix),y
        jmp e01488
e01475: lda #7
        ldy #10
        sta (z80_ix),y
e01488: lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #50
        ldy #12
        sta (z80_ix),y
e01521: lda varp
        ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp e01669
        lda #5
        sta z80_c
        lda #10
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        ldy #8
        lda (z80_ix),y
        cmp varo
        bcc *+5
        jmp e01624
        lda #5
        ldy #10
        sta (z80_ix),y
        jmp e01637
e01624: lda #1
        ldy #10
        sta (z80_ix),y
e01637: lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #50
        ldy #12
        sta (z80_ix),y
e01669: ldy #8
        lda (z80_ix),y
        ldy #11
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y
        sta varobj
        lda varo
        ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp e01767
        lda varo
        sta z80_c
        ; ldy #11
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #11
        sta (z80_ix),y
        jmp e01823
e01767: lda varo
        ldy #11
        sta (z80_ix),y
        ldy #8
        lda (z80_ix),y
        sta z80_c
        ldy #11
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #11
        sta (z80_ix),y
e01823: lda varp
        cmp varobj
        bcc *+5
        jmp e01871
        lda varp
        sta z80_c
        lda varobj
        sec
        sbc z80_c
        sta varobj
        jmp e01911
e01871: lda varp
        sta varobj
        ldy #9
        lda (z80_ix),y
        sta z80_c
        lda varobj
        sec
        sbc z80_c
        sta varobj
e01911: lda varobj
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp e02146
        lda #5
        sta z80_c
        lda #10
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda varo
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp e02058
        iny; ldy #9
        lda (z80_ix),y
        cmp varp
        bcc *+5
        jmp e02040
        lda #2
        iny; ldy #10
        sta (z80_ix),y
        jmp e02053
e02040: lda #8
        ldy #10
        sta (z80_ix),y
e02053: jmp e02113
e02058: ldy #9
        lda (z80_ix),y
        cmp varp
        bcc *+5
        jmp e02100
        lda #4
        iny; ldy #10
        sta (z80_ix),y
        jmp e02113
e02100: lda #6
        ldy #10
        sta (z80_ix),y
e02113: lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #50
        ldy #12
        sta (z80_ix),y
e02146: rts
.out .sprintf("EVENT 4 SIZE = %d", (* - evnt04))

evnt05:
        lda #0
        cmp scno
        beq *+5
        jmp f00222
        lda #255
        ldy #10
        sta (z80_ix),y
        lda #0
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp f00219
        ; lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00219
:
        lda #1
        ldy #12
        sta (z80_ix),y
        lda #6
        sta varh
        lda #1
        sta varl
        lda #5
        sta loopa
f00115: lda varl
        sta chary
        lda #30
        sta charx
        lda #18
        jsr dmsg
        lda varl
        clc
        adc #1
        sta varl
        lda varl
        sta chary
        lda #30
        sta charx
        lda #19
        jsr dmsg
        lda #2
        sta z80_c
        lda varl
        clc
        adc z80_c
        sta varl
        dec loopa
        beq :+
        jmp f00115
:
f00219: rts
f00222: lda #13
        cmp varg
        bcc *+5
        jmp f00266
        lda #200
        ldy #8
        sta (z80_ix),y
        lda #255
        ldy #5
        sta (z80_ix),y
f00266: lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00361
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
f00361: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00413
:
        lda #0
        cmp varc
        beq *+5
        jmp f00408
        lda #30
        sta varc
f00408: jmp f00413
f00413: lda #5
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        lda #5
        sec
        sbc z80_c
        lda #225
        sta z80_c
        lda varrnd
        clc
        adc z80_c
        sta varrnd
        lda varrnd
        asl a
        sta sndtyp
        lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00606
        lda #4
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp f00593
        ; ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp f00606
f00593: lda #255
        ldy #5
        sta (z80_ix),y
f00606: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00800
        lda #4
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp f00700
        ; ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp f00714
f00700: lda #255
        ldy #5
        sta (z80_ix),y
f00714: ldy #9
        lda (z80_ix),y
        cmp #212
        bcc *+5
        jmp f00786
        ; ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp f00800
f00786: lda #255
        ldy #5
        sta (z80_ix),y
f00800: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00910
        dey; ldy #9
        lda (z80_ix),y
        cmp #212
        bcc *+5
        jmp f00896
        ; ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp f00910
f00896: lda #255
        ldy #5
        sta (z80_ix),y
f00910: lda #4
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f01104
        ldy #8
        lda (z80_ix),y
        cmp #164
        bcc *+5
        jmp f01004
        ; ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp f01018
f01004: lda #255
        ldy #5
        sta (z80_ix),y
f01018: ldy #9
        lda (z80_ix),y
        cmp #212
        bcc *+5
        jmp f01090
        ; ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp f01104
f01090: lda #255
        ldy #5
        sta (z80_ix),y
f01104: lda #5
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f01212
        ldy #8
        lda (z80_ix),y
        cmp #164
        bcc *+5
        jmp f01198
        ; ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp f01212
f01198: lda #255
        ldy #5
        sta (z80_ix),y
f01212: lda #6
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f01406
        ldy #8
        lda (z80_ix),y
        cmp #164
        bcc *+5
        jmp f01307
        ; ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp f01320
f01307: lda #255
        ldy #5
        sta (z80_ix),y
f01320: lda #4
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp f01392
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp f01406
f01392: lda #255
        ldy #5
        sta (z80_ix),y
f01406: lda #7
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f01515
        lda #4
        dey; ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp f01501
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp f01515
f01501: lda #255
        ldy #5
        sta (z80_ix),y
f01515: lda #8
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f01708
        lda #4
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp f01609
        ; ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp f01623
f01609: lda #255
        ldy #5
        sta (z80_ix),y
f01623: lda #4
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp f01694
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp f01708
f01694: lda #255
        ldy #5
        sta (z80_ix),y
f01708: rts
.out .sprintf("EVENT 5 SIZE = %d", (* - evnt05))

evnt06:
        rts
.out .sprintf("EVENT 6 SIZE = %d", (* - evnt06))

evnt07:
        lda #6
        cmp varg
        beq *+5
        jmp h00218
        lda #13
        sta chary
        lda #2
        sta charx
        lda #15
        jsr dmsg
        lda #70
        jsr delay
        lda #13
        sta chary
        lda #2
        sta charx
        lda #16
        jsr dmsg
        lda #70
        jsr delay
        lda #13
        sta chary
        lda #2
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        lda #17
        jsr dmsg
        lda #70
        jsr delay
        lda #13
        sta chary
        lda #2
        sta charx
        lda #25
        sta loopa
h00170: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        dec loopa
        beq :+
        jmp h00170
:
        lda #7
        sta varg
h00218: lda #14
        cmp varg
        beq *+5
        jmp h00291
        lda #13
        sta chary
        lda #4
        sta charx
        lda #31
        jsr dmsg
        lda #70
        jsr delay
        lda #200
        ldy #8
        sta (z80_ix),y
        lda #15
        sta varg
h00291: lda #15
        cmp varg
        beq *+5
        jmp h00330
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #16
        sta varg
h00330: lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp h00448
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
        lda #190
        ldy #11
        sta (z80_ix),y
        lda #0
        jsr animsp
        lda varg
        clc
        adc #1
        sta varg
h00448: lda #0
        ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp h00535
        ; ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
        lda #200
        ; ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp h00535
        lda #0
        ; ldy #11
        sta (z80_ix),y
h00535: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp h00993
        ; lda #0
        ldy #7
        sta (z80_ix),y
        lda #1
        ldy #11
        sta (z80_ix),y
        lda #5
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda vart
        cmp varrnd
        beq *+5
        jmp h00675
        lda varrnd
        clc
        adc #1
        sta varrnd
        lda #5
        cmp varrnd
        beq *+5
        jmp h00675
        lda #0
        sta varrnd
h00675: lda #0
        cmp varrnd
        beq *+5
        jmp h00729
        lda #24
        ldy #8
        sta (z80_ix),y
        lda #56
        iny; ldy #9
        sta (z80_ix),y
        lda #0
        sta vart
h00729: lda #1
        cmp varrnd
        beq *+5
        jmp h00783
        lda #24
        ldy #8
        sta (z80_ix),y
        lda #160
        iny; ldy #9
        sta (z80_ix),y
        lda #1
        sta vart
h00783: lda #2
        cmp varrnd
        beq *+5
        jmp h00836
        lda #136
        ldy #8
        sta (z80_ix),y
        lda #56
        iny; ldy #9
        sta (z80_ix),y
        lda #2
        sta vart
h00836: lda #3
        cmp varrnd
        beq *+5
        jmp h00890
        lda #136
        ldy #8
        sta (z80_ix),y
        lda #160
        iny; ldy #9
        sta (z80_ix),y
        lda #3
        sta vart
h00890: lda #4
        cmp varrnd
        beq *+5
        jmp h00944
        lda #158
        ldy #8
        sta (z80_ix),y
        lda #108
        iny; ldy #9
        sta (z80_ix),y
        lda #4
        sta vart
h00944: lda #4
        cmp varrnd
        beq *+5
        jmp h00980
        lda #1
        ldy #10
        sta (z80_ix),y
        jmp h00993
h00980: lda #0
        ldy #10
        sta (z80_ix),y
h00993: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp h01069
        jsr cangu
        beq :+
        jmp h01056
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp h01069
h01056: lda #2
        ldy #10
        sta (z80_ix),y
h01069: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp h01145
        jsr cangr
        beq :+
        jmp h01132
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp h01145
h01132: lda #3
        ldy #10
        sta (z80_ix),y
h01145: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp h01221
        jsr cangd
        beq :+
        jmp h01208
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp h01221
h01208: lda #0
        ldy #10
        sta (z80_ix),y
h01221: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp h01297
        jsr cangl
        beq :+
        jmp h01284
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp h01297
h01284: lda #1
        ldy #10
        sta (z80_ix),y
h01297: lda #31
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp h01333
        lda #1
        sta varq
        jmp h01447
h01333: lda #100
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp h01368
        lda #1
        sta varq
        jmp h01447
h01368: lda #147
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp h01404
        lda #1
        sta varq
        jmp h01447
h01404: lda #190
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp h01439
        lda #1
        sta varq
        jmp h01447
h01439: lda #0
        sta varq
h01447: lda #1
        cmp varq
        beq *+5
        jmp h02273
        lda #5
        sta z80_c
        lda #14
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #0
        cmp vart
        beq *+5
        jmp h01534
        lda #7
        ldy #10
        sta (z80_ix),y
h01534: lda #1
        cmp vart
        beq *+5
        jmp h01564
        lda #3
        ldy #10
        sta (z80_ix),y
h01564: lda #2
        cmp vart
        beq *+5
        jmp h01594
        lda #7
        ldy #10
        sta (z80_ix),y
h01594: lda #3
        cmp vart
        beq *+5
        jmp h01625
        ; lda #3
        ldy #10
        sta (z80_ix),y
h01625: lda #4
        cmp vart
        beq *+5
        jmp h01655
        lda #1
        ldy #10
        sta (z80_ix),y
h01655: lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #5
        sta z80_c
        lda #14
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #0
        cmp vart
        beq *+5
        jmp h01744
        lda #3
        ldy #10
        sta (z80_ix),y
h01744: lda #1
        cmp vart
        beq *+5
        jmp h01774
        lda #7
        ldy #10
        sta (z80_ix),y
h01774: lda #2
        cmp vart
        beq *+5
        jmp h01804
        lda #3
        ldy #10
        sta (z80_ix),y
h01804: lda #3
        cmp vart
        beq *+5
        jmp h01835
        lda #7
        ldy #10
        sta (z80_ix),y
h01835: lda #4
        cmp vart
        beq *+5
        jmp h01865
        lda #2
        ldy #10
        sta (z80_ix),y
h01865: lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #5
        sta z80_c
        lda #14
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #0
        cmp vart
        beq *+5
        jmp h01953
        lda #4
        ldy #10
        sta (z80_ix),y
h01953: lda #1
        cmp vart
        beq *+5
        jmp h01984
        lda #6
        ldy #10
        sta (z80_ix),y
h01984: lda #2
        cmp vart
        beq *+5
        jmp h02014
        ; lda #2
        ldy #10
        sta (z80_ix),y
h02014: lda #3
        cmp vart
        beq *+5
        jmp h02045
        lda #8
        ldy #10
        sta (z80_ix),y
h02045: lda #4
        cmp vart
        beq *+5
        jmp h02075
        lda #8
        ldy #10
        sta (z80_ix),y
h02075: lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #5
        sta z80_c
        lda #14
        sta z80_b
        jsr spawn
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #0
        cmp vart
        beq *+5
        jmp h02163
        lda #5
        ldy #10
        sta (z80_ix),y
h02163: lda #1
        cmp vart
        beq *+5
        jmp h02194
        lda #5
        ldy #10
        sta (z80_ix),y
h02194: lda #2
        cmp vart
        beq *+5
        jmp h02224
        lda #1
        ldy #10
        sta (z80_ix),y
h02224: lda #3
        cmp vart
        beq *+5
        jmp h02254
        lda #1
        ldy #10
        sta (z80_ix),y
h02254: lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
h02273: rts
.out .sprintf("EVENT 7 SIZE = %d", (* - evnt07))

evnt08:
        rts
.out .sprintf("EVENT 8 SIZE = %d", (* - evnt08))

evnt09:
        lda #2
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00198
        lda #8
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
        lda varrnd
        ldy #10
        sta (z80_ix),y
        lda #51
        ldy #12
        sta (z80_ix),y
        lda #6
        ldy #6
        sta (z80_ix),y
        lda #2
        iny; ldy #7
        sta (z80_ix),y
        lda #5
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
        lda varrnd
        ldy #11
        sta (z80_ix),y
j00198: lda #4
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00308
        lda #0
        ldy #12
        sta (z80_ix),y
        lda #4
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
        lda varrnd
        asl a
        sta varrnd
        lda varrnd
        ldy #10
        sta (z80_ix),y
j00308: lda #5
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00405
        lda #0
        ldy #12
        sta (z80_ix),y
        lda #8
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
        lda varrnd
        ldy #10
        sta (z80_ix),y
j00405: lda #6
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00545
        lda #0
        ldy #11
        sta (z80_ix),y
        lda #51
        iny; ldy #12
        sta (z80_ix),y
        lda #14
        ldy #6
        sta (z80_ix),y
        lda #2
        iny; ldy #7
        sta (z80_ix),y
        lda #8
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
        lda varrnd
        ldy #10
        sta (z80_ix),y
j00545: lda #7
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00596
        lda #0
        ldy #11
        sta (z80_ix),y
        ; lda #0
        iny; ldy #12
        sta (z80_ix),y
j00596: rts
.out .sprintf("EVENT 9 SIZE = %d", (* - evnt09))

evnt10:
        lda vara
        clc
        adc #1
        sta vara
        lda #4
        cmp vara
        beq *+5
        jmp k00042
        lda #0
        sta vara
k00042: lda #0
        cmp varb
        bcc *+5
        jmp k00099
        lda varb
        clc
        adc #1
        sta varb
        lda #255
        cmp varb
        beq *+5
        jmp k00099
        lda #0
        sta varb
k00099: lda #0
        cmp vars
        bcc *+5
        jmp k00131
        lda vars
        sec
        sbc #1
        sta vars
k00131: lda #30
        cmp varc
        beq *+5
        jmp k00498
        lda varh
        sec
        sbc #1
        sta varh
        lda #5
        cmp varh
        beq *+5
        jmp k00230
        lda #1
        sta chary
        lda #30
        sta charx
        lda #20
        jsr dmsg
        lda #2
        sta chary
        lda #30
        sta charx
        lda #21
        jsr dmsg
k00230: lda #4
        cmp varh
        beq *+5
        jmp k00297
        ; lda #4
        sta chary
        lda #30
        sta charx
        lda #20
        jsr dmsg
        lda #5
        sta chary
        lda #30
        sta charx
        lda #21
        jsr dmsg
k00297: lda #3
        cmp varh
        beq *+5
        jmp k00364
        lda #7
        sta chary
        lda #30
        sta charx
        lda #20
        jsr dmsg
        lda #8
        sta chary
        lda #30
        sta charx
        lda #21
        jsr dmsg
k00364: lda #2
        cmp varh
        beq *+5
        jmp k00431
        lda #10
        sta chary
        lda #30
        sta charx
        lda #20
        jsr dmsg
        lda #11
        sta chary
        lda #30
        sta charx
        lda #21
        jsr dmsg
k00431: lda #1
        cmp varh
        beq *+5
        jmp k00498
        lda #13
        sta chary
        lda #30
        sta charx
        lda #20
        jsr dmsg
        lda #14
        sta chary
        lda #30
        sta charx
        lda #21
        jsr dmsg
k00498: lda #0
        cmp varc
        bcc *+5
        jmp k00530
        lda varc
        sec
        sbc #1
        sta varc
k00530: lda #0
        cmp varh
        beq *+5
        jmp k00555
        lda #1
        sta deadf
k00555: rts
.out .sprintf("EVENT 10 SIZE = %d", (* - evnt10))

evnt11:
        lda #0
        cmp vark
        beq *+5
        jmp l00034
        lda vark
        clc
        adc #1
        sta vark
l00034: lda #1
        cmp scno
        beq *+5
        jmp l00218
        ; lda #1
        cmp vark
        beq *+5
        jmp l00218
        ; lda #1
        cmp varg
        bcc *+5
        jmp l00218
        lda #9
        sta chary
        lda #1
        sta charx
        lda #10
        jsr dmsg
        lda #10
        sta chary
        lda #1
        sta charx
        lda #11
        jsr dmsg
        lda #11
        sta chary
        lda #1
        sta charx
        lda #12
        jsr dmsg
        lda #12
        sta chary
        lda #1
        sta charx
        lda #13
        jsr dmsg
        lda #13
        sta chary
        lda #1
        sta charx
        lda #14
        jsr dmsg
        lda #2
        sta vark
l00218: lda #2
        cmp scno
        beq *+5
        jmp l00402
        lda #1
        cmp vark
        beq *+5
        jmp l00402
        lda #2
        cmp varg
        bcc *+5
        jmp l00402
        lda #9
        sta chary
        lda #25
        sta charx
        lda #10
        jsr dmsg
        lda #10
        sta chary
        lda #25
        sta charx
        lda #11
        jsr dmsg
        lda #11
        sta chary
        lda #25
        sta charx
        lda #12
        jsr dmsg
        lda #12
        sta chary
        lda #25
        sta charx
        lda #13
        jsr dmsg
        lda #13
        sta chary
        lda #25
        sta charx
        lda #14
        jsr dmsg
        lda #2
        sta vark
l00402: lda #3
        cmp scno
        beq *+5
        jmp l00585
        lda #1
        cmp vark
        beq *+5
        jmp l00585
        lda #3
        cmp varg
        bcc *+5
        jmp l00585
        lda #1
        sta chary
        lda #25
        sta charx
        lda #10
        jsr dmsg
        lda #2
        sta chary
        lda #25
        sta charx
        lda #11
        jsr dmsg
        lda #3
        sta chary
        lda #25
        sta charx
        lda #12
        jsr dmsg
        lda #4
        sta chary
        lda #25
        sta charx
        lda #13
        jsr dmsg
        lda #5
        sta chary
        lda #25
        sta charx
        lda #14
        jsr dmsg
        lda #2
        sta vark
l00585: lda #4
        cmp scno
        beq *+5
        jmp l00768
        lda #1
        cmp vark
        beq *+5
        jmp l00768
        lda #4
        cmp varg
        bcc *+5
        jmp l00768
        lda #17
        sta chary
        lda #1
        sta charx
        lda #10
        jsr dmsg
        lda #18
        sta chary
        lda #1
        sta charx
        lda #11
        jsr dmsg
        lda #19
        sta chary
        lda #1
        sta charx
        lda #12
        jsr dmsg
        lda #20
        sta chary
        lda #1
        sta charx
        lda #13
        jsr dmsg
        lda #21
        sta chary
        lda #1
        sta charx
        lda #14
        jsr dmsg
        lda #2
        sta vark
l00768: lda #6
        cmp scno
        beq *+5
        jmp l00923
        lda #1
        cmp vark
        beq *+5
        jmp l00923
        lda #12
        sta chary
        lda #25
        sta charx
        lda #3
        sta loopa
l00827: lda #2
        cmp varg
        bcc *+5
        jmp l00875
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        jmp l00901
l00875: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #25
        jsr pattr
l00901: dec loopa
        beq :+
        jmp l00827
:
        lda #2
        sta vark
l00923: lda #17
        cmp scno
        beq *+5
        jmp l01101
        lda #1
        cmp vark
        beq *+5
        jmp l01101
        lda #10
        sta varl
        lda #3
        sta loopa
l00973: lda varl
        sta chary
        lda #28
        sta charx
        lda #1
        cmp varg
        bcc *+5
        jmp l01038
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        jmp l01065
l01038: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #25
        jsr pattr
l01065: lda varl
        clc
        adc #1
        sta varl
        dec loopa
        beq :+
        jmp l00973
:
        lda #2
        sta vark
l01101: lda #9
        cmp scno
        beq *+5
        jmp l01256
        lda #1
        cmp vark
        beq *+5
        jmp l01256
        lda #22
        sta chary
        lda #13
        sta charx
        lda #3
        sta loopa
l01160: ; lda #3
        cmp varg
        bcc *+5
        jmp l01208
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        jmp l01235
l01208: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #25
        jsr pattr
l01235: dec loopa
        beq :+
        jmp l01160
:
        lda #2
        sta vark
l01256: lda #5
        cmp scno
        beq *+5
        jmp l01411
        lda #1
        cmp vark
        beq *+5
        jmp l01411
        lda #22
        sta chary
        lda #13
        sta charx
        lda #3
        sta loopa
l01315: lda #4
        cmp varg
        bcc *+5
        jmp l01363
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        jmp l01389
l01363: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #25
        jsr pattr
l01389: dec loopa
        beq :+
        jmp l01315
:
        lda #2
        sta vark
l01411: lda #18
        cmp scno
        beq *+5
        jmp l01453
        lda #5
        cmp varg
        beq *+5
        jmp l01453
        lda #6
        sta varg
l01453: lda #19
        cmp varg
        beq *+5
        jmp l01478
        lda #1
        sta gamwon
l01478: lda #16
        cmp varg
        beq *+5
        jmp l01520
        jsr cls
        lda #0
        sta scno
        jsr nwscr
        lda #17
        sta varg
l01520: rts
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
        lda #254
        sta varg
        lda #6
        sta varh
        lda #0
        sta varn
        ; lda #0
        sta varo
        ; lda #0
        sta varp
        ; lda #0
        sta varq
        ; lda #0
        sta varr
        ; lda #0
        sta vart
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
        sta vark
        ldy #8
        lda (z80_ix),y
        sta varo
        iny; ldy #9
        lda (z80_ix),y
        sta varp
        lda #100
        sta vars
        lda #23
        sta chary
        lda #5
        sta charx
        tya; lda #9
        jsr dmsg
        lda #23
        sta chary
        lda #9
        sta charx
        ; lda #9
        jsr dmsg
        lda #0
        sta chary
        lda #30
        sta charx
        lda #22
        jsr dmsg
        lda #1
        sta chary
        lda #30
        sta charx
        lda #6
        cmp varh
        beq *+5
        jmp o00220
        lda #18
        jsr dmsg
        lda #2
        sta chary
        lda #30
        sta charx
        lda #19
        jsr dmsg
        jmp o00253
o00220: lda #20
        jsr dmsg
        lda #2
        sta chary
        lda #30
        sta charx
        lda #21
        jsr dmsg
o00253: lda #4
        sta chary
        lda #30
        sta charx
        lda #4
        cmp varh
        bcc *+5
        jmp o00325
        lda #18
        jsr dmsg
        lda #5
        sta chary
        lda #30
        sta charx
        lda #19
        jsr dmsg
        jmp o00358
o00325: lda #20
        jsr dmsg
        lda #5
        sta chary
        lda #30
        sta charx
        lda #21
        jsr dmsg
o00358: lda #7
        sta chary
        lda #30
        sta charx
        lda #3
        cmp varh
        bcc *+5
        jmp o00430
        lda #18
        jsr dmsg
        lda #8
        sta chary
        lda #30
        sta charx
        lda #19
        jsr dmsg
        jmp o00463
o00430: lda #20
        jsr dmsg
        lda #8
        sta chary
        lda #30
        sta charx
        lda #21
        jsr dmsg
o00463: lda #10
        sta chary
        lda #30
        sta charx
        lda #2
        cmp varh
        bcc *+5
        jmp o00535
        lda #18
        jsr dmsg
        lda #11
        sta chary
        lda #30
        sta charx
        lda #19
        jsr dmsg
        jmp o00568
o00535: lda #20
        jsr dmsg
        lda #11
        sta chary
        lda #30
        sta charx
        lda #21
        jsr dmsg
o00568: lda #13
        sta chary
        lda #30
        sta charx
        lda #1
        cmp varh
        bcc *+5
        jmp o00640
        lda #18
        jsr dmsg
        lda #14
        sta chary
        lda #30
        sta charx
        lda #19
        jsr dmsg
        jmp o00674
o00640: lda #20
        jsr dmsg
        lda #14
        sta chary
        lda #30
        sta charx
        lda #21
        jsr dmsg
o00674: lda #16
        sta chary
        lda #30
        sta charx
        lda #0
        cmp varh
        bcc *+5
        jmp o00891
        lda #18
        jsr dmsg
        lda #17
        sta chary
        lda #30
        sta charx
        lda #19
        jsr dmsg
        lda #3
        sta varl
        lda #6
        sta loopa
o00757: lda varl
        sta chary
        lda #30
        sta charx
        lda #23
        jsr dmsg
        lda #3
        sta z80_c
        lda varl
        clc
        adc z80_c
        sta varl
        dec loopa
        beq :+
        jmp o00757
:
        lda #19
        sta varl
        lda #4
        sta loopa
o00837: lda varl
        sta chary
        lda #30
        sta charx
        lda #24
        jsr dmsg
        lda varl
        clc
        adc #1
        sta varl
        dec loopa
        beq :+
        jmp o00837
:
o00891: rts
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
        jmp r00398
        ; lda #0
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp r00056
        lda #255
        ; ldy #5
        sta (z80_ix),y
r00056: lda #1
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp r00092
        lda #255
        ; ldy #5
        sta (z80_ix),y
r00092: lda #2
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp r00128
        lda #255
        ; ldy #5
        sta (z80_ix),y
r00128: lda #4
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp r00164
        lda #255
        ; ldy #5
        sta (z80_ix),y
r00164: lda #5
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp r00201
        lda #255
        ; ldy #5
        sta (z80_ix),y
r00201: lda #6
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp r00237
        lda #255
        ; ldy #5
        sta (z80_ix),y
r00237: lda #7
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp r00273
        lda #255
        ; ldy #5
        sta (z80_ix),y
r00273: jsr cls
        lda #7
        sta chary
        lda #4
        sta charx
        lda #27
        jsr dmsg
        lda #9
        sta chary
        lda #5
        sta charx
        lda #28
        jsr dmsg
        lda #11
        sta chary
        lda #5
        sta charx
        lda #29
        jsr dmsg
        lda #13
        sta chary
        lda #3
        sta charx
        lda #30
        jsr dmsg
        lda #5
        jsr delay
        jsr prskey
        lda #1
        sta gamwon
r00398: rts
.out .sprintf("EVENT 17 SIZE = %d", (* - evnt17))

evnt18:
        lda #5
        jsr delay
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
        .byte 39
        .byte "A GREAT EVIL",141
        .byte "HAS INVADED OUR",141
        .byte "PRECIOUS FOREST.",39
        .byte 141
        .byte 39
        .byte "FIND ALL THE",141
        .byte " RUNESTONES TO",141
        .byte "  DRIVE IT OUT.",39
        .byte 141
        .byte 39
        .byte "IF YOU ARE",141
        .byte "HURT, MY LOVE",141
        .byte "WILL HEAL YOU.",39
        .byte 141
        .byte "*****************",141
        .byte "abc",141
        .byte "dhe",141
        .byte "die",141
        .byte "dje",141
        .byte "fkg",141
        .byte 39
        .byte "PATHETIC WOMAN! DID YOU",141
        .byte "REALLY THINK YOUR FEEBLE",141
        .byte "MAGIC COULD DESTROY ME?",39
        .byte 141
        .byte "lm",141
        .byte "no",141
        .byte "pq",141
        .byte "rs",141
        .byte "vw",141
        .byte "tu",141
        .byte "xy",141
        .byte "S O R C E R E S S  2",141
        .byte "THE MYSTIC FOREST",141
        .byte "THE SORCERESS IS DEAD.",141
        .byte "THE EVIL WILL SPREAD",141
        .byte "BEYOND THE FOREST TO",141
        .byte "CLAIM THE WHOLE WORLD...",141
        .byte 39
        .byte "NO! NOOO! NOOOOOOOO!",39
        .byte 141
        .byte "PRESS SPACE",141
        .byte 39
        .byte "THE NECROMANCER IS",141
        .byte "DEAD. THE MYSTIC",141
        .byte "FOREST IS SAFE!",39
        .byte 141
        .byte "A NEW PATH IS OPEN...",141
nummsg:
        .byte 37
.out .sprintf("MESSAGE SIZE = %d", (* - msgdat))

chgfx:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 161,36,88,80,5,5,88,74,5,5,26,10,5,4,0,0
        .byte 4,0,2,64,32,32,10,10,36,161,80,80,161,5,26,66
        .byte 80,112,80,96,240,96,224,208,224,240,112,208,96,176,96,224
        .byte 160,160,176,80,176,160,208,64,208,160,176,192,112,96,176,160
        .byte 80,112,80,96,240,96,96,208,96,176,112,176,224,224,128,64
        .byte 160,160,176,112,176,160,208,64,224,224,208,160,112,96,48,48
        .byte 160,164,82,82,4,164,26,26,32,133,16,26,32,36,0,16
        .byte 164,133,26,66,4,160,10,8,133,4,10,8,32,4,8,0
        .byte 0,1,2,82,1,161,18,74,161,133,80,90,37,36,74,88
        .byte 129,0,74,0,133,160,10,72,133,4,90,10,161,4,88,26
        .byte 133,37,90,88,161,165,74,74,37,33,88,88,164,32,10,88
        .byte 161,133,88,82,37,129,10,74,37,133,72,8,161,5,88,2
        .byte 160,160,90,88,165,165,82,26,165,164,74,90,37,161,88,88
        .byte 161,165,88,88,37,37,26,72,129,5,10,10,5,129,64,64
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 246,254,247,245,248,254,247,247,247,251,243,250,247,251,240,247
        .byte 251,246,246,252,250,254,254,246,247,249,255,246,251,246,255,248
        .byte 251,251,254,255,255,252,252,247,255,253,247,255,250,243,255,254
        .byte 255,255,159,47,159,63,255,239,159,63,159,47,159,63,255,239
        .byte 0,17,0,114,17,100,34,250,17,213,114,216,100,177,216,250
        .byte 128,0,234,0,100,128,250,0,85,196,234,216,160,228,186,114
        .byte 213,213,186,114,177,85,234,234,177,177,170,216,100,245,216,216
        .byte 242,242,245,245,250,250,253,247,255,253,247,255,250,243,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 160,32,24,2,5,36,80,26,37,160,80,66,36,161,0,0
        .byte 5,5,10,10,133,133,82,82,37,37,2,2,32,32,2,2
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 0,4,66,8,133,33,16,72,4,129,10,10,32,160,16,72
        .byte 64,144,48,224,224,96,64,48,80,32,224,96,48,208,80,64
        .byte 68,160,50,200,160,100,64,186,68,32,234,98,49,213,80,64
        .byte 0,128,2,72,5,160,16,0,1,160,64,88,160,128,16,72
        .byte 0,0,36,4,88,10,161,1,66,66,161,1,72,10,36,4
        .byte 33,0,2,8,32,0,2,16,129,33,72,24,133,160,66,72
        .byte 76,70,38,141,19,0,1,38,68,17,38,140,76,76,1,38
        .byte 165,225,90,240,225,240,240,120,240,165,120,120,165,180,90,120
        .byte 225,225,90,240,225,225,240,90,240,225,120,240,165,240,120,120
        .byte 0,7,3,15,7,15,7,15,7,15,7,15,15,15,15,15
        .byte 13,8,15,12,15,14,15,14,15,15,15,15,15,15,15,15
        .byte 15,15,15,14,15,14,15,14,15,14,15,12,15,8,8,0
        .byte 15,15,15,15,15,15,7,15,15,15,15,15,7,15,3,3
        .byte 12,1,14,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,8,3
        .byte 15,15,15,15,15,15,15,143,15,15,15,15,31,15,15,15
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 15,15,15,15,15,79,15,175,15,15,47,15,31,15,15,15
        .byte 4,160,2,10,160,132,24,8,5,160,82,72,1,128,18,72
        .byte 0,128,16,200,85,196,216,152,213,177,200,186,100,177,64,170
        .byte 136,34,34,85,119,34,34,0,0,136,68,34,170,119,68,34
        .byte 4,5,18,18,1,133,0,80,161,33,66,88,1,133,0,18
        .byte 160,32,72,72,165,128,10,0,160,133,90,66,161,128,72,0
        .byte 0,4,16,66,0,160,64,88,32,37,18,18,129,161,64,88
        .byte 128,128,88,8,133,32,26,64,36,4,72,72,133,129,90,2
        .byte 37,37,18,74,1,161,18,88,36,164,82,26,5,36,74,88
        .byte 15,165,15,15,165,135,30,15,15,45,90,75,15,15,15,15
        .byte 15,135,30,203,95,199,218,158,215,181,203,190,109,181,75,175
        .byte 96,240,112,160,80,208,0,0,176,96,224,112,208,48,0,0
        .byte 19,87,5,170,87,87,171,175,95,95,5,175,27,95,175,175
        .byte 93,78,38,171,95,78,175,175,95,95,140,35,0,0,35,140
        .byte 78,8,1,38,78,25,175,170,95,93,175,141,95,93,175,170
        .byte 27,95,175,175,27,95,5,175,27,95,39,175,95,95,171,175
        .byte 95,93,175,141,95,78,175,141,95,95,175,141,95,10,175,141
        .byte 27,95,39,175,27,95,175,141,27,95,39,171,10,93,140,170
        .byte 95,93,175,175,78,78,175,141,27,78,141,175,78,10,4,4
        .byte 95,95,136,1,0,0,35,140,95,95,175,175,25,19,137,137
        .byte 25,95,137,141,76,17,174,35,95,95,175,175,95,19,175,35
        .byte 78,87,174,4,76,76,140,140,25,8,137,137,95,25,140,35
        .byte 78,19,175,175,95,95,175,175,27,78,141,171,27,87,34,170
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 161,36,88,80,5,5,26,90,36,36,88,80,4,5,80,66
        .byte 161,36,88,80,5,5,90,10,4,133,88,66,5,36,64,72
        .byte 119,204,255,238,187,170,153,34,255,238,102,204,119,204,85,68
        .byte 133,165,82,66,5,165,18,88,37,37,88,74,36,165,26,26
        .byte 80,112,112,112,240,176,224,160,96,240,208,208,224,176,96,224
        .byte 128,0,240,0,80,240,96,192,48,240,0,112,0,16,0,0
        .byte 160,160,176,80,176,176,208,80,208,160,176,208,112,96,176,160
        .byte 0,16,48,96,240,192,176,128,96,0,192,0,0,0,0,0
        .byte 80,80,160,160,112,112,224,208,224,240,112,208,96,176,96,224
        .byte 0,88,160,33,80,88,0,37,26,18,36,133,16,90,0,33
        .byte 26,0,164,1,26,74,4,132,10,0,165,37,26,72,132,0
        .byte 0,64,32,32,16,2,32,133,8,82,36,33,18,18,1,133
        .byte 64,2,164,132,80,8,161,1,82,2,132,36,80,72,165,128
        .byte 0,213,170,50,85,213,0,114,177,49,98,216,17,245,0,50
        .byte 177,0,234,16,177,228,64,200,160,0,250,114,177,196,200,0
        .byte 0,68,34,34,17,32,34,216,128,117,98,50,49,49,16,216
        .byte 68,32,234,200,85,128,186,16,117,32,200,98,85,196,250,136
        .byte 0,8,1,140,85,76,141,137,93,27,140,171,70,27,4,170
        .byte 0,187,51,238,51,238,85,153,255,221,238,34,68,68,204,238
        .byte 204,0,119,68,187,170,238,102,136,170,85,153,17,34,119,17
        .byte 221,204,153,153,187,17,102,51,238,221,119,187,85,85,34,170
        .byte 255,187,102,85,136,238,51,85,102,187,68,85,68,170,187,68
bprop:
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
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
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
        .byte 5
        .byte 1
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
        .byte 2
.out .sprintf("BLOCKS SIZE = %d", (* - chgfx))

sprgfx:
        .byte 0,160,1,80,2,224,5,192,10,240,21,96,42,0,85,64,10,168,20,160,33,16,194,128,130,160,156,160,97,0,1,128
        .byte 0,160,1,80,2,224,5,192,10,240,85,96,42,0,21,64,10,164,52,152,65,64,70,48,137,8,178,48,196,64,2,0
        .byte 5,0,10,128,7,64,3,160,15,80,6,168,0,84,2,170,21,80,5,40,8,132,1,67,5,65,5,57,0,134,1,128
        .byte 5,0,10,128,7,64,3,160,15,80,6,170,0,84,2,168,37,80,25,44,2,130,12,98,16,145,12,77,2,35,0,64
        .byte 2,128,5,64,11,160,21,80,11,160,17,16,40,40,23,208,42,168,73,36,83,148,150,210,132,66,178,154,194,134,6,192
        .byte 0,0,0,0,0,0,0,0,8,16,3,192,4,32,5,160,5,160,4,32,3,192,8,16,0,0,0,0,0,0,0,0
        .byte 128,1,0,0,32,4,19,200,4,32,9,144,19,200,22,104,22,104,19,200,9,144,4,32,19,200,32,4,0,0,128,1
        .byte 0,0,7,0,56,224,66,16,80,136,160,72,213,8,234,150,117,177,63,73,78,198,147,229,36,152,73,12,40,192,6,56
        .byte 15,128,48,96,66,16,64,136,144,36,160,4,213,8,106,150,117,177,63,73,206,198,19,229,100,153,137,6,145,32,12,192
        .byte 0,0,0,224,7,28,8,66,17,10,18,5,16,171,105,87,141,174,146,252,99,114,167,201,25,36,48,146,3,20,28,96
        .byte 1,240,6,12,8,66,17,2,36,9,32,5,16,171,105,86,141,174,146,252,99,115,167,200,153,38,96,145,4,137,3,48
        .byte 0,0,0,8,36,28,14,60,15,24,7,186,33,240,121,224,63,232,15,248,25,220,51,72,70,102,28,224,24,68,64,0
        .byte 0,4,152,78,60,222,62,252,31,253,15,254,110,222,255,15,62,30,29,188,159,222,63,252,127,249,255,252,156,230,56,67
        .byte 112,102,248,239,237,187,119,242,17,194,24,135,108,14,248,3,60,1,24,1,144,15,48,2,96,33,226,50,175,111,57,195
        .byte 6,96,3,48,4,232,6,78,15,255,31,213,59,170,124,254,255,188,223,80,238,182,241,100,127,240,190,248,31,116,29,0
        .byte 1,152,3,48,12,232,14,78,31,255,63,213,123,128,123,192,253,234,223,254,238,252,245,97,122,230,189,240,22,120,0,58
        .byte 6,96,12,192,23,32,114,96,255,240,171,248,85,220,127,62,61,255,10,251,109,119,38,143,15,254,31,125,46,248,0,184
        .byte 25,128,12,192,23,48,114,112,255,248,171,252,1,222,3,222,87,191,127,251,63,119,134,175,103,94,15,189,30,104,92,0
        .byte 16,16,16,16,73,36,135,194,169,42,73,36,54,216,40,40,100,76,112,28,239,238,241,30,95,244,99,140,63,248,10,160
        .byte 4,64,8,32,9,32,103,204,137,34,169,42,118,220,36,72,114,156,120,60,239,238,247,222,89,52,71,196,63,248,10,160
        .byte 17,4,128,64,8,1,33,20,0,64,18,8,0,162,68,0,1,80,8,2,130,128,40,32,1,4,0,64,16,17,130,0
        .byte 6,192,8,32,11,160,17,16,19,144,33,8,32,8,70,196,43,168,9,32,18,144,7,192,7,192,15,224,15,224,21,80
        .byte 16,0,40,129,36,66,82,102,81,62,137,44,196,190,161,245,151,229,135,240,95,172,79,64,30,136,181,16,65,0,162,128
        .byte 0,0,0,66,0,66,48,102,76,62,83,44,72,190,164,117,146,229,80,240,75,168,33,68,24,144,87,8,32,192,81,0
        .byte 0,8,129,20,66,36,102,74,124,138,52,145,125,35,175,133,167,233,15,225,53,250,2,242,17,120,8,173,0,130,1,69
        .byte 0,0,66,0,66,0,102,12,124,50,52,202,125,18,174,37,167,73,15,10,21,210,34,132,9,24,16,234,3,4,0,138
        .byte 0,0,32,8,80,20,72,36,139,162,151,210,165,74,198,198,131,130,66,132,0,0,0,0,2,128,1,0,0,0,0,0
        .byte 0,0,0,0,0,0,48,24,75,164,151,210,149,82,166,202,163,138,146,146,80,20,50,152,9,32,0,0,0,0,0,0
        .byte 4,0,3,64,27,144,33,32,82,144,17,8,32,16,36,72,42,168,72,36,84,4,64,4,64,8,128,4,128,2,170,170
        .byte 4,0,3,64,28,112,62,224,93,112,30,248,63,240,59,184,53,88,119,220,107,252,127,252,127,248,255,252,255,254,213,86
frmlst:
        .byte 0,2
        .byte 2,2
        .byte 4,1
        .byte 5,2
        .byte 7,2
        .byte 9,2
        .byte 11,3
        .byte 14,2
        .byte 16,2
        .byte 18,2
        .byte 20,1
        .byte 21,1
        .byte 22,2
        .byte 24,2
        .byte 26,2
        .byte 28,2,30,0
.out .sprintf("SPRITES SIZE = %d", (* - sprgfx))

scdat:
        .word 366,417,429,381,413,227,308,275,323,395,337,321,316,315,373,363,362,338,309
        .byte 0,9,11,13,10,0,1,1,255,2,4,1,0,0,0,255,1,5,0,7,11,36,11,13,12,11,9,36,11,13,12,10,0,0
        .byte 255,1,4,255,0,11,11,13,14,35,13,11,53,14,13,12,14,14,255,0,17,7,36,13,11,13,14,11,13,11,13,35,12
        .byte 255,0,18,7,12,8,7,12,7,11,14,12,14,8,255,0,10,16,17,255,0,10,77,0,7,14,12,8,255,0,4,20,255,22,9
        .byte 21,255,0,7,74,73,0,0,77,75,76,0,0,0,20,255,22,11,21,255,0,7,3,0,0,3,4,255,0,5,16,255,23,9
        .byte 17,255,0,4,30,0,0,74,73,34,0,3,4,255,0,5,16,18,24,24,18,18,19,18,19,18,17,255,0,4,31,0,29
        .byte 0,3,33,0,5,6,255,0,5,16,18,24,24,255,18,6,17,255,0,6,33,0,5,255,0,9,16,18,0,0,255,18,6
        .byte 17,255,0,67,9,13,13,10,255,0,22,30,0,34,11,11,12,12,10,255,0,19,32,0,28,0,31,11,12,35,14,12,255,0,19
        .byte 33,255,0,4,12,12,11,12,12,255,0,24,36,14,12,35,8,255,0,17,47,37,43,44,41,44,38,7,12,14,8,255,0,7
        .byte 30,255,0,6,48,0,0,37,54,255,44,4,43,44,0,77,4,255,0,6,29,0,28,255,0,4,29,0,0,37,43,44,44,44
        .byte 43,45,44,42,39,74,73,4,0,0,48,0,0,0,31,255,0,6,31,0,0,40,44,44,45,44,42,55,39,0,0,0,3
        .byte 4,255,0,9,32,255,0,12,46,0,2,2,2,5,6,255,2,8,0,33,0,0,0,255,2,11,69,70
        .byte 36,11,11,12,11,11,11,14,11,12,8,0,84,85,0,7,11,13,12,12,8,0,7,11,36,14,11,11,14,11,11,36,11,11
        .byte 12,35,14,8,0,0,0,82,83,0,0,0,77,75,76,0,0,0,7,11,11,12,12,11,0,7,8,14,12,14,8,255,0,10
        .byte 3,4,255,0,6,7,11,35,14,0,0,0,77,4,255,0,12,5,6,255,0,7,7,11,12,0,0,74,73,4,255,0,23
        .byte 77,0,0,0,3,75,76,255,0,22,3,0,0,0,3,4,255,0,22,74,73,0,0,0,5,6,0,51,52,255,0,8,37
        .byte 41,41,38,255,0,8,3,255,0,6,49,50,255,0,5,37,41,41,44,45,44,44,41,38,255,0,6,3,0,57,58,59,68
        .byte 255,0,7,47,43,255,44,8,45,38,255,0,4,5,0,60,64,61,68,255,0,6,37,54,255,44,7,45,44,44,44,255,0,6
        .byte 60,65,61,68,255,0,6,44,43,44,44,44,45,43,44,44,44,43,42,39,255,0,6,60,66,61,68,255,0,6,40,255,44,7
        .byte 42,39,255,0,9,62,67,63,68,255,0,7,255,44,5,42,39,0,86,255,0,6,9,11,255,0,5,80,81,255,0,5,40
        .byte 42,45,55,39,0,47,0,46,255,0,5,9,11,11,9,13,11,10,0,78,79,255,0,8,46,0,0,46,255,0,7,11,13
        .byte 12,13,11,11,14,10,255,0,6,51,52,255,0,12,9,11,11,13,11,14,11,12,12,255,0,6,49,50,255,0,12,7,36
        .byte 12,11,36,11,14,13,12,10,255,0,5,49,50,255,0,13,7,11,12,11,14,14,35,12,14,255,0,22,77,8,77,4,7
        .byte 12,8,255,0,12,255,2,4,255,0,6,3,0,3,4,255,0,12,2,2,2,255,1,4,2,2,255,0,4,3,0,5,6
        .byte 0,255,2,11,1,1,1,255,0,4,1,1,2,2,2,0,5
        .byte 11,13,13,12,12,14,12,0,255,1,5,0,0,0,9,13,11,10,0,1,1,1,0,49,50,84,85,12,14,12,11,14,14
        .byte 8,255,0,9,11,12,11,14,255,0,7,82,83,14,11,14,12,12,8,255,0,10,72,13,14,12,10,255,0,8,13,12,14
        .byte 8,255,0,11,9,11,11,13,14,12,255,0,7,51,12,8,8,255,0,12,13,14,11,14,12,14,255,0,7,49,4,255,0,14
        .byte 7,12,12,12,14,8,255,0,5,51,52,49,75,76,255,0,6,9,13,11,10,255,0,4,7,12,14,8,255,0,6,49,50
        .byte 49,4,255,0,6,9,11,11,14,11,10,255,0,4,77,4,255,0,7,49,50,0,4,255,0,5,9,72,11,13,11,14,11
        .byte 10,0,32,0,3,4,255,0,5,68,255,0,4,75,76,255,0,4,72,12,11,12,14,13,11,14,0,31,0,5,6,255,0,5
        .byte 68,57,58,59,0,4,255,0,5,7,11,13,14,11,12,14,8,255,0,10,68,60,64,61,0,6,255,0,6,7,14,12,12
        .byte 14,8,0,0,0,30,255,0,7,68,60,65,61,255,0,9,7,12,14,8,0,0,34,0,28,255,0,7,68,60,66,61,0
        .byte 13,10,255,0,6,74,77,4,0,0,0,33,255,0,9,68,62,67,63,0,14,13,10,255,0,6,3,75,76,255,0,12,68
        .byte 255,0,4,11,12,14,255,0,6,3,4,255,0,15,9,11,13,14,11,12,10,255,0,5,5,6,255,0,14,9,13,11,12,12,12
        .byte 14,12,255,0,20,9,72,13,11,14,13,14,12,8,255,0,20,72,11,11,12,11,8,14,8,255,0,21,7,14,12,11,14
        .byte 4,255,0,13,47,0,47,37,41,44,38,255,0,5,7,8,77,4,255,0,10,37,41,44,54,41,54,44,44,45,44,41,43
        .byte 44,38,0,0,0,3,6,0,2,2,2,0,37,41,44,43,41,44,44,43,255,44,9,45,44,41,38,0,3
        .byte 13,14,11,14,11,14,8,0,255,1,5,255,2,8,1,1,1,255,0,5,12,11,13,12,14,8,255,0,7,255,1,8,0,0,0
        .byte 68,57,58,59,0,35,14,12,8,255,0,20,68,60,64,61,0,14,8,255,0,22,68,60,65,61,0,4,255,0,23,68,60
        .byte 66,61,0,4,255,0,23,68,62,67,63,0,4,255,0,14,51,52,0,0,34,255,0,9,4,255,0,7,255,2,4,0,0,0
        .byte 49,50,0,2,33,2,2,0,0,9,13,11,10,0,4,0,0,0,255,2,4,255,1,4,0,80,81,49,50,0,255,1,4
        .byte 0,9,11,11,11,12,10,4,0,0,0,255,1,4,255,0,5,78,79,49,50,255,0,6,72,11,12,11,14,12,4,255,0,12
        .byte 78,79,255,0,7,9,13,11,11,12,12,14,4,0,0,0,84,85,255,0,16,11,36,12,13,12,12,35,6,0,0,0,82
        .byte 83,255,0,16,7,11,11,12,35,14,12,255,0,4,82,83,255,0,18,7,11,12,8,0,52,255,0,8,9,13,11,10,255,0,12
        .byte 77,4,0,0,50,255,0,7,9,11,13,14,12,10,255,0,10,74,73,4,0,0,50,255,0,7,72,36,14,12,14,14,255,0,11
        .byte 3,75,76,0,50,51,52,255,0,5,7,14,13,14,12,8,255,0,11,3,4,0,32,50,49,50,255,0,6,7,14,12,8
        .byte 255,0,11,74,73,4,0,33,50,49,50,255,0,7,77,4,255,0,13,3,4,0,0,0,49,50,255,0,7,3,4,255,0,13
        .byte 5,6,0,0,255,2,6,0,30,0,0,3,4,255,0,6,87,88,255,0,9,255,1,6,2,31,2,2,5,6,2,0,0,0
        .byte 2,2,89,90,255,2,8,0
        .byte 52,255,1,10,51,52,0,0,0,255,1,4,255,2,4,1,1,1,49,50,50,255,0,10,49,50,255,0,7,255,1,4,0,0,0
        .byte 49,50,50,255,0,10,49,50,255,0,14,49,50,50,0,0,0,84,85,255,0,5,49,50,255,0,20,82,83,255,0,22,84
        .byte 52,0,0,51,52,255,0,5,51,52,255,0,7,47,255,0,8,82,50,0,0,49,50,255,0,5,49,50,255,0,5,37,41
        .byte 54,41,38,255,0,6,82,50,0,0,49,50,255,0,5,49,50,37,255,41,4,255,44,5,41,45,38,0,0,0,82,50,0,0
        .byte 49,50,255,0,7,45,255,44,7,45,255,44,4,255,0,4,50,255,0,4,80,81,255,0,5,40,42,44,43,44,43,255,44,6
        .byte 55,0,0,0,51,255,0,5,78,79,51,52,255,0,5,40,42,255,44,8,39,0,0,0,49,47,255,0,6,49,50,255,0,7
        .byte 40,42,42,255,44,5,255,0,4,49,54,41,38,255,0,6,47,255,0,9,40,42,45,55,39,255,0,4,49,44,45,44,41
        .byte 38,255,0,4,46,32,255,0,5,51,52,255,0,4,46,255,0,6,255,44,5,38,255,0,4,31,86,255,0,4,49,50,255,0,5
        .byte 47,255,0,4,51,42,44,44,43,42,39,255,0,5,46,255,0,4,49,50,29,255,0,4,46,34,0,0,0,49,255,0,18
        .byte 28,255,0,5,33,0,0,0,49,0,57,58,59,68,255,0,5,51,52,255,0,16,49,0,60,64,61,68,255,0,5,49,50
        .byte 255,0,16,49,0,60,65,61,68,255,0,5,49,50,255,0,16,49,0,60,66,61,68,255,0,4,51,52,80,81,255,0,7
        .byte 84,85,255,0,8,62,67,63,68,255,0,4,49,50,78,79,255,0,7,82,83,255,0,5,80,81,255,2,9,49,50,78,79
        .byte 255,2,7,82,83,255,2,5,78,79
        .byte 11,35,11,12,8,0,1,1,1,255,2,4,0,0,0,255,1,4,2,2,2,1,0,9,11,13,11,12,14,8,255,0,6
        .byte 255,1,4,255,0,7,1,1,1,0,9,11,13,11,14,4,255,0,23,72,36,11,12,11,4,255,0,23,72,12,11,11,14
        .byte 4,255,0,23,7,13,12,13,35,6,255,0,24,7,14,12,14,255,0,28,77,51,52,255,0,25,74,73,49,50,255,0,26
        .byte 3,49,50,255,0,26,5,255,0,87,11,10,255,0,26,51,11,14,255,0,26,49,36,11,10,255,0,25,49,11,12,11,10
        .byte 255,0,25,14,11,35,8,255,0,23,51,52,12,14,8,255,0,24,49,50,75,76,0,0,84,85,255,0,21,49,50,4,0,0,0
        .byte 82,83,0,29,255,0,16,32,255,0,4,4,0,0,0,82,83,2,33,2,2,2,87,88,0,0,0,87,88,0,34,0,0
        .byte 2,2,31,255,2,4,6,255,2,4,255,1,6,89,90,0,0,0,89,90,2,28,2,2,255,1,7
        .byte 11,12,14,8,0,7,11,12,11,14,12,35,12,12,8,0,40,43,44,55,44,45,42,255,44,5,43,4,255,0,6,7,14
        .byte 11,12,14,8,255,0,6,46,0,0,0,40,42,45,44,44,44,4,255,0,8,77,4,255,0,15,40,55,44,6,255,0,8
        .byte 3,4,255,0,16,46,40,255,0,9,5,6,255,0,45,51,52,255,0,27,49,50,51,52,255,0,25,49,50,49,50,255,0,5
        .byte 86,255,0,19,49,50,41,44,41,38,0,0,47,46,47,255,0,5,47,0,47,255,0,4,47,255,0,7,43,44,44,44,41
        .byte 44,54,44,54,41,41,44,43,41,54,38,46,86,0,0,0,46,255,0,6,51,255,44,7,43,44,45,255,44,6,41,54,44
        .byte 41,41,44,41,41,38,0,0,0,49,44,44,42,45,44,42,55,42,255,44,5,45,45,44,44,44,45,44,42,43,42,55,39
        .byte 0,0,0,49,255,0,6,46,0,40,43,42,44,44,44,42,42,43,42,44,39,0,0,0,46,255,0,4,49,84,85,255,0,27
        .byte 82,83,255,0,26,9,255,0,27,9,12,255,0,26,9,13,11,255,0,26,11,12,11,52,255,0,25,7,11,13,50,0,80
        .byte 81,255,0,23,7,14,50,0,78,79,0,51,52,255,0,9,255,2,7,255,0,5,77,50,0,78,79,0,49,50,0,255,2,8
        .byte 255,1,7,255,2,4,0,3
        .byte 11,13,12,13,10,0,255,1,7,0,0,0,255,1,11,2,0,12,12,35,12,8,255,0,22,1,0,13,12,12,8,255,0,24
        .byte 2,12,14,8,255,0,25,1,4,255,0,28,75,76,255,0,27,4,255,0,28,6,0,86,255,0,24,51,52,0,47,46,37
        .byte 41,43,44,44,41,44,38,255,0,16,49,50,37,54,41,255,44,4,45,44,44,44,41,44,38,0,47,0,47,0,0,47,0,0,0
        .byte 37,41,44,45,41,44,44,44,45,255,44,6,43,43,44,44,38,46,86,46,37,41,54,41,41,43,255,44,5,45,255,44,14
        .byte 41,54,41,255,44,8,43,44,43,42,44,43,255,44,4,45,255,44,5,45,44,44,42,45,42,55,44,42,44,43,44,44,44
        .byte 42,42,0,40,44,42,255,44,8,42,44,39,255,0,4,46,0,0,40,44,42,45,39,51,52,255,0,4,40,44,42,55,42
        .byte 45,39,255,0,16,49,50,11,10,255,0,5,46,255,0,19,49,50,12,13,10,255,0,26,36,12,12,255,0,26,12,12,8
        .byte 255,0,26,12,8,255,0,26,51,4,255,0,25,80,81,49,4,255,0,23,84,85,78,79,49,6,255,2,12,0,0,0,255,2,7
        .byte 0,82,83,78,79,49
        .byte 13,14,12,8,0,255,1,4,255,2,8,255,1,4,0,7,13,13,11,12,13,11,12,35,8,255,0,6,255,1,8,255,0,6
        .byte 7,13,36,11,14,12,8,8,255,0,23,7,14,7,14,75,76,255,0,26,77,4,255,0,27,3,4,255,0,26,74,73,75
        .byte 76,255,0,26,3,4,255,0,27,3,4,255,0,27,3,4,255,0,4,87,88,255,0,10,9,11,13,10,255,0,7,5,4
        .byte 255,0,4,89,90,255,0,5,2,2,0,0,9,11,13,14,12,10,255,0,7,4,255,0,5,87,88,0,84,85,0,1,1
        .byte 0,9,12,13,11,12,12,14,10,255,0,6,4,255,0,5,89,90,0,82,83,255,0,4,11,13,36,12,14,35,14,8,255,0,6
        .byte 4,255,0,8,82,83,255,0,4,11,12,12,11,12,14,10,14,255,0,4,51,52,4,255,0,14,7,11,11,35,14,12,12
        .byte 8,255,0,4,49,50,75,76,255,0,14,7,8,14,14,8,8,255,0,5,49,50,4,255,0,17,77,75,76,255,0,8,4
        .byte 255,0,17,3,4,255,0,7,87,88,6,255,0,16,74,73,4,255,0,7,89,90,255,0,18,3,4,255,0,9,2,255,0,17
        .byte 3,4,0,48,255,0,5,2,2,1,2,2,255,0,5,34,0,0,0,32,255,0,5,5,6,0,0,0,255,2,4,1,1
        .byte 0,1,1,255,2,5,33,2,2,2,28,0,0,0,255,2,7,255,1,4,0,0
        .byte 255,1,7,0,78,79,0,51,52,0,0,0,255,1,5,0,7,11,36,12,11,11,12,255,0,8,78,79,0,49,50,255,0,10
        .byte 7,11,11,12,14,11,51,52,255,0,9,49,50,255,0,12,7,11,12,14,49,50,255,0,7,51,52,255,0,16,74,77,255,0,9
        .byte 49,50,51,52,255,0,15,3,52,0,0,0,34,255,0,6,49,50,255,0,15,3,50,0,0,0,33,255,0,6,49,50,0,0,0
        .byte 87,88,0,51,52,255,0,7,3,50,255,0,10,49,50,0,51,52,89,90,29,49,50,0,84,85,255,0,4,3,50,0,0,0
        .byte 84,85,255,0,8,49,50,255,0,6,82,83,255,0,4,5,255,0,4,82,83,255,0,23,52,0,0,0,82,83,255,0,4
        .byte 51,52,255,0,14,47,37,43,50,0,0,51,52,0,0,51,52,0,49,50,30,51,52,80,81,0,51,52,0,37,43,41,44,44
        .byte 54,44,44,50,0,0,49,50,0,0,49,50,255,0,4,49,50,78,79,0,49,50,0,40,44,42,255,44,4,45,50,0,0
        .byte 49,50,0,0,49,50,255,0,15,40,45,42,44,44,255,0,4,80,81,255,0,21,40,42,52,0,0,0,78,79,255,0,5
        .byte 51,52,0,48,255,0,14,50,255,0,10,49,50,0,0,0,51,52,255,0,10,51,50,0,0,87,88,255,0,11,49,50,0,0
        .byte 51,52,255,0,6,49,0,0,0,89,90,255,0,5,51,52,255,0,4,49,50,84,85,49,50,0,48,255,0,5,32,255,0,9
        .byte 49,50,255,0,6,82,83,255,0,7,51,52,33,255,0,7,80,81,255,0,17,49,50,255,0,8,78,79,0,51,52,255,0,12
        .byte 30,0,49,50,255,2,8,78,79,0,49,50,0,0,0,255,2,9,31,2,2,2
        .byte 36,11,12,13,12,14,8,0,1,2,2,2,1,0,0,0,255,1,4,0,7,14,13,11,12,13,11,14,13,12,35,14,8
        .byte 255,0,4,1,1,1,255,0,10,7,11,14,11,14,14,11,12,14,8,255,0,20,7,36,14,12,11,35,75,76,255,0,24
        .byte 7,14,12,4,255,0,26,74,77,75,76,255,0,26,3,4,255,0,27,3,4,255,0,14,9,13,10,9,11,10,255,0,7
        .byte 3,4,255,0,6,9,11,13,10,0,0,0,9,36,11,11,12,11,14,255,0,6,74,73,4,255,0,5,9,11,13,11,12
        .byte 10,0,0,72,13,11,12,12,11,12,10,255,0,6,3,4,255,0,5,11,13,11,14,12,12,0,9,11,11,12,13,12,12
        .byte 35,12,10,255,0,5,3,4,255,0,5,7,11,12,13,12,8,0,7,13,12,14,12,12,14,12,12,14,255,0,5,3,4
        .byte 255,0,6,7,11,14,8,0,0,0,72,13,11,14,12,14,12,12,8,255,0,5,3,4,255,0,7,77,4,255,0,4,7
        .byte 36,12,12,35,12,12,8,255,0,6,5,75,76,255,0,6,3,4,255,0,5,7,8,12,14,14,8,255,0,8,4,255,0,7
        .byte 5,6,255,0,7,77,4,255,0,8,87,88,4,255,0,16,3,4,255,0,8,89,90,4,255,0,16,5,6,255,0,10,6
        .byte 255,0,27,29,0,48,255,0,26,28,2,2,255,0,27,1,1,2,255,0,23,2,2,2,0,0,1,255,2,10,0,0,0
        .byte 255,2,10,1,1,1
        .byte 13,12,14,11,12,14,0,255,1,5,255,2,8,0,7,11,11,12,11,11,11,12,12,11,14,35,14,8,255,0,6,255,1,8
        .byte 0,0,7,11,36,12,14,11,35,35,12,14,14,8,255,0,18,7,11,14,11,35,14,14,12,8,255,0,22,7,8,12,14
        .byte 4,255,0,27,77,75,76,255,0,26,3,4,255,0,26,74,73,4,255,0,18,84,85,255,0,7,3,4,255,0,16,51,52
        .byte 82,83,255,0,7,3,6,255,0,16,49,50,255,0,8,48,3,255,0,10,9,13,11,10,0,0,0,49,50,255,0,9,5
        .byte 255,0,8,9,11,36,11,11,13,11,10,255,0,20,9,13,11,13,11,12,35,11,12,10,0,48,255,0,8,9,11,52,255,0,6
        .byte 72,13,12,11,12,11,14,14,13,12,255,0,9,9,11,14,50,255,0,6,7,36,11,11,14,11,12,11,12,8,255,0,9
        .byte 72,36,11,50,255,0,7,7,8,14,12,14,35,14,8,255,0,9,9,11,12,11,255,0,11,77,75,76,255,0,11,7,11,11
        .byte 35,51,52,255,0,9,3,4,255,0,13,7,8,14,49,50,255,0,8,74,73,4,255,0,15,77,49,50,255,0,9,3,4
        .byte 255,0,13,48,0,3,52,51,52,255,0,8,5,6,255,0,12,30,0,0,3,50,49,50,255,0,7,32,255,0,10,255,2,4
        .byte 28,2,2,5,255,2,10,33,2,2,0,0,0,255,2,5,255,1,8
        .byte 13,12,11,12,8,0,255,1,5,2,2,0,0,0,2,2,1,1,1,2,2,1,0,7,11,13,12,12,35,12,8,255,0,7
        .byte 1,1,2,2,2,1,1,0,0,0,1,1,0,0,0,7,12,13,11,14,8,255,0,10,1,1,1,255,0,11,7,14,12
        .byte 8,255,0,26,77,4,255,0,27,3,75,76,255,0,26,5,4,255,0,28,4,255,0,8,37,41,38,255,0,6,47,255,0,8
        .byte 51,52,4,255,0,6,37,44,44,44,43,38,0,0,47,0,37,54,41,44,44,38,255,0,4,49,50,6,255,0,5,86,255,44,7
        .byte 41,54,255,44,6,39,255,0,4,49,50,255,0,6,46,44,43,45,255,44,8,43,44,44,39,255,0,14,40,255,44,5,43
        .byte 255,44,6,39,255,0,16,255,44,7,45,255,44,4,255,0,9,47,255,0,7,40,255,44,10,43,38,255,0,6,9,11,46
        .byte 255,0,8,40,45,55,44,44,43,45,44,44,44,42,39,255,0,5,9,11,11,255,0,11,46,40,44,42,42,44,39,255,0,7
        .byte 9,12,13,12,51,52,255,0,23,7,11,14,11,49,50,255,0,24,7,11,11,52,255,0,26,7,14,50,80,81,255,0,24
        .byte 74,77,50,78,79,255,0,25,3,51,52,0,0,47,0,0,51,52,0,51,52,255,0,14,47,0,3,49,50,51,52,46,2
        .byte 0,49,50,0,49,50,0,255,2,12,0,46,0,5
        .byte 13,12,12,36,12,11,12,8,0,255,1,4,0,0,0,7,36,11,12,11,12,35,12,14,8,0,74,73,12,13,11,14,12
        .byte 14,12,0,0,0,48,255,0,6,7,8,14,12,14,8,14,8,0,0,0,3,13,35,12,13,14,35,8,255,0,13,77,75
        .byte 76,255,0,5,3,11,12,14,12,14,8,255,0,13,74,73,4,0,0,0,48,0,0,3,14,12,14,8,255,0,16,3,4
        .byte 0,34,0,0,0,74,73,75,76,255,0,18,5,6,0,33,255,0,4,3,4,255,0,18,48,255,0,8,3,4,255,0,18
        .byte 37,38,255,0,7,3,4,255,0,17,37,44,44,43,38,255,0,5,3,4,255,0,16,37,44,45,44,44,39,255,0,5,5
        .byte 75,76,255,0,11,47,0,37,41,44,44,44,39,47,255,0,7,4,255,0,7,86,0,47,37,41,54,45,44,44,44,55,39
        .byte 0,46,255,0,7,4,255,0,6,37,54,44,54,255,44,5,45,39,46,255,0,10,4,255,0,6,40,44,55,43,42,43,42
        .byte 39,255,0,13,51,4,255,0,8,46,255,0,18,49,4,255,0,27,49,6,255,0,26,51,52,255,0,27,49,50,52,255,0,26
        .byte 49,50,50,255,0,26,49,50,51,52,255,0,9,32,255,0,17,49,50,255,0,9,31,255,2,8,255,0,6,51,52,0,49
        .byte 50,84,85,0,255,2,6,255,1,9,255,2,5,0,49,50,0
        .byte 13,11,12,11,12,13,12,10,0,255,1,4,0,0,0,255,1,6,0,7,11,36,11,14,12,12,12,13,14,35,12,12,8
        .byte 255,0,16,7,11,12,11,35,36,13,12,14,12,14,8,255,0,19,7,14,12,11,12,35,14,8,255,0,23,77,14,12,14
        .byte 8,255,0,24,3,4,255,0,26,74,73,4,0,2,2,255,0,23,74,73,4,0,1,1,2,2,255,0,11,9,13,11,10
        .byte 255,0,7,3,4,0,0,0,1,1,0,9,11,13,10,255,0,4,9,11,11,12,13,12,10,255,0,6,3,6,255,0,5
        .byte 9,11,11,11,12,10,0,0,9,11,12,11,13,14,14,12,11,10,255,0,4,3,255,0,5,9,11,12,13,14,11,14,10
        .byte 0,72,13,11,12,14,36,12,12,14,14,255,0,4,3,255,0,5,72,13,11,35,11,14,12,12,0,7,36,14,11,11,12
        .byte 14,35,12,8,255,0,4,3,255,0,5,7,14,13,14,12,35,14,8,0,0,7,11,8,14,12,14,14,8,255,0,5,3
        .byte 10,255,0,5,7,8,14,12,14,8,255,0,6,77,4,255,0,9,11,10,255,0,6,77,75,76,255,0,6,74,73,4,255,0,8
        .byte 51,14,12,10,255,0,5,3,4,255,0,8,3,75,76,255,0,7,49,13,12,14,255,0,5,5,6,255,0,8,3,4,255,0,8
        .byte 49,12,35,8,255,0,15,5,6,255,0,9,14,8,255,0,26,2,4,255,0,26,2,1,75,76,255,0,10,34,255,0,12
        .byte 2,2,1,0,6,255,0,7,255,2,4,33,255,2,6,255,0,4,2,2,1,1,0,0,255,2,8,255,1,11,255,2,4
        .byte 1,1,0,0,0,48
        .byte 11,12,13,11,14,13,14,12,8,0,1,1,1,0,0,0,72,11,14,11,12,14,0,7,11,13,12,13,11,13,14,12,14
        .byte 12,14,8,255,0,9,7,72,11,35,11,8,0,0,72,12,11,12,14,14,12,11,12,14,8,255,0,11,7,13,14,8,0,0,0
        .byte 7,12,12,13,35,12,14,12,8,255,0,14,77,4,255,0,5,72,12,14,14,77,4,255,0,16,3,75,76,255,0,4,7
        .byte 14,8,12,3,75,76,255,0,15,5,6,255,0,8,77,3,4,255,0,14,9,10,255,0,9,74,73,3,4,255,0,12,9
        .byte 13,13,11,13,10,255,0,8,3,73,4,255,0,11,9,11,11,12,11,12,11,10,255,0,7,3,3,4,255,0,10,9,11
        .byte 13,11,14,11,12,13,14,10,255,0,6,5,3,4,255,0,9,9,11,36,11,12,11,14,11,12,35,14,10,255,0,6,5
        .byte 6,255,0,9,72,11,14,11,13,12,14,14,12,11,12,14,255,0,17,11,12,11,11,14,11,14,11,14,12,14,8,255,0,6
        .byte 52,255,0,10,7,14,14,14,11,14,12,35,12,12,8,255,0,6,29,50,255,0,12,7,14,12,14,14,12,14,8,255,0,7
        .byte 33,50,255,0,14,74,77,4,255,0,11,50,51,52,255,0,13,73,4,255,0,10,51,0,49,50,255,0,13,3,4,255,0,10
        .byte 49,0,49,50,255,0,13,5,6,255,0,10,49,255,0,27,51,52,255,2,5,255,0,22,49,50,255,1,5,255,2,4,255,0,16
        .byte 80,81,49,50,255,0,5,255,1,4,255,2,4,0,0,0,255,2,6,0,51,52,78,79,0,0
        .byte 12,12,8,0,1,255,2,11,255,1,5,0,7,11,11,12,13,11,14,14,8,0,0,0,255,1,11,255,0,8,7,11,36
        .byte 13,11,4,255,0,24,7,11,14,12,75,76,255,0,25,77,4,4,255,0,26,3,4,75,76,255,0,24,74,73,4,4,255,0,26
        .byte 3,4,4,255,0,25,74,73,4,4,255,0,26,3,4,6,255,0,7,9,11,13,10,255,0,15,5,6,255,0,7,9,11
        .byte 12,13,11,10,255,0,23,72,14,11,13,12,14,255,0,23,11,36,11,12,14,10,255,0,16,10,255,0,6,7,11,12,35
        .byte 12,8,255,0,4,9,13,12,10,255,0,6,51,52,12,11,10,255,0,5,7,12,14,8,255,0,5,72,11,13,14,10,255,0,5
        .byte 49,50,13,12,14,255,0,6,77,75,76,255,0,4,9,13,13,35,14,8,255,0,5,49,50,12,11,8,255,0,6,3,4
        .byte 255,0,5,53,11,12,14,11,10,255,0,5,49,50,13,12,12,10,255,0,5,5,6,255,0,5,53,12,36,11,14,11,255,0,5
        .byte 49,50,12,13,13,12,10,255,0,11,7,11,12,14,14,8,255,0,7,12,35,12,12,8,0,0,87,88,0,48,255,0,7
        .byte 77,4,0,0,30,255,0,4,2,2,14,8,14,8,0,80,81,89,90,0,0,51,52,255,0,5,3,75,76,0,28,0,0
        .byte 2,2,1,1,77,4,0,0,0,78,79,255,0,4,49,50,255,0,4,48,3,4,0,0,0,2,2,1,1,0,0,3,4
        .byte 0,0,51,52,0,0,84,85,0,49,50,0,0,0,32,0,5,6,0,2,2,1,1,255,0,4
        .byte 13,11,12,13,14,12,8,7,36,11,14,7,13,13,14,11,14,35,13,14,1,7,13,11,7,13,11,14,35,11,12,13,35
        .byte 12,8,4,0,7,14,12,8,7,36,13,14,12,14,12,8,77,4,7,8,14,7,11,13,12,36,11,12,14,8,3,4,0,0
        .byte 77,4,0,3,7,14,12,14,35,8,0,3,4,0,0,0,77,7,8,14,14,12,8,0,0,3,75,76,0,3,4,0,5
        .byte 6,0,77,4,0,0,74,73,4,0,0,0,3,4,0,77,77,4,0,0,0,5,6,0,0,3,4,255,0,4,3,4,0,0,0
        .byte 3,75,76,0,74,73,4,0,3,3,75,76,255,0,6,3,4,255,0,4,5,6,0,48,0,3,4,0,0,0,3,4,0
        .byte 3,3,4,255,0,7,5,6,255,0,9,5,6,0,0,0,3,4,74,73,3,4,255,0,23,5,6,0,3,3,4,255,0,26
        .byte 3,5,6,255,0,26,5,255,0,38,34,255,0,4,255,2,8,255,0,14,2,2,28,255,2,4,255,1,8,255,0,7,11
        .byte 10,255,0,5,255,1,7,255,0,14,77,14,12,10,255,0,24,74,73,13,14,12,255,0,25,3,12,12,8,255,0,25,3
        .byte 14,8,255,0,26,3,75,76,255,0,26,5,4,255,0,28,6,255,0,26,2,2,0,0,32,0,0,0,255,2,5,87,88
        .byte 0,0,0,87,88,0,0,0,255,2,6,1,1,2,2,31,2,2,2,255,1,5,89,90,0,0,0,89,90,2,2,2,255,1,6
        .byte 0,0
        .byte 255,56,13,71,71,71,255,56,14,0,0,27,255,0,8,56,0,0,0,56,255,0,8,27,0,0,56,56,0,0,27,255,0,8
        .byte 56,0,0,0,56,255,0,8,27,0,0,56,56,0,0,27,255,0,8,56,0,0,0,56,255,0,8,27,0,0,56,56,0,0
        .byte 27,255,0,8,56,0,0,0,56,255,0,8,27,0,0,56,56,0,0,27,255,0,8,56,0,0,0,56,255,0,8,27,0,0
        .byte 56,56,0,0,27,255,0,8,56,0,0,0,56,255,0,8,27,0,0,56,56,0,0,255,56,10,0,0,0,255,56,10,0,0
        .byte 56,56,255,0,27,56,56,255,0,27,56,56,255,0,27,255,56,12,255,0,7,255,56,12,255,0,27,56,56,255,0,27,56,56
        .byte 255,0,27,56,56,0,0,255,56,10,0,0,0,255,56,10,0,0,56,56,0,0,27,255,0,6,56,255,0,7,56,255,0,6
        .byte 27,0,0,56,56,0,0,27,255,0,6,56,255,0,7,56,255,0,6,27,0,0,56,56,0,0,27,255,0,6,56,27,27
        .byte 56,56,56,27,27,56,255,0,6,27,0,0,56,56,0,0,27,255,0,6,56,255,0,7,56,255,0,6,27,0,0,56,56
        .byte 0,0,27,255,0,6,56,255,0,7,56,255,0,6,27,0,0,56,56,0,0,27,255,0,6,56,255,0,7,56,255,0,6
        .byte 27,0,0,255,56,30
numsc:
        .byte 19
.out .sprintf("SCREENS SIZE = %d", (* - scdat))

nmedat:
        .byte 0,2,136,104,5,11,72,88,255
        .byte 2,6,24,72,2,6,152,56,2,6,40,176,2,6,136,144,255
        .byte 2,6,24,184,2,6,88,144,2,6,136,168,2,6,32,40,2,6,152,48,255
        .byte 4,9,72,136,2,6,32,104,2,6,48,16,2,6,128,40,255
        .byte 2,6,24,176,2,6,96,200,2,6,144,128,2,6,48,56,2,6,120,56,2,6,152,48,255
        .byte 4,9,64,152,2,6,32,40,2,6,112,72,2,6,136,160,2,6,24,160,255
        .byte 2,6,16,40,2,6,40,184,2,6,16,112,2,6,144,64,2,6,128,144,255
        .byte 2,6,144,48,2,6,120,168,2,6,16,40,2,6,24,168,2,6,112,104,2,6,40,80,255
        .byte 2,6,104,96,2,6,136,16,2,6,128,192,2,6,16,160,2,6,40,64,255
        .byte 2,6,16,160,2,6,56,200,2,6,56,56,2,6,136,56,2,6,112,168,2,6,16,24,255
        .byte 2,6,40,32,2,6,112,88,2,6,136,32,2,6,40,192,2,6,152,176,255
        .byte 2,6,120,24,4,9,24,104,2,6,144,176,2,6,32,40,2,6,64,176,255
        .byte 2,6,32,40,2,6,48,112,2,6,32,184,2,6,144,168,2,6,136,88,2,6,152,32,255
        .byte 2,6,48,112,2,6,40,32,4,9,136,48,2,6,136,136,2,6,40,192,255
        .byte 2,6,152,32,2,6,112,104,2,6,136,192,2,6,32,80,2,6,16,168,2,6,72,200,255
        .byte 2,6,48,88,2,6,32,32,2,6,128,32,2,6,120,80,2,6,56,184,2,6,144,168,255
        .byte 2,6,120,184,2,6,120,40,2,6,48,160,2,6,32,32,255
        .byte 2,6,48,32,2,6,64,112,2,6,136,48,2,6,128,192,2,6,112,136,2,6,64,168,255
        .byte 7,15,128,112,255
.out .sprintf("POSITIONS SIZE = %d", (* - nmedat))

NUMOBJ = 6
objdta:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,144,104,0,144,104
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,168,112,0,168,112
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,88,24,0,88,24
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,80,0,0,80,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,80,192,0,80,192
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,80,216,0,80,216
.out .sprintf("OBJECTS SIZE = %d", (* - objdta))

font:
        .byte 0,0,0,0,0,0,0,0
        .byte 0,96,32,32,32,0,32,0
        .byte 0,108,36,0,0,0,0,0
        .byte 0,108,254,108,108,254,108,0
        .byte 0,24,126,120,126,30,126,24
        .byte 0,230,236,24,48,110,206,0
        .byte 0,48,120,48,126,204,126,0
        .byte 0,24,48,0,0,0,0,0
        .byte 0,12,24,24,24,24,12,0
        .byte 0,96,48,48,48,48,96,0
        .byte 255,255,255,255,255,255,255,255
        .byte 0,0,24,24,126,24,24,0
        .byte 0,0,0,0,0,24,24,48
        .byte 0,0,0,0,126,0,0,0
        .byte 0,0,0,0,0,48,48,0
        .byte 0,0,6,12,24,48,96,0
        .byte 0,124,206,222,246,230,124,0
        .byte 0,56,120,24,24,24,126,0
        .byte 0,120,132,4,120,130,252,0
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
        .byte 0,248,68,68,120,64,224,0
        .byte 0,124,130,130,162,146,124,8
        .byte 0,252,66,66,124,72,198,0
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
        .byte 55,90,119,191,255,95,191,255
        .byte 222,107,254,255,255,195,0,60
        .byte 232,22,233,250,253,253,253,250
        .byte 191,255,191,95,191,127,255,191
        .byte 253,253,254,253,255,253,250,253
        .byte 191,127,191,253,191,123,173,202
        .byte 253,255,238,253,190,223,234,68
        .byte 255,129,0,60,255,255,147,153
        .byte 159,157,193,227,255,255,243,243
        .byte 231,228,204,204,152,153,249,195
        .byte 227,255,255,255,190,219,183,42
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
