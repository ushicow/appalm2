#define KEYBOARD        0xC000
#define CLR80STORE      0xC000
#define SET80STORE      0xC001
#define RDMAINRAM       0xC002
#define RDCARDRAM       0xC003
#define WRMAINRAM       0xC004
#define WRCARDRAM       0xC005
#define SETSLOTCXROM    0xC006
#define SETINTCXROM     0xC007
#define SETSTDZP        0xC008
#define SETALTZP        0xC009
#define SETINTC3ROM     0xC00A
#define SETSLOTC3ROM    0xC00B
#define CLR80VID        0xC00C
#define SET80VID        0xC00D
#define CLRALTCHAR      0xC00E
#define SETALTCHAR      0xC00F

#define KBDSTRB         0xC010
#define RDLCBNK2        0xC011
#define RDLCRAM         0xC012
#define RDRAMRD         0xC013
#define RDRAMWRT        0xC014
#define RDCXROM         0xC015
#define RDALTZP         0xC016
#define RDC3ROM         0xC017
#define RD80STORE       0xC018
#define RDVBLBAR        0xC019
#define RDTEXT          0xC01A
#define RDMIXED         0xC01B
#define RDPAGE2         0xC01C
#define RDHIRES         0xC01D
#define RDALTCHAR       0xC01E
#define RD80COL         0xC01F

#define TAPEOUT         0xC020
#define PRNTOUT         0xC021        /* Virtual printer */
#define SPKR            0xC030

#define TXTCLR          0xC050
#define TXTSET          0xC051
#define MIXCLR          0xC052
#define MIXSET          0xC053
#define LOWSCR          0xC054
#define HISCR           0xC055
#define LOWRES          0xC056
#define HIRES           0xC057
#define CLRAN0          0xC058
#define SETAN0          0xC059
#define CLRAN1          0xC05A
#define SETAN1          0xC05B
#define CLRAN2          0xC05C
#define SETAN2          0xC05D
#define CLRAN3          0xC05E
#define SETDHIRES       0xC05E
#define SETAN3          0xC05F
#define CLRDHIRES       0xC05F

#define TAPEIN          0xC060
#define BUTN0           0xC061
#define BUTN1           0xC062
#define BUTN2           0xC063
#define PADDL0          0xC064
#define PADDL1          0xC065
#define PADDL2          0xC066
#define PADDL3          0xC067

#define BTNDURATION     0x10
#define PDLDURATION     0x10

#define PTRIG           0xC070
#define SETIOUDIS       0xC07E
#define RDIOUDIS        0xC07E
#define CLRIOUDIS       0xC07F
#define RDDHIRES        0xC07F

#define CLRROM          0xCFFF

/* Memory soft switches */
#define SS_SLOTCXROM    (1<<1)
#define SS_SLOTC3ROM    (1<<2)
#define SS_EXPNROM      (1<<3)        /* 80 column ROM $C800-$CFFF active */
#define SS_RAMRD        (1<<4)
#define SS_RAMWRT       (1<<5)
#define SS_LCRAMMAP     (1<<6)
#define SS_LCBNK2       (1<<7)
#define SS_LCRAMWRT     (1<<8)

/* Video soft switched */
#define SS_TEXT         (1<<0)
#define SS_MIXED        (1<<1)
#define SS_PAGE2        (1<<2)
#define SS_HIRES        (1<<3)
#define SS_VBLBAR       (1<<4)
#define SS_80STORE      (1<<5)
#define SS_80VID        (1<<6)
#define SS_ALTZIP       (1<<7)

#define SS_AN0          (1<<1)
#define SS_AN1          (1<<2)
#define SS_AN2          (1<<3)
#define SS_AN3          (1<<4)

#define SS_SET(iou, ss)     iou |= (ss)
#define SS_RESET(iou, ss)   iou &= ~(ss)
#define SS_ISSET(iou, ss)   (iou & (ss))
#define IO_PAGE(a)          ((a)&0xFF)
#define IO_SLOT(a)          (((a)>>4)&0x0F)
#define IO_OFFSET(a)        ((a)&0x0F)
extern UInt16               memIOU, vidIOU, kbdIOU, spkrIOU, btnIOU[3], pdlIOU[2], pdlResetIOU[2];

