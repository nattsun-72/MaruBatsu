#pragma once
// ============================================================
//  AssetDLL - Resource ID Definitions
// ============================================================
//
//  各アセットに固有のID番号を割り当てる。
//  番号帯:
//    1000番台 — テクスチャ
//    2000番台 — 3Dモデル
//    3000番台 — サウンド (BGM / SE)
//    4000番台 — シェーダー (.cso)
//
//  使い方:
//    DLL側: assets.rc でこのIDとファイルを紐付ける
//    ゲーム側: AssetDLL_GetData(IDR_TEX_WHITE, &data, &size) で取得
// ============================================================

// ── テクスチャ (1000番台) ─────────────────────────────────
// 汎用テクスチャ
#define IDR_TEX_WHITE                   1001
#define IDR_TEX_EXPLOSION               1002
#define IDR_TEX_EFFECT000               1003
#define IDR_TEX_FLOOR                   1004
#define IDR_TEX_WALL                    1005
#define IDR_TEX_CONSOLAB                1006

// UIテクスチャ
#define IDR_TEX_UI_NUMBERS              1101
#define IDR_TEX_UI_HEART                1102
#define IDR_TEX_UI_HEART_FULL           1111
#define IDR_TEX_UI_HEART_EMPTY          1112
#define IDR_TEX_UI_COMBO_TEXT           1103
#define IDR_TEX_UI_PRESS_START          1104
#define IDR_TEX_UI_RESULT_BG            1105
#define IDR_TEX_UI_RESULT_TITLE         1106
#define IDR_TEX_UI_SETTINGS_BG          1107
#define IDR_TEX_UI_SETTINGS_TITLE       1108
#define IDR_TEX_UI_TITLE_BG             1109
#define IDR_TEX_UI_TITLE_LOGO           1110

// 敵スライステクスチャ
#define IDR_TEX_ENEMY_SL                1201
#define IDR_TEX_ENEMY_SL_HL             1202
#define IDR_TEX_DRONE_DANMEN            1203

// LongSword PBRテクスチャ
#define IDR_TEX_SWORD_BASECOLOR         1301
#define IDR_TEX_SWORD_NORMAL            1302
#define IDR_TEX_SWORD_METALLIC          1303
#define IDR_TEX_SWORD_ROUGHNESS         1304
#define IDR_TEX_SWORD_EMISSION          1305
#define IDR_TEX_SWORD_ALPHA             1306
#define IDR_TEX_SWORD_DISPLACEMENT      1307
#define IDR_TEX_SWORD_BAKE              1308

// OptionBooster PBRテクスチャ
#define IDR_TEX_BOOSTER_BASECOLOR       1401
#define IDR_TEX_BOOSTER_NORMAL          1402
#define IDR_TEX_BOOSTER_METALLIC        1403
#define IDR_TEX_BOOSTER_ROUGHNESS       1404
#define IDR_TEX_BOOSTER_EMISSION        1405
#define IDR_TEX_BOOSTER_ALPHA           1406
#define IDR_TEX_BOOSTER_DISPLACEMENT    1407
#define IDR_TEX_BOOSTER_BAKE            1408
#define IDR_TEX_BOOSTER_BAKE2           1409

// ── 3Dモデル (2000番台) ──────────────────────────────────
#define IDR_MODEL_BOX                   2001
#define IDR_MODEL_BULLET                2002
#define IDR_MODEL_ROBOT                 2003
#define IDR_MODEL_SKY                   2004
#define IDR_MODEL_LONGSWORD             2005
#define IDR_MODEL_OPTION_BOOSTER        2006

// ── サウンド (3000番台) ──────────────────────────────────
// BGM
#define IDR_SND_BGM_TITLE               3001
#define IDR_SND_BGM_GAME                3002
#define IDR_SND_BGM_RESULT              3003

// SE
#define IDR_SND_SE_SLASH                3101
#define IDR_SND_SE_SLASH_HEAVY          3102
#define IDR_SND_SE_JUMP                 3103
#define IDR_SND_SE_STEP                 3104
#define IDR_SND_SE_DECIDE               3105
#define IDR_SND_SE_SELECT               3106
#define IDR_SND_SE_CANCEL               3107
#define IDR_SND_SE_TITLE_SLASH          3108
#define IDR_SND_SE_ENEMY_FIRE           3109
#define IDR_SND_SE_SLASH_HIT            3112
#define IDR_SND_SE_PLAYER_DAMAGE        3113
#define IDR_SND_SE_DOUBLE_JUMP          3114
#define IDR_SND_SE_DASH_CHARGE          3115
#define IDR_SND_SE_DASH                 3116
#define IDR_SND_SE_LAND                 3117
#define IDR_SND_SE_COMBO_1              3118
#define IDR_SND_SE_COMBO_2              3119
#define IDR_SND_SE_COMBO_3              3120
#define IDR_SND_SE_COMBO_4              3121
#define IDR_SND_SE_ENEMY_DEATH          3122
#define IDR_SND_SE_ENEMY_SLICE          3123
#define IDR_SND_SE_FLYING_CHARGE        3110
#define IDR_SND_SE_FLYING_WHOOSH        3111
#define IDR_SND_SE_PAUSE                3124
#define IDR_SND_SE_SCORE_COUNT          3125
#define IDR_SND_SE_SCORE_FINISH         3126

// ── コンパイル済みシェーダー (4000番台) ──────────────────
// Vertex Shaders
#define IDR_SHADER_VS_2D                4001
#define IDR_SHADER_VS_3D                4002
#define IDR_SHADER_VS_3D_UNLIT          4003
#define IDR_SHADER_VS_SKINNED           4004
#define IDR_SHADER_VS_BILLBOARD         4005
#define IDR_SHADER_VS_FIELD             4006
#define IDR_SHADER_VS_POSTPROCESS       4007

// Pixel Shaders
#define IDR_SHADER_PS_2D                4101
#define IDR_SHADER_PS_3D                4102
#define IDR_SHADER_PS_3D_UNLIT          4103
#define IDR_SHADER_PS_SKINNED           4104
#define IDR_SHADER_PS_BILLBOARD         4105
#define IDR_SHADER_PS_FIELD             4106
#define IDR_SHADER_PS_PP_PASSTHROUGH    4107
#define IDR_SHADER_PS_PP_RADIAL         4108
#define IDR_SHADER_PS_PP_DIRECTIONAL    4109
#define IDR_SHADER_PS_PP_BLOOM_DOWN     4110
#define IDR_SHADER_PS_PP_BLOOM_BLUR     4111
#define IDR_SHADER_PS_PP_BLOOM_COMP     4112

// HLSL Source (ランタイムコンパイル用)
#define IDR_SHADER_HLSL_SHADOW_MAP          4201
#define IDR_SHADER_HLSL_SHADOW_MAP_SKINNED  4202
