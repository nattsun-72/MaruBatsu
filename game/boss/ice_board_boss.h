/****************************************
 * @file   ice_board_boss.h
 * @brief  ボス1: 氷盤の支配者
 * @author Natsume Shidara
 * @date   2026/05/15
 *
 * 設置時/設置後フックを使う最初のボス。難易度★1。
 ****************************************/
#ifndef ICE_BOARD_BOSS_H
#define ICE_BOARD_BOSS_H

//--------------------------------------
// インクルード
//--------------------------------------
#include "boss/boss.h"

//======================================
// 氷盤の支配者 (ボス1)
//======================================
/**
 * @class  IceBoardBoss
 * @brief  駒を置くたび盤上の全駒が滑るギミックを持つボス
 * @detail Boss を継承する最初のボス。難易度★1。
 *         ボス側ギミックは BossIceSlideAbility、報酬は IceTileAbility。
 */
class IceBoardBoss : public Boss
{
public:
    //======================================
    // コンストラクタ
    //======================================
    /** @brief 名前・説明・難易度を設定する */
    IceBoardBoss();

    //======================================
    // Boss インターフェース実装
    //======================================
    /** @brief ボス戦中に登録するアビリティ群を返す */
    std::vector<std::shared_ptr<Ability>> GetBossAbilities() override;
    /** @brief 撃破報酬として渡すアビリティを返す */
    std::shared_ptr<Ability> GetRewardAbility() override;
};

#endif // ICE_BOARD_BOSS_H
