/* Japanese menu tables. Included from menu.c when ENABLE_LANG_JA is set.
 *
 * The preprocessor guards mirror the English tables exactly - keep the two in
 * sync when a menu entry is added upstream.
 *
 * Layout budget: the menu name column is 48 px wide, so a name may hold at
 * most 6 Japanese characters (8 px each) or 6 ASCII characters. Sub menu
 * values are drawn in the 78 px wide value pane.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

const t_menu_item MenuList[] =
{
//   text,              menu ID
    {"ステップ",        MENU_STEP          },
    {"送信出力",        MENU_TXP           },
    {"受信DCS",         MENU_R_DCS         },
    {"受信CTCS",        MENU_R_CTCS        },
    {"送信DCS",         MENU_T_DCS         },
    {"送信CTCS",        MENU_T_CTCS        },
    {"シフト方向",      MENU_SFT_D         },
    {"シフト幅",        MENU_OFFSET        },
    {"帯域幅",          MENU_W_N           },
#ifndef ENABLE_FEAT_F4HWN
    {"秘話",            MENU_SCR           },
#endif
    {"話中禁止",        MENU_BCL           },
    {"音声圧縮",        MENU_COMPAND       },
    {"変調方式",        MENU_AM            },
#ifdef ENABLE_FEAT_F4HWN
    {"送信ロック",      MENU_TX_LOCK       },
#endif
    {"CH一覧",          MENU_LIST_CH       },
    {"CH保存",          MENU_MEM_CH        },
    {"CH削除",          MENU_DEL_CH        },
    {"CH名称",          MENU_MEM_NAME      },

    {"SC一覧",          MENU_S_LIST        },
    {"SC優先",          MENU_S_PRI         },
    {"優先CH1",         MENU_S_PRI_CH_1    },
    {"優先CH2",         MENU_S_PRI_CH_2    },
    {"SC復帰",          MENU_SC_REV        },
#ifndef ENABLE_FEAT_F4HWN
    #ifdef ENABLE_NOAA
        {"NOAA受信",    MENU_NOAA_S        },
    #endif
#endif
    {"F1短押し",        MENU_F1SHRT        },
    {"F1長押し",        MENU_F1LONG        },
    {"F2短押し",        MENU_F2SHRT        },
    {"F2長押し",        MENU_F2LONG        },
    {"M長押し",         MENU_MLONG         },

    {"キーロック",      MENU_AUTOLK        },
    {"送信時間制限",    MENU_TOT           },
    {"節電モード",      MENU_SAVE          },
    {"電池表示",        MENU_BAT_TXT       },
    {"マイク感度",      MENU_MIC           },
    {"マイクバー",      MENU_MIC_BAR       },
    {"CH表示",          MENU_MDF           },
    {"起動画面",        MENU_PONMSG        },
    {"消灯時間",        MENU_ABR           },
    {"明るさ最小",      MENU_ABR_MIN       },
    {"明るさ最大",      MENU_ABR_MAX       },
    {"点灯条件",        MENU_ABR_ON_TX_RX  },
    {"操作音",          MENU_BEEP          },
#ifdef ENABLE_VOICE
    {"音声案内",        MENU_VOICE         },
#endif
    {"終話音",          MENU_ROGER         },
    {"尾切り",          MENU_STE           },
    {"中継尾切り",      MENU_RP_STE        },
    {"呼出CH",          MENU_1_CALL        },
#ifdef ENABLE_ALARM
    {"警報音",          MENU_AL_MOD        },
#endif
#ifdef ENABLE_DTMF_CALLING
    {"ANI ID",          MENU_ANI_ID        },
#endif
    {"UP符号",          MENU_UPCODE        },
    {"DW符号",          MENU_DWCODE        },
    {"PTT ID",          MENU_PTT_ID        },
    {"DTMF音",          MENU_D_ST          },
#ifdef ENABLE_DTMF_CALLING
    {"D応答",           MENU_D_RSP         },
    {"D保持",           MENU_D_HOLD        },
#endif
    {"DTMF遅延",        MENU_D_PRE         },
#ifdef ENABLE_DTMF_CALLING
    {"D検出",           MENU_D_DCD         },
    {"D一覧",           MENU_D_LIST        },
#endif
    {"DTMF表示",        MENU_D_LIVE_DEC    },
#ifndef ENABLE_FEAT_F4HWN
    #ifdef ENABLE_AM_FIX
        {"AM補正",      MENU_AM_FIX        },
    #endif
#endif
    {"VOX",             MENU_VOX           },
#ifdef ENABLE_FEAT_F4HWN
    {"本体情報",        MENU_VOL           },
#else
    {"電池電圧",        MENU_VOL           },
#endif
    {"受信方式",        MENU_TDR           },
    {"スケルチ",        MENU_SQL           },
#ifdef ENABLE_FEAT_F4HWN
    {"出力設定",        MENU_SET_PWR       },
    {"PTT動作",         MENU_SET_PTT       },
    {"制限通知",        MENU_SET_TOT       },
    {"終了通知",        MENU_SET_EOT       },
    {"画面濃度",        MENU_SET_CTR       },
    {"白黒反転",        MENU_SET_INV       },
    {"ロック範囲",      MENU_SET_LCK       },
    {"メーター",        MENU_SET_MET       },
    {"画面表示",        MENU_SET_GUI       },
#ifdef ENABLE_FEAT_F4HWN_AUDIO
    {"音質設定",        MENU_SET_AUD       },
#endif
    {"タイマー",        MENU_SET_TMR       },
#ifdef ENABLE_FEAT_F4HWN_SLEEP
    {"自動電源断",      MENU_SET_OFF       },
#endif
#ifdef ENABLE_FEAT_F4HWN_NARROWER
    {"狭帯域設定",      MENU_SET_NFM       },
#endif
#ifdef ENABLE_FEAT_F4HWN_VOL
    {"音量設定",        MENU_SET_VOL       },
#endif
#ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
    {"キー割当",        MENU_SET_KEY       },
#endif
#ifdef ENABLE_NOAA
    {"NOAA受信",        MENU_NOAA_S        },
#endif
#ifdef ENABLE_FEAT_F4HWN_SCAN_FASTER
    {"SC速度",          MENU_SET_SCN       },
#endif
#ifdef ENABLE_FEAT_F4HWN_LOGO_SAV
    {"起動ロゴ",        MENU_SET_SAV       },
#endif
#endif
    // hidden menu items from here on
    // enabled if pressing both the PTT and upper side button at power-on
    {"送信帯域",        MENU_F_LOCK        },
#ifndef ENABLE_FEAT_F4HWN
    {"Tx 200",          MENU_200TX         },
    {"Tx 350",          MENU_350TX         },
    {"Tx 500",          MENU_500TX         },
#endif
    {"350MHz",          MENU_350EN         },
#ifndef ENABLE_FEAT_F4HWN
    {"秘話許可",        MENU_SCREN         },
#endif
#ifdef ENABLE_F_CAL_MENU
    {"周波数校正",      MENU_F_CALI        },
#endif
    {"電池校正",        MENU_BATCAL        },
    {"電池種類",        MENU_BATTYP        },
#ifndef ENABLE_FIXED_NAV_K5
    {"キー方向",        MENU_SET_NAV       },
#endif
    {"初期化",          MENU_RESET         },

    {"",                              0xff               }  // end of list - DO NOT delete or move this this
};

const uint8_t FIRST_HIDDEN_MENU_ITEM = MENU_F_LOCK;

const char* const gSubMenu_TXP[] =
{
    "手動",
    "弱 1",
    "弱 2",
    "弱 3",
    "弱 4",
    "弱 5",
    "中",
    "強"
};

const char* const gSubMenu_SFT_D[] =
{
    "オフ",
    "+",
    "-"
};

const char* const gSubMenu_W_N[] =
{
    "ワイド",
    "ナロー"
};

const char* const gSubMenu_OFF_ON[] =
{
    "オフ",
    "オン"
};

const char* gSubMenu_NA = "非対応";

const char* const gSubMenu_RXMode[] =
{
    "メイン\nのみ",       // TX and RX on main only
    "両方受信\n応答",     // Watch both and respond
    "クロス\nバンド",     // TX on main, RX on secondary
    "送信メイン\n両方受信" // always TX on main, but RX on both
};

#ifdef ENABLE_VOICE
    const char* const gSubMenu_VOICE[] =
    {
        "オフ",
        "中国語",
        "英語"
    };
#endif

const char* const gSubMenu_MDF[] =
{
    "周波数",
    "CH番号",
    "名称",
    "名称\n+\n周波数"
};

#ifdef ENABLE_ALARM
    const char* const gSubMenu_AL_MOD[] =
    {
        "現場",
        "トーン"
    };
#endif

#ifdef ENABLE_DTMF_CALLING
const char* const gSubMenu_D_RSP[] =
{
    "なにも\nしない",
    "呼出音",
    "応答",
    "両方"
};
#endif

const char* const gSubMenu_PTT_ID[] =
{
    "オフ",
    "送信前",
    "送信後",
    "送信\n前と後",
    "APOLLO\nQUINDAR"
};

const char* const gSubMenu_PONMSG[] =
{
#ifdef ENABLE_FEAT_F4HWN
    "すべて",
    "音のみ",
#else
    "すべて",
#endif
    "メッセージ",
    "電圧",
#ifdef ENABLE_FEAT_F4HWN_LOGO
    "ロゴ",
#endif
    "なし"
};

#if defined(ENABLE_FEAT_F4HWN) && defined(ENABLE_FEAT_F4HWN_LOGO_SAV)
const char* const gSubMenu_SET_SAV[] =
{
    "オフ",
    "ロゴ",
    "ロゴ+",
    "マトリクス"
};
#endif

const char* const gSubMenu_ROGER[] =
{
    "オフ",
    "ロジャー",
    "MDC"
};

const char* const gSubMenu_RESET[] =
{
    "VFOのみ",
    "すべて"
};

const char* const gSubMenu_F_LOCK[] =
{
    "標準+\n137-174\n400-470",
    "米国\n144-148\n420-450",
#ifdef ENABLE_FEAT_F4HWN_CA
    "カナダ\n144-148\n430-450",
#endif
    "欧州/日本\n144-146\n430-440",
    "英国\n144-148\n430-440",
    "137-174\n400-430",
    "137-174\n400-438",
#ifdef ENABLE_FEAT_F4HWN_PMR
    "PMR 446",
#endif
#ifdef ENABLE_FEAT_F4HWN_GMRS_FRS_MURS
    "GMRS\nFRS\nMURS",
#endif
    "送信\n全面禁止",
    "全周波数\n送信可",
};

const char* const gSubMenu_RX_TX[] =
{
    "オフ",
    "送信",
    "受信",
    "送受信"
};

const char* const gSubMenu_BAT_TXT[] =
{
    "なし",
    "電圧",
    "残量%"
};

const char* const gSubMenu_BATTYP[] =
{
    "1600mAh K5",
    "2200mAh K5",
    "3500mAh K5",
    "1400mAh K1",
    "2500mAh K1"
};

const char* const gSubMenu_SET_NAV[] =
{
    "左右\nUV-K1",
    "上下\nUV-K5(8)",
};

#ifndef ENABLE_FEAT_F4HWN
const char* const gSubMenu_SCRAMBLER[] =
{
    "オフ",
    "2600Hz",
    "2700Hz",
    "2800Hz",
    "2900Hz",
    "3000Hz",
    "3100Hz",
    "3200Hz",
    "3300Hz",
    "3400Hz",
    "3500Hz"
};
#endif

#ifdef ENABLE_FEAT_F4HWN
    const char* const gSubMenu_SET_PWR[] =
    {
        "< 20m",
        "125m",
        "250m",
        "500m",
        "1",
        "2",
        "5"
    };

    const char* const gSubMenu_SET_PTT[] =
    {
        "押し続け",
        "ワンプッシュ"
    };

    const char* const gSubMenu_SET_TOT[] =
    {
        "オフ",
        "音",
        "画面",
        "両方"
    };

    const char* const gSubMenu_SET_LCK[] =
    {
        "キー",
        "キー\n機能",
        "キー\nPTT",
        "キー\n機能\nPTT"
    };

    const char* const gSubMenu_SET_MET[] =
    {
        "小型",
        "標準"
    };

    #ifdef ENABLE_FEAT_F4HWN_SCAN_FASTER
        const char* const gSubMenu_SET_SCN[] =
        {
            "標準",
            "高速"
        };
    #endif

    #ifdef ENABLE_FEAT_F4HWN_AUDIO
        // MUST STAY ASCII. Besides the menu, these two tables are drawn on the
        // main screen by GUI_DisplaySmallest() (ui/main.c), which uses the 3x5
        // pixel font - far too small for Japanese, and it has no glyphs for it.
        const char* const gSubMenu_SET_AUD_FM[] =
        {
            "FLAT",
            "CLEAN",
            "MID",
            "BOOST",
            "MAX"
        };

        const char* const gSubMenu_SET_AUD_AM[] =
        {
            "SHARP",
            "STOCK",
            "OPEN"
        };
    #endif

    #ifdef ENABLE_FEAT_F4HWN_NARROWER
        const char* const gSubMenu_SET_NFM[] =
        {
            "ナロー",
            "より狭い"
        };
    #endif

    #ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
        const char* const gSubMenu_SET_KEY[] =
        {
            "KEY_MENU",
            "KEY_UP",
            "KEY_DOWN",
            "KEY_EXIT",
            "KEY_STAR"
        };
    #endif
#endif

const t_sidefunction gSubMenu_SIDEFUNCTIONS[] =
{
    {"なし",            ACTION_OPT_NONE},
#ifdef ENABLE_FLASHLIGHT
    {"ライト",          ACTION_OPT_FLASHLIGHT},
#endif
    {"送信出力",        ACTION_OPT_POWER},
    {"モニター",        ACTION_OPT_MONITOR},
    {"スキャン",        ACTION_OPT_SCAN},
#ifdef ENABLE_VOX
    {"VOX",             ACTION_OPT_VOX},
#endif
#ifdef ENABLE_ALARM
    {"警報",            ACTION_OPT_ALARM},
#endif
#ifdef ENABLE_FMRADIO
    {"FMラジオ",        ACTION_OPT_FM},
#endif
#ifdef ENABLE_TX1750
    {"1750Hz",          ACTION_OPT_1750},
#endif
    {"キー\nロック",    ACTION_OPT_KEYLOCK},
    {"VFO A\nVFO B",    ACTION_OPT_A_B},
    {"VFO\nメモリ",     ACTION_OPT_VFO_MR},
    {"変調方式",        ACTION_OPT_SWITCH_DEMODUL},
#ifdef ENABLE_BLMIN_TMP_OFF
    {"輝度最小\n一時", ACTION_OPT_BLMIN_TMP_OFF},
#endif
#ifdef ENABLE_FEAT_F4HWN
    {"受信方式",        ACTION_OPT_RXMODE},
    {"メイン\nのみ",    ACTION_OPT_MAINONLY},
    {"PTT",             ACTION_OPT_PTT},
    {"帯域幅",          ACTION_OPT_WN},
    {"ミュート",        ACTION_OPT_MUTE},
#ifdef ENABLE_REG_DUMP_SCREEN
    {"レジスタ",        ACTION_OPT_REGDUMP},
#endif
#ifdef ENABLE_SI4732
    {"短波受信",        ACTION_OPT_SI4732},
#endif
    #ifdef ENABLE_FEAT_F4HWN_AUDIO
        {"音質",           ACTION_OPT_RXA},
    #endif
    #ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
        {"出力\n最大",     ACTION_OPT_POWER_HIGH},
        {"シフト\n解除",   ACTION_OPT_REMOVE_OFFSET},
    #endif
    #ifdef ENABLE_FEAT_F4HWN_BEAM
        {"BEAM",            ACTION_OPT_BEAM},
    #endif
    #ifdef ENABLE_FEAT_F4HWN_RXTX_LOG
        {"RF LOG",          ACTION_OPT_RXTX_LOG},
    #endif
    #ifdef ENABLE_FEAT_F4HWN_FOXHUNT
        {"FOX HUNT\nBEACON", ACTION_OPT_FOXHUNT},
    #endif
#endif
};

const uint8_t gSubMenu_SIDEFUNCTIONS_size = ARRAY_SIZE(gSubMenu_SIDEFUNCTIONS);
