/****************************************
 * @file sampler.h
 * @brief テクスチャサンプラーステート管理モジュールのヘッダ
 * @author Natsume Shidara
 * @date 2025/06/06
 *
 * ポイント・リニア・異方性フィルタリングの
 * サンプラーステート切り替えインターフェースを提供する。
 ****************************************/
#ifndef SAMPLER_H
#define SAMPLER_H

#include <d3d11.h>

/** @brief サンプラー初期化（各フィルタモードのステート作成） */
void Sampler_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
/** @brief サンプラー終了処理 */
void Sampler_Finalize(void);

/** @brief ポイントフィルタリングに切り替え */
void Sampler_SetFilterPoint();
/** @brief リニアフィルタリングに切り替え */
void Sampler_SetFilterLinear();
/** @brief 異方性フィルタリングに切り替え */
void Sampler_SetFilterAnisotropic();

#endif // SAMPLER_H
