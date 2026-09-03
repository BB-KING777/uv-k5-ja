/* Si4732 receive mode. See si4732.h.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#include <string.h>

#include "app/main.h"
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
    // Ordered so that a narrow band is matched before the wide one it sits
    // inside: band_for() takes the first hit. Amateur edges are the Japanese
    // allocations, and the broadcast segments are the ones that actually carry
    // something receivable here.
    { "LW",     153000,    279000,    198000, 1000, SI4732_MODE_AM  },
    { "MW",     522000,   1710000,    594000, 9000, SI4732_MODE_AM  },  // 9 kHz raster
    { "120m",  2300000,   2495000,   2400000, 5000, SI4732_MODE_AM  },
    { "90m",   3200000,   3400000,   3300000, 5000, SI4732_MODE_AM  },
    { "JMH1",  3620000,   3625000,   3622500, 1000, SI4732_MODE_USB },  // 気象FAX 3622.5
    { "80mHam",3500000,   3805000,   3537000, 1000, SI4732_MODE_LSB },
    { "75m",   3900000,   4000000,   3925000, 5000, SI4732_MODE_AM  },  // ラジオNIKKEI
    { "4MMar", 4000000,   4438000,   4200000, 1000, SI4732_MODE_USB },
    { "60m",   4750000,   5060000,   4900000, 5000, SI4732_MODE_AM  },
    { "5MAir", 5450000,   5730000,   5628000, 1000, SI4732_MODE_USB },  // 洋上管制
    { "49m",   5800000,   6200000,   6055000, 5000, SI4732_MODE_AM  },  // ラジオNIKKEI
    { "6MMar", 6200000,   6525000,   6350000, 1000, SI4732_MODE_USB },
    { "6MAir", 6525000,   6765000,   6655000, 1000, SI4732_MODE_USB },
    { "40mHam",7000000,   7200000,   7100000, 1000, SI4732_MODE_LSB },
    { "41m",   7200000,   7600000,   7300000, 5000, SI4732_MODE_AM  },
    { "JMH2",  7793000,   7798000,   7795000, 1000, SI4732_MODE_USB },  // 気象FAX 7795
    { "8MMar", 8100000,   8815000,   8400000, 1000, SI4732_MODE_USB },
    { "8MAir", 8815000,   9040000,   8951000, 1000, SI4732_MODE_USB },
    { "31m",   9200000,   9990000,   9595000, 5000, SI4732_MODE_AM  },  // ラジオNIKKEI
    { "TIME",  9990000,  10010000,  10000000, 1000, SI4732_MODE_AM  },  // WWV / WWVH
    { "30mHam",10100000, 10150000,  10120000, 1000, SI4732_MODE_USB },
    { "11MAir",11175000, 11400000,  11330000, 1000, SI4732_MODE_USB },
    { "25m",  11600000,  12100000,  11800000, 5000, SI4732_MODE_AM  },
    { "12MMar",12230000, 13200000,  12600000, 1000, SI4732_MODE_USB },
    { "13MAir",13200000, 13360000,  13300000, 1000, SI4732_MODE_USB },
    { "22m",  13570000,  13870000,  13700000, 5000, SI4732_MODE_AM  },
    { "JMH3", 13986000,  13991000,  13988500, 1000, SI4732_MODE_USB },  // 気象FAX 13988.5
    { "20mHam",14000000, 14350000,  14100000, 1000, SI4732_MODE_USB },
    { "19m",  15100000,  15830000,  15400000, 5000, SI4732_MODE_AM  },
    { "16MMar",16360000, 17410000,  16800000, 1000, SI4732_MODE_USB },
    { "16m",  17480000,  17900000,  17600000, 5000, SI4732_MODE_AM  },
    { "17mHam",18068000, 18168000,  18100000, 1000, SI4732_MODE_USB },
    { "15mHam",21000000, 21450000,  21200000, 1000, SI4732_MODE_USB },
    { "13m",  21450000,  21850000,  21600000, 5000, SI4732_MODE_AM  },
    { "FM",   76000000, 108000000,  80000000,  100, SI4732_MODE_FM  },
    // Catch all, last on purpose: keyed in frequencies that fall outside every
    // named band land here. AM_TUNE_FREQ stops at 23 MHz.
    { "SW",     150000,  23000000,   6055000, 5000, SI4732_MODE_AM  },
};

const uint8_t gSI4732BandCount = ARRAY_SIZE(gSI4732Bands);

static const uint16_t kSteps[] = { 10, 50, 100, 500, 1000, 5000, 9000, 10000 };

static const char *const kModeNames[] = { "FM", "AM", "LSB", "USB" };

// Sensitivity, built on AM_AGC_OVERRIDE (AN332 command 0x48): index 0 is
// minimum attenuation, 37 is maximum. AUTO leaves the chip's own AGC alone,
// which is right on a whip and wrong on an outdoor antenna with a strong
// local carrier in band.
static const struct {
    const char *name;
    bool        manual;
    uint8_t     attenuation;
} kAgc[] = {
    { "AUTO", false,  0 },
    { "DX",   true,   0 },   // hold maximum gain, for weak signals
    { "NOR",  true,  12 },
    { "LOC",  true,  26 },   // local, strong signal territory
    { "ATT",  true,  37 },   // maximum attenuation
};

static const char *const kBandwidthNames[] = {
    "6.0k", "4.0k", "3.0k", "2.5k", "2.0k", "1.8k", "1.0k",
};

uint8_t         gSI4732Band;
uint32_t        gSI4732Frequency;
SI4732_Mode_t   gSI4732Mode;
uint8_t         gSI4732Bandwidth;
uint8_t         gSI4732StepIndex;
uint8_t         gSI4732Agc;
int16_t         gSI4732Bfo;
SI4732_Status_t gSI4732Status;
bool            gSI4732Present;

static uint8_t gPollDivider;

// Seek state. The chip does the walking; this just starts it, waits, and
// gives up if it never reports back.
bool           gSI4732Seeking;
static bool    gSeekUp;
static uint16_t gSeekTicks;

#define SI4732_SEEK_TIMEOUT_TICKS 2500   // 25 s at one tick per 10 ms

static void seek_poll(void);

static uint8_t step_index_for(uint32_t step_hz)
{
    // Match the table value exactly. An earlier "* 10" fallback matched a
    // tenth sized step first, so a 5 kHz broadcast band came up on 500 Hz.
    for (uint8_t i = 0; i < ARRAY_SIZE(kSteps); i++)
        if (kSteps[i] == step_hz)
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

// Direct frequency entry, in kHz. Six digits covers 150 kHz through the top
// of the FM band, so the buffer auto applies once it is full.
#define SI4732_ENTRY_MAX 6

static char    gEntry[SI4732_ENTRY_MAX + 1];
static uint8_t gEntryLen;

const char *SI4732APP_EntryText(void)
{
    return gEntryLen ? gEntry : 0;
}

static uint8_t band_all(void)
{
    return (uint8_t)(gSI4732BandCount - 1);
}

static uint8_t band_for(uint32_t hz)
{
    for (uint8_t i = 0; i < band_all(); i++)
        if (hz >= gSI4732Bands[i].low_hz && hz <= gSI4732Bands[i].high_hz)
            return i;

    return band_all();
}

const char *SI4732APP_AgcName(void)
{
    return kAgc[gSI4732Agc].name;
}

static void si4732_apply_agc(void)
{
    if (gSI4732Mode == SI4732_MODE_FM)
        return;   // 0x48 is the AM side only

    SI4732_SetAgcOverride(kAgc[gSI4732Agc].manual, kAgc[gSI4732Agc].attenuation);
}

static void si4732_apply(void)
{
    SI4732_Tune(gSI4732Mode, gSI4732Frequency);
    SI4732_SetBandwidth(gSI4732Mode, gSI4732Bandwidth);
    si4732_apply_agc();

    if (gSI4732Mode == SI4732_MODE_LSB || gSI4732Mode == SI4732_MODE_USB)
        SI4732_SetBfo(gSI4732Bfo);
}

static void si4732_set_frequency(uint32_t hz)
{
    const uint8_t band = band_for(hz);

    gSI4732Band      = band;
    gSI4732Mode      = gSI4732Bands[band].mode;
    gSI4732StepIndex = step_index_for(gSI4732Bands[band].step_hz);
    gSI4732Bfo       = 0;

    if ((gSI4732Mode == SI4732_MODE_LSB || gSI4732Mode == SI4732_MODE_USB) &&
        !SI4732_PatchAvailable())
        gSI4732Mode = SI4732_MODE_AM;

    gSI4732Frequency = hz;

    si4732_apply();
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

    // Powering the chip up into SSB without the patch in flash leaves it dead,
    // which used to silence every band from 160 m upwards.
    if ((gSI4732Mode == SI4732_MODE_LSB || gSI4732Mode == SI4732_MODE_USB) &&
        !SI4732_PatchAvailable())
        gSI4732Mode = SI4732_MODE_AM;

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

    gEntryLen = 0;
    gEntry[0] = 0;

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
    if (gSI4732Seeking) {
        SI4732_SeekCancel(gSI4732Mode);
        gSI4732Seeking = false;
    }

    if (gSI4732Present)
        SI4732_PowerDown();

    gPollDivider = 0;
}

void SI4732APP_Poll(void)
{
    if (!gSI4732Present)
        return;

    if (gSI4732Seeking) {
        seek_poll();
        return;
    }

    // The RSQ read is a two transaction I2C round trip, so it runs at about
    // 5 Hz rather than on every 20 ms display tick.
    if (++gPollDivider < 10)
        return;

    gPollDivider = 0;

    SI4732_GetStatus(gSI4732Mode, &gSI4732Status);
    gUpdateDisplay = true;
}

// The transceiver side closes the audio path whenever its own squelch says so,
// which is why this screen used to need monitor mode held down. Nothing on the
// BK4829 side is being listened to here, so take the path back.
void SI4732APP_HoldAudio(void)
{
    if (!gSI4732Present || gEnableSpeaker)
        return;

    BK4819_SetAF(BK4819_AF_MUTE);
    AUDIO_AudioPathOn();
    gEnableSpeaker = true;
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

static void seek_stop(void)
{
    if (!gSI4732Seeking)
        return;

    SI4732_SeekCancel(gSI4732Mode);
    gSI4732Seeking = false;
    si4732_apply();
    gUpdateDisplay = true;
}

static void seek_start(bool up)
{
    const SI4732_Band_t *band = &gSI4732Bands[gSI4732Band];

    if (gSI4732Mode == SI4732_MODE_LSB || gSI4732Mode == SI4732_MODE_USB)
        return;   // the chip has no SSB seek

    gSeekUp    = up;
    gSeekTicks = 0;

    gSI4732Seeking = SI4732_SeekStart(gSI4732Mode, up, band->low_hz,
                                      band->high_hz, SI4732APP_StepHz());
    gUpdateDisplay = true;
}

static void seek_poll(void)
{
    uint32_t found      = 0;
    bool     band_limit = false;

    if (++gSeekTicks > SI4732_SEEK_TIMEOUT_TICKS) {
        seek_stop();
        return;
    }

    if (!SI4732_SeekPoll(gSI4732Mode, &found, &band_limit))
        return;

    gSI4732Seeking = false;

    if (found >= gSI4732Bands[gSI4732Band].low_hz &&
        found <= gSI4732Bands[gSI4732Band].high_hz)
        gSI4732Frequency = found;

    // Hitting the band edge is the end of the run, not a station.
    if (band_limit)
        si4732_apply();

    gUpdateDisplay = true;
}

bool SI4732APP_SeekDirectionUp(void)
{
    return gSeekUp;
}

// --------------------------------------------------------- chip hand off

bool SI4732APP_CanTune(uint32_t hz)
{
    const SI4732_Band_t *all = &gSI4732Bands[gSI4732BandCount - 1];
    const SI4732_Band_t *fm  = &gSI4732Bands[gSI4732BandCount - 2];

    return (hz >= all->low_hz && hz <= all->high_hz) ||
           (hz >= fm->low_hz  && hz <= fm->high_hz);
}

// Called from the main screen when a keyed in frequency falls below what the
// BK4819 can reach: the Si4732 takes it instead of the number being clamped
// up to the transceiver's lowest band.
bool SI4732APP_TakeOver(uint32_t hz)
{
    if (!SI4732APP_CanTune(hz))
        return false;

    if (gScreenToDisplay != DISPLAY_SI4732) {
        SI4732APP_Init();

        if (!gSI4732Present)
            return false;
    }

    si4732_set_frequency(hz);

    gRequestDisplayScreen = DISPLAY_SI4732;

    return true;
}

static void si4732_apply_entry(void)
{
    uint32_t khz = 0;

    for (uint8_t i = 0; i < gEntryLen; i++)
        khz = khz * 10u + (uint32_t)(gEntry[i] - '0');

    gEntryLen = 0;
    gEntry[0] = 0;

    const uint32_t hz = khz * 1000u;

    if (SI4732APP_CanTune(hz)) {
        si4732_set_frequency(hz);
        return;
    }

    // Above the Si4732's 23 MHz ceiling the transceiver is the right radio,
    // so pass the frequency across instead of refusing it.
    if (MAIN_TuneHz(hz)) {
        SI4732APP_Stop();
        gRequestDisplayScreen = DISPLAY_MAIN;
    }
}

void SI4732APP_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld)
{
    // Everything except the digits acts on release, because the key state
    // machine delivers the initial press before it knows the key is going to
    // be held: acting on the press meant a long press always performed the
    // short press action first, so changing the sensitivity also stepped the
    // bandwidth. Digits stay on the press - typing wants to feel immediate,
    // and they have no long press of their own.
    if (bKeyPressed && !bKeyHeld && Key <= KEY_9 && !gSI4732Seeking) {
        if (gEntryLen < SI4732_ENTRY_MAX) {
            gEntry[gEntryLen++] = (char)('0' + (Key - KEY_0));
            gEntry[gEntryLen]   = 0;
        }

        if (gEntryLen == SI4732_ENTRY_MAX)
            si4732_apply_entry();

        gUpdateDisplay = true;
        return;
    }

    if (bKeyPressed)
        return;

    if (gSI4732Seeking) {   // any key stops a seek
        seek_stop();
        return;
    }

    if (bKeyHeld) {
        if (!gSI4732Present)
            return;

        switch (Key) {
            case KEY_UP:
            case KEY_DOWN:
                seek_start(Key == KEY_UP);
                break;

            case KEY_STAR:   // band forward
                si4732_select_band((uint8_t)(gSI4732Band + 1) % gSI4732BandCount);
                break;

            case KEY_MENU:   // band back
                si4732_select_band((uint8_t)(gSI4732Band + gSI4732BandCount - 1) %
                                   gSI4732BandCount);
                break;

            case KEY_F:      // sensitivity
                if (++gSI4732Agc >= ARRAY_SIZE(kAgc))
                    gSI4732Agc = 0;
                si4732_apply_agc();
                break;

            default:
                return;
        }

        gUpdateDisplay = true;
        return;
    }

    switch (Key) {
        case KEY_UP:
            si4732_tune_by(1);
            break;

        case KEY_DOWN:
            si4732_tune_by(-1);
            break;

        case KEY_MENU:
            if (gEntryLen)
                si4732_apply_entry();
            else
                si4732_cycle_mode();
            break;

        case KEY_F:
            if (++gSI4732Bandwidth >= ARRAY_SIZE(kBandwidthNames))
                gSI4732Bandwidth = 0;
            SI4732_SetBandwidth(gSI4732Mode, gSI4732Bandwidth);
            break;

        case KEY_STAR:
            if (++gSI4732StepIndex >= ARRAY_SIZE(kSteps))
                gSI4732StepIndex = 0;
            break;

        case KEY_EXIT:
            if (gEntryLen) {  // backspace out of a half typed frequency
                gEntry[--gEntryLen] = 0;
                break;
            }
            SI4732APP_Stop();
            gRequestDisplayScreen = DISPLAY_MAIN;
            return;

        default:
            return;
    }

    gUpdateDisplay = true;
}

#endif
