/****************************************
 * @file   gravity_tyrant_boss.h
 * @brief  ボス4: 重力の専制者
 * @author Natsume Shidara
 * @date   2026/06/05
 *
 * 重力(全駒落下)ギミックを持つボス。難易度★4。
 * 設置のたび全駒を下端へ落とす BossGravity を登録する。
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
 * @brief  重力(全駒落下)ギミックを持つボス
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
    /** @brief ボス戦中に登録するアビリティ群 (重力ギミック) */
    std::vector<std::shared_ptr<Ability>> GetBossAbilities() override;
    /** @brief 撃破報酬として渡すアビリティ (重駒) */
    std::shared_ptr<Ability> GetRewardAbility() override;
};

#endif // GRAVITY_TYRANT_BOSS_H
