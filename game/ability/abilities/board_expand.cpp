/****************************************
 * @file   board_expand.cpp
 * @brief  盤面拡張の実装
 * @author Natsume Shidara
 * @date   2026/05/15
 * @update 2026/05/22 - 隅支配勝利(旧ライン本数追加)から盤面拡張へ仕様変更
 ****************************************/
#include "board_expand.h"
#include "ability/ability_registry.h"

#include <algorithm>

//======================================
// 構築
//======================================
BoardExpandAbility::BoardExpandAbility()
{
    name        = "盤面拡張";
    description = "盤面が3×3から\n4×4に広がる";
    rarity      = Rarity::Epic;
    unique      = true;   // 2個目を取得しても盤面サイズは変わらないため一度限り
}

//======================================
// IBoardModifier
//======================================
void BoardExpandAbility::GetBoardSize(int& width, int& height)
{
    // チェイン先 (デフォルト等) が決めたサイズを基準として取得する
    if (chained) chained->GetBoardSize(width, height);

    // 基準サイズより大きい場合のみ拡張する (縮小はしない)
    width  = (std::max)(width,  targetWidth);
    height = (std::max)(height, targetHeight);
}

//======================================
// Ability
//======================================
void BoardExpandAbility::RegisterTo(AbilityRegistry& registry)
{
    auto self = std::dynamic_pointer_cast<IBoardModifier>(shared_from_this());
    chained = registry.SetBoardModifier(self);  // 旧サイズフックをチェインに保存
}
