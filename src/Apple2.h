#include "memory.h"
#include "6502.h"
#include "iou.h"
#define GRMODE_MONO         0
#define GRMODE_COLOR        1
#define GRMODE_SONYHR_MONO  2
#define GRMODE_SONYHR_COLOR 3
#define GRMODE_OS5HR_MONO   4
#define GRMODE_OS5HR_COLOR  5
#define CLRMODE_INTERP      0
#define CLRMODE_STD         1
#define GRMODE_ISCOLOR(m)   ((m)&1)
#ifdef DEBUG
#define Apple2Run(c)        \
do {                        \
    static UInt8 *PC_6502,*prevPC_6502;\
    UInt16 i;               \
    UInt32 badop;           \
    MemSemaphoreReserve(true);\
    for(i=(c); i; i--) {    \
    prevPC_6502 = PC_6502;\
    PC_6502 = (UInt8*)state6502.LongRegs.PC;\
    badop = run6502(1);   \
    if (badop) {            \
        Char string[40];    \
        Apple2Pause(true);  \
        StrPrintF(string, "Bad opcode 0x%02x @ 0x%04x", (UInt16)badop, (UInt16)(badop >> 16)); \
        WinDrawChars(string, StrLen(string), 15, 140); \
        StrPrintF(string, "Prev opcode 0x%02x @ 0x%04x", (UInt16)*prevPC_6502, (UInt16)(prevPC_6502 - AppleMemory)); \
        WinDrawChars(string, StrLen(string), 15, 150);\
        break;              \
    }                       \
    }                       \
    MemSemaphoreRelease(true);\
} while(0)
#else
#define Apple2Run(c)        \
do {                        \
    UInt32 badop;           \
    MemSemaphoreReserve(true);\
    badop = run6502((c));   \
    MemSemaphoreRelease(true);\
    if (badop) {\
        Char string[40];    \
        if (!prefs.ignoreBadOps){ \
            Apple2Pause(true);  \
            StrPrintF(string, "Bad opcode 0x%02x @ 0x%04x", (UInt16)badop, (UInt16)(badop >> 16)); \
            WinDrawChars(string, StrLen(string), 15, 140); \
        }                   \
        state6502.LongRegs.PC++;\
    }                       \
} while(0)
#endif

#define Apple2UpdateVideo()             \
do {                                    \
    if (dirtyVideo){                    \
        if (dirtyVideo > 1)             \
            setVideoFuncs(false);       \
        updateVideo();                  \
     }                                  \
} while(0)

#if CLIE_SOUND
#define Apple2UpdateAudio()             \
do {                                    \
    if (AppleVolume) {                  \
    if (refPa1) {                       \
        static UInt32 lastFreq=0;       \
    	Boolean retval;                 \
        if (spkrIOU) {                  \
            lastFreq=spkrIOU >> 4;      \
        	PA1L_midiNoteOn(refPa1,hMidi,0,lastFreq,127,&retval);\
            spkrIOU=0;                  \
        }                               \
        else if (lastFreq) {            \
    	    PA1L_midiNoteOff(refPa1,hMidi,0,lastFreq,1,&retval);\
            lastFreq=0;                 \
        }                               \
    }                                   \
    else if (spkrIOU) {                 \
        SndCommandType sndCmd;          \
        sndCmd.cmd    = sndCmdFreqDurationAmp;\
        sndCmd.param1 = ((UInt32)spkrIOU)/*Hz*/;\
        sndCmd.param2 = 20/*msec*/;     \
        sndCmd.param3 = AppleVolume;    \
        SndDoCmd(0, &sndCmd, true/*noWait*/);\
        spkrIOU = 0;                    \
    }                                   \
    }                                   \
} while(0)
#else
#define Apple2UpdateAudio()             \
do {                                    \
    if (AppleVolume) {                  \
        static UInt32 lastFreq=0;       \
        SndCommandType sndCmd;          \
        if (spkrIOU) {                  \
            lastFreq=spkrIOU*A2Hz/prefs.refreshRate;\
            sndCmd.cmd    = sndCmdFrqOn;\
            sndCmd.param1 = lastFreq/*Hz*/;\
            sndCmd.param2 = 20/*msec*/; \
            sndCmd.param3 = AppleVolume;\
            SndDoCmd(0, &sndCmd, true/*noWait*/);\
            spkrIOU=0;                  \
        }                               \
        else if (lastFreq) {            \
            sndCmd.cmd    = sndCmdQuiet;\
            sndCmd.param1 = 0/*Hz*/;    \
            sndCmd.param2 = 0/*msec*/;  \
            sndCmd.param3 = AppleVolume;\
            SndDoCmd(0, &sndCmd, true/*noWait*/);\
            lastFreq=0;                 \
        }                               \
    }                                   \
} while(0)
#endif

#define Apple2PutKey(key)                           \
do {                                                \
    UInt8 keyCode;                                  \
    if (kbdCount < KBD_BUF_SIZE) {                  \
        if (keyCtrlMod && TxtCharIsAlpha(key)) {    \
            keyCode = 0x80 | (TxtCharIsLower(key) ? ((key) - 'a' + 1) : ((key) - 'A' + 1));\
            if (keyCode == 0x83) kbdCount = 0;      \
        } else if (prefs.capsLock && TxtCharIsAlpha(key) && TxtCharIsLower(key)) {\
            keyCode = 0x80 | (key) - 'a' + 'A';     \
        } else {                                    \
            keyCode = 0x80 | (key);                 \
        }                                           \
        if (kbdCount++)                             \
            kbdBuffer[(kbdHead + kbdCount - 1)      \
                     & KBD_BUF_MASK] = keyCode;     \
        else                                        \
            kbdIOU =  keyCode;                      \
    }                                               \
    keyCtrlMod = false;                             \
} while (0)
#define Apple2PutKeyNonAlpha(key)                   \
do {                                                \
    UInt8 keyCode;                                  \
    if (kbdCount < KBD_BUF_SIZE) {                  \
        keyCode = 0x80 | (key);                     \
        if (kbdCount++)                             \
            kbdBuffer[(kbdHead + kbdCount - 1)      \
                     & KBD_BUF_MASK] = keyCode;     \
        else                                        \
            kbdIOU =  keyCode;                      \
    }                                               \
    keyCtrlMod = false;                             \
} while (0)

#define Apple2GetKey()  (kbdIOU = ((kbdCount) ? kbdBuffer[(kbdHead + --kbdCount) & KBD_BUF_MASK] : kbdIOU & 0x7F))
extern UInt32 AppleInstrCount;
extern UInt32 AppleInstrInc;
/*
 * Preferences.
 */
#define PREFS_VERSION       0x00000013L
struct _prefs_t
{
    UInt32  version;
    UInt32  state6502_ACC;
    UInt32  state6502_X;
    UInt32  state6502_Y;
    UInt32  state6502_SP_CC;
    UInt32  state6502_PC;
    UInt8   status6502;
    UInt16  memIOU;
    UInt16  vidIOU;
    Char    currentDsk[2][32];
    Char    writeEnable[2];
    UInt16  currentTrack[2];
    UInt16  currentPos[2];
    UInt8   currentDrive;
    Boolean currentMotor;
    Boolean ignoreBadOps;
    Boolean muteSound;
    Boolean enable80Col;
    Boolean capsLock;
    UInt8   refreshRate;
    UInt32  keyHardMask;
    UInt16  keyHard1;
    UInt16  keyHard2;
    UInt16  keyHard3;
    UInt16  keyHard4;
    UInt8   centerJoystickHorizPos;
    UInt8   centerJoystickVertPos;
    UInt8   centerJoystickRate;
    UInt8   moveJoystickRate;
};
extern struct _prefs_t prefs;

