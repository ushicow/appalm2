#include <stdio.h>
#include <string.h>
#include <time.h>
#ifdef __APPLE_CC__
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#define	TRUE 		1
#define	FALSE 		0

typedef	unsigned char	BYTE;
typedef	unsigned	ADDR;

typedef unsigned char  UInt8;
typedef          char  Int8;
typedef unsigned short UInt16;
typedef          short Int16;
typedef unsigned long  UInt32;
typedef          long  Int32;
typedef unsigned long  LocalID;

#ifdef __APPLE_CC__
#define BE_UINT8(b)     ((UInt8)(b))
#define BE_UINT16(i)    ((UInt16)(i))
#define BE_UINT32(i)    ((UInt32)(i))
#define BE_INT8(b)      ((Int8)(b))
#define BE_INT16(i)     ((Int16)(i))
#define BE_INT32(i)     ((Int32)(i))
#define BE_LOCALID(i)   ((Int32)(i))
#else
#define BE_UINT8(b)     ((UInt8)(b))
#define BE_UINT16(i)    ((((UInt16)(i)&0xFF00)>>8)|(((UInt16)(i)&0xFF)<<8))
#define BE_UINT32(i)    (((((UInt32)(i))>>24))|((((UInt32)(i))>>8)&0x0000FF00)|((((UInt32)(i))<<8)&0x00FF0000)|(((UInt32)(i))<<24))
#define BE_INT8(b)      ((UInt8)(b))
#define BE_INT16(i)     ((((UInt16)(i)&0xFF00)>>8)|(((UInt16)(i)&0xFF)<<8))
#define BE_INT32(i)     (((((UInt32)(i))>>24))|((((UInt32)(i))>>8)&0x0000FF00)|((((UInt32)(i))<<8)&0x00FF0000)|(((UInt32)(i))<<24))
#define BE_LOCALID(i)   (((((UInt32)(i))>>24))|((((UInt32)(i))>>8)&0x0000FF00)|((((UInt32)(i))<<8)&0x00FF0000)|(((UInt32)(i))<<24))
#endif

#ifdef _WIN32
#define Apl2    'Apl2'
#else
#define Apl2    BE_UINT32('Apl2')
#endif
typedef struct {
	LocalID	localChunkID;
	UInt8	attributes;
	UInt8	uniqueID[3];
} RecordEntryType;
typedef struct {
	LocalID	nextRecordListID;
	UInt16	numRecords;
	UInt16	firstEntry;
} RecordListType;
typedef struct {
	UInt8	name[32];
	UInt16	attributes;
    UInt16  version;
	UInt32	creationDate;
	UInt32	modificationDate;
	UInt32	lastBackupDate;
	UInt32	modificationNumber;
	LocalID	appInfoID;
	LocalID	sortInfoID;
	UInt32	type;
	UInt32	creator;
	UInt32	uniqueIDSeed;
	RecordListType	recordList;
} DatabaseHdrType;
int main(int argc, char **argv)
{
    FILE *romimage, *pdb;
    int rom_size, header_size, i, j;
    char pdbfile[64], *p;

    DatabaseHdrType pdb_header;
    RecordEntryType pdb_record_entries[1];
    int now;

    if (argc >= 2)
    {
        if (!(romimage = fopen(argv[1], "rb")))
        {
            fprintf(stderr, "Unable to read ROM file %s.\n", argv[1]);
            exit(1);
        }
        /*
         * Strip leading pathname from ROM name.
         */
        for (p = argv[1]; *p; p++)
        {
            if (*p == '/' || *p == '\\')
                argv[1] = p + 1;
        }
        fseek(romimage, 0, SEEK_END);
        rom_size = ftell(romimage);
        strcpy(pdbfile, argv[1]);
        strcat(pdbfile, ".pdb");
        if (!(pdb = fopen(pdbfile, "wb")))
        {
            fprintf(stderr, "Unable to create PDB file %s.\n", pdbfile);
            exit(1);
        }
        header_size = 78/*sizeof(DatabaseHdrType)*/ + 8/*sizeof(RecordEntryType)*/ + 2;
        now = time(NULL);
        memset(&pdb_header, 0, sizeof(DatabaseHdrType));
        memset(&pdb_record_entries, 0, sizeof(RecordEntryType));
        if (strlen(argv[1]) >= 31)
            argv[1][31] = '\0';
        strcpy(pdb_header.name, argv[1]);
        pdb_header.modificationDate      = BE_UINT32(now);
        pdb_header.creationDate          = BE_UINT32(now);
        pdb_header.type                  = Apl2;
        pdb_header.creator               = Apl2;
        pdb_header.recordList.numRecords = BE_UINT16(1);
        j = header_size;
        pdb_record_entries[0].localChunkID = BE_LOCALID(j);
        pdb_record_entries[0].attributes   = 0x40;
        pdb_record_entries[0].uniqueID[0]  = 0;
        fwrite(&pdb_header, 78/*sizeof(DatabaseHdrType)*/, 1, pdb);
        fwrite(&pdb_record_entries, 8/*sizeof(RecordEntryType)*/, 1, pdb);
        i = 0;
        fwrite(&i, 2, 1, pdb); /* Filler */
        {
            unsigned char *rom = malloc(rom_size);
            fseek(romimage, 0L, 0L);
            fread(rom,  rom_size, 1, romimage);
            fwrite(rom, rom_size, 1, pdb);
            free(rom);
        }
        fclose(romimage);
        fclose(pdb);
    }
    else
        //fprintf(stderr, "Usage: %s <DSK file> <PDB file> [sectors/track (16)] [total tracks (35)]\n", argv[0]);
        fprintf(stderr, "Usage: %s <ROM file> \n", argv[0]);
    return (0);
}

