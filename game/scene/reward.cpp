/****************************************
 * @file   reward.cpp
 * @brief  報酬シーン実装
 * @author Natsume Shidara
 * @date   2026/05/15
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
#include "sound_manager.h"

#include <DirectXMath.h>
#include <cstdio>

//--------------------------------------
// 定数・内部状態 (無名名前空間)
//--------------------------------------
namespace
{
    // カードの見た目に関する寸法・文字サイズ
    constexpr float CARD_W       = 280.0f;   // カード幅
    constexpr float CARD_H       = 360.0f;   // カード高さ
    constexpr float CARD_GAP     = 40.0f;    // カード間の余白
    constexpr float CARD_BORDER  = 4.0f;     // 縁取りの太さ
    constexpr float TEXT_TITLE   = 28.0f;    // アビリティ名
    constexpr float TEXT_RARITY  = 16.0f;    // レアリティ表記
    constexpr float TEXT_DESC    = 16.0f;    // 説明文
    constexpr float TEXT_HEADER  = 36.0f;    // 見出し
    constexpr float TEXT_HINT    = 18.0f;    // 操作ヒント

    // 配色 (RGBA)
    const DirectX::XMFLOAT4 COLOR_BG         { 0.04f, 0.04f, 0.08f, 1.0f };  // 背景
    const DirectX::XMFLOAT4 COLOR_CARD_BG    { 0.10f, 0.10f, 0.15f, 1.0f };  // カード本体
    const DirectX::XMFLOAT4 COLOR_CARD_HOVER { 0.18f, 0.18f, 0.26f, 1.0f };  // ホバー時の本体
    const DirectX::XMFLOAT4 COLOR_TEXT       { 0.90f, 0.92f, 0.98f, 1.0f };  // 標準文字
    const DirectX::XMFLOAT4 COLOR_HINT       { 0.55f, 0.60f, 0.70f, 1.0f };  // 補助文字
    const DirectX::XMFLOAT4 COLOR_HEADER     { 0.60f, 1.00f, 0.60f, 1.0f };  // 見出し文字

    /**
     * @brief  レアリティに対応する表示色を返す
     * @param  r レアリティ
     * @return 縁取り・レアリティ表記に使う色
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
     * @return レアリティ名文字列
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

    /**
     * @brief  index 番目のカードの左端X座標を算出する
     * @param  index   カード番号 (0始まり)
     * @param  screenW 画面幅
     * @param  total   カード総数
     * @return カード左端のX座標 (カード群は画面中央に配置)
     */
    float CardOriginX(int index, float screenW, int total)
    {
        const float totalW = total * CARD_W + (total - 1) * CARD_GAP;
        const float startX = (screenW - totalW) * 0.5f;
        return startX + index * (CARD_W + CARD_GAP);
    }

    /**
     * @brief  カードの上端Y座標を算出する
     * @param  screenH 画面高さ
     * @return カード上端のY座標 (画面中央やや下寄り)
     */
    float CardOriginY(float screenH)
    {
        return (screenH - CARD_H) * 0.5f + 20.0f;
    }

    int g_HoverIndex = -1;   // マウスが乗っているカード番号 (-1 = なし)
}

//======================================
// 初期化／終了
//======================================
void Reward_Initialize()
{
    Prim::Initialize();
    Text::Initialize();

    // 報酬候補が未生成なら生成する
    if (RunState::RewardChoices().empty())
    {
        RunState::GenerateRewardChoices();
    }
    g_HoverIndex = -1;

    // 報酬選択中も勝利テーマを流し続ける
    // (決着時に再生済みなら PlayBGM は鳴り直さず継続。次の対戦開始でBGMへ戻る)
    SoundManager_PlayBGM(SOUND_BGM_WIN);
}

void Reward_Finalize()
{
}

//======================================
// 更新
//======================================
void Reward_Update(double /*elapsed_time*/)
{
    const auto& choices = RunState::RewardChoices();
    if (choices.empty()) return;

    const float screenW = static_cast<float>(Direct3D_GetBackBufferWidth());
    const float screenH = static_cast<float>(Direct3D_GetBackBufferHeight());
    const int   total   = static_cast<int>(choices.size());
    const auto& mouse   = InputManager_GetMouseState();
    const float oy      = CardOriginY(screenH);

    /*--- マウスが乗っているカードを判定 ---*/
    const int prevHover = g_HoverIndex;
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
    // 別カードへホバーが移った瞬間だけカーソル音 (毎フレーム鳴らさない)
    if (g_HoverIndex >= 0 && g_HoverIndex != prevHover)
    {
        SoundManager_PlaySE(SOUND_SE_SELECT);
    }

    /*--- 左クリックで選択を確定 → プレイヤー所持に追加 ---*/
    if (InputManager_IsMouseLeftTrigger() && g_HoverIndex >= 0)
    {
        SoundManager_PlaySE(SOUND_SE_DECIDE);
        RunState::PlayerAbilities().push_back(choices[g_HoverIndex]);
        RunState::RewardChoices().clear();

        // 二重防御: 万一ここで完走状態なら、次戦へは進めずタイトルへ。
        // (通常はゲームシーン側で完走を先に検出するため到達しない)
        if (RunState::IsRunComplete())
        {
            RunState::MarkRunCleared();
            RunState::ClearSave();
            Scene_Change(Scene::TITLE);
        }
        else
        {
            // 勝ち進行+選んだ報酬をこの時点で保存しておく。
            // (次戦の ResetMatch まで保存を遅らせると、報酬画面中の終了で
            //  撃破済みボスの再戦・報酬喪失が起きるため)
            RunState::SaveRun();
            Scene_Change(Scene::GAME);
        }
        return;
    }

    /*--- ESC でタイトルへ戻る (ラン中断扱い) ---*/
    if (Keyboard_IsKeyDown(KK_ESCAPE))
    {
        SoundManager_PlaySE(SOUND_SE_CANCEL);
        RunState::RewardChoices().clear();
        Scene_Change(Scene::TITLE);
    }
}

//======================================
// 描画
//======================================
void Reward_Draw()
{
    const float screenW = static_cast<float>(Direct3D_GetBackBufferWidth());
    const float screenH = static_cast<float>(Direct3D_GetBackBufferHeight());

    // 背景を塗りつぶす
    Prim::DrawRect(0, 0, screenW, screenH, COLOR_BG);

    /*--- 見出しを上部中央に描画 ---*/
    const char* header = "報酬を選択";
    const float hw = Text::MeasureWidth(header, TEXT_HEADER);
    Text::Draw((screenW - hw) * 0.5f, 36.0f, header, TEXT_HEADER, COLOR_HEADER);

    const auto& choices = RunState::RewardChoices();
    const int total = static_cast<int>(choices.size());
    const float oy = CardOriginY(screenH);

    /*--- 報酬カードを1枚ずつ描画 ---*/
    for (int i = 0; i < total; ++i)
    {
        const auto& ability = choices[i];
        const float ox = CardOriginX(i, screenW, total);
        const auto rarityColor = RarityColor(ability->rarity);

        // 縁取り (レアリティ色)
        Prim::DrawRect(ox - CARD_BORDER, oy - CARD_BORDER,
                       CARD_W + CARD_BORDER * 2, CARD_H + CARD_BORDER * 2,
                       rarityColor);
        // 本体 (ホバー中は明るい色)
        const auto& bodyColor = (i == g_HoverIndex) ? COLOR_CARD_HOVER : COLOR_CARD_BG;
        Prim::DrawRect(ox, oy, CARD_W, CARD_H, bodyColor);

        // レアリティ表記 (カード上部中央)
        const char* rarText = RarityName(ability->rarity);
        const float rw = Text::MeasureWidth(rarText, TEXT_RARITY);
        Text::Draw(ox + (CARD_W - rw) * 0.5f, oy + 18.0f,
                   rarText, TEXT_RARITY, rarityColor);

        // アビリティ名 (中央寄せ)
        const float nw = Text::MeasureWidth(ability->name.c_str(), TEXT_TITLE);
        Text::Draw(ox + (CARD_W - nw) * 0.5f, oy + 60.0f,
                   ability->name.c_str(), TEXT_TITLE, COLOR_TEXT);

        // 説明文 (改行は description 内の \n で明示済み)
        Text::Draw(ox + 16.0f, oy + 130.0f, ability->description.c_str(),
                   TEXT_DESC, COLOR_HINT);
    }

    /*--- 操作ヒントを下部中央に描画 ---*/
    const char* hint = "カードをクリックで選択    [ESC] ラン中断";
    const float hwh = Text::MeasureWidth(hint, TEXT_HINT);
    Text::Draw((screenW - hwh) * 0.5f, screenH - 40.0f,
               hint, TEXT_HINT, COLOR_HINT);
}
