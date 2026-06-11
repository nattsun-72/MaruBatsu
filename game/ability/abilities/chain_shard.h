/****************************************
 * @file   chain_shard.h
 * @brief  連鎖の欠片 — 任意発動で次の自駒設置が連鎖拡張 (ボス3撃破報酬)
 * @author Natsume Shidara
 * @date   2026/06/11
 *
 * 連鎖の鼓動のギミックの弱化版。クリックで「装填」し、次に自分が
 * 駒を置いたとき、隣接する空きマスへ自駒が1個自動拡張する。
 * チャージ制(ラン中の残回数)。
 ****************************************/
#ifndef ABILITY_CHAIN_SHARD_H
#define ABILITY_CHAIN_SHARD_H

#include "ability/ability.h"
#include "ability/hooks.h"

//======================================
// 連鎖の欠片アビリティ
//======================================
/**
 * @class  ChainShardAbility
 * @brief  装填式の連鎖拡張アビリティ (任意発動 + 設置フック)
 * @detail Activate で armed を立て、次の自駒設置の OnPlace で
 *         隣接空きマスへ自駒を1個出現させる。
 */
class ChainShardAbility : public Ability, public IPlacementHandler
{
public:
    //--------------------------------------
    // メンバ変数
    //--------------------------------------
    int  charges = 3;       // 残りチャージ数 (ラン中共有)
    bool armed   = false;   // 装填済みか (次の自駒設置で発動)

    ChainShardAbility();

    //======================================
    // 任意発動アビリティのインターフェイス
    //======================================
    /** @brief 発動: チャージを1消費して装填する */
    void Activate(GameState& state) override;
    /** @brief 発動可能か (チャージあり かつ 未装填) */
    bool CanActivate(const GameState& state) const override;
    /** @brief 残りチャージ数を返す */
    int  ChargesLeft() const override { return charges; }

    //======================================
    // IPlacementHandler
    //======================================
    /** @brief 装填中に自駒が置かれたら、隣接空きマスへ1個拡張する */
    void OnPlace(GameState& state, Vec2 pos, Piece placedBy) override;

    //======================================
    // Ability
    //======================================
    /** @brief 設置ハンドラとして registry に登録 */
    void RegisterTo(AbilityRegistry& registry) override;
};

#endif // ABILITY_CHAIN_SHARD_H
