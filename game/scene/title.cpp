/****************************************
 * @file   title.cpp
 * @brief  タイトルシーン (Day 1-2)
 * @author Natsume Shidara
 * @date   2026/05/15
 ****************************************/

#include "title.h"
#include "scene.h"
#include "run_state.h"
#include "boss/boss_roster.h"
#include "render_primitives.h"
#include "text_draw.h"
#include "config.h"

#include "direct3d.h"
#include "input_manager.h"
#include "keyboard.h"
#include "sound_manager.h"

//--------------------------------------
// 定数・内部状態 (無名名前空間)
//--------------------------------------
namespace
{
    // 文字サイズ
    constexpr float TEXT_TITLE_SIZE  = 72.0f;   // メインロゴ
    constexpr float TEXT_SUBTITLE    = 22.0f;   // サブタイトル
    constexpr float TEXT_HINT_SIZE   = 22.0f;   // 開始プロンプト
    constexpr float TEXT_CLEAR_SIZE  = 30.0f;   // ランクリア(制覇)表示
    constexpr float TEXT_BTN_SIZE    = 22.0f;   // メニューボタン文字

    // 配色 (RGBA)
    const DirectX::XMFLOAT4 COLOR_BG       { 0.04f, 0.04f, 0.08f, 1.0f };  // 背景
    const DirectX::XMFLOAT4 COLOR_TITLE    { 0.90f, 0.95f, 1.00f, 1.0f };  // ロゴ文字
    const DirectX::XMFLOAT4 COLOR_SUB      { 0.55f, 0.85f, 1.00f, 1.0f };  // サブタイトル
    const DirectX::XMFLOAT4 COLOR_HINT     { 0.60f, 0.65f, 0.75f, 1.0f };  // プロンプト
    const DirectX::XMFLOAT4 COLOR_CLEAR    { 0.60f, 1.00f, 0.60f, 1.0f };  // 制覇表示
    const DirectX::XMFLOAT4 COLOR_WARN     { 1.00f, 0.70f, 0.30f, 1.0f };  // 設定エラー警告
    const DirectX::XMFLOAT4 COLOR_BTN_BG   { 0.12f, 0.13f, 0.20f, 0.96f };  // ボタン本体
    const DirectX::XMFLOAT4 COLOR_BTN_HOVER{ 0.24f, 0.28f, 0.40f, 0.98f };  // ボタン(ホバー)
    const DirectX::XMFLOAT4 COLOR_BTN_EDGE { 0.55f, 0.85f, 1.00f, 1.0f };  // ボタン縁取り

    // メニューボタンの寸法
    constexpr float BTN_W    = 260.0f;
    constexpr float BTN_H    = 56.0f;
    constexpr float BTN_GAP  = 40.0f;
    constexpr float HBTN_W   = 200.0f;   // 戦績ボタン
    constexpr float HBTN_H   = 44.0f;

    double g_BlinkTimer = 0.0;   // 開始プロンプト点滅用の経過時間
    bool   g_ConfigOk   = true;  // game_config.json の読み込みに成功したか

    //--------------------------------------
    // メニューボタンのレイアウト (Update/Draw 共有)
    //--------------------------------------
    /** @brief 矩形ボタン */
    struct Btn { float x, y, w, h; };

    /** @brief 「つづきから」ボタン領域 (セーブあり時の左ボタン) */
    Btn ContinueBtn(float screenW, float screenH)
    {
        const float totalW = BTN_W * 2.0f + BTN_GAP;
        const float x0 = (screenW - totalW) * 0.5f;
        return { x0, screenH * 0.66f, BTN_W, BTN_H };
    }
    /** @brief 「はじめから」ボタン領域 (セーブあり時の右ボタン) */
    Btn NewGameBtn(float screenW, float screenH)
    {
        const float totalW = BTN_W * 2.0f + BTN_GAP;
        const float x0 = (screenW - totalW) * 0.5f;
        return { x0 + BTN_W + BTN_GAP, screenH * 0.66f, BTN_W, BTN_H };
    }
    /** @brief 「戦績」ボタン領域 (画面下部中央。常時表示) */
    Btn HistoryBtn(float screenW, float screenH)
    {
        return { (screenW - HBTN_W) * 0.5f, screenH * 0.85f, HBTN_W, HBTN_H };
    }
    /** @brief 座標がボタン領域内か */
    bool BtnContains(const Btn& b, int mx, int my)
    {
        return mx >= b.x && mx <= b.x + b.w && my >= b.y && my <= b.y + b.h;
    }
    /** @brief ボタンを縁取り+本体+中央ラベルで描画 */
    void DrawBtn(const Btn& b, const char* label, bool hover)
    {
        Prim::DrawRect(b.x - 3.0f, b.y - 3.0f, b.w + 6.0f, b.h + 6.0f, COLOR_BTN_EDGE);
        Prim::DrawRect(b.x, b.y, b.w, b.h, hover ? COLOR_BTN_HOVER : COLOR_BTN_BG);
        const float tw = Text::MeasureWidth(label, TEXT_BTN_SIZE);
        Text::Draw(b.x + (b.w - tw) * 0.5f, b.y + (b.h - TEXT_BTN_SIZE) * 0.5f,
                   label, TEXT_BTN_SIZE, COLOR_TITLE);
    }

    /** @brief 新規ランを開始する (ラン状態を初期化して本編へ) */
    void StartNewRun()
    {
        SoundManager_PlaySE(SOUND_SE_DECIDE);
        RunState::ResetRun();
        Scene_Change(Scene::GAME);
    }
}

//======================================
// 初期化／終了
//======================================
void Title_Initialize()
{
    // 設定ファイルを再読込 (タイトルへ戻るたびに調整が反映される)。
    // 失敗時はタイトルに警告を表示し、調整が効かない原因に気づけるようにする。
    g_ConfigOk = Config::Reload();
    // 設定反映後にボス出現順を再構築する ("bossOrder" の編集を反映)。
    BossRoster::Reload();

    // 描画モジュールを初期化し、点滅タイマーをリセット
    Prim::Initialize();
    Text::Initialize();
    g_BlinkTimer = 0.0;

    // タイトルBGM (素材未配置なら無音で安全に継続)
    SoundManager_PlayBGM(SOUND_BGM_TITLE);
}

void Title_Finalize()
{
}

//======================================
// 更新
//======================================
void Title_Update(double elapsed_time)
{
    // 点滅プロンプト用に経過時間を積算
    g_BlinkTimer += elapsed_time;

    const float screenW = static_cast<float>(Direct3D_GetBackBufferWidth());
    const float screenH = static_cast<float>(Direct3D_GetBackBufferHeight());
    const auto& mouse   = InputManager_GetMouseState();

    // 「戦績」ボタンのクリックを最優先で処理 (開始入力に吸われないよう先に消費)
    if (InputManager_IsMouseLeftTrigger()
     && BtnContains(HistoryBtn(screenW, screenH), mouse.x, mouse.y))
    {
        SoundManager_PlaySE(SOUND_SE_DECIDE);
        Scene_Change(Scene::HISTORY);
        return;
    }

    if (RunState::HasSave())
    {
        /*--- セーブあり: 「つづきから」「はじめから」の2択 ---*/
        const Btn cont = ContinueBtn(screenW, screenH);
        const Btn neu  = NewGameBtn(screenW, screenH);

        // マウスクリック(明示選択)を最優先。クリックが無いフレームのみ、
        // Space/Enter を「つづきから」(再開が既定の意図) として扱う。
        bool wantContinue = false;
        bool wantNew      = false;
        if (InputManager_IsMouseLeftTrigger())
        {
            if (BtnContains(cont, mouse.x, mouse.y)) wantContinue = true;
            else if (BtnContains(neu, mouse.x, mouse.y)) wantNew = true;
        }
        if (!wantContinue && !wantNew
            && (Keyboard_IsKeyDown(KK_SPACE) || Keyboard_IsKeyDown(KK_ENTER)))
        {
            wantContinue = true;
        }

        if (wantContinue)
        {
            SoundManager_PlaySE(SOUND_SE_DECIDE);
            // 復元失敗時は安全側で新規ランにフォールバック。
            // 破損セーブはここで削除し、「つづきから」が再表示され続けるのを防ぐ。
            if (!RunState::LoadRun())
            {
                RunState::ClearSave();
                RunState::ResetRun();
            }
            Scene_Change(Scene::GAME);
        }
        else if (wantNew)
        {
            StartNewRun();   // セーブは次戦開始時(ResetMatch)に上書きされる
        }
    }
    else
    {
        /*--- セーブなし: Space / Enter / 左クリックで新規開始 ---*/
        if (Keyboard_IsKeyDown(KK_SPACE)
         || Keyboard_IsKeyDown(KK_ENTER)
         || InputManager_IsMouseLeftTrigger())
        {
            StartNewRun();
        }
    }
}

//======================================
// 描画
//======================================
void Title_Draw()
{
    const float screenW = static_cast<float>(Direct3D_GetBackBufferWidth());
    const float screenH = static_cast<float>(Direct3D_GetBackBufferHeight());

    // 背景を塗りつぶす
    Prim::DrawRect(0, 0, screenW, screenH, COLOR_BG);

    /*--- タイトルロゴ + サブタイトルを中央に描画 ---*/
    const char* title    = "MARUBATSU";
    const char* subtitle = "〇×ローグライト";
    const float titleW = Text::MeasureWidth(title, TEXT_TITLE_SIZE);
    const float subW   = Text::MeasureWidth(subtitle, TEXT_SUBTITLE);

    const float titleY = screenH * 0.35f;
    Text::Draw((screenW - titleW) * 0.5f, titleY, title, TEXT_TITLE_SIZE, COLOR_TITLE);
    Text::Draw((screenW - subW)   * 0.5f, titleY + TEXT_TITLE_SIZE + 8.0f,
               subtitle, TEXT_SUBTITLE, COLOR_SUB);

    /*--- ランクリア(制覇)表示 — 全ボス撃破後にタイトルへ戻った場合 ---*/
    if (RunState::IsRunCleared())
    {
        const char* clearMsg = "全ボス撃破！ 制覇おめでとう";
        const float clearW = Text::MeasureWidth(clearMsg, TEXT_CLEAR_SIZE);
        Text::Draw((screenW - clearW) * 0.5f, screenH * 0.18f,
                   clearMsg, TEXT_CLEAR_SIZE, COLOR_CLEAR);
    }

    if (RunState::HasSave())
    {
        /*--- セーブあり: 2択メニューを表示 ---*/
        const Btn cont = ContinueBtn(screenW, screenH);
        const Btn neu  = NewGameBtn(screenW, screenH);
        const auto& mouse = InputManager_GetMouseState();
        DrawBtn(cont, "つづきから", BtnContains(cont, mouse.x, mouse.y));
        DrawBtn(neu,  "はじめから", BtnContains(neu,  mouse.x, mouse.y));
    }
    else
    {
        /*--- セーブなし: 開始プロンプト (一定周期で点滅) ---*/
        const bool show = (static_cast<int>(g_BlinkTimer * 1.5) % 2) == 0;
        if (show)
        {
            const char* hint = "スペース または クリックで開始";
            const float hintW = Text::MeasureWidth(hint, TEXT_HINT_SIZE);
            Text::Draw((screenW - hintW) * 0.5f, screenH * 0.72f,
                       hint, TEXT_HINT_SIZE, COLOR_HINT);
        }
    }

    /*--- 設定読み込み失敗の警告 (調整が効かない原因に気づけるように) ---*/
    if (!g_ConfigOk)
    {
        const char* warn = "game_config.json 読み込み失敗 — 既定値で動作中";
        const float ww = Text::MeasureWidth(warn, 16.0f);
        Text::Draw((screenW - ww) * 0.5f, screenH - 28.0f, warn, 16.0f, COLOR_WARN);
    }

    /*--- 「戦績」ボタン (常時表示) ---*/
    {
        const Btn h = HistoryBtn(screenW, screenH);
        const auto& mouse = InputManager_GetMouseState();
        DrawBtn(h, "戦績", BtnContains(h, mouse.x, mouse.y));
    }

    /*--- 設定キーの案内 (左上、控えめに) ---*/
    Text::Draw(24.0f, 24.0f, "O キー: 設定", 18.0f, COLOR_HINT);
}
