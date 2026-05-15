/****************************************
 * @file texture.h
 * @brief テクスチャ管理モジュールのヘッダ
 * @author Natsume Shidara
 * @date 2025/06/06
 *
 * テクスチャの読み込み・管理・バインドを行う。
 * ファイルパス指定とメモリ指定(DLL用)の両方に対応。
 * ハッシュマップによる重複読み込み防止機能付き。
 ****************************************/

#ifndef TEXTURE_H
#define TEXTURE_H

#include <d3d11.h>

/** @brief テクスチャ管理の初期化 */
void Texture_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

/** @brief テクスチャ管理の終了処理（全テクスチャ解放） */
void Texture_Finalize(void);

/**
 * @brief テクスチャ画像をファイルから読み込み
 * @param pFilename 読み込みたいファイルのパス
 * @return 管理番号（失敗時-1）
 * @pre Texture_Initialize()で初期化後に呼ぶこと
 */
int Texture_Load(const wchar_t* pFilename);

/**
 * @brief メモリ上のデータからテクスチャを読み込み（DLL用）
 * @param pData テクスチャデータの先頭ポインタ
 * @param dataSize データサイズ（バイト）
 * @param pDebugName ハッシュマップのキー用名前
 * @return 管理番号（失敗時-1）
 */
int Texture_LoadFromMemory(const void* pData, size_t dataSize, const wchar_t* pDebugName);

/** @brief 全テクスチャを解放 */
void Texture_AllRelease();

/**
 * @brief テクスチャをピクセルシェーダーにバインド
 * @param texid 管理番号
 * @param slot  シェーダーリソーススロット番号
 */
void Texture_SetTexture(int texid, int slot = 0);

/** @brief テクスチャの幅を取得 */
int Texture_Width(int texid);
/** @brief テクスチャの高さを取得 */
int Texture_Height(int texid);

#endif // TEXTURE_H
