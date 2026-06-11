/****************************************
 * @file   immortal.cpp
 * @brief  不死の実装
 * @author Natsume Shidara
 * @date   2026/06/11
 ****************************************/
#include "immortal.h"

#include "ability/ability_registry.h"
#include "config.h"

//======================================
// 構築
//======================================
ImmortalAbility::ImmortalAbility()
{
    name        = "不死";
    description = "敗北を打ち消す\n(ラン中の残回数制)";
    rarity      = Rarity::Legendary;
    unique      = true;   // チャージ制のため重複所持は不可
    charges     = Config::GetInt("abilities.immortal.charges", 2);
}

//======================================
// IDefeatHandler
//======================================
bool ImmortalAbility::OnDefeat(GameState& /*state*/, Piece loser)
{
    if (loser != Piece::Player) return false;   // 守るのはプレイヤーのみ
    if (charges <= 0)           return false;   // チャージ切れ

    --charges;
    return true;   // 敗北を打ち消す (後処理はゲームシーン側)
}

//======================================
// Ability
//======================================
void ImmortalAbility::RegisterTo(AbilityRegistry& registry)
{
    auto self = std::dynamic_pointer_cast<IDefeatHandler>(shared_from_this());
    registry.AddDefeatHandler(self);   // 敗北ハンドラとして追加登録
}
