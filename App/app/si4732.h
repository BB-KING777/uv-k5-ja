/* Si4732 receive mode: band plan, tuning state and key handling.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#ifndef APP_SI4732_H
#define APP_SI4732_H

#include <stdbool.h>
#include <stdint.h>

#ifdef ENABLE_SI4732

#include "driver/keyboard.h"
#include "driver/si4732.h"

typedef struct {
    const char   *name;
    uint32_t      low_hz;
    uint32_t      high_hz;
    uint32_t      default_hz;
    uint16_t      step_hz;
    SI4732_Mode_t mode;
} SI4732_Band_t;

extern const SI4732_Band_t gSI4732Bands[];
extern const uint8_t       gSI4732BandCount;

extern uint8_t         gSI4732Band;
extern uint32_t        gSI4732Frequency;
extern SI4732_Mode_t   gSI4732Mode;
extern uint8_t         gSI4732Bandwidth;
extern uint8_t         gSI4732StepIndex;
extern int16_t         gSI4732Bfo;
extern SI4732_Status_t gSI4732Status;
extern bool            gSI4732Present;

void SI4732APP_Init(void);
void SI4732APP_Stop(void);
void SI4732APP_Poll(void);
void SI4732APP_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);

const char *SI4732APP_ModeName(void);
const char *SI4732APP_BandwidthName(void);
uint32_t    SI4732APP_StepHz(void);

#endif
#endif
