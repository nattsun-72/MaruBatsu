/****************************************
 * @file anim_loader.h
 * @brief FBXモデル読み込み（メッシュ・マテリアル・テクスチャ・クリップ抽出）
 * @author Natsume Shidara
 * @date 2025/06/06
 ****************************************/

#ifndef ANIM_LOADER_H
#define ANIM_LOADER_H

#include "animation_model.h"

// モデル読み込み（メッシュ・ボーン・マテリアル・アニメーション）
ANIM_MODEL* AnimLoader_LoadFromFile(const char* fileName, float scale);

// 追加アニメーションクリップ読み込み
bool AnimLoader_LoadClip(ANIM_MODEL* model, const char* clipFileName, const char* clipName);

// テクスチャ共有リソース
ID3D11ShaderResourceView* AnimLoader_GetWhiteSRV();
void AnimLoader_InitWhiteTexture();
void AnimLoader_ReleaseWhiteTexture();

#endif // ANIM_LOADER_H
