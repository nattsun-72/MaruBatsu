#pragma once
// ============================================================
//  AssetDLL - Public API Header
// ============================================================
//
//  ゲーム本体からインクルードして使うヘッダ。
//
//  使い方:
//    1. AssetDLL.lib をリンク
//    2. #include "asset_dll.h"
//    3. AssetDLL_Init() を起動時に呼ぶ
//    4. AssetDLL_GetData(リソースID, &data, &size) でアセット取得
//    5. AssetDLL_Shutdown() を終了時に呼ぶ
//
//  データのライフサイクル:
//    - GetDataで返されるポインタはDLLがアンロードされるまで有効
//    - free/delete してはいけない（DLL内のリソース領域を直接指している）
// ============================================================

#include <windows.h>

// ── DLLエクスポート/インポート切り替え ───────────────────
//  DLL側 (AssetDLL.cpp) では ASSET_DLL_EXPORTS が定義されている
//  → __declspec(dllexport) でエクスポート
//
//  ゲーム側では定義されていない
//  → __declspec(dllimport) でインポート
// ──────────────────────────────────────────────────────────
#ifdef ASSET_DLL_EXPORTS
    #define ASSET_API __declspec(dllexport)
#else
    #define ASSET_API __declspec(dllimport)
#endif

extern "C" {

/// DLLの初期化。DLLモジュールハンドルを内部で取得する。
/// @return 成功時 true
ASSET_API bool AssetDLL_Init();

/// リソースIDを指定してアセットデータを取得する。
/// @param resourceId  resource.h で定義したID (例: IDR_TEX_WHITE = 1001)
/// @param outData     [out] アセットデータの先頭ポインタ
/// @param outSize     [out] アセットデータのバイトサイズ
/// @return 成功時 true
ASSET_API bool AssetDLL_GetData(int resourceId, const void** outData, size_t* outSize);

/// DLLのシャットダウン (現在は何もしないが、将来の拡張用)
ASSET_API void AssetDLL_Shutdown();

}
