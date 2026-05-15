// ============================================================
//  AssetDLL - Implementation
// ============================================================
//  Windows の「リソースAPI」を使って、DLL内に埋め込まれた
//  アセットデータのポインタとサイズを返す。
//
//  リソースAPI の流れ:
//    FindResource()   → リソースの場所を検索
//    LoadResource()   → リソースをメモリにマッピング
//    LockResource()   → 読み取り用ポインタを取得
//    SizeofResource() → データサイズを取得
//
//  これらのAPIはDLLのモジュールハンドル (HMODULE) が必要。
//  DllMain() で受け取ったハンドルを保存しておく。
// ============================================================

#define ASSET_DLL_EXPORTS  // dllexport を有効にする
#include "asset_dll.h"
#include "resource.h"

// ── DLLモジュールハンドル ────────────────────────────────
//  DllMain() で自動的にセットされる。
//  このハンドルを使って、DLL自身の中のリソースを検索する。
static HMODULE g_hAssetModule = NULL;

// ── DLLエントリーポイント ────────────────────────────────
//  Windowsがこの関数を自動的に呼び出す:
//    DLL_PROCESS_ATTACH: DLLがプロセスにロードされた時
//    DLL_PROCESS_DETACH: DLLがアンロードされる時
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID /*lpReserved*/)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_hAssetModule = hModule;  // ハンドルを保存！
        break;
    case DLL_PROCESS_DETACH:
        g_hAssetModule = NULL;
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

// ── AssetDLL_Init ────────────────────────────────────────
//  DllMain で既にハンドルをセット済みなので、ここでは検証のみ。
//  将来的にはキャッシュ初期化等を追加できる。
ASSET_API bool AssetDLL_Init()
{
    return (g_hAssetModule != NULL);
}

// ── AssetDLL_GetData ─────────────────────────────────────
//  これがメインの関数。
//
//  ゲーム側は以下のように呼び出す:
//    const void* data = nullptr;
//    size_t size = 0;
//    if (AssetDLL_GetData(IDR_TEX_WHITE, &data, &size)) {
//        // data にテクスチャのバイナリデータ、size にサイズが入る
//        CreateWICTextureFromMemory(device, context, data, size, ...);
//    }
ASSET_API bool AssetDLL_GetData(int resourceId, const void** outData, size_t* outSize)
{
    if (!g_hAssetModule || !outData || !outSize)
        return false;

    // Step 1: リソースを検索
    //   MAKEINTRESOURCE(id) はint→リソース名ポインタへの変換マクロ
    //   RT_RCDATA は "生バイナリデータ" タイプを示す定数
    HRSRC hRes = FindResource(g_hAssetModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hRes)
        return false;

    // Step 2: リソースをロード (実際にはメモリマッピング)
    HGLOBAL hData = LoadResource(g_hAssetModule, hRes);
    if (!hData)
        return false;

    // Step 3: データポインタを取得
    //   LockResourceは実際にはロックしない（Win32の歴史的な名前）。
    //   返されるポインタはDLLがロードされている間ずっと有効。
    *outData = LockResource(hData);

    // Step 4: データサイズを取得
    *outSize = static_cast<size_t>(SizeofResource(g_hAssetModule, hRes));

    return (*outData != nullptr && *outSize > 0);
}

// ── AssetDLL_Shutdown ────────────────────────────────────
//  現在は特に何もしない。
//  リソースはDLLアンロード時に自動解放される。
ASSET_API void AssetDLL_Shutdown()
{
    // 将来の拡張用 (キャッシュクリア等)
}
