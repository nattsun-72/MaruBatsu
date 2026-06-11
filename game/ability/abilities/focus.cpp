/****************************************
 * @file   focus.cpp
 * @brief  集中の実装
 * @author Natsume Shidara
 * @date   2026/05/22
 ****************************************/
#include "focus.h"
#include "game_state.h"
#include "config.h"

#include <cstdio>

//======================================
// 構築
//======================================
FocusAbility::FocusAbility()
{
    name         = "集中";
    rarity       = Rarity::Common;
    activatable  = true;   // 任意発動アビリティとして登録
    charges      = Config::GetInt("abilities.focus.charges", 3);
    bonusSeconds = Config::GetDouble("abilities.focus.bonusSeconds", 120.0);

    const int m = static_cast<int>(bonusSeconds) / 60;
    const int s = static_cast<int>(bonusSeconds) % 60;
    char buf[64];
    std::snprintf(buf, sizeof(buf), "クリックで\nこのターン +%d:%02d", m, s);
    description = buf;
    std::snprintf(buf, sizeof(buf), "+%d:%02d", m, s);
    flashText   = buf;   // 発動時のHUDフラッシュ表示
}

//======================================
// 任意発動アビリティのインターフェイス
//======================================
void FocusAbility::Activate(GameState& state)
{
    if (charges <= 0) return;             // チャージ切れなら何もしない
    state.remainingTime += bonusSeconds;  // このターンの残り時間を加算
    --charges;
}

bool FocusAbility::CanActivate(const GameState& /*state*/) const
{
    return charges > 0;
}
