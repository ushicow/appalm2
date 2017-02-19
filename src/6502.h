UInt32 run6502(UInt32);
void reset6502(UInt8 *, WRITEBYTE *, READBYTE *);
void setPcCheckFuncs(void);
void clrPcCheckFuncs(void);
extern UInt16 opTable6502[256];
extern union state6502_t
{
    struct
    {
        UInt32  DATA;   /* D0 */
        UInt32  INST;   /* D1 */
        UInt32  ACC;    /* D2 */
        UInt32  X;      /* D3 */
        UInt32  Y;      /* D4 */
        UInt32  SP_CC;  /* D5 */
        UInt32  EA;     /* D6 */
        UInt32  PAGE;   /* D7 */
        UInt32  PC;     /* A0 */
        UInt32  OPBASE; /* A1 */
        UInt32  MEMBASE;/* A2 */
        UInt32	WRMEM;  /* A3 */
        UInt32	RDMEM;  /* A4 */
        UInt32  ADDR;   /* A6 */
    } LongRegs;
    struct
    {
        UInt16  Filler0;
        UInt16  DATA;   /* D0 */
        UInt16  INSTHI;
        UInt16  INSTLO; /* D1 */
        UInt16  Filler1;
        UInt16  ACC;    /* D2 */
        UInt16  Filler2;
        UInt16  X;      /* D3 */
        UInt16  Filler3;
        UInt16  Y;      /* D4 */
        UInt16  SP;
        UInt16  CC;     /* D5 */
        UInt16  Filler5;
        UInt16  EA;     /* D6 */
        UInt16  Filler6;
        UInt16  PAGE;   /* D7 */
        UInt16  Filler7;
        UInt16  PC;     /* A0 */
        UInt16  Filler8;
        UInt16  OPBASE; /* A1 */
        UInt16  Filler9;
        UInt16  MEMBASE;/* A2 */
        UInt16  Filler10;
        UInt16  WRMEM;  /* A3 */
        UInt16  Filler11;
        UInt16  RDMEM;  /* A4 */
        UInt16  Filler12;
        UInt16  ADDR;   /* A6 */
    } WordRegs;
    struct 
    {
        UInt8  Filler0;
        UInt8  Filler1;
        UInt8  Filler2;
        UInt8  DATA;    /* D0 */
        UInt8  INST3;
        UInt8  INST2;
        UInt8  INST1;
        UInt8  INST0;   /* D1 */
        UInt8  Filler3;
        UInt8  Filler4;
        UInt8  Filler5;
        UInt8  ACC;     /* D2 */
        UInt8  Filler6;
        UInt8  Filler7;
        UInt8  Filler8;
        UInt8  X;       /* D3 */
        UInt8  Filler9;
        UInt8  Filler10;
        UInt8  Filler11;
        UInt8  Y;       /* D4 */
        UInt8  SPHI;
        UInt8  SPLO;
        UInt8  Filler14;
        UInt8  CC;      /* D5 */
        UInt8  Filler18;
        UInt8  Filler19;
        UInt8  Filler20;
        UInt8  EA;      /* D6 */
        UInt8  Filler21;
        UInt8  Filler22;
        UInt8  Filler23;
        UInt8  PAGE;    /* D7 */
        UInt8  Filler24;
        UInt8  Filler25;
        UInt8  PCHI;    /* A0 */
        UInt8  PCLO;    /* A0 */
        UInt8  Filler26;
        UInt8  Filler27;
        UInt8  OPBASEHI;/* A1 */
        UInt8  OPBASELO;/* A1 */
        UInt8  Filler28;
        UInt8  Filler29;
        UInt8  MEMBASEHI;/* A2 */
        UInt8  MEMBASELO;/* A2 */
        UInt8  Filler30;
        UInt8  Filler31;
        UInt8  WRMEM;   /* A3 */
        UInt8  WRMEMLO; /* A3 */
        UInt8  Filler32;
        UInt8  Filler33;
        UInt8  RDMEM;   /* A4 */
        UInt8  RDMEMLO; /* A4 */
        UInt8  Filler34;
        UInt8  Filler35;
        UInt8  ADDRHI;  /* A6 */
        UInt8  ADDRLO;  /* A6 */
    } ByteRegs;
} state6502;
extern UInt8 status6502;

