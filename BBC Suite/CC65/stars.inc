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

WINDOWTOP = 2
WINDOWLFT = 1
WINDOWHGT = 21
WINDOWWID = 30 ;面面面面面面面面面面面面
MAPWID = 13
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255
mapdat:
        .byte 255,0,1,2,3,4,5,6,7,8,9,10,255,255,11,12,13,14,15,16,17,18,19,20,21,255,255,22,23,24,25,26,27,28,29,30,31,32,255,255,33,34,35,36,37,38,39,40,41,42,43,255,255,44,45,46,47,48,49,50,255,255,255,255,255
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255
stmap:  .byte 1

evnt00:
        ldy #8
        lda (z80_ix),y
        sta vark
        lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp a00055
:
        lda #1
        ldy #7
        sta (z80_ix),y
        jmp a00085
a00055: lda #25
        cmp varc
        bcc *+5
        jmp a00085
        lda #0
        ldy #7
        sta (z80_ix),y
a00085: lda #1
        cmp vare
        bne *+5
        jmp a00136
        jsr lcol
        bcs :+
        jmp a00136
:
        lda #40
        asl a
        sta sndtyp
        lda #1
        sta deadf
a00136: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a00168
:
        jmp a00238
a00168: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a00200
:
        jmp a00238
a00200: lda #0
        cmp vare
        beq *+5
        jmp a00238
        lda #4
        cmp vara
        bne *+5
        jmp a00238
        jmp a00238
a00238: lda #1
        cmp vare
        bne *+5
        jmp a00371
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a00313
:
        lda varo
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp a00313
        lda #1
        sta deadf
a00313: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a00371
:
        lda varp
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a00371
        lda #1
        sta deadf
a00371: lda #0
        sta chary
        lda #6
        sta charx
        lda scno
        jsr disply
        lda #0
        sta chary
        lda #31
        sta charx
        lda numlif
        jsr disply
        lda #0
        sta chary
        lda #16
        sta charx
        jsr dscor
        lda #0
        cmp vare
        beq *+5
        jmp a00745
        lda #4
        cmp vara
        beq *+5
        jmp a00548
        lda #25
        cmp varc
        bcc *+5
        jmp a00543
        lda #2
        cmp vard
        bcc *+5
        jmp a00530
        ; lda #2
        ldy #7
        sta (z80_ix),y
        jmp a00543
a00530: lda #0
        ldy #7
        sta (z80_ix),y
a00543: jmp a00569
a00548: lda #6
        cmp vard
        bcc *+5
        jmp a00569
        jsr vapour
a00569: lda #25
        cmp varc
        bcc *+5
        jmp a00706
        lda #24
        cmp varc
        bcc *+5
        jmp a00634
        lda #3
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a00634
        lda #50
        jsr explod
a00634: lda #0
        ldy #6
        sta (z80_ix),y
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a00679
:
        jmp a00701
a00679: lda #3
        ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp a00701
a00701: jmp a00745
a00706: lda varn
        asl a
        sta sndtyp
        lda #0
        jsr animsp
        jsr vapour
        lda #3
        ldy #6
        sta (z80_ix),y
a00745: lda #0
        cmp vare
        beq *+5
        jmp a01811
        lda #25
        cmp varc
        bcc *+5
        jmp a01697
        lda #9
        cmp varf
        beq *+5
        jmp a00800
        jmp a01697
a00800: lda joyval
        and #1
        beq :+
        jmp a00831
:
        lda #0
        ldy #11
        sta (z80_ix),y
a00831: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp a01016
        lda #1
        sta varb
        lda #2
        sta vara
        lda #231
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp a00914
        lda #8
        ; ldy #9
        sta (z80_ix),y
        rts
        jmp a01016
a00914: jsr cangr
        beq :+
        jmp a00967
:
        lda #0
        ldy #6
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp a01016
a00967: lda #1
        ldy #11
        sta (z80_ix),y
        lda #2
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
a01016: lda joyval
        and #2
        beq :+
        jmp a01047
:
        lda #1
        ldy #11
        sta (z80_ix),y
a01047: lda #1
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp a01197
        ; lda #1
        sta varb
        lda #3
        sta vara
        lda #8
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp a01130
        lda #230
        ; ldy #9
        sta (z80_ix),y
        rts
        jmp a01197
a01130: jsr cangl
        beq :+
        jmp a01183
:
        lda #0
        ldy #6
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp a01197
a01183: lda #0
        ldy #11
        sta (z80_ix),y
a01197: lda joyval
        and #4
        beq :+
        jmp a01228
:
        lda #2
        ldy #11
        sta (z80_ix),y
a01228: lda #2
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp a01412
        lda #1
        sta varb
        lda #0
        sta vara
        lda #165
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a01310
        lda #16
        ; ldy #8
        sta (z80_ix),y
        rts
        jmp a01412
a01310: jsr cangd
        beq :+
        jmp a01364
:
        lda #0
        ldy #6
        sta (z80_ix),y
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp a01412
a01364: lda #3
        ldy #11
        sta (z80_ix),y
        lda #2
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
a01412: lda joyval
        and #8
        beq :+
        jmp a01443
:
        lda #3
        ldy #11
        sta (z80_ix),y
a01443: lda #3
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp a01593
        lda #1
        sta varb
        ; lda #1
        sta vara
        lda #16
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp a01526
        lda #166
        ; ldy #8
        sta (z80_ix),y
        rts
        jmp a01593
a01526: jsr cangu
        beq :+
        jmp a01580
:
        lda #0
        ldy #6
        sta (z80_ix),y
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp a01593
a01580: lda #2
        ldy #11
        sta (z80_ix),y
a01593: lda joyval
        and #16
        beq :+
        jmp a01697
:
        lda #4
        cmp varf
        beq *+5
        jmp a01697
        lda #3
        ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp a01655
        jmp a01697
a01655: lda vara
        sta varf
        lda #2
        sta z80_c
        lda #5
        sta z80_b
        jsr spawn
        lda #25
        asl a
        sta sndtyp
a01697: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01729
:
        jmp a01811
a01729: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01761
:
        jmp a01811
a01761: lda #4
        cmp varf
        beq *+5
        jmp a01777
a01777: lda #6
        cmp varf
        beq *+5
        jmp a01794
a01794: lda #8
        cmp varf
        beq *+5
        jmp a01811
a01811: ldy #9
        lda (z80_ix),y
        sta vari
        lda #1
        cmp vare
        beq *+5
        jmp a02370
        lda #50
        cmp scno
        beq *+5
        jmp a01863
        jmp a01896
a01863: lda #20
        sta varf
        lda #10
        sta chary
        lda #11
        sta charx
        lda #3
        jsr dmsg
a01896: lda #18
        cmp varc
        bcc *+5
        jmp a02370
        lda varn
        asl a
        sta sndtyp
        jsr vapour
        ldy #8
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #8
        sta (z80_ix),y
        lda #4
        sta z80_c
        iny; ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #100
        ; ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp a02099
        lda #0
        ldy #6
        sta (z80_ix),y
        lda #1
        iny; ldy #7
        sta (z80_ix),y
        iny; ldy #8
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #8
        sta (z80_ix),y
        iny; ldy #9
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #9
        sta (z80_ix),y
a02099: lda #50
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp a02190
        lda #2
        ldy #7
        sta (z80_ix),y
        iny; ldy #8
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #8
        sta (z80_ix),y
        iny; ldy #9
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #9
        sta (z80_ix),y
a02190: lda #10
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp a02280
        lda #50
        cmp scno
        beq *+5
        jmp a02250
        lda #2
        sta varl
        lda #1
        sta restfl
        jmp a02280
a02250: lda #1
        sta varl
        lda #0
        sta varc
        lda #255
        ldy #5
        sta (z80_ix),y
a02280: lda #18
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp a02370
        lda #50
        cmp scno
        beq *+5
        jmp a02340
        lda #2
        sta varl
        lda #1
        sta restfl
        jmp a02370
a02340: lda #1
        sta varl
        lda #0
        sta varc
        lda #255
        ldy #5
        sta (z80_ix),y
a02370: rts
.out .sprintf("EVENT 0 SIZE = %d", (* - evnt00))

evnt01:
        lda #6
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00072
:
        jsr cangu
        beq :+
        jmp b00072
:
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
b00072: lda #0
        cmp vare
        beq *+5
        jmp b00188
        lda #23
        sta chary
        lda #9
        cmp varq
        bcs *+5
        jmp b00149
        lda #19
        sta charx
        lda #8
        cmp varf
        beq *+5
        jmp b00144
        jmp b00144
b00144: jmp b00179
b00149: lda #18
        sta charx
        lda #8
        cmp varf
        beq *+5
        jmp b00179
        jmp b00179
b00179: lda varq
        jsr disply
b00188: lda #50
        cmp varq
        bcc *+5
        jmp b00221
        lda #0
        sta varq
        lda #1
        sta deadf
b00221: lda #0
        cmp varq
        bcs *+5
        jmp b00255
        ; lda #0
        sta varq
        lda #1
        sta deadf
b00255: lda #1
        cmp varl
        beq *+5
        jmp b00314
        lda #49
        cmp varc
        bcc *+5
        jmp b00314
        lda #30
        sta varq
        lda #1
        sta nexlev
        ; lda #1
        sta restfl
b00314: lda #25
        cmp varc
        bcc *+5
        jmp b00471
        ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp b00390
:
        jmp b00390
b00390: lda #2
        ldy #11
        cmp (z80_ix),y
        bcc *+5
        jmp b00466
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp b00444
:
        jmp b00444
b00444: lda #0
        jsr animsp
        lda #0
        ldy #11
        sta (z80_ix),y
b00466: jmp b00489
b00471: jsr vapour
        lda #0
        ldy #7
        sta (z80_ix),y
b00489: lda #0
        cmp varb
        beq *+5
        jmp b00577
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp b00577
:
        lda #1
        sta vare
        lda #0
        sta varg
        lda #1
        ldy #6
        sta (z80_ix),y
        lda #3
        iny; ldy #7
        sta (z80_ix),y
b00577: lda #1
        cmp varb
        beq *+5
        jmp b01609
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp b00673
:
        lda #<100
        sta z80_l
        lda #>100
        sta z80_h
        jsr addsc
        lda #0
        sta varc
        ; lda #0
        sta varb
        lda #1
        sta vare
        jmp b01609
b00673: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b01609
:
        lda #6
        asl a
        sta sndtyp
        lda #<5
        sta z80_l
        lda #>5
        sta z80_h
        jsr addsc
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp b00760
:
        jmp b00760
b00760: lda #0
        cmp vara
        beq *+5
        jmp b00972
        lda #159
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp b00821
        lda #32
        ; ldy #8
        sta (z80_ix),y
        rts
        jmp b00972
b00821: jsr cangd
        beq :+
        jmp b00875
:
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp b00915
b00875: lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp b00972
b00915: lda #6
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00972
:
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
b00972: lda #1
        cmp vara
        beq *+5
        jmp b01184
        lda #32
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp b01033
        lda #160
        ; ldy #8
        sta (z80_ix),y
        rts
        jmp b01184
b01033: jsr cangu
        beq :+
        jmp b01087
:
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp b01127
b01087: lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp b01184
b01127: lda #6
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b01184
:
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
b01184: lda #2
        cmp vara
        beq *+5
        jmp b01397
        lda #216
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp b01245
        lda #24
        ; ldy #9
        sta (z80_ix),y
        rts
        jmp b01397
b01245: jsr cangr
        beq :+
        jmp b01299
:
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp b01339
b01299: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp b01397
b01339: lda #6
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b01397
:
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
b01397: lda #3
        cmp vara
        beq *+5
        jmp b01609
        lda #23
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp b01458
        lda #216
        ; ldy #9
        sta (z80_ix),y
        rts
        jmp b01609
b01458: jsr cangl
        beq :+
        jmp b01511
:
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp b01552
b01511: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp b01609
b01552: lda #6
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b01609
:
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
b01609: lda #2
        cmp varb
        beq *+5
        jmp b02397
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp b01658
:
        jmp b01658
b01658: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp b01690
:
        jmp b02397
b01690: lda #0
        cmp varh
        beq *+5
        jmp b01862
        lda #159
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp b01751
        lda #32
        ; ldy #8
        sta (z80_ix),y
        rts
        jmp b01862
b01751: jsr cangd
        beq :+
        jmp b01800
:
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
b01800: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp b01862
:
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
b01862: lda #1
        cmp varh
        beq *+5
        jmp b02035
        lda #216
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp b01923
        lda #24
        ; ldy #9
        sta (z80_ix),y
        rts
        jmp b02035
b01923: jsr cangr
        beq :+
        jmp b01972
:
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
b01972: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp b02035
:
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
b02035: lda #2
        cmp varh
        beq *+5
        jmp b02208
        lda #32
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp b02096
        lda #160
        ; ldy #8
        sta (z80_ix),y
        rts
        jmp b02208
b02096: jsr cangu
        beq :+
        jmp b02145
:
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
b02145: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp b02208
:
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
b02208: lda #3
        cmp varh
        beq *+5
        jmp b02381
        lda #23
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp b02269
        lda #216
        ; ldy #9
        sta (z80_ix),y
        rts
        jmp b02381
b02269: jsr cangl
        beq :+
        jmp b02318
:
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
b02318: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp b02381
:
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
b02381: lda #1
        sta varb
        lda #0
        sta vara
b02397: rts
.out .sprintf("EVENT 1 SIZE = %d", (* - evnt01))

evnt02:
        rts
.out .sprintf("EVENT 2 SIZE = %d", (* - evnt02))

evnt03:
        lda #1
        cmp vare
        beq *+5
        jmp d00041
        lda #20
        jsr explod
        lda #255
        ldy #5
        sta (z80_ix),y
d00041: lda #0
        cmp varh
        beq *+5
        jmp d00118
        lda #5
        cmp scno
        bcc *+5
        jmp d00118
        lda #2
        ldy #6
        sta (z80_ix),y
        lda #21
        cmp scno
        bcc *+5
        jmp d00118
        lda #7
        ; ldy #6
        sta (z80_ix),y
d00118: lda #1
        cmp varh
        beq *+5
        jmp d00165
        lda #4
        cmp scno
        bcc *+5
        jmp d00165
        lda #6
        ldy #6
        sta (z80_ix),y
d00165: lda #2
        cmp varh
        beq *+5
        jmp d00212
        ; lda #2
        cmp scno
        bcc *+5
        jmp d00212
        lda #7
        ldy #6
        sta (z80_ix),y
d00212: lda #3
        cmp varh
        beq *+5
        jmp d00260
        ; lda #3
        cmp scno
        bcc *+5
        jmp d00260
        lda #8
        ldy #6
        sta (z80_ix),y
d00260: lda #20
        cmp varg
        beq *+5
        jmp d00308
        lda #170
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
d00308: lda #32
        cmp varrnd
        bcs *+5
        jmp d00334
        lda #50
        sta varrnd
d00334: lda #5
        cmp varf
        beq *+5
        jmp d00589
        ; lda #5
        cmp varg
        bcc *+5
        jmp d00589
        lda #70
        cmp varrnd
        bcc *+5
        jmp d00576
        lda varrnd
        ldy #8
        sta (z80_ix),y
        jsr cangl
        beq :+
        jmp d00453
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #9
        sta (z80_ix),y
        lda #0
        jsr animsp
        jmp d00571
d00453: ldy #9
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #9
        sta (z80_ix),y
        lda #50
        sta varg
        lda #0
        jsr animsp
        lda #8
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp d00571
        lda #10
        jsr explod
        lda #0
        ldy #7
        sta (z80_ix),y
        ; lda #0
        sta varg
        lda #5
        sta varrnd
        lda #230
        ldy #9
        sta (z80_ix),y
d00571: jmp d00589
d00576: lda #0
        ldy #7
        sta (z80_ix),y
d00589: lda #2
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00750
        lda #1
        iny; ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp d00750
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00750
:
        lda #20
        asl a
        sta sndtyp
        lda #<50
        sta z80_l
        lda #>50
        sta z80_h
        jsr addsc
        lda #5
        jsr explod
        lda #5
        sta varrnd
        lda #230
        ldy #9
        sta (z80_ix),y
        lda #11
        sta varg
        lda #4
        sta varf
        lda #0
        ldy #7
        sta (z80_ix),y
d00750: lda #2
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00882
        lda #1
        iny; ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp d00882
        lda #4
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00882
:
        lda #10
        asl a
        sta sndtyp
        lda #5
        jsr explod
        lda #5
        sta varrnd
        lda #230
        ldy #9
        sta (z80_ix),y
        lda #11
        sta varg
        lda #0
        ldy #7
        sta (z80_ix),y
d00882: lda #6
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d01044
        lda #1
        iny; ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp d01044
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d01044
:
        lda #20
        asl a
        sta sndtyp
        lda #<50
        sta z80_l
        lda #>50
        sta z80_h
        jsr addsc
        lda #5
        jsr explod
        lda #6
        sta varf
        lda #5
        sta varrnd
        lda #230
        ldy #9
        sta (z80_ix),y
        lda #0
        sta varg
        ; lda #0
        ldy #7
        sta (z80_ix),y
d01044: lda #6
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d01183
        lda #1
        iny; ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp d01183
        lda #4
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d01183
:
        lda #10
        asl a
        sta sndtyp
        lda #5
        jsr explod
        lda #7
        sta varf
        lda #5
        sta varrnd
        lda #0
        sta varg
        lda #230
        ldy #9
        sta (z80_ix),y
        lda #0
        ldy #7
        sta (z80_ix),y
d01183: lda #7
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d01344
        lda #1
        iny; ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp d01344
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d01344
:
        lda #20
        asl a
        sta sndtyp
        lda #<50
        sta z80_l
        lda #>50
        sta z80_h
        jsr addsc
        lda #5
        jsr explod
        lda #8
        sta varf
        lda #5
        sta varrnd
        lda #70
        sta varg
        lda #230
        ldy #9
        sta (z80_ix),y
        lda #0
        ldy #7
        sta (z80_ix),y
d01344: lda #7
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d01484
        lda #1
        iny; ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp d01484
        lda #4
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d01484
:
        lda #10
        asl a
        sta sndtyp
        lda #5
        jsr explod
        lda #9
        sta varf
        lda #5
        sta varrnd
        lda #70
        sta varg
        lda #230
        ldy #9
        sta (z80_ix),y
        lda #0
        ldy #7
        sta (z80_ix),y
d01484: lda #8
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d01725
        lda #1
        iny; ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp d01725
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d01725
:
        lda #20
        asl a
        sta sndtyp
        lda #<50
        sta z80_l
        lda #>50
        sta z80_h
        jsr addsc
        lda #5
        jsr explod
        lda #5
        sta z80_c
        lda varq
        clc
        adc z80_c
        sta varq
        lda #30
        cmp varq
        bcc *+5
        jmp d01644
        ; lda #30
        sta varq
d01644: lda #7
        cmp numlif
        bcc *+5
        jmp d01666
        jmp d01682
d01666: lda numlif
        clc
        adc #1
        sta numlif
d01682: lda #5
        sta varrnd
        lda #0
        sta varg
        lda #230
        ldy #9
        sta (z80_ix),y
        lda #0
        ldy #7
        sta (z80_ix),y
d01725: lda #8
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d01923
        lda #1
        iny; ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp d01923
        lda #4
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d01923
:
        lda #10
        asl a
        sta sndtyp
        lda #5
        jsr explod
        lda #5
        sta z80_c
        lda varq
        sec
        sbc z80_c
        sta varq
        lda #9
        cmp varq
        bcs *+5
        jmp d01879
        lda #23
        sta chary
        lda #18
        sta charx
        lda #13
        jsr dmsg
d01879: lda #5
        sta varrnd
        lda #0
        sta varg
        lda #230
        ldy #9
        sta (z80_ix),y
        lda #0
        ldy #7
        sta (z80_ix),y
d01923: lda #1
        cmp vare
        beq *+5
        jmp d01953
        lda #0
        ldy #7
        sta (z80_ix),y
d01953: rts
.out .sprintf("EVENT 3 SIZE = %d", (* - evnt03))

evnt04:
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00080
:
        lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp e00050
        jmp e00080
e00050: lda #8
        cmp varf
        beq *+5
        jmp e00072
        jmp e00080
e00072: lda #1
        sta deadf
e00080: lda #2
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00193
:
        lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp e00128
        jmp e00193
e00128: lda #45
        asl a
        sta sndtyp
        lda #<15
        sta z80_l
        lda #>15
        sta z80_h
        jsr addsc
        lda #50
        jsr explod
        lda #0
        ldy #7
        sta (z80_ix),y
        ; lda #0
        sta varg
e00193: lda #100
        cmp varg
        bcs *+5
        jmp e00228
        lda #0
        ldy #7
        sta (z80_ix),y
        jmp e00314
e00228: lda #9
        ldy #6
        sta (z80_ix),y
        lda #8
        cmp varf
        beq *+5
        jmp e00272
        lda #0
        jsr animbk
        jmp e00314
e00272: lda #2
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp e00306
        lda #3
        asl a
        sta sndtyp
e00306: lda #0
        jsr animsp
e00314: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp e00346
:
        jmp e00400
e00346: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp e00378
:
        jmp e00400
e00378: lda #8
        cmp varf
        beq *+5
        jmp e00400
        jmp e00400
e00400: lda #4
        cmp vara
        beq *+5
        jmp e00434
        lda #0
        ldy #7
        sta (z80_ix),y
        jmp e01382
e00434: jsr vapour
        lda #8
        cmp varf
        beq *+5
        jmp e00461
        jmp e01382
e00461: lda #230
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp e00500
        lda #11
        ; ldy #9
        sta (z80_ix),y
        rts
e00500: lda #10
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp e00540
        lda #228
        ; ldy #9
        sta (z80_ix),y
        rts
e00540: lda #166
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp e00579
        lda #18
        ; ldy #8
        sta (z80_ix),y
        rts
e00579: lda #17
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp e00619
        lda #165
        ; ldy #8
        sta (z80_ix),y
        rts
e00619: lda #20
        cmp scno
        bcc *+5
        jmp e00747
        lda vari
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp e00678
        lda #0
        iny; ldy #10
        sta (z80_ix),y
        jmp e00742
e00678: lda #1
        ldy #10
        sta (z80_ix),y
        lda #130
        cmp varrnd
        bcs *+5
        jmp e00728
        lda #0
        iny; ldy #11
        sta (z80_ix),y
        jmp e00742
e00728: lda #1
        ldy #11
        sta (z80_ix),y
e00742: jmp e00915
e00747: lda vari
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp e00803
        lda #0
        iny; ldy #10
        sta (z80_ix),y
        lda #1
        iny; ldy #11
        sta (z80_ix),y
        jmp e00915
e00803: lda #1
        ldy #10
        sta (z80_ix),y
        lda #0
        iny; ldy #11
        sta (z80_ix),y
        lda vari
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp e00887
        lda #1
        ldy #10
        sta (z80_ix),y
        lda #0
        iny; ldy #11
        sta (z80_ix),y
        jmp e00915
e00887: lda #0
        ldy #10
        sta (z80_ix),y
        lda #1
        iny; ldy #11
        sta (z80_ix),y
e00915: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp e01000
        jsr cangl
        beq :+
        jmp e00982
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #9
        sta (z80_ix),y
        jmp e00996
e00982: lda #1
        ldy #11
        sta (z80_ix),y
e00996: jmp e01058
e01000: jsr cangr
        beq :+
        jmp e01045
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
        jmp e01058
e01045: lda #0
        ldy #11
        sta (z80_ix),y
e01058: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e01143
        jsr cangu
        beq :+
        jmp e01125
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #8
        sta (z80_ix),y
        jmp e01139
e01125: lda #1
        ldy #10
        sta (z80_ix),y
e01139: jmp e01201
e01143: jsr cangd
        beq :+
        jmp e01188
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #8
        sta (z80_ix),y
        jmp e01201
e01188: lda #0
        ldy #10
        sta (z80_ix),y
e01201: lda #1
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e01231
:
        lda #2
        sta varb
e01231: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp e01263
:
        jmp e01382
e01263: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp e01295
:
        jmp e01382
e01295: lda #9
        cmp varf
        beq *+5
        jmp e01317
        jmp e01338
e01317: lda #20
        cmp scno
        bcs *+5
        jmp e01338
        jmp e01338
e01338: lda #7
        cmp varf
        beq *+5
        jmp e01360
        jmp e01382
e01360: lda #20
        cmp scno
        bcs *+5
        jmp e01382
        jmp e01382
e01382: lda #1
        cmp vare
        beq *+5
        jmp e01421
        lda #25
        jsr explod
        lda #255
        ldy #5
        sta (z80_ix),y
e01421: rts
.out .sprintf("EVENT 4 SIZE = %d", (* - evnt04))

evnt05:
        lda #1
        cmp vare
        bne *+5
        jmp f00070
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00070
:
        lda #6
        cmp varf
        beq *+5
        jmp f00062
        jmp f00070
f00062: lda #1
        sta deadf
f00070: ldy #8
        lda (z80_ix),y
        sta varp
        lda #4
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        lda #1
        cmp vare
        beq *+5
        jmp f00200
        ; lda #1
        ldy #11
        sta (z80_ix),y
        jsr cangd
        beq :+
        jmp f00200
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
f00200: lda #39
        cmp varc
        bcc *+5
        jmp f00510
        lda #7
        cmp varf
        beq *+5
        jmp f00247
        lda #0
        ldy #11
        sta (z80_ix),y
f00247: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp f00412
        jsr cangu
        beq :+
        jmp f00377
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
        ; ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp f00408
f00377: lda #6
        cmp vard
        bcc *+5
        jmp f00408
        lda #1
        ldy #11
        sta (z80_ix),y
f00408: jmp f00505
f00412: jsr cangd
        beq :+
        jmp f00475
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
        jmp f00505
f00475: lda #6
        cmp vard
        bcc *+5
        jmp f00505
        lda #0
        ldy #11
        sta (z80_ix),y
f00505: jmp f00523
f00510: lda #0
        ldy #7
        sta (z80_ix),y
f00523: lda #6
        cmp varf
        beq *+5
        jmp f00554
        lda #1
        ldy #11
        sta (z80_ix),y
f00554: rts
.out .sprintf("EVENT 5 SIZE = %d", (* - evnt05))

evnt06:
        lda #21
        cmp scno
        bcc *+5
        jmp g00274
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp g00051
:
        jmp g00051
g00051: lda #1
        cmp vare
        bne *+5
        jmp g00269
        lda #6
        cmp varf
        beq *+5
        jmp g00089
        jmp g00269
g00089: lda #1
        cmp varm
        beq *+5
        jmp g00110
        jmp g00269
g00110: lda #2
        cmp vard
        bcc *+5
        jmp g00269
        lda vark
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp g00198
        lda #23
        asl a
        sta sndtyp
        jsr jump
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp g00198
:
        jmp g00198
g00198: lda vari
        ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp g00269
        lda #23
        asl a
        sta sndtyp
        jsr jump
        lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp g00269
:
        jmp g00269
g00269: jmp g00274
g00274: lda #5
        cmp varf
        beq *+5
        jmp g00299
        lda #0
        sta varm
g00299: lda #2
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00364
:
        lda #<25
        sta z80_l
        lda #>25
        sta z80_h
        jsr addsc
        lda #0
        ldy #7
        sta (z80_ix),y
        lda #1
        sta varm
g00364: lda #1
        cmp vare
        bne *+5
        jmp g00429
        ; lda #1
        cmp varm
        beq *+5
        jmp g00416
        lda #0
        ldy #7
        sta (z80_ix),y
        jmp g00429
g00416: lda #1
        ldy #7
        sta (z80_ix),y
g00429: lda #1
        cmp vare
        bne *+5
        jmp g00993
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00524
:
        lda #6
        cmp varf
        beq *+5
        jmp g00489
        jmp g00524
g00489: lda #1
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp g00524
        ; lda #1
        sta deadf
        jmp g00524
g00524: lda #6
        cmp varf
        beq *+5
        jmp g00546
        jmp g00839
g00546: lda #7
        cmp varf
        bne *+5
        jmp g00725
        lda #0
        cmp varm
        beq *+5
        jmp g00627
        lda #4
        cmp vara
        beq *+5
        jmp g00614
        lda #0
        ldy #7
        sta (z80_ix),y
        jmp g00627
g00614: lda #1
        ldy #7
        sta (z80_ix),y
g00627: lda #0
        cmp varm
        beq *+5
        jmp g00725
        lda #100
        cmp varg
        beq *+5
        jmp g00725
        lda #70
        asl a
        sta sndtyp
        lda #0
        ldy #10
        sta (z80_ix),y
        ; ldy #10
        lda (z80_ix),y
        jsr shoot
        lda #1
        ; ldy #10
        sta (z80_ix),y
        ; lda #1
        jsr shoot
g00725: lda #7
        cmp varf
        beq *+5
        jmp g00839
        lda #1
        ldy #7
        sta (z80_ix),y
        lda #3
        cmp vard
        bcc *+5
        jmp g00831
        lda #66
        asl a
        sta sndtyp
        lda #0
        ldy #10
        sta (z80_ix),y
        ; ldy #10
        lda (z80_ix),y
        jsr shoot
        lda #1
        ; ldy #10
        sta (z80_ix),y
        jmp g00839
g00831: lda #1
        jsr shoot
g00839: lda #6
        cmp varf
        beq *+5
        jmp g00874
        lda #0
        ldy #7
        sta (z80_ix),y
        jmp g00993
g00874: lda #0
        cmp varm
        beq *+5
        jmp g00993
        ldy #8
        lda (z80_ix),y
        cmp vark
        beq *+5
        jmp g00993
        lda #3
        cmp vard
        bcc *+5
        jmp g00993
        lda #73
        asl a
        sta sndtyp
        lda #0
        ldy #10
        sta (z80_ix),y
        ; ldy #10
        lda (z80_ix),y
        jsr shoot
        lda #1
        ; ldy #10
        sta (z80_ix),y
        ; lda #1
        jsr shoot
g00993: jmp grav
.out .sprintf("EVENT 6 SIZE = %d", (* - evnt06))

evnt07:
        lda #0
        cmp scno
        beq *+5
        jmp h00145
        lda #9
        sta chary
        ; lda #9
        sta charx
        lda #4
        jsr dmsg
        lda #11
        sta chary
        lda #10
        sta charx
        lda #5
        jsr dmsg
        lda #13
        sta chary
        lda #11
        sta charx
        lda #6
        jsr dmsg
        lda #19
        sta chary
        lda #11
        sta charx
        lda #7
        jsr dmsg
        jsr prskey
        lda #1
        sta scno
        jsr nwscr
        ; lda #1
        sta restfl
h00145: lda #50
        cmp scno
        beq *+5
        jmp h00382
        lda #4
        cmp vara
        bne *+5
        jmp h00382
        lda #0
        cmp varj
        bcc *+5
        jmp h00224
        lda #80
        ldy #8
        sta (z80_ix),y
        lda #104
        iny; ldy #9
        sta (z80_ix),y
h00224: lda #6
        cmp varj
        bcc *+5
        jmp h00269
        lda #112
        ldy #8
        sta (z80_ix),y
        lda #72
        iny; ldy #9
        sta (z80_ix),y
h00269: lda #12
        cmp varj
        bcc *+5
        jmp h00315
        lda #128
        ldy #8
        sta (z80_ix),y
        lda #120
        iny; ldy #9
        sta (z80_ix),y
h00315: lda #18
        cmp varj
        bcc *+5
        jmp h00360
        lda #96
        ldy #8
        sta (z80_ix),y
        lda #152
        iny; ldy #9
        sta (z80_ix),y
h00360: lda #1
        cmp vare
        bne *+5
        jmp h00382
        jmp h00382
h00382: lda #43
        cmp scno
        beq *+5
        jmp h00846
        lda #4
        cmp vara
        bne *+5
        jmp h00846
        lda #0
        cmp varj
        bcc *+5
        jmp h00461
        lda #48
        ldy #8
        sta (z80_ix),y
        lda #168
        iny; ldy #9
        sta (z80_ix),y
h00461: lda #3
        cmp varj
        bcc *+5
        jmp h00506
        lda #96
        ldy #8
        sta (z80_ix),y
        lda #200
        iny; ldy #9
        sta (z80_ix),y
h00506: lda #6
        cmp varj
        bcc *+5
        jmp h00551
        lda #128
        ldy #8
        sta (z80_ix),y
        lda #40
        iny; ldy #9
        sta (z80_ix),y
h00551: lda #9
        cmp varj
        bcc *+5
        jmp h00596
        lda #64
        ldy #8
        sta (z80_ix),y
        lda #56
        iny; ldy #9
        sta (z80_ix),y
h00596: lda #12
        cmp varj
        bcc *+5
        jmp h00642
        lda #128
        ldy #8
        sta (z80_ix),y
        lda #120
        iny; ldy #9
        sta (z80_ix),y
h00642: lda #15
        cmp varj
        bcc *+5
        jmp h00688
        lda #128
        ldy #8
        sta (z80_ix),y
        lda #184
        iny; ldy #9
        sta (z80_ix),y
h00688: lda #18
        cmp varj
        bcc *+5
        jmp h00733
        lda #80
        ldy #8
        sta (z80_ix),y
        lda #104
        iny; ldy #9
        sta (z80_ix),y
h00733: lda #21
        cmp varj
        bcc *+5
        jmp h00779
        lda #96
        ldy #8
        sta (z80_ix),y
        lda #152
        iny; ldy #9
        sta (z80_ix),y
h00779: lda #24
        cmp varj
        bcc *+5
        jmp h00824
        lda #112
        ldy #8
        sta (z80_ix),y
        lda #72
        iny; ldy #9
        sta (z80_ix),y
h00824: lda #1
        cmp vare
        bne *+5
        jmp h00846
        jmp h00846
h00846: rts
.out .sprintf("EVENT 7 SIZE = %d", (* - evnt07))

evnt08:
        lda #1
        cmp vare
        bne *+5
        jmp i00070
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i00070
:
        lda #6
        cmp varf
        beq *+5
        jmp i00062
        jmp i00070
i00062: lda #1
        sta deadf
i00070: ldy #8
        lda (z80_ix),y
        sta varo
        lda #4
        ldy #6
        sta (z80_ix),y
        lda #0
        iny; ldy #7
        sta (z80_ix),y
        lda #1
        cmp vare
        beq *+5
        jmp i00200
        lda #0
        ldy #11
        sta (z80_ix),y
        jsr cangu
        beq :+
        jmp i00200
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
i00200: lda #39
        cmp varc
        bcc *+5
        jmp i00510
        lda #7
        cmp varf
        beq *+5
        jmp i00247
        lda #1
        ldy #11
        sta (z80_ix),y
i00247: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp i00367
        jsr cangu
        beq :+
        jmp i00332
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
        jmp i00362
i00332: lda #6
        cmp vard
        bcc *+5
        jmp i00362
        lda #1
        ldy #11
        sta (z80_ix),y
i00362: jmp i00505
i00367: jsr cangd
        beq :+
        jmp i00475
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
        ; ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp i00505
i00475: lda #6
        cmp vard
        bcc *+5
        jmp i00505
        lda #0
        ldy #11
        sta (z80_ix),y
i00505: jmp i00523
i00510: lda #0
        ldy #7
        sta (z80_ix),y
i00523: lda #6
        cmp varf
        beq *+5
        jmp i00553
        lda #0
        ldy #11
        sta (z80_ix),y
i00553: rts
.out .sprintf("EVENT 8 SIZE = %d", (* - evnt08))

evnt09:
        rts
.out .sprintf("EVENT 9 SIZE = %d", (* - evnt09))

evnt10:
        lda #4
        cmp vara
        beq *+5
        jmp k00167
        lda joyval
        and #16
        beq :+
        jmp k00167
:
        lda joyval
        and #32
        beq :+
        jmp k00167
:
        lda joyval
        and #64
        beq :+
        jmp k00167
:
        lda #0
        cmp varr
        beq *+5
        jmp k00122
        lda #30
        asl a
        sta sndtyp
        lda #10
        jsr delay
        lda #1
        sta varr
        jmp k00167
k00122: lda #1
        cmp varr
        beq *+5
        jmp k00167
        lda #35
        asl a
        sta sndtyp
        lda #10
        jsr delay
        lda #0
        sta varr
k00167: lda #29
        cmp varq
        beq *+5
        jmp k00201
        lda #1
        cmp varj
        beq *+5
        jmp k00201
k00201: lda #8
        cmp varf
        beq *+5
        jmp k00341
        lda #9
        cmp varq
        bcs *+5
        jmp k00298
        lda #72
        cmp varg
        beq *+5
        jmp k00298
        lda #23
        sta chary
        lda #18
        sta charx
        lda #0
        cmp vare
        beq *+5
        jmp k00290
        jmp k00290
k00290: lda #13
        jsr dmsg
k00298: lda #249
        cmp varg
        bcc *+5
        jmp k00341
        lda #23
        sta chary
        lda #18
        sta charx
        lda #13
        jsr dmsg
k00341: lda #9
        cmp varq
        beq *+5
        jmp k00421
        lda #1
        cmp varj
        beq *+5
        jmp k00374
k00374: lda #23
        sta chary
        lda #18
        sta charx
        lda #8
        cmp varf
        beq *+5
        jmp k00413
        jmp k00413
k00413: lda #13
        jsr dmsg
k00421: lda varc
        clc
        adc #1
        sta varc
        lda #50
        cmp varc
        bcc *+5
        jmp k00462
        ; lda #50
        sta varc
k00462: lda vard
        clc
        adc #1
        sta vard
        lda #7
        cmp vard
        bcc *+5
        jmp k00502
        lda #0
        sta vard
k00502: lda #1
        cmp vare
        bne *+5
        jmp k00577
        lda #8
        cmp varf
        beq *+5
        jmp k00540
        jmp k00577
k00540: lda #4
        cmp vara
        beq *+5
        jmp k00562
        jmp k00577
k00562: lda varj
        clc
        adc #1
        sta varj
k00577: lda #25
        cmp varj
        bcc *+5
        jmp k00659
        lda #0
        cmp varr
        beq *+5
        jmp k00643
        lda #11
        asl a
        sta sndtyp
        lda varq
        sec
        sbc #1
        sta varq
        jmp k00651
k00643: lda #30
        sta varq
k00651: lda #0
        sta varj
k00659: lda #6
        cmp vard
        bcc *+5
        jmp k00697
        sta z80_c
        jsr qrand
        and #3
        bne :+
        jsr star
:
k00697: rts
.out .sprintf("EVENT 10 SIZE = %d", (* - evnt10))

evnt11:
        lda #128
        sta vara
        lda #192
        sta z80_c
        lda varg
        clc
        adc z80_c
        sta varg
        lda #192
        sta varg
        lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        sta varh
        lda #1
        cmp #1
        beq *+5
        jmp l00126
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        sta loopa
l00126: lda varf
        sta varn
        lda #192
        sta varn
        lda #0
        cmp varh
        beq *+5
        jmp l00177
        lda clock
        cmp #21
        bcs *+5
        jmp l00177
l00177: rts
.out .sprintf("EVENT 11 SIZE = %d", (* - evnt11))

evnt12:
        jsr cls
        ldy #8
        lda (z80_ix),y
        sta loopa
m00020: lda #128
        jsr dmsg
        dec loopa
        beq :+
        jmp m00020
:
        ldy #10
        lda (z80_ix),y
        sta chary
        lda #192
        jsr dmsg
        lda #128
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #192
        sta vara
        lda #1
        sta varb
        lda #0
        sta varc
        ; lda #0
        sta vard
        ; lda #0
        sta vare
        ; lda #0
        sta varl
        lda #5
        sta varf
        lda #0
        sta varg
        ; lda #0
        sta varj
        lda #30
        sta varq
        lda #190
        sta varp
        lda #10
        sta varo
        lda #3
        sta numlif
        lda #0
        sta varn
        ; lda #0
        sta varm
        ; lda #0
        sta varr
        rts
.out .sprintf("EVENT 12 SIZE = %d", (* - evnt12))

evnt13:
        lda #2
        cmp varl
        bne *+5
        jmp n00384
        lda #0
        cmp scno
        beq *+5
        jmp n00048
        lda #120
        asl a
        sta sndtyp
n00048: lda #0
        cmp scno
        bne *+5
        jmp n00384
        ; lda #0
        sta chary
        ; lda #0
        sta charx
        lda #10
        jsr dmsg
        lda #0
        sta chary
        lda #100
        ldy #9
        sta (z80_ix),y
        lda #60
        jsr explod
        lda #75
        jsr delay
        lda #60
        ldy #8
        sta (z80_ix),y
        lda #180
        iny; ldy #9
        sta (z80_ix),y
        lda #70
        jsr explod
        lda #75
        jsr delay
        lda #80
        ldy #8
        sta (z80_ix),y
        lda #120
        iny; ldy #9
        sta (z80_ix),y
        lda #80
        jsr explod
        lda #75
        jsr delay
        lda #100
        jsr explod
        jsr prskey
        jsr cls
        lda #4
        sta vara
        lda #1
        sta varb
        lda #0
        sta varc
        ; lda #0
        sta vare
        ; lda #0
        sta varl
        lda #5
        sta varf
        lda #0
        sta varg
        ; lda #0
        sta varj
        lda #30
        sta varq
        lda #190
        sta varp
        lda #10
        sta varo
        lda #3
        sta numlif
        lda #100
        sta varn
        lda #0
        sta varm
        ; lda #0
        sta varr
        ; lda #0
        sta scno
        jsr nwscr
        lda #1
        sta restfl
n00384: rts
.out .sprintf("EVENT 13 SIZE = %d", (* - evnt13))

evnt14:
        rts
.out .sprintf("EVENT 14 SIZE = %d", (* - evnt14))

evnt15:
        lda numlif
        sec
        sbc #1
        sta numlif
        lda #50
        asl a
        sta sndtyp
        lda #0
        cmp numlif
        bne *+5
        jmp p00156
        lda #5
        jsr delay
        lda #10
        sta chary
        lda #13
        sta charx
        lda #11
        jsr dmsg
        lda #0
        cmp varq
        bcs *+5
        jmp p00139
        lda #8
        sta chary
        lda #12
        sta charx
        lda #14
        jsr dmsg
        lda #30
        sta varq
        lda #0
        sta varj
p00139: lda #80
        jsr delay
        lda #1
        sta restfl
p00156: lda #0
        cmp numlif
        beq *+5
        jmp p00345
        lda #30
        jsr delay
        lda #100
        asl a
        sta sndtyp
        jsr cls
        ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        jsr drpob
        lda #12
        jsr dmsg
        lda #12
        sta chary
        lda #10
        sta charx
        lda #9
        jsr dmsg
        lda #12
        sta chary
        lda #17
        sta charx
        jsr dscor
        lda #230
        jsr delay
        jsr cls
        lda #0
        sta scno
        jsr nwscr
        lda #1
        sta restfl
        lda #3
        sta numlif
        lda #30
        sta varq
        lda #0
        sta varj
p00345: rts
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
        jsr cangl
        beq :+
        jmp       
:
        lda varl
        sta varobj
        jsr cangu
        beq :+
        jmp       
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jsr ifall
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        jsr cangd
        beq :+
        jmp       
:
        lda charx
        sta dispx
        lda chary
        sta dispy
        ldy #12
        lda (z80_ix),y
        jsr pattr
        lda vard
        sta varf
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        lda varr
        sta varf
        jsr cangd
        beq :+
        jmp       
:
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        lda varf
        sta varf
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda varm
        jsr pattr
        lda varobj
        sta varf
        sta z80_b
        sta z80_c
        jsr iscrly
        jsr cangd
        beq :+
        jmp t02003
:
        lda vare
        tax
        jsr minve
        lda #1
        sta prtmod
        jsr cangu
        beq :+
        jmp t02003
:
        jsr cangu
        beq :+
        jmp t02003
:
        lda #0
        sta prtmod
        jsr cangu
        beq :+
        jmp t00349
:
t00349: lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        lda varf
        sta varf
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        lda varf
        sta varf
        lda varb
        sta varf
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        jsr cangd
        beq :+
        jmp t02003
:
        lda vare
        tax
        jsr minve
        jsr cangd
        beq :+
        jmp t02003
:
        lda vare
        tax
        jsr minve
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        lda varf
        sta varf
        jsr cangu
        beq :+
        jmp t02003
:
        jsr cangu
        beq :+
        jmp t02003
:
        jsr cangl
        beq :+
        jmp t02003
:
        lda #1
        sta prtmod
        jsr cangu
        beq :+
        jmp       
:
        jsr cangu
        beq :+
        jmp       
:
        lda #0
        sta prtmod
        jsr cangl
        beq :+
        jmp       
:
        lda #1
        sta prtmod
        lda #0
        sta prtmod
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        lda varobj
        sta varf
        lda charx
        sta varf
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        lda varobj
        sta varf
        lda clock
        sta varf
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        lda varobj
        sta varf
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda varobj
        sta varf
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda numlif
        sta varf
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        lda charx
        sta varobj
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda vard
        jsr pattr
        lda varobj
        sta varobj
        lda clock
        sta varobj
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        lda varobj
        sta varobj
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda varobj
        sta varobj
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda vara
        sta varobj
        lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp       
:
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        lda charx
        sta varobj
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        lda varobj
        sta varobj
        lda clock
        sta varobj
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        lda varobj
        sta varobj
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda varobj
        sta varobj
        lda spptr
        sta z80_x
        lda spptr+1
        sta z80_i
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        lda varf
        sta varobj
        lda varf
        sta varobj
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        lda varf
        sta varobj
        lda varb
        sta varobj
        lda #1
        sta prtmod
        lda #0
        sta prtmod
        lda charx
        sta dispx
        lda chary
        sta dispy
        ldy #12
        lda (z80_ix),y
        jsr pattr
        lda varf
        sta varobj
        lda vard
        sta varobj
        jsr prskey
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        lda varf
        sta varobj
        lda vare
        sta varobj
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        lda varr
        sta varobj
        jsr cangd
        beq :+
        jmp       
:
        jsr prskey
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        jsr cangd
        beq :+
        jmp       
:
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        lda vare
        sta varf
        lda charx
        sta dispx
        lda chary
        sta dispy
        ldy #12
        lda (z80_ix),y
        jsr pattr
        lda varf
        sta varf
        lda vard
        sta varf
        lda charx
        sta dispx
        lda chary
        sta dispy
        ldy #11
        lda (z80_ix),y
        jsr pattr
        lda varf
        sta varf
        lda vard
        sta varf
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        jsr cangu
        beq :+
        jmp       
:
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda varobj
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        jsr cangu
        beq :+
        jmp       
:
        jsr cangu
        beq :+
        jmp       
:
        jsr cangu
        beq :+
        jmp       
:
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        jsr laddd
        beq :+
        jmp       
:
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        jsr cangd
        beq :+
        jmp       
:
        lda vare
        tax
        jsr minve
        lda varobj
        sta varo
        sta z80_b
        sta z80_c
        jsr iscrly
        lda varr
        sta varo
        jsr cangu
        beq :+
        jmp       
:
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        lda varf
        sta varf
        lda charx
        sta dispx
        lda chary
        sta dispy
        ldy #12
        lda (z80_ix),y
        jsr pattr
        lda varf
        sta varf
        lda vard
        sta varf
        jsr scrr
        lda charx
        sta dispx
        lda chary
        sta dispy
        jsr pattr
        lda varf
        sta varf
        lda varg
        sta varf
        jsr scrr
        jsr cangu
        beq :+
        jmp       
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
t02003: jmp grav
.out .sprintf("EVENT 19 SIZE = %d", (* - evnt19))

evnt20:
        rts
.out .sprintf("EVENT 20 SIZE = %d", (* - evnt20))

ptcusr: rts
script_end:

.out .sprintf("TOTAL SCRIPT SIZE = %d", (* - script_start))

data_start:
msgdat:
        .byte "*______________________________*",141
        .byte "********************************",141
        .byte "&&",141
        .byte "NEXT_LEVEL",141
        .byte "1._KEYBOARD",141
        .byte "2._KEMPSTON",141
        .byte "3._SINCLAIR",141
        .byte "2015_kas29",141
        .byte "LIVES",141
        .byte "SCORE",141
        .byte "LEVEL",141
        .byte "OOPS...",141
        .byte "GAME_OVER",141
        .byte "0",141
        .byte "TIME__OUT",141
        .byte "TIME:_",141
        .byte "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$",141
        .byte "GOOD__JOB",141
        .byte "GAME_OVER",141
nummsg:
        .byte 19
.out .sprintf("MESSAGE SIZE = %d", (* - msgdat))

chgfx:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,64,1,0,0,0,4,1,0,16,128,0,0,0,1,128
        .byte 32,0,0,136,0,0,0,34,0,0,0,16,0,0,50,0
        .byte 0,0,0,0,0,16,0,0,0,2,0,0,0,8,32,0
        .byte 0,0,0,0,128,17,0,16,68,0,0,0,17,0,0,64
        .byte 95,95,136,0,10,10,141,5,10,10,141,5,10,10,175,175
        .byte 95,95,90,90,95,95,90,90,95,95,90,90,95,95,90,90
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 240,240,175,175,240,240,175,175,240,240,175,175,240,240,175,175
        .byte 95,95,240,240,95,95,240,240,95,95,240,240,95,95,240,240
        .byte 0,0,0,0,0,0,0,0,0,0,34,170,85,85,170,170
        .byte 0,0,0,0,0,0,0,0,0,0,170,170,85,85,170,170
        .byte 87,95,39,175,87,95,39,175,87,95,39,175,87,95,39,175
        .byte 0,0,0,0,0,0,0,0,0,0,170,0,68,0,170,0
        .byte 0,0,34,0,0,0,34,0,0,0,34,0,0,0,34,0
        .byte 0,0,34,0,0,0,34,0,32,0,64,0,128,0,0,0
        .byte 0,0,34,0,0,0,34,0,0,0,34,170,85,85,170,170
        .byte 95,78,175,174,95,78,175,174,95,78,175,174,95,78,175,174
        .byte 136,0,136,0,136,0,136,0,136,0,136,0,136,0,136,0
        .byte 0,17,0,17,0,17,0,17,0,17,0,17,0,17,0,17
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,255
        .byte 255,255,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,128,16,0
        .byte 17,0,0,136,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,17,0,0,136
        .byte 0,128,16,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 255,255,136,0,136,0,136,0,136,0,136,0,136,0,136,0
        .byte 255,255,0,17,0,17,0,17,0,17,0,17,0,17,0,17
        .byte 136,0,136,0,136,0,136,0,136,0,136,0,136,0,255,255
        .byte 0,17,0,17,0,17,0,17,0,17,0,17,0,17,255,255
        .byte 0,0,0,0,34,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,49,0,114,0,245,16,234,49,196,114,0,196,0,250,250
        .byte 117,245,0,98,17,196,114,136,245,0,234,0,196,0,136,0
        .byte 0,0,0,0,0,0,16,136,49,0,98,0,196,0,250,136
        .byte 17,245,50,250,0,100,0,200,17,128,50,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,200,17,196,34,200,68,196,250,200
        .byte 0,213,0,216,0,196,0,200,0,196,34,200,100,0,0,0
        .byte 0,0,0,0,0,0,0,0,17,196,34,98,68,228,250,200
        .byte 245,17,0,34,0,68,0,0,0,0,0,0,0,0,0,0
        .byte 196,0,98,0,49,0,16,136,0,196,0,64,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16
        .byte 255,85,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 170,255,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,17,0,35,0,87,0,175,17,95,35,175,87,95,170,170
        .byte 95,95,175,174,95,78,175,174,95,78,175,174,95,78,136,0
        .byte 245,245,250,250,245,245,250,250,245,245,250,250,245,245,250,250
        .byte 95,95,143,15,31,95,175,175,31,95,175,175,31,95,175,175
        .byte 95,95,0,1,10,25,5,5,10,25,5,5,10,27,175,175
        .byte 82,90,0,36,16,72,37,128,90,0,164,0,72,0,128,0
        .byte 0,0,160,160,240,240,240,240,240,240,80,80,160,160,0,0
        .byte 0,0,0,0,80,80,5,5,80,80,5,5,0,0,0,0
        .byte 8,0,140,0,78,0,175,0,95,8,175,140,95,78,170,171
        .byte 90,90,255,255,90,90,255,255,90,90,255,255,90,90,255,255
        .byte 145,117,80,98,196,0,16,186,196,128,0,34,228,160,136,152
        .byte 245,15,15,250,245,15,15,250,245,15,15,250,245,15,15,250
        .byte 90,90,165,175,95,91,167,167,94,94,173,175,95,90,165,165
        .byte 1,5,3,8,6,0,8,0,4,0,8,0,0,0,8,0
        .byte 170,153,221,85,170,153,221,85,170,153,221,85,170,153,221,85
        .byte 0,0,0,0,0,0,234,0,128,160,170,170,228,228,0,0
        .byte 0,0,0,0,0,32,0,0,245,160,170,170,160,160,0,0
        .byte 90,90,165,165,90,90,4,0,74,10,165,4,74,10,165,4
        .byte 90,90,165,165,90,90,5,5,82,90,37,165,82,90,37,165
        .byte 74,10,165,4,74,10,165,4,64,0,165,165,90,90,165,165
        .byte 0,0,0,0,0,0,0,0,0,0,152,152,145,145,0,0
        .byte 0,0,152,152,145,145,0,0,0,0,0,0,0,0,0,0
        .byte 95,95,15,15,95,95,175,143,95,95,175,143,95,95,175,143
        .byte 31,95,175,175,31,95,175,175,31,95,175,175,15,15,175,175
        .byte 95,95,175,143,95,95,175,143,95,95,175,143,15,31,175,175
        .byte 10,8,5,4,2,10,1,1,8,10,4,5,2,2,1,1
        .byte 17,17,34,34,68,85,136,170,17,85,34,170,85,68,170,136
        .byte 95,95,136,0,93,85,170,170,93,85,170,170,93,85,170,170
        .byte 95,95,0,1,85,85,170,137,85,85,170,137,85,85,170,137
        .byte 93,85,170,170,93,85,170,170,93,85,170,170,93,85,175,175
        .byte 85,85,170,137,85,85,170,137,85,85,170,137,85,85,175,175
        .byte 90,10,164,132,90,10,165,4,90,10,164,132,90,10,165,4
        .byte 82,90,37,165,82,90,37,165,82,90,37,165,82,90,37,165
        .byte 0,0,250,250,128,0,136,0,128,0,136,0,245,245,0,0
        .byte 0,0,175,175,0,17,0,1,0,17,0,1,95,95,0,0
        .byte 0,0,80,80,240,240,240,240,240,240,160,160,80,80,0,0
        .byte 95,95,5,5,95,95,5,5,10,10,5,5,0,0,0,0
        .byte 0,0,39,0,87,95,39,175,87,95,39,175,87,85,39,175
        .byte 95,95,47,15,95,95,47,15,95,15,47,15,95,15,47,15
        .byte 0,0,0,0,95,95,175,175,95,95,175,175,85,85,175,175
        .byte 95,95,15,15,95,95,15,15,15,15,15,15,15,15,15,15
        .byte 165,165,75,75,165,165,90,75,165,165,75,75,165,165,90,75
        .byte 37,165,82,90,37,165,82,90,37,165,82,90,37,165,82,90
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 87,95,39,5,87,95,39,5,87,10,39,5,87,0,39,0
        .byte 12,1,12,0,0,1,0,0,0,0,0,4,1,0,12,0
        .byte 6,0,0,4,0,2,2,0,0,9,0,0,8,5,12,0
        .byte 10,7,2,7,8,2,10,1,5,0,6,8,3,5,0,10
        .byte 37,165,82,90,37,165,82,90,5,5,90,90,165,165,90,90
        .byte 165,165,90,90,165,165,64,0,164,160,90,64,164,160,90,64
        .byte 164,160,90,64,164,160,90,64,4,0,90,90,165,165,90,90
        .byte 165,165,90,90,165,165,80,80,37,165,82,90,37,165,82,90
        .byte 95,95,0,0,10,10,5,5,10,10,5,5,10,10,175,175
        .byte 82,90,37,165,82,90,37,165,80,80,165,165,90,90,165,165
        .byte 17,0,0,136,0,0,0,0,0,0,170,0,68,0,170,0
        .byte 85,85,170,170,95,95,175,175,95,95,175,175,85,85,175,175
        .byte 255,255,136,0,170,170,221,85,170,170,221,85,170,170,221,85
        .byte 170,170,221,85,170,170,221,85,170,170,221,85,170,170,255,255
        .byte 255,255,0,17,170,153,85,85,170,153,85,85,170,153,85,85
        .byte 170,153,85,85,170,153,85,85,170,153,85,85,170,153,255,255
        .byte 0,85,39,0,87,95,39,175,87,95,39,175,87,85,39,175
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 10,10,39,175,19,95,1,175,0,95,0,39,0,19,0,1
        .byte 10,27,175,174,95,76,175,136,95,0,174,0,76,0,136,0
bprop:
        .byte 0
        .byte 5
        .byte 5
        .byte 5
        .byte 5
        .byte 2
        .byte 2
        .byte 6
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 2
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
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 1
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 0
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
.out .sprintf("BLOCKS SIZE = %d", (* - chgfx))

sprgfx:
        .byte 0,0,0,0,0,0,3,192,15,240,15,240,63,252,111,246,179,205,156,57,87,234,50,76,15,240,67,194,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,3,192,7,224,27,216,36,36,51,204,29,56,3,192,8,16,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,0,3,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,1,0,1,0,3,128,15,224,3,128,1,0,1,0,0,0,0,0,0,0,0,0,0,0
        .byte 128,0,0,0,32,8,16,16,12,96,15,224,7,192,3,128,7,192,14,192,12,96,16,48,0,8,64,4,0,0,0,1
        .byte 1,0,1,0,1,0,3,128,10,160,7,192,31,240,247,222,31,240,7,192,10,160,3,128,1,0,1,0,1,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,3,192,0,0,7,224,0,0,15,240,0,0,59,220,59,220,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,0,3,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,1,0,3,128,7,192,3,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,1,0,1,0,5,64,3,128,31,240,3,128,5,64,1,0,1,0,0,0,0,0,0,0,0,0
        .byte 0,0,1,0,1,0,3,0,3,0,3,128,7,240,127,252,31,192,3,128,1,128,1,128,1,0,1,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,63,252,0,0,63,252,0,0,63,252,0,0,63,252,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,4,32,21,40,9,16,53,172,3,64,5,216,31,176,2,192,53,172,8,144,20,168,4,32,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,28,56,62,124,63,252,63,252,31,248,15,240,7,224,3,192,1,128,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,112,0,248,1,252,1,252,1,244,0,232,28,112,46,0,95,28,127,62,127,127,62,127,28,125,0,58,0,28,0,0
        .byte 14,0,29,0,62,128,63,128,63,156,31,58,14,125,0,127,14,127,31,62,63,156,63,128,47,128,23,0,14,0,0,0
        .byte 0,0,0,0,56,56,124,92,254,190,254,254,190,254,92,124,56,56,3,128,7,192,15,224,15,224,15,160,7,64,3,128
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,3,192,7,224,6,96,3,192,0,0,3,192,55,236,96,6,239,247,0,0
        .byte 2,64,0,0,16,8,0,0,0,0,0,0,3,192,247,239,244,47,3,192,0,0,3,192,55,236,96,6,239,247,0,0
        .byte 0,0,239,247,96,6,55,236,3,192,0,0,3,192,6,96,7,224,3,192,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,239,247,96,6,55,236,3,192,0,0,3,192,244,47,247,239,3,192,0,0,0,0,0,0,16,8,0,0,2,64
frmlst:
        .byte 0,3
        .byte 3,4
        .byte 7,2
        .byte 9,4
        .byte 13,1
        .byte 14,1
        .byte 15,2
        .byte 17,2
        .byte 19,2
        .byte 21,4
        .byte 25,2
        .byte 27,2,29,0
.out .sprintf("SPRITES SIZE = %d", (* - sprgfx))

scdat:
        .word 202,40,153,40,226,160,129,147,220,297,154,254,185,294,99,437,295,331,316,165,366,267,199,113,342,139
        .word 199,373,117,206,91,149,367,212,316,332,126,85,209,210,310,237,220,123,362,123,85,183,280,187,77
        .byte 255,0,65,255,20,6,0,31,40,33,35,37,31,0,255,20,6,255,0,8,40,0,26,21,21,21,41,0,0,48,34,38,36
        .byte 39,48,58,59,42,21,21,21,27,0,0,30,255,0,4,30,0,0,18,255,0,18,19,255,0,10,18,255,0,18,19,255,0,10
        .byte 18,255,0,18,19,0,0,0,40,255,0,6,18,255,0,18,19,255,0,5,40,255,0,4,18,255,0,18,19,255,0,10,18
        .byte 255,0,18,19,255,0,10,18,255,0,18,19,255,0,10,18,255,0,18,19,0,30,255,0,7,30,18,255,0,18,19,255,0,10
        .byte 18,255,0,18,19,255,0,10,18,255,0,18,19,40,255,0,9,18,255,0,18,19,255,0,10,18,255,0,18,19,0,0,0
        .byte 30,0,0,30,0,0,0,28,20,20,20,255,0,12,20,20,20,29,255,0,10,255,21,4,41,255,21,10,42,255,21,4,255,0,65
        .byte 255,0,0,255,0,57,24,1,2,22,255,0,26,25,4,3,23,255,0,223,31,40,33,35,37,31,255,0,24,32,34,38,36
        .byte 39,32,58,59,255,0,22
        .byte 255,0,134,70,71,255,0,28,72,73,255,0,18,70,71,70,71,70,71,255,0,4,70,71,255,0,18,72,73,72,73,72,73
        .byte 255,0,4,72,73,255,0,18,70,71,255,0,8,70,71,255,0,5,24,1,2,22,255,0,9,72,73,255,0,8,72,73,255,0,5
        .byte 25,4,3,23,255,0,9,70,71,255,0,8,70,71,255,0,18,72,73,255,0,8,72,73,255,0,18,70,71,70,71,70,71
        .byte 255,0,4,70,71,255,0,18,72,73,72,73,72,73,255,0,4,72,73,255,0,28,70,71,255,0,28,72,73,255,0,104,31
        .byte 40,33,35,37,31,255,0,24,32,34,38,36,39,32,58,59,255,0,22
        .byte 255,0,0,255,0,237,24,1,2,22,255,0,26,25,4,3,23,255,0,43,31,40,33,35,37,31,255,0,24,32,34,38,36
        .byte 39,32,58,59,255,0,22
        .byte 255,0,38,86,255,0,14,86,255,0,14,70,71,82,80,80,255,82,7,80,80,70,71,255,0,14,72,73,79,87,87,255,79,7
        .byte 87,87,72,73,255,0,28,70,71,255,0,28,72,73,255,0,28,70,71,255,0,28,72,73,255,0,28,70,71,255,0,28,72
        .byte 73,255,0,14,70,71,70,71,70,71,70,71,255,0,6,70,71,255,0,14,72,73,72,73,72,73,72,73,255,0,6,72,73
        .byte 255,0,14,70,71,0,0,0,24,1,2,22,255,0,5,70,71,255,0,14,72,73,0,0,0,25,4,3,23,255,0,5,72
        .byte 73,255,0,14,70,71,255,0,12,70,71,255,0,14,72,73,255,0,12,72,73,255,0,14,70,71,255,0,12,70,71,255,0,14
        .byte 72,73,255,0,12,72,73,255,0,14,70,71,82,80,80,255,82,7,80,80,70,71,255,0,6,31,40,33,35,37,31,0,0
        .byte 72,73,79,87,87,255,79,7,87,87,72,73,255,0,6,32,34,38,36,39,32,58,59,255,0,22
        .byte 255,0,126,80,82,80,80,0,0,80,82,80,80,82,80,0,0,80,82,80,80,255,0,12,87,79,87,87,0,0,87,79,87,87
        .byte 79,87,0,0,87,79,87,87,255,0,102,10,11,13,255,0,13,10,11,13,255,0,11,6,6,14,255,0,4,24,1,2,22
        .byte 255,0,5,6,6,14,255,0,11,6,6,15,0,10,11,13,25,4,3,23,0,10,11,13,0,6,6,15,255,0,15,6,6
        .byte 14,255,0,5,6,6,14,255,0,19,6,6,15,255,0,5,6,6,15,255,0,21,80,82,80,80,82,80,255,0,24,87,79
        .byte 87,87,79,87,255,0,102,31,40,33,35,37,31,255,0,24,32,34,38,36,39,32,58,59,255,0,22
        .byte 255,0,124,46,65,0,0,70,71,255,0,10,70,71,0,0,46,65,255,0,8,66,67,0,0,72,73,255,0,10,72,73,0,0
        .byte 66,67,255,0,69,24,1,2,22,255,0,5,99,101,255,0,19,25,4,3,23,255,0,5,100,102,255,0,78,46,65,0,0
        .byte 70,71,255,0,10,70,71,0,0,46,65,255,0,8,66,67,0,0,72,73,255,0,10,72,73,0,0,66,67,255,0,18,99
        .byte 101,255,0,28,100,102,255,0,104,31,40,33,35,37,31,255,0,24,32,34,38,36,39,32,58,59,255,0,22
        .byte 255,0,124,43,255,83,4,51,255,0,24,57,255,0,4,57,255,0,24,57,255,0,4,57,0,0,0,24,1,2,22,255,0,17
        .byte 57,255,0,4,57,0,0,0,25,4,3,23,255,0,17,57,255,0,4,57,255,0,12,46,46,65,65,255,0,8,108,255,83,4
        .byte 109,255,0,12,66,66,67,67,255,0,18,43,255,83,4,51,255,0,24,57,255,0,4,57,255,0,24,57,255,0,4,57,255,0,24
        .byte 57,255,0,4,57,255,0,24,57,255,0,4,57,255,0,24,108,255,83,4,109,255,0,100,31,40,33,35,37,31,255,0,24
        .byte 32,34,38,36,39,32,58,59,255,0,22
        .byte 255,0,14,10,11,13,255,0,13,43,51,255,0,12,45,45,14,255,0,11,43,51,54,54,255,0,12,45,45,14,255,0,11
        .byte 255,54,4,255,0,12,45,45,15,255,0,11,255,54,4,255,0,26,255,54,4,255,0,26,255,54,4,255,0,26,255,54,4
        .byte 255,0,26,255,54,4,0,0,0,24,1,2,22,255,0,5,94,92,255,0,12,255,54,4,0,0,0,25,4,3,23,255,0,5
        .byte 76,77,255,0,12,255,54,4,255,0,12,85,84,255,0,12,255,54,4,255,0,12,85,84,255,0,12,255,54,4,255,0,12
        .byte 76,77,255,0,12,255,54,4,255,0,12,91,93,255,0,12,255,54,4,255,0,26,255,54,4,255,0,26,255,54,4,255,0,26
        .byte 255,54,4,255,0,12,10,11,13,255,0,11,54,54,108,109,255,0,12,45,45,14,255,0,11,108,109,31,40,33,35,37,31
        .byte 255,0,8,45,45,15,255,0,13,32,34,38,36,39,32,58,59,255,0,22
        .byte 70,71,255,0,10,66,67,255,0,14,66,67,98,98,80,80,255,0,8,69,68,255,0,14,69,68,83,83,81,81,255,0,8
        .byte 68,69,255,0,14,68,69,66,67,255,0,10,69,68,255,0,14,69,68,68,69,255,0,10,68,69,255,0,14,68,69,69,68
        .byte 255,0,10,70,71,255,0,14,70,71,68,69,255,0,10,103,98,82,80,80,82,82,80,255,0,6,80,82,98,103,69,68,255,0,10
        .byte 81,83,83,81,81,83,83,81,255,0,6,81,83,83,81,68,69,255,0,16,66,67,0,24,1,2,22,0,66,67,66,67,69
        .byte 68,255,0,16,69,68,0,25,4,3,23,0,69,68,0,0,68,69,255,0,16,68,69,255,0,6,68,69,0,0,70,71,255,0,16
        .byte 70,71,255,0,6,70,71,0,0,98,98,80,255,82,4,80,80,255,82,4,80,255,0,4,103,98,82,82,82,80,80,82,98,98
        .byte 82,80,83,83,81,255,83,4,81,81,255,83,4,81,255,0,4,81,255,83,4,81,81,255,83,4,81,255,0,12,66,67,255,0,28
        .byte 69,68,255,0,28,68,69,255,0,28,69,68,255,0,16,5,95,95,47,255,0,8,68,69,255,0,14,5,47,31,40,33,35
        .byte 37,31,255,0,6,69,68,255,0,16,32,34,38,36,39,32,58,59,255,0,4,70,71,255,0,16
        .byte 255,0,44,94,92,255,0,28,76,77,255,0,28,85,84,255,0,28,85,84,255,0,28,85,84,255,0,28,76,77,255,0,28
        .byte 91,93,255,0,75,70,71,82,80,82,82,82,80,82,70,71,0,24,1,2,22,0,70,71,82,80,82,82,82,80,82,70,71
        .byte 0,0,72,73,79,87,79,79,79,87,79,72,73,0,25,4,3,23,0,72,73,79,87,79,79,79,87,79,72,73,255,0,75
        .byte 94,92,255,0,28,76,77,255,0,28,85,84,255,0,28,85,84,255,0,28,85,84,255,0,14,31,40,33,35,37,31,255,0,8
        .byte 76,77,255,0,14,32,34,38,36,39,32,58,59,255,0,6,91,93,255,0,14
        .byte 255,0,40,46,65,255,0,6,46,65,255,0,20,66,67,255,0,6,66,67,255,0,20,78,49,255,0,6,78,49,255,0,20
        .byte 70,71,255,0,6,70,71,255,0,20,72,73,255,0,6,72,73,255,0,20,70,71,0,24,1,2,22,0,70,71,255,0,20
        .byte 72,73,0,25,4,3,23,0,72,73,255,0,11,86,46,65,255,0,4,99,101,98,103,82,82,80,80,82,82,103,98,99,101
        .byte 255,0,4,46,65,0,86,0,86,66,67,255,0,4,100,102,79,87,79,79,87,87,79,79,87,79,100,102,255,0,4,66,67
        .byte 0,86,255,0,121,86,70,71,70,71,70,71,70,71,255,0,10,70,71,70,71,70,71,70,71,0,86,0,86,72,73,72,73
        .byte 72,73,72,73,255,0,10,72,73,72,73,72,73,72,73,0,86,255,0,10,70,71,255,0,6,70,71,255,0,20,72,73,255,0,6
        .byte 72,73,255,0,20,78,49,255,0,6,78,49,255,0,10,31,40,33,35,37,31,255,0,4,46,65,255,0,6,46,65,255,0,10
        .byte 32,34,38,36,39,32,58,59,0,0,66,67,255,0,6,66,67,255,0,10
        .byte 255,0,38,94,92,255,0,10,94,92,255,0,16,85,84,255,0,10,85,84,255,0,16,76,77,255,0,10,76,77,255,0,16
        .byte 91,93,255,0,10,91,93,255,0,16,255,78,4,50,255,49,4,50,255,49,4,255,0,21,24,1,2,22,255,0,26,25,4
        .byte 3,23,255,0,77,46,65,82,80,82,80,82,82,82,80,80,82,80,80,82,82,82,80,82,80,46,65,255,0,8,66,67,83
        .byte 81,83,81,83,83,83,81,81,83,81,81,83,83,83,81,83,81,66,67,255,0,132,255,78,4,50,78,78,49,49,50,255,49,4
        .byte 255,0,16,94,92,255,0,10,94,92,255,0,16,76,77,255,0,10,76,77,255,0,8,31,40,33,35,37,31,0,0,91,93
        .byte 255,0,10,91,93,255,0,8,32,34,38,36,39,32,58,59,255,0,22
        .byte 255,0,90,46,65,83,83,81,255,83,4,81,255,83,5,81,255,83,5,81,255,83,4,81,83,46,65,66,67,255,0,26,66
        .byte 67,46,65,255,0,26,46,65,66,67,255,0,26,66,67,46,65,255,0,26,46,65,66,67,255,0,10,78,50,78,49,50,49
        .byte 255,0,10,66,67,78,49,255,0,10,57,255,0,4,57,255,0,10,78,49,46,65,255,0,10,57,255,0,4,57,255,0,10
        .byte 46,65,66,67,255,0,10,57,255,0,4,57,255,0,10,66,67,255,0,4,46,65,255,0,6,57,255,0,4,57,255,0,6
        .byte 46,65,255,0,8,66,67,255,0,6,57,255,0,4,57,255,0,6,66,67,255,0,8,46,65,255,0,6,57,255,0,4,57
        .byte 0,24,1,2,22,0,46,65,255,0,8,66,67,255,0,6,57,255,0,4,57,0,25,4,3,23,0,66,67,255,0,8,46
        .byte 65,255,0,6,57,255,0,4,57,255,0,6,46,65,255,0,8,66,67,255,0,6,57,255,0,4,57,255,0,6,66,67,255,0,4
        .byte 83,83,81,255,83,5,81,83,83,83,57,255,0,4,57,255,83,4,81,255,83,4,81,83,83,31,40,33,35,37,31,255,0,6
        .byte 57,255,0,4,57,255,0,12,32,34,38,36,39,32,58,59,255,0,4,57,255,0,4,57,255,0,12
        .byte 255,0,104,10,11,13,255,0,27,6,6,14,255,0,27,6,6,15,255,0,77,80,82,82,80,255,0,5,24,1,2,22,255,0,5
        .byte 80,82,82,80,255,0,8,81,83,83,81,255,0,5,25,4,3,23,255,0,5,81,83,83,81,255,0,50,13,255,0,27,6,6
        .byte 14,255,0,27,6,6,15,255,0,163,31,40,33,35,37,31,255,0,24,32,34,38,36,39,32,58,59,255,0,22
        .byte 0,57,255,0,6,85,84,0,57,255,0,6,57,0,85,84,255,0,6,57,0,0,57,255,0,6,85,84,0,57,255,0,6
        .byte 57,0,91,93,255,0,6,57,0,0,57,255,0,6,85,84,0,57,255,0,6,103,5,47,5,47,80,80,82,82,82,103,82
        .byte 0,57,255,0,6,91,93,0,57,255,0,6,81,5,9,9,47,81,81,83,83,83,81,83,82,103,82,82,82,80,80,5,47
        .byte 5,47,103,255,0,8,64,64,255,0,6,57,0,83,81,83,83,83,81,81,5,9,9,47,81,255,0,8,7,7,255,0,6
        .byte 57,0,0,57,255,0,6,64,64,255,0,10,7,7,255,0,6,57,0,0,57,255,0,6,7,7,255,0,10,7,7,255,0,6
        .byte 57,0,0,57,0,24,1,2,22,0,7,7,255,0,10,7,7,255,0,6,57,0,0,57,0,25,4,3,23,0,7,7,255,0,10
        .byte 7,7,255,0,6,57,0,0,57,255,0,6,7,7,255,0,10,7,7,255,0,6,57,0,0,57,255,0,6,7,7,255,0,10
        .byte 7,7,255,0,6,57,0,0,57,255,0,6,7,7,255,0,10,7,7,255,0,6,57,0,0,57,255,0,6,7,7,255,0,10
        .byte 63,63,255,0,6,57,0,0,57,255,0,6,7,7,255,0,8,80,5,8,8,47,80,80,82,82,82,103,82,0,57,255,0,6
        .byte 63,63,255,0,8,81,5,47,5,47,81,81,83,83,83,81,83,82,103,82,82,82,80,80,5,8,8,47,80,255,0,6,57
        .byte 0,94,92,255,0,6,57,0,83,81,83,83,83,81,81,5,47,5,47,81,255,0,6,57,0,85,84,255,0,6,57,0,0
        .byte 57,255,0,6,94,92,0,57,255,0,6,57,0,85,84,255,0,6,57,0,31,57,33,35,37,31,0,0,85,84,0,57,255,0,6
        .byte 57,0,85,84,255,0,6,57,0,32,34,38,36,39,32,58,59,85,84,0,57,255,0,6,57,0,85,84,255,0,6,57,0
        .byte 255,0,14,75,74,255,0,20,70,71,0,0,78,78,50,78,96,62,49,50,49,49,255,0,16,72,73,0,0,70,95,95,255,55,4
        .byte 95,95,71,255,0,16,70,71,0,0,72,95,95,55,8,8,55,95,95,73,255,0,16,72,73,255,0,6,64,64,255,0,20
        .byte 70,71,255,0,6,7,7,255,0,20,72,73,255,0,6,7,7,255,0,20,70,71,255,0,6,7,7,255,0,20,72,73,255,0,6
        .byte 7,7,255,0,4,70,71,255,0,14,70,71,255,0,6,7,7,255,0,4,72,73,255,0,14,72,73,255,0,6,7,7,255,0,4
        .byte 70,71,255,0,14,70,71,255,0,6,7,7,255,0,4,72,73,255,0,14,72,73,0,24,1,2,22,0,7,7,255,0,4
        .byte 70,71,255,0,14,70,71,0,25,4,3,23,0,7,7,255,0,4,72,73,255,0,14,72,73,255,0,6,7,7,255,0,20
        .byte 70,71,255,0,6,7,7,255,0,20,72,73,255,0,6,7,7,255,0,20,70,71,255,0,6,63,63,255,0,20,72,73,0,0
        .byte 70,95,95,55,52,52,55,95,95,71,255,0,10,31,40,33,35,37,31,255,0,4,72,95,95,255,55,4,95,95,73,255,0,10
        .byte 32,34,38,36,39,32,58,59,0,0,78,78,50,78,61,60,49,50,49,49,255,0,10
        .byte 255,0,6,57,0,85,84,0,57,255,0,6,57,0,85,84,0,57,255,0,12,57,0,85,84,0,57,255,0,6,57,0,91
        .byte 93,0,57,255,0,12,57,0,85,84,0,57,255,0,6,99,70,95,95,71,101,255,0,12,57,0,85,84,49,49,50,78,78
        .byte 50,78,78,100,72,9,9,73,102,255,0,12,57,0,85,84,0,57,255,0,8,64,64,255,0,14,57,0,91,93,0,57,255,0,8
        .byte 7,7,255,0,14,99,70,95,95,71,101,255,0,8,7,7,255,0,14,100,72,9,9,73,102,255,0,8,7,7,255,0,16
        .byte 64,64,255,0,10,7,7,255,0,16,7,7,255,0,10,7,7,255,0,16,7,7,0,0,0,24,1,2,22,0,0,0,7,7
        .byte 255,0,16,7,7,0,0,0,25,4,3,23,0,0,0,7,7,255,0,16,7,7,255,0,10,7,7,255,0,16,7,7,255,0,10
        .byte 63,63,255,0,16,7,7,255,0,8,99,70,8,8,71,101,255,0,14,7,7,255,0,8,100,72,95,95,73,102,255,0,14
        .byte 7,7,255,0,8,57,0,94,92,0,57,255,0,14,63,63,255,0,8,57,0,85,84,0,57,255,0,12,99,70,8,8,71
        .byte 101,78,78,50,49,49,50,78,78,85,84,0,57,255,0,6,31,40,33,35,37,31,100,72,95,95,73,102,255,0,6,57,0
        .byte 85,84,0,57,255,0,6,32,34,38,36,39,32,58,59,94,92,255,0,8,57,0,85,84,0,57,255,0,6
        .byte 255,0,20,75,74,255,0,28,96,62,255,0,26,70,255,55,4,71,255,0,9,78,78,78,50,49,49,49,50,49,49,49,50
        .byte 78,78,78,72,55,8,8,55,73,255,0,8,70,255,55,4,71,255,0,12,64,64,0,0,70,71,50,70,71,0,0,86,72
        .byte 55,8,8,55,73,255,0,12,7,7,0,0,72,73,49,72,73,255,0,5,64,64,255,0,14,7,7,255,0,12,7,7,255,0,14
        .byte 7,7,255,0,12,7,7,255,0,14,7,7,255,0,12,7,7,255,0,14,7,7,255,0,12,7,7,0,0,80,78,70,71
        .byte 255,0,8,7,7,255,0,12,7,7,0,0,87,78,72,73,255,0,8,7,7,255,0,12,7,7,255,0,14,7,7,255,0,12
        .byte 7,7,255,0,14,63,63,255,0,12,7,7,255,0,12,70,55,52,52,55,71,255,0,10,63,63,255,0,12,72,255,55,4
        .byte 73,255,0,7,86,70,55,9,9,55,71,0,0,0,24,1,2,22,0,0,0,80,82,61,60,82,80,82,82,80,70,71,0,0,0
        .byte 72,255,55,4,73,0,0,0,25,4,3,23,0,0,0,87,79,75,74,79,87,79,79,87,72,109,255,0,4,78,78,78,50
        .byte 49,49,49,50,255,49,4,50,49,49,0,0,75,74,255,0,8,31,40,33,35,37,31,255,0,14,75,74,255,0,8,32,34
        .byte 38,36,39,32,58,59,255,0,12,75,74,255,0,8
        .byte 255,0,184,94,92,99,71,99,71,255,0,10,94,92,99,71,99,71,255,0,8,85,84,72,102,72,102,255,0,10,85,84,72
        .byte 102,72,102,255,0,8,76,77,255,0,14,76,77,255,0,12,85,84,255,0,14,85,84,255,0,12,85,84,255,0,14,85,84
        .byte 0,24,1,2,22,255,0,7,85,84,255,0,14,85,84,0,25,4,3,23,255,0,7,85,84,255,0,14,85,84,255,0,12
        .byte 76,77,255,0,14,76,77,255,0,12,85,84,99,71,99,71,255,0,10,85,84,99,71,99,71,255,0,8,91,93,72,102,72
        .byte 102,255,0,10,91,93,72,102,72,102,255,0,94,31,40,33,35,37,31,255,0,24,32,34,38,36,39,32,58,59,255,0,22
        .byte 0,0,0,57,255,0,10,85,84,255,0,10,57,255,0,6,57,0,0,0,57,255,0,4,86,0,91,93,0,86,255,0,4
        .byte 57,0,0,0,57,255,0,6,57,0,0,0,57,255,0,4,99,70,95,95,71,101,255,0,4,57,0,0,0,57,255,0,5
        .byte 5,255,95,4,109,255,0,4,100,72,9,9,73,102,255,0,4,108,255,95,4,47,255,0,5,57,0,24,1,2,22,255,0,5
        .byte 64,64,255,0,10,57,255,0,6,57,0,25,4,3,23,255,0,5,7,7,255,0,10,57,255,0,6,57,255,0,10,7,7
        .byte 255,0,10,57,255,0,6,57,255,0,10,7,7,255,0,10,57,255,0,6,57,255,0,10,7,7,255,0,10,57,255,0,6
        .byte 57,255,0,10,7,7,255,0,10,57,255,0,5,5,255,95,4,47,255,0,6,7,7,255,0,6,5,255,95,4,47,255,0,4
        .byte 5,255,95,4,47,255,0,6,7,7,255,0,6,5,255,95,4,109,255,0,5,57,255,0,10,7,7,255,0,10,57,255,0,6
        .byte 57,255,0,10,7,7,255,0,10,57,255,0,6,57,255,0,10,7,7,255,0,10,57,255,0,6,57,255,0,10,7,7,255,0,10
        .byte 57,255,0,5,108,255,95,4,51,255,0,6,7,7,255,0,6,43,255,95,4,47,255,0,5,57,0,0,0,57,255,0,6
        .byte 63,63,255,0,6,57,0,0,0,57,255,0,10,57,255,0,4,99,70,8,8,71,101,255,0,4,57,255,0,7,31,40,33
        .byte 35,37,31,255,0,6,100,72,95,95,73,102,255,0,12,32,34,38,36,39,32,58,59,255,0,6,94,92,255,0,14
        .byte 255,0,34,61,60,255,0,18,61,60,255,0,8,76,77,255,0,18,75,74,255,0,8,75,74,255,0,18,76,77,255,0,8
        .byte 75,74,255,0,18,75,74,255,0,8,75,74,255,0,8,10,11,13,255,0,7,96,62,255,0,8,75,74,255,0,8,6,6
        .byte 14,255,0,17,76,77,255,0,8,6,6,15,255,0,9,86,255,0,7,75,74,255,0,7,24,1,2,22,255,0,7,61,60
        .byte 255,0,8,75,74,0,0,10,11,13,0,0,25,4,3,23,0,0,0,10,11,13,0,76,77,255,0,8,75,74,0,0,6,6
        .byte 14,255,0,9,6,6,14,0,75,74,255,0,8,75,74,0,0,6,6,15,0,0,0,10,11,13,0,0,0,6,6,15,0
        .byte 75,74,255,0,8,76,77,255,0,8,6,6,14,255,0,7,75,74,255,0,8,96,62,255,0,8,6,6,15,255,0,7,76
        .byte 77,255,0,28,75,74,255,0,7,86,255,0,20,75,74,255,0,8,61,60,255,0,18,75,74,255,0,8,76,77,255,0,18
        .byte 76,77,255,0,8,96,62,255,0,18,96,62,255,0,4,31,40,33,35,37,31,255,0,24,32,34,38,36,39,32,58,59,255,0,22
        .byte 255,0,34,46,65,255,0,4,99,101,255,0,6,99,101,255,0,4,46,65,255,0,8,66,67,255,0,4,100,102,255,0,6
        .byte 100,102,255,0,4,66,67,255,0,38,46,65,255,0,4,99,101,255,0,6,99,101,255,0,4,46,65,255,0,8,66,67,255,0,4
        .byte 100,102,255,0,6,100,102,255,0,4,66,67,255,0,248,46,65,255,0,4,99,101,82,80,82,80,82,80,99,101,255,0,4
        .byte 46,65,255,0,8,66,67,255,0,4,100,102,83,81,83,81,83,81,100,102,255,0,4,66,67,255,0,17,24,1,2,22,255,0,17
        .byte 46,65,255,0,4,99,101,0,25,4,3,23,0,99,101,255,0,4,46,65,255,0,8,66,67,255,0,4,100,102,255,0,6
        .byte 100,102,255,0,4,66,67,255,0,4,31,40,33,35,37,31,255,0,24,32,34,38,36,39,32,58,59,255,0,22
        .byte 255,0,136,70,71,82,80,82,80,70,51,255,0,22,72,73,79,87,79,87,72,73,255,0,88,94,92,255,0,28,85,84,255,0,21
        .byte 24,1,2,22,0,0,0,76,77,255,0,21,25,4,3,23,0,0,0,91,93,255,0,72,46,65,82,82,82,80,82,82,80
        .byte 82,82,82,46,65,255,0,16,66,67,83,83,83,81,83,83,81,83,83,83,66,67,255,0,100,31,40,33,35,37,31,255,0,24
        .byte 32,34,38,36,39,32,58,59,255,0,22
        .byte 0,20,255,0,11,85,84,85,84,255,0,11,20,0,0,57,255,0,8,86,0,0,91,93,91,93,0,0,86,255,0,8,57
        .byte 0,0,57,255,0,8,80,5,95,95,47,95,95,95,47,80,255,0,8,57,0,0,57,255,0,8,81,5,255,9,6,47,81
        .byte 255,0,8,57,0,43,71,70,71,70,71,255,0,6,255,64,6,255,0,10,57,0,72,73,72,73,72,73,255,0,6,255,7,6
        .byte 255,0,10,57,0,0,57,255,0,10,255,7,6,255,0,10,57,0,0,57,255,0,10,255,7,6,255,0,10,57,0,0,57
        .byte 0,24,1,2,22,255,0,5,255,7,6,255,0,10,57,0,0,57,0,25,4,3,23,255,0,5,255,7,6,255,0,10,57
        .byte 0,0,57,255,0,10,255,7,6,255,0,10,57,0,0,57,255,0,10,255,7,6,255,0,10,57,0,70,71,70,71,70,71
        .byte 255,0,6,255,7,6,255,0,8,70,71,70,51,72,73,72,73,72,73,255,0,6,255,7,6,255,0,8,72,73,72,73,0
        .byte 57,255,0,10,255,7,6,255,0,10,57,0,0,57,255,0,10,255,7,6,255,0,10,57,0,0,57,255,0,10,255,7,6
        .byte 255,0,10,57,0,0,57,255,0,10,255,63,6,255,0,10,57,0,0,57,255,0,8,80,5,255,8,6,47,80,255,0,8
        .byte 57,0,31,40,33,35,37,31,255,0,4,81,5,95,95,47,5,95,95,47,81,255,0,8,21,0,32,34,38,36,39,32,58
        .byte 59,255,0,5,94,92,94,92,255,0,13
        .byte 255,0,48,46,65,255,0,28,66,67,255,0,28,100,102,255,0,92,99,101,255,0,28,46,65,255,0,16,99,101,255,0,8
        .byte 99,46,66,46,65,101,255,0,14,46,65,255,0,8,100,66,67,65,67,102,255,0,12,99,46,66,46,65,101,0,24,1,2
        .byte 22,0,0,0,66,67,255,0,14,100,66,67,65,67,102,0,25,4,3,23,0,0,0,100,102,255,0,16,66,67,255,0,28
        .byte 100,102,255,0,156,99,101,255,0,10,31,40,33,35,37,31,255,0,12,46,65,255,0,10,32,34,38,36,39,32,58,59,255,0,10
        .byte 66,67,255,0,10
        .byte 255,0,34,78,50,50,49,255,0,14,78,50,50,49,255,0,8,70,71,70,71,255,0,14,70,71,70,71,255,0,8,72,73
        .byte 72,73,255,0,14,72,73,72,73,255,0,12,70,71,255,0,28,72,73,255,0,30,46,65,255,0,6,46,65,255,0,20,66
        .byte 67,255,0,6,66,67,255,0,22,80,80,82,82,80,80,255,0,24,81,81,83,83,81,81,255,0,19,24,1,2,22,255,0,26
        .byte 25,4,3,23,255,0,31,80,80,82,82,80,80,255,0,24,81,81,83,83,81,81,255,0,22,46,65,255,0,6,46,65,255,0,20
        .byte 66,67,255,0,6,66,67,255,0,18,70,71,255,0,10,70,71,255,0,16,72,73,255,0,10,72,73,255,0,12,78,50,50
        .byte 49,255,0,14,78,50,50,49,255,0,4,31,40,33,35,37,31,255,0,24,32,34,38,36,39,32,58,59,255,0,22
        .byte 255,0,16,85,84,255,0,7,57,255,0,14,99,44,101,0,0,0,91,93,0,0,0,99,44,101,0,57,255,0,14,44,100
        .byte 44,0,99,5,47,5,47,101,0,44,100,44,0,57,255,0,14,100,44,102,0,100,5,9,9,47,102,0,100,44,102,0,57
        .byte 255,0,5,44,255,0,14,64,64,255,0,6,255,44,4,101,0,0,57,255,0,14,7,7,255,0,6,100,44,44,44,102,0,0
        .byte 57,255,0,14,7,7,255,0,8,57,255,0,4,57,255,0,14,7,7,255,0,8,57,255,0,4,57,255,0,8,99,44,255,0,4
        .byte 7,7,0,0,0,24,1,2,22,0,57,255,0,4,57,255,0,8,44,44,255,0,4,7,7,0,0,0,25,4,3,23,0
        .byte 57,255,0,4,57,255,0,8,44,44,255,0,4,7,7,255,0,8,57,255,0,4,57,255,0,8,44,44,255,0,4,7,7
        .byte 255,0,8,57,255,0,4,57,255,0,8,44,44,255,0,4,7,7,255,0,6,99,44,44,44,101,0,0,44,255,0,8,100
        .byte 44,255,0,4,7,7,255,0,6,255,44,4,102,255,0,11,44,44,255,0,4,7,7,255,0,6,44,44,255,0,14,44,44
        .byte 255,0,4,7,7,255,0,6,44,44,255,0,14,44,44,255,0,4,7,7,255,0,6,100,44,255,0,14,44,44,255,0,4
        .byte 63,63,255,0,6,44,44,255,0,14,44,44,101,0,99,5,8,8,47,101,0,44,44,44,99,44,255,0,4,31,40,33,35
        .byte 37,31,255,0,4,100,44,102,0,100,5,47,5,47,102,0,100,44,100,44,102,255,0,4,32,34,38,36,39,32,58,59,255,0,8
        .byte 94,92,255,0,12
        .byte 255,0,96,10,11,13,255,0,5,10,11,13,255,0,5,10,11,13,255,0,11,45,45,14,255,0,5,45,45,14,255,0,5
        .byte 45,45,14,255,0,11,45,45,15,255,0,5,45,45,15,255,0,5,45,45,15,255,0,255,99,101,82,80,82,82,80,82,99
        .byte 101,255,0,20,100,102,83,81,83,83,81,83,100,102,255,0,23,24,1,2,22,255,0,26,25,4,3,23,255,0,43,31,40
        .byte 33,35,37,31,255,0,24,32,34,38,36,39,32,58,59,255,0,22
        .byte 255,0,91,46,65,78,78,50,78,78,50,49,49,50,78,78,50,50,49,49,50,49,49,50,78,78,50,78,78,46,65,0,0
        .byte 66,67,65,255,0,22,46,66,67,0,0,0,66,67,255,0,22,66,67,0,0,0,46,65,65,255,0,9,24,1,2,22,255,0,9
        .byte 46,46,65,0,0,66,67,67,255,0,9,25,4,3,23,255,0,9,66,66,67,255,0,15,70,71,255,0,28,72,73,255,0,75
        .byte 46,46,65,255,0,4,70,71,53,53,70,71,0,0,70,71,53,53,70,71,255,0,4,46,65,65,0,0,66,66,67,255,0,4
        .byte 72,73,53,53,72,73,0,0,72,73,53,53,72,73,255,0,4,66,67,67,255,0,122,255,78,4,50,49,49,50,49,49,50
        .byte 49,49,50,50,78,78,50,78,78,50,78,78,50,255,49,4,0,31,40,33,35,37,31,255,0,24,32,34,38,36,39,32,58
        .byte 59,255,0,22
        .byte 255,0,112,10,11,13,255,0,27,6,6,14,255,0,27,6,6,15,255,0,18,24,1,2,22,255,0,26,25,4,3,23,255,0,59
        .byte 10,11,13,255,0,27,6,6,14,255,0,27,6,6,15,255,0,111,10,11,13,255,0,27,6,6,14,255,0,27,6,6,15
        .byte 255,0,47,31,40,33,35,37,31,255,0,24,32,34,38,36,39,32,58,59,255,0,22
        .byte 255,0,14,78,49,255,0,28,43,51,255,0,28,70,71,255,0,28,72,73,255,0,156,70,71,255,0,28,72,73,255,0,19
        .byte 24,1,2,22,0,0,0,70,71,70,71,255,0,19,25,4,3,23,0,0,0,72,73,72,73,255,0,26,70,71,255,0,28
        .byte 72,73,255,0,14,80,80,82,82,80,80,82,82,82,80,80,82,82,82,80,80,255,0,14,87,87,79,79,87,87,79,79,79
        .byte 87,87,79,79,79,87,87,255,0,52,70,71,255,0,28,72,73,255,0,14,31,40,33,35,37,31,255,0,8,108,109,255,0,14
        .byte 32,34,38,36,39,32,58,59,255,0,6,78,49,255,0,14
        .byte 255,0,14,75,74,255,0,16,43,51,255,0,6,86,0,0,0,96,62,0,0,0,86,255,0,6,43,51,255,0,4,76,77
        .byte 255,0,6,80,80,82,255,55,4,82,80,80,255,0,6,76,77,0,0,0,86,108,109,255,0,6,81,81,83,55,8,8,55
        .byte 83,81,81,255,0,6,108,109,86,255,0,15,64,64,255,0,28,7,7,255,0,22,46,65,255,0,4,7,7,255,0,4,46
        .byte 65,255,0,16,66,67,255,0,4,7,7,255,0,4,66,67,255,0,9,86,43,51,0,24,1,2,22,255,0,5,7,7,255,0,10
        .byte 43,51,86,0,0,0,76,77,0,25,4,3,23,255,0,5,7,7,255,0,10,76,77,255,0,4,12,17,255,0,10,7,7
        .byte 255,0,10,12,17,255,0,4,12,17,255,0,10,7,7,255,0,10,12,17,255,0,4,46,65,46,65,46,65,46,65,46,65
        .byte 0,0,7,7,0,0,46,65,46,65,46,65,46,65,46,65,255,0,4,66,67,66,67,66,67,66,67,66,67,0,0,7,7
        .byte 0,0,66,67,66,67,66,67,66,67,66,67,255,0,4,12,17,255,0,10,7,7,255,0,10,12,17,255,0,4,12,17,255,0,10
        .byte 7,7,255,0,10,12,17,255,0,4,12,17,255,0,10,7,7,255,0,10,12,17,255,0,4,76,77,255,0,10,63,63,255,0,10
        .byte 76,77,255,0,4,108,109,255,0,6,80,80,82,55,52,52,55,82,80,80,255,0,6,108,109,0,0,31,40,33,35,37,31
        .byte 255,0,4,81,81,83,255,55,4,83,81,81,255,0,10,32,34,38,36,39,32,58,59,255,0,6,61,60,255,0,14
        .byte 255,0,10,85,84,255,0,26,86,0,91,93,0,86,255,0,24,46,5,47,5,47,65,255,0,24,66,5,9,9,47,67,255,0,26
        .byte 64,64,255,0,28,7,7,255,0,22,46,65,46,65,0,0,7,7,255,0,22,66,46,65,67,0,0,7,7,255,0,22,46
        .byte 66,46,65,0,0,7,7,255,0,22,66,67,66,67,0,0,7,7,255,0,28,7,7,255,0,4,46,65,46,65,255,0,20
        .byte 7,7,255,0,4,66,46,65,67,255,0,20,7,7,255,0,4,46,66,46,65,255,0,20,7,7,255,0,4,66,67,66,67
        .byte 255,0,20,7,7,255,0,9,24,1,2,22,255,0,15,7,7,255,0,9,25,4,3,23,255,0,15,7,7,255,0,28,63,63
        .byte 255,0,26,46,5,8,8,47,65,255,0,16,31,40,33,35,37,31,0,0,66,5,47,5,47,67,255,0,16,32,34,38,36
        .byte 39,32,58,59,0,0,94,92,255,0,18
        .byte 255,0,10,70,71,255,0,8,70,71,255,0,12,46,65,255,0,4,103,103,82,82,255,80,4,82,82,103,103,255,0,12,66
        .byte 67,255,0,4,81,81,83,83,255,81,4,83,83,81,81,255,0,12,78,49,255,0,4,78,50,49,49,50,49,49,50,49,49
        .byte 50,49,255,0,12,46,65,255,0,4,66,67,255,0,8,66,67,255,0,12,66,67,255,0,4,69,68,255,0,8,69,68,255,0,18
        .byte 68,69,0,0,0,24,1,2,22,0,68,69,255,0,18,69,68,0,0,0,25,4,3,23,0,69,68,255,0,18,68,69,255,0,8
        .byte 68,69,255,0,18,70,71,255,0,8,70,71,255,0,12,80,80,82,80,80,82,103,103,255,0,4,80,80,82,82,103,103,255,0,12
        .byte 81,81,83,81,81,83,81,81,255,0,4,81,81,83,83,81,81,255,0,12,66,67,255,0,14,66,67,255,0,12,69,68,255,0,14
        .byte 69,68,255,0,12,68,69,255,0,14,68,69,255,0,12,69,68,255,0,14,69,68,255,0,12,68,69,255,0,6,46,65,255,0,6
        .byte 68,69,255,0,12,69,68,255,0,6,66,67,255,0,6,70,71,255,0,12,68,69,255,0,4,80,80,82,82,82,80,80,80
        .byte 82,82,103,103,255,0,8,31,40,33,35,37,31,255,0,4,87,87,79,79,79,87,87,87,79,79,87,87,255,0,8,32,34
        .byte 38,36,39,32,58,59,255,0,22
        .byte 255,0,32,43,51,78,50,255,49,4,50,49,255,0,10,43,51,78,78,50,49,43,51,0,0,76,77,70,71,70,71,70,71
        .byte 70,71,255,0,10,76,77,70,71,70,71,76,77,0,0,54,54,72,73,72,73,72,73,72,73,255,0,10,54,54,72,73,72
        .byte 73,54,54,0,0,54,54,255,0,18,54,54,255,0,4,54,54,0,0,54,54,255,0,18,54,54,255,0,4,54,54,0,0
        .byte 54,54,255,0,18,54,54,255,0,4,54,54,0,0,54,54,255,0,18,54,54,255,0,4,54,54,0,0,54,54,255,0,18
        .byte 76,77,255,0,4,54,54,0,0,54,54,255,0,18,108,109,255,0,4,54,54,0,0,54,54,255,0,12,70,71,255,0,10
        .byte 54,54,0,0,54,54,255,0,12,72,73,255,0,10,54,54,0,0,54,54,0,24,1,2,22,0,43,51,255,0,16,54,54
        .byte 0,0,54,54,0,25,4,3,23,0,76,77,255,0,16,54,54,0,0,54,54,255,0,6,54,54,255,0,16,54,54,0,0
        .byte 54,54,255,0,6,54,54,255,0,16,54,54,0,0,54,54,70,71,70,71,70,71,54,54,255,0,10,70,71,70,71,70,71
        .byte 54,54,0,0,76,77,72,73,72,73,72,73,76,77,255,0,10,72,73,72,73,72,73,76,77,0,0,108,109,78,50,49,49
        .byte 50,49,108,109,255,0,10,78,50,49,49,50,49,108,109,31,40,33,35,37,31,255,0,24,32,34,38,36,39,32,58,59,255,0,22
        .byte 255,0,104,10,11,13,255,0,27,45,45,14,255,0,27,45,45,15,255,0,26,24,1,2,22,255,0,26,25,4,3,23,255,0,23
        .byte 53,53,255,0,6,53,53,255,0,20,53,53,0,0,10,11,13,0,53,53,255,0,24,45,45,14,255,0,27,45,45,15,255,0,53
        .byte 10,11,13,255,0,5,10,11,13,255,0,19,45,45,14,255,0,5,45,45,14,255,0,19,45,45,15,255,0,5,45,45,15
        .byte 255,0,99,31,40,33,35,37,31,255,0,24,32,34,38,36,39,32,58,59,255,0,22
        .byte 255,0,190,99,101,0,24,1,2,22,0,99,101,255,0,20,100,102,0,25,4,3,23,0,100,102,255,0,22,46,46,46,65,65,65
        .byte 255,0,24,66,66,66,67,67,67,255,0,144,46,46,46,65,65,65,255,0,24,66,66,66,67,67,67,255,0,102,31,40,33
        .byte 35,37,31,255,0,24,32,34,38,36,39,32,58,59,255,0,22
        .byte 255,0,36,46,65,255,0,14,46,65,255,0,12,66,67,255,0,14,66,67,255,0,12,78,49,255,0,14,78,49,255,0,12
        .byte 46,65,255,0,28,66,67,255,0,22,82,82,80,82,82,80,82,80,255,0,16,80,82,80,82,82,82,83,83,81,83,83,81
        .byte 83,81,255,0,16,81,83,81,83,83,83,255,0,120,82,82,80,82,82,80,82,80,255,0,6,46,65,255,0,6,80,82,80
        .byte 82,82,80,82,82,83,83,81,83,83,81,83,81,255,0,6,66,67,255,0,6,81,83,81,83,83,81,83,83,0,24,1,2
        .byte 22,0,46,65,255,0,14,46,65,255,0,7,25,4,3,23,0,66,67,255,0,14,66,67,255,0,12,78,49,255,0,14,78
        .byte 49,255,0,12,46,65,255,0,14,46,65,255,0,12,66,67,255,0,14,66,67,255,0,6,31,40,33,35,37,31,255,0,24
        .byte 32,34,38,36,39,32,58,59,255,0,22
        .byte 255,0,40,70,71,255,0,28,72,73,255,0,57,24,1,2,22,255,0,26,25,4,3,23,255,0,17,94,92,255,0,26,94
        .byte 92,76,77,255,0,26,76,77,85,84,255,53,8,49,49,50,78,70,71,78,78,70,71,49,49,70,71,49,50,78,78,85,84
        .byte 85,84,255,0,12,72,73,0,0,72,73,0,0,72,73,255,0,4,85,84,76,77,5,47,255,0,18,70,71,255,0,4,76
        .byte 77,85,84,255,0,20,72,73,255,0,4,85,84,85,84,255,0,26,85,84,76,77,49,49,50,255,49,8,50,49,49,0,0
        .byte 70,71,255,0,8,76,77,85,84,70,71,255,0,10,70,71,0,0,72,73,255,0,8,85,84,91,93,72,73,255,0,10,72
        .byte 73,49,49,50,255,49,6,50,78,78,91,93,255,0,70,70,71,255,0,18,31,40,33,35,37,31,255,0,4,72,73,255,0,18
        .byte 32,34,38,36,39,32,58,59,255,0,22
        .byte 255,0,14,75,74,255,0,24,78,78,50,78,96,62,49,49,49,50,78,78,61,60,255,0,16,80,82,80,255,55,4,80,82
        .byte 80,82,80,75,74,255,0,16,81,83,81,55,8,8,55,81,83,81,83,81,75,74,255,0,14,99,71,99,71,0,0,64,64
        .byte 255,0,6,75,74,255,0,14,72,102,72,102,0,0,7,7,255,0,6,76,77,255,0,14,99,71,255,0,4,7,7,255,0,6
        .byte 75,74,255,0,14,72,102,255,0,4,7,7,255,0,6,75,74,255,0,14,99,71,255,0,4,7,7,255,0,6,75,74,255,0,14
        .byte 72,102,255,0,4,7,7,255,0,6,75,74,255,0,14,99,71,255,0,4,7,7,0,0,99,71,0,0,76,77,255,0,14
        .byte 72,102,255,0,4,7,7,0,0,72,102,0,0,75,74,255,0,20,7,7,255,0,6,75,74,255,0,20,7,7,255,0,6
        .byte 75,74,255,0,20,7,7,255,0,6,75,74,255,0,20,7,7,255,0,6,76,77,255,0,20,7,7,0,24,1,2,22,0
        .byte 75,74,255,0,20,63,63,0,25,4,3,23,0,75,74,255,0,16,80,82,80,55,52,52,55,80,82,80,82,80,75,74,255,0,6
        .byte 31,40,33,35,37,31,255,0,4,81,83,81,255,55,4,81,83,81,83,81,96,62,255,0,6,32,34,38,36,39,32,58,59
        .byte 255,0,6,61,60,255,0,14
        .byte 255,0,31,46,65,0,46,65,255,0,8,43,51,255,0,8,46,65,0,46,65,0,0,66,67,0,66,67,255,0,8,76,77
        .byte 255,0,8,66,67,0,66,67,0,0,78,78,50,49,49,255,0,8,12,17,255,0,8,78,78,50,49,49,255,0,15,12,17
        .byte 255,0,28,12,17,255,0,23,24,1,2,22,0,76,77,255,0,23,25,4,3,23,0,108,109,255,0,20,43,71,70,71,70
        .byte 71,70,71,255,0,22,72,73,72,73,72,73,72,73,255,0,32,70,71,70,71,70,71,70,71,255,0,22,72,73,72,73,72
        .byte 73,72,109,255,0,20,43,51,255,0,28,76,77,255,0,28,12,17,255,0,28,12,17,255,0,15,78,78,50,49,49,255,0,8
        .byte 12,17,255,0,8,78,78,50,49,49,0,0,46,65,0,46,65,255,0,8,76,77,255,0,8,46,65,0,46,65,0,0,66
        .byte 67,0,66,67,255,0,8,108,109,255,0,8,66,67,0,66,67,0,31,40,33,35,37,31,255,0,24,32,34,38,36,39,32
        .byte 58,59,255,0,22
        .byte 255,0,36,46,65,255,0,16,46,65,255,0,10,66,67,255,0,16,66,67,255,0,10,46,65,255,0,16,46,65,255,0,10
        .byte 46,65,255,0,16,66,67,255,0,10,66,67,255,0,16,46,65,255,0,16,70,101,255,0,10,66,67,255,0,16,100,73,255,0,10
        .byte 46,65,255,0,22,43,51,255,0,4,66,67,255,0,22,76,77,255,0,4,43,51,255,0,17,24,1,2,22,0,54,54,255,0,4
        .byte 76,77,255,0,17,25,4,3,23,0,54,54,255,0,4,108,109,255,0,22,76,77,255,0,4,46,65,255,0,22,108,109,255,0,4
        .byte 66,67,255,0,16,99,71,255,0,10,46,65,255,0,16,72,102,255,0,10,66,67,255,0,10,46,65,255,0,16,46,65,255,0,10
        .byte 66,67,255,0,16,66,67,255,0,10,46,65,255,0,16,46,65,255,0,4,31,40,33,35,37,31,66,67,255,0,16,66,67
        .byte 255,0,4,32,34,38,36,39,32,58,59,255,0,22
        .byte 255,0,140,56,89,255,0,28,90,88,255,0,14,56,89,255,0,28,90,88,255,0,34,56,89,255,0,28,90,88,255,0,34
        .byte 56,89,255,0,4,56,89,255,0,22,90,88,255,0,4,90,88,255,0,12,56,89,255,0,28,90,88,255,0,24,56,89,0,0,0
        .byte 24,1,2,22,0,56,89,255,0,6,56,89,255,0,10,90,88,0,0,0,25,4,3,23,0,90,88,255,0,6,90,88,255,0,96
        .byte 31,40,33,35,37,31,255,0,24,32,34,38,36,39,32,58,59,255,0,22
        .byte 255,0,4,85,84,255,0,10,85,84,255,0,13,70,71,82,91,93,82,80,255,82,7,80,91,93,82,80,255,82,6,80,70
        .byte 71,0,0,72,73,70,95,95,71,87,255,79,7,70,95,95,71,87,255,79,6,87,72,73,255,0,4,72,9,9,73,255,0,8
        .byte 72,9,9,73,0,0,0,85,84,255,0,7,46,65,82,64,64,82,80,255,82,7,80,64,64,82,80,255,82,6,80,46,65
        .byte 0,0,66,67,83,7,7,83,81,255,83,7,81,7,7,83,81,255,83,6,81,66,67,255,0,5,7,7,0,0,0,24,1
        .byte 2,22,0,0,0,7,7,255,0,4,85,84,255,0,10,7,7,0,0,0,25,4,3,23,0,0,0,7,7,255,0,4,76
        .byte 77,255,0,10,7,7,255,0,10,7,7,255,0,4,85,84,255,0,10,7,7,255,0,10,7,7,255,0,4,91,93,255,0,10
        .byte 7,7,255,0,10,7,7,255,0,16,7,7,255,0,10,7,7,255,0,16,7,7,255,0,10,7,7,255,0,16,63,63,255,0,10
        .byte 63,63,255,0,13,46,65,70,8,8,71,80,255,82,6,80,70,8,8,71,80,255,82,6,80,46,65,0,0,66,67,72,95,95
        .byte 73,81,255,83,6,81,72,95,95,73,81,255,83,6,81,66,67,255,0,5,94,92,255,0,10,94,92,255,0,13,70,71,255,82,4
        .byte 80,255,82,6,80,255,82,4,80,255,82,6,80,70,71,0,0,72,73,255,79,4,87,255,79,6,87,255,79,4,87,255,79,6
        .byte 87,72,73,0,31,40,33,35,37,31,255,0,24,32,34,38,36,39,32,58,59,255,0,22
        .byte 255,0,140,70,71,255,0,28,72,73,255,0,27,24,1,2,22,0,43,51,255,0,23,25,4,3,23,0,76,77,255,0,8
        .byte 43,51,255,0,10,94,92,255,0,6,54,54,255,0,8,54,54,255,0,10,76,77,255,0,6,54,54,255,0,8,54,54,255,0,10
        .byte 85,84,255,0,6,76,77,255,0,8,108,109,255,0,10,85,84,255,0,6,108,109,255,0,20,76,77,255,0,28,91,93,255,0,162
        .byte 31,40,33,35,37,31,255,0,24,32,34,38,36,39,32,58,59,255,0,22
        .byte 255,0,252,46,65,49,49,46,65,255,0,24,66,67,49,49,66,67,255,0,80,70,71,0,0,0,24,1,2,22,0,0,0
        .byte 70,71,255,0,16,72,73,0,0,0,25,4,3,23,0,0,0,72,73,255,0,22,70,71,255,0,28,72,73,255,0,104,31
        .byte 40,33,35,37,31,255,0,24,32,34,38,36,39,32,58,59,255,0,22
        .byte 255,0,184,61,60,255,0,8,61,60,255,0,8,61,60,255,0,8,75,74,255,0,8,75,74,255,0,8,75,74,255,0,8
        .byte 76,77,255,0,8,75,74,255,0,8,76,77,255,0,8,75,74,255,0,8,76,77,255,0,8,75,74,255,0,8,75,74,0,0,0
        .byte 24,1,2,22,0,75,74,255,0,8,75,74,255,0,8,75,74,0,0,0,25,4,3,23,0,75,74,255,0,8,75,74,255,0,8
        .byte 75,74,255,0,8,76,77,255,0,8,75,74,255,0,8,76,77,255,0,8,75,74,255,0,8,76,77,255,0,8,75,74,255,0,8
        .byte 75,74,255,0,8,75,74,255,0,8,96,62,255,0,8,96,62,255,0,8,96,62,255,0,94,31,40,33,35,37,31,255,0,24
        .byte 32,34,38,36,39,32,58,59,255,0,22
        .byte 255,0,10,85,84,255,0,6,85,84,255,0,18,78,78,91,93,49,49,50,50,49,49,91,93,49,49,255,0,16,80,70,95,95
        .byte 71,80,82,82,80,70,95,95,71,80,255,0,16,87,72,9,9,73,87,79,79,87,72,9,9,73,87,255,0,18,64,64,0
        .byte 24,1,2,22,0,64,64,255,0,20,7,7,0,25,4,3,23,0,7,7,255,0,20,7,7,255,0,6,7,7,255,0,20
        .byte 7,7,255,0,6,7,7,255,0,20,7,7,255,0,6,7,7,255,0,20,7,7,255,0,6,7,7,255,0,20,7,7,255,0,6
        .byte 7,7,255,0,20,7,7,255,0,6,7,7,255,0,20,7,7,255,0,6,7,7,255,0,20,7,7,255,0,6,7,7,255,0,20
        .byte 7,7,255,0,6,7,7,255,0,20,63,63,255,0,6,63,63,255,0,18,80,70,8,8,71,80,82,82,80,70,8,8,71
        .byte 80,255,0,16,81,72,95,95,73,81,83,83,81,72,95,95,73,81,255,0,16,78,78,94,92,49,49,50,50,49,49,94,92
        .byte 49,49,255,0,8,31,40,33,35,37,31,255,0,4,85,84,255,0,6,85,84,255,0,10,32,34,38,36,39,32,58,59,0,0
        .byte 85,84,255,0,6,85,84,255,0,10
        .byte 255,0,50,78,50,49,49,50,49,255,0,24,46,65,46,65,46,65,255,0,24,66,67,66,67,66,67,255,0,4,70,101,255,0,16
        .byte 43,51,0,24,1,2,22,0,43,51,86,0,100,73,255,0,16,76,77,0,25,4,3,23,0,76,77,0,0,70,101,70,101
        .byte 255,0,14,12,17,255,0,6,12,17,0,0,100,73,100,73,255,0,14,12,17,255,0,6,12,17,0,0,70,101,70,101,255,0,14
        .byte 76,77,255,0,6,76,77,0,0,100,73,100,73,255,0,14,108,109,255,0,6,108,109,86,255,0,83,53,53,255,0,28,53,53
        .byte 255,0,86,70,71,70,71,70,71,255,0,24,72,73,72,73,72,73,255,0,24,78,50,78,78,50,49,255,0,4,31,40,33
        .byte 35,37,31,255,0,24,32,34,38,36,39,32,58,59,255,0,22
        .byte 255,0,252,104,105,255,0,28,107,106,255,0,34,104,105,255,0,28,107,106,255,0,18,104,105,0,0,0,24,1,2,22,255,0,21
        .byte 107,106,0,0,0,25,4,3,23,255,0,27,104,105,255,0,28,107,106,255,0,104,31,40,33,35,37,31,255,0,24,32,34
        .byte 38,36,39,32,58,59,255,0,22
numsc:
        .byte 51
.out .sprintf("SCREENS SIZE = %d", (* - scdat))

nmedat:
        .byte 7,4,168,232,255
        .byte 0,1,96,184,1,1,96,56,255
        .byte 0,1,48,144,1,1,80,72,255
        .byte 0,3,120,96,1,1,64,168,4,9,144,56,255
        .byte 0,3,32,216,1,1,144,216,4,9,96,144,3,2,112,232,255
        .byte 0,3,24,120,1,1,112,216,4,9,112,24,3,2,160,232,6,11,64,120,255
        .byte 0,3,32,120,1,1,160,200,6,10,64,120,6,11,96,120,6,11,144,120,255
        .byte 0,3,24,192,1,1,144,40,6,10,64,192,3,2,152,232,4,9,80,96,255
        .byte 0,3,24,56,1,1,144,168,4,9,112,32,6,11,48,120,6,11,128,120,255
        .byte 0,3,64,184,1,1,144,72,6,10,48,120,4,9,80,136,6,10,144,232,255
        .byte 0,3,24,24,1,1,144,168,4,9,168,48,3,2,168,232,6,10,80,184,255
        .byte 0,3,120,120,1,1,160,200,4,9,160,120,6,11,96,120,3,2,168,232,255
        .byte 0,3,24,208,1,1,112,120,6,10,80,56,6,11,112,184,255
        .byte 0,3,80,208,1,1,128,72,6,10,64,120,4,9,48,120,3,2,168,232,255
        .byte 0,3,24,16,1,1,160,184,6,11,64,120,6,10,96,120,3,2,168,232,255
        .byte 0,3,32,120,1,1,80,200,8,4,64,72,8,4,48,168,255
        .byte 0,3,88,88,1,1,96,216,6,10,64,168,5,4,144,120,6,11,128,168,255
        .byte 0,3,32,24,1,1,32,216,8,4,80,72,8,4,48,168,6,11,80,56,6,10,112,184,255
        .byte 0,3,56,112,1,1,112,24,5,4,112,168,5,4,128,40,4,9,88,16,255
        .byte 6,10,48,56,6,11,144,184,4,9,24,224,0,3,160,224,1,1,96,72,255
        .byte 0,3,120,48,1,1,48,200,4,9,32,160,3,2,168,232,8,4,48,120,255
        .byte 0,3,32,168,1,1,144,72,6,10,64,200,6,10,128,40,4,9,112,144,255
        .byte 0,3,24,16,1,1,112,200,6,10,112,120,3,2,160,224,4,9,152,104,255
        .byte 1,1,160,56,0,3,24,224,6,11,64,168,6,10,112,88,255
        .byte 0,3,128,56,1,1,48,200,8,4,48,120,8,4,48,104,8,4,48,136,6,10,96,216,255
        .byte 6,10,144,152,6,11,48,152,0,3,32,24,1,1,144,200,4,9,152,24,255
        .byte 0,3,136,48,1,1,144,104,6,10,96,120,4,9,96,136,3,2,168,232,255
        .byte 0,3,48,184,1,1,128,232,8,4,48,136,3,2,168,232,4,9,136,112,255
        .byte 0,3,128,56,1,1,96,120,6,10,112,88,6,10,112,152,255
        .byte 0,3,96,40,1,1,144,120,6,10,144,88,6,10,144,152,255
        .byte 0,3,48,56,1,1,128,168,6,10,80,136,6,10,32,184,6,10,128,88,255
        .byte 0,3,40,32,1,1,144,184,6,10,112,56,6,10,64,184,4,9,24,224,255
        .byte 0,3,144,48,1,1,144,184,5,4,144,120,4,9,152,56,3,2,168,232,255
        .byte 0,3,32,32,8,4,48,88,1,1,96,104,4,9,152,56,6,11,96,40,255
        .byte 0,3,48,152,1,1,144,200,4,9,144,128,255
        .byte 0,3,128,56,1,1,48,216,6,10,80,136,4,9,144,104,3,2,168,232,255
        .byte 0,3,24,32,1,1,144,200,6,10,112,88,6,10,112,152,255
        .byte 0,3,152,40,1,1,112,120,4,9,48,232,6,10,48,152,6,11,144,120,255
        .byte 0,3,40,24,1,1,96,24,4,9,16,168,6,10,96,120,3,2,168,232,255
        .byte 0,0,88,208,1,1,128,104,6,11,104,24,6,10,64,88,6,10,48,232,255
        .byte 0,3,72,168,1,1,80,216,4,9,112,168,3,2,168,232,5,4,144,120,255
        .byte 0,3,104,88,6,10,80,184,1,1,64,152,6,10,128,40,3,2,168,232,255
        .byte 0,3,32,176,1,1,112,232,6,10,128,56,6,11,80,104,6,10,64,152,255
        .byte 0,3,80,56,1,1,32,136,6,11,144,120,7,4,112,72,4,9,80,152,255
        .byte 0,3,64,168,1,1,64,24,8,4,48,40,8,4,48,136,4,9,64,200,255
        .byte 0,3,80,168,1,1,80,88,4,9,64,104,6,10,64,136,6,11,128,136,255
        .byte 0,3,48,200,1,1,48,40,6,10,64,120,6,11,96,120,4,9,80,192,255
        .byte 0,3,96,64,1,1,96,168,6,10,48,120,4,9,128,216,6,11,144,120,255
        .byte 0,3,56,208,1,1,144,40,6,10,128,120,8,4,48,88,8,4,48,152,255
        .byte 0,3,64,56,1,1,128,88,4,4,40,136,6,10,128,184,6,10,48,24,255
        .byte 0,3,136,168,1,1,48,56,4,4,32,88,7,4,80,104,255
.out .sprintf("POSITIONS SIZE = %d", (* - nmedat))

NUMOBJ = 1
objdta:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,254,56,120,254,56,120
.out .sprintf("OBJECTS SIZE = %d", (* - objdta))

font:
        .byte 0,50,38,125,201,201,201,0
        .byte 0,0,8,8,8,0,8,0
        .byte 0,0,40,40,0,0,0,0
        .byte 0,0,36,126,36,126,36,0
        .byte 0,48,88,11,232,13,6,0
        .byte 0,0,38,12,24,48,100,0
        .byte 0,0,16,40,26,36,58,0
        .byte 0,0,8,16,0,0,0,0
        .byte 0,0,16,32,32,32,16,0
        .byte 0,0,8,4,4,4,8,0
        .byte 20,18,6,60,96,72,40,8
        .byte 0,0,16,16,124,16,16,0
        .byte 0,0,0,0,0,4,8,0
        .byte 0,0,0,0,124,0,0,0
        .byte 0,0,0,0,0,12,12,0
        .byte 0,0,6,12,24,48,96,0
        .byte 0,0,60,70,90,98,60,0
        .byte 0,0,24,56,24,24,60,0
        .byte 0,0,62,2,62,32,62,0
        .byte 0,0,62,2,30,2,62,0
        .byte 0,0,34,34,62,2,2,0
        .byte 0,0,62,32,62,2,62,0
        .byte 0,0,126,64,126,66,126,0
        .byte 0,0,126,2,2,2,2,0
        .byte 0,0,126,66,126,66,126,0
        .byte 0,0,126,66,126,2,2,0
        .byte 0,0,0,24,0,0,24,0
        .byte 0,0,0,8,0,8,16,0
        .byte 0,0,8,16,32,16,8,0
        .byte 0,0,0,60,0,60,0,0
        .byte 0,0,16,8,4,8,16,0
        .byte 0,0,60,66,12,16,16,0
        .byte 0,0,126,90,94,64,126,0
        .byte 0,0,60,66,66,126,66,0
        .byte 0,0,124,66,124,66,124,0
        .byte 0,0,126,64,64,64,126,0
        .byte 0,0,124,66,66,66,124,0
        .byte 0,0,126,64,124,64,126,0
        .byte 0,0,126,64,124,64,64,0
        .byte 0,0,126,64,78,66,126,0
        .byte 0,0,66,66,126,66,66,0
        .byte 0,0,124,16,16,16,124,0
        .byte 0,0,60,8,8,72,48,0
        .byte 0,0,36,40,48,40,36,0
        .byte 0,0,64,64,64,64,124,0
        .byte 0,0,126,90,66,66,66,0
        .byte 0,0,126,66,66,66,66,0
        .byte 0,0,126,66,66,66,126,0
        .byte 0,0,126,66,126,64,64,0
        .byte 0,0,126,66,66,74,126,0
        .byte 0,0,126,66,126,72,70,0
        .byte 0,0,126,64,126,2,126,0
        .byte 0,0,62,8,8,8,8,0
        .byte 0,0,66,66,66,66,126,0
        .byte 0,0,66,66,66,36,24,0
        .byte 0,0,66,66,66,90,60,0
        .byte 0,0,34,20,8,20,34,0
        .byte 0,0,34,34,30,2,60,0
        .byte 0,0,126,2,60,64,126,0
        .byte 0,0,24,16,16,16,24,0
        .byte 0,0,96,48,24,12,6,0
        .byte 0,0,24,8,8,8,24,0
        .byte 0,0,16,56,84,16,16,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,12,18,24,16,62,0
        .byte 0,0,126,2,126,66,126,0
        .byte 0,0,64,64,126,66,126,0
        .byte 0,0,62,64,64,64,62,0
        .byte 0,0,2,2,126,66,126,0
        .byte 0,0,126,66,126,64,126,0
        .byte 0,0,62,64,124,64,64,0
        .byte 0,0,126,66,126,2,126,0
        .byte 0,0,64,64,126,66,66,0
        .byte 0,0,24,8,8,8,28,0
        .byte 0,0,14,2,2,34,28,0
        .byte 0,0,68,88,112,72,68,0
        .byte 0,0,64,64,64,64,60,0
        .byte 0,0,60,90,66,66,66,0
        .byte 0,0,60,66,66,66,66,0
        .byte 0,0,60,66,66,66,60,0
        .byte 0,0,60,66,124,64,64,0
        .byte 0,0,60,66,66,74,60,0
        .byte 0,0,60,66,64,64,64,0
        .byte 0,0,62,64,60,2,124,0
        .byte 0,0,16,56,16,20,24,0
        .byte 0,0,66,66,66,66,60,0
        .byte 0,0,34,34,34,20,8,0
        .byte 0,0,66,66,66,90,60,0
        .byte 0,0,34,20,8,20,34,0
        .byte 0,0,36,20,12,36,24,0
        .byte 0,0,124,2,60,64,62,0
        .byte 0,0,24,16,48,16,24,0
        .byte 0,0,24,24,24,24,24,0
        .byte 0,0,24,8,12,8,24,0
        .byte 0,0,20,40,0,0,0,0
        .byte 0,0,126,66,94,66,126,0
.out .sprintf("FONT SIZE = %d", (* - font))

jtab:
        .byte 248,250,252,254,254,255,255,255,0,0,0,1,1,1,2,2,4,6,8,8,8,99
keys:   .byte 66,97,104,72,73,16,55,48,49,17,18
.out .sprintf("KEYS SIZE = %d", (* - keys))

data_end:
.out .sprintf("TOTAL DATA SIZE = %d", (* - data_start))
