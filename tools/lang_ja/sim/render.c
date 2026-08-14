/* Host side renderer: links the real firmware text routines against a plain
 * framebuffer so the Japanese layout can be checked without flashing a radio.
 * Writes a PGM per screen; see render.sh.
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "driver/st7565.h"
#include "ui/helper.h"

uint8_t gFrameBuffer[7][128];
uint8_t gStatusLine[128];

// stubs for the firmware globals helper.c touches but the text routines do not
uint8_t gInputBox[8];
uint8_t gInputBoxIndex;
bool    gKeypadLocked;
struct { uint8_t pad[4096]; } gEeprom;
int sprintf_(char *s, const char *fmt, ...) { (void)s; (void)fmt; return 0; }

static void dump(const char *name, int scale)
{
    char path[256];
    snprintf(path, sizeof(path), "%s.pgm", name);
    FILE *f = fopen(path, "wb");
    fprintf(f, "P2\n%d %d\n255\n", 128 * scale, 56 * scale);
    for (int y = 0; y < 56 * scale; y++) {
        for (int x = 0; x < 128 * scale; x++) {
            const int py = y / scale, px = x / scale;
            const int on = (gFrameBuffer[py / 8][px] >> (py % 8)) & 1;
            fprintf(f, "%d ", on ? 0 : 255);
        }
        fputc('\n', f);
    }
    fclose(f);
}

static void clear(void) { memset(gFrameBuffer, 0, sizeof(gFrameBuffer)); }

int main(int argc, char **argv)
{
    const int scale = 3;

    // 1. the menu list column, exactly as UI_DisplayMenu draws it
    clear();
    UI_DrawLineBuffer(gFrameBuffer, 48, 0, 48, 55, 1);
    UI_PrintStringSmallNormal("スケルチ", 0, 0, 1);
    UI_PrintString("送信出力", 0, 48, 2, 8);
    for (unsigned int px = 0; px < 48; px++) {
        gFrameBuffer[2][px] ^= 0xFF;
        gFrameBuffer[3][px] ^= 0xFF;
    }
    UI_PrintStringSmallNormal("受信CTCS", 0, 0, 4);
    UI_PrintString("強", 50, 127, 2, 8);
    UI_PrintStringSmallNormal("01/78", 6, 0, 6);
    dump("screen_menu", scale);

    // 2. a long name plus a multi line value
    clear();
    UI_DrawLineBuffer(gFrameBuffer, 48, 0, 48, 55, 1);
    UI_PrintStringSmallNormal("シフト方向", 0, 0, 1);
    UI_PrintString("自動電源断", 0, 48, 2, 8);
    for (unsigned int px = 0; px < 48; px++) {
        gFrameBuffer[2][px] ^= 0xFF;
        gFrameBuffer[3][px] ^= 0xFF;
    }
    UI_PrintStringSmallNormal("狭帯域設定", 0, 0, 4);
    UI_PrintString("メッセージ", 50, 127, 1, 8);
    UI_PrintString("5.00kHz", 50, 127, 3, 8);
    dump("screen_long", scale);

    // 3. three line value pane (F LOCK) and mixed ascii
    clear();
    UI_DrawLineBuffer(gFrameBuffer, 48, 0, 48, 55, 1);
    UI_PrintStringSmallNormal("キーロック", 0, 0, 1);
    UI_PrintString("送信帯域", 0, 48, 2, 8);
    for (unsigned int px = 0; px < 48; px++) {
        gFrameBuffer[2][px] ^= 0xFF;
        gFrameBuffer[3][px] ^= 0xFF;
    }
    UI_PrintStringSmallNormal("優先CH1", 0, 0, 4);
    UI_PrintString("欧州/日本", 50, 127, 1, 8);
    UI_PrintString("144-146", 50, 127, 3, 8);
    UI_PrintString("430-440", 50, 127, 5, 8);
    dump("screen_flock", scale);

    // 4. every glyph in the table, small font, to eyeball the bitmaps
    clear();
    UI_PrintStringSmallNormal("アイウエオカキクケコサシスセソ", 0, 0, 0);
    UI_PrintStringSmallNormal("送信出力周波数帯域幅設定初期化", 0, 0, 2);
    UI_PrintStringSmallNormal("ABC 123 混在 テスト", 0, 0, 4);
    UI_PrintStringSmallNormalInverse("反転表示 OK", 2, 0, 6);
    dump("screen_glyphs", scale);

    (void)argc; (void)argv;
    return 0;
}
