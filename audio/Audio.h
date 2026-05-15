/****************************************
 * @file Audio.h
 * @brief XAudio2を用いたサウンド管理モジュール
 * @author Natsume Shidara
 * @date 2025/06/06
 *
 * WAV/MP3ファイルの読み込み・再生・停止・
 * ボリューム制御・ピッチ制御を提供する。
 *
 * @update 2026/01/13 - StopAudio追加
 * @update 2026/01/13 - ボリューム制御追加
 ****************************************/

#ifndef AUDIO_H
#define AUDIO_H

 /**
  * @brief オーディオシステム初期化
  */
void InitAudio();

/**
 * @brief オーディオシステム解放
 */
void UninitAudio();

/**
 * @brief WAV/MP3ファイル読み込み
 * @param FileName 読み込むファイル名
 * @return int 読み込みに使用したインデックス（失敗時は -1）
 */
int LoadAudio(const char* FileName);

/**
 * @brief メモリ上のサウンドデータ読み込み (DLL用)
 * @param pData サウンドデータの先頭ポインタ
 * @param dataSize データサイズ (バイト)
 * @param format "mp3" or "wav"
 * @return int 読み込みに使用したインデックス（失敗時は -1）
 */
int LoadAudioFromMemory(const void* pData, size_t dataSize, const char* format);

/**
 * @brief サウンドデータ解放
 * @param Index 解放するオーディオのインデックス
 */
void UnloadAudio(int Index);

/**
 * @brief サウンドを再生
 * @param Index 再生するオーディオのインデックス
 * @param Loop ループ再生するかどうか（true=ループ）
 */
void PlayAudio(int Index, bool Loop = false);

/**
 * @brief サウンドを停止
 * @param Index 停止するオーディオのインデックス
 */
void StopAudio(int Index);

/**
 * @brief 全サウンドを停止
 */
void StopAllAudio();

/**
 * @brief 個別サウンドのボリューム設定
 * @param Index オーディオのインデックス
 * @param volume ボリューム（0.0〜1.0）
 */
void SetAudioVolume(int Index, float volume);

/**
 * @brief マスターボリューム設定
 * @param volume ボリューム（0.0〜1.0）
 */
void SetMasterVolume(float volume);

/**
 * @brief サウンドのピッチ（再生速度）設定
 * @param Index オーディオのインデックス
 * @param ratio 周波数比率（1.0=通常、>1.0=高音/速い、<1.0=低音/遅い）
 *              XAudio2の仕様上、XAUDIO2_MIN_FREQ_RATIO〜XAUDIO2_MAX_FREQ_RATIO
 */
void SetAudioPitch(int Index, float ratio);

#endif // AUDIO_H