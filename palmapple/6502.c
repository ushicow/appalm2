#include "6502.h"
#include "iou.h"
#include "memory.h"

extern READBYTE  ReadFunction[256];
extern WRITEBYTE WriteFunction[256];
extern UInt8     *AppleMemory;
extern UInt8     *AuxMemory;
extern UInt8     *AppleROM;
extern UInt16    *IOUState;

extern UInt16 IOUSoftSwitch[2];

#define LOW_BYTE  1
#define HIGH_BYTE 0

// Macro for the Memory Read Operation

#define INSTRUCTION_FETCH(opcode) opcode = _AppleMemory[PC.PC]; asm("and.l #0xFFFF, %2\n\tmove.b 2(%1, %2.l), %0\n\tlsl.w #0x08, %0\n\tmove.b 1(%1, %2.l), %0" : : "d" (PreFetch), "a" (_AppleMemory), "d" (PC.PC)); PC.PC++;

#define MEMORY_READ(value, address)  TempFuncRead = ReadFunction[address.Byte[HIGH_BYTE]]; if (TempFuncRead) { Address.PC = address.PC; value = TempFuncRead(); } else { value = _AppleMemory[address.PC]; }

#define MEMORY_WRITE(value, address)  TempFuncWrite = WriteFunction[address.Byte[HIGH_BYTE]]; if (TempFuncWrite) { Address.PC = address.PC; TempFuncWrite(value); } else _AppleMemory[address.PC] = value;

#define ZERO_PAGE_READ(addr)        _AppleMemory[addr.Byte[LOW_BYTE]]
#define ZERO_PAGE_WRITE(addr, data) _AppleMemory[addr.Byte[LOW_BYTE]] = data;

#define PUSH(X) _AppleMemory[Stack.PC] = X; Stack.Byte[LOW_BYTE]--;
#define POP(X)  Stack.Byte[LOW_BYTE]++; X = _AppleMemory[Stack.PC];

// #define eaimm(value) INSTRUCTION_FETCH(value);
#define eaimm(value) PC.PC++; value = PreFetch & 0xFF;

// #define eazpx(value) INSTRUCTION_FETCH(value); value = value + X;
#define eazpx(value) PC.PC++; value = PreFetch & 0xFF + X;

/* #define mr_eazpx(value) eazpx(TempByte); value = _AppleMemory[TempByte]; */
/* #define mw_eazpx(value) eazpx(TempByte); _AppleMemory[TempByte] = value; */
#define mr_eazpx(value) PC.PC++; TempByte = (PreFetch & 0xFF) + X; value = _AppleMemory[TempByte];
#define mw_eazpx(value) PC.PC++; TempByte = (PreFetch & 0xFF) + X; _AppleMemory[TempByte] = value;

// #define eazpy(value) INSTRUCTION_FETCH(TempByte); value = value + Y;
#define eazpy(value) PC.PC++; value = PreFetch & 0xFF + Y;

/* #define mr_eazpy(value) eazpy(TempByte); value = _AppleMemory[TempByte]; */
/* #define mw_eazpy(value) eazpy(TempByte); _AppleMemory[TempByte] = value; */
#define mr_eazpy(value) PC.PC++; TempByte = PreFetch & 0xFF + Y; value = _AppleMemory[TempByte];
#define mw_eazpy(value) PC.PC++; TempByte = PreFetch & 0xFF + Y; _AppleMemory[TempByte] = value;

// #define eaabs(value) INSTRUCTION_FETCH(value.Byte[LOW_BYTE]); INSTRUCTION_FETCH(value.Byte[HIGH_BYTE]);

#define eaabs(value) PC.PC+=2; value.PC = PreFetch;
#define mr_eaabs(value) PC.PC+=2; TempWord.PC = PreFetch; MEMORY_READ(value, TempWord);
#define mw_eaabs(value) PC.PC+=2; TempWord.PC = PreFetch; MEMORY_WRITE(value, TempWord);

#define eaabsind() INSTRUCTION_FETCH(TempWord.Byte[LOW_BYTE]); \
INSTRUCTION_FETCH(TempWord.Byte[HIGH_BYTE]); MEMORY_READ(PC.Byte[LOW_BYTE], TempWord); \
TempWord.PC += 1; MEMORY_READ(PC.Byte[HIGH_BYTE], TempWord);

#define eaabsx(value) eaabs(value); value.PC += X;
#define mr_eaabsx(value) eaabsx(TempWord); MEMORY_READ(value, TempWord);
#define mw_eaabsx(value) eaabsx(TempWord); MEMORY_WRITE(value, TempWord);

#define eaabsy(value) eaabs(value); value.PC += Y;
#define mr_eaabsy(value) eaabsy(TempWord); MEMORY_READ(value, TempWord);
#define mw_eaabsy(value) eaabsy(TempWord); MEMORY_WRITE(value, TempWord);

#define earel(value) INSTRUCTION_FETCH(value.Byte[LOW_BYTE]); asm("ext.w %0" : : "d" (value.PC));

#define eazp(value) PC.PC++; TempByte = PreFetch & 0xFF; value = _AppleMemory[TempByte];
#define mw_eazp(value) PC.PC++; TempByte = PreFetch & 0xFF; _AppleMemory[TempByte] = value;

#define eazpxind(value) eazpx(TempByte); value.Byte[LOW_BYTE] = _AppleMemory[TempByte]; \
value.Byte[HIGH_BYTE] = _AppleMemory[TempByte + 1];

#define mr_eazpxind(value) eazpxind(TempWord); MEMORY_READ(value, TempWord);
#define mw_eazpxind(value) eazpxind(TempWord); MEMORY_WRITE(value, TempWord);

#define eazpindy(value) eaimm(TempByte); value.Byte[LOW_BYTE] = _AppleMemory[TempByte]; \
value.Byte[HIGH_BYTE] = _AppleMemory[TempByte + 1]; value.PC += Y;

#define mr_eazpindy(value) eazpindy(TempWord); MEMORY_READ(value, TempWord);
#define mw_eazpindy(value) eazpindy(TempWord); MEMORY_WRITE(value, TempWord);

/* MC6502 Instruction New Address Mode */
#define eazpind() PC.PC++; TempByte = PreFetch & 0xFF; TempWord.Byte[LOW_BYTE] = _AppleMemory[TempByte]; TempWord.Byte[HIGH_BYTE] = _AppleMemory[TempByte + 1];
#define mr_eazpind(value) eazpind(); value = _AppleMemory[TempWord.PC];
#define mw_eazpind(value) eazpind(); _AppleMemory[TempWord.PC] = value;

#define eaabsxind() eaabs(TempWord); TempWord.PC += X; PC.Byte[LOW_BYTE] = _AppleMemory[TempWord.PC]; PC.Byte[HIGH_BYTE] = _AppleMemory[TempWord.PC + 1];

union PCStruct PC;
union PCStruct Stack;
union PCStruct Address;
UInt8          X, Y, _A, Decimal;
Char           buf[50];
UInt16         Status;
BOOL           LOOP, QUIT;
UInt32         Start, End;
UInt16         Count, Count1;

extern UInt16  refNum;
extern char *AppleFontBitmap;
extern UInt8 LastKey;

UInt8 *AppleFont;
UInt8 *pointer;

UInt16 ScanlineOffsetTable[192];
UInt16 ScanlineAddressTable[192];
UInt16 ScanlineAddressTableH[192];
UInt16 AppleClock, DiskOffset =0;
extern UInt16 LastAppleClock;
extern UInt16 motor_on;

void SetupVideo(void) {
  WinHandle onScreen;
  UInt32 depth = 1;
  Boolean colorMode = true;
  BitmapType *scanline;

#ifdef SONY
  HRWinScreenMode(refNum, winScreenModeSet, 0, 0, &depth, &colorMode);
#else
  WinScreenMode(winScreenModeSet, 0, 0, &depth, &colorMode);
#endif

  onScreen = WinGetDisplayWindow();
  scanline = WinGetBitmap(onScreen);
  pointer = (UInt8*) BmpGetBits(scanline);

#ifdef SONY
  pointer += 160 * 12;
#else
  pointer += 20 * 20;
#endif

  {
    MemHandle handle;
    UInt16 count;

    handle = MemHandleNew(0x100 * 0x08);
    AppleFont = MemHandleLock(handle);

    MemMove(AppleFont,                                 AppleFontBitmap,               0x40 * 0x08);
    MemMove(AppleFont + 0x40 * 0x08,                   AppleFontBitmap,               0x40 * 0x08);
    MemMove(AppleFont + 0x40 * 0x08 * 2,               AppleFontBitmap,               0x40 * 0x08);
    MemMove(AppleFont + 0x40 * 0x08 * 3,               AppleFontBitmap,               0x20 * 0x08);
    MemMove(AppleFont + 0x40 * 0x08 * 3 + 0x20 * 0x08, AppleFontBitmap + 0x40 * 0x08, 0x20 * 0x08);
    for (count = 0x80 * 0x08; count < (0x100 * 0x08) - 1; count++) AppleFont[count] = ~AppleFont[count];
  }

  {
    UInt16 i;

    for (i = 0;i < 192;i++) {
      ScanlineOffsetTable[i] = (i & 7) * 0x400 + ((i >> 3) & 7) * 0x80 + (i >> 6) * 0x28;
    }

    for (i = 0;i < 192;i++) {
      ScanlineAddressTable[i] = ScanlineOffsetTable[i & ~7] + 0x400;
      ScanlineAddressTableH[i] = ScanlineOffsetTable[i] + 0x2000;
    }
  }

}

void init_6502() {
  Stack.PC = 0x01FF;
  _A = 0; X = 0; Y = 0;
  PC.Byte[LOW_BYTE] = AppleMemory[0x0000FFFC];
  PC.Byte[HIGH_BYTE] = AppleMemory[0x0000FFFD];
  LOOP = 1; QUIT = 1;
  Count = 0; Count1 = 0;
  Decimal = 0;
  AppleClock = 0;
  SetupVideo();
}

void run_6502() {

  register UInt8          Opcode;
  register READBYTE       TempFuncRead;
  register WRITEBYTE      TempFuncWrite;
  register UInt8          *_AppleMemory;
  register union PCStruct TempWord;
  register UInt8          TempByte;
  register UInt8          MemValue;
  register UInt16         PreFetch asm ("d4");
  register Int16          hori, vert;
  register UInt16         fps, scanaddr, chary;
  register UInt8          *_pointer;

#ifdef SONY
  hori = 191; vert = 39; fps = 0; chary = 7;
  _pointer = pointer + 192 * 40;
  if (IOUSoftSwitch[0] & SS_HIRES)
    scanaddr = ScanlineAddressTableH[hori] + 39;
  else
    scanaddr = ScanlineAddressTable[hori] + 39;
#else
  hori = 119; vert = 19; fps = 0; chary = 7;
  _pointer = pointer + 120 * 20;
  if (IOUSoftSwitch[0] & SS_HIRES)
    scanaddr = ScanlineAddressTableH[hori] + 19;
  else
    scanaddr = ScanlineAddressTable[hori] + 19;
#endif

  _AppleMemory = AppleMemory;

 run_again:

#ifdef DEBUG
  if (!LOOP) HostTraceOutputT(sysErrorClass, "PC : %X\n", PC.PC);
#endif

  INSTRUCTION_FETCH(Opcode);

#ifdef DEBUG
  if (!LOOP) HostTraceOutputT(sysErrorClass, "O : %X A : %X X : %X Y : %X\n", Opcode, _A, X, Y);
#endif

  switch (Opcode) {
  case 0x69: /* ADC #imm */
    Status &= 0xFFEF; if (Status & 0x01) Status |= 0x10;
    Status |= 0x04;
    PC.PC++;
    asm("move.b %0, %1\n\tbne .adc_69_decimal" : : "m" (Decimal), "d" (Decimal));
    asm("move.w %0, %%ccr\n\taddx.b %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	:
	: "m" (Status), "d" (_A), "d" (PreFetch), "m" (_A)
	);
    asm("bra .adc_69_finish");
    asm(".adc_69_decimal:");
    asm("move.w %0, %%ccr\n\tabcd %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	:
	: "m" (Status), "d" (_A), "d" (PreFetch), "m" (_A)
	);
    asm(".adc_69_finish:");
    AppleClock+=2;
    break;
  case 0x6D: /* ADC abs */
    Status &= 0xFFEF; if (Status & 0x01) Status |= 0x10;
    Status |= 0x04;
    mr_eaabs(MemValue);
    asm("move.b %0, %1\n\tbne .adc_6d_decimal" : : "m" (Decimal), "d" (Decimal));
    asm("move.w %0, %%ccr\n\taddx.b %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	:
	: "m" (Status), "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm("bra .adc_6d_finish");
    asm(".adc_6d_decimal:");
    asm("move.w %0, %%ccr\n\tabcd %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	:
	: "m" (Status), "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm(".adc_6d_finish:");
    AppleClock+=4;
    break;
  case 0x65: /* ADC zp */
    Status &= 0xFFEF; if (Status & 0x01) Status |= 0x10;
    Status |= 0x04;
    eazp(MemValue);
    asm("move.b %0, %1\n\tbne .adc_65_decimal" : : "m" (Decimal), "d" (Decimal));
    asm("move.w %0, %%ccr\n\taddx.b %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	:
	: "m" (Status), "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm("bra .adc_65_finish");
    asm(".adc_65_decimal:");
    asm("move.w %0, %%ccr\n\tabcd %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	:
	: "m" (Status), "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm(".adc_65_finish:");
    AppleClock+=3;
    break;
  case 0x61: /* ADC (zp, X) */
    Status &= 0xFFEF; if (Status & 0x01) Status |= 0x10;
    Status |= 0x04;
    mr_eazpxind(MemValue);
    asm("move.b %0, %1\n\tbne .adc_61_decimal" : : "m" (Decimal), "d" (Decimal));
    asm("move.w %0, %%ccr\n\taddx.b %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	:
	: "m" (Status), "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm("bra .adc_61_finish");
    asm(".adc_61_decimal:");
    asm("move.w %0, %%ccr\n\tabcd %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	:
	: "m" (Status), "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm(".adc_61_finish:");
    AppleClock+=6;
    break;
  case 0x71: /* ADC (zp), Y */
    Status &= 0xFFEF; if (Status & 0x01) Status |= 0x10;
    Status |= 0x04;
    mr_eazpindy(MemValue);
    asm("move.b %0, %1\n\tbne .adc_71_decimal" : : "m" (Decimal), "d" (Decimal));
    asm("move.w %0, %%ccr\n\taddx.b %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	:
	: "m" (Status), "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm("bra .adc_71_finish");
    asm(".adc_71_decimal:");
    asm("move.w %0, %%ccr\n\tabcd %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	:
	: "m" (Status), "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm(".adc_71_finish:");
    AppleClock+=5;
    break;
  case 0x75: /* ADC zp, X */
    Status &= 0xFFEF; if (Status & 0x01) Status |= 0x10;
    Status |= 0x04;
    mr_eazpx(MemValue);
    asm("move.b %0, %1\n\tbne .adc_75_decimal" : : "m" (Decimal), "d" (Decimal));
    asm("move.w %0, %%ccr\n\taddx.b %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	:
	: "m" (Status), "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm("bra .adc_75_finish");
    asm(".adc_75_decimal:");
    asm("move.w %0, %%ccr\n\tabcd %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	:
	: "m" (Status), "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm(".adc_75_finish:");
    AppleClock+=4;
    break;
  case 0x7D: /* ADC abs, X */
    Status &= 0xFFEF; if (Status & 0x01) Status |= 0x10;
    Status |= 0x04;
    mr_eaabsx(MemValue);
    asm("move.b %0, %1\n\tbne .adc_7d_decimal" : : "m" (Decimal), "d" (Decimal));
    asm("move.w %0, %%ccr\n\taddx.b %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	:
	: "m" (Status), "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm("bra .adc_7d_finish");
    asm(".adc_7d_decimal:");
    asm("move.w %0, %%ccr\n\tabcd %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	:
	: "m" (Status), "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm(".adc_7d_finish:");
    AppleClock+=4;
    break;
  case 0x79: /* ADC abs, Y */
    Status &= 0xFFEF; if (Status & 0x01) Status |= 0x10;
    Status |= 0x04;
    mr_eaabsy(MemValue);
    asm("move.b %0, %1\n\tbne .adc_79_decimal" : : "m" (Decimal), "d" (Decimal));
    asm("move.w %0, %%ccr\n\taddx.b %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	:
	: "m" (Status), "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm("bra .adc_79_finish");
    asm(".adc_79_decimal:");
    asm("move.w %0, %%ccr\n\tabcd %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	:
	: "m" (Status), "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm(".adc_79_finish:");
    AppleClock+=4;
    break;
  case 0x29: /* AND #imm */
    // eaimm(MemValue);
    PC.PC++;
    asm("and.b %2, %1\n\tmove.w %%sr, %3\n\tmove %0, %4\n\tandi.w #0x000C, %3\n\tandi.w #0xFFF3, %4\n\tor.b %3, %4\n\tmove.w %4, %0"
	: "=m" (Status)
	: "m" (_A), "d" (PreFetch), "d" (Status), "d" (MemValue)	
	);
    AppleClock+=2;
    break;
  case 0x2D: /* AND abs */
    mr_eaabs(MemValue);
    asm("and.b %2, %1\n\tmove.w %%sr, %3\n\tmove %0, %4\n\tandi.w #0x000C, %3\n\tandi.w #0xFFF3, %4\n\tor.b %3, %4\n\tmove.w %4, %0"
	: "=m" (Status)
	: "m" (_A), "d" (MemValue), "d" (Status), "d" (MemValue)	
	);
    AppleClock+=4;
    break;
  case 0x25: /* AND zp */
    eazp(MemValue);
    asm("and.b %2, %1\n\tmove.w %%sr, %3\n\tmove %0, %4\n\tandi.w #0x000C, %3\n\tandi.w #0xFFF3, %4\n\tor.b %3, %4\n\tmove.w %4, %0"
	: "=m" (Status)
	: "m" (_A), "d" (MemValue), "d" (Status), "d" (MemValue)	
	);
    AppleClock+=3;
    break;
  case 0x21: /* AND (zp, X) */
    mr_eazpxind(MemValue);
    asm("and.b %2, %1\n\tmove.w %%sr, %3\n\tmove %0, %4\n\tandi.w #0x000C, %3\n\tandi.w #0xFFF3, %4\n\tor.b %3, %4\n\tmove.w %4, %0"
	: "=m" (Status)
	: "m" (_A), "d" (MemValue), "d" (Status), "d" (MemValue)	
	);
    AppleClock+=6;
    break;
  case 0x31: /* AND (zp), Y */
    mr_eazpindy(MemValue);
    asm("and.b %2, %1\n\tmove.w %%sr, %3\n\tmove %0, %4\n\tandi.w #0x000C, %3\n\tandi.w #0xFFF3, %4\n\tor.b %3, %4\n\tmove.w %4, %0"
	: "=m" (Status)
	: "m" (_A), "d" (MemValue), "d" (Status), "d" (MemValue)	
	);
    AppleClock+=5;
    break;
  case 0x35: /* AND zp, X */
    mr_eazpx(MemValue);
    asm("and.b %2, %1\n\tmove.w %%sr, %3\n\tmove %0, %4\n\tandi.w #0x000C, %3\n\tandi.w #0xFFF3, %4\n\tor.b %3, %4\n\tmove.w %4, %0"
	: "=m" (Status)
	: "m" (_A), "d" (MemValue), "d" (Status), "d" (MemValue)	
	);
    AppleClock+=4;
    break;
  case 0x3D: /* AND abs, X */
    mr_eaabsx(MemValue);
    asm("and.b %2, %1\n\tmove.w %%sr, %3\n\tmove %0, %4\n\tandi.w #0x000C, %3\n\tandi.w #0xFFF3, %4\n\tor.b %3, %4\n\tmove.w %4, %0"
	: "=m" (Status)
	: "m" (_A), "d" (MemValue), "d" (Status), "d" (MemValue)	
	);
    break;
  case 0x39: /* AND abs, Y */
    mr_eaabsy(MemValue);
    asm("and.b %2, %1\n\tmove.w %%sr, %3\n\tmove %0, %4\n\tandi.w #0x000C, %3\n\tandi.w #0xFFF3, %4\n\tor.b %3, %4\n\tmove.w %4, %0"
	: "=m" (Status)
	: "m" (_A), "d" (MemValue), "d" (Status), "d" (MemValue)	
	);
    AppleClock+=4;
    break;

  case 0x0E: /* ASL abs */
    mr_eaabs(MemValue);

    asm("lsl.b #1, %0\n\tmove.w %%sr, %1\n\tandi.w #0x0002, %2\n\tor.w %1, %2"
	:
	: "d" (MemValue), "d" (Status), "m" (Status)
	);

    MEMORY_WRITE(MemValue, TempWord);
    AppleClock+=6;
    break;
  case 0x06: /* ASL zp */
    eazp(MemValue);

    asm("lsl.b #1, %0\n\tmove.w %%sr, %1\n\tandi.w #0x0002, %2\n\tor.w %1, %2"
	:
	: "d" (MemValue), "d" (Status), "m" (Status)
	);

    _AppleMemory[TempByte] = MemValue;
    AppleClock+=5;
    break;
  case 0x0A: /* ASL acc */
    asm("lsl.b #1, %0\n\tmove.w %%sr, %1\n\tandi.w #0x0002, %2\n\tor.w %1, %2\n\tmove.b %0, %3"
	:
	: "d" (_A), "d" (Status), "m" (Status), "m" (_A)
	);
    AppleClock+=2;
    break;
  case 0x16: /* ASL zp, X */
    mr_eazpx(MemValue);

    asm("lsl.b #1, %0\n\tmove.w %%sr, %1\n\tandi.w #0x0002, %2\n\tor.w %1, %2"
	:
	: "d" (MemValue), "d" (Status), "m" (Status)
	);

    _AppleMemory[TempByte] = MemValue;
    AppleClock+=6;
    break;
  case 0x1E: /* ASL abs, X */
    mr_eaabsx(MemValue);

    asm("lsl.b #1, %0\n\tmove.w %%sr, %1\n\tandi.w #0x0002, %2\n\tor.w %1, %2"
	:
	: "d" (MemValue), "d" (Status), "m" (Status)
	);

    MEMORY_WRITE(MemValue, TempWord);
    AppleClock+=7;
    break;

  case 0x90: /* BCC rr */
    AppleClock+=2;
    PC.PC++;
    asm("move %0, %%ccr" : "=m" (Status));
    asm("bcs  .bcc_leave");
    asm("ext.w %0" : : "d" (PreFetch));
    PC.PC += PreFetch;
    AppleClock++;
    asm(".bcc_leave:");
    break;
  case 0xB0: /* BCS rr */
    PC.PC++;
    AppleClock+=2;
    asm("move %0, %%ccr" : "=m" (Status));
    asm("bcc  .bcs_leave");
    asm("ext.w %0" : : "d" (PreFetch));
    PC.PC += PreFetch;
    AppleClock++;
    asm(".bcs_leave:");
    break;
  case 0xF0: /* BEQ rr */
    PC.PC++;
    AppleClock+=2;
    asm("move %0, %%ccr" : "=m" (Status));
    asm("bne  .beq_leave");
    asm("ext.w %0" : : "d" (PreFetch));
    PC.PC += PreFetch;
    AppleClock++;
    asm(".beq_leave:");
    break;

  case 0x2C: /* BIT abs */
    mr_eaabs(MemValue);
    asm("andi.w #0xFFF1, %0\n\tand.b %2, %1\n\tmove.w %%sr, %3\n\tandi.w #0x04, %3\n\tor.w %3, %0\n\tclr.w %3\n\tmove.b %2, %3\n\tandi.w #0x0080, %3\n\tlsr.w #4, %3\n\tor.w %3, %0\n\tclr.w %3\n\tmove.b %2, %3\n\tandi.w #0x0040, %3\n\tlsr.w #5, %3\n\tor.w %3, %0"
	:
	: "m" (Status), "d" (_A), "d" (MemValue), "d" (Status)
	);
    AppleClock+=4;
    break;
  case 0x24: /* BIT zp */
    eazp(MemValue);
    asm("andi.w #0xFFF1, %0\n\tand.b %2, %1\n\tmove.w %%sr, %3\n\tandi.w #0x04, %3\n\tor.w %3, %0\n\tclr.w %3\n\tmove.b %2, %3\n\tandi.w #0x0080, %3\n\tlsr.w #4, %3\n\tor.w %3, %0\n\tclr.w %3\n\tmove.b %2, %3\n\tandi.w #0x0040, %3\n\tlsr.w #5, %3\n\tor.w %3, %0"
	:
	: "m" (Status), "d" (_A), "d" (MemValue), "d" (Status)
	);
    AppleClock+=3;
    break;

  case 0x30: /* BMI rr */
    PC.PC++;
    AppleClock+=2;
    asm("move %0, %%ccr" : "=m" (Status));
    asm("bpl  .bmi_leave");
    asm("ext.w %0" : : "d" (PreFetch));
    PC.PC += PreFetch;
    AppleClock++;
    asm(".bmi_leave:");
    break;
  case 0xD0: /* BNE rr */
    PC.PC++;
    AppleClock+=2;
    asm("move %0, %%ccr" : "=m" (Status));
    asm("beq  .bne_leave");
    asm("ext.w %0" : : "d" (PreFetch));
    PC.PC += PreFetch;
    AppleClock++;
    asm(".bne_leave:");
    break;
  case 0x10: /* BPL rr */
    PC.PC++;
    AppleClock+=2;
    asm("move %0, %%ccr" : "=m" (Status));
    asm("bmi  .bpl_leave");
    asm("ext.w %0" : : "d" (PreFetch));
    PC.PC += PreFetch;
    AppleClock++;
    asm(".bpl_leave:");
    break;

  case 0x00: /* BRK */
    break;

  case 0x50: /* BVC rr */
    PC.PC++;
    AppleClock+=2;
    asm("move %0, %%ccr" : "=m" (Status));
    asm("bvs  .bvc_leave");
    asm("ext.w %0" : : "d" (PreFetch));
    PC.PC += PreFetch;
    AppleClock++;
    asm(".bvc_leave:");
    break;
  case 0x70: /* BVS rr */
    PC.PC++;
    AppleClock+=2;
    asm("move %0, %%ccr" : "=m" (Status));
    asm("bvc  .bvs_leave");
    asm("ext.w %0" : : "d" (PreFetch));
    PC.PC += PreFetch;
    AppleClock++;
    asm(".bvs_leave:");
    break;

  case 0x18: /* CLC */
    asm("andi.w #0xFFEE, %0" : "=m" (Status));
    AppleClock+=2;
    break;
  case 0xD8: /* CLD */
    asm("clr.b %0" : : "m" (Decimal));
    AppleClock+=2;
    break;
  case 0x58: /* CLI */
    AppleClock+=2;
    break;
  case 0xB8: /* CLV */
    asm("andi.w #0xFFFD, %0" : "=m" (Status));
    AppleClock+=2;
    break;
  case 0xC9: /* CMP #imm */
    // eaimm(MemValue);
    PC.PC++;
    asm("neg.b %1\n\tadd.b %1, %0\n\tmove %%sr, %2\n\tandi.w #0xFFF2, %3\n\tandi.w #0x000D, %2\n\tor.w %2, %3\n\tneg.b %1"
	:
	: "d" (_A), "d" (PreFetch), "d" (Status), "m" (Status)
	);
    if (_A >= (PreFetch & 0xFF)) Status |= 0x01; else Status &= 0xFFFE;
    AppleClock+=2;
    break;
  case 0xCD: /* CMP abs */
    mr_eaabs(MemValue);
    asm("neg.b %1\n\tadd.b %1, %0\n\tmove %%sr, %2\n\tandi.w #0xFFF2, %3\n\tandi.w #0x000D, %2\n\tor.w %2, %3\n\tneg.b %1"
	:
	: "d" (_A), "d" (MemValue), "d" (Status), "m" (Status)
	);
    if (_A >= MemValue) Status |= 0x01; else Status &= 0xFFFE;
    AppleClock+=4;
    break;
  case 0xC5: /* CMP zp */
    eazp(MemValue);
    asm("neg.b %1\n\tadd.b %1, %0\n\tmove %%sr, %2\n\tandi.w #0xFFF2, %3\n\tandi.w #0x000D, %2\n\tor.w %2, %3\n\tneg.b %1"
	:
	: "d" (_A), "d" (MemValue), "d" (Status), "m" (Status)
	);
    if (_A >= MemValue) Status |= 0x01; else Status &= 0xFFFE;
    AppleClock+=3;
    break;
  case 0xC1: /* CMP (zp, X) */
    mr_eazpxind(MemValue);
    asm("neg.b %1\n\tadd.b %1, %0\n\tmove %%sr, %2\n\tandi.w #0xFFF2, %3\n\tandi.w #0x000D, %2\n\tor.w %2, %3\n\tneg.b %1"
	:
	: "d" (_A), "d" (MemValue), "d" (Status), "m" (Status)
	);
    if (_A >= MemValue) Status |= 0x01; else Status &= 0xFFFE;
    AppleClock+=6;
    break;
  case 0xD1: /* CMP (zp), Y */
    mr_eazpindy(MemValue);
    asm("neg.b %1\n\tadd.b %1, %0\n\tmove %%sr, %2\n\tandi.w #0xFFF2, %3\n\tandi.w #0x000D, %2\n\tor.w %2, %3\n\tneg.b %1"
	:
	: "d" (_A), "d" (MemValue), "d" (Status), "m" (Status)
	);
    if (_A >= MemValue) Status |= 0x01; else Status &= 0xFFFE;
    AppleClock+=5;
    break;
  case 0xD5: /* CMP zp, X */
    mr_eazpx(MemValue);
    asm("neg.b %1\n\tadd.b %1, %0\n\tmove %%sr, %2\n\tandi.w #0xFFF2, %3\n\tandi.w #0x000D, %2\n\tor.w %2, %3\n\tneg.b %1"
	:
	: "d" (_A), "d" (MemValue), "d" (Status), "m" (Status)
	);
    if (_A >= MemValue) Status |= 0x01; else Status &= 0xFFFE;
    AppleClock+=4;
    break;
  case 0xDD: /* CMP abs, X */
    mr_eaabsx(MemValue);
    asm("neg.b %1\n\tadd.b %1, %0\n\tmove %%sr, %2\n\tandi.w #0xFFF2, %3\n\tandi.w #0x000D, %2\n\tor.w %2, %3\n\tneg.b %1"
	:
	: "d" (_A), "d" (MemValue), "d" (Status), "m" (Status)
	);
    if (_A >= MemValue) Status |= 0x01; else Status &= 0xFFFE;
/*     if (!LOOP) HostTraceOutputT(sysErrorClass, "CMP abs, X : %X %X %X %X\n", TempWord.PC, X, MemValue, _A); */
    AppleClock+=4;
    break;
  case 0xD9: /* CMP abs, Y */
    mr_eaabsy(MemValue);
    asm("neg.b %1\n\tadd.b %1, %0\n\tmove %%sr, %2\n\tandi.w #0xFFF2, %3\n\tandi.w #0x000D, %2\n\tor.w %2, %3\n\tneg.b %1"
	:
	: "d" (_A), "d" (MemValue), "d" (Status), "m" (Status)
	);
    if (_A >= MemValue) Status |= 0x01; else Status &= 0xFFFE;
    AppleClock+=4;
    break;

  case 0xE0: /* CPX #imm */
    // eaimm(MemValue);
    PC.PC++;
    asm("neg.b %1\n\tadd.b %1, %0\n\tmove %%sr, %2\n\tandi.w #0xFFF2, %3\n\tandi.w #0x000D, %2\n\tor.w %2, %3\n\tneg.b %1"
	:
	: "d" (X), "d" (PreFetch), "d" (Status), "m" (Status)
	);
    if (X >= (PreFetch & 0xFF)) Status |= 0x01; else Status &= 0xFFFE;
    AppleClock+=2;
    break;
  case 0xEC: /* CPX abs */
    mr_eaabs(MemValue);
    asm("neg.b %1\n\tadd.b %1, %0\n\tmove %%sr, %2\n\tandi.w #0xFFF2, %3\n\tandi.w #0x000D, %2\n\tor.w %2, %3\n\tneg.b %1"
	:
	: "d" (X), "d" (MemValue), "d" (Status), "m" (Status)
	);
    if (X >= MemValue) Status |= 0x01; else Status &= 0xFFFE;
    AppleClock+=4;
    break;
  case 0xE4: /* CPX zp */
    eazp(MemValue);
    asm("neg.b %1\n\tadd.b %1, %0\n\tmove %%sr, %2\n\tandi.w #0xFFF2, %3\n\tandi.w #0x000D, %2\n\tor.w %2, %3\n\tneg.b %1"
	:
	: "d" (X), "d" (MemValue), "d" (Status), "m" (Status)
	);
    if (X >= MemValue) Status |= 0x01; else Status &= 0xFFFE;
    AppleClock+=3;
    break;

  case 0xC0: /* CPY #imm */
    // eaimm(MemValue);
    PC.PC++;
    asm("neg.b %1\n\tadd.b %1, %0\n\tmove %%sr, %2\n\tandi.w #0xFFF2, %3\n\tandi.w #0x000D, %2\n\tor.w %2, %3\n\tneg.b %1"
	:
	: "d" (Y), "d" (PreFetch), "d" (Status), "m" (Status)
	);
    if (Y >= (PreFetch & 0xFF)) Status |= 0x01; else Status &= 0xFFFE;
    AppleClock+=2;
    break;
  case 0xCC: /* CPY abs */
    mr_eaabs(MemValue);
    asm("neg.b %1\n\tadd.b %1, %0\n\tmove %%sr, %2\n\tandi.w #0xFFF2, %3\n\tandi.w #0x000D, %2\n\tor.w %2, %3\n\tneg.b %1"
	:
	: "d" (Y), "d" (MemValue), "d" (Status), "m" (Status)
	);
    if (Y >= MemValue) Status |= 0x01; else Status &= 0xFFFE;
    AppleClock+=4;
    break;
  case 0xC4: /* CPY zp */
    eazp(MemValue);
    asm("neg.b %1\n\tadd.b %1, %0\n\tmove %%sr, %2\n\tandi.w #0xFFF2, %3\n\tandi.w #0x000D, %2\n\tor.w %2, %3\n\tneg.b %1"
	:
	: "d" (Y), "d" (MemValue), "d" (Status), "m" (Status)
	);
    if (Y >= MemValue) Status |= 0x01; else Status &= 0xFFFE;
    AppleClock+=3;
    break;

  case 0xCE: /* DEC abs */
    mr_eaabs(MemValue);
    asm("addi.b #0xFF, %0\n\tmove %%sr, %1\n\tandi.w #0x000C, %1\n\tandi.w #0xFFF3, %2\n\tor.w %1, %2"
	:
	: "d" (MemValue), "d" (Status),  "m" (Status)
	);
    MEMORY_WRITE(MemValue, TempWord);
    AppleClock+=6;
    break;
  case 0xC6: /* DEC zp */
    eazp(MemValue);
    asm("addi.b #0xFF, %0\n\tmove %%sr, %1\n\tandi.w #0x000C, %1\n\tandi.w #0xFFF3, %2\n\tor.w %1, %2"
	:
	: "d" (MemValue), "d" (Status),  "m" (Status)
	);
    _AppleMemory[TempByte] = MemValue;    
    AppleClock+=5;
    break;
  case 0xD6: /* DEC zp, X */
    mr_eazpx(MemValue);
    asm("addi.b #0xFF, %0\n\tmove %%sr, %1\n\tandi.w #0x000C, %1\n\tandi.w #0xFFF3, %2\n\tor.w %1, %2"
	:
	: "d" (MemValue), "d" (Status),  "m" (Status)
	);
    _AppleMemory[TempByte] = MemValue;
    AppleClock+=6;
    break;
  case 0xDE: /* DEC abs, X */
    mr_eaabsx(MemValue);
    asm("addi.b #0xFF, %0\n\tmove %%sr, %1\n\tandi.w #0x000C, %1\n\tandi.w #0xFFF3, %2\n\tor.w %1, %2"
	:
	: "d" (MemValue), "d" (Status),  "m" (Status)
	);
    MEMORY_WRITE(MemValue, TempWord);
    AppleClock+=7;
    break;
  case 0xCA: /* DEX */
    asm("addi.b #0xFF, %0\n\tmove %%sr, %1\n\tandi.w #0x000C, %1\n\tandi.w #0xFFF3, %2\n\tor.w %1, %2"
	:
	: "m" (X), "d" (Status),  "m" (Status)
	);
    AppleClock+=2;
    break;
  case 0x88: /* DEY */
    asm("addi.b #0xFF, %0\n\tmove %%sr, %1\n\tandi.w #0x000C, %1\n\tandi.w #0xFFF3, %2\n\tor.w %1, %2"
	:
	: "m" (Y), "d" (Status),  "m" (Status)
	);
    AppleClock+=2;
    break;

  case 0x49: /* EOR #imm */
    // eaimm(MemValue);
    PC.PC++;
    asm("eor.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (PreFetch), "m" (_A), "d" (Status),  "m" (Status)
	);
    AppleClock+=2;
    break;
  case 0x4D: /* EOR abs */
    mr_eaabs(MemValue);
    asm("eor.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (MemValue), "m" (_A), "d" (Status),  "m" (Status)
	);
    AppleClock+=4;
    break;
  case 0x45: /* EOR zp */
    eazp(MemValue);
    asm("eor.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (MemValue), "m" (_A), "d" (Status),  "m" (Status)
	);
    AppleClock+=3;
    break;
  case 0x41: /* EOR (zp, X) */
    mr_eazpxind(MemValue);
    asm("eor.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (MemValue), "m" (_A), "d" (Status),  "m" (Status)
	);
    AppleClock+=6;
    break;
  case 0x51: /* EOR (zp), Y */
    mr_eazpindy(MemValue);
    asm("eor.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (MemValue), "m" (_A), "d" (Status),  "m" (Status)
	);
    AppleClock+=5;
    break;
  case 0x55: /* EOR zp, X */
    mr_eazpx(MemValue);
    asm("eor.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (MemValue), "m" (_A), "d" (Status),  "m" (Status)
	);
    AppleClock+=4;
    break;
  case 0x5D: /* EOR abs, X */
    mr_eaabsx(MemValue);
    asm("eor.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (MemValue), "m" (_A), "d" (Status),  "m" (Status)
	);
    AppleClock+=4;
    break;
  case 0x59: /* EOR abs, Y */
    mr_eaabsy(MemValue);
    asm("eor.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (MemValue), "m" (_A), "d" (Status),  "m" (Status)
	);
    AppleClock+=4;
    break;

  case 0xEE: /* INC abs */
    mr_eaabs(MemValue);
    asm("addi.b #0x01, %0\n\tmove %%sr, %1\n\tandi.w #0x000C, %1\n\tandi.w #0xFFF3, %2\n\tor.w %1, %2"
	:
	: "d" (MemValue), "d" (Status),  "m" (Status)
	);
    MEMORY_WRITE(MemValue, TempWord);
    AppleClock+=6;
    break;
  case 0xE6: /* INC zp */
    eazp(MemValue);
    asm("addi.b #0x01, %0\n\tmove %%sr, %1\n\tandi.w #0x000C, %1\n\tandi.w #0xFFF3, %2\n\tor.w %1, %2"
	:
	: "d" (MemValue), "d" (Status),  "m" (Status)
	);
    _AppleMemory[TempByte] = MemValue;    
    AppleClock+=5;
    break;
  case 0xF6: /* INC zp, X */
    mr_eazpx(MemValue);
    asm("addi.b #0x01, %0\n\tmove %%sr, %1\n\tandi.w #0x000C, %1\n\tandi.w #0xFFF3, %2\n\tor.w %1, %2"
	:
	: "d" (MemValue), "d" (Status),  "m" (Status)
	);
    _AppleMemory[TempByte] = MemValue;
    AppleClock+=6;
    break;
  case 0xFE: /* INC abs, X */
    mr_eaabsx(MemValue);
    asm("addi.b #0x01, %0\n\tmove %%sr, %1\n\tandi.w #0x000C, %1\n\tandi.w #0xFFF3, %2\n\tor.w %1, %2"
	:
	: "d" (MemValue), "d" (Status),  "m" (Status)
	);
    MEMORY_WRITE(MemValue, TempWord);
    AppleClock+=7;
    break;
  case 0xE8: /* INX */
    asm("addi.b #0x01, %0\n\tmove %%sr, %1\n\tandi.w #0x000C, %1\n\tandi.w #0xFFF3, %2\n\tor.w %1, %2"
	:
	: "m" (X), "d" (Status),  "m" (Status)
	);
    AppleClock+=2;
    break;
  case 0xC8: /* INY */
    asm("addi.b #0x01, %0\n\tmove %%sr, %1\n\tandi.w #0x000C, %1\n\tandi.w #0xFFF3, %2\n\tor.w %1, %2"
	:
	: "m" (Y), "d" (Status),  "m" (Status)
	);
    AppleClock+=2;
    break;

  case 0x4C: /* JMP abs */
    // eaabs(TempWord);
    PC.PC = PreFetch;
    AppleClock+=3;
    break;
  case 0x6C: /* JMP (abs) */
    eaabsind();
    AppleClock+=5;
    break;

  case 0x20: /* JSR abs */
    // eaabs(TempWord);
    PC.PC++;
    PUSH(PC.Byte[HIGH_BYTE]);
    PUSH(PC.Byte[LOW_BYTE]);
    PC.PC = PreFetch;
    // PC.PC = TempWord.PC;
    AppleClock+=6;
    break;

  case 0xA9: /* LDA #imm */
    // eaimm(MemValue);
    PC.PC++;
    asm("move.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (PreFetch), "m" (_A), "d" (Status),  "m" (Status)
	);
    AppleClock+=2;
    break;
  case 0xAD: /* LDA abs */
    mr_eaabs(MemValue);
    asm("move.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (MemValue), "m" (_A), "d" (Status),  "m" (Status)
	);    
/*     if (!LOOP) HostTraceOutputT(sysErrorClass, "LDA abs : %X %X\n", TempWord.PC, MemValue); */
    AppleClock+=4;
    break;
  case 0xA5: /* LDA zp */
    eazp(MemValue);
    asm("move.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (MemValue), "m" (_A), "d" (Status),  "m" (Status)
	);
    AppleClock+=3;
    break;
  case 0xA1: /* LDA (zp, X) */
    mr_eazpxind(MemValue);
    asm("move.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (MemValue), "m" (_A), "d" (Status),  "m" (Status)
	);
    AppleClock+=6;
    break;
  case 0xB1: /* LDA (zp), Y */
    mr_eazpindy(MemValue);
    asm("move.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (MemValue), "m" (_A), "d" (Status),  "m" (Status)
	);
/*     if (!LOOP) HostTraceOutputT(sysErrorClass, "LDA (zp), Y : %X %X %X\n", MemValue, TempByte, TempWord.PC); */
    AppleClock+=5;
    break;
  case 0xB5: /* LDA zp, X */
    mr_eazpx(MemValue);
    asm("move.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (MemValue), "m" (_A), "d" (Status),  "m" (Status)
	);
    AppleClock+=4;
    break;
  case 0xBD: /* LDA abs, X */
    mr_eaabsx(MemValue);
    asm("move.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (MemValue), "m" (_A), "d" (Status),  "m" (Status)
	);
    AppleClock+=4;
/*     if (!LOOP) HostTraceOutputT(sysErrorClass, "LDA abs, X : %X %X %X\n", TempWord.PC, X, PreFetch); */
    break;
  case 0xB9: /* LDA abs, Y */
    mr_eaabsy(MemValue);
    asm("move.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (MemValue), "m" (_A), "d" (Status),  "m" (Status)
	);
/*     if (!LOOP) HostTraceOutputT(sysErrorClass, "LDA abs, Y : %X %X\n", TempWord.PC, MemValue); */
    AppleClock+=4;
    break;

  case 0xA2: /* LDX #imm */
    // eaimm(MemValue);
    PC.PC++;
    asm("move.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (PreFetch), "m" (X), "d" (Status),  "m" (Status)
	);
    AppleClock+=2;
    break;
  case 0xAE: /* LDX abs */
    mr_eaabs(MemValue);
    asm("move.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (MemValue), "m" (X), "d" (Status),  "m" (Status)
	);
    AppleClock+=4;
    break;
  case 0xA6: /* LDX zp */
    eazp(MemValue);
    asm("move.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (MemValue), "m" (X), "d" (Status),  "m" (Status)
	);
    AppleClock+=3;
    break;
  case 0xBE: /* LDX abs, Y */
    mr_eaabsy(MemValue);
    asm("move.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (MemValue), "m" (X), "d" (Status),  "m" (Status)
	);
    AppleClock+=4;
    break;
  case 0xB6: /* LDX zp, Y */
    mr_eazpy(MemValue);
    asm("move.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (MemValue), "m" (X), "d" (Status),  "m" (Status)
	);
    AppleClock+=4;
    break;

  case 0xA0: /* LDY #imm */
    // eaimm(MemValue);
    PC.PC++;
    asm("move.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (PreFetch), "m" (Y), "d" (Status),  "m" (Status)
	);
    AppleClock+=2;
    break;
  case 0xAC: /* LDY abs */
    mr_eaabs(MemValue);
    asm("move.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (MemValue), "m" (Y), "d" (Status),  "m" (Status)
	);
    AppleClock+=4;
    break;
  case 0xA4: /* LDY zp */
    eazp(MemValue);
    asm("move.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (MemValue), "m" (Y), "d" (Status),  "m" (Status)
	);
    AppleClock+=3;
    break;
  case 0xB4: /* LDY zp, X */
    mr_eazpx(MemValue);
    asm("move.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (MemValue), "m" (Y), "d" (Status),  "m" (Status)
	);
    AppleClock+=4;
    break;
  case 0xBC: /* LDY abs, X */
    mr_eaabsx(MemValue);
    asm("move.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (MemValue), "m" (Y), "d" (Status),  "m" (Status)
	);
    AppleClock+=4;
    break;

  case 0x4E: /* LSR abs */
    mr_eaabs(MemValue);

    asm("lsr.b #1, %0\n\tmove.w %%sr, %1\n\tandi.w #0x0002, %2\n\tandi.w #0xFFF7, %1\n\tor.w %1, %2"
	:
	: "d" (MemValue), "d" (Status), "m" (Status)
	);

    MEMORY_WRITE(MemValue, TempWord);
    AppleClock+=6;
    break;
  case 0x46: /* LSR zp */
    eazp(MemValue);

    asm("lsr.b #1, %0\n\tmove.w %%sr, %1\n\tandi.w #0x0002, %2\n\tandi.w #0xFFF7, %1\n\tor.w %1, %2"
	:
	: "d" (MemValue), "d" (Status), "m" (Status)
	);

    _AppleMemory[TempByte] = MemValue;
    AppleClock+=5;
    break;
  case 0x4A: /* LSR acc */
    asm("lsr.b #1, %0\n\tmove.w %%sr, %1\n\tandi.w #0x0002, %2\n\tandi.w #0xFFF7, %1\n\tor.w %1, %2\n\tmove.b %0, %3"
	:
	: "d" (_A), "d" (Status), "m" (Status), "m" (_A)
	);
    AppleClock+=2;
    break;
  case 0x56: /* LSR zp, X */
    mr_eazpx(MemValue);

    asm("lsr.b #1, %0\n\tmove.w %%sr, %1\n\tandi.w #0x0002, %2\n\tandi.w #0xFFF7, %1\n\tor.w %1, %2"
	:
	: "d" (MemValue), "d" (Status), "m" (Status)
	);

    _AppleMemory[TempByte] = MemValue;
    AppleClock+=6;
    break;
  case 0x5E: /* LSR abs, X */
    mr_eaabsx(MemValue);

    asm("lsr.b #1, %0\n\tmove.w %%sr, %1\n\tandi.w #0x0002, %2\n\tandi.w #0xFFF7, %1\n\tor.w %1, %2"
	:
	: "d" (MemValue), "d" (Status), "m" (Status)
	);

    MEMORY_WRITE(MemValue, TempWord);
    AppleClock+=7;
    break;

  case 0xEA: /* NOP */
    AppleClock+=2;
    break;

  case 0x09: /* ORA #imm */
    // eaimm(MemValue);
    PC.PC++;
    asm("or.b %2, %1\n\tmove.w %%sr, %3\n\tmove %0, %4\n\tandi.w #0x000C, %3\n\tandi.w #0xFFF3, %4\n\tor.b %3, %4\n\tmove.w %4, %0"
	: "=m" (Status)
	: "m" (_A), "d" (PreFetch), "d" (Status), "d" (MemValue)	
	);
    AppleClock+=2;
    break;
  case 0x0D: /* ORA abs */
    mr_eaabs(MemValue);
    asm("or.b %2, %1\n\tmove.w %%sr, %3\n\tmove %0, %4\n\tandi.w #0x000C, %3\n\tandi.w #0xFFF3, %4\n\tor.b %3, %4\n\tmove.w %4, %0"
	: "=m" (Status)
	: "m" (_A), "d" (MemValue), "d" (Status), "d" (MemValue)	
	);
    AppleClock+=4;
    break;
  case 0x05: /* ORA zp */
    eazp(MemValue);
    asm("or.b %2, %1\n\tmove.w %%sr, %3\n\tmove %0, %4\n\tandi.w #0x000C, %3\n\tandi.w #0xFFF3, %4\n\tor.b %3, %4\n\tmove.w %4, %0"
	: "=m" (Status)
	: "m" (_A), "d" (MemValue), "d" (Status), "d" (MemValue)	
	);
    AppleClock+=3;
    break;
  case 0x01: /* ORA (zp, X) */
    mr_eazpxind(MemValue);
    asm("or.b %2, %1\n\tmove.w %%sr, %3\n\tmove %0, %4\n\tandi.w #0x000C, %3\n\tandi.w #0xFFF3, %4\n\tor.b %3, %4\n\tmove.w %4, %0"
	: "=m" (Status)
	: "m" (_A), "d" (MemValue), "d" (Status), "d" (MemValue)	
	);
    AppleClock+=6;
    break;
  case 0x11: /* ORA (zp), Y */
    mr_eazpindy(MemValue);
    asm("or.b %2, %1\n\tmove.w %%sr, %3\n\tmove %0, %4\n\tandi.w #0x000C, %3\n\tandi.w #0xFFF3, %4\n\tor.b %3, %4\n\tmove.w %4, %0"
	: "=m" (Status)
	: "m" (_A), "d" (MemValue), "d" (Status), "d" (MemValue)	
	);
    AppleClock+=5;
    break;
  case 0x15: /* ORA zp, X */
    mr_eazpx(MemValue);
    asm("or.b %2, %1\n\tmove.w %%sr, %3\n\tmove %0, %4\n\tandi.w #0x000C, %3\n\tandi.w #0xFFF3, %4\n\tor.b %3, %4\n\tmove.w %4, %0"
	: "=m" (Status)
	: "m" (_A), "d" (MemValue), "d" (Status), "d" (MemValue)	
	);
    AppleClock+=4;
    break;
  case 0x1D: /* ORA abs, X */
    mr_eaabsx(MemValue);
    asm("or.b %2, %1\n\tmove.w %%sr, %3\n\tmove %0, %4\n\tandi.w #0x000C, %3\n\tandi.w #0xFFF3, %4\n\tor.b %3, %4\n\tmove.w %4, %0"
	: "=m" (Status)
	: "m" (_A), "d" (MemValue), "d" (Status), "d" (MemValue)	
	);
    AppleClock+=4;
    break;
  case 0x19: /* ORA abs, Y */
    mr_eaabsy(MemValue);
    asm("or.b %2, %1\n\tmove.w %%sr, %3\n\tmove %0, %4\n\tandi.w #0x000C, %3\n\tandi.w #0xFFF3, %4\n\tor.b %3, %4\n\tmove.w %4, %0"
	: "=m" (Status)
	: "m" (_A), "d" (MemValue), "d" (Status), "d" (MemValue)	
	);
    AppleClock+=4;
    break;

  case 0x48: /* PHA */
    PUSH(_A);
    AppleClock+=3;
    break;
  case 0x08: /* PHP */
    asm("clr.w %0"
	:
	: "d" (MemValue)
	);
    /* N Flags */
    asm("move %0, %1\n\tandi.w #0x08, %1\n\tlsl.w #4, %1\n\tor.w %1, %2"
	:
	: "m" (Status), "d" (Status), "d" (MemValue)
	);
    /* V Flags */
    asm("move %0, %1\n\tandi.w #0x02, %1\n\tlsl.w #5, %1\n\tor.w %1, %2"
	:
	: "m" (Status), "d" (Status), "d" (MemValue)
	);
    /* Bit 5 Flags */
    asm("ori.w #0x20, %0"
	:
	: "d" (MemValue)
	);
    /* Z Flags */
    asm("move %0, %1\n\tandi.w #0x04, %1\n\tlsr.w #1, %1\n\tor.w %1, %2"
	:
	: "m" (Status), "d" (Status), "d" (MemValue)
	);
    /* C Flags */
    asm("move %0, %1\n\tandi.w #0x01, %1\n\tor.w %1, %2"
	:
	: "m" (Status), "d" (Status), "d" (MemValue)
	);
    MemValue |= Decimal;
    PUSH(MemValue);
    AppleClock+=3;
    break;
  case 0x68: /* PLA */
    POP(_A);
    asm("move.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "m" (_A), "d" (_A), "d" (Status), "m" (Status)
	);
    AppleClock+=4;
    break;
  case 0x28: /* PLP */
    POP(MemValue);
    asm("clr.w %0"
	:
	: "m" (Status)
	);
    /* N Flags */
    asm("move %2, %1\n\tandi.w #0x80, %1\n\tlsr.w #4, %1\n\tor.w %1, %0"
	:
	: "m" (Status), "d" (Status), "d" (MemValue)
	);
    /* V Flags */
    asm("move %2, %1\n\tandi.w #0x40, %1\n\tlsr.w #5, %1\n\tor.w %1, %0"
	:
	: "m" (Status), "d" (Status), "d" (MemValue)
	);
    /* Z Flags */
    asm("move %2, %1\n\tandi.w #0x02, %1\n\tlsl.w #1, %1\n\tor.w %1, %0"
	:
	: "m" (Status), "d" (Status), "d" (MemValue)
	);
    /* C Flags */
    asm("move %2, %1\n\tandi.w #0x01, %1\n\tor.w %1, %0"
	:
	: "m" (Status), "d" (Status), "d" (MemValue)
	);
    /* D Flags */
    asm("move %2, %1\n\tandi.w #0x08, %1\n\tmove.b %1, %0"
	:
	: "m" (Decimal), "d" (Status), "d" (MemValue)
	);
    AppleClock+=4;
    break;

  case 0x2E: /* ROL abs */
    mr_eaabs(MemValue);
    Status &= 0xFFEF; if (Status & 0x01) Status |= 0x10;
    asm("move.w %2, %%ccr\n\troxl.b #1, %0\n\tmove.w %%sr, %1\n\tandi.w #0x0002, %2\n\tor.w %1, %2"
	:
	: "d" (MemValue), "d" (Status), "m" (Status)
	);

    MEMORY_WRITE(MemValue, TempWord);
    AppleClock+=6;
    break;
  case 0x26: /* ROL zp */
    eazp(MemValue);
    Status &= 0xFFEF; if (Status & 0x01) Status |= 0x10;
    asm("move.w %2, %%ccr\n\troxl.b #1, %0\n\tmove.w %%sr, %1\n\tandi.w #0x0002, %2\n\tor.w %1, %2"
	:
	: "d" (MemValue), "d" (Status), "m" (Status)
	);

    _AppleMemory[TempByte] = MemValue;
    AppleClock+=5;
    break;
  case 0x2A: /* ROL acc */
    Status &= 0xFFEF; if (Status & 0x01) Status |= 0x10;
    asm("move.w %2, %%ccr\n\troxl.b #1, %0\n\tmove.w %%sr, %1\n\tandi.w #0x0002, %2\n\tor.w %1, %2\n\tmove.b %0, %3"
	:
	: "d" (_A), "d" (Status), "m" (Status), "m" (_A)
	);
    AppleClock+=2;
    break;
  case 0x36: /* ROL zp, X */
    mr_eazpx(MemValue);
    Status &= 0xFFEF; if (Status & 0x01) Status |= 0x10;
    asm("move.w %2, %%ccr\n\troxl.b #1, %0\n\tmove.w %%sr, %1\n\tandi.w #0x0002, %2\n\tor.w %1, %2"
	:
	: "d" (MemValue), "d" (Status), "m" (Status)
	);

    _AppleMemory[TempByte] = MemValue;
    AppleClock+=6;
    break;
  case 0x3E: /* ROL abs, X */
    mr_eaabsx(MemValue);
    Status &= 0xFFEF; if (Status & 0x01) Status |= 0x10;
    asm("move.w %2, %%ccr\n\troxl.b #1, %0\n\tmove.w %%sr, %1\n\tandi.w #0x0002, %2\n\tor.w %1, %2"
	:
	: "d" (MemValue), "d" (Status), "m" (Status)
	);

    MEMORY_WRITE(MemValue, TempWord);
    AppleClock+=7;
    break;

  case 0x6E: /* ROR abs */
    mr_eaabs(MemValue);
    Status &= 0xFFEF; if (Status & 0x01) Status |= 0x10;
    asm("move.w %2, %%ccr\n\troxr.b #1, %0\n\tmove.w %%sr, %1\n\tandi.w #0x0002, %2\n\tor.w %1, %2"
	:
	: "d" (MemValue), "d" (Status), "m" (Status)
	);

    MEMORY_WRITE(MemValue, TempWord);
    AppleClock+=6;
    break;
  case 0x66: /* ROR zp */
    eazp(MemValue);
    Status &= 0xFFEF; if (Status & 0x01) Status |= 0x10;
    asm("move.w %2, %%ccr\n\troxr.b #1, %0\n\tmove.w %%sr, %1\n\tandi.w #0x0002, %2\n\tor.w %1, %2"
	:
	: "d" (MemValue), "d" (Status), "m" (Status)
	);

    _AppleMemory[TempByte] = MemValue;
    AppleClock+=5;
    break;
  case 0x6A: /* ROR acc */
    Status &= 0xFFEF; if (Status & 0x01) Status |= 0x10;
    asm("move.w %2, %%ccr\n\troxr.b #1, %0\n\tmove.w %%sr, %1\n\tandi.w #0x0002, %2\n\tor.w %1, %2\n\tmove.b %0, %3"
	:
	: "d" (_A), "d" (Status), "m" (Status), "m" (_A)
	);
    AppleClock+=2;
    break;
  case 0x76: /* ROR zp, X */
    mr_eazpx(MemValue);
    Status &= 0xFFEF; if (Status & 0x01) Status |= 0x10;
    asm("move.w %2, %%ccr\n\troxr.b #1, %0\n\tmove.w %%sr, %1\n\tandi.w #0x0002, %2\n\tor.w %1, %2"
	:
	: "d" (MemValue), "d" (Status), "m" (Status)
	);

    _AppleMemory[TempByte] = MemValue;
    AppleClock+=6;
    break;
  case 0x7E: /* ROR abs, X */
    mr_eaabsx(MemValue);
    Status &= 0xFFEF; if (Status & 0x01) Status |= 0x10;
    asm("move.w %2, %%ccr\n\troxr.b #1, %0\n\tmove.w %%sr, %1\n\tandi.w #0x0002, %2\n\tor.w %1, %2"
	:
	: "d" (MemValue), "d" (Status), "m" (Status)
	);

    MEMORY_WRITE(MemValue, TempWord);
    AppleClock+=7;
    break;

  case 0x40: /* RTI */
    POP(MemValue);
    asm("clr.w %0"
	:
	: "m" (Status)
	);
    /* N Flags */
    asm("move %2, %1\n\tandi.w #0x80, %1\n\tlsr.w #4, %1\n\tor.w %1, %0"
	:
	: "m" (Status), "d" (Status), "d" (MemValue)
	);
    /* V Flags */
    asm("move %2, %1\n\tandi.w #0x40, %1\n\tlsr.w #5, %1\n\tor.w %1, %0"
	:
	: "m" (Status), "d" (Status), "d" (MemValue)
	);
    /* Z Flags */
    asm("move %2, %1\n\tandi.w #0x02, %1\n\tlsl.w #1, %1\n\tor.w %1, %0"
	:
	: "m" (Status), "d" (Status), "d" (MemValue)
	);
    /* C Flags */
    asm("move %2, %1\n\tandi.w #0x01, %1\n\tor.w %1, %0"
	:
	: "m" (Status), "d" (Status), "d" (MemValue)
	);
    /* D Flags */
    asm("move %2, %1\n\tandi.w #0x08, %1\n\tmove.b %1, %0"
	:
	: "m" (Decimal), "d" (Status), "d" (MemValue)
	);
    POP(PC.Byte[LOW_BYTE]);
    POP(PC.Byte[HIGH_BYTE]);
    AppleClock+=6;
    break;

  case 0x60: /* RTS */
    POP(PC.Byte[LOW_BYTE]);
    POP(PC.Byte[HIGH_BYTE]);
    PC.PC++;
    AppleClock+=6;
    break;

  case 0xE9: /* SBC #imm */
    // eaimm(MemValue);
    PC.PC++;
    if (Status & 0x01) Status &= 0xFFEE; else Status |= 0x0011;
    Status |= 0x04;
    asm("move.b %0, %1\n\tbne .sbc_e9_decimal" : : "m" (Decimal), "d" (Decimal));
    asm("move.w %0, %%ccr\n\tsubx.b %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	: "=m" (Status)
	: "d" (_A), "d" (PreFetch), "m" (_A)
	);
    asm("bra .sbc_e9_finish");
    asm(".sbc_e9_decimal:");
    asm("move.w %0, %%ccr\n\tsbcd %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	: "=m" (Status)
	: "d" (_A), "d" (PreFetch), "m" (_A)
	);
    asm(".sbc_e9_finish:");
    if (Status & 0x01) Status &= 0xFFEE; else Status |= 0x0011;
    AppleClock+=2;
    break;
  case 0xED: /* SBC abs */
    mr_eaabs(MemValue);
    if (Status & 0x01) Status &= 0xFFEE; else Status |= 0x0011;
    Status |= 0x04;
    asm("move.b %0, %1\n\tbne .sbc_ed_decimal" : : "m" (Decimal), "d" (Decimal));
    asm("move.w %0, %%ccr\n\tsubx.b %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	: "=m" (Status)
	: "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm("bra .sbc_ed_finish");
    asm(".sbc_ed_decimal:");
    asm("move.w %0, %%ccr\n\tsbcd %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	: "=m" (Status)
	: "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm(".sbc_ed_finish:");
    if (Status & 0x01) Status &= 0xFFEE; else Status |= 0x0011;
    AppleClock+=4;
    break;
  case 0xE5: /* SBC zp */
    eazp(MemValue);
    if (Status & 0x01) Status &= 0xFFEE; else Status |= 0x0011;
    Status |= 0x04;
    asm("move.b %0, %1\n\tbne .sbc_e5_decimal" : : "m" (Decimal), "d" (Decimal));
    asm("move.w %0, %%ccr\n\tsubx.b %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	: "=m" (Status)
	: "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm("bra .sbc_e5_finish");
    asm(".sbc_e5_decimal:");
    asm("move.w %0, %%ccr\n\tsbcd %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	: "=m" (Status)
	: "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm(".sbc_e5_finish:");
    if (Status & 0x01) Status &= 0xFFEE; else Status |= 0x0011;
    AppleClock+=3;
    break;
  case 0xE1: /* SBC (zp, X) */
    mr_eazpxind(MemValue);
    if (Status & 0x01) Status &= 0xFFEE; else Status |= 0x0011;
    Status |= 0x04;
    asm("move.b %0, %1\n\tbne .sbc_e1_decimal" : : "m" (Decimal), "d" (Decimal));
    asm("move.w %0, %%ccr\n\tsubx.b %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	: "=m" (Status)
	: "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm("bra .sbc_e1_finish");
    asm(".sbc_e1_decimal:");
    asm("move.w %0, %%ccr\n\tsbcd %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	: "=m" (Status)
	: "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm(".sbc_e1_finish:");
    if (Status & 0x01) Status &= 0xFFEE; else Status |= 0x0011;
    AppleClock+=6;
    break;
  case 0xF1: /* SBC (zp), Y */
    mr_eazpindy(MemValue);
    if (Status & 0x01) Status &= 0xFFEE; else Status |= 0x0011;
    Status |= 0x04;
    asm("move.b %0, %1\n\tbne .sbc_f1_decimal" : : "m" (Decimal), "d" (Decimal));
    asm("move.w %0, %%ccr\n\tsubx.b %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	: "=m" (Status)
	: "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm("bra .sbc_f1_finish");
    asm(".sbc_f1_decimal:");
    asm("move.w %0, %%ccr\n\tsbcd %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	: "=m" (Status)
	: "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm(".sbc_f1_finish:");
    if (Status & 0x01) Status &= 0xFFEE; else Status |= 0x0011;
    AppleClock+=5;
    break;
  case 0xF5: /* SBC zp, X */
    mr_eazpx(MemValue);
    if (Status & 0x01) Status &= 0xFFEE; else Status |= 0x0011;
    Status |= 0x04;
    asm("move.b %0, %1\n\tbne .sbc_f5_decimal" : : "m" (Decimal), "d" (Decimal));
    asm("move.w %0, %%ccr\n\tsubx.b %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	: "=m" (Status)
	: "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm("bra .sbc_f5_finish");
    asm(".sbc_f5_decimal:");
    asm("move.w %0, %%ccr\n\tsbcd %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	: "=m" (Status)
	: "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm(".sbc_f5_finish:");
    if (Status & 0x01) Status &= 0xFFEE; else Status |= 0x0011;
    AppleClock+=4;
    break;
  case 0xFD: /* SBC abs, X */
    mr_eaabsx(MemValue);
    if (Status & 0x01) Status &= 0xFFEE; else Status |= 0x0011;
    Status |= 0x04;
    asm("move.b %0, %1\n\tbne .sbc_fd_decimal" : : "m" (Decimal), "d" (Decimal));
    asm("move.w %0, %%ccr\n\tsubx.b %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	: "=m" (Status)
	: "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm("bra .sbc_fd_finish");
    asm(".sbc_fd_decimal:");
    asm("move.w %0, %%ccr\n\tsbcd %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	: "=m" (Status)
	: "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm(".sbc_fd_finish:");
    if (Status & 0x01) Status &= 0xFFEE; else Status |= 0x0011;
    AppleClock+=4;
    break;
  case 0xF9: /* SBC abs, Y */
    mr_eaabsy(MemValue);
    if (Status & 0x01) Status &= 0xFFEE; else Status |= 0x0011;
    Status |= 0x04;
    asm("move.b %0, %1\n\tbne .sbc_f9_decimal" : : "m" (Decimal), "d" (Decimal));
    asm("move.w %0, %%ccr\n\tsubx.b %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	: "=m" (Status)
	: "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm("bra .sbc_f9_finish");
    asm(".sbc_f9_decimal:");
    asm("move.w %0, %%ccr\n\tsbcd %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	: "=m" (Status)
	: "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm(".sbc_f9_finish:");
    if (Status & 0x01) Status &= 0xFFEE; else Status |= 0x0011;
    AppleClock+=4;
    break;

  case 0x38: /* SEC */
    asm("ori.w #0x11, %0" : "=m" (Status));    
    AppleClock+=2;
    break;
  case 0xF8: /* SED */
    asm("move.b #0x08, %0" : : "m" (Decimal));
    AppleClock+=2;
    break;
  case 0x78: /* SEI */
    AppleClock+=2;
    break;

  case 0x8D: /* STA abs */
    mw_eaabs(_A);
/*     if (!LOOP) HostTraceOutputT(sysErrorClass, "STA abs : %X %X %X\n", */
/* 				TempWord.PC, _A, _AppleMemory[TempWord.PC]); */
    AppleClock+=4;
    break;
  case 0x85: /* STA zp */
    mw_eazp(_A);
    AppleClock+=3;
    break;
  case 0x81: /* STA (zp, X) */
    mw_eazpxind(_A);
    AppleClock+=6;
    break;
  case 0x91: /* STA (zp), Y */
    mw_eazpindy(_A);
    AppleClock+=6;
    break;
  case 0x95: /* STA zp, X */
    mw_eazpx(_A);    
    AppleClock+=4;
    break;
  case 0x9D: /* STA abs, X */
    mw_eaabsx(_A);
    AppleClock+=5;
    break;
  case 0x99: /* STA abs, Y */
    mw_eaabsy(_A);
/*     if (!LOOP) HostTraceOutputT(sysErrorClass, "STA abs, Y : %X %X %X %X\n", TempWord.PC, Y, _A, PreFetch); */
    AppleClock+=5;
    break;

  case 0x8E: /* STX abs */
    mw_eaabs(X);
    AppleClock+=4;
    break;
  case 0x86: /* STX zp */
    mw_eazp(X);
    AppleClock+=3;
    break;
  case 0x96: /* STX zp, Y */
    mw_eazpy(X);
    AppleClock+=4;
    break;

  case 0x8C: /* STY abs */
    mw_eaabs(Y);
    AppleClock+=4;
    break;
  case 0x84: /* STY zp */
    mw_eazp(Y);
    AppleClock+=3;
    break;
  case 0x94: /* STY zp, X */
    mw_eazpx(Y);
    AppleClock+=4;
    break;

  case 0xAA: /* TAX */
    asm("move.b %0, %1\n\tmove.w %%sr, %3\n\tmove %2, %4\n\tandi.w #0x000C, %3\n\tandi.w #0xFFF3, %4\n\tor.b %3, %4\n\tmove.w %4, %2"
	:
	: "m" (_A), "m" (X), "m" (Status), "d" (MemValue), "d" (Status)
	);
    AppleClock+=2;
    break;
  case 0xA8: /* TAY */
    asm("move.b %0, %1\n\tmove.w %%sr, %3\n\tmove %2, %4\n\tandi.w #0x000C, %3\n\tandi.w #0xFFF3, %4\n\tor.b %3, %4\n\tmove.w %4, %2"
	:
	: "m" (_A), "m" (Y), "m" (Status), "d" (MemValue), "d" (Status)
	);
    AppleClock+=2;
    break;

  case 0xBA: /* TSX */
    asm("move.b %0, %1\n\tmove.w %%sr, %3\n\tmove %2, %4\n\tandi.w #0x000C, %3\n\tandi.w #0xFFF3, %4\n\tor.b %3, %4\n\tmove.w %4, %2"
	:
	: "m" (Stack.Byte[LOW_BYTE]), "m" (X), "m" (Status), "d" (MemValue), "d" (Status)
	);
    AppleClock+=2;
    break;
  case 0x8A: /* TXA */
    asm("move.b %0, %1\n\tmove.w %%sr, %3\n\tmove %2, %4\n\tandi.w #0x000C, %3\n\tandi.w #0xFFF3, %4\n\tor.b %3, %4\n\tmove.w %4, %2"
	:
	: "m" (X), "m" (_A), "m" (Status), "d" (MemValue), "d" (Status)
	);
    AppleClock+=2;
    break;
  case 0x9A: /* TXS */
    asm("move.b %0, %1"
	:
	: "m" (X), "m" (Stack.Byte[LOW_BYTE])
	);
    AppleClock+=2;
    break;
  case 0x98: /* TYA */
    asm("move.b %0, %1\n\tmove.w %%sr, %3\n\tmove %2, %4\n\tandi.w #0x000C, %3\n\tandi.w #0xFFF3, %4\n\tor.b %3, %4\n\tmove.w %4, %2"
	:
	: "m" (Y), "m" (_A), "m" (Status), "d" (MemValue), "d" (Status)
	);
    AppleClock+=2;
    break;
/* MC6502 Instruction Set */
  case 0x72: /* ADC (zp) */
    Status &= 0xFFEF; if (Status & 0x01) Status |= 0x10;
    Status |= 0x04;
    mr_eazpind(MemValue);
    asm("move.b %0, %1\n\tbne .adc_72_decimal" : : "m" (Decimal), "d" (Decimal));
    asm("move.w %0, %%ccr\n\taddx.b %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	:
	: "m" (Status), "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm("bra .adc_72_finish");
    asm(".adc_72_decimal:");
    asm("move.w %0, %%ccr\n\tabcd %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	:
	: "m" (Status), "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm(".adc_72_finish:");
    AppleClock+=5;
    break;
  case 0x32: /* AND (zp) */
    mr_eazpind(MemValue);
    asm("and.b %2, %1\n\tmove.w %%sr, %3\n\tmove %0, %4\n\tandi.w #0x000C, %3\n\tandi.w #0xFFF3, %4\n\tor.b %3, %4\n\tmove.w %4, %0"
	: "=m" (Status)
	: "m" (_A), "d" (MemValue), "d" (Status), "d" (MemValue)	
	);
    AppleClock+=5;
    break;
  case 0x34: /* BIT zp, X */
    mr_eazpx(MemValue);
    asm("andi.w #0xFFF1, %0\n\tand.b %2, %1\n\tmove.w %%sr, %3\n\tandi.w #0x04, %3\n\tor.w %3, %0\n\tclr.w %3\n\tmove.b %2, %3\n\tandi.w #0x0080, %3\n\tlsr.w #4, %3\n\tor.w %3, %0\n\tclr.w %3\n\tmove.b %2, %3\n\tandi.w #0x0040, %3\n\tlsr.w #5, %3\n\tor.w %3, %0"
	:
	: "m" (Status), "d" (_A), "d" (MemValue), "d" (Status)
	);
    AppleClock+=3;
    break;
  case 0x89: /* BIT #imm */
    PC.PC++;
    MemValue = PreFetch & 0xFF;
    asm("andi.w #0xFFF1, %0\n\tand.b %2, %1\n\tmove.w %%sr, %3\n\tandi.w #0x04, %3\n\tor.w %3, %0\n\tclr.w %3\n\tmove.b %2, %3\n\tandi.w #0x0080, %3\n\tlsr.w #4, %3\n\tor.w %3, %0\n\tclr.w %3\n\tmove.b %2, %3\n\tandi.w #0x0040, %3\n\tlsr.w #5, %3\n\tor.w %3, %0"
	:
	: "m" (Status), "d" (_A), "d" (MemValue), "d" (Status)
	);
    AppleClock+=2;
    break;
  case 0x3C: /* BIT abs, X */
    mr_eaabsx(MemValue);
    asm("andi.w #0xFFF1, %0\n\tand.b %2, %1\n\tmove.w %%sr, %3\n\tandi.w #0x04, %3\n\tor.w %3, %0\n\tclr.w %3\n\tmove.b %2, %3\n\tandi.w #0x0080, %3\n\tlsr.w #4, %3\n\tor.w %3, %0\n\tclr.w %3\n\tmove.b %2, %3\n\tandi.w #0x0040, %3\n\tlsr.w #5, %3\n\tor.w %3, %0"
	:
	: "m" (Status), "d" (_A), "d" (MemValue), "d" (Status)
	);
    AppleClock+=4;
    break;
  case 0x80: /* BRA rr */
    PC.PC++;
    AppleClock+=3;
    asm("ext.w %0" : : "d" (PreFetch));
    PC.PC += PreFetch;
    break;
  case 0xD2: /* CMP (zp) */
    mr_eazpind(MemValue);
    asm("neg.b %1\n\tadd.b %1, %0\n\tmove %%sr, %2\n\tandi.w #0xFFF2, %3\n\tandi.w #0x000D, %2\n\tor.w %2, %3\n\tneg.b %1"
	:
	: "d" (_A), "d" (MemValue), "d" (Status), "m" (Status)
	);
    if (_A >= MemValue) Status |= 0x01; else Status &= 0xFFFE;
    AppleClock+=5;
    break;
  case 0x3A: /* DEA acc */
    asm("addi.b #0xFF, %0\n\tmove %%sr, %1\n\tandi.w #0x000C, %1\n\tandi.w #0xFFF3, %2\n\tor.w %1, %2"
	:
	: "m" (_A), "d" (Status),  "m" (Status)
	);
    AppleClock+=2;
    break;
  case 0x52: /* EOR (zp) */
    mr_eazpind(MemValue);
    asm("eor.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (MemValue), "m" (_A), "d" (Status),  "m" (Status)
	);
    AppleClock+=5;
    break;
  case 0x1A: /* INA acc */
    asm("addi.b #0x01, %0\n\tmove %%sr, %1\n\tandi.w #0x000C, %1\n\tandi.w #0xFFF3, %2\n\tor.w %1, %2"
	:
	: "m" (_A), "d" (Status),  "m" (Status)
	);
    AppleClock+=2;
    break;
  case 0x7C: /* JMP (abs, X) */
    eaabsxind();
    AppleClock+=6;
    break;
  case 0xB2: /* LDA (zp) */
    mr_eazpind(MemValue);
    asm("move.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "d" (MemValue), "m" (_A), "d" (Status),  "m" (Status)
	);
    //    if (!LOOP) HostTraceOutputT(sysErrorClass, "LDA (zp) : %X %X %X\n", TempWord.PC, TempByte, MemValue);
    AppleClock+=5;
    break;
  case 0x12: /* ORA (zp) */
    mr_eazpind(MemValue);
    asm("or.b %2, %1\n\tmove.w %%sr, %3\n\tmove %0, %4\n\tandi.w #0x000C, %3\n\tandi.w #0xFFF3, %4\n\tor.b %3, %4\n\tmove.w %4, %0"
	: "=m" (Status)
	: "m" (_A), "d" (MemValue), "d" (Status), "d" (MemValue)	
	);
    AppleClock+=5;
    break;
  case 0xDA: /* PHX */
    PUSH(X);
    AppleClock+=3;
    break;
  case 0xFA: /* PLX */
    POP(X);
    asm("move.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "m" (X), "d" (X), "d" (Status), "m" (Status)
	);
    AppleClock+=4;
    break;
  case 0x5A: /* PHY */
    PUSH(Y);
    AppleClock+=3;
    break;
  case 0x7A: /* PLY */
    POP(Y);
    asm("move.b %0, %1\n\tmove %%sr, %2\n\tandi.w #0x000C, %2\n\tandi.w #0xFFF3, %3\n\tor.w %2, %3"
	:
	: "m" (Y), "d" (Y), "d" (Status), "m" (Status)
	);
    AppleClock+=4;
    break;
  case 0xF2: /* SBC (zp) */
    mr_eazpind(MemValue);
    if (Status & 0x01) Status &= 0xFFEE; else Status |= 0x0011;
    Status |= 0x04;
    asm("move.b %0, %1\n\tbne .sbc_f2_decimal" : : "m" (Decimal), "d" (Decimal));
    asm("move.w %0, %%ccr\n\tsubx.b %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	: "=m" (Status)
	: "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm("bra .sbc_f2_finish");
    asm(".sbc_f2_decimal:");
    asm("move.w %0, %%ccr\n\tsbcd %2, %1\n\tmove.w %%sr, %0\n\tmove.b %1, %3"
	: "=m" (Status)
	: "d" (_A), "d" (MemValue), "m" (_A)
	);
    asm(".sbc_f2_finish:");
    if (Status & 0x01) Status &= 0xFFEE; else Status |= 0x0011;
    AppleClock+=4;    
    break;
  case 0x92: /* STA (zp) */
    mw_eazpind(_A);
    AppleClock+=6;
    break;
  case 0x9C: /* STZ abs */
    mw_eaabs(0);
    AppleClock+=3;
    break;
  case 0x64: /* STZ zp */
    mw_eazp(0);
    AppleClock+=3;
    break;
  case 0x74: /* STZ zp, X */
    mw_eazpx(0);
    AppleClock+=3;
    break;
  case 0x9E: /* STZ abs, X */
    mw_eaabsx(0);
    AppleClock+=3;
    break;
  case 0x1C: /* TRB abs */
    mr_eaabs(MemValue);
    asm("andi.w #0xFFF1, %3\n\tmove.b %0, %1\n\tandi.b #0x80, %1\n\tlsr.b #0x04, %1\n\tor.w %1, %3\n\tmove.b %0, %1\n\tand.b %4, %1\n\tmove.w %%sr, %1\n\tandi.w #0x04, %1\n\tor.w %1, %3\n\tmove.b %0, %1\n\tandi.b #0x40, %1\n\tlsr.b #0x05, %1\n\tor.w %1, %3"
	:
	: "d" (MemValue), "d" (_A), "m" (Status), "d" (Status), "m" (_A)
	);
    MEMORY_WRITE(MemValue & ~_A, TempWord);
    break;
  case 0x14: /* TRB zp */
    eazp(MemValue);
    asm("andi.w #0xFFF1, %3\n\tmove.b %0, %1\n\tandi.b #0x80, %1\n\tlsr.b #0x04, %1\n\tor.w %1, %3\n\tmove.b %0, %1\n\tand.b %4, %1\n\tmove.w %%sr, %1\n\tandi.w #0x04, %1\n\tor.w %1, %3\n\tmove.b %0, %1\n\tandi.b #0x40, %1\n\tlsr.b #0x05, %1\n\tor.w %1, %3"
	:
	: "d" (MemValue), "d" (_A), "m" (Status), "d" (Status), "m" (_A)
	);
    _AppleMemory[TempByte] = MemValue;
    break;
  case 0x0C: /* TSB abs */
    mr_eaabs(MemValue);
    asm("andi.w #0xFFF1, %3\n\tmove.b %0, %1\n\tandi.b #0x80, %1\n\tlsr.b #0x04, %1\n\tor.w %1, %3\n\tmove.b %0, %1\n\tand.b %4, %1\n\tmove.w %%sr, %1\n\tandi.w #0x04, %1\n\tor.w %1, %3\n\tmove.b %0, %1\n\tandi.b #0x40, %1\n\tlsr.b #0x05, %1\n\tor.w %1, %3"
	:
	: "d" (MemValue), "d" (_A), "m" (Status), "d" (Status), "m" (_A)
	);
    MEMORY_WRITE(MemValue | _A, TempWord);
    break;
  case 0x04: /* TSB zp */
    eazp(MemValue);
    asm("andi.w #0xFFF1, %3\n\tmove.b %0, %1\n\tandi.b #0x80, %1\n\tlsr.b #0x04, %1\n\tor.w %1, %3\n\tmove.b %0, %1\n\tand.b %4, %1\n\tmove.w %%sr, %1\n\tandi.w #0x04, %1\n\tor.w %1, %3\n\tmove.b %0, %1\n\tandi.b #0x40, %1\n\tlsr.b #0x05, %1\n\tor.w %1, %3"
	:
	: "d" (MemValue), "d" (_A), "m" (Status), "d" (Status), "m" (_A)
	);
    _AppleMemory[TempByte] = MemValue | _A;
    break;
  default:
    StrPrintF(buf, "Unknown Opcode %X", Opcode);
    WinDrawChars(buf, StrLen(buf), 60, 0);
    break;
  }

  asm(".screen1:");

  if (IOUSoftSwitch[0] & SS_HIRES)
    *(--_pointer) = ~_AppleMemory[scanaddr--];
  else
    *(--_pointer) = AppleFont[_AppleMemory[scanaddr--] * 8 + chary];

  vert--;

  asm(".screen2:");

  if (vert < 0) {
#ifdef SONY
    vert = 39; hori--; chary--; chary &= 0x07;
  if (IOUSoftSwitch[0] & SS_HIRES)
    scanaddr = ScanlineAddressTableH[hori] + 39;
  else
    scanaddr = ScanlineAddressTable[hori] + 39;
#else
    vert = 19; hori--; chary--; chary &= 0x07;
  if (IOUSoftSwitch[0] & SS_HIRES)
    scanaddr = ScanlineAddressTableH[hori] + 19;
  else
    scanaddr = ScanlineAddressTable[hori] + 19;
#endif
  }

  asm(".screen3:");

  if (hori < 0) {
#ifdef SONY
    _pointer += 40 * 192;  chary = 7;
    vert = 39; hori = 191; fps++;
    if (IOUSoftSwitch[0] & SS_HIRES)
      scanaddr = ScanlineAddressTableH[hori] + 39;
    else
      if (IOUSoftSwitch[0] & SS_TEXT)
	scanaddr = ScanlineAddressTable[hori] + 39;
#else
    _pointer += 20 * 120;  chary = 7;
    vert = 19; hori = 119; fps++;
    if (IOUSoftSwitch[0] & SS_HIRES)
      scanaddr = ScanlineAddressTableH[hori] + 19;
    else
      if (IOUSoftSwitch[0] & SS_TEXT)
	scanaddr = ScanlineAddressTable[hori] + 19;
#endif
  }

  asm(".screen4:");

  if (QUIT) goto run_again;
}
