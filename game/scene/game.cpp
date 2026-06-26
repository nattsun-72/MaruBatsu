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
#include "win_check.h"
#include "render_primitives.h"
#include "text_draw.h"

#include "ability/ability_registry.h"
#include "ability/abilities/boss_engraved.h"
#include "ability/abilities/boss_gravity.h"
#include "ability/abilities/boss_ice_slide.h"
#include "ability/abilities/boss_spiral_rotate.h"
#include "boss/boss.h"
#include "boss/boss_roster.h"

#include "run_state.h"
#include "config.h"
#include "view3d.h"

#include "scene.h"
#include "direct3d.h"
#include "sprite.h"
#include "input_manager.h"
#include "keyboard.h"
#include "sound_manager.h"

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
    // AI遅延・効果アニメ (設定ファイル駆動。ResetMatch で読み込む)
    //--------------------------------------
    double g_AiDelay       = 0.35;   // AIが着手するまでの間(秒)
    double g_PhaseDuration = 0.18;   // 1効果フェーズの再生時間(秒)

    //--------------------------------------
    // 方向インジケータ (シェブロン矢印)
    //   ※ 大きさ・太さ・マージンは「マス単位」で各描画関数が持ち、
    //     2D/3Dの遠近スケール (PxPerCell) を掛けて画面サイズへ変換する
    //--------------------------------------
    constexpr int   CHEVRON_COUNT     = 3;       // 滑る先の辺に沿って並べる数

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
    constexpr float ABILITY_LIST_X = 24.0f;   // 一覧の左端X (消費系の列)
    constexpr float ABILITY_ROW_H  = 36.0f;   // 1行の高さ
    constexpr float ABILITY_BTN_W  = 220.0f;  // 1列の幅 / 任意発動ボタンの幅
    constexpr float ABILITY_BTN_H  = 30.0f;   // 任意発動ボタンの高さ
    constexpr float ABILITY_COL_GAP = 20.0f;  // 消費列とパッシブ列の間隔

    //--------------------------------------
    // アビリティ履歴ポップアップ
    //--------------------------------------
    constexpr int   ABILITY_VISIBLE_MAX = 10;      // 1列に表示する最大グループ数(超過は「…」)
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
    const DirectX::XMFLOAT4 COLOR_CHAIN_PULSE { 1.00f, 0.55f, 0.30f, 1.0f };  // 連鎖パルス線(橙赤)
    const DirectX::XMFLOAT4 COLOR_GRAVITY_ARROW{ 0.80f, 0.85f, 0.95f, 1.0f }; // 重力矢印(鋼)
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
    std::shared_ptr<BossGravityAbility>         g_GravityRef;    // 重力インジケータ用 (重力場参照)
    std::shared_ptr<BossEngravedAbility>        g_EngravedRef;   // モード表示用 (刻まれし力参照)

    //--------------------------------------
    // 不死(敗北打ち消し)の演出
    //--------------------------------------
    double g_ImmortalFlash = 0.0;              // 「不死発動」バナーの残り表示時間
    constexpr double IMMORTAL_FLASH_DURATION = 1.4;   // バナー表示時間(秒)

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
    double g_TimerFlash = 0.0;  // アビリティ発動フラッシュ演出の残り時間
    std::string g_FlashText;    // 発動フラッシュに表示する文言 (アビリティ毎)

    //--------------------------------------
    // 効果音トリガ用の状態
    //--------------------------------------
    constexpr double TIMER_LOW_SE_THRESHOLD = 10.0;  // 残時間が これ以下 で警告音(1度)
    MatchResult g_PrevResult     = MatchResult::Playing;  // 前フレームの勝敗(遷移検出用)
    bool        g_TimerLowSePlayed = false;               // 当該ターンで警告音を鳴らしたか

    //--------------------------------------
    // 決着演出 (勝利ラインのグロー + 確定フラッシュ)
    //--------------------------------------
    std::vector<Vec2> g_WinLine;                       // 勝利ラインのマス列 (空=ライン勝利でない)
    Piece             g_WinLineSide = Piece::Empty;    // 勝者(ライン/フラッシュの色分け用)
    double            g_WinFlash    = 0.0;             // 決着フラッシュの残り時間(秒)
    constexpr double  WIN_FLASH_DURATION = 0.45;       // フラッシュ演出の長さ(秒)

    //--------------------------------------
    // 画面シェイク / ヒットストップ (手応えの演出)
    //--------------------------------------
    double g_ShakeTime = 0.0;   // 残りシェイク時間(秒)
    double g_ShakeDur  = 0.0;   // シェイクの総時間(振幅減衰の基準)
    double g_ShakeMag  = 0.0;   // シェイクの初期振幅(px)
    double g_HitStop   = 0.0;   // 残りヒットストップ時間(秒。>0 の間ゲーム進行を凍結)

    /**
     * @brief  画面シェイクを開始する (強い方を採用 = 進行中のより弱い揺れは上書きしない)
     * @param  mag 初期振幅(px)  @param dur 持続時間(秒)
     * @detail 設置直後に弱い効果シェイクが続いても、設置の手応えを減衰させないため、
     *         現在の残り振幅より弱い要求は無視する。
     */
    void TriggerShake(double mag, double dur)
    {
        const double curAmp = (g_ShakeTime > 0.0 && g_ShakeDur > 0.0)
                            ? g_ShakeMag * (g_ShakeTime / g_ShakeDur) : 0.0;
        if (mag < curAmp) return;   // 進行中のより強い揺れを優先

        g_ShakeMag  = mag;
        g_ShakeDur  = dur;
        g_ShakeTime = dur;
    }

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
    //======================================
    // 3D/2D 共通の盤面座標ヘルパ
    //======================================
    /**
     * @brief  盤面セル座標(マス単位)をスクリーン座標へ変換する
     * @param  u 列方向の位置 (盤外も可)
     * @param  v 行方向の位置 (盤外も可)
     * @param  h 盤面からの高さ(マス単位)。2Dビューでは無視される。
     * @return スクリーン座標
     * @detail 3Dビュー有効時は透視投影、無効時は従来の平面配置。
     *         盤面・駒・インジケータの全描画がこの1点を通ることで、
     *         視点切替を局所化している。
     */
    View3D::P2 BoardPoint(float u, float v, float h = 0.0f)
    {
        if (View3D::Enabled()) return View3D::Project(u, v, h);
        return { BoardOriginX() + u * CellSize(),
                 BoardOriginY() + v * CellSize() };
    }

    /**
     * @brief  位置 v における1マスあたりのピクセル数 (遠近スケール)
     * @param  v 行方向の位置
     * @return ピクセル/マス。2Dビューでは一定。
     */
    float PxPerCell(float v)
    {
        return View3D::Enabled() ? View3D::Scale(v) : CellSize();
    }

    /**
     * @brief  点が凸四角形の内部にあるか判定する (3Dピッキング用)
     * @param  px,py 判定する点
     * @param  a,b,c,d 四角形の頂点 (時計回り or 反時計回りの順)
     * @return 内部(辺上含む)なら true
     */
    bool PointInQuad(float px, float py,
                     const View3D::P2& a, const View3D::P2& b,
                     const View3D::P2& c, const View3D::P2& d)
    {
        auto cross = [](const View3D::P2& o, const View3D::P2& q,
                        float rx, float ry)
        {
            return (q.x - o.x) * (ry - o.y) - (q.y - o.y) * (rx - o.x);
        };
        const float c1 = cross(a, b, px, py);
        const float c2 = cross(b, c, px, py);
        const float c3 = cross(c, d, px, py);
        const float c4 = cross(d, a, px, py);
        // 全ての外積の符号が揃っていれば内部 (凸前提)
        const bool anyNeg = (c1 < 0.0f || c2 < 0.0f || c3 < 0.0f || c4 < 0.0f);
        const bool anyPos = (c1 > 0.0f || c2 > 0.0f || c3 > 0.0f || c4 > 0.0f);
        return !(anyNeg && anyPos);
    }

    Vec2 ScreenToCell(int sx, int sy)
    {
        if (View3D::Enabled())
        {
            /*--- 3D: 各セルの投影四角形に対する内外判定で命中セルを探す ---*/
            const float fx = static_cast<float>(sx);
            const float fy = static_cast<float>(sy);
            for (int y = 0; y < g_State.board.height; ++y)
            {
                for (int x = 0; x < g_State.board.width; ++x)
                {
                    const auto a = View3D::Project(static_cast<float>(x),     static_cast<float>(y));
                    const auto b = View3D::Project(static_cast<float>(x + 1), static_cast<float>(y));
                    const auto c = View3D::Project(static_cast<float>(x + 1), static_cast<float>(y + 1));
                    const auto d = View3D::Project(static_cast<float>(x),     static_cast<float>(y + 1));
                    if (PointInQuad(fx, fy, a, b, c, d)) return { x, y };
                }
            }
            return { -1, -1 };
        }

        /*--- 2D: 従来の平面配置の逆変換 ---*/
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
    //   画面左下に「消費系」「パッシブ」の2列を並べる。各列はヘッダ + count行を
    //   下端揃えで上へ積む。Draw/クリック判定で共通使用。
    //======================================
    /**
     * @brief  列 col の左端X座標 (0=消費列, 1=パッシブ列)
     */
    float AbilityColX(int col)
    {
        return ABILITY_LIST_X + col * (ABILITY_BTN_W + ABILITY_COL_GAP);
    }

    /**
     * @brief  アビリティ一覧 (ヘッダ含む) の上端Y座標
     * @param  count 一覧の行数
     * @return ヘッダ行の上端Y座標 (行数によらず下端は画面下から一定)
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
        // 使い切った消費系(残チャージ合計0)は表示から除外する。
        // パッシブ(回数無制限=ChargesLeft<0)は常に残す。
        groups.erase(std::remove_if(groups.begin(), groups.end(),
            [](const AbilityGroup& g)
            {
                if (!g.rep->activatable) return false;   // パッシブは残す
                int total = 0;
                for (Ability* a : g.instances)
                {
                    const int c = a->ChargesLeft();
                    if (c < 0) return false;             // 無制限が混在 → 残す
                    total += c;
                }
                return total == 0;                       // 全消費済みなら除外
            }), groups.end());

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

    // 前方宣言 (任意発動が盤面を書き換えた場合の後処理で使用)
    void StartEffectAnimation(const Board& boardStart);
    bool EvaluateBoardOutcome();

    /**
     * @brief  グループ内の発動可能なインスタンスを1つ発動する
     * @param  g 対象グループ
     * @detail 取得順で最初に発動できたものを使用し、発動フラッシュを出す。
     *         盤面を書き換える発動(螺旋の欠片等)は、効果アニメの再生と
     *         勝敗の再評価まで行う。
     */
    void GroupActivate(const AbilityGroup& g)
    {
        for (Ability* a : g.instances)
        {
            if (a->CanActivate(g_State))
            {
                // 発動前の盤面を控え、効果フェーズの出力をクリアする
                g_State.animPhases.clear();
                const Board boardBefore = g_State.board;

                a->Activate(g_State);
                SoundManager_PlaySE(SOUND_SE_ABILITY); // 任意発動アビリティの効果音
                g_FlashText  = a->flashText;           // 発動フィードバック文言
                g_TimerFlash = TIMER_FLASH_DURATION;

                // 盤面を書き換えた発動なら、アニメ再生と勝敗再評価を行う
                if (!g_State.animPhases.empty())
                {
                    StartEffectAnimation(boardBefore);
                    EvaluateBoardOutcome();
                }
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
        RunState::AddRunTurn();   // ラン統計: 総ターン数を加算
        g_State.currentPlayer       = p;
        g_State.placementsRemaining = g_Registry.GetPlacementCount(p);
        g_State.remainingTime       = g_Registry.GetTurnTime(p);
        g_TimerLowSePlayed          = false;   // 新ターン: 残時間警告音をリセット
        g_Registry.OnTurnStart(g_State, p);

        if (p == Piece::Enemy)
        {
            g_AiPending = true;
            g_AiTimer   = g_AiDelay;
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
        g_GravityRef.reset();
        g_EngravedRef.reset();

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
            if (auto grav = std::dynamic_pointer_cast<BossGravityAbility>(a))
            {
                g_GravityRef = grav;
            }
            if (auto eng = std::dynamic_pointer_cast<BossEngravedAbility>(a))
            {
                g_EngravedRef = eng;
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
        /*--- 設定値の読み込み (タイトルで再読込された値を反映) ---*/
        g_AiDelay       = Config::GetDouble("rules.aiDelaySeconds",     0.35);
        g_PhaseDuration = Config::GetDouble("rules.effectPhaseSeconds", 0.18);

        g_State = {};
        g_State.board.Reset();
        g_State.result = MatchResult::Playing;
        g_State.winner = Piece::Empty;
        g_State.turnCount = 1;
        g_PrevResult   = MatchResult::Playing;   // 勝敗SEの遷移検出をリセット

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
        g_ImmortalFlash    = 0.0;
        g_WinLine.clear();
        g_WinLineSide      = Piece::Empty;
        g_WinFlash         = 0.0;
        g_ShakeTime        = 0.0;
        g_HitStop          = 0.0;
        Sprite_SetViewOffset(0.0f, 0.0f);
        View3D::ClearImpact();

        /*--- ボス再ロード → 盤面サイズ確定 → 先手ターン開始 ---*/
        LoadBoss();

        // 盤面サイズフックを適用 (盤面拡張アビリティ等を反映)
        int boardW = 3, boardH = 3;
        g_Registry.GetBoardSize(boardW, boardH);
        g_State.board.Resize(boardW, boardH);

        // この戦況(ボス番号+所持アビリティ+統計)を BeginTurn の前にセーブする。
        // BeginTurn 後に保存すると、その戦の初手ターン加算がセーブ値に含まれ、
        // 「つづきから」再開時に ResetMatch→BeginTurn で初手が二重計上されるため。
        RunState::SaveRun();

        BeginTurn(Piece::Player);
    }

    //======================================
    // 着手後の判定
    //======================================
    /**
     * @brief  現在の盤面に対して勝敗・引き分けを評価する
     * @return 決着 or 敗北回避が起きたら true (ターン進行を打ち切るべき)
     * @detail 着手後と任意発動の盤面変化後の双方から呼ばれる共通判定。
     *         両陣営の勝利条件を確認し、双方同時成立時はプレイヤー優先。
     *         敗北時は IDefeatHandler (不死等) に打ち消しの機会を与え、
     *         打ち消された場合は盤面を再生して試合を続行する。
     */
    bool EvaluateBoardOutcome()
    {
        /*--- 勝利判定 (ボスギミックで動いた後の最終盤面・両陣営) ---*/
        const bool playerWon = g_Registry.CheckWin(g_State.board, Piece::Player);
        const bool enemyWon  = g_Registry.CheckWin(g_State.board, Piece::Enemy);
        if (playerWon || enemyWon)
        {
            // 敗北しそうな場合、敗北フック(不死等)に打ち消しの機会を与える
            if (enemyWon && !playerWon && g_Registry.HandleDefeat(g_State, Piece::Player))
            {
                // 敗北回避: 盤面を再生し、進行中のアニメを破棄してバナー表示
                g_State.board.Reset();
                g_State.animPhases.clear();
                g_AnimPhases.clear();
                g_PhaseStartBoards.clear();
                g_AnimPhaseIndex  = 0;
                g_AnimPhaseTimer  = 0.0;
                g_LastPlacedPos   = { -1, -1 };
                g_LastPlacedTimer = 0.0;
                g_ImmortalFlash   = IMMORTAL_FLASH_DURATION;
                return true;   // 決着はしないが、通常のターン進行は打ち切る
            }

            // 双方同時成立時はプレイヤーを優先 (理不尽な敗北を避ける)
            const Piece winner = playerWon ? Piece::Player : Piece::Enemy;
            g_State.winner = winner;
            g_State.result = (winner == Piece::Player)
                ? MatchResult::Win : MatchResult::Lose;
            return true;
        }

        /*--- 引き分け判定 ---*/
        if (g_State.board.IsFull())
        {
            g_State.result = MatchResult::Draw;
            return true;
        }
        return false;
    }

    /**
     * @brief  駒を1個置いた直後の勝敗・ターン進行を評価する
     * @param  justPlaced 直前に駒を置いた陣営
     * @detail 勝敗(EvaluateBoardOutcome)→追加着手→ターン終了の順で判定する。
     *         OnPlace は PlaceAndEvaluate 側で先に呼ぶため、ここでは扱わない。
     */
    void EvaluateAfterPlace(Piece justPlaced)
    {
        // 1) 勝敗・引き分け・敗北回避の共通評価
        if (EvaluateBoardOutcome())
        {
            // 敗北回避(盤面再生)で試合続行の場合は、相手の手番へ移す
            if (g_State.result == MatchResult::Playing)
            {
                g_Registry.OnTurnEnd(g_State, justPlaced);
                ++g_State.turnCount;
                BeginTurn(Opponent(justPlaced));
            }
            return;
        }

        // 2) 同一プレイヤーの追加着手が残っているか
        if (g_State.placementsRemaining > 0) return;

        // 3) ターン終了
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
        // 移動は source を空にする。出現(isSpawn)は発生源を残す。
        for (const auto& m : phase)
        {
            if (!m.isSpawn) board.Set(m.fromX, m.fromY, Piece::Empty);
        }
        for (const auto& m : phase) board.Set(m.toX, m.toY, m.piece);
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

        // 盤面が動く効果(滑り/回転/連鎖等)には軽い揺れで手応えを添える。
        // 氷盤ボス等は毎手発火するため、疲れない程度の弱い揺れに留める。
        TriggerShake(3.5, 0.12);

        // 各フェーズ開始時の盤面を前計算 (描画時の中間状態復元に使う)
        g_PhaseStartBoards.reserve(g_AnimPhases.size());
        Board cur = boardStart;
        for (const auto& phase : g_AnimPhases)
        {
            g_PhaseStartBoards.push_back(cur);
            ApplyPhase(cur, phase);
        }
        g_AnimPhaseTimer = g_PhaseDuration;
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

        SoundManager_PlaySE(SOUND_SE_PLACE);   // 駒の設置音

        // 設置の手応え (自駒・敵駒どちらも): 3Dは着手マスを起点に盤面を
        // 傾けて振動させる。2D視点では高さ変位が効かないため軽い全画面シェイクで代替。
        View3D::SetImpact(pos.x + 0.5f, pos.y + 0.5f);
        if (!View3D::Enabled()) TriggerShake(5.0, 0.12);

        // 設置時演出 (フック発火前に位置を記録)
        g_LastPlacedPos   = pos;
        g_LastPlacedPiece = placedBy;
        g_LastPlacedTimer = GLOW_DURATION;

        // 効果フックの出力をクリアし、OnPlace前 (新駒設置済み) の盤面を控える
        g_State.animPhases.clear();
        const Board boardStart = g_State.board;

        // 設置時フック (氷駒/氷盤/連鎖/爆弾等が animPhases に効果を積む)
        g_Registry.OnPlace(g_State, pos, placedBy);

        // ギミックが盤面を動かした場合のみ、発動音を重ねる
        if (!g_State.animPhases.empty())
        {
            SoundManager_PlaySE(SOUND_SE_ABILITY);
        }

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
     * @brief  アビリティ1列ぶんのクリックを処理する
     * @param  col       列番号 (0=消費, 1=パッシブ)
     * @param  list      その列のグループ一覧
     * @param  clickable 消費系列(任意発動ボタン)なら true
     * @return クリックを消費したら true
     * @detail 「…」行で履歴ポップアップを開き、消費系ボタンの命中で発動する。
     */
    bool HandleAbilityColumnClick(int col, const std::vector<AbilityGroup>& list,
                                  bool clickable)
    {
        if (list.empty()) return false;

        const int  n        = static_cast<int>(list.size());
        const int  visible  = (n < ABILITY_VISIBLE_MAX) ? n : ABILITY_VISIBLE_MAX;
        const bool showMore = n > ABILITY_VISIBLE_MAX;
        const int  rowCount = visible + (showMore ? 1 : 0);
        const float x       = AbilityColX(col);
        const auto& mouse   = InputManager_GetMouseState();

        // 「…」行: 履歴ポップアップを開く (手番を問わず操作可)
        if (showMore)
        {
            const float ry = AbilityRowY(visible, rowCount);
            if (mouse.x >= x && mouse.x <= x + ABILITY_BTN_W
             && mouse.y >= ry && mouse.y <= ry + ABILITY_BTN_H)
            {
                g_AbilityPopupOpen = true;
                g_PopupScroll      = 0.0;
                return true;
            }
        }

        // 任意発動ボタン: 消費系列 かつ プレイヤー手番のみ
        if (clickable && g_State.currentPlayer == Piece::Player)
        {
            for (int i = 0; i < visible; ++i)
            {
                const float ry = AbilityRowY(i, rowCount);
                if (mouse.x < x || mouse.x > x + ABILITY_BTN_W) continue;
                if (mouse.y < ry || mouse.y > ry + ABILITY_BTN_H) continue;
                // ボタン命中 — 発動可能なら発動 (不可でもクリックは消費する)
                if (GroupCanActivate(list[i])) GroupActivate(list[i]);
                return true;
            }
        }
        return false;
    }

    /**
     * @brief  所持アビリティ一覧(HUD)のクリックを処理する
     * @return クリックを消費したら true (= 駒設置へ回さない)
     * @detail 消費系(左列)・パッシブ(右列)に分け、各列を順に判定する。
     */
    bool HandleAbilityClick()
    {
        if (!InputManager_IsMouseLeftTrigger()) return false;

        const auto groups = BuildAbilityGroups();
        if (groups.empty()) return false;

        // 消費系(col0) と パッシブ(col1) に振り分ける
        std::vector<AbilityGroup> cons, pass;
        for (const auto& g : groups)
            (g.rep->activatable ? cons : pass).push_back(g);

        if (HandleAbilityColumnClick(0, cons, true))  return true;
        if (HandleAbilityColumnClick(1, pass, false)) return true;
        return false;
    }

    /**
     * @brief  AIの手番を処理する
     * @param  dt 前フレームからの経過秒数
     * @detail g_AiDelay 経過後に着手。追加着手が残れば再度AIを予約する。
     */
    void HandleAiTurn(double dt)
    {
        if (g_State.currentPlayer != Piece::Enemy || !g_AiPending) return;
        g_AiTimer -= dt;
        if (g_AiTimer > 0.0) return;

        // 設置+ボスギミック適用後の盤面を返す遷移関数 (AIがギミックを読むために使う)。
        // 現ターンの slideAnchorSide(重駒の固定対象)を引き継いで実戦と同条件で評価する。
        // 既知の制約: この sim は完全な純関数ではない。OnPlace は「生きた」
        // アビリティ実体を走らせるため、探索中に各ギミックの共有静的RNGが
        // 進む(=後で実戦で見えるギミック抽選が探索で進んだRNG位置から出る)。
        // 乱択ギミックの結果は元々一様乱数なのでプレイヤーから見て不正には
        // ならず、現状シード固定ラン等も無いため実害は無い。将来シード固定
        // ランを入れる際は、探索用にアビリティ状態/RNGを複製して分離すること。
        AI::PlacementSim sim = [](const Board& b, Vec2 pos, Piece who) -> Board
        {
            GameState tmp;
            tmp.board           = b;
            tmp.currentPlayer   = who;
            tmp.slideAnchorSide = g_State.slideAnchorSide;
            tmp.board.Set(pos.x, pos.y, who);
            g_Registry.OnPlace(tmp, pos, who);   // ギミックを盤面へ適用 (animPhasesは無視)
            return tmp.board;
        };
        // 各陣営の着手数 (二手打ち/焦燥などで2になり得る)。
        AI::PlacementCountFn counts = [](Piece p) -> int
        {
            return g_Registry.GetPlacementCount(p);
        };

        const Vec2 v = AI::ChooseMove(
            g_State.board, Piece::Enemy, g_Registry.GetWinCondition(), sim, counts);
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
            g_AiTimer   = g_AiDelay;
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

        // 残時間が しきい値 を下回った最初の1回だけ警告音を鳴らす
        if (!g_TimerLowSePlayed
         && g_State.remainingTime <= TIMER_LOW_SE_THRESHOLD
         && g_State.remainingTime > 0.0)
        {
            SoundManager_PlaySE(SOUND_SE_TIMER_LOW);
            g_TimerLowSePlayed = true;
        }

        if (g_State.remainingTime > 0.0) return;

        g_State.remainingTime = 0.0;
        // 敗北フック (不死等で打ち消し可能)
        if (g_Registry.HandleDefeat(g_State, Piece::Player))
        {
            // 敗北回避: 残時間を新規取得。回復後の再カウントダウンでも警告音が
            // 鳴るよう、残時間わずか フラグもここでリセットする。
            g_State.remainingTime = g_Registry.GetTurnTime(Piece::Player);
            g_TimerLowSePlayed    = false;
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

    //======================================
    // 3Dビュー描画 (斜め俯瞰の盤面と立体駒)
    //======================================
    //--------------------------------------
    // 3D駒の造形パラメータ (マス単位)
    //--------------------------------------
    constexpr float P3_RING_R      = 0.30f;   // 〇リングの半径
    constexpr float P3_RING_H_LOW  = 0.06f;   // 〇リング下段の高さ
    constexpr float P3_RING_H_TOP  = 0.20f;   // 〇リング上段の高さ
    constexpr float P3_CROSS_LEN   = 0.33f;   // ×バーの半長
    constexpr float P3_CROSS_WID   = 0.085f;  // ×バーの半幅
    constexpr float P3_CROSS_H     = 0.18f;   // ×バー上面の高さ
    constexpr float P3_DROP_H      = 0.90f;   // 設置ドロップ演出の開始高さ

    /**
     * @brief  スクリーン座標の凸四角形を塗りつぶす (走査線方式)
     * @param  a,b,c,d 頂点 (a-b が上辺、d-c が下辺になる向きで渡す)
     * @param  color   塗り色
     * @detail 左辺(a→d)と右辺(b→c)を補間した帯を重ねて面を表現する。
     *         3Dビューにはポリゴン描画が無いため、太い線で代用する。
     */
    void FillQuad3D(const View3D::P2& a, const View3D::P2& b,
                    const View3D::P2& c, const View3D::P2& d,
                    const DirectX::XMFLOAT4& color)
    {
        constexpr int BANDS = 12;
        // 左辺の全長から1帯の太さを求める (1.6倍重ねて隙間を消す)
        const float edgeLen = std::sqrt((d.x - a.x) * (d.x - a.x)
                                      + (d.y - a.y) * (d.y - a.y));
        const float band = edgeLen / BANDS * 1.6f;
        for (int i = 0; i < BANDS; ++i)
        {
            const float t = (i + 0.5f) / BANDS;
            const float lx = a.x + (d.x - a.x) * t;
            const float ly = a.y + (d.y - a.y) * t;
            const float rx = b.x + (c.x - b.x) * t;
            const float ry = b.y + (c.y - b.y) * t;
            Prim::DrawLine(lx, ly, rx, ry, band, color);
        }
    }

    /**
     * @brief  セル(x,y)の投影四角形を塗りつぶす
     * @param  x,y   セル座標
     * @param  inset 縁からの内側マージン(マス)
     * @param  color 塗り色
     */
    void FillCell3D(int x, int y, float inset, const DirectX::XMFLOAT4& color)
    {
        const float u0 = x + inset,     v0 = y + inset;
        const float u1 = x + 1 - inset, v1 = y + 1 - inset;
        FillQuad3D(View3D::Project(u0, v0), View3D::Project(u1, v0),
                   View3D::Project(u1, v1), View3D::Project(u0, v1), color);
    }

    /**
     * @brief  盤面上の円環を3D投影で描画する
     * @param  u,v       中心のセル座標
     * @param  radius    半径(マス)
     * @param  h         高さ(マス)
     * @param  thickness 線の太さ(マス)
     * @param  color     基本色 (奥側は自動で暗くなる)
     * @param  segments  円周の分割数
     */
    void Ring3D(float u, float v, float radius, float h,
                float thickness, const DirectX::XMFLOAT4& color,
                int segments = 28)
    {
        constexpr float PI_F = 3.14159265358979323846f;
        View3D::P2 prev = View3D::Project(u + radius, v, h);
        for (int i = 1; i <= segments; ++i)
        {
            const float a  = static_cast<float>(i) / segments * 2.0f * PI_F;
            const float vv = v + std::sin(a) * radius;
            const View3D::P2 cur = View3D::Project(u + std::cos(a) * radius, vv, h);

            // 奥の弧ほど暗くして立体感を出す (vv が小さい=奥)
            const float rel   = (std::sin(a) + 1.0f) * 0.5f;   // 0=奥, 1=手前
            const float shade = 0.68f + 0.32f * rel;
            const DirectX::XMFLOAT4 col{ color.x * shade, color.y * shade,
                                         color.z * shade, color.w };
            Prim::DrawLine(prev.x, prev.y, cur.x, cur.y,
                           thickness * View3D::Scale(vv), col);
            prev = cur;
        }
    }

    /**
     * @brief  ×駒の1本のバーを押し出し立体として描画する
     * @param  u,v    中心のセル座標
     * @param  dirX,Y バーの軸方向 (正規化済み)
     * @param  scale  造形スケール (出現アニメ用)
     * @param  hExtra 追加の高さ (ドロップ演出用)
     */
    void CrossBar3D(float u, float v, float dirX, float dirY,
                    float scale, float hExtra)
    {
        const float L = P3_CROSS_LEN * scale;
        const float W = P3_CROSS_WID * scale;
        const float hTop = P3_CROSS_H * scale + hExtra;

        // バー両端の中心線座標
        const float e0u = u - dirX * L, e0v = v - dirY * L;
        const float e1u = u + dirX * L, e1v = v + dirY * L;

        // 接地影 (h=0 の暗い太線)
        {
            const View3D::P2 s0 = View3D::Project(e0u, e0v, 0.0f);
            const View3D::P2 s1 = View3D::Project(e1u, e1v, 0.0f);
            Prim::DrawLine(s0.x, s0.y, s1.x, s1.y,
                           W * 2.1f * View3D::Scale(v),
                           { 0.0f, 0.0f, 0.0f, 0.30f });
        }

        // 両端の支柱 (接地から上面へ。暗色で立体の厚みを示す)
        DirectX::XMFLOAT4 sideCol{ COLOR_PIECE_X.x * 0.45f, COLOR_PIECE_X.y * 0.45f,
                                   COLOR_PIECE_X.z * 0.45f, COLOR_PIECE_X.w };
        {
            const View3D::P2 b0 = View3D::Project(e0u, e0v, 0.0f);
            const View3D::P2 t0 = View3D::Project(e0u, e0v, hTop);
            const View3D::P2 b1 = View3D::Project(e1u, e1v, 0.0f);
            const View3D::P2 t1 = View3D::Project(e1u, e1v, hTop);
            Prim::DrawLine(b0.x, b0.y, t0.x, t0.y, W * 1.8f * View3D::Scale(e0v), sideCol);
            Prim::DrawLine(b1.x, b1.y, t1.x, t1.y, W * 1.8f * View3D::Scale(e1v), sideCol);
        }

        // 上面 (明色の太線で面を表現)
        {
            const View3D::P2 t0 = View3D::Project(e0u, e0v, hTop);
            const View3D::P2 t1 = View3D::Project(e1u, e1v, hTop);
            Prim::DrawLine(t0.x, t0.y, t1.x, t1.y,
                           W * 2.0f * View3D::Scale(v), COLOR_PIECE_X);
        }
    }

    /**
     * @brief  1マス分の駒を3Dの立体造形で描画する
     * @param  p      駒の陣営 (Empty なら何も描かない)
     * @param  u,v    セル中心のセル座標
     * @param  scale  造形スケール (1=通常。出現アニメで0→1)
     * @param  hExtra 追加の高さ (ドロップ演出用。通常0)
     * @detail 〇は二段の円環(トーラス風)、×は押し出しクロス。
     *         接地影と奥側の減光で立体感を出す。
     */
    void DrawPiece3D(Piece p, float u, float v, float scale = 1.0f,
                     float hExtra = 0.0f)
    {
        if (p == Piece::Empty || scale <= 0.01f) return;

        if (p == Piece::Player)
        {
            const float R = P3_RING_R * scale;

            // 接地影
            Ring3D(u, v, R * 0.95f, 0.0f, 0.10f * scale,
                   { 0.0f, 0.0f, 0.0f, 0.30f }, 20);

            // 下段リング (暗色) → 上段リング (明色) で円環の厚みを表現
            const DirectX::XMFLOAT4 lowCol{ COLOR_PIECE_O.x * 0.45f,
                                            COLOR_PIECE_O.y * 0.45f,
                                            COLOR_PIECE_O.z * 0.45f, 1.0f };
            Ring3D(u, v, R, P3_RING_H_LOW * scale + hExtra, 0.050f * scale, lowCol);
            Ring3D(u, v, R, P3_RING_H_TOP * scale + hExtra, 0.062f * scale, COLOR_PIECE_O);
        }
        else
        {
            // ×: 2本のバーを45度交差で押し出し
            constexpr float INV_SQRT2 = 0.70710678f;
            CrossBar3D(u, v,  INV_SQRT2, INV_SQRT2, scale, hExtra);
            CrossBar3D(u, v,  INV_SQRT2, -INV_SQRT2, scale, hExtra);
        }
    }

    /**
     * @brief  盤面と駒を3Dビュー(斜め俯瞰)で描画する
     * @detail 盤面パネル→罫線→ホバー→グロー→駒(静止/アニメ補間)の順。
     *         2D版 DrawBoardAndPieces と同じ状態機械を共有し、
     *         描画座標の計算だけを透視投影に置き換えている。
     */
    void DrawBoardAndPieces3D()
    {
        const int cols = g_State.board.width;
        const int rows = g_State.board.height;

        /*--- 盤面パネル (外周マージン付きの台座) ---*/
        {
            const float m = 0.32f;
            DirectX::XMFLOAT4 panel{ 0.02f, 0.02f, 0.04f, 0.85f };
            if (g_Boss)
            {
                // ボステーマ色を僅かに含ませて沈んだ台座色にする
                panel = { g_Boss->bgR * 0.5f, g_Boss->bgG * 0.5f,
                          g_Boss->bgB * 0.5f, 0.92f };
            }
            FillQuad3D(View3D::Project(-m,          -m),
                       View3D::Project(cols + m,    -m),
                       View3D::Project(cols + m, rows + m),
                       View3D::Project(-m,       rows + m), panel);
        }

        /*--- 罫線 (外枠含む) ---*/
        for (int i = 0; i <= cols; ++i)
        {
            const View3D::P2 p0 = View3D::Project(static_cast<float>(i), 0.0f);
            const View3D::P2 p1 = View3D::Project(static_cast<float>(i), static_cast<float>(rows));
            Prim::DrawLine(p0.x, p0.y, p1.x, p1.y,
                           0.022f * View3D::Scale(rows * 0.5f), COLOR_GRID);
        }
        for (int i = 0; i <= rows; ++i)
        {
            const View3D::P2 p0 = View3D::Project(0.0f,                      static_cast<float>(i));
            const View3D::P2 p1 = View3D::Project(static_cast<float>(cols),  static_cast<float>(i));
            Prim::DrawLine(p0.x, p0.y, p1.x, p1.y,
                           0.022f * View3D::Scale(static_cast<float>(i)), COLOR_GRID);
        }

        /*--- ホバー中の空きセルをハイライト (プレイヤー手番のみ) ---*/
        if (g_State.result == MatchResult::Playing
            && g_State.currentPlayer == Piece::Player
            && !IsEffectAnimating() && !g_AbilityPopupOpen)
        {
            const auto& mouse = InputManager_GetMouseState();
            const Vec2 hover = ScreenToCell(mouse.x, mouse.y);
            if (hover.x >= 0 && g_State.board.IsEmpty(hover.x, hover.y))
            {
                DirectX::XMFLOAT4 hi = COLOR_PIECE_O;
                hi.w = 0.10f + 0.04f * static_cast<float>(
                           std::sin(g_AnimClock * 4.0));
                FillCell3D(hover.x, hover.y, 0.06f, hi);
            }
        }

        /*--- 直近設置のグロー (広がって消える円環) ---*/
        if (g_LastPlacedTimer > 0.0 && g_LastPlacedPos.x >= 0)
        {
            const float t = static_cast<float>(g_LastPlacedTimer / GLOW_DURATION); // 1→0
            const float glowR = P3_RING_R * (1.0f + (1.0f - t) * 0.7f);
            DirectX::XMFLOAT4 glow = (g_LastPlacedPiece == Piece::Player)
                ? COLOR_PIECE_O : COLOR_PIECE_X;
            glow.w = t * 0.45f;
            Ring3D(g_LastPlacedPos.x + 0.5f, g_LastPlacedPos.y + 0.5f,
                   glowR, 0.10f, 0.10f, glow, 24);
        }

        if (IsEffectAnimating())
        {
            /*--- 効果アニメ中: 再生中フェーズの中間状態を描画 ---*/
            const int                 phaseIdx = g_AnimPhaseIndex;
            const Board&              disp     = g_PhaseStartBoards[phaseIdx];
            const BoardOps::MoveList& phase    = g_AnimPhases[phaseIdx];

            // 進捗 0→1 (ease-out で滑らかに減速)
            float raw = 1.0f - static_cast<float>(g_AnimPhaseTimer / g_PhaseDuration);
            raw = (raw < 0.0f) ? 0.0f : (raw > 1.0f ? 1.0f : raw);
            const float t = 1.0f - (1.0f - raw) * (1.0f - raw);

            // このフェーズで動かない駒を静止描画 (出現の発生源は静止扱い)
            for (int y = 0; y < disp.height; ++y)
            {
                for (int x = 0; x < disp.width; ++x)
                {
                    bool isMover = false;
                    for (const auto& m : phase)
                    {
                        if (!m.isSpawn && m.fromX == x && m.fromY == y)
                        {
                            isMover = true;
                            break;
                        }
                    }
                    if (isMover) continue;
                    DrawPiece3D(disp.Get(x, y), x + 0.5f, y + 0.5f);
                }
            }
            // 移動駒は補間、出現駒は拡大+連鎖パルス線
            for (const auto& m : phase)
            {
                if (m.isSpawn)
                {
                    if (m.fromX >= 0 && m.fromY >= 0)
                    {
                        const View3D::P2 s = View3D::Project(m.fromX + 0.5f, m.fromY + 0.5f, 0.12f);
                        const View3D::P2 e = View3D::Project(m.toX + 0.5f,   m.toY + 0.5f,   0.12f);
                        DirectX::XMFLOAT4 pulse = COLOR_CHAIN_PULSE;
                        pulse.w = (1.0f - t) * 0.7f;
                        Prim::DrawLine(s.x, s.y, e.x, e.y,
                                       0.030f * View3D::Scale(m.toY + 0.5f), pulse);
                    }
                    DrawPiece3D(m.piece, m.toX + 0.5f, m.toY + 0.5f, t);
                }
                else
                {
                    const float fu = m.fromX + (m.toX - m.fromX) * t + 0.5f;
                    const float fv = m.fromY + (m.toY - m.fromY) * t + 0.5f;
                    DrawPiece3D(m.piece, fu, fv);
                }
            }
        }
        else
        {
            /*--- 非アニメ時: 現在の盤面をそのまま描画 ---*/
            for (int y = 0; y < rows; ++y)
            {
                for (int x = 0; x < cols; ++x)
                {
                    const Piece p = g_State.board.Get(x, y);
                    if (p == Piece::Empty) continue;

                    // 直近に置いた駒は上空から落ちて着地する演出を付ける
                    float hExtra = 0.0f;
                    if (g_LastPlacedTimer > 0.0
                        && x == g_LastPlacedPos.x && y == g_LastPlacedPos.y)
                    {
                        const float t = static_cast<float>(
                            g_LastPlacedTimer / GLOW_DURATION);   // 1→0
                        hExtra = t * t * P3_DROP_H;
                    }
                    DrawPiece3D(p, x + 0.5f, y + 0.5f, 1.0f, hExtra);
                }
            }
        }
    }

    /**
     * @brief  盤面の罫線と駒を描画する
     * @detail 効果アニメ中はフェーズ中間状態を補間描画し、
     *         非アニメ時は現在の盤面をそのまま描画する。
     */
    void DrawBoardAndPieces()
    {
        // 3Dビュー有効時は斜め俯瞰の立体描画へ委譲する
        if (View3D::Enabled())
        {
            DrawBoardAndPieces3D();
            return;
        }

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
            float raw = 1.0f - static_cast<float>(g_AnimPhaseTimer / g_PhaseDuration);
            raw = (raw < 0.0f) ? 0.0f : (raw > 1.0f ? 1.0f : raw);
            const float t = 1.0f - (1.0f - raw) * (1.0f - raw);

            // このフェーズで動かない駒を静止描画。
            // 出発マスの駒は補間描画に回す(出現効果の発生源は移動しないので除外しない)。
            for (int y = 0; y < disp.height; ++y)
            {
                for (int x = 0; x < disp.width; ++x)
                {
                    bool isMover = false;
                    for (const auto& m : phase)
                    {
                        if (!m.isSpawn && m.fromX == x && m.fromY == y)
                        {
                            isMover = true;
                            break;
                        }
                    }
                    if (isMover) continue;
                    const float cx = ox + x * CELL_SIZE + halfCell;
                    const float cy = oy + y * CELL_SIZE + halfCell;
                    DrawPiece(disp.Get(x, y), cx, cy, pieceRadius);
                }
            }
            // 移動駒は from→to で補間描画、出現駒は拡大しながら出現させる
            for (const auto& m : phase)
            {
                if (m.isSpawn)
                {
                    // 出現: 発生源から出現先へ連鎖パルス線を引き、駒を拡大表示する
                    const float scx = ox + m.fromX * CELL_SIZE + halfCell;
                    const float scy = oy + m.fromY * CELL_SIZE + halfCell;
                    const float dcx = ox + m.toX   * CELL_SIZE + halfCell;
                    const float dcy = oy + m.toY   * CELL_SIZE + halfCell;

                    // 連鎖パルス線 (発生源が盤内のときのみ。進行とともに減衰)
                    if (m.fromX >= 0 && m.fromY >= 0)
                    {
                        DirectX::XMFLOAT4 pulse = COLOR_CHAIN_PULSE;
                        pulse.w = (1.0f - t) * 0.7f;
                        Prim::DrawLine(scx, scy, dcx, dcy, 4.0f, pulse);
                    }
                    // 出現する駒 (半径を 0→満 へ拡大)
                    DrawPiece(m.piece, dcx, dcy, pieceRadius * t);
                }
                else
                {
                    const float fx = m.fromX + (m.toX - m.fromX) * t;
                    const float fy = m.fromY + (m.toY - m.fromY) * t;
                    const float cx = ox + fx * CELL_SIZE + halfCell;
                    const float cy = oy + fy * CELL_SIZE + halfCell;
                    DrawPiece(m.piece, cx, cy, pieceRadius);
                }
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
     * @brief  スライド方向を、辺に沿ったシェブロン矢印で可視化する
     * @param  d 滑走方向
     * @detail 氷盤ギミック(氷盤ボス/刻まれし者の氷盤モード)で使用。
     *         矢印は滑る方向へ流れる脈動アニメーションを伴う。
     */
    void DrawDirChevrons(Direction d)
    {
        const float cols = static_cast<float>(g_State.board.width);
        const float rows = static_cast<float>(g_State.board.height);
        const float M    = 0.18f;   // 盤端からの距離(マス)
        const bool horizontalSlide = (d == Direction::Left || d == Direction::Right);

        for (int i = 0; i < CHEVRON_COUNT; ++i)
        {
            // 辺に沿った位置 (端を避けて等間隔に分散)
            const float frac = (i + 1.0f) / (CHEVRON_COUNT + 1.0f);

            float u, v;
            if (horizontalSlide)
            {
                // 左右スライド → 縦の辺に沿って縦並び
                v = rows * frac;
                u = (d == Direction::Right) ? (cols + M) : (-M);
            }
            else
            {
                // 上下スライド → 横の辺に沿って横並び
                u = cols * frac;
                v = (d == Direction::Down) ? (rows + M) : (-M);
            }

            // 滑る方向へ流れる脈動 (位相を矢印ごとにずらす)
            const float wave = 0.5f + 0.5f * std::sin(
                static_cast<float>(g_AnimClock) * 5.0f - i * 0.9f);
            DirectX::XMFLOAT4 col = COLOR_SLIDE_ARROW;
            col.w = 0.30f + 0.60f * wave;

            const auto  p  = BoardPoint(u, v, 0.10f);
            const float px = PxPerCell(v);
            DrawChevron(p.x, p.y, d, 0.10f * px, 0.034f * px, col);
        }
    }

    /**
     * @brief  盤面の回転方向を、四辺を巡る循環シェブロンで可視化する
     * @param  cw 回転方向 (true=時計回り)
     * @detail 螺旋ギミック(螺旋盤ボス/刻まれし者の螺旋モード)で使用。
     *         脈動を循環順に走らせて回転方向を提示する。
     */
    void DrawRotChevrons(bool cw)
    {
        const float cols = static_cast<float>(g_State.board.width);
        const float rows = static_cast<float>(g_State.board.height);
        const float M    = 0.18f;   // 盤端からの距離(マス)

        // 四辺のシェブロンを「循環順」に並べる。各辺は循環方向を指す。
        struct Spot { float u, v; Direction dir; };
        Spot spots[4];
        if (cw)
        {
            // 時計回り: 上(右向き)→右(下向き)→下(左向き)→左(上向き)
            spots[0] = { cols * 0.5f, -M,           Direction::Right };
            spots[1] = { cols + M,    rows * 0.5f,  Direction::Down  };
            spots[2] = { cols * 0.5f, rows + M,     Direction::Left  };
            spots[3] = { -M,          rows * 0.5f,  Direction::Up    };
        }
        else
        {
            // 反時計回り: 上(左向き)→左(下向き)→下(右向き)→右(上向き)
            spots[0] = { cols * 0.5f, -M,           Direction::Left  };
            spots[1] = { -M,          rows * 0.5f,  Direction::Down  };
            spots[2] = { cols * 0.5f, rows + M,     Direction::Right };
            spots[3] = { cols + M,    rows * 0.5f,  Direction::Up    };
        }

        for (int i = 0; i < 4; ++i)
        {
            // 循環順(spots順)に脈動を走らせて回転の流れを表現する
            const float wave = 0.5f + 0.5f * std::sin(
                static_cast<float>(g_AnimClock) * 5.0f - i * 0.9f);
            DirectX::XMFLOAT4 col = COLOR_ROTATE_ARROW;
            col.w = 0.30f + 0.60f * wave;

            const auto  p  = BoardPoint(spots[i].u, spots[i].v, 0.10f);
            const float px = PxPerCell(spots[i].v);
            DrawChevron(p.x, p.y, spots[i].dir, 0.10f * px, 0.034f * px, col);
        }
    }

    /**
     * @brief  重力(下方向への落下)を、左右の辺の下向きシェブロンで可視化する
     * @detail 重力ギミック(重力場ボス/刻まれし者の重力モード)で使用。
     *         脈動が下へ流れ、駒が底へ吸われることを提示する。
     */
    void DrawGravityChevrons()
    {
        const float cols = static_cast<float>(g_State.board.width);
        const float rows = static_cast<float>(g_State.board.height);
        const float M    = 0.18f;   // 盤端からの距離(マス)

        for (int i = 0; i < CHEVRON_COUNT; ++i)
        {
            const float frac = (i + 1.0f) / (CHEVRON_COUNT + 1.0f);
            const float v    = rows * frac;

            // 下へ流れる脈動 (上の矢印ほど先に光る)
            const float wave = 0.5f + 0.5f * std::sin(
                static_cast<float>(g_AnimClock) * 5.0f - i * 0.9f);
            DirectX::XMFLOAT4 col = COLOR_GRAVITY_ARROW;
            col.w = 0.30f + 0.60f * wave;

            // 左右両辺に下向きシェブロンを描く
            const auto  pl = BoardPoint(-M,       v, 0.10f);
            const auto  pr = BoardPoint(cols + M, v, 0.10f);
            const float px = PxPerCell(v);
            DrawChevron(pl.x, pl.y, Direction::Down, 0.10f * px, 0.034f * px, col);
            DrawChevron(pr.x, pr.y, Direction::Down, 0.10f * px, 0.034f * px, col);
        }
    }

    /**
     * @brief  連鎖(隣接拡張)を、盤の四隅で脈動するダイヤ形で可視化する
     * @detail 連鎖ギミック(刻まれし者の連鎖モード等)で使用。
     *         鼓動のような点滅で「増殖する圧」を提示する。
     */
    void DrawChainCorners()
    {
        const float cols = static_cast<float>(g_State.board.width);
        const float rows = static_cast<float>(g_State.board.height);
        const float M    = 0.18f;   // 盤端からの距離(マス)

        const float corners[4][2] = {
            { -M,       -M },
            { cols + M, -M },
            { -M,       rows + M },
            { cols + M, rows + M },
        };

        // 鼓動風の脈動 (全隅が同位相で強く明滅する)
        const float beat = 0.5f + 0.5f * std::sin(static_cast<float>(g_AnimClock) * 6.0f);
        DirectX::XMFLOAT4 col = COLOR_CHAIN_PULSE;
        col.w = 0.25f + 0.65f * beat;

        for (const auto& c : corners)
        {
            const auto  p  = BoardPoint(c[0], c[1], 0.10f);
            const float px = PxPerCell(c[1]);
            const float r  = 0.10f * px * (0.7f + 0.3f * beat);
            const float th = 0.024f * px;

            // 4本の線でダイヤ(◇)を描く
            Prim::DrawLine(p.x,     p.y - r, p.x + r, p.y,     th, col);
            Prim::DrawLine(p.x + r, p.y,     p.x,     p.y + r, th, col);
            Prim::DrawLine(p.x,     p.y + r, p.x - r, p.y,     th, col);
            Prim::DrawLine(p.x - r, p.y,     p.x,     p.y - r, th, col);
        }
    }

    /**
     * @brief  不死(敗北打ち消し)発動時のバナーと画面フラッシュを描画する
     * @detail 盤面再生の瞬間が分かるよう、白フラッシュ＋中央バナーで提示する。
     */
    void DrawImmortalBanner()
    {
        if (g_ImmortalFlash <= 0.0) return;

        const float screenW = static_cast<float>(Direct3D_GetBackBufferWidth());
        const float screenH = static_cast<float>(Direct3D_GetBackBufferHeight());
        const float k = static_cast<float>(g_ImmortalFlash / IMMORTAL_FLASH_DURATION); // 1→0

        // 画面全体の白フラッシュ (発動直後が最も強い)
        Prim::DrawRect(0, 0, screenW, screenH, { 1.0f, 1.0f, 1.0f, 0.35f * k });

        // 中央バナー
        const char* msg = "不死発動 — 盤面が再生した";
        const float w = Text::MeasureWidth(msg, TEXT_SIZE_BIG * 0.7f);
        DirectX::XMFLOAT4 col = COLOR_TEXT_WIN;
        col.w = (k > 0.8f) ? 1.0f : (k / 0.8f);   // 終盤はフェードアウト
        Text::Draw((screenW - w) * 0.5f, screenH * 0.30f,
                   msg, TEXT_SIZE_BIG * 0.7f, col);
    }

    /**
     * @brief  直近に置いた駒から広がる衝撃リングを描画する (設置の手応え)
     * @detail グロー演出の進行に合わせてリングが広がりながら薄れる。
     *         BoardPoint 経由なので2D/3D双方で駒の位置に乗る。
     */
    void DrawPlacementImpact()
    {
        if (g_LastPlacedTimer <= 0.0 || g_LastPlacedPos.x < 0) return;

        const float t = 1.0f - static_cast<float>(g_LastPlacedTimer / GLOW_DURATION); // 0→1
        const View3D::P2 c = BoardPoint(g_LastPlacedPos.x + 0.5f, g_LastPlacedPos.y + 0.5f);
        const float cell = PxPerCell(g_LastPlacedPos.y + 0.5f);
        const float r    = cell * (0.20f + 0.55f * t);   // 設置点から外へ広がる
        DirectX::XMFLOAT4 col =
            (g_LastPlacedPiece == Piece::Player) ? COLOR_PIECE_O : COLOR_PIECE_X;
        col.w = (1.0f - t) * 0.55f;                       // 広がるほど薄れる
        Prim::DrawCircle(c.x, c.y, r, 3.0f, col, 24);
    }

    /**
     * @brief  勝利ラインをグロー付きで強調描画する (決着の見せ場)
     * @detail 勝利した並びのマス中心を貫く光の帯を、太→細の重ね描きで表現する。
     *         BoardPoint 経由なので2D/3D双方で正しい位置に乗る。
     *         隅支配・四角支配等の非ライン勝利では g_WinLine が空なので描かない。
     */
    void DrawWinningLine()
    {
        if (g_State.result == MatchResult::Playing) return;
        if (g_WinLine.size() < 2) return;

        const Vec2 a = g_WinLine.front();
        const Vec2 b = g_WinLine.back();
        const View3D::P2 pa = BoardPoint(a.x + 0.5f, a.y + 0.5f);
        const View3D::P2 pb = BoardPoint(b.x + 0.5f, b.y + 0.5f);

        // 脈動と、奥行きに応じた太さ基準 (3Dの遠近にも追従)
        const float pulse = 0.78f + 0.22f * static_cast<float>(std::sin(g_AnimClock * 6.0));
        const float midV  = (a.y + b.y) * 0.5f + 0.5f;
        const float baseW = PxPerCell(midV) * 0.40f;

        const DirectX::XMFLOAT4 col =
            (g_WinLineSide == Piece::Player) ? COLOR_PIECE_O : COLOR_PIECE_X;

        // 太く淡い → 細く濃い の重ね描きでグローを表現する
        for (int i = 0; i < 4; ++i)
        {
            const float t = i / 3.0f;                       // 0..1
            const float w = baseW * (1.0f - t * 0.78f);
            DirectX::XMFLOAT4 c = col;
            c.w = (0.12f + 0.34f * t) * pulse;
            Prim::DrawLine(pa.x, pa.y, pb.x, pb.y, w, c);
        }
        // 中央の白いコア
        Prim::DrawLine(pa.x, pa.y, pb.x, pb.y, baseW * 0.13f,
                       { 1.0f, 1.0f, 1.0f, 0.85f * pulse });
    }

    /**
     * @brief  決着の瞬間に全画面フラッシュを焚く (勝=緑寄り / 敗=赤寄り)
     * @detail 立ち上がりが最も明るく、二次関数で素早くフェードする。
     */
    void DrawWinFlash()
    {
        if (g_WinFlash <= 0.0) return;
        const float screenW = static_cast<float>(Direct3D_GetBackBufferWidth());
        const float screenH = static_cast<float>(Direct3D_GetBackBufferHeight());
        const float k = static_cast<float>(g_WinFlash / WIN_FLASH_DURATION);  // 1→0
        DirectX::XMFLOAT4 c = (g_WinLineSide == Piece::Player)
            ? DirectX::XMFLOAT4{ 0.55f, 1.00f, 0.65f, 0.0f }
            : DirectX::XMFLOAT4{ 1.00f, 0.45f, 0.45f, 0.0f };
        c.w = k * k * 0.5f;
        Prim::DrawRect(0, 0, screenW, screenH, c);
    }

    /**
     * @brief  現在のボスギミックに応じた視覚的補助をまとめて描画する
     * @detail 規約「盤面が動く効果には視覚的補助を必ず入れる」の集約点。
     *         固定ギミックのボスは対応する参照から、刻まれし者は
     *         現在モードに応じて切り替えて描画する。
     */
    void DrawGimmickIndicators()
    {
        if (g_State.result != MatchResult::Playing) return;

        if (g_IceSlideRef) DrawDirChevrons(g_IceSlideRef->currentDir);
        if (g_SpiralRef)   DrawRotChevrons(g_SpiralRef->clockwise);
        if (g_GravityRef)  DrawGravityChevrons();

        if (g_EngravedRef)
        {
            // 刻まれし者: 3つの力(設置系+構造系=回転+駒強化系=重力)を同時提示。
            // 構造系(回転)と駒強化系(重力)は常時、設置系は氷盤/連鎖で切替。
            DrawRotChevrons(g_EngravedRef->clockwise);
            DrawGravityChevrons();
            if (g_EngravedRef->IsChainPlace()) DrawChainCorners();
            else                               DrawDirChevrons(g_EngravedRef->currentDir);
        }
    }

    /**
     * @brief  アビリティ1列ぶん(ヘッダ+行+「…」)を描画する
     * @param  col        列番号 (0=消費, 1=パッシブ)
     * @param  header     列見出し
     * @param  list       その列のグループ一覧 (使い切った消費系は除外済み)
     * @param  clickable  消費系列なら true (任意発動ボタンで描画)
     * @param  playerTurn 今プレイヤーが発動操作できる状況か
     * @detail 各列は最大 ABILITY_VISIBLE_MAX 件、超過分は「…」行に集約する。
     *         下端揃えで上へ積むため、列ごとに高さが違っても下辺が揃う。
     */
    void DrawAbilityColumn(int col, const char* header,
                           const std::vector<AbilityGroup>& list,
                           bool clickable, bool playerTurn)
    {
        if (list.empty()) return;

        const int  n        = static_cast<int>(list.size());
        const int  visible  = (n < ABILITY_VISIBLE_MAX) ? n : ABILITY_VISIBLE_MAX;
        const bool showMore = n > ABILITY_VISIBLE_MAX;
        const int  rowCount = visible + (showMore ? 1 : 0);
        const float x       = AbilityColX(col);
        const auto& mouse   = InputManager_GetMouseState();

        Text::Draw(x, AbilityListTop(rowCount), header, TEXT_SIZE_HUD, COLOR_TEXT);

        for (int i = 0; i < visible; ++i)
        {
            const AbilityGroup& g = list[i];
            const float ry = AbilityRowY(i, rowCount);

            if (clickable)
            {
                // 任意発動(消費系): クリック可能ボタン
                const bool usable = playerTurn && GroupCanActivate(g);
                const bool hover  = usable
                    && mouse.x >= x && mouse.x <= x + ABILITY_BTN_W
                    && mouse.y >= ry && mouse.y <= ry + ABILITY_BTN_H;

                DirectX::XMFLOAT4 accent = usable ? COLOR_SLIDE_ARROW : COLOR_TEXT_HINT;
                if (!usable) accent.w = 0.40f;
                Prim::DrawRect(x - 3.0f, ry - 3.0f,
                               ABILITY_BTN_W + 6.0f, ABILITY_BTN_H + 6.0f, accent);
                Prim::DrawRect(x, ry, ABILITY_BTN_W, ABILITY_BTN_H,
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
                Text::Draw(x + 12.0f, ry + (ABILITY_BTN_H - TEXT_SIZE_HINT) * 0.5f,
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
                    std::snprintf(rowBuf, sizeof(rowBuf), "・%s", g.rep->name.c_str());
                Text::Draw(x, ry + (ABILITY_ROW_H - TEXT_SIZE_HINT) * 0.5f,
                           rowBuf, TEXT_SIZE_HINT, COLOR_TEXT_HINT);
            }
        }

        /*--- 超過分: 「…」行 (クリックで履歴ポップアップを開く) ---*/
        if (showMore)
        {
            const float ry = AbilityRowY(visible, rowCount);
            const bool hover = !g_AbilityPopupOpen
                && mouse.x >= x && mouse.x <= x + ABILITY_BTN_W
                && mouse.y >= ry && mouse.y <= ry + ABILITY_BTN_H;
            Prim::DrawRect(x - 3.0f, ry - 3.0f,
                           ABILITY_BTN_W + 6.0f, ABILITY_BTN_H + 6.0f, COLOR_TEXT_HINT);
            Prim::DrawRect(x, ry, ABILITY_BTN_W, ABILITY_BTN_H,
                           hover ? COLOR_BTN_HOVER : COLOR_BTN_BG);
            char moreLabel[64];
            std::snprintf(moreLabel, sizeof(moreLabel), "…  他%d件を見る", n - visible);
            Text::Draw(x + 12.0f, ry + (ABILITY_BTN_H - TEXT_SIZE_HINT) * 0.5f,
                       moreLabel, TEXT_SIZE_HINT, COLOR_TEXT);
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

        /*--- 設定キーの案内 (右下、控えめに) ---*/
        {
            const float screenH = static_cast<float>(Direct3D_GetBackBufferHeight());
            const char* sHint = "O 設定";
            const float sw = Text::MeasureWidth(sHint, TEXT_SIZE_HINT);
            Text::Draw(screenW - sw - 24.0f, screenH - 30.0f,
                       sHint, TEXT_SIZE_HINT, COLOR_TEXT_HINT);
        }

        /*--- アビリティ発動直後の浮き上がりフラッシュ表示 ---*/
        if (g_TimerFlash > 0.0 && !g_FlashText.empty())
        {
            const float k = static_cast<float>(g_TimerFlash / TIMER_FLASH_DURATION); // 1→0
            DirectX::XMFLOAT4 popColor = COLOR_TEXT_WIN;
            popColor.w = k;
            const float popY = 24.0f - (1.0f - k) * 22.0f;
            Text::Draw(24.0f + Text::MeasureWidth(buf, TEXT_SIZE_HUD) + 14.0f,
                       popY, g_FlashText.c_str(), TEXT_SIZE_HINT, popColor);
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

            // 刻まれし者: 現在振るっている力の名前を提示 (毎ターン変わるため)
            if (g_EngravedRef && g_State.result == MatchResult::Playing)
            {
                char modeBuf[64];
                std::snprintf(modeBuf, sizeof(modeBuf), "宿りし力：%s",
                              g_EngravedRef->PowersLabel());
                const float modeW = Text::MeasureWidth(modeBuf, TEXT_SIZE_HINT);
                Text::Draw((screenW - modeW) * 0.5f,
                           24.0f + TEXT_SIZE_HUD + 4.0f + TEXT_SIZE_HINT + 4.0f,
                           modeBuf, TEXT_SIZE_HINT, COLOR_TEXT_URGENT);
            }
        }
        // 盤面ギミックの方向/回転/重力/連鎖は DrawGimmickIndicators で可視化

        /*--- 所持アビリティ (画面左下: 消費系 / パッシブ の2列) ---*/
        // 各列10件まで表示し、超過は「…」行から履歴ポップアップへ。
        // 使い切った消費系(残0)は BuildAbilityGroups の時点で除外済み。
        const auto groups = BuildAbilityGroups();
        if (!groups.empty())
        {
            std::vector<AbilityGroup> cons, pass;   // 消費系 / パッシブ
            for (const auto& g : groups)
                (g.rep->activatable ? cons : pass).push_back(g);

            const bool playerTurn =
                (g_State.result == MatchResult::Playing
                 && g_State.currentPlayer == Piece::Player
                 && !IsEffectAnimating()
                 && !g_AbilityPopupOpen);

            DrawAbilityColumn(0, "消費",     cons, true,  playerTurn);
            DrawAbilityColumn(1, "パッシブ", pass, false, playerTurn);
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
    enum class GameOverAction { Reward, Result };

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
     * @detail 通常ボス撃破は「報酬を受け取る」(報酬画面へ)。
     *         最終ボス撃破(クリア)・敗北はラン終了なので「結果を見る」
     *         (リザルト画面へ)。Update/Draw で共有する。
     */
    std::vector<GameOverButton> BuildGameOverButtons()
    {
        std::vector<GameOverButton> list;
        const float screenW = static_cast<float>(Direct3D_GetBackBufferWidth());
        const float screenH = static_cast<float>(Direct3D_GetBackBufferHeight());
        const float by = screenH * 0.5f + 30.0f;
        const float x  = (screenW - BUTTON_W) * 0.5f;

        const bool isFinalBoss =
            (RunState::CurrentBossIndex() >= RunState::BossCount() - 1);

        if (g_State.result == MatchResult::Win && !isFinalBoss)
        {
            // 通常ボス撃破: 報酬画面へ進む
            list.push_back({ { x, by, BUTTON_W, BUTTON_H, "報酬を受け取る" },
                             GameOverAction::Reward, COLOR_TEXT_WIN });
        }
        else
        {
            // 最終ボス撃破(クリア) または 敗北/引分/時間切れ → リザルトへ
            const DirectX::XMFLOAT4 accent = (g_State.result == MatchResult::Win)
                ? COLOR_TEXT_WIN : COLOR_PIECE_O;
            list.push_back({ { x, by, BUTTON_W, BUTTON_H, "結果を見る" },
                             GameOverAction::Result, accent });
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

        // 終局の着手・効果アニメを見せ切るまでオーバーレイは出さない。
        // (最後のマークが着地してから「勝利！」と報酬選択を提示する)
        if (IsEffectAnimating() || g_LastPlacedTimer > 0.0) return;

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

    // 対戦BGM (素材未配置なら無音)。報酬画面と同一曲なので往復で途切れない。
    SoundManager_PlayBGM(SOUND_BGM_GAME);
}

void Game_Finalize()
{
    // 他シーンへ揺れを持ち越さないよう、画面オフセットを必ず戻す
    Sprite_SetViewOffset(0.0f, 0.0f);
}

//======================================
// 更新
//======================================
void Game_Update(double elapsed_time)
{
    // 3Dカメラを現在の盤面サイズ・画面サイズに合わせる
    // (クリックのセル判定が投影を参照するため、更新側でも構成する)
    View3D::Setup(g_State.board.width, g_State.board.height,
                  static_cast<float>(Direct3D_GetBackBufferWidth()),
                  static_cast<float>(Direct3D_GetBackBufferHeight()));

    g_AnimClock += elapsed_time;  // シェブロン脈動用クロック
    if (g_TimerFlash    > 0.0) g_TimerFlash    -= elapsed_time;
    if (g_ImmortalFlash > 0.0) g_ImmortalFlash -= elapsed_time;
    if (g_WinFlash      > 0.0) g_WinFlash      -= elapsed_time;

    View3D::UpdateImpact(elapsed_time);  // 着手の衝撃(盤面の傾き振動)を減衰させる

    // 画面シェイク: 残り時間に比例して振幅を減衰させ、全描画へオフセットを適用する。
    // (Sprite側でシフトするため盤面・駒・HUD・3D投影がまとめて揺れる)
    if (g_ShakeTime > 0.0)
    {
        g_ShakeTime -= elapsed_time;
        if (g_ShakeTime < 0.0) g_ShakeTime = 0.0;
        const double amp = g_ShakeMag * (g_ShakeDur > 0.0 ? g_ShakeTime / g_ShakeDur : 0.0);
        Sprite_SetViewOffset(
            static_cast<float>(amp * std::sin(g_AnimClock * 47.0)),
            static_cast<float>(amp * std::cos(g_AnimClock * 41.0)));
    }
    else
    {
        Sprite_SetViewOffset(0.0f, 0.0f);
    }

    // 勝敗が確定した瞬間に一度だけ結果音と決着演出をセットする (Playing→終局の遷移を検出)
    if (g_State.result != g_PrevResult)
    {
        if (g_State.result == MatchResult::Win)
        {
            SoundManager_PlayResultTheme(SOUND_SE_WIN);   // 対戦BGMを止めて単独再生
            g_WinLineSide = Piece::Player;
            g_WinLine     = WinCheck::FindWinningLine(g_State.board, Piece::Player);
            g_WinFlash    = WIN_FLASH_DURATION;
            TriggerShake(15.0, 0.45);
            g_HitStop = 0.07;   // 決着の一瞬を凍結して手応えを出す
        }
        else if (g_State.result == MatchResult::Lose)
        {
            SoundManager_PlayResultTheme(SOUND_SE_LOSE);  // 対戦BGMを止めて単独再生
            g_WinLineSide = Piece::Enemy;
            g_WinLine     = WinCheck::FindWinningLine(g_State.board, Piece::Enemy);
            g_WinFlash    = WIN_FLASH_DURATION;
            TriggerShake(13.0, 0.40);
            g_HitStop = 0.07;
        }
        else if (g_State.result == MatchResult::Timeout)
        {
            SoundManager_PlayResultTheme(SOUND_SE_LOSE);  // 対戦BGMを止めて単独再生
            g_WinLineSide = Piece::Enemy;   // 時間切れはライン無し・赤フラッシュのみ
            g_WinLine.clear();
            g_WinFlash    = WIN_FLASH_DURATION;
            TriggerShake(8.0, 0.35);
        }
        g_PrevResult = g_State.result;
    }

    // ヒットストップ中はゲーム進行を凍結する (シェイク等の演出系は上で更新済み)
    if (g_HitStop > 0.0)
    {
        g_HitStop -= elapsed_time;
        return;
    }

    // マウスホイールの変化量を算出 (履歴ポップアップのスクロールに使用)
    {
        const int wheel = InputManager_GetMouseState().scrollWheelValue;
        g_WheelDelta = wheel - g_WheelPrev;
        g_WheelPrev  = wheel;
    }

    // ラン統計: 対戦中の経過時間を累積する
    if (g_State.result == MatchResult::Playing)
    {
        RunState::AddRunTime(elapsed_time);
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
            if (IsEffectAnimating()) g_AnimPhaseTimer = g_PhaseDuration;
        }
        if (g_LastPlacedTimer > 0.0)
        {
            g_LastPlacedTimer -= elapsed_time;
            if (g_LastPlacedTimer < 0.0) g_LastPlacedTimer = 0.0;
        }
        return;
    }

    /*--- 終局直後: 最後の着手の演出を見せ切ってから終了UIへ移る ---*/
    //   勝敗が確定しても、直前に置いたマスの着地・グロー演出が残っている
    //   間は終了オーバーレイ/報酬ボタンを出さず、最後のマークを見せ切る。
    if (g_State.result != MatchResult::Playing && g_LastPlacedTimer > 0.0)
    {
        g_LastPlacedTimer -= elapsed_time;
        if (g_LastPlacedTimer < 0.0) g_LastPlacedTimer = 0.0;
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
        SoundManager_PlaySE(SOUND_SE_DECIDE);
        switch (go.action)
        {
        case GameOverAction::Reward:
            // 通常ボス撃破: 固有スキルを「保留報酬」として提示し、撃破画面で
            // 獲得可否(はい/いいえ)を選ばせる。その後に別枠の通常抽選(3択)へ進む。
            if (g_Boss)
                RunState::SetPendingBossReward(g_Boss->GetRewardAbility(),
                                               g_Boss->name, g_Boss->description);
            RunState::IncrementBoss();
            Scene_Change(Scene::BOSS_REWARD);
            break;
        case GameOverAction::Result:
            // ラン終了(最終ボス撃破=クリア / 敗北) → 戦績を確定しリザルトへ
            if (g_State.result == MatchResult::Win)
            {
                RunState::IncrementBoss();    // 最終撃破を反映(撃破数=総数)
                RunState::MarkRunCleared();
                RunState::ClearSave();        // ラン完走 → セーブ削除
                RunState::CaptureResult(true, "");
            }
            else
            {
                // 敗北/引き分け: 対戦ボス名を記録(セーブは寛容仕様で残す)。
                // 引き分け終了は敗北と区別して記録する。
                const bool wasDraw = (g_State.result == MatchResult::Draw);
                RunState::CaptureResult(false,
                                        g_Boss ? g_Boss->name : std::string(),
                                        wasDraw);
            }
            Scene_Change(Scene::RESULT);
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
    // 3Dカメラを現在の盤面サイズ・画面サイズに合わせる
    View3D::Setup(g_State.board.width, g_State.board.height,
                  static_cast<float>(Direct3D_GetBackBufferWidth()),
                  static_cast<float>(Direct3D_GetBackBufferHeight()));

    // ボスごとのテーマ色で全画面背景を塗る (盤面の周囲に雰囲気を出す)
    if (g_Boss)
    {
        const float screenW = static_cast<float>(Direct3D_GetBackBufferWidth());
        const float screenH = static_cast<float>(Direct3D_GetBackBufferHeight());
        Prim::DrawRect(0, 0, screenW, screenH,
                       { g_Boss->bgR, g_Boss->bgG, g_Boss->bgB, 1.0f });
    }

    DrawBoardAndPieces();     // 盤面と駒
    DrawPlacementImpact();    // 直近設置の衝撃リング
    DrawWinningLine();        // 勝利ラインのグロー強調 (決着時)
    DrawGimmickIndicators();  // ボスギミックの視覚的補助 (方向/回転/重力/連鎖)
    DrawImmortalBanner();     // 不死発動 (敗北打ち消し) のバナー
    DrawWinFlash();           // 決着の全画面フラッシュ (勝=緑/敗=赤)
    DrawHud();                // HUD (残時間/手番/アビリティ)
    DrawAbilityPopup();       // アビリティ履歴ポップアップ
    DrawGameOverOverlay();    // ゲームオーバー時のオーバーレイ
}
