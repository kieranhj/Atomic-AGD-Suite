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
WINDOWHGT = 20
WINDOWWID = 30 ;面面面面面面面面面面面面
MAPWID = 5
        .byte 255,255,255,255,255
mapdat:
        .byte 255,0,8,16,255,255,1,9,17,255,255,2,10,18,255,255,3,11,19,255,255,4,12,20,255,255,5,13,21,255,255,6,14,22,255,255,7,15,255,255
        .byte 255,255,255,255,255
stmap:  .byte 1

evnt00:
        lda #5
        sta z80_b
        jsr sktyp
        bcs :+
        jmp a00685
:
        jsr cangl
        beq :+
        jmp a00189
:
        lda #1
        cmp vard
        beq *+5
        jmp a00189
        lda #0
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a00095
        ; lda #0
        ; ldy #6
        sta (z80_ix),y
        jmp a00131
a00095: lda #1
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a00131
        ; lda #1
        ; ldy #6
        sta (z80_ix),y
a00131: jsr cangr
        beq :+
        jmp a00189
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
a00189: jsr cangr
        beq :+
        jmp a00354
:
        lda #2
        cmp vard
        beq *+5
        jmp a00354
        lda #0
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a00260
        ; lda #0
        ; ldy #6
        sta (z80_ix),y
        jmp a00296
a00260: lda #1
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a00296
        ; lda #1
        ; ldy #6
        sta (z80_ix),y
a00296: jsr cangl
        beq :+
        jmp a00354
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
a00354: jsr cangu
        beq :+
        jmp a00520
:
        lda #4
        cmp vard
        beq *+5
        jmp a00520
        lda #2
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a00426
        ; lda #2
        ; ldy #6
        sta (z80_ix),y
        jmp a00462
a00426: lda #3
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a00462
        ; lda #3
        ; ldy #6
        sta (z80_ix),y
a00462: jsr cangd
        beq :+
        jmp a00520
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
a00520: jsr cangd
        beq :+
        jmp a00685
:
        lda #3
        cmp vard
        beq *+5
        jmp a00685
        lda #2
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a00591
        ; lda #2
        ; ldy #6
        sta (z80_ix),y
        jmp a00627
a00591: lda #3
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a00627
        ; lda #3
        ; ldy #6
        sta (z80_ix),y
a00627: jsr cangu
        beq :+
        jmp a00685
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
a00685: lda joyval
        and #16
        beq :+
        jmp a01023
:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp a01018
        lda #30
        asl a
        sta sndtyp
        lda #0
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a00808
        lda #4
        ldy #10
        sta (z80_ix),y
        lda #1
        sta z80_c
        lda #4
        sta z80_b
        jsr spawn
        lda #1
        ldy #11
        sta (z80_ix),y
a00808: lda #1
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a00878
        lda #3
        ldy #10
        sta (z80_ix),y
        lda #1
        sta z80_c
        lda #4
        sta z80_b
        jsr spawn
        lda #1
        ldy #11
        sta (z80_ix),y
a00878: lda #2
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a00948
        lda #1
        ldy #10
        sta (z80_ix),y
        ; lda #1
        sta z80_c
        lda #5
        sta z80_b
        jsr spawn
        lda #1
        ldy #11
        sta (z80_ix),y
a00948: lda #3
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a01018
        lda #2
        ldy #10
        sta (z80_ix),y
        lda #1
        sta z80_c
        lda #5
        sta z80_b
        jsr spawn
        lda #1
        ldy #11
        sta (z80_ix),y
a01018: jmp a01036
a01023: lda #0
        ldy #11
        sta (z80_ix),y
a01036: lda joyval
        and #2
        beq :+
        jmp a01363
:
        jsr cangl
        beq :+
        jmp a01358
:
        lda #0
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a01104
        ; lda #0
        ; ldy #6
        sta (z80_ix),y
a01104: lda #1
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a01140
        ; lda #1
        ; ldy #6
        sta (z80_ix),y
a01140: lda #2
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a01222
        lda #4
        cmp vard
        beq *+5
        jmp a01192
        lda #1
        ; ldy #6
        sta (z80_ix),y
a01192: lda #3
        cmp vard
        beq *+5
        jmp a01222
        lda #0
        ldy #6
        sta (z80_ix),y
a01222: lda #3
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a01305
        ; lda #3
        cmp vard
        beq *+5
        jmp a01275
        lda #1
        ; ldy #6
        sta (z80_ix),y
a01275: lda #4
        cmp vard
        beq *+5
        jmp a01305
        lda #0
        ldy #6
        sta (z80_ix),y
a01305: ldy #9
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
        sta vard
a01358: jmp a02325
a01363: lda joyval
        and #1
        beq :+
        jmp a01688
:
        jsr cangr
        beq :+
        jmp a01684
:
        lda #0
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a01430
        ; lda #0
        ; ldy #6
        sta (z80_ix),y
a01430: lda #1
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a01466
        ; lda #1
        ; ldy #6
        sta (z80_ix),y
a01466: lda #2
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a01549
        lda #4
        cmp vard
        beq *+5
        jmp a01519
        lda #0
        ; ldy #6
        sta (z80_ix),y
a01519: lda #3
        cmp vard
        beq *+5
        jmp a01549
        lda #1
        ldy #6
        sta (z80_ix),y
a01549: lda #3
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a01631
        lda #4
        cmp vard
        beq *+5
        jmp a01601
        lda #1
        ; ldy #6
        sta (z80_ix),y
a01601: lda #3
        cmp vard
        beq *+5
        jmp a01631
        lda #0
        ldy #6
        sta (z80_ix),y
a01631: ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #2
        sta vard
a01684: jmp a02325
a01688: lda joyval
        and #4
        beq :+
        jmp a02015
:
        jsr cangd
        beq :+
        jmp a02010
:
        lda #0
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a01802
        lda #2
        cmp vard
        beq *+5
        jmp a01772
        lda #3
        ; ldy #6
        sta (z80_ix),y
a01772: lda #1
        cmp vard
        beq *+5
        jmp a01802
        lda #2
        ldy #6
        sta (z80_ix),y
a01802: lda #1
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a01885
        lda #2
        cmp vard
        beq *+5
        jmp a01855
        ; lda #2
        ; ldy #6
        sta (z80_ix),y
a01855: lda #1
        cmp vard
        beq *+5
        jmp a01885
        lda #3
        ldy #6
        sta (z80_ix),y
a01885: lda #2
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a01921
        ; lda #2
        ; ldy #6
        sta (z80_ix),y
a01921: lda #3
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a01957
        ; lda #3
        ; ldy #6
        sta (z80_ix),y
a01957: ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #3
        sta vard
a02010: jmp a02325
a02015: lda joyval
        and #8
        beq :+
        jmp a02325
:
        jsr cangu
        beq :+
        jmp a02325
:
        lda #0
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a02189
        lda #1
        cmp vard
        beq *+5
        jmp a02098
        lda #3
        ; ldy #6
        sta (z80_ix),y
a02098: lda #4
        cmp vard
        beq *+5
        jmp a02128
        lda #2
        ldy #6
        sta (z80_ix),y
a02128: lda #3
        cmp vard
        beq *+5
        jmp a02159
        ; lda #3
        ldy #6
        sta (z80_ix),y
a02159: lda #2
        cmp vard
        beq *+5
        jmp a02189
        ; lda #2
        ldy #6
        sta (z80_ix),y
a02189: lda #1
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a02272
        ; lda #1
        cmp vard
        beq *+5
        jmp a02242
        lda #2
        ; ldy #6
        sta (z80_ix),y
a02242: lda #2
        cmp vard
        beq *+5
        jmp a02272
        lda #3
        ldy #6
        sta (z80_ix),y
a02272: ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #4
        sta vard
a02325: lda #2
        sta z80_b
        jsr sktyp
        bcs :+
        jmp a02375
:
        lda #8
        ldy #6
        sta (z80_ix),y
        ; lda #8
        dey; ldy #5
        sta (z80_ix),y
a02375: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a02430
:
        lda #8
        ldy #6
        sta (z80_ix),y
        ; lda #8
        dey; ldy #5
        sta (z80_ix),y
a02430: rts
.out .sprintf("EVENT 0 SIZE = %d", (* - evnt00))

evnt01:
        lda #0
        cmp vara
        beq *+5
        jmp b00027
        ; lda #0
        jsr animsp
b00027: lda #1
        sta varl
        ldy #9
        lda (z80_ix),y
        lsr a
        lsr a
        ; ldy #9
        sta (z80_ix),y
        ; ldy #9
        lda (z80_ix),y
        asl a
        asl a
        ; ldy #9
        sta (z80_ix),y
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp b00146
:
        lda #0
        sta varl
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #2
        jsr explod
b00146: lda #3
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00205
:
        lda #0
        sta varl
        lda vark
        clc
        adc #1
        sta vark
        lda #255
        ldy #5
        sta (z80_ix),y
b00205: lda #2
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00303
:
        lda #0
        sta varl
        lda #255
        ldy #5
        sta (z80_ix),y
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #7
        ; ldy #5
        sta (z80_ix),y
        lda #<150
        sta z80_l
        lda #>150
        sta z80_h
        jsr addsc
b00303: lda #6
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00339
:
        lda #255
        ldy #5
        sta (z80_ix),y
b00339: lda #5
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00374
:
        lda #255
        ldy #5
        sta (z80_ix),y
b00374: lda #4
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00504
        jsr cangu
        beq :+
        jmp b00482
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
        ; ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp b00504
b00482: lda #255
        ldy #5
        sta (z80_ix),y
        lda #0
        sta varl
b00504: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00633
        jsr cangd
        beq :+
        jmp b00611
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
        ; ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp b00633
b00611: lda #255
        ldy #5
        sta (z80_ix),y
        lda #0
        sta varl
b00633: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00762
        jsr cangl
        beq :+
        jmp b00740
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
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp b00762
b00740: lda #255
        ldy #5
        sta (z80_ix),y
        lda #0
        sta varl
b00762: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00891
        jsr cangr
        beq :+
        jmp b00870
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
        ; ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp b00891
b00870: lda #255
        ldy #5
        sta (z80_ix),y
        lda #0
        sta varl
b00891: rts
.out .sprintf("EVENT 1 SIZE = %d", (* - evnt01))

evnt02:
        lda #9
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00028
        jmp c00077
c00028: lda #6
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00055
        jmp c00077
c00055: lda #12
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00077
c00077: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00201
:
        lda #9
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00134
        ; lda #9
        sta vari
        jmp c00201
c00134: lda #6
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00170
        ; lda #6
        sta vari
        jmp c00201
c00170: lda #12
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00201
        ; lda #12
        sta vari
c00201: lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00525
:
        lda #7
        ldy #5
        sta (z80_ix),y
        lda #9
        cmp vari
        beq *+5
        jmp c00381
        lda vare
        clc
        adc #1
        sta vare
        lda #6
        cmp vare
        beq *+5
        jmp c00327
        lda #14
        iny; ldy #6
        sta (z80_ix),y
        lda #255
        dey; ldy #5
        sta (z80_ix),y
        lda #2
        jsr explod
        jmp c00341
c00327: lda #7
        ldy #5
        sta (z80_ix),y
c00341: lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #0
        sta varl
c00381: lda #6
        cmp vari
        beq *+5
        jmp c00525
        lda vare
        clc
        adc #1
        sta vare
        lda #6
        cmp vare
        beq *+5
        jmp c00471
        lda #14
        ldy #6
        sta (z80_ix),y
        lda #255
        dey; ldy #5
        sta (z80_ix),y
        lda #2
        jsr explod
        jmp c00485
c00471: lda #7
        ldy #5
        sta (z80_ix),y
c00485: lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #0
        sta varl
c00525: lda #0
        cmp vara
        beq *+5
        jmp c00551
        ; lda #0
        jsr animsp
c00551: rts
.out .sprintf("EVENT 2 SIZE = %d", (* - evnt02))

evnt03:
        lda #17
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00080
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
d00080: lda #16
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00159
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
d00159: lda #11
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00181
d00181: lda #7
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00381
        lda #0
        cmp vark
        beq *+5
        jmp d00233
        ; lda #0
        sta varl
        jmp d00381
d00233: lda #1
        cmp vark
        beq *+5
        jmp d00262
        lda #0
        sta varl
        jmp d00381
d00262: lda #2
        cmp vark
        beq *+5
        jmp d00292
        lda #0
        sta varl
        jmp d00381
d00292: lda #3
        cmp vark
        beq *+5
        jmp d00381
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #0
        sta varl
        ; lda #0
        sta vark
        lda #10
        iny; ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        lda #4
        ldy #5
        sta (z80_ix),y
d00381: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp d00635
        jsr cangl
        beq :+
        jmp d00558
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #2
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00496
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #1
        ldy #11
        sta (z80_ix),y
d00496: lda #5
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00554
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #1
        ldy #11
        sta (z80_ix),y
d00554: jmp d00630
d00558: lda #1
        ldy #11
        sta (z80_ix),y
        ; lda #1
        cmp scno
        beq *+5
        jmp d00601
        lda #10
        asl a
        sta sndtyp
d00601: lda #6
        cmp scno
        beq *+5
        jmp d00630
        lda #10
        asl a
        sta sndtyp
d00630: jmp d00861
d00635: jsr cangr
        beq :+
        jmp d00790
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #2
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00728
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #0
        ldy #11
        sta (z80_ix),y
d00728: lda #5
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00785
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #0
        ldy #11
        sta (z80_ix),y
d00785: jmp d00861
d00790: lda #0
        ldy #11
        sta (z80_ix),y
        lda #1
        cmp scno
        beq *+5
        jmp d00832
        lda #10
        asl a
        sta sndtyp
d00832: lda #6
        cmp scno
        beq *+5
        jmp d00861
        lda #10
        asl a
        sta sndtyp
d00861: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp d01116
        jsr cangu
        beq :+
        jmp d01038
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #2
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00976
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #1
        ldy #10
        sta (z80_ix),y
d00976: lda #5
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d01034
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #1
        ldy #10
        sta (z80_ix),y
d01034: jmp d01112
d01038: lda #1
        ldy #10
        sta (z80_ix),y
        ; lda #1
        cmp scno
        beq *+5
        jmp d01083
        tya; lda #10
        asl a
        sta sndtyp
d01083: lda #6
        cmp scno
        beq *+5
        jmp d01112
        lda #10
        asl a
        sta sndtyp
d01112: jmp d01344
d01116: jsr cangd
        beq :+
        jmp d01271
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #2
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d01209
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #0
        ldy #10
        sta (z80_ix),y
d01209: lda #5
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d01266
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #0
        ldy #10
        sta (z80_ix),y
d01266: jmp d01344
d01271: lda #0
        ldy #10
        sta (z80_ix),y
        lda #1
        cmp scno
        beq *+5
        jmp d01315
        tya; lda #10
        asl a
        sta sndtyp
d01315: lda #6
        cmp scno
        beq *+5
        jmp d01344
        lda #10
        asl a
        sta sndtyp
d01344: lda #0
        cmp vara
        beq *+5
        jmp d01370
        ; lda #0
        jsr animsp
d01370: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d01669
:
        lda #7
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d01461
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #8
        ; ldy #6
        sta (z80_ix),y
        ; lda #8
        dey; ldy #5
        sta (z80_ix),y
d01461: lda #11
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d01530
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #8
        ; ldy #6
        sta (z80_ix),y
        ; lda #8
        dey; ldy #5
        sta (z80_ix),y
d01530: lda #16
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d01600
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #8
        ; ldy #6
        sta (z80_ix),y
        ; lda #8
        dey; ldy #5
        sta (z80_ix),y
d01600: lda #17
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d01669
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #8
        ; ldy #6
        sta (z80_ix),y
        ; lda #8
        dey; ldy #5
        sta (z80_ix),y
d01669: rts
.out .sprintf("EVENT 3 SIZE = %d", (* - evnt03))

evnt04:
        lda #0
        sta varj
        lda #1
        cmp varo
        beq *+5
        jmp e00031
        jmp e00109
e00031: lda #2
        cmp varo
        beq *+5
        jmp e00109
        lda #3
        ldy #5
        sta (z80_ix),y
        lda #7
        iny; ldy #6
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp e00101
        ; lda #0
        jsr animsp
e00101: lda #0
        sta varo
e00109: lda #0
        cmp vara
        beq *+5
        jmp e00135
        ; lda #0
        jsr animsp
e00135: rts
.out .sprintf("EVENT 4 SIZE = %d", (* - evnt04))

evnt05:
        lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00084
:
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #7
        ; ldy #5
        sta (z80_ix),y
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ; ldy #5
        sta (z80_ix),y
f00084: rts
.out .sprintf("EVENT 5 SIZE = %d", (* - evnt05))

evnt06:
        lda #18
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp g00334
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
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp g00268
        jsr cangu
        beq :+
        jmp g00238
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #2
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00185
:
        lda #1
        ldy #11
        sta (z80_ix),y
        lda #12
        asl a
        sta sndtyp
g00185: lda #5
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00233
:
        lda #1
        ldy #11
        sta (z80_ix),y
        lda #12
        asl a
        sta sndtyp
g00233: jmp g00263
g00238: lda #1
        ldy #11
        sta (z80_ix),y
        lda #12
        asl a
        sta sndtyp
g00263: jmp g00334
g00268: jsr cangd
        beq :+
        jmp g00308
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp g00334
g00308: lda #0
        ldy #11
        sta (z80_ix),y
        lda #12
        asl a
        sta sndtyp
g00334: lda #11
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp g00705
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp g00544
        jsr cangu
        beq :+
        jmp g00514
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #2
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00462
:
        lda #1
        ldy #11
        sta (z80_ix),y
        lda #12
        asl a
        sta sndtyp
g00462: lda #5
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00509
:
        lda #1
        ldy #11
        sta (z80_ix),y
        lda #12
        asl a
        sta sndtyp
g00509: jmp g00540
g00514: lda #1
        ldy #11
        sta (z80_ix),y
        lda #12
        asl a
        sta sndtyp
g00540: jmp g00705
g00544: jsr cangd
        beq :+
        jmp g00680
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #2
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00627
:
        lda #0
        ldy #11
        sta (z80_ix),y
        lda #12
        asl a
        sta sndtyp
g00627: lda #5
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00675
:
        lda #0
        ldy #11
        sta (z80_ix),y
        lda #12
        asl a
        sta sndtyp
g00675: jmp g00705
g00680: lda #0
        ldy #11
        sta (z80_ix),y
        lda #12
        asl a
        sta sndtyp
g00705: lda #15
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp g01179
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00781
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #8
        ldy #5
        sta (z80_ix),y
g00781: lda #0
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp g00994
        jsr cangl
        beq :+
        jmp g00963
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
        lda #16
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #5
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00958
:
        lda #1
        ldy #12
        sta (z80_ix),y
        lda #14
        asl a
        sta sndtyp
g00958: jmp g00989
g00963: lda #1
        ldy #12
        sta (z80_ix),y
        lda #14
        asl a
        sta sndtyp
g00989: jmp g01179
g00994: jsr cangr
        beq :+
        jmp g01153
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
        lda #16
        sta z80_c
        ; ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #5
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g01149
:
        lda #0
        ldy #12
        sta (z80_ix),y
        lda #14
        asl a
        sta sndtyp
g01149: jmp g01179
g01153: lda #0
        ldy #12
        sta (z80_ix),y
        lda #14
        asl a
        sta sndtyp
g01179: lda #0
        cmp vara
        beq *+5
        jmp g01205
        ; lda #0
        jsr animsp
g01205: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g01258
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #8
        ldy #5
        sta (z80_ix),y
g01258: rts
.out .sprintf("EVENT 6 SIZE = %d", (* - evnt06))

evnt07:
        lda #7
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
        sta varq
        lda #13
        ldy #6
        sta (z80_ix),y
        lda #50
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #10
        sta z80_c
        lda varrnd
        clc
        adc z80_c
        sta varrnd
        lda varrnd
        asl a
        sta sndtyp
        lda varg
        clc
        adc #1
        sta varg
        lda #20
        cmp varg
        beq *+5
        jmp h00248
        lda #13
        ; ldy #6
        sta (z80_ix),y
        lda #255
        dey; ldy #5
        sta (z80_ix),y
        lda #0
        sta varg
        lda #6
        cmp vare
        beq *+5
        jmp h00240
        lda #1
        sta varb
        jmp h00248
h00240: lda #0
        sta varb
h00248: lda #0
        cmp vara
        beq *+5
        jmp h00274
        ; lda #0
        jsr animsp
h00274: lda #<10
        sta z80_l
        lda #>10
        sta z80_h
        jsr addsc
        rts
.out .sprintf("EVENT 7 SIZE = %d", (* - evnt07))

evnt08:
        lda #22
        cmp scno
        beq *+5
        jmp i00240
        lda #0
        ldy #6
        sta (z80_ix),y
        ; lda #0
        cmp vara
        beq *+5
        jmp i00141
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #16
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #5
        sta z80_c
        lda varrnd
        clc
        adc z80_c
        sta varrnd
        lda varrnd
        asl a
        sta sndtyp
i00141: lda #4
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp i00236
        lda #255
        ldy #5
        sta (z80_ix),y
        tya; lda #5
        sta chary
        lda #8
        sta charx
        lda #13
        jsr dmsg
        lda #7
        sta chary
        lda #4
        sta charx
        lda #14
        jsr dmsg
        lda #1
        sta gamwon
i00236: jmp i00692
i00240: lda #20
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #5
        sta z80_c
        lda varrnd
        clc
        adc z80_c
        sta varrnd
        lda varrnd
        asl a
        sta sndtyp
        lda #8
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp i00349
        ; lda #0
        jsr animsp
i00349: jsr cangu
        beq :+
        jmp i00412
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
        jmp i00692
i00412: ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #16
        ; ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp i00692
        lda #10
        jsr explod
        lda #255
        ldy #5
        sta (z80_ix),y
        lda varm
        sec
        sbc #1
        sta varm
        lda #2
        cmp varm
        beq *+5
        jmp i00575
        lda #10
        sta chary
        lda #8
        sta charx
        lda #10
        jsr dmsg
        lda #60
        jsr delay
        lda #1
        sta deadf
i00575: lda #1
        cmp varm
        beq *+5
        jmp i00634
        lda #10
        sta chary
        lda #8
        sta charx
        lda #10
        jsr dmsg
        lda #60
        jsr delay
        lda #1
        sta deadf
i00634: lda #0
        cmp varm
        beq *+5
        jmp i00692
        lda #10
        sta chary
        lda #12
        sta charx
        lda #0
        jsr dmsg
        lda #60
        jsr delay
        lda #1
        sta deadf
i00692: rts
.out .sprintf("EVENT 8 SIZE = %d", (* - evnt08))

evnt09:
        lda #0
        sta varo
        lda #2
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00075
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
j00075: rts
.out .sprintf("EVENT 9 SIZE = %d", (* - evnt09))

evnt10:
        lda #22
        cmp scno
        beq *+5
        jmp k00044
        lda #0
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k00044: lda scno
        sta varn
        lda varn
        clc
        adc #1
        sta varn
        lda #6
        cmp vare
        beq *+5
        jmp k00168
        lda #10
        sta chary
        lda #11
        sta charx
        ; lda #11
        jsr dmsg
        lda #12
        sta chary
        lda #10
        sta charx
        lda #12
        jsr dmsg
        lda #0
        sta vare
        lda #80
        jsr delay
        lda #1
        sta nexlev
        lda #0
        sta varb
k00168: lda #0
        cmp varj
        beq *+5
        jmp k00225
        lda varh
        clc
        adc #1
        sta varh
        lda #255
        cmp varh
        beq *+5
        jmp k00225
        lda #0
        sta varh
k00225: lda #0
        cmp scno
        beq *+5
        jmp k00255
        jsr prskey
        lda #1
        sta nexlev
k00255: lda #0
        cmp vara
        beq *+5
        jmp k00285
        lda #1
        sta vara
        jmp k00293
k00285: lda #0
        sta vara
k00293: lda #22
        sta chary
        lda #8
        sta charx
        jsr dscor
        rts
.out .sprintf("EVENT 10 SIZE = %d", (* - evnt10))

evnt11:
        lda #1
        cmp varh
        beq *+5
        jmp l00027
        ; lda #1
        sta varo
l00027: lda #250
        cmp varh
        beq *+5
        jmp l00052
        lda #2
        sta varo
l00052: rts
.out .sprintf("EVENT 11 SIZE = %d", (* - evnt11))

evnt12:
        rts
.out .sprintf("EVENT 12 SIZE = %d", (* - evnt12))

evnt13:
        lda #0
        sta varg
        lda #3
        sta varm
        lda #0
        sta varn
        ; lda #0
        sta varf
        ; lda #0
        sta varl
        ; lda #0
        sta varo
        ; lda #0
        sta varh
        lda #1
        sta varj
        lda #0
        sta vark
        ; lda #0
        sta vara
        ; lda #0
        sta vare
        ; lda #0
        sta vard
        lda #3
        sta numlif
        jsr cls
        lda #0
        sta chary
        ; lda #0
        sta charx
        lda #9
        jsr dmsg
        lda #0
        sta chary
        lda #31
        sta charx
        lda #9
        jsr dmsg
        lda #23
        sta chary
        lda #31
        sta charx
        lda #9
        jsr dmsg
        lda #23
        sta chary
        lda #0
        sta charx
        lda #9
        jsr dmsg
        lda #0
        sta chary
        lda #1
        sta charx
        ; lda #1
        jsr dmsg
        lda #21
        sta chary
        lda #1
        sta charx
        ; lda #1
        jsr dmsg
        lda #23
        sta chary
        lda #1
        sta charx
        ; lda #1
        jsr dmsg
        lda #0
        sta chary
        lda #11
        sta charx
        lda #2
        jsr dmsg
        lda #0
        sta chary
        lda #17
        sta charx
        lda #3
        jsr dmsg
        lda #22
        sta chary
        lda #2
        sta charx
        lda #4
        jsr dmsg
        lda #22
        sta chary
        lda #15
        sta charx
        lda #5
        jsr dmsg
        lda #22
        sta chary
        lda #23
        sta charx
        lda #6
        jsr dmsg
        lda #1
        sta chary
        lda #22
        sta loopa
n00430: lda #0
        sta charx
        lda #7
        jsr dmsg
        dec loopa
        beq :+
        jmp n00430
:
        lda #1
        sta chary
        lda #22
        sta loopa
n00476: lda #31
        sta charx
        lda #8
        jsr dmsg
        dec loopa
        beq :+
        jmp n00476
:
        rts
.out .sprintf("EVENT 13 SIZE = %d", (* - evnt13))

evnt14:
        lda #0
        sta varg
        ; lda #0
        sta varf
        ; lda #0
        sta vare
        lda numlif
        sta varm
        lda #1
        sta varj
        lda #0
        sta varh
        ; lda #0
        sta vark
        ; lda #0
        sta varl
        ; lda #0
        sta vara
        lda #22
        sta chary
        lda #21
        sta charx
        lda numlif
        jsr disply
        lda #22
        sta chary
        lda #29
        sta charx
        lda varn
        jsr disply
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
        lda #0
        sta varg
        ; lda #0
        sta vare
        ; lda #0
        sta varl
        lda numlif
        sec
        sbc #1
        sta numlif
        lda #30
        jsr delay
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
        .byte "GAME OVER",141
        .byte "((((((((((((((((((((((((((((((",141
        .byte "SPACE",141
        .byte "JUNK",141
        .byte "SCORE",141
        .byte "LIVES",141
        .byte "LEVEL",141
        .byte ")",141
        .byte "+",141
        .byte "*",141
        .byte "YOU LOST A SHIP",141
        .byte "WELL DONE",141
        .byte "LEVEL CLEAR",141
        .byte "CONGRATULATIONS!",141
        .byte "TIME TO COME BACK HOME!",141
nummsg:
        .byte 15
.out .sprintf("MESSAGE SIZE = %d", (* - msgdat))

chgfx:
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 240,240,242,250,244,240,240,240,244,244,240,248,244,240,240,240
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 33,32,66,16,36,37,66,2,33,36,82,26,129,33,2,74
        .byte 0,0,39,174,85,10,39,174,87,78,5,170,87,78,0,0
        .byte 15,143,15,143,31,79,47,79,127,79,79,111,239,111,175,95
        .byte 15,15,15,15,15,15,15,15,31,15,15,15,15,15,15,15
        .byte 0,0,39,39,17,85,39,5,68,85,39,39,0,0,0,0
        .byte 0,0,38,174,2,2,34,34,2,2,39,34,0,0,0,0
        .byte 0,0,240,160,128,0,160,160,160,160,160,160,0,0,0,0
        .byte 0,0,224,160,128,160,160,160,32,0,224,224,0,0,0,0
        .byte 0,0,224,224,128,0,192,64,0,64,224,64,0,0,0,0
        .byte 0,0,224,128,128,128,192,128,0,0,224,224,0,0,0,0
        .byte 0,0,224,0,160,0,160,0,32,0,224,0,0,0,0,0
        .byte 0,0,174,174,78,8,170,170,78,78,170,174,0,0,0,0
        .byte 0,0,140,0,10,0,170,0,78,0,140,0,0,0,0,0
        .byte 241,245,242,250,245,240,250,240,244,240,248,240,240,241,248,242
        .byte 90,90,225,180,120,240,120,225,60,75,45,75,45,15,30,15
        .byte 15,15,15,15,15,143,15,15,15,15,15,15,15,15,15,15
        .byte 90,90,225,180,120,240,120,225,60,75,45,75,45,15,30,15
        .byte 161,5,88,82,36,32,0,24,129,36,90,16,36,165,0,66
        .byte 241,245,242,250,245,240,250,240,244,240,248,240,240,241,248,242
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,8,0,0,0,0,0,0,0,0,0,0
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,45,15,30,90
        .byte 165,45,75,75,45,45,75,15,15,15,15,15,15,15,15,15
        .byte 80,74,5,164,0,0,165,37,0,0,160,160,80,80,0,0
        .byte 0,0,5,5,10,10,0,0,78,95,0,0,87,10,39,170
        .byte 85,228,80,234,0,0,250,114,0,0,170,170,85,85,0,0
        .byte 0,0,80,80,160,160,0,0,224,240,0,0,112,160,112,160
        .byte 0,10,164,129,0,10,164,129,74,2,164,129,74,10,0,129
        .byte 0,0,5,5,10,10,0,0,74,90,0,0,82,10,37,160
        .byte 15,31,15,127,63,239,111,15,255,95,127,175,15,255,15,31
        .byte 255,255,79,191,111,111,63,111,31,79,47,207,127,143,239,15
        .byte 32,160,64,96,32,224,64,192,32,160,64,96,32,224,64,192
        .byte 228,17,200,34,128,85,0,170,17,85,50,170,117,196,250,136
        .byte 0,10,174,137,0,10,174,137,78,2,174,137,78,10,0,137
        .byte 0,0,1,174,19,10,39,174,87,78,5,170,87,78,0,0
        .byte 0,0,175,175,0,0,175,175,95,95,5,5,10,10,0,0
        .byte 87,78,175,175,8,17,175,175,8,17,175,175,68,2,39,174
        .byte 0,0,112,224,80,160,112,224,112,224,80,160,112,224,0,0
        .byte 0,0,39,136,85,76,39,174,87,78,5,170,87,78,0,0
        .byte 0,0,39,174,85,10,39,174,87,78,35,170,17,78,0,0
        .byte 0,0,39,174,85,10,39,174,87,78,5,140,87,8,0,0
        .byte 85,10,5,140,85,10,5,140,85,10,5,140,85,10,5,140
        .byte 85,0,137,39,85,0,137,39,68,87,137,39,85,87,137,0
        .byte 0,0,137,137,2,68,5,34,68,10,34,4,25,25,0,0
        .byte 161,37,66,64,32,32,64,0,0,0,0,0,0,0,0,0
        .byte 128,17,0,16,177,213,0,0,177,213,50,216,128,17,0,16
        .byte 128,17,0,16,177,213,0,0,177,213,50,216,128,17,0,16
        .byte 0,10,164,129,0,10,164,129,74,2,164,129,74,10,0,129
        .byte 136,17,0,17,187,221,0,0,187,221,51,221,136,17,0,17
        .byte 0,160,224,144,0,160,224,144,224,32,224,144,224,160,0,144
        .byte 85,10,5,140,85,10,5,140,85,10,5,140,85,10,5,140
        .byte 0,0,200,0,0,0,64,0,68,0,64,0,228,128,0,0
        .byte 0,0,153,119,153,0,170,102,204,68,153,68,153,119,0,0
        .byte 0,0,85,119,85,68,119,119,34,17,0,0,34,119,0,0
        .byte 0,0,39,39,85,85,5,5,85,87,5,4,87,68,0,0
        .byte 0,0,39,39,85,85,5,5,85,87,4,5,85,85,0,0
        .byte 0,0,96,192,64,160,96,160,32,192,32,128,96,128,0,0
        .byte 0,0,234,0,32,0,234,0,0,0,136,0,228,128,0,0
        .byte 0,0,153,119,34,0,204,102,136,68,170,68,153,119,0,0
        .byte 0,0,119,51,102,170,68,170,68,187,68,170,68,170,0,0
        .byte 0,0,51,238,170,68,187,68,17,68,17,68,51,85,0,0
        .byte 0,0,234,0,32,0,64,0,32,0,0,0,228,128,0,0
        .byte 0,0,221,119,0,17,221,85,85,85,85,85,221,85,0,0
        .byte 0,0,102,153,68,153,68,153,68,153,68,153,102,221,0,0
        .byte 0,0,221,102,0,17,85,85,221,102,85,85,85,85,0,0
        .byte 30,15,45,15,75,15,45,15,105,135,150,75,45,15,30,15
        .byte 31,15,47,15,31,79,15,15,31,79,15,15,31,31,175,15
        .byte 31,95,143,143,15,15,47,143,15,15,47,15,31,79,15,143
        .byte 33,160,80,2,5,4,24,2,32,32,80,0,4,164,2,64
        .byte 224,112,96,224,240,240,32,96,96,64,240,240,112,96,224,112
        .byte 160,213,170,98,117,49,16,152,32,196,50,170,85,17,0,170
        .byte 15,165,30,15,15,15,30,15,15,135,15,15,15,15,15,15
        .byte 4,132,16,66,5,161,80,88,32,133,64,66,33,32,80,0
        .byte 32,128,64,66,160,33,82,66,129,33,18,82,4,32,64,72
        .byte 36,132,80,72,33,128,18,0,32,4,0,10,1,36,2,72
        .byte 100,196,80,200,49,128,50,0,32,68,0,170,17,100,34,200
        .byte 112,0,208,128,128,128,208,160,112,48,0,16,144,112,96,96
        .byte 0,119,17,255,51,119,102,255,119,255,187,255,51,255,153,255
        .byte 238,0,17,136,204,204,238,102,255,34,255,153,255,153,255,153
        .byte 25,95,136,39,8,0,5,5,68,10,35,5,17,95,0,39
        .byte 95,25,174,1,0,19,5,38,10,78,5,140,95,8,174,0
        .byte 112,0,208,128,128,128,208,160,112,48,0,16,144,112,96,96
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
bprop:
        .byte 0
        .byte 2
        .byte 0
        .byte 2
        .byte 2
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
        .byte 2
        .byte 2
        .byte 0
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
        .byte 2
        .byte 2
        .byte 2
        .byte 5
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
.out .sprintf("BLOCKS SIZE = %d", (* - chgfx))

sprgfx:
        .byte 0,0,0,0,32,4,96,6,0,0,96,6,1,128,98,70,68,34,84,170,52,172,124,190,220,59,174,117,219,219,121,158
        .byte 121,158,219,219,174,117,220,59,124,190,52,172,84,170,68,34,98,70,1,128,96,6,0,0,96,6,32,4,0,0,0,0
        .byte 0,14,21,219,53,53,0,123,0,31,0,252,1,6,2,3,2,115,1,6,0,252,0,31,0,123,53,53,21,219,0,30
        .byte 112,0,219,168,172,172,222,0,248,0,63,0,96,128,192,64,206,64,96,128,63,0,248,0,222,0,172,172,219,168,120,0
        .byte 0,0,32,4,32,4,0,0,64,2,64,2,0,0,32,4,32,4,0,0,64,2,64,2,0,0,32,4,32,4,0,0
        .byte 0,0,64,2,64,2,0,0,32,4,32,4,0,0,64,2,64,2,0,0,32,4,32,4,0,0,64,2,64,2,0,0
        .byte 0,0,12,48,97,134,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,97,134,12,48,0,0
        .byte 0,0,97,134,12,48,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,48,97,134,0,0
        .byte 0,0,0,0,0,0,0,0,1,64,3,192,12,32,4,176,13,32,4,48,3,192,2,128,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,2,64,7,224,8,16,16,8,50,76,17,136,17,136,50,76,16,8,8,16,7,224,2,64,0,0,0,0
        .byte 0,0,4,32,15,240,24,24,48,12,96,134,35,196,34,228,39,68,35,196,97,6,48,12,24,24,15,240,4,32,0,0
        .byte 0,0,0,0,2,64,7,224,8,16,16,8,50,140,17,200,19,136,49,76,16,8,8,16,7,224,2,64,0,0,0,0
        .byte 10,80,22,40,46,116,94,122,188,61,124,62,248,21,0,0,0,0,168,31,124,62,188,61,94,122,46,116,20,40,10,80
        .byte 10,80,20,40,46,116,92,58,185,157,113,142,160,5,13,176,13,176,160,5,113,142,185,157,92,58,46,116,20,40,10,80
        .byte 10,80,20,40,41,148,82,74,161,133,64,2,144,9,41,20,40,148,144,9,64,2,161,133,82,74,41,148,20,40,10,80
        .byte 10,80,20,40,46,116,92,58,184,29,113,142,160,5,4,32,4,32,160,5,113,142,184,29,92,58,46,116,20,40,10,80
        .byte 0,0,0,12,0,12,24,0,24,0,0,0,0,48,0,48,48,0,48,0,0,0,1,128,1,134,0,6,0,0,0,0
        .byte 0,12,24,30,60,30,60,12,24,0,0,0,0,48,48,120,120,48,120,0,48,0,1,134,67,207,225,143,64,6,0,0
        .byte 28,6,62,79,127,31,127,7,62,55,4,121,48,252,120,252,252,120,252,48,121,134,19,207,103,239,247,239,243,207,97,134
        .byte 0,12,24,30,60,30,60,12,24,0,3,0,3,48,48,120,120,120,120,48,48,0,1,134,67,207,225,143,64,6,0,0
        .byte 0,0,0,12,24,12,24,12,24,0,0,0,0,48,0,48,24,48,24,0,24,0,1,134,1,134,49,134,48,6,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,1,64,3,192,12,32,4,176,13,32,4,48,3,192,2,128,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,2,64,7,224,8,16,16,8,48,12,17,8,16,136,48,12,16,8,8,16,7,224,2,64,0,0,0,0
        .byte 0,0,4,32,15,240,24,24,48,12,96,6,32,4,33,132,33,132,32,4,96,6,48,12,24,24,15,240,4,32,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 1,128,3,192,3,192,1,128,0,0,0,0,96,0,144,0,144,0,96,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 1,128,2,64,2,64,1,128,0,0,0,0,0,6,0,15,0,15,0,6,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,6,0,9,0,9,0,6,0,0,0,0,1,128,3,192,3,192,1,128
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,96,0,240,0,240,0,96,0,0,0,0,0,1,128,2,64,2,64,1,128
        .byte 4,32,31,248,32,4,32,5,36,36,160,4,32,4,31,248,0,0,31,248,0,0,51,204,25,152,72,18,1,0,80,10
        .byte 2,64,31,248,32,4,32,4,164,37,32,4,32,4,31,248,0,0,31,248,0,0,51,204,25,152,168,21,0,0,160,133
        .byte 1,128,31,248,32,4,160,4,36,36,32,5,32,4,31,248,0,0,31,248,0,0,51,204,25,152,72,18,1,0,80,10
        .byte 2,64,31,248,32,4,32,4,164,37,32,4,32,4,31,248,0,0,31,248,0,0,51,204,25,152,168,21,0,0,161,5
        .byte 126,254,170,213,247,175,171,133,211,131,162,133,130,255,254,28,56,127,255,65,193,67,161,197,209,195,171,213,213,107,127,126
        .byte 0,0,8,8,28,156,93,220,201,200,224,130,64,19,16,59,57,58,59,144,19,128,65,2,228,39,238,119,78,114,4,32
        .byte 4,0,14,32,68,114,160,37,64,2,0,128,1,72,8,156,156,8,72,0,128,2,1,7,19,146,41,40,16,16,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
        .byte 1,128,2,64,4,32,5,160,4,32,9,144,10,80,11,208,57,156,120,26,253,191,247,255,127,190,63,252,23,232,33,4
        .byte 1,128,2,64,4,32,5,32,4,32,9,144,10,80,11,208,57,156,88,30,253,191,255,247,125,254,63,252,23,232,33,4
        .byte 1,128,2,64,4,32,4,160,4,32,9,144,10,80,11,208,57,156,120,30,221,187,255,255,127,254,62,252,23,232,33,4
        .byte 192,3,228,39,72,18,19,200,38,100,76,50,24,88,25,152,25,152,26,24,76,50,38,100,19,200,72,18,228,39,192,3
        .byte 1,128,0,0,35,196,16,8,7,224,12,48,42,20,169,149,169,149,40,84,12,48,7,224,16,8,35,196,0,0,1,128
        .byte 15,240,63,252,87,234,179,205,51,204,191,253,28,56,113,14,5,32,5,32,6,160,9,80,9,82,70,138,41,68,18,48
        .byte 15,240,63,252,87,234,179,205,51,204,190,125,24,24,113,78,4,160,4,160,5,34,38,81,69,73,41,70,17,48,6,8
        .byte 15,240,63,252,87,234,179,205,51,204,190,125,24,24,114,78,5,160,67,34,134,161,74,89,50,134,2,112,68,10,56,4
        .byte 15,240,63,252,87,234,179,205,51,204,191,253,24,24,113,78,4,160,4,160,5,34,38,81,69,73,41,70,17,48,6,8
        .byte 1,0,3,128,19,128,1,0,0,16,3,128,7,192,15,224,15,224,7,192,3,128,0,8,33,0,3,128,3,128,1,0
        .byte 3,128,7,194,11,160,9,32,4,64,67,128,0,8,3,128,3,128,32,0,3,128,4,64,9,32,11,164,7,192,3,128
        .byte 1,128,3,192,67,192,1,128,0,132,9,128,1,128,2,64,2,64,17,128,1,144,1,0,65,128,2,64,2,64,1,128
        .byte 1,128,3,192,35,192,1,136,1,0,7,224,140,48,11,208,11,208,4,48,7,226,0,128,9,128,2,72,66,64,1,128
frmlst:
        .byte 0,1
        .byte 1,1
        .byte 2,1
        .byte 3,1
        .byte 4,2
        .byte 6,2
        .byte 8,4
        .byte 12,4
        .byte 16,6
        .byte 22,3
        .byte 25,5
        .byte 30,4
        .byte 34,1
        .byte 35,3
        .byte 38,1
        .byte 39,3
        .byte 42,2
        .byte 44,4
        .byte 48,4,52,0
.out .sprintf("SPRITES SIZE = %d", (* - sprgfx))

scdat:
        .word 540,354,488,485,498,489,391,439,501,445,518,458,540,504,475,517,490,438,520,507,465,484,207
        .byte 2,2,2,6,255,2,4,6,255,2,9,6,255,2,5,6,2,2,6,2,6,2,6,24,24,24,6,24,2,24,2,2,24
        .byte 2,6,24,24,2,6,24,24,24,6,255,2,4,6,255,2,5,37,21,30,2,37,36,41,2,37,16,41,2,37,38,41,2
        .byte 37,39,41,2,255,49,7,41,2,2,35,25,2,2,35,17,21,2,44,25,44,2,24,2,17,2,44,2,44,2,255,48,8
        .byte 6,2,42,36,41,2,37,46,43,2,44,35,44,2,39,2,6,37,38,16,43,2,49,255,22,6,49,2,2,2,25,28,2
        .byte 44,25,2,2,44,2,44,2,44,2,24,25,35,2,2,2,48,22,54,55,56,22,22,48,2,39,38,21,43,2,35,2,6
        .byte 2,16,2,39,2,42,38,43,2,17,38,41,2,48,22,22,57,58,59,22,48,2,17,25,25,2,6,25,2,2,2,6,2
        .byte 25,6,2,25,2,2,2,6,53,2,51,255,22,6,51,2,6,24,24,24,2,24,2,24,2,24,2,24,2,24,2,24,6
        .byte 2,2,46,2,51,22,60,61,62,63,22,51,2,2,37,35,41,2,16,2,16,2,37,35,41,2,41,2,21,2,2,2,17
        .byte 2,48,255,22,6,48,6,2,25,17,35,2,42,2,43,2,44,17,39,2,53,24,43,255,2,5,48,22,64,65,66,67,22
        .byte 48,2,2,86,86,53,2,53,2,44,2,35,2,44,6,44,41,2,2,86,86,6,2,49,84,255,22,5,49,37,35,86,86
        .byte 44,2,44,24,44,2,27,2,44,2,44,17,39,2,86,86,2,2,48,48,48,51,51,48,48,48,44,42,39,38,43,2,42
        .byte 46,43,2,26,2,42,38,43,2,42,38,46,38,41,2,42,49,49,48,48,49,49,49,44,25,17,2,2,2,17,25,2,6
        .byte 25,2,17,25,2,6,2,25,17,25,84,2,2,25,17,25,25,25,17,25,44,2,2,6,24,24,2,2,2,24,24,2,2,2
        .byte 24,2,24,2,2,2,84,2,6,2,68,2,6,2,68,2,42,38,36,46,16,38,36,39,38,36,46,45,38,39,38,46,16
        .byte 38,38,38,43,255,2,7,68,2,2,17,25,25,53,255,47,11,53,25,25,25,17,255,2,4,6,255,2,5,68,2,2,44
        .byte 22,9,10,11,12,13,7,8,22,14,15,44,2,6,2,68,255,2,11,6,2,42,38,36,46,45,38,38,38,36,46,45,38
        .byte 43,255,2,5,6,255,2,4,6,2,2
        .byte 255,2,6,37,41,255,2,4,6,255,2,9,39,41,2,2,2,6,2,2,6,255,0,5,27,17,2,255,0,7,18,255,0,5
        .byte 19,44,2,255,0,5,2,0,36,39,35,38,43,2,2,255,0,14,42,35,46,36,40,0,0,2,0,0,255,2,4,6,255,0,16
        .byte 255,2,4,6,0,2,255,0,10,46,46,255,0,4,46,46,255,0,4,18,255,0,6,2,18,255,0,9,82,83,2,0,0,0
        .byte 82,83,2,255,0,10,16,35,38,39,255,0,8,2,2,0,18,0,0,2,6,255,0,6,39,38,35,21,255,2,5,255,0,5
        .byte 18,255,0,12,18,0,0,0,255,2,4,0,0,18,255,0,26,2,255,0,8,16,35,38,38,46,40,40,46,38,38,35,16
        .byte 255,0,6,18,0,0,6,255,0,5,18,0,0,19,17,25,17,25,17,17,25,17,25,17,17,2,255,0,8,2,255,0,9
        .byte 2,6,255,2,4,6,255,2,5,255,0,8,16,35,38,39,255,0,22,39,35,38,36,17,25,17,44,2,255,0,22,2,2
        .byte 6,2,0,0,35,2,255,0,6,18,255,0,9,18,255,0,8,2,18,0,27,2,255,0,8,37,16,21,41,255,0,13,2
        .byte 0,0,53,2,0,18,255,0,6,44,17,17,53,2,255,0,8,37,16,0,0,2,0,0,19,2,255,0,8,42,2,6,43
        .byte 2,255,0,6,18,0,42,17,2,0,6,0,0,0,6,255,0,4,35,41,0,0,0,2,0,0,2,0,37,35,255,0,6
        .byte 2,6,0,2,255,0,8,19,43,2,255,0,7,42,17,2,255,0,8
        .byte 24,37,35,36,39,38,35,41,22,37,255,2,5,6,255,2,6,16,41,2,6,255,2,4,45,43,25,17,17,17,25,44,23
        .byte 44,2,255,0,11,25,25,2,0,0,0,18,0,25,25,255,2,5,16,22,42,36,38,46,38,35,0,0,0,37,16,38,36
        .byte 40,16,2,0,35,41,0,0,2,40,2,18,0,0,0,44,38,53,25,17,17,17,42,86,0,0,44,25,17,17,25,43,2
        .byte 0,19,17,2,0,2,16,2,0,37,36,24,39,25,39,255,2,5,86,0,0,0,255,2,6,0,0,2,2,0,6,53,2
        .byte 0,19,17,25,17,2,17,2,0,0,0,18,255,0,15,2,44,2,0,0,255,2,6,255,0,10,18,255,0,8,37,43,2
        .byte 255,0,15,35,255,0,11,44,25,2,255,0,5,37,35,38,36,39,36,38,16,0,0,42,35,255,0,4,37,35,0,0,18
        .byte 0,42,41,2,0,0,0,18,0,25,25,25,17,17,17,25,53,2,0,19,17,2,0,0,0,19,17,2,0,0,0,2,44
        .byte 6,0,16,21,0,0,37,41,255,2,5,44,2,0,0,2,2,255,0,4,2,2,0,0,0,2,26,2,0,25,25,2,0
        .byte 80,81,6,255,0,4,44,2,255,0,13,2,27,2,0,27,27,2,0,46,46,2,0,0,37,21,43,2,0,18,0,0,0
        .byte 45,21,38,39,38,40,21,41,2,44,2,0,44,53,2,0,82,83,2,0,0,0,2,2,2,255,0,6,255,2,6,31,24
        .byte 16,24,37,38,43,2,0,28,28,2,0,18,255,0,16,40,37,43,25,17,42,17,2,0,25,25,2,0,39,38,36,16,45
        .byte 46,38,41,0,0,18,0,37,21,41,0,0,44,39,25,255,2,5,0,27,27,2,0,19,17,25,53,2,2,39,43,2,0,0,0
        .byte 40,25,17,2,0,27,42,41,2,18,255,0,4,42,43,2,0,0,2,2,44,2,0,19,17,2,0,0,0,53,2,2,2
        .byte 0,44,6,42,38,36,46,36,35,38,16,41,2,255,0,4,16,2,0,0,2,2,0,0,0,27,2,0,0,0,35,2,2
        .byte 255,25,7,42,38,36,39,45,35,43,2,0,0,0,18,0,0,0,39,35,38,45,35,43
        .byte 24,24,24,255,2,6,6,2,2,2,44,255,2,7,35,41,255,2,6,41,37,35,41,255,0,10,25,2,0,18,255,0,4
        .byte 19,17,2,0,0,0,18,0,44,25,17,16,38,39,38,36,0,0,0,37,38,16,41,2,0,35,38,41,0,0,0,2,2
        .byte 0,37,36,0,0,39,2,2,53,255,2,5,0,0,19,17,2,44,2,0,0,2,44,2,255,0,5,44,25,2,0,44,2
        .byte 0,44,2,255,0,7,2,2,40,2,0,0,0,42,38,39,39,38,41,37,35,38,38,16,43,6,0,42,16,38,38,41,255,0,7
        .byte 39,36,0,0,0,255,2,4,42,43,255,2,6,0,25,2,2,42,44,2,255,0,6,19,17,2,255,0,6,19,17,2,255,0,4
        .byte 2,0,41,2,0,19,42,27,46,27,35,41,0,18,0,2,2,0,0,37,41,0,0,0,2,2,0,39,0,0,2,0,44
        .byte 2,0,0,255,2,5,21,2,255,0,6,46,46,2,0,18,0,0,0,27,2,0,2,0,35,2,255,0,5,18,0,42,38
        .byte 16,0,0,0,35,38,82,83,35,39,40,36,39,38,43,16,40,6,0,42,40,38,39,38,35,41,0,0,0,2,44,2,0,0,0
        .byte 255,2,13,0,0,255,2,5,35,2,0,0,0,39,2,255,0,15,2,255,0,7,42,38,41,0,0,0,2,0,0,0,37
        .byte 39,0,0,0,18,0,0,37,35,0,0,37,38,39,38,21,41,0,18,0,37,44,2,255,0,6,19,17,2,255,0,5,19
        .byte 17,2,0,35,22,47,22,22,44,2,0,0,39,42,36,255,0,4,18,0,0,2,2,0,80,81,0,0,0,2,2,0,39
        .byte 38,35,39,38,39,41,0,0,0,2,2,2,255,0,9,19,17,2,255,0,5,44,25,17,25,17,25,27,2,255,0,10,80
        .byte 81,0,0,0,2,2,255,0,5,21,255,2,7,0,0,27,0,0,18,255,0,4,82,83,2,255,0,9,53,2,0,0,0
        .byte 18,255,0,4,44,2,0,0,37,41,0,0,0,2,2,0,37,35,0,0,0,18,0,0,42,40,38,46,38,35,39,40,38
        .byte 35,43,2,0,0,25,17,2,255,0,5,19,25,2,255,0,5
        .byte 37,38,39,38,35,36,2,2,6,255,2,8,37,41,255,2,7,6,2,2,16,255,2,7,255,0,10,53,44,2,255,0,9
        .byte 39,2,255,0,8,37,38,35,39,38,41,0,0,44,35,21,36,46,45,38,35,38,41,0,0,44,2,0,37,0,18,0,35
        .byte 0,0,42,36,38,41,25,17,2,0,46,53,255,2,9,0,17,6,0,16,2,0,0,44,2,0,0,2,25,44,2,2,2
        .byte 0,35,44,2,255,0,9,24,2,0,42,2,0,37,43,2,0,0,0,37,43,2,0,0,0,82,83,2,0,37,38,38,35
        .byte 38,38,45,38,39,2,0,0,2,0,39,37,38,39,0,0,40,25,2,0,39,38,43,39,2,0,0,2,2,39,6,2,2,2
        .byte 35,2,255,0,4,44,17,25,17,2,0,42,41,2,0,19,25,25,17,2,18,0,0,0,25,2,0,0,0,40,37,46,41
        .byte 0,0,19,2,6,2,2,0,25,44,2,0,0,0,37,38,39,38,41,0,0,37,45,39,0,0,53,17,25,17,2,0,0
        .byte 2,255,0,4,37,43,2,0,0,0,44,47,47,47,44,2,0,21,2,2,2,0,44,2,2,2,6,255,0,4,45,35,38
        .byte 46,38,36,16,38,38,39,38,35,38,43,2,18,53,2,0,0,18,39,2,255,0,8,2,2,44,255,2,5,6,255,2,5
        .byte 0,44,2,0,0,0,44,2,18,255,0,5,18,0,0,0,27,2,255,0,11,16,2,0,0,0,35,2,0,37,38,45,41
        .byte 255,0,5,39,2,0,39,38,36,45,38,40,35,36,45,35,46,35,36,0,0,44,2,0,44,47,47,44,2,255,0,4,44
        .byte 2,0,0,255,2,13,0,27,2,0,42,36,38,39,2,0,39,0,0,35,2,255,0,8,18,255,0,7,44,2,0,19,25,25
        .byte 53,2,0,44,6,0,42,35,38,45,46,36,38,39,41,255,0,5,37,38,46,38,43,2,0,0,18,37,16,2,0,44,2
        .byte 0,0,255,2,9,255,0,4,40,47,22,47,46,6,0,0,0,42,41,2,0,35,2,255,0,5,18,255,0,6,39,0,18
        .byte 44,22,22,22,44,21,35,38,46,38,43,2,0,42,21,38,45,39,255,0,4,37,16,38,45,38,43,2,0,42,38,45,16
        .byte 43
        .byte 255,2,14,44,23,22,22,22,23,22,53,255,2,4,44,22,22,23,2,255,0,5,18,255,0,7,42,38,45,39,36,38,36
        .byte 43,2,18,0,0,27,23,22,22,2,0,45,39,35,40,255,0,5,41,0,0,0,255,2,8,0,37,16,42,41,23,22,2
        .byte 0,0,255,2,4,255,0,4,44,2,18,255,0,10,44,2,2,39,22,23,2,255,0,10,42,38,39,35,41,37,35,39,38
        .byte 36,46,45,16,43,2,0,0,39,35,2,0,18,0,0,0,35,16,41,0,0,0,2,2,2,44,53,255,2,8,255,0,4
        .byte 2,2,255,0,6,2,44,2,0,0,0,18,0,42,43,2,255,0,12,2,255,0,7,39,2,0,79,79,0,0,19,17,2
        .byte 0,35,41,0,0,18,0,37,35,41,18,0,2,255,0,5,18,0,25,2,18,82,83,2,0,0,2,2,0,42,35,41,2
        .byte 0,37,43,23,44,2,0,37,36,38,41,255,0,4,39,2,0,0,2,2,255,0,6,2,44,45,21,36,38,39,43,2,0
        .byte 39,25,35,39,2,0,0,0,44,2,255,0,8,18,0,0,44,17,255,2,6,0,2,2,17,17,2,0,0,0,42,38,46
        .byte 38,41,37,38,41,39,35,41,0,0,27,24,0,0,18,255,0,4,2,0,0,2,2,255,0,4,2,17,2,42,43,25,44,44
        .byte 25,17,2,0,35,36,38,36,38,36,38,35,43,2,255,0,9,24,255,2,4,42,43,2,2,2,0,53,25,17,39,255,2,6
        .byte 0,0,0,18,255,0,5,39,2,18,0,0,37,41,2,0,0,0,39,2,2,44,2,255,0,4,2,255,0,9,35,24,41
        .byte 2,0,39,35,2,0,42,36,2,2,0,42,40,45,41,18,0,2,0,37,36,35,46,255,0,4,42,45,43,2,0,53,44
        .byte 2,0,0,2,2,0,0,0,2,2,42,2,0,2,0,39,17,42,43,2,0,18,0,0,2,2,2,0,44,27,2,255,0,5
        .byte 18,255,0,4,2,0,2,0,0,255,2,4,255,0,8,35,42,45,41,24,37,35,36,38,41,0,0,18,0,0,2,18,255,0,6
        .byte 37,45,38,21,36,38,45,43,22,47,42,36,43,23,47,22,42,38,39,36,35,41
        .byte 255,2,4,37,41,255,2,6,45,35,39,39,35,36,255,2,6,37,41,255,2,5,0,0,0,42,17,2,0,18,255,0,4
        .byte 255,2,6,255,0,5,19,43,2,0,18,0,37,36,0,0,0,2,2,255,0,16,18,0,2,2,0,45,41,44,17,2,255,0,11
        .byte 80,81,0,0,18,255,0,9,19,44,43,2,2,255,0,7,18,0,0,0,82,83,2,255,0,12,42,2,2,255,0,13,2,2
        .byte 255,0,12,18,2,0,18,255,0,5,37,41,255,0,10,37,41,0,0,0,18,255,0,4,2,255,0,5,18,0,19,43,2
        .byte 255,0,5,18,0,0,0,42,17,2,255,0,7,2,255,0,8,2,2,255,0,10,2,2,255,0,7,2,255,0,11,18,0
        .byte 35,35,255,0,14,2,0,0,0,37,39,255,0,4,45,46,38,45,43,42,36,38,46,36,0,0,18,0,39,41,255,0,4
        .byte 2,0,0,0,44,17,2,255,0,4,255,2,10,0,0,0,19,44,2,0,18,0,2,0,18,0,0,2,2,255,0,18,2,2
        .byte 0,0,0,2,255,0,9,18,0,0,0,37,41,0,0,0,18,255,0,10,80,81,255,0,5,37,69,69,41,0,0,0,19
        .byte 17,2,0,0,37,69,69,41,255,0,5,80,81,82,83,2,255,0,4,42,82,83,43,2,0,0,0,2,2,0,0,42,82
        .byte 83,43,2,0,0,18,0,82,83,2,2,2,0,0,18,0,0,255,2,4,255,0,5,18,0,0,255,2,4,255,0,5,2,2
        .byte 255,0,29,2,18,0,0,37,41,255,0,6,37,255,0,4,41,255,0,4,18,0,37,41,0,0,0,18,2,0,0,0,44
        .byte 17,2,0,0,0,18,37,35,2,0,0,0,35,41,255,0,5,19,44,2,0,0,0
        .byte 255,2,5,44,255,2,18,44,255,2,6,255,0,4,19,2,255,0,6,18,255,0,10,19,2,255,0,4,37,41,255,0,4
        .byte 2,0,0,18,255,0,15,2,0,0,37,41,16,17,2,255,0,10,37,38,38,35,41,0,0,18,255,0,7,19,35,53,2,2
        .byte 255,0,9,37,43,40,22,22,42,41,255,0,10,53,27,2,255,0,4,18,255,0,4,37,43,47,44,22,22,22,42,41,255,0,6
        .byte 18,0,0,27,44,2,255,0,8,37,43,22,22,35,22,22,23,22,42,41,255,0,8,44,27,2,255,0,7,37,43,22,23
        .byte 22,27,255,22,5,42,41,255,0,7,27,39,2,0,0,45,39,35,38,45,43,255,22,4,46,255,22,6,40,2,18,255,0,5
        .byte 39,17,2,0,0,0,2,2,2,255,22,6,44,22,23,255,22,5,2,255,0,6,17,24,2,255,0,4,18,0,22,22,22
        .byte 23,22,22,27,255,22,7,2,255,0,4,18,0,24,35,2,255,0,6,40,255,22,5,42,41,22,22,23,22,37,38,45,46
        .byte 38,35,41,0,0,35,44,2,255,0,6,42,41,22,22,22,23,22,35,255,22,4,44,255,2,5,44,2,0,44,27,2,255,0,7
        .byte 42,41,255,22,4,39,22,22,22,37,43,2,255,0,4,27,2,0,27,44,2,0,0,0,18,255,0,4,42,41,22,22,22
        .byte 53,22,22,37,43,17,45,35,41,0,18,44,2,0,44,39,2,255,0,9,42,41,22,22,44,37,40,43,255,2,4,44,2
        .byte 0,39,2,0,39,53,2,255,0,5,18,255,0,4,42,38,45,38,43,2,2,2,0,0,0,27,2,0,19,2,0,44,40
        .byte 2,0,37,39,41,255,0,7,255,2,5,0,0,18,0,0,44,2,0,0,2,0,40,35,2,0,44,47,35,2,255,0,16
        .byte 39,2,0,0,0,18,16,42,35,38,43,22,42,38,36,45,36,46,36,35,36,40,36,38,46,35,36,45,36,41,17,37,45
        .byte 38,45,16,43
        .byte 37,31,80,84,84,81,255,31,8,80,84,84,81,31,31,80,84,84,81,80,84,84,81,31,41,50,50,82,83,82,83,50,255,30,7
        .byte 82,83,82,83,30,30,82,83,82,83,82,83,82,83,30,30,50,49,255,2,17,50,255,2,8,49,50,50,49,2,255,0,4
        .byte 18,255,0,11,30,2,255,0,7,49,30,30,51,2,0,255,31,7,41,255,0,5,32,51,30,2,0,80,81,0,18,0,0
        .byte 49,30,30,51,2,0,42,50,50,50,82,83,30,30,2,0,0,0,18,32,48,30,2,0,51,50,2,0,0,0,48,30,30
        .byte 48,2,0,0,2,50,49,2,2,49,50,2,255,0,5,49,50,2,18,49,50,2,0,0,18,51,30,30,48,2,0,18,0
        .byte 42,48,2,0,49,50,2,255,0,5,255,31,4,48,43,2,0,0,0,51,50,30,48,2,255,0,4,2,2,0,48,30,2
        .byte 18,255,0,4,25,25,82,83,43,2,2,0,0,0,48,30,30,49,2,255,0,7,51,81,2,255,0,4,18,50,50,255,2,4
        .byte 18,0,0,0,48,50,30,31,80,84,31,41,0,18,0,0,48,83,2,255,0,5,49,50,2,255,0,7,48,50,30,50,82
        .byte 83,30,30,2,0,0,0,48,30,2,0,84,84,0,0,51,30,2,255,0,7,48,30,30,49,2,2,49,50,2,0,0,0
        .byte 49,30,2,0,82,83,2,0,48,30,2,0,48,41,255,0,4,48,50,30,49,2,0,48,50,2,0,0,0,49,50,2,0,0
        .byte 2,2,0,49,81,2,0,49,50,2,0,0,0,48,30,30,51,2,0,0,2,2,0,80,81,31,31,2,255,0,5,49,83
        .byte 2,0,49,43,2,0,0,0,48,30,30,48,2,0,0,18,0,0,42,50,30,30,2,0,0,0,18,0,31,30,80,81,43
        .byte 2,6,0,0,0,48,30,30,48,2,255,0,6,2,25,50,2,18,255,0,4,42,30,50,43,2,2,255,0,4,48,30,50
        .byte 48,2,255,0,7,48,30,2,255,0,6,255,2,4,0,0,0,37,49,49,50,50,49,2,0,255,5,6,49,30,5,5,255,0,7
        .byte 18,255,0,4,25,51,49,50,42,26,80,81,255,26,8,80,84,82,83,84,81,26,26,26,80,81,80,81,80,81,26,26,43
        .byte 37,255,31,4,50,31,31,50,255,31,4,82,49,84,48,83,255,31,4,50,31,31,50,31,31,31,41,50,255,2,8,6,255,2,5
        .byte 6,255,2,9,6,2,2,2,50,30,2,255,0,20,18,255,0,5,18,30,30,2,0,18,0,0,18,255,0,4,80,81,255,0,4
        .byte 18,255,0,11,30,31,6,255,0,9,48,30,2,255,0,7,18,0,0,26,0,0,18,0,50,50,2,255,0,7,37,31,31,31
        .byte 2,0,84,31,31,31,49,255,0,4,31,2,0,0,0,26,30,41,0,0,0,18,0,0,0,30,30,30,43,2,0,42,30
        .byte 26,50,48,2,255,0,4,6,0,0,37,50,26,49,2,255,0,4,18,0,30,49,255,22,6,51,31,31,81,255,0,7,49
        .byte 50,31,48,6,255,0,6,42,84,22,22,23,22,22,22,42,25,50,83,6,255,0,6,48,30,50,51,2,0,26,255,0,6
        .byte 255,22,6,2,6,2,2,2,0,0,26,0,18,0,51,30,80,81,2,0,31,2,255,0,5,255,22,6,0,0,0,18,0,0,0
        .byte 31,2,0,0,80,81,82,83,2,0,50,6,0,80,31,31,41,255,22,4,23,22,37,31,255,0,6,6,0,0,82,83,50
        .byte 49,2,0,26,2,0,82,49,30,84,255,22,6,30,49,2,0,0,18,255,0,5,49,50,26,49,2,0,31,2,0,0,51
        .byte 30,31,31,31,2,0,37,31,84,48,2,255,0,8,42,50,31,48,6,0,30,2,0,0,42,30,49,25,43,2,0,30,30
        .byte 25,43,2,255,0,8,2,31,50,48,2,0,0,2,0,0,0,255,2,5,0,30,49,2,2,6,255,0,9,50,30,43,2
        .byte 0,18,255,0,10,82,83,6,255,0,4,18,0,0,18,0,0,0,50,31,6,2,255,0,9,18,0,0,0,2,2,255,0,11
        .byte 30,50,2,0,18,255,0,5,18,255,0,19,30,42,255,26,4,50,26,26,26,50,26,26,26,80,49,84,48,81,255,26,4
        .byte 50,26,26,50,26,26,26,43
        .byte 37,255,31,28,41,31,49,2,2,26,26,2,2,6,2,2,2,26,26,6,2,2,6,2,2,26,26,2,2,6,2,2,2
        .byte 49,31,26,49,2,0,30,49,2,255,0,5,53,31,2,255,0,5,84,44,2,255,0,5,49,26,31,48,6,0,30,48,6
        .byte 0,80,81,0,18,44,49,2,0,37,26,0,18,49,26,6,0,31,31,0,18,80,31,50,48,2,0,82,83,2,0,84,31
        .byte 2,0,31,51,2,0,31,31,2,0,48,26,2,0,49,53,2,0,82,50,30,51,2,0,0,2,2,0,44,49,2,0,31
        .byte 48,2,18,49,31,2,0,42,26,30,26,30,26,2,0,51,30,26,51,2,0,18,0,0,0,44,81,6,0,26,26,2,0
        .byte 48,81,2,0,0,255,2,6,0,51,26,31,48,2,255,0,5,31,83,2,0,31,43,6,0,48,83,2,255,0,6,18,0,0
        .byte 48,31,50,48,2,0,0,0,18,0,31,48,2,0,0,2,2,0,51,2,2,0,18,255,0,4,85,80,26,48,50,30,81
        .byte 2,0,80,41,0,0,26,26,2,0,18,0,0,0,48,41,2,255,0,6,85,82,31,48,30,26,83,2,0,31,31,2,0
        .byte 82,83,2,255,0,5,48,84,2,255,0,4,18,0,0,2,2,48,26,31,48,6,0,30,30,2,0,0,2,2,255,0,5
        .byte 49,31,6,0,80,81,255,0,6,48,31,50,48,2,0,30,49,2,0,18,0,0,0,26,26,0,0,26,26,2,0,26,26
        .byte 80,81,80,81,0,0,51,50,30,51,2,0,30,48,2,255,0,5,31,31,2,0,31,53,2,0,42,255,31,5,2,0,51
        .byte 30,26,51,2,0,82,51,2,0,5,5,0,18,31,49,2,0,31,44,2,0,0,2,2,2,84,53,2,0,48,26,31,48
        .byte 2,18,0,48,2,0,26,26,0,0,80,48,6,0,26,26,2,18,255,0,4,48,30,2,18,48,31,50,48,2,0,80,48
        .byte 2,0,47,47,2,0,82,31,2,0,80,31,2,0,0,0,18,0,51,30,2,0,48,50,30,49,2,0,50,49,2,0,51
        .byte 49,2,0,0,2,2,18,82,31,2,0,26,26,0,0,48,30,2,0,49,30,26,49,2,0,30,31,2,0,31,31,2,18
        .byte 255,0,4,31,31,2,32,26,49,2,0,82,83,6,0,49,26,42,255,26,28,43
        .byte 255,2,5,6,255,2,8,51,41,255,2,11,6,2,2,2,18,255,0,6,5,5,0,18,0,0,25,44,2,0,0,0,5,5
        .byte 255,0,8,49,48,26,41,0,0,0,32,26,26,255,0,4,48,27,2,0,0,0,26,26,0,0,18,32,26,41,0,0,29
        .byte 31,46,29,2,0,18,32,27,29,2,0,0,0,25,29,2,18,0,0,49,29,2,0,0,32,46,29,2,0,25,42,48,26
        .byte 2,0,0,32,48,43,2,0,0,0,49,26,2,0,0,0,48,43,6,0,0,32,49,43,6,0,255,2,4,6,255,0,4
        .byte 2,6,255,0,4,6,2,255,0,4,2,2,255,0,4,2,2,0,2,255,0,11,18,255,0,10,18,255,0,6,2,0,18
        .byte 255,0,27,2,255,0,4,48,41,0,18,255,0,10,18,0,0,0,5,5,255,0,5,6,255,0,4,26,26,0,0,0,32
        .byte 26,41,255,0,4,37,26,0,0,0,32,26,30,0,0,18,0,32,36,46,45,29,37,46,29,2,0,0,32,46,29,2,18
        .byte 0,0,25,44,2,0,0,32,46,29,2,0,0,0,32,48,48,26,43,42,48,43,6,0,0,32,28,43,2,0,0,0,42
        .byte 48,2,0,0,32,48,83,6,255,0,4,2,6,255,2,6,255,0,4,2,6,255,0,4,6,2,255,0,4,2,2,255,0,4
        .byte 2,0,0,0,18,255,0,16,18,255,0,8,2,0,5,5,255,0,10,5,5,255,0,14,2,0,46,46,255,0,4,80,81
        .byte 0,0,18,0,31,29,255,0,4,80,81,0,0,18,0,46,46,0,18,2,0,25,25,6,0,0,0,79,84,2,0,0,0
        .byte 46,46,2,0,0,0,79,84,2,0,0,0,25,25,6,0,2,0,82,83,2,0,0,0,82,83,6,0,0,0,82,83,2
        .byte 0,18,0,82,83,2,0,0,0,82,83,2,0,6,0,0,6,2,0,18,0,0,2,2,255,0,4,2,6,255,0,4,6
        .byte 2,255,0,4,6,2,0,2,255,0,4,5,5,5,255,0,6,18,255,0,8,5,5,5,255,0,4
        .byte 37,50,30,50,36,38,45,50,30,82,83,50,26,26,50,46,50,26,50,30,50,30,50,36,84,45,50,30,50,41,31,6,255,2,8
        .byte 6,255,2,11,6,255,2,6,31,50,2,0,18,255,0,9,18,255,0,11,18,0,0,0,50,81,2,0,37,52,36,0,0,0
        .byte 37,31,30,27,49,27,50,30,41,0,18,0,0,49,46,29,27,41,0,0,80,83,2,0,24,2,2,2,18,0,30,43,255,2,6
        .byte 44,2,255,0,4,2,2,2,49,2,0,82,30,2,0,31,2,255,0,4,47,2,6,255,0,5,49,2,0,0,18,255,0,4
        .byte 49,2,18,30,26,2,18,48,2,0,37,26,26,46,49,48,41,0,0,18,0,48,29,30,29,30,36,41,0,0,48,2,0
        .byte 26,31,2,0,29,6,0,48,6,255,2,4,27,2,0,45,45,50,255,2,5,49,2,0,48,2,0,31,53,2,0,49,2
        .byte 0,48,2,0,0,0,18,26,6,0,0,2,51,2,0,0,18,0,48,2,0,51,2,0,53,44,2,0,48,2,18,46,2
        .byte 0,39,0,0,29,2,0,0,0,48,2,0,28,0,0,48,2,0,51,6,0,44,44,6,0,26,2,0,0,2,0,27,2
        .byte 0,49,2,0,0,18,49,2,0,46,2,0,51,2,0,48,2,0,44,26,2,0,47,2,0,0,0,18,53,2,0,48,2
        .byte 0,46,45,50,6,0,26,2,18,48,2,18,48,2,0,26,31,2,0,31,6,255,0,4,27,2,0,51,2,0,0,2,49
        .byte 2,0,84,2,0,31,2,0,49,2,0,31,30,2,0,39,2,0,46,36,38,46,38,45,48,2,0,0,0,48,2,18,48
        .byte 2,0,39,2,0,49,2,18,50,81,2,0,48,2,0,0,255,2,5,49,2,18,0,0,51,2,0,48,2,0,0,2,0
        .byte 47,2,0,80,83,2,18,49,2,0,18,255,0,5,49,2,0,0,0,49,2,0,49,2,0,18,0,0,27,2,0,84,50
        .byte 2,0,42,30,84,36,0,0,18,0,0,26,2,0,45,29,4,29,50,47,30,36,36,46,36,43,2,0,82,50,2,0,0
        .byte 255,2,4,255,0,4,47,2,0,0,255,2,7,6,2,2,2,6,0,30,26,6,255,0,4,18,255,0,5,29,6,0,0
        .byte 18,0,0,0,18,255,0,8,26,42,50,80,81,50,46,50,50,80,81,30,50,43,2,37,38,45,30,30,50,46,50,30,80
        .byte 84,81,50,30,50,43
        .byte 37,49,48,51,48,49,49,50,30,49,39,49,30,50,38,46,38,43,25,42,45,46,36,48,49,36,46,45,49,41,25,2,2,2
        .byte 6,255,2,10,49,255,2,5,49,255,2,4,6,2,2,51,27,2,255,0,6,18,255,0,6,48,2,0,18,0,0,48,2
        .byte 255,0,6,49,39,2,0,37,30,45,41,255,0,8,51,2,0,45,46,38,43,2,0,0,0,37,0,0,48,53,2,18,49
        .byte 2,2,51,2,0,48,0,0,49,0,0,48,2,18,0,2,2,6,2,0,18,0,49,2,0,49,49,2,0,0,2,0,0
        .byte 2,0,42,30,45,43,2,0,49,2,255,0,9,48,6,0,51,43,6,0,0,0,18,255,0,4,255,2,4,0,49,39,2
        .byte 0,0,0,37,45,41,0,0,49,2,0,46,25,2,255,0,11,18,0,42,43,6,0,0,0,39,25,48,2,0,51,2,18
        .byte 47,37,46,41,24,51,48,37,38,45,38,41,255,0,5,2,2,255,0,4,42,49,50,38,43,2,0,44,53,22,53,38,39
        .byte 38,43,25,48,51,42,49,255,0,8,18,0,0,2,6,2,2,2,0,39,44,23,44,255,2,5,6,255,2,4,0,0,18
        .byte 255,0,11,18,0,47,42,46,43,2,255,0,8,42,36,38,36,41,255,0,4,37,30,48,46,48,30,38,39,41,37,2,2,2
        .byte 0,18,255,0,7,2,2,2,44,6,0,48,24,44,2,6,2,2,2,6,2,53,49,2,255,0,6,5,0,0,18,255,0,4
        .byte 39,2,0,49,46,43,2,0,0,18,0,0,0,27,43,2,0,37,51,48,41,24,37,49,41,24,37,48,41,24,53,2,0,0
        .byte 2,2,2,0,37,51,41,0,0,42,24,6,0,42,46,25,42,46,43,25,42,46,43,25,42,46,43,2,0,0,18,0,0,0
        .byte 44,46,53,2,18,47,46,2,0,0,33,2,2,33,2,6,2,33,2,6,2,33,2,2,255,0,4,37,51,48,49,43,2
        .byte 0,37,27,2,0,18,255,0,12,18,255,0,5,39,6,255,2,4,0,53,53,2,255,0,6,18,255,0,10,18,0,0,44
        .byte 2,0,0,18,0,0,27,42,49,48,51,48,49,41,37,46,36,30,50,49,48,51,51,48,49,50,30,36,38,43,24,37,36
        .byte 48,46,45,43
        .byte 255,2,5,35,22,22,23,255,22,8,23,255,22,6,35,255,2,5,6,0,0,0,18,16,255,22,6,23,255,22,8,23,22,22
        .byte 16,6,0,0,18,0,2,0,16,0,0,42,16,35,52,46,34,52,35,16,16,16,21,35,52,34,46,52,35,16,43,2,0
        .byte 16,0,0,2,0,21,2,0,0,6,2,2,53,2,2,6,2,53,16,2,2,6,255,2,4,6,2,2,0,21,2,0,2
        .byte 0,35,2,255,0,5,44,6,0,0,0,44,53,2,255,0,10,35,2,18,2,0,53,6,255,0,5,42,46,51,0,0,35
        .byte 44,2,0,37,51,36,255,0,6,53,2,0,2,18,44,2,255,0,6,2,2,2,0,25,35,2,0,53,6,2,2,255,0,5
        .byte 44,2,0,2,0,29,2,0,0,18,255,0,7,35,25,49,48,44,2,0,18,0,0,0,18,0,29,6,0,2,0,34,2
        .byte 255,0,7,18,0,0,44,35,2,6,46,2,255,0,7,34,2,18,2,0,46,2,255,0,10,35,53,2,0,43,2,255,0,7
        .byte 46,2,0,2,0,34,6,255,0,10,25,35,2,0,0,2,0,0,0,18,0,0,0,34,2,0,6,0,29,2,0,0,0
        .byte 18,255,0,6,35,25,2,0,0,18,255,0,7,29,2,18,2,0,34,36,46,41,255,0,8,53,35,6,255,0,10,34,2
        .byte 0,2,0,53,2,2,44,6,255,0,7,44,53,2,255,0,10,53,6,0,2,0,44,2,0,19,2,0,0,0,18,0,0,0
        .byte 16,44,2,0,0,18,0,0,37,46,36,0,0,44,2,0,2,0,35,6,0,0,2,255,0,5,37,16,43,42,16,41,255,0,4
        .byte 53,2,2,2,0,35,2,0,2,0,16,2,255,0,8,21,6,2,2,2,21,2,0,0,0,44,6,0,0,0,16,2,18
        .byte 2,0,42,21,35,52,34,39,34,52,35,16,43,2,0,0,18,42,16,35,52,34,39,34,52,35,21,43,2,0,6,0,0
        .byte 255,2,5,6,255,2,5,255,0,4,255,2,5,6,2,2,2,6,2,0,2,0,0,0,18,255,0,6,18,255,0,9,18
        .byte 255,0,4,18,0,0,0
        .byte 21,21,30,27,27,29,29,30,30,27,27,31,31,30,30,21,30,30,27,27,29,29,31,31,30,30,27,27,21,21,21,2,2
        .byte 6,255,2,5,30,2,2,2,6,255,2,10,6,255,2,4,21,30,2,255,0,6,18,26,2,255,0,7,18,255,0,10,30
        .byte 28,2,0,37,38,30,30,28,28,39,29,30,41,0,18,0,0,37,16,30,30,30,39,38,36,35,41,0,0,31,31,2,18
        .byte 26,255,22,5,23,22,22,27,2,0,0,0,27,255,22,4,23,22,22,23,27,2,0,29,30,2,0,27,22,23,22,23,22,22
        .byte 23,22,42,41,0,0,0,26,22,23,22,22,22,23,22,22,40,2,0,28,26,2,0,28,255,22,4,23,22,22,22,37,43
        .byte 2,0,0,31,22,22,22,23,255,22,4,26,2,0,30,27,6,0,42,31,41,23,255,22,4,23,30,2,2,0,0,35,22
        .byte 23,255,22,4,23,22,27,2,18,30,30,2,0,0,2,42,21,21,30,30,29,29,43,2,0,0,0,42,29,27,28,22,23
        .byte 22,28,27,43,2,0,28,21,2,0,18,0,2,2,6,255,2,6,255,0,4,2,2,6,255,2,7,0,21,21,2,255,0,13
        .byte 18,255,0,11,18,0,21,28,2,0,37,26,28,28,30,22,22,22,41,255,0,4,37,21,21,30,30,30,29,38,41,255,0,4
        .byte 29,27,6,0,16,22,22,22,23,22,22,23,16,0,18,0,0,28,255,22,5,23,22,30,38,41,0,0,30,30,2,0,16
        .byte 22,23,22,22,23,22,22,42,41,0,0,0,27,22,23,255,22,7,25,2,0,28,26,2,0,30,22,22,23,22,22,23,22,22
        .byte 29,2,0,0,42,41,22,22,23,22,22,22,23,22,37,2,0,29,29,2,0,28,23,22,22,23,22,22,22,23,28,2,0,0,0
        .byte 27,22,23,22,22,23,22,22,23,27,2,18,31,30,2,0,42,29,27,27,39,30,30,21,21,43,2,0,0,0,42,38,30
        .byte 39,30,30,16,28,38,43,2,0,30,30,6,0,18,255,2,8,6,2,0,0,18,0,2,2,28,2,6,255,2,5,0,30
        .byte 21,2,255,0,6,18,255,0,10,18,31,2,255,0,4,18,0,0,21,21,21,30,31,31,27,27,27,29,29,31,30,30,31
        .byte 21,21,29,30,30,29,27,27,27,29,29,31,31,30,21,21
        .byte 255,22,30,37,16,16,41,22,23,255,22,5,23,255,22,5,37,16,34,48,34,16,41,16,34,48,34,16,41,25,2,2,35
        .byte 255,22,13,16,6,255,2,4,16,2,2,6,2,2,16,26,2,0,26,22,22,22,23,255,22,6,23,22,22,21,2,255,0,4
        .byte 51,2,255,0,4,21,21,2,18,21,255,22,13,34,2,0,49,18,0,51,2,0,48,0,0,26,24,2,0,46,255,22,4
        .byte 16,36,45,16,255,22,5,34,2,0,48,2,0,49,2,18,49,2,0,48,26,2,0,25,255,22,4,34,22,22,34,255,22,5
        .byte 34,2,18,48,2,0,48,2,0,49,2,0,48,34,2,0,28,255,22,4,46,22,22,46,22,23,22,22,22,16,2,0,51
        .byte 2,0,48,2,0,51,2,0,48,31,2,0,30,22,23,255,22,5,35,255,22,5,42,21,41,51,2,0,0,2,18,51,2
        .byte 0,31,27,6,0,16,255,22,7,34,255,22,7,34,31,2,18,0,0,0,31,2,18,16,21,2,0,42,16,30,34,16,43
        .byte 46,46,39,255,22,4,23,22,22,42,36,46,34,45,46,45,43,2,2,16,26,2,0,2,255,22,9,23,255,22,9,23,255,22,4
        .byte 23,21,39,2,0,2,22,22,22,23,255,22,11,23,255,22,9,34,31,2,18,2,23,22,37,38,41,46,46,37,16,34,30
        .byte 38,34,16,38,21,34,30,34,21,41,22,23,22,22,28,52,2,0,37,34,30,34,41,34,2,2,34,255,2,6,39,2,2
        .byte 6,2,2,21,22,22,22,23,27,21,6,0,33,33,33,32,34,26,2,0,35,2,0,18,0,0,0,51,2,0,0,0,18
        .byte 42,34,49,2,2,34,25,2,0,0,2,2,32,26,35,2,0,39,2,0,37,41,0,0,51,2,0,49,0,0,0,2,2,2
        .byte 0,21,53,2,0,18,0,0,32,27,34,2,0,0,2,0,48,49,2,18,0,2,0,21,2,255,0,4,18,0,16,16,2
        .byte 255,0,4,32,44,34,2,0,18,0,0,51,49,2,0,0,0,18,34,21,34,30,24,34,21,16,43,42,16,50,52,50,46
        .byte 30,42,43,16,16,42,30,16,43,42,30,45,46,21,38,43,255,47,4,22,47,22,47
        .byte 255,2,31,0,0,0,18,255,0,25,2,0,21,30,36,16,0,0,0,21,34,36,16,0,0,18,0,16,34,38,21,0,0,0
        .byte 21,34,36,16,0,18,2,0,29,22,23,29,2,0,0,28,22,22,28,2,0,0,0,44,23,22,34,6,0,0,28,22,22
        .byte 26,2,0,2,0,25,22,22,26,6,0,0,25,23,22,25,2,0,0,0,34,22,22,44,2,0,0,31,22,23,25,2,0
        .byte 49,50,16,36,50,21,2,0,18,16,36,30,21,2,0,0,0,21,38,34,16,2,18,0,16,36,34,21,36,49,255,2,7
        .byte 0,0,0,255,2,4,18,0,0,0,33,33,2,2,255,0,4,2,2,2,6,2,0,0,18,255,0,26,2,0,0,0,51
        .byte 49,50,16,40,50,21,16,38,34,41,37,38,34,21,16,40,50,16,48,0,0,0,18,0,0,2,255,0,4,255,2,9,46
        .byte 21,6,255,2,8,255,0,5,2,255,0,7,18,255,0,5,16,46,2,255,0,5,18,255,0,7,2,0,18,255,0,4,51
        .byte 50,40,21,16,34,38,43,42,34,38,21,16,50,40,21,16,40,51,255,0,4,2,255,0,7,255,2,19,0,18,0,6,255,0,4
        .byte 18,255,0,4,5,5,255,0,18,2,0,21,45,34,16,0,0,0,16,47,47,16,0,0,0,18,21,34,45,16,30,46,45
        .byte 16,38,48,21,0,0,2,0,34,22,22,44,2,0,0,28,22,23,25,2,0,0,0,27,2,2,53,2,2,2,22,23,22
        .byte 39,2,0,2,0,44,23,22,34,2,18,0,31,22,22,35,2,0,0,0,35,2,0,44,2,18,0,22,22,22,47,6,0
        .byte 2,18,16,34,45,21,6,0,0,16,36,34,16,34,36,45,34,16,2,0,16,2,0,48,21,30,45,16,2,0,2,0,0
        .byte 255,2,4,0,0,0,255,2,6,6,2,2,0,27,2,0,0,2,6,2,2,2,0,2,0,0,18,255,0,7,18,255,0,8
        .byte 25,2,255,0,8
        .byte 73,20,75,47,70,70,47,20,25,71,20,76,21,21,76,75,71,37,30,30,39,30,30,36,46,45,38,21,21,41,3,6,255,2,7
        .byte 73,75,2,6,255,2,9,6,255,2,6,21,47,2,0,0,18,255,0,5,2,2,255,0,4,18,255,0,10,18,0,21,72
        .byte 73,3,0,0,0,3,0,0,18,0,0,0,37,21,21,49,75,255,0,11,3,75,71,20,2,0,0,46,21,47,46,47,21
        .byte 38,43,46,46,47,42,45,45,75,0,18,255,0,6,25,74,2,2,6,0,0,44,2,2,2,6,255,2,8,32,31,75,0,0,0
        .byte 37,21,0,0,31,71,2,255,0,4,25,2,255,0,11,32,42,53,2,0,0,21,43,2,0,30,73,2,255,0,4,24,2
        .byte 0,24,3,72,0,0,0,18,0,0,0,32,47,42,41,0,0,0,2,2,0,26,74,2,18,0,0,0,77,2,0,3,19
        .byte 75,3,72,0,0,0,37,71,41,2,2,75,2,255,0,5,31,71,2,255,0,4,71,2,0,0,2,77,73,75,2,0,18
        .byte 71,72,43,2,0,47,36,41,0,18,0,37,30,72,6,0,0,18,0,25,2,0,0,0,74,77,77,2,0,5,47,2,6
        .byte 2,0,3,21,75,2,0,0,42,30,75,2,255,0,4,24,3,24,0,18,0,2,77,6,0,71,39,2,0,0,0,73,2,2
        .byte 6,0,0,0,26,77,2,0,0,69,69,46,21,20,255,0,4,74,2,0,0,31,6,0,40,24,21,2,255,0,5,34,74
        .byte 2,18,0,73,71,21,25,20,75,73,71,72,71,2,0,0,42,2,0,0,21,43,6,0,37,21,18,0,31,24,5,0,0
        .byte 72,75,255,2,4,6,255,2,4,0,0,0,2,18,0,0,2,2,0,21,43,2,0,30,76,75,2,0,77,20,6,255,0,7
        .byte 18,255,0,10,31,2,6,0,30,47,25,2,0,74,72,2,0,71,72,75,0,3,75,71,72,75,0,37,30,46,0,0,18
        .byte 0,46,2,0,0,26,3,2,0,0,73,75,2,0,0,2,2,2,18,2,2,6,2,2,0,2,2,72,41,0,0,0,6
        .byte 0,37,21,77,6,0,0,77,74,2,0,18,255,0,9,18,0,0,75,21,2,255,0,4,21,21,255,47,7,73,3,71,73
        .byte 20,72,75,73,77,76,77,72,71,73,74,21,21,36,46,45,21,21,43
        .byte 76,20,20,76,255,0,5,20,79,6,0,0,0,79,75,76,71,3,79,6,255,0,4,76,20,20,76,78,2,2,2,6,255,0,5
        .byte 2,2,255,0,4,2,2,6,2,2,2,255,0,5,2,6,78,77,2,255,0,6,18,255,0,20,77,77,2,0,79,76,255,0,6
        .byte 76,79,0,18,255,0,8,18,0,79,79,0,0,77,25,2,0,73,79,2,0,79,3,0,0,79,75,2,255,0,4,76,255,0,6
        .byte 79,71,2,0,76,72,2,0,0,2,2,0,76,77,2,0,0,2,2,0,76,0,0,77,2,0,79,3,0,0,0,2,6
        .byte 0,72,75,6,255,0,5,77,77,6,255,0,5,3,24,24,71,6,0,71,77,2,255,0,5,75,25,2,255,0,5,73,76
        .byte 2,255,0,5,79,76,25,79,2,0,76,75,2,0,0,18,0,0,25,24,2,0,79,3,0,0,3,79,2,0,79,76,0,0,0
        .byte 2,6,2,2,0,76,79,2,0,76,79,0,0,24,77,2,0,75,78,2,0,0,2,2,0,77,78,2,255,0,8,2,6
        .byte 0,78,3,2,0,77,77,2,18,3,77,6,255,0,5,71,77,2,255,0,11,71,75,2,0,77,25,2,0,71,79,2,0,0,0
        .byte 18,0,73,79,2,0,3,79,76,3,255,0,6,79,74,2,0,25,72,2,0,0,2,2,0,75,79,0,0,0,2,2,0
        .byte 77,76,47,75,2,0,79,76,0,0,0,6,2,0,72,75,2,0,0,0,18,0,77,75,2,255,0,5,73,47,23,78,2
        .byte 0,3,77,2,255,0,5,75,74,2,255,0,5,3,77,2,0,18,0,0,0,71,22,22,77,2,0,77,3,2,255,0,5
        .byte 74,24,6,0,79,79,0,0,79,76,2,0,79,75,0,0,79,25,75,79,2,0,76,79,6,0,79,79,0,0,24,77,2
        .byte 0,79,75,2,0,0,6,2,0,76,79,6,0,0,0,79,75,6,0,0,2,2,0,75,79,2,0,77,77,2,0,0,2
        .byte 6,255,0,6,2,2,255,0,4,2,2,255,0,6,2,6,0,77,78,2,2,2,255,0,6,18,255,0,4,18,255,0,5
        .byte 18,255,0,7,78,76,20,25,76,2,0,18,76,79,0,0,0,18,0,0,79,3,25,71,73,3,79,79,3,25,25,76,20,20
        .byte 76
        .byte 80,81,255,0,15,82,83,2,255,0,8,80,81,77,77,2,0,80,81,255,0,4,18,255,0,7,2,6,255,0,6,18,0
        .byte 75,77,75,77,6,0,79,84,2,255,0,17,80,81,0,0,255,77,4,2,0,79,84,2,255,0,6,80,81,255,0,7,18
        .byte 0,79,84,2,0,77,79,77,84,2,0,82,83,2,255,0,4,76,79,84,76,2,0,76,79,84,76,0,0,0,79,84,2
        .byte 0,77,75,77,75,2,0,0,6,2,0,0,0,80,79,80,81,79,2,0,79,80,81,84,81,0,0,82,83,6,0,75,77,77,77
        .byte 2,255,0,7,79,79,82,83,75,2,0,73,82,83,79,79,2,0,0,2,2,0,255,77,4,2,255,0,7,82,79,2,6
        .byte 79,6,0,79,2,2,79,83,6,255,0,5,82,83,79,77,2,18,255,0,4,18,0,0,82,2,0,78,2,0,78,2,0
        .byte 83,2,2,255,0,7,75,77,2,255,0,9,6,0,78,2,0,78,6,0,0,2,255,0,6,80,81,82,83,2,0,80,81
        .byte 255,0,8,82,79,84,83,2,255,0,5,80,81,0,0,77,77,6,2,2,0,79,84,6,255,0,8,79,84,2,2,255,0,5
        .byte 79,84,2,0,77,84,80,81,0,0,82,83,2,255,0,4,18,0,80,81,80,81,80,81,255,0,5,82,83,2,0,77,75
        .byte 71,77,2,0,0,2,2,255,0,6,82,255,46,4,83,2,255,0,5,2,6,0,77,77,75,77,2,255,0,5,80,81,255,0,4
        .byte 2,79,84,2,6,2,0,18,255,0,6,79,77,77,84,6,255,0,5,79,84,2,255,0,4,79,84,2,255,0,10,75,77,77
        .byte 75,2,0,0,0,18,0,82,83,6,255,0,4,80,81,2,255,0,6,18,0,0,0,77,84,84,77,2,255,0,6,2,2
        .byte 255,0,4,82,83,2,255,0,10,77,75,77,77,2,0,80,81,0,0,18,255,0,4,18,0,0,2,6,255,0,6,80,81
        .byte 0,0,77,77,82,83,2,0,82,83,6,255,0,4,80,81,255,0,6,80,81,18,0,0,82,83,2,0,82,83
        .byte 80,81,0,80,79,79,79,81,0,80,79,79,79,81,0,80,79,79,79,81,0,80,255,79,4,81,0,80,81,79,84,2,82
        .byte 84,84,84,83,2,82,84,84,84,83,2,82,84,84,84,83,6,82,255,84,4,83,2,82,83,79,84,255,2,6,6,0,46
        .byte 255,2,4,0,6,2,2,80,81,2,2,6,255,2,4,0,2,79,84,6,255,0,7,46,2,18,255,0,6,82,83,2,255,0,4
        .byte 18,0,80,81,82,83,2,0,80,81,0,0,0,80,46,2,0,0,80,81,0,0,0,80,81,2,0,0,80,81,80,81,79
        .byte 84,6,2,2,0,82,255,46,5,83,6,0,0,82,83,2,18,0,82,83,2,0,0,82,46,46,83,79,84,80,81,0,18
        .byte 0,6,255,2,6,0,0,0,6,2,0,0,0,2,2,0,0,0,2,2,2,79,84,79,84,2,255,0,4,18,255,0,14
        .byte 18,255,0,5,82,83,79,84,2,255,0,4,5,255,0,15,5,5,255,0,4,6,79,84,2,80,255,69,6,81,79,80,255,0,4
        .byte 81,79,80,255,69,5,81,0,0,80,81,82,83,6,82,255,70,6,83,84,82,2,0,0,0,83,84,82,255,70,5,83,6
        .byte 0,79,84,2,2,2,0,255,2,5,6,255,2,4,255,0,4,6,255,2,8,0,79,84,80,81,255,0,4,18,255,0,8
        .byte 18,255,0,9,18,0,0,79,84,79,84,2,18,255,0,7,18,255,0,16,82,83,79,84,2,0,80,81,80,3,81,80,81
        .byte 0,80,81,80,81,0,0,0,80,81,80,3,81,80,81,0,0,0,2,79,84,2,0,82,255,46,5,83,2,82,46,46,83
        .byte 6,0,0,82,255,46,5,83,6,0,80,81,82,83,2,0,0,255,2,6,6,0,2,46,81,2,18,0,0,2,82,46,83
        .byte 2,6,2,0,79,84,2,2,6,255,0,11,82,83,2,255,0,5,2,2,2,0,0,0,79,84,80,81,0,80,255,79,4
        .byte 81,0,80,79,79,81,22,22,80,79,79,79,81,0,80,79,79,79,81,0,79,84,82,83,2,82,255,84,4,83,6,82,84,84
        .byte 83,22,23,82,84,84,84,83,6,82,84,84,84,83,6,82,83
        .byte 255,0,14,85,85,255,0,28,85,85,255,0,11,18,255,0,4,18,255,0,7,18,255,0,10,18,255,0,44,18,255,0,62
        .byte 18,255,0,5,18,255,0,8,18,255,0,8,18,255,0,37,80,81,255,0,4,80,81,255,0,22,84,84,2,0,0,0,84,84
        .byte 6,255,0,17,18,0,0,0,82,83,6,0,0,0,82,83,2,0,0,18,255,0,19,2,2,255,0,4,2,2,255,0,21
        .byte 80,81,0,18,0,0,80,81,255,0,15,18,255,0,6,84,84,2,0,0,0,84,84,2,255,0,7,18,255,0,13,82,83
        .byte 2,0,0,0,82,83,6,255,0,19,18,0,0,2,6,0,0,18,0,2,2,255,0,16,18,255,0,7,80,81,255,0,26
        .byte 80,81,79,79,80,81,0,0,0,18,255,0,9,18,255,0,10,255,79,6,2,255,0,6,18,255,0,14,80,81,82,83,82
        .byte 83,82,83,80,81,255,0,10
numsc:
        .byte 23
.out .sprintf("SCREENS SIZE = %d", (* - scdat))

nmedat:
        .byte 0,0,96,152,255
        .byte 0,0,104,120,2,6,8,40,2,6,8,200,2,6,152,120,2,6,40,120,2,6,120,16,2,6,152,216,3,7,48,48,255
        .byte 0,0,152,176,2,6,24,48,2,9,144,208,2,6,88,112,2,6,136,24,2,6,144,112,2,6,8,168,6,15,56,104,6,15,112,208,6,11,112,136,255
        .byte 0,0,136,216,6,11,64,120,2,6,8,96,2,9,24,224,2,6,144,16,2,6,56,32,2,9,64,232,2,6,32,8,255
        .byte 0,0,152,120,2,6,152,56,2,9,144,24,2,6,152,192,2,9,72,128,2,6,8,160,2,9,88,208,3,7,8,192,255
        .byte 0,0,136,160,3,7,8,8,6,15,32,56,6,11,80,56,2,6,8,200,2,6,152,8,2,6,112,96,2,6,32,208,2,6,64,184,2,6,112,208,255
        .byte 0,0,152,120,2,6,8,8,2,6,8,232,2,6,152,8,2,6,152,232,2,6,56,120,2,6,96,120,3,7,16,80,6,11,24,24,6,11,120,216,5,12,72,24,255
        .byte 0,0,144,120,2,6,40,128,2,6,120,112,6,15,80,224,3,7,16,72,2,6,8,8,2,6,8,232,5,12,80,72,2,6,104,176,5,12,96,224,2,6,144,200,255
        .byte 0,0,80,120,3,16,72,200,5,12,24,88,5,12,144,152,6,15,24,24,2,6,112,120,2,6,104,24,2,6,56,72,2,6,136,72,2,6,48,168,2,6,112,168,255
        .byte 0,0,144,48,2,6,64,96,2,6,64,128,2,6,96,96,2,6,96,128,2,6,32,80,2,6,128,144,6,15,80,144,6,11,48,112,3,7,16,224,5,12,144,128,255
        .byte 0,0,144,24,3,7,64,168,2,6,16,88,2,6,32,184,2,6,144,184,2,6,144,216,2,6,144,104,2,6,104,72,6,11,24,56,6,11,112,120,6,15,80,192,255
        .byte 0,0,152,120,3,7,56,120,2,6,8,8,2,6,8,216,2,6,80,120,2,6,152,216,2,6,72,8,2,6,152,24,6,15,56,8,6,11,104,216,255
        .byte 0,0,144,72,2,6,96,88,2,6,40,96,2,6,120,152,2,6,40,64,2,6,144,192,2,6,72,200,5,12,144,224,3,7,16,40,6,11,72,112,6,15,16,104,255
        .byte 0,0,144,168,2,9,40,40,2,9,32,88,2,6,104,120,2,6,144,192,2,6,16,160,2,6,56,200,6,11,40,144,5,12,104,184,6,15,80,224,255
        .byte 0,0,152,120,6,15,72,64,6,11,112,88,3,16,80,184,2,6,32,88,2,6,112,32,2,6,48,136,2,6,128,192,2,6,32,64,2,6,72,136,255
        .byte 0,0,144,152,2,6,16,64,2,6,144,176,2,6,16,88,2,6,48,224,2,6,104,40,2,6,40,152,5,12,144,16,3,7,80,120,6,15,80,56,6,11,16,16,255
        .byte 0,0,144,16,5,12,96,16,6,11,48,16,6,15,96,80,3,7,24,112,2,6,56,152,2,6,24,176,2,6,72,200,2,6,24,16,2,6,56,80,2,6,120,80,255
        .byte 0,0,128,120,3,16,32,120,3,7,72,224,6,15,80,8,2,6,128,208,2,6,128,152,2,6,80,136,2,6,80,104,2,6,32,232,2,6,32,8,255
        .byte 0,0,144,16,3,7,120,224,2,6,56,144,2,6,72,168,2,6,96,96,2,6,144,160,2,6,144,216,2,6,16,96,6,15,16,216,6,18,40,40,6,15,120,56,255
        .byte 0,0,152,96,3,17,8,192,3,17,32,16,3,16,72,160,2,6,16,16,2,6,40,136,2,6,56,96,2,6,88,176,2,6,112,96,2,6,56,208,255
        .byte 0,0,152,128,2,6,72,128,2,6,64,152,2,6,64,104,2,6,8,200,2,6,72,40,2,6,152,216,3,11,88,88,3,11,136,168,6,15,16,128,255
        .byte 0,0,136,104,3,7,96,216,6,18,64,120,6,15,104,88,2,6,136,160,2,6,136,200,2,6,32,64,2,6,24,216,2,6,24,120,2,6,80,216,255
        .byte 8,0,120,120,255
.out .sprintf("POSITIONS SIZE = %d", (* - nmedat))

NUMOBJ = 1
objdta:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,254,56,120,254,56,120
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
        .byte 0,0,0,85,170,255,0,0
        .byte 24,20,24,20,24,20,24,20
        .byte 24,60,0,231,231,0,60,24
        .byte 40,24,40,24,40,24,40,24
        .byte 0,0,0,0,0,24,24,48
        .byte 0,0,0,0,126,0,0,0
        .byte 0,0,0,0,0,56,56,0
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
        .byte 0,116,70,6,126,102,102,0
        .byte 0,252,6,252,198,6,254,0
        .byte 0,110,102,32,32,102,110,0
        .byte 0,248,12,70,70,12,248,0
        .byte 0,110,96,76,76,96,110,0
        .byte 0,222,0,220,0,192,192,0
        .byte 0,252,198,0,222,198,94,0
        .byte 0,198,68,254,0,198,198,0
        .byte 0,94,0,24,24,24,122,0
        .byte 0,6,6,6,102,102,116,0
        .byte 0,110,108,96,104,108,102,0
        .byte 0,224,96,96,96,2,254,0
        .byte 0,198,238,186,0,146,198,0
        .byte 0,100,118,126,96,110,102,0
        .byte 0,126,70,70,64,14,126,0
        .byte 0,110,102,102,110,96,96,0
        .byte 0,116,198,6,210,200,100,0
        .byte 0,252,192,70,92,64,198,0
        .byte 0,110,96,126,6,6,118,0
        .byte 0,254,130,40,56,56,40,0
        .byte 0,102,102,102,102,96,126,0
        .byte 0,198,198,198,0,108,40,0
        .byte 0,198,198,198,0,214,108,0
        .byte 0,198,68,16,56,108,198,0
        .byte 0,132,204,104,16,48,32,0
        .byte 0,246,12,0,48,96,222,0
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
        .byte 0,0,248,252,252,252,252,0
        .byte 0,0,248,204,204,204,204,0
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
