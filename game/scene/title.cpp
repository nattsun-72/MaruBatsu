/****************************************
 * @file   title.cpp
 * @brief  タイトルシーン (Day 1-2)
 * @author Natsume Shidara
 * @date   2026/05/15
 ****************************************/

#include "title.h"
#include "scene.h"
#include "run_state.h"
#include "render_primitives.h"
#include "text_draw.h"

#include "direct3d.h"
#include "input_manager.h"
#include "keyboard.h"

//--------------------------------------
// 定数・内部状態 (無名名前空間)
//--------------------------------------
namespace
{
    // 文字サイズ
    constexpr float TEXT_TITLE_SIZE  = 72.0f;   // メインロゴ
    constexpr float TEXT_SUBTITLE    = 22.0f;   // サブタイトル
    constexpr float TEXT_HINT_SIZE   = 22.0f;   // 開始プロンプト

    // 配色 (RGBA)
    const DirectX::XMFLOAT4 COLOR_BG       { 0.04f, 0.04f, 0.08f, 1.0f };  // 背景
    const DirectX::XMFLOAT4 COLOR_TITLE    { 0.90f, 0.95f, 1.00f, 1.0f };  // ロゴ文字
    const DirectX::XMFLOAT4 COLOR_SUB      { 0.55f, 0.85f, 1.00f, 1.0f };  // サブタイトル
    const DirectX::XMFLOAT4 COLOR_HINT     { 0.60f, 0.65f, 0.75f, 1.0f };  // プロンプト

    double g_BlinkTimer = 0.0;   // 開始プロンプト点滅用の経過時間
}

//======================================
// 初期化／終了
//======================================
void Title_Initialize()
{
    // 描画モジュールを初期化し、点滅タイマーをリセット
    Prim::Initialize();
    Text::Initialize();
    g_BlinkTimer = 0.0;
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

    // Space / Enter / 左クリックでスタート → 新規ラン開始
    if (Keyboard_IsKeyDown(KK_SPACE)
     || Keyboard_IsKeyDown(KK_ENTER)
     || InputManager_IsMouseLeftTrigger())
    {
        RunState::ResetRun();          // ラン状態を初期化
        Scene_Change(Scene::GAME);     // ゲーム本編へ遷移を予約
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

    /*--- 開始プロンプト (一定周期で点滅) ---*/
    const bool show = (static_cast<int>(g_BlinkTimer * 1.5) % 2) == 0;
    if (show)
    {
        const char* hint = "スペース または クリックで開始";
        const float hintW = Text::MeasureWidth(hint, TEXT_HINT_SIZE);
        Text::Draw((screenW - hintW) * 0.5f, screenH * 0.72f,
                   hint, TEXT_HINT_SIZE, COLOR_HINT);
    }
}
