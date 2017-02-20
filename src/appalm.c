#include <PalmOS.h>
#include <SonyCLIE.h>
//#define keyBitGameExt0      0x00010000L // SONY GameController Extension bit0
//#define keyBitGameExt1      0x00020000L // SONY GameController Extension bit1
#define KEY_MASK_ALL        (~(keyBitPageUp | keyBitPageDown | keyBitHard1 | keyBitHard2 | keyBitHard3 | keyBitHard4 | keyBitGameExt0 | keyBitGameExt1))
#define KEY_MASK_BTTNS      (~(keyBitHard1 | keyBitHard4 | keyBitGameExt0 | keyBitGameExt1))
#include "Apple2.h"
#include "appalm_rsc.h"
#define ROM_VERSION_2_0     0x02000000L
#define ROM_VERSION_3_1     0x03100000L
#define ROM_VERSION_3_5     0x03500000L
#ifdef CLIE_SOUND
#include "Pa1Lib.h"
#define sonySysFtrNumSystemBase 10000
#define sonySysFtrNumSystemAOutSndStateOnHandlerP (sonySysFtrNumSystemBase + 4)
#define sonySysFtrNumSystemAOutSndStateOffHandlerP (sonySysFtrNumSystemBase + 5)
#define aOutSndKindSp (0) /* speaker */
#define aOutSndKindHp (2) /* headphone */
#define ADPCM_MODE_NONCONTINUOUS_PB 0x01
#define ADPCM_MODE_INTERRUPT_MODE 0x02
#define ADPCM_4_KHZ 0
#define ADPCM_8_KHZ 1
#define appStopSoundEvent firstUserEvent
typedef void (*sndStateOnType)(UInt8 /* kind */,
                               UInt8 /* left volume 0-31 */,
                               UInt8 /* right volume 0-31 */);
typedef void (*sndStateOffType)(UInt8 /* kind */);
#endif
/*
 * Globals.
 */
struct _prefs_t prefs;
UInt32  romVersion;
UInt16  grMode;
UInt16  refHR, refSnd, refPa1;
UInt8   hMidi;
UInt16  AppleVolume, A2Hz;
UInt32  jogBits;
UInt16 **jogAstMaskP;
UInt16 *jogAstOldMask;
UInt16  jogAstMask[2];
Char    Title[50];
Boolean pause = false;
Boolean keyCtrlMod = false;
UInt8   LoadDiskDrive;
UInt32  AppleInstrCount;
UInt32  AppleInstrInc;
static void Apple2Pause(Boolean);

/*****************************************************************************\
*                                                                             *
*                             Utility Routines                                *
*                                                                             *
\*****************************************************************************/

/*
 * ROM version check.
 */
static Err RomVersionCompatible(UInt32 requiredVersion, UInt16 launchFlags)
{
    UInt32 localRomVersion;
    FtrGet(sysFtrCreator, sysFtrNumROMVersion, &localRomVersion);
    if (sysAppLaunchCmdNormalLaunch)
        romVersion = localRomVersion;
    if (localRomVersion < requiredVersion)
    {
        if ((launchFlags & (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) == (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp))
        {
            FrmAlert(RomIncompatibleAlert);
            /*
             * Pilot 1.0 will continuously relaunch this app unless we switch to
             * another safe one.  The sysFileCDefaultApp is considered "safe".
             */
            if (localRomVersion < ROM_VERSION_2_0)
                AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
        }
        return (sysErrRomIncompatible);
    }
    return (0);
}
/*
 * Sony HIRES routines.
 */
void GrSetMode(void)
{
    Err    error;
    UInt32 width, height, depth;
    Boolean colorMode;

    switch (grMode)
    {
        case GRMODE_MONO:
            depth = 2; width = height = 160;
            WinScreenMode(winScreenModeSet, &width, &height, &depth, NULL);
            break;
        case GRMODE_COLOR:
            depth = 8; width = height = 160;
            WinScreenMode(winScreenModeSet, &width, &height, &depth, NULL);
            break;
//        case GRMODE_SONYHR_MONO:
        case GRMODE_SONYHR_COLOR:
            error = HROpen(refHR);
            if (!error)
            {
                width = hrWidth; height = hrHeight; depth = 8;//(grMode == GRMODE_SONYHR_COLOR) ? 8 : 1;
                HRWinScreenMode(refHR, winScreenModeSet, &width, &height, &depth, NULL);
            }
            else
            {
                WinScreenMode(winScreenModeSet, NULL, NULL, &depth, NULL);
                grMode = GRMODE_MONO;
            }
            break;
//        case GRMODE_OS5HR_MONO:
//            depth = 1; width = 320;
//            WinScreenMode(winScreenModeSet, &width, NULL, &depth, NULL);
//            break;
        case GRMODE_OS5HR_COLOR:
            depth = 8; width = 320; colorMode = true;
            WinScreenMode(winScreenModeSet, &width, NULL, &depth, NULL/*&colorMode*/);
            break;
    }
}
void GrSetDefaultMode(void)
{
    switch (grMode)
    {
        case GRMODE_MONO:
        case GRMODE_COLOR:
            WinScreenMode(winScreenModeSetToDefaults, NULL, NULL, NULL, NULL);
            break;
        case GRMODE_SONYHR_MONO:
        case GRMODE_SONYHR_COLOR:
            HRWinScreenMode (refHR, winScreenModeSetToDefaults, NULL, NULL, NULL, NULL);
            HRClose(refHR);
            break;
        case GRMODE_OS5HR_MONO:
        case GRMODE_OS5HR_COLOR:
            WinScreenMode(winScreenModeSetToDefaults, NULL, NULL, NULL, NULL);
            break;
    }
}
#ifdef CLIE_SOUND
static void SonySetVolume(UInt16 pa1LibRefNum, UInt8 volume)
{
    Boolean successful;

    // the sndState function pointers must be called to initialize
    // sound playback properly
    if (volume)
    {
        sndStateOnType sndStateOnFuncP;
        if (FtrGet(sonySysFileCSystem, sonySysFtrNumSystemAOutSndStateOnHandlerP, (UInt32*) &sndStateOnFuncP) == errNone)
        {
            sndStateOnFuncP(aOutSndKindSp, volume, volume);
            sndStateOnFuncP(aOutSndKindHp, volume, volume);
        }
        PA1L_devSpVolume(pa1LibRefNum, volume, &successful);
        PA1L_devHpVolume(pa1LibRefNum, volume, volume, &successful);
    }
    else
    {
        sndStateOffType sndStateOffFuncP;
        if (FtrGet(sonySysFileCSystem, sonySysFtrNumSystemAOutSndStateOffHandlerP, (UInt32*) &sndStateOffFuncP) == errNone)
        {
            sndStateOffFuncP(aOutSndKindSp);
            sndStateOffFuncP(aOutSndKindHp);
        }
    }
}
#endif
/*
 * Drive gadgets.
 */
void showDiskName(UInt16 drive_no, Char *diskname)
{
    RectangleType rect;

    WinSetDrawWindow(WinGetDisplayWindow());
    if (grMode > GRMODE_COLOR)
    {
        rect.topLeft.x = (drive_no == 1 ? 0 : 80) + 2;
        rect.topLeft.y = 125;
        rect.extent.x  = 76;
        rect.extent.y  = 11;
        WinSetForeColor(0);
        WinSetTextColor(0);
        WinSetBackColor(216);
    }
    else
    {
        rect.topLeft.x = 80;
        rect.topLeft.y = 0;
        rect.extent.x  = 80;
        rect.extent.y  = 10;
    }
    WinSetClip(&rect);
    WinEraseRectangle(&rect, 0);
    WinDrawChars(diskname, StrLen(diskname), rect.topLeft.x, rect.topLeft.y);
    WinResetClip();
}
void showDriveState(UInt16 drive_no, Boolean motor_on)
{
    static UInt16 spin = 0;
    RectangleType rect;

    WinSetDrawWindow(WinGetDisplayWindow());
    if (grMode > GRMODE_COLOR)
    {
        rect.topLeft.x = drive_no == 1 ? 70 : 150;
        rect.topLeft.y = 150;
        rect.extent.x  = 5;
        rect.extent.y  = 5;
        WinSetForeColor(motor_on ? 125 : 197);
        WinDrawRectangle(&rect, 0);
    }
    else
    {
        if (motor_on)
            WinDrawChars(spin++ & 1 ? "+" : "x", 1, 70, 0);
        else
            WinDrawChars("   ", 3, 70, 0);
    }
}
void showDrive(UInt16 id, RectangleType *gadgetRect)
{
    RectangleType rect;

    WinSetDrawWindow(WinGetDisplayWindow());
    WinSetForeColor(0);
    WinSetTextColor(0);
    WinSetBackColor(216);
    WinEraseRectangle(gadgetRect, 0);
    WinDrawRectangleFrame(dialogFrame, gadgetRect);
    rect.extent.x  = gadgetRect->extent.x - 10;
    rect.extent.y  = 2;
    rect.topLeft.x = gadgetRect->topLeft.x + (gadgetRect->extent.x - rect.extent.x) / 2;
    rect.topLeft.y = gadgetRect->topLeft.y + (gadgetRect->extent.y - rect.extent.y) / 2;
    WinDrawRectangleFrame(rectangleFrame, &rect);
    rect.extent.x  = 20;
    rect.extent.y  = 8;
    rect.topLeft.x = gadgetRect->topLeft.x + (gadgetRect->extent.x - rect.extent.x) / 2;
    rect.topLeft.y = gadgetRect->topLeft.y + (gadgetRect->extent.y - rect.extent.y) / 2;
    WinEraseRectangle(&rect, 0);
    WinDrawRectangleFrame(rectangleFrame, &rect);
    WinDrawChars(id == gadgetDrive1 ? "Drive 1" : "Drive 2", 7, gadgetRect->topLeft.x + 2, gadgetRect->topLeft.y + 23);
}
Boolean DriveHandleEvent(struct FormGadgetTypeInCallback *gadget, UInt16 cmd, void *param)
{
    static UInt16 penX, penY;
    Boolean handled = false;

    if (cmd == formGadgetDrawCmd)
    {
        showDrive(gadget->id, &(gadget->rect));
        gadget->attr.usable   = true;
        gadget->attr.extended = true;
        gadget->attr.visible  = true;
        handled = true;
    }
    else if (cmd == formGadgetHandleEventCmd)
    {
#if 0
        EventType *event = param;
        WinSetDrawWindow(WinGetDisplayWindow());
        WinInvertRectangle(&(gadget->rect), 0);
        switch (event->eType)
        {
            case frmUpdateEvent:
                showDrive(gadget->id, &(gadget->rect));
                handled = true;
                break;
            case penDownEvent:
                penX = event->screenX;
                penY = event->screenY;
                handled = true;
                break;
            case penMoveEvent:
                penX = ~0;
                penY = ~0;
                handled = true;
                break;
            case penUpEvent:
                if ((penX == event->screenX) && (penY == event->screenY))
                {
                    LoadDiskDrive = (gadget->id == gadgetDrive1) ? 1 : 2;
                    FrmPopupForm(LoadDiskForm);
                }
                handled = true;
                break;
        }
#else
        LoadDiskDrive = (gadget->id == gadgetDrive1) ? 1 : 2;
        FrmPopupForm(LoadDiskForm);
        Apple2Pause(true);
#endif
    }
    return (handled);
}

/*****************************************************************************\
*                                                                             *
*                               Forms                                         *
*                                                                             *
\*****************************************************************************/

/*
 * Menu.
 */
static void Apple2Pause(Boolean Pause)
{
    pause = Pause;
    FrmSetTitle(FrmGetActiveForm(), pause ? "(paused)" : Title);
    WinSetDrawWindow(WinGetDisplayWindow());
}
static void Apple2MenuSetItems(void)
{
    if (prefs.ignoreBadOps)
        MenuHideItem(menuIgnoreBadOps);
    if (grMode > GRMODE_COLOR)
    {
        if (prefs.enable80Col)
        {
            MenuHideItem(menuEnable80Col);
            MenuShowItem(menuDisable80Col);
        }
        else
        {
            MenuHideItem(menuDisable80Col);
            MenuShowItem(menuEnable80Col);
        }
    }
    else
    {
        MenuHideItem(menuDisable80Col);
        MenuHideItem(menuEnable80Col);
    }
    if (prefs.capsLock)
    {
        MenuHideItem(menuCapsLock);
        MenuShowItem(menuCapsUnlock);
    }
    else
    {
        MenuHideItem(menuCapsUnlock);
        MenuShowItem(menuCapsLock);
    }
    if (prefs.muteSound)
    {
        MenuHideItem(menuMuteSound);
        MenuShowItem(menuUnmuteSound);
    }
    else
    {
        MenuHideItem(menuUnmuteSound);
        MenuShowItem(menuMuteSound);
    }
    if (prefs.keyHardMask == KEY_MASK_ALL)
    {
        MenuHideItem(menuEnableJoystick);
        MenuShowItem(menuDisableJoystick);
    }
    else
    {
        MenuHideItem(menuDisableJoystick);
        MenuShowItem(menuEnableJoystick);
    }
    if (DmFindDatabase(0, "A2 Manager"))
        MenuShowItem(menuA2Manager);
    else
        MenuHideItem(menuA2Manager);
}
static Boolean Apple2MenuHandleEvent(UInt16 menuID)
{
    Char string[20];
    Boolean pause = false;

    switch (menuID)
    {
        case menuLoadDisk1:
            LoadDiskDrive = 1;
            FrmPopupForm(LoadDiskForm);
            pause = true;
            break;
        case menuLoadDisk2:
            LoadDiskDrive = 2;
            FrmPopupForm(LoadDiskForm);
            pause = true;
            break;
        case menuA2Manager:
            {
                UInt32 dbid;
                if ((dbid = DmFindDatabase(0, "A2 Manager")))
                    SysUIAppSwitch(0, dbid, sysAppLaunchCmdNormalLaunch, NULL);
                else
                    FrmCustomAlert(MessageBox, "Appalm ][ Manager not available", NULL, NULL);
            }
            break;
        case menuESC:
            kbdCount = 0;
            Apple2PutKeyNonAlpha(27);
            break;
        case menuCTRL:
            keyCtrlMod = true;
            break;
        case menuCapsLock:
        case menuCapsUnlock:
            prefs.capsLock = !prefs.capsLock;
            Apple2MenuSetItems();
            break;
        case menuFlushKbdBuf:
            kbdCount = 0;
            kbdIOU &= 0x7F;
            break;
        case menuDisableJoystick:
            prefs.keyHardMask = KEY_MASK_BTTNS;
            KeySetMask(prefs.keyHardMask);
            Apple2MenuSetItems();
            break;
        case menuEnableJoystick:
            prefs.keyHardMask = KEY_MASK_ALL;
            KeySetMask(prefs.keyHardMask);
            Apple2MenuSetItems();
            break;
        case menuJoystickSettings:
            FrmPopupForm(JoystickSettingsForm);
            pause = true;
            break;
        case menuIncRefresh:
            if (prefs.refreshRate > 1)
                prefs.refreshRate--;
            else
                prefs.refreshRate = 1;
            AppleInstrInc = prefs.refreshRate * 5000;
            break;
        case menuDecRefresh:
            AppleInstrInc = ++prefs.refreshRate * 5000;
            break;
        case menuDisable80Col:
        case menuEnable80Col:
            prefs.enable80Col = !prefs.enable80Col;
            Apple2MenuSetItems();
            break;
        case menuMuteSound:
            prefs.muteSound = true;
            AppleVolume     = 0;
            Apple2MenuSetItems();
            break;
        case menuUnmuteSound:
            prefs.muteSound = false;
            spkrIOU         = 0;
            AppleVolume     = PrefGetPreference(prefGameSoundVolume);
            Apple2MenuSetItems();
            break;
        case menuUnpause:
            break;
        case menuPause:
            Apple2Pause(true);
            pause = true;
            break;
        case menuReset:
            MemSemaphoreReserve(true);
            initMemory();
            initVideo();
            resetDisks();
            reset6502(AppleMemory, WriteFunction, ReadFunction);
            MemSemaphoreRelease(true);
            break;
        case menuIgnoreBadOps:
            prefs.ignoreBadOps = true;
            Apple2MenuSetItems();
            break;
        case menuAbout:
            Apple2Pause(true);
            FrmAlert(AboutBox);
            break;
    }
    Apple2Pause(pause);
    return (true);
}
/*
 * Main form.
 */
static Boolean Apple2HandleEvent(EventType *event)
{
    static FormType *frmApple2;
    RectangleType rect;
    Boolean handled = false;
    switch (event->eType)
    {
        case keyDownEvent:
            if (event->data.keyDown.modifiers & controlKeyMask)
                keyCtrlMod = true;
            switch (event->data.keyDown.chr)
            {
                case vchrJogPushedUp:
                    jogBits = keyBitHard1;
                case vchrJogUp:
                    pdlResetIOU[0] += prefs.moveJoystickRate;
                    if (pdlResetIOU[0] > 0xFF)
                        pdlResetIOU[0] = 0xFF;
                    handled = true;
                    break;
                case vchrJogPushedDown:
                    jogBits = keyBitHard1;
                case vchrJogDown:
                    pdlResetIOU[0] -= prefs.moveJoystickRate;
                    if (pdlResetIOU[0] > 0xFF)
                        pdlResetIOU[0] = 0x00;
                    handled = true;
                    break;
                case vchrJogPush:
                    jogBits = keyBitHard1;
                    handled = true;
                    break;
                case vchrJogRelease:
                    jogBits = 0;
                    handled = true;
                    break;
                case vchrJogPushRepeat:
                    jogBits ^= keyBitHard1;
                    handled = true;
                    break;
                case vchrJogBack:
                    event->data.keyDown.chr = 'P';
                    break;
                case /*vchrFind*/ 0x02F5: // CTRL-C
                    kbdCount = 0;
                    event->data.keyDown.chr = 3;
                    break;
                case /*vchrCalc*/ 0x02F6: // ESC
                    kbdCount = 0;
                    event->data.keyDown.chr = 27;
                    break;
                case chrLeftArrow:
                case /*vchrHard2*/ 0x02F2:
                    event->data.keyDown.chr = 136;
                    break;
                case chrRightArrow:
                case /*vchrHard3*/ 0x02F3:
                    event->data.keyDown.chr = 149;
                    break;
                case chrUpArrow:
                case vchrPageUp:
                    event->data.keyDown.chr = 139;
                    break;
                case chrDownArrow:
                case vchrPageDown:
                    event->data.keyDown.chr = 138;
                    break;
                case 10: // RETURN
                    event->data.keyDown.chr = 13;
                    break;
            }
            if (!handled)
                Apple2PutKey(event->data.keyDown.chr);
            handled = true;
            break;
        case frmOpenEvent:
            frmApple2 = FrmGetActiveForm();
            if (grMode > GRMODE_COLOR)
            {
                /*
                 * Create gadgets for disk drive images.
                 */
                FrmNewGadget(&frmApple2, gadgetDrive1,  1, 125, 78, 34);
                FrmNewGadget(&frmApple2, gadgetDrive2, 81, 125, 78, 34);
                FrmSetGadgetHandler(frmApple2, FrmGetObjectIndex(frmApple2, gadgetDrive1), DriveHandleEvent);
                FrmSetGadgetHandler(frmApple2, FrmGetObjectIndex(frmApple2, gadgetDrive2), DriveHandleEvent);
            }
            FrmDrawForm(frmApple2);
            mountDisk(1, prefs.currentDsk[0][0] ? prefs.currentDsk[0] : NULL, prefs.writeEnable[0]);
            positionDisk(1, prefs.currentTrack[0], prefs.currentPos[0]);
            mountDisk(2, prefs.currentDsk[1][0] ? prefs.currentDsk[1] : NULL, prefs.writeEnable[1]);
            positionDisk(2, prefs.currentTrack[1], prefs.currentPos[1]);
            setCurrentDisk(prefs.currentDrive ? prefs.currentDrive : 1);
            toggleMotor(prefs.currentMotor);
            pause = false;
            handled = true;
            break;
        case frmCloseEvent:
            if (grMode > GRMODE_COLOR)
            {
                /*
                 * Remove gadgets for disk drive images.
                 */
                FrmRemoveObject(&frmApple2, FrmGetObjectIndex(frmApple2, gadgetDrive2));
                FrmRemoveObject(&frmApple2, FrmGetObjectIndex(frmApple2, gadgetDrive1));
            }
            break;
        case menuEvent:
            handled = Apple2MenuHandleEvent(event->data.menu.itemID);
            break;
        case menuOpenEvent:
            Apple2MenuSetItems();
            break;
    }
    return (handled);
}
/*
 * Form UI elements helper routines.
 */
static void *GetObjectPtr(UInt16 objectID)
{
    FormType *form = FrmGetActiveForm();
    return (FrmGetObjectPtr(form, FrmGetObjectIndex(form, objectID)));
}
static void FieldSetValue(FieldType *field, UInt16 value)
{
    Char        *mem;
    MemHandle    hMem = FldGetTextHandle(field);
    FldSetTextHandle(field, NULL);
    if (hMem)
        MemHandleFree(hMem);
    hMem = MemHandleNew(8);
    mem  = MemHandleLock(hMem);
    StrPrintF(mem, "%d", value);
    MemHandleUnlock(hMem);
    FldSetTextHandle(field, hMem);
}
static void FieldSpin(FieldType *field, Int16 spin, Int16 min, Int16 max)
{
    MemHandle    hMem;
    Char        *mem;
    Int16       tmp;

    hMem = FldGetTextHandle(field);
    FldSetTextHandle(field, NULL);
    mem  = MemHandleLock(hMem);
    tmp  = StrAToI(mem);
    if (spin > 0)
    {
        if ((Int32)(tmp + spin) > (Int32)max)
            tmp = max;
        else
            tmp += spin;
    }
    else
    {
        if ((Int32)(tmp + spin) < (Int32)min)
            tmp = min;
        else
            tmp += spin;
    }
    StrPrintF(mem, "%d", tmp);
    MemHandleUnlock(hMem);
    FldSetTextHandle(field, hMem);
    FldDrawField(field);
}
/*
 * Load disk image.
 */
static Boolean LoadDiskHandleEvent(EventType *event)
{
    static Int16      numDsks;
    static Char     **nameDsks;
    Int16             i, cardNo, otherDiskDrive;
    UInt32            dbType;
    DmSearchStateType searchState;
    LocalID           dbID;
    Char              name[33];
    Boolean           first, handled = false;
    switch (event->eType)
    {
        case frmOpenEvent:
            if (jogAstMaskP)
                *jogAstMaskP = jogAstOldMask;
            /*
             * Scan databases for disk images.
             */
            otherDiskDrive = (LoadDiskDrive - 1) ^ 1;
            first = true;
            numDsks = 1;
            while (!DmGetNextDatabaseByTypeCreator(first, &searchState, NULL, 'Apl2', false, &cardNo, &dbID))
            {
                first = false;
                DmDatabaseInfo(cardNo, dbID, name, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &dbType, NULL);
                if (dbType == 'RDSK' || dbType == 'DDSK')
                    if (StrCompare(name, prefs.currentDsk[otherDiskDrive]))
                        numDsks++;
            }
            first = true;
            nameDsks = MemPtrNew(numDsks * sizeof(Char *));
            nameDsks[0] = MemPtrNew(8);
            StrCopy(nameDsks[0], "(none)");
            numDsks = 1;
            while (!DmGetNextDatabaseByTypeCreator(first, &searchState, NULL, 'Apl2', false, &cardNo, &dbID))
            {
                first = false;
                DmDatabaseInfo(cardNo, dbID, name, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &dbType, NULL);
                if (dbType == 'RDSK' || dbType == 'DDSK')
                {
                    if (StrCompare(name, prefs.currentDsk[otherDiskDrive]))
                    {
                        nameDsks[numDsks] = MemPtrNew(32);
                        StrCopy(nameDsks[numDsks], name);
                        numDsks++;
                    }
                }
            }
            LstSetListChoices(GetObjectPtr(lstDsk), nameDsks, numDsks);
            FrmSetTitle(FrmGetActiveForm(), LoadDiskDrive == 1 ? "Load Disk 1 Image" : "Load Disk 2 Image");
            FrmDrawForm(FrmGetActiveForm());
            handled = true;
            break;
        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID)
            {
                case buttonOk:
                    if ((i = LstGetSelection(GetObjectPtr(lstDsk))) != 0)
                        StrCopy(prefs.currentDsk[LoadDiskDrive - 1], LstGetSelectionText(GetObjectPtr(lstDsk), i));
                    else
                        prefs.currentDsk[LoadDiskDrive - 1][0] = '\0';
                    MemSemaphoreReserve(true);
                    if (CtlGetValue(GetObjectPtr(checkReset)))
                    {
                        initMemory();
                        initVideo();
                        reset6502(AppleMemory, WriteFunction, ReadFunction);
                    }
                    prefs.writeEnable[LoadDiskDrive - 1] = CtlGetValue(GetObjectPtr(checkWriteEnable));
                    MemSemaphoreRelease(true);
                case buttonCancel:
                    while(numDsks)
                        MemPtrFree(nameDsks[--numDsks]);
                    MemPtrFree(nameDsks);
                    FrmReturnToForm(0);
                    if (event->data.ctlSelect.controlID == buttonOk)
                    {
                        MemSemaphoreReserve(true);
                        if (!mountDisk(LoadDiskDrive, prefs.currentDsk[LoadDiskDrive - 1][0] ? prefs.currentDsk[LoadDiskDrive - 1] : NULL, prefs.writeEnable[LoadDiskDrive - 1]))
                            prefs.currentDsk[LoadDiskDrive - 1][0] = '\0';
                        MemSemaphoreRelease(true);
                    }
                    handled = true;
                    Apple2Pause(false);
                    if (jogAstMaskP)
                        *jogAstMaskP = jogAstMask;
                    break;
            }
            break;
    }
    return (handled);
}
/*
 * Joystick settings.
 */
static Boolean JoystickSettingsHandleEvent(EventType *event)
{
    Boolean handled = false;
    switch (event->eType)
    {
        case frmOpenEvent:
            if (jogAstMaskP)
                *jogAstMaskP = jogAstOldMask;
            CtlSetValue(GetObjectPtr(checkAutoCenter), prefs.centerJoystickRate);
            CtlSetValue(GetObjectPtr(checkSwapBttns),  prefs.keyHard1);
            FieldSetValue(GetObjectPtr(fieldMoveRate), prefs.moveJoystickRate);
            FieldSetValue(GetObjectPtr(fieldYCenter),  prefs.centerJoystickVertPos - 128);
            FieldSetValue(GetObjectPtr(fieldXCenter),  prefs.centerJoystickHorizPos - 128);
            FrmDrawForm(FrmGetActiveForm());
            handled = true;
            break;
        case ctlRepeatEvent:
            switch (event->data.ctlSelect.controlID)
            {
                case buttonIncRate:
                case buttonDecRate:
                    FieldSpin(GetObjectPtr(fieldMoveRate), event->data.ctlSelect.controlID == buttonIncRate ? 5 : -5, 1, 128);
                    break;
                case buttonIncYCenter:
                case buttonDecYCenter:
                    FieldSpin(GetObjectPtr(fieldYCenter), event->data.ctlSelect.controlID == buttonIncYCenter ? 4 : -4, -120, 120);
                    break;
                case buttonIncXCenter:
                case buttonDecXCenter:
                    FieldSpin(GetObjectPtr(fieldXCenter), event->data.ctlSelect.controlID == buttonIncXCenter ? 4 : -4, -120, 120);
                    break;
            }
            break;
        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID)
            {
                case buttonOk:
                    prefs.centerJoystickRate     = CtlGetValue(GetObjectPtr(checkAutoCenter))  ? 0x10 : 0x00;
                    prefs.keyHard1               = CtlGetValue(GetObjectPtr(checkSwapBttns)) ? 1 : 0;
                    prefs.keyHard4               = 1 - prefs.keyHard1;
                    prefs.moveJoystickRate       = StrAToI(FldGetTextPtr(GetObjectPtr(fieldMoveRate)));
                    prefs.centerJoystickVertPos  = 128 + StrAToI(FldGetTextPtr(GetObjectPtr(fieldYCenter)));
                    prefs.centerJoystickHorizPos = 128 + StrAToI(FldGetTextPtr(GetObjectPtr(fieldXCenter)));
                case buttonCancel:
                    FrmReturnToForm(0);
                    handled = true;
                    Apple2Pause(false);
                    if (jogAstMaskP)
                        *jogAstMaskP = jogAstMask;
                    break;
            }
            break;
    }
    return (handled);
}

/*****************************************************************************\
*                                                                             *
*                   Application start/stop/main routines                      *
*                                                                             *
\*****************************************************************************/

static Boolean AppStart(void)
{
    UInt16             prefSize;
    Err                error;
    SonySysFtrSysInfoP sonySysFtrSysInfoP;
    UInt32             version, depths, screen_width;
    Boolean            reset = false;
    /*
     * Get sound volume.
     */
    AppleVolume = PrefGetPreference(prefGameSoundVolume);
    A2Hz        = 50;
    refSnd      = refPa1 = 0;
    jogAstMaskP = NULL;
    /*
     * Check for audio and HiRes capability.
     */
    WinScreenMode(winScreenModeGetSupportedDepths, NULL, NULL, &depths, NULL);
    grMode = (depths & 0x80) ? GRMODE_COLOR : GRMODE_MONO;
    if (!(error = FtrGet(sonySysFtrCreator, sonySysFtrNumSysInfoP, (UInt32*)&sonySysFtrSysInfoP)))
    {
        if (sonySysFtrSysInfoP->libr & sonySysFtrSysInfoLibrHR)
        {
            if ((error = SysLibFind(sonySysLibNameHR, &refHR)))
            {
                if (error == sysErrLibNotFound)
                    error = SysLibLoad('libr', sonySysFileCHRLib, &refHR);
            }
            grMode = GRMODE_SONYHR_COLOR;
        }
#ifdef CLIE_SOUND
        if (AppleVolume)
        {
            if (sonySysFtrSysInfoP->libr & sonySysFtrSysInfoLibrFm)
            {
                if ((error = SysLibFind(sonySysLibNameSound, &refSnd)))
                {
                    if (error == sysErrLibNotFound)
                        error = SysLibLoad('libr', sonySysFileCSoundLib, &refSnd);
                }
            }
            if ((error = SysLibFind(PA1Lib_NAME, &refPa1)))
                error = SysLibLoad('libr', PA1Lib_ID, &refPa1);
            if (!error)
            {
                if ((error = PA1LibOpen(refPa1)))
                    refPa1 = 0;
            }
            if (refPa1)
            {
                Boolean retval;
                PA1L_midiOpen(refPa1, NULL, &hMidi, &retval);
                SonySetVolume(refPa1, AppleVolume);
            }
        }
#endif
        /*
         * Mask off the JogAssist Handler.
         */
        if (!FtrGet(sonySysFtrCreator, sonySysFtrNumJogAstMaskP, (UInt32*)&jogAstMaskP))
        {
            jogAstMask[0] = 2;
            jogAstMask[1] = 0x00FF;
            jogAstOldMask = *jogAstMaskP;
            *jogAstMaskP  = jogAstMask;
        }
        A2Hz >>= 1; // Clie plays sound too fast
    }
    else
    {
        error = FtrGet(sysFtrCreator, sysFtrNumWinVersion, &version);
        if (version >= 4)
        {
            error = WinScreenGetAttribute(winScreenWidth, &screen_width);
            if (screen_width >= 320)
                grMode = GRMODE_OS5HR_COLOR;
            else
            {
                UInt16 density = 0;
                do
                {
                    error = WinGetSupportedDensity(&density);
                    if (!error)
                    {
                        if (density == kDensityDouble)
                            grMode = GRMODE_OS5HR_COLOR;
                    }
                    else
                        density = 0;
                } while (density != 0);
            }
        }
    }
    /*
     * Initialize globals.
     */
    StrCopy(Title, "Appalm ][");
    jogBits = 0;
    /*
     * Look for existing emulator memory.  Create if not found.
     */
    MemSemaphoreReserve(true);
    if (FtrGet('Apl2', 1, (UInt32 *)&AppleMemory) != 0)
    {
        FtrPtrNew('Apl2', 1, 65536, (void **)&AppleMemory);
        reset = true;
    }
    if (FtrGet('Apl2', 2, (UInt32 *)&AuxMemory) != 0)
    {
        FtrPtrNew('Apl2', 2, 16384+2048, (void **)&AuxMemory);
        reset = true;
    }
    MemSemaphoreRelease(true);
    ReadFunction = (READBYTE *)MemPtrNew(2048);
    MemSet(ReadFunction, 2048, 0);
    WriteFunction = (WRITEBYTE *)&(ReadFunction[256]);
    /*
     * Get preferences.
     */
    prefSize = sizeof(prefs);
    if (!PrefGetAppPreferences('Apl2', 0, &prefs, &prefSize, true) && (prefs.version == PREFS_VERSION))
    {
        /*
         * Copy saved state into emulator.
         */
        reset6502(AppleMemory, WriteFunction, ReadFunction);
        state6502.LongRegs.ACC   = prefs.state6502_ACC;
        state6502.LongRegs.X     = prefs.state6502_X;
        state6502.LongRegs.Y     = prefs.state6502_Y;
        state6502.LongRegs.SP_CC = prefs.state6502_SP_CC;
        state6502.LongRegs.PC    = prefs.state6502_PC + (UInt32)AppleMemory;
        status6502               = prefs.status6502;
        memIOU                   = prefs.memIOU;
        vidIOU                   = prefs.vidIOU;
        MemSemaphoreReserve(true);
        loadROM();
        setMemFuncs();
        MemSemaphoreRelease(true);
    }
    else
    {
        MemSet(&prefs, prefSize, 0);
        /*
         * Set default prefs.
         */
        prefs.centerJoystickRate     = 0;
        prefs.moveJoystickRate       = 16;
        prefs.centerJoystickHorizPos = 0x80;
        prefs.centerJoystickVertPos  = 0x80;
        prefs.keyHardMask            = KEY_MASK_ALL;
        prefs.keyHard1               = 0;
        prefs.keyHard4               = 1;
        prefs.refreshRate            = 2;
        prefs.enable80Col            = true;
        prefs.muteSound              = false;
        prefs.capsLock               = false;
        prefs.ignoreBadOps           = false;
        reset = true;
    }
    if (reset)
    {
        MemSemaphoreReserve(true);
        initMemory();
        reset6502(AppleMemory, WriteFunction, ReadFunction);
        MemSemaphoreRelease(true);
    }
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
    /*
     * Set up video.
     */
    GrSetMode();
    initVideo();
    /*
     * Check for muted audio.
     */
    if (prefs.muteSound)
        AppleVolume = 0;
    /*
     * Mask off the hard key events.
     */
    KeySetMask(prefs.keyHardMask);
    /*
     * Init the instruction count and increment.
     */
    AppleInstrCount = 0;
    AppleInstrInc   = prefs.refreshRate ? prefs.refreshRate * 5000 : 5000;
    /*
     * Don't start running yet.
     */
    pause = true;
    /*
     * Goto main form.
     */
    FrmGotoForm(Apple2Form);
    return (true);
}
static void AppStop(void)
{
    /*
     * Make sure default graphics mode is set.
     */
    GrSetDefaultMode();
#ifdef CLIE_SOUND
    if (refPa1)
    {
        UInt16 usecount;
        Boolean retval;

        SonySetVolume(refPa1, 0);
        PA1L_midiClose(refPa1, hMidi, &retval);
        PA1LibClose(refPa1, &usecount);
        if (usecount == 0)
            SysLibRemove(refPa1);
    }
#endif
    /*
     * Clear Jog Assist mask.
     */
    if (jogAstMaskP)
        *jogAstMaskP = jogAstOldMask;
    /*
     * Save preferences.
     */
    prefs.version         = PREFS_VERSION;
    prefs.state6502_ACC   = state6502.LongRegs.ACC;
    prefs.state6502_X     = state6502.LongRegs.X;
    prefs.state6502_Y     = state6502.LongRegs.Y;
    prefs.state6502_SP_CC = state6502.LongRegs.SP_CC;
    prefs.state6502_PC    = state6502.LongRegs.PC - state6502.LongRegs.MEMBASE;
    prefs.status6502      = status6502;
    prefs.memIOU          = memIOU;
    prefs.vidIOU          = vidIOU;
    unloadROM();
    MemSemaphoreReserve(true);
    queryDisk(1, &prefs.currentTrack[0], &prefs.currentPos[0]);
    queryDisk(2, &prefs.currentTrack[1], &prefs.currentPos[1]);
    prefs.currentMotor = getCurrentDrive() & 0x80 ? true : false;
    prefs.currentDrive = getCurrentDrive() & 0x0F;
    umountDisk(1);
    umountDisk(2);
    MemSemaphoreRelease(true);
    PrefSetAppPreferences('Apl2', 0, 0, &prefs, sizeof(prefs), true);
    /*
     * Release any resources.
     */
    MemPtrFree(ReadFunction);
    /*
     * Unmask off the hard key events.
     */
    KeySetMask(keyBitsAll);
}
static Boolean AppProcessEvent(EventType *event)
{
    UInt16    error;
    FormType *frm;

    /*
     * Hijack the hard keys.
     */
    if ((event->eType == keyDownEvent) && (event->data.keyDown.modifiers & commandKeyMask))
    {
        /*
         * Remap the silk-screen keys.
         */
        if ((event->data.keyDown.chr >= vchrFind) && (event->data.keyDown.chr <= vchrCalc))
        {
            event->data.keyDown.modifiers &= ~commandKeyMask;
            event->data.keyDown.chr = event->data.keyDown.chr - vchrFind + 0x02F5;
        }
        /*
         * Remap the hard keys.
         */
        else if ((event->data.keyDown.chr >= vchrHard1) && (event->data.keyDown.chr <= vchrHard4))
        {
            event->data.keyDown.modifiers &= ~commandKeyMask;
            event->data.keyDown.chr = event->data.keyDown.chr - vchrHard1 + 0x02F1;
        }
        /*
         * Capture menu bar open/close.
         */
        else if ((event->data.keyDown.chr == vchrMenu) && (FrmGetFormId(FrmGetActiveForm()) == Apple2Form))
        {
            Apple2Pause(true);
        }
    }
    if (!SysHandleEvent(event) && !MenuHandleEvent(0, event, &error))
    {
        /*
         * Application specific event.
         */
        if (event->eType == frmLoadEvent)
        {
            /*
             * Load the form resource specified in the event then activate the form.
             */
            frm = FrmInitForm(event->data.frmLoad.formID);
            FrmSetActiveForm(frm);
            /*
             * Set the event handler for the form.  The handler of the currently
             * active form is called by FrmDispatchEvent each time it receives an event.
             */
            switch (event->data.frmLoad.formID)
            {
                case Apple2Form:
                    FrmSetEventHandler(frm, Apple2HandleEvent);
                    break;
                case LoadDiskForm:
                    FrmSetEventHandler(frm, LoadDiskHandleEvent);
                    break;
                case JoystickSettingsForm:
                    FrmSetEventHandler(frm, JoystickSettingsHandleEvent);
                    break;
            }
        }
        else
            /*
             * Pass it on to the form handler.
             */
            FrmDispatchEvent(event);
    }
    return (event->eType != appStopEvent);
}
/*
 * Main.
 */
UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
    UInt16    error, offTimer;
    EventType event;
    Boolean   running;
    UInt32    hardKeys, prevHardKeys;

    if ((error = RomVersionCompatible(ROM_VERSION_3_5, launchFlags)))
        return(error);
    if (cmd == sysAppLaunchCmdNormalLaunch)
    {
        if (AppStart())
        {
            /*
             * Main event loop.
             */
            prevHardKeys = 0;
            offTimer = 500;
            running = true;
            while (running)
            {
                EvtGetEvent(&event, 0);
                if (event.eType == nilEvent && !pause)
                {
                    Apple2Run(AppleInstrInc);
                    AppleInstrCount += AppleInstrInc;
                    hardKeys = KeyCurrentState() | jogBits;
                    btnIOU[prefs.keyHard1] = hardKeys & keyBitHard1;
                    btnIOU[prefs.keyHard4] = hardKeys & keyBitHard4;
                    if (prefs.keyHardMask == KEY_MASK_ALL)
                    {
                        if (hardKeys & (keyBitPageUp | keyBitPageDown))
                        {
                            if ((hardKeys & (keyBitPageUp | keyBitPageDown)) == (keyBitPageUp | keyBitPageDown))
                            {
                                pdlResetIOU[1] = prefs.centerJoystickHorizPos;
                            }
                            else if (hardKeys & keyBitPageUp)
                            {
                                pdlResetIOU[1] -= prefs.moveJoystickRate;
                                if (pdlResetIOU[1] > 0xFF)
                                    pdlResetIOU[1] = 0x00;
                            }
                            else
                            {
                                pdlResetIOU[1] += prefs.moveJoystickRate;
                                if (pdlResetIOU[1] > 0xFF)
                                    pdlResetIOU[1] = 0xFF;
                            }
                        }
                        else if (prefs.centerJoystickRate)
                        {
                            pdlResetIOU[1] = prefs.centerJoystickVertPos;
                        }
                        if (hardKeys & (keyBitHard2 | keyBitHard3))
                        {
                            if ((hardKeys & (keyBitHard2 | keyBitHard3)) == (keyBitHard2 | keyBitHard3))
                            {
                                pdlResetIOU[0] = prefs.centerJoystickHorizPos;
                            }
                            else if (hardKeys & keyBitHard2)
                            {
                                pdlResetIOU[0] -= prefs.moveJoystickRate;
                                if (pdlResetIOU[0] > 0xFF)
                                    pdlResetIOU[0] = 0x00;
                            }
                            else
                            {
                                pdlResetIOU[0] += prefs.moveJoystickRate;
                                if (pdlResetIOU[0] > 0xFF)
                                    pdlResetIOU[0] = 0xFF;
                            }
                        }
                        else if (prefs.centerJoystickRate)
                        {
                            pdlResetIOU[0] = prefs.centerJoystickHorizPos;
                        }
                    }
                    if (hardKeys & (keyBitGameExt0 | keyBitGameExt1))
                    {
                        if ((hardKeys & keyBitGameExt0) && !(prevHardKeys & keyBitGameExt0))
                            Apple2PutKeyNonAlpha(136); // Left Arrow
                        if ((hardKeys & keyBitGameExt1) && !(prevHardKeys & keyBitGameExt1))
                            Apple2PutKeyNonAlpha(149); // Right Arrow
                    }
                    prevHardKeys = hardKeys;
                    Apple2UpdateVideo();
                    Apple2UpdateAudio();
                    if (--offTimer == 0)
                    {
                        offTimer = 500;
                        EvtResetAutoOffTimer();
                    }
                }
                else
                {
                    offTimer = 500;
                    running = AppProcessEvent(&event);
                }
            }
            AppStop();
        }
    }
    return(0);
}

