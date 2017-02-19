/*
 *	fonts.c
 */

/*
 *	apple fonts bitmap files
 */

#include "fonts4x6/char00.bmp"
#include "fonts4x6/char01.bmp"
#include "fonts4x6/char02.bmp"
#include "fonts4x6/char03.bmp"
#include "fonts4x6/char04.bmp"
#include "fonts4x6/char05.bmp"
#include "fonts4x6/char06.bmp"
#include "fonts4x6/char07.bmp"
#include "fonts4x6/char08.bmp"
#include "fonts4x6/char09.bmp"
#include "fonts4x6/char0a.bmp"
#include "fonts4x6/char0b.bmp"
#include "fonts4x6/char0c.bmp"
#include "fonts4x6/char0d.bmp"
#include "fonts4x6/char0e.bmp"
#include "fonts4x6/char0f.bmp"
#include "fonts4x6/char10.bmp"
#include "fonts4x6/char11.bmp"
#include "fonts4x6/char12.bmp"
#include "fonts4x6/char13.bmp"
#include "fonts4x6/char14.bmp"
#include "fonts4x6/char15.bmp"
#include "fonts4x6/char16.bmp"
#include "fonts4x6/char17.bmp"
#include "fonts4x6/char18.bmp"
#include "fonts4x6/char19.bmp"
#include "fonts4x6/char1a.bmp"
#include "fonts4x6/char1b.bmp"
#include "fonts4x6/char1c.bmp"
#include "fonts4x6/char1d.bmp"
#include "fonts4x6/char1e.bmp"
#include "fonts4x6/char1f.bmp"
#include "fonts4x6/char20.bmp"
#include "fonts4x6/char21.bmp"
#include "fonts4x6/char22.bmp"
#include "fonts4x6/char23.bmp"
#include "fonts4x6/char24.bmp"
#include "fonts4x6/char25.bmp"
#include "fonts4x6/char26.bmp"
#include "fonts4x6/char27.bmp"
#include "fonts4x6/char28.bmp"
#include "fonts4x6/char29.bmp"
#include "fonts4x6/char2a.bmp"
#include "fonts4x6/char2b.bmp"
#include "fonts4x6/char2c.bmp"
#include "fonts4x6/char2d.bmp"
#include "fonts4x6/char2e.bmp"
#include "fonts4x6/char2f.bmp"
#include "fonts4x6/char30.bmp"
#include "fonts4x6/char31.bmp"
#include "fonts4x6/char32.bmp"
#include "fonts4x6/char33.bmp"
#include "fonts4x6/char34.bmp"
#include "fonts4x6/char35.bmp"
#include "fonts4x6/char36.bmp"
#include "fonts4x6/char37.bmp"
#include "fonts4x6/char38.bmp"
#include "fonts4x6/char39.bmp"
#include "fonts4x6/char3a.bmp"
#include "fonts4x6/char3b.bmp"
#include "fonts4x6/char3c.bmp"
#include "fonts4x6/char3d.bmp"
#include "fonts4x6/char3e.bmp"
#include "fonts4x6/char3f.bmp"
#include "fonts4x6/char40.bmp"
#include "fonts4x6/char41.bmp"
#include "fonts4x6/char42.bmp"
#include "fonts4x6/char43.bmp"
#include "fonts4x6/char44.bmp"
#include "fonts4x6/char45.bmp"
#include "fonts4x6/char46.bmp"
#include "fonts4x6/char47.bmp"
#include "fonts4x6/char48.bmp"
#include "fonts4x6/char49.bmp"
#include "fonts4x6/char4a.bmp"
#include "fonts4x6/char4b.bmp"
#include "fonts4x6/char4c.bmp"
#include "fonts4x6/char4d.bmp"
#include "fonts4x6/char4e.bmp"
#include "fonts4x6/char4f.bmp"
#include "fonts4x6/char50.bmp"
#include "fonts4x6/char51.bmp"
#include "fonts4x6/char52.bmp"
#include "fonts4x6/char53.bmp"
#include "fonts4x6/char54.bmp"
#include "fonts4x6/char55.bmp"
#include "fonts4x6/char56.bmp"
#include "fonts4x6/char57.bmp"
#include "fonts4x6/char58.bmp"
#include "fonts4x6/char59.bmp"
#include "fonts4x6/char5a.bmp"
#include "fonts4x6/char5b.bmp"
#include "fonts4x6/char5c.bmp"
#include "fonts4x6/char5d.bmp"
#include "fonts4x6/char5e.bmp"
#include "fonts4x6/char5f.bmp"

/*
 *	bitmap index array
 */

#define	MAX_CHAR_BITMAP	0x60

char *AppleFontBitmap4x6[MAX_CHAR_BITMAP] = {
	char00_bits, char01_bits, char02_bits, char03_bits,
	char04_bits, char05_bits, char06_bits, char07_bits,
	char08_bits, char09_bits, char0a_bits, char0b_bits,
	char0c_bits, char0d_bits, char0e_bits, char0f_bits,
	char10_bits, char11_bits, char12_bits, char13_bits,
	char14_bits, char15_bits, char16_bits, char17_bits,
	char18_bits, char19_bits, char1a_bits, char1b_bits,
	char1c_bits, char1d_bits, char1e_bits, char1f_bits,
	char20_bits, char21_bits, char22_bits, char23_bits,
	char24_bits, char25_bits, char26_bits, char27_bits,
	char28_bits, char29_bits, char2a_bits, char2b_bits,
	char2c_bits, char2d_bits, char2e_bits, char2f_bits,
	char30_bits, char31_bits, char32_bits, char33_bits,
	char34_bits, char35_bits, char36_bits, char37_bits,
	char38_bits, char39_bits, char3a_bits, char3b_bits,
	char3c_bits, char3d_bits, char3e_bits, char3f_bits,
	char40_bits, char41_bits, char42_bits, char43_bits,
	char44_bits, char45_bits, char46_bits, char47_bits,
	char48_bits, char49_bits, char4a_bits, char4b_bits,
	char4c_bits, char4d_bits, char4e_bits, char4f_bits,
	char50_bits, char51_bits, char52_bits, char53_bits,
	char54_bits, char55_bits, char56_bits, char57_bits,
	char58_bits, char59_bits, char5a_bits, char5b_bits,
	char5c_bits, char5d_bits, char5e_bits, char5f_bits,
};
