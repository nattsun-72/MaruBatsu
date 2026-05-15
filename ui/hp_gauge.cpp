/****************************************
 * @file hp_gauge.cpp
 * @brief HPゲージ表示の処理
 * @author Natsume Shidara
 * @date 2025/08/15
 * @update 2026/01/12 - 斬空アクションゲーム用に改修
 ****************************************/

#include "hp_gauge.h"
#include "texture.h"
#include "sprite.h"
#include "direct3d.h"
#include "player.h"
#include "fever.h"

#include <algorithm>
#include <cmath>
#include <DirectXMath.h>

#ifdef USE_ASSET_DLL
#include "asset_dll.h"
#include "resource.h"
#endif

using namespace DirectX;

// 定数
namespace
{
    // テクスチャパス
    constexpr const wchar_t* HEART_TEXTURE_PATH = L"assets/ui/heart.png";

    // ハートテクスチャのUV設定（横に3つ並んでいる: 満、半分、空）
    constexpr int HEART_SRC_WIDTH = 12;
    constexpr int HEART_SRC_HEIGHT = 12;

    // 描画サイズ
    constexpr float HEART_DRAW_WIDTH = 64.0f;   // 48→64 大きく
    constexpr float HEART_DRAW_HEIGHT = 64.0f;  // 48→64

    // 表示位置（画面左下）
    constexpr float MARGIN_X = 24.0f;
    constexpr float MARGIN_Y = 24.0f;

    // アニメーション
    constexpr float DAMAGE_FLASH_DURATION = 0.5f;
    constexpr float LOW_HP_PULSE_SPEED = 4.0f;
    constexpr int LOW_HP_THRESHOLD = 3;

    // 回復ウェーブアニメーション
    constexpr float RECOVER_WAVE_SPEED = 7.0f;      // ウェーブの速さ
    constexpr float RECOVER_WAVE_AMPLITUDE = 8.0f;   // 上下の振れ幅（ピクセル）
    constexpr float RECOVER_WAVE_DELAY = 0.12f;      // 各ハート間のウェーブ遅延
    constexpr float RECOVER_FLASH_DURATION = 1.0f;   // 回復アニメーション持続時間

    // HP計算
    constexpr int HP_PER_HEART = 2;                   // 1ハートあたりのHP量

    // 低HP脈動
    constexpr float LOW_HP_PULSE_AMPLITUDE = 0.1f;    // 低HP時の脈動振幅

    // ハート間隔
    constexpr float HEART_SPACING = 6.0f;             // ハート間のピクセル間隔

    // 回復ウェーブ色パラメータ
    constexpr float RECOVER_GREEN_BOOST = 0.15f;      // 回復中のGreen成分加算量
    constexpr float RECOVER_RB_REDUCTION = 0.1f;      // 回復中のRed/Blue成分減算量
    constexpr float RECOVER_WAVE_HALF = 0.5f;         // 回復中ウェーブの振幅倍率

    // 回復バウンスパラメータ
    constexpr float BOUNCE_DURATION_RATIO = 0.6f;     // バウンス持続時間の比率
    constexpr float BOUNCE_SPEED_MULTIPLIER = 2.0f;   // バウンスの速度倍率
    constexpr float BOUNCE_FLASH_INTENSITY = 0.4f;    // バウンスフラッシュの強度
    constexpr float BOUNCE_FLASH_RB_RATIO = 0.3f;     // バウンスフラッシュのRed/Blue減算比率
}

// 内部変数
static int g_HeartTexId = -1;
static XMFLOAT2 g_HpGaugePos = { 0.0f, 0.0f };

// アニメーション用
static int g_PrevHP = 0;
static float g_DamageFlashTimer = 0.0f;
static float g_PulseTimer = 0.0f;

// 回復ウェーブ用
static float g_RecoverWaveTimer = -1.0f;   // -1 = 非アクティブ
static bool  g_IsRecovering = false;       // 現在回復中か

// 初期化
void HpGauge_Initialize()
{
#ifdef USE_ASSET_DLL
    {
        const void* pData = nullptr;
        size_t dataSize = 0;
        if (AssetDLL_GetData(IDR_TEX_UI_HEART, &pData, &dataSize))
            g_HeartTexId = Texture_LoadFromMemory(pData, dataSize, L"hp_heart");
    }
#else
    g_HeartTexId = Texture_Load(HEART_TEXTURE_PATH);
#endif

    // 画面サイズから位置を計算（左下）
    float screenHeight = static_cast<float>(Direct3D_GetBackBufferHeight());
    g_HpGaugePos.x = MARGIN_X;
    g_HpGaugePos.y = screenHeight - MARGIN_Y - HEART_DRAW_HEIGHT;

    g_PrevHP = Player_GetHP();
    g_DamageFlashTimer = 0.0f;
    g_PulseTimer = 0.0f;
    g_RecoverWaveTimer = -1.0f;
    g_IsRecovering = false;
}

// 終了処理
void HpGauge_Finalize()
{
    // テクスチャはTexture_AllRelease()で解放
    g_HeartTexId = -1;
}

// 更新
void HpGauge_Update(double elapsed_time)
{
    float dt = static_cast<float>(elapsed_time);
    int currentHP = Player_GetHP();
    int maxHP = Player_GetMaxHP();

    // ダメージ検出
    if (currentHP < g_PrevHP)
    {
        g_DamageFlashTimer = DAMAGE_FLASH_DURATION;
    }

    // 回復検出（HPが増えたらウェーブ発動）
    if (currentHP > g_PrevHP)
    {
        g_RecoverWaveTimer = 0.0f;
    }

    g_PrevHP = currentHP;

    // 回復中フラグ（Fever Tier2+ かつ HP未満）
    g_IsRecovering = (Fever_GetTier() >= FeverTier::Tier2) && (currentHP < maxHP);

    // タイマー更新
    if (g_DamageFlashTimer > 0.0f)
    {
        g_DamageFlashTimer -= dt;
    }

    // 回復ウェーブタイマー更新
    if (g_RecoverWaveTimer >= 0.0f)
    {
        g_RecoverWaveTimer += dt;
        if (g_RecoverWaveTimer >= RECOVER_FLASH_DURATION)
        {
            g_RecoverWaveTimer = -1.0f; // アニメーション終了
        }
    }

    // 脈動タイマー
    g_PulseTimer += dt * LOW_HP_PULSE_SPEED;
}

// 描画
void HpGauge_Draw()
{
    if (g_HeartTexId < 0) return;

    // 現在HPと最大HPを取得
    int currentHP = Player_GetHP();
    int maxHP = Player_GetMaxHP();

    // ハートの数（1ハート = HP_PER_HEART HP）
    int heartCount = (maxHP + 1) / HP_PER_HEART;

    // 描画色
    XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

    // ダメージフラッシュ時
    if (g_DamageFlashTimer > 0.0f)
    {
        float flash = g_DamageFlashTimer / DAMAGE_FLASH_DURATION;
        color.y = 1.0f - flash * 0.5f;
        color.z = 1.0f - flash * 0.5f;
    }

    // 低HP時の脈動
    float scale = 1.0f;
    if (currentHP <= LOW_HP_THRESHOLD && currentHP > 0)
    {
        scale = 1.0f + sinf(g_PulseTimer) * LOW_HP_PULSE_AMPLITUDE;
    }

    Sprite_Begin();

    for (int i = 0; i < heartCount; i++)
    {
        // 各ハートが表すHP範囲 [HP_PER_HEART*i, HP_PER_HEART*i+HP_PER_HEART)
        int hpSegment = currentHP - (i * HP_PER_HEART);

        // UV切り抜き位置を決定
        float srcX = 0.0f;
        if (hpSegment >= HP_PER_HEART)
        {
            // HP_PER_HEART以上残っている → 満タンハート
            srcX = static_cast<float>(HEART_SRC_WIDTH * HP_PER_HEART);
        }
        else if (hpSegment == 1)
        {
            // 残り1つ残っている → 半分ハート
            srcX = static_cast<float>(HEART_SRC_WIDTH);
        }
        else
        {
            // 0以下 → 空ハート
            srcX = 0.0f;
        }

        // 描画位置
        float drawX = g_HpGaugePos.x + i * (HEART_DRAW_WIDTH + HEART_SPACING);
        float drawY = g_HpGaugePos.y;

        // 回復中ウェーブ（左から順に遅延する上下アニメーション）
        float heartOffsetY = 0.0f;
        XMFLOAT4 heartColor = color;

        if (g_IsRecovering && hpSegment > 0)
        {
            // 回復中の常時ウェーブ（ゆっくり波打つ）
            float phase = g_PulseTimer * RECOVER_WAVE_SPEED - i * RECOVER_WAVE_DELAY * RECOVER_WAVE_SPEED;
            heartOffsetY = sinf(phase) * RECOVER_WAVE_AMPLITUDE * RECOVER_WAVE_HALF;

            // 緑がかった色味を追加（回復中の視覚表現）
            heartColor.y = (std::min)(1.0f, heartColor.y + RECOVER_GREEN_BOOST);
            heartColor.x = (std::max)(0.0f, heartColor.x - RECOVER_RB_REDUCTION);
            heartColor.z = (std::max)(0.0f, heartColor.z - RECOVER_RB_REDUCTION);
        }

        if (g_RecoverWaveTimer >= 0.0f && hpSegment > 0)
        {
            // 回復瞬間のポップウェーブ（左から順に大きくバウンド）
            float heartDelay = i * RECOVER_WAVE_DELAY;
            float localTime = g_RecoverWaveTimer - heartDelay;

            if (localTime > 0.0f && localTime < RECOVER_FLASH_DURATION * BOUNCE_DURATION_RATIO)
            {
                // 減衰バウンド
                float progress = localTime / (RECOVER_FLASH_DURATION * BOUNCE_DURATION_RATIO);
                float decay = 1.0f - progress;
                float bounce = sinf(localTime * RECOVER_WAVE_SPEED * BOUNCE_SPEED_MULTIPLIER) * decay;
                heartOffsetY += bounce * RECOVER_WAVE_AMPLITUDE;

                // 明るい緑フラッシュ
                float flash = decay * BOUNCE_FLASH_INTENSITY;
                heartColor.y = (std::min)(1.0f, heartColor.y + flash);
                heartColor.x = (std::max)(0.0f, heartColor.x - flash * BOUNCE_FLASH_RB_RATIO);
                heartColor.z = (std::max)(0.0f, heartColor.z - flash * BOUNCE_FLASH_RB_RATIO);
            }
        }

        drawY += heartOffsetY;

        // 低HP時のスケール適用
        float drawW = HEART_DRAW_WIDTH * scale;
        float drawH = HEART_DRAW_HEIGHT * scale;

        // スケール時は中心を保持
        if (scale != 1.0f)
        {
            drawX -= (drawW - HEART_DRAW_WIDTH) * 0.5f;
            drawY -= (drawH - HEART_DRAW_HEIGHT) * 0.5f;
        }

        // 描画（フルスペックで）
        // 引数順: texid, x, y, uvX, uvY, uvW, uvH, drawW, drawH, angle, color
        Sprite_Draw(
            g_HeartTexId,
            drawX, drawY,
            srcX, 0.0f,                                    // UV切り抜き位置
            static_cast<float>(HEART_SRC_WIDTH),           // UV幅
            static_cast<float>(HEART_SRC_HEIGHT),          // UV高さ
            drawW, drawH,                                  // 描画サイズ
            0.0f,                                          // 角度
            heartColor                                     // 色（回復アニメ反映）
        );
    }
}