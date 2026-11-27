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
WINDOWHGT = 18
WINDOWWID = 30 ;面面面面面面面面面面面面
MAPWID = 13
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255
mapdat:
        .byte 255,1,2,3,4,5,6,7,8,9,10,11,255,255,12,13,14,15,16,17,18,19,20,21,22,255,255,23,24,25,255,255,255,255,255,255,255,255,255
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255
stmap:  .byte 1

evnt00:
        jsr skobj
        sta varobj
        lda #6
        cmp varobj
        bcs *+5
        jmp a00054
        lda varobj
        jsr getob
        lda #50
        asl a
        sta sndtyp
        jmp a00190
a00054: lda #20
        cmp varobj
        beq *+5
        jmp a00129
        lda varobj
        jsr getob
        lda #50
        asl a
        sta sndtyp
        lda #<100
        sta z80_l
        lda #>100
        sta z80_h
        jsr addsc
        lda #10
        sta varc
        jmp a00190
a00129: lda #255
        cmp varobj
        bne *+5
        jmp a00190
        lda varobj
        jsr getob
        lda #50
        asl a
        sta sndtyp
        lda #<10
        sta z80_l
        lda #>10
        sta z80_h
        jsr addsc
a00190: lda #22
        sta chary
        lda #15
        sta charx
        jsr dscor
        lda #0
        sta varf
        lda joyval
        and #8
        beq :+
        jmp a00320
:
        lda #1
        sta varf
        jsr laddu
        beq :+
        jmp a00320
:
        lda #5
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp varb
        beq *+5
        jmp a00320
        ; lda #0
        jsr animsp
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a00320: lda joyval
        and #4
        beq :+
        jmp a00402
:
        lda #1
        sta varf
        jsr laddd
        beq :+
        jmp a00402
:
        lda #5
        ldy #6
        sta (z80_ix),y
        lda #0
        jsr animsp
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a00402: lda joyval
        and #1
        beq :+
        jmp a00669
:
        lda #1
        sta varf
        lda #230
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp a00493
        lda #45
        asl a
        sta sndtyp
        lda #50
        jsr delay
        jsr scrr
        lda #8
        ldy #9
        sta (z80_ix),y
        jmp a00669
a00493: jsr cangr
        beq :+
        jmp a00669
:
        lda joyval
        and #8
        beq :+
        jmp a00608
:
        jsr laddu
        beq :+
        jmp a00543
:
        jmp a00604
a00543: lda #1
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp varb
        beq *+5
        jmp a00582
        ; lda #0
        jsr animsp
a00582: ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a00604: jmp a00669
a00608: lda #1
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp varb
        beq *+5
        jmp a00647
        ; lda #0
        jsr animsp
a00647: ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a00669: lda joyval
        and #2
        beq :+
        jmp a00872
:
        lda #1
        sta varf
        jsr cangl
        beq :+
        jmp a00872
:
        lda joyval
        and #8
        beq :+
        jmp a00810
:
        jsr laddu
        beq :+
        jmp a00744
:
        jmp a00806
a00744: lda #0
        ldy #6
        sta (z80_ix),y
        ; lda #0
        cmp varb
        beq *+5
        jmp a00784
        ; lda #0
        jsr animsp
a00784: ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a00806: jmp a00872
a00810: lda #0
        ldy #6
        sta (z80_ix),y
        ; lda #0
        cmp varb
        beq *+5
        jmp a00850
        ; lda #0
        jsr animsp
a00850: ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a00872: lda #0
        cmp varf
        beq *+5
        jmp a00902
        ; lda #0
        ldy #7
        sta (z80_ix),y
a00902: lda joyval
        and #16
        beq :+
        jmp a01015
:
        lda #19
        sta chary
        lda #13
        sta charx
        lda #18
        jsr dmsg
        jsr prskey
        lda #19
        sta chary
        lda #13
        sta charx
        lda #6
        sta loopa
a00975: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        dec loopa
        beq :+
        jmp a00975
:
a01015: jsr cangd
        beq :+
        jmp a01051
:
        jsr laddd
        beq :+
        jmp a01047
:
        jmp a01051
a01047: jsr ifall
a01051: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01196
:
        lda #9
        cmp scno
        bcs *+5
        jmp a01154
        lda #78
        asl a
        sta sndtyp
        lda #19
        sta chary
        lda #12
        sta charx
        lda #32
        jsr dmsg
        lda #50
        jsr delay
        lda #1
        sta deadf
        jmp a01171
a01154: lda #45
        asl a
        sta sndtyp
        jsr jump
a01171: lda #17
        cmp scno
        beq *+5
        jmp a01196
        lda #1
        sta deadf
a01196: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01336
:
        lda #0
        cmp vard
        beq *+5
        jmp a01289
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
        jmp a01336
a01289: lda #23
        cmp scno
        beq *+5
        jmp a01336
        lda #45
        asl a
        sta sndtyp
        lda #50
        jsr delay
        lda #1
        sta nexlev
a01336: jmp grav
.out .sprintf("EVENT 0 SIZE = %d", (* - evnt00))

evnt01:
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00161
        jsr laddu
        beq :+
        jmp b00081
:
        lda #0
        cmp varb
        beq *+5
        jmp b00076
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
b00076: jmp b00161
b00081: lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp b00147
        lda #2
        ldy #10
        sta (z80_ix),y
        jmp b00161
b00147: lda #3
        ldy #10
        sta (z80_ix),y
b00161: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00320
        jsr laddd
        beq :+
        jmp b00240
:
        lda #0
        cmp varb
        beq *+5
        jmp b00235
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
b00235: jmp b00320
b00240: lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp b00307
        lda #2
        ldy #10
        sta (z80_ix),y
        jmp b00320
b00307: lda #3
        ldy #10
        sta (z80_ix),y
b00320: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00518
        jsr cangl
        beq :+
        jmp b00397
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        tya; lda #9
        ldy #6
        sta (z80_ix),y
        jmp b00411
b00397: lda #3
        ldy #10
        sta (z80_ix),y
b00411: lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp b00518
        jsr laddu
        beq :+
        jmp b00491
:
        lda #0
        ldy #10
        sta (z80_ix),y
        jmp b00518
b00491: jsr laddd
        beq :+
        jmp b00518
:
        lda #1
        ldy #10
        sta (z80_ix),y
b00518: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00714
        jsr cangr
        beq :+
        jmp b00594
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #10
        ldy #6
        sta (z80_ix),y
        jmp b00607
b00594: lda #2
        ldy #10
        sta (z80_ix),y
b00607: lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp b00714
        jsr laddu
        beq :+
        jmp b00687
:
        lda #0
        ldy #10
        sta (z80_ix),y
        jmp b00714
b00687: jsr laddd
        beq :+
        jmp b00714
:
        lda #1
        ldy #10
        sta (z80_ix),y
b00714: lda #0
        cmp varb
        beq *+5
        jmp b00740
        ; lda #0
        jsr animsp
b00740: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00816
:
        lda #78
        asl a
        sta sndtyp
        lda #19
        sta chary
        lda #6
        sta charx
        lda #24
        jsr dmsg
        lda #50
        jsr delay
        lda #1
        sta deadf
b00816: jsr cangd
        beq :+
        jmp b00884
:
        lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00870
        lda #3
        ; ldy #10
        sta (z80_ix),y
        jmp b00884
b00870: lda #2
        ldy #10
        sta (z80_ix),y
b00884: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp b00924
:
        lda #2
        ldy #10
        sta (z80_ix),y
b00924: rts
.out .sprintf("EVENT 1 SIZE = %d", (* - evnt01))

evnt02:
        lda #1
        cmp scno
        beq *+5
        jmp c00035
        lda #32
        sta vark
        lda #204
        sta varj
c00035: lda #3
        cmp scno
        beq *+5
        jmp c00069
        lda #128
        sta vark
        lda #24
        sta varj
c00069: lda #5
        cmp scno
        beq *+5
        jmp c00102
        lda #56
        sta vark
        lda #32
        sta varj
c00102: lda #6
        cmp scno
        beq *+5
        jmp c00135
        lda #48
        sta vark
        lda #104
        sta varj
c00135: lda #7
        cmp scno
        beq *+5
        jmp c00169
        lda #16
        sta vark
        lda #32
        sta varj
c00169: lda #8
        cmp scno
        beq *+5
        jmp c00202
        lda #32
        sta vark
        ; lda #32
        sta varj
c00202: lda #10
        cmp scno
        beq *+5
        jmp c00236
        lda #128
        sta vark
        lda #24
        sta varj
c00236: lda #12
        cmp scno
        beq *+5
        jmp c00270
        lda #48
        sta vark
        lda #120
        sta varj
c00270: lda #13
        cmp scno
        beq *+5
        jmp c00303
        lda #32
        sta vark
        lda #60
        sta varj
c00303: lda #15
        cmp scno
        beq *+5
        jmp c00337
        lda #16
        sta vark
        ; lda #16
        sta varj
c00337: lda #16
        cmp scno
        beq *+5
        jmp c00371
        lda #128
        sta vark
        lda #32
        sta varj
c00371: lda #17
        cmp scno
        beq *+5
        jmp c00405
        lda #16
        sta vark
        ; lda #16
        sta varj
c00405: lda #18
        cmp scno
        beq *+5
        jmp c00439
        lda #128
        sta vark
        lda #32
        sta varj
c00439: lda #19
        cmp scno
        beq *+5
        jmp c00472
        lda #80
        sta vark
        lda #16
        sta varj
c00472: lda #22
        cmp scno
        beq *+5
        jmp c00506
        lda #84
        sta vark
        lda #24
        sta varj
c00506: lda #23
        cmp scno
        beq *+5
        jmp c00540
        lda #104
        sta vark
        lda #16
        sta varj
c00540: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp c00637
        lda #66
        iny; ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c00605
        lda #1
        dey; ldy #11
        sta (z80_ix),y
        jmp c00632
c00605: ldy #12
        lda (z80_ix),y
        sec
        sbc #1
        ; ldy #12
        sta (z80_ix),y
c00632: jmp c00705
c00637: lda #70
        ldy #12
        cmp (z80_ix),y
        beq *+5
        jmp c00679
        lda #0
        dey; ldy #11
        sta (z80_ix),y
        jmp c00705
c00679: ldy #12
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #12
        sta (z80_ix),y
c00705: lda #0
        cmp varg
        beq *+5
        jmp c00731
        ; lda #0
        jsr animsp
c00731: lda #3
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00794
:
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #90
        asl a
        sta sndtyp
        lda vara
        sec
        sbc #1
        sta vara
c00794: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00894
:
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #27
        asl a
        sta sndtyp
        lda #<20
        sta z80_l
        lda #>20
        sta z80_h
        jsr addsc
        lda varc
        clc
        adc #1
        sta varc
        lda vara
        sec
        sbc #1
        sta vara
c00894: lda #0
        cmp vara
        beq *+5
        jmp c00961
        lda vark
        ldy #8
        sta (z80_ix),y
        lda varj
        iny; ldy #9
        sta (z80_ix),y
        lda #7
        sta z80_c
        lda #11
        sta z80_b
        jsr spawn
c00961: rts
.out .sprintf("EVENT 2 SIZE = %d", (* - evnt02))

evnt03:
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp d00161
        jsr laddu
        beq :+
        jmp d00081
:
        lda #0
        cmp varg
        beq *+5
        jmp d00076
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
d00076: jmp d00161
d00081: lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp d00147
        lda #2
        ldy #10
        sta (z80_ix),y
        jmp d00161
d00147: lda #3
        ldy #10
        sta (z80_ix),y
d00161: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp d00320
        jsr laddd
        beq :+
        jmp d00240
:
        lda #0
        cmp varg
        beq *+5
        jmp d00235
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
d00235: jmp d00320
d00240: lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp d00307
        lda #2
        ldy #10
        sta (z80_ix),y
        jmp d00320
d00307: lda #3
        ldy #10
        sta (z80_ix),y
d00320: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp d00533
        jsr cangl
        beq :+
        jmp d00413
:
        lda #0
        cmp varg
        beq *+5
        jmp d00395
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
d00395: lda #2
        ldy #6
        sta (z80_ix),y
        jmp d00426
d00413: lda #3
        ldy #10
        sta (z80_ix),y
d00426: lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp d00533
        jsr laddu
        beq :+
        jmp d00506
:
        lda #0
        ldy #10
        sta (z80_ix),y
        jmp d00533
d00506: jsr laddd
        beq :+
        jmp d00533
:
        lda #1
        ldy #10
        sta (z80_ix),y
d00533: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp d00746
        jsr cangr
        beq :+
        jmp d00626
:
        lda #0
        cmp varg
        beq *+5
        jmp d00608
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
d00608: lda #3
        ldy #6
        sta (z80_ix),y
        jmp d00639
d00626: lda #2
        ldy #10
        sta (z80_ix),y
d00639: lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp d00746
        jsr laddu
        beq :+
        jmp d00719
:
        lda #0
        ldy #10
        sta (z80_ix),y
        jmp d00746
d00719: jsr laddd
        beq :+
        jmp d00746
:
        lda #1
        ldy #10
        sta (z80_ix),y
d00746: lda #0
        cmp varg
        beq *+5
        jmp d00772
        ; lda #0
        jsr animsp
d00772: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00856
:
        lda #78
        asl a
        sta sndtyp
        lda #0
        sta varc
        lda #19
        sta chary
        lda #5
        sta charx
        lda #19
        jsr dmsg
        lda #50
        jsr delay
        lda #1
        sta deadf
d00856: lda #8
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00903
:
        lda #78
        asl a
        sta sndtyp
        lda #255
        ldy #5
        sta (z80_ix),y
d00903: jsr cangd
        beq :+
        jmp d00971
:
        lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp d00958
        lda #3
        ; ldy #10
        sta (z80_ix),y
        jmp d00971
d00958: lda #2
        ldy #10
        sta (z80_ix),y
d00971: rts
.out .sprintf("EVENT 3 SIZE = %d", (* - evnt03))

evnt04:
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e00161
        jsr laddu
        beq :+
        jmp e00081
:
        lda #0
        cmp varb
        beq *+5
        jmp e00076
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
e00076: jmp e00161
e00081: lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp e00147
        lda #2
        ldy #10
        sta (z80_ix),y
        jmp e00161
e00147: lda #3
        ldy #10
        sta (z80_ix),y
e00161: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e00320
        jsr laddd
        beq :+
        jmp e00240
:
        lda #0
        cmp varb
        beq *+5
        jmp e00235
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
e00235: jmp e00320
e00240: lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp e00307
        lda #2
        ldy #10
        sta (z80_ix),y
        jmp e00320
e00307: lda #3
        ldy #10
        sta (z80_ix),y
e00320: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e00516
        jsr cangl
        beq :+
        jmp e00396
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #6
        ldy #6
        sta (z80_ix),y
        jmp e00409
e00396: lda #3
        ldy #10
        sta (z80_ix),y
e00409: lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp e00516
        jsr laddu
        beq :+
        jmp e00489
:
        lda #0
        ldy #10
        sta (z80_ix),y
        jmp e00516
e00489: jsr laddd
        beq :+
        jmp e00516
:
        lda #1
        ldy #10
        sta (z80_ix),y
e00516: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e00713
        jsr cangr
        beq :+
        jmp e00592
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #7
        ldy #6
        sta (z80_ix),y
        jmp e00606
e00592: lda #2
        ldy #10
        sta (z80_ix),y
e00606: lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp e00713
        jsr laddu
        beq :+
        jmp e00686
:
        lda #0
        ldy #10
        sta (z80_ix),y
        jmp e00713
e00686: jsr laddd
        beq :+
        jmp e00713
:
        lda #1
        ldy #10
        sta (z80_ix),y
e00713: lda #0
        cmp varg
        beq *+5
        jmp e00738
        ; lda #0
        jsr animsp
e00738: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00860
:
        lda #1
        cmp varc
        bcs *+5
        jmp e00790
        lda #0
        sta varc
        jmp e00855
e00790: lda #19
        sta chary
        lda #10
        sta charx
        lda #26
        jsr dmsg
        lda #1
        jsr delay
        lda #0
        cmp varb
        beq *+5
        jmp e00855
        lda varc
        sec
        sbc #1
        sta varc
e00855: jmp e00926
e00860: lda #19
        sta chary
        lda #10
        sta charx
        ; lda #10
        sta loopa
e00886: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        dec loopa
        beq :+
        jmp e00886
:
e00926: jsr cangd
        beq :+
        jmp e00994
:
        lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e00980
        lda #3
        ; ldy #10
        sta (z80_ix),y
        jmp e00994
e00980: lda #2
        ldy #10
        sta (z80_ix),y
e00994: rts
.out .sprintf("EVENT 4 SIZE = %d", (* - evnt04))

evnt05:
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00161
        jsr laddu
        beq :+
        jmp f00081
:
        lda #0
        cmp varb
        beq *+5
        jmp f00076
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
f00076: jmp f00161
f00081: lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp f00147
        lda #2
        ldy #10
        sta (z80_ix),y
        jmp f00161
f00147: lda #3
        ldy #10
        sta (z80_ix),y
f00161: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00320
        jsr laddd
        beq :+
        jmp f00240
:
        lda #0
        cmp varb
        beq *+5
        jmp f00235
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
f00235: jmp f00320
f00240: lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp f00307
        lda #2
        ldy #10
        sta (z80_ix),y
        jmp f00320
f00307: lda #3
        ldy #10
        sta (z80_ix),y
f00320: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00517
        jsr cangl
        beq :+
        jmp f00396
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #12
        ldy #6
        sta (z80_ix),y
        jmp f00410
f00396: lda #3
        ldy #10
        sta (z80_ix),y
f00410: lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp f00517
        jsr laddu
        beq :+
        jmp f00490
:
        lda #0
        ldy #10
        sta (z80_ix),y
        jmp f00517
f00490: jsr laddd
        beq :+
        jmp f00517
:
        lda #1
        ldy #10
        sta (z80_ix),y
f00517: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00713
        jsr cangr
        beq :+
        jmp f00593
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #13
        ldy #6
        sta (z80_ix),y
        jmp f00606
f00593: lda #2
        ldy #10
        sta (z80_ix),y
f00606: lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp f00713
        jsr laddu
        beq :+
        jmp f00686
:
        lda #0
        ldy #10
        sta (z80_ix),y
        jmp f00713
f00686: jsr laddd
        beq :+
        jmp f00713
:
        lda #1
        ldy #10
        sta (z80_ix),y
f00713: lda #0
        cmp varb
        beq *+5
        jmp f00739
        ; lda #0
        jsr animsp
f00739: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00815
:
        lda #78
        asl a
        sta sndtyp
        lda #19
        sta chary
        lda #6
        sta charx
        lda #24
        jsr dmsg
        lda #50
        jsr delay
        lda #1
        sta deadf
f00815: jsr cangd
        beq :+
        jmp f00883
:
        lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00869
        lda #3
        ; ldy #10
        sta (z80_ix),y
        jmp f00883
f00869: lda #2
        ldy #10
        sta (z80_ix),y
f00883: rts
.out .sprintf("EVENT 5 SIZE = %d", (* - evnt05))

evnt06:
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp g00161
        jsr laddu
        beq :+
        jmp g00081
:
        lda #0
        cmp varb
        beq *+5
        jmp g00076
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
g00076: jmp g00161
g00081: lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp g00147
        lda #2
        ldy #10
        sta (z80_ix),y
        jmp g00161
g00147: lda #3
        ldy #10
        sta (z80_ix),y
g00161: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp g00320
        jsr laddd
        beq :+
        jmp g00240
:
        lda #0
        cmp varb
        beq *+5
        jmp g00235
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
g00235: jmp g00320
g00240: lda #2
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp g00307
        lda #2
        ldy #10
        sta (z80_ix),y
        jmp g00320
g00307: lda #3
        ldy #10
        sta (z80_ix),y
g00320: lda #2
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp g00517
        jsr cangl
        beq :+
        jmp g00396
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #15
        ldy #6
        sta (z80_ix),y
        jmp g00410
g00396: lda #3
        ldy #10
        sta (z80_ix),y
g00410: lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp g00517
        jsr laddu
        beq :+
        jmp g00490
:
        lda #0
        ldy #10
        sta (z80_ix),y
        jmp g00517
g00490: jsr laddd
        beq :+
        jmp g00517
:
        lda #1
        ldy #10
        sta (z80_ix),y
g00517: lda #3
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp g00713
        jsr cangr
        beq :+
        jmp g00593
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #14
        ldy #6
        sta (z80_ix),y
        jmp g00606
g00593: lda #2
        ldy #10
        sta (z80_ix),y
g00606: lda #4
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        cmp varrnd
        beq *+5
        jmp g00713
        jsr laddu
        beq :+
        jmp g00686
:
        lda #0
        ldy #10
        sta (z80_ix),y
        jmp g00713
g00686: jsr laddd
        beq :+
        jmp g00713
:
        lda #1
        ldy #10
        sta (z80_ix),y
g00713: lda #0
        cmp varg
        beq *+5
        jmp g00739
        ; lda #0
        jsr animsp
g00739: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00943
:
        lda #0
        cmp varb
        beq *+5
        jmp g00872
        lda vare
        clc
        adc #1
        sta vare
        lda #30
        cmp vare
        beq *+5
        jmp g00872
        lda #78
        asl a
        sta sndtyp
        lda #19
        sta chary
        lda #11
        sta charx
        lda #25
        jsr dmsg
        lda #50
        jsr delay
        lda #1
        sta deadf
        lda #0
        sta vare
g00872: lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #2
        asl a
        sta sndtyp
g00943: jsr cangd
        beq :+
        jmp g00961
:
        jsr ifall
g00961: jmp grav
.out .sprintf("EVENT 6 SIZE = %d", (* - evnt06))

evnt07:
        lda #23
        cmp scno
        beq *+5
        jmp h00032
        lda #1
        ldy #7
        sta (z80_ix),y
h00032: lda #70
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp h00141
:
        lda #50
        asl a
        sta sndtyp
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #<10
        sta z80_l
        lda #>10
        sta z80_h
        jsr addsc
        lda #1
        sta vard
h00141: rts
.out .sprintf("EVENT 7 SIZE = %d", (* - evnt07))

evnt08:
        lda #3
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i00024
i00024: lda #7
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i00046
i00046: lda #8
        ldy #6
        cmp (z80_ix),y
        bcc *+5
        jmp i00068
i00068: lda #16
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i00116
        lda #0
        cmp varb
        beq *+5
        jmp i00116
        ; lda #0
        jsr animsp
i00116: lda #0
        cmp varb
        beq *+5
        jmp i00141
        ; lda #0
        jsr animsp
i00141: lda #8
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i00545
        lda #9
        cmp scno
        beq *+5
        jmp i00321
        lda #1
        jsr gotob
        cmp #255
        beq :+
        jmp i00321
:
        jsr cangd
        beq :+
        jmp i00242
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp i00321
i00242: lda #255
        ldy #5
        sta (z80_ix),y
        lda #17
        sta chary
        lda #28
        sta charx
        lda #2
        sta loopa
i00281: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #16
        jsr pattr
        dec loopa
        beq :+
        jmp i00281
:
i00321: lda #13
        cmp scno
        beq *+5
        jmp i00545
        ; lda #13
        jsr gotob
        cmp #255
        beq :+
        jmp i00545
:
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i00436
:
        lda #78
        asl a
        sta sndtyp
        lda #19
        sta chary
        lda #12
        sta charx
        lda #32
        jsr dmsg
        lda #50
        jsr delay
        lda #1
        sta deadf
i00436: lda #20
        cmp varh
        beq *+5
        jmp i00476
        jsr cangd
        beq :+
        jmp i00471
:
        jsr ifall
i00471: jmp i00545
i00476: lda #40
        cmp varh
        bcc *+5
        jmp i00545
        jsr cangu
        beq :+
        jmp i00545
:
        lda #1
        cmp varb
        beq *+5
        jmp i00545
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
i00545: jmp grav
.out .sprintf("EVENT 8 SIZE = %d", (* - evnt08))

evnt09:
        lda #2
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00067
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
j00067: rts
.out .sprintf("EVENT 9 SIZE = %d", (* - evnt09))

evnt10:
        lda #0
        cmp scno
        beq *+5
        jmp k00219
        lda #4
        sta chary
        lda #5
        sta charx
        lda #10
        jsr dmsg
        lda #4
        sta chary
        lda #17
        sta charx
        lda #13
        jsr dmsg
        lda #10
        sta chary
        lda #5
        sta charx
        lda #16
        jsr dmsg
        lda #10
        sta chary
        lda #17
        sta charx
        lda #14
        jsr dmsg
        lda #16
        sta chary
        lda #5
        sta charx
        lda #17
        jsr dmsg
        lda #16
        sta chary
        lda #17
        sta charx
        lda #15
        jsr dmsg
        lda #7
        sta chary
        lda #11
        sta charx
        lda #12
        jsr dmsg
        lda #13
        sta chary
        lda #11
        sta charx
        ; lda #11
        jsr dmsg
k00219: lda #24
        cmp scno
        beq *+5
        jmp k00543
        lda #4
        sta chary
        lda #8
        sta charx
        lda #1
        sta prtmod
        lda #29
        jsr dmsg
        lda #0
        sta prtmod
        lda #8
        sta chary
        lda #4
        sta charx
        lda #1
        sta prtmod
        lda #30
        jsr dmsg
        lda #0
        sta prtmod
        lda #10
        sta chary
        ; lda #10
        sta charx
        lda #1
        sta prtmod
        lda #31
        jsr dmsg
        lda #0
        sta prtmod
        lda #14
        sta chary
        lda #15
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #61
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #62
        jsr pattr
        lda #15
        sta chary
        ; lda #15
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #63
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #64
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #89
        jsr pattr
        jsr prskey
        lda #1
        sta gamwon
k00543: lda #1
        cmp varb
        beq *+5
        jmp k00572
        lda #0
        sta varb
        jmp k00588
k00572: lda varb
        clc
        adc #1
        sta varb
k00588: lda #2
        cmp varg
        beq *+5
        jmp k00617
        lda #0
        sta varg
        jmp k00632
k00617: lda varg
        clc
        adc #1
        sta varg
k00632: lda #60
        cmp varh
        beq *+5
        jmp k00662
        lda #0
        sta varh
        jmp k00677
k00662: lda varh
        clc
        adc #1
        sta varh
k00677: lda #1
        cmp vard
        beq *+5
        jmp k00742
        lda #21
        sta chary
        lda #1
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #30
        jsr pattr
        jmp k00785
k00742: lda #21
        sta chary
        lda #1
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
k00785: lda #22
        sta chary
        lda #28
        sta charx
        lda #99
        cmp varc
        bcc *+5
        jmp k00833
        lda varc
        jsr disply
        jmp k00923
k00833: lda #9
        cmp varc
        bcc *+5
        jmp k00889
        lda #21
        jsr dmsg
        lda #22
        sta chary
        lda #29
        sta charx
        lda varc
        jsr disply
        jmp k00923
k00889: lda #22
        jsr dmsg
        lda #22
        sta chary
        lda #30
        sta charx
        lda varc
        jsr disply
k00923: lda #21
        sta chary
        lda #19
        sta charx
        lda #9
        cmp vare
        bcc *+5
        jmp k00970
        lda vare
        jsr disply
        jmp k01005
k00970: lda #21
        jsr dmsg
        lda #21
        sta chary
        lda #20
        sta charx
        lda vare
        jsr disply
k01005: rts
.out .sprintf("EVENT 10 SIZE = %d", (* - evnt10))

evnt11:
        lda #13
        cmp scno
        beq *+5
        jmp l00281
        lda #2
        sta chary
        lda #5
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #87
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #87
        jsr pattr
        lda #24
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #88
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #88
        jsr pattr
        lda #3
        sta chary
        lda #5
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #87
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #87
        jsr pattr
        lda #24
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #88
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #88
        jsr pattr
l00281: lda #7
        jsr gotob
        cmp #255
        beq :+
        jmp l00346
:
        lda #20
        sta chary
        lda #13
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #29
        jsr pattr
l00346: lda #8
        jsr gotob
        cmp #255
        beq :+
        jmp l00411
:
        lda #20
        sta chary
        lda #27
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #18
        jsr pattr
l00411: lda #9
        jsr gotob
        cmp #255
        beq :+
        jmp l00476
:
        lda #20
        sta chary
        lda #29
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #19
        jsr pattr
l00476: lda #10
        jsr gotob
        cmp #255
        beq :+
        jmp l00541
:
        lda #20
        sta chary
        lda #21
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #20
        jsr pattr
l00541: lda #11
        jsr gotob
        cmp #255
        beq :+
        jmp l00606
:
        lda #20
        sta chary
        lda #15
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #21
        jsr pattr
l00606: lda #12
        jsr gotob
        cmp #255
        beq :+
        jmp l00671
:
        lda #20
        sta chary
        lda #17
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #22
        jsr pattr
l00671: lda #13
        jsr gotob
        cmp #255
        beq :+
        jmp l00736
:
        lda #20
        sta chary
        lda #9
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #23
        jsr pattr
l00736: lda #14
        jsr gotob
        cmp #255
        beq :+
        jmp l00802
:
        lda #20
        sta chary
        lda #25
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #24
        jsr pattr
l00802: lda #15
        jsr gotob
        cmp #255
        beq :+
        jmp l00867
:
        lda #20
        sta chary
        lda #7
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #25
        jsr pattr
l00867: lda #16
        jsr gotob
        cmp #255
        beq :+
        jmp l00932
:
        lda #20
        sta chary
        lda #11
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #26
        jsr pattr
l00932: lda #19
        jsr gotob
        cmp #255
        beq :+
        jmp l00997
:
        lda #20
        sta chary
        lda #19
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #17
        jsr pattr
l00997: lda #18
        jsr gotob
        cmp #255
        beq :+
        jmp l01062
:
        lda #20
        sta chary
        lda #23
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #28
        jsr pattr
l01062: lda #25
        cmp scno
        beq *+5
        jmp l01386
        lda #4
        sta chary
        lda #11
        sta charx
        lda #1
        sta prtmod
        lda #0
        jsr dmsg
        lda #0
        sta prtmod
        lda #8
        sta chary
        lda #10
        sta charx
        lda #1
        sta prtmod
        lda #33
        jsr dmsg
        lda #0
        sta prtmod
        lda #10
        sta chary
        lda #11
        sta charx
        lda #1
        sta prtmod
        lda #34
        jsr dmsg
        lda #0
        sta prtmod
        lda #14
        sta chary
        lda #15
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #61
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #62
        jsr pattr
        lda #15
        sta chary
        ; lda #15
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #91
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #92
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #90
        jsr pattr
        jsr prskey
        lda #1
        sta gamwon
l01386: lda #6
        cmp scno
        beq *+5
        jmp l01467
        lda #0
        jsr gotob
        cmp #255
        beq :+
        jmp l01467
:
        lda #7
        sta chary
        lda #12
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
l01467: lda #12
        cmp scno
        beq *+5
        jmp l01571
        lda #2
        jsr gotob
        cmp #255
        beq :+
        jmp l01571
:
        lda #12
        sta chary
        lda #11
        sta charx
        lda #10
        sta loopa
l01531: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #33
        jsr pattr
        dec loopa
        beq :+
        jmp l01531
:
l01571: lda #13
        cmp scno
        beq *+5
        jmp l01674
        lda #3
        jsr gotob
        cmp #255
        beq :+
        jmp l01674
:
        lda #12
        sta chary
        lda #9
        sta charx
        lda #8
        sta loopa
l01634: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #71
        jsr pattr
        dec loopa
        beq :+
        jmp l01634
:
l01674: lda #16
        cmp scno
        beq *+5
        jmp l01783
        lda #4
        jsr gotob
        cmp #255
        beq :+
        jmp l01783
:
        lda #17
        sta chary
        lda #26
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #74
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #67
        jsr pattr
l01783: lda #19
        cmp scno
        beq *+5
        jmp l02295
        lda #5
        jsr gotob
        cmp #255
        beq :+
        jmp l02295
:
        lda #0
        cmp varh
        bcc *+5
        jmp l01908
        lda #12
        sta chary
        lda #22
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #73
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #73
        jsr pattr
l01908: lda #21
        cmp varh
        bcc *+5
        jmp l02065
        lda #12
        sta chary
        lda #22
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
        lda #12
        sta chary
        lda #24
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #73
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #73
        jsr pattr
l02065: lda #41
        cmp varh
        bcc *+5
        jmp l02226
        lda #12
        sta chary
        lda #24
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
        lda #12
        sta chary
        lda #26
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #73
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #73
        jsr pattr
        jmp l02295
l02226: lda #12
        sta chary
        lda #26
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
l02295: lda #22
        cmp scno
        beq *+5
        jmp l02404
        lda #6
        jsr gotob
        cmp #255
        beq :+
        jmp l02404
:
        lda #11
        sta chary
        lda #26
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #74
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #67
        jsr pattr
l02404: rts
.out .sprintf("EVENT 11 SIZE = %d", (* - evnt11))

evnt12:
        jsr cls
        lda #1
        sta varm
        lda #0
        sta chary
        lda #4
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #34
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #35
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #38
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #39
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #42
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #43
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #57
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #58
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #54
        jsr pattr
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
        lda #38
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #39
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #42
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #43
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #46
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #47
        jsr pattr
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
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #34
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #35
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #38
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #39
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #50
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #51
        jsr pattr
        lda #1
        sta chary
        lda #4
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #36
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #37
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #40
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #41
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #44
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #45
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #59
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #60
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #55
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #56
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #40
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #41
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #44
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #45
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #48
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #49
        jsr pattr
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
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #36
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #37
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #40
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #41
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #52
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #53
        jsr pattr
        lda #2
        sta chary
        lda #15
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #61
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #62
        jsr pattr
        lda #3
        sta chary
        lda #15
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #63
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #64
        jsr pattr
        lda #5
        sta chary
        lda #11
        sta charx
        lda #20
        jsr dmsg
        lda #7
        sta chary
        lda #11
        sta charx
        lda #4
        jsr dmsg
        lda #9
        sta chary
        lda #11
        sta charx
        lda #5
        jsr dmsg
        lda #11
        sta chary
        ; lda #11
        sta charx
        lda #6
        jsr dmsg
        lda #14
        sta chary
        lda #0
        sta charx
        lda #27
        jsr dmsg
        lda #13
        sta chary
        lda #12
        sta charx
        lda #28
        jsr dmsg
        lda #93
        sta varopt
        lda #5
        sta varobj
        lda #23
        sta chary
        lda #0
        sta charx
        lda #32
        sta z80_c
        lda #7
        sta z80_b
        jsr iscrly
        lda #5
        sta chary
        lda #22
        sta charx
        lda #96
        sta scrly
        rts
.out .sprintf("EVENT 12 SIZE = %d", (* - evnt12))

evnt13:
        lda #0
        sta varc
        ; lda #0
        sta vard
        lda #10
        sta numlif
        lda #9
        sta varl
        jsr cls
        rts
.out .sprintf("EVENT 13 SIZE = %d", (* - evnt13))

evnt14:
        lda #0
        sta vara
        ; lda #0
        sta vard
        ; lda #0
        sta vare
        ; lda #0
        sta chary
        lda #10
        sta charx
        lda #1
        jsr dmsg
        lda #20
        sta chary
        lda #1
        sta charx
        lda #8
        jsr dmsg
        lda #22
        sta chary
        lda #7
        sta charx
        lda varl
        jsr disply
        lda #22
        sta chary
        lda #15
        sta charx
        jsr dscor
        lda #22
        sta chary
        lda #1
        sta charx
        lda #3
        jsr dmsg
        lda #22
        sta chary
        lda #9
        sta charx
        lda #2
        jsr dmsg
        lda #22
        sta chary
        ; lda #22
        sta charx
        lda #9
        jsr dmsg
        lda #21
        sta chary
        lda #9
        sta charx
        lda #23
        jsr dmsg
        lda #3
        cmp scno
        bcs *+5
        jmp o00253
        lda #2
        sta varm
        jmp o00427
o00253: lda #6
        cmp scno
        bcs *+5
        jmp o00283
        lda #3
        sta varm
        jmp o00427
o00283: lda #9
        cmp scno
        bcs *+5
        jmp o00312
        lda #4
        sta varm
        jmp o00427
o00312: lda #13
        cmp scno
        bcs *+5
        jmp o00342
        lda #5
        sta varm
        jmp o00427
o00342: lda #16
        cmp scno
        bcs *+5
        jmp o00372
        lda #2
        sta varm
        jmp o00427
o00372: lda #19
        cmp scno
        bcs *+5
        jmp o00402
        lda #3
        sta varm
        jmp o00427
o00402: lda #22
        cmp scno
        bcs *+5
        jmp o00427
        lda #4
        sta varm
o00427: lda #23
        cmp scno
        beq *+5
        jmp o00452
        lda #9
        sta varm
o00452: lda #24
        cmp scno
        beq *+5
        jmp o00477
        lda #6
        sta varm
o00477: lda #25
        cmp scno
        beq *+5
        jmp o00502
        lda #7
        sta varm
o00502: rts
.out .sprintf("EVENT 14 SIZE = %d", (* - evnt14))

evnt15:
        lda #0
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp p00078
        lda #78
        asl a
        sta sndtyp
        lda #19
        sta chary
        lda #12
        sta charx
        lda #32
        jsr dmsg
        lda #50
        jsr delay
        lda #1
        sta deadf
p00078: rts
.out .sprintf("EVENT 15 SIZE = %d", (* - evnt15))

evnt16:
        lda #5
        jsr delay
        lda numlif
        sec
        sbc #1
        sta numlif
        lda varl
        sec
        sbc #1
        sta varl
        jsr cls
        lda #0
        cmp varl
        beq *+5
        jmp q00075
        lda #25
        sta scno
        jsr nwscr
q00075: rts
.out .sprintf("EVENT 16 SIZE = %d", (* - evnt16))

evnt17:
        rts
.out .sprintf("EVENT 17 SIZE = %d", (* - evnt17))

evnt18:
        jsr cls
        lda #8
        sta varm
        lda #10
        sta chary
        ; lda #10
        sta charx
        lda #1
        sta prtmod
        lda #0
        jsr dmsg
        lda #0
        sta prtmod
        lda #100
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
        .byte "GAME OVER<",141
        .byte "TEA=LEAF TED",141
        .byte "SCORE:",141
        .byte "LIVES:",141
        .byte "1^KEYBOARD",141
        .byte "2^KEMPSTON",141
        .byte "3^SINCLAIR",141
        .byte "TEA=LEAF TED  2018 by Jaime",13
        .byte "Grilo @@",13
        .byte "Move around the levels",13
        .byte "collecting coins and",13
        .byte "items` avoiding the policemen`",13
        .byte "the policewomen`",13
        .byte "the police dogs` the gold digger",13
        .byte "chicks and the crooks^ @@",13
        .byte "More information in",13
        .byte "http:??jg=spectrum^webnode^pt @@",13
        .byte 13
        .byte 141
        .byte "ITEMS:",141
        .byte "COINS:",141
        .byte "PLAYER",141
        .byte "CROOK",141
        .byte "GOLD DIGGER CHICK",141
        .byte "POLICEMAN",141
        .byte "POLICEWOMAN",141
        .byte "POLICE DOG",141
        .byte "COINS",141
        .byte "ITEMS",141
        .byte "PAUSED",141
        .byte "GIMME ALL YOUR MONEY<",141
        .byte "R^REDEFINE",141
        .byte "0",141
        .byte "00",141
        .byte "DOG BITES:",141
        .byte "YOU>RE UNDER ARREST<",141
        .byte "GRRRRRRR<",141
        .byte "DARLING<<<",141
        .byte " Game made by Jaime Grilo using:",13
        .byte " =AGD by Jonathan Cauldwell",13
        .byte " =AGDMusicizer by David Saphier",13
        .byte " Extra code by Travis Highrise",13
        .byte " Loading screen by Andy Green",13
        .byte " Music by Prog Master",13
        .byte " Font by Damien Guard: Pitstop",13
        .byte " Title idea by Paul Pollington",141
        .byte "CREDITS:",141
        .byte "CONGRATULATIONS<",141
        .byte "YOU>VE FINISHED THE GAME",141
        .byte "WITH SUCCESS<",141
        .byte "AAAHHHHHH<",141
        .byte "YOU LOSE ALL",141
        .byte "YOUR LIVES",141
nummsg:
        .byte 35
.out .sprintf("MESSAGE SIZE = %d", (* - msgdat))

chgfx:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 240,176,240,176,240,176,0,0,224,240,224,240,224,240,0,0
        .byte 204,0,255,255,187,255,204,0,204,0,255,255,187,255,204,0
        .byte 0,51,255,255,255,221,0,51,0,51,255,255,255,221,0,51
        .byte 165,161,90,26,165,161,0,0,164,165,74,90,164,165,0,0
        .byte 90,26,165,161,90,26,0,0,74,90,164,165,74,90,0,0
        .byte 165,215,175,158,165,31,175,158,165,31,250,250,135,31,250,250
        .byte 245,245,143,30,245,245,175,30,165,151,175,30,165,151,175,218
        .byte 181,95,158,90,135,95,158,90,135,95,250,250,135,31,250,250
        .byte 245,245,143,30,245,245,143,90,151,95,143,90,151,95,190,90
        .byte 102,0,238,0,170,0,204,0,170,0,187,0,170,136,255,204
        .byte 255,204,170,136,187,0,170,0,204,0,170,0,238,0,102,0
        .byte 15,79,15,15,15,79,175,175,31,15,15,15,31,15,175,175
        .byte 241,240,242,248,245,244,242,250,244,240,250,240,241,240,240,250
        .byte 255,238,136,187,255,119,170,0,204,0,136,0,0,0,0,0
        .byte 119,255,221,17,238,255,0,85,0,51,0,17,0,0,0,0
        .byte 0,0,0,0,0,68,34,170,85,187,170,85,119,238,187,187
        .byte 0,0,0,0,0,68,17,238,119,255,204,51,119,238,0,0
        .byte 0,0,0,0,17,204,17,0,17,204,17,204,51,204,119,68
        .byte 0,0,0,0,51,136,0,136,51,136,51,136,51,204,34,238
        .byte 0,0,0,0,0,0,255,255,153,153,102,102,0,0,0,0
        .byte 17,0,85,68,85,68,85,68,119,221,119,238,68,68,51,136
        .byte 0,136,34,170,34,170,34,170,187,238,119,238,34,34,17,204
        .byte 17,136,51,204,68,34,170,17,153,153,136,153,68,170,51,204
        .byte 34,68,51,204,51,204,102,102,102,102,102,102,204,51,204,51
        .byte 17,153,119,255,255,255,153,153,17,136,17,136,17,136,17,136
        .byte 17,204,0,102,17,204,0,136,17,0,34,0,68,0,136,0
        .byte 51,136,68,68,170,170,153,34,102,204,85,68,34,136,17,0
        .byte 0,0,0,0,102,102,187,221,238,119,187,221,170,85,51,204
        .byte 17,136,17,136,34,68,17,136,34,68,34,68,68,34,68,34
        .byte 0,0,64,0,160,0,250,250,160,85,64,80,0,0,0,0
        .byte 0,0,0,0,145,32,0,0,0,0,0,0,0,0,0,0
        .byte 245,177,250,186,245,177,0,0,228,245,234,250,228,245,0,0
        .byte 80,80,165,165,66,26,5,5,82,90,32,160,0,0,0,0
        .byte 240,240,240,240,240,240,192,48,0,48,0,48,0,48,0,48
        .byte 240,224,240,224,240,224,128,96,128,0,128,0,128,0,128,0
        .byte 0,48,0,48,0,48,0,48,0,48,0,112,0,112,0,0
        .byte 128,0,128,0,128,0,128,0,128,0,192,0,192,0,0,0
        .byte 0,0,50,250,117,245,114,250,117,0,114,0,117,0,114,250
        .byte 0,0,250,234,245,228,250,234,0,100,0,0,0,0,136,0
        .byte 117,245,114,0,117,0,114,0,117,245,114,250,49,245,0,0
        .byte 128,0,0,0,0,0,0,98,245,228,250,234,245,228,0,0
        .byte 0,0,16,90,33,165,18,90,37,128,82,0,37,0,82,90
        .byte 0,0,90,8,165,132,90,72,0,164,0,74,0,164,90,74
        .byte 37,165,82,0,37,0,82,0,37,0,82,0,37,0,0,0
        .byte 165,164,0,74,0,164,0,74,0,164,0,74,0,164,0,0
        .byte 0,0,35,175,87,95,39,175,87,0,39,0,87,0,39,175
        .byte 0,0,175,174,95,78,175,174,0,70,0,0,0,0,136,0
        .byte 87,95,39,0,87,0,39,0,87,0,39,0,87,0,0,0
        .byte 8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,37,165,82,90,37,165,82,0,37,0,82,0,37,0
        .byte 0,0,132,0,90,8,165,128,18,72,1,164,0,74,0,164
        .byte 82,0,37,0,82,0,37,0,82,90,37,165,82,90,0,0
        .byte 0,74,0,164,16,74,33,132,90,8,165,128,72,0,0,0
        .byte 0,0,7,8,7,0,7,0,7,0,7,0,7,0,7,0
        .byte 7,0,7,0,7,0,7,0,7,15,7,15,7,15,0,0
        .byte 0,0,0,0,0,0,0,6,15,14,15,14,15,14,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,17,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,136
        .byte 17,255,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 255,136,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,16,0,112,16,160,48,80,96,160,112,240,48,0
        .byte 0,0,128,0,96,0,240,128,240,192,80,96,240,224,0,192
        .byte 0,68,34,0,17,17,0,238,0,221,0,102,0,51,0,0
        .byte 34,0,0,68,136,136,119,0,187,0,102,0,204,0,0,0
        .byte 0,102,0,119,0,85,0,51,0,85,0,221,17,85,51,255
        .byte 51,255,17,85,0,221,0,85,0,51,0,85,0,119,0,102
        .byte 119,238,136,17,119,238,136,17,119,238,136,17,119,238,136,17
        .byte 95,95,175,175,95,95,175,175,95,95,175,175,95,95,175,175
        .byte 95,95,175,175,95,95,175,175,95,95,175,175,95,95,175,175
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 10,10,0,0,25,68,170,170,8,0,141,5,95,95,175,175
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 255,187,255,187,255,187,0,0,238,255,238,255,238,255,0,0
        .byte 119,238,136,17,119,238,136,17,119,238,136,17,119,238,136,17
        .byte 15,11,15,11,15,11,0,0,14,15,14,15,14,15,0,0
        .byte 240,176,240,176,240,176,0,64,224,240,224,240,224,240,0,0
        .byte 0,0,0,0,0,0,176,208,240,176,112,208,16,224,0,16
        .byte 76,0,1,141,19,70,38,171,85,85,38,171,19,70,1,140
        .byte 0,32,64,16,128,160,0,80,0,96,128,48,192,32,96,48
        .byte 27,17,5,35,87,2,35,34,17,0,0,140,0,78,0,171
        .byte 224,0,48,0,16,128,192,192,240,224,64,144,240,224,240,128
        .byte 8,0,5,140,19,70,38,171,85,85,38,171,19,70,1,140
        .byte 0,0,0,234,17,245,34,0,49,0,80,250,196,196,136,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 245,177,250,186,245,177,0,0,228,245,234,250,228,245,16,0
        .byte 90,26,165,161,90,26,0,0,74,90,164,165,74,90,1,0
        .byte 0,0,0,0,160,1,10,10,5,32,80,24,5,1,64,10
        .byte 0,0,144,0,208,32,208,80,176,80,176,80,144,32,0,0
        .byte 0,68,34,0,17,17,0,238,0,238,0,102,0,51,0,0
        .byte 34,0,0,68,136,136,119,0,119,0,102,0,204,0,0,0
        .byte 0,136,0,204,170,170,170,153,170,170,0,204,0,136,0,0
        .byte 17,0,51,0,85,85,153,85,85,85,51,0,17,0,0,0
        .byte 51,136,0,0,51,136,0,0,255,238,68,68,34,136,17,0
        .byte 17,0,34,136,68,68,255,238,0,0,51,136,0,0,51,136
        .byte 102,204,136,34,136,34,170,170,170,170,136,34,136,34,102,204
        .byte 255,187,255,187,255,187,0,0,238,255,238,255,238,255,0,0
bprop:
        .byte 0
        .byte 2
        .byte 3
        .byte 3
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 6
        .byte 6
        .byte 0
        .byte 0
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
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 1
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
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 5
        .byte 0
        .byte 0
        .byte 0
        .byte 1
        .byte 0
        .byte 2
        .byte 0
        .byte 2
        .byte 1
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
        .byte 1
.out .sprintf("BLOCKS SIZE = %d", (* - chgfx))

sprgfx:
        .byte 3,224,7,240,0,48,3,192,2,192,0,24,1,180,2,36,7,226,10,58,11,250,14,10,3,180,3,96,1,176,3,240
        .byte 3,224,7,240,0,48,3,192,2,192,0,0,1,176,4,40,11,232,8,16,7,232,1,56,0,240,3,120,3,56,7,8
        .byte 3,224,7,240,0,48,3,192,2,192,0,24,1,180,2,36,7,226,10,58,11,250,14,10,3,180,3,96,1,176,3,240
        .byte 3,224,7,240,0,48,3,192,2,192,0,24,25,180,36,42,39,225,18,45,15,233,1,41,1,214,3,184,3,56,7,8
        .byte 7,192,15,224,12,0,3,192,3,64,24,0,45,128,36,64,71,224,92,80,95,208,80,112,45,192,6,192,13,128,15,192
        .byte 7,192,15,224,12,0,3,192,3,64,0,0,13,128,20,32,23,208,8,16,23,224,28,128,15,0,30,192,28,192,16,224
        .byte 7,192,15,224,12,0,3,192,3,64,24,0,45,128,36,64,71,224,92,80,95,208,80,112,45,192,6,192,13,128,15,192
        .byte 7,192,15,224,12,0,3,192,3,64,24,0,45,152,84,36,135,228,180,72,151,240,148,128,107,128,29,192,28,192,16,224
        .byte 3,240,7,248,4,24,2,212,4,0,3,24,0,180,2,36,3,98,38,186,22,186,10,42,7,236,3,96,1,176,3,240
        .byte 3,240,7,248,4,24,2,212,4,0,3,24,64,180,34,34,31,97,18,173,26,169,6,41,7,246,2,216,3,56,7,8
        .byte 15,192,31,224,24,32,43,64,0,32,24,192,45,0,36,64,70,192,93,100,93,104,84,80,55,224,6,192,13,128,15,192
        .byte 15,192,31,224,24,32,43,64,0,32,24,192,45,2,68,68,134,248,181,72,149,88,148,96,111,224,27,64,28,192,16,224
        .byte 0,0,7,192,28,112,48,24,34,136,103,204,70,132,71,196,66,196,103,204,34,136,48,24,28,112,7,192,0,0,0,0
        .byte 0,0,1,128,0,0,1,128,0,0,1,128,0,0,1,128,0,0,1,128,0,0,1,128,0,0,1,128,0,0,0,0
        .byte 7,128,79,192,175,192,168,70,164,138,147,18,92,228,56,72,15,240,8,64,7,128,24,0,45,128,48,128,113,128,1,192
        .byte 1,224,3,242,3,245,98,21,81,37,72,201,39,58,18,28,15,240,2,16,1,224,0,24,1,180,1,12,1,142,3,128
        .byte 3,128,7,192,4,96,2,176,0,40,1,0,0,0,4,192,13,160,6,208,8,16,11,144,7,192,2,128,7,128,10,128
        .byte 3,128,7,192,4,104,2,176,0,32,1,0,0,0,4,192,13,160,6,160,0,64,6,224,15,192,4,192,12,96,20,160
        .byte 1,192,3,224,6,32,13,64,20,0,0,128,0,0,3,32,5,176,11,96,8,16,9,208,3,224,1,64,1,224,1,80
        .byte 1,192,3,224,22,32,13,64,4,0,0,128,0,0,3,32,5,176,5,96,2,0,7,96,3,240,3,32,6,48,5,40
        .byte 255,254,194,134,90,180,218,182,213,86,0,0,255,254,187,186,0,0,68,68,68,68,68,68,0,0,170,170,238,238,68,68
        .byte 15,184,7,240,4,96,2,160,4,0,3,24,128,60,67,236,38,236,27,216,15,216,0,0,1,176,3,96,1,176,3,240
        .byte 15,184,7,240,4,96,34,160,36,0,35,24,32,60,51,254,42,230,59,238,27,236,0,0,3,176,2,216,3,56,7,8
        .byte 29,240,15,224,6,32,5,64,0,32,24,192,60,1,55,194,55,100,27,216,27,240,0,0,13,128,6,192,13,128,15,192
        .byte 29,240,15,224,6,32,5,68,0,36,24,196,60,4,127,204,103,84,119,220,55,216,0,0,13,192,27,64,28,192,16,224
        .byte 0,0,0,0,0,0,0,0,8,0,28,0,34,0,107,254,34,36,28,54,8,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,15,128,31,224,56,0,56,16,60,0,63,16,31,240,0,0,15,224,0,0,0,0,0,0,0,0,0,0
        .byte 3,192,15,192,4,96,34,188,36,0,35,0,32,96,55,240,45,88,31,204,1,140,0,8,1,160,3,96,1,176,3,240
        .byte 3,192,15,192,4,96,2,176,4,16,3,8,128,96,67,240,37,80,27,144,14,224,2,0,3,176,2,216,3,56,7,8
        .byte 3,192,3,240,6,32,61,68,0,36,0,196,6,4,15,236,26,180,51,248,49,128,16,0,5,128,6,192,13,128,15,192
        .byte 3,192,3,240,6,32,13,64,8,32,16,192,6,1,15,194,10,164,9,216,7,112,0,64,13,192,27,64,28,192,16,224
        .byte 0,0,0,0,0,0,0,0,0,0,0,36,0,24,0,62,28,92,63,240,95,208,155,160,60,96,36,96,20,96,0,0
        .byte 9,88,21,80,29,88,21,208,0,0,0,36,0,24,0,62,28,88,63,244,219,208,23,224,60,208,104,144,72,144,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,36,0,24,0,62,28,92,255,240,31,208,27,224,55,96,52,160,36,160,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,36,0,24,0,62,156,92,127,240,27,208,23,224,60,208,104,144,72,144,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,36,0,24,0,124,0,58,56,15,252,11,250,5,217,6,60,6,36,6,40,0,0
        .byte 4,172,10,168,14,172,10,232,0,0,36,0,24,0,124,0,26,56,47,252,11,219,7,232,11,60,9,22,9,18,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,36,0,24,0,124,0,58,56,15,255,11,248,7,216,6,236,5,44,5,36,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,36,0,24,0,124,0,58,57,15,254,11,216,7,232,11,60,9,22,9,18,0,0
        .byte 129,160,8,17,48,8,4,192,138,32,4,65,32,136,16,20,5,8,58,129,65,16,8,12,32,34,68,80,25,36,132,3
        .byte 0,65,4,136,74,64,4,1,128,64,32,8,10,149,53,8,66,0,0,64,81,12,132,65,0,161,32,70,194,9,4,0
        .byte 10,136,132,97,34,0,4,64,1,132,18,138,165,4,2,64,64,12,136,1,1,64,16,161,65,66,48,4,66,17,5,32
frmlst:
        .byte 0,4
        .byte 4,4
        .byte 8,2
        .byte 10,2
        .byte 12,2
        .byte 14,2
        .byte 16,2
        .byte 18,2
        .byte 20,1
        .byte 21,2
        .byte 23,2
        .byte 25,2
        .byte 27,2
        .byte 29,2
        .byte 31,4
        .byte 35,4
        .byte 39,3,42,0
.out .sprintf("SPRITES SIZE = %d", (* - sprgfx))

scdat:
        .word 139,185,252,213,205,243,292,240,247,300,211,230,291,311,251,251,298,252,199,227,234,290,217,106,9,9
        .byte 255,0,9,255,20,12,255,0,8,1,255,0,29,1,255,0,29,1,255,0,29,1,0,0,20,20,255,0,10,20,20,255,0,13
        .byte 1,255,0,29,1,255,0,29,1,255,0,8,20,20,255,0,19,1,255,0,29,1,255,0,29,1,0,0,20,20,255,0,10
        .byte 20,20,255,0,13,1,255,0,23,72,255,0,5,1,255,0,17,1,255,0,4,72,72,255,0,5,9,255,0,8,20,20,255,0,7
        .byte 1,255,0,4,72,72,255,0,5,8,255,0,17,255,33,12,1,255,0,29,1,0,0,20,20,255,0,10,20,20,255,0,13
        .byte 1,255,73,30
        .byte 255,1,31,255,0,6,1,255,0,21,1,1,255,0,6,1,255,0,21,255,1,6,2,3,1,255,0,21,1,1,255,0,4
        .byte 2,3,1,255,0,21,1,1,255,0,4,2,3,1,1,1,2,3,255,1,15,2,3,1,1,255,0,4,2,3,0,0,0
        .byte 2,3,255,0,15,2,3,1,1,255,0,4,2,3,0,0,0,2,3,255,0,15,2,3,255,1,19,2,3,255,1,11,255,0,16
        .byte 1,2,3,255,0,9,1,1,255,0,16,1,2,3,255,0,9,255,1,11,2,3,255,1,15,2,3,1,7,255,0,9,2
        .byte 3,255,0,15,2,3,1,6,255,0,9,2,3,255,0,15,2,3,1,76,76,255,1,16,2,3,255,1,11,255,0,17,2
        .byte 3,255,0,9,9,1,255,0,17,2,3,255,0,9,8,255,1,30
        .byte 255,1,31,255,0,4,1,255,0,10,1,1,1,255,0,10,1,1,255,0,4,1,255,0,11,1,255,0,11,1,1,2,3
        .byte 1,1,1,255,0,23,1,1,2,3,0,0,1,255,0,23,1,1,2,3,0,0,255,1,5,2,3,255,1,15,2,3,1,1
        .byte 2,3,255,0,7,2,3,255,0,5,1,255,0,9,2,3,1,1,2,3,255,0,7,2,3,255,0,5,1,255,0,9,2
        .byte 3,255,1,6,2,3,255,1,11,2,3,255,1,11,255,0,4,2,3,255,0,10,1,2,3,255,0,9,1,1,255,0,4
        .byte 2,3,255,0,10,1,2,3,255,0,9,255,1,6,2,3,255,1,8,0,0,255,1,10,2,3,1,1,255,0,4,2,3
        .byte 255,0,20,2,3,1,1,255,0,4,2,3,255,0,20,2,3,255,1,6,2,3,255,1,4,2,3,255,1,9,2,3,255,1,6
        .byte 7,255,0,4,2,3,255,0,4,2,3,255,0,4,1,255,0,4,2,3,255,0,5,9,6,255,0,4,2,3,255,0,4
        .byte 2,3,255,0,4,1,255,0,4,2,3,255,0,5,8,255,1,30
        .byte 255,1,26,255,68,4,1,255,0,6,1,255,0,17,1,1,1,68,68,1,255,0,6,1,255,0,19,255,1,4,255,0,6
        .byte 1,255,0,21,1,1,255,0,6,1,255,0,21,1,1,0,0,255,1,5,0,0,255,1,9,2,3,255,1,10,255,0,18
        .byte 2,3,255,0,8,9,1,255,0,18,2,3,255,0,8,8,255,1,4,2,3,255,1,13,2,3,255,1,10,0,0,0,2
        .byte 3,0,0,1,255,0,10,2,3,255,0,8,1,1,0,0,0,2,3,0,0,1,255,0,10,2,3,255,0,8,255,1,7
        .byte 0,0,1,1,2,3,255,1,15,2,3,1,1,255,0,9,2,3,255,0,15,2,3,1,1,255,0,9,2,3,255,0,15
        .byte 2,3,1,1,0,0,255,1,7,2,3,255,1,7,0,0,255,1,6,2,3,1,7,255,0,9,2,3,255,0,15,2,3
        .byte 1,6,255,0,9,2,3,255,0,15,2,3,255,1,31
        .byte 255,68,4,255,4,26,68,68,4,4,4,255,0,24,255,4,4,255,0,26,4,4,255,0,28,4,4,255,0,28,4,4,0,0
        .byte 255,4,8,2,3,255,4,10,2,3,4,4,0,0,4,7,255,0,10,2,3,255,0,10,2,3,255,0,4,4,6,255,0,10
        .byte 2,3,255,0,10,2,3,255,0,4,255,4,6,2,3,255,4,10,2,3,255,4,12,255,0,4,2,3,255,0,10,2,3
        .byte 255,0,10,4,4,255,0,4,2,3,255,0,10,2,3,255,0,10,255,4,12,2,3,255,4,10,2,3,255,4,6,255,0,10
        .byte 2,3,255,0,10,2,3,255,0,4,9,4,255,0,10,2,3,255,0,10,2,3,255,0,4,8,255,4,5,2,3,255,4,10
        .byte 2,3,255,4,12,255,0,4,2,3,255,0,10,2,3,255,0,10,4,4,255,0,4,2,3,255,0,10,2,3,255,0,10
        .byte 255,4,31
        .byte 255,4,31,255,0,28,4,4,255,0,28,4,4,2,3,4,4,2,3,255,0,22,4,4,2,3,0,0,2,3,255,0,22
        .byte 4,4,2,3,0,0,255,4,10,2,3,255,4,10,0,0,4,4,2,3,255,0,12,2,3,255,0,12,4,4,2,3,255,0,12
        .byte 2,3,255,0,12,255,4,6,0,0,255,4,11,2,3,255,4,7,2,3,4,4,255,0,16,4,2,3,255,0,7,2,3
        .byte 4,4,255,0,16,4,2,3,255,0,7,2,3,4,4,0,0,4,4,2,3,255,4,7,2,3,4,4,2,3,255,4,7
        .byte 2,3,4,7,255,0,4,2,3,255,0,4,4,0,0,2,3,0,0,2,3,0,0,0,4,0,0,0,2,3,4,6,255,0,4
        .byte 2,3,255,0,4,4,0,0,2,3,0,0,2,3,0,0,0,4,0,0,0,2,3,255,4,6,2,3,255,4,7,2,3
        .byte 255,4,15,255,0,4,2,3,255,0,4,4,0,0,2,3,255,0,13,9,4,255,0,4,2,3,255,0,4,4,0,0,2
        .byte 3,255,0,13,8,255,4,30
        .byte 255,4,31,255,0,28,4,4,255,0,28,4,4,2,3,255,4,4,255,33,7,4,4,255,33,7,255,4,4,2,3,4,4
        .byte 2,3,4,255,0,10,4,4,255,0,10,4,2,3,4,4,2,3,255,0,11,4,4,255,0,11,2,3,4,4,2,3,255,0,8
        .byte 4,0,0,4,4,255,0,11,2,3,255,4,5,2,3,4,4,4,2,3,255,4,5,255,0,11,2,3,4,4,0,0,0
        .byte 2,3,4,0,0,2,3,0,0,255,4,4,2,3,4,2,3,255,4,9,0,0,0,2,3,4,0,0,2,3,0,0,4
        .byte 11,0,0,2,3,4,2,3,255,0,7,9,4,0,0,0,2,3,4,0,0,2,3,0,0,4,10,0,0,2,3,4,2
        .byte 3,255,0,7,8,255,4,7,0,0,2,3,0,0,255,4,18,255,0,8,2,3,255,0,18,4,4,255,0,8,2,3,255,0,18
        .byte 255,4,5,2,3,4,4,4,2,3,4,4,4,2,3,4,4,4,2,3,4,4,4,2,3,255,4,4,7,0,0,0,2
        .byte 3,0,0,0,2,3,0,0,0,2,3,0,0,0,2,3,0,0,0,2,3,0,0,0,4,6,0,0,0,2,3,0,0,0
        .byte 2,3,0,0,0,2,3,0,0,0,2,3,0,0,0,2,3,0,0,0,255,4,31
        .byte 255,12,31,255,0,28,12,12,255,0,28,12,12,2,3,12,12,255,0,24,12,12,2,3,0,12,255,0,24,12,12,2,3
        .byte 0,255,12,5,2,3,255,12,8,2,3,255,12,10,2,3,255,0,6,2,3,255,0,8,2,3,255,0,8,12,12,2,3
        .byte 255,0,6,2,3,255,0,8,2,3,255,0,8,255,12,15,2,3,255,12,11,2,3,12,7,255,0,13,2,3,255,0,11
        .byte 2,3,12,6,255,0,13,2,3,255,0,11,2,3,255,12,6,2,3,255,12,16,2,3,255,12,6,255,0,4,2,3,255,0,4
        .byte 12,255,0,6,12,0,0,0,12,2,3,255,0,4,9,12,255,0,4,2,3,255,0,4,12,255,0,6,12,0,0,0,12
        .byte 2,3,255,0,4,8,12,255,0,4,2,3,255,0,4,12,12,12,2,3,12,12,12,0,0,0,12,2,3,255,12,6,255,0,4
        .byte 2,3,255,0,7,2,3,255,0,7,2,3,255,0,4,12,12,255,0,4,2,3,255,0,7,2,3,255,0,7,2,3,255,0,4
        .byte 255,12,31
        .byte 255,12,31,255,0,28,12,12,255,0,28,12,12,255,0,4,12,12,2,3,12,12,255,0,8,12,12,2,3,12,12,255,0,4
        .byte 12,12,255,0,4,12,0,2,3,0,12,255,0,8,12,0,2,3,0,12,255,0,4,12,12,0,0,12,12,12,0,2,3
        .byte 0,12,12,12,255,0,4,12,12,12,0,2,3,0,12,12,12,0,0,12,12,255,0,6,2,3,255,0,12,2,3,255,0,6
        .byte 12,12,255,0,6,2,3,255,0,12,2,3,255,0,6,12,12,2,3,255,12,9,2,3,255,12,13,2,3,12,12,2,3
        .byte 255,0,8,12,2,3,255,0,13,2,3,12,12,2,3,255,0,8,12,2,3,255,0,13,2,3,255,12,8,2,3,12,12,12
        .byte 2,3,255,12,16,7,255,0,6,2,3,0,0,0,2,3,255,0,15,12,6,255,0,6,2,3,0,0,0,2,3,255,0,15
        .byte 255,12,8,2,3,255,12,8,2,3,255,12,12,255,0,6,2,3,255,0,8,2,3,255,0,10,9,12,255,0,6,2,3
        .byte 255,0,8,2,3,255,0,10,8,255,12,30
        .byte 12,255,68,25,255,12,5,68,12,68,12,68,12,68,12,68,12,68,12,68,12,68,12,68,12,68,12,68,12,68,12,68,12
        .byte 14,15,255,12,28,0,0,12,12,255,0,25,12,2,3,12,12,255,0,25,12,2,3,255,12,9,2,3,255,12,10,2,3
        .byte 255,12,5,2,3,12,12,255,0,7,2,3,255,0,10,2,3,255,0,5,2,3,12,12,255,0,7,2,3,255,0,10,2
        .byte 3,255,0,5,2,3,255,12,5,2,3,12,12,2,3,255,12,4,2,3,255,12,15,0,0,0,2,3,0,0,2,3,255,0,4
        .byte 2,3,255,0,10,12,0,0,12,12,0,0,0,2,3,0,0,2,3,255,0,4,2,3,255,0,10,12,0,0,12,12,2
        .byte 3,255,12,8,2,3,255,12,6,2,3,12,12,12,2,3,12,0,0,12,12,2,3,255,0,8,2,3,12,255,0,5,2
        .byte 3,12,0,0,2,3,12,0,0,12,12,2,3,255,0,8,2,3,12,255,0,5,2,3,12,0,0,2,3,12,0,0,255,12,7
        .byte 2,3,255,12,6,2,3,255,12,6,2,3,12,12,12,0,0,12,7,255,0,5,2,3,255,0,6,2,3,255,0,5,12
        .byte 2,3,255,0,5,9,6,255,0,5,2,3,255,0,6,2,3,255,0,5,12,2,3,0,0,0,13,13,8,255,12,30
        .byte 255,32,31,255,0,28,9,32,255,0,28,8,32,32,32,2,3,32,32,32,255,33,15,255,32,10,2,3,32,32,255,0,17
        .byte 255,32,9,2,3,32,32,255,0,17,255,32,7,0,0,2,3,255,0,24,32,32,0,0,2,3,255,0,4,74,67,74,255,0,7
        .byte 74,67,74,255,0,7,32,32,2,3,255,32,28,2,3,255,0,26,32,32,2,3,255,0,26,32,32,2,3,255,0,13,255,32,11
        .byte 2,3,32,32,2,3,255,0,16,32,255,0,6,32,2,3,32,32,2,3,0,32,0,0,32,67,67,32,0,0,32,67,67
        .byte 0,0,0,32,255,0,6,32,2,3,255,32,6,2,3,255,32,4,2,3,255,32,4,0,0,32,255,0,6,32,2,3,32
        .byte 7,255,0,4,2,3,255,0,4,2,3,255,0,14,2,3,32,6,255,0,4,2,3,255,0,4,2,3,255,0,8,74,67,67
        .byte 74,0,0,2,3,255,32,31
        .byte 255,32,30,7,255,0,17,32,255,0,10,32,6,255,0,17,32,255,0,10,255,32,7,2,3,32,255,0,9,255,32,5,2
        .byte 3,255,32,6,255,0,5,2,3,255,32,6,255,0,4,32,255,0,4,2,3,255,0,4,32,32,255,0,5,2,3,255,0,15
        .byte 2,3,255,0,4,32,32,255,0,5,2,3,255,0,15,2,3,255,0,4,32,32,2,3,255,32,11,2,3,255,32,11,2
        .byte 3,32,32,2,3,255,0,11,2,3,255,0,11,2,3,32,32,2,3,255,0,11,2,3,255,0,11,2,3,32,32,2,3
        .byte 255,0,11,2,3,255,0,11,2,3,32,32,2,3,255,32,24,2,3,32,32,2,3,255,0,24,2,3,32,32,2,3,255,0,24
        .byte 2,3,255,32,7,2,3,255,32,6,2,3,255,32,6,2,3,255,32,7,255,0,5,2,3,255,0,6,2,3,255,0,6
        .byte 2,3,255,0,5,9,32,255,0,5,2,3,255,0,6,2,3,255,0,6,2,3,255,0,5,8,255,32,30
        .byte 255,32,31,255,0,25,32,14,15,32,32,255,0,25,32,0,0,32,32,255,0,25,32,2,3,32,32,2,3,255,32,4,2
        .byte 3,255,32,12,2,3,255,32,4,2,3,32,32,2,3,255,0,4,2,3,32,32,255,0,8,32,32,2,3,255,0,4,2
        .byte 3,32,32,2,3,255,0,4,2,3,32,32,255,0,8,32,32,2,3,255,0,4,2,3,32,32,2,3,255,32,4,2,3
        .byte 255,32,5,2,3,255,32,15,2,3,255,0,4,2,3,255,0,5,2,3,255,0,13,32,32,2,3,255,0,4,2,3,255,0,5
        .byte 2,3,255,0,13,32,32,2,3,255,0,4,2,3,255,0,5,2,3,255,0,13,32,32,2,3,255,32,4,2,3,32,255,0,10
        .byte 32,2,3,255,32,4,2,3,32,32,2,3,255,0,4,2,3,32,32,32,255,0,6,32,32,32,2,3,255,0,4,2,3
        .byte 32,32,2,3,255,0,4,2,3,32,32,255,0,8,32,32,2,3,255,0,4,2,3,255,32,6,0,0,2,3,32,32,255,0,8
        .byte 32,32,2,3,255,32,7,7,255,0,6,2,3,32,32,255,0,8,32,32,2,3,255,0,6,9,6,255,0,6,2,3,32,32
        .byte 255,0,8,32,32,2,3,255,0,6,8,255,32,11,255,12,8,255,32,11
        .byte 32,68,32,68,32,68,32,68,32,255,68,8,5,68,5,68,5,68,5,68,5,68,5,68,5,255,32,4,87,87,255,32,4
        .byte 255,68,6,255,5,7,88,88,255,5,5,255,32,4,87,87,255,32,4,255,68,6,255,5,7,88,88,255,5,5,32,255,0,7
        .byte 66,32,255,68,6,5,255,0,12,9,32,255,0,7,65,32,255,68,6,5,255,0,12,8,32,2,3,255,32,7,255,68,6
        .byte 5,2,3,255,5,11,32,2,3,255,0,4,32,32,255,68,7,5,2,3,255,0,10,5,32,2,3,255,0,4,32,255,68,8
        .byte 5,2,3,255,0,10,5,255,32,5,2,3,32,255,69,8,255,5,11,2,3,5,32,255,0,4,2,3,255,69,19,5,2
        .byte 3,5,32,255,0,4,2,3,255,69,19,5,2,3,5,32,2,3,255,32,5,255,69,8,255,5,8,2,3,5,2,3,5
        .byte 32,2,3,255,0,4,32,32,32,255,69,4,5,5,5,255,0,7,2,3,0,2,3,5,32,2,3,255,0,4,32,32,255,69,6
        .byte 5,5,255,0,7,2,3,0,2,3,5,255,32,5,2,3,32,255,69,8,5,2,3,255,5,8,2,3,5,7,255,0,4
        .byte 2,3,32,255,70,8,5,2,3,255,0,8,2,3,5,6,255,0,4,2,3,32,255,70,8,5,2,3,255,0,8,2,3
        .byte 5,255,32,8,255,12,8,255,5,14
        .byte 255,5,31,255,0,14,5,255,0,13,5,5,255,0,14,5,255,0,13,5,7,255,0,14,5,255,0,13,5,6,255,0,14
        .byte 5,255,0,13,255,5,14,2,3,255,5,7,2,3,5,5,5,2,3,5,5,255,0,12,2,3,5,255,0,6,2,3,0,0,0
        .byte 2,3,5,5,255,0,12,2,3,5,255,0,6,2,3,0,0,0,2,3,5,5,2,3,5,5,2,3,5,5,2,3,255,5,7
        .byte 2,3,255,5,11,2,3,0,0,2,3,0,0,2,3,255,0,4,5,0,0,2,3,255,0,9,5,5,2,3,0,0,2
        .byte 3,0,0,2,3,255,0,4,5,0,0,2,3,255,0,9,5,5,2,3,255,5,13,0,0,255,5,9,2,3,5,5,2
        .byte 3,255,0,24,2,3,5,5,2,3,255,0,24,2,3,255,5,8,2,3,5,5,0,0,255,5,5,2,3,5,5,2,3
        .byte 255,5,7,255,0,6,2,3,255,0,6,5,0,0,2,3,0,0,2,3,255,0,5,9,5,255,0,6,2,3,255,0,6
        .byte 5,0,0,2,3,0,0,2,3,255,0,5,8,255,5,30
        .byte 255,5,22,255,68,8,5,255,0,14,5,255,0,5,5,255,68,8,5,255,0,14,5,255,0,5,5,5,5,255,68,6,5
        .byte 2,3,255,5,5,2,3,255,5,6,255,0,7,5,5,5,255,68,4,5,2,3,255,0,5,2,3,255,0,15,5,5,5
        .byte 68,68,5,2,3,255,0,5,2,3,255,0,17,255,5,4,2,3,255,0,5,2,3,255,0,11,5,5,255,0,6,5,5
        .byte 2,3,255,0,5,2,3,255,0,9,74,67,5,5,255,0,6,255,5,6,2,3,255,5,4,2,3,255,5,14,2,3,5,5
        .byte 255,0,4,2,3,255,0,4,2,3,255,0,14,2,3,5,5,255,0,4,2,3,255,0,4,2,3,255,0,14,2,3,5,5
        .byte 2,3,255,5,11,0,0,0,255,5,4,0,0,255,5,8,2,3,255,0,14,5,255,0,11,5,5,2,3,255,0,14,5
        .byte 255,0,11,255,5,9,2,3,255,5,13,2,3,255,5,5,7,255,0,7,2,3,5,255,0,12,2,3,255,0,4,9,6
        .byte 255,0,7,2,3,5,255,0,12,2,3,255,0,4,8,255,5,30
        .byte 255,68,8,255,5,22,255,68,8,5,255,0,12,5,255,0,7,5,255,68,6,5,5,5,255,0,12,5,255,0,7,5,255,68,4
        .byte 255,5,4,255,0,7,5,255,0,5,5,2,3,5,5,5,2,3,5,68,68,5,5,5,0,0,5,255,0,7,5,255,0,5
        .byte 5,2,3,0,0,0,2,3,255,5,4,255,0,4,5,2,3,255,5,6,2,3,255,5,4,2,3,0,0,0,2,3,5,5
        .byte 11,255,0,6,2,3,5,255,0,4,5,2,3,255,0,4,2,3,0,0,0,2,3,5,5,10,255,0,6,2,3,5,255,0,4
        .byte 5,2,3,255,0,4,2,3,0,0,0,2,3,255,5,9,2,3,5,255,0,4,5,2,3,255,5,13,255,0,7,2,3
        .byte 5,255,0,5,2,3,255,0,11,9,5,255,0,7,2,3,5,255,0,5,2,3,255,0,11,8,5,2,3,255,5,8,2
        .byte 3,255,5,8,2,3,255,5,8,2,3,255,0,8,2,3,255,0,8,2,3,5,255,0,5,5,5,2,3,255,0,8,2
        .byte 3,255,0,8,2,3,5,255,0,5,255,5,7,2,3,255,5,8,2,3,255,5,6,255,0,5,5,7,255,0,5,2,3
        .byte 255,0,8,2,3,255,0,9,5,5,5,6,255,0,5,2,3,255,0,8,2,3,255,0,9,255,5,33
        .byte 255,73,16,1,1,255,68,12,73,255,0,14,73,73,1,1,255,68,11,73,255,0,15,73,73,1,1,255,68,10,73,2,3
        .byte 255,73,10,2,3,255,73,4,1,1,255,68,9,73,2,3,255,0,9,73,2,3,0,0,0,73,73,255,1,10,73,2,3
        .byte 255,0,9,73,2,3,255,0,4,255,73,12,2,3,255,0,9,73,2,3,255,0,14,73,73,2,3,255,0,9,73,2,3
        .byte 255,0,14,255,73,6,2,3,255,73,15,2,3,255,73,6,7,255,0,4,2,3,255,0,8,73,255,0,6,2,3,255,0,5
        .byte 73,6,255,0,4,2,3,255,0,8,73,255,0,6,2,3,255,0,5,255,73,6,2,3,255,73,6,2,3,255,73,12,2
        .byte 3,73,73,255,0,4,2,3,73,255,0,5,2,3,73,255,0,11,2,3,73,73,255,0,4,2,3,73,255,0,5,2,3
        .byte 73,255,0,11,2,3,73,73,255,0,4,2,3,255,73,10,2,3,255,73,12,255,0,4,2,3,255,0,10,2,3,255,0,10
        .byte 9,73,255,0,4,2,3,255,0,10,2,3,255,0,10,8,255,73,30
        .byte 255,68,14,1,1,255,73,14,255,68,13,1,1,73,73,255,0,12,9,255,68,12,1,1,73,73,255,0,13,8,255,68,11
        .byte 1,1,255,73,4,2,3,255,73,11,255,1,12,73,73,0,0,0,2,3,255,0,10,255,73,14,255,0,4,2,3,255,0,10
        .byte 73,73,255,0,16,2,3,255,0,10,73,73,255,0,16,2,3,255,0,10,73,73,2,3,255,73,24,2,3,73,73,2,3
        .byte 255,0,24,2,3,73,73,2,3,255,0,24,2,3,73,73,2,3,255,73,11,2,3,255,73,11,2,3,73,73,2,3,255,0,10
        .byte 73,2,3,73,255,0,10,2,3,73,73,2,3,255,0,10,73,2,3,73,255,0,10,2,3,255,73,15,2,3,73,2,3
        .byte 255,73,11,7,255,0,13,2,3,0,2,3,255,0,10,73,6,255,0,13,2,3,0,2,3,255,0,10,255,73,31
        .byte 255,73,30,7,255,0,28,73,6,255,0,28,73,98,98,255,73,17,2,3,255,73,6,2,3,73,73,255,0,18,2,3,255,0,6
        .byte 2,3,73,73,255,0,18,2,3,255,0,6,2,3,255,73,5,255,0,15,2,3,255,0,6,2,3,255,73,5,67,74,255,0,13
        .byte 2,3,255,0,6,2,3,255,73,7,255,0,6,73,73,2,3,255,73,15,0,0,0,255,73,4,255,0,6,2,3,73,255,0,12
        .byte 9,73,255,0,4,73,73,73,67,74,255,0,4,2,3,73,255,0,12,8,73,255,0,5,255,73,8,2,3,73,2,3,73,73
        .byte 255,0,6,255,73,4,255,0,11,66,73,2,3,73,2,3,255,0,10,73,73,0,0,67,255,0,8,65,73,2,3,73,2
        .byte 3,255,0,10,73,73,2,3,255,73,11,2,3,73,2,3,73,73,255,0,4,255,73,6,2,3,255,0,11,2,3,0,2
        .byte 3,255,0,10,73,73,2,3,255,0,11,2,3,0,2,3,255,0,10,255,73,31
        .byte 255,75,31,255,0,28,9,75,255,0,28,8,75,255,0,4,255,75,8,2,3,255,75,6,2,3,255,75,8,255,0,4,75
        .byte 255,0,7,2,3,255,0,6,2,3,255,0,6,75,75,0,0,75,75,75,255,0,7,2,3,255,0,6,2,3,255,0,6
        .byte 75,75,255,0,12,2,3,255,0,6,2,3,255,0,6,75,75,255,0,12,2,3,255,0,6,2,3,255,0,6,255,75,10
        .byte 2,3,255,75,16,2,3,75,7,255,0,8,2,3,75,255,0,14,75,2,3,75,6,255,0,8,2,3,75,255,0,14,75
        .byte 2,3,255,75,13,255,0,14,75,2,3,75,75,255,0,26,2,3,75,75,255,0,26,2,3,75,75,0,0,75,75,2,3
        .byte 75,75,0,0,75,75,2,3,75,75,0,0,75,75,2,3,75,75,0,0,2,3,75,75,255,0,4,2,3,255,0,6,2
        .byte 3,255,0,6,2,3,255,0,4,2,3,75,75,255,0,4,2,3,255,0,6,2,3,255,0,6,2,3,255,0,4,2,3
        .byte 255,75,31
        .byte 255,75,30,7,255,0,10,75,255,0,17,75,6,255,0,10,75,255,0,17,255,75,6,2,3,255,75,5,255,0,17,75,75
        .byte 255,0,4,2,3,255,0,4,75,255,0,17,75,75,255,0,4,2,3,255,0,4,255,75,9,2,3,75,75,75,2,3,255,75,4
        .byte 255,0,4,2,3,255,0,4,75,255,0,8,2,3,0,0,0,2,3,0,0,9,75,255,0,4,2,3,255,0,4,75,255,0,8
        .byte 2,3,0,0,0,2,3,0,0,8,75,75,75,0,0,2,3,0,0,75,75,75,0,0,75,75,2,3,255,75,13,255,0,4
        .byte 2,3,255,0,7,75,0,2,3,255,0,11,75,75,255,0,4,2,3,255,0,7,75,0,2,3,255,0,11,255,75,4,0,0
        .byte 2,3,255,0,4,255,75,4,0,2,3,255,0,11,75,75,255,0,4,2,3,255,0,9,2,3,255,0,11,75,75,255,0,4
        .byte 2,3,255,0,9,2,3,255,0,11,75,75,2,3,255,75,6,2,3,255,75,7,2,3,75,75,75,2,3,255,75,6,2
        .byte 3,255,0,6,2,3,0,0,0,75,0,0,0,2,3,0,75,0,2,3,255,0,4,75,75,2,3,255,0,6,2,3,0,0,0
        .byte 75,0,0,0,2,3,0,75,0,2,3,255,0,4,255,75,31
        .byte 255,75,31,255,0,28,75,75,255,0,28,75,75,255,0,28,75,75,255,0,28,255,75,15,67,74,255,0,4,74,67,74,255,0,6
        .byte 75,7,255,0,12,255,75,14,2,3,75,6,255,0,13,75,75,255,0,11,2,3,255,75,12,255,0,4,75,75,255,0,10
        .byte 2,3,75,75,75,255,0,8,75,75,255,0,4,75,75,255,0,9,255,75,4,11,255,0,9,75,75,255,0,4,75,75,255,0,8
        .byte 255,75,4,10,255,0,10,75,75,255,0,4,75,2,3,255,75,13,255,0,9,75,75,255,0,4,2,3,255,0,8,9,75,75,75
        .byte 255,0,4,67,74,255,0,5,75,75,0,0,0,2,3,255,0,8,8,75,75,255,0,5,75,75,75,255,0,5,255,75,4
        .byte 2,3,75,75,75,2,3,255,75,5,255,0,18,2,3,0,0,0,2,3,0,0,0,75,75,255,0,9,67,74,255,0,7
        .byte 2,3,0,0,0,2,3,0,0,0,255,75,31
        .byte 1,1,1,255,68,27,1,1,255,68,28,1,255,68,29,1,255,68,29,1,255,68,29,1,255,68,29,1,255,68,29,1,255,69,29
        .byte 1,255,69,29,1,255,69,29,1,1,1,255,69,27,1,1,255,69,28,7,255,70,29,6,255,70,29,255,1,4,255,0,23
        .byte 83,0,0,255,1,5,255,0,21,77,79,81,0,255,1,6,67,74,255,0,11,74,67,1,67,74,0,0,78,80,82,0,255,1,22
        .byte 255,73,8
        .byte 255,0,0,255,0,0,255,0,28
        .byte 255,0,0,255,0,0,255,0,28
numsc:
        .byte 26
.out .sprintf("SCREENS SIZE = %d", (* - scdat))

nmedat:
        .byte 8,1,24,24,8,10,24,120,2,4,72,24,8,14,120,120,8,7,48,72,8,13,72,120,8,3,96,72,0,1,104,152,7,11,104,216,255
        .byte 0,1,104,16,2,4,80,128,2,4,128,16,2,4,32,72,1,9,32,192,2,4,80,48,2,4,56,16,255
        .byte 0,1,128,16,1,9,32,120,2,4,32,64,2,4,16,32,2,4,104,152,5,12,32,208,2,4,128,128,255
        .byte 0,1,128,16,1,10,32,120,2,4,32,88,2,4,80,120,2,4,104,176,6,15,128,200,5,13,80,200,2,4,56,16,255
        .byte 0,1,56,16,2,4,32,208,2,4,32,32,5,12,104,168,4,7,32,56,2,4,104,224,2,4,128,64,255
        .byte 0,1,104,16,2,4,104,200,2,4,56,64,2,4,128,16,2,4,32,208,4,6,56,192,6,15,32,152,5,13,32,88,255
        .byte 0,1,128,16,2,4,128,56,2,4,80,16,2,4,56,136,2,4,128,224,6,14,128,176,6,15,16,192,1,9,104,224,255
        .byte 0,1,80,16,2,4,104,104,2,4,32,48,2,4,80,160,2,4,128,224,1,9,32,224,6,15,128,208,255
        .byte 0,1,104,16,2,4,104,88,2,4,32,96,2,4,32,144,2,4,128,208,5,12,56,120,5,13,104,176,255
        .byte 0,1,128,16,8,8,80,224,2,4,32,16,2,4,128,160,2,4,32,200,2,4,128,208,1,10,32,120,6,15,56,144,6,15,128,144,255
        .byte 0,1,128,16,2,4,80,184,2,4,104,96,2,4,56,224,2,4,16,16,1,9,56,136,6,15,128,208,255
        .byte 0,1,16,16,2,4,48,160,2,4,104,120,2,4,24,104,4,6,48,136,6,15,80,192,4,7,128,16,2,4,16,160,255
        .byte 0,1,128,16,8,16,128,96,8,16,128,112,8,16,128,128,8,16,128,144,3,2,80,224,2,4,24,160,2,4,104,32,2,4,128,200,255
        .byte 0,1,128,16,8,16,128,72,8,16,128,88,8,16,128,104,8,16,128,120,8,8,16,40,8,8,16,192,2,4,104,168,2,4,32,216,3,2,80,184,2,4,56,32,255
        .byte 0,1,32,16,2,4,32,136,2,4,80,64,2,4,128,136,2,4,128,112,1,10,128,16,1,9,104,208,255
        .byte 0,1,128,16,2,4,80,144,2,4,56,224,2,4,104,48,4,7,56,120,6,14,128,96,5,13,16,112,255
        .byte 0,1,128,16,3,3,80,112,2,4,112,224,2,4,32,112,2,4,16,200,2,4,32,160,6,15,128,208,255
        .byte 0,1,80,16,2,4,104,72,2,4,56,224,2,4,80,136,1,9,128,200,6,15,16,112,2,4,128,16,255
        .byte 0,1,128,16,2,4,128,224,2,4,16,224,2,4,104,96,2,4,104,144,4,6,56,208,4,7,56,32,255
        .byte 0,1,16,16,2,4,128,224,2,4,40,16,2,4,80,224,2,4,16,224,1,9,128,184,255
        .byte 0,1,80,16,2,4,32,32,2,4,104,32,2,4,104,160,2,4,104,64,2,4,104,192,5,12,128,208,6,15,56,144,255
        .byte 0,1,16,16,2,4,80,96,2,4,56,16,2,4,56,80,2,4,128,128,2,4,32,104,3,2,104,224,6,14,128,48,255
        .byte 0,1,56,16,1,9,40,144,2,4,32,16,6,15,128,224,2,4,128,16,2,4,80,208,255
        .byte 0,1,104,16,1,9,128,112,2,4,104,72,2,4,128,144,2,4,128,200,255
.out .sprintf("POSITIONS SIZE = %d", (* - nmedat))

NUMOBJ = 21
objdta:
        .byte 0,0,0,0,0,0,0,0,0,0,4,0,36,0,220,0,220,0,36,0,4,0,0,0,0,0,0,0,0,0,0,0,6,80,120,6,80,120
        .byte 1,128,1,128,2,64,1,128,1,128,7,224,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,16,224,9,16,224
        .byte 1,128,1,128,2,64,1,128,1,128,7,224,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,16,224,12,16,224
        .byte 0,0,0,0,0,0,0,0,0,0,0,32,0,36,0,59,0,59,0,36,0,32,0,0,0,0,0,0,0,0,0,0,13,32,64,13,32,64
        .byte 0,0,0,0,0,0,0,0,0,0,4,0,36,0,220,0,220,0,36,0,4,0,0,0,0,0,0,0,0,0,0,0,16,56,16,16,56,16
        .byte 0,0,0,0,0,0,0,0,0,0,0,32,0,36,0,59,0,59,0,36,0,32,0,0,0,0,0,0,0,0,0,0,19,104,96,19,104,96
        .byte 0,0,0,0,0,0,0,0,0,0,4,0,36,0,220,0,220,0,36,0,4,0,0,0,0,0,0,0,0,0,0,0,22,88,16,22,88,16
        .byte 2,128,6,192,4,64,1,0,2,128,6,192,5,64,12,96,12,96,24,48,24,48,48,24,0,0,96,12,96,12,0,0,10,56,16,10,56,16
        .byte 0,0,0,0,3,224,2,0,3,224,3,224,3,224,3,224,3,224,7,224,15,224,63,224,126,96,0,0,0,0,0,0,11,16,224,11,16,224
        .byte 0,0,0,0,7,192,0,64,7,192,7,192,7,192,7,192,7,192,7,224,7,240,7,252,6,126,0,0,0,0,0,0,15,104,224,15,104,224
        .byte 0,0,0,0,0,0,128,1,126,126,63,252,97,134,99,198,62,124,28,56,0,0,0,0,0,0,0,0,0,0,0,0,2,128,152,2,128,152
        .byte 0,0,1,128,13,176,13,176,109,176,109,176,96,48,127,230,63,238,24,124,31,232,31,240,31,224,8,64,15,192,0,0,23,120,176,23,120,176
        .byte 0,0,1,128,13,176,13,176,13,182,13,182,12,6,103,254,119,252,62,24,23,248,15,248,7,248,2,16,3,240,0,0,17,16,104,17,16,104
        .byte 3,128,1,0,3,128,13,96,24,48,16,16,36,8,34,8,49,216,32,8,16,16,24,48,13,96,3,128,0,0,0,0,13,128,32,13,128,32
        .byte 0,0,0,0,15,224,10,160,15,224,14,224,15,224,30,240,28,112,28,112,28,112,56,56,56,56,56,56,112,28,112,28,7,32,224,7,32,224
        .byte 0,0,0,0,0,200,3,248,7,248,4,200,0,192,0,192,0,192,0,192,0,192,0,192,0,192,0,192,0,0,0,0,4,128,96,4,128,96
        .byte 0,0,0,224,1,48,0,120,0,168,0,48,0,224,1,192,3,128,7,0,14,0,28,0,40,0,80,0,56,0,0,0,19,104,224,19,104,224
        .byte 0,0,0,0,7,224,9,144,26,88,36,36,35,196,26,88,20,40,10,80,9,144,6,96,4,32,2,64,1,128,0,0,0,120,24,0,120,24
        .byte 0,0,0,0,24,24,60,60,66,66,85,170,104,22,111,246,104,22,111,246,104,22,111,246,104,22,15,240,0,0,0,0,3,80,16,3,80,16
        .byte 0,0,0,0,0,0,0,32,0,248,3,252,7,190,13,119,31,255,32,2,31,252,0,0,0,0,0,0,0,0,0,0,1,16,24,1,16,24
        .byte 0,0,0,0,84,84,84,84,0,0,42,42,42,42,0,0,84,84,84,84,0,0,42,42,42,42,0,0,84,84,84,84,0,104,184,0,104,184
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
        .byte 60,102,110,86,102,102,60,0
        .byte 24,40,56,24,24,24,24,0
        .byte 124,10,6,60,96,96,62,0
        .byte 124,10,6,124,6,6,124,0
        .byte 96,96,108,108,54,12,12,0
        .byte 62,80,96,60,6,102,60,0
        .byte 62,96,96,92,102,102,60,0
        .byte 124,6,6,26,6,6,6,0
        .byte 60,86,102,60,102,102,60,0
        .byte 60,102,102,58,6,6,124,0
        .byte 0,24,24,0,24,24,0,0
        .byte 0,24,24,0,24,24,48,0
        .byte 24,24,24,40,56,0,24,0
        .byte 0,0,0,126,0,0,0,0
        .byte 24,24,48,0,0,0,0,0
        .byte 0,6,10,20,40,80,96,0
        .byte 108,56,108,56,108,0,0,0
        .byte 60,102,102,86,126,102,102,0
        .byte 124,102,102,92,102,102,124,0
        .byte 62,80,96,96,96,96,62,0
        .byte 124,86,102,102,102,102,124,0
        .byte 62,96,96,92,96,96,62,0
        .byte 62,96,96,92,96,96,96,0
        .byte 62,96,96,110,102,106,62,0
        .byte 102,102,102,94,102,102,102,0
        .byte 126,20,24,24,24,24,126,0
        .byte 6,6,6,6,6,10,124,0
        .byte 102,102,108,88,108,102,102,0
        .byte 96,96,96,96,96,80,62,0
        .byte 108,190,254,214,198,198,198,0
        .byte 102,86,126,126,110,102,102,0
        .byte 60,106,102,102,102,102,60,0
        .byte 124,102,102,92,96,96,96,0
        .byte 60,102,102,102,100,106,54,0
        .byte 124,102,102,92,102,102,102,0
        .byte 62,80,96,60,6,6,124,0
        .byte 126,20,24,24,24,24,24,0
        .byte 102,102,102,102,102,86,60,0
        .byte 102,102,102,102,86,60,24,0
        .byte 198,198,198,214,190,238,198,0
        .byte 102,102,86,60,102,102,102,0
        .byte 102,102,86,60,24,24,24,0
        .byte 126,10,12,24,48,96,126,0
        .byte 14,20,24,24,24,24,14,0
        .byte 0,96,80,40,20,10,6,0
        .byte 112,40,24,24,24,24,112,0
        .byte 0,0,0,0,24,24,0,0
        .byte 0,0,0,0,0,0,0,255
        .byte 0,0,0,0,24,24,48,0
        .byte 0,0,60,6,62,106,62,0
        .byte 0,96,96,92,102,102,124,0
        .byte 0,0,62,80,96,96,62,0
        .byte 0,6,6,58,102,102,62,0
        .byte 0,0,60,102,94,96,62,0
        .byte 0,14,24,110,24,24,24,0
        .byte 0,0,62,102,58,6,60,0
        .byte 0,96,96,92,102,102,102,0
        .byte 0,24,0,56,40,24,24,0
        .byte 0,6,0,14,10,6,124,0
        .byte 0,96,102,102,92,102,102,0
        .byte 0,48,48,48,48,40,28,0
        .byte 0,0,108,190,214,214,214,0
        .byte 0,0,124,86,102,102,102,0
        .byte 0,0,60,86,102,102,60,0
        .byte 0,0,124,102,102,92,96,96
        .byte 0,0,62,102,102,58,6,6
        .byte 0,0,124,86,96,96,96,0
        .byte 0,0,62,80,60,6,124,0
        .byte 0,24,118,24,24,24,14,0
        .byte 0,0,102,102,102,86,60,0
        .byte 0,0,102,102,86,60,24,0
        .byte 0,0,198,214,214,190,108,0
        .byte 0,0,102,86,60,102,102,0
        .byte 0,0,102,102,58,6,124,0
        .byte 0,0,126,10,60,96,126,0
        .byte 14,20,24,112,24,24,14,0
        .byte 24,24,24,24,24,24,24,0
        .byte 112,40,24,14,24,24,112,0
        .byte 54,94,108,0,0,0,0,0
        .byte 124,130,186,162,162,186,130,124
.out .sprintf("FONT SIZE = %d", (* - font))

jtab:
        .byte 248,250,252,254,254,255,255,255,0,0,0,1,1,1,2,2,4,6,8,8,8,99
keys:   .byte 66,97,104,72,73,16,55,48,49,17,18
.out .sprintf("KEYS SIZE = %d", (* - keys))

data_end:
.out .sprintf("TOTAL DATA SIZE = %d", (* - data_start))
