/*
 *	fonts.c
 */

/*
 *	apple fonts bitmap files
 */

#include "fonts7x8/char00.bmp"
#include "fonts7x8/char01.bmp"
#include "fonts7x8/char02.bmp"
#include "fonts7x8/char03.bmp"
#include "fonts7x8/char04.bmp"
#include "fonts7x8/char05.bmp"
#include "fonts7x8/char06.bmp"
#include "fonts7x8/char07.bmp"
#include "fonts7x8/char08.bmp"
#include "fonts7x8/char09.bmp"
#include "fonts7x8/char0a.bmp"
#include "fonts7x8/char0b.bmp"
#include "fonts7x8/char0c.bmp"
#include "fonts7x8/char0d.bmp"
#include "fonts7x8/char0e.bmp"
#include "fonts7x8/char0f.bmp"
#include "fonts7x8/char10.bmp"
#include "fonts7x8/char11.bmp"
#include "fonts7x8/char12.bmp"
#include "fonts7x8/char13.bmp"
#include "fonts7x8/char14.bmp"
#include "fonts7x8/char15.bmp"
#include "fonts7x8/char16.bmp"
#include "fonts7x8/char17.bmp"
#include "fonts7x8/char18.bmp"
#include "fonts7x8/char19.bmp"
#include "fonts7x8/char1a.bmp"
#include "fonts7x8/char1b.bmp"
#include "fonts7x8/char1c.bmp"
#include "fonts7x8/char1d.bmp"
#include "fonts7x8/char1e.bmp"
#include "fonts7x8/char1f.bmp"
#include "fonts7x8/char20.bmp"
#include "fonts7x8/char21.bmp"
#include "fonts7x8/char22.bmp"
#include "fonts7x8/char23.bmp"
#include "fonts7x8/char24.bmp"
#include "fonts7x8/char25.bmp"
#include "fonts7x8/char26.bmp"
#include "fonts7x8/char27.bmp"
#include "fonts7x8/char28.bmp"
#include "fonts7x8/char29.bmp"
#include "fonts7x8/char2a.bmp"
#include "fonts7x8/char2b.bmp"
#include "fonts7x8/char2c.bmp"
#include "fonts7x8/char2d.bmp"
#include "fonts7x8/char2e.bmp"
#include "fonts7x8/char2f.bmp"
#include "fonts7x8/char30.bmp"
#include "fonts7x8/char31.bmp"
#include "fonts7x8/char32.bmp"
#include "fonts7x8/char33.bmp"
#include "fonts7x8/char34.bmp"
#include "fonts7x8/char35.bmp"
#include "fonts7x8/char36.bmp"
#include "fonts7x8/char37.bmp"
#include "fonts7x8/char38.bmp"
#include "fonts7x8/char39.bmp"
#include "fonts7x8/char3a.bmp"
#include "fonts7x8/char3b.bmp"
#include "fonts7x8/char3c.bmp"
#include "fonts7x8/char3d.bmp"
#include "fonts7x8/char3e.bmp"
#include "fonts7x8/char3f.bmp"
#include "fonts7x8/char40.bmp"
#include "fonts7x8/char41.bmp"
#include "fonts7x8/char42.bmp"
#include "fonts7x8/char43.bmp"
#include "fonts7x8/char44.bmp"
#include "fonts7x8/char45.bmp"
#include "fonts7x8/char46.bmp"
#include "fonts7x8/char47.bmp"
#include "fonts7x8/char48.bmp"
#include "fonts7x8/char49.bmp"
#include "fonts7x8/char4a.bmp"
#include "fonts7x8/char4b.bmp"
#include "fonts7x8/char4c.bmp"
#include "fonts7x8/char4d.bmp"
#include "fonts7x8/char4e.bmp"
#include "fonts7x8/char4f.bmp"
#include "fonts7x8/char50.bmp"
#include "fonts7x8/char51.bmp"
#include "fonts7x8/char52.bmp"
#include "fonts7x8/char53.bmp"
#include "fonts7x8/char54.bmp"
#include "fonts7x8/char55.bmp"
#include "fonts7x8/char56.bmp"
#include "fonts7x8/char57.bmp"
#include "fonts7x8/char58.bmp"
#include "fonts7x8/char59.bmp"
#include "fonts7x8/char5a.bmp"
#include "fonts7x8/char5b.bmp"
#include "fonts7x8/char5c.bmp"
#include "fonts7x8/char5d.bmp"
#include "fonts7x8/char5e.bmp"
#include "fonts7x8/char5f.bmp"

/*
 *	bitmap index array
 */

#define	MAX_CHAR_BITMAP	0x60

char *AppleFontBitmap7x8[MAX_CHAR_BITMAP] = {
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
