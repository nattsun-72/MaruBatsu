/****************************************
 * @file ability_registry.cpp
 * @brief AbilityRegistry実装
 * @author Natsume Shidara
 * @date 2026/05/15
 ****************************************/
#include "ability_registry.h"
#include "default_hooks.h"

AbilityRegistry::AbilityRegistry()
{
    ResetToDefaults();
}

void AbilityRegistry::ResetToDefaults()
{
    m_Win          = std::make_shared<DefaultWinCondition>();
    m_Turn         = std::make_shared<DefaultTurnHandler>();
    m_TimeProvider = std::make_shared<DefaultTurnTimeProvider>();
    m_OnPlace.clear();
    m_OnDefeat.clear();
}

std::shared_ptr<IWinCondition> AbilityRegistry::SetWinCondition(std::shared_ptr<IWinCondition> w)
{
    auto prev = m_Win;
    if (w) m_Win = std::move(w);
    return prev;
}

std::shared_ptr<ITurnHandler> AbilityRegistry::SetTurnHandler(std::shared_ptr<ITurnHandler> t)
{
    auto prev = m_Turn;
    if (t) m_Turn = std::move(t);
    return prev;
}

std::shared_ptr<ITurnTimeProvider> AbilityRegistry::SetTurnTimeProvider(std::shared_ptr<ITurnTimeProvider> p)
{
    auto prev = m_TimeProvider;
    if (p) m_TimeProvider = std::move(p);
    return prev;
}

void AbilityRegistry::AddPlacementHandler(std::shared_ptr<IPlacementHandler> h)
{
    if (h) m_OnPlace.push_back(std::move(h));
}

void AbilityRegistry::AddDefeatHandler(std::shared_ptr<IDefeatHandler> h)
{
    if (h) m_OnDefeat.push_back(std::move(h));
}

bool AbilityRegistry::CheckWin(const Board& board, Piece player)
{
    return m_Win ? m_Win->Check(board, player) : false;
}

void AbilityRegistry::OnPlace(GameState& state, Vec2 pos, Piece placedBy)
{
    for (auto& h : m_OnPlace) h->OnPlace(state, pos, placedBy);
}

int AbilityRegistry::GetPlacementCount(Piece player)
{
    return m_Turn ? m_Turn->GetPlacementCount(player) : 1;
}

void AbilityRegistry::OnTurnStart(GameState& state, Piece player)
{
    if (m_Turn) m_Turn->OnTurnStart(state, player);
}

void AbilityRegistry::OnTurnEnd(GameState& state, Piece player)
{
    if (m_Turn) m_Turn->OnTurnEnd(state, player);
}

double AbilityRegistry::GetTurnTime(Piece player)
{
    return m_TimeProvider ? m_TimeProvider->GetTurnTimeSeconds(player) : 180.0;
}

bool AbilityRegistry::HandleDefeat(GameState& state, Piece loser)
{
    for (auto& h : m_OnDefeat)
    {
        if (h->OnDefeat(state, loser)) return true;
    }
    return false;
}
