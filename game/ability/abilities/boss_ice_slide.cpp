/****************************************
 * @file boss_ice_slide.cpp
 * @brief 氷盤の支配者 ボス側アビリティの実装
 * @author Natsume Shidara
 * @date 2026/05/15
 ****************************************/
#include "boss_ice_slide.h"

#include "ability/ability_registry.h"
#include "board_ops.h"
#include "game_state.h"

#include <random>

namespace
{
    Direction RandomDir()
    {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, 3);
        return static_cast<Direction>(dist(rng));
    }
}

BossIceSlideAbility::BossIceSlideAbility()
{
    name        = "Ice Board (Boss)";
    description = "All pieces slide to the edge each placement. Direction randomizes per turn.";
    rarity      = Rarity::Rare;
    currentDir  = RandomDir();
}

void BossIceSlideAbility::OnPlace(GameState& state, Vec2 /*pos*/, Piece /*placedBy*/)
{
    BoardOps::SlideAll(state.board, currentDir);
}

int BossIceSlideAbility::GetPlacementCount(Piece player)
{
    return chainedTurn ? chainedTurn->GetPlacementCount(player) : 1;
}

void BossIceSlideAbility::OnTurnStart(GameState& state, Piece player)
{
    if (chainedTurn) chainedTurn->OnTurnStart(state, player);
    currentDir = RandomDir();
}

void BossIceSlideAbility::OnTurnEnd(GameState& state, Piece player)
{
    if (chainedTurn) chainedTurn->OnTurnEnd(state, player);
}

void BossIceSlideAbility::RegisterTo(AbilityRegistry& registry)
{
    auto self = shared_from_this();
    registry.AddPlacementHandler(std::dynamic_pointer_cast<IPlacementHandler>(self));
    auto turnSelf = std::dynamic_pointer_cast<ITurnHandler>(self);
    chainedTurn = registry.SetTurnHandler(turnSelf);
}
