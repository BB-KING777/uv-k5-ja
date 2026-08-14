/* Si4732 receive mode. See si4732.h.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#include <string.h>

#include "app/si4732.h"
#include "audio.h"
#include "driver/bk1080.h"
#include "driver/bk4819.h"
#include "misc.h"
#include "settings.h"
#include "ui/ui.h"

#ifdef ENABLE_SI4732

// Band plan. Amateur allocations use the Japanese sub bands where they are
// narrower than the ITU region 3 ones, so the edges match what is legal to
// listen for and transmit on here.
const SI4732_Band_t gSI4732Bands[] = {
    { "LW",    153000,    279000,    198000, 1000, SI4732_MODE_AM  },
    { "MW",    522000,   1710000,    810000, 9000, SI4732_MODE_AM  },
    { "160m", 1800000,   1875000,   1810000, 1000, SI4732_MODE_LSB },
    { "120m", 2300000,   2495000,   2400000, 5000, SI4732_MODE_AM  },
    { "90m",  3200000,   3400000,   3300000, 5000, SI4732_MODE_AM  },
    { "80m",  3500000,   3805000,   3537000, 1000, SI4732_MODE_LSB },
    { "75m",  3900000,   4000000,   3950000, 5000, SI4732_MODE_AM  },
    { "60m",  4750000,   5060000,   4900000, 5000, SI4732_MODE_AM  },
    { "49m",  5800000,   6200000,   6000000, 5000, SI4732_MODE_AM  },
    { "40m",  7000000,   7200000,   7100000, 1000, SI4732_MODE_LSB },
    { "41m",  7200000,   7600000,   7300000, 5000, SI4732_MODE_AM  },
    { "31m",  9200000,   9990000,   9600000, 5000, SI4732_MODE_AM  },
    { "30m", 10100000,  10150000,  10120000, 1000, SI4732_MODE_USB },
    { "25m", 11600000,  12200000,  11800000, 5000, SI4732_MODE_AM  },
    { "22m", 13570000,  13870000,  13700000, 5000, SI4732_MODE_AM  },
    { "20m", 14000000,  14350000,  14100000, 1000, SI4732_MODE_USB },
    { "19m", 15100000,  15830000,  15400000, 5000, SI4732_MODE_AM  },
    { "17m", 18068000,  18168000,  18100000, 1000, SI4732_MODE_USB },
    { "16m", 17480000,  17900000,  17600000, 5000, SI4732_MODE_AM  },
    { "15m", 21000000,  21450000,  21200000, 1000, SI4732_MODE_USB },
    { "13m", 21450000,  21850000,  21600000, 5000, SI4732_MODE_AM  },
    { "12m", 24890000,  24990000,  24940000, 1000, SI4732_MODE_USB },
    { "11m", 25670000,  26100000,  25800000, 5000, SI4732_MODE_AM  },
    { "10m", 28000000,  29700000,  28500000, 1000, SI4732_MODE_USB },
    { "FM",  76000000, 108000000,  80000000,  100, SI4732_MODE_FM  },
};

const uint8_t gSI4732BandCount = ARRAY_SIZE(gSI4732Bands);

static const uint16_t kSteps[] = { 10, 50, 100, 500, 1000, 5000, 9000, 10000 };

static const char *const kModeNames[] = { "FM", "AM", "LSB", "USB" };

static const char *const kBandwidthNames[] = {
    "6.0k", "4.0k", "3.0k", "2.5k", "2.0k", "1.8k", "1.0k",
};

uint8_t         gSI4732Band;
uint32_t        gSI4732Frequency;
SI4732_Mode_t   gSI4732Mode;
uint8_t         gSI4732Bandwidth;
uint8_t         gSI4732StepIndex;
int16_t         gSI4732Bfo;
SI4732_Status_t gSI4732Status;
bool            gSI4732Present;

static uint8_t gPollDivider;

static uint8_t step_index_for(uint32_t step_hz)
{
    for (uint8_t i = 0; i < ARRAY_SIZE(kSteps); i++)
        if ((uint32_t)kSteps[i] * 10u == step_hz || kSteps[i] == step_hz)
            return i;

    return 2;
}

uint32_t SI4732APP_StepHz(void)
{
    return (uint32_t)kSteps[gSI4732StepIndex] * ((gSI4732Mode == SI4732_MODE_FM) ? 1000u : 1u);
}

const char *SI4732APP_ModeName(void)
{
    return kModeNames[gSI4732Mode];
}

const char *SI4732APP_BandwidthName(void)
{
    if (gSI4732Mode == SI4732_MODE_FM)
        return "WIDE";

    return kBandwidthNames[gSI4732Bandwidth < ARRAY_SIZE(kBandwidthNames) ? gSI4732Bandwidth : 0];
}

static void si4732_apply(void)
{
    SI4732_Tune(gSI4732Mode, gSI4732Frequency);
    SI4732_SetBandwidth(gSI4732Mode, gSI4732Bandwidth);

    if (gSI4732Mode == SI4732_MODE_LSB || gSI4732Mode == SI4732_MODE_USB)
        SI4732_SetBfo(gSI4732Bfo);
}

static void si4732_select_band(uint8_t band)
{
    if (band >= gSI4732BandCount)
        band = 0;

    gSI4732Band      = band;
    gSI4732Frequency = gSI4732Bands[band].default_hz;
    gSI4732Mode      = gSI4732Bands[band].mode;
    gSI4732StepIndex = step_index_for(gSI4732Bands[band].step_hz);
    gSI4732Bfo       = 0;

    si4732_apply();
}

void SI4732APP_Init(void)
{
    // The Si4732 replaces the BK1080 on the shared bus and on the audio node,
    // so the old FM chip has to be parked before we take over.
#ifdef ENABLE_FMRADIO
    BK1080_Init(0, 0);
#endif

    BK4819_SetAF(BK4819_AF_MUTE);

    gSI4732Present = SI4732_Detect();

    if (!gSI4732Present)
        return;

    if (gSI4732Frequency == 0)
        si4732_select_band(gSI4732Band);
    else
        si4732_apply();

    AUDIO_AudioPathOn();
}

void SI4732APP_Stop(void)
{
    if (gSI4732Present)
        SI4732_PowerDown();

    gPollDivider = 0;
}

void SI4732APP_Poll(void)
{
    if (!gSI4732Present)
        return;

    // The RSQ read is a two transaction I2C round trip, so it runs at about
    // 5 Hz rather than on every 20 ms display tick.
    if (++gPollDivider < 10)
        return;

    gPollDivider = 0;

    SI4732_GetStatus(gSI4732Mode, &gSI4732Status);
    gUpdateDisplay = true;
}

static void si4732_tune_by(int8_t direction)
{
    const SI4732_Band_t *band = &gSI4732Bands[gSI4732Band];
    const uint32_t       step = SI4732APP_StepHz();

    if (direction > 0) {
        gSI4732Frequency += step;
        if (gSI4732Frequency > band->high_hz)
            gSI4732Frequency = band->low_hz;
    } else {
        if (gSI4732Frequency < band->low_hz + step)
            gSI4732Frequency = band->high_hz;
        else
            gSI4732Frequency -= step;
    }

    si4732_apply();
}

static void si4732_cycle_mode(void)
{
    // FM is a property of the band, so only the AM / SSB triple rotates.
    if (gSI4732Mode == SI4732_MODE_FM)
        return;

    switch (gSI4732Mode) {
        case SI4732_MODE_AM:  gSI4732Mode = SI4732_MODE_LSB; break;
        case SI4732_MODE_LSB: gSI4732Mode = SI4732_MODE_USB; break;
        default:              gSI4732Mode = SI4732_MODE_AM;  break;
    }

    if (gSI4732Mode != SI4732_MODE_AM && !SI4732_PatchAvailable()) {
        // No SSB patch in flash: stay on AM rather than powering up into a
        // mode the chip cannot demodulate.
        gSI4732Mode = SI4732_MODE_AM;
    }

    si4732_apply();
}

void SI4732APP_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld)
{
    if (!bKeyPressed || bKeyHeld)
        return;

    switch (Key) {
        case KEY_UP:
            si4732_tune_by(1);
            break;

        case KEY_DOWN:
            si4732_tune_by(-1);
            break;

        case KEY_1:
            si4732_select_band((uint8_t)(gSI4732Band + 1) % gSI4732BandCount);
            break;

        case KEY_7:
            si4732_select_band((uint8_t)(gSI4732Band + gSI4732BandCount - 1) % gSI4732BandCount);
            break;

        case KEY_2:
            si4732_cycle_mode();
            break;

        case KEY_3:
            if (++gSI4732Bandwidth >= ARRAY_SIZE(kBandwidthNames))
                gSI4732Bandwidth = 0;
            SI4732_SetBandwidth(gSI4732Mode, gSI4732Bandwidth);
            break;

        case KEY_STAR:
            if (++gSI4732StepIndex >= ARRAY_SIZE(kSteps))
                gSI4732StepIndex = 0;
            break;

        case KEY_4:
            gSI4732Bfo -= 50;
            SI4732_SetBfo(gSI4732Bfo);
            break;

        case KEY_6:
            gSI4732Bfo += 50;
            SI4732_SetBfo(gSI4732Bfo);
            break;

        case KEY_EXIT:
            SI4732APP_Stop();
            gRequestDisplayScreen = DISPLAY_MAIN;
            return;

        default:
            return;
    }

    gUpdateDisplay = true;
}

#endif
