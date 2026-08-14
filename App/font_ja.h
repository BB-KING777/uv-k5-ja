/* Japanese 8x8 glyph support for the UV-K1 / UV-K5 V3 firmware.
 *
 * Glyphs come from the Misaki Gothic 8x8 font (c) Num Kadoma
 * https://littlelimit.net/misaki.htm
 * "These fonts are free softwares. Unlimited permission is granted to use,
 *  copy, and distribute it, with or without modification, either commercially
 *  and noncommercially. THESE FONTS ARE PROVIDED "AS IS" WITHOUT WARRANTY."
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#ifndef FONT_JA_H
#define FONT_JA_H

#include <stdint.h>

#ifdef ENABLE_LANG_JA

// Every Japanese glyph occupies a fixed 8 x 8 pixel cell.
#define JA_CELL_WIDTH 8

// Generated tables (tools/lang_ja/misaki2c.py). Sorted by code point so the
// lookup can binary search. Every glyph is 8 columns, one byte per column,
// bit 0 = top pixel, matching the ST7565 page layout of gFrameBuffer.
extern const uint16_t gFontJaIndex[];
extern const uint8_t  gFontJa[][8];
extern const uint16_t gFontJaCount;

// Decodes one UTF-8 sequence and advances *pp. Invalid bytes are skipped and
// reported as 0xFFFD so a corrupted string can never spin the caller.
uint32_t UTF8_Next(const char **pp);

// Returns the 8 column bytes for cp, or NULL when the glyph is not embedded.
const uint8_t *FONT_JA_Glyph(uint32_t cp);

#endif

#endif
