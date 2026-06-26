/****************************************
 * @file   commons.cpp
 * @brief  コモンアビリティ群の実装
 * @author Natsume Shidara
 * @date   2026/06/25
 ****************************************/
#include "commons.h"

#include "game_state.h"

#include <cstdio>

namespace
{
    /** @brief "毎ターン思考時間\n+N秒 (恒久)" の説明文を作る */
    std::string FlatDesc(int sec)
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "毎ターン思考時間\n+%d秒 (恒久)", sec);
        return buf;
    }

    /** @brief "クリックで\nこのターン +M:SS" の説明文を作る */
    std::string BurstDesc(int sec)
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "クリックで\nこのターン +%d:%02d", sec / 60, sec % 60);
        return buf;
    }

    /** @brief "+M:SS" のフラッシュ文言を作る */
    std::string BurstFlash(int sec)
    {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "+%d:%02d", sec / 60, sec % 60);
        return buf;
    }
}

//======================================
// 加算系 (熟考派生): 名前・加算量・説明だけ上書き
//======================================
ReadyMindCommon::ReadyMindCommon()
{
    name = "心構え"; rarity = Rarity::Common; bonusSeconds = 20.0;
    description = FlatDesc(static_cast<int>(bonusSeconds));
}
CalmMindCommon::CalmMindCommon()
{
    name = "平常心"; rarity = Rarity::Common; bonusSeconds = 25.0;
    description = FlatDesc(static_cast<int>(bonusSeconds));
}
BreatherCommon::BreatherCommon()
{
    name = "一服"; rarity = Rarity::Common; bonusSeconds = 40.0;
    description = FlatDesc(static_cast<int>(bonusSeconds));
}
DeepPlanCommon::DeepPlanCommon()
{
    name = "深謀遠慮"; rarity = Rarity::Common; bonusSeconds = 50.0;
    description = FlatDesc(static_cast<int>(bonusSeconds));
}

//======================================
// 拾い読み (乗算)
//======================================
SkimCommon::SkimCommon()
{
    name        = "拾い読み";
    rarity      = Rarity::Common;
    unique      = true;   // 乗算の多重掛けは過剰になるため一度限り
    description = "思考時間が\n常に1.2倍になる";
}
double SkimCommon::GetTurnTimeSeconds(Piece player)
{
    const double base = chained ? chained->GetTurnTimeSeconds(player) : baseSeconds;
    return (player == targetSide) ? base * factor : base;
}

//======================================
// 任意発動・加算系 (集中派生): 名前・回数・加算量・説明を上書き
//======================================
ReserveCommon::ReserveCommon()
{
    name = "底力"; rarity = Rarity::Common; charges = 2; bonusSeconds = 90.0;
    description = BurstDesc(static_cast<int>(bonusSeconds));
    flashText   = BurstFlash(static_cast<int>(bonusSeconds));
}
ShortRestCommon::ShortRestCommon()
{
    name = "小休止"; rarity = Rarity::Common; charges = 3; bonusSeconds = 45.0;
    description = BurstDesc(static_cast<int>(bonusSeconds));
    flashText   = BurstFlash(static_cast<int>(bonusSeconds));
}
ReconsiderCommon::ReconsiderCommon()
{
    name = "再考"; rarity = Rarity::Common; charges = 1; bonusSeconds = 180.0;
    description = BurstDesc(static_cast<int>(bonusSeconds));
    flashText   = BurstFlash(static_cast<int>(bonusSeconds));
}
QuickSightCommon::QuickSightCommon()
{
    name = "早見え"; rarity = Rarity::Common; charges = 2; bonusSeconds = 120.0;
    description = BurstDesc(static_cast<int>(bonusSeconds));
    flashText   = BurstFlash(static_cast<int>(bonusSeconds));
}

//======================================
// 見極め (残時間を floor まで補充)
//======================================
DiscernCommon::DiscernCommon()
{
    name        = "見極め";
    rarity      = Rarity::Common;
    charges     = 2;
    floorSeconds = 100.0;
    description = "クリックで残り時間を\n100秒まで補充";
    flashText   = "時間補充";
}
void DiscernCommon::Activate(GameState& state)
{
    if (charges <= 0) return;
    if (state.remainingTime < floorSeconds) state.remainingTime = floorSeconds;
    --charges;
}
bool DiscernCommon::CanActivate(const GameState& state) const
{
    return charges > 0 && state.remainingTime < floorSeconds;
}
