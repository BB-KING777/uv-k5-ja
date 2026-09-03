/* Replays key events against the real SI4732APP_ProcessKeys.
 *
 * APP_CheckKeys cannot know that a key is going to be held until it has
 * already delivered the press, so a long press arrives as three events:
 *
 *     press    (key, pressed = 1, held = 0)
 *     repeat   (key, pressed = 1, held = 1)
 *     release  (key, pressed = 0, held = 1)
 *
 * and a short press as two:
 *
 *     press    (key, pressed = 1, held = 0)
 *     release  (key, pressed = 0, held = 0)
 *
 * A handler that acts on the press therefore runs the short action on the way
 * to every long press. That is exactly the bug this file was written for, so
 * the sequences below are the ones the radio actually produces rather than the
 * ones that would be convenient.
 *
 * Build and run with ./run.sh. Exits non-zero if anything regresses.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "driver/keyboard.h"
#include "driver/si4732.h"
#include "ui/ui.h"

/* ------------------------------------------------- firmware globals it uses */

bool              gUpdateDisplay;
uint8_t           gEnableSpeaker;
GUI_DisplayType_t gRequestDisplayScreen;
GUI_DisplayType_t gScreenToDisplay;

/* --------------------------------------------------------- driver stand ins */

static int agc_writes, bandwidth_writes, tunes, seeks;

bool SI4732_Detect(void)                              { return true; }
bool SI4732_PowerUp(SI4732_Mode_t m)                  { (void)m; return true; }
void SI4732_PowerDown(void)                           { }
bool SI4732_Tune(SI4732_Mode_t m, uint32_t f)         { (void)m; (void)f; tunes++; return true; }
bool SI4732_TuneFast(SI4732_Mode_t m, uint32_t f)     { (void)m; (void)f; return true; }
void SI4732_SetBandwidth(SI4732_Mode_t m, uint8_t i)  { (void)m; (void)i; bandwidth_writes++; }
void SI4732_SetBfo(int16_t hz)                        { (void)hz; }
void SI4732_SetAgcOverride(bool off, uint8_t att)     { (void)off; (void)att; agc_writes++; }
bool SI4732_PatchAvailable(void)                      { return false; }
void SI4732_GetStatus(SI4732_Mode_t m, SI4732_Status_t *s) { (void)m; memset(s, 0, sizeof(*s)); }

bool SI4732_SeekStart(SI4732_Mode_t m, bool up, uint32_t lo, uint32_t hi, uint32_t sp)
{ (void)m; (void)up; (void)lo; (void)hi; (void)sp; seeks++; return true; }
bool SI4732_SeekPoll(SI4732_Mode_t m, uint32_t *f, bool *b) { (void)m; (void)f; (void)b; return false; }
void SI4732_SeekCancel(SI4732_Mode_t m)               { (void)m; }

void AUDIO_AudioPathOn(void)                          { }
void AUDIO_AudioPathOff(void)                         { }
void BK4819_SetAF(int mode)                           { (void)mode; }
void BK1080_Init(uint16_t chan, bool enable)          { (void)chan; (void)enable; }
bool MAIN_TuneHz(uint32_t hz)                         { (void)hz; return false; }

#include "app/si4732.h"

/* ------------------------------------------------------------------ harness */

static int failures;

// printf pads by bytes, and the labels below are Japanese, so pad by the width
// the terminal will actually give them: two columns per non-ASCII character.
static void label(const char *what)
{
    int columns = 0;

    for (const unsigned char *p = (const unsigned char *)what; *p; p++)
        if (*p < 0x80)            columns += 1;
        else if ((*p & 0xC0) != 0x80) columns += 2;   // lead byte of a wide one

    printf("  %s%*s", what, columns < 38 ? 38 - columns : 1, "");
}

static void check(const char *what, const char *got, const char *want)
{
    const bool ok = strcmp(got, want) == 0;

    label(what);
    printf("%-6s %s\n", got, ok ? "ok" : "");
    if (!ok) { printf("%42s期待値 %s\n", "", want); failures++; }
}

static void check_u(const char *what, unsigned got, unsigned want)
{
    const bool ok = got == want;

    label(what);
    printf("%-6u %s\n", got, ok ? "ok" : "");
    if (!ok) { printf("%42s期待値 %u\n", "", want); failures++; }
}

static void tap(KEY_Code_t key)          /* short press */
{
    SI4732APP_ProcessKeys(key, true,  false);
    SI4732APP_ProcessKeys(key, false, false);
}

static void hold(KEY_Code_t key)         /* long press */
{
    SI4732APP_ProcessKeys(key, true,  false);
    SI4732APP_ProcessKeys(key, true,  true);
    SI4732APP_ProcessKeys(key, false, true);
}

int main(void)
{
    SI4732APP_Init();

    printf("短押しと長押しが混ざらないこと\n");

    const char *bw = SI4732APP_BandwidthName();
    hold(KEY_F);
    check("F 長押し: 感度が進む",        SI4732APP_AgcName(), "DX");
    check("F 長押し: 帯域幅は動かない",  SI4732APP_BandwidthName(), bw);

    const char *agc = SI4732APP_AgcName();
    tap(KEY_F);
    check("F 短押し: 帯域幅が進む",      SI4732APP_BandwidthName(), "4.0k");
    check("F 短押し: 感度は動かない",    SI4732APP_AgcName(), agc);

    printf("\n感度が一巡すること\n");
    hold(KEY_F); check("2回目", SI4732APP_AgcName(), "NOR");
    hold(KEY_F); check("3回目", SI4732APP_AgcName(), "LOC");
    hold(KEY_F); check("4回目", SI4732APP_AgcName(), "ATT");
    hold(KEY_F); check("5回目で AUTO に戻る", SI4732APP_AgcName(), "AUTO");

    printf("\n周波数の直接入力\n");
    tap(KEY_6); tap(KEY_0); tap(KEY_5); tap(KEY_5);
    check("入力中の表示", SI4732APP_EntryText(), "6055");
    tap(KEY_MENU);
    check_u("確定後の周波数 [kHz]", gSI4732Frequency / 1000, 6055);
    check("49m の既定モード", SI4732APP_ModeName(), "AM");
    check_u("49m の既定ステップ [Hz]", SI4732APP_StepHz(), 5000);

    printf("\nバンドの送り戻し\n");
    const uint8_t band = gSI4732Band;
    hold(KEY_STAR); check_u("✱ 長押しで次のバンド", gSI4732Band, band + 1);
    hold(KEY_MENU); check_u("MENU 長押しで元に戻る", gSI4732Band, band);

    printf("\nステップ\n");
    const uint8_t step_band = gSI4732Band;
    tap(KEY_STAR);
    check_u("✱ 短押しでバンドは動かない", gSI4732Band, step_band);

    printf("\nスキャン\n");
    seeks = 0;
    hold(KEY_UP);
    check_u("↑ 長押しでシークが始まる", seeks, 1);
    check_u("シーク中である", gSI4732Seeking, 1);
    tap(KEY_5);
    check_u("何かキーを押すと止まる", gSI4732Seeking, 0);

    printf("\n%s\n", failures ? "失敗あり" : "すべて通過");
    return failures ? 1 : 0;
}
