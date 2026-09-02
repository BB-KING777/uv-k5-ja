/* Silicon Labs Si4732-A10 receiver driver (LW / MW / SW / FM, plus SSB with
 * the runtime patch).
 *
 * The chip sits on the same bit banged I2C bus as the BK1080 it replaces.
 * BK1080 answers at 0x80, the Si4732 at 0x22 with SEN tied low, so the two
 * never collide - but only one of them may drive the audio node at a time.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#ifndef DRIVER_SI4732_H
#define DRIVER_SI4732_H

#include <stdbool.h>
#include <stdint.h>

#ifdef ENABLE_SI4732

// 8 bit I2C addresses (SEN low)
#define SI4732_I2C_WRITE        0x22
#define SI4732_I2C_READ         0x23

// Commands
#define SI4732_CMD_POWER_UP     0x01
#define SI4732_CMD_GET_REV      0x10
#define SI4732_CMD_POWER_DOWN   0x11
#define SI4732_CMD_SET_PROPERTY 0x12
#define SI4732_CMD_GET_PROPERTY 0x13
#define SI4732_CMD_GET_INT_STAT 0x14
#define SI4732_CMD_FM_TUNE_FREQ 0x20
#define SI4732_CMD_FM_TUNE_STAT 0x22
#define SI4732_CMD_FM_RSQ_STAT  0x23
#define SI4732_CMD_AM_TUNE_FREQ 0x40
#define SI4732_CMD_AM_TUNE_STAT 0x42
#define SI4732_CMD_AM_RSQ_STAT  0x43

// Properties
#define SI4732_PROP_GPO_IEN                 0x0001
#define SI4732_PROP_REFCLK_FREQ             0x0201
#define SI4732_PROP_REFCLK_PRESCALE         0x0202
#define SI4732_PROP_SSB_BFO                 0x0100
#define SI4732_PROP_SSB_MODE                0x0101
#define SI4732_PROP_FM_DEEMPHASIS           0x1100
#define SI4732_PROP_FM_SOFT_MUTE_MAX_ATT    0x1302
#define SI4732_PROP_AM_CHANNEL_FILTER       0x3102
#define SI4732_PROP_AM_AVC_MAX_GAIN         0x3103
#define SI4732_PROP_AM_SOFT_MUTE_MAX_ATT    0x3302
#define SI4732_PROP_RX_VOLUME               0x4000
#define SI4732_PROP_RX_HARD_MUTE            0x4001

// Where the SSB patch image lives in the 2 MB SPI flash. Far above the
// settings and channel area, which stops below 0x010000.
#define SI4732_PATCH_FLASH_ADDR 0x1F0000
#define SI4732_PATCH_MAX_SIZE   0x4000
#define SI4732_PATCH_MAGIC      0x53534231  // "SSB1"

typedef enum {
    SI4732_MODE_FM = 0,
    SI4732_MODE_AM,
    SI4732_MODE_LSB,
    SI4732_MODE_USB,
} SI4732_Mode_t;

typedef struct {
    uint8_t  rssi;      // dBuV
    uint8_t  snr;       // dB
    bool     valid;     // chip reports a valid tune
    bool     smute;     // soft mute engaged
    bool     afcrl;     // AFC railed
    uint8_t  raw[6];    // STATUS + RESP1..RESP5, for the diagnostic line
} SI4732_Status_t;

bool SI4732_Detect(void);
bool SI4732_PowerUp(SI4732_Mode_t mode);
void SI4732_PowerDown(void);
void SI4732_SetProperty(uint16_t property, uint16_t value);
bool SI4732_Tune(SI4732_Mode_t mode, uint32_t freq_hz);
void SI4732_GetStatus(SI4732_Mode_t mode, SI4732_Status_t *status);
void SI4732_SetVolume(uint8_t volume);
void SI4732_SetMute(bool mute);
void SI4732_SetBandwidth(SI4732_Mode_t mode, uint8_t index);
void SI4732_SetBfo(int16_t hz);
void SI4732_SetAgcOverride(bool disable, uint8_t attenuation);

// True when a usable patch image is present in SPI flash.
bool SI4732_PatchAvailable(void);

#endif
#endif
