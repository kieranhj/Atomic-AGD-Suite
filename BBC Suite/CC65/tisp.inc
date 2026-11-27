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
WINDOWWID = 24 ;面面面面面面面面面面面面
MAPWID = 9
        .byte 255,255,255,255,255,255,255,255,255
mapdat:
        .byte 255,255,255,0,255,255,22,255,255,255,255,255,17,18,19,20,21,255,255,255,255,16,15,14,13,12,255,255,255,255,10,9,8,7,11,255,255,4,255,1,2,3,5,6,255
        .byte 255,255,255,255,255,255,255,255,255
stmap:  .byte 39

evnt00:
        jsr ifall
        lda #2
        cmp vart
        beq *+5
        jmp a00051
        lda #56
        ldy #8
        sta (z80_ix),y
        lda #48
        iny; ldy #9
        sta (z80_ix),y
a00051: jsr skobj
        sta varobj
        lda joyval
        and #32
        beq :+
        jmp a00135
:
        lda #255
        cmp varobj
        bne *+5
        jmp a00135
        lda varobj
        sta varq
        lda varobj
        jsr getob
        lda #75
        asl a
        sta sndtyp
        lda #15
        jsr delay
a00135: lda #6
        cmp varq
        beq *+5
        jmp a00160
        lda #1
        sta varr
a00160: lda #7
        cmp varq
        beq *+5
        jmp a00184
        lda #1
        sta varr
a00184: lda #11
        cmp varq
        beq *+5
        jmp a00209
        lda #1
        sta varr
a00209: lda #12
        cmp varq
        beq *+5
        jmp a00234
        lda #1
        sta varr
a00234: lda #255
        cmp varq
        bne *+5
        jmp a00494
        lda #1
        cmp varr
        beq *+5
        jmp a00494
        ; lda #1
        cmp varg
        beq *+5
        jmp a00323
        ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #7
        jsr drpob
a00323: lda #2
        cmp varg
        beq *+5
        jmp a00377
        ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #6
        jsr drpob
a00377: lda #3
        cmp varg
        beq *+5
        jmp a00432
        ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #11
        jsr drpob
a00432: lda #4
        cmp varg
        beq *+5
        jmp a00486
        ldy #9
        lda (z80_ix),y
        sta dispx
        dey; ldy #8
        lda (z80_ix),y
        sta dispy
        lda #12
        jsr drpob
a00486: lda #0
        sta varr
a00494: lda #7
        cmp varq
        beq *+5
        jmp a00528
        lda #1
        sta varg
        lda #255
        sta varq
a00528: lda #6
        cmp varq
        beq *+5
        jmp a00561
        lda #2
        sta varg
        lda #255
        sta varq
a00561: lda #11
        cmp varq
        beq *+5
        jmp a00594
        lda #3
        sta varg
        lda #255
        sta varq
a00594: lda #12
        cmp varq
        beq *+5
        jmp a00628
        lda #4
        sta varg
        lda #255
        sta varq
a00628: lda #7
        cmp varq
        bcc *+5
        jmp a00706
        lda #10
        cmp varq
        bcs *+5
        jmp a00706
        lda varl
        clc
        adc #1
        sta varl
        lda #66
        asl a
        sta sndtyp
        lda #1
        sta varn
        lda #255
        sta varq
a00706: lda #167
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp a00811
        jsr scrr
        lda #10
        ldy #9
        sta (z80_ix),y
        dey; ldy #8
        lda (z80_ix),y
        sta varo
        lda #2
        sta z80_c
        lda varo
        clc
        adc z80_c
        sta varo
        iny; ldy #9
        lda (z80_ix),y
        sta varp
        lda #1
        sta varh
        rts
a00811: lda #8
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp a00892
        jsr scrl
        lda #166
        ldy #9
        sta (z80_ix),y
        dey; ldy #8
        lda (z80_ix),y
        sta varo
        iny; ldy #9
        lda (z80_ix),y
        sta varp
        lda #1
        sta varh
        rts
a00892: lda #9
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp a00972
        jsr scru
        lda #149
        ldy #8
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        sta varo
        iny; ldy #9
        lda (z80_ix),y
        sta varp
        lda #1
        sta varh
        rts
a00972: lda #160
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp a01053
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
        lda #1
        sta varh
        rts
a01053: lda #0
        cmp vark
        beq *+5
        jmp a02136
        lda joyval
        and #8
        beq :+
        jmp a01195
:
        jsr laddu
        beq :+
        jmp a01195
:
        lda #2
        ldy #6
        sta (z80_ix),y
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #2
        cmp vara
        beq *+5
        jmp a01195
        lda #0
        jsr animsp
        lda #1
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a01195
        lda #10
        asl a
        sta sndtyp
a01195: lda joyval
        and #4
        beq :+
        jmp a01321
:
        jsr laddd
        beq :+
        jmp a01321
:
        lda #2
        ldy #6
        sta (z80_ix),y
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #2
        cmp vara
        beq *+5
        jmp a01321
        lda #0
        jsr animsp
        lda #1
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a01321
        lda #10
        asl a
        sta sndtyp
a01321: lda joyval
        and #1
        beq :+
        jmp a01717
:
        jsr cangr
        beq :+
        jmp a01717
:
        lda joyval
        and #4
        beq :+
        jmp a01487
:
        jsr laddd
        beq :+
        jmp a01388
:
        jmp a01483
a01388: lda #0
        ldy #6
        sta (z80_ix),y
        lda #2
        cmp vara
        beq *+5
        jmp a01461
        lda #0
        jsr animsp
        lda #2
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a01461
        lda #15
        asl a
        sta sndtyp
a01461: ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a01483: jmp a01717
a01487: lda joyval
        and #8
        beq :+
        jmp a01623
:
        jsr laddu
        beq :+
        jmp a01523
:
        jmp a01618
a01523: lda #0
        ldy #6
        sta (z80_ix),y
        lda #2
        cmp vara
        beq *+5
        jmp a01596
        lda #0
        jsr animsp
        lda #2
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a01596
        lda #15
        asl a
        sta sndtyp
a01596: ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a01618: jmp a01717
a01623: lda #0
        ldy #6
        sta (z80_ix),y
        lda #2
        cmp vara
        beq *+5
        jmp a01695
        lda #0
        jsr animsp
        lda #2
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a01695
        lda #15
        asl a
        sta sndtyp
a01695: ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a01717: lda joyval
        and #2
        beq :+
        jmp a02114
:
        jsr cangl
        beq :+
        jmp a02114
:
        lda joyval
        and #4
        beq :+
        jmp a01884
:
        jsr laddd
        beq :+
        jmp a01784
:
        jmp a01879
a01784: lda #1
        ldy #6
        sta (z80_ix),y
        lda #2
        cmp vara
        beq *+5
        jmp a01857
        lda #0
        jsr animsp
        lda #2
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a01857
        lda #15
        asl a
        sta sndtyp
a01857: ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a01879: jmp a02114
a01884: lda joyval
        and #8
        beq :+
        jmp a02019
:
        jsr laddu
        beq :+
        jmp a01919
:
        jmp a02014
a01919: lda #1
        ldy #6
        sta (z80_ix),y
        lda #2
        cmp vara
        beq *+5
        jmp a01992
        lda #0
        jsr animsp
        lda #2
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a01992
        lda #15
        asl a
        sta sndtyp
a01992: ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a02014: jmp a02114
a02019: lda #1
        ldy #6
        sta (z80_ix),y
        lda #2
        cmp vara
        beq *+5
        jmp a02092
        lda #0
        jsr animsp
        lda #2
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a02092
        lda #15
        asl a
        sta sndtyp
a02092: ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a02114: lda joyval
        and #16
        beq :+
        jmp a02136
:
        jsr jump
a02136: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a02201
:
        lda #0
        cmp varh
        beq *+5
        jmp a02201
        lda #1
        sta vark
        lda #11
        ldy #6
        sta (z80_ix),y
a02201: lda #1
        cmp vark
        beq *+5
        jmp a02255
        lda #2
        cmp vara
        beq *+5
        jmp a02255
        lda #0
        jsr animsp
        lda #50
        asl a
        sta sndtyp
a02255: jmp grav
.out .sprintf("EVENT 0 SIZE = %d", (* - evnt00))

evnt01:
        lda #7
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp b00036
        lda #3
        asl a
        sta sndtyp
b00036: lda #14
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp b00058
b00058: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b00116
        lda #4
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp b00116
        lda #3
        ; ldy #6
        sta (z80_ix),y
b00116: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b00295
        jsr cangl
        beq :+
        jmp b00277
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
        jmp b00237
:
        lda #1
        ldy #11
        sta (z80_ix),y
b00237: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp b00290
b00277: lda #1
        ldy #11
        sta (z80_ix),y
b00290: jmp b00447
b00295: jsr cangr
        beq :+
        jmp b00434
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
        jmp b00394
:
        lda #0
        ldy #11
        sta (z80_ix),y
b00394: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp b00447
b00434: lda #0
        ldy #11
        sta (z80_ix),y
b00447: lda #1
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b00505
        lda #3
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp b00505
        lda #4
        ; ldy #6
        sta (z80_ix),y
b00505: lda #2
        cmp vara
        beq *+5
        jmp b00531
        lda #0
        jsr animsp
b00531: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00628
:
        lda #0
        cmp varh
        beq *+5
        jmp b00628
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #11
        ldy #6
        sta (z80_ix),y
        lda #1
        sta vark
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
b00628: lda #7
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp b00650
b00650: lda #14
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp b00672
b00672: rts
.out .sprintf("EVENT 1 SIZE = %d", (* - evnt01))

evnt02:
        lda #7
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00036
        lda #3
        asl a
        sta sndtyp
c00036: lda #12
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00119
        lda #1
        iny; ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp c00098
        lda #130
        asl a
        sta sndtyp
        jmp c00111
c00098: lda #130
        asl a
        sta sndtyp
c00111: lda #0
        jsr animsp
c00119: lda #2
        cmp vara
        beq *+5
        jmp c00145
        lda #0
        jsr animsp
c00145: lda #7
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00247
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00247
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #11
        ldy #6
        sta (z80_ix),y
        lda #1
        sta vark
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
c00247: lda #12
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00349
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00349
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #11
        ldy #6
        sta (z80_ix),y
        lda #1
        sta vark
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
c00349: rts
.out .sprintf("EVENT 2 SIZE = %d", (* - evnt02))

evnt03:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp d00082
        jsr cangu
        beq :+
        jmp d00064
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp d00078
d00064: lda #1
        ldy #11
        sta (z80_ix),y
d00078: jmp d00136
d00082: jsr cangd
        beq :+
        jmp d00123
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp d00136
d00123: lda #0
        ldy #11
        sta (z80_ix),y
d00136: lda #0
        jsr animsp
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00242
:
        lda #0
        cmp varh
        beq *+5
        jmp d00242
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #11
        ldy #6
        sta (z80_ix),y
        lda #1
        sta vark
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
d00242: lda #7
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00264
d00264: rts
.out .sprintf("EVENT 3 SIZE = %d", (* - evnt03))

evnt04:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp e00082
        jsr cangl
        beq :+
        jmp e00064
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp e00078
e00064: lda #1
        ldy #11
        sta (z80_ix),y
e00078: jmp e00136
e00082: jsr cangr
        beq :+
        jmp e00123
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp e00136
e00123: lda #0
        ldy #11
        sta (z80_ix),y
e00136: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp e00217
        jsr cangu
        beq :+
        jmp e00199
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp e00212
e00199: lda #1
        ldy #10
        sta (z80_ix),y
e00212: jmp e00271
e00217: jsr cangd
        beq :+
        jmp e00257
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp e00271
e00257: lda #0
        ldy #10
        sta (z80_ix),y
e00271: lda #0
        jsr animsp
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00376
:
        lda #0
        cmp varh
        beq *+5
        jmp e00376
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #1
        sta vark
        lda #11
        ldy #6
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
e00376: rts
.out .sprintf("EVENT 4 SIZE = %d", (* - evnt04))

evnt05:
        lda #25
        cmp vare
        bcs *+5
        jmp f00076
        lda #0
        ldy #7
        sta (z80_ix),y
        iny; ldy #8
        lda (z80_ix),y
        ldy #11
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y
        ldy #12
        sta (z80_ix),y
        jmp f00225
f00076: jsr cangd
        beq :+
        jmp f00144
:
        lda #1
        ldy #7
        sta (z80_ix),y
        lda #4
        sta z80_c
        iny; ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        jmp f00225
f00144: lda #0
        ldy #7
        sta (z80_ix),y
        lda #6
        sta z80_c
        lda #5
        sta z80_b
        jsr spawn
        ldy #11
        lda (z80_ix),y
        ldy #8
        sta (z80_ix),y
        ldy #12
        lda (z80_ix),y
        ldy #9
        sta (z80_ix),y
        lda #1
        sta vare
f00225: lda #1
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp f00327
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00327
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #11
        ldy #6
        sta (z80_ix),y
        lda #1
        sta vark
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
f00327: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp f00507
:
        lda #0
        ldy #7
        sta (z80_ix),y
        lda #4
        sta z80_c
        iny; ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #6
        sta z80_c
        lda #5
        sta z80_b
        jsr spawn
        lda #4
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #8
        sta (z80_ix),y
        ldy #11
        lda (z80_ix),y
        ldy #8
        sta (z80_ix),y
        ldy #12
        lda (z80_ix),y
        ldy #9
        sta (z80_ix),y
        lda #1
        sta vare
f00507: rts
.out .sprintf("EVENT 5 SIZE = %d", (* - evnt05))

evnt06:
        lda #5
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp g00129
        ; lda #5
        iny; ldy #7
        cmp (z80_ix),y
        bcs *+5
        jmp g00073
        lda #2
        cmp vara
        beq *+5
        jmp g00073
        lda #0
        jsr animsp
g00073: lda #6
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp g00129
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #1
        sta vare
        lda #45
        asl a
        sta sndtyp
g00129: lda #5
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp g00231
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00231
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #11
        ldy #6
        sta (z80_ix),y
        lda #1
        sta vark
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
g00231: rts
.out .sprintf("EVENT 6 SIZE = %d", (* - evnt06))

evnt07:
        lda #15
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h00060
        lda #1
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp h00060
        lda #16
        ldy #6
        sta (z80_ix),y
h00060: lda #16
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h00118
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp h00118
        lda #15
        ldy #6
        sta (z80_ix),y
h00118: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp h00198
        jsr cangl
        beq :+
        jmp h00180
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp h00194
h00180: lda #1
        ldy #11
        sta (z80_ix),y
h00194: jmp h00252
h00198: jsr cangr
        beq :+
        jmp h00239
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp h00252
h00239: lda #0
        ldy #11
        sta (z80_ix),y
h00252: lda #0
        jsr animsp
        lda #7
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h00283
h00283: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp h00380
:
        lda #0
        cmp varh
        beq *+5
        jmp h00380
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #11
        ldy #6
        sta (z80_ix),y
        lda #1
        sta vark
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
h00380: rts
.out .sprintf("EVENT 7 SIZE = %d", (* - evnt07))

evnt08:
        lda #10
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i00049
        lda #2
        cmp vara
        beq *+5
        jmp i00049
        lda #0
        jsr animsp
i00049: lda #6
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i00161
        lda #2
        cmp vara
        beq *+5
        jmp i00096
        lda #0
        jsr animsp
i00096: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i00161
:
        lda joyval
        and #32
        beq :+
        jmp i00161
:
        lda #6
        cmp varf
        beq *+5
        jmp i00161
        lda #1
        sta vart
i00161: lda #9
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i00198
        lda varg
        iny; ldy #7
        sta (z80_ix),y
i00198: lda #10
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i01590
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i01590
:
        lda joyval
        and #32
        beq :+
        jmp i01590
:
        lda #0
        cmp varg
        beq *+5
        jmp i00279
        rts
i00279: lda #3
        cmp varg
        beq *+5
        jmp i00446
        lda #12
        cmp scno
        beq *+5
        jmp i00446
        lda #19
        sta chary
        lda #10
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
        lda #39
        jsr pattr
        lda #16
        sta chary
        lda #12
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        lda #97
        asl a
        sta sndtyp
        lda #0
        sta varg
i00446: lda #2
        cmp varg
        beq *+5
        jmp i00899
        lda #7
        cmp scno
        beq *+5
        jmp i00899
        lda #10
        sta varc
        lda #5
        sta loopa
i00496: lda #17
        sta chary
        lda varc
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
        lda #100
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #101
        jsr pattr
        lda #18
        sta chary
        lda varc
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
        lda #103
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #102
        jsr pattr
        lda #128
        asl a
        sta sndtyp
        lda #3
        jsr delay
        lda varc
        clc
        adc #1
        sta varc
        dec loopa
        beq :+
        jmp i00496
:
        lda #17
        sta chary
        lda #15
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
        lda #18
        sta chary
        lda #15
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
        lda #100
        asl a
        sta sndtyp
        lda #0
        sta varg
i00899: lda #1
        cmp varg
        beq *+5
        jmp i01085
        lda #15
        cmp scno
        beq *+5
        jmp i01085
        lda #14
        sta varc
        lda #8
        sta loopa
i00950: lda #4
        sta charx
        lda varc
        sta chary
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
        lda #36
        jsr pattr
        lda #42
        asl a
        sta sndtyp
        lda #4
        jsr delay
        lda varc
        clc
        adc #1
        sta varc
        dec loopa
        beq :+
        jmp i00950
:
        lda #0
        sta varg
        lda #1
        sta vars
i01085: lda #4
        cmp varg
        beq *+5
        jmp i01590
        lda #0
        cmp scno
        beq *+5
        jmp i01590
        lda #7
        sta varc
        lda #6
        sta loopa
i01135: lda varc
        sta chary
        lda varc
        ldy #11
        sta (z80_ix),y
        lda #15
        sta charx
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
        lda #61
        jsr pattr
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
        lda #61
        jsr pattr
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
        lda #46
        jsr pattr
        ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #11
        sta (z80_ix),y
        ; ldy #11
        lda (z80_ix),y
        sta chary
        lda #15
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
        lda #0
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        lda varc
        sec
        sbc #1
        sta varc
        lda #115
        asl a
        sta sndtyp
        lda #2
        jsr delay
        dec loopa
        beq :+
        jmp i01135
:
        lda #0
        sta varg
i01590: rts
.out .sprintf("EVENT 8 SIZE = %d", (* - evnt08))

evnt09:
        lda #0
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00052
        lda varo
        ldy #8
        sta (z80_ix),y
        lda varp
        iny; ldy #9
        sta (z80_ix),y
j00052: lda #5
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00087
        lda #1
        sta vare
        jmp j00095
j00087: lda #0
        sta vare
j00095: rts
.out .sprintf("EVENT 9 SIZE = %d", (* - evnt09))

evnt10:
        lda #1
        cmp varh
        beq *+5
        jmp k00034
        lda vari
        clc
        adc #1
        sta vari
k00034: lda vara
        clc
        adc #1
        sta vara
        lda varb
        clc
        adc #1
        sta varb
        lda #3
        cmp vara
        beq *+5
        jmp k00089
        lda #0
        sta vara
k00089: lda #254
        cmp varb
        beq *+5
        jmp k00164
        lda #0
        cmp vard
        beq *+5
        jmp k00164
        ; lda #0
        sta varb
        lda #1
        sta vard
        lda #23
        sta chary
        lda #1
        sta charx
        lda #2
        jsr dmsg
k00164: lda #254
        cmp varb
        beq *+5
        jmp k00240
        lda #1
        cmp vard
        beq *+5
        jmp k00240
        lda #0
        sta varb
        ; lda #0
        sta vard
        lda #23
        sta chary
        lda #1
        sta charx
        lda #11
        jsr dmsg
k00240: lda #10
        cmp vari
        beq *+5
        jmp k00273
        lda #0
        sta vari
        ; lda #0
        sta varh
k00273: lda #0
        cmp vart
        beq *+5
        jmp k00411
        lda #1
        cmp varn
        beq *+5
        jmp k00411
        lda #25
        sta varc
        lda #11
        sta chary
        lda #25
        sta charx
        lda #8
        jsr dmsg
        lda varl
        sta loopa
k00349: lda #11
        sta chary
        lda varc
        sta charx
        lda #9
        jsr dmsg
        lda varc
        clc
        adc #1
        sta varc
        dec loopa
        beq :+
        jmp k00349
:
        lda #0
        sta varn
k00411: lda #1
        cmp vars
        beq *+5
        jmp k00588
        lda #9
        cmp scno
        beq *+5
        jmp k00588
        lda #1
        sta varc
        lda #7
        sta loopa
k00460: lda varc
        sta chary
        lda #4
        sta charx
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
        lda #36
        jsr pattr
        lda #95
        asl a
        sta sndtyp
        lda #4
        jsr delay
        lda varc
        clc
        adc #1
        sta varc
        dec loopa
        beq :+
        jmp k00460
:
        lda #0
        sta vars
k00588: lda #1
        cmp vart
        beq *+5
        jmp k00712
        ; lda #1
        sta varc
        jsr cls
        lda #50
        sta loopa
k00625: lda varc
        asl a
        sta sndtyp
        lda #1
        jsr delay
        lda #2
        sta z80_c
        lda varc
        clc
        adc z80_c
        sta varc
        dec loopa
        beq :+
        jmp k00625
:
        lda #4
        sta scno
        jsr nwscr
        lda #3
        sta varm
        lda #2
        sta vart
k00712: lda #3
        cmp vart
        beq *+5
        jmp k00755
        lda joyval
        and #32
        beq :+
        jmp k00755
:
        lda #1
        sta gamwon
k00755: lda #5
        cmp vart
        beq *+5
        jmp k00788
        lda #2
        sta varm
        lda #0
        sta vart
k00788: rts
.out .sprintf("EVENT 10 SIZE = %d", (* - evnt10))

evnt11:
        lda #1
        cmp vark
        beq *+5
        jmp l00067
        lda #2
        cmp vara
        beq *+5
        jmp l00067
        lda #9
        cmp varj
        bcs *+5
        jmp l00067
        lda varj
        clc
        adc #1
        sta varj
l00067: lda #10
        cmp varj
        beq *+5
        jmp l00109
        lda #0
        sta varj
        ; lda #0
        sta vark
        lda #1
        sta deadf
l00109: lda #0
        cmp vare
        bcc *+5
        jmp l00141
        lda vare
        clc
        adc #1
        sta vare
l00141: lda #255
        cmp varq
        bne *+5
        jmp l00408
        lda #5
        sta chary
        lda #0
        cmp varq
        beq *+5
        jmp l00200
        lda #25
        sta charx
        lda #21
        jsr dmsg
l00200: lda #1
        cmp varq
        beq *+5
        jmp l00233
        lda #27
        sta charx
        lda #21
        jsr dmsg
l00233: lda #2
        cmp varq
        beq *+5
        jmp l00267
        lda #29
        sta charx
        lda #21
        jsr dmsg
l00267: lda #3
        cmp varq
        beq *+5
        jmp l00309
        ; lda #3
        sta chary
        lda #27
        sta charx
        lda #6
        jsr dmsg
l00309: lda #4
        sta chary
        ; lda #4
        cmp varq
        beq *+5
        jmp l00351
        lda #26
        sta charx
        lda #22
        jsr dmsg
l00351: lda #5
        cmp varq
        beq *+5
        jmp l00385
        lda #28
        sta charx
        lda #22
        jsr dmsg
l00385: lda #255
        sta varq
        lda varf
        clc
        adc #1
        sta varf
l00408: lda #2
        cmp vart
        beq *+5
        jmp l00484
        lda #1
        sta chary
        lda #25
        sta charx
        ; lda #25
        jsr dmsg
        lda #2
        sta chary
        lda #25
        sta charx
        lda #26
        jsr dmsg
        lda #3
        sta vart
l00484: rts
.out .sprintf("EVENT 11 SIZE = %d", (* - evnt11))

evnt12:
        lda #100
        jsr delay
        jsr cls
        lda #1
        sta varm
        ; lda #1
        sta chary
        ; lda #1
        sta charx
        lda #12
        jsr dmsg
        lda #6
        sta chary
        lda #0
        sta charx
        lda #13
        jsr dmsg
        lda #9
        sta chary
        lda #2
        sta charx
        lda #17
        jsr dmsg
        lda #9
        sta chary
        lda #8
        sta charx
        lda #19
        jsr dmsg
        lda #9
        sta chary
        lda #24
        sta charx
        lda #18
        jsr dmsg
        lda #23
        sta chary
        lda #1
        sta charx
        lda #11
        jsr dmsg
        lda #11
        sta chary
        lda #9
        sta charx
        lda #14
        jsr dmsg
        lda #13
        sta chary
        lda #9
        sta charx
        lda #15
        jsr dmsg
        lda #15
        sta chary
        lda #9
        sta charx
        lda #16
        jsr dmsg
        lda #17
        sta chary
        lda #0
        sta charx
        lda #20
        jsr dmsg
        jsr prskey
        lda #105
        asl a
        sta sndtyp
        lda #20
        jsr delay
        lda #5
        sta vart
        lda #2
        sta varm
        rts
.out .sprintf("EVENT 12 SIZE = %d", (* - evnt12))

evnt13:
        lda #0
        sta vara
        lda #3
        sta varl
        lda #0
        sta varb
        lda #1
        sta vard
        lda #0
        sta varj
        ; lda #0
        sta vark
        lda #152
        ldy #8
        sta (z80_ix),y
        lda #56
        iny; ldy #9
        sta (z80_ix),y
        dey; ldy #8
        lda (z80_ix),y
        sta varo
        iny; ldy #9
        lda (z80_ix),y
        sta varp
        lda #0
        sta varm
        ; lda #0
        sta vari
        ; lda #0
        sta varh
        ; lda #0
        sta vare
        ; lda #0
        sta varg
        ; lda #0
        sta varf
        ; lda #0
        sta varn
        ; lda #0
        sta varr
        lda #5
        sta vart
        lda #3
        sta numlif
        jsr cls
        lda #23
        sta chary
        lda #1
        sta charx
        lda #2
        jsr dmsg
        lda #0
        sta chary
        lda #24
        sta charx
        lda #1
        jsr dmsg
        lda #24
        sta charx
        lda #3
        jsr dmsg
        lda #5
        sta chary
        lda #24
        sta charx
        lda #4
        jsr dmsg
        lda #4
        sta chary
        lda #24
        sta charx
        lda #5
        jsr dmsg
        lda #3
        sta chary
        lda #27
        sta charx
        lda #6
        jsr dmsg
        lda #2
        sta chary
        lda #24
        sta charx
        lda #7
        jsr dmsg
        lda #8
        sta chary
        lda #24
        sta charx
        lda #10
        jsr dmsg
        lda #9
        sta chary
        lda #24
        sta charx
        lda #7
        jsr dmsg
        lda #11
        sta chary
        lda #25
        sta charx
        lda #8
        jsr dmsg
        lda #7
        sta chary
        lda #24
        sta charx
        lda #7
        jsr dmsg
        rts
.out .sprintf("EVENT 13 SIZE = %d", (* - evnt13))

evnt14:
        lda #1
        sta varn
        rts
.out .sprintf("EVENT 14 SIZE = %d", (* - evnt14))

evnt15:
        rts
.out .sprintf("EVENT 15 SIZE = %d", (* - evnt15))

evnt16:
        lda #150
        asl a
        sta sndtyp
        lda varl
        sec
        sbc #1
        sta varl
        lda #0
        cmp varl
        beq *+5
        jmp q00055
        ; lda #0
        sta numlif
q00055: lda #0
        sta vark
        ; lda #0
        sta varc
        lda varo
        ldy #8
        sta (z80_ix),y
        lda varp
        iny; ldy #9
        sta (z80_ix),y
        rts
.out .sprintf("EVENT 16 SIZE = %d", (* - evnt16))

evnt17:
        jsr cls
        lda #10
        sta chary
        lda #12
        sta charx
        lda #0
        jsr dmsg
        lda #200
        jsr delay
        rts
.out .sprintf("EVENT 17 SIZE = %d", (* - evnt17))

evnt18:
        jsr cls
        lda #50
        jsr delay
        lda #4
        sta chary
        lda #5
        sta charx
        lda #23
        jsr dmsg
        lda #8
        sta chary
        lda #0
        sta charx
        lda #24
        jsr dmsg
        lda #20
        sta chary
        lda #10
        sta charx
        lda #25
        jsr dmsg
        lda #20
        sta chary
        lda #16
        sta charx
        lda #26
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
        .byte "GAME OVER",141
        .byte "aELIXIRa",141
        .byte "THEaINCREDIBLEaSHRINKINGaPROF.",141
        .byte "ELEMENTS",141
        .byte "ababab",141
        .byte "aacac",141
        .byte "d",141
        .byte "eefeffef",141
        .byte "gggggg",141
        .byte "g",141
        .byte "aLIVES:a",141
        .byte "aaaaa2017aRUCKSACKaGAMESaaaaa",141
        .byte "aPECULIARaPERPLEXINGaPERILSaOF",13
        .byte "aaaaaaaPROFESSORaPOLDARK!",13
        .byte 13
        .byte "aTHEaINCREDIBLEaSHRINKINGaPROF.",13
        .byte 141
        .byte "aaaaaAaGAMEaBY:aJOHNaBLYTHE",13
        .byte "aaaaaaaaaaaaVER.a1.1",141
        .byte "a1a-aKEYBOARDa",141
        .byte "a2a-aKEMPSTONa",141
        .byte "a3a-aSINCLAIRa",141
        .byte "hijjjj",141
        .byte "jjjjkl",141
        .byte "aPOWEREDaBYaAGDa",141
        .byte "aaaaaaaaaaaaCONTROLS:",13
        .byte "aaaaaaaZ-LEFTaX-RIGHTaK-UP",13
        .byte "aaaaaaaaaM-DOWNaaL-JUMPa",13
        .byte "aaaaaaaaSPACE-PICKUP/USE",141
        .byte "b",141
        .byte "c",141
        .byte "WELLaDONEaPROFESSOR",141
        .byte "aYOU",39
        .byte "VEaSAVEDaYOURSELFaANDaNOW",13
        .byte "aYOUaCANaWORKaONaSAVINGaTHEaLAB",13
        .byte "aFROMaTHESEaPESKYaMICROaBOTS.",13
        .byte 13
        .byte "aTHANKSaFORaPLAYING...",141
        .byte "PRESS",141
        .byte "SPACE",141
nummsg:
        .byte 27
.out .sprintf("MESSAGE SIZE = %d", (* - msgdat))

chgfx:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 245,245,240,240,244,241,242,250,240,240,248,248,244,244,240,240
        .byte 80,80,160,0,0,64,160,160,16,16,160,160,240,240,0,0
        .byte 221,0,170,0,221,0,170,0,221,0,170,0,221,0,170,0
        .byte 0,27,0,5,0,27,0,5,0,27,0,5,0,27,0,5
        .byte 221,0,170,0,221,0,170,0,221,85,102,238,119,85,17,255
        .byte 0,0,0,0,0,0,0,0,85,85,170,170,85,85,175,175
        .byte 2,85,38,171,76,85,0,171,85,85,171,170,85,70,175,136
        .byte 0,17,0,102,0,255,17,238,51,85,102,170,85,68,170,136
        .byte 51,204,119,238,255,255,255,255,255,255,255,255,119,238,51,204
        .byte 0,0,0,0,102,0,255,0,255,0,102,0,0,0,0,0
        .byte 8,0,38,0,27,0,5,136,10,76,5,38,2,10,1,5
        .byte 85,0,238,0,85,0,136,0,0,0,0,0,0,0,0,0
        .byte 0,10,0,39,0,2,0,1,0,0,0,0,0,0,0,0
        .byte 95,78,170,175,17,70,0,34,0,85,0,35,0,85,0,171
        .byte 119,255,238,170,85,0,102,0,204,0,170,0,221,0,170,0
        .byte 255,255,170,170,0,0,85,85,0,0,0,0,0,0,0,0
        .byte 245,245,250,250,245,245,250,250,255,255,255,255,255,255,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,68,160,186,80
        .byte 90,90,135,165,90,120,165,165,120,90,165,135,90,210,165,165
        .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
        .byte 240,240,240,240,240,240,240,240,240,240,240,240,240,240,240,240
        .byte 221,187,170,221,187,85,85,153,102,51,51,255,0,51,0,17
        .byte 221,187,187,85,170,221,221,153,238,34,255,204,204,0,136,0
        .byte 153,255,68,255,34,85,0,0,51,255,0,0,51,255,0,0
        .byte 245,245,250,250,85,85,0,0,245,213,0,0,245,213,0,0
        .byte 245,245,250,170,85,68,0,0,160,128,0,0,160,128,0,0
        .byte 51,255,0,0,119,255,68,0,187,255,34,85,68,255,153,85
        .byte 245,213,0,0,245,213,0,0,245,85,250,250,245,245,170,170
        .byte 160,128,0,0,160,128,0,0,160,128,250,200,213,100,170,186
        .byte 0,0,0,0,0,0,0,0,0,204,17,51,34,0,204,0
        .byte 245,245,240,240,244,241,242,248,240,240,248,242,241,240,240,240
        .byte 245,245,240,240,240,245,242,248,240,240,242,242,241,244,240,240
        .byte 80,96,160,160,0,96,160,160,80,80,32,208,240,208,0,16
        .byte 160,160,192,80,224,0,208,80,160,128,208,80,240,240,0,0
        .byte 0,17,0,17,0,17,0,17,0,34,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,136,0,136,0,136,0,136,0
        .byte 1,164,66,66,129,164,26,74,165,164,66,10,1,4,0,0
        .byte 255,136,0,238,255,187,255,255,255,255,170,170,85,68,0,0
        .byte 240,128,0,224,240,176,240,240,240,240,160,160,80,0,0,0
        .byte 17,78,38,38,25,78,171,174,95,78,38,170,17,68,0,0
        .byte 0,0,85,221,85,238,85,221,85,238,51,221,34,238,17,204
        .byte 0,0,1,132,2,74,32,164,80,88,5,164,80,88,5,164
        .byte 241,245,248,242,241,245,248,242,241,245,248,242,241,245,248,242
        .byte 241,91,248,188,241,91,248,188,241,91,248,188,241,91,248,188
        .byte 240,240,250,250,241,245,248,242,241,245,248,242,241,245,240,240
        .byte 90,90,250,250,241,91,248,188,241,91,248,188,90,90,165,165
        .byte 64,240,64,240,64,240,64,240,64,240,64,240,64,240,64,240
        .byte 240,96,240,160,240,96,240,160,240,96,240,160,240,96,240,160
        .byte 0,16,0,48,0,80,0,176,16,48,32,112,32,112,64,240
        .byte 224,0,240,0,208,128,224,128,240,64,224,192,240,96,224,160
        .byte 15,31,143,175,79,79,15,143,31,15,47,47,79,31,143,143
        .byte 79,31,175,47,31,31,15,143,95,31,15,143,79,95,143,47
        .byte 239,255,143,31,191,223,175,79,47,95,191,223,143,31,255,127
        .byte 15,15,15,15,79,79,15,143,31,15,47,47,79,31,143,143
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
        .byte 255,255,153,170,170,136,255,255,255,221,170,170,238,221,255,255
        .byte 0,0,119,255,119,204,119,0,68,0,119,136,119,136,119,255
        .byte 0,0,255,238,51,238,0,238,0,34,17,238,17,238,255,238
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 245,245,250,250,245,245,250,250,245,245,250,250,245,245,250,250
        .byte 240,240,250,250,245,245,250,250,245,245,250,250,245,245,240,240
        .byte 255,95,239,175,255,95,239,175,255,95,239,175,255,95,239,175
        .byte 95,79,175,143,95,79,175,143,95,79,175,143,95,79,175,143
        .byte 95,95,175,175,95,95,175,175,95,95,175,175,95,95,175,175
        .byte 95,95,174,39,93,19,170,137,25,17,140,35,78,87,175,175
        .byte 5,5,10,10,5,5,10,10,5,5,10,10,5,5,10,10
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 240,180,240,90,225,180,240,120,240,180,240,120,225,180,90,90
        .byte 245,245,250,250,245,245,250,250,245,245,250,250,245,245,250,250
        .byte 199,61,190,218,79,45,15,15,15,15,15,75,135,31,203,62
        .byte 245,245,250,250,245,245,250,250,245,245,250,250,245,245,250,250
        .byte 196,49,216,114,228,177,216,114,245,245,234,250,245,245,250,250
        .byte 196,49,234,186,213,117,250,186,228,245,250,250,245,245,250,250
        .byte 3,12,4,2,11,13,15,15,15,15,15,13,6,10,3,12
        .byte 80,80,160,0,0,64,160,160,16,16,160,160,240,240,0,0
        .byte 127,239,191,255,255,223,255,255,255,223,255,191,223,95,127,239
        .byte 245,245,250,250,245,245,80,114,160,245,80,50,32,145,16,0
        .byte 90,90,165,165,90,90,165,165,90,90,165,165,90,90,165,165
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 95,95,175,175,95,68,175,175,95,95,141,175,25,0,175,175
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 17,0,16,0,17,0,16,136,17,128,16,136,17,128,98,234
        .byte 1,15,1,10,3,13,3,10,7,5,7,10,15,5,14,10
        .byte 15,8,9,8,2,12,9,4,4,10,8,6,4,11,10,5
        .byte 1,10,1,10,1,10,1,10,1,10,1,10,1,10,1,10
        .byte 4,8,1,0,4,8,1,0,4,8,1,0,4,8,1,0
        .byte 15,15,10,10,5,5,10,10,5,5,10,10,5,5,10,10
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 16,74,36,36,24,74,161,164,90,74,36,160,16,64,0,0
        .byte 0,17,0,17,0,34,0,34,0,34,0,34,0,17,0,17
        .byte 48,192,64,32,176,208,240,240,240,240,240,208,96,160,48,192
        .byte 245,245,152,250,228,245,250,114,245,177,250,186,245,213,250,234
        .byte 117,245,186,216,213,177,234,186,245,49,200,50,177,245,250,250
        .byte 95,76,0,34,0,93,35,5,87,8,38,137,68,27,35,0
        .byte 19,95,4,0,27,0,170,140,17,78,137,170,93,2,0,140
        .byte 1,161,88,26,133,132,0,16,132,5,90,16,129,0,72,16
        .byte 133,128,88,26,33,161,8,0,160,33,0,90,128,129,0,18
        .byte 0,238,51,119,119,187,255,255,255,255,85,255,51,255,119,255
        .byte 238,204,255,102,255,119,255,255,255,255,255,34,255,204,255,238
        .byte 255,255,255,255,255,238,255,136,255,238,187,238,119,204,51,136
        .byte 119,255,255,255,255,255,255,119,255,119,102,255,17,221,0,238
        .byte 196,117,186,186,85,85,80,234,117,160,170,34,213,213,234,50
        .byte 213,213,200,152,213,85,186,80,177,213,186,216,177,177,114,250
        .byte 245,213,234,50,228,245,34,50,228,245,34,152,228,117,250,250
        .byte 196,196,186,98,177,213,114,200,117,245,114,114,128,245,250,250
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
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 5
        .byte 5
        .byte 5
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 3
        .byte 3
        .byte 3
        .byte 3
        .byte 3
        .byte 3
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
        .byte 5
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 0
        .byte 1
        .byte 0
        .byte 0
        .byte 0
        .byte 1
        .byte 0
        .byte 2
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 5
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 2
        .byte 0
        .byte 2
        .byte 0
        .byte 1
        .byte 0
        .byte 0
        .byte 5
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
.out .sprintf("BLOCKS SIZE = %d", (* - chgfx))

sprgfx:
        .byte 5,228,7,252,14,72,29,176,11,40,3,40,13,176,6,64,27,224,60,16,119,182,165,182,206,176,15,56,30,220,0,224
        .byte 9,224,7,254,14,72,29,176,11,40,3,40,13,176,6,64,27,224,28,16,55,180,55,180,53,176,12,188,27,8,3,176
        .byte 0,0,1,224,23,252,14,10,29,176,11,40,3,40,5,176,10,64,28,16,27,176,21,176,13,180,19,184,13,0,14,56
        .byte 9,224,7,254,14,72,29,176,11,40,3,40,13,176,6,64,27,224,28,16,55,180,55,180,53,176,12,188,27,8,3,176
        .byte 39,160,63,224,18,112,13,184,20,208,20,192,13,176,2,96,7,216,8,60,109,238,109,165,13,115,28,240,59,120,7,0
        .byte 7,144,127,224,18,112,13,184,20,208,20,192,13,176,2,96,7,216,8,56,45,236,45,236,13,172,61,48,16,216,13,192
        .byte 0,0,7,128,63,232,80,112,13,184,20,208,20,192,13,160,2,80,8,56,13,216,13,168,45,176,29,200,0,176,28,112
        .byte 7,144,127,224,18,112,13,184,20,208,20,192,13,176,2,96,7,216,8,56,45,236,45,236,13,172,61,48,16,216,13,192
        .byte 3,128,13,224,31,240,31,240,31,240,31,240,109,160,82,80,63,248,31,252,15,236,31,240,31,240,63,56,58,216,60,224
        .byte 1,192,6,240,15,248,15,248,15,248,15,248,5,182,10,74,31,252,63,248,55,240,15,248,15,248,28,252,27,92,7,56
        .byte 0,0,0,0,0,0,192,8,32,16,32,18,48,52,74,121,218,134,122,206,244,95,170,175,138,87,82,73,18,72,36,36
        .byte 0,0,64,16,64,16,64,18,32,52,32,121,50,134,74,206,218,255,119,127,249,159,138,167,82,163,36,144,36,136,8,0
        .byte 0,0,16,0,16,32,16,16,16,20,16,52,48,121,74,134,218,206,122,255,245,95,202,167,18,161,37,34,73,32,73,64
        .byte 0,0,0,0,64,16,32,16,32,18,32,52,48,121,74,134,218,206,122,255,245,95,138,167,82,161,18,162,36,144,36,144
        .byte 0,0,0,0,0,0,16,3,8,4,72,4,44,12,158,82,97,91,115,94,250,47,245,85,234,81,146,74,18,72,36,36
        .byte 0,0,8,2,8,2,72,2,44,4,158,4,97,76,115,82,255,91,254,238,249,159,229,81,197,74,9,36,17,36,0,16
        .byte 0,0,0,8,4,8,8,8,40,8,44,8,158,12,97,82,115,91,255,94,250,175,229,83,133,72,68,164,4,146,2,146
        .byte 0,0,0,0,8,2,8,4,72,4,44,4,158,12,97,82,115,91,255,94,250,175,229,81,133,74,69,72,9,36,9,36
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,118,94
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,252,119,254
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,248,2,124,34,252,119,254
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,240,3,252,4,190,84,254,83,252,127,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,32,1,1,8,2,4,14,7,92,7,252,3,248,137,240,127,255
        .byte 0,0,0,0,0,12,0,0,0,0,4,0,0,0,0,0,0,0,0,64,0,0,0,0,14,142,3,24,137,240,127,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,91,122
        .byte 0,0,0,0,0,0,2,0,64,0,160,0,64,0,0,0,0,4,0,0,0,0,8,0,0,0,0,64,0,160,64,64
        .byte 2,0,64,0,160,0,64,0,0,0,0,4,0,0,0,0,8,0,0,0,0,64,0,160,64,64,0,0,0,0,0,0
        .byte 0,0,0,0,0,4,0,0,0,0,8,0,0,64,0,160,0,64,64,0,0,0,2,0,0,0,0,0,0,0,0,64
        .byte 0,0,0,64,8,160,0,64,0,0,96,0,96,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,192,0,196
        .byte 80,10,168,21,84,42,160,5,13,176,28,56,29,184,1,128,45,180,108,182,1,0,97,134,112,142,56,28,36,36,16,8
        .byte 0,0,0,0,28,56,32,4,205,179,28,56,29,184,1,128,45,180,108,182,1,0,97,134,112,142,56,28,36,36,16,8
        .byte 8,4,64,64,5,8,3,160,39,192,149,80,15,232,15,226,23,208,71,192,11,168,4,64,65,18,8,0,32,64,0,0
        .byte 32,16,2,2,16,160,5,192,3,228,10,169,23,240,71,240,11,232,3,226,21,208,2,32,72,130,0,16,2,4,0,0
        .byte 0,0,0,0,7,192,7,224,0,96,0,96,3,224,3,192,3,0,3,0,0,0,3,0,3,0,0,0,0,0,0,0
        .byte 0,0,0,12,0,18,32,34,24,193,7,224,56,30,7,224,24,28,7,224,56,30,3,224,60,30,3,224,124,28,0,0
        .byte 0,0,1,128,1,128,1,0,7,192,23,240,11,224,11,224,11,64,11,160,11,64,11,160,11,224,11,224,23,240,0,0
        .byte 0,0,0,0,0,0,14,224,48,56,78,244,190,254,254,254,84,172,58,88,14,224,0,0,0,0,0,0,0,0,0,0
        .byte 28,0,122,0,118,0,227,0,227,0,161,128,1,128,0,192,0,192,0,96,0,96,0,48,0,48,0,24,0,26,0,12
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 127,254,128,1,143,241,143,241,128,49,128,49,128,49,131,241,131,241,131,1,131,1,128,1,131,1,131,1,128,1,127,254
        .byte 85,84,0,1,138,160,5,81,128,32,0,17,128,32,1,81,130,160,1,1,130,0,0,1,130,0,1,1,128,0,42,170
        .byte 68,68,0,1,138,160,0,16,0,0,0,17,128,0,0,16,0,160,1,1,128,0,0,0,0,0,1,1,128,0,34,34
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 19,144,79,224,48,20,10,168,73,32,211,144,236,96,84,80,55,216,24,60,14,107,28,246,30,240,62,56,50,216,44,224
        .byte 9,200,7,242,40,12,21,80,4,146,9,203,6,55,10,234,27,236,60,24,214,112,111,56,15,120,28,124,27,76,7,52
        .byte 1,128,1,128,3,192,3,192,3,192,7,224,7,224,23,224,14,240,14,116,30,120,29,184,61,188,59,220,59,220,29,184
        .byte 0,0,0,0,1,128,1,128,19,192,3,192,3,192,7,228,7,224,7,224,15,240,15,112,30,120,61,188,59,220,29,184
        .byte 0,0,16,0,0,0,0,0,1,4,1,128,1,128,3,192,3,192,3,192,7,224,7,224,14,240,30,120,61,188,29,184
        .byte 31,246,7,212,3,144,3,184,3,144,3,192,6,192,5,96,10,16,8,80,8,208,4,32,3,192,0,0,0,0,0,0
        .byte 1,0,1,16,1,56,1,144,3,128,67,128,2,196,5,64,6,192,12,32,8,32,8,160,4,72,3,128,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,3,192,12,48,16,8,34,68,34,68,96,6,112,14,252,63,223,251,175,213,127,254
        .byte 0,0,0,0,0,0,3,192,12,48,16,8,34,68,66,66,96,6,224,7,240,15,252,63,191,253,223,251,171,213,127,254
        .byte 0,0,3,192,12,48,16,8,38,100,66,66,64,2,224,7,224,7,176,13,252,63,191,253,223,251,175,245,117,174,31,248
        .byte 0,4,0,12,0,0,0,22,15,192,48,86,79,215,191,215,255,215,95,215,53,86,15,192,0,22,0,0,0,12,0,4
        .byte 0,4,0,12,0,0,0,22,15,192,48,86,79,214,191,214,255,214,95,214,53,86,15,192,0,22,0,0,0,12,0,4
        .byte 32,0,48,0,0,0,104,0,3,240,106,12,235,242,235,253,235,255,235,250,106,172,3,240,104,0,0,0,48,0,32,0
        .byte 32,0,48,0,0,0,104,0,3,240,106,12,107,242,107,253,107,255,107,250,106,172,3,240,104,0,0,0,48,0,32,0
frmlst:
        .byte 0,4
        .byte 4,4
        .byte 8,2
        .byte 10,4
        .byte 14,4
        .byte 18,7
        .byte 25,4
        .byte 29,2
        .byte 31,2
        .byte 33,6
        .byte 39,5
        .byte 44,2
        .byte 46,3
        .byte 49,2
        .byte 51,3
        .byte 54,2
        .byte 56,2,58,0
.out .sprintf("SPRITES SIZE = %d", (* - sprgfx))

scdat:
        .word 285,378,262,328,418,285,448,336,280,321,352,445,421,360,364,380,368,419,360,272,357,391,353
        .byte 53,59,59,53,255,52,19,53,54,59,59,54,255,81,19,54,53,57,58,53,255,0,18,81,51,54,255,0,11,71,72,71,255,0,7
        .byte 81,51,51,255,0,11,104,75,71,255,0,7,81,51,51,255,0,11,71,94,71,255,0,7,62,51,51,61,46,255,0,9,79
        .byte 71,95,255,0,7,62,51,51,16,14,255,0,19,62,51,51,10,4,255,0,12,45,255,61,4,46,0,62,51,51,0,4,255,0,10
        .byte 42,15,255,18,6,14,62,51,51,0,4,255,0,8,37,39,41,3,255,17,6,4,62,51,51,0,4,255,0,7,40,38,40
        .byte 39,5,255,6,6,7,62,51,51,0,4,255,0,5,255,1,4,70,70,255,1,8,62,51,51,0,13,11,255,0,5,22,23
        .byte 255,0,8,22,23,0,62,51,51,68,68,4,255,0,18,62,51,51,68,68,4,42,255,0,17,62,51,51,66,66,4,41,255,0,17
        .byte 62,51,51,20,20,4,45,46,255,0,16,62,51,51,20,20,4,16,16,14,255,0,15,62,51,51,19,19,4,21,21,4,255,0,5
        .byte 24,26,255,0,8,62,51,51,6,6,7,6,6,7,37,38,0,0,30,27,29,255,0,8,62,51,51,255,1,9,31,32,255,1,4
        .byte 255,0,5,1,1,51,53,255,52,22,53
        .byte 53,59,59,53,255,52,19,53,54,59,59,54,255,2,9,70,70,255,2,8,54,53,57,58,53,0,0,22,23,255,0,11,22
        .byte 23,0,62,51,54,255,0,21,62,51,51,45,61,46,255,0,18,62,51,51,15,16,14,255,0,18,62,51,51,3,10,4,255,0,18
        .byte 62,51,51,3,0,4,255,0,15,71,72,71,62,51,51,3,0,4,0,0,0,71,72,71,255,0,9,71,75,71,62,51,51
        .byte 3,21,4,0,0,0,71,75,71,0,0,62,64,63,255,0,4,107,106,104,62,51,51,3,21,45,61,46,0,105,106,107,0,0
        .byte 62,64,63,255,0,4,71,71,71,62,51,51,3,21,15,16,14,0,79,71,71,0,0,62,65,63,255,0,6,1,1,51,51
        .byte 3,21,3,0,4,0,0,0,73,72,73,62,64,63,45,46,255,0,6,51,51,3,19,3,0,4,0,0,0,73,74,73,62
        .byte 55,63,43,44,255,0,6,51,51,3,8,12,10,13,11,0,0,73,73,73,62,56,63,45,46,255,0,6,51,51,15,16,16
        .byte 14,0,4,0,0,73,73,73,62,56,63,24,25,25,26,255,0,4,51,51,3,9,66,4,20,4,0,76,0,0,0,62,55
        .byte 63,15,16,16,14,255,0,4,51,51,3,10,66,4,20,4,255,0,5,62,64,63,3,0,0,4,255,0,4,51,51,3,0
        .byte 66,4,20,4,255,0,4,42,62,65,63,3,69,69,4,42,0,0,0,51,51,3,19,19,4,20,4,0,0,0,37,38,62
        .byte 64,63,3,21,21,4,41,0,0,0,51,51,5,6,6,7,6,7,0,0,40,39,41,62,64,63,5,6,6,7,40,39,0,0
        .byte 51,51,255,1,22,51,53,255,52,22,53
        .byte 53,59,59,53,255,52,19,53,54,59,59,54,0,0,0,77,255,2,9,0,0,0,77,2,2,54,53,57,58,53,255,0,7
        .byte 22,23,255,0,8,62,64,51,54,65,63,255,0,18,62,65,51,51,64,63,0,93,255,0,14,76,0,62,64,51,51,56,63
        .byte 255,0,18,62,55,51,51,56,63,0,0,0,76,255,0,10,93,0,0,0,62,56,51,51,56,63,255,0,14,42,0,0,0
        .byte 62,55,51,51,64,63,45,46,42,255,0,10,78,41,40,39,0,62,64,51,51,65,63,43,44,41,0,15,255,16,8,14,24
        .byte 26,45,46,62,65,51,51,64,63,45,46,40,39,5,255,6,8,7,27,29,45,46,62,64,51,51,255,1,22,51,51,0,0
        .byte 22,23,255,0,14,22,23,0,0,51,51,255,0,7,73,72,73,255,0,12,51,51,255,0,7,73,74,73,255,0,12,51,51
        .byte 255,0,7,73,73,73,255,0,12,51,51,255,0,7,73,73,73,255,0,12,51,51,255,0,22,51,51,255,0,22,51,51,0,0,0
        .byte 24,26,255,0,17,51,51,0,0,30,27,29,255,0,14,78,0,0,51,51,255,1,22,51,53,255,52,22,53
        .byte 53,59,59,53,255,52,19,53,54,59,59,54,255,2,19,54,53,57,58,53,0,22,23,255,0,12,22,23,0,0,51,54,63
        .byte 255,0,21,51,51,63,255,0,15,71,72,71,0,0,0,51,51,63,255,0,15,71,75,71,0,0,0,51,51,63,255,0,15
        .byte 104,106,105,0,0,0,51,51,63,255,0,15,79,71,71,0,0,0,51,51,63,255,0,21,51,51,63,255,0,21,51,51,63
        .byte 0,0,0,71,72,71,255,0,4,15,14,255,0,9,51,51,1,0,0,0,71,75,71,255,0,4,3,4,78,255,0,6,76
        .byte 0,51,51,255,0,4,71,104,71,72,73,0,0,3,4,37,39,255,0,7,51,51,255,0,4,79,71,71,74,73,0,0,3
        .byte 4,0,45,61,46,255,0,5,51,51,255,0,6,73,73,45,61,61,3,4,0,15,16,14,0,93,0,0,0,51,51,255,0,6
        .byte 73,15,16,16,16,3,4,0,3,0,4,255,0,5,51,51,255,0,6,42,3,10,0,8,12,13,11,3,0,4,0,0,0
        .byte 76,0,51,51,255,0,6,41,3,0,8,12,9,0,13,11,0,4,255,0,5,51,51,255,0,5,40,38,3,8,12,10,4
        .byte 69,69,13,11,4,40,39,0,0,0,51,51,0,0,0,24,25,25,26,3,3,255,19,6,4,4,42,78,0,0,0,51,51
        .byte 0,0,30,27,28,28,29,5,5,255,6,6,7,7,41,37,38,0,0,51,51,255,1,22,51,53,255,52,22,53
        .byte 0,20,64,0,20,0,0,64,64,20,20,80,80,20,0,0,0,64,20,255,0,7,20,64,20,20,64,20,20,255,80,5,20,20
        .byte 0,0,55,20,255,0,4,20,20,0,20,20,0,20,20,80,71,80,71,80,71,80,80,20,0,64,20,255,0,6,20,80,20,20,20
        .byte 80,71,80,71,80,71,55,71,80,80,20,0,20,255,0,7,80,80,80,0,80,80,0,80,80,80,71,80,80,80,20,0,20
        .byte 255,0,6,80,0,0,0,255,80,4,0,0,0,80,80,80,20,64,20,255,0,7,255,80,14,20,0,20,255,0,10,81,90,90
        .byte 81,255,0,9,80,71,255,0,6,55,55,64,64,90,90,81,55,55,64,64,64,0,19,80,20,0,80,255,0,6,55,64,0
        .byte 64,81,19,0,55,64,0,64,64,0,80,80,80,0,80,255,0,6,255,64,4,0,80,0,255,64,5,0,255,80,4,19,255,0,10
        .byte 71,80,80,255,0,5,80,71,80,80,19,255,0,7,71,0,55,71,255,80,7,71,71,80,80,255,0,9,80,0,255,80,4
        .byte 0,255,80,8,255,0,9,19,80,255,0,4,19,255,80,8,0,55,64,255,0,7,80,80,80,19,19,255,80,4,0,80,80,80
        .byte 0,80,0,55,55,64,255,0,5,80,255,0,8,255,80,4,0,80,45,61,46,55,255,0,5,80,0,255,55,5,0,255,80,4
        .byte 19,0,80,15,16,14,55,64,255,0,4,19,80,255,0,5,255,80,4,19,0,19,80,3,66,4,55,55,255,0,5,80,71,71
        .byte 255,80,5,19,0,0,19,80,80,3,66,4,55,55,42,255,0,4,19,255,80,4,19,19,0,0,19,19,19,80,8,9,66
        .byte 13,11,55,41,0,0,0,55,255,0,7,19,19,19,24,25,26,3,21,21,21,4,40,38,0,0,64,55,55,255,64,4,255,0,4
        .byte 30,27,28,29,5,6,6,6,7,37,39
        .byte 53,59,59,53,255,52,19,53,54,59,59,54,2,2,33,34,255,2,15,54,53,57,58,53,0,0,35,36,0,0,45,255,61,4
        .byte 46,0,0,22,23,0,0,0,51,54,255,0,5,35,36,0,0,15,255,16,4,14,255,0,7,51,51,255,0,5,35,36,0,0
        .byte 3,9,0,0,0,4,255,0,7,51,51,255,0,5,35,36,49,50,3,10,68,68,68,4,255,0,7,51,51,255,0,5,35
        .byte 36,47,48,3,255,68,4,4,255,0,7,51,51,255,0,5,35,36,43,44,3,255,20,4,4,0,0,71,72,71,0,0,51,51
        .byte 255,0,5,35,36,43,44,3,255,19,4,4,0,0,71,75,71,0,0,51,51,0,0,0,76,0,35,36,43,44,3,255,21,4
        .byte 4,83,69,105,106,107,0,0,51,51,0,0,76,93,76,35,36,45,46,5,255,6,4,7,69,69,79,71,71,0,0,51,51
        .byte 255,1,16,255,0,6,51,51,0,22,23,255,0,10,22,23,255,0,7,51,51,255,0,22,51,51,255,0,22,51,51,255,0,22
        .byte 51,51,255,0,22,51,51,255,0,22,51,51,255,0,22,51,51,0,0,24,26,255,0,18,51,51,0,0,27,29,255,0,13
        .byte 37,38,0,0,0,51,51,255,1,22,51,53,255,52,22,53
        .byte 53,59,59,53,255,52,19,53,54,59,59,54,2,2,33,34,255,2,10,33,34,2,2,2,54,53,57,58,53,0,0,35,36
        .byte 22,23,0,0,0,45,46,0,0,0,35,36,22,23,0,51,54,255,0,5,35,36,255,0,4,43,44,255,0,4,35,36,0,0
        .byte 45,51,51,255,0,5,35,36,255,0,4,43,44,255,0,4,35,36,0,0,15,51,51,255,0,5,35,36,255,0,4,45,46
        .byte 255,0,4,35,36,0,0,3,51,51,255,0,11,62,64,63,0,0,0,35,36,0,0,3,51,51,255,0,10,42,62,64,63
        .byte 0,0,0,35,36,0,0,3,51,51,0,0,71,72,71,255,0,4,78,41,62,64,63,0,0,0,35,36,0,0,3,51,51
        .byte 0,0,71,75,71,0,0,0,78,37,38,62,65,63,0,0,0,35,36,0,0,3,51,51,0,0,71,104,71,0,0,24,25
        .byte 26,0,62,64,63,0,0,0,35,36,0,0,5,51,51,0,0,71,71,71,0,0,15,16,14,0,62,55,63,1,1,1,31
        .byte 32,1,1,1,51,51,255,0,6,78,3,0,4,0,62,56,63,0,22,23,35,36,22,23,0,51,51,0,0,0,93,0,255,1,6
        .byte 62,56,63,0,0,0,35,36,0,15,16,51,51,255,0,5,2,0,3,0,4,2,62,56,63,0,0,0,35,36,0,3,10
        .byte 51,51,0,76,0,0,0,2,0,3,21,4,2,62,55,63,0,0,0,35,36,0,3,0,51,51,255,0,5,2,0,3,21
        .byte 4,2,62,64,63,0,0,0,35,36,0,3,0,51,51,255,0,4,42,2,0,3,21,4,2,62,65,63,78,0,0,35,36
        .byte 0,3,20,51,51,255,0,4,41,2,0,3,19,4,2,62,64,63,42,0,0,35,36,0,3,20,51,51,0,0,0,45,46
        .byte 2,0,5,6,7,2,62,64,63,41,40,39,0,0,0,3,20,51,51,0,0,0,45,46,255,1,6,62,64,63,37,38,78
        .byte 0,0,67,5,6,51,51,255,1,22,51,53,255,52,22,53
        .byte 53,59,59,53,255,52,19,53,54,59,59,54,2,33,34,255,2,11,33,34,2,2,2,54,53,57,58,53,0,35,36,255,0,4
        .byte 22,23,255,0,5,35,36,24,26,0,51,54,65,63,0,0,35,36,255,0,11,35,36,15,14,0,51,51,64,63,0,0,35
        .byte 36,255,0,11,35,36,3,4,0,51,51,55,63,0,0,35,36,255,0,11,35,36,3,4,0,51,51,56,63,78,0,35,36
        .byte 255,0,9,255,1,7,51,51,56,63,78,78,35,36,255,0,9,2,0,0,0,3,4,2,51,51,55,63,45,61,61,46,255,0,7
        .byte 76,0,2,0,0,0,3,4,2,51,51,64,63,43,56,60,44,255,0,9,2,0,0,0,3,4,2,51,51,65,63,43,60,60
        .byte 44,42,255,0,4,93,0,0,0,2,0,0,42,3,4,2,51,51,64,63,45,61,61,46,41,255,0,7,78,2,0,30,41
        .byte 5,7,2,51,51,255,1,16,31,32,255,1,4,51,51,0,0,22,23,255,0,12,35,36,0,0,22,23,51,51,255,0,16
        .byte 35,36,0,0,45,61,51,51,255,0,16,35,36,0,0,15,9,51,51,255,0,16,35,36,0,0,3,10,51,51,255,0,14
        .byte 96,97,255,0,4,3,0,51,51,255,0,14,98,99,255,0,4,3,0,51,51,255,0,7,24,25,26,255,0,8,40,39,3
        .byte 20,51,51,255,0,6,30,27,28,29,255,0,6,40,38,37,38,5,6,51,51,255,1,5,31,32,255,1,15,51,53,255,52,22
        .byte 53
        .byte 53,59,59,53,255,52,19,53,54,59,59,54,0,0,35,36,255,0,6,255,2,9,54,53,57,58,53,0,0,35,36,255,0,9
        .byte 22,23,0,0,0,62,51,54,0,0,4,0,0,35,36,255,0,14,62,51,51,0,0,4,255,0,18,62,51,51,68,68,13
        .byte 11,255,0,17,62,51,51,68,68,68,13,11,255,0,16,62,51,51,255,20,4,4,255,0,16,62,51,51,255,20,4,4,78
        .byte 255,0,15,62,51,51,255,19,4,4,40,39,42,255,0,13,62,51,51,255,6,4,7,37,38,41,255,0,7,73,72,73,0,0,0
        .byte 62,51,51,1,1,1,70,70,255,1,4,0,0,0,93,0,0,73,74,73,0,0,0,62,51,51,255,0,5,22,23,255,0,8
        .byte 73,73,73,0,0,1,1,51,51,255,0,15,73,73,71,72,71,0,0,51,51,255,0,17,71,75,71,0,0,51,51,255,0,16
        .byte 76,104,105,107,0,0,51,51,255,0,17,79,71,71,0,0,51,51,255,0,10,45,61,46,255,0,9,51,51,255,0,10,43
        .byte 60,44,255,0,9,51,51,15,255,0,8,14,45,61,61,46,255,0,8,51,51,5,255,6,8,7,43,60,60,44,255,0,8
        .byte 51,51,255,1,22,51,53,255,52,22,53
        .byte 53,59,59,53,255,52,19,53,54,59,59,54,255,0,5,255,2,4,255,0,4,35,36,0,0,3,0,54,53,57,58,53,255,0,6
        .byte 22,23,255,0,5,35,36,0,0,3,0,51,54,255,0,16,35,36,0,0,3,0,51,51,255,0,16,35,36,0,0,3,0
        .byte 51,51,255,0,19,8,9,0,51,51,255,0,18,8,12,10,0,51,51,255,0,12,45,61,46,0,0,0,3,20,20,20,51,51
        .byte 255,0,11,78,43,60,44,0,0,0,3,20,20,20,51,51,42,255,0,8,62,64,63,43,60,44,24,25,26,3,19,19,19
        .byte 51,51,41,255,84,7,0,62,64,63,45,61,46,27,28,29,5,6,6,6,51,51,255,1,8,0,62,65,63,0,255,1,9
        .byte 51,51,255,0,5,22,23,0,0,62,64,63,0,0,22,23,255,0,6,51,51,255,0,9,62,55,63,255,0,10,51,51,255,0,9
        .byte 62,56,63,49,50,255,0,8,51,51,255,0,9,62,56,63,47,48,255,0,8,51,51,255,0,9,62,55,63,47,48,255,0,8
        .byte 51,51,255,0,9,62,64,63,43,44,255,0,8,51,51,255,0,8,84,62,65,63,43,44,255,0,8,51,51,255,0,8,42
        .byte 62,64,63,43,44,255,0,6,24,26,51,51,255,0,7,84,41,62,64,63,45,46,255,0,5,30,27,29,51,51,1,1,1
        .byte 0,0,0,255,1,10,0,0,0,1,1,1,51,53,255,52,22,53
        .byte 53,59,59,53,255,52,19,53,54,59,59,54,255,0,7,35,36,62,64,63,2,2,70,70,2,2,2,54,53,57,58,53,255,0,7
        .byte 35,36,62,55,63,255,0,7,51,54,26,255,0,9,35,36,62,56,63,255,0,7,51,51,14,255,0,9,35,36,62,56,63
        .byte 255,0,7,51,51,4,255,0,9,93,0,62,55,63,255,0,7,51,51,4,255,0,6,93,255,0,4,62,64,63,255,0,7
        .byte 51,51,4,0,0,76,255,0,8,62,64,63,255,0,7,51,51,4,255,0,11,62,65,63,255,0,7,51,51,13,11,255,0,4
        .byte 73,72,73,0,0,0,62,64,63,0,67,0,0,0,45,46,51,51,10,13,11,0,0,0,73,73,73,0,0,0,62,64,63
        .byte 67,67,0,0,0,43,44,51,51,0,0,4,0,0,0,73,73,73,0,0,255,1,11,51,51,56,55,4,0,0,0,73,73,73
        .byte 255,0,5,22,23,255,0,6,51,51,55,55,4,42,255,0,5,71,72,71,255,0,10,51,51,68,68,4,41,40,38,0,0,0
        .byte 71,75,71,255,0,10,51,51,68,68,4,24,25,25,26,0,0,71,107,106,255,0,10,51,51,66,66,4,15,16,16,14,49
        .byte 50,79,71,71,255,0,10,51,51,20,20,4,3,9,0,4,47,48,255,0,13,51,51,20,20,4,3,69,69,4,43,44,78
        .byte 255,0,12,51,51,19,19,4,3,21,21,4,43,24,25,26,42,255,0,10,51,51,6,6,7,5,6,6,7,45,27,28,29
        .byte 41,37,39,255,0,8,51,51,255,1,22,51,53,255,52,22,53
        .byte 53,59,59,53,255,52,19,53,54,59,59,54,2,2,33,34,255,2,10,33,34,2,2,2,54,53,57,58,53,16,14,35,36
        .byte 0,0,0,22,23,255,0,5,35,36,22,23,0,51,54,0,0,3,0,4,35,36,255,0,5,62,82,0,0,0,35,36,0,0
        .byte 45,51,51,0,0,3,0,4,35,36,255,0,5,62,82,0,0,0,35,36,0,0,15,51,51,0,0,3,0,4,35,36,255,0,5
        .byte 62,82,0,0,0,35,36,0,0,3,51,51,0,0,3,0,4,35,36,255,0,5,62,82,0,0,0,35,36,0,0,3,51,51
        .byte 0,0,3,0,4,35,36,255,0,5,62,82,0,0,0,35,36,0,0,3,51,51,0,0,3,0,4,35,36,255,0,5,62
        .byte 82,0,0,0,35,36,0,0,3,51,51,0,8,9,0,4,255,0,7,62,82,255,0,7,3,51,51,8,12,10,0,4,255,0,7
        .byte 62,82,255,0,6,76,3,51,51,3,0,0,0,4,0,0,76,255,0,4,62,82,0,73,72,73,0,0,0,3,51,51,3
        .byte 0,0,0,4,255,0,5,93,0,62,82,0,73,73,73,0,0,0,3,51,51,3,55,56,55,4,255,0,7,62,82,0,73,73
        .byte 71,72,71,0,3,51,51,46,68,68,68,4,0,76,255,0,5,62,82,0,73,73,95,75,71,0,3,51,51,14,68,68,68
        .byte 4,255,0,6,93,62,82,0,0,0,71,105,106,93,3,51,51,4,20,20,20,4,255,0,7,62,82,0,0,76,79,71,71
        .byte 0,3,51,51,4,20,20,20,4,255,0,6,78,62,82,255,0,7,3,51,51,4,20,20,20,4,255,0,5,45,46,62,82
        .byte 40,39,255,0,5,3,51,51,4,19,19,19,4,0,0,24,25,26,43,44,62,82,78,42,42,0,0,0,37,39,51,51,7
        .byte 6,6,6,7,0,30,27,28,29,45,46,62,82,78,78,41,0,30,37,38,78,51,51,255,1,5,31,32,255,1,10,31,32
        .byte 1,1,1,51,53,255,52,22,53
        .byte 53,59,59,53,255,52,19,53,54,59,59,54,0,35,36,255,0,6,62,64,63,0,35,36,0,0,0,62,54,53,57,58,53
        .byte 0,35,36,255,0,6,62,55,63,0,35,36,0,0,0,62,51,54,45,61,46,0,35,36,255,0,6,62,56,63,0,35,36
        .byte 0,0,0,62,51,51,43,55,44,0,35,36,255,0,4,49,50,62,56,63,42,35,36,0,0,0,62,51,51,43,60,44,0
        .byte 35,36,255,0,4,47,48,62,55,63,41,35,36,0,0,0,62,51,51,45,61,46,0,35,36,255,0,4,47,48,62,64,63
        .byte 40,39,255,0,4,62,51,51,0,24,25,26,255,0,6,43,44,62,65,63,45,61,46,0,0,0,62,51,51,30,27,28,29
        .byte 255,0,6,43,44,62,64,63,43,60,44,0,67,67,62,51,51,45,61,46,0,76,255,0,4,78,43,44,62,64,63,43,60
        .byte 44,42,67,67,62,51,51,15,16,14,255,0,5,37,38,45,46,62,64,63,45,61,46,41,37,39,62,51,51,3,0,4,255,0,4
        .byte 255,1,15,51,51,3,0,4,0,71,72,71,255,0,7,22,23,255,0,6,51,51,3,0,4,0,71,75,71,255,0,12,24
        .byte 25,25,51,51,3,20,4,93,71,107,71,255,0,12,15,16,16,51,51,3,20,4,0,79,71,104,255,0,5,42,255,0,6
        .byte 3,9,0,51,51,3,19,4,93,255,0,7,78,41,78,255,0,5,3,10,0,51,51,3,19,4,255,0,8,40,39,37,38
        .byte 255,0,4,3,0,0,51,51,3,21,4,76,255,0,7,42,37,38,40,39,0,0,0,3,83,83,51,51,3,21,4,255,0,8
        .byte 41,40,39,42,255,0,4,3,19,19,51,51,5,6,7,0,0,0,30,0,37,38,78,40,38,78,41,37,39,0,30,5,6,6
        .byte 51,51,255,1,5,31,32,255,1,10,31,32,1,1,1,51,53,255,52,22,53
        .byte 53,59,59,53,255,52,19,53,54,59,59,54,255,2,11,255,0,5,35,36,81,54,53,57,58,53,0,0,0,22,23,0,0,0
        .byte 45,61,46,255,0,5,35,36,81,51,54,255,0,9,15,16,16,14,255,0,6,35,36,81,51,51,255,0,9,3,9,0,4
        .byte 0,0,0,93,0,0,35,36,81,51,51,255,0,7,67,59,3,10,69,4,255,0,6,35,36,81,51,51,255,0,7,59,59
        .byte 3,69,69,4,255,0,6,35,36,81,51,51,255,0,7,37,39,3,21,21,4,255,0,6,35,36,81,51,51,255,0,4,83
        .byte 69,24,25,26,3,21,21,4,255,0,6,35,36,81,51,51,255,0,4,69,69,27,28,29,5,6,6,7,255,0,6,35,36
        .byte 81,51,51,255,1,14,255,0,5,35,36,81,51,51,0,22,23,255,0,7,22,23,255,0,7,35,36,81,51,51,255,0,11
        .byte 24,25,26,255,0,5,35,36,81,51,51,255,0,11,15,16,14,255,0,7,81,51,51,255,0,11,3,0,4,0,71,72,71
        .byte 0,0,0,81,51,51,255,0,11,3,0,4,0,71,75,71,0,0,0,81,51,51,255,0,10,8,9,0,13,11,71,104,71
        .byte 0,0,0,81,51,51,255,0,10,3,10,68,68,4,71,71,71,76,0,0,81,51,51,255,0,9,42,3,20,20,20,4,255,0,6
        .byte 81,51,51,255,0,9,41,3,19,19,19,4,78,255,0,5,81,51,51,255,0,5,30,0,0,40,39,5,6,6,6,7,40
        .byte 38,0,30,0,0,81,51,51,255,1,4,31,32,255,1,11,31,32,1,1,1,51,53,255,52,22,53
        .byte 53,59,59,53,255,52,19,53,54,59,59,54,255,0,9,70,70,255,2,8,54,53,57,58,53,255,0,12,22,23,255,0,5
        .byte 51,54,62,64,63,255,0,19,51,51,62,55,63,37,39,255,0,17,51,51,62,56,63,78,78,37,38,255,0,15,51,51,62
        .byte 55,63,45,61,61,61,46,255,0,14,51,51,62,64,63,43,55,56,55,44,255,0,14,51,51,62,65,63,43,60,60,60,44
        .byte 42,255,0,13,51,51,62,64,63,45,61,61,61,46,41,40,39,255,0,11,51,51,255,1,5,31,255,1,6,255,0,10,51,51
        .byte 0,22,23,0,0,92,0,0,0,22,23,0,15,14,0,0,0,73,72,73,0,0,51,51,255,0,5,92,255,0,6,3,4
        .byte 0,93,0,73,74,73,0,0,51,51,255,0,5,35,36,255,0,5,3,4,0,0,71,72,71,73,0,0,51,51,255,0,5
        .byte 35,36,255,0,5,3,4,0,0,71,75,71,73,0,0,51,51,255,0,5,35,36,255,0,4,8,9,13,11,0,105,106,107
        .byte 0,0,0,51,51,255,0,5,35,36,0,0,0,8,12,10,0,13,11,79,71,104,0,0,0,51,51,255,0,5,35,36,0,0,0
        .byte 3,255,83,4,4,255,0,6,51,51,25,25,25,26,0,35,36,0,0,0,3,255,19,4,4,45,46,255,0,4,51,51,16,16
        .byte 14,0,0,35,36,0,0,0,3,255,19,4,4,43,44,42,0,0,0,51,51,0,0,4,0,0,35,36,0,0,0,5,255,6,4
        .byte 7,45,46,41,78,0,0,51,51,0,0,4,0,0,35,36,255,0,6,255,1,9,51,53,255,52,22,53
        .byte 53,59,59,53,255,52,19,53,54,59,59,54,2,2,2,33,34,255,2,14,54,53,57,58,53,0,22,23,35,36,0,45,46
        .byte 255,0,8,22,23,0,51,54,255,0,6,35,36,0,15,14,255,0,9,49,50,51,51,255,0,6,35,36,0,3,4,255,0,9
        .byte 47,48,51,51,255,0,6,35,36,0,3,4,255,0,9,47,48,51,51,255,0,6,35,36,0,3,4,255,0,9,43,44,51,51
        .byte 255,0,6,35,36,8,9,13,11,255,0,6,83,69,43,44,51,51,255,0,6,35,36,3,69,69,4,255,0,6,24,26,43
        .byte 44,51,51,255,0,5,24,25,26,3,21,21,4,255,0,5,30,27,29,45,46,51,51,255,0,4,30,27,28,29,5,6,6
        .byte 7,0,0,0,1,31,32,255,1,4,51,51,1,1,1,31,255,1,8,255,0,4,35,36,255,0,4,51,51,0,0,0,92
        .byte 0,0,22,23,0,0,81,255,0,5,35,36,255,0,4,51,51,2,0,0,92,255,0,5,2,81,255,0,5,35,36,255,0,4
        .byte 51,51,2,255,0,8,2,2,255,0,5,35,36,255,0,4,51,51,2,255,0,8,2,2,255,0,5,35,36,255,0,4,51,51
        .byte 2,255,0,8,2,2,255,0,5,35,36,255,0,4,51,51,2,255,0,8,2,2,255,0,5,35,36,255,0,4,51,51,2
        .byte 67,67,255,0,6,2,2,255,0,5,35,36,0,24,25,25,51,51,2,67,67,255,0,6,2,2,255,0,5,35,36,0,0
        .byte 15,16,51,51,2,37,38,255,0,6,2,2,255,0,5,35,36,0,0,3,0,51,51,1,1,1,255,0,6,2,2,255,0,5
        .byte 35,36,0,0,3,0,51,53,255,52,22,53
        .byte 53,59,59,53,255,52,19,53,54,59,59,54,255,0,4,33,34,255,2,6,70,70,255,2,5,54,53,57,58,53,255,0,4
        .byte 35,36,0,0,22,2,23,255,0,8,51,54,78,255,0,6,35,36,0,0,0,2,255,0,9,51,51,37,38,255,0,5,35
        .byte 36,0,0,0,2,255,0,9,51,51,25,26,255,0,5,35,36,49,50,78,2,255,0,9,51,51,16,14,255,0,5,35,36
        .byte 47,48,45,2,255,0,9,51,51,0,4,255,0,5,35,36,47,48,43,2,255,0,9,51,51,0,4,255,0,5,35,36,43
        .byte 44,43,2,255,0,9,51,51,0,4,255,0,5,35,36,43,44,43,2,255,0,9,51,51,0,4,255,0,5,35,36,45,46
        .byte 43,2,255,0,9,51,51,0,4,255,0,5,35,36,255,1,4,255,0,7,1,1,51,51,21,4,255,0,4,73,72,73,0
        .byte 22,23,255,0,5,24,25,25,26,2,51,51,21,4,76,0,0,0,73,74,73,255,0,4,93,0,0,0,15,16,16,14,2
        .byte 51,51,21,4,255,0,4,73,73,73,255,0,8,3,68,68,4,2,51,51,21,4,0,93,0,0,73,73,73,255,0,5,76
        .byte 0,0,3,68,68,4,2,51,51,21,4,255,0,15,3,20,20,4,2,51,51,19,4,93,255,0,12,45,61,61,61,46,4
        .byte 2,51,51,19,4,255,0,13,43,56,55,60,44,4,2,51,51,19,4,0,76,255,0,8,24,25,26,43,60,60,60,44,4
        .byte 2,51,51,6,7,255,0,9,30,27,28,29,45,61,61,61,46,7,2,51,51,255,1,4,255,0,6,31,32,255,1,10,51
        .byte 53,255,52,22,53
        .byte 53,59,59,53,255,52,19,53,54,59,59,54,255,2,6,33,34,255,2,4,255,0,5,2,2,54,53,57,58,53,23,255,0,5
        .byte 35,36,22,23,255,0,9,51,54,62,82,255,0,7,35,36,62,64,63,255,0,8,51,51,62,82,255,0,5,93,0,35,36
        .byte 62,64,63,0,71,72,71,255,0,4,51,51,62,82,255,0,7,35,36,62,64,63,76,71,75,95,255,0,4,51,51,62,82
        .byte 255,0,7,35,36,62,64,63,0,71,104,71,255,0,4,51,51,62,82,255,0,7,35,36,62,64,63,93,79,71,71,255,0,4
        .byte 51,51,62,82,255,0,7,35,36,62,65,63,255,0,8,51,51,62,82,255,0,7,35,36,62,64,63,76,255,0,7,51,51
        .byte 62,82,255,0,7,35,36,62,55,63,0,0,0,85,86,0,1,1,51,51,62,82,255,0,7,35,36,62,56,63,0,93,0
        .byte 87,88,0,0,2,51,51,62,82,255,0,7,35,36,62,56,63,0,0,0,87,88,0,0,2,51,51,62,82,255,0,4,71
        .byte 72,71,93,0,62,55,63,0,0,0,87,88,0,0,2,51,51,62,82,255,0,4,71,75,71,0,0,62,64,63,0,0,0
        .byte 87,88,0,0,2,51,51,62,82,0,76,0,0,71,71,71,0,0,62,65,63,0,0,0,87,88,0,0,2,51,51,62,82
        .byte 255,0,4,79,71,71,0,0,62,64,63,0,0,0,87,88,0,0,2,51,51,62,82,255,0,4,76,255,0,4,62,64,63
        .byte 0,0,0,87,88,0,42,2,51,51,62,82,255,0,8,84,62,64,63,0,0,0,87,88,0,41,2,51,51,62,82,255,0,7
        .byte 24,25,26,64,63,0,84,84,85,86,40,39,2,51,51,62,82,84,255,0,5,30,27,28,29,64,63,84,85,255,89,4,86
        .byte 2,51,51,1,1,1,255,0,4,31,32,255,1,13,51,53,255,52,22,53
        .byte 53,59,59,53,255,52,19,53,54,59,59,54,255,2,7,70,70,255,2,10,54,53,57,58,53,0,22,23,255,0,12,22,23
        .byte 0,0,51,54,255,0,22,51,51,255,0,9,255,59,4,255,0,9,51,51,255,0,9,255,59,4,255,0,9,51,51,255,0,8
        .byte 85,255,89,4,86,255,0,8,51,51,255,0,8,87,255,0,4,88,42,255,0,7,51,51,255,0,6,93,0,87,255,0,4
        .byte 88,41,255,0,7,51,51,255,0,8,87,255,0,4,88,37,38,255,0,6,51,51,1,1,1,85,86,255,1,5,85,86,255,1,5
        .byte 85,86,1,1,1,51,51,2,23,0,87,88,255,0,5,87,88,255,0,5,87,88,0,22,23,51,51,2,0,0,87,88,255,0,5
        .byte 87,88,255,0,5,87,88,0,0,0,51,51,2,0,0,87,88,255,0,5,87,88,255,0,5,87,88,0,0,0,51,51,2
        .byte 0,0,87,88,255,0,5,87,88,255,0,5,87,88,0,0,0,51,51,2,0,0,87,88,255,0,5,87,88,255,0,5,87
        .byte 88,0,0,0,51,51,2,0,0,87,88,255,0,5,87,88,255,0,5,87,88,0,0,0,51,51,2,0,0,87,88,255,0,5
        .byte 87,88,255,0,5,87,88,0,0,0,51,51,2,0,0,87,88,255,0,5,87,88,255,0,5,87,88,0,0,0,51,51,2
        .byte 0,0,85,86,255,0,5,85,86,255,0,5,85,86,0,0,0,51,51,2,0,85,89,89,86,0,0,0,85,89,89,86,0,0,0
        .byte 85,89,89,86,0,0,51,51,255,1,6,31,32,255,1,14,51,53,255,52,22,53
        .byte 53,59,59,53,255,52,19,53,54,59,59,54,255,2,19,54,53,57,58,53,0,22,23,255,0,11,22,23,0,0,0,51,54
        .byte 255,0,21,37,51,51,255,0,21,62,51,51,255,0,10,15,14,255,0,9,62,51,51,255,0,9,8,9,13,11,255,0,8
        .byte 62,51,51,255,0,9,3,10,0,4,255,0,8,62,51,51,255,0,8,15,16,14,19,4,24,26,255,0,6,62,51,51,255,0,8
        .byte 5,6,7,6,7,27,29,255,0,6,62,51,51,255,1,16,255,0,5,62,51,51,255,0,12,22,2,2,0,0,0,76,0,0
        .byte 62,51,51,255,0,13,2,2,255,0,6,62,51,51,255,0,13,2,2,255,0,4,93,0,62,51,51,255,0,13,2,2,255,0,6
        .byte 62,51,51,255,0,13,2,2,0,0,73,72,73,0,62,51,51,255,0,13,2,2,0,0,73,74,73,0,62,51,51,255,0,13
        .byte 2,2,0,0,73,73,73,45,46,51,51,255,0,13,2,2,0,0,73,73,73,43,44,51,51,255,0,13,2,2,67,59,0,0
        .byte 42,43,44,51,51,255,0,13,2,2,59,59,40,39,41,45,46,51,51,255,1,4,255,0,8,255,1,10,51,53,255,52,22
        .byte 53
        .byte 53,59,59,53,255,52,19,53,54,59,59,54,2,2,33,34,255,2,15,54,53,57,58,53,0,0,35,36,255,0,6,22,23
        .byte 255,0,4,22,23,0,51,54,39,255,0,4,35,36,255,0,6,24,26,255,0,7,51,51,64,63,0,0,0,35,36,255,0,6
        .byte 27,29,255,0,7,51,51,64,63,0,0,0,35,36,255,0,5,45,61,61,46,255,0,6,51,51,65,63,0,0,0,35,36
        .byte 255,0,5,45,61,61,46,255,0,6,51,51,64,63,0,0,0,35,36,255,0,5,43,55,55,44,255,0,5,78,51,51,64
        .byte 63,255,0,10,43,55,55,44,0,0,0,15,16,14,51,51,55,63,255,0,6,76,0,0,0,43,71,71,44,83,69,0,3
        .byte 20,4,51,51,56,63,255,0,10,45,61,61,46,69,69,40,38,6,7,51,51,56,63,255,0,9,255,1,11,51,51,56,63
        .byte 0,0,0,71,72,71,255,0,5,22,23,255,0,4,92,0,81,51,51,56,63,0,0,0,95,75,71,255,0,11,35,36,81
        .byte 51,51,55,63,42,0,0,105,106,107,255,0,11,35,36,81,51,51,64,63,41,0,0,79,104,71,255,0,11,35,36,81,51,51
        .byte 64,63,40,39,255,0,15,35,36,81,51,51,65,24,25,25,25,26,255,0,13,35,36,81,51,51,64,15,16,16,16,14,42
        .byte 255,0,12,35,36,81,51,51,64,3,19,19,19,4,41,255,0,12,35,36,81,51,51,64,5,6,6,6,7,37,38,0,0,0
        .byte 84,255,0,7,35,36,81,51,51,255,1,15,255,0,4,35,36,81,51,53,255,52,22,53
        .byte 53,59,59,53,255,52,19,53,54,59,59,54,255,2,19,54,53,57,58,53,255,0,18,45,51,54,255,0,21,15,51,51,255,0,21
        .byte 3,51,51,255,0,12,45,61,61,46,255,0,5,3,51,51,255,0,12,43,60,60,44,0,76,0,0,0,3,51,51,45,61
        .byte 46,255,0,4,71,72,71,0,0,43,55,55,44,255,0,5,3,51,51,43,71,44,255,0,4,71,75,71,0,0,43,55,55
        .byte 44,255,0,4,42,3,51,51,43,71,44,255,0,4,71,95,71,72,73,43,60,60,44,255,0,4,41,3,51,51,45,61,46
        .byte 0,0,30,0,79,71,71,74,73,45,61,61,46,0,30,0,37,38,5,51,51,255,1,4,31,32,1,0,0,73,73,73,255,1,4
        .byte 31,32,255,1,4,51,51,0,0,22,23,35,36,0,0,0,73,73,73,0,22,23,0,35,36,0,0,22,23,51,51,0,24
        .byte 26,0,35,36,255,0,7,40,39,0,35,36,0,0,0,81,51,51,0,15,14,0,35,36,255,0,6,45,46,37,38,35,36
        .byte 0,0,0,62,51,51,0,3,4,0,35,36,255,0,6,45,61,46,0,35,36,0,0,0,62,51,51,0,3,4,0,35,36
        .byte 255,0,7,45,61,46,35,36,0,0,0,62,51,51,8,9,13,11,35,36,255,0,6,62,64,63,0,35,36,0,0,0,62
        .byte 51,51,3,10,0,4,35,36,255,0,6,62,64,63,0,35,36,0,0,0,62,51,51,3,69,69,4,35,36,255,0,6,62
        .byte 64,63,0,35,36,0,0,0,62,51,51,3,21,21,4,35,36,255,0,6,62,65,63,0,35,36,0,0,0,62,51,51,5
        .byte 6,6,7,35,36,255,0,6,62,64,63,0,35,36,0,0,0,62,51,53,255,52,22,53
        .byte 53,59,59,53,255,52,19,53,54,59,59,54,255,0,13,255,81,6,54,53,57,58,53,255,0,18,81,51,54,24,25,26,255,0,18
        .byte 81,51,51,15,16,14,255,0,17,67,81,51,51,3,9,4,255,0,17,67,81,51,51,3,10,4,255,0,17,62,82,51,51
        .byte 3,0,4,255,0,17,62,82,51,51,3,21,4,255,0,16,76,62,82,51,51,3,19,4,255,0,17,62,82,51,51,3,20
        .byte 4,255,0,6,45,46,0,45,46,0,0,0,93,0,0,62,82,51,51,5,6,7,0,30,0,15,14,0,15,14,0,15,14
        .byte 255,0,6,62,82,51,51,1,1,1,33,34,1,3,4,1,3,4,1,3,4,1,1,255,0,4,62,82,51,51,22,2,0
        .byte 35,36,0,3,4,0,3,4,0,3,4,2,23,255,0,4,62,82,51,51,0,2,0,35,36,0,3,4,0,3,4,0,3
        .byte 4,2,255,0,4,93,62,82,51,51,0,2,0,35,36,0,3,4,0,3,4,0,3,4,2,255,0,5,62,82,51,51,0
        .byte 2,0,35,36,0,3,4,0,3,4,0,3,4,2,0,76,0,0,0,62,82,51,51,0,2,0,35,36,0,3,4,0,3
        .byte 4,0,3,4,2,255,0,5,62,82,51,51,0,2,0,35,36,0,5,7,0,5,7,0,5,7,2,0,0,0,37,38,62
        .byte 82,51,51,0,2,0,35,36,255,0,9,2,67,67,0,78,42,62,82,51,51,0,2,255,0,4,30,255,0,7,2,67,67
        .byte 40,39,41,62,82,51,51,255,1,5,31,32,255,1,15,51,53,255,52,22,53
numsc:
        .byte 23
.out .sprintf("SCREENS SIZE = %d", (* - scdat))

nmedat:
        .byte 0,0,152,72,8,9,0,8,2,5,120,8,8,10,64,88,8,6,72,120,8,6,72,144,3,7,16,40,7,16,128,112,5,13,104,96,255
        .byte 0,0,152,56,2,6,144,128,3,7,72,136,8,9,0,8,5,13,16,104,255
        .byte 0,0,152,8,2,6,64,88,1,3,152,128,1,4,152,48,4,8,64,64,2,7,120,96,8,9,0,8,255
        .byte 0,0,152,8,2,6,144,104,8,9,0,8,3,7,56,96,3,7,16,64,255
        .byte 255,0,0,152,8,8,9,0,8,2,6,64,96,2,6,48,88,2,5,40,104,1,14,152,40,1,4,152,128,7,7,120,88,255
        .byte 0,0,152,16,8,9,0,8,255
        .byte 0,0,152,16,8,9,0,8,3,7,16,72,1,4,80,64,8,10,136,72,255
        .byte 0,1,152,160,8,9,0,8,2,6,56,8,2,5,40,8,3,7,120,88,3,7,8,88,3,8,80,160,5,13,96,32,255
        .byte 0,0,136,168,8,9,0,8,3,7,96,136,7,15,64,64,3,7,96,32,2,6,56,168,255
        .byte 0,1,152,160,8,9,0,8,3,7,96,40,2,6,120,8,2,6,144,40,1,14,152,120,5,13,16,144,255
        .byte 0,0,152,48,8,9,0,8,3,7,16,72,3,7,104,72,3,8,72,128,2,5,112,16,255
        .byte 0,0,152,56,8,9,0,8,8,10,152,32,3,7,8,56,2,6,144,168,255
        .byte 0,0,152,48,8,9,0,8,3,7,88,40,7,15,64,160,7,15,40,152,2,6,48,88,2,6,136,104,255
        .byte 0,0,128,8,8,9,0,8,3,8,16,152,2,6,136,104,3,8,80,120,5,13,16,104,255
        .byte 0,0,128,160,8,9,0,8,8,10,72,24,7,16,96,96,7,16,64,104,2,6,64,80,255
        .byte 0,0,152,80,8,9,0,8,3,7,72,40,7,16,40,24,2,5,112,152,5,13,16,128,255
        .byte 0,0,152,64,8,9,0,8,2,12,64,144,7,16,40,24,7,16,56,40,7,16,72,56,255
        .byte 0,0,64,16,8,9,0,8,2,12,64,32,2,12,64,88,2,12,64,144,2,6,32,80,3,7,88,112,5,13,16,88,255
        .byte 0,0,152,16,8,9,0,8,1,14,64,56,7,15,24,160,4,8,88,8,255
        .byte 0,0,152,104,8,9,0,8,7,15,40,88,3,7,96,24,7,8,128,56,255
        .byte 0,0,72,32,8,9,0,8,2,6,152,16,7,16,112,32,7,15,128,88,7,7,16,96,255
        .byte 0,0,152,40,8,9,0,8,4,8,8,32,1,3,152,104,255
.out .sprintf("POSITIONS SIZE = %d", (* - nmedat))

NUMOBJ = 13
objdta:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,3,64,3,64,7,160,7,160,15,208,10,208,29,232,31,232,15,208,0,0,15,144,16,15,144,16
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,3,64,3,64,7,160,7,160,15,208,10,208,29,232,31,232,15,208,0,0,12,64,160,12,64,160
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,3,64,3,64,7,160,7,160,15,208,10,208,29,232,31,232,15,208,0,0,22,152,128,22,152,128
        .byte 0,0,0,0,0,0,0,0,7,224,13,16,31,248,13,16,11,16,5,32,7,32,3,64,3,64,1,128,1,128,0,0,6,152,152,6,152,152
        .byte 0,0,0,0,0,0,0,0,3,128,15,224,19,240,19,240,47,248,63,232,63,216,30,176,21,80,14,224,3,128,0,0,13,40,64,13,40,64
        .byte 0,0,0,0,0,0,0,0,3,128,15,224,19,240,19,240,47,248,63,232,63,216,30,176,21,80,14,224,3,128,0,0,19,152,128,19,152,128
        .byte 1,192,1,192,1,128,11,240,0,0,23,248,23,248,23,80,23,168,23,80,23,168,23,80,23,248,0,0,23,248,47,252,22,32,160,22,32,160
        .byte 0,0,0,0,0,0,0,12,0,18,24,97,7,129,120,121,7,128,24,124,7,128,120,124,7,128,56,124,7,128,120,104,7,64,144,7,64,144
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,6,96,15,240,31,248,31,248,31,248,15,208,7,160,3,64,1,128,0,0,5,72,128,5,72,128
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,6,96,15,240,31,248,31,248,31,248,15,208,7,160,3,64,1,128,0,0,13,64,40,13,64,40
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,6,96,15,240,31,248,31,248,31,248,15,208,7,160,3,64,1,128,0,0,20,72,136,20,72,136
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,14,224,48,56,94,244,254,254,254,254,106,84,52,184,14,224,15,48,152,15,48,152
        .byte 24,0,60,0,122,0,118,0,227,0,163,0,1,128,1,128,0,192,0,192,0,96,0,96,0,48,0,48,0,26,0,12,10,72,128,10,72,128
.out .sprintf("OBJECTS SIZE = %d", (* - objdta))

font:
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,32,32,32,0,32,0
        .byte 0,0,40,40,0,0,0,0
        .byte 0,108,254,108,108,254,108,0
        .byte 0,24,126,120,126,30,126,24
        .byte 0,230,236,24,48,110,206,0
        .byte 0,48,120,48,126,204,126,0
        .byte 0,0,16,16,0,0,0,0
        .byte 0,12,24,24,24,24,12,0
        .byte 0,96,48,48,48,48,96,0
        .byte 0,0,60,24,126,24,60,0
        .byte 0,0,24,24,126,24,24,0
        .byte 0,0,0,0,0,24,24,48
        .byte 0,0,0,0,60,0,0,0
        .byte 0,0,0,0,0,0,16,0
        .byte 0,0,6,12,24,48,96,0
        .byte 0,0,60,36,36,36,60,0
        .byte 0,0,8,24,8,8,28,0
        .byte 0,0,60,4,60,32,60,0
        .byte 0,0,60,4,60,4,60,0
        .byte 0,0,36,36,60,4,4,0
        .byte 0,0,60,32,60,4,60,0
        .byte 0,0,60,32,60,36,60,0
        .byte 0,0,60,4,4,4,4,0
        .byte 0,0,60,36,60,36,60,0
        .byte 0,0,60,36,60,4,60,0
        .byte 0,0,0,0,32,0,32,0
        .byte 0,0,48,0,0,48,48,96
        .byte 0,0,12,24,48,24,12,0
        .byte 0,0,0,126,0,126,0,0
        .byte 0,0,48,24,12,24,48,0
        .byte 0,124,198,12,24,0,24,0
        .byte 0,124,222,254,254,192,124,0
        .byte 0,0,24,36,60,36,36,0
        .byte 0,0,56,36,56,36,56,0
        .byte 0,0,24,36,32,36,24,0
        .byte 0,0,56,36,36,36,56,0
        .byte 0,0,60,32,56,32,60,0
        .byte 0,0,60,32,56,32,32,0
        .byte 0,0,28,32,44,36,28,0
        .byte 0,0,36,36,60,36,36,0
        .byte 0,0,24,0,24,24,24,0
        .byte 0,0,28,4,4,36,24,0
        .byte 0,0,36,40,48,40,36,0
        .byte 0,0,32,32,32,36,60,0
        .byte 0,0,36,60,36,36,36,0
        .byte 0,0,36,52,44,36,36,0
        .byte 0,0,24,36,36,36,24,0
        .byte 0,0,56,36,56,32,32,0
        .byte 0,0,24,36,36,40,20,0
        .byte 0,0,56,36,56,40,36,0
        .byte 0,0,28,32,24,4,56,0
        .byte 0,0,60,24,24,24,24,0
        .byte 0,0,36,36,36,36,24,0
        .byte 0,0,36,36,36,20,12,0
        .byte 0,0,36,36,36,60,36,0
        .byte 0,0,36,24,24,24,36,0
        .byte 0,0,36,36,28,4,56,0
        .byte 0,0,60,4,24,32,60,0
        .byte 0,30,24,24,24,24,30,0
        .byte 0,0,192,96,48,24,12,0
        .byte 0,240,48,48,48,48,240,0
        .byte 0,48,120,252,48,48,48,0
        .byte 0,0,0,0,0,0,0,255
        .byte 0,60,102,248,96,96,254,0
        .byte 0,0,0,0,0,0,0,0
        .byte 24,24,60,60,86,110,255,126
        .byte 0,56,76,206,190,254,124,56
        .byte 56,84,130,108,108,40,40,16
        .byte 255,110,36,36,4,10,14,4
        .byte 223,159,14,14,12,4,0,0
        .byte 108,182,254,254,124,56,16,0
        .byte 149,149,149,149,149,149,149,149
        .byte 111,111,111,111,111,111,111,111
        .byte 255,255,255,255,255,255,255,255
        .byte 237,237,237,237,237,237,237,237
        .byte 82,82,82,82,82,82,82,82
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
        .byte 0,0,60,36,44,36,60,0
.out .sprintf("FONT SIZE = %d", (* - font))

jtab:
        .byte 248,250,252,254,254,255,255,255,0,0,0,1,1,1,2,2,4,6,8,8,8,99
keys:   .byte 66,97,104,72,73,16,55,48,49,17,18
.out .sprintf("KEYS SIZE = %d", (* - keys))

data_end:
.out .sprintf("TOTAL DATA SIZE = %d", (* - data_start))
