#ifndef _FONT_H_
#define _FONT_H_

/**
 * 8x8 monochrome bitmap fonts for rendering
 * Author: Daniel Hepper <daniel@hepper.net>
 *
 * License: Public Domain
 *
 * Based on:
 * // Summary: font8x8.h
 * // 8x8 monochrome bitmap fonts for rendering
 * //
 * // Author:
 * //     Marcel Sondaar
 * //     International Business Machines (public domain VGA fonts)
 * //
 * // License:
 * //     Public Domain
 *
 * Fetched from: http://dimensionalrift.homelinux.net/combuster/mos3/?p=viewsource&file=/modules/gfx/font8_8.asm
 **/

#include <stdint.h>

#define FONT_NUM_ROWS       8
#define FONT_MAX_CHAR       129
#define FONT_DEGREE_MARK    ((char)128)

extern const uint8_t font8x8_basic[FONT_MAX_CHAR][FONT_NUM_ROWS];

#endif//_FONT_H_
