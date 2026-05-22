/****************************************
 * @file ability_registry.h
 * @brief 各フック実装を保持し、ゲームロジックへ提供
 * @author Natsume Shidara
 * @date 2026/05/15
 *
 * Game シーンは AbilityRegistry を介してフックを呼び出す。
 * デフォルト実装が常にセットされており、アビリティ獲得時に
 * Set/Add で上書き/追加される。
 ****************************************/
#ifndef ABILITY_REGISTRY_H
#define ABILITY_REGISTRY_H

#include "hooks.h"
#include <memory>
#include <vector>

class AbilityRegistry
{
public:
    AbilityRegistry();

    // ── デフォルト実装の再インストール (初期化/リセット時) ──
    void ResetToDefaults();

    // ── 単一フックの差し替え (旧実装を返却 — アビリティ側で chained に保存して呼び出し可) ─
    std::shared_ptr<IWinCondition>     SetWinCondition   (std::shared_ptr<IWinCondition>   w);
    std::shared_ptr<ITurnHandler>      SetTurnHandler    (std::shared_ptr<ITurnHandler>    t);
    std::shared_ptr<ITurnTimeProvider> SetTurnTimeProvider(std::shared_ptr<ITurnTimeProvider> p);

    // ── 複数登録可能なフック ─────────────────────────────
    void AddPlacementHandler(std::shared_ptr<IPlacementHandler> h);
    void AddDefeatHandler  (std::shared_ptr<IDefeatHandler>   h);

    // ── 呼び出しヘルパ ────────────────────────────────────
    bool   CheckWin     (const Board& board, Piece player);
    void   OnPlace      (GameState& state, Vec2 pos, Piece placedBy);
    int    GetPlacementCount(Piece player);
    void   OnTurnStart  (GameState& state, Piece player);
    void   OnTurnEnd    (GameState& state, Piece player);
    double GetTurnTime  (Piece player);
    bool   HandleDefeat (GameState& state, Piece loser);

    // ── アクセサ (AIが直接フックを参照する場合) ─────────
    IWinCondition* GetWinCondition() { return m_Win.get(); }

private:
    std::shared_ptr<IWinCondition>     m_Win;
    std::shared_ptr<ITurnHandler>      m_Turn;
    std::shared_ptr<ITurnTimeProvider> m_TimeProvider;
    std::vector<std::shared_ptr<IPlacementHandler>> m_OnPlace;
    std::vector<std::shared_ptr<IDefeatHandler>>    m_OnDefeat;
};

#endif // ABILITY_REGISTRY_H
