/* On-screen BK4829 register dump. See regdump.h.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#include <string.h>

#include "driver/bk4819.h"
#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "misc.h"
#include "ui/helper.h"
#include "ui/regdump.h"
#include "ui/ui.h"

#ifdef ENABLE_REG_DUMP_SCREEN

// Registers the firmware never writes, paired with the reset value published
// in Beken's register table (DRT01-230606-C01, section 2). These still hold
// their power-on value, so they are a direct test of whether that document
// really describes this silicon or is the BK4819 table retitled.
typedef struct {
    uint8_t  reg;
    uint16_t expected;
} RegCheck_t;

static const RegCheck_t kChecks[] = {
    { 0x1A, 0x5850 },   // Crystal vReg / iBit
    { 0x2E, 0x9608 },   // CTCSS/CDCSS Tx Gain2
    { 0x34, 0x0000 },   // GPIO output type
    { 0x35, 0x0000 },   // GPIO output type
    { 0x44, 0x9009 },   // 300Hz AF response, Tx
    { 0x45, 0x31A9 },   // 300Hz AF response, Tx
    { 0x74, 0xF50B },   // 3000Hz AF response, Tx
    { 0x75, 0xF50B },   // 3000Hz AF response, Rx
};

uint8_t gRegDumpPage;

static void draw_verify_page(uint8_t page)
{
    char     text[24];
    unsigned first = page * REGDUMP_ROWS;
    unsigned line  = 0;

    for (unsigned i = first; i < ARRAY_SIZE(kChecks) && line < REGDUMP_ROWS; i++, line++) {
        const uint16_t value = BK4819_ReadRegister(kChecks[i].reg);

        sprintf(text, "%02X %04X %s", kChecks[i].reg, value,
                (value == kChecks[i].expected) ? "OK" : "NG");
        UI_PrintStringSmallNormal(text, 2, 0, line);

        if (value != kChecks[i].expected) {
            sprintf(text, "%04X", kChecks[i].expected);
            UI_PrintStringSmallNormal(text, 86, 0, line);
        }
    }

    // summary on the last verify page
    if (line < REGDUMP_ROWS && first + line >= ARRAY_SIZE(kChecks)) {
        unsigned ok = 0;
        for (unsigned i = 0; i < ARRAY_SIZE(kChecks); i++)
            if (BK4819_ReadRegister(kChecks[i].reg) == kChecks[i].expected)
                ok++;

        sprintf(text, "一致 %u/%u", ok, (unsigned)ARRAY_SIZE(kChecks));
        UI_PrintStringSmallNormal(text, 2, 0, line + 1);

        UI_PrintStringSmallNormal(ok == ARRAY_SIZE(kChecks) ? "表は実機と一致"
                                                            : "表と食い違いあり",
                                  2, 0, line + 2);
    }
}

static void draw_full_page(uint8_t page)
{
    char     text[24];
    unsigned first = page * REGDUMP_PER_PAGE;

    for (unsigned row = 0; row < REGDUMP_FULL_ROWS; row++) {
        const unsigned a = first + (row * 2);
        const unsigned b = a + 1;

        if (a >= 0x80)
            break;

        if (b < 0x80) {
            sprintf(text, "%02X:%04X %02X:%04X",
                    a, BK4819_ReadRegister(a),
                    b, BK4819_ReadRegister(b));
        } else {
            sprintf(text, "%02X:%04X", a, BK4819_ReadRegister(a));
        }

        UI_PrintStringSmallNormal(text, 2, 0, row);
    }
}

void UI_DisplayRegDump(void)
{
    char text[16];

    UI_DisplayClear();

    if (gRegDumpPage < REGDUMP_VERIFY_PAGES)
        draw_verify_page(gRegDumpPage);
    else
        draw_full_page(gRegDumpPage - REGDUMP_VERIFY_PAGES);

    // page counter, bottom right, in the tiny font so it never collides
    sprintf(text, "%u/%u", gRegDumpPage + 1, REGDUMP_PAGES);
    GUI_DisplaySmallest(text, 104, 50, false, true);

    ST7565_BlitFullScreen();
}

void REGDUMP_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld)
{
    if (!bKeyPressed || bKeyHeld)
        return;

    switch (Key) {
        case KEY_UP:
            gRegDumpPage = (gRegDumpPage + REGDUMP_PAGES - 1) % REGDUMP_PAGES;
            break;

        case KEY_DOWN:
            gRegDumpPage = (gRegDumpPage + 1) % REGDUMP_PAGES;
            break;

        case KEY_EXIT:
            gRequestDisplayScreen = DISPLAY_MAIN;
            return;

        default:
            return;
    }

    gUpdateDisplay = true;
}

#endif
