/****************************************
 * @file sound_manager.h
 * @brief ゲーム全体のサウンド管理 (2D版・最小構成)
 * @author Natsume Shidara
 * @date 2026/01/13
 * @update 2026/05/15 - 〇×ローグライト用にスリム化 (BGM/UI SE のみ)
 ****************************************/

#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

// サウンドID定義
// ※ ここに ID を追加したら sound_manager.cpp の SOUND_FILES[] にも
//    同じ順序で対応パスを1行追加すること (要素数が SOUND_MAX と一致する必要あり)。
enum SoundID
{
    // BGM
    SOUND_BGM_TITLE,
    SOUND_BGM_GAME,
    SOUND_BGM_RESULT,
    SOUND_BGM_BOSS,     // ラスボス戦専用
    SOUND_BGM_WIN,      // 勝利テーマ (長尺。決着〜報酬画面まで継続)
    SOUND_BGM_LOSE,     // 敗北テーマ (長尺)

    // SE - UI
    SOUND_SE_SELECT,    // カーソル移動・カードホバー
    SOUND_SE_DECIDE,    // 決定
    SOUND_SE_CANCEL,    // 戻る・キャンセル
    SOUND_SE_PAUSE,     // 設定/ポーズを開く

    // SE - ゲームプレイ
    SOUND_SE_PLACE,     // 駒の設置
    SOUND_SE_ABILITY,   // アビリティ/ボスギミックの発動
    SOUND_SE_TIMER_LOW, // 思考時間 残りわずか

    // 総数
    SOUND_MAX
};

// 初期化・終了
void SoundManager_Initialize();
void SoundManager_Finalize();

// BGM制御
void SoundManager_PlayBGM(SoundID id);
void SoundManager_StopBGM();
void SoundManager_SetBGMVolume(float volume);  // 0.0 ~ 1.0

// 決着曲(勝利/敗北)の排他再生:
//   長尺の win/lose を「現在のBGMを止めて単独で」鳴らす(対戦BGMとの被り防止)。
//   BGM枠・BGM音量で再生され、次にシーンが PlayBGM を呼ぶと自然に置き換わる。
void SoundManager_PlayResultTheme(SoundID id);

// SE再生
void SoundManager_PlaySE(SoundID id);
void SoundManager_SetSEVolume(float volume);   // 0.0 ~ 1.0

// 更新
void SoundManager_Update(float dt);

// 一括停止
void SoundManager_StopAll();

#endif // SOUND_MANAGER_H
