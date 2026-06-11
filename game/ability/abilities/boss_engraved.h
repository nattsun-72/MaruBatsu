/****************************************
 * @file   boss_engraved.h
 * @brief  刻まれし力 — ボス5「刻まれし者」のボス側アビリティ
 * @author Natsume Shidara
 * @date   2026/06/11
 *
 * 仕様: 過去ボスの集合体。毎ターン開始時に「氷盤/螺旋/連鎖/重力」の
 *       いずれかのギミックを抽選し、そのターン中はその力を振るう。
 *       現在のギミックはHUDとインジケータで必ず提示される
 *       (規約: 盤面が動く効果は視覚的補助を徹底する)。
 ****************************************/
#ifndef ABILITY_BOSS_ENGRAVED_H
#define ABILITY_BOSS_ENGRAVED_H

#include "ability/ability.h"
#include "ability/hooks.h"
#include "direction.h"

#include <memory>

//======================================
// ギミックモード
//======================================
/**
 * @enum   EngravedMode
 * @brief  刻まれし者が現在振るっている過去ボスの力
 */
enum class EngravedMode
{
    Ice,      // 氷盤: 全駒スライド
    Spiral,   // 螺旋: 盤面90度回転
    Chain,    // 連鎖: ボス駒の隣接拡張
    Gravity,  // 重力: 設置駒の落下
};

//======================================
// 刻まれし力アビリティ (ボス側)
//======================================
/**
 * @class  BossEngravedAbility
 * @brief  毎ターン過去ボスのギミックを切り替えて振るうラスボス能力
 * @detail ITurnHandler でターン開始時にモードを抽選し、
 *         IPlacementHandler で現在モードの効果を発動する。
 */
class BossEngravedAbility
    : public Ability
    , public IPlacementHandler
    , public ITurnHandler
{
public:
    //--------------------------------------
    // メンバ変数
    //--------------------------------------
    EngravedMode mode       = EngravedMode::Ice;      // 現在のギミック
    Direction    currentDir = Direction::Right;       // 氷盤モードの滑走方向
    bool         clockwise  = true;                   // 螺旋モードの回転方向
    int          pulseSteps = 1;                      // 連鎖モードの拡張数

    // 旧 ITurnHandler をチェインで保持し、委譲する
    std::shared_ptr<ITurnHandler> chainedTurn;

    //======================================
    // 構築
    //======================================
    BossEngravedAbility();

    //======================================
    // IPlacementHandler
    //======================================
    /** @brief 設置時、現在モードのギミックを発動する */
    void OnPlace(GameState& state, Vec2 pos, Piece placedBy) override;

    //======================================
    // ITurnHandler
    //======================================
    /** @brief 1ターンの着手数 (チェイン先へ委譲) */
    int  GetPlacementCount(Piece player) override;
    /** @brief ターン開始時、モードと方向を抽選する */
    void OnTurnStart(GameState& state, Piece player) override;
    /** @brief ターン終了時 (チェイン先へ委譲) */
    void OnTurnEnd  (GameState& state, Piece player) override;

    //======================================
    // Ability
    //======================================
    /** @brief 設置ハンドラ・ターン処理として registry に登録 */
    void RegisterTo(AbilityRegistry& registry) override;

    /** @brief 現在モードの日本語名を返す (HUD表示用) */
    const char* ModeName() const;
};

#endif // ABILITY_BOSS_ENGRAVED_H
