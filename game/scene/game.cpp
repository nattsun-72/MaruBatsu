/****************************************
 * @file   game.cpp
 * @brief  ゲームシーン (Day 3 — フック設計対応)
 * @author Natsume Shidara
 * @date   2025/07/01
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
#include "board_ops.h"
#include "render_primitives.h"
#include "text_draw.h"

#include "ability/ability_registry.h"
#include "ability/abilities/boss_ice_slide.h"
#include "ability/abilities/boss_spiral_rotate.h"
#include "boss/boss.h"
#include "boss/boss_roster.h"

#include "run_state.h"

#include "scene.h"
#include "direct3d.h"
#include "input_manager.h"
#include "keyboard.h"

#include <DirectXMath.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

//--------------------------------------
// 定数・内部状態 (無名名前空間)
//--------------------------------------
namespace
{
    //--------------------------------------
    // 盤面の見た目
    //--------------------------------------
    constexpr float BOARD_SIZE         = 540.0f;             // 盤面全体の一辺
    constexpr float LINE_THICKNESS     = 4.0f;               // 罫線の太さ
    // 1マスの一辺は盤面サイズ(3×3/4×4)で変わるため CellSize() で動的算出する
    constexpr float PIECE_RADIUS_RATIO = 0.32f;              // 駒半径 / セルサイズ
    constexpr float PIECE_THICKNESS    = 6.0f;               // 駒の線の太さ

    //--------------------------------------
    // テキスト
    //--------------------------------------
    constexpr float TEXT_SIZE_HUD  = 24.0f;   // HUD文字
    constexpr float TEXT_SIZE_BIG  = 56.0f;   // 勝敗メッセージ
    constexpr float TEXT_SIZE_HINT = 18.0f;   // 補助文字

    //--------------------------------------
    // AI遅延 (即時行動だと視認性が悪いため)
    //--------------------------------------
    constexpr double AI_DELAY = 0.35;

    //--------------------------------------
    // 効果アニメーション
    //--------------------------------------
    constexpr double PHASE_DURATION = 0.18;  // 1効果フェーズの再生時間(秒)

    //--------------------------------------
    // 方向インジケータ (シェブロン矢印)
    //--------------------------------------
    constexpr int   CHEVRON_COUNT     = 3;       // 滑る先の辺に沿って並べる数
    constexpr float CHEVRON_SIZE      = 18.0f;   // シェブロンの大きさ
    constexpr float CHEVRON_THICKNESS = 6.0f;    // シェブロンの線の太さ
    constexpr float CHEVRON_MARGIN    = 24.0f;   // 盤面端からの距離

    //--------------------------------------
    // ボタンUI
    //--------------------------------------
    constexpr float BUTTON_W         = 240.0f;  // ボタン幅
    constexpr float BUTTON_H         = 60.0f;   // ボタン高さ
    constexpr float BUTTON_GAP       = 32.0f;   // ボタン間の余白
    constexpr float TEXT_SIZE_BUTTON = 22.0f;   // ボタン文字サイズ

    //--------------------------------------
    // 所持アビリティ一覧
    //--------------------------------------
    constexpr float ABILITY_LIST_X = 24.0f;   // 一覧の左端X
    constexpr float ABILITY_ROW_H  = 36.0f;   // 1行の高さ
    constexpr float ABILITY_BTN_W  = 220.0f;  // 任意発動ボタンの幅
    constexpr float ABILITY_BTN_H  = 30.0f;   // 任意発動ボタンの高さ

    //--------------------------------------
    // アビリティ履歴ポップアップ
    //--------------------------------------
    constexpr int   ABILITY_VISIBLE_MAX = 5;       // HUDに表示する最大グループ数
    constexpr float POPUP_CARD_W        = 200.0f;  // カード幅
    constexpr float POPUP_CARD_H        = 280.0f;  // カード高さ
    constexpr float POPUP_CARD_GAP      = 24.0f;   // カード間の余白
    constexpr float POPUP_SIDE_MARGIN   = 90.0f;   // カード帯の左右マージン
    constexpr float POPUP_SCROLL_SPEED  = 1.5f;    // ホイール変化量→スクロール量の係数

    //--------------------------------------
    // タイマー加算フィードバック
    //--------------------------------------
    constexpr double TIMER_FLASH_DURATION = 0.8;  // フラッシュ演出の長さ(秒)

    //--------------------------------------
    // カラーテーマ (RGBA)
    //--------------------------------------
    const DirectX::XMFLOAT4 COLOR_BG          { 0.04f, 0.04f, 0.08f, 1.0f };  // 背景
    const DirectX::XMFLOAT4 COLOR_GRID        { 0.70f, 0.70f, 0.80f, 1.0f };  // 罫線
    const DirectX::XMFLOAT4 COLOR_PIECE_O     { 0.50f, 0.85f, 1.00f, 1.0f };  // 〇 (プレイヤー)
    const DirectX::XMFLOAT4 COLOR_PIECE_X     { 1.00f, 0.45f, 0.85f, 1.0f };  // × (敵)
    const DirectX::XMFLOAT4 COLOR_TEXT        { 0.85f, 0.90f, 0.95f, 1.0f };  // 標準文字
    const DirectX::XMFLOAT4 COLOR_TEXT_HINT   { 0.55f, 0.60f, 0.70f, 1.0f };  // 補助文字
    const DirectX::XMFLOAT4 COLOR_TEXT_WIN    { 0.60f, 1.00f, 0.60f, 1.0f };  // 勝利
    const DirectX::XMFLOAT4 COLOR_TEXT_LOSE   { 1.00f, 0.50f, 0.50f, 1.0f };  // 敗北
    const DirectX::XMFLOAT4 COLOR_TEXT_DRAW   { 0.80f, 0.80f, 0.90f, 1.0f };  // 引き分け
    const DirectX::XMFLOAT4 COLOR_TEXT_URGENT { 1.00f, 0.85f, 0.40f, 1.0f };  // 警告 (残時間少)
    const DirectX::XMFLOAT4 COLOR_SLIDE_ARROW { 0.55f, 0.85f, 1.00f, 1.0f };  // 滑り方向矢印
    const DirectX::XMFLOAT4 COLOR_ROTATE_ARROW{ 0.82f, 0.55f, 1.00f, 1.0f };  // 回転方向矢印(紫)
    const DirectX::XMFLOAT4 COLOR_BTN_BG      { 0.12f, 0.13f, 0.20f, 0.96f };  // ボタン本体
    const DirectX::XMFLOAT4 COLOR_BTN_HOVER   { 0.24f, 0.28f, 0.40f, 0.98f };  // ボタン (ホバー)

    //--------------------------------------
    // 永続状態
    //--------------------------------------
    GameState         g_State;            // 盤面 + ターン状態
    AbilityRegistry   g_Registry;         // フック実装群
    double            g_AiTimer   = 0.0;  // AI着手までの残り遅延
    bool              g_AiPending = false; // AI着手待ちフラグ

    std::unique_ptr<Boss>                       g_Boss;          // 現在のボス
    std::vector<std::shared_ptr<Ability>>       g_BossAbilities; // ボス側アビリティ
    std::shared_ptr<BossIceSlideAbility>        g_IceSlideRef;   // 方向インジケータ用 (氷盤参照)
    std::shared_ptr<BossSpiralRotateAbility>    g_SpiralRef;     // 回転インジケータ用 (螺旋盤参照)

    //--------------------------------------
    // 直近着手の演出
    //--------------------------------------
    Vec2   g_LastPlacedPos { -1, -1 };       // 直近に駒を置いたマス
    Piece  g_LastPlacedPiece = Piece::Empty; // 直近に置いた駒の陣営
    double g_LastPlacedTimer = 0.0;          // グロー演出の残り時間
    constexpr double GLOW_DURATION = 0.55;   // グロー演出の長さ(秒)

    //--------------------------------------
    // GameOver後の表示 → 報酬遷移の待機
    //--------------------------------------
    double g_GameOverDwell = 0.0;            // ゲーム終了後の経過時間
    constexpr double GAMEOVER_MIN_DWELL = 0.6; // ボタン表示までの最低待機(秒)

    //--------------------------------------
    // 効果アニメーション再生状態
    //   駒を動かす効果 (氷盤/氷駒/連鎖など) を「フェーズ」単位で順次再生する。
    //   1フェーズ内の移動は同時、フェーズ間は順番に再生される。
    //--------------------------------------
    std::vector<BoardOps::MoveList> g_AnimPhases;        // 各フェーズの移動
    std::vector<Board>              g_PhaseStartBoards;  // 各フェーズ開始時の盤面
    int    g_AnimPhaseIndex = 0;    // 再生中フェーズ番号 (size以上で再生終了)
    double g_AnimPhaseTimer = 0.0;  // 現フェーズの残り時間

    double g_AnimClock  = 0.0;  // 演出用クロック (シェブロンの脈動)
    double g_TimerFlash = 0.0;  // 思考時間加算フラッシュ演出の残り時間

    //--------------------------------------
    // アビリティ履歴ポップアップの状態
    //--------------------------------------
    bool   g_AbilityPopupOpen = false;  // ポップアップ表示中か
    double g_PopupScroll      = 0.0;    // カード帯の横スクロール量(px)
    int    g_WheelPrev        = 0;      // 前フレームのホイール累積値
    int    g_WheelDelta       = 0;      // 当該フレームのホイール変化量

    //======================================
    // 補助関数 (状態問い合わせ・座標変換)
    //======================================
    /**
     * @brief  効果アニメーションが再生中かを返す
     * @return 再生中なら true
     */
    bool IsEffectAnimating()
    {
        return g_AnimPhaseIndex < static_cast<int>(g_AnimPhases.size());
    }

    /** @brief 盤面左端のX座標 (画面中央寄せ) */
    float BoardOriginX() { return (Direct3D_GetBackBufferWidth()  - BOARD_SIZE) * 0.5f; }
    /** @brief 盤面上端のY座標 (画面中央寄せ) */
    float BoardOriginY() { return (Direct3D_GetBackBufferHeight() - BOARD_SIZE) * 0.5f; }

    /**
     * @brief  現在の盤面サイズに応じた1マスの一辺を返す
     * @return BOARD_SIZE を盤面の最大マス数で割った値 (正方マス)
     * @detail 盤面拡張アビリティで 3×3→4×4 等に変わるため動的に算出する。
     */
    float CellSize()
    {
        const int n = (g_State.board.width > g_State.board.height)
                    ? g_State.board.width : g_State.board.height;
        return BOARD_SIZE / static_cast<float>(n > 0 ? n : 1);
    }

    /**
     * @brief  スクリーン座標を盤面のセル座標へ変換する
     * @param  sx スクリーンX座標
     * @param  sy スクリーンY座標
     * @return 命中したセル座標。盤外なら {-1,-1}
     */
    Vec2 ScreenToCell(int sx, int sy)
    {
        const float rx = sx - BoardOriginX();
        const float ry = sy - BoardOriginY();
        if (rx < 0 || ry < 0 || rx >= BOARD_SIZE || ry >= BOARD_SIZE)
        {
            return { -1, -1 };
        }
        const float CELL_SIZE = CellSize();
        return { static_cast<int>(rx / CELL_SIZE),
                 static_cast<int>(ry / CELL_SIZE) };
    }

    //======================================
    // 所持アビリティ一覧のレイアウト
    //   画面左下にヘッダ + count行を配置する。Draw/クリック判定で共通使用。
    //======================================
    /**
     * @brief  アビリティ一覧 (ヘッダ含む) の上端Y座標
     * @param  count 一覧の行数
     * @return ヘッダ行の上端Y座標
     */
    float AbilityListTop(int count)
    {
        return static_cast<float>(Direct3D_GetBackBufferHeight())
             - 24.0f - (TEXT_SIZE_HUD + 6.0f + count * ABILITY_ROW_H);
    }

    /**
     * @brief  index 番目のアビリティ行の上端Y座標
     * @param  index 行番号 (0始まり)
     * @param  count 一覧の行数
     * @return 該当行の上端Y座標
     */
    float AbilityRowY(int index, int count)
    {
        return AbilityListTop(count) + TEXT_SIZE_HUD + 6.0f
             + index * ABILITY_ROW_H;
    }

    //======================================
    // 所持アビリティのグルーピング
    //======================================
    /**
     * @struct AbilityGroup
     * @brief  同名アビリティをまとめた表示単位
     * @detail 同じアビリティを複数取得した場合、1グループ + 取得数で表す
     *         (例: 氷駒を3個 → "氷駒 ×3")。
     */
    struct AbilityGroup
    {
        Ability*              rep   = nullptr;  // 代表インスタンス (名前/説明/レアリティ表示用)
        int                   count = 0;        // 取得回数
        std::vector<Ability*> instances;        // 同名アビリティの実体一覧
    };

    /**
     * @brief  プレイヤー所持アビリティを同名でまとめ、表示順にソートして返す
     * @return グループ一覧。消費系(任意発動)を先頭に、続いてレアリティ降順。
     * @detail HUDの5枠表示・履歴ポップアップの双方で共通利用する。
     */
    std::vector<AbilityGroup> BuildAbilityGroups()
    {
        std::vector<AbilityGroup> groups;
        for (const auto& a : RunState::PlayerAbilities())
        {
            // 同名グループがあれば実体を追加、無ければ新規グループを作る
            AbilityGroup* found = nullptr;
            for (auto& g : groups)
            {
                if (g.rep->name == a->name) { found = &g; break; }
            }
            if (found)
            {
                found->instances.push_back(a.get());
                ++found->count;
            }
            else
            {
                AbilityGroup g;
                g.rep   = a.get();
                g.count = 1;
                g.instances.push_back(a.get());
                groups.push_back(std::move(g));
            }
        }
        // 消費系(任意発動)を優先し、その後はレアリティの高い順に並べる
        std::stable_sort(groups.begin(), groups.end(),
            [](const AbilityGroup& x, const AbilityGroup& y)
            {
                if (x.rep->activatable != y.rep->activatable)
                    return x.rep->activatable;   // 任意発動を先頭へ
                return static_cast<int>(x.rep->rarity)
                     > static_cast<int>(y.rep->rarity);
            });
        return groups;
    }

    /**
     * @brief  グループの残りチャージ合計を返す
     * @param  g 対象グループ
     * @return チャージ合計。無制限(回数表示なし)を含む場合は -1。
     */
    int GroupCharges(const AbilityGroup& g)
    {
        int total = 0;
        for (Ability* a : g.instances)
        {
            const int c = a->ChargesLeft();
            if (c < 0) return -1;   // 無制限が混在 → 回数表示なし
            total += c;
        }
        return total;
    }

    /**
     * @brief  グループ内に発動可能なインスタンスがあるか判定
     * @param  g 対象グループ
     * @return 1つでも発動可能なら true
     */
    bool GroupCanActivate(const AbilityGroup& g)
    {
        for (Ability* a : g.instances)
        {
            if (a->CanActivate(g_State)) return true;
        }
        return false;
    }

    /**
     * @brief  グループ内の発動可能なインスタンスを1つ発動する
     * @param  g 対象グループ
     * @detail 取得順で最初に発動できたものを使用し、加算フラッシュを出す。
     */
    void GroupActivate(const AbilityGroup& g)
    {
        for (Ability* a : g.instances)
        {
            if (a->CanActivate(g_State))
            {
                a->Activate(g_State);
                g_TimerFlash = TIMER_FLASH_DURATION;   // 加算フィードバック
                return;
            }
        }
    }

    /**
     * @brief  レアリティに対応する表示色を返す
     * @param  r レアリティ
     */
    DirectX::XMFLOAT4 RarityColor(Rarity r)
    {
        switch (r)
        {
        case Rarity::Common:    return { 0.70f, 0.75f, 0.80f, 1.0f };
        case Rarity::Rare:      return { 0.40f, 0.70f, 1.00f, 1.0f };
        case Rarity::Epic:      return { 0.80f, 0.50f, 1.00f, 1.0f };
        case Rarity::Legendary: return { 1.00f, 0.80f, 0.30f, 1.0f };
        case Rarity::Curse:     return { 1.00f, 0.40f, 0.40f, 1.0f };
        }
        return { 1, 1, 1, 1 };
    }

    /**
     * @brief  レアリティに対応する日本語名を返す
     * @param  r レアリティ
     */
    const char* RarityName(Rarity r)
    {
        switch (r)
        {
        case Rarity::Common:    return "コモン";
        case Rarity::Rare:      return "レア";
        case Rarity::Epic:      return "エピック";
        case Rarity::Legendary: return "レジェンダリー";
        case Rarity::Curse:     return "呪い";
        }
        return "";
    }

    //======================================
    // ターン進行
    //======================================
    /**
     * @brief  指定陣営のターンを開始する
     * @param  p 手番を握る陣営
     * @detail 着手数・思考時間をフックから取得し、敵番ならAI着手を予約する。
     */
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

    /**
     * @brief  ボスを生成し、アビリティをレジストリへ登録する
     * @detail プロト版は固定でボス1。プレイヤー側を先に登録することで
     *         OnPlace がプレイヤー効果 → ボス効果の順に発火する。
     */
    void LoadBoss()
    {
        // 現在のボス番号に応じてロスターからボスを生成する(データ駆動)。
        g_Boss = BossRoster::Create(RunState::CurrentBossIndex());
        if (!g_Boss) return;   // 念のためのガード(通常は到達しない)
        g_BossAbilities = g_Boss->GetBossAbilities();
        g_IceSlideRef.reset();
        g_SpiralRef.reset();

        // プレイヤー側を先に登録 → OnPlace はプレイヤー効果が先に発火する。
        // (氷駒が「置いた直後の駒」を滑らせてから、氷盤が全体を滑らせる)
        for (auto& a : RunState::PlayerAbilities())
        {
            a->RegisterTo(g_Registry);
        }
        // ボス側を後に登録 (ITurnHandler等のチェインはボス側が最終ラッパ)
        for (auto& a : g_BossAbilities)
        {
            a->RegisterTo(g_Registry);
            // 方向/回転インジケータ描画用に、該当ギミックの参照を控える
            if (auto ice = std::dynamic_pointer_cast<BossIceSlideAbility>(a))
            {
                g_IceSlideRef = ice;
            }
            if (auto spiral = std::dynamic_pointer_cast<BossSpiralRotateAbility>(a))
            {
                g_SpiralRef = spiral;
            }
        }
    }

    /**
     * @brief  対戦を初期状態へリセットする
     * @detail 盤面・演出・レジストリを初期化し、ボスを再ロードして
     *         プレイヤーの先手ターンを開始する。
     */
    void ResetMatch()
    {
        /*--- 盤面・対戦状態の初期化 ---*/
        g_State = {};
        g_State.board.Reset();
        g_State.result = MatchResult::Playing;
        g_State.winner = Piece::Empty;
        g_State.turnCount = 1;

        /*--- フック・演出状態の初期化 ---*/
        g_Registry.ResetToDefaults();
        g_LastPlacedPos    = { -1, -1 };
        g_LastPlacedPiece  = Piece::Empty;
        g_LastPlacedTimer  = 0.0;
        g_GameOverDwell    = 0.0;
        g_AnimPhases.clear();
        g_PhaseStartBoards.clear();
        g_AnimPhaseIndex   = 0;
        g_AnimPhaseTimer   = 0.0;
        g_AbilityPopupOpen = false;
        g_PopupScroll      = 0.0;

        /*--- ボス再ロード → 盤面サイズ確定 → 先手ターン開始 ---*/
        LoadBoss();

        // 盤面サイズフックを適用 (盤面拡張アビリティ等を反映)
        int boardW = 3, boardH = 3;
        g_Registry.GetBoardSize(boardW, boardH);
        g_State.board.Resize(boardW, boardH);

        BeginTurn(Piece::Player);
    }

    //======================================
    // 着手後の判定
    //======================================
    /**
     * @brief  駒を1個置いた直後の勝敗・ターン進行を評価する
     * @param  justPlaced 直前に駒を置いた陣営
     * @detail 勝利→引き分け→追加着手→ターン終了の順で判定する。
     *         OnPlace は PlaceAndEvaluate 側で先に呼ぶため、ここでは扱わない。
     *         勝利判定はボスギミック(氷盤スライド等)で盤面が動いた後の
     *         最終盤面に対して行い、着手側だけでなく両陣営を確認する。
     */
    void EvaluateAfterPlace(Piece justPlaced)
    {
        // 1) 設置時イベント (アビリティ用フック)
        // ※ EvaluateAfterPlace の前にOnPlaceを呼んでいるので、ここでは省略。
        //    OnPlaceは PlaceAndEvaluate 側で先に呼ぶ。

        // 2) 勝利判定
        // ボスギミックで全駒が動くと、着手していない側の駒が揃うこともある。
        // そのため着手側だけでなく両陣営の勝利条件を確認する。
        const bool playerWon = g_Registry.CheckWin(g_State.board, Piece::Player);
        const bool enemyWon  = g_Registry.CheckWin(g_State.board, Piece::Enemy);
        if (playerWon || enemyWon)
        {
            // 双方同時成立時はプレイヤーを優先 (理不尽な敗北を避ける)
            const Piece winner = playerWon ? Piece::Player : Piece::Enemy;
            g_State.winner = winner;
            g_State.result = (winner == Piece::Player)
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

    /**
     * @brief  1フェーズ分の移動を盤面へ適用する
     * @param  board 対象の盤面 (更新される)
     * @param  phase 適用する移動リスト
     * @detail 全sourceを空にしてから全destへ配置する (同時移動を表現)。
     */
    void ApplyPhase(Board& board, const BoardOps::MoveList& phase)
    {
        for (const auto& m : phase) board.Set(m.fromX, m.fromY, Piece::Empty);
        for (const auto& m : phase) board.Set(m.toX,   m.toY,   m.piece);
    }

    /**
     * @brief  OnPlace で積まれた効果フェーズ列をアニメ再生用にセットアップ
     * @param  boardStart 効果適用前 (新駒設置済み) の盤面
     * @detail 各フェーズ開始時の盤面を前計算し、描画時の中間状態復元に使う。
     */
    void StartEffectAnimation(const Board& boardStart)
    {
        g_AnimPhases     = g_State.animPhases;
        g_PhaseStartBoards.clear();
        g_AnimPhaseIndex = 0;
        g_AnimPhaseTimer = 0.0;
        if (g_AnimPhases.empty()) return;

        // 各フェーズ開始時の盤面を前計算 (描画時の中間状態復元に使う)
        g_PhaseStartBoards.reserve(g_AnimPhases.size());
        Board cur = boardStart;
        for (const auto& phase : g_AnimPhases)
        {
            g_PhaseStartBoards.push_back(cur);
            ApplyPhase(cur, phase);
        }
        g_AnimPhaseTimer = PHASE_DURATION;
    }

    /**
     * @brief  駒を1個置き、フック発火と勝敗評価まで行う
     * @param  pos      設置先のセル座標
     * @param  placedBy 置く陣営
     * @detail 空きマスでなければ何もしない。設置 → 演出記録 →
     *         OnPlace フック → 効果アニメ準備 → 勝敗評価 の順に処理する。
     */
    void PlaceAndEvaluate(Vec2 pos, Piece placedBy)
    {
        if (!g_State.board.IsEmpty(pos.x, pos.y)) return;
        g_State.board.Set(pos.x, pos.y, placedBy);
        --g_State.placementsRemaining;

        // 設置時演出 (フック発火前に位置を記録)
        g_LastPlacedPos   = pos;
        g_LastPlacedPiece = placedBy;
        g_LastPlacedTimer = GLOW_DURATION;

        // 効果フックの出力をクリアし、OnPlace前 (新駒設置済み) の盤面を控える
        g_State.animPhases.clear();
        const Board boardStart = g_State.board;

        // 設置時フック (氷駒/氷盤/連鎖/爆弾等が animPhases に効果を積む)
        g_Registry.OnPlace(g_State, pos, placedBy);

        // 積まれた効果フェーズを順次アニメ再生
        StartEffectAnimation(boardStart);

        EvaluateAfterPlace(placedBy);
    }

    //======================================
    // 入力 / AI
    //======================================
    /**
     * @brief  プレイヤーのマウス入力を処理し、駒を設置する
     * @detail プレイヤーの手番かつ左クリック、命中マスが空のときのみ設置。
     */
    void HandlePlayerInput()
    {
        if (g_State.currentPlayer != Piece::Player) return;
        if (!InputManager_IsMouseLeftTrigger()) return;
        const auto& mouse = InputManager_GetMouseState();
        const Vec2 cell = ScreenToCell(mouse.x, mouse.y);
        if (!g_State.board.IsEmpty(cell.x, cell.y)) return;
        PlaceAndEvaluate(cell, Piece::Player);
    }

    /**
     * @brief  所持アビリティ一覧(HUD)のクリックを処理する
     * @return クリックを消費したら true (= 駒設置へ回さない)
     * @detail 任意発動ボタンの命中で発動、「……」行で履歴ポップアップを開く。
     *         発動不可でもボタン命中ならクリックを消費する。
     */
    bool HandleAbilityClick()
    {
        if (!InputManager_IsMouseLeftTrigger()) return false;

        const auto groups = BuildAbilityGroups();
        if (groups.empty()) return false;

        const int  total    = static_cast<int>(groups.size());
        const int  visible  = (total < ABILITY_VISIBLE_MAX)
                             ? total : ABILITY_VISIBLE_MAX;
        const bool showMore = total > ABILITY_VISIBLE_MAX;
        const int  rowCount = visible + (showMore ? 1 : 0);
        const auto& mouse   = InputManager_GetMouseState();

        // 「……」行: 履歴ポップアップを開く (手番を問わず操作可)
        if (showMore)
        {
            const float ry = AbilityRowY(visible, rowCount);
            if (mouse.x >= ABILITY_LIST_X && mouse.x <= ABILITY_LIST_X + ABILITY_BTN_W
             && mouse.y >= ry && mouse.y <= ry + ABILITY_BTN_H)
            {
                g_AbilityPopupOpen = true;
                g_PopupScroll      = 0.0;
                return true;
            }
        }

        // 任意発動ボタン: プレイヤー手番のみ発動できる
        if (g_State.currentPlayer != Piece::Player) return false;
        for (int i = 0; i < visible; ++i)
        {
            if (!groups[i].rep->activatable) continue;
            const float ry = AbilityRowY(i, rowCount);
            if (mouse.x < ABILITY_LIST_X || mouse.x > ABILITY_LIST_X + ABILITY_BTN_W) continue;
            if (mouse.y < ry || mouse.y > ry + ABILITY_BTN_H) continue;

            // ボタンに命中 — 発動可能なら発動 (不可でもクリックは消費する)
            if (GroupCanActivate(groups[i])) GroupActivate(groups[i]);
            return true;
        }
        return false;
    }

    /**
     * @brief  AIの手番を処理する
     * @param  dt 前フレームからの経過秒数
     * @detail AI_DELAY 経過後に着手。追加着手が残れば再度AIを予約する。
     */
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

    /**
     * @brief  プレイヤー手番の思考時間を消費させる
     * @param  dt 前フレームからの経過秒数
     * @detail 残時間が尽きたら敗北フックを試行し、打ち消されなければ時間切れ。
     */
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

    //======================================
    // 描画
    //======================================
    /**
     * @brief  1マス分の駒を描画する
     * @param  p      駒の陣営 (Empty なら何も描かない)
     * @param  cx     セル中心のX座標
     * @param  cy     セル中心のY座標
     * @param  radius 駒の半径
     */
    void DrawPiece(Piece p, float cx, float cy, float radius)
    {
        switch (p)
        {
        case Piece::Player:
            Prim::DrawCircle(cx, cy, radius, PIECE_THICKNESS, COLOR_PIECE_O);
            break;
        case Piece::Enemy:
            Prim::DrawCross(cx, cy, radius, PIECE_THICKNESS, COLOR_PIECE_X);
            break;
        default:
            break;
        }
    }

    /**
     * @brief  指定方向を指すシェブロン矢印 ( > < ^ v ) を描画する
     * @param  cx        矢印中心のX座標
     * @param  cy        矢印中心のY座標
     * @param  d         指し示す方向
     * @param  size      矢印の大きさ
     * @param  thickness 線の太さ
     * @param  color     描画色
     */
    void DrawChevron(float cx, float cy, Direction d,
                     float size, float thickness, const DirectX::XMFLOAT4& color)
    {
        // 方向ごとに頂点(apex)と両翼端(a1,a2)の座標を決定する
        float apexX, apexY, a1x, a1y, a2x, a2y;
        switch (d)
        {
        case Direction::Right:
            apexX = cx + size; apexY = cy;
            a1x = cx - size;   a1y = cy - size;
            a2x = cx - size;   a2y = cy + size;
            break;
        case Direction::Left:
            apexX = cx - size; apexY = cy;
            a1x = cx + size;   a1y = cy - size;
            a2x = cx + size;   a2y = cy + size;
            break;
        case Direction::Down:
            apexX = cx;        apexY = cy + size;
            a1x = cx - size;   a1y = cy - size;
            a2x = cx + size;   a2y = cy - size;
            break;
        case Direction::Up:
        default:
            apexX = cx;        apexY = cy - size;
            a1x = cx - size;   a1y = cy + size;
            a2x = cx + size;   a2y = cy + size;
            break;
        }
        // 翼端→頂点→翼端 の2本の線で「く」の字を描く
        Prim::DrawLine(a1x, a1y, apexX, apexY, thickness, color);
        Prim::DrawLine(apexX, apexY, a2x, a2y, thickness, color);
    }

    /**
     * @brief  盤面の罫線と駒を描画する
     * @detail 効果アニメ中はフェーズ中間状態を補間描画し、
     *         非アニメ時は現在の盤面をそのまま描画する。
     */
    void DrawBoardAndPieces()
    {
        const float ox = BoardOriginX();
        const float oy = BoardOriginY();

        // 当該フレームの1マスサイズ (盤面拡張で変動するため動的取得)
        const float CELL_SIZE = CellSize();

        /*--- 盤面背景と罫線 ---*/
        Prim::DrawRect(ox - 8, oy - 8, BOARD_SIZE + 16, BOARD_SIZE + 16, COLOR_BG);

        // 内側の罫線を盤面サイズに応じて引く (3×3なら2本、4×4なら3本)
        const int cols = g_State.board.width;
        const int rows = g_State.board.height;
        for (int i = 1; i < cols; ++i)
        {
            const float gx = ox + CELL_SIZE * i;
            Prim::DrawLine(gx, oy, gx, oy + CELL_SIZE * rows, LINE_THICKNESS, COLOR_GRID);
        }
        for (int i = 1; i < rows; ++i)
        {
            const float gy = oy + CELL_SIZE * i;
            Prim::DrawLine(ox, gy, ox + CELL_SIZE * cols, gy, LINE_THICKNESS, COLOR_GRID);
        }

        const float halfCell    = CELL_SIZE * 0.5f;
        const float pieceRadius = CELL_SIZE * PIECE_RADIUS_RATIO;

        /*--- 直近設置のグロー (拡大半径・低不透明度の同色形状を後ろに重ねる) ---*/
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

        if (IsEffectAnimating())
        {
            /*--- 効果アニメ中: 再生中フェーズの中間状態を描画 ---*/
            const int                 phaseIdx = g_AnimPhaseIndex;
            const Board&              disp     = g_PhaseStartBoards[phaseIdx];
            const BoardOps::MoveList& phase    = g_AnimPhases[phaseIdx];

            // 進捗 0→1 (ease-out で滑らかに減速)
            float raw = 1.0f - static_cast<float>(g_AnimPhaseTimer / PHASE_DURATION);
            raw = (raw < 0.0f) ? 0.0f : (raw > 1.0f ? 1.0f : raw);
            const float t = 1.0f - (1.0f - raw) * (1.0f - raw);

            // このフェーズで動かない駒を静止描画 (出発マスの駒は補間描画に回す)
            for (int y = 0; y < disp.height; ++y)
            {
                for (int x = 0; x < disp.width; ++x)
                {
                    bool isMover = false;
                    for (const auto& m : phase)
                    {
                        if (m.fromX == x && m.fromY == y) { isMover = true; break; }
                    }
                    if (isMover) continue;
                    const float cx = ox + x * CELL_SIZE + halfCell;
                    const float cy = oy + y * CELL_SIZE + halfCell;
                    DrawPiece(disp.Get(x, y), cx, cy, pieceRadius);
                }
            }
            // 移動駒を from→to で補間描画
            for (const auto& m : phase)
            {
                const float fx = m.fromX + (m.toX - m.fromX) * t;
                const float fy = m.fromY + (m.toY - m.fromY) * t;
                const float cx = ox + fx * CELL_SIZE + halfCell;
                const float cy = oy + fy * CELL_SIZE + halfCell;
                DrawPiece(m.piece, cx, cy, pieceRadius);
            }
        }
        else
        {
            /*--- 非アニメ時: 現在の盤面をそのまま描画 ---*/
            for (int y = 0; y < g_State.board.height; ++y)
            {
                for (int x = 0; x < g_State.board.width; ++x)
                {
                    const float cx = ox + x * CELL_SIZE + halfCell;
                    const float cy = oy + y * CELL_SIZE + halfCell;
                    DrawPiece(g_State.board.Get(x, y), cx, cy, pieceRadius);
                }
            }
        }
    }

    /**
     * @brief  氷盤が次に滑る方向を、辺に沿ったシェブロン矢印で可視化する
     * @detail 氷盤ボスが存在しプレイ中のときのみ描画。矢印は滑る方向へ
     *         流れる脈動アニメーションを伴う。
     */
    void DrawDirectionIndicator()
    {
        if (!g_IceSlideRef) return;
        if (g_State.result != MatchResult::Playing) return;

        const Direction d  = g_IceSlideRef->currentDir;
        const float     ox = BoardOriginX();
        const float     oy = BoardOriginY();
        const bool horizontalSlide = (d == Direction::Left || d == Direction::Right);

        for (int i = 0; i < CHEVRON_COUNT; ++i)
        {
            // 辺に沿った位置 (端を避けて等間隔に分散)
            const float frac  = (i + 1.0f) / (CHEVRON_COUNT + 1.0f);
            const float along = BOARD_SIZE * frac;

            float cx, cy;
            if (horizontalSlide)
            {
                // 左右スライド → 縦の辺に沿って縦並び
                cy = oy + along;
                cx = (d == Direction::Right) ? (ox + BOARD_SIZE + CHEVRON_MARGIN)
                                             : (ox - CHEVRON_MARGIN);
            }
            else
            {
                // 上下スライド → 横の辺に沿って横並び
                cx = ox + along;
                cy = (d == Direction::Down) ? (oy + BOARD_SIZE + CHEVRON_MARGIN)
                                            : (oy - CHEVRON_MARGIN);
            }

            // 滑る方向へ流れる脈動 (位相を矢印ごとにずらす)
            const float wave = 0.5f + 0.5f * std::sin(
                static_cast<float>(g_AnimClock) * 5.0f - i * 0.9f);
            DirectX::XMFLOAT4 col = COLOR_SLIDE_ARROW;
            col.w = 0.30f + 0.60f * wave;

            DrawChevron(cx, cy, d, CHEVRON_SIZE, CHEVRON_THICKNESS, col);
        }
    }

    /**
     * @brief  螺旋盤が回転する向きを、盤の四辺を巡る循環シェブロンで可視化する
     * @detail 螺旋盤ボスが存在しプレイ中のときのみ描画。各辺のシェブロンを
     *         循環方向に向け、脈動を循環順に走らせて回転方向を提示する。
     */
    void DrawRotationIndicator()
    {
        if (!g_SpiralRef) return;
        if (g_State.result != MatchResult::Playing) return;

        const float ox   = BoardOriginX();
        const float oy   = BoardOriginY();
        const float half = BOARD_SIZE * 0.5f;
        const bool  cw   = g_SpiralRef->clockwise;

        // 四辺のシェブロンを「循環順」に並べる。各辺は循環方向を指す。
        struct Spot { float cx, cy; Direction dir; };
        Spot spots[4];
        if (cw)
        {
            // 時計回り: 上(右向き)→右(下向き)→下(左向き)→左(上向き)
            spots[0] = { ox + half,                        oy - CHEVRON_MARGIN,            Direction::Right };
            spots[1] = { ox + BOARD_SIZE + CHEVRON_MARGIN, oy + half,                      Direction::Down  };
            spots[2] = { ox + half,                        oy + BOARD_SIZE + CHEVRON_MARGIN, Direction::Left };
            spots[3] = { ox - CHEVRON_MARGIN,              oy + half,                      Direction::Up    };
        }
        else
        {
            // 反時計回り: 上(左向き)→左(下向き)→下(右向き)→右(上向き)
            spots[0] = { ox + half,                        oy - CHEVRON_MARGIN,            Direction::Left  };
            spots[1] = { ox - CHEVRON_MARGIN,              oy + half,                      Direction::Down  };
            spots[2] = { ox + half,                        oy + BOARD_SIZE + CHEVRON_MARGIN, Direction::Right };
            spots[3] = { ox + BOARD_SIZE + CHEVRON_MARGIN, oy + half,                      Direction::Up    };
        }

        for (int i = 0; i < 4; ++i)
        {
            // 循環順(spots順)に脈動を走らせて回転の流れを表現する
            const float wave = 0.5f + 0.5f * std::sin(
                static_cast<float>(g_AnimClock) * 5.0f - i * 0.9f);
            DirectX::XMFLOAT4 col = COLOR_ROTATE_ARROW;
            col.w = 0.30f + 0.60f * wave;

            DrawChevron(spots[i].cx, spots[i].cy, spots[i].dir,
                        CHEVRON_SIZE, CHEVRON_THICKNESS, col);
        }
    }

    /**
     * @brief  HUD (残時間・手番・ボス名・所持アビリティ) を描画する
     */
    void DrawHud()
    {
        const float screenW = static_cast<float>(Direct3D_GetBackBufferWidth());

        /*--- 残り思考時間 (左上) ---*/
        char buf[64];
        const int total   = static_cast<int>(g_State.remainingTime + 0.999);
        const int minutes = total / 60;
        const int seconds = total % 60;
        std::snprintf(buf, sizeof(buf), "残り %d:%02d", minutes, seconds);
        DirectX::XMFLOAT4 timeColor =
            (g_State.remainingTime < 30.0
             && g_State.result == MatchResult::Playing
             && g_State.currentPlayer == Piece::Player)
                ? COLOR_TEXT_URGENT : COLOR_TEXT;
        if (g_TimerFlash > 0.0) timeColor = COLOR_TEXT_WIN;  // 時間加算フラッシュ
        Text::Draw(24.0f, 24.0f, buf, TEXT_SIZE_HUD, timeColor);

        /*--- 集中などで加算した直後の「+2:00」浮き上がり表示 ---*/
        if (g_TimerFlash > 0.0)
        {
            const float k = static_cast<float>(g_TimerFlash / TIMER_FLASH_DURATION); // 1→0
            DirectX::XMFLOAT4 popColor = COLOR_TEXT_WIN;
            popColor.w = k;
            const float popY = 24.0f - (1.0f - k) * 22.0f;
            Text::Draw(24.0f + Text::MeasureWidth(buf, TEXT_SIZE_HUD) + 14.0f,
                       popY, "+2:00", TEXT_SIZE_HINT, popColor);
        }

        /*--- 現在の手番表示 (右上) ---*/
        if (g_State.result == MatchResult::Playing)
        {
            const char* turn =
                (g_State.currentPlayer == Piece::Player) ? "あなたの番（〇）" : "ボスの番（×）";
            const float w = Text::MeasureWidth(turn, TEXT_SIZE_HUD);
            Text::Draw(screenW - w - 24.0f, 24.0f, turn, TEXT_SIZE_HUD, COLOR_TEXT);
        }

        /*--- ボス情報 (画面中央上)。名前の下に能力フレーバーを表示 ---*/
        if (g_Boss)
        {
            char nameBuf[64];
            std::snprintf(nameBuf, sizeof(nameBuf), "ボス：%s", g_Boss->name.c_str());
            const float nameW = Text::MeasureWidth(nameBuf, TEXT_SIZE_HUD);
            Text::Draw((screenW - nameW) * 0.5f, 24.0f, nameBuf,
                       TEXT_SIZE_HUD, COLOR_TEXT_URGENT);

            // フレーバーテキスト (ボス能力の簡単な説明)
            if (!g_Boss->description.empty())
            {
                const float flavorW =
                    Text::MeasureWidth(g_Boss->description.c_str(), TEXT_SIZE_HINT);
                Text::Draw((screenW - flavorW) * 0.5f, 24.0f + TEXT_SIZE_HUD + 4.0f,
                           g_Boss->description.c_str(), TEXT_SIZE_HINT, COLOR_TEXT_HINT);
            }
        }
        // 盤面ギミックの方向は Draw(Direction/Rotation)Indicator のシェブロンで可視化

        /*--- 所持アビリティ (画面左下) ---*/
        // 5グループまで表示し、超過分は「……」行から履歴ポップアップへ。
        const auto groups = BuildAbilityGroups();
        if (!groups.empty())
        {
            const int  groupCount = static_cast<int>(groups.size());
            const int  visible    = (groupCount < ABILITY_VISIBLE_MAX)
                                   ? groupCount : ABILITY_VISIBLE_MAX;
            const bool showMore   = groupCount > ABILITY_VISIBLE_MAX;
            const int  rowCount   = visible + (showMore ? 1 : 0);
            const float listTop   = AbilityListTop(rowCount);
            Text::Draw(ABILITY_LIST_X, listTop, "所持アビリティ",
                       TEXT_SIZE_HUD, COLOR_TEXT);

            const auto& mouse = InputManager_GetMouseState();
            const bool playerTurn =
                (g_State.result == MatchResult::Playing
                 && g_State.currentPlayer == Piece::Player
                 && !IsEffectAnimating()
                 && !g_AbilityPopupOpen);

            for (int i = 0; i < visible; ++i)
            {
                const AbilityGroup& g = groups[i];
                const float ry = AbilityRowY(i, rowCount);

                if (g.rep->activatable)
                {
                    // 任意発動(消費系): クリック可能ボタン
                    const bool usable = playerTurn && GroupCanActivate(g);
                    const bool hover  = usable
                        && mouse.x >= ABILITY_LIST_X
                        && mouse.x <= ABILITY_LIST_X + ABILITY_BTN_W
                        && mouse.y >= ry && mouse.y <= ry + ABILITY_BTN_H;

                    // 縁取り (発動可ならアクセント色、不可なら半透明)
                    DirectX::XMFLOAT4 accent = usable ? COLOR_SLIDE_ARROW
                                                      : COLOR_TEXT_HINT;
                    if (!usable) accent.w = 0.40f;
                    Prim::DrawRect(ABILITY_LIST_X - 3.0f, ry - 3.0f,
                                   ABILITY_BTN_W + 6.0f, ABILITY_BTN_H + 6.0f, accent);
                    Prim::DrawRect(ABILITY_LIST_X, ry, ABILITY_BTN_W, ABILITY_BTN_H,
                                   hover ? COLOR_BTN_HOVER : COLOR_BTN_BG);

                    // ラベル (名前 + 取得数×N + 残りチャージ)
                    char countPart[16] = "";
                    if (g.count > 1)
                        std::snprintf(countPart, sizeof(countPart), " ×%d", g.count);
                    char label[128];
                    const int charges = GroupCharges(g);
                    if (charges >= 0)
                        std::snprintf(label, sizeof(label), "%s%s  残%d",
                                      g.rep->name.c_str(), countPart, charges);
                    else
                        std::snprintf(label, sizeof(label), "%s%s",
                                      g.rep->name.c_str(), countPart);
                    DirectX::XMFLOAT4 txtCol = COLOR_TEXT;
                    if (!usable) txtCol.w = 0.5f;
                    Text::Draw(ABILITY_LIST_X + 12.0f,
                               ry + (ABILITY_BTN_H - TEXT_SIZE_HINT) * 0.5f,
                               label, TEXT_SIZE_HINT, txtCol);
                }
                else
                {
                    // パッシブ: テキスト表示のみ
                    char rowBuf[128];
                    if (g.count > 1)
                        std::snprintf(rowBuf, sizeof(rowBuf), "・%s ×%d",
                                      g.rep->name.c_str(), g.count);
                    else
                        std::snprintf(rowBuf, sizeof(rowBuf), "・%s",
                                      g.rep->name.c_str());
                    Text::Draw(ABILITY_LIST_X,
                               ry + (ABILITY_ROW_H - TEXT_SIZE_HINT) * 0.5f,
                               rowBuf, TEXT_SIZE_HINT, COLOR_TEXT_HINT);
                }
            }

            /*--- 超過分: 「……」行 (クリックで履歴ポップアップを開く) ---*/
            if (showMore)
            {
                const float ry = AbilityRowY(visible, rowCount);
                const bool hover = !g_AbilityPopupOpen
                    && mouse.x >= ABILITY_LIST_X
                    && mouse.x <= ABILITY_LIST_X + ABILITY_BTN_W
                    && mouse.y >= ry && mouse.y <= ry + ABILITY_BTN_H;
                Prim::DrawRect(ABILITY_LIST_X - 3.0f, ry - 3.0f,
                               ABILITY_BTN_W + 6.0f, ABILITY_BTN_H + 6.0f,
                               COLOR_TEXT_HINT);
                Prim::DrawRect(ABILITY_LIST_X, ry, ABILITY_BTN_W, ABILITY_BTN_H,
                               hover ? COLOR_BTN_HOVER : COLOR_BTN_BG);
                char moreLabel[64];
                std::snprintf(moreLabel, sizeof(moreLabel),
                              "……  他%d件を見る", groupCount - visible);
                Text::Draw(ABILITY_LIST_X + 12.0f,
                           ry + (ABILITY_BTN_H - TEXT_SIZE_HINT) * 0.5f,
                           moreLabel, TEXT_SIZE_HINT, COLOR_TEXT);
            }
        }
    }

    //======================================
    // ボタンUI
    //======================================
    /**
     * @struct UiButton
     * @brief  矩形ボタン1個の領域とラベル
     */
    struct UiButton
    {
        float x, y, w, h;     // ボタンの矩形領域
        const char* label;    // 表示ラベル
    };

    /**
     * @brief  座標がボタン領域内かを判定する
     * @param  b  対象ボタン
     * @param  mx 判定するX座標
     * @param  my 判定するY座標
     * @return 領域内なら true
     */
    bool ButtonContains(const UiButton& b, int mx, int my)
    {
        return mx >= b.x && mx <= b.x + b.w
            && my >= b.y && my <= b.y + b.h;
    }

    /**
     * @brief  ボタンを描画する (縁取り + 本体 + 中央ラベル)
     * @param  b      対象ボタン
     * @param  hover  ホバー中なら true (本体色が変わる)
     * @param  accent 縁取りのアクセント色
     */
    void DrawButton(const UiButton& b, bool hover, const DirectX::XMFLOAT4& accent)
    {
        constexpr float border = 3.0f;
        Prim::DrawRect(b.x - border, b.y - border,
                       b.w + border * 2.0f, b.h + border * 2.0f, accent);
        Prim::DrawRect(b.x, b.y, b.w, b.h,
                       hover ? COLOR_BTN_HOVER : COLOR_BTN_BG);
        const float tw = Text::MeasureWidth(b.label, TEXT_SIZE_BUTTON);
        Text::Draw(b.x + (b.w - tw) * 0.5f,
                   b.y + (b.h - TEXT_SIZE_BUTTON) * 0.5f,
                   b.label, TEXT_SIZE_BUTTON, COLOR_TEXT);
    }

    //--------------------------------------
    // ゲームオーバー画面のボタン定義
    //--------------------------------------
    /**
     * @enum  GameOverAction
     * @brief ゲームオーバー画面のボタンが実行する操作
     */
    enum class GameOverAction { Reward, Restart, Title };

    /**
     * @struct GameOverButton
     * @brief  ゲームオーバー画面のボタン1個 (領域 + 操作 + 色)
     */
    struct GameOverButton
    {
        UiButton          btn;     // ボタン領域とラベル
        GameOverAction    action;  // クリック時の操作
        DirectX::XMFLOAT4 accent;  // 縁取り色
    };

    /**
     * @brief  現在の結果に応じたボタン一覧を生成する
     * @return ゲームオーバー画面に表示するボタン群
     * @detail 勝利時は「報酬を受け取る」1個、それ以外は
     *         「リスタート」「タイトルへ」の2個。Update/Draw で共有する。
     */
    std::vector<GameOverButton> BuildGameOverButtons()
    {
        std::vector<GameOverButton> list;
        const float screenW = static_cast<float>(Direct3D_GetBackBufferWidth());
        const float screenH = static_cast<float>(Direct3D_GetBackBufferHeight());
        const float by = screenH * 0.5f + 30.0f;

        if (g_State.result == MatchResult::Win)
        {
            // 勝利: 報酬画面へ進むボタン1個
            const float x = (screenW - BUTTON_W) * 0.5f;
            list.push_back({ { x, by, BUTTON_W, BUTTON_H, "報酬を受け取る" },
                             GameOverAction::Reward, COLOR_TEXT_WIN });
        }
        else
        {
            // 敗北/引分/時間切れ: リスタート と タイトルへ の2個
            const float totalW = BUTTON_W * 2.0f + BUTTON_GAP;
            const float x0 = (screenW - totalW) * 0.5f;
            list.push_back({ { x0, by, BUTTON_W, BUTTON_H, "リスタート" },
                             GameOverAction::Restart, COLOR_PIECE_O });
            list.push_back({ { x0 + BUTTON_W + BUTTON_GAP, by,
                               BUTTON_W, BUTTON_H, "タイトルへ" },
                             GameOverAction::Title, COLOR_TEXT_HINT });
        }
        return list;
    }

    /**
     * @brief  ゲームオーバー時の暗転オーバーレイと結果表示を描画する
     * @detail プレイ中は何も描かない。結果メッセージを表示し、
     *         一定時間 (GAMEOVER_MIN_DWELL) 経過後にボタンを表示する。
     */
    void DrawGameOverOverlay()
    {
        if (g_State.result == MatchResult::Playing) return;

        const float screenW = static_cast<float>(Direct3D_GetBackBufferWidth());
        const float screenH = static_cast<float>(Direct3D_GetBackBufferHeight());

        // 画面全体を暗転
        Prim::DrawRect(0, 0, screenW, screenH, { 0.0f, 0.0f, 0.05f, 0.55f });

        /*--- 結果メッセージ ---*/
        const char* msg = "";
        DirectX::XMFLOAT4 color = COLOR_TEXT;
        switch (g_State.result)
        {
        case MatchResult::Win:     msg = "勝利！";   color = COLOR_TEXT_WIN; break;
        case MatchResult::Lose:    msg = "敗北";     color = COLOR_TEXT_LOSE; break;
        case MatchResult::Draw:    msg = "引き分け"; color = COLOR_TEXT_DRAW; break;
        case MatchResult::Timeout: msg = "時間切れ"; color = COLOR_TEXT_LOSE; break;
        default: break;
        }
        const float msgW = Text::MeasureWidth(msg, TEXT_SIZE_BIG);
        Text::Draw((screenW - msgW) * 0.5f,
                   screenH * 0.5f - TEXT_SIZE_BIG - 20.0f,
                   msg, TEXT_SIZE_BIG, color);

        // 結果を見せるための待機後にボタンを表示
        if (g_GameOverDwell < GAMEOVER_MIN_DWELL) return;

        /*--- 操作ボタン (ホバー判定込み) ---*/
        const auto& mouse = InputManager_GetMouseState();
        for (const auto& go : BuildGameOverButtons())
        {
            const bool hover = ButtonContains(go.btn, mouse.x, mouse.y);
            DrawButton(go.btn, hover, go.accent);
        }
    }

    //======================================
    // アビリティ履歴ポップアップ
    //======================================
    /**
     * @brief  ポップアップの「閉じる」ボタン領域を返す
     * @param  screenW 画面幅
     */
    UiButton PopupCloseButton(float screenW)
    {
        return { screenW - 160.0f, 28.0f, 130.0f, 44.0f, "閉じる" };
    }

    /**
     * @brief  ポップアップのカード横スクロール最大量を返す
     * @param  total   カード総数
     * @param  screenW 画面幅
     * @return スクロール可能量(px)。カードが画面に収まる場合は 0。
     */
    float PopupMaxScroll(int total, float screenW)
    {
        const float stripW   = total * POPUP_CARD_W
                             + (total - 1) * POPUP_CARD_GAP;
        const float viewport = screenW - POPUP_SIDE_MARGIN * 2.0f;
        return (stripW > viewport) ? (stripW - viewport) : 0.0f;
    }

    /**
     * @brief  index 番目のポップアップカードの左端X座標を返す
     * @param  index   カード番号 (0始まり)
     * @param  total   カード総数
     * @param  screenW 画面幅
     * @detail カード帯が画面に収まる場合は中央寄せ、溢れる場合は
     *         スクロール量 g_PopupScroll を反映する。
     */
    float PopupCardX(int index, int total, float screenW)
    {
        const float stripW   = total * POPUP_CARD_W
                             + (total - 1) * POPUP_CARD_GAP;
        const float viewport = screenW - POPUP_SIDE_MARGIN * 2.0f;
        const float baseX    = (stripW <= viewport)
            ? (screenW - stripW) * 0.5f
            : (POPUP_SIDE_MARGIN - static_cast<float>(g_PopupScroll));
        return baseX + index * (POPUP_CARD_W + POPUP_CARD_GAP);
    }

    /**
     * @brief  アビリティ履歴ポップアップを描画する
     * @detail 所持アビリティ全件を横並びカードで表示する。
     *         任意発動アビリティはクリックで発動できる。
     */
    void DrawAbilityPopup()
    {
        if (!g_AbilityPopupOpen) return;
        if (g_State.result != MatchResult::Playing) return;

        const float screenW = static_cast<float>(Direct3D_GetBackBufferWidth());
        const float screenH = static_cast<float>(Direct3D_GetBackBufferHeight());

        // 背景を暗転 (ゲーム画面・HUDより手前)
        Prim::DrawRect(0, 0, screenW, screenH, { 0.0f, 0.0f, 0.05f, 0.85f });

        // 見出し
        const char* title = "所持アビリティ一覧";
        const float titleW = Text::MeasureWidth(title, TEXT_SIZE_HUD);
        Text::Draw((screenW - titleW) * 0.5f, 30.0f,
                   title, TEXT_SIZE_HUD, COLOR_TEXT);

        // 閉じるボタン
        const auto& mouse = InputManager_GetMouseState();
        const UiButton closeBtn = PopupCloseButton(screenW);
        DrawButton(closeBtn, ButtonContains(closeBtn, mouse.x, mouse.y),
                   COLOR_TEXT_HINT);

        const auto  groups = BuildAbilityGroups();
        const int   total  = static_cast<int>(groups.size());
        const float cardY  = (screenH - POPUP_CARD_H) * 0.5f + 10.0f;
        const bool  playerTurn =
            (g_State.currentPlayer == Piece::Player && !IsEffectAnimating());

        /*--- カードを横並びで描画 (画面外は省略) ---*/
        for (int i = 0; i < total; ++i)
        {
            const AbilityGroup& g = groups[i];
            const float cx = PopupCardX(i, total, screenW);
            if (cx + POPUP_CARD_W < 0.0f || cx > screenW) continue;

            const auto rarityCol  = RarityColor(g.rep->rarity);
            const bool activatable = g.rep->activatable;
            const bool usable = activatable && playerTurn && GroupCanActivate(g);
            const bool hover  = usable
                && mouse.x >= cx && mouse.x <= cx + POPUP_CARD_W
                && mouse.y >= cardY && mouse.y <= cardY + POPUP_CARD_H;

            // 縁取り (レアリティ色) + 本体
            Prim::DrawRect(cx - 4.0f, cardY - 4.0f,
                           POPUP_CARD_W + 8.0f, POPUP_CARD_H + 8.0f, rarityCol);
            Prim::DrawRect(cx, cardY, POPUP_CARD_W, POPUP_CARD_H,
                           hover ? COLOR_BTN_HOVER : COLOR_BTN_BG);

            // レアリティ表記
            const char* rar = RarityName(g.rep->rarity);
            const float rarW = Text::MeasureWidth(rar, TEXT_SIZE_HINT);
            Text::Draw(cx + (POPUP_CARD_W - rarW) * 0.5f, cardY + 14.0f,
                       rar, TEXT_SIZE_HINT, rarityCol);

            // アビリティ名 (+ 取得数)
            char nameBuf[96];
            if (g.count > 1)
                std::snprintf(nameBuf, sizeof(nameBuf), "%s ×%d",
                              g.rep->name.c_str(), g.count);
            else
                std::snprintf(nameBuf, sizeof(nameBuf), "%s",
                              g.rep->name.c_str());
            const float nameW = Text::MeasureWidth(nameBuf, TEXT_SIZE_HUD);
            Text::Draw(cx + (POPUP_CARD_W - nameW) * 0.5f, cardY + 48.0f,
                       nameBuf, TEXT_SIZE_HUD, COLOR_TEXT);

            // 説明文 (改行は description 内の \n で明示済み)
            Text::Draw(cx + 14.0f, cardY + 96.0f,
                       g.rep->description.c_str(), TEXT_SIZE_HINT, COLOR_TEXT_HINT);

            // 任意発動: 残りチャージ / 操作ヒント
            if (activatable)
            {
                const int charges = GroupCharges(g);
                char useBuf[64];
                if (charges >= 0)
                    std::snprintf(useBuf, sizeof(useBuf), "残り %d 回", charges);
                else
                    std::snprintf(useBuf, sizeof(useBuf), "クリックで発動");
                const DirectX::XMFLOAT4 useCol = usable ? COLOR_SLIDE_ARROW
                                                        : COLOR_TEXT_HINT;
                const float useW = Text::MeasureWidth(useBuf, TEXT_SIZE_HINT);
                Text::Draw(cx + (POPUP_CARD_W - useW) * 0.5f,
                           cardY + POPUP_CARD_H - 34.0f,
                           useBuf, TEXT_SIZE_HINT, useCol);
            }
        }

        /*--- 操作ヒント ---*/
        const char* hint = (PopupMaxScroll(total, screenW) > 0.0f)
            ? "ホイールで左右スクロール / 消費アビリティはクリックで発動"
            : "消費アビリティはクリックで発動";
        const float hintW = Text::MeasureWidth(hint, TEXT_SIZE_HINT);
        Text::Draw((screenW - hintW) * 0.5f, screenH - 36.0f,
                   hint, TEXT_SIZE_HINT, COLOR_TEXT_HINT);
    }

    /**
     * @brief  アビリティ履歴ポップアップの入力を処理する
     * @detail ホイールでスクロール、カードクリックで発動、
     *         「閉じる」ボタン/カード外クリックで閉じる。
     *         ポップアップ表示中もゲーム時間は進行する。
     */
    void HandleAbilityPopup()
    {
        const float screenW = static_cast<float>(Direct3D_GetBackBufferWidth());
        const float screenH = static_cast<float>(Direct3D_GetBackBufferHeight());

        const auto groups = BuildAbilityGroups();
        const int  total  = static_cast<int>(groups.size());

        /*--- ホイールスクロール (範囲内へクランプ) ---*/
        const float maxScroll = PopupMaxScroll(total, screenW);
        g_PopupScroll -= g_WheelDelta * POPUP_SCROLL_SPEED;
        if (g_PopupScroll < 0.0)       g_PopupScroll = 0.0;
        if (g_PopupScroll > maxScroll) g_PopupScroll = maxScroll;

        if (!InputManager_IsMouseLeftTrigger()) return;
        const auto& mouse = InputManager_GetMouseState();

        /*--- 閉じるボタン ---*/
        const UiButton closeBtn = PopupCloseButton(screenW);
        if (ButtonContains(closeBtn, mouse.x, mouse.y))
        {
            g_AbilityPopupOpen = false;
            return;
        }

        /*--- カードクリック ---*/
        const float cardY = (screenH - POPUP_CARD_H) * 0.5f + 10.0f;
        const bool  playerTurn =
            (g_State.currentPlayer == Piece::Player && !IsEffectAnimating());
        for (int i = 0; i < total; ++i)
        {
            const float cx = PopupCardX(i, total, screenW);
            if (mouse.x < cx || mouse.x > cx + POPUP_CARD_W) continue;
            if (mouse.y < cardY || mouse.y > cardY + POPUP_CARD_H) continue;

            // カード命中: 任意発動かつ発動可能なら発動 (パッシブは何もしない)
            const AbilityGroup& g = groups[i];
            if (g.rep->activatable && playerTurn && GroupCanActivate(g))
            {
                GroupActivate(g);
            }
            return;   // カード上のクリックは消費 (ポップアップは閉じない)
        }

        /*--- カード外クリック: ポップアップを閉じる ---*/
        g_AbilityPopupOpen = false;
    }
}

//======================================
// 初期化／終了
//======================================
void Game_Initialize()
{
    Prim::Initialize();
    Text::Initialize();
    ResetMatch();
}

void Game_Finalize()
{
}

//======================================
// 更新
//======================================
void Game_Update(double elapsed_time)
{
    g_AnimClock += elapsed_time;  // シェブロン脈動用クロック
    if (g_TimerFlash > 0.0) g_TimerFlash -= elapsed_time;

    // マウスホイールの変化量を算出 (履歴ポップアップのスクロールに使用)
    {
        const int wheel = InputManager_GetMouseState().scrollWheelValue;
        g_WheelDelta = wheel - g_WheelPrev;
        g_WheelPrev  = wheel;
    }

    // R はリスタートのショートカット (ゲームオーバー画面のボタンでも可)
    // ※ ESC はウィンドウ側で終了確認に割当て済みのため使用しない
    if (Keyboard_IsKeyDown(KK_R))
    {
        RunState::ResetRun();   // 取得アビリティ・ボス進行を破棄しランを初期化
        ResetMatch();
        return;
    }

    /*--- 効果アニメ再生中: 入力/AI/タイマー/終了処理を止めフェーズを進める ---*/
    if (IsEffectAnimating())
    {
        g_AnimPhaseTimer -= elapsed_time;
        if (g_AnimPhaseTimer <= 0.0)
        {
            ++g_AnimPhaseIndex;  // 次フェーズへ
            if (IsEffectAnimating()) g_AnimPhaseTimer = PHASE_DURATION;
        }
        if (g_LastPlacedTimer > 0.0)
        {
            g_LastPlacedTimer -= elapsed_time;
            if (g_LastPlacedTimer < 0.0) g_LastPlacedTimer = 0.0;
        }
        return;
    }

    /*--- プレイ中: タイマー消費 → 入力/アビリティ → AI ---*/
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
            if (g_AbilityPopupOpen)
            {
                // 履歴ポップアップ表示中: 盤面入力は止めるが時間は進行する
                HandleAbilityPopup();
            }
            else
            {
                // アビリティUIのクリックを優先。消費しなければ駒設置へ。
                if (!HandleAbilityClick()) HandlePlayerInput();
            }
            HandleAiTurn(elapsed_time);
        }
        return;
    }

    /*--- ゲーム終了後: マウスでボタン操作 ---*/
    g_GameOverDwell += elapsed_time;
    if (g_GameOverDwell < GAMEOVER_MIN_DWELL) return;
    if (!InputManager_IsMouseLeftTrigger()) return;

    const auto& mouse = InputManager_GetMouseState();
    for (const auto& go : BuildGameOverButtons())
    {
        if (!ButtonContains(go.btn, mouse.x, mouse.y)) continue;
        switch (go.action)
        {
        case GameOverAction::Reward:
            // 勝利: ボス番号を進める
            RunState::IncrementBoss();
            if (RunState::IsRunComplete())
            {
                // 全ボス撃破=ランクリア。報酬を挟まず制覇表示→タイトルへ。
                // (ここで GenerateRewardChoices を呼ばないことで、範囲外ボスへの
                //  アクセスと空報酬の再生成を構造的に回避する)
                RunState::MarkRunCleared();
                Scene_Change(Scene::TITLE);
            }
            else
            {
                // 次のボスへ向けて報酬画面へ
                RunState::GenerateRewardChoices();
                Scene_Change(Scene::REWARD);
            }
            break;
        case GameOverAction::Restart:
            // ランを最初からやり直す (取得アビリティ・ボス進行をリセット)
            RunState::ResetRun();
            ResetMatch();
            break;
        case GameOverAction::Title:
            // タイトルへ戻る
            Scene_Change(Scene::TITLE);
            break;
        }
        return;
    }
}

//======================================
// 描画
//======================================
void Game_Draw()
{
    // ボスごとのテーマ色で全画面背景を塗る (盤面の周囲に雰囲気を出す)
    if (g_Boss)
    {
        const float screenW = static_cast<float>(Direct3D_GetBackBufferWidth());
        const float screenH = static_cast<float>(Direct3D_GetBackBufferHeight());
        Prim::DrawRect(0, 0, screenW, screenH,
                       { g_Boss->bgR, g_Boss->bgG, g_Boss->bgB, 1.0f });
    }

    DrawBoardAndPieces();     // 盤面と駒
    DrawDirectionIndicator(); // 氷盤の滑り方向矢印
    DrawRotationIndicator();  // 螺旋盤の回転方向シェブロン
    DrawHud();                // HUD (残時間/手番/アビリティ)
    DrawAbilityPopup();       // アビリティ履歴ポップアップ
    DrawGameOverOverlay();    // ゲームオーバー時のオーバーレイ
}
