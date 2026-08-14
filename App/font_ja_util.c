/* Japanese glyph lookup helpers. See font_ja.h for licensing.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#include "font_ja.h"

#ifdef ENABLE_LANG_JA

uint32_t UTF8_Next(const char **pp)
{
    const uint8_t *p = (const uint8_t *)*pp;
    uint32_t cp;
    uint8_t  extra;

    const uint8_t c = *p++;

    if (c < 0x80) {
        *pp = (const char *)p;
        return c;
    }

    if ((c & 0xE0) == 0xC0) {
        cp = c & 0x1F;
        extra = 1;
    } else if ((c & 0xF0) == 0xE0) {
        cp = c & 0x0F;
        extra = 2;
    } else if ((c & 0xF8) == 0xF0) {
        cp = c & 0x07;
        extra = 3;
    } else {
        // stray continuation byte, skip it
        *pp = (const char *)p;
        return 0xFFFD;
    }

    while (extra--) {
        if ((*p & 0xC0) != 0x80) {  // truncated sequence
            *pp = (const char *)p;
            return 0xFFFD;
        }
        cp = (cp << 6) | (*p++ & 0x3F);
    }

    *pp = (const char *)p;
    return cp;
}

const uint8_t *FONT_JA_Glyph(uint32_t cp)
{
    uint16_t lo = 0;
    uint16_t hi = gFontJaCount;

    if (cp > 0xFFFF)
        return 0;

    while (lo < hi) {
        const uint16_t mid = lo + ((hi - lo) / 2);
        const uint16_t val = gFontJaIndex[mid];

        if (val == (uint16_t)cp)
            return gFontJa[mid];

        if (val < (uint16_t)cp)
            lo = mid + 1;
        else
            hi = mid;
    }

    return 0;
}

#endif
