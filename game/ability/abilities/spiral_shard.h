/****************************************
 * @file   spiral_shard.h
 * @brief  螺旋の欠片 — 任意発動で盤面を90度回転 (ボス2撃破報酬)
 * @author Natsume Shidara
 * @date   2026/06/11
 *
 * 螺旋の女王のギミックの弱化版。クリックで盤面を時計回りに
 * 90度回転させる。チャージ制(ラン中の残回数)。
 ****************************************/
#ifndef ABILITY_SPIRAL_SHARD_H
#define ABILITY_SPIRAL_SHARD_H

#include "ability/ability.h"

//======================================
// 螺旋の欠片アビリティ
//======================================
/**
 * @class  SpiralShardAbility
 * @brief  クリック発動で盤面を90度回転させるアビリティ
 * @detail 任意発動(activatable)型。回転は効果フェーズとして記録され、
 *         ゲームシーン側でアニメ再生と勝敗再評価が行われる。
 */
class SpiralShardAbility : public Ability
{
public:
    //--------------------------------------
    // メンバ変数
    //--------------------------------------
    int  charges   = 2;      // 残りチャージ数 (ラン中共有)
    bool clockwise = true;   // 回転方向

    SpiralShardAbility();

    //======================================
    // 任意発動アビリティのインターフェイス
    //======================================
    /** @brief 発動: チャージを1消費し、盤面を90度回転させる */
    void Activate(GameState& state) override;
    /** @brief 発動可能か (チャージが残っているか) */
    bool CanActivate(const GameState& state) const override;
    /** @brief 残りチャージ数を返す */
    int  ChargesLeft() const override { return charges; }

    //======================================
    // Ability
    //======================================
    /** @brief パッシブフックは持たないため、登録処理は空 */
    void RegisterTo(AbilityRegistry& /*registry*/) override {}
};

#endif // ABILITY_SPIRAL_SHARD_H
