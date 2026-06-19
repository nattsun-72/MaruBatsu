/****************************************
 * @file   result.cpp
 * @brief  リザルトシーン実装
 * @author Natsume Shidara
 * @date   2026/06/05
 *
 * RunState::LastResult() の戦績を表示する。ボタンで「もう一度挑戦」
 * (新規ラン) / 「タイトルへ」を選べる。
 ****************************************/

#include "result.h"
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
    constexpr float TEXT_BIG   = 56.0f;   // 結果見出し
    constexpr float TEXT_STAT  = 24.0f;   // 戦績行
    constexpr float TEXT_ABIL  = 20.0f;   // アビリティ行
    constexpr float TEXT_BTN   = 22.0f;   // ボタン文字

    // ボタン寸法
    constexpr float BTN_W   = 240.0f;
    constexpr float BTN_H   = 56.0f;
    constexpr float BTN_GAP = 40.0f;

    // 配色 (RGBA)
    const DirectX::XMFLOAT4 COLOR_BG       { 0.05f, 0.05f, 0.09f, 1.0f };  // 背景
    const DirectX::XMFLOAT4 COLOR_WIN      { 0.60f, 1.00f, 0.60f, 1.0f };  // クリア
    const DirectX::XMFLOAT4 COLOR_LOSE     { 1.00f, 0.50f, 0.50f, 1.0f };  // 敗北
    const DirectX::XMFLOAT4 COLOR_TEXT     { 0.88f, 0.92f, 0.98f, 1.0f };  // 標準文字
    const DirectX::XMFLOAT4 COLOR_HINT     { 0.60f, 0.65f, 0.75f, 1.0f };  // 補助文字
    const DirectX::XMFLOAT4 COLOR_BTN_BG   { 0.12f, 0.13f, 0.20f, 0.96f };  // ボタン本体
    const DirectX::XMFLOAT4 COLOR_BTN_HOVER{ 0.24f, 0.28f, 0.40f, 0.98f };  // ボタン(ホバー)
    const DirectX::XMFLOAT4 COLOR_BTN_EDGE { 0.55f, 0.85f, 1.00f, 1.0f };  // ボタン縁取り

    //--------------------------------------
    // ボタン (Update/Draw 共有)
    //--------------------------------------
    /** @brief 矩形ボタン */
    struct Btn { float x, y, w, h; };

    /** @brief 「もう一度挑戦」ボタン領域 (左) */
    Btn RetryBtn(float screenW, float screenH)
    {
        const float totalW = BTN_W * 2.0f + BTN_GAP;
        const float x0 = (screenW - totalW) * 0.5f;
        return { x0, screenH - 110.0f, BTN_W, BTN_H };
    }
    /** @brief 「タイトルへ」ボタン領域 (右) */
    Btn TitleBtn(float screenW, float screenH)
    {
        const float totalW = BTN_W * 2.0f + BTN_GAP;
        const float x0 = (screenW - totalW) * 0.5f;
        return { x0 + BTN_W + BTN_GAP, screenH - 110.0f, BTN_W, BTN_H };
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
        const float tw = Text::MeasureWidth(label, TEXT_BTN);
        Text::Draw(b.x + (b.w - tw) * 0.5f, b.y + (b.h - TEXT_BTN) * 0.5f,
                   label, TEXT_BTN, COLOR_TEXT);
    }

    /**
     * @struct AbilGroup
     * @brief  リザルト表示用に同名アビリティをまとめた単位
     */
    struct AbilGroup { std::string name; int count; };

    /**
     * @brief  アビリティ名一覧を、出現順を保ったまま同名でまとめる
     * @param  names アビリティ名一覧 (取得順)
     * @return まとめたグループ一覧
     */
    std::vector<AbilGroup> GroupNames(const std::vector<std::string>& names)
    {
        std::vector<AbilGroup> groups;
        for (const auto& n : names)
        {
            bool found = false;
            for (auto& g : groups)
            {
                if (g.name == n) { ++g.count; found = true; break; }
            }
            if (!found) groups.push_back({ n, 1 });
        }
        return groups;
    }
}

//======================================
// 初期化／終了
//======================================
void Result_Initialize()
{
    Prim::Initialize();
    Text::Initialize();

    // リザルトBGM (素材未配置なら無音)
    SoundManager_PlayBGM(SOUND_BGM_RESULT);
}

void Result_Finalize()
{
}

//======================================
// 更新
//======================================
void Result_Update(double /*elapsed_time*/)
{
    const float screenW = static_cast<float>(Direct3D_GetBackBufferWidth());
    const float screenH = static_cast<float>(Direct3D_GetBackBufferHeight());

    if (!InputManager_IsMouseLeftTrigger()) return;
    const auto& mouse = InputManager_GetMouseState();

    // 「もう一度挑戦」: ラン状態を初期化して本編へ
    if (BtnContains(RetryBtn(screenW, screenH), mouse.x, mouse.y))
    {
        SoundManager_PlaySE(SOUND_SE_DECIDE);
        RunState::ResetRun();
        Scene_Change(Scene::GAME);
        return;
    }
    // 「タイトルへ」: タイトル画面へ
    if (BtnContains(TitleBtn(screenW, screenH), mouse.x, mouse.y))
    {
        SoundManager_PlaySE(SOUND_SE_DECIDE);
        Scene_Change(Scene::TITLE);
        return;
    }
}

//======================================
// 描画
//======================================
void Result_Draw()
{
    const float screenW = static_cast<float>(Direct3D_GetBackBufferWidth());
    const float screenH = static_cast<float>(Direct3D_GetBackBufferHeight());
    const RunResult& r = RunState::LastResult();

    // 背景
    Prim::DrawRect(0, 0, screenW, screenH, COLOR_BG);

    /*--- 結果見出し (クリア / 引き分け / 敗北 を区別) ---*/
    const char* head = r.cleared ? "ラン クリア！"
                                 : (r.wasDraw ? "ラン 引き分け" : "ラン 敗北");
    const DirectX::XMFLOAT4 headColor = r.cleared ? COLOR_WIN
                                                  : (r.wasDraw ? COLOR_HINT : COLOR_LOSE);
    const float headW = Text::MeasureWidth(head, TEXT_BIG);
    Text::Draw((screenW - headW) * 0.5f, 60.0f, head, TEXT_BIG, headColor);

    /*--- レジェンダリー初解放の告知 (ラスボス初撃破時) ---*/
    if (r.cleared && r.legendaryUnlocked)
    {
        const char* unlock = "★ レジェンダリー解放！ 以降の報酬に出現する ★";
        const DirectX::XMFLOAT4 gold{ 1.00f, 0.80f, 0.30f, 1.0f };
        const float uw = Text::MeasureWidth(unlock, TEXT_STAT);
        Text::Draw((screenW - uw) * 0.5f, 60.0f + TEXT_BIG + 6.0f,
                   unlock, TEXT_STAT, gold);
    }

    /*--- 戦績ブロック (中央寄せ) ---*/
    char buf[128];
    float y = 170.0f;
    const float lineH = TEXT_STAT + 10.0f;
    auto drawStat = [&](const char* text) {
        const float w = Text::MeasureWidth(text, TEXT_STAT);
        Text::Draw((screenW - w) * 0.5f, y, text, TEXT_STAT, COLOR_TEXT);
        y += lineH;
    };

    std::snprintf(buf, sizeof(buf), "撃破ボス： %d / %d", r.bossesDefeated, r.bossTotal);
    drawStat(buf);

    const int totalSec = static_cast<int>(r.timeSeconds);
    std::snprintf(buf, sizeof(buf), "プレイ時間： %d:%02d", totalSec / 60, totalSec % 60);
    drawStat(buf);

    std::snprintf(buf, sizeof(buf), "総ターン数： %d", r.turns);
    drawStat(buf);

    if (!r.cleared && !r.defeatedByBoss.empty())
    {
        std::snprintf(buf, sizeof(buf), "%s： %s",
                      r.wasDraw ? "引き分け" : "敗北", r.defeatedByBoss.c_str());
        drawStat(buf);
    }
    if (!r.dateTime.empty())
    {
        std::snprintf(buf, sizeof(buf), "日時： %s", r.dateTime.c_str());
        drawStat(buf);
    }

    /*--- 取得アビリティ一覧 ---*/
    y += 6.0f;
    const char* abilHead = "取得アビリティ";
    const float ahW = Text::MeasureWidth(abilHead, TEXT_STAT);
    Text::Draw((screenW - ahW) * 0.5f, y, abilHead, TEXT_STAT, COLOR_HINT);
    y += TEXT_STAT + 6.0f;

    const auto groups = GroupNames(r.abilityNames);
    if (groups.empty())
    {
        const char* none = "（なし）";
        const float nw = Text::MeasureWidth(none, TEXT_ABIL);
        Text::Draw((screenW - nw) * 0.5f, y, none, TEXT_ABIL, COLOR_HINT);
    }
    else
    {
        for (const auto& g : groups)
        {
            char line[128];
            if (g.count > 1)
                std::snprintf(line, sizeof(line), "・%s ×%d", g.name.c_str(), g.count);
            else
                std::snprintf(line, sizeof(line), "・%s", g.name.c_str());
            const float w = Text::MeasureWidth(line, TEXT_ABIL);
            Text::Draw((screenW - w) * 0.5f, y, line, TEXT_ABIL, COLOR_TEXT);
            y += TEXT_ABIL + 4.0f;
        }
    }

    /*--- 操作ボタン ---*/
    const auto& mouse = InputManager_GetMouseState();
    const Btn retry = RetryBtn(screenW, screenH);
    const Btn title = TitleBtn(screenW, screenH);
    DrawBtn(retry, "もう一度挑戦", BtnContains(retry, mouse.x, mouse.y));
    DrawBtn(title, "タイトルへ",   BtnContains(title, mouse.x, mouse.y));
}
