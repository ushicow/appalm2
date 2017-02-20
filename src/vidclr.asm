#define SRC		%a0
#define DST		%a1
#define	PIXPAIR		%a2
#define FONTBASE	%a2
#define FONT		%a2
#define CLR16BASE	%a2
#define ASRC		%a3
#define PIXPAIREVENTABLE %a3
#define PIXMONOLOTABLE 	%a3
#define ACACHE		%a4
#define PIXPAIRODDTABLE	%a4
#define PIXMONOHITABLE	%a4
#define CACHE		%a6
#define DSTDBL		%a6
#define	BITMASK		%d0
#define	PREFETCH	%d1
#define	VCHAR		%d1
#define	HCOUNT		%d2
#define	VCOUNT		%d3
#define DATA		%d6
#define DATALO		%d6
#define MSBOFFSET	%d7
#define DATAHI		%d7

.text
.even
/*
 * Update a single HiRes scanline.
 */
.global gryUpdateHiresScanline
gryUpdateHiresScanline:
	movm.l	#0x133A, -(%sp)
	move.l	(28+4)(%sp), SRC
	move.l	(28+8)(%sp), DST
	move.l	HRMonoLo@END.w(%a5), PIXMONOLOTABLE
	move.l	HRMonoHi@END.w(%a5), PIXMONOHITABLE
	clr.w	DATAHI
	move.w	#19, HCOUNT
gryscanloop:
	move.w	(SRC)+, DATALO
	and.w	#0x7F7F, DATALO
	move.b	DATALO, DATAHI
	lsr.w	#8, DATALO
	move.b	(PIXMONOLOTABLE,DATALO.w), BITMASK
	move.b	DATAHI, PREFETCH
	not.b	PREFETCH
	and.b	#0x01, PREFETCH
	or.b	PREFETCH, BITMASK
	lsl.w	#8, BITMASK
	move.b	(PIXMONOHITABLE,DATAHI.w), BITMASK
	not.b	DATALO
	and.b	#0x40, DATALO
	add.b	DATALO, DATALO
	or.b	DATALO, BITMASK
	move.w	BITMASK, (DST)+
	dbf.w	HCOUNT, gryscanloop
	movm.l	(%sp)+, #0x5CC8
	rts
.global clrUpdateHiresScanline
clrUpdateHiresScanline:
	movm.l	#0x133A, -(%sp)
	move.l	(28+4)(%sp), SRC
	move.l	(28+12)(%sp), CACHE
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	bne	clrcachedirty
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	bne	clrcachedirty
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	bne	clrcachedirty
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	bne	clrcachedirty
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	bne	clrcachedirty
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	bne	clrcachedirty
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	bne	clrcachedirty
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	bne	clrcachedirty
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	bne	clrcachedirty
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	beq	clrscanexit
clrcachedirty:
	move.l	(28+4)(%sp), SRC
	move.l	(28+8)(%sp), DST
	move.l	(28+12)(%sp), CACHE
	clr.w	DATA
	clr.w	MSBOFFSET
	lea	pixEven@END.w(%a5), PIXPAIREVENTABLE
	lea	pixOdd@END.w(%a5), PIXPAIRODDTABLE
	move.w	#18, HCOUNT
	move.w	(SRC)+, BITMASK
	move.w	BITMASK, (CACHE)+
	ror.w	#8, BITMASK
	add.b	BITMASK, BITMASK
	scs.b	MSBOFFSET
	and.b	#16, MSBOFFSET
clrscanloop:
	lea	(PIXPAIREVENTABLE,MSBOFFSET.w), PIXPAIR
	move.b	BITMASK, DATA
	and.b	#0x0F, DATA
	move.b	(PIXPAIR,DATA.w), (DST)+
	lsr.w	#2, BITMASK
	move.b	BITMASK, DATA
	and.b	#0x0F, DATA
	move.b	(PIXPAIR,DATA.w), (DST)+
	lsr.w	#2, BITMASK
	move.b	BITMASK, DATA
	and.b	#0x0F, DATA
	move.b	(PIXPAIR,DATA.w), (DST)+
	lsr.w	#2, BITMASK
	move.b	BITMASK, DATA
	and.b	#0x0F, DATA
 	btst	#9, BITMASK
 	sne.b	MSBOFFSET
 	and.b	#16, MSBOFFSET
	add.w	MSBOFFSET, DATA
	add.w	MSBOFFSET, DATA
	move.b	32(PIXPAIR,DATA.w), (DST)+
	lea	(PIXPAIRODDTABLE,MSBOFFSET.w), PIXPAIR
	lsr.w	#1, BITMASK
	move.b	BITMASK, DATA
	and.b	#0x0F, DATA
	move.b	(PIXPAIR,DATA.w), (DST)+
	lsr.w	#2, BITMASK
	move.b	BITMASK, DATA
	and.b	#0x0F, DATA
	move.b	(PIXPAIR,DATA.w), (DST)+
	lsr.w	#2, BITMASK
	move.b	BITMASK, DATA
	and.b	#0x0F, DATA
	move.b	(PIXPAIR,DATA.w), (DST)+
	lsr.w	#2, BITMASK
	move.w	(SRC)+, PREFETCH
	move.w	PREFETCH, (CACHE)+
	ror.w	#8, PREFETCH
	add.b	PREFETCH, PREFETCH
 	scs.b	MSBOFFSET
 	and.b	#16, MSBOFFSET
	move.b	PREFETCH, DATA
	add.b	DATA, DATA
	and.b	#0x0C, DATA
	and.b	#0x03, BITMASK
	or.b	BITMASK, DATA
	add.w	MSBOFFSET, DATA
	add.w	MSBOFFSET, DATA
	move.b	32(PIXPAIR,DATA.w), (DST)+
	lsr.b	#1, BITMASK
	and.w	#1, BITMASK
	or.w	PREFETCH, BITMASK
	dbf.w	HCOUNT, clrscanloop
	/*
	 * Last byte doesn't require prefetch.
	 */
clrlastword:
	lea	(PIXPAIREVENTABLE,MSBOFFSET.w), PIXPAIR
	move.b	BITMASK, DATA
	and.b	#0x0F, DATA
	move.b	(PIXPAIR,DATA.w), (DST)+
	lsr.w	#2, BITMASK
	move.b	BITMASK, DATA
	and.b	#0x0F, DATA
	move.b	(PIXPAIR,DATA.w), (DST)+
	lsr.w	#2, BITMASK
	move.b	BITMASK, DATA
	and.b	#0x0F, DATA
	move.b	(PIXPAIR,DATA.w), (DST)+
	lsr.w	#2, BITMASK
	move.b	BITMASK, DATA
	and.b	#0x0F, DATA
 	btst	#9, BITMASK
 	sne.b	MSBOFFSET
 	and.b	#16, MSBOFFSET
	add.w	MSBOFFSET, DATA
	add.w	MSBOFFSET, DATA
	move.b	32(PIXPAIR,DATA.w), (DST)+
	lea	(PIXPAIRODDTABLE,MSBOFFSET.w), PIXPAIR
	lsr.w	#1, BITMASK
	move.b	BITMASK, DATA
	and.b	#0x0F, DATA
	move.b	(PIXPAIR,DATA.w), (DST)+
	lsr.w	#2, BITMASK
	move.b	BITMASK, DATA
	and.b	#0x0F, DATA
	move.b	(PIXPAIR,DATA.w), (DST)+
	lsr.w	#2, BITMASK
	move.b	BITMASK, DATA
	and.b	#0x0F, DATA
	move.b	(PIXPAIR,DATA.w), (DST)+
	lsr.w	#2, BITMASK
	and.b	#0x03, BITMASK
	add.w	MSBOFFSET, BITMASK
	add.w	MSBOFFSET, BITMASK
	move.b	32(PIXPAIR,BITMASK.w), (DST)
clrscanexit:
	movm.l	(%sp)+, #0x5CC8
	rts
.global hrclrUpdateHiresScanline
hrclrUpdateHiresScanline:
	movm.l	#0x133A, -(%sp)
	move.l	(28+4)(%sp), SRC
	move.l	(28+12)(%sp), CACHE
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	bne	hrcachedirty
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	bne	hrcachedirty
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	bne	hrcachedirty
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	bne	hrcachedirty
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	bne	hrcachedirty
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	bne	hrcachedirty
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	bne	hrcachedirty
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	bne	hrcachedirty
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	bne	hrcachedirty
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	beq	hrscanexit
hrcachedirty:
	move.l	(28+4)(%sp), SRC
	move.l	(28+8)(%sp), DST
	move.l	(28+12)(%sp), CACHE
	clr.w	DATA
	clr.w	MSBOFFSET
	lea	pixPairEven@END.w(%a5), PIXPAIREVENTABLE
	lea	pixPairOdd@END.w(%a5), PIXPAIRODDTABLE
	move.w	#18, HCOUNT
	move.w	(SRC)+, BITMASK
	move.w	BITMASK, (CACHE)+
	ror.w	#8, BITMASK
	add.b	BITMASK, BITMASK
	scs.b	MSBOFFSET
	and.b	#32, MSBOFFSET
hrscanloop:
	lea	(PIXPAIREVENTABLE,MSBOFFSET.w), PIXPAIR
	move.b	BITMASK, DATA
	and.b	#0x0F, DATA
	add.b	DATA, DATA
	move.w	(PIXPAIR,DATA.w), (DST)+
	lsr.w	#1, BITMASK
	move.b	BITMASK, DATA
	and.b	#0x1E, DATA
	move.w	(PIXPAIR,DATA.w), (DST)+
	lsr.w	#2, BITMASK
	move.b	BITMASK, DATA
	and.b	#0x1E, DATA
	move.w	(PIXPAIR,DATA.w), (DST)+
	lsr.w	#3, BITMASK
	move.b	BITMASK, DATA
	and.b	#0x0F, DATA
 	btst	#9, BITMASK
 	sne.b	MSBOFFSET
 	and.b	#32, MSBOFFSET
	add.w	MSBOFFSET, DATA
	add.w	DATA, DATA
	move.w	64(PIXPAIR,DATA.w), (DST)+
	lea	(PIXPAIRODDTABLE,MSBOFFSET.w), PIXPAIR
	move.b	BITMASK, DATA
	and.b	#0x1E, DATA
	move.w	(PIXPAIR,DATA.w), (DST)+
	lsr.w	#2, BITMASK
	move.b	BITMASK, DATA
	and.b	#0x1E, DATA
	move.w	(PIXPAIR,DATA.w), (DST)+
	lsr.w	#2, BITMASK
	move.b	BITMASK, DATA
	and.b	#0x1E, DATA
	move.w	(PIXPAIR,DATA.w), (DST)+
	lsr.w	#3, BITMASK
	move.w	(SRC)+, PREFETCH
	move.w	PREFETCH, (CACHE)+
	ror.w	#8, PREFETCH
	add.b	PREFETCH, PREFETCH
 	scs.b	MSBOFFSET
 	and.b	#32, MSBOFFSET
	move.b	PREFETCH, DATA
	add.b	DATA, DATA
	and.b	#0x0C, DATA
	and.b	#0x03, BITMASK
	or.b	BITMASK, DATA
	add.w	MSBOFFSET, DATA
	add.w	DATA, DATA
	move.w	64(PIXPAIR,DATA.w), (DST)+
	lsr.b	#1, BITMASK
	and.w	#1, BITMASK
	or.w	PREFETCH, BITMASK
	dbf.w	HCOUNT, hrscanloop
	/*
	 * Last byte doesn't require prefetch.
	 */
hrlastword:
	lea	(PIXPAIREVENTABLE,MSBOFFSET.w), PIXPAIR
	move.b	BITMASK, DATA
	and.b	#0x0F, DATA
	add.b	DATA, DATA
	move.w	(PIXPAIR,DATA.w), (DST)+
	lsr.w	#1, BITMASK
	move.b	BITMASK, DATA
	and.b	#0x1E, DATA
	move.w	(PIXPAIR,DATA.w), (DST)+
	lsr.w	#2, BITMASK
	move.b	BITMASK, DATA
	and.b	#0x1E, DATA
	move.w	(PIXPAIR,DATA.w), (DST)+
	lsr.w	#3, BITMASK
	move.b	BITMASK, DATA
	and.b	#0x0F, DATA
 	btst	#9, BITMASK
 	sne.b	MSBOFFSET
 	and.b	#32, MSBOFFSET
	add.w	MSBOFFSET, DATA
	add.w	DATA, DATA
	move.w	64(PIXPAIR,DATA.w), (DST)+
	lea	(PIXPAIRODDTABLE,MSBOFFSET.w), PIXPAIR
	move.b	BITMASK, DATA
	and.b	#0x1E, DATA
	move.w	(PIXPAIR,DATA.w), (DST)+
	lsr.w	#2, BITMASK
	move.b	BITMASK, DATA
	and.b	#0x1E, DATA
	move.w	(PIXPAIR,DATA.w), (DST)+
	lsr.w	#2, BITMASK
	move.b	BITMASK, DATA
	and.b	#0x1E, DATA
	move.w	(PIXPAIR,DATA.w), (DST)+
	lsr.w	#3, BITMASK
	and.b	#0x03, BITMASK
	add.w	MSBOFFSET, BITMASK
	add.w	BITMASK, BITMASK
	move.w	64(PIXPAIR,BITMASK.w), (DST)
hrscanexit:
	movm.l	(%sp)+, #0x5CC8
	rts
/*
 * Render double height scanline.
 */
.global hrclrUpdateHiresDblScanline
hrclrUpdateHiresDblScanline:
	movm.l	#0x133A, -(%sp)
	move.l	(28+4)(%sp), SRC
	move.l	(28+12)(%sp), CACHE
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	bne	hrcachedirtydbl
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	bne	hrcachedirtydbl
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	bne	hrcachedirtydbl
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	bne	hrcachedirtydbl
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	bne	hrcachedirtydbl
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	bne	hrcachedirtydbl
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	bne	hrcachedirtydbl
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	bne	hrcachedirtydbl
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	bne	hrcachedirtydbl
	move.l	(SRC)+, BITMASK
	cmp.l	(CACHE)+, BITMASK
	beq	hrscanexitdbl
hrcachedirtydbl:
	move.l	(28+4)(%sp), SRC
	move.l	(28+12)(%sp), CACHE
	move.l	(SRC)+, (CACHE)+
	move.l	(SRC)+, (CACHE)+
	move.l	(SRC)+, (CACHE)+
	move.l	(SRC)+, (CACHE)+
	move.l	(SRC)+, (CACHE)+
	move.l	(SRC)+, (CACHE)+
	move.l	(SRC)+, (CACHE)+
	move.l	(SRC)+, (CACHE)+
	move.l	(SRC)+, (CACHE)+
	move.l	(SRC)+, (CACHE)+
	move.l	(28+4)(%sp), SRC
	move.l	(28+8)(%sp), DST
	move.l	DST, DSTDBL
//	add.l	#320, DSTDBL
	add.l	#480, DSTDBL
	clr.w	DATA
	clr.w	MSBOFFSET
	lea	pixPairEven@END.w(%a5), PIXPAIREVENTABLE
	lea	pixPairOdd@END.w(%a5), PIXPAIRODDTABLE
	move.w	#18, HCOUNT
	move.w	(SRC)+, BITMASK
	ror.w	#8, BITMASK
	add.b	BITMASK, BITMASK
	scs.b	MSBOFFSET
	and.b	#32, MSBOFFSET
hrscanloopdbl:
	lea	(PIXPAIREVENTABLE,MSBOFFSET.w), PIXPAIR
	move.b	BITMASK, DATA
	and.w	#0x0F, DATA
	add.b	DATA, DATA
	move.w	(PIXPAIR,DATA.w), DATA
	move.w	DATA, (DST)+
	move.w	DATA, (DSTDBL)+
	lsr.w	#1, BITMASK
	move.b	BITMASK, DATA
	and.w	#0x1E, DATA
	move.w	(PIXPAIR,DATA.w), DATA
	move.w	DATA, (DST)+
	move.w	DATA, (DSTDBL)+
	lsr.w	#2, BITMASK
	move.b	BITMASK, DATA
	and.w	#0x1E, DATA
	move.w	(PIXPAIR,DATA.w), DATA
	move.w	DATA, (DST)+
	move.w	DATA, (DSTDBL)+
	lsr.w	#3, BITMASK
	move.b	BITMASK, DATA
	and.w	#0x0F, DATA
 	btst	#9, BITMASK
 	sne.b	MSBOFFSET
 	and.b	#32, MSBOFFSET
	add.w	MSBOFFSET, DATA
	add.w	DATA, DATA
	move.w	64(PIXPAIR,DATA.w), DATA
	move.w	DATA, (DST)+
	move.w	DATA, (DSTDBL)+
	lea	(PIXPAIRODDTABLE,MSBOFFSET.w), PIXPAIR
	move.b	BITMASK, DATA
	and.w	#0x1E, DATA
	move.w	(PIXPAIR,DATA.w), DATA
	move.w	DATA, (DST)+
	move.w	DATA, (DSTDBL)+
	lsr.w	#2, BITMASK
	move.b	BITMASK, DATA
	and.w	#0x1E, DATA
	move.w	(PIXPAIR,DATA.w), DATA
	move.w	DATA, (DST)+
	move.w	DATA, (DSTDBL)+
	lsr.w	#2, BITMASK
	move.b	BITMASK, DATA
	and.w	#0x1E, DATA
	move.w	(PIXPAIR,DATA.w), DATA
	move.w	DATA, (DST)+
	move.w	DATA, (DSTDBL)+
	lsr.w	#3, BITMASK
	move.w	(SRC)+, PREFETCH
	ror.w	#8, PREFETCH
	add.b	PREFETCH, PREFETCH
 	scs.b	MSBOFFSET
 	and.b	#32, MSBOFFSET
	move.b	PREFETCH, DATA
	and.w	#0x06, DATA
	add.b	DATA, DATA
	and.b	#0x03, BITMASK
	or.b	BITMASK, DATA
	add.w	MSBOFFSET, DATA
	add.w	DATA, DATA
	move.w	64(PIXPAIR,DATA.w), DATA
	move.w	DATA, (DST)+
	move.w	DATA, (DSTDBL)+
	lsr.b	#1, BITMASK
	and.w	#1, BITMASK
	or.w	PREFETCH, BITMASK
	dbf.w	HCOUNT, hrscanloopdbl
	/*
	 * Last byte doesn't require prefetch.
	 */
hrlastworddbl:
	lea	(PIXPAIREVENTABLE,MSBOFFSET.w), PIXPAIR
	move.b	BITMASK, DATA
	and.w	#0x0F, DATA
	add.b	DATA, DATA
	move.w	(PIXPAIR,DATA.w), DATA
	move.w	DATA, (DST)+
	move.w	DATA, (DSTDBL)+
	lsr.w	#1, BITMASK
	move.b	BITMASK, DATA
	and.w	#0x1E, DATA
	move.w	(PIXPAIR,DATA.w), DATA
	move.w	DATA, (DST)+
	move.w	DATA, (DSTDBL)+
	lsr.w	#2, BITMASK
	move.b	BITMASK, DATA
	and.w	#0x1E, DATA
	move.w	(PIXPAIR,DATA.w), DATA
	move.w	DATA, (DST)+
	move.w	DATA, (DSTDBL)+
	lsr.w	#3, BITMASK
	move.b	BITMASK, DATA
	and.w	#0x0F, DATA
 	btst	#9, BITMASK
 	sne.b	MSBOFFSET
 	and.b	#32, MSBOFFSET
	add.w	MSBOFFSET, DATA
	add.w	DATA, DATA
	move.w	64(PIXPAIR,DATA.w), DATA
	move.w	DATA, (DST)+
	move.w	DATA, (DSTDBL)+
	lea	(PIXPAIRODDTABLE,MSBOFFSET.w), PIXPAIR
	move.b	BITMASK, DATA
	and.w	#0x1E, DATA
	move.w	(PIXPAIR,DATA.w), DATA
	move.w	DATA, (DST)+
	move.w	DATA, (DSTDBL)+
	lsr.w	#2, BITMASK
	move.b	BITMASK, DATA
	and.w	#0x1E, DATA
	move.w	(PIXPAIR,DATA.w), DATA
	move.w	DATA, (DST)+
	move.w	DATA, (DSTDBL)+
	lsr.w	#2, BITMASK
	move.b	BITMASK, DATA
	and.w	#0x1E, DATA
	move.w	(PIXPAIR,DATA.w), DATA
	move.w	DATA, (DST)+
	move.w	DATA, (DSTDBL)+
	lsr.w	#3, BITMASK
	and.b	#0x03, BITMASK
	add.w	MSBOFFSET, BITMASK
	add.w	BITMASK, BITMASK
	move.w	64(PIXPAIR,BITMASK.w), DATA
	move.w	DATA, (DST)
	move.w	DATA, (DSTDBL)
hrscanexitdbl:
	movm.l	(%sp)+, #0x5CC8
	rts
/*
 * Update a row of text.
 */
.global gryUpdateTextRow
gryUpdateTextRow:
	movm.l	#0x1F3A, -(%sp)
	move.l	(36+4)(%sp), SRC
	move.l	(36+8)(%sp), DST
	move.l	(36+12)(%sp), CACHE
	move.w	#39, HCOUNT
grycharloop:
	clr.w	DATA
	move.b	(SRC)+, DATA
	cmp.b	(CACHE), DATA
	beq.b	gryskipchar
	move.b	DATA, (CACHE)+
	move.w	DATA, BITMASK
	lsl.w	#2, DATA
	lsl.w	#1, BITMASK
	add.w	BITMASK, DATA
	move.l	AppleFont@END.w(%a5), FONT
	add.w	DATA, FONT 
	move.w	#5, VCOUNT
grycharscanloop:
	move.b	(FONT)+, (DST)
	add.w	#40, DST
	dbf.w	VCOUNT, grycharscanloop
	sub.w	#(40*6-1), DST
	dbf.w	HCOUNT, grycharloop
	movm.l	(%sp)+, #0x5CF8
	rts
gryskipchar:
	addq.l	#1, CACHE
	addq.l	#1, DST
	dbf.w	HCOUNT, grycharloop
	movm.l	(%sp)+, #0x5CF8
	rts
.global clrUpdateTextRow
clrUpdateTextRow:
	movm.l	#0x1F3A, -(%sp)
	move.l	(36+4)(%sp), SRC
	move.l	(36+8)(%sp), DST
	move.l	(36+12)(%sp), CACHE
	move.w	#39, HCOUNT
charloop:
	clr.w	DATA
	move.b	(SRC)+, DATA
	cmp.b	(CACHE), DATA
	beq.b	skipchar
	move.b	DATA, (CACHE)+
	move.w	DATA, BITMASK
	lsl.w	#2, DATA
	lsl.w	#1, BITMASK
	add.w	BITMASK, DATA
	move.l	AppleFont@END.w(%a5), FONT
	add.w	DATA, FONT 
	move.w	#5, VCOUNT
charscanloop:
	move.b	(FONT)+, BITMASK
	lsl.b	#4, BITMASK
	add.b	BITMASK, BITMASK
	scs.b	DATA
	lsl.w	#8, DATA
	add.b	BITMASK, BITMASK
	scs.b	DATA
	move.w	DATA, (DST)+
	add.b	BITMASK, BITMASK
	scs.b	DATA
	lsl.w	#8, DATA
	add.b	BITMASK, BITMASK
	scs.b	DATA
	move.w	DATA, (DST)
	add.w	#(160-2), DST
	dbf.w	VCOUNT, charscanloop
	sub.w	#(160*6-4), DST
	dbf.w	HCOUNT, charloop
	movm.l	(%sp)+, #0x5CF8
	rts
skipchar:
	addq.l	#1, CACHE
	addq.l	#4, DST
	dbf.w	HCOUNT, charloop
	movm.l	(%sp)+, #0x5CF8
	rts
.global hrclrUpdateTextRow
hrclrUpdateTextRow:
	movm.l	#0x1F3A, -(%sp)
	move.l	(36+4)(%sp), SRC
	move.l	(36+8)(%sp), DST
	move.l	(36+12)(%sp), CACHE
	move.w	#39, HCOUNT
hrcharloop:
	move.b	(SRC), DATA
	cmp.b	(CACHE), DATA
	beq.b	hrskipchar
	move.b	DATA, (CACHE)+
	and.b	#0xC0, DATA
	sne.b	BITMASK
	move.b	BITMASK, DATA
	lsl.w	#8, DATA
	move.b	BITMASK, DATA
	move.w	DATA, (DST)+
	move.w	DATA, (DST)+
	move.w	DATA, (DST)+
	move.w	DATA, (DST)
	clr.w	DATA
	move.b	(SRC)+, DATA
	lsl.w	#3, DATA
	move.l	AppleFont@END.w(%a5), FONT
	add.w	DATA, FONT 
	move.w	#7, VCOUNT
hrcharscanloop:
/*	add.w	#(320-6), DST
*/
	add.w	#(480-6), DST
	move.b	(FONT)+, BITMASK
	add.b	BITMASK, BITMASK
	scs.b	DATA
	lsl.w	#8, DATA
	add.b	BITMASK, BITMASK
	scs.b	DATA
	move.w	DATA, (DST)+
	add.b	BITMASK, BITMASK
	scs.b	DATA
	lsl.w	#8, DATA
	add.b	BITMASK, BITMASK
	scs.b	DATA
	move.w	DATA, (DST)+
	add.b	BITMASK, BITMASK
	scs.b	DATA
	lsl.w	#8, DATA
	add.b	BITMASK, BITMASK
	scs.b	DATA
	move.w	DATA, (DST)+
	add.b	BITMASK, BITMASK
	scs.b	DATA
	lsl.w	#8, DATA
	add.b	BITMASK, BITMASK
	scs.b	DATA
	move.w	DATA, (DST)
	dbf.w	VCOUNT, hrcharscanloop
/*	sub.w	#(320*8-2), DST
*/
	sub.w	#(480*8-2), DST
	dbf.w	HCOUNT, hrcharloop
	movm.l	(%sp)+, #0x5CF8
	rts
hrskipchar:
	addq.l	#1, SRC
	addq.l	#1, CACHE
	addq.l	#8, DST
	dbf.w	HCOUNT, hrcharloop
	movm.l	(%sp)+, #0x5CF8
	rts
.global hrclrUpdateDblTextRow
hrclrUpdateDblTextRow:
	movm.l	#0x1F3A, -(%sp)
	move.l	(36+4)(%sp), SRC
	move.l	(36+8)(%sp), DST
	move.l	(36+12)(%sp), CACHE
	move.w	#39, HCOUNT
dblcharloop:
	move.b	(SRC), DATA
	cmp.b	(CACHE), DATA
	beq.b	skipdblchar
	move.b	DATA, (CACHE)+
	and.b	#0xC0, DATA
	sne.b	BITMASK
	move.b	BITMASK, DATA
	lsl.w	#8, DATA
	move.b	BITMASK, DATA
	move.w	DATA, (DST)+
	move.w	DATA, (DST)
	addq.l	#6, DST
	clr.w	DATA
	move.b	(SRC)+, DATA
	lsl.w	#3, DATA
	move.l	AppleFont@END.w(%a5), FONT
	add.w	DATA, FONT 
	move.w	#7, VCOUNT
dblcharscanloop:
//	add.w	#(320-8), DST
	add.w	#(480-8), DST
	move.b	(FONT)+, BITMASK
	not.b	BITMASK
	move.b	BITMASK, DATA
	and.b	#0x83, DATA
	seq.b	(DST)+
	move.b	BITMASK, DATA
	and.b	#0x40, DATA
	seq.b	(DST)+
	move.b	BITMASK, DATA
	and.b	#0x38, DATA
	seq.b	(DST)+
	move.b	BITMASK, DATA
	and.b	#0x04, DATA
	seq.b	(DST)
	addq.l	#5, DST
	dbf.w	VCOUNT, dblcharscanloop
//	sub.w	#(320*8), DST
	sub.w	#(480*8), DST
	dbf.w	HCOUNT, dblcharloop
	movm.l	(%sp)+, #0x5CF8
	rts
skipdblchar:
	addq.l	#1, SRC
	addq.l	#1, CACHE
	addq.l	#8, DST
	dbf.w	HCOUNT, dblcharloop
	movm.l	(%sp)+, #0x5CF8
	rts
/*
 * Update a row of lores blocks.
 */
.global hrclrUpdateLoresRow
hrclrUpdateLoresRow:
	movm.l	#0x1F3A, -(%sp)
	move.l	(36+4)(%sp), SRC
	move.l	(36+8)(%sp), DST
	move.l	(36+12)(%sp), CACHE
	lea	clr16@END.w(%a5), CLR16BASE
	move.w	#39, HCOUNT
blockloop:
	move.b	(SRC), DATA
	cmp.b	(CACHE), DATA
	beq.b	skipblock
	move.b	DATA, (CACHE)+
	andi.w	#0x0F, DATA
	move.b	(CLR16BASE,DATA.w), DATA
	move.b	DATA, BITMASK
	lsl.w	#8, DATA
	move.b	BITMASK, DATA
	move.w	DATA, BITMASK
	swap	DATA
	move.w	BITMASK, DATA
	move.w	#4, VCOUNT
blockscantoploop:
	move.l	DATA, (DST)+
	move.l	DATA, (DST)+
//	add.w	#(320-8), DST
	add.w	#(480-8), DST
	dbf.w	VCOUNT, blockscantoploop
	clr.w	DATA
	move.b	(SRC)+, DATA
	lsr.b	#4, DATA
	move.b	(CLR16BASE,DATA.w), DATA
	move.b	DATA, BITMASK
	lsl.w	#8, DATA
	move.b	BITMASK, DATA
	move.w	DATA, BITMASK
	swap	DATA
	move.w	BITMASK, DATA
	move.w	#3, VCOUNT
blockscanbottomloop:
	move.l	DATA, (DST)+
	move.l	DATA, (DST)+
//	add.w	#(320-8), DST
	add.w	#(480-8), DST
	dbf.w	VCOUNT, blockscanbottomloop
//	sub.w	#(320*9-8), DST
	sub.w	#(480*9-8), DST
	dbf.w	HCOUNT, blockloop
	movm.l	(%sp)+, #0x5CF8
	rts
skipblock:
	addq.l	#1, SRC
	addq.l	#1, CACHE
	addq.l	#8, DST
	dbf.w	HCOUNT, blockloop
	movm.l	(%sp)+, #0x5CF8
	rts
