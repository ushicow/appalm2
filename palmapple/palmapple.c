// 6502 is Little Endian -- Least Signature Byte First
// ++++++++++++++++++++++++++++++++++++
// | Address 1      |   Address 0     |
// ++++++++++++++++++++++++++++++++++++
// | High Byte      |   Low Byte      |
// ++++++++++++++++++++++++++++++++++++
//
// Dragon Ball is Big Endian -- Most Signature Byte First
// ++++++++++++++++++++++++++++++++++++
// | Address 0      |   Address 1     |
// ++++++++++++++++++++++++++++++++++++
// | High Byte      |   Low Byte      |
// ++++++++++++++++++++++++++++++++++++

#include <PalmOS.h>
#include <PalmCompatibility.h>
#include <SonyCLIE.h>

#define	TRUE 		1
#define	FALSE 		0

static int	StartApplication(void);
static void	EventLoop(void);
static FormType *mainForm;

UInt16 refNum;

void init_memory();
void init_6502();

void switchHRMode(void);
void switchNormalMode(void);

UInt16    buffer1[16] = {0xF1, 0xF2, 0xF1, 0xF2,0xF1, 0xF2, 0xF1, 0xF2, 0xF1, 0xF2, 0xF1, 0xF2,0xF1, 0xF2, 0xF1, 0xF2,};

union PCStruct {
  UInt8  Byte[2];
  UInt16 PC;
};

DWord PilotMain (Word cmd, Ptr cmdPBP, Word launchFlags)
{
  int error;

  if (cmd == sysAppLaunchCmdNormalLaunch) {

#ifdef DEBUG
    HostTraceInit();
#endif

    MemSemaphoreReserve(TRUE);

    error = StartApplication();
    if (error) return error;

    EventLoop();

#ifdef SONY
    switchNormalMode();
#endif

    FtrPtrFree('Pala', 0);

    MemSemaphoreRelease(FALSE);

#ifdef DEBUG
    HostTraceClose();
#endif

  }

  return 0;
}

void switchHRMode(void) {
  Err error;
  UInt32 width, height, depth;

  error = HROpen(refNum);
  if (!error) {
    width = hrWidth; height = hrHeight; depth = 8;

    error = HRWinScreenMode(refNum, winScreenModeSet, &width, &height, &depth, NULL);
    if (error == errNone) {
    }
  }

}

void switchNormalMode(void) {
  Err error;

  error = HRWinScreenMode ( refNum, winScreenModeSetToDefaults, NULL, NULL, NULL, NULL );
  if ( error != errNone ){

  } else {

  }

  error = HRClose(refNum);
}

static int StartApplication(void)
{
#ifdef SONY
  SonySysFtrSysInfoP sonySysFtrSysInfoP;
  Err error = 0;

  if ((error = FtrGet(sonySysFtrCreator, sonySysFtrNumSysInfoP, (UInt32*)&sonySysFtrSysInfoP))) {
	
  } else {
    if (sonySysFtrSysInfoP->libr & sonySysFtrSysInfoLibrHR) {
      if ((error = SysLibFind(sonySysLibNameHR, &refNum))){
	if (error == sysErrLibNotFound) {
	  error = SysLibLoad('libr', sonySysFileCHRLib, &refNum );
	}
      }
    }
  }

  switchHRMode();
#endif

  mainForm = FrmNewForm(0, "AppleIIe", 0, 0, 160, 160, FALSE, 0, 0, 0);

  init_memory();
  init_6502();

  FrmDrawForm(mainForm);
  GsiInitialize();
  GsiSetLocation(0, 150);
  GsiEnable(true);

  return 0;
}

static void EventLoop(void)
{
  short     err;
  int       formID, count = 0, count1 = 0;
  UInt8*    pointer;
  FormPtr   form;
  EventType event;
  unsigned  sigcode;

  EvtGetEvent(&event, 0);
  if (!SysHandleEvent(&event)) {
    if (event.eType == frmLoadEvent) {
      form = FrmInitForm(0);
      FrmSetActiveForm(form);
    }
    if (event.eType == appStopEvent) return;
  }
  
  run_6502();

/*   do { */
/*       EvtGetEvent(&event, -1); */
/*       if (SysHandleEvent(&event)) continue; */

/*       FrmDispatchEvent(&event); */
/*   } while (event.eType != appStopEvent); */

}
