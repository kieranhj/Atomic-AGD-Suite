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
WINDOWHGT = 18
WINDOWWID = 30 ;面面面面面面面面面面面面
MAPWID = 9
        .byte 255,255,255,255,255,255,255,255,255
mapdat:
        .byte 255,7,0,1,2,3,4,5,255,255,21,8,9,10,6,255,255,255,255,20,22,23,12,13,24,255,255,255,19,18,15,14,16,17,255,255,255,255,255,255,27,26,25,255,255,255,11,255,255,255,255,255,255,255
        .byte 255,255,255,255,255,255,255,255,255
stmap:  .byte 1

evnt00:
        lda #0
        sta chary
        lda #7
        sta charx
        jsr dscor
        lda #16
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp a00092
        jsr scru
        lda #142
        ldy #8
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        sta varo
        iny; ldy #9
        lda (z80_ix),y
        sta varp
a00092: lda #142
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a00162
        jsr scrd
        lda #18
        ldy #8
        sta (z80_ix),y
        ; ldy #8
        lda (z80_ix),y
        sta varo
        iny; ldy #9
        lda (z80_ix),y
        sta varp
a00162: jsr skobj
        sta varobj
        lda #0
        sta chary
        lda #30
        sta charx
        lda varj
        jsr disply
        lda #1
        sta chary
        lda #29
        sta charx
        lda varf
        jsr disply
        lda #16
        cmp varobj
        bcs *+5
        jmp a00410
        lda #6
        cmp varobj
        bcs *+5
        jmp a00334
        lda #0
        cmp varb
        beq *+5
        jmp a00329
        lda varobj
        jsr getob
        lda #<100
        sta z80_l
        lda #>100
        sta z80_h
        jsr addsc
        lda varj
        clc
        adc #1
        sta varj
        lda #1
        sta varb
a00329: jmp a00405
a00334: lda #0
        cmp varc
        beq *+5
        jmp a00405
        lda varobj
        jsr getob
        lda #<50
        sta z80_l
        lda #>50
        sta z80_h
        jsr addsc
        lda varf
        clc
        adc #1
        sta varf
        lda #1
        sta varc
a00405: jmp a00801
a00410: lda #17
        cmp varobj
        beq *+5
        jmp a00444
        lda varobj
        jsr getob
        lda #1
        sta vars
a00444: lda #18
        cmp varobj
        beq *+5
        jmp a00479
        lda varobj
        jsr getob
        lda #1
        sta varn
a00479: lda #19
        cmp varobj
        beq *+5
        jmp a00514
        lda varobj
        jsr getob
        lda #1
        sta varg
a00514: lda #20
        cmp varobj
        beq *+5
        jmp a00549
        lda varobj
        jsr getob
        lda #1
        sta vark
a00549: lda #21
        cmp varobj
        beq *+5
        jmp a00583
        lda varobj
        jsr getob
        lda #1
        sta vard
a00583: lda #22
        cmp varobj
        beq *+5
        jmp a00618
        lda varobj
        jsr getob
        lda #1
        sta varh
a00618: lda #23
        cmp varobj
        beq *+5
        jmp a00653
        lda varobj
        jsr getob
        lda #1
        sta vari
a00653: lda #24
        cmp varobj
        beq *+5
        jmp a00727
        lda varobj
        jsr getob
        lda #<50
        sta z80_l
        lda #>50
        sta z80_h
        jsr addsc
        lda #20
        sta chary
        lda #2
        sta charx
        lda #21
        jsr dmsg
a00727: lda #25
        cmp varobj
        beq *+5
        jmp a00801
        lda varobj
        jsr getob
        lda #<50
        sta z80_l
        lda #>50
        sta z80_h
        jsr addsc
        lda #20
        sta chary
        ; lda #20
        sta charx
        lda #22
        jsr dmsg
a00801: lda #7
        cmp scno
        beq *+5
        jmp a01966
        lda #0
        sta varb
        ; lda #0
        sta varc
        ; lda #0
        jsr gotob
        cmp #255
        beq :+
        jmp a00997
:
        lda #13
        sta chary
        ; lda #13
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
        lda #23
        jsr pattr
        lda #14
        sta chary
        lda #13
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #24
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #25
        jsr pattr
a00997: lda #1
        jsr gotob
        cmp #255
        beq :+
        jmp a01158
:
        lda #13
        sta chary
        lda #17
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #26
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        lda #14
        sta chary
        lda #17
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #27
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #28
        jsr pattr
a01158: lda #2
        jsr gotob
        cmp #255
        beq :+
        jmp a01319
:
        lda #15
        sta chary
        lda #13
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #29
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #30
        jsr pattr
        lda #16
        sta chary
        lda #13
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #31
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #32
        jsr pattr
a01319: lda #3
        jsr gotob
        cmp #255
        beq :+
        jmp a01481
:
        lda #15
        sta chary
        lda #17
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #33
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #34
        jsr pattr
        lda #16
        sta chary
        lda #17
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
a01481: lda #4
        jsr gotob
        cmp #255
        beq :+
        jmp a01643
:
        lda #15
        sta chary
        ; lda #15
        sta charx
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
        lda #38
        jsr pattr
        lda #16
        sta chary
        lda #15
        sta charx
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
        lda #40
        jsr pattr
a01643: lda #5
        jsr gotob
        cmp #255
        beq :+
        jmp a01804
:
        lda #13
        sta chary
        lda #15
        sta charx
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
        lda #42
        jsr pattr
        lda #14
        sta chary
        lda #15
        sta charx
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
        lda #44
        jsr pattr
a01804: lda #6
        jsr gotob
        cmp #255
        beq :+
        jmp a01966
:
        lda #11
        sta chary
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
        lda #46
        jsr pattr
        lda #12
        sta chary
        lda #15
        sta charx
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
        lda #48
        jsr pattr
a01966: lda joyval
        and #8
        beq :+
        jmp a02055
:
        jsr cangu
        beq :+
        jmp a02050
:
        lda #16
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp a02028
        jsr scru
        jmp a02050
a02028: ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a02050: jmp a02122
a02055: jsr cangd
        beq :+
        jmp a02122
:
        lda #142
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a02100
        jsr scrd
        jmp a02122
a02100: ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a02122: lda joyval
        and #1
        beq :+
        jmp a02302
:
        lda #1
        ldy #10
        sta (z80_ix),y
        lda #230
        dey; ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp a02245
        lda #0
        sta varl
        lda #1
        sta varq
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
        jmp a02302
a02245: jsr cangr
        beq :+
        jmp a02302
:
        lda #0
        ldy #6
        sta (z80_ix),y
        lda #1
        sta varq
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a02302: lda joyval
        and #2
        beq :+
        jmp a02482
:
        lda #0
        ldy #10
        sta (z80_ix),y
        lda #9
        dey; ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp a02425
        lda #0
        sta varl
        lda #2
        sta varq
        jsr scrl
        lda #230
        ldy #9
        sta (z80_ix),y
        dey; ldy #8
        lda (z80_ix),y
        sta varo
        iny; ldy #9
        lda (z80_ix),y
        sta varp
        jmp a02482
a02425: jsr cangl
        beq :+
        jmp a02482
:
        lda #1
        ldy #6
        sta (z80_ix),y
        lda #2
        sta varq
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a02482: lda joyval
        and #16
        beq :+
        jmp a02665
:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp a02660
        ; lda #0
        cmp varl
        beq *+5
        jmp a02660
        lda #98
        asl a
        sta sndtyp
        lda varq
        sta varl
        lda #1
        ; ldy #11
        sta (z80_ix),y
        lda #0
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a02617
        lda #2
        sta z80_c
        lda #21
        sta z80_b
        jsr spawn
a02617: lda #1
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp a02660
        lda #2
        sta z80_c
        lda #24
        sta z80_b
        jsr spawn
a02660: jmp a02678
a02665: lda #0
        ldy #11
        sta (z80_ix),y
a02678: lda joyval
        and #64
        beq :+
        jmp a02751
:
        lda #1
        sta chary
        lda #13
        sta charx
        lda #20
        jsr dmsg
        jsr prskey
        lda #1
        sta chary
        lda #13
        sta charx
        lda #7
        jsr dmsg
a02751: lda #0
        cmp vara
        beq *+5
        jmp a02776
        ; lda #0
        jsr animsp
a02776: lda #96
        cmp varr
        beq *+5
        jmp a02821
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #6
        ; ldy #5
        sta (z80_ix),y
a02821: lda #DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a02876
:
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #6
        ; ldy #5
        sta (z80_ix),y
a02876: lda #CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a02925
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a02925: rts
.out .sprintf("EVENT 0 SIZE = %d", (* - evnt00))

evnt01:
        lda #19
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp b00343
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b00127
        jsr cangl
        beq :+
        jmp b00109
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
        jmp b00122
b00109: lda #1
        ldy #11
        sta (z80_ix),y
b00122: jmp b00203
b00127: jsr cangr
        beq :+
        jmp b00190
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
        jmp b00203
b00190: lda #0
        ldy #11
        sta (z80_ix),y
b00203: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00284
        jsr cangu
        beq :+
        jmp b00266
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp b00279
b00266: lda #1
        ldy #10
        sta (z80_ix),y
b00279: jmp b00338
b00284: jsr cangd
        beq :+
        jmp b00324
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp b00338
b00324: lda #0
        ldy #10
        sta (z80_ix),y
b00338: jmp b00612
b00343: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b00423
        jsr cangl
        beq :+
        jmp b00405
:
        ldy #9
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp b00419
b00405: lda #1
        ldy #11
        sta (z80_ix),y
b00419: jmp b00477
b00423: jsr cangr
        beq :+
        jmp b00464
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp b00477
b00464: lda #0
        ldy #11
        sta (z80_ix),y
b00477: lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp b00558
        jsr cangu
        beq :+
        jmp b00540
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp b00553
b00540: lda #1
        ldy #10
        sta (z80_ix),y
b00553: jmp b00612
b00558: jsr cangd
        beq :+
        jmp b00598
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp b00612
b00598: lda #0
        ldy #10
        sta (z80_ix),y
b00612: lda #0
        cmp vara
        beq *+5
        jmp b00637
        ; lda #0
        jsr animsp
b00637: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00892
:
        lda #2
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp b00797
        lda #0
        cmp vara
        beq *+5
        jmp b00792
        lda #50
        asl a
        sta sndtyp
        lda #1
        jsr delay
        lda varr
        clc
        adc #1
        sta varr
        lda #21
        sta chary
        lda vare
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #4
        jsr pattr
        lda vare
        clc
        adc #1
        sta vare
b00792: jmp b00892
b00797: lda #50
        asl a
        sta sndtyp
        lda #1
        jsr delay
        lda varr
        clc
        adc #1
        sta varr
        lda #21
        sta chary
        lda vare
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #4
        jsr pattr
        lda vare
        clc
        adc #1
        sta vare
b00892: lda #2
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b01439
:
        lda #50
        asl a
        sta sndtyp
        lda #8
        jsr explod
        lda #7
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp b01085
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        dey; ldy #5
        sta (z80_ix),y
        lda #0
        sta varl
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #255
        ; ldy #5
        sta (z80_ix),y
        lda #64
        ldy #8
        sta (z80_ix),y
        ; lda #64
        iny; ldy #9
        sta (z80_ix),y
        lda #1
        sta z80_c
        lda #7
        sta z80_b
        jsr spawn
        jmp b01439
b01085: lda #19
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp b01236
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        dey; ldy #5
        sta (z80_ix),y
        lda #0
        sta varl
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #255
        ; ldy #5
        sta (z80_ix),y
        lda #96
        ldy #8
        sta (z80_ix),y
        lda #64
        iny; ldy #9
        sta (z80_ix),y
        lda #1
        sta z80_c
        lda #19
        sta z80_b
        jsr spawn
        jmp b01439
b01236: lda #2
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp b01366
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        dey; ldy #5
        sta (z80_ix),y
        lda #0
        sta varl
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #255
        ; ldy #5
        sta (z80_ix),y
        lda #3
        iny; ldy #6
        sta (z80_ix),y
        lda #8
        dey; ldy #5
        sta (z80_ix),y
        jmp b01439
b01366: lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #0
        sta varl
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #255
        ; ldy #5
        sta (z80_ix),y
b01439: rts
.out .sprintf("EVENT 1 SIZE = %d", (* - evnt01))

evnt02:
        lda #20
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00122
        lda #8
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
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp c00122
        ; lda #0
        jsr animsp
c00122: lda #21
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00280
        lda #1
        cmp varl
        beq *+5
        jmp c00280
        jsr cangr
        beq :+
        jmp c00246
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
        jmp c00280
c00246: lda #9
        asl a
        sta sndtyp
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #0
        sta varl
c00280: lda #24
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00438
        lda #2
        cmp varl
        beq *+5
        jmp c00438
        jsr cangl
        beq :+
        jmp c00404
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
        jmp c00438
c00404: lda #9
        asl a
        sta sndtyp
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #0
        sta varl
c00438: lda #22
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00829
        lda #70
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        ldy #11
        sta (z80_ix),y
        lda #0
        jsr animsp
        lda #23
        cmp scno
        beq *+5
        jmp c00639
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00635
:
        lda joyval
        and #32
        beq :+
        jmp c00635
:
        lda #25
        jsr gotob
        cmp #255
        beq :+
        jmp c00610
:
        lda #3
        sta scno
        jsr nwscr
        jmp c00635
c00610: lda #3
        sta chary
        lda #8
        sta charx
        lda #23
        jsr dmsg
c00635: jmp c00829
c00639: lda #3
        cmp scno
        beq *+5
        jmp c00764
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00760
:
        lda joyval
        and #32
        beq :+
        jmp c00760
:
        lda #25
        jsr gotob
        cmp #255
        beq :+
        jmp c00735
:
        lda #23
        sta scno
        jsr nwscr
        jmp c00760
c00735: lda #3
        sta chary
        lda #8
        sta charx
        lda #23
        jsr dmsg
c00760: jmp c00829
c00764: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00829
:
        lda joyval
        and #32
        beq :+
        jmp c00829
:
        lda #14
        sta chary
        lda #3
        sta charx
        lda #30
        jsr dmsg
c00829: rts
.out .sprintf("EVENT 2 SIZE = %d", (* - evnt02))

evnt03:
        lda #18
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00256
        lda #40
        sta z80_d
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp d00148
        jsr cangd
        beq :+
        jmp d00118
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp d00143
d00118: lda #1
        ldy #11
        sta (z80_ix),y
        lda #10
        asl a
        sta sndtyp
d00143: jmp d00220
d00148: jsr cangu
        beq :+
        jmp d00188
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp d00220
d00188: lda varrnd
        cmp vara
        beq *+5
        jmp d00220
        lda #0
        ldy #11
        sta (z80_ix),y
d00220: lda varrnd
        cmp vara
        beq *+5
        jmp d00251
        lda #0
        ldy #11
        sta (z80_ix),y
d00251: jmp d00390
d00256: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp d00336
        jsr cangu
        beq :+
        jmp d00318
:
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp d00332
d00318: lda #1
        ldy #11
        sta (z80_ix),y
d00332: jmp d00390
d00336: jsr cangd
        beq :+
        jmp d00377
:
        ldy #8
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp d00390
d00377: lda #0
        ldy #11
        sta (z80_ix),y
d00390: lda #0
        jsr animsp
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00485
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #6
        ; ldy #5
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
d00485: lda #2
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00574
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #0
        sta varl
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #2
        jsr explod
d00574: rts
.out .sprintf("EVENT 3 SIZE = %d", (* - evnt03))

evnt04:
        lda #8
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00200
        jsr cangl
        beq :+
        jmp e00086
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
        jmp e00200
e00086: lda #255
        ldy #5
        sta (z80_ix),y
        lda #3
        cmp scno
        beq *+5
        jmp e00150
        lda #88
        ldy #8
        sta (z80_ix),y
        lda #208
        iny; ldy #9
        sta (z80_ix),y
        jmp e00179
e00150: lda #120
        ldy #8
        sta (z80_ix),y
        lda #208
        iny; ldy #9
        sta (z80_ix),y
e00179: lda #4
        sta z80_c
        lda #8
        sta z80_b
        jsr spawn
e00200: lda #5
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00365
        jsr cangr
        beq :+
        jmp e00285
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
        jmp e00365
e00285: lda #255
        ldy #5
        sta (z80_ix),y
        lda #13
        cmp scno
        beq *+5
        jmp e00344
        lda #72
        ldy #8
        sta (z80_ix),y
        lda #32
        iny; ldy #9
        sta (z80_ix),y
e00344: lda #4
        sta z80_c
        lda #5
        sta z80_b
        jsr spawn
e00365: lda #0
        jsr animsp
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00459
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #6
        ldy #5
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #255
        ; ldy #5
        sta (z80_ix),y
e00459: lda #2
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00548
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #0
        sta varl
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #2
        jsr explod
e00548: rts
.out .sprintf("EVENT 4 SIZE = %d", (* - evnt04))

evnt05:
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp f00169
        lda #16
        sta z80_c
        dey; ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ; ldy #9
        sta (z80_ix),y
        jsr cangl
        beq :+
        jmp f00114
:
        lda #15
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        jmp f00165
f00114: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ; ldy #9
        sta (z80_ix),y
        lda #1
        iny; ldy #10
        sta (z80_ix),y
f00165: jmp f00227
f00169: jsr cangr
        beq :+
        jmp f00214
:
        ldy #9
        lda (z80_ix),y
        clc
        adc #1
        ; ldy #9
        sta (z80_ix),y
        jmp f00227
f00214: lda #0
        ldy #10
        sta (z80_ix),y
f00227: lda #8
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
        sec
        sbc z80_c
        ; ldy #8
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp f00325
        ; lda #0
        jsr animsp
f00325: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00442
:
        lda #50
        asl a
        sta sndtyp
        lda #1
        jsr delay
        lda varr
        clc
        adc #1
        sta varr
        lda #21
        sta chary
        lda vare
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #4
        jsr pattr
        lda vare
        clc
        adc #1
        sta vare
f00442: lda #2
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00531
:
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
        lda #0
        sta varl
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #2
        jsr explod
f00531: rts
.out .sprintf("EVENT 5 SIZE = %d", (* - evnt05))

evnt06:
        lda #90
        asl a
        sta sndtyp
        lda #8
        jsr explod
        lda #6
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp g00061
        ; lda #0
        jsr animsp
g00061: lda #7
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp g00137
        lda #1
        sta deadf
        lda #50
        jsr delay
        jsr cls
        lda #0
        sta varr
        lda #21
        sta chary
        lda vare
        sta charx
        lda #5
        jsr dmsg
g00137: rts
.out .sprintf("EVENT 6 SIZE = %d", (* - evnt06))

evnt07:
        jsr cangu
        beq :+
        jmp h00071
:
        lda #0
        cmp vara
        beq *+5
        jmp h00066
        ldy #8
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #88
        asl a
        sta sndtyp
h00066: jmp h00085
h00071: lda #255
        ldy #5
        sta (z80_ix),y
h00085: lda #0
        jsr animsp
        rts
.out .sprintf("EVENT 7 SIZE = %d", (* - evnt07))

evnt08:
        lda #17
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i00466
        lda #7
        cmp scno
        beq *+5
        jmp i00423
        lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i00418
:
        lda #7
        cmp varj
        beq *+5
        jmp i00328
        lda #10
        cmp varf
        beq *+5
        jmp i00233
        lda #122
        asl a
        sta sndtyp
        lda #<100
        sta z80_l
        lda #>100
        sta z80_h
        jsr addsc
        lda skptr
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255
        ldy #5
        sta (z80_ix),y
        lda ogptr
        sta z80_x
        lda ogptr+1
        sta z80_i
        lda #4
        sta chary
        lda #8
        sta charx
        lda #9
        jsr dmsg
        lda #100
        jsr delay
        lda #11
        sta scno
        jsr nwscr
        jmp i00323
i00233: lda #9
        asl a
        sta sndtyp
        lda #4
        sta chary
        lda #2
        sta charx
        lda #11
        jsr dmsg
        lda #6
        cmp varj
        bcs *+5
        jmp i00323
        lda #9
        asl a
        sta sndtyp
        lda #6
        sta chary
        lda #2
        sta charx
        lda #10
        jsr dmsg
i00323: jmp i00418
i00328: lda #9
        asl a
        sta sndtyp
        lda #4
        sta chary
        lda #2
        sta charx
        lda #10
        jsr dmsg
        lda #9
        cmp varf
        bcs *+5
        jmp i00418
        ; lda #9
        asl a
        sta sndtyp
        lda #6
        sta chary
        lda #2
        sta charx
        lda #11
        jsr dmsg
i00418: jmp i00466
i00423: lda #11
        cmp scno
        beq *+5
        jmp i00466
        lda #200
        cmp vart
        beq *+5
        jmp i00466
        lda #1
        sta gamwon
i00466: lda #3
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i00575
        jsr ifall
        lda #0
        cmp vara
        beq *+5
        jmp i00518
        ; lda #0
        jsr animsp
i00518: lda #0
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i00575
:
        lda #<1
        sta z80_l
        lda #>1
        sta z80_h
        jsr addsc
        lda #255
        ldy #5
        sta (z80_ix),y
i00575: jmp grav
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
j00052: rts
.out .sprintf("EVENT 9 SIZE = %d", (* - evnt09))

evnt10:
        lda #1
        cmp varb
        beq *+5
        jmp k00067
        ; lda #1
        sta chary
        lda #11
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #49
        jsr pattr
        jmp k00110
k00067: lda #1
        sta chary
        lda #11
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #83
        jsr pattr
k00110: lda #1
        cmp varc
        beq *+5
        jmp k00175
        ; lda #1
        sta chary
        lda #21
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #50
        jsr pattr
        jmp k00219
k00175: lda #1
        sta chary
        lda #21
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #83
        jsr pattr
k00219: lda #1
        cmp vars
        beq *+5
        jmp k00279
        ; lda #1
        sta chary
        lda #13
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #74
        jsr pattr
k00279: lda #1
        cmp varn
        beq *+5
        jmp k00340
        ; lda #1
        sta chary
        lda #14
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #75
        jsr pattr
k00340: lda #1
        cmp varg
        beq *+5
        jmp k00400
        ; lda #1
        sta chary
        lda #15
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #76
        jsr pattr
k00400: lda #1
        cmp vark
        beq *+5
        jmp k00461
        ; lda #1
        sta chary
        lda #16
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #77
        jsr pattr
k00461: lda #1
        cmp vard
        beq *+5
        jmp k00521
        ; lda #1
        sta chary
        lda #17
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #78
        jsr pattr
k00521: lda #1
        cmp varh
        beq *+5
        jmp k00582
        ; lda #1
        sta chary
        lda #18
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #79
        jsr pattr
k00582: lda #1
        cmp vari
        beq *+5
        jmp k00642
        ; lda #1
        sta chary
        lda #19
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #80
        jsr pattr
k00642: rts
.out .sprintf("EVENT 10 SIZE = %d", (* - evnt10))

evnt11:
        lda #2
        cmp vara
        beq *+5
        jmp l00031
        lda #0
        sta vara
        jmp l00046
l00031: lda vara
        clc
        adc #1
        sta vara
l00046: lda #7
        cmp scno
        beq *+5
        jmp l00220
        lda #10
        cmp varf
        beq *+5
        jmp l00220
        lda #16
        sta chary
        lda #10
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #51
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #52
        jsr pattr
        lda #17
        sta chary
        lda #10
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #53
        jsr pattr
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #54
        jsr pattr
l00220: lda #11
        cmp scno
        beq *+5
        jmp l00282
        lda #201
        cmp vart
        beq *+5
        jmp l00267
        lda #0
        sta vart
        jmp l00282
l00267: lda vart
        clc
        adc #1
        sta vart
l00282: lda #23
        cmp scno
        beq *+5
        jmp l00386
        lda #24
        jsr gotob
        cmp #255
        beq :+
        jmp l00386
:
        lda #12
        sta chary
        lda #7
        sta charx
        lda #9
        sta loopa
l00346: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #0
        jsr pattr
        dec loopa
        beq :+
        jmp l00346
:
l00386: lda #16
        cmp scno
        beq *+5
        jmp l00490
        lda #24
        jsr gotob
        cmp #255
        beq :+
        jmp l00490
:
        lda #13
        sta chary
        lda #9
        sta charx
        lda #16
        sta loopa
l00450: lda charx
        sta dispx
        lda chary
        sta dispy
        lda #71
        jsr pattr
        dec loopa
        beq :+
        jmp l00450
:
l00490: rts
.out .sprintf("EVENT 11 SIZE = %d", (* - evnt11))

evnt12:
        jsr cls
        lda #1
        sta varm
        ; lda #1
        sta chary
        lda #4
        sta charx
        lda #1
        jsr dmsg
        lda #3
        sta chary
        lda #5
        sta charx
        lda #12
        jsr dmsg
        lda #5
        sta chary
        lda #0
        sta charx
        lda #13
        jsr dmsg
        lda #23
        sta chary
        lda #3
        sta charx
        lda #15
        jsr dmsg
        jsr prskey
        jsr cls
        lda #1
        sta chary
        lda #4
        sta charx
        lda #1
        jsr dmsg
        lda #3
        sta chary
        lda #5
        sta charx
        lda #12
        jsr dmsg
        lda #5
        sta chary
        lda #0
        sta charx
        lda #14
        jsr dmsg
        lda #23
        sta chary
        lda #3
        sta charx
        lda #15
        jsr dmsg
        jsr prskey
        jsr cls
        lda #1
        sta chary
        lda #4
        sta charx
        lda #1
        jsr dmsg
        lda #3
        sta chary
        lda #5
        sta charx
        lda #12
        jsr dmsg
        lda #5
        sta chary
        lda #0
        sta charx
        lda #24
        jsr dmsg
        lda #23
        sta chary
        lda #3
        sta charx
        lda #15
        jsr dmsg
        jsr prskey
        jsr cls
        lda #1
        sta chary
        lda #4
        sta charx
        lda #1
        jsr dmsg
        lda #3
        sta chary
        lda #5
        sta charx
        lda #12
        jsr dmsg
        lda #5
        sta chary
        lda #0
        sta charx
        lda #31
        jsr dmsg
        lda #8
        sta chary
        lda #4
        sta charx
        lda #25
        jsr dmsg
        lda #23
        sta chary
        lda #3
        sta charx
        lda #15
        jsr dmsg
        jsr prskey
        jsr cls
        lda #1
        sta chary
        lda #4
        sta charx
        lda #1
        jsr dmsg
        lda #3
        sta chary
        lda #5
        sta charx
        lda #12
        jsr dmsg
        lda #15
        sta chary
        lda #1
        sta charx
        lda #16
        jsr dmsg
        lda #7
        sta chary
        lda #5
        sta charx
        lda #17
        jsr dmsg
        lda #9
        sta chary
        lda #5
        sta charx
        lda #18
        jsr dmsg
        lda #11
        sta chary
        lda #5
        sta charx
        lda #19
        jsr dmsg
        jsr prskey
        rts
.out .sprintf("EVENT 12 SIZE = %d", (* - evnt12))

evnt13:
        lda #0
        sta vars
        ; lda #0
        sta varn
        ; lda #0
        sta varg
        ; lda #0
        sta vark
        ; lda #0
        sta vart
        ; lda #0
        sta vard
        ; lda #0
        sta vara
        ; lda #0
        sta varb
        ; lda #0
        sta varc
        ; lda #0
        sta vare
        ; lda #0
        sta varf
        ; lda #0
        sta varh
        ; lda #0
        sta vari
        ; lda #0
        sta varj
        ; lda #0
        sta varl
        ; lda #0
        sta varr
        lda #120
        sta varo
        ; lda #120
        sta varp
        lda #9
        sta numlif
        lda #2
        sta varm
        jsr cls
        lda #21
        sta chary
        lda vare
        sta charx
        lda #5
        jsr dmsg
        rts
.out .sprintf("EVENT 13 SIZE = %d", (* - evnt13))

evnt14:
        lda #0
        sta varl
        lda #16
        sta chary
        lda #0
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #83
        jsr pattr
        lda #31
        sta charx
        lda charx
        sta dispx
        lda chary
        sta dispy
        lda #83
        jsr pattr
        lda #1
        sta chary
        lda #13
        sta charx
        lda #7
        jsr dmsg
        lda #20
        sta chary
        lda #0
        sta charx
        lda #4
        jsr dmsg
        lda #1
        sta chary
        ; lda #1
        sta charx
        lda #2
        jsr dmsg
        lda #1
        sta chary
        lda #7
        sta charx
        lda numlif
        jsr disply
        lda #0
        sta chary
        lda #7
        sta charx
        jsr dscor
        lda #0
        sta chary
        lda #1
        sta charx
        lda #3
        jsr dmsg
        lda #0
        sta chary
        lda #14
        sta charx
        lda #6
        jsr dmsg
        lda #0
        sta chary
        lda #30
        sta charx
        lda varj
        jsr disply
        lda #1
        sta chary
        lda #24
        sta charx
        lda #8
        jsr dmsg
        lda #1
        sta chary
        lda #29
        sta charx
        lda varf
        jsr disply
        lda #24
        jsr gotob
        cmp #255
        beq :+
        jmp o00382
:
        lda #20
        sta chary
        lda #2
        sta charx
        lda #21
        jsr dmsg
o00382: lda #25
        jsr gotob
        cmp #255
        beq :+
        jmp o00429
:
        lda #20
        sta chary
        ; lda #20
        sta charx
        lda #22
        jsr dmsg
o00429: rts
.out .sprintf("EVENT 14 SIZE = %d", (* - evnt14))

evnt15:
        rts
.out .sprintf("EVENT 15 SIZE = %d", (* - evnt15))

evnt16:
        lda #0
        sta vare
        ; lda #0
        sta varr
        lda #21
        sta chary
        lda vare
        sta charx
        lda #5
        jsr dmsg
        lda numlif
        sec
        sbc #1
        sta numlif
        rts
.out .sprintf("EVENT 16 SIZE = %d", (* - evnt16))

evnt17:
        jsr cls
        lda #4
        sta varm
        lda #2
        sta chary
        lda #6
        sta charx
        lda #28
        jsr dmsg
        lda #8
        sta chary
        lda #11
        sta charx
        lda #0
        jsr dmsg
        lda #11
        sta chary
        lda #10
        sta charx
        lda #26
        jsr dmsg
        lda #13
        sta chary
        ; lda #13
        sta charx
        jsr dscor
        lda #18
        sta chary
        lda #4
        sta charx
        lda #29
        jsr dmsg
        lda #23
        sta chary
        lda #3
        sta charx
        lda #15
        jsr dmsg
        jsr prskey
        rts
.out .sprintf("EVENT 17 SIZE = %d", (* - evnt17))

evnt18:
        jsr cls
        lda #3
        sta varm
        lda #1
        sta chary
        ; lda #1
        sta charx
        lda #27
        jsr dmsg
        lda #18
        sta chary
        lda #4
        sta charx
        lda #29
        jsr dmsg
        lda #11
        sta chary
        lda #10
        sta charx
        lda #26
        jsr dmsg
        lda #13
        sta chary
        ; lda #13
        sta charx
        jsr dscor
        lda #8
        sta chary
        lda #11
        sta charx
        lda #0
        jsr dmsg
        lda #23
        sta chary
        lda #3
        sta charx
        lda #15
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
        .byte "GAME OVER!",141
        .byte "_       ZUKINOX        _",13
        .byte 141
        .byte "LIVES:",141
        .byte "SCORE:",141
        .byte "_            ENERGY            _",13
        .byte 141
        .byte "________________________________",13
        .byte "________________________________",13
        .byte "________________________________",13
        .byte 141
        .byte "SPACESHIP PARTS:",141
        .byte "-------",141
        .byte "FUEL:",141
        .byte "CONGRATULATIONS!",13
        .byte "    YOU",39
        .byte "VE FINISHED THE GAME!",141
        .byte "NOT ENOUGH SPACESHIP PARTS!",141
        .byte "NOT ENOUGH FUEL!",141
        .byte "By Jaime Grilo (2017)",141
        .byte " In this game you control a",13
        .byte " spacecraft inside the Caves of",13
        .byte " Lavolatum.",13
        .byte 13
        .byte " Your mission is to collect the",13
        .byte " 7 parts of spaceship ZUKINOX.",13
        .byte 13
        .byte " You also have to collect the",13
        .byte " 10 fuel containers.",13
        .byte 13
        .byte " You can only collect 1 part",13
        .byte " and 1 container at a time.",13
        .byte " The flashing (SP) and (F) near",13
        .byte " the ZUKINOX indicates that you",13
        .byte " are carrying 1 part and/or 1",13
        .byte " fuel container.",13
        .byte 141
        .byte " After picking 1 part and/or 1",13
        .byte " fuel container, you must go to",13
        .byte " the starting screen to drop it",13
        .byte " (just enter the room) to be",13
        .byte " able to pick another one(s).",13
        .byte 13
        .byte " In your way you will find many",13
        .byte " hazards.",13
        .byte " Some of them only take energy.",13
        .byte " Others will kill you.",13
        .byte 13
        .byte " When you",39
        .byte "re out of energy, you",13
        .byte " lose a life.",13
        .byte 13
        .byte " After losing a life, you reco-",13
        .byte " ver all your energy.",13
        .byte 13
        .byte 141
        .byte "PRESS ANY KEY TO CONTINUE!",141
        .byte "CREDITS:",13
        .byte 13
        .byte " Game made by Jaime Grilo using:",13
        .byte " - AGD by Jonathan Cauldwell",13
        .byte " - AGDMusicizer by David Saphier",13
        .byte " Loading screen by Andy Green",13
        .byte " Music by Riskej",13
        .byte " Font characters by Damien Guard",13
        .byte 141
        .byte "1. KEYBOARD",141
        .byte "2. KEMPSTON JOYSTICK",141
        .byte "3. SINCLAIR",141
        .byte "_PAUSED",141
        .byte "AntiMagnet",141
        .byte "Access Key",141
        .byte "Need Access Key!",141
        .byte " Collect the 7 letters to form",13
        .byte " the word ZUKINOX.",13
        .byte 13
        .byte " You",39
        .byte "ll also find an AntiMagnet",13
        .byte " that allows you to pass through",13
        .byte " magnetic fields.",13
        .byte 13
        .byte " To use the teleporter, you have",13
        .byte " to find the Access Key.",13
        .byte 13
        .byte " You start with 9 lives.",13
        .byte 13
        .byte " You can control the player via",13
        .byte " Keyboard, Kempton Joystick or",13
        .byte " Sinclair.",13
        .byte 141
        .byte " The KEYBOARD keys are:",13
        .byte 13
        .byte "       Q for UP;",13
        .byte 13
        .byte "       O for LEFT;",13
        .byte 13
        .byte "       P for RIGHT;",13
        .byte 13
        .byte "       SPACE for FIRE;",13
        .byte 13
        .byte "       H to PAUSE te game;",13
        .byte 13
        .byte "       T to USE Teleporter.",13
        .byte 13
        .byte 141
        .byte "FINAL SCORE:",141
        .byte "Off went Zukinox, in search of",13
        .byte 13
        .byte " new adventures...",13
        .byte 13
        .byte 13
        .byte 141
        .byte "Good luck next time!",141
        .byte "Another try? LET",39
        .byte "S GO!!!",141
        .byte "NOT WORKING!",141
        .byte "Cassette release by BUMFUN 2017.",13
        .byte 141
nummsg:
        .byte 32
.out .sprintf("MESSAGE SIZE = %d", (* - msgdat))

chgfx:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 165,165,0,0,5,5,10,10,5,5,10,10,5,5,90,90
        .byte 245,228,170,170,196,68,136,136,213,68,170,170,196,68,136,136
        .byte 245,245,248,240,242,245,248,240,240,241,250,244,240,241,250,250
        .byte 0,17,0,152,17,17,50,152,17,17,34,16,0,17,250,250
        .byte 33,165,64,0,5,5,10,10,5,5,10,10,5,5,18,90
        .byte 165,132,0,2,5,5,10,10,5,5,10,10,5,36,90,72
        .byte 87,78,136,1,10,85,137,171,25,27,170,5,25,25,39,174
        .byte 228,117,0,16,177,213,170,34,177,32,170,80,145,177,234,114
        .byte 37,164,90,82,5,133,90,66,160,165,90,88,36,165,26,10
        .byte 87,93,175,39,85,93,175,38,10,95,175,141,70,95,171,170
        .byte 187,221,17,136,170,85,221,187,221,187,170,85,17,136,187,221
        .byte 119,204,119,221,102,204,119,17,119,136,119,221,119,238,0,0
        .byte 51,238,51,238,51,102,0,238,17,238,51,238,119,238,0,0
        .byte 0,0,119,238,119,221,119,136,119,17,102,204,119,221,119,204
        .byte 0,0,119,238,51,238,17,238,0,238,51,102,51,238,51,238
        .byte 0,0,17,255,51,0,102,255,68,255,85,119,85,51,85,255
        .byte 85,255,85,119,85,51,85,255,102,255,51,0,17,170,0,0
        .byte 255,255,119,119,51,51,255,255,255,255,0,0,170,170,0,0
        .byte 0,0,255,255,0,0,255,255,255,255,119,119,51,51,255,255
        .byte 0,51,0,119,0,255,17,221,51,85,119,153,238,17,255,255
        .byte 255,255,238,17,119,153,51,85,17,221,0,255,0,119,0,51
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1
        .byte 0,0,0,0,0,17,0,35,0,87,0,174,17,76,35,137
        .byte 0,19,0,34,95,10,175,170,87,10,39,170,95,10,175,171
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,136,0
        .byte 76,0,140,0,93,95,141,1,93,19,141,39,93,87,141,39
        .byte 0,0,0,0,8,0,140,0,78,0,175,0,95,8,175,140
        .byte 2,95,34,39,19,95,35,175,2,8,35,39,19,85,0,5
        .byte 95,27,175,171,95,27,175,171,2,19,141,171,93,27,140,1
        .byte 0,85,0,5,0,87,0,0,0,0,0,0,0,0,0,0
        .byte 76,0,140,0,76,0,0,0,0,0,0,0,0,17,0,35
        .byte 93,87,141,175,93,95,141,175,93,68,141,171,93,10,136,34
        .byte 95,76,175,140,95,76,175,140,17,68,174,140,78,76,174,0
        .byte 0,2,0,34,0,19,0,0,0,0,0,0,8,0,140,0
        .byte 78,0,174,0,78,0,0,0,0,0,0,0,0,0,0,0
        .byte 93,87,141,140,78,87,175,34,95,76,175,174,95,95,175,175
        .byte 95,27,35,171,78,87,4,175,19,95,39,175,95,95,175,175
        .byte 19,93,0,175,68,0,39,175,19,93,39,175,95,25,175,136
        .byte 27,76,175,0,0,2,175,174,27,76,175,174,25,95,1,175
        .byte 19,78,38,141,68,27,5,136,85,95,5,140,93,95,175,140
        .byte 95,76,175,174,95,78,1,174,95,78,35,174,95,95,35,175
        .byte 19,95,39,0,78,87,141,140,93,19,141,4,93,87,141,140
        .byte 95,95,0,175,78,87,35,171,95,27,35,171,95,27,35,171
        .byte 0,17,0,1,0,87,1,174,19,25,39,39,17,76,0,175
        .byte 8,0,136,0,78,0,175,136,95,76,174,174,19,8,175,0
        .byte 0,68,0,39,0,76,0,175,17,8,1,175,17,95,1,175
        .byte 2,0,174,0,19,0,175,0,17,8,141,136,27,8,39,136
        .byte 245,245,152,16,177,85,152,16,213,117,216,114,145,117,250,250
        .byte 245,245,200,50,213,245,200,114,213,245,216,250,213,245,250,250
        .byte 95,95,5,5,70,10,39,175,87,95,4,170,85,10,140,170
        .byte 95,95,5,38,10,10,175,174,95,78,137,38,27,70,137,39
        .byte 93,10,5,170,85,8,39,175,87,95,38,170,85,85,175,175
        .byte 27,87,171,38,25,2,175,174,95,78,170,170,85,70,175,175
        .byte 245,245,80,80,100,160,114,250,117,245,64,170,85,160,200,170
        .byte 245,245,80,98,160,160,250,234,245,228,152,98,177,100,152,114
        .byte 213,160,80,170,85,128,114,250,117,245,98,170,85,85,250,250
        .byte 177,117,186,98,145,32,250,234,245,228,170,170,85,100,250,250
        .byte 95,95,15,175,31,31,47,175,31,95,47,143,95,31,15,15
        .byte 0,0,255,136,0,204,255,102,255,34,187,170,153,170,255,170
        .byte 255,170,187,170,153,170,255,170,255,102,0,204,85,136,0,0
        .byte 204,0,238,0,255,0,187,136,170,204,153,238,136,119,255,255
        .byte 255,255,136,119,153,238,170,204,187,136,255,0,238,0,204,0
        .byte 15,15,15,143,15,15,15,143,95,15,15,47,31,15,15,15
        .byte 187,255,187,255,187,187,68,68,187,187,187,238,187,221,170,170
        .byte 187,255,187,255,187,187,68,68,187,187,187,255,187,255,187,255
        .byte 51,238,85,255,187,187,0,68,187,187,187,255,187,255,187,255
        .byte 27,76,174,170,95,85,0,0,95,93,175,170,95,93,175,170
        .byte 95,85,175,170,95,85,0,0,95,85,174,170,95,85,175,170
        .byte 95,93,175,170,95,85,0,0,93,85,170,170,85,85,171,170
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 95,95,175,39,78,87,140,4,8,68,140,4,78,87,175,39
        .byte 0,17,0,16,228,49,200,114,128,245,16,16,32,17,250,250
        .byte 49,145,50,152,49,145,50,152,49,245,0,16,128,49,250,250
        .byte 49,145,250,16,0,49,0,114,32,49,50,16,49,145,250,250
        .byte 196,49,200,50,245,245,234,114,228,117,200,50,196,49,250,250
        .byte 0,49,0,16,245,145,50,152,49,145,50,152,49,145,250,250
        .byte 128,49,0,16,245,145,50,152,49,145,0,16,128,49,250,250
        .byte 49,145,50,152,128,49,250,250,128,49,50,152,49,145,250,250
        .byte 117,213,250,114,85,213,250,98,160,245,250,216,100,245,186,170
        .byte 136,34,0,136,170,34,0,153,85,0,0,34,153,0,68,85
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,68,0,0,0,0,0,0,0,0,34,0,0
        .byte 0,0,0,0,0,0,0,136,0,0,0,0,0,0,0,0
        .byte 0,0,17,136,34,68,85,170,85,170,34,68,17,136,0,0
        .byte 95,78,170,170,76,68,136,136,95,76,170,170,76,68,136,136
bprop:
        .byte 0
        .byte 2
        .byte 2
        .byte 5
        .byte 0
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
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 6
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
        .byte 0
        .byte 0
        .byte 0
        .byte 2
.out .sprintf("BLOCKS SIZE = %d", (* - chgfx))

sprgfx:
        .byte 127,0,17,128,15,240,63,8,224,36,239,20,169,126,173,81,230,81,48,254,7,204,126,24,55,240,3,224,0,0,0,0
        .byte 127,0,17,128,15,240,63,8,224,36,175,20,237,126,169,73,230,73,48,254,7,204,126,24,31,240,3,224,0,0,0,0
        .byte 127,0,17,128,15,240,63,8,224,36,175,20,171,126,233,69,230,69,48,254,7,204,126,24,55,240,3,224,0,0,0,0
        .byte 127,0,17,128,15,240,63,8,224,36,175,20,233,126,171,73,230,73,48,254,7,204,126,24,103,240,3,224,0,0,0,0
        .byte 0,254,1,136,15,240,16,252,36,7,40,247,126,149,138,181,138,103,127,12,51,224,24,126,15,236,7,192,0,0,0,0
        .byte 0,254,1,136,15,240,16,252,36,7,40,245,126,151,146,213,146,103,127,12,51,224,24,126,15,248,7,192,0,0,0,0
        .byte 0,254,1,136,15,240,16,252,36,7,40,245,126,213,162,151,162,103,127,12,51,224,24,126,15,236,7,192,0,0,0,0
        .byte 0,254,1,136,15,240,16,252,36,7,40,245,126,183,146,149,146,103,127,12,51,224,24,126,15,230,7,192,0,0,0,0
        .byte 3,192,4,160,21,40,20,184,37,60,76,190,157,55,188,177,157,55,76,190,37,60,20,184,21,56,4,160,5,32,7,224
        .byte 3,192,5,32,20,168,29,40,60,164,125,50,236,185,141,61,236,185,125,50,60,164,29,40,28,168,5,32,4,160,7,224
        .byte 0,192,64,162,21,40,4,56,37,56,76,170,29,38,40,160,148,54,8,54,37,28,20,176,21,48,4,32,5,0,7,0
        .byte 32,192,0,168,37,34,4,40,37,56,76,170,29,38,40,160,148,54,8,54,37,28,84,176,21,50,4,32,5,0,7,0
        .byte 4,200,16,161,5,32,68,40,5,56,76,170,29,38,40,160,148,54,8,54,37,28,20,176,21,48,68,34,5,0,7,0
        .byte 0,0,1,128,3,192,4,224,4,224,236,247,164,231,140,227,26,88,154,219,136,147,236,183,2,192,4,96,8,240,31,248
        .byte 0,0,0,0,0,0,0,0,95,224,96,24,95,212,64,18,86,223,95,254,95,220,96,24,95,224,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,64,0,127,240,64,20,86,222,95,255,95,222,96,28,127,248,127,224,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,95,224,96,24,86,212,95,242,95,223,96,30,95,252,96,24,95,224,0,0,0,0,0,0
        .byte 0,0,3,128,15,192,63,240,96,24,111,228,45,222,45,208,102,96,48,158,31,228,15,232,7,240,3,96,0,0,0,0
        .byte 0,0,3,0,7,192,59,240,64,24,111,228,45,94,45,208,38,96,48,150,31,228,7,224,19,240,1,96,2,8,0,0
        .byte 0,128,10,0,7,196,75,224,0,24,111,228,37,94,44,208,38,96,48,150,27,100,6,160,3,112,33,32,0,0,4,4
        .byte 8,0,0,34,135,0,10,224,0,24,110,164,37,90,44,208,38,64,48,148,26,100,6,32,1,112,0,32,64,0,0,1
        .byte 0,33,0,0,7,8,10,224,0,16,106,164,5,90,8,144,2,0,48,148,10,68,6,32,0,64,0,8,1,0,128,0
        .byte 8,0,0,4,1,0,8,224,0,16,200,160,0,17,8,144,2,0,0,2,106,64,6,32,0,64,0,0,0,4,0,128
        .byte 0,68,0,0,1,0,32,64,0,16,136,0,0,0,0,128,0,0,0,1,128,64,2,0,0,64,0,0,0,0,4,2
        .byte 32,128,0,0,0,0,0,0,0,0,8,1,0,0,0,0,0,0,0,0,0,0,128,0,0,0,0,64,0,0,2,1
        .byte 0,0,0,0,35,196,84,42,139,209,84,42,35,196,0,0,127,254,128,1,126,126,15,240,0,0,14,112,7,224,3,192
        .byte 0,0,0,0,3,192,1,128,3,192,1,128,3,192,0,0,127,254,129,129,126,126,15,240,1,128,14,112,7,224,3,192
        .byte 0,0,0,0,0,0,0,0,7,250,24,6,43,250,72,2,251,106,127,250,59,250,24,6,7,250,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,2,15,254,40,2,123,106,255,250,123,250,56,6,31,254,7,254,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,7,250,24,6,43,106,79,250,251,250,120,6,63,250,24,6,7,250,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,3,0,2,1,250,3,250,7,122,14,122,28,250,57,251
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,0,192,0,192,0,223,128,209,192,211,224,215,240,215,248,215,252
        .byte 47,251,39,251,63,251,63,251,40,43,55,219,53,219,5,193,5,192,5,192,7,192,0,0,0,0,0,0,0,1,0,3
        .byte 215,252,223,252,223,252,223,252,212,20,219,236,218,236,130,224,2,224,2,224,3,224,0,0,0,0,0,0,128,0,192,0
        .byte 215,251,220,59,231,231,242,79,252,63,254,127,255,255,255,255,61,188,15,240,64,2,127,254,29,184,127,254,249,159,248,31
        .byte 62,252,109,254,75,254,88,30,95,254,92,62,223,254,252,63,63,255,112,15,231,231,220,59,211,251,212,59,215,251,220,59
        .byte 1,128,1,128,7,224,31,248,57,252,119,238,28,56,15,240,4,32,7,224,12,48,15,240,24,24,31,216,31,184,31,120
        .byte 0,0,16,16,16,16,16,16,16,16,20,16,4,64,16,64,5,160,9,0,3,160,9,64,4,64,2,128,1,0,0,0
        .byte 0,0,0,0,16,8,8,8,8,16,12,16,4,80,0,64,5,160,8,0,2,160,9,64,5,64,2,128,4,0,0,0
        .byte 0,0,0,28,0,62,0,20,0,8,0,28,0,28,0,8,0,62,0,42,0,77,0,215,1,101,1,119,85,101,170,103
        .byte 3,192,7,224,14,112,118,110,174,117,167,229,246,111,143,113,142,241,246,111,167,229,174,117,118,110,14,112,7,224,3,192
        .byte 1,128,6,96,8,16,18,8,36,4,32,4,27,104,32,4,79,254,148,159,180,159,148,159,95,254,41,148,57,156,0,0
        .byte 1,128,6,96,8,16,19,8,36,4,32,4,22,216,32,4,79,254,153,47,185,47,153,47,95,254,56,28,56,28,0,0
        .byte 0,0,0,0,42,84,20,40,0,0,28,184,29,56,0,0,28,56,28,56,28,56,28,56,28,56,30,120,15,240,7,224
        .byte 0,0,0,0,20,40,42,84,0,0,29,56,28,184,0,0,28,56,28,56,28,56,28,56,28,56,30,120,15,240,7,224
        .byte 0,0,0,0,0,0,0,0,0,0,20,144,71,252,2,74,71,252,20,144,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 255,255,128,1,0,0,0,32,4,0,1,0,128,33,132,1,16,4,0,160,4,0,0,0,196,35,159,249,185,157,255,255
        .byte 0,0,0,0,0,0,2,0,0,8,0,0,36,4,1,16,0,0,4,32,0,0,0,128,196,35,159,249,185,157,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,9,40,63,226,82,64,63,226,9,40,0,0,0,0,0,0,0,0,0,0,0,0
frmlst:
        .byte 0,4
        .byte 4,4
        .byte 8,2
        .byte 10,3
        .byte 13,1
        .byte 14,3
        .byte 17,8
        .byte 25,2
        .byte 27,3
        .byte 30,1
        .byte 31,1
        .byte 32,1
        .byte 33,1
        .byte 34,1
        .byte 35,1
        .byte 36,1
        .byte 37,2
        .byte 39,1
        .byte 40,1
        .byte 41,2
        .byte 43,2
        .byte 45,1
        .byte 46,2
        .byte 48,1
        .byte 49,1,50,0
.out .sprintf("SPRITES SIZE = %d", (* - sprgfx))

scdat:
        .word 322,258,231,217,224,238,255,273,145,272,251,283,272,274,198,205,155,289,181,273,252,164,179,221,232,263
        .word 282,267
        .byte 9,9,9,5,255,1,18,6,255,9,10,5,6,255,71,16,5,6,255,9,10,5,6,71,255,0,15,5,6,255,9,10,5
        .byte 6,71,255,0,15,5,6,255,9,10,5,6,71,0,5,1,3,3,255,0,4,3,3,1,6,0,0,5,255,1,8,9,9,9
        .byte 5,6,71,0,5,1,3,3,71,0,0,0,3,3,1,6,71,0,0,255,71,8,9,9,9,5,6,71,0,0,5,1,3
        .byte 71,0,0,0,3,1,6,71,71,255,0,10,9,9,9,5,6,71,0,0,0,5,6,71,0,0,0,5,6,71,71,255,0,11
        .byte 9,9,9,5,6,71,0,0,0,5,6,71,0,0,0,5,6,71,255,0,7,5,255,1,4,9,9,9,5,6,71,255,0,4
        .byte 71,71,255,0,4,71,71,255,0,8,71,71,71,5,9,9,9,5,6,71,255,0,23,5,9,9,9,5,6,71,255,0,23
        .byte 5,255,2,5,71,255,0,23,5,255,86,4,0,71,0,0,0,5,6,255,0,4,5,6,255,0,12,5,255,86,4,255,0,5
        .byte 5,6,71,0,0,0,5,6,71,255,0,7,5,255,1,4,255,86,4,255,0,4,5,1,3,71,0,0,0,3,1,6,255,0,7
        .byte 5,6,9,9,9,255,86,4,0,0,0,5,1,3,3,71,0,0,0,3,3,1,6,255,0,6,5,6,9,9,9,255,2,4
        .byte 255,1,5,3,3,71,0,0,0,3,3,255,1,9,6,9,9,9
        .byte 9,9,5,255,1,19,6,9,5,255,1,5,9,9,5,6,255,71,17,5,6,9,5,6,255,71,4,9,9,5,6,71,255,0,16
        .byte 5,6,9,5,6,71,0,0,0,9,9,5,6,71,255,0,16,5,6,9,5,6,71,0,0,0,1,1,1,6,71,255,0,6
        .byte 5,255,1,4,6,255,0,4,5,1,1,1,6,71,0,5,1,0,255,71,4,255,0,6,5,6,255,71,5,255,0,4,255,71,5
        .byte 0,0,5,255,0,11,5,6,71,255,0,15,5,255,0,11,5,6,71,255,0,15,5,255,1,6,6,255,0,4,5,6,71
        .byte 255,0,15,5,6,255,71,7,255,0,4,71,71,255,0,15,5,6,255,0,16,5,255,1,12,6,255,0,16,5,6,255,71,8
        .byte 5,6,9,255,1,11,14,15,255,1,5,6,71,255,0,7,5,6,9,255,1,11,12,13,255,1,5,6,71,255,0,7,5
        .byte 6,9,5,6,255,71,18,0,0,5,6,0,0,0,5,6,9,5,6,255,0,20,5,6,71,0,0,5,6,9,5,6,255,0,20
        .byte 5,6,71,0,0,5,6,9,5,255,1,22,6,71,0,0,5,6,9
        .byte 255,1,30,255,71,10,5,6,255,71,17,5,255,0,10,5,6,71,255,0,16,5,255,0,10,5,6,71,255,0,16,5,255,1,6
        .byte 6,0,0,0,5,6,71,0,0,5,255,1,6,6,0,0,0,5,1,1,1,6,255,71,7,0,0,5,6,71,0,0,5
        .byte 6,255,71,7,0,0,0,71,71,71,6,71,255,0,8,5,6,71,0,0,5,6,71,255,0,12,6,71,255,0,8,5,6
        .byte 71,0,0,5,6,71,255,0,12,6,71,255,0,8,5,6,71,0,0,5,255,1,14,6,71,255,0,8,5,6,71,0,0
        .byte 5,1,255,9,13,6,71,255,0,9,71,71,0,0,5,6,255,9,13,6,71,55,56,255,0,11,5,6,255,9,13,6,71
        .byte 57,58,71,255,0,10,5,6,255,9,13,255,1,6,6,255,0,8,5,6,255,9,18,5,6,71,255,0,7,5,6,255,9,18
        .byte 5,6,71,255,0,7,5,6,255,9,18,5,6,71,255,0,7,5,6,255,9,18,5,255,1,10,6,255,9,13
        .byte 255,7,28,10,10,7,255,71,26,7,10,10,7,71,55,56,255,0,23,7,10,10,7,71,57,58,71,255,0,22,7,10,10
        .byte 255,7,9,255,0,4,255,7,10,16,19,19,60,7,10,10,0,255,71,9,0,0,0,7,255,10,9,17,18,18,61,7,10,10
        .byte 255,0,13,7,255,10,9,255,7,7,255,0,13,7,255,10,8,7,7,255,71,5,255,7,10,255,0,4,255,7,10,71,71
        .byte 0,0,0,20,7,7,255,71,9,255,0,4,255,71,10,0,0,16,19,19,7,7,71,255,0,24,17,18,18,255,7,6,255,0,8
        .byte 55,56,255,0,5,7,0,0,7,255,0,4,21,7,255,10,4,7,71,255,0,7,57,58,71,0,0,0,7,7,14,15,7,7
        .byte 255,0,4,7,255,10,4,255,7,17,12,13,255,7,7,255,10,8,7,255,71,21,255,10,8,7,71,255,0,20,255,10,8
        .byte 7,71,255,0,20,255,10,8,7,71,0,0,255,7,18
        .byte 255,10,13,255,7,15,255,10,15,7,255,71,13,7,255,10,15,7,71,255,0,12,7,255,10,10,255,7,6,71,255,0,12
        .byte 7,255,10,10,7,255,71,4,7,71,255,0,12,255,7,12,71,0,0,0,255,7,11,255,0,4,71,7,7,255,71,9,255,0,4
        .byte 255,71,11,255,0,4,7,7,71,255,0,27,7,7,71,255,0,11,255,7,11,255,0,5,7,7,71,0,0,255,7,5,255,0,4
        .byte 7,255,71,11,255,0,4,7,7,71,0,0,0,71,71,71,7,71,0,0,0,7,71,255,0,14,7,7,71,255,0,6,7
        .byte 71,0,0,0,7,71,255,0,14,7,7,71,255,0,6,7,71,0,0,0,7,71,255,0,14,255,7,6,0,0,0,7,71
        .byte 0,0,0,7,7,7,255,0,7,255,7,7,0,255,71,5,0,0,7,71,255,0,4,71,71,71,255,0,7,255,71,6,255,0,8
        .byte 7,71,0,55,56,255,0,25,7,71,0,57,58,71,255,0,16,255,7,30
        .byte 255,10,8,255,7,20,255,10,10,7,255,71,18,7,255,10,10,7,71,255,0,17,7,255,10,10,7,71,255,0,17,7,10,10
        .byte 255,7,9,71,255,0,13,255,7,5,10,10,7,255,71,9,255,0,13,7,255,10,6,7,255,0,14,7,7,14,15,7,7
        .byte 0,0,7,255,10,6,7,255,0,12,255,7,4,12,13,255,7,12,3,3,255,0,8,3,3,7,255,71,9,7,10,10,255,7,5
        .byte 3,3,3,255,0,6,3,3,3,7,71,255,0,8,7,10,10,255,7,6,3,3,3,255,0,4,3,3,3,7,7,71,255,0,8
        .byte 255,7,28,14,15,7,7,255,71,4,7,7,255,71,11,255,7,9,12,13,7,71,71,0,0,20,7,7,71,255,0,11,255,71,12
        .byte 0,16,19,19,7,0,71,255,0,24,17,18,18,7,255,0,4,7,0,0,0,7,0,0,0,7,0,0,0,7,0,0,0
        .byte 7,255,0,7,21,7,255,0,4,7,71,0,0,7,71,0,0,7,71,0,0,7,71,0,0,7,71,255,0,7,255,7,31
        .byte 255,10,8,7,255,0,4,255,7,17,255,10,8,7,71,0,0,0,7,7,255,71,14,255,7,10,71,0,0,0,7,71,71
        .byte 255,0,13,7,7,255,71,9,255,0,4,71,255,0,14,7,7,71,255,0,27,7,7,71,255,0,27,7,7,71,255,0,27
        .byte 7,7,71,255,0,25,255,7,4,71,255,0,11,55,56,255,0,11,255,7,5,71,0,0,7,7,255,0,6,7,57,58,7
        .byte 255,0,5,45,46,0,0,255,7,6,71,0,0,7,7,7,255,0,4,255,7,6,255,0,4,47,48,0,7,7,71,71,7,7,7
        .byte 71,0,0,0,255,7,7,255,3,4,255,7,9,71,71,0,0,7,7,71,255,0,4,255,7,5,255,71,6,255,7,7,71,71
        .byte 0,0,20,7,7,71,255,0,5,255,71,5,255,0,6,255,71,7,0,16,19,19,7,7,71,255,0,24,17,18,18,7,7
        .byte 71,0,0,0,7,0,0,0,7,0,0,0,55,56,0,0,0,7,255,0,9,21,7,7,71,0,0,0,7,71,0,0,7
        .byte 71,0,0,57,58,71,0,0,7,71,255,0,9,255,7,23,0,0,255,7,6
        .byte 255,22,29,9,22,83,85,255,83,15,84,255,83,4,84,83,83,83,85,83,9,22,255,83,6,84,255,83,17,85,83,83,83
        .byte 9,22,255,83,11,85,83,83,84,83,83,83,85,255,83,9,9,22,255,83,27,9,9,22,83,83,83,85,255,83,4,84,255,83,12
        .byte 85,255,83,5,9,9,22,84,255,83,24,85,83,9,9,22,83,83,83,67,68,255,83,10,85,255,83,7,67,68,83,9,9,9
        .byte 22,83,83,83,66,69,255,83,5,85,255,83,7,84,255,83,4,66,69,83,9,9,9,22,83,85,83,66,69,83,83,83,85
        .byte 255,83,4,85,255,83,9,66,69,83,255,9,4,83,83,83,66,69,255,83,18,66,69,255,9,5,83,83,83,66,69,255,83,18
        .byte 66,69,255,9,6,83,83,66,69,83,85,255,83,7,84,83,83,83,85,255,83,4,66,69,255,2,4,9,9,83,83,66,69
        .byte 255,83,16,85,83,66,69,255,86,4,9,9,9,83,66,69,83,83,83,55,56,255,83,13,66,69,255,86,4,9,9,9,83
        .byte 66,69,83,83,83,57,58,5,255,1,6,6,255,83,5,66,69,255,86,4,255,9,4,65,70,5,255,1,16,6,65,70,255,86,4
        .byte 255,2,30
        .byte 255,2,4,255,1,5,3,3,255,0,4,3,3,255,1,9,6,255,9,6,5,6,255,71,7,255,0,4,255,71,9,5,6
        .byte 255,9,6,5,6,71,255,0,19,5,6,255,9,6,5,6,71,255,0,19,5,6,255,9,6,5,6,71,255,0,19,5,255,1,4
        .byte 9,9,9,5,6,71,255,0,20,255,71,4,9,9,9,5,6,71,255,0,24,9,9,9,5,6,71,255,0,24,255,1,4
        .byte 6,71,255,0,19,5,255,1,4,2,255,71,5,255,0,20,71,71,71,5,2,71,255,0,27,5,2,71,255,0,27,5,2
        .byte 71,255,0,27,5,255,1,30,255,9,120
        .byte 5,255,1,22,6,0,0,0,5,6,9,9,5,6,255,71,19,5,6,71,0,0,5,6,9,9,9,5,6,255,0,18,5
        .byte 6,71,0,0,5,6,255,9,4,5,6,255,0,17,5,6,71,0,0,5,6,9,255,1,5,6,255,0,6,5,1,6,0,0
        .byte 5,255,1,5,6,71,0,0,5,1,1,0,255,71,6,255,0,4,5,1,6,71,71,0,5,6,255,71,6,0,0,0,71
        .byte 5,255,0,10,5,1,6,71,71,0,0,5,6,71,255,0,9,5,255,0,9,5,1,6,71,71,0,0,0,5,6,71,255,0,9
        .byte 5,255,1,10,6,71,71,255,0,4,5,6,71,255,0,9,5,6,255,71,11,255,0,5,5,6,71,255,0,9,5,6,71
        .byte 255,0,15,5,6,71,255,0,9,5,6,71,255,0,15,5,6,71,255,0,9,5,6,71,0,0,0,5,255,3,11,5,6
        .byte 71,255,0,9,5,1,1,6,0,0,5,255,3,11,5,6,71,255,0,7,5,1,1,9,6,71,0,0,255,71,14,255,0,8
        .byte 5,6,9,9,6,71,255,0,19,55,56,0,0,0,5,6,9,9,6,71,255,0,19,57,58,71,0,0,5,6,9,9,255,1,27
        .byte 6,9
        .byte 255,9,5,5,255,1,10,6,255,9,12,5,255,9,5,5,6,255,71,8,5,6,255,9,12,5,255,9,5,5,6,71,255,0,7
        .byte 5,255,1,14,255,9,5,5,6,71,255,0,9,255,71,12,5,255,1,6,6,71,255,0,21,5,6,255,71,7,255,0,21
        .byte 5,6,71,255,0,27,5,6,71,255,0,10,5,255,1,12,6,0,0,0,5,6,71,255,0,11,5,255,1,4,6,255,71,8
        .byte 0,0,5,6,71,255,0,12,5,1,1,6,71,71,255,0,9,5,6,71,255,0,13,5,6,71,71,255,0,10,5,6,71
        .byte 0,55,56,255,0,10,5,6,71,255,0,11,5,6,71,0,57,58,71,255,0,9,5,6,71,0,0,0,5,255,1,17,6
        .byte 255,0,5,5,6,71,255,0,4,255,71,7,5,255,9,5,5,1,1,6,71,71,255,0,4,5,6,71,255,0,11,5,255,9,5
        .byte 5,1,6,71,71,255,0,4,5,1,1,6,255,0,11,5,255,9,5,5,6,71,71,255,0,4,5,255,1,4,6,255,0,10
        .byte 5,255,9,5,5,1,6,255,0,4,5,255,1,17
        .byte 255,83,27,0,83,9,83,83,85,255,83,15,84,255,83,4,84,83,83,83,85,83,9,255,83,7,84,0,255,83,16,85,83,83,83
        .byte 9,255,83,12,85,83,83,84,83,83,83,85,255,83,9,9,255,83,9,0,255,83,18,9,9,255,83,4,85,255,83,4,84
        .byte 255,83,6,0,255,83,5,85,255,83,5,9,9,83,84,255,83,14,0,255,83,9,85,83,9,9,255,83,4,67,68,255,83,10
        .byte 85,0,255,83,6,67,68,83,9,9,9,255,83,4,66,69,255,83,5,85,255,83,7,84,255,83,4,66,69,83,9,9,9
        .byte 83,83,85,83,66,69,83,83,83,85,255,83,4,85,255,83,9,66,69,83,255,9,4,83,83,83,66,69,255,83,18,66,69
        .byte 255,9,5,83,83,83,66,69,255,83,18,66,69,255,9,6,83,83,66,69,83,85,255,83,7,84,83,83,83,85,255,83,4
        .byte 66,69,255,2,4,9,9,83,83,66,69,255,83,16,85,83,66,69,255,86,4,9,9,9,83,66,69,83,83,83,51,52,255,83,13
        .byte 66,69,255,86,4,9,9,9,83,66,69,83,83,83,53,54,5,255,1,6,6,255,83,5,66,69,255,86,4,255,9,4,65
        .byte 70,5,255,1,16,6,66,70,255,86,4,255,2,30
        .byte 255,64,5,59,59,59,255,0,4,255,59,7,64,64,255,59,17,71,0,0,0,59,255,71,5,59,64,64,59,255,71,7,59,59
        .byte 255,71,8,255,0,4,71,255,0,4,255,59,4,71,255,0,6,59,59,71,255,0,17,255,71,4,255,0,6,59,59,71,255,0,27
        .byte 59,59,71,255,0,27,59,59,71,255,0,27,59,59,71,255,0,27,59,59,71,255,0,10,59,59,0,0,59,59,255,0,11
        .byte 59,59,71,255,0,9,59,59,59,71,0,59,59,59,255,0,10,59,59,71,255,0,9,59,59,71,71,0,0,59,59,71,255,0,9
        .byte 59,59,71,255,0,8,59,59,71,71,255,0,4,59,59,255,0,4,255,59,7,71,255,0,8,59,59,71,255,0,5,59,59
        .byte 71,0,0,0,255,59,7,71,255,0,8,59,59,59,255,0,4,59,59,59,71,0,0,0,59,59,255,71,4,59,71,255,0,9
        .byte 59,59,59,0,0,59,59,59,71,71,0,0,0,59,59,71,0,0,0,59,71,55,56,255,0,8,59,59,71,0,59,59,71,71
        .byte 255,0,5,71,71,0,0,0,59,71,57,58,71,255,0,7,59,59,71,0,59,59,71,255,0,11,255,59,14,71,0,255,59,14
        .byte 255,59,22,71,71,255,59,7,255,71,15,255,59,6,71,0,255,59,7,71,255,0,14,255,59,6,71,0,255,59,7,71,255,0,17
        .byte 255,71,4,0,0,0,73,71,71,59,59,71,255,0,24,73,0,0,59,59,71,255,0,6,255,59,19,0,0,59,59,62,255,0,6
        .byte 59,71,71,59,71,71,59,59,255,71,10,59,71,0,59,59,19,19,60,255,0,5,71,0,0,71,0,0,71,71,255,0,9
        .byte 59,0,0,59,59,18,18,61,255,0,22,59,0,0,59,59,63,255,0,6,59,0,0,59,0,0,59,59,255,0,4,59,255,0,8
        .byte 59,59,71,255,0,6,59,71,0,59,71,0,59,59,71,0,0,0,59,71,255,0,7,255,59,22,71,255,0,7,255,59,7
        .byte 71,71,59,71,71,59,71,71,59,59,255,71,4,59,71,255,0,6,20,59,0,255,71,6,0,0,71,0,0,71,0,0,71,71
        .byte 255,0,4,71,255,0,4,16,19,19,59,255,0,26,17,18,18,59,255,0,4,59,59,0,0,59,0,0,59,0,0,59,59
        .byte 255,0,4,59,255,0,7,21,59,255,0,4,59,59,71,0,59,71,0,59,71,0,59,59,71,0,0,0,59,71,255,0,7
        .byte 255,59,31
        .byte 255,8,14,0,0,255,8,15,255,71,11,8,8,71,0,8,8,255,71,11,8,8,71,255,0,10,8,8,71,0,8,8,71
        .byte 255,0,10,8,8,71,0,255,8,11,71,0,255,8,11,0,0,8,8,71,0,0,255,71,11,0,0,255,71,11,0,8,8
        .byte 71,255,0,27,8,8,71,255,0,27,8,8,71,0,255,8,24,0,0,8,8,71,0,0,255,71,24,0,8,8,71,255,0,27
        .byte 8,8,71,255,0,27,8,8,71,0,255,8,14,255,0,12,8,8,71,0,0,255,71,12,8,71,255,0,11,8,8,71,255,0,14
        .byte 8,71,255,0,11,255,8,8,255,0,5,8,8,0,0,8,8,255,0,5,255,8,7,255,71,8,255,0,4,8,8,71,0
        .byte 8,8,71,255,0,5,255,71,6,255,0,12,8,8,71,0,8,8,71,255,0,11,255,8,14,71,0,255,8,14
        .byte 81,81,81,255,2,17,0,0,0,255,2,7,81,81,81,255,2,17,71,0,0,255,2,7,81,81,81,2,2,255,71,13,2,2
        .byte 71,0,0,2,2,71,71,71,255,2,7,71,255,0,12,2,2,71,0,0,2,2,71,0,0,255,2,7,71,255,0,11,2,2
        .byte 71,71,0,0,2,2,71,0,0,255,2,4,255,71,4,255,0,10,2,2,71,71,0,0,0,2,2,71,0,0,255,2,4
        .byte 71,255,0,11,2,2,2,71,71,255,0,4,255,2,9,71,255,0,11,2,2,71,71,255,0,5,255,2,9,71,255,0,12
        .byte 71,71,255,0,7,255,71,4,255,2,4,71,255,0,25,255,2,4,71,255,0,25,2,2,0,71,71,255,0,25,2,2,255,0,28
        .byte 2,2,255,0,23,255,2,17,255,0,13,255,2,17,71,255,0,13,255,71,6,255,81,8,255,2,12,255,0,10,255,81,8
        .byte 255,2,22
        .byte 255,81,28,8,8,255,81,28,8,8,255,81,28,255,8,34,255,71,14,0,0,255,71,7,0,71,0,255,8,4,71,255,0,25
        .byte 255,8,4,71,255,0,23,71,71,255,8,4,71,255,0,25,255,8,4,71,255,0,25,255,8,4,71,255,0,26,71,8,8
        .byte 71,255,0,13,71,71,255,0,12,255,8,8,255,72,16,255,8,8,255,71,7,255,0,16,71,71,71,255,8,4,71,255,0,25
        .byte 255,8,4,71,0,0,0,8,255,87,18,8,0,0,8,8,0,71,71,0,0,0,8,255,71,18,8,71,0,0,71,255,0,6
        .byte 8,71,255,0,17,8,71,0,0,0,255,8,7,71,255,0,17,255,8,5
        .byte 255,8,7,0,0,8,0,0,0,255,8,6,0,0,0,255,8,7,81,8,8,255,71,4,8,71,0,8,71,0,0,8,8
        .byte 71,71,8,8,71,0,0,8,255,71,5,8,81,8,8,71,0,0,0,8,71,0,8,71,0,0,0,71,71,0,0,71,71
        .byte 0,0,8,71,255,0,4,8,81,8,8,71,0,0,0,8,71,0,8,71,255,0,11,8,71,255,0,4,8,81,8,8,71
        .byte 0,8,8,8,71,0,255,8,14,71,255,0,4,8,81,8,8,71,0,8,71,71,71,0,0,255,71,14,255,0,4,8,81
        .byte 255,8,5,71,255,0,22,8,81,8,8,255,71,4,255,0,22,8,81,8,8,71,255,0,25,8,81,0,71,71,255,0,22
        .byte 255,8,4,81,255,0,25,8,255,81,4,255,8,10,255,0,12,255,8,4,255,81,4,8,8,255,71,7,8,71,255,0,11
        .byte 8,255,81,7,8,8,71,255,0,6,8,71,255,0,11,8,255,81,7,8,8,71,255,0,6,255,8,4,255,0,6,255,8,4
        .byte 255,81,7,0,71,71,0,0,3,3,0,0,0,71,71,8,71,0,55,56,0,0,8,255,81,10,255,0,5,8,8,71,255,0,4
        .byte 8,71,0,57,58,71,0,8,255,81,10,255,8,7,71,0,255,8,11,255,81,10
        .byte 2,0,0,2,2,2,63,21,2,2,2,63,21,255,2,4,63,21,255,2,6,71,255,0,4,2,0,0,2,255,71,20,2
        .byte 71,255,0,5,255,71,4,255,0,19,2,71,255,0,28,255,2,6,255,0,24,255,2,10,255,0,11,2,2,62,20,2,2
        .byte 0,0,0,255,2,12,62,20,2,2,2,62,20,2,2,2,255,71,6,0,0,0,71,71,71,255,2,5,255,71,14,255,0,11
        .byte 2,2,2,71,71,71,255,0,24,2,2,2,71,255,0,26,2,2,2,71,255,0,26,2,2,2,71,255,0,27,71,2,2,2
        .byte 255,0,27,255,2,4,255,0,26,255,2,10,255,0,14,255,2,6,255,81,8,2,2,71,255,0,13,2,255,81,13,255,2,13
        .byte 0,0,0,2,255,81,25,255,2,5,255,81,5
        .byte 255,2,4,0,0,255,2,7,63,21,255,2,19,71,0,2,2,2,255,71,18,255,2,4,255,71,4,0,0,3,71,71,255,0,18
        .byte 71,71,2,71,255,0,5,3,71,255,0,21,2,71,0,255,2,6,255,0,21,2,71,0,0,71,71,2,71,255,2,5,62
        .byte 20,255,2,4,255,0,5,255,2,7,71,255,0,4,2,71,0,255,71,11,255,0,5,71,71,71,2,2,2,71,0,255,2,4
        .byte 71,255,0,21,2,2,71,0,2,255,71,4,255,0,7,71,255,0,13,2,2,71,0,2,71,255,0,9,71,0,71,255,0,12
        .byte 2,2,71,0,2,71,255,0,8,71,0,71,0,71,255,0,11,2,2,71,0,2,71,255,0,7,71,0,71,0,71,0,71
        .byte 255,0,10,2,2,71,0,2,71,255,0,6,71,0,71,0,71,0,71,0,71,255,0,9,2,2,71,0,2,71,255,0,7
        .byte 71,0,71,0,71,0,71,255,0,10,2,2,71,0,255,2,4,255,0,20,255,2,4,71,0,0,255,71,4,255,0,5,55
        .byte 56,0,0,55,56,255,0,7,2,2,81,81,2,71,255,0,11,57,58,71,0,57,58,71,255,0,5,2,2,81,81,81,255,2,27
        .byte 81,81,81
        .byte 11,71,0,255,11,28,71,0,11,2,2,2,255,71,22,11,11,71,255,0,4,2,71,255,0,21,11,11,71,0,0,0,2
        .byte 0,71,2,0,2,0,2,0,2,0,2,0,2,0,2,0,2,2,2,0,2,0,2,11,11,71,0,11,2,0,71,0,2
        .byte 71,2,71,2,2,0,71,2,71,2,2,2,71,2,71,2,71,0,2,0,11,11,71,0,11,2,2,2,0,0,2,0,71
        .byte 2,71,2,0,2,71,2,71,2,71,2,2,2,71,2,0,2,11,11,71,0,11,255,71,4,0,0,71,0,0,71,0,71
        .byte 0,71,0,71,0,71,0,71,71,71,0,71,0,11,11,71,0,255,11,28,71,0,0,71,71,0,255,71,10,0,71,255,0,5
        .byte 255,71,5,11,11,71,255,0,16,71,255,0,5,71,255,0,4,11,11,71,255,0,22,71,255,0,4,11,11,71,0,255,11,28
        .byte 71,0,11,255,82,26,11,71,0,11,255,82,26,11,71,0,255,11,4,255,82,23,11,71,0,0,71,71,11,255,82,23,11
        .byte 71,255,0,4,11,255,82,23,255,11,4,0,0,11,11,11,255,82,21
        .byte 255,2,31,255,71,28,2,2,71,255,0,27,2,2,71,0,5,255,1,25,2,2,71,0,0,255,71,25,2,2,71,255,0,27
        .byte 2,2,255,7,19,0,0,255,7,7,2,2,255,71,20,0,7,255,71,6,2,2,71,255,0,20,8,71,255,0,5,2,2
        .byte 71,0,255,8,20,71,0,255,8,4,2,2,71,0,0,255,71,20,0,0,71,71,71,2,2,71,255,0,27,2,2,255,11,26
        .byte 0,0,2,2,255,71,5,3,3,255,71,8,3,3,255,71,10,0,2,2,71,255,0,5,71,71,255,0,8,71,71,255,0,10
        .byte 2,2,71,255,0,27,2,2,71,255,0,9,3,3,255,0,8,3,3,255,0,6,2,2,71,0,255,2,27
        .byte 255,11,31,255,71,10,11,255,71,5,11,255,71,10,11,11,11,71,255,0,9,11,71,255,0,4,11,71,255,0,9,11,11,11
        .byte 71,255,0,10,71,255,0,4,11,71,255,0,9,11,11,11,71,255,0,15,11,71,255,0,10,71,11,71,255,0,9,11,255,0,5
        .byte 11,71,255,0,11,11,71,0,255,11,6,0,0,11,71,255,0,4,255,11,17,255,0,4,255,11,6,0,0,0,11,255,0,12
        .byte 11,255,0,8,255,71,6,255,0,15,11,255,0,29,11,255,0,16,11,255,0,10,11,11,11,255,0,7,255,11,21,82,11
        .byte 0,0,255,11,6,255,82,21,11,0,0,11,255,82,26,11,0,0,11,255,82,26,11,0,0,11,255,82,26,11,0,0,11
        .byte 255,82,26,11,0,0,11,255,82,26
        .byte 255,11,32,255,71,27,11,11,11,71,255,0,26,11,11,11,71,255,0,26,11,0,71,71,255,0,14,255,11,6,16,19,19
        .byte 60,0,0,11,255,0,17,11,255,71,4,11,17,18,18,61,71,0,255,11,5,255,0,13,11,71,0,0,0,255,11,5,71
        .byte 0,11,0,71,71,73,71,255,0,12,11,71,0,0,0,11,255,71,5,0,11,0,0,0,73,71,255,0,12,11,71,255,0,4
        .byte 71,255,0,5,11,0,0,0,73,71,255,0,12,11,71,255,0,10,255,11,7,255,72,9,255,11,15,82,82,82,11,71,71,71
        .byte 255,0,9,255,71,13,11,82,82,82,11,71,255,0,24,11,82,82,82,11,255,87,12,11,255,0,12,11,82,82,82,11,255,71,12
        .byte 11,71,255,0,11,11,82,82,82,11,71,255,0,11,11,71,255,0,8,55,56,0,11,82,82,82,11,71,255,0,11,11,71
        .byte 255,0,8,57,58,71,11,82,82,82,255,11,17,0,0,0,255,11,7
        .byte 255,81,94,255,8,18,255,81,12,8,255,0,16,8,255,81,12,8,255,71,16,8,255,81,12,8,71,255,0,15,255,8,5
        .byte 255,81,8,8,71,255,0,18,71,8,255,81,8,8,71,255,0,19,8,255,81,4,255,8,5,71,255,0,19,8,255,81,4
        .byte 8,255,71,5,255,0,7,55,56,255,0,7,55,56,0,8,255,81,4,8,71,255,0,11,57,58,255,0,7,57,58,0,8
        .byte 255,81,4,8,71,255,0,7,255,8,6,0,0,255,8,12,81,8,71,255,0,7,8,255,71,6,0,0,8,255,71,4,0
        .byte 71,0,0,0,8,81,8,71,255,0,7,8,71,255,0,7,8,71,255,0,4,71,0,0,0,8,81,8,71,255,0,4,8
        .byte 0,0,8,71,255,0,7,8,71,255,0,4,71,0,0,0,8,81,8,71,0,0,0,8,8,71,0,8,71,0,0,8,8
        .byte 0,0,8,8,71,0,0,8,8,71,0,0,0,8,81,255,8,7,71,0,8,71,0,0,255,8,6,71,0,0,255,8,7
        .byte 81
        .byte 255,19,6,60,0,0,16,255,19,9,60,255,82,10,255,18,6,61,71,0,17,255,18,9,61,255,82,10,60,255,71,4,16
        .byte 60,71,0,16,60,255,71,7,16,60,255,82,10,61,71,0,0,0,17,61,71,0,17,61,71,255,0,6,17,61,255,82,10
        .byte 60,71,255,0,4,71,71,0,0,71,71,255,0,6,16,60,255,82,10,61,71,255,0,16,17,61,255,82,10,60,71,255,0,16
        .byte 16,255,19,6,60,255,82,4,61,71,255,0,16,63,21,255,18,5,61,255,82,4,60,71,0,0,0,16,60,255,0,12,255,71,5
        .byte 16,60,255,82,4,61,71,0,0,0,17,61,255,0,17,17,61,255,82,4,60,71,255,0,22,16,60,255,82,4,61,71,255,0,18
        .byte 16,60,16,60,17,61,255,82,4,60,71,255,0,17,71,17,61,17,61,16,60,255,82,4,61,71,255,0,18,255,71,4,17
        .byte 61,255,82,4,0,71,255,0,11,16,60,255,0,9,16,60,255,82,4,255,0,5,3,3,0,0,3,3,0,0,17,61,71
        .byte 255,0,8,17,61,255,82,4,255,19,18,62,20,255,19,5,60,255,82,4,255,18,25,61,255,82,4
        .byte 255,19,30,255,18,30,60,255,71,6,16,60,255,71,10,73,73,255,71,8,16,61,71,255,0,5,17,61,71,255,0,9,73,73
        .byte 71,255,0,7,17,60,71,255,0,5,16,60,71,255,0,9,73,73,71,255,0,7,16,61,71,255,0,5,17,61,71,255,0,9
        .byte 3,3,71,255,0,7,17,0,71,255,0,5,16,60,71,255,0,7,16,255,19,7,60,0,0,0,16,255,0,7,17,61,71
        .byte 255,0,7,17,63,21,63,21,18,18,18,61,71,0,0,17,60,255,0,6,16,60,71,255,0,8,255,71,6,16,60,71,0,0
        .byte 16,61,71,255,0,5,17,61,71,255,0,14,17,61,71,0,0,17,60,71,255,0,5,16,60,71,255,0,14,16,60,71,0,0
        .byte 16,61,71,255,0,5,17,61,71,255,0,12,16,60,17,61,71,0,0,17,60,71,255,0,5,73,73,71,255,0,12,17,61
        .byte 16,60,71,0,0,16,61,71,255,0,5,73,73,71,255,0,13,71,17,61,71,0,0,17,60,71,255,0,5,73,73,71,255,0,14
        .byte 16,60,71,0,0,0,61,71,0,3,3,0,0,3,3,71,0,3,3,0,0,3,3,255,0,7,17,61,71,0,0,0,255,19,18
        .byte 62,20,62,20,255,19,8,255,18,30
        .byte 255,19,13,60,0,0,16,255,19,13,255,18,13,61,71,0,17,255,18,13,255,82,12,16,60,71,0,16,60,255,71,6,16
        .byte 60,71,71,71,16,255,82,12,17,61,71,0,17,61,71,255,0,5,17,61,71,0,0,17,255,82,12,16,60,71,0,0,71,71
        .byte 255,0,5,16,60,71,0,0,16,255,82,12,17,61,71,255,0,9,17,61,71,0,0,17,255,82,12,16,255,19,8,60,0,0
        .byte 16,60,71,0,0,0,255,82,12,17,255,18,5,63,21,18,61,0,0,17,61,71,0,0,0,255,82,12,16,60,255,71,9
        .byte 0,16,60,71,0,0,16,255,82,12,17,61,71,255,0,9,17,61,71,0,0,17,16,255,19,12,60,71,255,0,9,16,60
        .byte 71,0,0,16,17,255,18,12,61,71,255,0,9,17,61,71,0,0,17,16,60,255,0,12,71,255,0,6,16,19,19,19,60
        .byte 71,0,0,16,17,61,255,0,12,71,255,0,6,17,18,18,18,61,71,0,0,17,16,60,255,0,10,16,60,55,56,255,0,6
        .byte 255,71,5,0,0,16,17,61,255,0,10,17,61,57,58,71,255,0,12,17,16,255,19,17,62,20,255,19,10,17,255,18,29
numsc:
        .byte 28
.out .sprintf("SCREENS SIZE = %d", (* - scdat))

nmedat:
        .byte 0,0,136,8,3,4,136,192,1,2,48,104,1,2,72,176,255
        .byte 0,0,64,8,3,4,56,152,1,2,88,120,3,4,64,72,5,7,128,24,255
        .byte 0,0,32,8,3,4,24,72,3,4,80,104,1,2,32,152,255
        .byte 0,0,64,8,4,8,88,216,2,22,32,200,255
        .byte 0,0,136,8,3,4,104,96,3,4,48,80,3,4,136,136,3,4,88,176,1,2,32,200,255
        .byte 0,0,136,8,4,8,120,216,5,7,40,160,255
        .byte 0,0,136,16,1,2,32,128,4,8,120,216,255
        .byte 0,0,136,224,8,17,128,160,255
        .byte 0,0,64,208,1,7,32,56,1,7,96,72,1,7,64,64,255
        .byte 0,0,64,8,5,19,24,160,3,4,120,184,255
        .byte 0,0,104,64,1,2,64,72,1,2,88,168,3,4,56,112,255
        .byte 7,15,88,120,7,9,104,104,7,14,104,120,7,10,104,136,7,11,120,104,7,13,120,120,7,12,120,136,7,16,128,104,7,16,128,136,8,17,128,160,7,23,104,120,255
        .byte 0,0,64,104,1,19,136,64,1,2,48,40,1,2,80,208,255
        .byte 0,0,136,8,4,5,72,24,4,8,120,224,255
        .byte 0,0,16,120,3,4,88,16,5,7,80,80,5,7,104,208,3,4,56,224,255
        .byte 0,1,136,232,1,2,48,64,1,19,88,40,5,7,120,96,3,4,80,120,255
        .byte 0,0,136,8,2,20,136,80,2,20,136,128,2,20,136,176,5,7,80,128,5,7,56,32,3,4,64,208,3,4,88,40,255
        .byte 0,0,136,8,3,4,64,160,1,19,80,96,255
        .byte 0,1,112,232,3,18,24,56,3,18,24,96,3,18,24,144,1,19,88,48,1,19,104,72,255
        .byte 0,1,40,232,1,2,96,48,3,4,72,112,1,19,128,88,1,19,104,216,3,18,24,112,255
        .byte 0,1,136,40,3,4,136,16,5,7,80,200,255
        .byte 0,0,136,32,5,7,96,32,1,19,32,16,255
        .byte 0,0,88,16,3,4,88,128,255
        .byte 0,0,48,8,2,20,128,56,2,20,128,104,2,22,32,200,3,4,80,128,255
        .byte 0,0,136,32,1,19,80,48,3,4,56,80,3,4,80,112,255
        .byte 0,0,32,64,5,7,80,176,3,4,128,64,3,18,80,152,3,19,112,112,255
        .byte 0,1,128,224,3,18,88,152,3,18,112,168,3,4,128,16,3,7,64,48,3,19,112,80,255
        .byte 0,0,48,120,3,18,96,152,2,22,128,32,5,7,48,184,255
.out .sprintf("POSITIONS SIZE = %d", (* - nmedat))

NUMOBJ = 26
objdta:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,3,0,2,1,250,3,250,7,122,14,122,28,250,57,251,3,104,136,3,104,136
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,0,192,0,192,0,223,128,209,192,211,224,215,240,215,248,215,252,5,88,56,5,88,56
        .byte 47,251,39,251,63,251,63,251,40,43,55,219,53,219,5,193,5,192,5,192,7,192,0,0,0,0,0,0,0,1,0,3,18,96,16,18,96,16
        .byte 215,252,223,252,223,252,223,252,212,20,219,236,218,236,130,224,2,224,2,224,3,224,0,0,0,0,0,0,128,0,192,0,26,128,184,26,128,184
        .byte 215,251,220,59,231,231,242,79,252,63,254,127,255,255,255,255,61,188,15,240,64,2,127,254,29,184,127,254,249,159,248,31,0,112,224,0,112,224
        .byte 62,252,109,254,75,254,88,30,95,254,92,62,223,255,252,63,63,255,112,15,231,231,220,59,211,251,212,59,215,251,220,59,21,24,216,21,24,216
        .byte 1,128,1,128,7,224,30,248,57,252,119,238,28,56,15,240,4,32,7,224,12,48,15,240,24,24,31,216,31,184,31,120,22,48,160,22,48,160
        .byte 0,0,18,72,45,180,109,182,255,255,138,139,186,187,186,187,154,155,186,187,186,187,184,137,255,255,109,182,50,76,31,248,25,128,184,25,128,184
        .byte 0,0,18,72,45,180,109,182,255,255,138,139,186,187,186,187,154,155,186,187,186,187,184,137,255,255,109,182,50,76,31,248,4,40,128,4,40,128
        .byte 0,0,18,72,45,180,109,182,255,255,138,139,186,187,186,187,154,155,186,187,186,187,184,137,255,255,109,182,50,76,31,248,5,32,200,5,32,200
        .byte 0,0,18,72,45,180,109,182,255,255,138,139,186,187,186,187,154,155,186,187,186,187,184,137,255,255,109,182,50,76,31,248,10,136,216,10,136,216
        .byte 0,0,18,72,45,180,109,182,255,255,138,139,186,187,186,187,154,155,186,187,186,187,184,137,255,255,109,182,50,76,31,248,8,104,216,8,104,216
        .byte 0,0,18,72,45,180,109,182,255,255,138,139,186,187,186,187,154,155,186,187,186,187,184,137,255,255,109,182,50,76,31,248,14,24,80,14,24,80
        .byte 0,0,18,72,45,180,109,182,255,255,138,139,186,187,186,187,154,155,186,187,186,187,184,137,255,255,109,182,50,76,31,248,15,48,120,15,48,120
        .byte 0,0,18,72,45,180,109,182,255,255,138,139,186,187,186,187,154,155,186,187,186,187,184,137,255,255,109,182,50,76,31,248,19,40,80,19,40,80
        .byte 0,0,18,72,45,180,109,182,255,255,138,139,186,187,186,187,154,155,186,187,186,187,184,137,255,255,109,182,50,76,31,248,22,48,48,22,48,48
        .byte 0,0,18,72,45,180,109,182,255,255,138,139,186,187,186,187,154,155,186,187,186,187,184,137,255,255,109,182,50,76,31,248,27,112,120,27,112,120
        .byte 0,0,15,224,16,16,47,232,80,20,80,20,94,52,92,116,88,244,81,20,82,20,47,232,16,16,15,224,0,0,0,0,1,96,24,1,96,24
        .byte 0,0,15,224,16,16,47,232,83,148,83,148,83,148,83,148,83,244,80,20,88,52,47,232,16,16,15,224,0,0,0,0,17,72,216,17,72,216
        .byte 0,0,15,224,16,16,47,232,83,148,95,20,80,52,80,116,82,52,83,20,83,20,47,232,16,16,15,224,0,0,0,0,9,104,16,9,104,16
        .byte 0,0,15,224,16,16,47,232,88,52,88,52,95,244,92,116,92,116,88,52,88,52,47,232,16,16,15,224,0,0,0,0,25,88,176,25,88,176
        .byte 0,0,15,224,16,16,47,232,80,52,80,20,95,148,83,148,83,148,83,148,83,148,47,232,16,16,15,224,0,0,0,0,13,72,224,13,72,224
        .byte 0,0,15,224,16,16,47,232,88,52,80,20,95,148,83,148,83,148,80,20,88,52,47,232,16,16,15,224,0,0,0,0,16,88,24,16,88,24
        .byte 0,0,15,224,16,16,47,232,83,148,83,148,88,52,95,244,88,52,83,148,83,148,47,232,16,16,15,224,0,0,0,0,23,80,160,23,80,160
        .byte 3,128,15,224,31,240,41,40,119,220,121,60,217,54,216,54,217,54,120,60,116,92,47,232,28,112,15,224,3,128,0,0,21,48,216,21,48,216
        .byte 0,0,0,0,0,0,15,240,61,124,74,182,126,246,89,50,126,242,89,50,126,246,78,246,63,252,10,160,0,0,0,0,24,128,208,24,128,208
.out .sprintf("OBJECTS SIZE = %d", (* - objdta))

font:
        .byte 0,0,0,0,0,0,0,0
        .byte 24,24,24,24,0,24,24,0
        .byte 102,238,204,0,0,0,0,0
        .byte 108,254,254,108,254,254,108,0
        .byte 126,254,208,124,22,254,252,0
        .byte 196,206,28,56,112,230,70,0
        .byte 112,248,216,254,204,254,126,0
        .byte 24,56,48,0,0,0,0,0
        .byte 28,60,112,112,112,60,28,0
        .byte 112,120,28,28,28,120,112,0
        .byte 24,126,126,60,126,126,24,0
        .byte 0,24,24,126,126,24,24,0
        .byte 0,0,0,0,0,24,56,48
        .byte 0,0,0,126,126,0,0,0
        .byte 0,0,0,0,0,24,24,0
        .byte 4,14,28,56,112,224,64,0
        .byte 56,124,238,198,238,124,56,0
        .byte 112,120,56,56,56,254,254,0
        .byte 252,254,14,124,0,254,254,0
        .byte 252,254,0,60,6,254,252,0
        .byte 56,124,236,204,238,110,12,0
        .byte 254,254,0,252,6,254,252,0
        .byte 126,254,192,220,198,254,124,0
        .byte 252,254,6,30,30,6,6,0
        .byte 124,254,6,124,198,254,124,0
        .byte 124,254,198,118,6,254,252,0
        .byte 0,0,24,24,0,24,24,0
        .byte 0,0,24,24,0,24,56,48
        .byte 14,28,56,112,56,28,14,0
        .byte 0,126,126,0,126,126,0,0
        .byte 112,56,28,14,28,56,112,0
        .byte 124,126,6,12,0,12,12,0
        .byte 124,254,214,222,192,254,126,0
        .byte 240,248,28,204,206,254,254,0
        .byte 252,254,6,252,198,254,252,0
        .byte 126,254,192,192,192,254,126,0
        .byte 248,252,14,198,206,252,248,0
        .byte 254,254,0,248,192,254,254,0
        .byte 254,254,0,248,248,192,192,0
        .byte 126,254,0,198,198,254,126,0
        .byte 198,198,254,254,192,198,198,0
        .byte 60,60,0,24,24,60,60,0
        .byte 6,6,6,6,14,124,120,0
        .byte 198,14,252,248,220,206,198,0
        .byte 192,192,192,192,192,254,126,0
        .byte 108,254,198,214,214,214,198,0
        .byte 252,254,6,198,198,198,198,0
        .byte 124,254,6,198,198,254,124,0
        .byte 252,254,6,254,252,192,192,0
        .byte 124,254,198,198,220,238,118,0
        .byte 252,254,6,252,254,198,198,0
        .byte 126,254,0,124,14,254,252,0
        .byte 126,126,0,24,24,24,24,0
        .byte 198,198,198,198,192,254,124,0
        .byte 198,198,198,198,238,124,56,0
        .byte 198,198,198,214,254,238,198,0
        .byte 198,198,124,0,124,198,198,0
        .byte 198,198,254,126,0,254,252,0
        .byte 254,254,28,56,112,238,222,0
        .byte 124,124,96,96,96,124,124,0
        .byte 64,224,112,56,28,14,4,0
        .byte 62,62,6,6,6,62,62,0
        .byte 16,56,124,254,0,56,56,0
        .byte 254,246,238,198,238,222,254,0
        .byte 62,126,0,124,192,254,254,0
        .byte 0,124,0,126,198,254,126,0
        .byte 192,220,222,198,198,254,252,0
        .byte 0,126,254,192,192,254,126,0
        .byte 6,118,246,198,198,254,126,0
        .byte 0,124,198,222,192,254,126,0
        .byte 30,62,0,120,120,48,48,0
        .byte 0,124,254,198,118,6,254,252
        .byte 192,220,222,198,198,198,198,0
        .byte 24,24,0,24,24,24,24,0
        .byte 6,6,0,6,6,14,126,124
        .byte 192,198,206,252,252,206,198,0
        .byte 24,28,12,12,12,12,12,0
        .byte 0,124,254,198,214,214,214,0
        .byte 0,124,254,198,198,198,198,0
        .byte 0,124,254,6,198,254,124,0
        .byte 0,252,254,198,198,222,220,192
        .byte 0,126,254,198,198,246,118,6
        .byte 0,124,254,198,192,192,192,0
        .byte 0,126,192,252,126,6,252,0
        .byte 96,240,240,96,96,126,62,0
        .byte 0,198,198,198,198,254,124,0
        .byte 0,198,198,198,238,124,56,0
        .byte 0,214,214,214,198,254,124,0
        .byte 0,198,238,0,124,238,198,0
        .byte 0,198,198,246,118,6,254,252
        .byte 0,254,254,28,112,238,222,0
        .byte 30,62,240,240,0,62,30,0
        .byte 24,24,24,24,24,24,24,0
        .byte 240,248,30,30,0,248,240,0
        .byte 54,126,108,0,0,0,0,0
        .byte 30,34,66,76,76,66,34,30
.out .sprintf("FONT SIZE = %d", (* - font))

jtab:
        .byte 248,250,252,254,254,255,255,255,0,0,0,1,1,1,2,2,4,6,8,8,8,99
keys:   .byte 66,97,104,72,73,16,55,48,49,17,18
.out .sprintf("KEYS SIZE = %d", (* - keys))

data_end:
.out .sprintf("TOTAL DATA SIZE = %d", (* - data_start))
