/****************************************
 * @file sprite_anim.h
 * @brief スプライトアニメーション描画モジュールのヘッダ
 * @author Natsume Shidara
 * @date 2025/06/06
 *
 * スプライトシートによるパターンアニメーションの
 * 登録・再生・描画インターフェースを提供する。
 * 2Dスプライト描画とビルボード描画に対応。
 ****************************************/

#ifndef SPRITE_ANIM_H
#define SPRITE_ANIM_H

#include <DirectXMath.h>

/** @brief スプライトアニメーション初期化 */
void SpriteAnim_Initialize();
/** @brief スプライトアニメーション終了処理 */
void SpriteAnim_Finalize(void);

/** @brief 全アニメーションプレイヤーの更新（フレーム進行） */
void SpriteAnim_Update(double elapsed_time);
/** @brief 2Dスプライトアニメーション描画 */
void SpriteAnim_Draw(int playid, float dx, float dy, float dw, float dh);

/**
 * @brief アニメーションパターンを登録
 * @param textureId テクスチャ管理番号
 * @param patternMax パターンの画像数
 * @param m_HPatternMax 一列（横）のパターン最大数
 * @param m_seconds_per_pattern 再生速度（1フレームあたりの秒数）
 * @param patternSize パターン1個のサイズ
 * @param patternStartPosition 最初のパターンの左上座標
 * @param isLoop ループするか
 * @return パターンID（失敗時-1）
 * @pre 必ずTexture_Load()のあとに呼ぶこと
 */
int SpriteAnim_RegisterPattern(
    int textureId,
    int patternMax,
    int m_HPatternMax,
    double m_seconds_per_pattern,
    DirectX::XMUINT2 patternSize,
    DirectX::XMUINT2 patternStartPosition,
    bool isLoop = true
);

/** @brief アニメーションプレイヤーを生成し再生開始 */
int SpriteAnim_CreatePlayer(int anim_pattern_id);
/** @brief アニメーション再生が停止しているか判定 */
bool SpriteAnim_IsStopped(int index);
/** @brief アニメーションプレイヤーを破棄 */
void SpriteAnim_DestroyPlayer(int index);

#endif // SPRITE_ANIM_H
