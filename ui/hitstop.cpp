/****************************************
 * @file hitstop.cpp
 * @brief ヒットストップ（時間停止演出）の実装
 * @detail
 * - 攻撃命中時にHitStop_Trigger()で発動
 * - FREEZE_DURATIONの間、時間スケールをほぼ0にする
 * - その後RECOVERY_DURATIONかけてスケールを1.0に復帰
 * - カメラシェイクも同時に発動
 *
 * @author Natsume Shidara
 * @date 2026/02/22
 ****************************************/

#include "hitstop.h"
#include "player_camera.h"
#include <algorithm>
#include <cmath>

// 定数
namespace
{
    // フリーズ持続時間（軽めで爽快感優先）
    constexpr float FREEZE_DURATION   = 0.025f;  // 25ms

    // 復帰時間（速めでテンポ維持）
    constexpr float RECOVERY_DURATION = 0.02f;   // 20ms

    // フリーズ中の最小スケール（完全0は物理計算に誤差が出る）
    constexpr float MIN_TIME_SCALE    = 0.05f;

    // カメラシェイク強度
    constexpr float HIT_SHAKE_POWER   = 0.3f;
}

// フェーズ
enum class HitStopPhase
{
    Inactive,   // 通常再生
    Freeze,     // フリーズ中
    Recovery    // 復帰中
};

// 内部変数
static HitStopPhase g_Phase     = HitStopPhase::Inactive;
static float        g_Timer     = 0.0f;
static float        g_TimeScale = 1.0f;

// 初期化・終了
void HitStop_Initialize()
{
    g_Phase     = HitStopPhase::Inactive;
    g_Timer     = 0.0f;
    g_TimeScale = 1.0f;
}

void HitStop_Finalize()
{
    g_Phase     = HitStopPhase::Inactive;
    g_Timer     = 0.0f;
    g_TimeScale = 1.0f;
}

// トリガー
void HitStop_Trigger()
{
    // フリーズ開始（既にフリーズ中ならタイマーリセットで延長）
    g_Phase     = HitStopPhase::Freeze;
    g_Timer     = 0.0f;
    g_TimeScale = MIN_TIME_SCALE;

    // カメラシェイクで衝撃感
    PLCamera_Shake(HIT_SHAKE_POWER);
}

// 更新（実時間ベース）
void HitStop_Update(float raw_dt)
{
    switch (g_Phase)
    {
    case HitStopPhase::Inactive:
        g_TimeScale = 1.0f;
        break;

    case HitStopPhase::Freeze:
        g_Timer += raw_dt;
        g_TimeScale = MIN_TIME_SCALE;

        if (g_Timer >= FREEZE_DURATION)
        {
            g_Phase = HitStopPhase::Recovery;
            g_Timer = 0.0f;
        }
        break;

    case HitStopPhase::Recovery:
        g_Timer += raw_dt;
        {
            float t = std::fminf(g_Timer / RECOVERY_DURATION, 1.0f);

            // 二次イーズアウト: 序盤が立ち上がり代わらかに1.0へ到達
            float eased = 1.0f - (1.0f - t) * (1.0f - t);
            g_TimeScale = MIN_TIME_SCALE + (1.0f - MIN_TIME_SCALE) * eased;
        }

        if (g_Timer >= RECOVERY_DURATION)
        {
            g_Phase     = HitStopPhase::Inactive;
            g_Timer     = 0.0f;
            g_TimeScale = 1.0f;
        }
        break;
    }
}

// 状態取得
float HitStop_GetTimeScale()
{
    return g_TimeScale;
}

bool HitStop_IsActive()
{
    return g_Phase != HitStopPhase::Inactive;
}
