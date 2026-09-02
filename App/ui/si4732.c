/* Si4732 receive screen.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#include <string.h>

#include "app/si4732.h"
#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "misc.h"
#include "ui/helper.h"
#include "ui/si4732.h"
#include "ui/ui.h"

#ifdef ENABLE_SI4732

// RSSI in dBuV mapped onto the 60 px bar. 0..64 dBuV covers the useful range
// of the chip's own AGC report.
#define SI4732_BAR_X      44
#define SI4732_BAR_WIDTH  60

static void draw_signal_bar(uint8_t line)
{
    uint8_t filled = gSI4732Status.rssi;

    if (filled > 64)
        filled = 64;

    filled = (uint8_t)((filled * SI4732_BAR_WIDTH) / 64);

    for (uint8_t x = 0; x < SI4732_BAR_WIDTH; x++)
        gFrameBuffer[line][SI4732_BAR_X + x] = (x < filled) ? 0x7E : 0x42;
}

// Band scope: 64 bins, two pixels wide, drawn upwards from the bottom of the
// plot area. Lines 1 to 5 give 40 pixels of height for a 0..64 dBuV reading.
#define SCOPE_TOP_LINE  1
#define SCOPE_LINES     5
#define SCOPE_HEIGHT    (SCOPE_LINES * 8)

static void draw_scope(void)
{
    char string[24];

    sprintf(string, "%u.%03u", gSI4732ScopeCenter / 1000000,
            (gSI4732ScopeCenter % 1000000) / 1000);
    UI_PrintStringSmallNormal(string, 2, 0, 0);

    sprintf(string, "SPAN %ukHz", SI4732APP_ScopeSpanHz() / 1000);
    UI_PrintStringSmallNormal(string, 50, LCD_WIDTH, 0);

    for (uint8_t bin = 0; bin < SI4732_SCOPE_BINS; bin++) {
        uint8_t level = gSI4732ScopeBin[bin];

        if (level > 64)
            level = 64;

        const uint8_t height = (uint8_t)(((uint16_t)level * SCOPE_HEIGHT) / 64);

        for (uint8_t row = 0; row < SCOPE_LINES; row++) {
            // Row 0 is the top of the plot, so it fills last.
            const uint8_t from_bottom = (uint8_t)((SCOPE_LINES - 1 - row) * 8);
            uint8_t       column      = 0;

            for (uint8_t bit = 0; bit < 8; bit++)
                if (height > from_bottom + (7 - bit))
                    column |= (uint8_t)(1u << bit);

            gFrameBuffer[SCOPE_TOP_LINE + row][bin * 2]     = column;
            gFrameBuffer[SCOPE_TOP_LINE + row][bin * 2 + 1] = column;
        }
    }

    // Centre marker on its own line, under the plot, so a tall bar cannot
    // swallow it.
    gFrameBuffer[SCOPE_TOP_LINE + SCOPE_LINES][LCD_WIDTH / 2 - 1] = 0x03;
    gFrameBuffer[SCOPE_TOP_LINE + SCOPE_LINES][LCD_WIDTH / 2]     = 0x07;
    gFrameBuffer[SCOPE_TOP_LINE + SCOPE_LINES][LCD_WIDTH / 2 + 1] = 0x03;

    UI_PrintStringSmallNormal("MENU=最大へ  *=幅", 0, LCD_WIDTH, 7);
}

void UI_DisplaySI4732(void)
{
    char string[24];

    UI_DisplayClear();

    if (!gSI4732Present) {
        UI_PrintString("SI4732", 0, LCD_WIDTH, 1, 8);
        UI_PrintStringSmallNormal("モジュールが", 0, LCD_WIDTH, 4);
        UI_PrintStringSmallNormal("見つかりません", 0, LCD_WIDTH, 5);
        ST7565_BlitFullScreen();
        return;
    }

    if (gSI4732Scope) {
        draw_scope();
        ST7565_BlitFullScreen();
        return;
    }

    // Band name, top left
    UI_PrintStringSmallNormal(gSI4732Bands[gSI4732Band].name, 2, 0, 0);

    // Mode and bandwidth, top right
    sprintf(string, "%s %s", SI4732APP_ModeName(), SI4732APP_BandwidthName());
    UI_PrintStringSmallNormal(string, 46, LCD_WIDTH, 0);

    // A frequency being keyed in takes over the big line until it is applied.
    const char *entry = SI4732APP_EntryText();

    if (entry) {
        sprintf(string, "%s_", entry);
        UI_PrintString(string, 0, LCD_WIDTH, 2, 8);
        UI_PrintStringSmallNormal("kHz  MENU=決定", 0, LCD_WIDTH, 4);
        UI_PrintStringSmallNormal("EXIT=1桁消す", 0, LCD_WIDTH, 5);
        ST7565_BlitFullScreen();
        return;
    }

    // Frequency. Broadcast bands read in kHz, FM in MHz.
    if (gSI4732Mode == SI4732_MODE_FM) {
        sprintf(string, "%u.%02u", gSI4732Frequency / 1000000,
                (gSI4732Frequency % 1000000) / 10000);
    } else if (gSI4732Frequency < 2000000) {
        sprintf(string, "%u", gSI4732Frequency / 1000);
    } else {
        sprintf(string, "%u.%03u", gSI4732Frequency / 1000000,
                (gSI4732Frequency % 1000000) / 1000);
    }

    UI_PrintString(string, 0, LCD_WIDTH, 2, 8);

    // Tuning step, or the BFO offset when SSB is doing something with it.
    // Right aligned on its own line so it can never collide with the signal
    // report below - the two used to overlap once the numbers got wide.
    if (gSI4732Mode == SI4732_MODE_LSB || gSI4732Mode == SI4732_MODE_USB)
        sprintf(string, "BFO %+d", gSI4732Bfo);
    else if (SI4732APP_StepHz() >= 1000)
        sprintf(string, "STEP %ukHz", SI4732APP_StepHz() / 1000);
    else
        sprintf(string, "STEP %uHz", SI4732APP_StepHz());

    UI_PrintStringSmallNormal(string, 0, LCD_WIDTH, 4);

    // Signal report
    UI_PrintStringSmallNormal("SIG", 2, 0, 5);
    draw_signal_bar(5);

    sprintf(string, "%udBuV  S/N %u", gSI4732Status.rssi, gSI4732Status.snr);
    UI_PrintStringSmallNormal(string, 0, LCD_WIDTH, 6);

    // Diagnostic: the raw RSQ response, so a stuck reading can be traced back
    // to the chip rather than guessed at.
    sprintf(string, "%02X %02X %02X %02X %02X %02X",
            gSI4732Status.raw[0], gSI4732Status.raw[1], gSI4732Status.raw[2],
            gSI4732Status.raw[3], gSI4732Status.raw[4], gSI4732Status.raw[5]);
    UI_PrintStringSmallNormal(string, 0, LCD_WIDTH, 7);

    ST7565_BlitFullScreen();
}

#endif
