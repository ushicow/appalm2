#include <PalmOS.h>
#include "Apple2.h"
extern UInt16 grMode;

UInt16    memIOU, vidIOU, kbdIOU, spkrIOU, btnIOU[3], pdlIOU[2], pdlResetIOU[2];
UInt8     kbdBuffer[16];
UInt8     kbdHead, kbdCount;
UInt8     *AppleMemory;
UInt8     *AuxMemory;
UInt8     *AppleROM;
UInt8     *BootROM;
READBYTE  *ReadFunction;
WRITEBYTE *WriteFunction;
UInt8      auxDirty = 0;

void showDiskName(UInt16, Char *);
void showDriveState(UInt16, Boolean);
UInt8 RIOM(UInt16);
void  WIOM(UInt16, UInt8);
UInt8 RC3M(UInt16);
void clrVidFuncs(UInt16);
void  toggleVideoSoftSwitch(UInt8);

/*
 * Function prototypes  for sector nibblizing
 */
static void init_GCR_table(void);
static UInt8 gcr_read_nibble(void);
static void gcr_write_nibble( UInt8 );
static void decode62( UInt8* );
static void encode62( UInt8* );
static void FM_encode( UInt8 );
static UInt8 FM_decode(void);
static void write_sync( int );
static int read_address_field( int*, int*, int* );
static void write_address_field( int, int, int );
static int read_data_field(void);
static void write_data_field(void);
int NibblesToSectors(UInt8 *nibbles, UInt8 *sectors, int volume, int track);
void SectorsToNibbles(UInt8 *sectors, UInt8 *nibbles, int volume, int track);

void initMemory(void)
{
    vidIOU         = SS_TEXT;
    memIOU         =
    kbdIOU         =
    spkrIOU        =
    btnIOU[0]      =
    btnIOU[1]      =
    btnIOU[2]      =
    pdlIOU[0]      =
    pdlIOU[1]      = 0;
    pdlResetIOU[0] =
    pdlResetIOU[1] = 0x80;
    kbdHead        =
    kbdCount       = 0;
    MemSet(AppleMemory, 65536, 0);
    MemSet(AuxMemory, 16384+2048, 0);
    MemSet(ReadFunction, 1024, 0);
    MemSet(WriteFunction, 1024, 0);
    loadROM();
    MemMove(AppleMemory + 0xC100, AppleROM, 0x3F00);
    setMemFuncs();
}

/*****************************************************************************\
*                                                                             *
*                                ROM Access                                   *
*                                                                             *
\*****************************************************************************/

/* Apple ROM Paging */

void READONLY(UInt16, UInt8);
UInt8 RRMP(UInt16);
void awrap_ROMReadWrite(void)
{
    asm(".global RRMP");
    asm(".global READONLY");
    asm("RRMP:");
    asm("clr.l  %d0");
    asm("READONLY:");
    asm("rts");
}
RDMEM_CWRAP(REXP, address)
{
    if (!SS_ISSET(memIOU, SS_SLOTCXROM) && address == 0xCFFF)
    {
        MemMove(AppleMemory + 0xCE00, BootROM, 256);
        SS_RESET(memIOU, SS_EXPNROM);
        return (0x00);
    }
    else
    {
        return (AppleMemory[address]);
    }
}
RDMEM_CWRAP(RC3M, address)
{
    if (!SS_ISSET(memIOU, SS_SLOTC3ROM))
    {
        SS_SET(memIOU, SS_EXPNROM);
        MemMove(AppleMemory + 0xC800, AppleROM + 0x700, 2048);
    }
    return (AppleMemory[address]);
}
void loadROM(void)
{
    DmOpenRef       romDB;
    LocalID         romID;
    MemHandle       romRec;
    UInt8           *roms;
    UInt32          romStart;
    UInt16          count;

    romID = DmFindDatabase(0, "apple2e.rom");
    romDB = DmOpenDatabase(0, romID, dmModeReadOnly);
    romRec = DmQueryRecord(romDB, 0);
    roms = (UInt8 *) MemHandleLock(romRec);
    romStart = 0x004100;
    AppleROM = MemPtrNew(0x3F00);
    MemMove(AppleROM, roms + romStart, 0x3F00);
    MemHandleUnlock(romRec);
    DmCloseDatabase(romDB);
    romID = DmFindDatabase(0, "slot6.rom");
    romDB = DmOpenDatabase(0, romID, dmModeReadOnly);
    romRec = DmQueryRecord(romDB, 0);
    roms = (UInt8 *) MemHandleLock(romRec);
    BootROM = MemPtrNew(256);
    MemMove(BootROM, roms, 256);
    MemHandleUnlock(romRec);
    DmCloseDatabase(romDB);
    BootROM[0x004C] = 0xA9;
    BootROM[0x004D] = 0x00;
    BootROM[0x004E] = 0xEA;
}
void unloadROM(void)
{
    MemPtrFree(AppleROM);
    MemPtrFree(BootROM);
}

/*****************************************************************************\
*                                                                             *
*                                Soft switches                                *
*                                                                             *
\*****************************************************************************/

void AuxWriteBnk1(UInt16, UInt8);
void AuxWriteBnk2(UInt16, UInt8);
void AuxWrite(UInt16, UInt8);
void AuxMapWriteBnk1(UInt16, UInt8);
void AuxMapWriteBnk2(UInt16, UInt8);
void AuxMapWrite(UInt16, UInt8);
UInt8 AuxReadBnk1(UInt16);
UInt8 AuxReadBnk2(UInt16);
UInt8 AuxRead(UInt16);
UInt8 RomRead(UInt16);
void awrap_AuxMemReadWrite(void)
{
    asm(".global AuxWriteBnk1");
    asm(".global AuxWriteBnk2");
    asm(".global AuxWrite");
    asm(".global AuxMapWriteBnk1");
    asm(".global AuxMapWriteBnk2");
    asm(".global AuxMapWrite");
    asm(".global AuxReadBnk1");
    asm(".global AuxReadBnk2");
    asm(".global AuxRead");
    asm(".global RomRead");
    asm("AuxWriteBnk1:");
    asm("move.l AuxMemory@END.w(%a5), %a6");
    asm("sub.l  #0xD000, %a6");
    asm("move.b %d0, (%a6, %d6.l)");
    asm("rts");
    asm("AuxWriteBnk2:");
    asm("AuxWrite:");
    asm("move.l AuxMemory@END.w(%a5), %a6");
    asm("sub.l  #0xC000, %a6");
    asm("move.b %d0, (%a6, %d6.l)");
    asm("rts");
    asm("AuxMapWriteBnk1:");
    asm("move.l AuxMemory@END.w(%a5), %a6");
    asm("sub.l  #0xD000, %a6");
    asm("move.b %d0, (%a6, %d6.l)");
    asm("move.b %d0, (%a2, %d6.l)");
    asm("rts");
    asm("AuxMapWriteBnk2:");
    asm("AuxMapWrite:");
    asm("move.l AuxMemory@END.w(%a5), %a6");
    asm("sub.l  #0xC000, %a6");
    asm("move.b %d0, (%a6, %d6.l)");
    asm("move.b %d0, (%a2, %d6.l)");
    asm("rts");
    asm("AuxReadBnk1:");
    asm("move.l AuxMemory@END.w(%a5), %a6");
    asm("sub.l  #0xD000, %a6");
    asm("clr.l  %d0");
    asm("move.b (%a6, %d6.l), %d0");
    asm("rts");
    asm("AuxReadBnk2:");
    asm("AuxRead:");
    asm("move.l AuxMemory@END.w(%a5), %a6");
    asm("sub.l  #0xC000, %a6");
    asm("clr.l  %d0");
    asm("move.b (%a6, %d6.l), %d0");
    asm("rts");
    asm("RomRead:");
    asm("move.l AppleROM@END.w(%a5), %a6");
    asm("sub.l  #0xC100, %a6");
    asm("clr.l  %d0");
    asm("move.b (%a6, %d6.l), %d0");
    asm("rts");
}
void WTXTM(UInt16, UInt8);
void WATXTM(UInt16, UInt8);
UInt8 RATXTM(UInt16 address);
void writeMemorySoftSwitch(UInt8 io)
{
    UInt16 page;
    UInt8  *tempPage;

    switch (io)
    {
        case IO_OFFSET(CLR80STORE):
            if (SS_ISSET(vidIOU, SS_80STORE))
            {
                clrVidFuncs(vidIOU);
                SS_RESET(vidIOU, SS_80STORE);
                if (SS_ISSET(vidIOU, SS_PAGE2))
                {
                    WriteFunction[0x08] = WTXTM;
                    WriteFunction[0x09] = WTXTM;
                    WriteFunction[0x0A] = WTXTM;
                    WriteFunction[0x0B] = WTXTM;
                }
                else
                {
                    WriteFunction[0x04] = WTXTM;
                    WriteFunction[0x05] = WTXTM;
                    WriteFunction[0x06] = WTXTM;
                    WriteFunction[0x07] = WTXTM;
                }
            }
            break;
        case IO_OFFSET(SET80STORE):
            if (!SS_ISSET(vidIOU, SS_80STORE))
            {
                clrVidFuncs(vidIOU);
                SS_SET(vidIOU, SS_80STORE);
                if (SS_ISSET(vidIOU, SS_PAGE2))
                {
                    if ((grMode > GRMODE_COLOR) && prefs.enable80Col)
                    {
                        WriteFunction[0x04] = WATXTM;
                        WriteFunction[0x05] = WATXTM;
                        WriteFunction[0x06] = WATXTM;
                        WriteFunction[0x07] = WATXTM;
                        ReadFunction[0x04]  = RATXTM;
                        ReadFunction[0x05]  = RATXTM;
                        ReadFunction[0x06]  = RATXTM;
                        ReadFunction[0x07]  = RATXTM;
                    }
                    else
                    {
                        WriteFunction[0x04] = READONLY;
                        WriteFunction[0x05] = READONLY;
                        WriteFunction[0x06] = READONLY;
                        WriteFunction[0x07] = READONLY;
                    }
                }
                else
                {
                    WriteFunction[0x04] = WTXTM;
                    WriteFunction[0x05] = WTXTM;
                    WriteFunction[0x06] = WTXTM;
                    WriteFunction[0x07] = WTXTM;
                }
            }
            break;
        case IO_OFFSET(RDMAINRAM):
            if (SS_ISSET(memIOU, SS_RAMRD))
            {
                SS_RESET(memIOU, SS_RAMRD);
                for (page = 0x02; page < 0xC0; page++)
                    ReadFunction[page] = NULL;
                setVideoFuncs(false);
            }
            break;
        case IO_OFFSET(RDCARDRAM):
            if (!SS_ISSET(memIOU, SS_RAMRD))
            {
                SS_SET(memIOU, SS_RAMRD);
                for (page = 0x02; page < 0xC0; page++)
                    ReadFunction[page] = RRMP;
            }
            break;
        case IO_OFFSET(WRMAINRAM):
            if (SS_ISSET(memIOU, SS_RAMWRT))
            {
                SS_RESET(memIOU, SS_RAMWRT);
                for (page = 0x02; page < 0xC0; page++)
                    WriteFunction[page] = NULL;
                setVideoFuncs(false);
            }
            break;
        case IO_OFFSET(WRCARDRAM):
            if (!SS_ISSET(memIOU, SS_RAMWRT))
            {
                SS_SET(memIOU, SS_RAMWRT);
                for (page = 0x02; page < 0xC0; page++)
                    WriteFunction[page] = READONLY;
            }
            break;
        case IO_OFFSET(SETSLOTCXROM):
            if (SS_ISSET(memIOU, SS_SLOTCXROM))
            {
                SS_RESET(memIOU, SS_SLOTCXROM);
                ReadFunction[0xC1] = RRMP;
                ReadFunction[0xC2] = RRMP;
                ReadFunction[0xC4] = RRMP;
                ReadFunction[0xC5] = RRMP;
                ReadFunction[0xC6] = RRMP;
                ReadFunction[0xC7] = RRMP;
                MemSet(AppleMemory + 0xC100, 512, 0);
                MemSet(AppleMemory + 0xC400, 1024, 0);
                ReadFunction[0xC6] = NULL;
                MemMove(AppleMemory + 0x00C600, BootROM, 256);
            }
            break;
        case IO_OFFSET(SETINTCXROM):
            if (!SS_ISSET(memIOU, SS_SLOTCXROM))
            {
                SS_SET(memIOU, SS_SLOTCXROM);
                ReadFunction[0xC1] = NULL;
                ReadFunction[0xC2] = NULL;
                ReadFunction[0xC4] = NULL;
                ReadFunction[0xC5] = NULL;
                ReadFunction[0xC6] = NULL;
                ReadFunction[0xC7] = NULL;
                MemMove(AppleMemory + 0xC100, AppleROM, 512);
                MemMove(AppleMemory + 0xC400, AppleROM + 0x300, 1024);
            }
            break;
        case IO_OFFSET(SETINTC3ROM):
            if (!SS_ISSET(memIOU, SS_SLOTC3ROM))
            {
                SS_SET(memIOU, SS_SLOTC3ROM);
                MemMove(AppleMemory + 0xC300, AppleROM + 0x200, 256);
            }
            break;
        case IO_OFFSET(SETSLOTC3ROM):
            if (SS_ISSET(memIOU, SS_SLOTC3ROM))
            {
                SS_RESET(memIOU, SS_SLOTC3ROM);
                MemSet(AppleMemory + 0xC300, 256, 0);
            }
            break;
        case IO_OFFSET(CLR80VID):
            SS_RESET(vidIOU, SS_80VID);
            setVideoFuncs(true);
            break;
        case IO_OFFSET(SET80VID):
            SS_SET(vidIOU, SS_80VID);
            setVideoFuncs(true);
            break;
    }
}
void updateLanguageCardMapping(void)
{
    READBYTE  rfn;
    WRITEBYTE wfn;
    UInt16 page;

    if (SS_ISSET(memIOU, SS_LCRAMMAP))
    {
        MemMove(AppleMemory + 0xD000, AuxMemory + (SS_ISSET(memIOU, SS_LCBNK2) ? 0x1000 : 0x0000), 4096);
        MemMove(AppleMemory + 0xE000, AuxMemory + 0x2000, 8192);
    }
    else
    {
        MemMove(AppleMemory + 0xD000, AppleROM + 0xD000 - 0xC100, 4096+8192);
    }
    if (SS_ISSET(memIOU, SS_LCRAMWRT))
    {
        if (SS_ISSET(memIOU, SS_LCRAMMAP))
        {
            wfn = SS_ISSET(memIOU, SS_LCBNK2) ? AuxMapWriteBnk2 : AuxMapWriteBnk1;
            WriteFunction[0xD0] = WriteFunction[0xD1] = wfn;
            WriteFunction[0xD2] = WriteFunction[0xD3] = wfn;
            WriteFunction[0xD4] = WriteFunction[0xD5] = wfn;
            WriteFunction[0xD6] = WriteFunction[0xD7] = wfn;
            WriteFunction[0xD8] = WriteFunction[0xD9] = wfn;
            WriteFunction[0xDA] = WriteFunction[0xDB] = wfn;
            WriteFunction[0xDC] = WriteFunction[0xDD] = wfn;
            WriteFunction[0xDE] = WriteFunction[0xDF] = wfn;
            for (page = 0xE0; page <= 0xFF; page++)
                WriteFunction[page] = AuxMapWrite;
        }
        else
        {
            wfn = SS_ISSET(memIOU, SS_LCBNK2) ? AuxWriteBnk2 : AuxWriteBnk1;
            WriteFunction[0xD0] = WriteFunction[0xD1] = wfn;
            WriteFunction[0xD2] = WriteFunction[0xD3] = wfn;
            WriteFunction[0xD4] = WriteFunction[0xD5] = wfn;
            WriteFunction[0xD6] = WriteFunction[0xD7] = wfn;
            WriteFunction[0xD8] = WriteFunction[0xD9] = wfn;
            WriteFunction[0xDA] = WriteFunction[0xDB] = wfn;
            WriteFunction[0xDC] = WriteFunction[0xDD] = wfn;
            WriteFunction[0xDE] = WriteFunction[0xDF] = wfn;
            for (page = 0xE0; page <= 0xFF; page++)
                WriteFunction[page] = AuxWrite;
        }
    }
    else
        for (page = 0xD0; page <= 0xFF; page++)
            WriteFunction[page] = READONLY;
    for (page = 0xD0; page <= 0xFF; page++)
        ReadFunction[page] = NULL;
    clrPcCheckFuncs();
    auxDirty = 0;
}
void toggleLanguageCard(UInt8 io)
{
    READBYTE  rfn;
    WRITEBYTE wfn;
    UInt16    page, dirty, pc6502;

    /* set IOU soft switches */
    dirty = memIOU;
    switch (io)
    {
        case 0x00:
        case 0x04:
            SS_SET(memIOU, SS_LCBNK2);
            SS_SET(memIOU, SS_LCRAMMAP);
            SS_RESET(memIOU, SS_LCRAMWRT);
            break;
        case 0x01:
        case 0x05:
            SS_SET(memIOU, SS_LCBNK2);
            SS_RESET(memIOU, SS_LCRAMMAP);
            SS_SET(memIOU, SS_LCRAMWRT);
            break;
        case 0x02:
        case 0x06:
            SS_SET(memIOU, SS_LCBNK2);
            SS_RESET(memIOU, SS_LCRAMMAP);
            SS_RESET(memIOU, SS_LCRAMWRT);
            break;
        case 0x03:
        case 0x07:
            SS_SET(memIOU, SS_LCBNK2);
            SS_SET(memIOU, SS_LCRAMMAP);
            SS_SET(memIOU, SS_LCRAMWRT);
            break;
        case 0x08:
        case 0x0C:
            SS_RESET(memIOU, SS_LCBNK2);
            SS_SET(memIOU, SS_LCRAMMAP);
            SS_RESET(memIOU, SS_LCRAMWRT);
            break;
        case 0x09:
        case 0x0D:
            SS_RESET(memIOU, SS_LCBNK2);
            SS_RESET(memIOU, SS_LCRAMMAP);
            SS_SET(memIOU, SS_LCRAMWRT);
            break;
        case 0x0A:
        case 0x0E:
            SS_RESET(memIOU, SS_LCBNK2);
            SS_RESET(memIOU, SS_LCRAMMAP);
            SS_RESET(memIOU, SS_LCRAMWRT);
            break;
        case 0x0B:
        case 0x0F:
            SS_RESET(memIOU, SS_LCBNK2);
            SS_SET(memIOU, SS_LCRAMMAP);
            SS_SET(memIOU, SS_LCRAMWRT);
            break;
    }
    /*
     * Update only the changed state.
     */
    if ((dirty ^= memIOU))
    {
        if (dirty & SS_LCRAMMAP)
        {
            updateLanguageCardMapping();
        }
        else
        {
            if (SS_ISSET(memIOU, SS_LCRAMMAP))
            {
                if (dirty & SS_LCBNK2)
                {
                    pc6502 = state6502.LongRegs.PC-state6502.LongRegs.MEMBASE;
                    if ((pc6502 >= 0xD000) && (pc6502 < 0xE000))
                    {
                        updateLanguageCardMapping();
                        return;
                    }
                    if (!auxDirty)
                    {
                        /*
                         * Install special versions of JMP, JSR, BRK, RTI, and RTS that check for memory ranges.
                         */
                        setPcCheckFuncs();
                        auxDirty = 1;
                    }
                    rfn = SS_ISSET(memIOU, SS_LCBNK2) ? AuxReadBnk2 : AuxReadBnk1;
                    ReadFunction[0xD0] = ReadFunction[0xD1] = rfn;
                    ReadFunction[0xD2] = ReadFunction[0xD3] = rfn;
                    ReadFunction[0xD4] = ReadFunction[0xD5] = rfn;
                    ReadFunction[0xD6] = ReadFunction[0xD7] = rfn;
                    ReadFunction[0xD8] = ReadFunction[0xD9] = rfn;
                    ReadFunction[0xDA] = ReadFunction[0xDB] = rfn;
                    ReadFunction[0xDC] = ReadFunction[0xDD] = rfn;
                    ReadFunction[0xDE] = ReadFunction[0xDF] = rfn;
                }
                if (SS_ISSET(memIOU, SS_LCRAMWRT))
                {
                    wfn = SS_ISSET(memIOU, SS_LCBNK2) ? AuxMapWriteBnk2 : AuxMapWriteBnk1;
                    WriteFunction[0xD0] = WriteFunction[0xD1] = wfn;
                    WriteFunction[0xD2] = WriteFunction[0xD3] = wfn;
                    WriteFunction[0xD4] = WriteFunction[0xD5] = wfn;
                    WriteFunction[0xD6] = WriteFunction[0xD7] = wfn;
                    WriteFunction[0xD8] = WriteFunction[0xD9] = wfn;
                    WriteFunction[0xDA] = WriteFunction[0xDB] = wfn;
                    WriteFunction[0xDC] = WriteFunction[0xDD] = wfn;
                    WriteFunction[0xDE] = WriteFunction[0xDF] = wfn;
                    if (dirty & SS_LCRAMWRT)
                        for (page = 0xE0; page <= 0xFF; page++)
                            WriteFunction[page] = AuxMapWrite;
                }
                else if (dirty & SS_LCRAMWRT)
                    for (page = 0xD0; page <= 0xFF; page++)
                        WriteFunction[page] = READONLY;
            }
            else
            {
                if (SS_ISSET(memIOU, SS_LCRAMWRT))
                {
                    wfn = SS_ISSET(memIOU, SS_LCBNK2) ? AuxWriteBnk2 : AuxWriteBnk1;
                    WriteFunction[0xD0] = WriteFunction[0xD1] = wfn;
                    WriteFunction[0xD2] = WriteFunction[0xD3] = wfn;
                    WriteFunction[0xD4] = WriteFunction[0xD5] = wfn;
                    WriteFunction[0xD6] = WriteFunction[0xD7] = wfn;
                    WriteFunction[0xD8] = WriteFunction[0xD9] = wfn;
                    WriteFunction[0xDA] = WriteFunction[0xDB] = wfn;
                    WriteFunction[0xDC] = WriteFunction[0xDD] = wfn;
                    WriteFunction[0xDE] = WriteFunction[0xDF] = wfn;
                    if (dirty & SS_LCRAMWRT)
                        for (page = 0xE0; page <= 0xFF; page++)
                            WriteFunction[page] = AuxWrite;
                }
                else if (dirty & SS_LCRAMWRT)
                    for (page = 0xD0; page <= 0xFF; page++)
                        WriteFunction[page] = READONLY;
            }
        }
    }
}
void setMemFuncs(void)
{
    UInt16 page;

    updateLanguageCardMapping();
    if (!SS_ISSET(memIOU, SS_SLOTCXROM))
    {
        ReadFunction[0xC1] = RRMP;
        ReadFunction[0xC2] = RRMP;
        ReadFunction[0xC4] = RRMP;
        ReadFunction[0xC5] = RRMP;
        ReadFunction[0xC6] = RRMP;
        ReadFunction[0xC7] = RRMP;
    }
    if (SS_ISSET(memIOU, SS_RAMRD))
      for (page = 0x02; page < 0xC0; page++)
          ReadFunction[page] = RRMP;
    if (SS_ISSET(memIOU, SS_RAMWRT))
      for (page = 0x02; page < 0xC0; page++)
          WriteFunction[page] = READONLY;
    ReadFunction[0xC0] = RIOM;
    ReadFunction[0xC3] = RC3M;
    WriteFunction[0xC0] = WIOM;
}

/*****************************************************************************\
*                                                                             *
*                              Disk access control                            *
*                                                                             *
\*****************************************************************************/

#define RAW_TRACK_BYTES     6192
#define RAW_SECTOR_BYTES    387
#define DOS_TRACK_BYTES     4096
#define RAW_TRACK_BITS (RAW_TRACK_BYTES*8)
#define NO_OF_PHASES            8
#define MAX_PHYSICAL_TRACK_NO   (40*NO_OF_PHASES)
#define MAX_DRIVE_NO            2

static Int8 stepper_movement_table[16][NO_OF_PHASES] =
{
    { 0,  0,  0,  0,  0,  0,  0,  0},                /* all electromagnets off  */
    { 0, -1, -2, -3,  0,  3,  2,  1},                /* EM 1 on                 */
    { 2,  1,  0, -1, -2, -3,  0,  3},                /* EM 2 on                 */
    { 1,  0, -1, -2, -3,  0,  3,  2},                /* EMs 1 & 2 on            */
    { 0,  3,  2,  1,  0, -1, -2, -3},                /* EM 3 on                 */
    { 0, -1,  0,  1,  0, -1,  0,  1},                /* EMs 1 & 3 on            */
    { 3,  2,  1,  0, -1, -2, -3,  0},                /* EMs 2 & 3 on            */
    { 2,  1,  0, -1, -2, -3,  0,  3},                /* EMs 1, 2 & 3 on         */
    {-2, -3,  0,  3,  2,  1,  0, -1},                /* EM 4 on                 */
    {-1, -2, -3,  0,  3,  2,  1,  0},                /* EMs 1 & 4 on            */
    { 0,  1,  0, -1,  0,  1,  0, -1},                /* EMs 2 & 4               */
    { 0, -1, -2, -3,  0,  3,  2,  1},                /* EMs 1, 2 & 4 on         */
    {-3,  0,  3,  2,  1,  0, -1, -2},                /* EMs 3 & 4 on            */
    {-2, -3,  0,  3,  2,  1,  0, -1},                /* EMs 1, 3 & 4 on         */
    { 0,  3,  2,  1,  0, -1, -2, -3},                /* EMs 2, 3 & 4 on         */
    { 0,  0,  0,  0,  0,  0,  0,  0}
};                                                    /* all electromagnets on */

static Boolean   track_buffer_valid;
static Int8      write_mode;
static UInt8    *nibble_buf;
static UInt16    track_buffer_pos;
static UInt16    track_buffer_size;
static UInt8     write_protect;
static UInt8     address_latch, data_latch;
static Boolean   motor_state;
static UInt8     drive_no = 0;
struct stateDisk_t
{
    char      diskname[32];
    LocalID   diskdb;
    DmOpenRef diskimage;
    MemHandle track_buffer_handle;
    Int16     physical_track_no;
    Int16     stepper_status;
    Boolean   track_buffer_valid;
    Boolean   track_buffer_raw;
    Boolean   track_buffer_dirty;
    Int8      track_count;
    Int8      track_buffer_track;
    Int8      write_mode;
    Int16     last_track;
    UInt8    *nibble_buf;
    UInt16    track_buffer_pos;
    UInt16    track_buffer_size;
    UInt8     write_protect;
    UInt8     logical_track;
    UInt8     address_latch, data_latch;
} stateDisk[2] = {0};
UInt32 DskIoICountDiff = 0;
UInt32 LastDskIoICount = 0;

/*
 * SLOT 6 IO read.
 */
RDMEM_CWRAP(SLT6, address)
{
    return (BootROM[address & 0xFF]);
}
void setCurrentDisk(int drive)
{
    if (drive != drive_no)
    {
        if (drive_no)
            showDriveState(drive_no, false);
        drive_no = drive;
        showDiskName(drive_no, stateDisk[drive_no - 1].diskname);
        showDriveState(drive_no, motor_state);
    }
    drive--;
    track_buffer_valid = stateDisk[drive].track_buffer_valid;
    write_mode         = stateDisk[drive].write_mode;
    nibble_buf         = stateDisk[drive].nibble_buf;
    track_buffer_pos   = stateDisk[drive].track_buffer_pos;
    track_buffer_size  = stateDisk[drive].track_buffer_size;
    write_protect      = stateDisk[drive].write_protect;
    address_latch      = stateDisk[drive].address_latch;
    data_latch         = stateDisk[drive].data_latch;
}
int getCurrentDrive(void)
{
    return (drive_no | (motor_state ? 0x80 : 0));
}
void updateCurrentDisk(int drive)
{
    if (drive--)
    {
        stateDisk[drive].track_buffer_pos = track_buffer_pos;
        stateDisk[drive].address_latch    = address_latch;
        stateDisk[drive].data_latch       = data_latch;
    }
}
void load_track_buffer(int drive)
{
    UInt8 *values;

    if (stateDisk[--drive].diskdb)
    {
        stateDisk[drive].logical_track = (stateDisk[drive].physical_track_no + 1) >> 2;
        if (stateDisk[drive].logical_track >= stateDisk[drive].track_count)
            stateDisk[drive].logical_track =  stateDisk[drive].track_count - 1;
        if ((stateDisk[drive].logical_track != stateDisk[drive].last_track) || !stateDisk[drive].track_buffer_valid)
        {
            if (stateDisk[drive].track_buffer_dirty && !stateDisk[drive].track_buffer_raw)
            {
                stateDisk[drive].track_buffer_handle = DmGetRecord(stateDisk[drive].diskimage, stateDisk[drive].last_track);
                values = (UInt8 *) MemHandleLock(stateDisk[drive].track_buffer_handle);
                NibblesToSectors(stateDisk[drive].nibble_buf, values, 254, stateDisk[drive].last_track);
                MemHandleUnlock(stateDisk[drive].track_buffer_handle);
                DmReleaseRecord(stateDisk[drive].diskimage, stateDisk[drive].last_track, true);
            }
            else if (stateDisk[drive].track_buffer_handle)
                MemHandleUnlock(stateDisk[drive].track_buffer_handle);
            stateDisk[drive].track_buffer_handle = DmQueryRecord(stateDisk[drive].diskimage, stateDisk[drive].logical_track);
            values = (UInt8 *) MemHandleLock(stateDisk[drive].track_buffer_handle);
            if (stateDisk[drive].track_buffer_raw)
                stateDisk[drive].nibble_buf = values;
            else
            {
                SectorsToNibbles(values, stateDisk[drive].nibble_buf, 254, stateDisk[drive].logical_track);
                MemHandleUnlock(stateDisk[drive].track_buffer_handle);
                stateDisk[drive].track_buffer_handle = 0;
            }
            stateDisk[drive].last_track         = stateDisk[drive].logical_track;
            stateDisk[drive].track_buffer_track = stateDisk[drive].physical_track_no;
            stateDisk[drive].track_buffer_dirty = false;
            stateDisk[drive].track_buffer_valid = true;
            stateDisk[drive].track_buffer_pos   = 0;
            if (drive_no == (drive + 1))
                setCurrentDisk(drive_no);
        }
    }
}
void write_nibble(UInt8 data)
{
    if (!track_buffer_valid)
        load_track_buffer(drive_no);
    else if (DskIoICountDiff > 100)
    {
        if (DskIoICountDiff < 65536)
        {
            /*
             * Advance position based on instruction count.
             */
            track_buffer_pos += (DskIoICountDiff >> 12) * (track_buffer_size >> 4); /* divide by ~4096 instructions per sector */
            if (track_buffer_pos >= track_buffer_size)
                track_buffer_pos -= track_buffer_size;
        }
        if (track_buffer_size == RAW_TRACK_BYTES)
        {
            /*
             * Round position up to a sector boundary.
             */
             if (track_buffer_pos > RAW_SECTOR_BYTES*15)
                 track_buffer_pos = 0;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*14)
                 track_buffer_pos = RAW_SECTOR_BYTES*15;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*13)
                 track_buffer_pos = RAW_SECTOR_BYTES*14;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*12)
                 track_buffer_pos = RAW_SECTOR_BYTES*13;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*11)
                 track_buffer_pos = RAW_SECTOR_BYTES*12;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*10)
                 track_buffer_pos = RAW_SECTOR_BYTES*11;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*9)
                 track_buffer_pos = RAW_SECTOR_BYTES*10;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*8)
                 track_buffer_pos = RAW_SECTOR_BYTES*9;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*7)
                 track_buffer_pos = RAW_SECTOR_BYTES*8;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*6)
                 track_buffer_pos = RAW_SECTOR_BYTES*7;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*5)
                 track_buffer_pos = RAW_SECTOR_BYTES*6;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*4)
                 track_buffer_pos = RAW_SECTOR_BYTES*5;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*3)
                 track_buffer_pos = RAW_SECTOR_BYTES*4;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*2)
                 track_buffer_pos = RAW_SECTOR_BYTES*3;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*1)
                 track_buffer_pos = RAW_SECTOR_BYTES*2;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*0)
                 track_buffer_pos = RAW_SECTOR_BYTES*1;
        }
    }
    nibble_buf[track_buffer_pos] = data;
    if (++track_buffer_pos >= track_buffer_size)
        track_buffer_pos = 0;
    stateDisk[drive_no - 1].track_buffer_dirty = true;
}
UInt8 read_nibble()
{
    UInt8  data;

    if (!track_buffer_valid)
        load_track_buffer(drive_no);
    else if (DskIoICountDiff > 100)
    {
        if (DskIoICountDiff < 65536)
        {
            /*
             * Advance position based on instruction count.
             */
            track_buffer_pos += (DskIoICountDiff >> 12) * (track_buffer_size >> 4); /* divide by ~4096 instructions per sector */
            if (track_buffer_pos >= track_buffer_size)
                track_buffer_pos -= track_buffer_size;
        }
        if (track_buffer_size == RAW_TRACK_BYTES)
        {
            /*
             * Round position up to a sector boundary.
             */
             if (track_buffer_pos > RAW_SECTOR_BYTES*15)
                 track_buffer_pos = 0;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*14)
                 track_buffer_pos = RAW_SECTOR_BYTES*15;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*13)
                 track_buffer_pos = RAW_SECTOR_BYTES*14;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*12)
                 track_buffer_pos = RAW_SECTOR_BYTES*13;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*11)
                 track_buffer_pos = RAW_SECTOR_BYTES*12;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*10)
                 track_buffer_pos = RAW_SECTOR_BYTES*11;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*9)
                 track_buffer_pos = RAW_SECTOR_BYTES*10;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*8)
                 track_buffer_pos = RAW_SECTOR_BYTES*9;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*7)
                 track_buffer_pos = RAW_SECTOR_BYTES*8;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*6)
                 track_buffer_pos = RAW_SECTOR_BYTES*7;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*5)
                 track_buffer_pos = RAW_SECTOR_BYTES*6;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*4)
                 track_buffer_pos = RAW_SECTOR_BYTES*5;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*3)
                 track_buffer_pos = RAW_SECTOR_BYTES*4;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*2)
                 track_buffer_pos = RAW_SECTOR_BYTES*3;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*1)
                 track_buffer_pos = RAW_SECTOR_BYTES*2;
             else if (track_buffer_pos > RAW_SECTOR_BYTES*0)
                 track_buffer_pos = RAW_SECTOR_BYTES*1;
        }
    }
    data = nibble_buf[track_buffer_pos];
    if (++track_buffer_pos >= track_buffer_size)
        track_buffer_pos = 0;
    return (data);
}
void toggleStepper(int drive, UInt8 magnet, UInt8 on)
{
    if (drive--)
    {
        if (on)
            stateDisk[drive].stepper_status |= (1 << magnet);
        else
            stateDisk[drive].stepper_status &= ~(1 << magnet);
        if (motor_state)
        {
            stateDisk[drive].physical_track_no += stepper_movement_table[stateDisk[drive].stepper_status][stateDisk[drive].physical_track_no & 0x07];
            if (stateDisk[drive].physical_track_no < 0)
                stateDisk[drive].physical_track_no = 0;
            else if (stateDisk[drive].physical_track_no >= MAX_PHYSICAL_TRACK_NO)
                stateDisk[drive].physical_track_no = MAX_PHYSICAL_TRACK_NO - 1;
            stateDisk[drive].track_buffer_valid = track_buffer_valid = false;
        }
    }
}
void toggleMotor(int drive, Boolean on_off)
{
    if (drive--)
    {
        if (motor_state != on_off)
        {
            motor_state = on_off;
            showDriveState(drive_no, motor_state);
        }
    }
}
void positionDisk(int drive, UInt16 new_track, UInt16 new_position)
{
    drive--;
    stateDisk[drive].physical_track_no = new_track << 2;
    if (stateDisk[drive].physical_track_no >= MAX_PHYSICAL_TRACK_NO)
        stateDisk[drive].physical_track_no = MAX_PHYSICAL_TRACK_NO - 1;
    load_track_buffer(drive + 1);
    stateDisk[drive].track_buffer_pos = new_position;
    if (stateDisk[drive].track_buffer_pos >= stateDisk[drive].track_buffer_size)
        stateDisk[drive].track_buffer_pos = 0;
    if (drive_no == (drive + 1))
        setCurrentDisk(drive_no);
}
void queryDisk(int drive, UInt16 *track, UInt16 *pos)
{
    if (drive-- == drive_no)
        updateCurrentDisk(drive_no);
    *track = stateDisk[drive].physical_track_no >> 2;
    *pos   = stateDisk[drive].track_buffer_pos;
}
int mountDisk(int drive, char *file, Boolean writeable)
{
    UInt16 attrs;
    UInt32 type, numrecs, ttlbytes, recbytes;
    umountDisk(drive--);
    stateDisk[drive].track_buffer_pos   = 0;
    stateDisk[drive].track_buffer_dirty = false;
    stateDisk[drive].diskdb             = file ? DmFindDatabase(0, file) : 0;
    if (stateDisk[drive].diskdb)
    {
        StrCopy(stateDisk[drive].diskname, file);
        stateDisk[drive].diskimage = DmOpenDatabase(0, stateDisk[drive].diskdb, 0);
        DmDatabaseInfo(0, stateDisk[drive].diskdb, NULL, &attrs, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &type, NULL);
        stateDisk[drive].track_buffer_raw = (type == (UInt32)'RDSK') ? true : false;
        stateDisk[drive].write_protect = attrs & dmHdrAttrReadOnly ? true : !writeable;
        DmDatabaseSize(0, stateDisk[drive].diskdb, &numrecs, &ttlbytes, &recbytes);
        stateDisk[drive].track_count = numrecs;
        if (stateDisk[drive].track_buffer_raw)
        {
            stateDisk[drive].track_buffer_size = recbytes /  stateDisk[drive].track_count;
        }
        else
        {
            stateDisk[drive].nibble_buf        = MemPtrNew(RAW_TRACK_BYTES);
            stateDisk[drive].track_buffer_size = RAW_TRACK_BYTES;
            init_GCR_table();
        }
        stateDisk[drive].track_buffer_handle = 0;
        stateDisk[drive].track_buffer_valid = false;
    }
    else
    {
        stateDisk[drive].diskname[0] = '\0';
        stateDisk[drive].diskimage   = 0;
        stateDisk[drive].nibble_buf  = MemPtrNew(RAW_TRACK_BYTES);
        MemSet(stateDisk[drive].nibble_buf, RAW_TRACK_BYTES, 0xFF);
        stateDisk[drive].track_buffer_handle = 0;
        stateDisk[drive].track_buffer_size   = RAW_TRACK_BYTES;
        stateDisk[drive].track_buffer_valid  = true;
        stateDisk[drive].write_protect       = true;
    }
    if (drive_no == (drive + 1))
    {
        drive_no = 0;
        setCurrentDisk(drive + 1);
    }
    else
    {
        showDiskName(drive + 1, stateDisk[drive].diskname);
        showDriveState(drive + 1, false);
    }
    return (stateDisk[drive].diskdb != 0);
}
void umountDisk(int drive)
{
    UInt8 *values;

    if (drive-- == drive_no)
        updateCurrentDisk(drive_no);
    if (stateDisk[drive].diskdb)
    {
        if (!stateDisk[drive].track_buffer_raw)
        {
            if (stateDisk[drive].track_buffer_dirty)
            {
                stateDisk[drive].track_buffer_handle = DmGetRecord(stateDisk[drive].diskimage, stateDisk[drive].last_track);
                values = (UInt8 *) MemHandleLock(stateDisk[drive].track_buffer_handle);
                NibblesToSectors(stateDisk[drive].nibble_buf, values, 254, stateDisk[drive].last_track);
                MemHandleUnlock(stateDisk[drive].track_buffer_handle);
                DmReleaseRecord(stateDisk[drive].diskimage, stateDisk[drive].last_track, true);
            }
            MemPtrFree(stateDisk[drive].nibble_buf);
        }
        else if (stateDisk[drive].track_buffer_handle)
        {
            MemHandleUnlock(stateDisk[drive].track_buffer_handle);
        }
        DmCloseDatabase(stateDisk[drive].diskimage);
        stateDisk[drive].diskdb = 0;
    }
    else if (stateDisk[drive].nibble_buf)
        MemPtrFree(stateDisk[drive].nibble_buf);
    stateDisk[drive].nibble_buf = NULL;
    stateDisk[drive].track_buffer_handle = 0;
    stateDisk[drive].track_buffer_valid  = false;
    toggleMotor(drive + 1, 0);
}
void resetDisks(void)
{
#if 0
    UInt8 drive, *values;

    updateCurrentDisk(drive_no);
    for (drive = 0; drive < 2; drive++)
    {
        if (stateDisk[drive].diskdb)
        {
            if (!stateDisk[drive].track_buffer_raw)
            {
                if (stateDisk[drive].track_buffer_dirty)
                {
                    stateDisk[drive].track_buffer_handle = DmGetRecord(stateDisk[drive].diskimage, stateDisk[drive].last_track);
                    values = (UInt8 *) MemHandleLock(stateDisk[drive].track_buffer_handle);
                    NibblesToSectors(stateDisk[drive].nibble_buf, values, 254, stateDisk[drive].last_track);
                    MemHandleUnlock(stateDisk[drive].track_buffer_handle);
                    DmReleaseRecord(stateDisk[drive].diskimage, stateDisk[drive].last_track, true);
                }
            }
            else if (stateDisk[drive].track_buffer_handle)
            {
                MemHandleUnlock(stateDisk[drive].track_buffer_handle);
            }
        }
        toggleMotor(drive + 1, 0);
        stateDisk[drive].physical_track_no   = 0;
        stateDisk[drive].track_buffer_handle = 0;
        stateDisk[drive].track_buffer_pos    = 0;
        stateDisk[drive].track_buffer_valid  = false;
        stateDisk[drive].track_buffer_dirty  = false;
    }
#else
    umountDisk(1);
    umountDisk(2);
    mountDisk(1, stateDisk[0].diskname, stateDisk[0].write_protect);
    mountDisk(2, stateDisk[1].diskname, stateDisk[1].write_protect);
#endif
    drive_no = 0;
    setCurrentDisk(1);
}

/*****************************************************************************\
*                                                                             *
*                               I/O page access                               *
*                                                                             *
\*****************************************************************************/

/*
 * I/O Read.
 */
UInt8 RIOM(UInt16);
UInt8 cwrap_RIOM(UInt16);
UInt8 awrap_RIOM(UInt16 a)
{
    asm(".global RIOM");
    asm("RIOM:");
    /*
     * Quick check for Disk read byte.
     */
    asm("cmp.b   #0xEC, %d6");
    asm("bne.b   L1");
    /*
     * Check for buffer valid.
     */
    asm("tst.b   track_buffer_valid@END.w(%a5)");
    asm("beq.b   L1");
    /*
     * Update access time.
     */
    asm("move.l  AppleInstrInc@END.w(%a5), %d0");
    asm("sub.l   %d1, %d0");
    asm("add.l   AppleInstrCount@END.w(%a5), %d0");
    asm("move.l  %d0, %a6");
    asm("sub.l   LastDskIoICount@END.w(%a5), %d0");
    asm("move.l  %d0, DskIoICountDiff@END.w(%a5)");
    asm("move.l  %a6, LastDskIoICount@END.w(%a5)");
    /*
     * Check for back-to-back read.
     */
    asm("cmp.l   #100, %d0"); // This is the instruction count that determines if it is back-to-back
    asm("bgt.b   L1");
    /*
     * Check for read or write.
     */
    asm("tst.b  write_mode@END.w(%a5)");
    asm("bne.b  L1");
    /*
     * Read next byte in buffer and advance position.
     */
    asm("move.w  track_buffer_pos@END.w(%a5), %a6");
    asm("move.l  nibble_buf@END.w(%a5), %d0");
    asm("move.b  (%a6,%d0.l), %d0");
    asm("exg     %a6, %d0");
    asm("addq.w  #1, %d0");
    asm("cmp.w   track_buffer_size@END.w(%a5), %d0");
    asm("blt.b   L2");
    asm("clr.w   %d0");
    asm("L2:");
    asm("move.w  %d0, track_buffer_pos@END.w(%a5)");
    asm("exg     %a6, %d0");
    asm("and.l  #0xFF, %d0");
    asm("move.b  #0x0C, address_latch@END.w(%a5)"); // Update address latch
    asm("rts");
    /*
     * Call C-wrapped function.
     */
    asm("L1:");
    asm("movem.l #0x1FFE, (state6502+4)@END.w(%a5)");
    asm("move.w  %d6, -(%sp)");
    asm("bsr.w   cwrap_RIOM");
    asm("addq.l  #2, %sp");
    asm("and.l   #0xFF,%d0");
    asm("movem.l (state6502+4)@END.w(%a5), #0x1FFE");
    asm("rts");
}
UInt8 cwrap_RIOM(UInt16 address)
{
    UInt8 data = 0;

    switch ((UInt8)address)
    {
        case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
        case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D: case 0x0E: case 0x0F:
            data = kbdIOU;
            break;
        case 0x10:
            data = kbdIOU & 0x7F;
            if (kbdCount)
            {
                kbdHead = (kbdHead + 1) & KBD_BUF_MASK;
                if (--kbdCount)
                    kbdIOU = kbdBuffer[kbdHead];
                else
                    kbdIOU &= 0x7F;
            }
            else
                kbdIOU &= 0x7F;
            break;
        case 0x11:
            if (SS_ISSET(memIOU, SS_LCBNK2))
                data = 0x80;
            break;
        case 0x12:
            data = 0x80;
            break;
        case 0x13:
            if (SS_ISSET(memIOU, SS_LCRAMMAP))
                data = 0x80;
            break;
        case 0x14:
            if (SS_ISSET(memIOU, SS_LCRAMWRT))
                data = 0x80;
            break;
        case 0x15:
            if (SS_ISSET(memIOU, SS_SLOTCXROM))
                data = 0x80;
            break;
        case 0x16:
            break;
        case 0x17:
            if (SS_ISSET(memIOU, SS_SLOTC3ROM))
                data = 0x80;
            break;
        case 0x18:
            if (SS_ISSET(vidIOU, SS_80STORE))
                data = 0x80;
            break;
        case 0x19:
            if (SS_ISSET(vidIOU, SS_VBLBAR))
                data = 0x80;
            break;
        case 0x1A:
            if (SS_ISSET(vidIOU, SS_TEXT))
                data = 0x80;
            break;
        case 0x1B:
            if (SS_ISSET(vidIOU, SS_MIXED))
                data = 0x80;
            break;
        case 0x1C:
            if (SS_ISSET(vidIOU, SS_PAGE2))
                data = 0x80;
            break;
        case 0x1D:
            if (SS_ISSET(vidIOU, SS_HIRES))
                data = 0x80;
            break;
        case 0x1E:
            break;
        case 0x1F:
          if (SS_ISSET(vidIOU, SS_80VID))
                data = 0x80;
            break;
        case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
        case 0x28: case 0x29: case 0x2A: case 0x2B: case 0x2C: case 0x2D: case 0x2E: case 0x2F:
            break;
        case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
        case 0x38: case 0x39: case 0x3A: case 0x3B: case 0x3C: case 0x3D: case 0x3E: case 0x3F:
            spkrIOU++;
            break;
        case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
        case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4E: case 0x4F:
            break;
        case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
        case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5E: case 0x5F:
            toggleVideoSoftSwitch(IO_OFFSET(address));
            break;
        case 0x60:
            break;
        case 0x61:
            if (btnIOU[0])
                data = 0x80;
            break;
        case 0x62:
            if (btnIOU[1])
                data = 0x80;
            break;
        case 0x63:
            if (btnIOU[2])
                data = 0x80;
            break;
        case 0x64:
            if (pdlIOU[0])
            {
                pdlIOU[0]--;
                data = 0x80;
            }
            break;
        case 0x65:
            if (pdlIOU[1])
            {
                pdlIOU[1]--;
                data = 0x80;
            }
            break;
        case 0x66: case 0x67: case 0x68: case 0x69: case 0x6A: case 0x6B:
            break;
        case 0x6C:
            if (pdlIOU[0])
            {
                pdlIOU[0]--;
                data = 0x80;
            }
            break;
        case 0x6D:
            if (pdlIOU[1])
            {
                pdlIOU[1]--;
                data = 0x80;
            }
            break;
        case 0x6E:
        case 0x6F:
            break;
        case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
        case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F:
            pdlIOU[0] = pdlResetIOU[0];
            pdlIOU[1] = pdlResetIOU[1];
            break;
        case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
        case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8E: case 0x8F:
            toggleLanguageCard(IO_OFFSET(address));
            break;
        case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
        case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9E: case 0x9F:
            break;
        case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: case 0xA6: case 0xA7:
        case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAE: case 0xAF:
            break;
        case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB6: case 0xB7:
        case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBE: case 0xBF:
            break;
        case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC4: case 0xC5: case 0xC6: case 0xC7:
        case 0xC8: case 0xC9: case 0xCA: case 0xCB: case 0xCC: case 0xCD: case 0xCE: case 0xCF:
            break;
        case 0xD0: case 0xD1: case 0xD2: case 0xD3: case 0xD4: case 0xD5: case 0xD6: case 0xD7:
        case 0xD8: case 0xD9: case 0xDA: case 0xDB: case 0xDC: case 0xDD: case 0xDE: case 0xDF:
            break;
        case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xE4: case 0xE5: case 0xE6: case 0xE7:
            toggleStepper(drive_no, (address >> 1) & 0x07, address & 1);
            break;
        case 0xE8:
            toggleMotor(drive_no, 0);
            break;
        case 0xE9:
            toggleMotor(drive_no, 1);
            break;
        case 0xEA:
            if (drive_no != 1)
            {
                updateCurrentDisk(drive_no);
                setCurrentDisk(1);
            }
            break;
        case 0xEB:
            if (drive_no != 2)
            {
                updateCurrentDisk(drive_no);
                setCurrentDisk(2);
            }
            break;
        case 0xEC:
            if (write_mode)
                write_nibble(data_latch);
            else
                data = read_nibble();
            address_latch = 0x0C;
            break;
        case 0xED:
            address_latch = 0x0D;
            break;
        case 0xEE:
            if (address_latch == 0x0D)
                /* check write protect */
                data = write_protect ? 0xFF: 0x00;
            address_latch = 0x0E;
            write_mode    = 0;
            break;
        case 0xEF:
            address_latch = 0x0F;
            write_mode    = 1;
            break;
        case 0xF0:
        case 0xF1:
        case 0xF2:
        case 0xF3:
        case 0xF4:
        case 0xF5:
        case 0xF6:
        case 0xF7:
        case 0xF8:
        case 0xF9:
        case 0xFA:
        case 0xFB:
        case 0xFC:
        case 0xFD:
        case 0xFE:
        case 0xFF:
            break;
    }
    return (data);
}
/*
 * I/O Write.
 */
void WIOM(UInt16, UInt8);
void cwrap_WIOM(UInt16, UInt8);
void awrap_wIOM(UInt16 a, UInt8 d)
{
    asm(".global WIOM");
    asm("WIOM:");
    /*
     * Quick check for Disk read byte.
     */
    asm("cmp.b   #0xEC, %d6");
    asm("bne.b   L3");
    /*
     * Update IO access time.
     */
    asm("move.l  %d0, %a6");
    asm("move.l  AppleInstrInc@END.w(%a5), %d0");
    asm("sub.l   %d1, %d0");
    asm("add.l   AppleInstrCount@END.w(%a5), %d0");
    asm("sub.l   LastDskIoICount@END.w(%a5), %d0");
    asm("move.l  %d0, DskIoICountDiff@END.w(%a5)");
    asm("add.l   %d0, LastDskIoICount@END.w(%a5)");
    asm("move.l  %a6, %d0");
    /*
     * Call C-wrapped function.
     */
    asm("L3:");
    asm("movem.l #0x1FFF, state6502@END.w(%a5)");
    asm("move.b  %d0, -(%sp)");
    asm("move.w  %d6, -(%sp)");
    asm("bsr.w   cwrap_WIOM");
    asm("addq.l  #4, %sp");
    asm("movem.l state6502@END.w(%a5), #0x1FFF");
    asm("rts");
}
void cwrap_WIOM(UInt16 address, UInt8 data)
{
    AppleMemory[address] = data;
    switch ((UInt8)address)
    {
        case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
        case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D: case 0x0E: case 0x0F:
            writeMemorySoftSwitch(IO_OFFSET(address));
            break;
        case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
        case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E: case 0x1F:
            if (kbdCount)
            {
                kbdHead = (kbdHead + 1) & KBD_BUF_MASK;
                if (--kbdCount)
                    kbdIOU = kbdBuffer[kbdHead];
                else
                    kbdIOU &= 0x7F;
            }
            else
                kbdIOU &= 0x7F;
            break;
        case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
        case 0x28: case 0x29: case 0x2A: case 0x2B: case 0x2C: case 0x2D: case 0x2E: case 0x2F:
            break;
        case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
        case 0x38: case 0x39: case 0x3A: case 0x3B: case 0x3C: case 0x3D: case 0x3E: case 0x3F:
            spkrIOU++;
            break;
        case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
        case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4E: case 0x4F:
            break;
        case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
        case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5E: case 0x5F:
            toggleVideoSoftSwitch(IO_OFFSET(address));
            break;
        case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
        case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6E: case 0x6F:
            break;
        case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
        case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F:
            pdlIOU[0] = pdlResetIOU[0];
            pdlIOU[1] = pdlResetIOU[1];
            break;
        case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
        case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8E: case 0x8F:
            toggleLanguageCard(IO_OFFSET(address));
            break;
        case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
        case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9E: case 0x9F:
            break;
        case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: case 0xA6: case 0xA7:
        case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAE: case 0xAF:
            break;
        case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB6: case 0xB7:
        case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBE: case 0xBF:
            break;
        case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC4: case 0xC5: case 0xC6: case 0xC7:
        case 0xC8: case 0xC9: case 0xCA: case 0xCB: case 0xCC: case 0xCD: case 0xCE: case 0xCF:
            break;
        case 0xD0: case 0xD1: case 0xD2: case 0xD3: case 0xD4: case 0xD5: case 0xD6: case 0xD7:
        case 0xD8: case 0xD9: case 0xDA: case 0xDB: case 0xDC: case 0xDD: case 0xDE: case 0xDF:
            break;
        case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xE4: case 0xE5: case 0xE6: case 0xE7:
            toggleStepper(drive_no, (address >> 1) & 0x07, address & 1);
            break;
        case 0xE8:
            toggleMotor(drive_no, 0);
            break;
        case 0xE9:
            toggleMotor(drive_no, 1);
            break;
        case 0xEA:
            if (drive_no != 1)
            {
                updateCurrentDisk(drive_no);
                setCurrentDisk(1);
            }
            break;
        case 0xEB:
            if (drive_no != 2)
            {
                updateCurrentDisk(drive_no);
                setCurrentDisk(2);
            }
            break;
        case 0xEC:
            if (write_mode)
                write_nibble(data_latch);
            else
                read_nibble();
            address_latch = 0x0C;
            break;
        case 0xED:
            data_latch     = data;
            address_latch = 0x0D;
            break;
        case 0xEE:
            address_latch = 0x0E;
            write_mode    = 0;
            break;
        case 0xEF:
            data_latch    = data;
            address_latch = 0x0F;
            write_mode    = 1;
            break;
        case 0xF0: case 0xF1: case 0xF2: case 0xF3: case 0xF4: case 0xF5: case 0xF6: case 0xF7:
        case 0xF8: case 0xF9: case 0xFA: case 0xFB: case 0xFC: case 0xFD: case 0xFE: case 0xFF:
            break;
    }
}

/*****************************************************************************\
*                                                                             *
*                           Sector <=> Nibble Encoding                        *
*                                                                             *
\*****************************************************************************/

static UInt8 GCR_encoding_table[64] = {
    0x96, 0x97, 0x9A, 0x9B, 0x9D, 0x9E, 0x9F, 0xA6,
    0xA7, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB2, 0xB3,
    0xB4, 0xB5, 0xB6, 0xB7, 0xB9, 0xBA, 0xBB, 0xBC,
    0xBD, 0xBE, 0xBF, 0xCB, 0xCD, 0xCE, 0xCF, 0xD3,
    0xD6, 0xD7, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE,
    0xDF, 0xE5, 0xE6, 0xE7, 0xE9, 0xEA, 0xEB, 0xEC,
    0xED, 0xEE, 0xEF, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6,
    0xF7, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF };

static UInt8    GCR_decoding_table[256];
static UInt8    Swap_Bit[4] = { 0, 2, 1, 3 }; /* swap lower 2 bits */
static UInt8    GCR_buffer[256];
static UInt8    GCR_buffer2[86];

static UInt16   Position=0;
static UInt8    *Track_Nibble;

/* physical sector no. to DOS 3.3 logical sector no. table */
static UInt8    Logical_Sector[16] = {
    0x0, 0x7, 0xE, 0x6, 0xD, 0x5, 0xC, 0x4,
    0xB, 0x3, 0xA, 0x2, 0x9, 0x1, 0x8, 0xF };

static UInt8    Physical_Sector[16];


#define FM_ENCODE(x)    gcrWriteNibble( ((x) >> 1) | 0xAA );\
            gcrWriteNibble( (x) | 0xAA )

static void init_GCR_table(void)
{
    static UInt8 initialized = 0;
    int     i;

    if ( !initialized ) {
       for( i = 0; i < 64; i++ )
          GCR_decoding_table[GCR_encoding_table[i]] = i;
       for( i = 0; i < 16; i++ )
          Physical_Sector[Logical_Sector[i]] = i;
       initialized = 1;
    }
}

static UInt8 gcr_read_nibble(void)
{
    UInt8   data;

    data = Track_Nibble[Position++];
    if ( Position >= RAW_TRACK_BYTES )
       Position = 0;
    return data;
}

static void gcr_write_nibble( UInt8 data )
{
    Track_Nibble[Position++] = data;
    if ( Position >= RAW_TRACK_BYTES )
       Position = 0;
}

static void decode62( UInt8 *page )
{
    int i, j;

    /* get 6 bits from GCR_buffer & 2 from GCR_buffer2 */
    for( i = 0, j = 86; i < 256; i++ ) {
      if ( --j < 0 ) j = 85;
      page[i] = (GCR_buffer[i] << 2) | Swap_Bit[GCR_buffer2[j] & 0x03];
      GCR_buffer2[j] >>= 2;
    }
}

static void encode62( UInt8 *page )
{
    int i, j;

    /* 86 * 3 = 258, so the first two byte are encoded twice */
    GCR_buffer2[0] = Swap_Bit[page[1]&0x03];
    GCR_buffer2[1] = Swap_Bit[page[0]&0x03];

    /* save higher 6 bits in GCR_buffer and lower 2 bits in GCR_buffer2 */
    for( i = 255, j = 2; i >= 0; i--, j = j == 85? 0: j + 1 ) {
       GCR_buffer2[j] = (GCR_buffer2[j] << 2) | Swap_Bit[page[i]&0x03];
       GCR_buffer[i] = page[i] >> 2;
    }

    /* clear off higher 2 bits of GCR_buffer2 set in the last call */
    for( i = 0; i < 86; i++ )
       GCR_buffer2[i] &= 0x3f;
}

/*
 * write an FM encoded value, used in writing address fields
 */
static void FM_encode( UInt8 data )
{
    gcr_write_nibble( (data >> 1) | 0xAA );
    gcr_write_nibble( data | 0xAA );
}

/*
 * return an FM encoded value, used in reading address fields
 */
static UInt8 FM_decode(void)
{
    int     tmp;

    /* C does not specify order of operand evaluation, don't
     * merge the following two expression into one
     */
    tmp = (gcr_read_nibble() << 1) | 0x01;
    return gcr_read_nibble() & tmp;
}

static void write_sync( int length )
{
    while( length-- )
       gcr_write_nibble( 0xFF );
}

/*
 * read_address_field: try to read a address field in a track
 * returns 1 if succeed, 0 otherwise
 */
static int read_address_field( int *volume, int *track, int *sector )
{
    int max_try;
    UInt8   nibble;

    max_try = 100;
    while( --max_try ) {
       nibble = gcr_read_nibble();
       check_D5:
       if ( nibble != 0xD5 )
          continue;
       nibble = gcr_read_nibble();
       if ( nibble != 0xAA )
       goto check_D5;
       nibble = gcr_read_nibble();
       if ( nibble != 0x96 )
          goto check_D5;
       *volume = FM_decode();
       *track = FM_decode();
       *sector = FM_decode();
       return ( *volume ^ *track ^ *sector ) == FM_decode() &&
          gcr_read_nibble() == 0xDE;
    }
    return 0;
}

static void write_address_field( int volume, int track, int sector )
{
    /*
     * write address mark
     */
    gcr_write_nibble( 0xD5 );
    gcr_write_nibble( 0xAA );
    gcr_write_nibble( 0x96 );

    /*
     * write Volume, Track, Sector & Check-sum
     */
    FM_encode( volume );
    FM_encode( track );
    FM_encode( sector );
    FM_encode( volume ^ track ^ sector );

    /*
     * write epilogue
     */
    gcr_write_nibble( 0xDE );
    gcr_write_nibble( 0xAA );
    gcr_write_nibble( 0xEB );
}

/*
 * read_data_field: read_data_field into GCR_buffers, return 0 if fail
 */
static int read_data_field(void)
{
    int i, max_try;
    UInt8   nibble, checksum;

    /*
     * read data mark
     */
    max_try = 32;
    while( --max_try ) {
       nibble = gcr_read_nibble();
    check_D5:
       if ( nibble != 0xD5 )
          continue;
       nibble = gcr_read_nibble();
       if ( nibble != 0xAA )
          goto check_D5;
       nibble = gcr_read_nibble();
       if ( nibble == 0xAD )
          break;
    }
    if ( !max_try ) /* fails to get address mark */
       return 0;

    for( i = 0x55, checksum = 0; i >= 0; i-- ) {
       checksum ^= GCR_decoding_table[gcr_read_nibble()];
       GCR_buffer2[i] = checksum;
    }

    for( i = 0; i < 256; i++ ) {
       checksum ^= GCR_decoding_table[gcr_read_nibble()];
       GCR_buffer[i] = checksum;
    }

    /* verify sector checksum */
    if ( checksum ^ GCR_decoding_table[gcr_read_nibble()] )
       return 0;

    /* check epilogue */
    return gcr_read_nibble() == 0xDE && gcr_read_nibble() == 0xAA;
}

static void write_data_field(void)
{
    int i;
    UInt8   last, checksum;

    /* write prologue */
    gcr_write_nibble( 0xD5 );
    gcr_write_nibble( 0xAA );
    gcr_write_nibble( 0xAD );

    /* write GCR encode data */
    for( i = 0x55, last = 0; i >= 0; i-- ) {
       checksum = last^ GCR_buffer2[i];
       gcr_write_nibble( GCR_encoding_table[checksum] );
       last = GCR_buffer2[i];
    }
    for( i = 0; i < 256; i++ ) {
       checksum = last ^ GCR_buffer[i];
       gcr_write_nibble( GCR_encoding_table[checksum] );
       last = GCR_buffer[i];
    }

    /* write checksum and epilogue */
    gcr_write_nibble( GCR_encoding_table[last] );
    gcr_write_nibble( 0xDE );
    gcr_write_nibble( 0xAA );
    gcr_write_nibble( 0xEB );
}

void SectorsToNibbles( UInt8 *sectors, UInt8 *nibbles, int volume, int track )
{
    int i;

    Track_Nibble = nibbles;
    Position = 0;

    /*write_sync( 128 );*/
    for( i = 0; i < 16; i ++ ) {
       encode62( sectors + Logical_Sector[i] * 0x100 );
       write_sync( 16 );
       write_address_field( volume, track, i );
       write_sync( 8 );
       write_data_field();
    }
}

int NibblesToSectors( UInt8 *nibbles, UInt8 *sectors, int volume, int track )
{
    int i, scanned[16], max_try, sectors_read;
    int vv, tt, ss; /* volume, track no. and sector no. */

    Track_Nibble = nibbles;
    Position = 0;

    for( i = 0; i < 16; i++ )
       scanned[i] = 0;
    sectors_read = 0;

    max_try = 200;
    while( --max_try ) {
       if ( !read_address_field( &vv, &tt, &ss ) )
          continue;

       if ( (volume && vv != volume ) || tt != track || ss < 0 || ss > 15 ){
          //printf("phy sector %d address field invalid\n", ss );
          continue; /* invalid values for vv, tt and ss, try again */
       }

       ss = Logical_Sector[ss];
       if ( scanned[ss] )   /* sector has been read */
          continue;

       if ( read_data_field() ) {
          decode62( sectors + ss * 0x100 );
          scanned[ss] = 1;  /* this sector's ok */
          sectors_read++;
       }
       else {
          //printf("fail reading data field of logical sector %d\n", ss );
       }
    }

    /* if has failed to read any one sector, report error */
    if ( sectors_read == 16 )
       return 1;
    else
       return 0;
}

