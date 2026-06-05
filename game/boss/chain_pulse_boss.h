/****************************************
 * @file   chain_pulse_boss.h
 * @brief  ボス3: 連鎖の鼓動
 * @author Natsume Shidara
 * @date   2026/06/05
 *
 * 駒が自動拡張するギミックを持つ予定のボス。難易度★3。
 * W1第1増分ではギミック未実装のスタブ(通常〇×として成立)。
 * 自動拡張ギミックは IPlacementHandler 系として後続増分で本実装する。
 ****************************************/
#ifndef CHAIN_PULSE_BOSS_H
#define CHAIN_PULSE_BOSS_H

//--------------------------------------
// インクルード
//--------------------------------------
#include "boss/boss.h"

//======================================
// 連鎖の鼓動 (ボス3)
//======================================
/**
 * @class  ChainPulseBoss
 * @brief  自動拡張ギミックを持つ予定のボス (現状スタブ)
 */
class ChainPulseBoss : public Boss
{
public:
    //======================================
    // コンストラクタ
    //======================================
    /** @brief 名前・説明・難易度を設定する */
    ChainPulseBoss();

    //======================================
    // Boss インターフェース実装
    //======================================
    /** @brief ボス戦中に登録するアビリティ群 (W1は空=通常〇×) */
    std::vector<std::shared_ptr<Ability>> GetBossAbilities() override;
    /** @brief 撃破報酬として渡すアビリティ (暫定) */
    std::shared_ptr<Ability> GetRewardAbility() override;
};

#endif // CHAIN_PULSE_BOSS_H
