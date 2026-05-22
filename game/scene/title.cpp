/****************************************
 * @file title.cpp
 * @brief タイトルシーン (Day 1-2)
 * @author Natsume Shidara
 * @date 2026/05/15
 ****************************************/

#include "title.h"
#include "scene.h"
#include "run_state.h"
#include "render_primitives.h"
#include "text_draw.h"

#include "direct3d.h"
#include "input_manager.h"
#include "keyboard.h"

namespace
{
    constexpr float TEXT_TITLE_SIZE  = 72.0f;
    constexpr float TEXT_SUBTITLE    = 22.0f;
    constexpr float TEXT_HINT_SIZE   = 22.0f;

    const DirectX::XMFLOAT4 COLOR_BG       { 0.04f, 0.04f, 0.08f, 1.0f };
    const DirectX::XMFLOAT4 COLOR_TITLE    { 0.90f, 0.95f, 1.00f, 1.0f };
    const DirectX::XMFLOAT4 COLOR_SUB      { 0.55f, 0.85f, 1.00f, 1.0f };
    const DirectX::XMFLOAT4 COLOR_HINT     { 0.60f, 0.65f, 0.75f, 1.0f };

    double g_BlinkTimer = 0.0;
}

void Title_Initialize()
{
    Prim::Initialize();
    Text::Initialize();
    g_BlinkTimer = 0.0;
}

void Title_Finalize()
{
}

void Title_Update(double elapsed_time)
{
    g_BlinkTimer += elapsed_time;

    // Space / Enter / 左クリックでスタート → 新規ラン開始
    if (Keyboard_IsKeyDown(KK_SPACE)
     || Keyboard_IsKeyDown(KK_ENTER)
     || InputManager_IsMouseLeftTrigger())
    {
        RunState::ResetRun();
        Scene_Change(Scene::GAME);
    }
}

void Title_Draw()
{
    const float screenW = static_cast<float>(Direct3D_GetBackBufferWidth());
    const float screenH = static_cast<float>(Direct3D_GetBackBufferHeight());

    Prim::DrawRect(0, 0, screenW, screenH, COLOR_BG);

    // タイトルロゴ
    const char* title    = "MARUBATSU";
    const char* subtitle = "tactical roguelite";
    const float titleW = Text::MeasureWidth(title, TEXT_TITLE_SIZE);
    const float subW   = Text::MeasureWidth(subtitle, TEXT_SUBTITLE);

    const float titleY = screenH * 0.35f;
    Text::Draw((screenW - titleW) * 0.5f, titleY, title, TEXT_TITLE_SIZE, COLOR_TITLE);
    Text::Draw((screenW - subW)   * 0.5f, titleY + TEXT_TITLE_SIZE + 8.0f,
               subtitle, TEXT_SUBTITLE, COLOR_SUB);

    // 点滅プロンプト
    const bool show = (static_cast<int>(g_BlinkTimer * 1.5) % 2) == 0;
    if (show)
    {
        const char* hint = "PRESS SPACE / CLICK TO START";
        const float hintW = Text::MeasureWidth(hint, TEXT_HINT_SIZE);
        Text::Draw((screenW - hintW) * 0.5f, screenH * 0.72f,
                   hint, TEXT_HINT_SIZE, COLOR_HINT);
    }
}
