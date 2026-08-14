/* Si4732-A10 receiver driver. See si4732.h for the bus layout.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#include <string.h>

#include "driver/si4732.h"
#include "driver/i2c.h"
#include "driver/py25q16.h"
#include "driver/system.h"
#include "driver/systick.h"

#ifdef ENABLE_SI4732

// Bandwidth tables, indexes match the menu order.
// AM_CHANNEL_FILTER: 0=6k 1=4k 2=3k 3=2k 4=1k 5=1.8k 6=2.5k
static const uint8_t kAmFilter[]  = { 0, 6, 1, 2, 5, 3, 4 };
// SSB_MODE low byte bandwidth field: 0=1.2k 1=2.2k 2=3k 3=4k 4=500Hz 5=1k
static const uint8_t kSsbFilter[] = { 2, 1, 0, 5, 4, 3, 3 };

static bool gPoweredUp;
static SI4732_Mode_t gCurrentMode;

// ---------------------------------------------------------------- transport

static bool si4732_wait_cts(uint16_t timeout_ms)
{
    while (timeout_ms--) {
        uint8_t status = 0;

        I2C_Start();
        if (I2C_Write(SI4732_I2C_READ) >= 0) {
            status = I2C_Read(true);
            I2C_Stop();
            if (status & 0x80)
                return true;
        } else {
            I2C_Stop();
        }

        SYSTEM_DelayMs(1);
    }

    return false;
}

static bool si4732_command(const uint8_t *cmd, uint8_t length)
{
    int result;

    I2C_Start();
    result = I2C_Write(SI4732_I2C_WRITE);
    if (result >= 0)
        result = I2C_WriteBuffer(cmd, length);
    I2C_Stop();

    if (result < 0)
        return false;

    return si4732_wait_cts(300);
}

static bool si4732_response(uint8_t *buffer, uint8_t length)
{
    int result;

    I2C_Start();
    result = I2C_Write(SI4732_I2C_READ);
    if (result >= 0)
        result = I2C_ReadBuffer(buffer, length);
    I2C_Stop();

    return result >= 0;
}

// ------------------------------------------------------------------ helpers

void SI4732_SetProperty(uint16_t property, uint16_t value)
{
    const uint8_t cmd[6] = {
        SI4732_CMD_SET_PROPERTY,
        0x00,
        (uint8_t)(property >> 8), (uint8_t)property,
        (uint8_t)(value >> 8),    (uint8_t)value,
    };

    si4732_command(cmd, sizeof(cmd));
}

bool SI4732_Detect(void)
{
    uint8_t cmd = SI4732_CMD_GET_INT_STAT;

    // A bare status read answers even before power up: the chip drives the
    // CTS bit as soon as it has a clock. A missing module leaves the bus
    // pulled high, which reads back as 0xFF - not a valid status byte.
    I2C_Start();
    const bool acked = I2C_Write(SI4732_I2C_WRITE) >= 0;
    if (acked)
        I2C_WriteBuffer(&cmd, 1);
    I2C_Stop();

    if (!acked)
        return false;

    uint8_t status = 0;
    if (!si4732_response(&status, 1))
        return false;

    return status != 0xFF;
}

// --------------------------------------------------------------- patch load

bool SI4732_PatchAvailable(void)
{
    uint8_t header[8];

    PY25Q16_ReadBuffer(SI4732_PATCH_FLASH_ADDR, header, sizeof(header));

    const uint32_t magic = ((uint32_t)header[0] << 24) | ((uint32_t)header[1] << 16) |
                           ((uint32_t)header[2] << 8)  |  (uint32_t)header[3];
    const uint32_t size  = ((uint32_t)header[4] << 24) | ((uint32_t)header[5] << 16) |
                           ((uint32_t)header[6] << 8)  |  (uint32_t)header[7];

    return magic == SI4732_PATCH_MAGIC && size > 0 && size <= SI4732_PATCH_MAX_SIZE &&
           (size % 8) == 0;
}

// Streams the SSB patch from SPI flash into the chip, 8 bytes at a time.
// Nothing is buffered in RAM beyond one block, which is what makes a 16 kB
// patch affordable on a 16 kB part.
static bool si4732_load_patch(void)
{
    uint8_t header[8];

    PY25Q16_ReadBuffer(SI4732_PATCH_FLASH_ADDR, header, sizeof(header));

    uint32_t size = ((uint32_t)header[4] << 24) | ((uint32_t)header[5] << 16) |
                    ((uint32_t)header[6] << 8)  |  (uint32_t)header[7];

    if (size == 0 || size > SI4732_PATCH_MAX_SIZE || (size % 8) != 0)
        return false;

    uint32_t address = SI4732_PATCH_FLASH_ADDR + sizeof(header);

    while (size) {
        uint8_t block[8];

        PY25Q16_ReadBuffer(address, block, sizeof(block));

        I2C_Start();
        const bool ok = I2C_Write(SI4732_I2C_WRITE) >= 0 &&
                        I2C_WriteBuffer(block, sizeof(block)) >= 0;
        I2C_Stop();

        if (!ok || !si4732_wait_cts(50))
            return false;

        address += sizeof(block);
        size    -= sizeof(block);
    }

    return true;
}

// ------------------------------------------------------------------ power up

bool SI4732_PowerUp(SI4732_Mode_t mode)
{
    const bool ssb = (mode == SI4732_MODE_LSB || mode == SI4732_MODE_USB);
    uint8_t    cmd[3];

    if (gPoweredUp)
        SI4732_PowerDown();

    // ARG1: CTSIEN | XOSCEN | (PATCH) | function
    //   function 0 = FM, 1 = AM. SSB reuses the AM function once patched.
    cmd[0] = SI4732_CMD_POWER_UP;
    cmd[1] = 0x90 | (ssb ? 0x20 : 0x00) | ((mode == SI4732_MODE_FM) ? 0x00 : 0x01);
    cmd[2] = 0x05;  // analog audio output

    if (!si4732_command(cmd, sizeof(cmd)))
        return false;

    if (ssb && !si4732_load_patch()) {
        SI4732_PowerDown();
        return false;
    }

    SYSTEM_DelayMs(10);

    // 32.768 kHz watch crystal on the module
    SI4732_SetProperty(SI4732_PROP_REFCLK_FREQ,     32768);
    SI4732_SetProperty(SI4732_PROP_REFCLK_PRESCALE, 1);
    SI4732_SetProperty(SI4732_PROP_GPO_IEN,         0);

    if (mode == SI4732_MODE_FM) {
        SI4732_SetProperty(SI4732_PROP_FM_DEEMPHASIS,        1);  // 50 us
        SI4732_SetProperty(SI4732_PROP_FM_SOFT_MUTE_MAX_ATT, 0);
    } else {
        SI4732_SetProperty(SI4732_PROP_AM_SOFT_MUTE_MAX_ATT, 0);
        SI4732_SetProperty(SI4732_PROP_AM_AVC_MAX_GAIN,      0x2A80);
    }

    SI4732_SetProperty(SI4732_PROP_RX_VOLUME,    63);
    SI4732_SetProperty(SI4732_PROP_RX_HARD_MUTE, 0);

    gPoweredUp   = true;
    gCurrentMode = mode;

    return true;
}

void SI4732_PowerDown(void)
{
    const uint8_t cmd = SI4732_CMD_POWER_DOWN;

    si4732_command(&cmd, 1);
    gPoweredUp = false;
}

// --------------------------------------------------------------------- tune

bool SI4732_Tune(SI4732_Mode_t mode, uint32_t freq_hz)
{
    uint8_t cmd[6];
    uint8_t length;

    if (!gPoweredUp || gCurrentMode != mode) {
        if (!SI4732_PowerUp(mode))
            return false;
    }

    if (mode == SI4732_MODE_FM) {
        const uint16_t units = (uint16_t)(freq_hz / 10000);  // 10 kHz units

        cmd[0] = SI4732_CMD_FM_TUNE_FREQ;
        cmd[1] = 0x00;
        cmd[2] = (uint8_t)(units >> 8);
        cmd[3] = (uint8_t)units;
        cmd[4] = 0x00;  // auto antenna cap
        length = 5;
    } else {
        const uint16_t khz = (uint16_t)(freq_hz / 1000);

        cmd[0] = SI4732_CMD_AM_TUNE_FREQ;
        cmd[1] = 0x00;
        cmd[2] = (uint8_t)(khz >> 8);
        cmd[3] = (uint8_t)khz;
        cmd[4] = 0x00;
        cmd[5] = 0x00;  // auto antenna cap
        length = 6;
    }

    return si4732_command(cmd, length);
}

void SI4732_GetStatus(SI4732_Mode_t mode, SI4732_Status_t *status)
{
    const uint8_t cmd[2] = {
        (mode == SI4732_MODE_FM) ? SI4732_CMD_FM_RSQ_STAT : SI4732_CMD_AM_RSQ_STAT,
        0x00,
    };
    uint8_t response[8];

    status->rssi  = 0;
    status->snr   = 0;
    status->valid = false;

    if (!gPoweredUp || !si4732_command(cmd, sizeof(cmd)))
        return;

    if (!si4732_response(response, sizeof(response)))
        return;

    // resp1 bit0 = valid, resp4 = RSSI, resp5 = SNR
    status->valid = (response[1] & 0x01) != 0;
    status->rssi  = response[4];
    status->snr   = response[5];
}

// ------------------------------------------------------------------ tuning

void SI4732_SetVolume(uint8_t volume)
{
    if (volume > 63)
        volume = 63;

    SI4732_SetProperty(SI4732_PROP_RX_VOLUME, volume);
}

void SI4732_SetMute(bool mute)
{
    SI4732_SetProperty(SI4732_PROP_RX_HARD_MUTE, mute ? 3 : 0);
}

void SI4732_SetBandwidth(SI4732_Mode_t mode, uint8_t index)
{
    if (mode == SI4732_MODE_FM)
        return;

    if (mode == SI4732_MODE_AM) {
        if (index >= sizeof(kAmFilter))
            index = 0;
        SI4732_SetProperty(SI4732_PROP_AM_CHANNEL_FILTER, kAmFilter[index]);
        return;
    }

    if (index >= sizeof(kSsbFilter))
        index = 0;

    // bit 4 selects LSB / USB, bits 1..0 the audio bandwidth
    const uint16_t sideband = (mode == SI4732_MODE_USB) ? 0x0000 : 0x0010;

    SI4732_SetProperty(SI4732_PROP_SSB_MODE, sideband | kSsbFilter[index] | 0x0080);
}

void SI4732_SetBfo(int16_t hz)
{
    SI4732_SetProperty(SI4732_PROP_SSB_BFO, (uint16_t)hz);
}

void SI4732_SetAgcOverride(bool disable, uint8_t attenuation)
{
    const uint8_t cmd[3] = { 0x48, (uint8_t)(disable ? 1 : 0), attenuation };

    si4732_command(cmd, sizeof(cmd));
}

#endif
