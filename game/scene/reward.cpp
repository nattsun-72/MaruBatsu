/****************************************
 * @file reward.cpp
 * @brief 報酬シーン実装
 * @author Natsume Shidara
 * @date 2026/05/15
 *
 * ボス撃破後に呼ばれ、3枚のカードからアビリティを1つ選ばせる。
 * 選択完了で Scene::GAME へ遷移し、次のボス戦開始。
 ****************************************/

#include "reward.h"
#include "scene.h"

#include "run_state.h"
#include "render_primitives.h"
#include "text_draw.h"
#include "ability/ability.h"

#include "direct3d.h"
#include "input_manager.h"
#include "keyboard.h"

#include <DirectXMath.h>

namespace
{
    // カード見た目
    constexpr float CARD_W       = 280.0f;
    constexpr float CARD_H       = 360.0f;
    constexpr float CARD_GAP     = 40.0f;
    constexpr float CARD_BORDER  = 4.0f;
    constexpr float TEXT_TITLE   = 28.0f;
    constexpr float TEXT_RARITY  = 16.0f;
    constexpr float TEXT_DESC    = 16.0f;
    constexpr float TEXT_HEADER  = 36.0f;
    constexpr float TEXT_HINT    = 18.0f;

    const DirectX::XMFLOAT4 COLOR_BG       { 0.04f, 0.04f, 0.08f, 1.0f };
    const DirectX::XMFLOAT4 COLOR_CARD_BG  { 0.10f, 0.10f, 0.15f, 1.0f };
    const DirectX::XMFLOAT4 COLOR_CARD_HOVER { 0.18f, 0.18f, 0.26f, 1.0f };
    const DirectX::XMFLOAT4 COLOR_TEXT     { 0.90f, 0.92f, 0.98f, 1.0f };
    const DirectX::XMFLOAT4 COLOR_HINT     { 0.55f, 0.60f, 0.70f, 1.0f };
    const DirectX::XMFLOAT4 COLOR_HEADER   { 0.60f, 1.00f, 0.60f, 1.0f };

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

    const char* RarityName(Rarity r)
    {
        switch (r)
        {
        case Rarity::Common:    return "COMMON";
        case Rarity::Rare:      return "RARE";
        case Rarity::Epic:      return "EPIC";
        case Rarity::Legendary: return "LEGENDARY";
        case Rarity::Curse:     return "CURSE";
        }
        return "";
    }

    float CardOriginX(int index, float screenW, int total)
    {
        const float totalW = total * CARD_W + (total - 1) * CARD_GAP;
        const float startX = (screenW - totalW) * 0.5f;
        return startX + index * (CARD_W + CARD_GAP);
    }
    float CardOriginY(float screenH)
    {
        return (screenH - CARD_H) * 0.5f + 20.0f;
    }

    int g_HoverIndex = -1;
}

void Reward_Initialize()
{
    Prim::Initialize();
    Text::Initialize();
    if (RunState::RewardChoices().empty())
    {
        RunState::GenerateRewardChoices();
    }
    g_HoverIndex = -1;
}

void Reward_Finalize()
{
}

void Reward_Update(double /*elapsed_time*/)
{
    const auto& choices = RunState::RewardChoices();
    if (choices.empty()) return;

    const float screenW = static_cast<float>(Direct3D_GetBackBufferWidth());
    const float screenH = static_cast<float>(Direct3D_GetBackBufferHeight());
    const int   total   = static_cast<int>(choices.size());
    const auto& mouse   = InputManager_GetMouseState();
    const float oy      = CardOriginY(screenH);

    g_HoverIndex = -1;
    for (int i = 0; i < total; ++i)
    {
        const float ox = CardOriginX(i, screenW, total);
        if (mouse.x >= ox && mouse.x <= ox + CARD_W
         && mouse.y >= oy && mouse.y <= oy + CARD_H)
        {
            g_HoverIndex = i;
            break;
        }
    }

    if (InputManager_IsMouseLeftTrigger() && g_HoverIndex >= 0)
    {
        // 選択を確定 → プレイヤー所持に追加して GAME へ
        RunState::PlayerAbilities().push_back(choices[g_HoverIndex]);
        RunState::RewardChoices().clear();
        Scene_Change(Scene::GAME);
        return;
    }

    if (Keyboard_IsKeyDown(KK_ESCAPE))
    {
        // タイトルに戻る場合はラン中断扱い
        RunState::RewardChoices().clear();
        Scene_Change(Scene::TITLE);
    }
}

void Reward_Draw()
{
    const float screenW = static_cast<float>(Direct3D_GetBackBufferWidth());
    const float screenH = static_cast<float>(Direct3D_GetBackBufferHeight());

    Prim::DrawRect(0, 0, screenW, screenH, COLOR_BG);

    // ヘッダ
    const char* header = "CHOOSE  YOUR  REWARD";
    const float hw = Text::MeasureWidth(header, TEXT_HEADER);
    Text::Draw((screenW - hw) * 0.5f, 60.0f, header, TEXT_HEADER, COLOR_HEADER);

    const auto& choices = RunState::RewardChoices();
    const int total = static_cast<int>(choices.size());
    const float oy = CardOriginY(screenH);

    for (int i = 0; i < total; ++i)
    {
        const auto& ability = choices[i];
        const float ox = CardOriginX(i, screenW, total);
        const auto rarityColor = RarityColor(ability->rarity);

        // 縁取り(レアリティ色)
        Prim::DrawRect(ox - CARD_BORDER, oy - CARD_BORDER,
                       CARD_W + CARD_BORDER * 2, CARD_H + CARD_BORDER * 2,
                       rarityColor);
        // 本体
        const auto& bodyColor = (i == g_HoverIndex) ? COLOR_CARD_HOVER : COLOR_CARD_BG;
        Prim::DrawRect(ox, oy, CARD_W, CARD_H, bodyColor);

        // レアリティ表記
        const char* rarText = RarityName(ability->rarity);
        const float rw = Text::MeasureWidth(rarText, TEXT_RARITY);
        Text::Draw(ox + (CARD_W - rw) * 0.5f, oy + 18.0f,
                   rarText, TEXT_RARITY, rarityColor);

        // 名前
        const float nw = Text::MeasureWidth(ability->name.c_str(), TEXT_TITLE);
        Text::Draw(ox + (CARD_W - nw) * 0.5f, oy + 60.0f,
                   ability->name.c_str(), TEXT_TITLE, COLOR_TEXT);

        // 説明 (簡易な折り返し処理: 26文字で改行)
        const std::string& desc = ability->description;
        constexpr size_t kWrap = 26;
        std::string wrapped;
        wrapped.reserve(desc.size() + desc.size() / kWrap);
        size_t col = 0;
        for (char c : desc)
        {
            if (col >= kWrap && c == ' ')
            {
                wrapped.push_back('\n');
                col = 0;
                continue;
            }
            wrapped.push_back(c);
            if (c == '\n') col = 0;
            else           ++col;
        }
        Text::Draw(ox + 16.0f, oy + 130.0f, wrapped.c_str(),
                   TEXT_DESC, COLOR_HINT);
    }

    // ヒント
    const char* hint = "CLICK A CARD TO CHOOSE    [ESC] ABANDON RUN";
    const float hwh = Text::MeasureWidth(hint, TEXT_HINT);
    Text::Draw((screenW - hwh) * 0.5f, screenH - 40.0f,
               hint, TEXT_HINT, COLOR_HINT);
}
