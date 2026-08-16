/* On-screen BK4829 register dump.
 *
 * Reads the registers straight out of the chip and shows them on the LCD, so
 * the values can be checked without a working CAT link.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#ifndef UI_REGDUMP_H
#define UI_REGDUMP_H

#include <stdbool.h>
#include <stdint.h>

#ifdef ENABLE_REG_DUMP_SCREEN

#include "driver/keyboard.h"

#define REGDUMP_VERIFY_PAGES 2
#define REGDUMP_ROWS         7
// The full dump keeps the bottom row clear so the page counter has somewhere
// to live without landing on top of a value.
#define REGDUMP_FULL_ROWS    6
#define REGDUMP_PER_PAGE     (REGDUMP_FULL_ROWS * 2)   // two registers per row
#define REGDUMP_FULL_PAGES   ((0x80 + REGDUMP_PER_PAGE - 1) / REGDUMP_PER_PAGE)
#define REGDUMP_PAGES        (REGDUMP_VERIFY_PAGES + REGDUMP_FULL_PAGES)

extern uint8_t gRegDumpPage;

void UI_DisplayRegDump(void);
void REGDUMP_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);

#endif
#endif
