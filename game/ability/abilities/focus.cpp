/****************************************
 * @file   focus.cpp
 * @brief  集中の実装
 * @author Natsume Shidara
 * @date   2026/05/22
 ****************************************/
#include "focus.h"
#include "game_state.h"

//======================================
// 構築
//======================================
FocusAbility::FocusAbility()
{
    name        = "集中";
    description = "クリックで\nこのターン +2分";
    rarity      = Rarity::Common;
    activatable = true;   // 任意発動アビリティとして登録
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
