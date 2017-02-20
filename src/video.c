#include <PalmOS.h>
#include "Apple2.h"

extern READBYTE  *ReadFunction;
extern WRITEBYTE *WriteFunction;
extern UInt8     *AppleMemory;
extern UInt8     *AuxMemory;
extern UInt8     *AppleROM;
extern UInt16     grMode;
extern char      *AppleFontBitmap7x8;
extern char      *AppleFontBitmap4x6;
void  READONLY(UInt16, UInt8);
UInt8 RRMP(UInt16);
#define VIDIMAGE_TEXT       0
#define VIDIMAGE_LORES_MIX  1
#define VIDIMAGE_LORES      2
#define VIDIMAGE_HIRES_MIX  3
#define VIDIMAGE_HIRES      4
#define VIDIMAGE_DBLTEXT    5
VIDIMAGE vidUpdateFuncs[6];
VIDIMAGE updateVideo;
void gryUpdateText(void);
void gryUpdateHires(void);
void gryUpdateHiresMixed(void);
void clrUpdateText(void);
void clrUpdateHires(void);
void clrUpdateHiresMixed(void);
void hrclrUpdateText(void);
void hrclrUpdateLores(void);
void hrclrUpdateLoresMixed(void);
void hrclrUpdateHires(void);
void hrclrUpdateHiresMixed(void);
void hrclrUpdateDblText(void);
/*
 * Variables needed to update the video image.
 */
UInt8  dirtyVideo;
UInt8 *AppleFont = 0;
UInt8 *vidImage;       // Destination image bitmap (screen)
UInt16 vidPageOffset; // Source apple memory offset
UInt16 ScanlineOffsetTable[192];
UInt8  HRMono[256];
UInt8 *HRMonoLo, *HRMonoHi;
/*
 * Use a direct copy of the screen.
 */
UInt8 *vidScreenCache = 0;
#define VID_CACHE_OFFSET(v)     ScanlineOffsetTable[v]
#define VID_CACHE_INVALIDATE    MemSet(vidScreenCache, 0x2000, 0x69)
void initVideo(void)
{
    WinHandle onScreen;
    BitmapType *scanline;
    UInt16 i, count;

    dirtyVideo = true;
    onScreen = WinGetDisplayWindow();
    scanline = WinGetBitmap(onScreen);
    vidImage = (UInt8*) BmpGetBits(scanline);
    if (!vidScreenCache)
        vidScreenCache = MemPtrNew(0x2000);
    if (grMode > GRMODE_COLOR)
    {
//        vidImage += (GRMODE_ISCOLOR(grMode) ? 320 : 40) * 30;
			vidImage += 480 * 30;
        if (!AppleFont)
            AppleFont = MemPtrNew(0x100 * 0x08);
        MemMove(AppleFont,                                 AppleFontBitmap7x8,               0x40 * 0x08);
        MemMove(AppleFont + 0x40 * 0x08,                   AppleFontBitmap7x8,               0x40 * 0x08);
        MemMove(AppleFont + 0x40 * 0x08 * 2,               AppleFontBitmap7x8,               0x40 * 0x08);
        MemMove(AppleFont + 0x40 * 0x08 * 3,               AppleFontBitmap7x8,               0x20 * 0x08);
        MemMove(AppleFont + 0x40 * 0x08 * 3 + 0x20 * 0x08, AppleFontBitmap7x8 + 0x40 * 0x08, 0x20 * 0x08);
        for (count = 0x80 * 0x08; count < (0x100 * 0x08); count++) AppleFont[count] = ~AppleFont[count];
        vidUpdateFuncs[VIDIMAGE_TEXT]      = hrclrUpdateText;
        vidUpdateFuncs[VIDIMAGE_LORES]     = hrclrUpdateLores;
        vidUpdateFuncs[VIDIMAGE_LORES_MIX] = hrclrUpdateLoresMixed;
        vidUpdateFuncs[VIDIMAGE_HIRES]     = hrclrUpdateHires;
        vidUpdateFuncs[VIDIMAGE_HIRES_MIX] = hrclrUpdateHiresMixed;
        vidUpdateFuncs[VIDIMAGE_DBLTEXT]   = hrclrUpdateDblText;
    }
    else
    {
        if (!AppleFont)
            AppleFont = MemPtrNew(0x100 * 0x06);
        MemMove(AppleFont,                                 AppleFontBitmap4x6,               0x40 * 0x06);
        MemMove(AppleFont + 0x40 * 0x06,                   AppleFontBitmap4x6,               0x40 * 0x06);
        MemMove(AppleFont + 0x40 * 0x06 * 2,               AppleFontBitmap4x6,               0x40 * 0x06);
        MemMove(AppleFont + 0x40 * 0x06 * 3,               AppleFontBitmap4x6,               0x20 * 0x06);
        MemMove(AppleFont + 0x40 * 0x06 * 3 + 0x20 * 0x06, AppleFontBitmap4x6 + 0x40 * 0x06, 0x20 * 0x06);
        if (grMode == GRMODE_COLOR)
        {
            vidImage += 160 * 15;
            /*
             * Invert font data.
             */
            for (count = 0x80 * 0x06; count < (0x100 * 0x06); count++)
                AppleFont[count] = ~AppleFont[count];
            vidUpdateFuncs[VIDIMAGE_TEXT]      = clrUpdateText;
            vidUpdateFuncs[VIDIMAGE_LORES_MIX] = clrUpdateText;
            vidUpdateFuncs[VIDIMAGE_LORES]     = clrUpdateText;
            vidUpdateFuncs[VIDIMAGE_HIRES]     = clrUpdateHires;
            vidUpdateFuncs[VIDIMAGE_HIRES_MIX] = clrUpdateHiresMixed;
            vidUpdateFuncs[VIDIMAGE_DBLTEXT]   = clrUpdateText;
        }
        else
        {
            vidImage += 40 * 15;
            /*
             * Expand font data.
             */
            for (count = 0x80 * 0x06; count < (0x100 * 0x06); count++)
            {
                AppleFont[count] = (((AppleFont[count] & 0x01) ? 0x00 : 0x03)
                                  | ((AppleFont[count] & 0x02) ? 0x00 : 0x0C)
                                  | ((AppleFont[count] & 0x04) ? 0x00 : 0x30)
                                  | ((AppleFont[count] & 0x08) ? 0x00 : 0xC0));
            }
            /*
             * Create Hires->Mono conversion table.
             */
            HRMonoLo = HRMono;
            HRMonoHi = HRMono + 128;
            for (count = 0; count < 0x80; count++)
            {
                HRMonoLo[count] = ((count & 0x40 ? 0 : 0x02)
                                 | (count & 0x20 ? 0 : 0x04)
                                 | (count & 0x10 ? 0 : 0x08)
                                 | (count & 0x08 ? 0 : 0x10)
                                 | (count & 0x04 ? 0 : 0x20)
                                 | (count & 0x02 ? 0 : 0x40)
                                 | (count & 0x01 ? 0 : 0x80));
                HRMonoHi[count] = ((count & 0x40 ? 0 : 0x01)
                                 | (count & 0x20 ? 0 : 0x02)
                                 | (count & 0x10 ? 0 : 0x04)
                                 | (count & 0x08 ? 0 : 0x08)
                                 | (count & 0x04 ? 0 : 0x10)
                                 | (count & 0x02 ? 0 : 0x20)
                                 | (count & 0x01 ? 0 : 0x40));
            }
            vidUpdateFuncs[VIDIMAGE_TEXT]      = gryUpdateText;
            vidUpdateFuncs[VIDIMAGE_LORES_MIX] = gryUpdateText;
            vidUpdateFuncs[VIDIMAGE_LORES]     = gryUpdateText;
            vidUpdateFuncs[VIDIMAGE_HIRES]     = gryUpdateHires;
            vidUpdateFuncs[VIDIMAGE_HIRES_MIX] = gryUpdateHiresMixed;
            vidUpdateFuncs[VIDIMAGE_DBLTEXT]   = gryUpdateText;
        }
    }
    /*
     * Create offset table for rendering video image top->down.
     */
    for (i = 0;i < 192;i++)
        ScanlineOffsetTable[i] = (i & 7) * 0x400 + ((i >> 3) & 7) * 0x80 + (i >> 6) * 0x28;
    setVideoFuncs(true);
}

/****************************************************************\
*                                                                *
*                        Video mode state                        *
*                                                                *
\****************************************************************/

void clrVidFuncs(UInt16 vidBits)
{
    UInt16 page;

    if (!SS_ISSET(vidBits, SS_TEXT) && SS_ISSET(vidBits, SS_HIRES))
    {
        if (SS_ISSET(vidBits, SS_PAGE2))
            for (page = 0x40; page < 0x60; page++)
                WriteFunction[page] = NULL;
        else
            for (page = 0x20; page < 0x40; page++)
                WriteFunction[page] = NULL;
    }
    else
    {
        if (SS_ISSET(vidBits, SS_PAGE2) && !SS_ISSET(vidBits, SS_80STORE))
        {
            WriteFunction[0x08] = NULL;
            WriteFunction[0x09] = NULL;
            WriteFunction[0x0A] = NULL;
            WriteFunction[0x0B] = NULL;
        }
        else
        {
            WriteFunction[0x04] = NULL;
            WriteFunction[0x05] = NULL;
            WriteFunction[0x06] = NULL;
            WriteFunction[0x07] = NULL;
            ReadFunction[0x04]  = NULL;
            ReadFunction[0x05]  = NULL;
            ReadFunction[0x06]  = NULL;
            ReadFunction[0x07]  = NULL;
        }
    }
}
/* Screen Write */
void WTXTM(UInt16, UInt8);
void WATXTM(UInt16, UInt8);
UInt8 RATXTM(UInt16 address);
void awrap_AuxTxtMemReadWrite(void)
{
    asm(".global WTXTM");
    asm(".global WATXTM");
    asm(".global RATXTM");
    asm("WTXTM:");
    asm("move.b %d0, (%a2,%d6.l)");
    asm("move.b #1, dirtyVideo@END.w(%a5)");
    asm("rts");
    asm("WATXTM:");
    asm("move.l AuxMemory@END.w(%a5), %a6");
    asm("add.l  #(16384-0x400), %a6");
    asm("move.b %d0, (%a6, %d6.l)");
    asm("move.b #1, dirtyVideo@END.w(%a5)");
    asm("rts");
    asm("RATXTM:");
    asm("move.l AuxMemory@END.w(%a5), %a6");
    asm("add.l  #(16384-0x400), %a6");
    asm("clr.l  %d0");
    asm("move.b (%a6, %d6.l), %d0");
    asm("rts");
}
WRMEM_CWRAP(WSCM, address, data)
{
    UInt16 page;

    dirtyVideo = 2; // Add call to setVideoFuncs at next VideoUpdate
    AppleMemory[address] = data;
    /*
     * Once dirtyVideo is set, no need to slow down the rest.
     */
    clrVidFuncs(vidIOU);
}
/*
 * Set the updateVideo function based on all the settings.
 * This is incomplete - missing lo-res checking.
 */
void setVideoFuncs(Boolean invalidate_cache)
{
    UInt16 page;

    if (invalidate_cache)
    {
        VID_CACHE_INVALIDATE;
    }
    if (!SS_ISSET(memIOU, SS_RAMWRT))
    {
        if (SS_ISSET(vidIOU, SS_TEXT))
        {
            updateVideo = vidUpdateFuncs[SS_ISSET(vidIOU, SS_80VID) ? VIDIMAGE_DBLTEXT : VIDIMAGE_TEXT];
            if (SS_ISSET(vidIOU, SS_PAGE2))
            {
                if (SS_ISSET(vidIOU, SS_80STORE))
                {
                    vidPageOffset = 0x0400;
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
                    vidPageOffset       = 0x0800;
                    WriteFunction[0x08] = WTXTM;
                    WriteFunction[0x09] = WTXTM;
                    WriteFunction[0x0A] = WTXTM;
                    WriteFunction[0x0B] = WTXTM;
                }
            }
            else
            {
                vidPageOffset       = 0x0400;
                WriteFunction[0x04] = WTXTM;
                WriteFunction[0x05] = WTXTM;
                WriteFunction[0x06] = WTXTM;
                WriteFunction[0x07] = WTXTM;
            }
        }
        else if (SS_ISSET(vidIOU, SS_HIRES))
        {
            if (SS_ISSET(vidIOU, SS_PAGE2))
            {
                vidPageOffset = 0x4000;
                for (page = 0x40; page < 0x60; page++)
                    WriteFunction[page] = WSCM;
                if (SS_ISSET(vidIOU, SS_MIXED))
                {
                    updateVideo         = vidUpdateFuncs[VIDIMAGE_HIRES_MIX];
                    WriteFunction[0x04] = NULL;
                    WriteFunction[0x05] = NULL;
                    WriteFunction[0x06] = NULL;
                    WriteFunction[0x07] = NULL;
                    WriteFunction[0x08] = WSCM;
                    WriteFunction[0x09] = WSCM;
                    WriteFunction[0x0A] = WSCM;
                    WriteFunction[0x0B] = WSCM;
                }
                else
                    updateVideo = vidUpdateFuncs[VIDIMAGE_HIRES];
            }
            else
            {
                vidPageOffset = 0x2000;
                for (page = 0x20; page < 0x40; page++)
                    WriteFunction[page] = WSCM;
                if (SS_ISSET(vidIOU, SS_MIXED))
                {
                    updateVideo = vidUpdateFuncs[VIDIMAGE_HIRES_MIX];
                    WriteFunction[0x04] = WSCM;
                    WriteFunction[0x05] = WSCM;
                    WriteFunction[0x06] = WSCM;
                    WriteFunction[0x07] = WSCM;
                    WriteFunction[0x08] = NULL;
                    WriteFunction[0x09] = NULL;
                    WriteFunction[0x0A] = NULL;
                    WriteFunction[0x0B] = NULL;
                }
                else
                    updateVideo = vidUpdateFuncs[VIDIMAGE_HIRES];
            }
        }
        else
        {
            if (SS_ISSET(vidIOU, SS_PAGE2))
            {
                vidPageOffset       = 0x0800;
                updateVideo         = SS_ISSET(vidIOU, SS_MIXED) ? vidUpdateFuncs[VIDIMAGE_LORES_MIX] : vidUpdateFuncs[VIDIMAGE_LORES];
                WriteFunction[0x08] = WSCM;
                WriteFunction[0x09] = WSCM;
                WriteFunction[0x0A] = WSCM;
                WriteFunction[0x0B] = WSCM;
            }
            else
            {
                vidPageOffset       = 0x0400;
                updateVideo         = SS_ISSET(vidIOU, SS_MIXED) ? vidUpdateFuncs[VIDIMAGE_LORES_MIX] : vidUpdateFuncs[VIDIMAGE_LORES];
                WriteFunction[0x04] = WSCM;
                WriteFunction[0x05] = WSCM;
                WriteFunction[0x06] = WSCM;
                WriteFunction[0x07] = WSCM;
            }
        }
    }
}
void toggleVideoSoftSwitch(UInt8 io)
{
    UInt16 dirty, page;
    Boolean invalidate = false;

    dirty = vidIOU;
    switch (io)
    {
        case 0x00: // Text -> Graphics
            if (SS_ISSET(vidIOU, SS_TEXT))
                invalidate = true;
            SS_RESET(vidIOU, SS_TEXT);
            break;
        case 0x01: // Graphics -> Text
            if (!SS_ISSET(vidIOU, SS_TEXT))
                invalidate = true;
            SS_SET(vidIOU, SS_TEXT);
            break;
        case 0x02: // Full Screen
            if (SS_ISSET(vidIOU, SS_MIXED))
                invalidate = true;
            SS_RESET(vidIOU, SS_MIXED);
            break;
        case 0x03: // Mixed Screen
            if (!SS_ISSET(vidIOU, SS_MIXED))
                invalidate = true;
            SS_SET(vidIOU, SS_MIXED);
            break;
        case 0x04: // page 1
            SS_RESET(vidIOU, SS_PAGE2);
            break;
        case 0x05: // page 2
            SS_SET(vidIOU, SS_PAGE2);
            break;
        case 0x06: // HiRes -> Text/LoRes
            if (SS_ISSET(vidIOU, SS_HIRES))
                invalidate = true;
            SS_RESET(vidIOU, SS_HIRES);
            break;
        case 0x07: // Text/LoRes -> HiRes
            if (!SS_ISSET(vidIOU, SS_HIRES))
                invalidate = true;
            SS_SET(vidIOU, SS_HIRES);
            break;
    }
    if (dirty != vidIOU)
    {
        dirtyVideo = true;
        clrVidFuncs(dirty);
        setVideoFuncs(invalidate);
        if (!SS_ISSET(vidIOU, SS_TEXT) && SS_ISSET((dirty ^ vidIOU), SS_PAGE2))
        {
            AppleInstrCount += AppleInstrInc - state6502.LongRegs.INST;
            state6502.LongRegs.INST = 0; // Keep frequent screen updates from holding off user input.
        }
    }
}

/****************************************************************\
*                                                                *
*                      Video update routines                     *
*                                                                *
\****************************************************************/

#define PALM_CLR_WHITE      0
#define PALM_CLR_BLACK      255
#define PALM_CLR_GREEN      210
#define PALM_CLR_VIOLET     17
#define PALM_CLR_PURPLE     17
#define PALM_CLR_RED        125
#define PALM_CLR_BLUE       107
#define PALM_CLR_DRKRED     (8*16+9)
#define PALM_CLR_DRKBLUE    (12*16+11)
#define PALM_CLR_DRKGREEN   (13*16+4)
#define PALM_CLR_DRKGREY    (13*16+11)
#define PALM_CLR_CYAN       (4*16+4)
#define PALM_CLR_BROWN      (8*16+7)
#define PALM_CLR_ORANGE     (7*16+5)
#define PALM_CLR_LTGREY     (13*16+14)
#define PALM_CLR_PINK       2
#define PALM_CLR_YELLOW     108
#define PALM_CLR_AQUA       (5*16+10)
#define PALM_CLR_GREY      (13*16+12)
/*
 * 16 color table.
 */
UInt8 clr16[16] =
{
    PALM_CLR_BLACK,
    PALM_CLR_DRKRED,
    PALM_CLR_DRKBLUE,
    PALM_CLR_VIOLET,
    PALM_CLR_DRKGREEN,
    PALM_CLR_DRKGREY,
    PALM_CLR_BLUE,
    PALM_CLR_CYAN,
    PALM_CLR_BROWN,
    PALM_CLR_ORANGE,
    PALM_CLR_LTGREY,
    PALM_CLR_PINK,
    PALM_CLR_GREEN,
    PALM_CLR_YELLOW,
    PALM_CLR_AQUA,
    PALM_CLR_WHITE
};
/*
 * Color array for standard res device.
 */
UInt8 pixEven[96] =
{
    /*
     * 0 MSB.
     */
    PALM_CLR_BLACK ,
    PALM_CLR_BLACK ,
    PALM_CLR_VIOLET,
    PALM_CLR_GREY,
    PALM_CLR_GREEN ,
    PALM_CLR_GREEN ,
    PALM_CLR_WHITE ,
    PALM_CLR_WHITE ,
    PALM_CLR_BLACK ,
    PALM_CLR_BLACK ,
    PALM_CLR_VIOLET,
    PALM_CLR_GREY,
    PALM_CLR_GREY,
    PALM_CLR_GREY,
    PALM_CLR_WHITE ,
    PALM_CLR_WHITE ,
    /*
     * 1 MSB.
     */
    PALM_CLR_BLACK ,
    PALM_CLR_BLACK ,
    PALM_CLR_BLUE  ,
    PALM_CLR_GREY,
    PALM_CLR_RED   ,
    PALM_CLR_RED   ,
    PALM_CLR_WHITE ,
    PALM_CLR_WHITE ,
    PALM_CLR_BLACK ,
    PALM_CLR_BLACK ,
    PALM_CLR_BLUE  ,
    PALM_CLR_GREY,
    PALM_CLR_GREY,
    PALM_CLR_GREY,
    PALM_CLR_WHITE ,
    PALM_CLR_WHITE ,
    /*
     * These values are for the even->odd byte transition.
     * Even 0 MSB -> Odd 0 MSB.
     */
    PALM_CLR_BLACK ,
    PALM_CLR_BLACK ,
    PALM_CLR_VIOLET,
    PALM_CLR_GREY,
    PALM_CLR_GREEN ,
    PALM_CLR_GREEN ,
    PALM_CLR_WHITE ,
    PALM_CLR_WHITE ,
    PALM_CLR_BLACK ,
    PALM_CLR_BLACK ,
    PALM_CLR_VIOLET,
    PALM_CLR_GREY,
    PALM_CLR_GREY,
    PALM_CLR_GREY,
    PALM_CLR_WHITE ,
    PALM_CLR_WHITE ,
    /*
     * These values are for the even->odd byte transition.
     * Even 1 MSB -> Odd 0 MSB.
     */
    PALM_CLR_BLACK ,
    PALM_CLR_BLACK ,
    PALM_CLR_BLUE  ,
    PALM_CLR_GREY,
    PALM_CLR_GREEN ,
    PALM_CLR_GREEN ,
    PALM_CLR_WHITE ,
    PALM_CLR_WHITE ,
    PALM_CLR_BLACK ,
    PALM_CLR_BLACK ,
    PALM_CLR_BLUE  ,
    PALM_CLR_GREY,
    PALM_CLR_GREY,
    PALM_CLR_GREY,
    PALM_CLR_WHITE ,
    PALM_CLR_WHITE ,
    /*
     * These values are for the even->odd byte transition.
     * Even 0 MSB -> Odd 1 MSB.
     */
    PALM_CLR_BLACK ,
    PALM_CLR_BLACK ,
    PALM_CLR_VIOLET,
    PALM_CLR_GREY,
    PALM_CLR_RED   ,
    PALM_CLR_RED   ,
    PALM_CLR_WHITE ,
    PALM_CLR_WHITE ,
    PALM_CLR_BLACK ,
    PALM_CLR_BLACK ,
    PALM_CLR_VIOLET,
    PALM_CLR_GREY,
    PALM_CLR_GREY,
    PALM_CLR_GREY,
    PALM_CLR_WHITE ,
    PALM_CLR_WHITE ,
    /*
     * These values are for the even->odd byte transition.
     * Even 1 MSB -> Odd 1 MSB.
     */
    PALM_CLR_BLACK ,
    PALM_CLR_BLACK ,
    PALM_CLR_BLUE  ,
    PALM_CLR_GREY,
    PALM_CLR_RED   ,
    PALM_CLR_RED   ,
    PALM_CLR_WHITE ,
    PALM_CLR_WHITE ,
    PALM_CLR_BLACK ,
    PALM_CLR_BLACK ,
    PALM_CLR_BLUE  ,
    PALM_CLR_GREY,
    PALM_CLR_GREY,
    PALM_CLR_GREY,
    PALM_CLR_WHITE ,
    PALM_CLR_WHITE
};
UInt8 pixOdd[96] =
{
    /*
     * 0 MSB.
     */
    PALM_CLR_BLACK ,
    PALM_CLR_BLACK ,
    PALM_CLR_GREEN ,
    PALM_CLR_GREY,
    PALM_CLR_VIOLET,
    PALM_CLR_VIOLET,
    PALM_CLR_WHITE ,
    PALM_CLR_WHITE ,
    PALM_CLR_BLACK ,
    PALM_CLR_BLACK ,
    PALM_CLR_GREEN ,
    PALM_CLR_GREY,
    PALM_CLR_GREY,
    PALM_CLR_GREY,
    PALM_CLR_WHITE ,
    PALM_CLR_WHITE ,
    /*
     * 1 MSB.
     */
    PALM_CLR_BLACK ,
    PALM_CLR_BLACK ,
    PALM_CLR_RED   ,
    PALM_CLR_GREY,
    PALM_CLR_BLUE  ,
    PALM_CLR_BLUE  ,
    PALM_CLR_WHITE ,
    PALM_CLR_WHITE ,
    PALM_CLR_BLACK ,
    PALM_CLR_BLACK ,
    PALM_CLR_RED   ,
    PALM_CLR_GREY,
    PALM_CLR_GREY,
    PALM_CLR_GREY,
    PALM_CLR_WHITE ,
    PALM_CLR_WHITE ,
    /*
     * These values are for the even->odd byte transition.
     * Odd 0 MSB -> Even 0 MSB.
     */
    PALM_CLR_BLACK ,
    PALM_CLR_BLACK ,
    PALM_CLR_GREEN ,
    PALM_CLR_GREY,
    PALM_CLR_VIOLET,
    PALM_CLR_VIOLET,
    PALM_CLR_WHITE ,
    PALM_CLR_WHITE ,
    PALM_CLR_BLACK ,
    PALM_CLR_BLACK ,
    PALM_CLR_GREEN ,
    PALM_CLR_GREY,
    PALM_CLR_GREY,
    PALM_CLR_GREY,
    PALM_CLR_WHITE ,
    PALM_CLR_WHITE ,
    /*
     * These values are for the even->odd byte transition.
     * Odd 1 MSB -> Even 0 MSB.
     */
    PALM_CLR_BLACK ,
    PALM_CLR_BLACK ,
    PALM_CLR_RED   ,
    PALM_CLR_GREY,
    PALM_CLR_VIOLET,
    PALM_CLR_VIOLET,
    PALM_CLR_WHITE ,
    PALM_CLR_WHITE ,
    PALM_CLR_BLACK ,
    PALM_CLR_BLACK ,
    PALM_CLR_RED   ,
    PALM_CLR_GREY,
    PALM_CLR_GREY,
    PALM_CLR_GREY,
    PALM_CLR_WHITE ,
    PALM_CLR_WHITE ,
    /*
     * These values are for the even->odd byte transition.
     * Odd 0 MSB -> Even 1 MSB.
     */
    PALM_CLR_BLACK ,
    PALM_CLR_BLACK ,
    PALM_CLR_GREEN ,
    PALM_CLR_GREY,
    PALM_CLR_BLUE  ,
    PALM_CLR_BLUE  ,
    PALM_CLR_WHITE ,
    PALM_CLR_WHITE ,
    PALM_CLR_BLACK ,
    PALM_CLR_BLACK ,
    PALM_CLR_GREEN ,
    PALM_CLR_GREY,
    PALM_CLR_GREY,
    PALM_CLR_GREY,
    PALM_CLR_WHITE ,
    PALM_CLR_WHITE ,
    /*
     * These values are for the even->odd byte transition.
     * Odd 1 MSB -> Even 1 MSB.
     */
    PALM_CLR_BLACK ,
    PALM_CLR_BLACK ,
    PALM_CLR_RED   ,
    PALM_CLR_GREY,
    PALM_CLR_BLUE  ,
    PALM_CLR_BLUE  ,
    PALM_CLR_WHITE ,
    PALM_CLR_WHITE ,
    PALM_CLR_BLACK ,
    PALM_CLR_BLACK ,
    PALM_CLR_RED   ,
    PALM_CLR_GREY,
    PALM_CLR_GREY,
    PALM_CLR_GREY,
    PALM_CLR_WHITE ,
    PALM_CLR_WHITE
};
/*
 * Color interpolation arrays.
 */
#define CLR_PAIR(c1,c2) (((PALM_CLR_##c1)<<8)|(PALM_CLR_##c2))
/*
 * Interpolated colors (sold fill).
 */
UInt16 pixPairEven[96] =
{
    /*
     * 0 MSB.
     */
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(VIOLET, VIOLET),
    CLR_PAIR(WHITE , BLACK ),
    CLR_PAIR(GREEN , GREEN ),
    CLR_PAIR(GREEN , GREEN ),
    CLR_PAIR(WHITE , WHITE ),
    CLR_PAIR(WHITE , WHITE ),
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(VIOLET, VIOLET),
    CLR_PAIR(WHITE , BLACK ),
    CLR_PAIR(BLACK , WHITE ),
    CLR_PAIR(BLACK , WHITE ),
    CLR_PAIR(WHITE , WHITE ),
    CLR_PAIR(WHITE , WHITE ),
    /*
     * 1 MSB.
     */
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(BLUE  , BLUE  ),
    CLR_PAIR(WHITE , BLACK ),
    CLR_PAIR(RED   , RED   ),
    CLR_PAIR(RED   , RED   ),
    CLR_PAIR(WHITE , WHITE ),
    CLR_PAIR(WHITE , WHITE ),
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(BLUE  , BLUE  ),
    CLR_PAIR(WHITE , BLACK ),
    CLR_PAIR(BLACK , WHITE ),
    CLR_PAIR(BLACK , WHITE ),
    CLR_PAIR(WHITE , WHITE ),
    CLR_PAIR(WHITE , WHITE ),
    /*
     * These values are for the even->odd byte transition.
     * Even 0 MSB -> Odd 0 MSB.
     */
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(VIOLET, VIOLET),
    CLR_PAIR(WHITE , BLACK ),
    CLR_PAIR(GREEN , GREEN ),
    CLR_PAIR(GREEN , GREEN ),
    CLR_PAIR(WHITE , WHITE ),
    CLR_PAIR(WHITE , WHITE ),
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(VIOLET, VIOLET),
    CLR_PAIR(WHITE , BLACK ),
    CLR_PAIR(BLACK , WHITE ),
    CLR_PAIR(BLACK , WHITE ),
    CLR_PAIR(WHITE , WHITE ),
    CLR_PAIR(WHITE , WHITE ),
    /*
     * These values are for the even->odd byte transition.
     * Even 1 MSB -> Odd 0 MSB.
     */
    CLR_PAIR(BLACK , BLACK),
    CLR_PAIR(BLACK , BLACK),
    CLR_PAIR(BLUE  , BLUE ),
    CLR_PAIR(WHITE , BLACK),
    CLR_PAIR(GREEN , GREEN),
    CLR_PAIR(GREEN , GREEN),
    CLR_PAIR(WHITE , WHITE),
    CLR_PAIR(WHITE , WHITE),
    CLR_PAIR(BLACK , BLACK),
    CLR_PAIR(BLACK , BLACK),
    CLR_PAIR(BLUE,   BLUE ),
    CLR_PAIR(WHITE , BLACK),
    CLR_PAIR(BLACK , WHITE),
    CLR_PAIR(BLACK , WHITE),
    CLR_PAIR(WHITE , WHITE),
    CLR_PAIR(WHITE , WHITE),
    /*
     * These values are for the even->odd byte transition.
     * Even 0 MSB -> Odd 1 MSB.
     */
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(VIOLET, VIOLET),
    CLR_PAIR(WHITE , BLACK ),
    CLR_PAIR(RED   , RED   ),
    CLR_PAIR(RED   , RED   ),
    CLR_PAIR(WHITE , WHITE ),
    CLR_PAIR(WHITE , WHITE ),
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(VIOLET, VIOLET),
    CLR_PAIR(WHITE , BLACK ),
    CLR_PAIR(BLACK , WHITE ),
    CLR_PAIR(BLACK , WHITE ),
    CLR_PAIR(WHITE , WHITE ),
    CLR_PAIR(WHITE , WHITE ),
    /*
     * These values are for the even->odd byte transition.
     * Even 1 MSB -> Odd 1 MSB.
     */
    CLR_PAIR(BLACK , BLACK),
    CLR_PAIR(BLACK , BLACK),
    CLR_PAIR(BLUE  , BLUE ),
    CLR_PAIR(WHITE , BLACK),
    CLR_PAIR(RED   , RED  ),
    CLR_PAIR(RED   , RED  ),
    CLR_PAIR(WHITE , WHITE),
    CLR_PAIR(WHITE , WHITE),
    CLR_PAIR(BLACK , BLACK),
    CLR_PAIR(BLACK , BLACK),
    CLR_PAIR(BLUE  , BLUE ),
    CLR_PAIR(WHITE , BLACK),
    CLR_PAIR(BLACK , WHITE),
    CLR_PAIR(BLACK , WHITE),
    CLR_PAIR(WHITE , WHITE),
    CLR_PAIR(WHITE , WHITE)
};
UInt16 pixPairOdd[96] =
{
    /*
     * 0 MSB.
     */
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(GREEN , GREEN ),
    CLR_PAIR(WHITE , BLACK ),
    CLR_PAIR(VIOLET, VIOLET),
    CLR_PAIR(VIOLET, VIOLET),
    CLR_PAIR(WHITE , WHITE ),
    CLR_PAIR(WHITE , WHITE ),
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(GREEN , GREEN ),
    CLR_PAIR(WHITE , BLACK ),
    CLR_PAIR(BLACK , WHITE ),
    CLR_PAIR(BLACK , WHITE ),
    CLR_PAIR(WHITE , WHITE ),
    CLR_PAIR(WHITE , WHITE ),
    /*
     * 1 MSB.
     */
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(RED   , RED   ),
    CLR_PAIR(WHITE , BLACK ),
    CLR_PAIR(BLUE  , BLUE  ),
    CLR_PAIR(BLUE  , BLUE  ),
    CLR_PAIR(WHITE , WHITE ),
    CLR_PAIR(WHITE , WHITE ),
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(RED   , RED   ),
    CLR_PAIR(WHITE , BLACK ),
    CLR_PAIR(BLACK , WHITE ),
    CLR_PAIR(BLACK , WHITE ),
    CLR_PAIR(WHITE , WHITE ),
    CLR_PAIR(WHITE , WHITE ),
    /*
     * These values are for the even->odd byte transition.
     * Odd 0 MSB -> Even 0 MSB.
     */
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(GREEN,  GREEN ),
    CLR_PAIR(WHITE , BLACK ),
    CLR_PAIR(VIOLET, VIOLET),
    CLR_PAIR(VIOLET, VIOLET),
    CLR_PAIR(WHITE , WHITE ),
    CLR_PAIR(WHITE , WHITE ),
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(GREEN , GREEN ),
    CLR_PAIR(WHITE , BLACK ),
    CLR_PAIR(BLACK , WHITE ),
    CLR_PAIR(BLACK , WHITE ),
    CLR_PAIR(WHITE , WHITE ),
    CLR_PAIR(WHITE , WHITE ),
    /*
     * These values are for the even->odd byte transition.
     * Odd 1 MSB -> Even 0 MSB.
     */
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(RED   , RED   ),
    CLR_PAIR(WHITE , BLACK ),
    CLR_PAIR(VIOLET, VIOLET),
    CLR_PAIR(VIOLET, VIOLET),
    CLR_PAIR(WHITE , WHITE ),
    CLR_PAIR(WHITE , WHITE ),
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(BLACK , BLACK ),
    CLR_PAIR(RED   , RED   ),
    CLR_PAIR(WHITE , BLACK ),
    CLR_PAIR(BLACK , WHITE ),
    CLR_PAIR(BLACK , WHITE ),
    CLR_PAIR(WHITE , WHITE ),
    CLR_PAIR(WHITE , WHITE ),
    /*
     * These values are for the even->odd byte transition.
     * Odd 0 MSB -> Even 1 MSB.
     */
    CLR_PAIR(BLACK , BLACK),
    CLR_PAIR(BLACK , BLACK),
    CLR_PAIR(GREEN , GREEN),
    CLR_PAIR(WHITE , BLACK),
    CLR_PAIR(BLUE  , BLUE ),
    CLR_PAIR(BLUE  , BLUE ),
    CLR_PAIR(WHITE , WHITE),
    CLR_PAIR(WHITE , WHITE),
    CLR_PAIR(BLACK , BLACK),
    CLR_PAIR(BLACK , BLACK),
    CLR_PAIR(GREEN , GREEN),
    CLR_PAIR(WHITE , BLACK),
    CLR_PAIR(BLACK , WHITE),
    CLR_PAIR(BLACK , WHITE),
    CLR_PAIR(WHITE , WHITE),
    CLR_PAIR(WHITE , WHITE),
    /*
     * These values are for the even->odd byte transition.
     * Odd 1 MSB -> Even 1 MSB.
     */
    CLR_PAIR(BLACK , BLACK),
    CLR_PAIR(BLACK , BLACK),
    CLR_PAIR(RED   , RED  ),
    CLR_PAIR(WHITE , BLACK),
    CLR_PAIR(BLUE  , BLUE ),
    CLR_PAIR(BLUE  , BLUE ),
    CLR_PAIR(WHITE , WHITE),
    CLR_PAIR(WHITE , WHITE),
    CLR_PAIR(BLACK , BLACK),
    CLR_PAIR(BLACK , BLACK),
    CLR_PAIR(RED   , RED  ),
    CLR_PAIR(WHITE , BLACK),
    CLR_PAIR(BLACK , WHITE),
    CLR_PAIR(BLACK , WHITE),
    CLR_PAIR(WHITE , WHITE),
    CLR_PAIR(WHITE , WHITE)
};
typedef void (*pfnVidUpdateSection)(UInt8 *, UInt8 *, UInt8 *);
void gryUpdateTextRow(UInt8 *, UInt8 *, UInt8 *);
void clrUpdateTextRow(UInt8 *, UInt8 *, UInt8 *);
void gryUpdateHiresScanline(UInt8 *, UInt8 *, UInt8 *);
void clrUpdateHiresScanline(UInt8 *, UInt8 *, UInt8 *);
void defUpdateText(pfnVidUpdateSection pfnText, UInt16 pitch)
{
    Int16  vert;
    UInt8 *_pointer = vidImage;
    dirtyVideo = false;
    for (vert = 0; vert < 192; vert += 8)
    {
        pfnText(&AppleMemory[ScanlineOffsetTable[vert] + vidPageOffset], _pointer, &vidScreenCache[VID_CACHE_OFFSET(vert)]);
        _pointer += pitch * 6;
    }
}
void gryUpdateText(void)
{
    defUpdateText(gryUpdateTextRow, 40);
}
void clrUpdateText(void)
{
    defUpdateText(clrUpdateTextRow, 160);
}
void defUpdateHires(pfnVidUpdateSection pfnHires, UInt16 pitch)
{
    Int16  hori, vert;
    UInt16 biteven, bitodd;
    UInt8 *_pointer = vidImage;
    dirtyVideo = false;
    for (vert = 0; vert < 192; vert++)
    {
        if ((vert & 0x03) == 0x01)
        {
            UInt32 shrinkScans[10], *scanptr, *nextscanptr;
            scanptr     = (UInt32 *)&AppleMemory[ScanlineOffsetTable[vert] + vidPageOffset];
            nextscanptr = (UInt32 *)&AppleMemory[ScanlineOffsetTable[++vert] + vidPageOffset];
            for (hori = 0; hori < 10; hori++)
                shrinkScans[hori] = *scanptr++ | *nextscanptr++;
            pfnHires((UInt8 *)&shrinkScans, _pointer, &vidScreenCache[VID_CACHE_OFFSET(vert)]);
        }
        else
        {
            pfnHires(&AppleMemory[ScanlineOffsetTable[vert] + vidPageOffset], _pointer, &vidScreenCache[VID_CACHE_OFFSET(vert)]);
        }
        _pointer += pitch;
    }
}
void gryUpdateHires(void)
{
    defUpdateHires(gryUpdateHiresScanline, 40);
}
void clrUpdateHires(void)
{
    defUpdateHires(clrUpdateHiresScanline, 160);
}
void defUpdateHiresMixed(pfnVidUpdateSection pfnHires, pfnVidUpdateSection pfnText, UInt16 pitch)
{
    Int16  hori, vert, textpage;
    UInt16 scanaddr, chary, biteven, bitodd;
    UInt8 *_pointer = vidImage;
    dirtyVideo = false;
    for (vert = 0; vert < 160/*144*/; vert++)
    {
        if ((vert & 0x03) == 0x01)
        {
            UInt32 shrinkScans[10], *scanptr, *nextscanptr;
            scanptr     = (UInt32 *)&AppleMemory[ScanlineOffsetTable[vert] + vidPageOffset];
            nextscanptr = (UInt32 *)&AppleMemory[ScanlineOffsetTable[++vert] + vidPageOffset];
            for (hori = 0; hori < 10; hori++)
                shrinkScans[hori] = *scanptr++ | *nextscanptr++;
            pfnHires((UInt8 *)&shrinkScans, _pointer, &vidScreenCache[VID_CACHE_OFFSET(vert)]);
        }
        else
        {
            pfnHires(&AppleMemory[ScanlineOffsetTable[vert] + vidPageOffset], _pointer, &vidScreenCache[VID_CACHE_OFFSET(vert)]);
        }
        _pointer += pitch;
    }
    chary    = 0;
    textpage = SS_ISSET(vidIOU, SS_PAGE2) ? 0x0800 : 0x0400;
    while (vert < 192/*144*/)
    {
        pfnText(&AppleMemory[ScanlineOffsetTable[vert] + textpage], _pointer, &vidScreenCache[VID_CACHE_OFFSET(vert)]);
        _pointer += pitch * 6;
        vert += 8;
    }
}
void gryUpdateHiresMixed(void)
{
    defUpdateHiresMixed(gryUpdateHiresScanline, gryUpdateTextRow, 40);
}
void clrUpdateHiresMixed(void)
{
    defUpdateHiresMixed(clrUpdateHiresScanline, clrUpdateTextRow, 160);
}
void hrclrUpdateTextRow(UInt8 *, UInt8 *, UInt8 *);
void hrclrUpdateText(void)
{
    Int16  vert;
    UInt8 *_pointer = vidImage;
    dirtyVideo = false;
    for (vert = 0; vert < 192; vert += 8)
    {
        hrclrUpdateTextRow(&AppleMemory[ScanlineOffsetTable[vert] + vidPageOffset], _pointer, &vidScreenCache[VID_CACHE_OFFSET(vert)]);
//        _pointer += 320 * 9;
		_pointer += 480 * 9;
    }
}
void hrclrUpdateDblTextRow(UInt8 *, UInt8 *, UInt8 *);
void hrclrUpdateDblText(void)
{
    Int16  vert;
    UInt8 *_pointer = vidImage;
    dirtyVideo = false;
    for (vert = 0; vert < 192; vert += 8)
    {
        hrclrUpdateDblTextRow(&AuxMemory[ScanlineOffsetTable[vert] + 16384], _pointer, &vidScreenCache[VID_CACHE_OFFSET(vert + 1)]);
        hrclrUpdateDblTextRow(&AppleMemory[ScanlineOffsetTable[vert] + vidPageOffset], _pointer + 4, &vidScreenCache[VID_CACHE_OFFSET(vert)]);
//        _pointer += 320 * 9;
		_pointer += 480 * 9;
    }
}
void hrclrUpdateLoresRow(UInt8 *, UInt8 *, UInt8 *);
void hrclrUpdateLoresMixed(void)
{
    Int16 vert;
    UInt8 *_pointer = vidImage;
    dirtyVideo = false;
    for (vert = 0; vert < 160; vert += 8)
    {
        hrclrUpdateLoresRow(&AppleMemory[ScanlineOffsetTable[vert] + vidPageOffset], _pointer, &vidScreenCache[VID_CACHE_OFFSET(vert)]);
//        _pointer += 320 * 9;
		_pointer += 480 * 9;
    }
    while (vert < 192)
    {
        hrclrUpdateTextRow(&AppleMemory[ScanlineOffsetTable[vert] + vidPageOffset], _pointer, &vidScreenCache[VID_CACHE_OFFSET(vert)]);
//        _pointer += 320 * 9;
		_pointer += 480 * 9;
        vert += 8;
    }
}
void hrclrUpdateLores(void)
{
    Int16  vert;
    UInt8 *_pointer = vidImage;
    dirtyVideo = false;
    for (vert = 0; vert < 192; vert += 8)
    {
        hrclrUpdateLoresRow(&AppleMemory[ScanlineOffsetTable[vert] + vidPageOffset], _pointer, &vidScreenCache[VID_CACHE_OFFSET(vert)]);
//        _pointer += 320 * 9;
		_pointer += 480 * 9;
    }
}
void hrclrUpdateHiresScanline(UInt8 *, UInt8 *, UInt8 *);
void hrclrUpdateHiresDblScanline(UInt8 *, UInt8 *, UInt8 *);
void hrclrUpdateHiresMixed(void)
{
    Int16 vert, textpage;
    UInt8 *_pointer = vidImage;
    dirtyVideo = false;
    for (vert = 0; vert < 160; vert++)
    {
        if ((vert & 7) == 0)
        {
            hrclrUpdateHiresDblScanline(&AppleMemory[ScanlineOffsetTable[vert] + vidPageOffset], _pointer, &vidScreenCache[VID_CACHE_OFFSET(vert)]);
//            _pointer += 640;
			_pointer += 960;
        }
        else
        {
            hrclrUpdateHiresScanline(&AppleMemory[ScanlineOffsetTable[vert] + vidPageOffset], _pointer, &vidScreenCache[VID_CACHE_OFFSET(vert)]);
//            _pointer += 320;
			_pointer += 480;
        }
    }
    textpage = SS_ISSET(vidIOU, SS_PAGE2) ? 0x0800 : 0x0400;
    while (vert < 192)
    {
        hrclrUpdateTextRow(&AppleMemory[ScanlineOffsetTable[vert] + textpage], _pointer, &vidScreenCache[VID_CACHE_OFFSET(vert)]);
//        _pointer += 320 * 9;
		_pointer += 480 * 9;
        vert += 8;
    }
}
void hrclrUpdateHires(void)
{
    Int16 vert;
    UInt8 *_pointer = vidImage;
    dirtyVideo = false;
    for (vert = 0; vert < 192; vert++)
    {
        if ((vert & 7) == 0)
        {
            hrclrUpdateHiresDblScanline(&AppleMemory[ScanlineOffsetTable[vert] + vidPageOffset], _pointer, &vidScreenCache[VID_CACHE_OFFSET(vert)]);
//            _pointer += 640;
			_pointer += 960;
        }
        else
        {
            hrclrUpdateHiresScanline(&AppleMemory[ScanlineOffsetTable[vert] + vidPageOffset], _pointer, &vidScreenCache[VID_CACHE_OFFSET(vert)]);
//            _pointer += 320;
			_pointer += 480;
        }
    }
}
