/****************************************
 * @file   boss_ice_slide.cpp
 * @brief  氷盤 (ボス側アビリティ) の実装
 * @author Natsume Shidara
 * @date   2026/05/15
 ****************************************/
#include "boss_ice_slide.h"

#include "ability/ability_registry.h"
#include "board_ops.h"
#include "game_state.h"

#include <random>

//--------------------------------------
// 内部ヘルパ
//--------------------------------------
namespace
{
    /** @brief 4方向からランダムに1つ返す */
    Direction RandomDir()
    {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, 3);
        return static_cast<Direction>(dist(rng));
    }
}

//======================================
// 構築
//======================================
BossIceSlideAbility::BossIceSlideAbility()
{
    name        = "氷盤";
    description = "全ての駒が指定方向へ\n端まで滑る";
    rarity      = Rarity::Rare;
    currentDir  = RandomDir();   // 初期方向もランダムに決める
}

//======================================
// IPlacementHandler
//======================================
void BossIceSlideAbility::OnPlace(GameState& state, Vec2 /*pos*/, Piece /*placedBy*/)
{
    // 全駒スライドを実行し、その移動を1つの効果フェーズとして記録する(アニメ用)
    BoardOps::MoveList moves;
    BoardOps::SlideAll(state.board, currentDir, &moves);
    if (!moves.empty())
    {
        state.animPhases.push_back(std::move(moves));
    }
}

//======================================
// ITurnHandler
//======================================
int BossIceSlideAbility::GetPlacementCount(Piece player)
{
    // 着手数自体は変えない。チェイン先(デフォルト等)の値を返す。
    return chainedTurn ? chainedTurn->GetPlacementCount(player) : 1;
}

void BossIceSlideAbility::OnTurnStart(GameState& state, Piece player)
{
    if (chainedTurn) chainedTurn->OnTurnStart(state, player);
    currentDir = RandomDir();   // ターンごとに滑走方向を抽選する
}

void BossIceSlideAbility::OnTurnEnd(GameState& state, Piece player)
{
    if (chainedTurn) chainedTurn->OnTurnEnd(state, player);
}

//======================================
// Ability
//======================================
void BossIceSlideAbility::RegisterTo(AbilityRegistry& registry)
{
    auto self = shared_from_this();
    // 設置ハンドラとして追加登録
    registry.AddPlacementHandler(std::dynamic_pointer_cast<IPlacementHandler>(self));
    // ターン処理を差し替え、旧実装はチェインで保持する
    auto turnSelf = std::dynamic_pointer_cast<ITurnHandler>(self);
    chainedTurn = registry.SetTurnHandler(turnSelf);
}
