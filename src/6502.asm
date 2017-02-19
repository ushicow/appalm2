/********************************************************************************\
*										 *
*                              6502 emulation					 * 
*										 *
\********************************************************************************/

/*
 * STRICT_IND_EMU tries to enforce exact addressing rules as it applies to zero page
 * access.  Disabling should improve performance and still work in 99.9% of the case.
 */
#define STRICT_IND_EMU		0
/*
 * Define some macros for memory model specific 4addresses.
 */
#define GLOBAL		%a5
#define OPTABLE(o)	(opTable6502+(o))@END.w
#define	STATE_6502	state6502@END.w
#define	STATE_6502_1	(state6502+4)@END.w
#define	STATE_6502_2	(state6502+8)@END.w
#define	STATUS_6502	status6502@END.w
#define	CC_68K_TO_6502	cc_68k_to_6502@END.w
#define	CC_6502_TO_68K	cc_6502_to_68k@END.w
/*
 * Register allocations.
 * Note: SP and CC share the same register.  
 *       Use SWAP to get access to the seperate 16 bit halves.
 */
#define DATA	%d0
#define	INSTRNS	%d1
#define	ACC	%d2
#define	X	%d3
#define	Y	%d4
#define	SP_CC	%d5
#define	CC	%d5
#define	SP	%d5
#define	EA	%d6
#define	PAGE	%d7
#define	PC	%a0
#define	OPBASE	%a1
#define	MEMBASE	%a2
#define	WRMEM	%a3
#define	RDMEM	%a4
#define	ADDR	%a6
/*
 * Condition code bits.
 */
#define CC_C_68K	0x01
#define CC_V_68K	0x02
#define CC_Z_68K	0x04
#define CC_N_68K	0x08
#define CC_X_68K	0x10
#define	CC_C_6502	0x01
#define	CC_Z_6502	0x02
#define	CC_I_6502	0x04
#define	CC_D_6502	0x08
#define	CC_B_6502	0x10
#define	CC_1_6502	0x20
#define	CC_V_6502	0x40
#define	CC_N_6502	0x80
/*
 * Effective address calculations.
 */
#define	EA_IMM					\
	move.b	(PC)+, DATA

#define	EA_ZP					\
	clr.w	EA;				\
	mov.b	(PC)+, EA

#define	EA_ZP_X					\
	move.w	X, EA;				\
	add.b	(PC)+, EA

#define	EA_ZP_Y					\
	move.w	Y, EA;				\
	add.b	(PC)+, EA

#if STRICT_IND_EMU
#define	EA_ZP_IND				\
	clr.w	PAGE;				\
	move.b	(PC)+, PAGE;			\
	move.b	(MEMBASE,PAGE.w), DATA;		\
	addq.b	#1, PAGE;			\
	move.b	(MEMBASE,PAGE.w), PAGE;		\
	lsl.w	#2, PAGE;			\
	move.w	PAGE, EA;			\
	lsl.w	#6, EA;				\
	move.b	DATA, EA

#define	EA_IND_X				\
	move.w	X, PAGE;			\
	add.b	(PC)+, PAGE;			\
	move.b	(MEMBASE,PAGE.w), DATA;		\
	addq.b	#1, PAGE;			\
	move.b	(MEMBASE,PAGE.w), PAGE;		\
	lsl.w	#2, PAGE;			\
	move.w	PAGE, EA;			\
	lsl.w	#6, EA;				\
	move.b	DATA, EA

#define	EA_IND_Y				\
	clr.w	PAGE;				\
	move.b	(PC)+, PAGE;			\
	move.b	(MEMBASE,PAGE.w), DATA;		\
	addq.b	#1, PAGE;			\
	move.b	(MEMBASE,PAGE.w), EA;		\
	lsl.w	#8, EA;				\
	move.b	DATA, EA;			\
	add.w	Y, EA;				\
	move.w	EA, PAGE;			\
	clr.b	PAGE;				\
	lsr.w	#6, PAGE
#else
#define	EA_ZP_IND				\
	clr.w	PAGE;				\
	move.b	(PC)+, PAGE;			\
	lea	1(MEMBASE,PAGE.w), ADDR;	\
	move.b	(ADDR), PAGE;			\
	lsl.w	#2, PAGE;			\
	move.w	PAGE, EA;			\
	lsl.w	#6, EA;				\
	move.b	-(ADDR), EA

#define	EA_IND_X				\
	move.w	X, PAGE;			\
	add.b	(PC)+, PAGE;			\
	lea	1(MEMBASE,PAGE.w), ADDR;	\
	move.b	(ADDR), PAGE;			\
	lsl.w	#2, PAGE;			\
	move.w	PAGE, EA;			\
	lsl.w	#6, EA;				\
	move.b	-(ADDR), EA 

#define	EA_IND_Y				\
	clr.w	PAGE;				\
	move.b	(PC)+, PAGE;			\
	lea	1(MEMBASE,PAGE.w), ADDR;	\
	move.b	(ADDR), EA;			\
	lsl.w	#8, EA;				\
	move.b	-(ADDR), EA;			\
	add.w	Y, EA;				\
	move.w	EA, PAGE;			\
	clr.b	PAGE;				\
	lsr.w	#6, PAGE
#endif

#define EA_ABS					\
	clr.w	PAGE;				\
	move.b	(PC)+, DATA;			\
	move.b	(PC)+, PAGE;			\
	lsl.w	#2, PAGE;			\
	move.w	PAGE, EA;			\
	lsl.w	#6, EA;				\
	move.b	DATA, EA

#define EA_ABS_X				\
	move.b	(PC)+, DATA;			\
	move.b	(PC)+, EA;			\
	lsl.w	#8, EA;				\
	move.b	DATA, EA;			\
	add.w	X, EA;				\
	move.w	EA, PAGE;			\
	clr.b	PAGE;				\
	lsr.w	#6, PAGE

#define EA_ABS_Y				\
	move.b	(PC)+, DATA;			\
	move.b	(PC)+, EA;			\
	lsl.w	#8, EA;				\
	move.b	DATA, EA;			\
	add.w	Y, EA;				\
	move.w	EA, PAGE;			\
	clr.b	PAGE;				\
	lsr.w	#6, PAGE

/*
 * Basic memory read.
 */
#define	RD_MEM					\
	move.w	(RDMEM, PAGE.w), DATA;		\
	beq.b	0f;				\
	swap	DATA;				\
	move.w	2(RDMEM, PAGE.w), DATA;		\
	move.l	DATA, ADDR;			\
	jsr	(ADDR);				\
	bra.b	1f;				\
0:;						\
	move.b	(MEMBASE,EA.l), DATA;		\
1:

#define	LDA_MEM					\
	move.w	(RDMEM, PAGE.w), DATA;		\
	beq.b	0f;				\
	swap	DATA;				\
	move.w	2(RDMEM, PAGE.w), DATA;		\
	move.l	DATA, ADDR;			\
	jsr	(ADDR);				\
	move.b	DATA, ACC;			\
	bra.b	1f;				\
0:;						\
	move.b	(MEMBASE,EA.l), ACC;		\
1:

#define	LDX_MEM					\
	move.w	(RDMEM, PAGE.w), DATA;		\
	beq.b	0f;				\
	swap	DATA;				\
	move.w	2(RDMEM, PAGE.w), DATA;		\
	move.l	DATA, ADDR;			\
	jsr	(ADDR);				\
	move.b	DATA, X;			\
	bra.b	1f;				\
0:;						\
	move.b	(MEMBASE,EA.l), X;		\
1:

#define	LDY_MEM					\
	move.w	(RDMEM, PAGE.w), DATA;		\
	beq.b	0f;				\
	swap	DATA;				\
	move.w	2(RDMEM, PAGE.w), DATA;		\
	move.l	DATA, ADDR;			\
	jsr	(ADDR);				\
	move.b	DATA, Y;			\
	bra.b	1f;				\
0:;						\
	move.b	(MEMBASE,EA.l), Y;		\
1:

/*
 * Basic memory write.
 */
#define	WR_MEM					\
	move.w	DATA, ADDR;			\
	move.w	(WRMEM, PAGE.w), DATA;		\
	beq.b	0f;				\
	swap	DATA;				\
	move.w	2(WRMEM, PAGE.w), DATA;		\
	exg	ADDR, DATA;			\
	and.l	#0xFF, DATA;			\
	jsr	(ADDR);				\
	bra.b	1f;				\
0:;						\
	move.w	ADDR, DATA;			\
	move.b	DATA, (MEMBASE, EA.l);		\
1:

#define	STA_MEM					\
	move.w	(WRMEM, PAGE.w), DATA;		\
	beq.b	0f;				\
	swap	DATA;				\
	move.w	2(WRMEM, PAGE.w), DATA;		\
	move.l	DATA, ADDR;			\
	clr.l	DATA;				\
	move.b	ACC, DATA;			\
	jsr	(ADDR);				\
	bra.b	1f;				\
0:;						\
	move.b	ACC, (MEMBASE, EA.l);		\
1:

#define	STX_MEM					\
	move.w	(WRMEM, PAGE.w), DATA;		\
	beq.b	0f;				\
	swap	DATA;				\
	move.w	2(WRMEM, PAGE.w), DATA;		\
	move.l	DATA, ADDR;			\
	clr.l	DATA;				\
	move.b	X, DATA;			\
	jsr	(ADDR);				\
	bra.b	1f;				\
0:;						\
	move.b	X, (MEMBASE, EA.l);		\
1:

#define	STY_MEM					\
	move.w	(WRMEM, PAGE.w), DATA;		\
	beq.b	0f;				\
	swap	DATA;				\
	move.w	2(WRMEM, PAGE.w), DATA;		\
	move.l	DATA, ADDR;			\
	clr.l	DATA;				\
	move.b	Y, DATA;			\
	jsr	(ADDR);				\
	bra.b	1f;				\
0:;						\
	move.b	Y, (MEMBASE, EA.l);		\
1:

/*
 * Memory write when the data is already in the ADDR register.
 */
#define	WRA_MEM					\
	move.w	(WRMEM, PAGE.w), DATA;		\
	beq.b	0f;				\
	swap	DATA;				\
	move.w	2(WRMEM, PAGE.w), DATA;		\
	exg	ADDR, DATA;			\
	and.l	#0xFF, DATA;			\
	jsr	(ADDR);				\
	bra.b	1f;				\
0:;						\
	move.w	ADDR, DATA;			\
	move.b	DATA, (MEMBASE, EA.l);		\
1:
/*
 * Decrement instruction count and fetch next instruction.
 */
#define	NEXT_OP					\
	dbf.w	INSTRNS, opfetch;		\
	bra.w	exit6502
/*
 * Update CC flags.
 */
#define UPDATE_Z_STATUS				\
 	move.w	%sr, DATA;			\
 	and.b	#0x04, DATA;			\
 	and.b	#0x1D, CC;			\
 	or.b	DATA, CC

#define UPDATE_NZ_STATUS			\
	move.w	%sr, DATA;			\
	and.b	#0x0C, DATA;			\
	and.b	#0x13, CC;			\
	or.b	DATA, CC

#define UPDATE_NZC_STATUS			\
	move.w	%sr, DATA;			\
	and.b	#0x1D, DATA;			\
	and.b	#0x02, CC;			\
	or.b	DATA, CC

/*
 * Opcodes.
 */
#define	OP_BIT					\
	and.b	#0x11, CC;			\
	move.w	CC, ADDR;			\
	btst	#7, DATA;			\
	sne	CC;				\
	and.b	#0x08, CC;			\
	add.w	CC, ADDR;			\
	btst	#6, DATA;			\
	sne	CC;				\
	and.b	#0x02, CC;			\
	add.w	CC, ADDR;			\
	and.b	ACC, DATA;			\
	move.w	%sr, CC;			\
	and.b	#0x04, CC;			\
	add.w	ADDR, CC

#define OP_TRB					\
	and.b	#0x11, CC;			\
	move.w	CC, ADDR;			\
	btst	#7, DATA;			\
	sne	CC;				\
	and.b	#0x08, CC;			\
	add.w	CC, ADDR;			\
	btst	#6, DATA;			\
	sne	CC;				\
	and.b	#0x02, CC;			\
	add.w	CC, ADDR;			\
	not.b	ACC;				\
	and.b	ACC, DATA;			\
	move.w	%sr, CC;			\
	and.b	#0x04, CC;			\
	add.w	ADDR, CC;			\
	not.b	ACC	

#define OP_TSB					\
	and.b	#0x11, CC;			\
	btst	#7, DATA;			\
	sne	CC;				\
	and.b	#0x08, CC;			\
	add.w	CC, ADDR;			\
	btst	#6, DATA;			\
	sne	CC;				\
	and.b	#0x02, CC;			\
	add.w	CC, ADDR;			\
	or.b	ACC, DATA;			\
	move.w	%sr, CC;			\
	and.b	#0x04, CC;			\
	add.w	ADDR, CC

#define	OP_ADC_BIN				\
	or.b	#0x04, CC;			\
	move.w	CC, %ccr;			\
	addx.b	DATA, ACC;			\
	move.w	%sr, CC

#define	OP_ADC_BCD				\
	or.b	#0x04, CC;			\
	move.w	CC, %ccr;			\
	abcd	DATA, ACC;			\
	move.w	%sr, CC

#define	OP_SBC_BIN				\
	eor.b	#0x11, CC;			\
	or.b	#0x04, CC;			\
	move.w	CC, %ccr;			\
	subx.b	DATA, ACC;			\
	move.w	%sr, CC;			\
	eor.b	#0x11, CC
	
#define	OP_SBC_BCD				\
	eor.b	#0x11, CC;			\
	or.b	#0x04, CC;			\
	move.w	CC, %ccr;			\
	sbcd	DATA, ACC;			\
	move.w	%sr, CC;			\
	eor.b	#0x11, CC
		
.data
.even
/*
 * Condition Code/Status Register translation tables.
 */
cc_6502_to_68k:
	.byte 0x00;	.byte 0x02;	.byte 0x08;	.byte 0x0A;
	.byte 0x11;	.byte 0x13;	.byte 0x19;	.byte 0x1B;
	.byte 0x04;	.byte 0x06;	.byte 0x0C;	.byte 0x0E;
	.byte 0x15;	.byte 0x17;	.byte 0x1D;	.byte 0x1F;
cc_68k_to_6502:
	.byte 0x00;	.byte 0x01;	.byte 0x40;	.byte 0x41;
	.byte 0x02;	.byte 0x03;	.byte 0x42;	.byte 0x43;
	.byte 0x80;	.byte 0x81;	.byte 0xC0;	.byte 0xC1;
	.byte 0x82;	.byte 0x83;	.byte 0xC2;	.byte 0xC3;
.text
.even
/*
 * uCode.
 */
UNIMPLEMENTED:
	move.l	PC, DATA
	subq.l	#1, DATA
	move.l	DATA, PC
	sub.l	MEMBASE, DATA
	swap	DATA
	move.b	(PC), DATA
	movem.l	#0x1FFC, STATE_6502_2(GLOBAL)	/* Save 6502 state		*/
	movem.l	(%sp)+, #0x7FFE			/* Restore all registers	*/
	rts
NOP:
	NEXT_OP
CLD:
	and.b	#~CC_D_6502, STATUS_6502(GLOBAL)
	bsr.w	set_bcd_ops
	NEXT_OP
SED:
	or.b	#CC_D_6502, STATUS_6502(GLOBAL)
	bsr.w	set_bcd_ops
	NEXT_OP
CLC:
	and.b	#0xEE, CC
	NEXT_OP
SEC:
	or.b	#0x11, CC
	NEXT_OP
CLV:
	and.b	#0xFD, CC
	NEXT_OP
SEI:
	or.b	#CC_I_6502, STATUS_6502(GLOBAL)
	NEXT_OP
CLI:
	and.b	#~CC_I_6502, STATUS_6502(GLOBAL)
	NEXT_OP
PHP:
	move.b	CC, DATA
	and.w	#0x0F, DATA
	lea	CC_68K_TO_6502(GLOBAL), ADDR
	move.b	(ADDR,DATA.w), DATA
	or.b	STATUS_6502(GLOBAL), DATA
	swap	SP_CC				/* SP is high 16 bits of SP_CC	*/
	move.b	DATA, (MEMBASE,SP.w)
	subq.b	#1, SP
	swap	SP_CC
	NEXT_OP
PHA:
	swap	SP_CC				/* SP is high 16 bits of SP_CC	*/
	move.b	ACC, (MEMBASE,SP.w)
	subq.b	#1, SP
	swap	SP_CC
	NEXT_OP
PHX:
	swap	SP_CC				/* SP is high 16 bits of SP_CC	*/
	move.b	X, (MEMBASE,SP.w)
	subq.b	#1, SP
	swap	SP_CC
	NEXT_OP
PLP:
	swap	SP_CC				/* SP is high 16 bits of SP_CC	*/
	addq.b	#1, SP
	move.b	(MEMBASE,SP.w), DATA
	swap	SP_CC
	clr.w	CC
	move.b	DATA, CC
	and.b	#0x3C, DATA			/* Mask out mapped state	*/
	and.b	#0xC3, CC
	move.b	DATA, STATUS_6502(GLOBAL)
	rol.b	#2, CC
	lea	CC_6502_TO_68K(GLOBAL), ADDR
	move.b	(ADDR,CC.w), CC
	bsr.w	set_bcd_ops
	NEXT_OP
PLA:
	swap	SP_CC				/* SP is high 16 bits of SP_CC	*/
	addq.b	#1, SP
	move.w	SP, ADDR
	swap	SP_CC
	move.b	(MEMBASE,ADDR.w), ACC
	UPDATE_NZ_STATUS
	NEXT_OP
PLX:
	swap	SP_CC				/* SP is high 16 bits of SP_CC	*/
	addq.b	#1, SP
	move.w	SP, ADDR
	swap	SP_CC
	move.b	(MEMBASE,ADDR.w), X
	UPDATE_NZ_STATUS
	NEXT_OP
BRK:
	sub.l	MEMBASE, PC			/* Push PC + 2			*/
	move.w	PC, DATA
	addq	#1, DATA
	ror.w	#8, DATA
	swap	SP_CC				/* SP is high 16 bits of SP_CC	*/
	move.b	DATA, (MEMBASE,SP.w)
	lsr.w	#8, DATA
	subq.b	#1, SP
	move.b	DATA, (MEMBASE,SP.w)
	subq.b	#1, SP
	swap	SP_CC
	or.b	#CC_B_6502, STATUS_6502(GLOBAL)	/* Set BRK flag			*/
	move.b	CC, DATA			/* Push P			*/
	and.w	#0x0F, DATA
	lea	CC_68K_TO_6502(GLOBAL), ADDR
	move.b	(ADDR,DATA.w), DATA
	or.b	STATUS_6502(GLOBAL), DATA
	swap	SP_CC				/* SP is high 16 bits of SP_CC	*/
	move.b	DATA, (MEMBASE,SP.w)
	subq.b	#1, SP
	swap	SP_CC
	or.b	#CC_I_6502, STATUS_6502(GLOBAL)	/* Set INT flag			*/
	move.l	#0xFFFE, DATA			/* Load PC from 0xFFFE		*/
 	move.w	(MEMBASE,DATA.l), DATA
 	ror.w	#8, DATA
	move.l	DATA, PC
	add.l	MEMBASE, PC
	NEXT_OP
JMP_ABS:
	move.b	1(PC), DATA
	lsl.w	#8, DATA
	move.b	(PC), DATA
	lea	(MEMBASE,DATA.l), PC
	NEXT_OP
JMP_IND:					/* 65C02 version		*/
	move.b	1(PC), DATA
	lsl.w	#8, DATA
	move.b	(PC), DATA
	lea	1(MEMBASE,DATA.l), ADDR
	move.b	(ADDR), DATA
	lsl.w	#8, DATA
	move.b	-(ADDR), DATA
	lea	(MEMBASE,DATA.l), PC
	NEXT_OP
JSR:
	swap	SP_CC				/* SP is high 16 bits of SP_CC	*/
	move.b	1(PC), DATA
	lsl.w	#8, DATA
	move.b	(PC)+, DATA
	move.l	DATA, ADDR
	sub.l	MEMBASE, PC
	move.w	PC, DATA
	move.l	ADDR, PC
	add.l	MEMBASE, PC
	ror.w	#8, DATA
	move.b	DATA, (MEMBASE,SP.w)
	lsr.w	#8, DATA
	subq.b	#1, SP
	move.b	DATA, (MEMBASE,SP.w)
	subq.b	#1, SP
	swap	SP_CC
	NEXT_OP
RTS:
	swap	SP_CC				/* SP is high 16 bits of SP_CC	*/
	addq.b	#1, SP
	move.b	(MEMBASE,SP.w), DATA
	addq.b	#1, SP
	lsl.w	#8, DATA
	move.b	(MEMBASE,SP.w), DATA
	rol.w	#8, DATA
	lea	1(MEMBASE,DATA.l), PC
	swap	SP_CC
	NEXT_OP
RTI:
	swap	SP_CC				/* SP is high 16 bits of SP_CC	*/
	addq.b	#1, SP
	move.b	(MEMBASE,SP.w), DATA
	swap	SP_CC
	clr.w	CC
	move.b	DATA, CC
	and.b	#0x3C, DATA			/* Mask out mapped state	*/
	move.b	DATA, STATUS_6502(GLOBAL)
	and.b	#0xC3, CC
	rol.b	#2, CC
	lea	CC_6502_TO_68K(GLOBAL), ADDR
	move.b	(ADDR,CC.w), CC
	bsr.w	set_bcd_ops
	swap	SP_CC
	addq.b	#1, SP
	move.b	(MEMBASE,SP.w), DATA
	addq.b	#1, SP
	lsl.w	#8, DATA
	move.b	(MEMBASE,SP.w), DATA
	rol.w	#8, DATA
	lea	(MEMBASE,DATA.l), PC
	swap	SP_CC
	NEXT_OP
/*
 * Special version of PC adjustment routines that check for specific ranges.
 * Very Apple ][ specific :-(
 */
BRK_CHK:
	movem.l #0x1FFF, STATE_6502(GLOBAL)
	bsr.w	updateLanguageCardMapping
	movem.l STATE_6502(GLOBAL), #0x1FFF
	sub.l	MEMBASE, PC			/* Push PC + 2			*/
	move.w	PC, DATA
	addq	#1, DATA
	ror.w	#8, DATA
	swap	SP_CC				/* SP is high 16 bits of SP_CC	*/
	move.b	DATA, (MEMBASE,SP.w)
	lsr.w	#8, DATA
	subq.b	#1, SP
	move.b	DATA, (MEMBASE,SP.w)
	subq.b	#1, SP
	swap	SP_CC
	or.b	#CC_B_6502, STATUS_6502(GLOBAL)	/* Set BRK flag			*/
	move.b	CC, DATA			/* Push P			*/
	and.w	#0x0F, DATA
	lea	CC_68K_TO_6502(GLOBAL), ADDR
	move.b	(ADDR,DATA.w), DATA
	or.b	STATUS_6502(GLOBAL), DATA
	swap	SP_CC				/* SP is high 16 bits of SP_CC	*/
	move.b	DATA, (MEMBASE,SP.w)
	subq.b	#1, SP
	swap	SP_CC
	or.b	#CC_I_6502, STATUS_6502(GLOBAL)	/* Set INT flag			*/
	move.l	#0xFFFE, DATA			/* Load PC from 0xFFFE		*/
	move.w	(MEMBASE,DATA.l), DATA
	ror.w	#8, DATA
	move.l	DATA, PC
	add.l	MEMBASE, PC
	NEXT_OP
JMP_ABS_CHK:
	move.b	1(PC), DATA
	lsl.w	#8, DATA
	move.b	(PC), DATA
	cmp.w	#0xD000, DATA
	bcs.b	0f
	movem.l #0x1FFF, STATE_6502(GLOBAL)
	bsr.w	updateLanguageCardMapping
	movem.l STATE_6502(GLOBAL), #0x1FFF
0:
	lea	(MEMBASE,DATA.l), PC
	NEXT_OP
JMP_IND_CHK:					/* 65C02 version		*/
	move.b	1(PC), DATA
	lsl.w	#8, DATA
	move.b	(PC), DATA
	cmp.w	#0xD000, DATA
	bcs.b	0f
	movem.l #0x1FFF, STATE_6502(GLOBAL)
	bsr.w	updateLanguageCardMapping
	movem.l STATE_6502(GLOBAL), #0x1FFF
0:
	lea	1(MEMBASE,DATA.l), ADDR
	move.b	(ADDR), DATA
	lsl.w	#8, DATA
	move.b	-(ADDR), DATA
	cmp.w	#0xD000, DATA
	bcs.b	1f
	movem.l #0x1FFF, STATE_6502(GLOBAL)
	bsr.w	updateLanguageCardMapping
	movem.l STATE_6502(GLOBAL), #0x1FFF
1:
	lea	(MEMBASE,DATA.l), PC
	NEXT_OP
JSR_CHK:
	swap	SP_CC				/* SP is high 16 bits of SP_CC	*/
	move.b	1(PC), DATA
	lsl.w	#8, DATA
	move.b	(PC)+, DATA
	cmp.w	#0xD000, DATA
	bcs.b	0f
	movem.l #0x1FFF, STATE_6502(GLOBAL)
	bsr.w	updateLanguageCardMapping
	movem.l STATE_6502(GLOBAL), #0x1FFF
0:
	move.l	DATA, ADDR
	sub.l	MEMBASE, PC
	move.w	PC, DATA
	move.l	ADDR, PC
	add.l	MEMBASE, PC
	ror.w	#8, DATA
	move.b	DATA, (MEMBASE,SP.w)
	lsr.w	#8, DATA
	subq.b	#1, SP
	move.b	DATA, (MEMBASE,SP.w)
	subq.b	#1, SP
	swap	SP_CC
	NEXT_OP
RTS_CHK:
	swap	SP_CC				/* SP is high 16 bits of SP_CC	*/
	addq.b	#1, SP
	move.b	(MEMBASE,SP.w), DATA
	addq.b	#1, SP
	lsl.w	#8, DATA
	move.b	(MEMBASE,SP.w), DATA
	rol.w	#8, DATA
	cmp.w	#0xEFFF, DATA
	bcs.b	0f
	movem.l #0x1FFF, STATE_6502(GLOBAL)
	bsr.w	updateLanguageCardMapping
	movem.l STATE_6502(GLOBAL), #0x1FFF
0:
	lea	1(MEMBASE,DATA.l), PC
	swap	SP_CC
	NEXT_OP
RTI_CHK:
	swap	SP_CC				/* SP is high 16 bits of SP_CC	*/
	addq.b	#1, SP
	move.b	(MEMBASE,SP.w), DATA
	swap	SP_CC
	clr.w	CC
	move.b	DATA, CC
	and.b	#0x3C, DATA			/* Mask out mapped state	*/
	move.b	DATA, STATUS_6502(GLOBAL)
	and.b	#0xC3, CC
	rol.b	#2, CC
	lea	CC_6502_TO_68K(GLOBAL), ADDR
	move.b	(ADDR,CC.w), CC
	bsr.w	set_bcd_ops
	swap	SP_CC
	addq.b	#1, SP
	move.b	(MEMBASE,SP.w), DATA
	addq.b	#1, SP
	lsl.w	#8, DATA
	move.b	(MEMBASE,SP.w), DATA
	rol.w	#8, DATA
	cmp.w	#0xD000, DATA
	bcs.b	0f
	movem.l #0x1FFF, STATE_6502(GLOBAL)
	bsr.w	updateLanguageCardMapping
	movem.l STATE_6502(GLOBAL), #0x1FFF
0:
	lea	(MEMBASE,DATA.l), PC
	swap	SP_CC
	NEXT_OP
BRA:
	move.b	(PC)+, DATA
	ext.w	DATA
	add.w	DATA, PC
	NEXT_OP
BCS:
	move.b	(PC)+, DATA
	btst	#0, CC
	beq.b	.+6
	ext.w	DATA
	add.w	DATA, PC
	NEXT_OP
BCC:
	move.b	(PC)+, DATA
	btst	#0, CC
	bne.b	.+6
	ext.w	DATA
	add.w	DATA, PC
	NEXT_OP
BEQ:
	move.b	(PC)+, DATA
	btst	#2, CC
	beq.b	.+6
	ext.w	DATA
	add.w	DATA, PC
	NEXT_OP
BNE:
	move.b	(PC)+, DATA
	btst	#2, CC
	bne.b	.+6
	ext.w	DATA
	add.w	DATA, PC
	NEXT_OP
BMI:
	move.b	(PC)+, DATA
	btst	#3, CC
	beq.b	.+6
	ext.w	DATA
	add.w	DATA, PC
	NEXT_OP
BPL:
	move.b	(PC)+, DATA
	btst	#3, CC
	bne.b	.+6
	ext.w	DATA
	add.w	DATA, PC
	NEXT_OP
BVS:
	move.b	(PC)+, DATA
	btst	#1, CC
	beq.b	.+6
	ext.w	DATA
	add.w	DATA, PC
	NEXT_OP
BVC:
	move.b	(PC)+, DATA
	btst	#1, CC
	bne.b	.+6
	ext.w	DATA
	add.w	DATA, PC
	NEXT_OP
LDA_IMM:
	move.b	(PC)+, ACC
	UPDATE_NZ_STATUS
	NEXT_OP
LDA_ZP:
	EA_ZP
	move.b	(MEMBASE,EA.w), ACC
	UPDATE_NZ_STATUS
	NEXT_OP
LDA_ZP_X:
	EA_ZP_X
	move.b	(MEMBASE,EA.w), ACC
	UPDATE_NZ_STATUS
	NEXT_OP
LDA_ZP_IND:
	EA_ZP_IND
	LDA_MEM
	UPDATE_NZ_STATUS
	NEXT_OP
LDA_IND_Y:
	EA_IND_Y
	LDA_MEM
	UPDATE_NZ_STATUS
	NEXT_OP
LDA_IND_X:
	EA_IND_X
	LDA_MEM
	UPDATE_NZ_STATUS
	NEXT_OP
LDA_ABS:
	EA_ABS
	LDA_MEM
	UPDATE_NZ_STATUS
	NEXT_OP
LDA_ABS_Y:
	EA_ABS_Y
	LDA_MEM
	UPDATE_NZ_STATUS
	NEXT_OP
LDA_ABS_X:
	EA_ABS_X
	LDA_MEM
	UPDATE_NZ_STATUS
	NEXT_OP
LDY_IMM:
	move.b	(PC)+, Y
	UPDATE_NZ_STATUS
	NEXT_OP
LDY_ZP:
	EA_ZP
	move.b	(MEMBASE,EA.w), Y
	UPDATE_NZ_STATUS
	NEXT_OP
LDY_ZP_X:
	EA_ZP_X
	move.b	(MEMBASE,EA.w), Y
	UPDATE_NZ_STATUS
	NEXT_OP
LDY_ABS:
	EA_ABS
	LDY_MEM
	UPDATE_NZ_STATUS
	NEXT_OP
LDY_ABS_X:
	EA_ABS_X
	LDY_MEM
	UPDATE_NZ_STATUS
	NEXT_OP
LDX_IMM:
	move.b	(PC)+, X
	UPDATE_NZ_STATUS
	NEXT_OP
LDX_ZP:
	EA_ZP
	move.b	(MEMBASE,EA.w), X
	UPDATE_NZ_STATUS
	NEXT_OP
LDX_ZP_Y:
	EA_ZP_Y
	move.b	(MEMBASE,EA.w), X
	UPDATE_NZ_STATUS
	NEXT_OP
LDX_ABS:
	EA_ABS
	LDX_MEM
	UPDATE_NZ_STATUS
	NEXT_OP
LDX_ABS_Y:
	EA_ABS_Y
	LDX_MEM
	UPDATE_NZ_STATUS
	NEXT_OP
STA_ZP:
	EA_ZP
	move.b	ACC, (MEMBASE,EA.w)
	NEXT_OP
STA_ZP_X:
	EA_ZP_X
	move.b	ACC, (MEMBASE,EA.w)
	NEXT_OP
STA_IND_Y:
	EA_IND_Y
	STA_MEM
	NEXT_OP
STA_IND_X:
	EA_IND_X
	STA_MEM
	NEXT_OP
STA_ABS:
	EA_ABS
	STA_MEM
	NEXT_OP
STA_ABS_Y:
	EA_ABS_Y
	STA_MEM
	NEXT_OP
STA_ABS_X:
	EA_ABS_X
	STA_MEM
	NEXT_OP
STY_ZP:
	EA_ZP
	move.b	Y, (MEMBASE,EA.w)
	NEXT_OP
STY_ZP_X:
	EA_ZP_X
	move.b	Y, (MEMBASE,EA.w)
	NEXT_OP
STY_ABS:
	EA_ABS
	STY_MEM
	NEXT_OP
STX_ZP_Y:
	EA_ZP_Y
	move.b	X, (MEMBASE,EA.w)
	NEXT_OP
STX_ZP:
	EA_ZP
	move.b	X, (MEMBASE,EA.w)
	NEXT_OP
STX_ABS:
	EA_ABS
	STX_MEM
	NEXT_OP
STZ_ZP:
	EA_ZP
	move.b	#0, (MEMBASE,EA.w)
	NEXT_OP
STZ_ABS:
	EA_ABS
	clr.w	DATA
	WR_MEM
	NEXT_OP
STZ_ABS_X:
	EA_ABS_X
	clr.w	DATA
	WR_MEM
	NEXT_OP
TAY:
	move.b	ACC, Y
	UPDATE_NZ_STATUS
	NEXT_OP
TYA:
	move.b	Y, ACC
	UPDATE_NZ_STATUS
	NEXT_OP
TAX:
	move.b	ACC, X
	UPDATE_NZ_STATUS
	NEXT_OP
TXA:
	move.b	X, ACC
	UPDATE_NZ_STATUS
	NEXT_OP
TXS:
	swap	SP_CC
	move.b	X, SP
	swap	SP_CC
	NEXT_OP
TSX:
	swap	SP_CC
	move.b	SP, DATA
	swap	SP_CC
	move.b	DATA, X
	UPDATE_NZ_STATUS
	NEXT_OP
BIT_IMM:
	EA_IMM
	OP_BIT
	NEXT_OP
BIT_ZP:
	EA_ZP
	move.b	(MEMBASE,EA.w), DATA
	OP_BIT
	NEXT_OP
BIT_ABS:
	EA_ABS
	RD_MEM
	OP_BIT
	NEXT_OP
TRB_ABS:
	EA_ABS
	RD_MEM
	OP_TRB
	WR_MEM
	NEXT_OP
TSB_ZP:
	EA_ZP
	move.b	(MEMBASE,EA.w), DATA
	OP_TSB
	move.b	DATA, (MEMBASE,EA.w)
	NEXT_OP
TSB_ABS:
	EA_ABS
	RD_MEM
	OP_TSB
	WR_MEM
	NEXT_OP
AND_IMM:
	and.b	(PC)+, ACC
	UPDATE_NZ_STATUS
	NEXT_OP
AND_ZP:
	EA_ZP
	and.b	(MEMBASE,EA.w), ACC
	UPDATE_NZ_STATUS
	NEXT_OP
AND_ZP_X:
	EA_ZP_X
	and.b	(MEMBASE,EA.w), ACC
	UPDATE_NZ_STATUS
	NEXT_OP
AND_ABS:
	EA_ABS
	RD_MEM
	and.b	DATA, ACC
	UPDATE_NZ_STATUS
	NEXT_OP
AND_ABS_Y:
	EA_ABS_Y
	RD_MEM
	and.b	DATA, ACC
	UPDATE_NZ_STATUS
	NEXT_OP
AND_ABS_X:
	EA_ABS_X
	RD_MEM
	and.b	DATA, ACC
	UPDATE_NZ_STATUS
	NEXT_OP
AND_IND_Y:
	EA_IND_Y
	RD_MEM
	and.b	DATA, ACC
	UPDATE_NZ_STATUS
	NEXT_OP
AND_IND_X:
	EA_IND_X
	RD_MEM
	and.b	DATA, ACC
	UPDATE_NZ_STATUS
	NEXT_OP
ORA_IMM:
	or.b	(PC)+, ACC
	UPDATE_NZ_STATUS
	NEXT_OP
ORA_ZP:
	EA_ZP
	or.b	(MEMBASE,EA.w), ACC
	UPDATE_NZ_STATUS
	NEXT_OP
ORA_ZP_X:
	EA_ZP_X
	or.b	(MEMBASE,EA.w), ACC
	UPDATE_NZ_STATUS
	NEXT_OP
ORA_ABS:
	EA_ABS
	RD_MEM
	or.b	DATA, ACC
	UPDATE_NZ_STATUS
	NEXT_OP
ORA_ABS_Y:
	EA_ABS_Y
	RD_MEM
	or.b	DATA, ACC
	UPDATE_NZ_STATUS
	NEXT_OP
ORA_ABS_X:
	EA_ABS_X
	RD_MEM
	or.b	DATA, ACC
	UPDATE_NZ_STATUS
	NEXT_OP
ORA_IND_Y:
	EA_IND_Y
	RD_MEM
	or.b	DATA, ACC
	UPDATE_NZ_STATUS
	NEXT_OP
ORA_IND_X:
	EA_IND_X
	RD_MEM
	or.b	DATA, ACC
	UPDATE_NZ_STATUS
	NEXT_OP
EOR_IMM:
	EA_IMM
	eor.b	DATA, ACC
	UPDATE_NZ_STATUS
	NEXT_OP
EOR_ZP:
	EA_ZP
	move.b	(MEMBASE,EA.w), DATA
	eor.b	DATA, ACC
	UPDATE_NZ_STATUS
	NEXT_OP
EOR_ZP_X:
	EA_ZP_X
	move.b	(MEMBASE,EA.w), DATA
	eor.b	DATA, ACC
	UPDATE_NZ_STATUS
	NEXT_OP
EOR_ABS:
	EA_ABS
	RD_MEM
	eor.b	DATA, ACC
	UPDATE_NZ_STATUS
	NEXT_OP
EOR_ABS_Y:
	EA_ABS_Y
	RD_MEM
	eor.b	DATA, ACC
	UPDATE_NZ_STATUS
	NEXT_OP
EOR_ABS_X:
	EA_ABS_X
	RD_MEM
	eor.b	DATA, ACC
	UPDATE_NZ_STATUS
	NEXT_OP
EOR_IND_Y:
	EA_IND_Y
	RD_MEM
	eor.b	DATA, ACC
	UPDATE_NZ_STATUS
	NEXT_OP
EOR_IND_X:
	EA_IND_X
	RD_MEM
	eor.b	DATA, ACC
	UPDATE_NZ_STATUS
	NEXT_OP
INC_ZP:
	EA_ZP
	addq.b	#1, (MEMBASE,EA.w)
	UPDATE_NZ_STATUS
	NEXT_OP
INC_ZP_X:
	EA_ZP_X
	addq.b	#1, (MEMBASE,EA.w)
	UPDATE_NZ_STATUS
	NEXT_OP
INC_ABS:
	EA_ABS
	RD_MEM
	addq.b	#1, DATA
	move.w	DATA, ADDR
	UPDATE_NZ_STATUS
	WRA_MEM
	NEXT_OP
INC_ABS_X:
	EA_ABS_X
	RD_MEM
	addq.b	#1, DATA
	move.w	DATA, ADDR
	UPDATE_NZ_STATUS
	WRA_MEM
	NEXT_OP
INA:
	addq.b	#1, ACC
	UPDATE_NZ_STATUS
	NEXT_OP	
INY:
	addq.b	#1, Y
	UPDATE_NZ_STATUS
	NEXT_OP	
INX:
	addq.b	#1, X
	UPDATE_NZ_STATUS
	NEXT_OP	
DEC_ZP:
	EA_ZP
	subq.b	#1, (MEMBASE,EA.w)
	UPDATE_NZ_STATUS
	NEXT_OP
DEC_ZP_X:
	EA_ZP_X
	subq.b	#1, (MEMBASE,EA.w)
	UPDATE_NZ_STATUS
	NEXT_OP
DEC_ABS:
	EA_ABS
	RD_MEM
	subq.b	#1, DATA
	move.w	DATA, ADDR
	UPDATE_NZ_STATUS
	WRA_MEM
	NEXT_OP
DEC_ABS_X:
	EA_ABS_X
	RD_MEM
	subq.b	#1, DATA
	move.w	DATA, ADDR
	UPDATE_NZ_STATUS
	WRA_MEM
	NEXT_OP
DEA:
	subq.b	#1, ACC
	UPDATE_NZ_STATUS
	NEXT_OP	
DEY:
	subq.b	#1, Y
	UPDATE_NZ_STATUS
	NEXT_OP	
DEX:
	subq.b	#1, X
	UPDATE_NZ_STATUS
	NEXT_OP	
ADC_BIN_IMM:
	EA_IMM
	OP_ADC_BIN
	NEXT_OP
ADC_BIN_ZP:
	EA_ZP
	move.b	(MEMBASE,EA.w), DATA
	OP_ADC_BIN
	NEXT_OP
ADC_BIN_ZP_X:
	EA_ZP_X
	move.b	(MEMBASE,EA.w), DATA
	OP_ADC_BIN
	NEXT_OP
ADC_BIN_ABS:
	EA_ABS
	RD_MEM
	OP_ADC_BIN
	NEXT_OP
ADC_BIN_ABS_X:
	EA_ABS_X
	RD_MEM
	OP_ADC_BIN
	NEXT_OP
ADC_BIN_ABS_Y:
	EA_ABS_Y
	RD_MEM
	OP_ADC_BIN
	NEXT_OP
ADC_BIN_IND_X:
	EA_IND_X
	RD_MEM
	OP_ADC_BIN
	NEXT_OP
ADC_BIN_IND_Y:
	EA_IND_Y
	RD_MEM
	OP_ADC_BIN
	NEXT_OP
ADC_BCD_IMM:
	EA_IMM
	OP_ADC_BCD
	NEXT_OP
ADC_BCD_ZP:
	EA_ZP
	move.b	(MEMBASE,EA.w), DATA
	OP_ADC_BCD
	NEXT_OP
ADC_BCD_ZP_X:
	EA_ZP_X
	move.b	(MEMBASE,EA.w), DATA
	OP_ADC_BCD
	NEXT_OP
ADC_BCD_ABS:
	EA_ABS
	RD_MEM
	OP_ADC_BCD
	NEXT_OP
ADC_BCD_ABS_X:
	EA_ABS_X
	RD_MEM
	OP_ADC_BCD
	NEXT_OP
ADC_BCD_ABS_Y:
	EA_ABS_Y
	RD_MEM
	OP_ADC_BCD
	NEXT_OP
ADC_BCD_IND_X:
	EA_IND_X
	RD_MEM
	OP_ADC_BCD
	NEXT_OP
ADC_BCD_IND_Y:
	EA_IND_Y
	RD_MEM
	OP_ADC_BCD
	NEXT_OP
SBC_BIN_IMM:
	EA_IMM
	OP_SBC_BIN
	NEXT_OP
SBC_BIN_ZP:
	EA_ZP
	move.b	(MEMBASE,EA.w), DATA
	OP_SBC_BIN
	NEXT_OP
SBC_BIN_ZP_X:
	EA_ZP_X
	move.b	(MEMBASE,EA.w), DATA
	OP_SBC_BIN
	NEXT_OP
SBC_BIN_ABS:
	EA_ABS
	RD_MEM
	OP_SBC_BIN
	NEXT_OP
SBC_BIN_ABS_X:
	EA_ABS_X
	RD_MEM
	OP_SBC_BIN
	NEXT_OP
SBC_BIN_ABS_Y:
	EA_ABS_Y
	RD_MEM
	OP_SBC_BIN
	NEXT_OP
SBC_BIN_IND_X:
	EA_IND_X
	RD_MEM
	OP_SBC_BIN
	NEXT_OP
SBC_BIN_IND_Y:
	EA_IND_Y
	RD_MEM
	OP_SBC_BIN
	NEXT_OP
SBC_BCD_IMM:
	EA_IMM
	OP_SBC_BCD
	NEXT_OP
SBC_BCD_ZP:
	EA_ZP
	move.b	(MEMBASE,EA.w), DATA
	OP_SBC_BCD
	NEXT_OP
SBC_BCD_ZP_X:
	EA_ZP_X
	move.b	(MEMBASE,EA.w), DATA
	OP_SBC_BCD
	NEXT_OP
SBC_BCD_ABS:
	EA_ABS
	RD_MEM
	OP_SBC_BCD
	NEXT_OP
SBC_BCD_ABS_X:
	EA_ABS_X
	RD_MEM
	OP_SBC_BCD
	NEXT_OP
SBC_BCD_ABS_Y:
	EA_ABS_Y
	RD_MEM
	OP_SBC_BCD
	NEXT_OP
SBC_BCD_IND_X:
	EA_IND_X
	RD_MEM
	OP_SBC_BCD
	NEXT_OP
SBC_BCD_IND_Y:
	EA_IND_Y
	RD_MEM
	OP_SBC_BCD
	NEXT_OP
CMP_IMM:
	move.w	ACC, DATA
	sub.b	(PC)+, DATA
	UPDATE_NZC_STATUS
	eor.b	#0x11, CC
	NEXT_OP
CMP_ZP:
	EA_ZP
	move.w	ACC, DATA
	sub.b	(MEMBASE,EA.w), DATA
	UPDATE_NZC_STATUS
	eor.b	#0x11, CC
	NEXT_OP
CMP_ZP_IND:
	EA_ZP_IND
	RD_MEM
	move.w	ACC, ADDR
	sub.b	DATA, ACC
	UPDATE_NZC_STATUS
	eor.b	#0x11, CC
	move.w	ADDR, ACC
	NEXT_OP
CMP_ZP_X:
	EA_ZP_X
	move.w	ACC, DATA
	sub.b	(MEMBASE,EA.w), DATA
	UPDATE_NZC_STATUS
	eor.b	#0x11, CC
	NEXT_OP
CMP_IND_Y:
	EA_IND_Y
	RD_MEM
	move.w	ACC, ADDR
	sub.b	DATA, ACC
	UPDATE_NZC_STATUS
	eor.b	#0x11, CC
	move.w	ADDR, ACC
	NEXT_OP
CMP_IND_X:
	EA_IND_X
	RD_MEM
	move.w	ACC, ADDR
	sub.b	DATA, ACC
	UPDATE_NZC_STATUS
	eor.b	#0x11, CC
	move.w	ADDR, ACC
	NEXT_OP
CMP_ABS:
	EA_ABS
	RD_MEM
	move.w	ACC, ADDR
	sub.b	DATA, ACC
	UPDATE_NZC_STATUS
	eor.b	#0x11, CC
	move.w	ADDR, ACC
	NEXT_OP
CMP_ABS_Y:
	EA_ABS_Y
	RD_MEM
	move.w	ACC, ADDR
	sub.b	DATA, ACC
	UPDATE_NZC_STATUS
	eor.b	#0x11, CC
	move.w	ADDR, ACC
	NEXT_OP
CMP_ABS_X:
	EA_ABS_X
	RD_MEM
	move.w	ACC, ADDR
	sub.b	DATA, ACC
	UPDATE_NZC_STATUS
	eor.b	#0x11, CC
	move.w	ADDR, ACC
	NEXT_OP
CPY_IMM:
	move.w	Y, DATA
	sub.b	(PC)+, DATA
	UPDATE_NZC_STATUS
	eor.b	#0x11, CC
	NEXT_OP
CPY_ZP:
	EA_ZP
	move.w	Y, DATA
	sub.b	(MEMBASE,EA.w), DATA
	UPDATE_NZC_STATUS
	eor.b	#0x11, CC
	NEXT_OP
CPY_ABS:
	EA_ABS
	RD_MEM
	move.w	Y, ADDR
	sub.b	DATA, Y
	UPDATE_NZC_STATUS
	eor.b	#0x11, CC
	move.w	ADDR, Y
	NEXT_OP
CPX_IMM:
	move.w	X, DATA
	sub.b	(PC)+, DATA
	UPDATE_NZC_STATUS
	eor.b	#0x11, CC
	NEXT_OP
CPX_ZP:
	EA_ZP
	move.w	X, DATA
	sub.b	(MEMBASE,EA.w), DATA
	UPDATE_NZC_STATUS
	eor.b	#0x11, CC
	NEXT_OP
CPX_ABS:
	EA_ABS
	RD_MEM
	move.w	X, ADDR
	sub.b	DATA, X
	UPDATE_NZC_STATUS
	eor.b	#0x11, CC
	move.w	ADDR, X
	NEXT_OP
ASL_ACC:
	asl.b	#1, ACC
	UPDATE_NZC_STATUS
	NEXT_OP	
ASL_ZP:
	EA_ZP
	move.b	(MEMBASE,EA.w), DATA
	asl.b	#1, DATA
	move.w	DATA, ADDR
	UPDATE_NZC_STATUS
	move.w	ADDR, DATA
	move.b	DATA, (MEMBASE,EA.w)
	NEXT_OP	
ASL_ZP_X:
	EA_ZP_X
	move.b	(MEMBASE,EA.w), DATA
	asl.b	#1, DATA
	move.w	DATA, ADDR
	UPDATE_NZC_STATUS
	move.w	ADDR, DATA
	move.b	DATA, (MEMBASE,EA.w)
	NEXT_OP	
ASL_ABS:
	EA_ABS
	RD_MEM
	asl.b	#1, DATA
	move.w	DATA, ADDR
	UPDATE_NZC_STATUS
	WRA_MEM
	NEXT_OP
ASL_ABS_X:
	EA_ABS_X
	RD_MEM
	asl.b	#1, DATA
	move.w	DATA, ADDR
	UPDATE_NZC_STATUS
	WRA_MEM
	NEXT_OP
LSR_ACC:
	lsr.b	#1, ACC
	UPDATE_NZC_STATUS
	NEXT_OP	
LSR_ZP:
	EA_ZP
	move.b	(MEMBASE,EA.w), DATA
	lsr.b	#1, DATA
	move.w	DATA, ADDR
	UPDATE_NZC_STATUS
	move.w	ADDR, DATA
	move.b	DATA, (MEMBASE,EA.w)
	NEXT_OP	
LSR_ZP_X:
	EA_ZP_X
	move.b	(MEMBASE,EA.w), DATA
	lsr.b	#1, DATA
	move.w	DATA, ADDR
	UPDATE_NZC_STATUS
	move.w	ADDR, DATA
	move.b	DATA, (MEMBASE,EA.w)
	NEXT_OP	
LSR_ABS:
	EA_ABS
	RD_MEM
        lsr.b	#1, DATA
	move.w	DATA, ADDR
        UPDATE_NZC_STATUS
	WRA_MEM
        NEXT_OP
LSR_ABS_X:
	EA_ABS_X
	RD_MEM
        lsr.b	#1, DATA
	move.w	DATA, ADDR
        UPDATE_NZC_STATUS
	WRA_MEM
        NEXT_OP
ROL_ACC:
	move.w	CC, %ccr
	roxl.b	#1, ACC
	UPDATE_NZC_STATUS
	NEXT_OP	
ROL_ZP:
	EA_ZP
	move.b	(MEMBASE,EA.w), DATA
	move.w	CC, %ccr
	roxl.b	#1, DATA
	move.w	DATA, ADDR
	UPDATE_NZC_STATUS
	move.w	ADDR, DATA
	move.b	DATA, (MEMBASE,EA.w)
	NEXT_OP	
ROL_ZP_X:
	EA_ZP_X
	move.b	(MEMBASE,EA.w), DATA
	move.w	CC, %ccr
	roxl.b	#1, DATA
	move.w	DATA, ADDR
	UPDATE_NZC_STATUS
	move.w	ADDR, DATA
	move.b	DATA, (MEMBASE,EA.w)
	NEXT_OP	
ROL_ABS:
	EA_ABS
	RD_MEM
	move.w	CC, %ccr
        roxl.b	#1, DATA
	move.w	DATA, ADDR
        UPDATE_NZC_STATUS
	WRA_MEM
        NEXT_OP
ROL_ABS_X:
	EA_ABS_X
	RD_MEM
	move.w	CC, %ccr
        roxl.b	#1, DATA
	move.w	DATA, ADDR
        UPDATE_NZC_STATUS
	WRA_MEM
        NEXT_OP
ROR_ACC:
	move.w	CC, %ccr
	roxr.b	#1, ACC
	UPDATE_NZC_STATUS
	NEXT_OP	
ROR_ZP:
	EA_ZP
	move.b	(MEMBASE,EA.w), DATA
	move.w	CC, %ccr
	roxr.b	#1, DATA
	move.w	DATA, ADDR
	UPDATE_NZC_STATUS
	move.w	ADDR, DATA
	move.b	DATA, (MEMBASE,EA.w)
	NEXT_OP	
ROR_ZP_X:
	EA_ZP_X
	move.b	(MEMBASE,EA.w), DATA
	move.w	CC, %ccr
	roxr.b	#1, DATA
	move.w	DATA, ADDR
	UPDATE_NZC_STATUS
	move.w	ADDR, DATA
	move.b	DATA, (MEMBASE,EA.w)
	NEXT_OP	
ROR_ABS:
	EA_ABS
	RD_MEM
	move.w	CC, %ccr
        roxr.b	#1, DATA
	move.w	DATA, ADDR
        UPDATE_NZC_STATUS
	WRA_MEM
        NEXT_OP
ROR_ABS_X:
	EA_ABS_X
	RD_MEM
	move.w	CC, %ccr
        roxr.b	#1, DATA
	move.w	DATA, ADDR
        UPDATE_NZC_STATUS
	WRA_MEM
        NEXT_OP
/*
 * Execute 6502 instructions for # of clocks.
 */
.global run6502
run6502:
	movem.l	#0x7FFE, -(%sp)			/* Save all registers		*/
	movem.l	STATE_6502_2(GLOBAL), #0x1FFC	/* Load 6502 state		*/
	clr.l	DATA
	sub.l	ADDR, ADDR
	move.l	60(%sp), INSTRNS		/* Load instruction count	*/
	NEXT_OP
opfetch:
	clr.w	DATA 
	move.b	(PC)+, DATA			/* Load opcode			*/
	add.w	DATA, DATA
	move.w	(OPBASE, DATA.w), ADDR
	jmp	2(%pc,ADDR.w)
exit6502:					/* This MUST follow the previous*/
	movem.l	#0x1FFC, STATE_6502_2(GLOBAL)	/* jmp from opfetch		*/
	movem.l	(%sp)+, #0x7FFE
	clr.l	%d0
	rts

#define	OP_OFFSET(op)	((op)-exit6502)

/*
 * Initialize the CPU state and opcode arrays.
 */
.globl reset6502
reset6502:
	movem.l	#0xFFFA, -(%sp)			/* Save all registers		*/
	/*
	 * Init CPU state.
	 */
 	move.l	60(%sp), MEMBASE		/* Load Pointer to memory	*/
	move.l	#0xFFFC, %d0
 	move.w	(MEMBASE,%d0.l), %d0
 	rol.w	#8, %d0
	move.l	%d0, PC
	add.l	MEMBASE, PC
 	lea	OPTABLE(0)(GLOBAL), OPBASE
 	move.l	64(%sp), WRMEM
 	move.l	68(%sp), RDMEM
	move.b	#CC_1_6502, STATUS_6502(GLOBAL)
	clr.l	ACC
	clr.l	X
	clr.l	Y
	move.l	#0x01FF0000, SP_CC
	clr.l	EA
	clr.l	PAGE
	movem.l	#0x1FFC, STATE_6502_2(GLOBAL)	/* Save 6502 state		*/
	lea	OPTABLE(0)(GLOBAL), %a0
	/*
	 * Set ADC and SBC based on DECIMAL flag.
	 */
	bsr	set_bcd_ops
	movem.l	(%sp)+, #0x5FFF			/* Restore all registers	*/
	rts
/*
 * Update the opcode pointers based on the BCD decimal flag.
 */
set_bcd_ops:
	btst	#3, STATUS_6502(GLOBAL)
	bne	bcd_ops
	move.w	#OP_OFFSET(ADC_BIN_IMM),	0x69*2(OPBASE)
	move.w	#OP_OFFSET(ADC_BIN_ZP),		0x65*2(OPBASE)
	move.w	#OP_OFFSET(ADC_BIN_ZP_X), 	0x75*2(OPBASE)
	move.w	#OP_OFFSET(ADC_BIN_ABS), 	0x6D*2(OPBASE)
	move.w	#OP_OFFSET(ADC_BIN_ABS_X), 	0x7D*2(OPBASE)
	move.w	#OP_OFFSET(ADC_BIN_ABS_Y), 	0x79*2(OPBASE)
	move.w	#OP_OFFSET(ADC_BIN_IND_X), 	0x61*2(OPBASE)
	move.w	#OP_OFFSET(ADC_BIN_IND_Y), 	0x71*2(OPBASE)
	move.w	#OP_OFFSET(SBC_BIN_IMM), 	0xE9*2(OPBASE)
	move.w	#OP_OFFSET(SBC_BIN_ZP), 	0xE5*2(OPBASE)
	move.w	#OP_OFFSET(SBC_BIN_ZP_X), 	0xF5*2(OPBASE)
	move.w	#OP_OFFSET(SBC_BIN_ABS), 	0xED*2(OPBASE)
	move.w	#OP_OFFSET(SBC_BIN_ABS_X), 	0xFD*2(OPBASE)
	move.w	#OP_OFFSET(SBC_BIN_ABS_Y), 	0xF9*2(OPBASE)
	move.w	#OP_OFFSET(SBC_BIN_IND_X), 	0xE1*2(OPBASE)
	move.w	#OP_OFFSET(SBC_BIN_IND_Y), 	0xF1*2(OPBASE)
	rts
bcd_ops:
	move.w	#OP_OFFSET(ADC_BCD_IMM), 	0x69*2(OPBASE)
	move.w	#OP_OFFSET(ADC_BCD_ZP), 	0x65*2(OPBASE)
	move.w	#OP_OFFSET(ADC_BCD_ZP_X), 	0x75*2(OPBASE)
	move.w	#OP_OFFSET(ADC_BCD_ABS), 	0x6D*2(OPBASE)
	move.w	#OP_OFFSET(ADC_BCD_ABS_X), 	0x7D*2(OPBASE)
	move.w	#OP_OFFSET(ADC_BCD_ABS_Y), 	0x79*2(OPBASE)
	move.w	#OP_OFFSET(ADC_BCD_IND_X), 	0x61*2(OPBASE)
	move.w	#OP_OFFSET(ADC_BCD_IND_Y), 	0x71*2(OPBASE)
	move.w	#OP_OFFSET(SBC_BCD_IMM), 	0xE9*2(OPBASE)
	move.w	#OP_OFFSET(SBC_BCD_ZP), 	0xE5*2(OPBASE)
	move.w	#OP_OFFSET(SBC_BCD_ZP_X), 	0xF5*2(OPBASE)
	move.w	#OP_OFFSET(SBC_BCD_ABS), 	0xED*2(OPBASE)
	move.w	#OP_OFFSET(SBC_BCD_ABS_X), 	0xFD*2(OPBASE)
	move.w	#OP_OFFSET(SBC_BCD_ABS_Y), 	0xF9*2(OPBASE)
	move.w	#OP_OFFSET(SBC_BCD_IND_X), 	0xE1*2(OPBASE)
	move.w	#OP_OFFSET(SBC_BCD_IND_Y), 	0xF1*2(OPBASE)
	rts
/*
 * Update to checked Program Counter range.
 */
.global setPcCheckFuncs
setPcCheckFuncs:
	move.w	#OP_OFFSET(BRK_CHK),		OPTABLE(0x00*2)(GLOBAL)
	move.w	#OP_OFFSET(JSR_CHK),		OPTABLE(0x20*2)(GLOBAL)
	move.w	#OP_OFFSET(RTI_CHK),		OPTABLE(0x40*2)(GLOBAL)
	move.w	#OP_OFFSET(JMP_ABS_CHK),	OPTABLE(0x4C*2)(GLOBAL)
	move.w	#OP_OFFSET(RTS_CHK),		OPTABLE(0x60*2)(GLOBAL)
	move.w	#OP_OFFSET(JMP_IND_CHK),	OPTABLE(0x6C*2)(GLOBAL)
	rts
/*
 * Update to unchecked Program Counter range.
 */
.global clrPcCheckFuncs
clrPcCheckFuncs:
	move.w	#OP_OFFSET(BRK),		OPTABLE(0x00*2)(GLOBAL)
	move.w	#OP_OFFSET(JSR),		OPTABLE(0x20*2)(GLOBAL)
	move.w	#OP_OFFSET(RTI),		OPTABLE(0x40*2)(GLOBAL)
	move.w	#OP_OFFSET(JMP_ABS),		OPTABLE(0x4C*2)(GLOBAL)
	move.w	#OP_OFFSET(RTS),		OPTABLE(0x60*2)(GLOBAL)
	move.w	#OP_OFFSET(JMP_IND),		OPTABLE(0x6C*2)(GLOBAL)
	rts

.data
.even
.global opTable6502
opTable6502:
	/*
	 * 0x00 - 0x0F
	 */
  	.word OP_OFFSET(BRK)
	.word OP_OFFSET(ORA_IND_X)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(TSB_ZP)
	.word OP_OFFSET(ORA_ZP)
	.word OP_OFFSET(ASL_ZP)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(PHP)
	.word OP_OFFSET(ORA_IMM)
	.word OP_OFFSET(ASL_ACC)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(TSB_ABS)
	.word OP_OFFSET(ORA_ABS)
	.word OP_OFFSET(ASL_ABS)
	.word OP_OFFSET(UNIMPLEMENTED)
	/*
	 * 0x10 - 0x1F
	 */
	.word OP_OFFSET(BPL)
	.word OP_OFFSET(ORA_IND_Y)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(ORA_ZP_X)
	.word OP_OFFSET(ASL_ZP_X)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(CLC)
	.word OP_OFFSET(ORA_ABS_Y)
	.word OP_OFFSET(INA)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(TRB_ABS)
	.word OP_OFFSET(ORA_ABS_X)
	.word OP_OFFSET(ASL_ABS_X)
	.word OP_OFFSET(UNIMPLEMENTED)
	/*
	 * 0x20 - 0x2F
	 */
	.word OP_OFFSET(JSR)
	.word OP_OFFSET(AND_IND_X)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(BIT_ZP)
	.word OP_OFFSET(AND_ZP)
	.word OP_OFFSET(ROL_ZP)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(PLP)
	.word OP_OFFSET(AND_IMM)
	.word OP_OFFSET(ROL_ACC)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(BIT_ABS)
	.word OP_OFFSET(AND_ABS)
	.word OP_OFFSET(ROL_ABS)
	.word OP_OFFSET(UNIMPLEMENTED)
	/*
	 * 0x30 - 0x3F
	 */
	.word OP_OFFSET(BMI)
	.word OP_OFFSET(AND_IND_Y)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(AND_ZP_X)
	.word OP_OFFSET(ROL_ZP_X)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(SEC)
	.word OP_OFFSET(AND_ABS_Y)
	.word OP_OFFSET(DEA)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(AND_ABS_X)
	.word OP_OFFSET(ROL_ABS_X)
	.word OP_OFFSET(UNIMPLEMENTED)
	/*
	 * 0x40 - 0x4F
	 */
	.word OP_OFFSET(RTI)
	.word OP_OFFSET(EOR_IND_X)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(EOR_ZP)
	.word OP_OFFSET(LSR_ZP)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(PHA)
	.word OP_OFFSET(EOR_IMM)
	.word OP_OFFSET(LSR_ACC)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(JMP_ABS)
	.word OP_OFFSET(EOR_ABS)
	.word OP_OFFSET(LSR_ABS)
	.word OP_OFFSET(UNIMPLEMENTED)
	/*
	 * 0x50 - 0x5F
	 */
	.word OP_OFFSET(BVC)
	.word OP_OFFSET(EOR_IND_Y)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(EOR_ZP_X)
	.word OP_OFFSET(LSR_ZP_X)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(CLI)
	.word OP_OFFSET(EOR_ABS_Y)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(EOR_ABS_X)
	.word OP_OFFSET(LSR_ABS_X)
	.word OP_OFFSET(UNIMPLEMENTED)
	/*
	 * 0x60 - 0x6F
	 */
	.word OP_OFFSET(RTS)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(STZ_ZP)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(ROR_ZP)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(PLA)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(ROR_ACC)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(JMP_IND)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(ROR_ABS)
	.word OP_OFFSET(UNIMPLEMENTED)
	/*
	 * 0x70 - 0x7F
	 */
	.word OP_OFFSET(BVS)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(ROR_ZP_X)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(SEI)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(ROR_ABS_X)
	.word OP_OFFSET(UNIMPLEMENTED)
	/*
	 * 0x80 - 0x8F
	 */
	.word OP_OFFSET(BRA)
	.word OP_OFFSET(STA_IND_X)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(STY_ZP)
	.word OP_OFFSET(STA_ZP)
	.word OP_OFFSET(STX_ZP)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(DEY)
	.word OP_OFFSET(BIT_IMM)
	.word OP_OFFSET(TXA)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(STY_ABS)
	.word OP_OFFSET(STA_ABS)
	.word OP_OFFSET(STX_ABS)
	.word OP_OFFSET(UNIMPLEMENTED)
	/*
	 * 0x90 - 0x9F
	 */
	.word OP_OFFSET(BCC)
	.word OP_OFFSET(STA_IND_Y)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(STY_ZP_X)
	.word OP_OFFSET(STA_ZP_X)
	.word OP_OFFSET(STX_ZP_Y)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(TYA)
	.word OP_OFFSET(STA_ABS_Y)
	.word OP_OFFSET(TXS)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(STZ_ABS)
	.word OP_OFFSET(STA_ABS_X)
	.word OP_OFFSET(STZ_ABS_X)
	.word OP_OFFSET(UNIMPLEMENTED)
	/*
	 * 0xA0 - 0xAF
	 */
	.word OP_OFFSET(LDY_IMM)
	.word OP_OFFSET(LDA_IND_X)
	.word OP_OFFSET(LDX_IMM)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(LDY_ZP)
	.word OP_OFFSET(LDA_ZP)
	.word OP_OFFSET(LDX_ZP)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(TAY)
	.word OP_OFFSET(LDA_IMM)
	.word OP_OFFSET(TAX)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(LDY_ABS)
	.word OP_OFFSET(LDA_ABS)
	.word OP_OFFSET(LDX_ABS)
	.word exit6502-UNIMPLEMENTED
	/*
	 * 0xB0 - 0xBF
	 */
	.word OP_OFFSET(BCS)
	.word OP_OFFSET(LDA_IND_Y)
	.word OP_OFFSET(LDA_ZP_IND)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(LDY_ZP_X)
	.word OP_OFFSET(LDA_ZP_X)
	.word OP_OFFSET(LDX_ZP_Y)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(CLV)
	.word OP_OFFSET(LDA_ABS_Y)
	.word OP_OFFSET(TSX)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(LDY_ABS_X)
	.word OP_OFFSET(LDA_ABS_X)
	.word OP_OFFSET(LDX_ABS_Y)
	.word OP_OFFSET(UNIMPLEMENTED)
	/*
	 * 0xC0 - 0xCF
	 */
	.word OP_OFFSET(CPY_IMM)
	.word OP_OFFSET(CMP_IND_X)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(CPY_ZP)
	.word OP_OFFSET(CMP_ZP)
	.word OP_OFFSET(DEC_ZP)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(INY)
	.word OP_OFFSET(CMP_IMM)
	.word OP_OFFSET(DEX)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(CPY_ABS)
	.word OP_OFFSET(CMP_ABS)
	.word OP_OFFSET(DEC_ABS)
	.word OP_OFFSET(UNIMPLEMENTED)
	/*
	 * 0xD0 - 0xDF
	 */
	.word OP_OFFSET(BNE)
	.word OP_OFFSET(CMP_IND_Y)
	.word OP_OFFSET(CMP_ZP_IND)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(CMP_ZP_X)
	.word OP_OFFSET(DEC_ZP_X)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(CLD)
	.word OP_OFFSET(CMP_ABS_Y)
	.word OP_OFFSET(PHX)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(CMP_ABS_X)
	.word OP_OFFSET(DEC_ABS_X)
	.word OP_OFFSET(UNIMPLEMENTED)
	/*
	 * 0xE0 - 0xEF
	 */
	.word OP_OFFSET(CPX_IMM)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(CPX_ZP)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(INC_ZP)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(INX)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(NOP)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(CPX_ABS)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(INC_ABS)
	.word OP_OFFSET(UNIMPLEMENTED)
	/*
	 * 0xF0 - 0xFF
	 */
	.word OP_OFFSET(BEQ)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(INC_ZP_X)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(SED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(PLX)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(UNIMPLEMENTED)
	.word OP_OFFSET(INC_ABS_X)
	.word OP_OFFSET(UNIMPLEMENTED)
.even
.comm state6502, 13*4
.comm status6502, 1

