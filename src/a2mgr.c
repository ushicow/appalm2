#include <PalmOS.h>
#include "a2mgr_rsc.h"
/*
 * Globals.
 */

/*****************************************************************************\
*                                                                             *
*                             Utility Routines                                *
*                                                                             *
\*****************************************************************************/

/*****************************************************************************\
*                                                                             *
*                               Forms                                         *
*                                                                             *
\*****************************************************************************/

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
 * Dsk Image Manager Form.
 */
char currentDsk[2][32];
static Boolean DskManagerHandleEvent(EventType *event)
{
    static Int16      numDsks;
    static Char     **nameDsks;
    static EventType  stopEvent;
    Int16             i, cardNo, otherDiskDrive, LoadDiskDrive=1;
    UInt32            dbType;
    DmSearchStateType searchState;
    LocalID           dbID;
    Char              name[33];
    Boolean           first, handled = false;
    switch (event->eType)
    {
        case frmOpenEvent:
            /*
             * Look for VFS volumes.
             */

            /*
             * Scan databases for disk images.
             */
            first = true;
            numDsks = 0;
            while (!DmGetNextDatabaseByTypeCreator(first, &searchState, NULL, 'Apl2', false, &cardNo, &dbID))
            {
                first = false;
                DmDatabaseInfo(cardNo, dbID, name, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &dbType, NULL);
                if (dbType == 'RDSK' || dbType == 'DDSK')
                    numDsks++;
            }
            first = true;
            nameDsks = MemPtrNew(numDsks * sizeof(Char *));
            nameDsks[0] = MemPtrNew(8);
            numDsks = 0;
            while (!DmGetNextDatabaseByTypeCreator(first, &searchState, NULL, 'Apl2', false, &cardNo, &dbID))
            {
                first = false;
                DmDatabaseInfo(cardNo, dbID, name, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &dbType, NULL);
                if (dbType == 'RDSK' || dbType == 'DDSK')
                {
                    nameDsks[numDsks] = MemPtrNew(32);
                    StrCopy(nameDsks[numDsks], name);
                    numDsks++;
                }
            }
            LstSetListChoices(GetObjectPtr(lstDsk), nameDsks, numDsks);
            FrmDrawForm(FrmGetActiveForm());
            handled = true;
            break;
        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID)
            {
                case buttonExit:
                {
                    UInt32 dbid, retval;
                    if ((dbid = DmFindDatabase(0, "Appalm ][")))
                        SysUIAppSwitch(0, dbid, sysAppLaunchCmdNormalLaunch, NULL);
                    else
                    {
                        stopEvent.eType = appStopEvent;
                        EvtAddEventToQueue(&stopEvent);
                    }
                }
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
    /*
     * Goto main form.
     */
    FrmGotoForm(DskMgrForm);
    return (true);
}
static void AppStop(void)
{
}
static Boolean AppProcessEvent(EventType *event)
{
    UInt16    error;
    FormType *frm;

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
                case DskMgrForm:
                    FrmSetEventHandler(frm, DskManagerHandleEvent);
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
    UInt16    error;
    EventType event;

    if (cmd == sysAppLaunchCmdNormalLaunch)
    {
        if (AppStart())
        {
            /*
             * Main event loop.
             */
            do
            {
                EvtGetEvent(&event, evtWaitForever);
            } while (AppProcessEvent(&event));
            AppStop();
        }
    }
    return(0);
}

