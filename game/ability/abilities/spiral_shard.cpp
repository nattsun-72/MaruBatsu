/****************************************
 * @file   spiral_shard.cpp
 * @brief  螺旋の欠片の実装
 * @author Natsume Shidara
 * @date   2026/06/11
 ****************************************/
#include "spiral_shard.h"

#include "board_ops.h"
#include "config.h"
#include "game_state.h"

//======================================
// 構築
//======================================
SpiralShardAbility::SpiralShardAbility()
{
    name          = "螺旋の欠片";
    description   = "クリックで盤面を\n時計回りに90度回転";
    rarity        = Rarity::Rare;
    bossExclusive = true;   // 螺旋の女王 撃破でのみ獲得
    activatable   = true;
    flashText     = "盤面が回転！";
    // ボス撃破報酬として強化(回数制限を緩和)。値は game_config.json で調整可。
    charges       = Config::GetInt("abilities.spiralShard.charges", 4);
}

//======================================
// 任意発動アビリティのインターフェイス
//======================================
void SpiralShardAbility::Activate(GameState& state)
{
    if (charges <= 0) return;
    --charges;

    // 盤面を回転し、移動を効果フェーズとして記録する。
    // アニメ再生と勝敗の再評価は、呼び出し元(ゲームシーン)が行う。
    BoardOps::MoveList moves;
    BoardOps::Rotate90(state.board, clockwise, &moves);
    if (!moves.empty())
    {
        state.animPhases.push_back(std::move(moves));
    }
}

bool SpiralShardAbility::CanActivate(const GameState& /*state*/) const
{
    return charges > 0;
}
