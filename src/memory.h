typedef UInt8 (*READBYTE)(UInt16);
typedef void (*WRITEBYTE)(UInt16, UInt8);
typedef void (*VIDIMAGE)(void);
extern Boolean dirtyVideo;
extern UInt8 *AppleMemory;
extern UInt8 *AuxMemory;
extern UInt8 *AppleROM;
extern UInt8 *BootROM;
extern READBYTE  *ReadFunction;
extern WRITEBYTE *WriteFunction;
extern VIDIMAGE updateVideo;
extern UInt16 clrPairEven[96], clrPairOdd[96], interpPairEven[96], interpPairOdd[96];

void initMemory(void);
void initVideo(void);
void setMemFuncs(void);
void setVideoFuncs(Boolean);
void loadROM(void);
int  getCurrentDrive(void);
void showCurrentDisk(void);
int  mountDisk(int, char *, Boolean);
void umountDisk(int);
void positionDisk(int, UInt16, UInt16);
void queryDisk(int, UInt16 *, UInt16 *);
void resetDisks(void);
/*
 * Macros to wrap a C function for assembly linkeage to emulator
 * memory R/W routines.
 */
#define WRMEM_CWRAP(fn,a,d)                         \
void fn(UInt16,UInt8);                              \
void cwrap_##fn(UInt16,UInt8);                      \
void awrap_##fn(UInt16 a, UInt8 d)                  \
{                                                   \
    asm(".global "#fn);                             \
    asm(#fn ":");                                   \
	asm("movem.l #0x1FFF, state6502@END.w(%a5)");   \
	asm("move.b  %d0, -(%sp)");                     \
	asm("move.w  %d6, -(%sp)");                     \
	asm("bsr.w   cwrap_"#fn);                       \
	asm("addq.l  #4, %sp");                         \
	asm("movem.l state6502@END.w(%a5), #0x1FFF");   \
	asm("rts");                                     \
}                                                   \
void cwrap_##fn(UInt16 a, UInt8 d)

#define RDMEM_CWRAP(fn,a)                           \
UInt8 fn(UInt16);                                   \
UInt8 cwrap_##fn(UInt16);                           \
UInt8 awrap_##fn(UInt16 a)                          \
{                                                   \
    asm(".global "#fn);                             \
    asm(#fn ":");                                   \
	asm("movem.l #0x1FFE, (state6502+4)@END.w(%a5)");\
	asm("move.w  %d6, -(%sp)");                     \
	asm("bsr.w   cwrap_"#fn);                       \
	asm("addq.l  #2, %sp");                         \
    asm("and.l   #0xFF,%d0");                       \
	asm("movem.l (state6502+4)@END.w(%a5), #0x1FFE");\
	asm("rts");                                     \
}                                                   \
UInt8 cwrap_##fn(UInt16 a)

/*
 * Keyboard type-ahead buffer.
 */
#define KBD_BUF_SIZE        16
#define KBD_BUF_MASK        15
extern UInt8 kbdBuffer[KBD_BUF_SIZE];
extern UInt8 kbdHead, kbdCount;

