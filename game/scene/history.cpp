/****************************************
 * @file   history.cpp
 * @brief  戦績履歴シーン実装
 * @author Natsume Shidara
 * @date   2026/06/05
 *
 * RunState::LoadHistory() で読み込んだ過去ランの戦績を一覧表示する。
 * マウスホイールで縦スクロール、「戻る」でタイトルへ。
 ****************************************/

#include "history.h"
#include "scene.h"

#include "run_state.h"
#include "render_primitives.h"
#include "text_draw.h"

#include "direct3d.h"
#include "input_manager.h"
#include "sound_manager.h"

#include <DirectXMath.h>
#include <cstdio>
#include <string>
#include <vector>

//--------------------------------------
// 定数・内部状態 (無名名前空間)
//--------------------------------------
namespace
{
    // 文字サイズ
    constexpr float TEXT_HEAD = 36.0f;   // 見出し
    constexpr float TEXT_ROW  = 20.0f;   // 履歴1行
    constexpr float TEXT_BTN  = 22.0f;   // ボタン

    // レイアウト
    constexpr float LIST_X      = 80.0f;   // 一覧の左端X
    constexpr float LIST_TOP    = 120.0f;  // 一覧の上端Y
    constexpr float ROW_H       = 30.0f;   // 1行の高さ
    constexpr float LIST_BOTTOM = 80.0f;   // 一覧の下端マージン(画面下からの距離)
    constexpr float SCROLL_SPEED = 1.5f;   // ホイール量→スクロール量の係数

    // 配色 (RGBA)
    const DirectX::XMFLOAT4 COLOR_BG       { 0.05f, 0.05f, 0.09f, 1.0f };  // 背景
    const DirectX::XMFLOAT4 COLOR_HEAD     { 0.85f, 0.90f, 1.00f, 1.0f };  // 見出し
    const DirectX::XMFLOAT4 COLOR_WIN      { 0.60f, 1.00f, 0.60f, 1.0f };  // クリア行
    const DirectX::XMFLOAT4 COLOR_LOSE     { 1.00f, 0.60f, 0.60f, 1.0f };  // 敗北行
    const DirectX::XMFLOAT4 COLOR_HINT     { 0.60f, 0.65f, 0.75f, 1.0f };  // 補助文字
    const DirectX::XMFLOAT4 COLOR_BTN_BG   { 0.12f, 0.13f, 0.20f, 0.96f };  // ボタン本体
    const DirectX::XMFLOAT4 COLOR_BTN_HOVER{ 0.24f, 0.28f, 0.40f, 0.98f };  // ボタン(ホバー)
    const DirectX::XMFLOAT4 COLOR_BTN_EDGE { 0.55f, 0.85f, 1.00f, 1.0f };  // ボタン縁取り

    // ボタン寸法
    constexpr float BTN_W = 200.0f;
    constexpr float BTN_H = 50.0f;

    std::vector<RunResult> g_History;   // 読み込んだ戦績(新しい順)
    double g_Scroll    = 0.0;           // 縦スクロール量(px)
    int    g_WheelPrev = 0;             // 前フレームのホイール累積値

    /** @brief 「戻る」ボタン領域 (右下) */
    void BackBtnRect(float screenW, float screenH, float& x, float& y)
    {
        x = screenW - BTN_W - 40.0f;
        y = screenH - BTN_H - 24.0f;
    }
    /** @brief 座標が戻るボタン内か */
    bool BackBtnContains(float screenW, float screenH, int mx, int my)
    {
        float bx, by;
        BackBtnRect(screenW, screenH, bx, by);
        return mx >= bx && mx <= bx + BTN_W && my >= by && my <= by + BTN_H;
    }

    /**
     * @brief  1ラン分の戦績を1行テキストへ整形する
     * @param  r   戦績
     * @param  out [out] 整形後の文字列
     */
    void FormatRow(const RunResult& r, char* out, size_t outSize)
    {
        const int sec = static_cast<int>(r.timeSeconds);
        char resultPart[64];
        if (r.cleared)
            std::snprintf(resultPart, sizeof(resultPart), "クリア");
        else if (!r.defeatedByBoss.empty())
            std::snprintf(resultPart, sizeof(resultPart), "敗北(%s)",
                          r.defeatedByBoss.c_str());
        else
            std::snprintf(resultPart, sizeof(resultPart), "敗北");

        std::snprintf(out, outSize,
            "%s  %s  %d/%d体  %d:%02d  %d手  アビ%d種",
            r.dateTime.c_str(), resultPart,
            r.bossesDefeated, r.bossTotal,
            sec / 60, sec % 60, r.turns,
            static_cast<int>(r.abilityNames.size()));
    }
}

//======================================
// 初期化／終了
//======================================
void History_Initialize()
{
    Prim::Initialize();
    Text::Initialize();
    g_History   = RunState::LoadHistory();
    g_Scroll    = 0.0;
    g_WheelPrev = InputManager_GetMouseState().scrollWheelValue;

    // 戦績はタイトルのサブ画面: タイトルBGMを継続させる (PlayBGMは同一曲なら無音)
    SoundManager_PlayBGM(SOUND_BGM_TITLE);
}

void History_Finalize()
{
    g_History.clear();
}

//======================================
// 更新
//======================================
void History_Update(double /*elapsed_time*/)
{
    const float screenW = static_cast<float>(Direct3D_GetBackBufferWidth());
    const float screenH = static_cast<float>(Direct3D_GetBackBufferHeight());

    /*--- ホイールスクロール (範囲内へクランプ) ---*/
    const int wheel = InputManager_GetMouseState().scrollWheelValue;
    const int delta = wheel - g_WheelPrev;
    g_WheelPrev = wheel;
    g_Scroll -= delta * SCROLL_SPEED;

    const float viewH    = screenH - LIST_TOP - LIST_BOTTOM;
    const float contentH = g_History.size() * ROW_H;
    const float maxScroll = (contentH > viewH) ? (contentH - viewH) : 0.0f;
    if (g_Scroll < 0.0)        g_Scroll = 0.0;
    if (g_Scroll > maxScroll)  g_Scroll = maxScroll;

    /*--- 戻る (タイトルへ) ---*/
    if (InputManager_IsMouseLeftTrigger())
    {
        const auto& mouse = InputManager_GetMouseState();
        if (BackBtnContains(screenW, screenH, mouse.x, mouse.y))
        {
            SoundManager_PlaySE(SOUND_SE_CANCEL);
            Scene_Change(Scene::TITLE);
        }
    }
}

//======================================
// 描画
//======================================
void History_Draw()
{
    const float screenW = static_cast<float>(Direct3D_GetBackBufferWidth());
    const float screenH = static_cast<float>(Direct3D_GetBackBufferHeight());

    // 背景
    Prim::DrawRect(0, 0, screenW, screenH, COLOR_BG);

    /*--- 見出し ---*/
    const char* head = "戦績ログ";
    const float hw = Text::MeasureWidth(head, TEXT_HEAD);
    Text::Draw((screenW - hw) * 0.5f, 50.0f, head, TEXT_HEAD, COLOR_HEAD);

    /*--- 履歴一覧 (スクロール + 表示域クリップ) ---*/
    const float viewTop    = LIST_TOP;
    const float viewBottom = screenH - LIST_BOTTOM;

    if (g_History.empty())
    {
        const char* none = "まだ戦績がありません";
        const float nw = Text::MeasureWidth(none, TEXT_ROW);
        Text::Draw((screenW - nw) * 0.5f, viewTop + 20.0f,
                   none, TEXT_ROW, COLOR_HINT);
    }
    else
    {
        for (size_t i = 0; i < g_History.size(); ++i)
        {
            const float y = viewTop + i * ROW_H - static_cast<float>(g_Scroll);
            // 表示域外の行は描かない (簡易クリップ)
            if (y + ROW_H < viewTop || y > viewBottom) continue;

            char row[160];
            FormatRow(g_History[i], row, sizeof(row));
            const DirectX::XMFLOAT4 col = g_History[i].cleared ? COLOR_WIN : COLOR_LOSE;
            Text::Draw(LIST_X, y, row, TEXT_ROW, col);
        }
    }

    /*--- スクロールヒント ---*/
    const float contentH = g_History.size() * ROW_H;
    if (contentH > (viewBottom - viewTop))
    {
        const char* hint = "ホイールでスクロール";
        Text::Draw(LIST_X, screenH - LIST_BOTTOM + 8.0f,
                   hint, 16.0f, COLOR_HINT);
    }

    /*--- 戻るボタン ---*/
    float bx, by;
    BackBtnRect(screenW, screenH, bx, by);
    const auto& mouse = InputManager_GetMouseState();
    const bool hover = BackBtnContains(screenW, screenH, mouse.x, mouse.y);
    Prim::DrawRect(bx - 3.0f, by - 3.0f, BTN_W + 6.0f, BTN_H + 6.0f, COLOR_BTN_EDGE);
    Prim::DrawRect(bx, by, BTN_W, BTN_H, hover ? COLOR_BTN_HOVER : COLOR_BTN_BG);
    const char* back = "戻る";
    const float tw = Text::MeasureWidth(back, TEXT_BTN);
    Text::Draw(bx + (BTN_W - tw) * 0.5f, by + (BTN_H - TEXT_BTN) * 0.5f,
               back, TEXT_BTN, COLOR_HEAD);
}
