/****************************************
 * @file game.cpp
 * @brief ゲームシーン (Day 3 — フック設計対応)
 * @author Natsume Shidara
 * @date 2025/07/01
 * @update 2026/05/15 - フック設計リファクタ (Day 3)
 *
 * 設計のキモ:
 *   GameState (盤面+ターン状態) + AbilityRegistry (フック実装群) の組合せで、
 *   勝敗判定/設置時イベント/ターン時間/敗北処理 を全て差し替え可能にする。
 *
 * Day 3時点では全フックがデフォルト実装で、Day 2と同等の挙動を再現する。
 ****************************************/

#include "game.h"

#include "game_state.h"
#include "ai.h"
#include "render_primitives.h"
#include "text_draw.h"

#include "ability/ability_registry.h"
#include "ability/abilities/boss_ice_slide.h"
#include "boss/boss.h"
#include "boss/ice_board_boss.h"

#include "run_state.h"

#include "scene.h"
#include "direct3d.h"
#include "input_manager.h"
#include "keyboard.h"

#include <DirectXMath.h>
#include <cstdio>
#include <memory>
#include <vector>

namespace
{
    // ── 盤面の見た目 ─────────────────────────────────────
    constexpr float BOARD_SIZE         = 540.0f;
    constexpr float CELL_SIZE          = BOARD_SIZE / 3.0f;
    constexpr float LINE_THICKNESS     = 4.0f;
    constexpr float PIECE_RADIUS_RATIO = 0.32f;
    constexpr float PIECE_THICKNESS    = 6.0f;

    // ── テキスト ─────────────────────────────────────────
    constexpr float TEXT_SIZE_HUD  = 24.0f;
    constexpr float TEXT_SIZE_BIG  = 56.0f;
    constexpr float TEXT_SIZE_HINT = 18.0f;

    // ── AI遅延 (即時行動だと視認性が悪いため) ───────────
    constexpr double AI_DELAY = 0.35;

    // ── カラーテーマ ─────────────────────────────────────
    const DirectX::XMFLOAT4 COLOR_BG          { 0.04f, 0.04f, 0.08f, 1.0f };
    const DirectX::XMFLOAT4 COLOR_GRID        { 0.70f, 0.70f, 0.80f, 1.0f };
    const DirectX::XMFLOAT4 COLOR_PIECE_O     { 0.50f, 0.85f, 1.00f, 1.0f };
    const DirectX::XMFLOAT4 COLOR_PIECE_X     { 1.00f, 0.45f, 0.85f, 1.0f };
    const DirectX::XMFLOAT4 COLOR_TEXT        { 0.85f, 0.90f, 0.95f, 1.0f };
    const DirectX::XMFLOAT4 COLOR_TEXT_HINT   { 0.55f, 0.60f, 0.70f, 1.0f };
    const DirectX::XMFLOAT4 COLOR_TEXT_WIN    { 0.60f, 1.00f, 0.60f, 1.0f };
    const DirectX::XMFLOAT4 COLOR_TEXT_LOSE   { 1.00f, 0.50f, 0.50f, 1.0f };
    const DirectX::XMFLOAT4 COLOR_TEXT_DRAW   { 0.80f, 0.80f, 0.90f, 1.0f };
    const DirectX::XMFLOAT4 COLOR_TEXT_URGENT { 1.00f, 0.85f, 0.40f, 1.0f };

    // ── 永続状態 ─────────────────────────────────────────
    GameState         g_State;
    AbilityRegistry   g_Registry;
    double            g_AiTimer   = 0.0;
    bool              g_AiPending = false;

    std::unique_ptr<Boss>                       g_Boss;
    std::vector<std::shared_ptr<Ability>>       g_BossAbilities;
    std::shared_ptr<BossIceSlideAbility>        g_IceSlideRef;  // HUD表示用

    // 直近着手の演出
    Vec2   g_LastPlacedPos { -1, -1 };
    Piece  g_LastPlacedPiece = Piece::Empty;
    double g_LastPlacedTimer = 0.0;
    constexpr double GLOW_DURATION = 0.55;

    // GameOver後の表示 → 報酬遷移の待機
    double g_GameOverDwell = 0.0;
    constexpr double GAMEOVER_MIN_DWELL = 0.6;

    float BoardOriginX() { return (Direct3D_GetBackBufferWidth()  - BOARD_SIZE) * 0.5f; }
    float BoardOriginY() { return (Direct3D_GetBackBufferHeight() - BOARD_SIZE) * 0.5f; }

    Vec2 ScreenToCell(int sx, int sy)
    {
        const float rx = sx - BoardOriginX();
        const float ry = sy - BoardOriginY();
        if (rx < 0 || ry < 0 || rx >= BOARD_SIZE || ry >= BOARD_SIZE)
        {
            return { -1, -1 };
        }
        return { static_cast<int>(rx / CELL_SIZE),
                 static_cast<int>(ry / CELL_SIZE) };
    }

    // ── ターン開始処理 ───────────────────────────────────
    void BeginTurn(Piece p)
    {
        g_State.currentPlayer       = p;
        g_State.placementsRemaining = g_Registry.GetPlacementCount(p);
        g_State.remainingTime       = g_Registry.GetTurnTime(p);
        g_Registry.OnTurnStart(g_State, p);

        if (p == Piece::Enemy)
        {
            g_AiPending = true;
            g_AiTimer   = AI_DELAY;
        }
        else
        {
            g_AiPending = false;
        }
    }

    void LoadBoss()
    {
        // プロト版: 固定でボス1を起動。
        // α版で RunManager が boss index に応じて切り替える。
        g_Boss = std::make_unique<IceBoardBoss>();
        g_BossAbilities = g_Boss->GetBossAbilities();
        g_IceSlideRef.reset();

        // 先にボス側、後にプレイヤー側を登録 (プレイヤー側が上書きするべき)
        for (auto& a : g_BossAbilities)
        {
            a->RegisterTo(g_Registry);
            if (auto ice = std::dynamic_pointer_cast<BossIceSlideAbility>(a))
            {
                g_IceSlideRef = ice;
            }
        }
        for (auto& a : RunState::PlayerAbilities())
        {
            a->RegisterTo(g_Registry);
        }
    }

    void ResetMatch()
    {
        g_State = {};
        g_State.board.Reset();
        g_State.result = MatchResult::Playing;
        g_State.winner = Piece::Empty;
        g_State.turnCount = 1;

        g_Registry.ResetToDefaults();
        g_LastPlacedPos    = { -1, -1 };
        g_LastPlacedPiece  = Piece::Empty;
        g_LastPlacedTimer  = 0.0;
        g_GameOverDwell    = 0.0;
        LoadBoss();
        BeginTurn(Piece::Player);
    }

    // ── 着手後の判定 ─────────────────────────────────────
    void EvaluateAfterPlace(Piece justPlaced)
    {
        // 1) 設置時イベント (アビリティ用フック)
        // ※ EvaluateAfterPlace の前にOnPlaceを呼んでいるので、ここでは省略。
        //    OnPlaceは PlaceAndEvaluate 側で先に呼ぶ。

        // 2) 勝利判定
        if (g_Registry.CheckWin(g_State.board, justPlaced))
        {
            g_State.winner = justPlaced;
            g_State.result = (justPlaced == Piece::Player)
                ? MatchResult::Win : MatchResult::Lose;
            return;
        }

        // 3) 引き分け判定
        if (g_State.board.IsFull())
        {
            g_State.result = MatchResult::Draw;
            return;
        }

        // 4) 同一プレイヤーの追加着手が残っているか
        if (g_State.placementsRemaining > 0) return;

        // 5) ターン終了
        g_Registry.OnTurnEnd(g_State, justPlaced);
        ++g_State.turnCount;
        BeginTurn(Opponent(justPlaced));
    }

    // 駒を1個置く -> フック発火 -> 勝敗評価
    void PlaceAndEvaluate(Vec2 pos, Piece placedBy)
    {
        if (!g_State.board.IsEmpty(pos.x, pos.y)) return;
        g_State.board.Set(pos.x, pos.y, placedBy);
        --g_State.placementsRemaining;

        // 設置時演出 (フック発火前に位置を記録 — 滑り後の最終位置はフック後に更新)
        g_LastPlacedPos   = pos;
        g_LastPlacedPiece = placedBy;
        g_LastPlacedTimer = GLOW_DURATION;

        // 設置時フック (氷駒/連鎖/爆弾等が盤面を書き換えうる)
        g_Registry.OnPlace(g_State, pos, placedBy);

        EvaluateAfterPlace(placedBy);
    }

    // ── 入力/AI ──────────────────────────────────────────
    void HandlePlayerInput()
    {
        if (g_State.currentPlayer != Piece::Player) return;
        if (!InputManager_IsMouseLeftTrigger()) return;
        const auto& mouse = InputManager_GetMouseState();
        const Vec2 cell = ScreenToCell(mouse.x, mouse.y);
        if (!g_State.board.IsEmpty(cell.x, cell.y)) return;
        PlaceAndEvaluate(cell, Piece::Player);
    }

    void HandleAiTurn(double dt)
    {
        if (g_State.currentPlayer != Piece::Enemy || !g_AiPending) return;
        g_AiTimer -= dt;
        if (g_AiTimer > 0.0) return;

        const Vec2 v = AI::ChooseMove_Random(
            g_State.board, Piece::Enemy, g_Registry.GetWinCondition());
        g_AiPending = false;
        if (v.x < 0)
        {
            g_State.result = MatchResult::Draw;
            return;
        }
        PlaceAndEvaluate(v, Piece::Enemy);
        // 追加着手が残っていれば再びAI実行
        if (g_State.placementsRemaining > 0 && g_State.result == MatchResult::Playing)
        {
            g_AiPending = true;
            g_AiTimer   = AI_DELAY;
        }
    }

    void HandleTurnTimer(double dt)
    {
        if (g_State.result != MatchResult::Playing) return;
        if (g_State.currentPlayer != Piece::Player) return;
        g_State.remainingTime -= dt;
        if (g_State.remainingTime > 0.0) return;

        g_State.remainingTime = 0.0;
        // 敗北フック (不死等で打ち消し可能)
        if (g_Registry.HandleDefeat(g_State, Piece::Player))
        {
            // 敗北回避: 残時間を新規取得
            g_State.remainingTime = g_Registry.GetTurnTime(Piece::Player);
            return;
        }
        g_State.result = MatchResult::Timeout;
    }

    // ── 描画 ─────────────────────────────────────────────
    void DrawBoardAndPieces()
    {
        const float ox = BoardOriginX();
        const float oy = BoardOriginY();

        Prim::DrawRect(ox - 8, oy - 8, BOARD_SIZE + 16, BOARD_SIZE + 16, COLOR_BG);

        for (int i = 1; i <= 2; ++i)
        {
            const float gx = ox + CELL_SIZE * i;
            const float gy = oy + CELL_SIZE * i;
            Prim::DrawLine(gx, oy, gx, oy + BOARD_SIZE, LINE_THICKNESS, COLOR_GRID);
            Prim::DrawLine(ox, gy, ox + BOARD_SIZE, gy, LINE_THICKNESS, COLOR_GRID);
        }

        const float halfCell    = CELL_SIZE * 0.5f;
        const float pieceRadius = CELL_SIZE * PIECE_RADIUS_RATIO;

        // 直近設置のグロー (拡大半径・低不透明度の同色形状を後ろに重ねる)
        if (g_LastPlacedTimer > 0.0 && g_LastPlacedPos.x >= 0)
        {
            const float t = static_cast<float>(g_LastPlacedTimer / GLOW_DURATION); // 1→0
            const float glowRadius = pieceRadius * (1.0f + (1.0f - t) * 0.6f);
            const float alpha = t * 0.5f;
            const float cx = ox + g_LastPlacedPos.x * CELL_SIZE + halfCell;
            const float cy = oy + g_LastPlacedPos.y * CELL_SIZE + halfCell;
            DirectX::XMFLOAT4 glowColor = (g_LastPlacedPiece == Piece::Player)
                ? DirectX::XMFLOAT4(COLOR_PIECE_O.x, COLOR_PIECE_O.y, COLOR_PIECE_O.z, alpha)
                : DirectX::XMFLOAT4(COLOR_PIECE_X.x, COLOR_PIECE_X.y, COLOR_PIECE_X.z, alpha);
            Prim::DrawCircle(cx, cy, glowRadius, PIECE_THICKNESS * 2.0f, glowColor, 24);
        }

        for (int y = 0; y < g_State.board.height; ++y)
        {
            for (int x = 0; x < g_State.board.width; ++x)
            {
                const float cx = ox + x * CELL_SIZE + halfCell;
                const float cy = oy + y * CELL_SIZE + halfCell;
                switch (g_State.board.Get(x, y))
                {
                case Piece::Player:
                    Prim::DrawCircle(cx, cy, pieceRadius, PIECE_THICKNESS, COLOR_PIECE_O);
                    break;
                case Piece::Enemy:
                    Prim::DrawCross(cx, cy, pieceRadius, PIECE_THICKNESS, COLOR_PIECE_X);
                    break;
                default: break;
                }
            }
        }
    }

    void DrawHud()
    {
        const float screenW = static_cast<float>(Direct3D_GetBackBufferWidth());

        char buf[64];
        const int total   = static_cast<int>(g_State.remainingTime + 0.999);
        const int minutes = total / 60;
        const int seconds = total % 60;
        std::snprintf(buf, sizeof(buf), "TIME  %d:%02d", minutes, seconds);
        const DirectX::XMFLOAT4& timeColor =
            (g_State.remainingTime < 30.0
             && g_State.result == MatchResult::Playing
             && g_State.currentPlayer == Piece::Player)
                ? COLOR_TEXT_URGENT : COLOR_TEXT;
        Text::Draw(24.0f, 24.0f, buf, TEXT_SIZE_HUD, timeColor);

        if (g_State.result == MatchResult::Playing)
        {
            const char* turn =
                (g_State.currentPlayer == Piece::Player) ? "PLAYER (O)" : "ENEMY  (X)";
            const float w = Text::MeasureWidth(turn, TEXT_SIZE_HUD);
            Text::Draw(screenW - w - 24.0f, 24.0f, turn, TEXT_SIZE_HUD, COLOR_TEXT);
        }

        // ボス情報 (画面中央上)
        if (g_Boss)
        {
            char nameBuf[64];
            std::snprintf(nameBuf, sizeof(nameBuf), "BOSS  %s", g_Boss->name.c_str());
            const float nameW = Text::MeasureWidth(nameBuf, TEXT_SIZE_HUD);
            Text::Draw((screenW - nameW) * 0.5f, 24.0f, nameBuf,
                       TEXT_SIZE_HUD, COLOR_TEXT_URGENT);
        }
        // 氷盤の現在方向
        if (g_IceSlideRef && g_State.result == MatchResult::Playing)
        {
            char dirBuf[64];
            std::snprintf(dirBuf, sizeof(dirBuf), "SLIDE %s",
                          DirectionName(g_IceSlideRef->currentDir));
            const float dw = Text::MeasureWidth(dirBuf, TEXT_SIZE_HINT);
            Text::Draw((screenW - dw) * 0.5f, 24.0f + TEXT_SIZE_HUD + 4.0f,
                       dirBuf, TEXT_SIZE_HINT, COLOR_TEXT_HINT);
        }

        // 所持アビリティ (画面左下)
        const auto& playerAbils = RunState::PlayerAbilities();
        if (!playerAbils.empty())
        {
            const float screenH = static_cast<float>(Direct3D_GetBackBufferHeight());
            const float ax = 24.0f;
            float ay = screenH - 24.0f
                       - TEXT_SIZE_HINT * static_cast<float>(playerAbils.size())
                       - TEXT_SIZE_HUD;
            Text::Draw(ax, ay, "ABILITIES", TEXT_SIZE_HUD, COLOR_TEXT);
            ay += TEXT_SIZE_HUD + 4.0f;
            for (const auto& a : playerAbils)
            {
                char rowBuf[128];
                std::snprintf(rowBuf, sizeof(rowBuf), "- %s", a->name.c_str());
                Text::Draw(ax, ay, rowBuf, TEXT_SIZE_HINT, COLOR_TEXT_HINT);
                ay += TEXT_SIZE_HINT;
            }
        }
    }

    void DrawGameOverOverlay()
    {
        if (g_State.result == MatchResult::Playing) return;

        const float screenW = static_cast<float>(Direct3D_GetBackBufferWidth());
        const float screenH = static_cast<float>(Direct3D_GetBackBufferHeight());

        Prim::DrawRect(0, 0, screenW, screenH, { 0.0f, 0.0f, 0.05f, 0.55f });

        const char* msg = "";
        DirectX::XMFLOAT4 color = COLOR_TEXT;
        switch (g_State.result)
        {
        case MatchResult::Win:     msg = "PLAYER WIN!"; color = COLOR_TEXT_WIN; break;
        case MatchResult::Lose:    msg = "YOU LOSE";    color = COLOR_TEXT_LOSE; break;
        case MatchResult::Draw:    msg = "DRAW";        color = COLOR_TEXT_DRAW; break;
        case MatchResult::Timeout: msg = "TIME OUT";    color = COLOR_TEXT_LOSE; break;
        default: break;
        }
        const float msgW = Text::MeasureWidth(msg, TEXT_SIZE_BIG);
        Text::Draw((screenW - msgW) * 0.5f, screenH * 0.5f - TEXT_SIZE_BIG,
                   msg, TEXT_SIZE_BIG, color);

        const char* hint = (g_State.result == MatchResult::Win)
            ? "CLICK / SPACE TO CLAIM REWARD"
            : "[R] RESTART    [ESC] TITLE";
        const float hintW = Text::MeasureWidth(hint, TEXT_SIZE_HINT);
        Text::Draw((screenW - hintW) * 0.5f, screenH * 0.5f + 30.0f,
                   hint, TEXT_SIZE_HINT, COLOR_TEXT_HINT);
    }
}

void Game_Initialize()
{
    Prim::Initialize();
    Text::Initialize();
    ResetMatch();
}

void Game_Finalize()
{
}

void Game_Update(double elapsed_time)
{
    if (Keyboard_IsKeyDown(KK_ESCAPE))
    {
        Scene_Change(Scene::TITLE);
        return;
    }
    if (Keyboard_IsKeyDown(KK_R))
    {
        ResetMatch();
        return;
    }

    // ── プレイ中 ────────────────────────────────────────
    if (g_State.result == MatchResult::Playing)
    {
        if (g_LastPlacedTimer > 0.0)
        {
            g_LastPlacedTimer -= elapsed_time;
            if (g_LastPlacedTimer < 0.0) g_LastPlacedTimer = 0.0;
        }

        HandleTurnTimer(elapsed_time);
        if (g_State.result == MatchResult::Playing)
        {
            HandlePlayerInput();
            HandleAiTurn(elapsed_time);
        }
        return;
    }

    // ── ゲーム終了後 ────────────────────────────────────
    g_GameOverDwell += elapsed_time;
    if (g_State.result == MatchResult::Win
        && g_GameOverDwell >= GAMEOVER_MIN_DWELL
        && (InputManager_IsMouseLeftTrigger()
            || Keyboard_IsKeyDown(KK_SPACE)
            || Keyboard_IsKeyDown(KK_ENTER)))
    {
        // 報酬画面へ
        RunState::IncrementBoss();
        RunState::GenerateRewardChoices();
        Scene_Change(Scene::REWARD);
        return;
    }
}

void Game_Draw()
{
    DrawBoardAndPieces();
    DrawHud();
    DrawGameOverOverlay();
}
