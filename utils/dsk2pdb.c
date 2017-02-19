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
#define RDSK    'RDSK'
#define DDSK    'DDSK'
#define Apl2    'Apl2'
#else
#define RDSK    BE_UINT32('RDSK')
#define DDSK    BE_UINT32('DDSK')
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



#define	SIGN_EXTEND(x)	((int)((char)x))
/*
 * GCR encoding/decoding utility
 *
 */

/*
 * raw track
 * this is obtained by
 * 14.31818MHz / 14 / 32 / 8
 *
 */
#define RAW_TRACK_BYTES 6192/*6320*/
#define DOS_TRACK_BYTES 4096
#define RAW_TRACK_BITS (RAW_TRACK_BYTES*8)

static  FILE	*diskimage;
static	int	write_mode;
static	int	write_protect;
static	BYTE	nibble[RAW_TRACK_BYTES];
static	BYTE	dos_track[4096];
static	int	position;
static  int     current_slot;        /* Current slot we have open */
static  int     current_drive;       /* Current drive we have open */
                                        /* i.e to which diskimage points */
static int		motor_on;
static int		physical_track_no;
static int		stepper_status;
static int		track_buffer_valid=0;
static int		track_buffer_dirty=0;
static int		track_buffer_track=0;

static BYTE		data_latch;
static BYTE		address_latch;
static UInt32	LastAppleClock, AppleClock;

static BYTE	boot_ROM[256];

char *DiskROM= "DISK.ROM";

static BYTE	GCR_encoding_table[64] = {
	0x96, 0x97, 0x9A, 0x9B, 0x9D, 0x9E, 0x9F, 0xA6,
	0xA7, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB2, 0xB3,
	0xB4, 0xB5, 0xB6, 0xB7, 0xB9, 0xBA, 0xBB, 0xBC,
	0xBD, 0xBE, 0xBF, 0xCB, 0xCD, 0xCE, 0xCF, 0xD3,
	0xD6, 0xD7, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE,
	0xDF, 0xE5, 0xE6, 0xE7, 0xE9, 0xEA, 0xEB, 0xEC,
	0xED, 0xEE, 0xEF, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6,
	0xF7, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF };

static BYTE	GCR_decoding_table[256];
static	int	Swap_Bit[4] = { 0, 2, 1, 3 }; /* swap lower 2 bits */
static BYTE	GCR_buffer[256];
static BYTE	GCR_buffer2[86];

static int	Position=0;
static BYTE	*Track_Nibble;

/* physical sector no. to DOS 3.3 logical sector no. table */
static int	Logical_Sector[16] = {
	0x0, 0x7, 0xE, 0x6, 0xD, 0x5, 0xC, 0x4,
	0xB, 0x3, 0xA, 0x2, 0x9, 0x1, 0x8, 0xF };

static int	Physical_Sector[16];

/* static function prototypes */

static void init_GCR_table(void);
static BYTE gcr_read_nibble(void);
static void gcr_write_nibble( BYTE );
static void decode62( BYTE* );
static void encode62( BYTE* );
static void FM_encode( BYTE );
static BYTE FM_decode(void);
static void write_sync( int );
static int read_address_field( int*, int*, int* );
static void write_address_field( int, int, int );
static int read_data_field(void);
static void write_data_field(void);

#define FM_ENCODE(x)	gcrWriteNibble( ((x) >> 1) | 0xAA );\
			gcrWriteNibble( (x) | 0xAA )

static void init_GCR_table(void)
{
	static int	initialized = 0;
	int		i;

	if ( !initialized ) {
	   for( i = 0; i < 64; i++ )
	      GCR_decoding_table[GCR_encoding_table[i]] = i;
	   for( i = 0; i < 16; i++ )
	      Physical_Sector[Logical_Sector[i]] = i;
	   initialized = 1;
	}
}

static BYTE gcr_read_nibble(void)
{
	BYTE	data;

	data = Track_Nibble[Position++];
	if ( Position >= RAW_TRACK_BYTES )
	   Position = 0;
	return data;
}

static void gcr_write_nibble( BYTE data )
{
	Track_Nibble[Position++] = data;
	if ( Position >= RAW_TRACK_BYTES )
	   Position = 0;
}

static void decode62( BYTE *page )
{
	int	i, j;

	/* get 6 bits from GCR_buffer & 2 from GCR_buffer2 */
	for( i = 0, j = 86; i < 256; i++ ) {
	  if ( --j < 0 ) j = 85;
	  page[i] = (GCR_buffer[i] << 2) | Swap_Bit[GCR_buffer2[j] & 0x03];
	  GCR_buffer2[j] >>= 2;
	}
}

static void encode62( BYTE *page )
{
	int	i, j;

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
static void FM_encode( BYTE data )
{
	gcr_write_nibble( (data >> 1) | 0xAA );
	gcr_write_nibble( data | 0xAA );
}

/*
 * return an FM encoded value, used in reading address fields
 */
static BYTE FM_decode(void)
{
	int		tmp;

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
	int	max_try;
	BYTE	nibble;

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
	int	i, max_try;
	BYTE	nibble, checksum;

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
	int	i;
	BYTE	last, checksum;

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

void SectorsToNibbles( BYTE *sectors, BYTE *nibbles, int volume, int track )
{
	int	i;

	init_GCR_table();
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

int NibblesToSectors( BYTE *nibbles, BYTE *sectors, int volume, int track )
{
	int	i, scanned[16], max_try, sectors_read;
	int	vv, tt, ss;	/* volume, track no. and sector no. */
	FILE	*fp;

	init_GCR_table();
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
	      printf("phy sector %d address field invalid\n", ss );
	      continue;	/* invalid values for vv, tt and ss, try again */
	   }

	   ss = Logical_Sector[ss];
	   if ( scanned[ss] )	/* sector has been read */
	      continue;

	   if ( read_data_field() ) {
	      decode62( sectors + ss * 0x100 );
	      scanned[ss] = 1;	/* this sector's ok */
	      sectors_read++;
	   }
	   else {
	      printf("fail reading data field of logical sector %d\n", ss );
	   }
	}

	/* if has failed to read any one sector, report error */
	if ( sectors_read == 16 )
	   return 1;
	else {
	   printf( "sectos_read = %d\n",sectors_read);
	   for( i = 0; i < 16; i++ )
	      if ( !scanned[i] )
	         printf( "sector %d(%d) corrupted\n", i, Physical_Sector[i] );
	   if((fp = fopen( ".track", "w"))!=NULL) {
	      fwrite( nibbles, 1, RAW_TRACK_BYTES, fp );
	      fclose(fp);
	   }
	   return 0;
	}
}

void load_track_buffer(void)
{
	int logical_track;

	if (!diskimage)
	   return;

	if ( physical_track_no & 0x3 ) {
	  fprintf( stderr, "Cannot read half track %g!\n",
	     physical_track_no * 0.25 );
	}

	logical_track = (physical_track_no+1)>>2;
	fseek( diskimage, logical_track * DOS_TRACK_BYTES, 0L );
	fread( dos_track, 1, DOS_TRACK_BYTES, diskimage );
	SectorsToNibbles( dos_track, nibble, 254, logical_track );
	track_buffer_track = physical_track_no;
#ifdef DEBUG
	printf( "load track %g\n", track_buffer_track*.25 );
#endif
	track_buffer_dirty = 0;
	track_buffer_valid = 1;

}
BYTE read_nibble(void)
{
	BYTE	data;
	static	flag;

	if ( !track_buffer_valid ) {
	  load_track_buffer();
	}

	flag = !flag;
	if ( flag )
	   return 0;

	data = nibble[position++];
	if ( position >= RAW_TRACK_BYTES )
	   position = 0;
	LastAppleClock = AppleClock;
	return data;


}

/* wkt -- still somehow need to stop diskRead from
 * reading when we don't have a disk to read
 */
int mount_disk( char *filename)
{
	if ( diskimage ) unmount_disk();
	write_protect = 0;
	diskimage = fopen( filename, "rb+" );
	if ( !diskimage ) {
	   write_protect = 1;
	   diskimage = fopen( filename, "rb" );
	}
	if (!diskimage) {
	   fprintf( stderr, "Fail to mount disk %s\n", filename );
	   return -1;
	}
	else {
#ifdef DEBUG
	   fprintf( stderr, "Mount disk %s\n", filename );
	   fprintf( stderr, "write protected = %d\n", write_protect );
#endif
	   current_slot=6; current_drive=0;
	   load_track_buffer();
	}
	return 0;
}

int unmount_disk()
{
	if ( diskimage ) {
	   fclose( diskimage );
	   diskimage = NULL;
	}
	return 0;
}
int main(int argc, char **argv)
{
    FILE *pdb;
    int tracks, sectors, file_size, header_size, track_size, i, j, cvt2raw, argfilename;
    char pdbfile[64], *p;

    DatabaseHdrType pdb_header;
    RecordEntryType pdb_record_entries[40];
    int now;

    /*
     * Strip leading pathname from executable name.
     */
    for (p = argv[0]; *p; p++)
    {
        if (*p == '/' || *p == '\\')
            argv[0] = p + 1;
    }
    if (!strcmp(argv[0], "dsk2pdb") || !strcmp(argv[0], "dsk2pdb.exe") || !strcmp(argv[0], "DSK2PDB.EXE"))
    {
        if (argc >= 2)
        {
            argfilename = 1;
            cvt2raw = 0;
            if (argv[1][0] == '-')
            {
                argfilename = 2;
                if (argv[1][1] == 'r')
                    cvt2raw = 1;
            }
            if (mount_disk(argv[argfilename]))
            {
                fprintf(stderr, "Unable to mount disk file %s.\n", argv[1]);
                exit(1);
            }
            tracks  = 35;
            sectors = 16;
            fseek(diskimage, 0, SEEK_END);
            file_size = ftell(diskimage);
            if (argc > argfilename + 1)
            {
                strcpy(pdbfile, argv[argfilename + 1]);
            }
            else
            {
                /*
                 * Strip leading pathname from DSK name.
                 */
                for (p = argv[argfilename]; *p; p++)
                {
                    if (*p == '/' || *p == '\\')
                        argv[argfilename] = p + 1;
                }
                strcpy(pdbfile, argv[argfilename]);
                strcat(pdbfile, ".pdb");
            }
            if (!(pdb = fopen(pdbfile, "wb")))
            {
                fprintf(stderr, "Unable to create PDB file %s.\n", argv[2]);
                exit(1);
            }
            header_size = 78/*sizeof(DatabaseHdrType)*/ + 8/*sizeof(RecordEntryType)*/ * tracks + 2;
            track_size  = cvt2raw ? RAW_TRACK_BYTES : (file_size / tracks);
            now = time(NULL);
            memset(&pdb_header, 0, sizeof(DatabaseHdrType));
            memset(&pdb_record_entries, 0, sizeof(RecordEntryType) * tracks);
            if (strlen(argv[argfilename]) >= 31)
                argv[argfilename][31] = '\0';
            strcpy(pdb_header.name, argv[argfilename]);
            pdb_header.modificationDate      = BE_UINT32(now);
            pdb_header.creationDate          = BE_UINT32(now);
            pdb_header.type                  = (track_size == DOS_TRACK_BYTES) ? DDSK : RDSK;
            pdb_header.creator               = Apl2;
            pdb_header.recordList.numRecords = BE_UINT16(tracks);
            for (i = 0; i < tracks; i++)
            {
                j = header_size + i * track_size;
                pdb_record_entries[i].localChunkID = BE_LOCALID(j);
                pdb_record_entries[i].attributes   = 0x40;
                pdb_record_entries[i].uniqueID[0]  = i;
            }
            fwrite(&pdb_header, 78/*sizeof(DatabaseHdrType)*/, 1, pdb);
            fwrite(&pdb_record_entries, 8/*sizeof(RecordEntryType)*/, tracks, pdb);
            i = 0;
            fwrite(&i, 2, 1, pdb); /* Filler */
            if (cvt2raw)
            {
                for (physical_track_no = 0; physical_track_no < tracks * 4; physical_track_no += 4)
                {
                    load_track_buffer();
                    fwrite(nibble, RAW_TRACK_BYTES, 1, pdb);
                }
            }
            else
            {
                unsigned char *all_tracks = malloc(track_size * tracks);
            	fseek(diskimage, 0L, 0L);
                fread(all_tracks,  track_size, tracks, diskimage);
                fwrite(all_tracks, track_size, tracks, pdb);
                free(all_tracks);
            }
            unmount_disk();
            fclose(pdb);
        }
        else
            //fprintf(stderr, "Usage: %s <DSK file> <PDB file> [sectors/track (16)] [total tracks (35)]\n", argv[0]);
            fprintf(stderr, "Usage: %s <DSK file> [PDB file]\n", argv[0]);
    }
    else if (!strcmp(argv[0], "pdb2dsk") || !strcmp(argv[0], "pdb2dsk.exe") || !strcmp(argv[0], "PDB2DSK.EXE"))
    {
        if (argc >= 2)
        {
            if (!(pdb = fopen(argv[1], "rb")))
            {
                fprintf(stderr, "Unable to open PDB file %s.\n", argv[1]);
                exit(1);
            }
            if (argc < 3)
            {
                fread(&pdb_header, 78/*sizeof(DatabaseHdrType)*/, 1, pdb);
                pdb_header.name[31] = '\0';
                if (!(diskimage = fopen(pdb_header.name, "wb")))
                {
                    fprintf(stderr, "Unable to open DSK file %s.\n", argv[2]);
                    exit(1);
                }
            }
            else
            {
                if (!(diskimage = fopen(argv[2], "wb")))
                {
                    fprintf(stderr, "Unable to open DSK file %s.\n", argv[2]);
                    exit(1);
                }
            }
            fseek(pdb, 78+8*35+2, 0L);
            while (fread(&i, 1, 1, pdb) > 0)
            {
                fwrite(&i, 1, 1, diskimage);
            }
            fclose(pdb);
            fclose(diskimage);
        }
        else
            //fprintf(stderr, "Usage: %s <DSK file> <PDB file> [sectors/track (16)] [total tracks (35)]\n", argv[0]);
            fprintf(stderr, "Usage: %s <PDB file> [DSK file]\n", argv[0]);
    }
    else
        fprintf(stderr, "Unknown launch name: %s\n", argv[0]);
    return (0);
}

