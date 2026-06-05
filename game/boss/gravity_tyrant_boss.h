/****************************************
 * @file   gravity_tyrant_boss.h
 * @brief  ボス4: 重力の専制者
 * @author Natsume Shidara
 * @date   2026/06/05
 *
 * 重い駒(駒強化)ギミックを持つ予定のボス。難易度★4。
 * W1第1増分ではギミック未実装のスタブ(通常〇×として成立)。
 * 駒強化ギミックは IPieceModifier 系として後続週で本実装する。
 ****************************************/
#ifndef GRAVITY_TYRANT_BOSS_H
#define GRAVITY_TYRANT_BOSS_H

//--------------------------------------
// インクルード
//--------------------------------------
#include "boss/boss.h"

//======================================
// 重力の専制者 (ボス4)
//======================================
/**
 * @class  GravityTyrantBoss
 * @brief  重い駒(駒強化)ギミックを持つ予定のボス (現状スタブ)
 */
class GravityTyrantBoss : public Boss
{
public:
    //======================================
    // コンストラクタ
    //======================================
    /** @brief 名前・説明・難易度を設定する */
    GravityTyrantBoss();

    //======================================
    // Boss インターフェース実装
    //======================================
    /** @brief ボス戦中に登録するアビリティ群 (W1は空=通常〇×) */
    std::vector<std::shared_ptr<Ability>> GetBossAbilities() override;
    /** @brief 撃破報酬として渡すアビリティ (暫定) */
    std::shared_ptr<Ability> GetRewardAbility() override;
};

#endif // GRAVITY_TYRANT_BOSS_H
