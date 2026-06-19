/****************************************
 * @file   boss_engraved.h
 * @brief  刻まれし力 — ボス5「刻まれし者」のボス側アビリティ
 * @author Natsume Shidara
 * @date   2026/06/11
 * @update 2026/06/19 - 毎ターン1つ切替 → カテゴリ別3ギミック同時発動へ(仕様準拠)
 *
 * 仕様(企画書 2.5): 過去ボスのギミックをカテゴリ別に抽選し、弱化なしで
 * 全て同時発動する。本実装では設置系(氷盤スライド or 連鎖拡張)・構造系
 * (盤面回転)・駒強化系(重力落下)の3つをラン開始時に確定し、設置のたびに
 * 3つすべてを順に発動する。現在の3つの力はHUDで常時提示する。
 ****************************************/
#ifndef ABILITY_BOSS_ENGRAVED_H
#define ABILITY_BOSS_ENGRAVED_H

#include "ability/ability.h"
#include "ability/hooks.h"
#include "direction.h"

#include <memory>

//======================================
// 設置系ギミックの選択
//======================================
/**
 * @enum   EngravedPlaceMode
 * @brief  刻まれし者が振るう「設置系」ギミックの種類
 */
enum class EngravedPlaceMode
{
    Ice,    // 氷盤: 全駒スライド
    Chain,  // 連鎖: ボス駒の隣接拡張
};

//======================================
// 刻まれし力アビリティ (ボス側)
//======================================
/**
 * @class  BossEngravedAbility
 * @brief  設置系+構造系+駒強化系の3ギミックを同時に振るうラスボス能力
 * @detail ITurnHandler でターン開始時に氷盤の方向を抽選し、
 *         IPlacementHandler で設置のたび3ギミックすべてを発動する。
 */
class BossEngravedAbility
    : public Ability
    , public IPlacementHandler
    , public ITurnHandler
{
public:
    //--------------------------------------
    // メンバ変数 (ラン開始時に確定する3つの力)
    //--------------------------------------
    EngravedPlaceMode placeMode  = EngravedPlaceMode::Ice;  // 設置系(氷盤 or 連鎖)
    Direction         currentDir = Direction::Right;        // 氷盤の滑走方向
    bool              clockwise  = true;                    // 構造系(回転)の向き
    int               pulseSteps = 1;                       // 連鎖の拡張数

    // 旧 ITurnHandler をチェインで保持し、委譲する
    std::shared_ptr<ITurnHandler> chainedTurn;

    //======================================
    // 構築
    //======================================
    BossEngravedAbility();

    //======================================
    // IPlacementHandler
    //======================================
    /** @brief 設置時、設置系+構造系+駒強化系の3ギミックを順に発動する */
    void OnPlace(GameState& state, Vec2 pos, Piece placedBy) override;

    //======================================
    // ITurnHandler
    //======================================
    /** @brief 1ターンの着手数 (チェイン先へ委譲) */
    int  GetPlacementCount(Piece player) override;
    /** @brief ターン開始時、氷盤の滑走方向を抽選し直す */
    void OnTurnStart(GameState& state, Piece player) override;
    /** @brief ターン終了時 (チェイン先へ委譲) */
    void OnTurnEnd  (GameState& state, Piece player) override;

    //======================================
    // Ability
    //======================================
    /** @brief 設置ハンドラ・ターン処理として registry に登録 */
    void RegisterTo(AbilityRegistry& registry) override;

    /** @brief 現在発動中の3つの力の名称を返す (HUD表示用。例 "氷盤＋螺旋＋重力") */
    const char* PowersLabel() const;
    /** @brief 設置系が連鎖か (true=連鎖 / false=氷盤)。インジケータ切替用 */
    bool IsChainPlace() const { return placeMode == EngravedPlaceMode::Chain; }
};

#endif // ABILITY_BOSS_ENGRAVED_H
