/****************************************
 * @file   settings_overlay.cpp
 * @brief  設定オーバーレイの実装
 * @author Natsume Shidara
 * @date   2026/06/24
 *
 * 半透明の暗幕 + 中央パネルに、BGM/SE音量スライダーと垂直同期トグル、
 * リセット/閉じるボタンを配置する。値は GameSettings を介して即時反映され、
 * 閉じる時にファイルへ保存される。
 ****************************************/
#include "settings_overlay.h"

#include "scene.h"
#include "game_settings.h"
#include "render_primitives.h"
#include "text_draw.h"

#include "direct3d.h"
#include "input_manager.h"
#include "sound_manager.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace
{
    //--------------------------------------
    // 状態
    //--------------------------------------
    bool g_Open       = false;
    int  g_DragSlider = -1;   // ドラッグ中のスライダー: -1=なし, 0=BGM, 1=SE

    //--------------------------------------
    // 文字サイズ
    //--------------------------------------
    constexpr float TEXT_TITLE = 40.0f;
    constexpr float TEXT_LABEL = 22.0f;
    constexpr float TEXT_VALUE = 20.0f;
    constexpr float TEXT_BTN   = 22.0f;
    constexpr float TEXT_HINT  = 18.0f;

    //--------------------------------------
    // パネル/行レイアウト
    //--------------------------------------
    constexpr float PANEL_W   = 600.0f;
    constexpr float PANEL_H   = 500.0f;   // リタイアボタン(ゲーム中のみ)の領域を含む
    constexpr float PAD       = 48.0f;    // パネル内の左右余白
    constexpr float ROW_H     = 70.0f;    // 1設定行の高さ
    constexpr float FIRST_ROW = 110.0f;   // パネル上端から最初の行中心までのオフセット
    constexpr float LABEL_W   = 190.0f;   // ラベル列の幅
    constexpr float VALUE_W   = 64.0f;    // 値(%)列の幅
    constexpr float KNOB_R    = 12.0f;    // スライダーつまみの半径
    constexpr float TRACK_H   = 8.0f;     // スライダー溝の太さ

    //--------------------------------------
    // 配色 (RGBA) — タイトル/ゲームのミニマル配色に合わせる
    //--------------------------------------
    const DirectX::XMFLOAT4 COLOR_DIM      { 0.00f, 0.00f, 0.04f, 0.62f };  // 背景暗幕
    const DirectX::XMFLOAT4 COLOR_PANEL    { 0.09f, 0.10f, 0.16f, 0.99f };  // パネル本体
    const DirectX::XMFLOAT4 COLOR_EDGE     { 0.55f, 0.85f, 1.00f, 1.00f };  // パネル縁取り(シアン)
    const DirectX::XMFLOAT4 COLOR_TITLE    { 0.90f, 0.95f, 1.00f, 1.00f };  // 見出し
    const DirectX::XMFLOAT4 COLOR_LABEL    { 0.85f, 0.90f, 0.95f, 1.00f };  // 項目名
    const DirectX::XMFLOAT4 COLOR_HINT     { 0.55f, 0.60f, 0.70f, 1.00f };  // 補助文字
    const DirectX::XMFLOAT4 COLOR_TRACK    { 0.20f, 0.22f, 0.30f, 1.00f };  // スライダー溝
    const DirectX::XMFLOAT4 COLOR_FILL     { 0.50f, 0.85f, 1.00f, 1.00f };  // スライダー有効部
    const DirectX::XMFLOAT4 COLOR_KNOB     { 0.92f, 0.96f, 1.00f, 1.00f };  // つまみ
    const DirectX::XMFLOAT4 COLOR_BTN_BG   { 0.14f, 0.16f, 0.24f, 1.00f };  // ボタン本体
    const DirectX::XMFLOAT4 COLOR_BTN_HOV  { 0.26f, 0.30f, 0.42f, 1.00f };  // ボタン(ホバー)
    const DirectX::XMFLOAT4 COLOR_ON       { 0.55f, 1.00f, 0.65f, 1.00f };  // トグルON
    const DirectX::XMFLOAT4 COLOR_OFF      { 0.70f, 0.72f, 0.80f, 1.00f };  // トグルOFF
    const DirectX::XMFLOAT4 COLOR_RETIRE   { 0.90f, 0.50f, 0.45f, 1.00f };  // リタイア(注意色)

    //--------------------------------------
    // 矩形ヘルパ
    //--------------------------------------
    struct Rect { float x, y, w, h; };

    bool Hit(const Rect& r, int mx, int my)
    {
        const float fx = static_cast<float>(mx), fy = static_cast<float>(my);
        return fx >= r.x && fx <= r.x + r.w && fy >= r.y && fy <= r.y + r.h;
    }

    /** @brief 中央寄せのパネル矩形 */
    Rect PanelRect()
    {
        const float sw = static_cast<float>(Direct3D_GetBackBufferWidth());
        const float sh = static_cast<float>(Direct3D_GetBackBufferHeight());
        return { (sw - PANEL_W) * 0.5f, (sh - PANEL_H) * 0.5f, PANEL_W, PANEL_H };
    }

    /** @brief 行 index の中心Y座標 */
    float RowCenterY(const Rect& panel, int index)
    {
        return panel.y + FIRST_ROW + index * ROW_H;
    }

    /** @brief スライダーの溝矩形 (行 index) */
    Rect SliderTrack(const Rect& panel, int index)
    {
        const float trackX = panel.x + PAD + LABEL_W;
        const float trackW = PANEL_W - PAD * 2.0f - LABEL_W - VALUE_W;
        const float cy     = RowCenterY(panel, index);
        return { trackX, cy - TRACK_H * 0.5f, trackW, TRACK_H };
    }

    /** @brief スライダーの当たり判定矩形 (つまみを掴みやすいよう上下に拡張) */
    Rect SliderHit(const Rect& panel, int index)
    {
        const Rect t = SliderTrack(panel, index);
        return { t.x - KNOB_R, t.y - KNOB_R, t.w + KNOB_R * 2.0f, KNOB_R * 2.0f };
    }

    /** @brief 垂直同期トグルのボタン矩形 (行2) */
    Rect VSyncButton(const Rect& panel)
    {
        const float cy = RowCenterY(panel, 2);
        const float w  = 120.0f, h = 40.0f;
        return { panel.x + PANEL_W - PAD - w, cy - h * 0.5f, w, h };
    }

    /** @brief 「タイトルへ戻る」(リタイア) ボタン矩形。ゲーム中のみ表示・操作する。 */
    Rect RetireButton(const Rect& panel)
    {
        const float w = 300.0f, h = 46.0f;
        const float cy = RowCenterY(panel, 3);   // VSync行(2)の下に1段
        return { panel.x + (PANEL_W - w) * 0.5f, cy - h * 0.5f, w, h };
    }

    /** @brief 下部の「デフォルトに戻す」ボタン矩形 */
    Rect ResetButton(const Rect& panel)
    {
        const float w = 200.0f, h = 48.0f;
        return { panel.x + PAD, panel.y + PANEL_H - h - 28.0f, w, h };
    }

    /** @brief 下部の「閉じる」ボタン矩形 */
    Rect CloseButton(const Rect& panel)
    {
        const float w = 200.0f, h = 48.0f;
        return { panel.x + PANEL_W - PAD - w, panel.y + PANEL_H - h - 28.0f, w, h };
    }

    //--------------------------------------
    // 描画ヘルパ
    //--------------------------------------
    /** @brief 縁取り付きボタンを描画 (中央ラベル) */
    void DrawButton(const Rect& r, const char* label, bool hover,
                    const DirectX::XMFLOAT4& accent)
    {
        Prim::DrawRect(r.x - 2.0f, r.y - 2.0f, r.w + 4.0f, r.h + 4.0f, accent);
        Prim::DrawRect(r.x, r.y, r.w, r.h, hover ? COLOR_BTN_HOV : COLOR_BTN_BG);
        const float tw = Text::MeasureWidth(label, TEXT_BTN);
        Text::Draw(r.x + (r.w - tw) * 0.5f, r.y + (r.h - TEXT_BTN) * 0.5f,
                   label, TEXT_BTN, COLOR_LABEL);
    }

    /** @brief 1本のスライダー(ラベル+溝+有効部+つまみ+%表示)を描画 */
    void DrawSlider(const Rect& panel, int index, const char* label, float value)
    {
        const float cy = RowCenterY(panel, index);

        // ラベル (左、縦中央)
        Text::Draw(panel.x + PAD, cy - TEXT_LABEL * 0.5f, label, TEXT_LABEL, COLOR_LABEL);

        const Rect t = SliderTrack(panel, index);
        Prim::DrawRect(t.x, t.y, t.w, t.h, COLOR_TRACK);            // 溝
        Prim::DrawRect(t.x, t.y, t.w * value, t.h, COLOR_FILL);    // 有効部

        // つまみ
        const float knobX = t.x + t.w * value;
        const float knobY = cy;
        Prim::DrawCircle(knobX, knobY, KNOB_R, 3.0f, COLOR_KNOB, 20);
        Prim::DrawRect(knobX - 3.0f, knobY - 3.0f, 6.0f, 6.0f, COLOR_KNOB);

        // % 表示 (右端)
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%d%%",
                      static_cast<int>(std::lround(value * 100.0f)));
        const float tw = Text::MeasureWidth(buf, TEXT_VALUE);
        Text::Draw(panel.x + PANEL_W - PAD - tw, cy - TEXT_VALUE * 0.5f,
                   buf, TEXT_VALUE, COLOR_HINT);
    }

    /** @brief 値を 0..1 に収める (windows.h の min/max マクロ回避のため自前実装) */
    float Clamp01(float v)
    {
        return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
    }

    /** @brief マウスX座標からスライダー値(0..1)を求める */
    float ValueFromMouse(const Rect& panel, int index, int mx)
    {
        const Rect t = SliderTrack(panel, index);
        const float v = (static_cast<float>(mx) - t.x) / t.w;
        return Clamp01(v);
    }

    /** @brief スライダー値を該当設定へ適用する */
    void ApplySlider(int index, float value)
    {
        if (index == 0)      GameSettings_SetBGMVolume(value);
        else if (index == 1) GameSettings_SetSEVolume(value);
    }
}

namespace Settings
{
    bool IsOpen() { return g_Open; }

    void Open()
    {
        if (g_Open) return;
        g_Open       = true;
        g_DragSlider = -1;
        SoundManager_PlaySE(SOUND_SE_PAUSE);   // 開く合図 (素材未配置なら無音)
    }

    void Close()
    {
        if (!g_Open) return;
        g_Open       = false;
        g_DragSlider = -1;
        GameSettings_Save();                   // 変更を永続化
        SoundManager_PlaySE(SOUND_SE_CANCEL);  // 閉じる合図
    }

    void Toggle()
    {
        if (g_Open) Close();
        else        Open();
    }

    void Update(double /*elapsed_time*/)
    {
        if (!g_Open) return;

        const Rect  panel   = PanelRect();
        const auto& mouse   = InputManager_GetMouseState();
        const bool  trigger = InputManager_IsMouseLeftTrigger();
        const bool  held    = mouse.leftButton;

        //--- ドラッグ中の追従 (ボタンを離すまでスライダーを動かし続ける) ---
        if (g_DragSlider >= 0)
        {
            if (held)
            {
                ApplySlider(g_DragSlider, ValueFromMouse(panel, g_DragSlider, mouse.x));
                return;   // ドラッグ中は他の操作を受けない
            }
            // 離した: SEスライダーなら結果音量を試聴できるよう短音を鳴らす
            if (g_DragSlider == 1) SoundManager_PlaySE(SOUND_SE_SELECT);
            g_DragSlider = -1;
            GameSettings_Save();   // 確定した音量を即保存 (終了方法に依らず残す)
        }

        if (!trigger) return;   // 以降はクリックした瞬間のみ処理

        //--- スライダー掴み開始 (BGM=行0 / SE=行1) ---
        for (int i = 0; i < 2; ++i)
        {
            if (Hit(SliderHit(panel, i), mouse.x, mouse.y))
            {
                g_DragSlider = i;
                ApplySlider(i, ValueFromMouse(panel, i, mouse.x));
                return;
            }
        }

        //--- 垂直同期トグル ---
        if (Hit(VSyncButton(panel), mouse.x, mouse.y))
        {
            GameSettings_SetVSync(!GameSettings_GetVSync());
            GameSettings_Save();   // 即保存
            SoundManager_PlaySE(SOUND_SE_DECIDE);
            return;
        }

        //--- タイトルへ戻る (ゲーム中のみ): ランは残す＝つづきから可能 ---
        if (Scene_Current() == Scene::GAME
            && Hit(RetireButton(panel), mouse.x, mouse.y))
        {
            SoundManager_PlaySE(SOUND_SE_DECIDE);
            Scene_Change(Scene::TITLE);   // セーブは消さない (タイトルで「つづきから」)
            Close();
            return;
        }

        //--- デフォルトに戻す (公開項目のみ。ウィンドウ等は変更しない) ---
        if (Hit(ResetButton(panel), mouse.x, mouse.y))
        {
            GameSettings_SetBGMVolume(SettingsRange::BGM_VOLUME_DEFAULT);
            GameSettings_SetSEVolume(SettingsRange::SE_VOLUME_DEFAULT);
            GameSettings_SetVSync(SettingsRange::VSYNC_DEFAULT);
            GameSettings_Save();   // 即保存
            SoundManager_PlaySE(SOUND_SE_DECIDE);
            return;
        }

        //--- 閉じる ボタン or パネル外クリック ---
        if (Hit(CloseButton(panel), mouse.x, mouse.y)
            || !Hit(panel, mouse.x, mouse.y))
        {
            Close();
            return;
        }
    }

    void Draw()
    {
        if (!g_Open) return;

        const float sw = static_cast<float>(Direct3D_GetBackBufferWidth());
        const float sh = static_cast<float>(Direct3D_GetBackBufferHeight());
        const Rect  panel = PanelRect();
        const auto& mouse = InputManager_GetMouseState();

        // 背景暗幕 + パネル
        Prim::DrawRect(0, 0, sw, sh, COLOR_DIM);
        Prim::DrawRect(panel.x - 3.0f, panel.y - 3.0f, panel.w + 6.0f, panel.h + 6.0f, COLOR_EDGE);
        Prim::DrawRect(panel.x, panel.y, panel.w, panel.h, COLOR_PANEL);

        // 見出し
        const char* title = "設定";
        const float titleW = Text::MeasureWidth(title, TEXT_TITLE);
        Text::Draw(panel.x + (panel.w - titleW) * 0.5f, panel.y + 28.0f,
                   title, TEXT_TITLE, COLOR_TITLE);

        // スライダー (BGM / SE)
        DrawSlider(panel, 0, "BGM 音量", GameSettings_GetBGMVolume());
        DrawSlider(panel, 1, "SE 音量",  GameSettings_GetSEVolume());

        // 垂直同期トグル
        {
            const float cy = RowCenterY(panel, 2);
            Text::Draw(panel.x + PAD, cy - TEXT_LABEL * 0.5f,
                       "垂直同期 (VSync)", TEXT_LABEL, COLOR_LABEL);
            const Rect vb = VSyncButton(panel);
            const bool on = GameSettings_GetVSync();
            const bool hv = Hit(vb, mouse.x, mouse.y);
            DrawButton(vb, on ? "ON" : "OFF", hv, on ? COLOR_ON : COLOR_OFF);
        }

        // タイトルへ戻る (ゲーム中のみ。ランは保持され「つづきから」で再開できる)
        if (Scene_Current() == Scene::GAME)
        {
            const Rect tb = RetireButton(panel);
            DrawButton(tb, "タイトルへ戻る", Hit(tb, mouse.x, mouse.y), COLOR_RETIRE);
        }

        // 下部ボタン
        const Rect rb = ResetButton(panel);
        const Rect cb = CloseButton(panel);
        DrawButton(rb, "デフォルトに戻す", Hit(rb, mouse.x, mouse.y), COLOR_OFF);
        DrawButton(cb, "閉じる (O)",       Hit(cb, mouse.x, mouse.y), COLOR_EDGE);

        // 操作ヒント
        const char* hint = "ドラッグで音量を調整  /  O で開閉";
        const float hintW = Text::MeasureWidth(hint, TEXT_HINT);
        Text::Draw(panel.x + (panel.w - hintW) * 0.5f,
                   cb.y - TEXT_HINT - 14.0f, hint, TEXT_HINT, COLOR_HINT);
    }
}
