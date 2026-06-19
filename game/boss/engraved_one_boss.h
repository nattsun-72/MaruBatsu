/****************************************
 * @file   engraved_one_boss.h
 * @brief  ボス5(ラスボス): 刻まれし者
 * @author Natsume Shidara
 * @date   2026/06/05
 *
 * 過去ボスの力をカテゴリ別に同時発動する複合ラスボス。難易度★5。
 * 設置のたび 設置系(氷盤/連鎖)+構造系(回転)+駒強化系(重力) を全発動する
 * BossEngravedAbility を登録する。撃破でラン完走(ランクリア)をトリガする。
 ****************************************/
#ifndef ENGRAVED_ONE_BOSS_H
#define ENGRAVED_ONE_BOSS_H

//--------------------------------------
// インクルード
//--------------------------------------
#include "boss/boss.h"

//======================================
// 刻まれし者 (ボス5 / ラスボス)
//======================================
/**
 * @class  EngravedOneBoss
 * @brief  過去ボスの力を3カテゴリ同時発動する複合ラスボス
 */
class EngravedOneBoss : public Boss
{
public:
    //======================================
    // コンストラクタ
    //======================================
    /** @brief 名前・説明・難易度を設定する */
    EngravedOneBoss();

    //======================================
    // Boss インターフェース実装
    //======================================
    /** @brief ボス戦中に登録するアビリティ群 (複合ギミック) */
    std::vector<std::shared_ptr<Ability>> GetBossAbilities() override;
    /** @brief 撃破報酬として渡すアビリティ (ラスボスのため通常は未使用) */
    std::shared_ptr<Ability> GetRewardAbility() override;
};

#endif // ENGRAVED_ONE_BOSS_H
