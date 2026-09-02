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

    if (gSI4732Seeking) {
        UI_PrintStringSmallNormal(SI4732APP_SeekDirectionUp() ? "スキャン中 ↑"
                                                             : "スキャン中 ↓",
                                  0, LCD_WIDTH, 4);
        UI_PrintStringSmallNormal("どれか押すと停止", 0, LCD_WIDTH, 5);
        ST7565_BlitFullScreen();
        return;
    }

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
