#include "6502.h"
#include "memory.h"
#include "iou.h"

#define LOW_BYTE  1
#define HIGH_BYTE 0

#define SET(x, y)	IOUState[y] = (IOUSoftSwitch[y] = IOUSoftSwitch[y] |  (x))
#define RESET(x, y)	IOUState[y] = (IOUSoftSwitch[y] = IOUSoftSwitch[y] &~ (x))

UInt8 *AppleMemory;
UInt8 *AuxMemory;
UInt8 *TempMemory;
UInt8 *AppleROM;
extern union PCStruct Address;

extern UInt8 *address_line[0x700];
extern char *AppleFontBitmap;

extern BOOL LOOP;
extern BOOL QUIT;
BOOL LOOP1 = 1;

extern UInt16 AppleClock;
extern UInt16 DiskOffset;
UInt16 LastAppleClock = 0;

UInt8 BootROM[256];
UInt8 BootImage[256];

UInt8 SlotROMStatus = 0;
UInt8 LastKey = 0;
UInt16 IOUSoftSwitch[2] = {0, 0};
UInt16 IOUState[2] = {0, 0};

UInt8 address_latch, data_latch, C800ROMSlot;

EventType event;

/* Apple IO Read */
UInt8 RIOM();

/* SLOT 5 IO Read */
UInt8 SLT5();

/* SLOT 6 IO Read */
UInt8 SLT6();

/* Apple IO Write */
void WIOM(UInt8 value);

/* Screen Write */
void WSCM(UInt8 value);

/* Protect ROM */
void READONLY(UInt8 value);

/* Aux Memory Read Write */
UInt8 AuxRead();
void AuxWrite(UInt8 value);

UInt8 REXP();

UInt8 RC3M();

void LoadROM();

void toggleLanguageCard();

#define NO_OF_PHASES 8
#define MAX_PHYSICAL_TRACK_NO (40*NO_OF_PHASES)
#define RAW_TRACK_BYTES 6392

static Int8 stepper_movement_table[16][NO_OF_PHASES] = {
  {  0,  0,  0,  0,  0,  0,  0,  0 },	/* all electromagnets off */
  {  0, -1, -2, -3,  0,  3,  2,  1 },	/* EM 1 on */
  {  2,  1,  0, -1, -2, -3,  0,  3 },	/* EM 2 on */
  {  1,  0, -1, -2, -3,  0,  3,  2 },	/* EMs 1 & 2 on */
  {  0,  3,  2,  1,  0, -1, -2, -3 },	/* EM 3 on */
  {  0, -1,  0,  1,  0, -1,  0,  1 },	/* EMs 1 & 3 on */
  {  3,  2,  1,  0, -1, -2, -3,  0 },	/* EMs 2 & 3 on */
  {  2,  1,  0, -1, -2, -3,  0,  3 },	/* EMs 1, 2 & 3 on */
  { -2, -3,  0,  3,  2,  1,  0, -1 },	/* EM 4 on */
  { -1, -2, -3,  0,  3,  2,  1,  0 },	/* EMs 1 & 4 on */
  {  0,  1,  0, -1,  0,  1,  0, -1 },	/* EMs 2 & 4 */
  {  0, -1, -2, -3,  0,  3,  2,  1 },	/* EMs 1, 2 & 4 on */
  { -3,  0,  3,  2,  1,  0, -1, -2 },	/* EMs 3 & 4 on */
  { -2, -3,  0,  3,  2,  1,  0, -1 },	/* EMs 1, 3 & 4 on */
  {  0,  3,  2,  1,  0, -1, -2, -3 },	/* EMs 2, 3 & 4 on */
  {  0,  0,  0,  0,  0,  0,  0,  0 } };	/* all electromagnets on */

static DmOpenRef        diskimage;
UInt16		motor_on;
static Int16		physical_track_no;
static Int16 		stepper_status;
static UInt16           read_count = 0;
static Int8		track_buffer_valid=0;
static Int8		track_buffer_dirty=0;
static Int8		track_buffer_track=0;
static Int8             write_mode;
static Int16            last_track = -1;
static UInt8	        nibble[RAW_TRACK_BYTES];
static UInt16    	position;
static UInt8     	write_protect;
static DmOpenRef        romDB;
static UInt8*           values;
static MemHandle        value;
static LocalID          rom;

UInt8 logical_track;

void toggleStepper();
void toggleMotor();
UInt8 read_nibble();
void load_track_buffer(void);
int mount_disk( int slot, int drive );

/* Read Operation of Memory Page Mapping */
READBYTE ReadFunction[256] = {
  /*          0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F*/
  /* 00 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  /* 10 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  /* 20 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  /* 30 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 

  /* 40 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  /* 50 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  /* 60 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  /* 70 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 

  /* 80 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  /* 90 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  /* A0 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  /* B0 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 

  /* C0 */ RIOM, NULL, NULL, RC3M, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  /* D0 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  /* E0 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  /* F0 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
};

WRITEBYTE WriteFunction[256] =  {
  /*          0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F*/
  /* 00 */ NULL, NULL, NULL, NULL, WSCM, WSCM, WSCM, WSCM, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  /* 10 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  /* 20 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  /* 30 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 

  /* 40 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  /* 50 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  /* 60 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  /* 70 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 

  /* 80 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  /* 90 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  /* A0 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  /* B0 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 

  /* C0 */ WIOM, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  /* D0 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  /* E0 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  /* F0 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
};

void init_memory(void) {
  FtrPtrNew('Pala', 0, 65536, (void**) &AppleMemory);
  MemSet(AppleMemory, 65535, 0);

  FtrPtrNew('Pala', 1, 65536, (void**) &AuxMemory);
  MemSet(AuxMemory, 65535, 0);

  FtrPtrNew('Pala', 2, 4096, (void**) &TempMemory);
  MemSet(TempMemory, 4096, 0);

  LoadROM();

  {
    UInt16 count;

    for (count = 0xD0; count < 0x100; count++) {
      WriteFunction[count] = READONLY;
    }
  }

  C800ROMSlot = 0;
}

void LoadROM(void) {
  DmOpenRef romDB;
  LocalID romID;
  MemHandle romRec;
  UInt8* roms;
  UInt32 romStart, memStart;
  MemHandle handle;

  romID  = DmFindDatabase(0, "apple2e.rom");

  romDB  = DmOpenDatabase(0, romID, dmModeReadOnly);

  romRec = DmQueryRecord(romDB, 0);

  roms   = (UInt8 *) MemHandleLock(romRec);  

  romStart = 0x004100; memStart = 0x00C100;

  handle = MemHandleNew(0x3F00);
  AppleROM = (UInt8*) MemHandleLock(handle);

  MemMove(AppleMemory + memStart, roms + romStart, 0x3F00);
  MemMove(AppleROM, roms + romStart, 0x3F00);

  MemHandleUnlock(romRec);
  DmCloseDatabase(romDB);  

  romID  = DmFindDatabase(0, "slot6.rom");

  romDB  = DmOpenDatabase(0, romID, dmModeReadOnly);

  romRec = DmQueryRecord(romDB, 0);

  roms   = (UInt8 *) MemHandleLock(romRec);  

  MemMove(BootROM, roms, 256);

  MemHandleUnlock(romRec);
  DmCloseDatabase(romDB);

  physical_track_no = 0;
  
  mount_disk(6, 0);

  position = 0; last_track = -1;

  BootROM[0x004C] = 0xA9;
  BootROM[0x004D] = 0x00;
  BootROM[0x004E] = 0xEA;

}

/* Apple ROM Paging */
UInt8 RRMP() {
  return 0x00;
}

UInt8 readKey() {

  if (EvtEventAvail()) {
    EvtGetEvent(&event, 0);
    SysHandleEvent(&event);
    if (event.eType == appStopEvent) {
      QUIT = 0;
      return LastKey;
    }
    if (event.eType == keyDownEvent) {
      UInt8 key;
      key = (event.data.keyDown.chr == 10 ? 13 : event.data.keyDown.chr) | 0x80;
      // if (event.data.keyDown.chr == 10) LOOP = 0;
      return key;
    }
  }

  return LastKey;
}

/* Apple Soft Switch Read */
UInt8 readSoftSwitch()
{
  UInt16 flag;
  UInt8  data;

  static UInt16 soft_switch_list[16] = {
    0 /* key strobe */, SS_LCBNK2, SS_LCRAM, SS_RAMRD,
    SS_RAMWRT, SS_SLOTCXROM, SS_ALTZP, SS_SLOTC3ROM,
    SS_80STORE, 0 /* VBL */, SS_TEXT, SS_MIXED,
    SS_PAGE2, SS_HIRES, SS_ALTCHAR, SS_80COL };

  data = 0;
  flag = soft_switch_list[Address.Byte[LOW_BYTE] & 0x0F];

  if (flag) {
    data = (flag & IOUSoftSwitch[0])? data | 0x80 : data & 0x7f; 
  } else {
    if (Address.PC == KBDSTRB) {
      LastKey &= 0x7F;

      LastKey = readKey();

      return LastKey;
    } else if (Address.PC == RDVBLBAR) {
      data = (SS_VBLBAR & IOUSoftSwitch[1])? data | 0x80 : data & 0x7f;
    }
  }
  return data;
}

UInt8 REXP() {
  if (!(IOUState[0] & SS_SLOTCXROM) && Address.PC == 0xCFFF) {
    // MemMove(AppleMemory + 0xCD00, BootROM, 256);
    MemMove(AppleMemory + 0xCE00, BootROM, 256);
    RESET(SS_EXPNROM, 1);
    return 0x00;
  } else {
    return AppleMemory[Address.PC];
  }
}

UInt8 RC3M() {
  if (!(IOUSoftSwitch[0] & SS_SLOTC3ROM)) {
    if (C800ROMSlot == 0) {
      C800ROMSlot = 3;
      SET(SS_EXPNROM, 1);
    }
    MemMove(AppleMemory + 0xC800, AppleROM + 0x700, 2048);
  }

  return AppleMemory[Address.PC];
}

void toggleVideoSoftSwitch() {
  UInt8 vswitch = Address.Byte[LOW_BYTE] & 0x0F;

  switch (vswitch) {
  case 0x00:
    RESET(SS_TEXT, 0);
    break;
  case 0x01:
    SET(SS_TEXT, 0);
    RESET(SS_HIRES, 0);
    break;
  case 0x04:
    if (IOUSoftSwitch[0] & SS_PAGE2){
      RESET(SS_PAGE2, 0);
      if (IOUSoftSwitch[0] & SS_80STORE) {
	ReadFunction[0x04] = NULL;
	ReadFunction[0x05] = NULL;
	ReadFunction[0x06] = NULL;
	ReadFunction[0x07] = NULL;
	WriteFunction[0x04] = NULL;
	WriteFunction[0x05] = NULL;
	WriteFunction[0x06] = NULL;
	WriteFunction[0x07] = NULL;
      }
    }
    break;
  case 0x05:
    if (!(IOUSoftSwitch[0] & SS_PAGE2)){
      SET(SS_PAGE2, 0);
      if (IOUSoftSwitch[0] & SS_80STORE) {
	ReadFunction[0x04] = RRMP;
	ReadFunction[0x05] = RRMP;
	ReadFunction[0x06] = RRMP;
	ReadFunction[0x07] = RRMP;
	WriteFunction[0x04] = READONLY;
	WriteFunction[0x05] = READONLY;
	WriteFunction[0x06] = READONLY;
	WriteFunction[0x07] = READONLY;
      }
    }
    break;
  case 0x06:
    if (IOUSoftSwitch[0] & SS_HIRES)
      RESET(SS_HIRES, 0);
    break;
  case 0x07:
    if (!(IOUSoftSwitch[0] & SS_HIRES)) {
      SET(SS_HIRES, 0);
      RESET(SS_TEXT, 0);
    }
    break;
  }
}

void toggleMemorySoftSwitch(UInt8 data) {
  UInt8 Page;

  switch (Address.PC) {
  case SETSLOTCXROM:
    RESET(SS_SLOTCXROM, 0);
    for (Page = 0xC1; Page < 0xC8; Page++) if (Page != 0xC3) ReadFunction[Page] = RRMP;
    MemSet(AppleMemory + 0xC100, 512, 0);
    MemSet(AppleMemory + 0xC400, 1024, 0);
    ReadFunction[0xC6] = NULL;
    MemMove(AppleMemory + 0x00C600, BootROM, 256);
    break;
  case SETINTCXROM:    
    SET(SS_SLOTCXROM, 0);
    for (Page = 0xC1; Page < 0xC8; Page++) if (Page != 0xC3) ReadFunction[Page] = NULL;
    MemMove(AppleMemory + 0xC100, AppleROM, 512);
    MemMove(AppleMemory + 0xC400, AppleROM + 0x300, 1024);
    break;
  case SETSTDZP:
    break;
  case SETALTZP:
    break;
  case SETINTC3ROM:
    MemMove(AppleMemory + 0xC300, AppleROM + 0x200, 256);
    SET(SS_SLOTC3ROM, 0);
    break;
  case SETSLOTC3ROM:
    MemSet(AppleMemory + 0xC300, 256, 0);
    RESET(SS_SLOTC3ROM, 0);
    break;
  case RDMAINRAM:
    if (IOUSoftSwitch[0] & SS_RAMRD){
      RESET(SS_RAMRD, 0);
    }
    break;
  case RDCARDRAM:	
    SET(SS_RAMRD, 0);
    break;
  case WRMAINRAM:	
    RESET(SS_RAMWRT, 0);
    break;
  case WRCARDRAM:	
    SET(SS_RAMWRT, 0);
    break;
  case CLR80STORE:
    if (IOUSoftSwitch[0] & SS_80STORE) {
      RESET(SS_80STORE, 0);
    }
    break;
  case SET80STORE:
    if (!(IOUSoftSwitch[0] & SS_80STORE)) {
      SET(SS_80STORE, 0);
    }
    break;
  }

  if (IOUState[0] & SS_RAMRD) {
    UInt16 Page;
    for (Page = 0x02; Page < 0xC0; Page++) ReadFunction[Page] = RRMP;
  } else {
    UInt16 Page;
    for (Page = 0x02; Page < 0xC0; Page++) ReadFunction[Page] = NULL;
  }


  if (IOUState[0] & SS_RAMWRT) {
    UInt16 Page;
    for (Page = 0x02; Page < 0xC0; Page++) WriteFunction[Page] = READONLY;
  } else {
    UInt16 Page;
    for (Page = 0x02; Page < 0xC0; Page++) WriteFunction[Page] = NULL;
  }
}

/* Apple IO Read */
UInt8 RIOM() {
  UInt8 value, soft;

  value = 0x00;
  //  soft = (UInt8) (Address.Byte[LOW_BYTE] & (UInt8) (0xF0));

  switch (Address.Byte[LOW_BYTE]) {
  case 0x00: case 0x01: case 0x02: case 0x03:
  case 0x04: case 0x05: case 0x06: case 0x07:
  case 0x08: case 0x09: case 0x0A: case 0x0B:
  case 0x0C: case 0x0D: case 0x0E: case 0x0F:
    LastKey = readKey();
    value = LastKey;
    break;
  case 0x10: case 0x11: case 0x12: case 0x13:
  case 0x14: case 0x15: case 0x16: case 0x17:
  case 0x18: case 0x19: case 0x1A: case 0x1B:
  case 0x1C: case 0x1D: case 0x1E: case 0x1F:
    value = readSoftSwitch();
    break;
  case 0x20: case 0x21: case 0x22: case 0x23:
  case 0x24: case 0x25: case 0x26: case 0x27:
  case 0x28: case 0x29: case 0x2A: case 0x2B:
  case 0x2C: case 0x2D: case 0x2E: case 0x2F:

  case 0x30: case 0x31: case 0x32: case 0x33:
  case 0x34: case 0x35: case 0x36: case 0x37:
  case 0x38: case 0x39: case 0x3A: case 0x3B:
  case 0x3C: case 0x3D: case 0x3E: case 0x3F:

  case 0x40: case 0x41: case 0x42: case 0x43:
  case 0x44: case 0x45: case 0x46: case 0x47:
  case 0x48: case 0x49: case 0x4A: case 0x4B:
  case 0x4C: case 0x4D: case 0x4E: case 0x4F:

    break;
  case 0x50: case 0x51: case 0x52: case 0x53:
  case 0x54: case 0x55: case 0x56: case 0x57:
  case 0x58: case 0x59: case 0x5A: case 0x5B:
  case 0x5C: case 0x5D: case 0x5E: case 0x5F:
    toggleVideoSoftSwitch();
    break;
  case 0x60: case 0x61: case 0x62: case 0x63:
  case 0x64: case 0x65: case 0x66: case 0x67:
  case 0x68: case 0x69: case 0x6A: case 0x6B:
  case 0x6C: case 0x6D: case 0x6E: case 0x6F:

  case 0x70: case 0x71: case 0x72: case 0x73:
  case 0x74: case 0x75: case 0x76: case 0x77:
  case 0x78: case 0x79: case 0x7A: case 0x7B:
  case 0x7C: case 0x7D: case 0x7E: case 0x7F:

    break;
    /* Slot 0 */
  case 0x80: case 0x81: case 0x82: case 0x83:
  case 0x84: case 0x85: case 0x86: case 0x87:
  case 0x88: case 0x89: case 0x8A: case 0x8B:
  case 0x8C: case 0x8D: case 0x8E: case 0x8F:
    toggleLanguageCard();
    break;
    /* Slot 1 */
  case 0x90: case 0x91: case 0x92: case 0x93:
  case 0x94: case 0x95: case 0x96: case 0x97:
  case 0x98: case 0x99: case 0x9A: case 0x9B:
  case 0x9C: case 0x9D: case 0x9E: case 0x9F:
    break;
    /* Slot 2 */
  case 0xA0: case 0xA1: case 0xA2: case 0xA3:
  case 0xA4: case 0xA5: case 0xA6: case 0xA7:
  case 0xA8: case 0xA9: case 0xAA: case 0xAB:
  case 0xAC: case 0xAD: case 0xAE: case 0xAF:
    break;
    /* Slot 3 */
  case 0xB0: case 0xB1: case 0xB2: case 0xB3:
  case 0xB4: case 0xB5: case 0xB6: case 0xB7:
  case 0xB8: case 0xB9: case 0xBA: case 0xBB:
  case 0xBC: case 0xBD: case 0xBE: case 0xBF:
    break;
    /* Slot 4 */
  case 0xC0: case 0xC1: case 0xC2: case 0xC3:
  case 0xC4: case 0xC5: case 0xC6: case 0xC7:
  case 0xC8: case 0xC9: case 0xCA: case 0xCB:
  case 0xCC: case 0xCD: case 0xCE: case 0xCF:
    break;
    /* Slot 5 */
  case 0xD0: case 0xD1: case 0xD2: case 0xD3:
  case 0xD4: case 0xD5: case 0xD6: case 0xD7:
  case 0xD8: case 0xD9: case 0xDA: case 0xDB:
  case 0xDC: case 0xDD: case 0xDE: case 0xDF:
    break;
    /* Slot 6 */
  case 0xEC:
/*       if ( address_latch == 0x0E ) */
/* 	write_mode = 0; */
/*       else if ( address_latch == 0x0F ) */
/* 	write_mode = 1; */

/*       if ( !write_mode ) */

/*     value = read_nibble(); */

/*     AppleMemory[Address.PC] = value; */
    
    value = nibble[position++];

    if ( position >= RAW_TRACK_BYTES ) position = 0;

    address_latch = 0x0C;
    break;
  case 0xED:
    address_latch = 0x0D;
    break;
  case 0xEE:
    address_latch = 0x0E;
    if ( address_latch == 0x0D )	/* check write protect */
      value = 0xFF;
    break;
  case 0xEF:
    address_latch = 0x0F;
    break;      
  case 0xE8:	/* motor off */
  case 0xE9:	/* motor on */
    toggleMotor();
    break;
  case 0xEA:	/* select driver 1 */
  case 0xEB:	/* select driver 2 */
    break;
    /* C0X0-C0X7, stepper toggle */
  case 0xE0: case 0xE1: case 0xE2: case 0xE3:
  case 0xE4: case 0xE5: case 0xE6: case 0xE7:
    toggleStepper( );
    break;
    /* Slot 7 */
  case 0xF0: case 0xF1: case 0xF2: case 0xF3:
  case 0xF4: case 0xF5: case 0xF6: case 0xF7:
  case 0xF8: case 0xF9: case 0xFA: case 0xFB:
  case 0xFC: case 0xFD: case 0xFE: case 0xFF:
    break;
  }
  
  return value;
}

/* SLOT 5 IO Read */
UInt8 SLT5() {
  Char buf[20];
  StrPrintF(buf, "Boot : %4X", Address.PC);
  WinDrawChars(buf, StrLen(buf), 50, 0);
  return BootROM[Address.Byte[LOW_BYTE]];
}

/* SLOT 6 IO Read */
UInt8 SLT6() {
  Char buf[20];
  StrPrintF(buf, "Boot : %4X", Address.PC);
  WinDrawChars(buf, StrLen(buf), 50, 0);
  return BootROM[Address.Byte[LOW_BYTE]];
}

/* Apple IO Write */
void WIOM(UInt8 value) {
  AppleMemory[Address.PC] = value;

  switch ((UInt8) (Address.Byte[LOW_BYTE] & (UInt8) (0xF0))) {
  case 0x00:
    toggleMemorySoftSwitch(value);
/*     HostTraceOutputTL(sysErrorClass, "Write Soft Card : %X %X", Address.PC, value); */
    break;
  case 0x10:
    if (Address.PC == KBDSTRB)
      LastKey &= 0x7F;
    break;
  case 0x20: case 0x30: case 0x40:
    break;
  case 0x50:
    toggleVideoSoftSwitch();
/*     HostTraceOutputTL(sysErrorClass, "Write Video Card : %X", Address.PC);     */
    break;
  case 0x60: case 0x70:
    break;
  case 0x80: /* Slot 0 */
    toggleLanguageCard();
/*     HostTraceOutputTL(sysErrorClass, "Write Language Card : %X %X", Address.PC, value); */
    break;
  case 0x90: /* Slot 1 */
    break;
  case 0xA0: /* Slot 2 */
    break;
  case 0xB0: /* Slot 3 */
    break;
  case 0xC0: /* Slot 4 */
    break;
  case 0xD0: /* Slot 5 */
    break;
  case 0xE0: /* Slot 6 */
    switch ((UInt8) (Address.Byte[LOW_BYTE] & (UInt8) (0x0F))) {
    case 0x0C:
      if ( address_latch == 0x0E )
	write_mode = 0;
      else if ( address_latch == 0x0F )
	write_mode = 1;
      address_latch = 0x0C;
      return;
    case 0x0D:
      address_latch = 0x0D;
      break;
    case 0x0E:
      if ( address_latch == 0x0D ) {	/* check write protect */
	address_latch = 0x0E;
	return;
      }
      address_latch = 0x0E;
      break;
    case 0x0F:
      address_latch = 0x0F;
      break;      
    case 0x08:	/* motor off */
    case 0x09:	/* motor on */
      toggleMotor();
      break;
    case 0x0A:	/* select driver 1 */
    case 0x0B:	/* select driver 2 */
      break;
    default:	/* C0X0-C0X7, stepper toggle */
      toggleStepper( );
      break;
    }
  }
}

UInt8 AuxRead() {
  UInt8 value;
  if ((IOUState[0] & SS_LCRAMRD))
    value = AuxMemory[Address.PC - ((!(IOUState[0] & SS_LCBNK2) && Address.PC < 0xE000) ? 0x1000 : 0x000)];
  else
    value = AppleMemory[Address.PC];

  return value;
}

void AuxWrite(UInt8 value) {
    AuxMemory[Address.PC - ((!(IOUState[0] & SS_LCBNK2) &&  Address.PC < 0xE000) ? 0x1000 : 0x000)] = value;
    AppleMemory[Address.PC] = value;
}

void READONLY(UInt8 data) {
  return;
}

/* Screen Write */
void WSCM(UInt8 data) {
  AppleMemory[Address.PC] = data;

  /*
  {
    UInt8 *line, value;
    UInt16 count, count1, count2, value1;

    value = data;
    if (value < 0xE0) value &= 0x3F;
    if (value >= 0xE0) value = value - 0xA0;
    count1 = value << 3; count2 = count1 + 8;
    if ((Address.PC - 0x400) < 0x800) {
      line = address_line[Address.PC - 0x400];
      value1 = 0;
      for (count = count1; count < count2; count++) {
	line[value1] = ~AppleFontBitmap[count];
	value1 += 40;
      }
    }
  }
  */
}

void toggleStepper()
{
  register Int16 magnet, on, old_track_no, new_status,phase;

  magnet = (Address.Byte[LOW_BYTE] & 0x0E) >> 1;
  on = Address.Byte[LOW_BYTE] & 0x01;
  old_track_no = physical_track_no;
  if ( on )
    stepper_status |= (1<<magnet); 
  else
    stepper_status &= ~(1<<magnet); 
  if ( motor_on ) {
    new_status = stepper_status;
    phase = physical_track_no & 0x07;
    physical_track_no += stepper_movement_table[new_status][phase];
    if ( physical_track_no < 0 ) {
      physical_track_no = 0;
    }
    else if ( physical_track_no >= MAX_PHYSICAL_TRACK_NO ) 
      physical_track_no = MAX_PHYSICAL_TRACK_NO-1;
    track_buffer_valid = 0;
  }

  if ( !track_buffer_valid ) {
    load_track_buffer();
  }
}

void toggleMotor( )
{
  int	on;

  on = Address.Byte[LOW_BYTE] & 0x01;

  if ( motor_on != on ) {
    WinDrawChars("I", 1, 10, 150);
    motor_on = on;
  } else {
    WinDrawChars("  ", 1, 10, 150);
  }

}

UInt8 read_nibble()
{ 
  UInt8	data;

  if ( !track_buffer_valid ) {
    load_track_buffer();
  }

  data = nibble[position++];

  if ( position >= RAW_TRACK_BYTES ) position = 0;

  return data;
}

int mount_disk( int slot, int drive )
{
  write_protect = 0;

  rom = DmFindDatabase(0, "prodos.dsk");
  diskimage = DmOpenDatabase(0, rom, dmModeReadOnly);

  load_track_buffer();

  return 0;
}

void load_track_buffer(void)
{
  Char  test[20];

  logical_track = (physical_track_no + 1) >> 2;

  if (logical_track == last_track) {
    position = 0;
    track_buffer_track = physical_track_no;
    track_buffer_dirty = 0;
    track_buffer_valid = 1;
    return;
  }

  last_track = logical_track;

  StrPrintF(test, "Track : %d    ", logical_track);
  WinDrawChars(test, StrLen(test), 50, 150);

  value = DmQueryRecord(diskimage, logical_track);
  values = (UInt8 *) MemHandleLock(value);

  MemMove(nibble, values, 6392);

  track_buffer_track = physical_track_no;
  track_buffer_dirty = 0;
  track_buffer_valid = 1;

  position = 0;

  MemHandleUnlock(value);
}

void toggleLanguageCard( )
{
/*   UInt8 old_ss, old_ss1; */

/*   old_ss = IOUSoftSwitch[0]; */
/*   old_ss1 = IOUSoftSwitch[1]; */

  /* set IOU soft switches */
  switch(Address.Byte[LOW_BYTE] & 0x0F){
  case 0:
    SET(SS_LCBNK2, 0);
    SET(SS_LCRAMRD, 0);
    RESET(SS_LCRAMWRT, 1);
    break;
  case 1:
    SET(SS_LCBNK2, 0);
    RESET(SS_LCRAMRD, 0);
    SET(SS_LCRAMWRT, 1);
    break;
  case 2:
    SET(SS_LCBNK2, 0);
    RESET(SS_LCRAMRD, 0);
    RESET(SS_LCRAMWRT, 1);
    break;
  case 3:
    SET(SS_LCBNK2, 0);
    SET(SS_LCRAMRD, 0);
    SET(SS_LCRAMWRT, 1);
    break;
  case 8:
    RESET(SS_LCBNK2, 0);
    SET(SS_LCRAMRD, 0);
    RESET(SS_LCRAMWRT, 1);
    break;
  case 9:
    RESET(SS_LCBNK2, 0);
    RESET(SS_LCRAMRD, 0);
    SET(SS_LCRAMWRT, 1);
    break;
  case 0xA:
    RESET(SS_LCBNK2, 0);
    RESET(SS_LCRAMRD, 0);
    RESET(SS_LCRAMWRT, 1);
    break;
  case 0xB:
    RESET(SS_LCBNK2, 0);
    SET(SS_LCRAMRD, 0);
    SET(SS_LCRAMWRT, 1);
  }

  if ((IOUState[1] & SS_LCRAMWRT)) {
      UInt16 Page;
      for (Page = 0xD0; Page <= 0xFF; Page++) WriteFunction[Page] = AuxWrite;
  } else {
      UInt16 Page;
      for (Page = 0xD0; Page <= 0xFF; Page++) WriteFunction[Page] = READONLY;
  }

  if ((IOUState[0] & SS_LCRAMRD)) {
    UInt16 Page;

    MemMove(AppleMemory + 0xD000, AuxMemory + 0xD000 - (!(IOUState[0] & SS_LCBNK2) ? 0x1000 : 0x000), 4096);

    MemMove(AppleMemory + 0xE000, AuxMemory + 0xE000, 8192);

    for (Page = 0xD0; Page <= 0xFF; Page++) ReadFunction[Page] = NULL;
  } else {
    UInt16 Page;

    MemMove(AppleMemory + 0xD000, AppleROM + 0xD000 - 0xC100, 4096);
    MemMove(AppleMemory + 0xE000, AppleROM + 0xE000 - 0xC100, 8192);

    for (Page = 0xD0; Page <= 0xFF; Page++) ReadFunction[Page] = NULL;
  }
}
