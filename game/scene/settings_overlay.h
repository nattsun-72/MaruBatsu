/****************************************
 * @file   settings_overlay.h
 * @brief  設定オーバーレイ (どのシーンの上にも重ねて開けるモーダル設定画面)
 * @author Natsume Shidara
 * @date   2026/06/24
 *
 * タイトル/ゲーム中に「O」キーで開閉する設定画面。シーン遷移を伴わず、
 * 下層シーンを一時停止したまま上から重ねて描画する(別シーンにすると
 * ゲーム中に開いた際に対戦状態が初期化されてしまうため)。
 *
 * 値の実体は GameSettings が保持する。ここは UI とその操作のみを担う。
 ****************************************/
#ifndef SETTINGS_OVERLAY_H
#define SETTINGS_OVERLAY_H

//======================================
// 設定オーバーレイ API
//======================================
namespace Settings
{
    /** @brief オーバーレイが表示中か */
    bool IsOpen();

    /** @brief 表示を開く (BGM継続のまま下層シーンを止める) */
    void Open();

    /** @brief 表示を閉じ、現在の設定をファイルへ保存する */
    void Close();

    /** @brief 開いていれば閉じ、閉じていれば開く */
    void Toggle();

    /**
     * @brief  表示中の入力処理 (スライダー操作・トグル・閉じる)
     * @param  elapsed_time 前フレームからの経過秒数
     * @detail Settings::IsOpen() が true のフレームのみ呼ぶ想定。
     */
    void Update(double elapsed_time);

    /** @brief 半透明オーバーレイと設定UIを描画する (下層シーン描画の後に呼ぶ) */
    void Draw();
}

#endif // SETTINGS_OVERLAY_H
