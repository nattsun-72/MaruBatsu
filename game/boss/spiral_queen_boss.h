/****************************************
 * @file   spiral_queen_boss.h
 * @brief  ボス2: 螺旋の女王
 * @author Natsume Shidara
 * @date   2026/06/05
 *
 * 盤面が回転するギミックを持つ予定のボス。難易度★2。
 * W1第1増分ではギミック未実装のスタブ(通常〇×として成立)。
 * 回転ギミックは IBoardModifier 系として後続増分で本実装する。
 ****************************************/
#ifndef SPIRAL_QUEEN_BOSS_H
#define SPIRAL_QUEEN_BOSS_H

//--------------------------------------
// インクルード
//--------------------------------------
#include "boss/boss.h"

//======================================
// 螺旋の女王 (ボス2)
//======================================
/**
 * @class  SpiralQueenBoss
 * @brief  盤面回転ギミックを持つ予定のボス (現状スタブ)
 */
class SpiralQueenBoss : public Boss
{
public:
    //======================================
    // コンストラクタ
    //======================================
    /** @brief 名前・説明・難易度を設定する */
    SpiralQueenBoss();

    //======================================
    // Boss インターフェース実装
    //======================================
    /** @brief ボス戦中に登録するアビリティ群 (W1は空=通常〇×) */
    std::vector<std::shared_ptr<Ability>> GetBossAbilities() override;
    /** @brief 撃破報酬として渡すアビリティ (暫定) */
    std::shared_ptr<Ability> GetRewardAbility() override;
};

#endif // SPIRAL_QUEEN_BOSS_H
