/****************************************
 * @file   boss_reward.cpp
 * @brief  ボス撃破スキル獲得シーンの実装
 * @author Natsume Shidara
 * @date   2026/06/24
 *
 * 撃破したボスの固有スキルを提示し、はい/いいえ で獲得可否を選ばせる。
 * 「はい」で所持へ加え、「いいえ」で見送り、いずれも通常抽選(REWARD)へ進む。
 ****************************************/
#include "boss_reward.h"
#include "scene.h"

#include "run_state.h"
#include "render_primitives.h"
#include "text_draw.h"
#include "ability/ability.h"

#include "direct3d.h"
#include "input_manager.h"
#include "keyboard.h"
#include "key_logger.h"
#include "sound_manager.h"

#include <DirectXMath.h>
#include <cstdio>
#include <string>

//--------------------------------------
// 定数・内部状態 (無名名前空間)
//--------------------------------------
namespace
{
    // 文字サイズ
    constexpr float TEXT_HEADER = 40.0f;   // 「○○」を撃破！
    constexpr float TEXT_QUERY  = 26.0f;   // スキル「△△」を獲得しますか？
    constexpr float TEXT_NAME   = 32.0f;   // スキル名 (枠内)
    constexpr float TEXT_RARITY = 16.0f;   // レアリティ
    constexpr float TEXT_DESC   = 18.0f;   // 説明文
    constexpr float TEXT_BTN    = 24.0f;   // ボタン文字
    constexpr float TEXT_HINT   = 18.0f;   // 操作ヒント

    // スキル枠 (中央パネル)
    constexpr float BOX_W      = 540.0f;
    constexpr float BOX_H      = 168.0f;
    constexpr float BOX_BORDER = 4.0f;

    // ボタン
    constexpr float BTN_W   = 220.0f;
    constexpr float BTN_H   = 60.0f;
    constexpr float BTN_GAP = 60.0f;

    // 配色 (RGBA)
    const DirectX::XMFLOAT4 COLOR_BG       { 0.04f, 0.04f, 0.08f, 1.0f };  // 背景
    const DirectX::XMFLOAT4 COLOR_HEADER   { 0.60f, 1.00f, 0.60f, 1.0f };  // 撃破見出し(緑)
    const DirectX::XMFLOAT4 COLOR_TEXT     { 0.90f, 0.92f, 0.98f, 1.0f };  // 標準文字
    const DirectX::XMFLOAT4 COLOR_HINT     { 0.55f, 0.60f, 0.70f, 1.0f };  // 補助文字
    const DirectX::XMFLOAT4 COLOR_BOX_BG   { 0.10f, 0.10f, 0.15f, 1.0f };  // スキル枠本体
    const DirectX::XMFLOAT4 COLOR_BTN_BG   { 0.12f, 0.13f, 0.20f, 1.0f };  // ボタン本体
    const DirectX::XMFLOAT4 COLOR_BTN_HOV  { 0.24f, 0.28f, 0.40f, 1.0f };  // ボタン(ホバー)
    const DirectX::XMFLOAT4 COLOR_YES      { 0.55f, 1.00f, 0.65f, 1.0f };  // はい(緑)
    const DirectX::XMFLOAT4 COLOR_NO       { 0.85f, 0.55f, 0.55f, 1.0f };  // いいえ(赤寄り)

    /** @brief レアリティに対応する表示色 */
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

    /** @brief レアリティに対応する日本語名 */
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

    //--------------------------------------
    // ボタンのレイアウト (Update/Draw 共有)
    //--------------------------------------
    struct Btn { float x, y, w, h; };

    bool BtnContains(const Btn& b, int mx, int my)
    {
        return mx >= b.x && mx <= b.x + b.w && my >= b.y && my <= b.y + b.h;
    }

    /** @brief 「はい」ボタン領域 (左) */
    Btn YesBtn(float screenW, float screenH)
    {
        const float totalW = BTN_W * 2.0f + BTN_GAP;
        const float x0 = (screenW - totalW) * 0.5f;
        return { x0, screenH * 0.74f, BTN_W, BTN_H };
    }
    /** @brief 「いいえ」ボタン領域 (右) */
    Btn NoBtn(float screenW, float screenH)
    {
        const float totalW = BTN_W * 2.0f + BTN_GAP;
        const float x0 = (screenW - totalW) * 0.5f;
        return { x0 + BTN_W + BTN_GAP, screenH * 0.74f, BTN_W, BTN_H };
    }

    /** @brief 縁取り付きボタンを描画 (中央ラベル) */
    void DrawBtn(const Btn& b, const char* label, bool hover,
                 const DirectX::XMFLOAT4& accent)
    {
        Prim::DrawRect(b.x - 3.0f, b.y - 3.0f, b.w + 6.0f, b.h + 6.0f, accent);
        Prim::DrawRect(b.x, b.y, b.w, b.h, hover ? COLOR_BTN_HOV : COLOR_BTN_BG);
        const float tw = Text::MeasureWidth(label, TEXT_BTN);
        Text::Draw(b.x + (b.w - tw) * 0.5f, b.y + (b.h - TEXT_BTN) * 0.5f,
                   label, TEXT_BTN, COLOR_TEXT);
    }

    int g_HoverBtn = -1;   // ホバー中ボタン: 0=左, 1=右, -1=なし

    /**
     * @enum  Phase
     * @brief 画面の段階
     * @detail Offer: 獲得しますか？(はい/いいえ)。
     *         ConfirmDecline: 「いいえ」の誤操作防止確認(受け取らない/戻る)。
     */
    enum class Phase { Offer, ConfirmDecline };
    Phase g_Phase = Phase::Offer;

    /** @brief 選択を確定し、通常抽選(REWARD)へ進む */
    void ResolveAndProceed(bool accept)
    {
        if (accept) RunState::AcceptPendingBossReward();
        else        RunState::DeclinePendingBossReward();
        // 通常抽選の候補は REWARD シーンの初期化で生成される
        Scene_Change(Scene::REWARD);
    }
}

//======================================
// 初期化／終了
//======================================
void BossReward_Initialize()
{
    Prim::Initialize();
    Text::Initialize();
    g_HoverBtn = -1;
    g_Phase    = Phase::Offer;

    // 保留中スキルが無い異常時は、空振りせず通常抽選へ素通りさせる
    if (!RunState::PendingBossReward())
    {
        Scene_Change(Scene::REWARD);
        return;
    }

    // 勝利後の報酬の流れなので、勝利テーマをそのまま継続させる
    // (決着時に再生済みなら PlayBGM は再スタートせず鳴り続ける)
    SoundManager_PlayBGM(SOUND_BGM_WIN);
}

void BossReward_Finalize()
{
}

//======================================
// 更新
//======================================
void BossReward_Update(double /*elapsed_time*/)
{
    if (!RunState::PendingBossReward()) return;   // Initialize で遷移予約済み

    const float screenW = static_cast<float>(Direct3D_GetBackBufferWidth());
    const float screenH = static_cast<float>(Direct3D_GetBackBufferHeight());
    const auto& mouse   = InputManager_GetMouseState();

    // ボタンは左右2つで両フェーズ共通の位置 (ラベル/意味はフェーズで変わる)
    const Btn bLeft  = YesBtn(screenW, screenH);
    const Btn bRight = NoBtn(screenW, screenH);

    /*--- ホバー判定 (移った瞬間だけカーソル音) ---*/
    const int prevHover = g_HoverBtn;
    g_HoverBtn = BtnContains(bLeft,  mouse.x, mouse.y) ? 0
               : BtnContains(bRight, mouse.x, mouse.y) ? 1 : -1;
    if (g_HoverBtn >= 0 && g_HoverBtn != prevHover)
    {
        SoundManager_PlaySE(SOUND_SE_SELECT);
    }

    /*--- 左クリックで確定 (フェーズごとに意味が異なる) ---*/
    if (InputManager_IsMouseLeftTrigger() && g_HoverBtn >= 0)
    {
        SoundManager_PlaySE(SOUND_SE_DECIDE);
        if (g_Phase == Phase::Offer)
        {
            if (g_HoverBtn == 0) ResolveAndProceed(true);     // 「はい」→ 獲得して抽選へ
            else { g_Phase = Phase::ConfirmDecline; g_HoverBtn = -1; }  // 「いいえ」→ 確認へ
        }
        else // ConfirmDecline
        {
            if (g_HoverBtn == 0) ResolveAndProceed(false);    // 「受け取らない」→ 見送って抽選へ
            else { g_Phase = Phase::Offer; g_HoverBtn = -1; } // 「戻る」→ 提示画面へ戻す
        }
        return;
    }

    /*--- ESC: 確認中は提示へ戻す。提示中はラン中断(タイトルへ) ---*/
    if (KeyLogger_IsTrigger(KK_ESCAPE))
    {
        SoundManager_PlaySE(SOUND_SE_CANCEL);
        if (g_Phase == Phase::ConfirmDecline)
        {
            g_Phase    = Phase::Offer;   // 確認をやめて提示へ戻る (誤操作の取り消し)
            g_HoverBtn = -1;
        }
        else
        {
            RunState::DeclinePendingBossReward();   // 保留中スキルは破棄
            Scene_Change(Scene::TITLE);
        }
    }
}

//======================================
// 描画
//======================================
void BossReward_Draw()
{
    const float screenW = static_cast<float>(Direct3D_GetBackBufferWidth());
    const float screenH = static_cast<float>(Direct3D_GetBackBufferHeight());

    // 背景
    Prim::DrawRect(0, 0, screenW, screenH, COLOR_BG);

    const auto& skill = RunState::PendingBossReward();
    if (!skill) return;   // 遷移予約済み (描く対象なし)

    /*--- 撃破見出し 「○○」を撃破！ ---*/
    char headBuf[128];
    std::snprintf(headBuf, sizeof(headBuf), "「%s」を撃破！",
                  RunState::PendingBossName().c_str());
    const float hw = Text::MeasureWidth(headBuf, TEXT_HEADER);
    Text::Draw((screenW - hw) * 0.5f, screenH * 0.16f, headBuf, TEXT_HEADER, COLOR_HEADER);

    /*--- ボスのギミック説明 (フレーバー) ---*/
    const std::string& bossDesc = RunState::PendingBossDesc();
    if (!bossDesc.empty())
    {
        const float bw = Text::MeasureWidth(bossDesc.c_str(), TEXT_DESC);
        Text::Draw((screenW - bw) * 0.5f, screenH * 0.16f + TEXT_HEADER + 12.0f,
                   bossDesc.c_str(), TEXT_DESC, COLOR_HINT);
    }

    /*--- 中段の一文 (フェーズで変化) ---*/
    char midBuf[160];
    DirectX::XMFLOAT4 midColor;
    if (g_Phase == Phase::Offer)
    {
        std::snprintf(midBuf, sizeof(midBuf), "スキル「%s」を獲得しますか？",
                      skill->name.c_str());
        midColor = COLOR_TEXT;
    }
    else
    {
        // 「いいえ」の誤操作防止確認
        std::snprintf(midBuf, sizeof(midBuf), "受け取らなくてよろしいですね？");
        midColor = COLOR_NO;
    }
    const float qw = Text::MeasureWidth(midBuf, TEXT_QUERY);
    const float queryY = screenH * 0.32f;
    Text::Draw((screenW - qw) * 0.5f, queryY, midBuf, TEXT_QUERY, midColor);

    /*--- スキル枠 (レアリティ色の縁取り。両フェーズ共通で何が懸かるか見せる) ---*/
    const auto rarityCol = RarityColor(skill->rarity);
    const float boxX = (screenW - BOX_W) * 0.5f;
    const float boxY = queryY + TEXT_QUERY + 20.0f;
    Prim::DrawRect(boxX - BOX_BORDER, boxY - BOX_BORDER,
                   BOX_W + BOX_BORDER * 2, BOX_H + BOX_BORDER * 2, rarityCol);
    Prim::DrawRect(boxX, boxY, BOX_W, BOX_H, COLOR_BOX_BG);

    // レアリティ表記 (枠上部中央)
    const char* rar = RarityName(skill->rarity);
    const float rw = Text::MeasureWidth(rar, TEXT_RARITY);
    Text::Draw(boxX + (BOX_W - rw) * 0.5f, boxY + 16.0f, rar, TEXT_RARITY, rarityCol);

    // スキル名 (中央寄せ)
    const float nw = Text::MeasureWidth(skill->name.c_str(), TEXT_NAME);
    Text::Draw(boxX + (BOX_W - nw) * 0.5f, boxY + 44.0f,
               skill->name.c_str(), TEXT_NAME, COLOR_TEXT);

    // スキル説明 (中央寄せ。description 内の \n で改行)
    const float dw = Text::MeasureWidth(skill->description.c_str(), TEXT_DESC);
    Text::Draw(boxX + (BOX_W - dw) * 0.5f, boxY + 96.0f,
               skill->description.c_str(), TEXT_DESC, COLOR_HINT);

    /*--- ボタン + 操作ヒント (フェーズで変化) ---*/
    const Btn bLeft  = YesBtn(screenW, screenH);
    const Btn bRight = NoBtn(screenW, screenH);
    const char* hint = "";
    if (g_Phase == Phase::Offer)
    {
        DrawBtn(bLeft,  "はい",   g_HoverBtn == 0, COLOR_YES);
        DrawBtn(bRight, "いいえ", g_HoverBtn == 1, COLOR_NO);
        hint = "はい: スキルを獲得    いいえ: 見送る    → どちらも報酬抽選へ進みます";
    }
    else
    {
        DrawBtn(bLeft,  "受け取らない", g_HoverBtn == 0, COLOR_NO);
        DrawBtn(bRight, "戻る",         g_HoverBtn == 1, COLOR_YES);
        hint = "受け取らない: 見送って抽選へ    戻る: 選び直す";
    }
    const float hwh = Text::MeasureWidth(hint, TEXT_HINT);
    Text::Draw((screenW - hwh) * 0.5f, screenH - 40.0f, hint, TEXT_HINT, COLOR_HINT);
}
