/****************************************
 * @file   ice_tile.cpp
 * @brief  氷駒の実装
 * @author Natsume Shidara
 * @date   2026/05/15
 ****************************************/
#include "ice_tile.h"

#include "ability/ability_registry.h"
#include "board_ops.h"
#include "game_state.h"
#include "config.h"

#include <string>

//======================================
// 構築
//======================================
IceTileAbility::IceTileAbility()
{
    name          = "氷駒";
    rarity        = Rarity::Rare;
    bossExclusive = true;   // ボス1「氷盤の支配者」撃破でのみ獲得

    // ボス撃破報酬として強化(滑走距離を延長)。値は game_config.json で調整可。
    slideDistance = Config::GetInt("abilities.iceTile.slideDistance", 2);
    if (slideDistance < 1) slideDistance = 1;
    description   = "置いた自分の駒が\n" + std::to_string(slideDistance) + "マス滑る";
}

//======================================
// IPlacementHandler
//======================================
void IceTileAbility::OnPlace(GameState& state, Vec2 pos, Piece placedBy)
{
    if (placedBy != ownerSide) return;   // 自分の駒以外は対象外

    // 重駒所持時: 自駒はあらゆる滑りの対象外 (氷駒の自滑走も含む)
    if (state.slideAnchorSide == ownerSide) return;

    /*--- slideDistance マスぶん、1マスずつ順に滑らせる ---*/
    // 各ステップで「設置した駒の現在位置」を pos から滑走方向へ走査して求める。
    // 先行コピーや前ステップで既に滑った後は設置マス pos が空になっているため、
    // この再走査により駒を追従できる(重ね掛け所持時の距離加算とも両立する)。
    // 1マスずつ別フェーズとして記録することで、滑走がコマ送りでアニメ表示される。
    for (int step = 0; step < slideDistance; ++step)
    {
        int cx = pos.x;
        int cy = pos.y;
        while (state.board.IsValid(cx, cy) && state.board.Get(cx, cy) == Piece::Empty)
        {
            cx += DirX(slideDir);
            cy += DirY(slideDir);
        }
        if (state.board.Get(cx, cy) != placedBy) return;  // 駒が見つからなければ終了

        BoardOps::MoveList moves;
        if (!BoardOps::SlideOne(state.board, cx, cy, slideDir, &moves))
        {
            break;   // これ以上は滑れない(壁/他駒に接触)
        }
        state.animPhases.push_back(std::move(moves));
    }
}

//======================================
// Ability
//======================================
void IceTileAbility::RegisterTo(AbilityRegistry& registry)
{
    auto self = shared_from_this();
    registry.AddPlacementHandler(std::dynamic_pointer_cast<IPlacementHandler>(self));
}
