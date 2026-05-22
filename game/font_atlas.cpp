/****************************************
 * @file font_atlas.cpp
 * @brief DirectWrite動的グリフアトラスの実装
 * @author Natsume Shidara
 * @date 2026/05/22
 *
 * 処理の流れ (1グリフあたり):
 *   1. Direct2D の WICビットマップRT に1文字をDrawText
 *   2. WICビットマップをロックし、プリマルチプライ済みBGRAを読む
 *   3. 「白RGB + カバレッジα」のストレートRGBAへ変換
 *   4. D3D11アトラステクスチャの空きセルへ UpdateSubresource
 *   5. セルの正規化UVをキャッシュ
 ****************************************/
#include "font_atlas.h"
#include "direct3d.h"

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <unordered_map>
#include <vector>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

using Microsoft::WRL::ComPtr;

namespace
{
    // ── アトラス構成 ─────────────────────────────────────
    constexpr UINT  ATLAS_SIZE    = 2048;            // アトラス1辺(px)
    constexpr UINT  CELL_SIZE     = 64;              // 1セル1辺(px)
    constexpr float GLYPH_FONT_PX = 52.0f;           // セル内フォントサイズ
    constexpr UINT  CELLS_PER_ROW = ATLAS_SIZE / CELL_SIZE;       // 32
    constexpr UINT  MAX_CELLS     = CELLS_PER_ROW * CELLS_PER_ROW; // 1024

    // 使用する日本語フォント (Windows標準)
    const wchar_t* FONT_FAMILY = L"Yu Gothic UI";

    // ── COMリソース ──────────────────────────────────────
    ComPtr<IDWriteFactory>       g_DWrite;
    ComPtr<ID2D1Factory>         g_D2D;
    ComPtr<IWICImagingFactory>   g_WIC;
    ComPtr<IDWriteTextFormat>    g_TextFormat;

    ComPtr<IWICBitmap>           g_CellBitmap;   // 1文字描画用オフスクリーン
    ComPtr<ID2D1RenderTarget>    g_CellRT;
    ComPtr<ID2D1SolidColorBrush> g_Brush;

    ComPtr<ID3D11Texture2D>          g_AtlasTex;
    ComPtr<ID3D11ShaderResourceView> g_AtlasSRV;

    // ── 状態 ─────────────────────────────────────────────
    UINT g_NextCell = 0;
    std::unordered_map<uint32_t, FontAtlas::GlyphUV> g_Cache;
    bool g_Ready = false;

    // コードポイント → UTF-16 (BMPは1、追加面はサロゲートペア)
    int CodepointToUtf16(uint32_t cp, wchar_t out[2])
    {
        if (cp <= 0xFFFF)
        {
            out[0] = static_cast<wchar_t>(cp);
            return 1;
        }
        cp -= 0x10000;
        out[0] = static_cast<wchar_t>(0xD800 + (cp >> 10));
        out[1] = static_cast<wchar_t>(0xDC00 + (cp & 0x3FF));
        return 2;
    }

    // 1グリフをラスタライズしてアトラスへ書き込む
    bool RasterizeGlyph(uint32_t codepoint, FontAtlas::GlyphUV& outUV)
    {
        if (g_NextCell >= MAX_CELLS) return false;

        wchar_t utf16[2];
        const int wlen = CodepointToUtf16(codepoint, utf16);

        // 1. Direct2D で1文字をセルビットマップへ描画
        g_CellRT->BeginDraw();
        g_CellRT->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
        const D2D1_RECT_F layout =
            D2D1::RectF(0.0f, 0.0f,
                        static_cast<float>(CELL_SIZE),
                        static_cast<float>(CELL_SIZE));
        g_CellRT->DrawText(utf16, static_cast<UINT32>(wlen),
                           g_TextFormat.Get(), layout, g_Brush.Get());
        if (FAILED(g_CellRT->EndDraw())) return false;

        // 2. WICビットマップをロックして画素を読む
        WICRect rc = { 0, 0, static_cast<INT>(CELL_SIZE), static_cast<INT>(CELL_SIZE) };
        ComPtr<IWICBitmapLock> lock;
        if (FAILED(g_CellBitmap->Lock(&rc, WICBitmapLockRead, &lock))) return false;

        UINT stride = 0, bufSize = 0;
        BYTE* src = nullptr;
        lock->GetStride(&stride);
        lock->GetDataPointer(&bufSize, &src);
        if (!src) return false;

        // 3. プリマルチプライBGRA → 「白RGB + α」ストレートRGBA へ変換
        //    D2Dは白文字をプリマルチプライで描くため α が被覆率になる。
        std::vector<uint32_t> cell(static_cast<size_t>(CELL_SIZE) * CELL_SIZE);
        for (UINT y = 0; y < CELL_SIZE; ++y)
        {
            const BYTE* row = src + static_cast<size_t>(y) * stride;
            for (UINT x = 0; x < CELL_SIZE; ++x)
            {
                const BYTE a = row[x * 4 + 3];        // アルファ = 被覆率
                // R8G8B8A8 (little-endian) = 0xAABBGGRR。白固定 + α。
                cell[static_cast<size_t>(y) * CELL_SIZE + x] =
                    (static_cast<uint32_t>(a) << 24) | 0x00FFFFFFu;
            }
        }
        lock.Reset();

        // 4. アトラスの空きセルへ転送
        const UINT cellX = (g_NextCell % CELLS_PER_ROW) * CELL_SIZE;
        const UINT cellY = (g_NextCell / CELLS_PER_ROW) * CELL_SIZE;
        D3D11_BOX box = { cellX, cellY, 0, cellX + CELL_SIZE, cellY + CELL_SIZE, 1 };
        Direct3D_GetContext()->UpdateSubresource(
            g_AtlasTex.Get(), 0, &box,
            cell.data(), CELL_SIZE * sizeof(uint32_t), 0);

        // 5. 正規化UV
        const float inv = 1.0f / static_cast<float>(ATLAS_SIZE);
        outUV.u0 = cellX * inv;
        outUV.v0 = cellY * inv;
        outUV.u1 = (cellX + CELL_SIZE) * inv;
        outUV.v1 = (cellY + CELL_SIZE) * inv;

        ++g_NextCell;
        return true;
    }
}

namespace FontAtlas
{
    bool Initialize()
    {
        if (g_Ready) return true;

        ID3D11Device* device = Direct3D_GetDevice();
        if (!device) return false;

        // DirectWrite
        if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                __uuidof(IDWriteFactory),
                reinterpret_cast<IUnknown**>(g_DWrite.GetAddressOf()))))
            return false;

        // Direct2D
        if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                g_D2D.GetAddressOf())))
            return false;

        // WIC
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                CLSCTX_INPROC_SERVER, IID_PPV_ARGS(g_WIC.GetAddressOf()))))
            return false;

        // テキストフォーマット (セル中央寄せ)
        if (FAILED(g_DWrite->CreateTextFormat(
                FONT_FAMILY, nullptr,
                DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL, GLYPH_FONT_PX, L"ja-jp",
                g_TextFormat.GetAddressOf())))
            return false;
        g_TextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        g_TextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        // 1文字描画用 WICビットマップ + D2Dレンダーターゲット
        if (FAILED(g_WIC->CreateBitmap(CELL_SIZE, CELL_SIZE,
                GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnLoad,
                g_CellBitmap.GetAddressOf())))
            return false;

        const D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                              D2D1_ALPHA_MODE_PREMULTIPLIED));
        if (FAILED(g_D2D->CreateWicBitmapRenderTarget(
                g_CellBitmap.Get(), rtProps, g_CellRT.GetAddressOf())))
            return false;
        g_CellRT->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
        if (FAILED(g_CellRT->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::White), g_Brush.GetAddressOf())))
            return false;

        // D3D11アトラステクスチャ
        D3D11_TEXTURE2D_DESC td = {};
        td.Width            = ATLAS_SIZE;
        td.Height           = ATLAS_SIZE;
        td.MipLevels        = 1;
        td.ArraySize        = 1;
        td.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage            = D3D11_USAGE_DEFAULT;
        td.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
        if (FAILED(device->CreateTexture2D(&td, nullptr, g_AtlasTex.GetAddressOf())))
            return false;
        if (FAILED(device->CreateShaderResourceView(
                g_AtlasTex.Get(), nullptr, g_AtlasSRV.GetAddressOf())))
            return false;

        g_NextCell = 0;
        g_Cache.clear();
        g_Ready = true;
        return true;
    }

    void Finalize()
    {
        g_Cache.clear();
        g_AtlasSRV.Reset();
        g_AtlasTex.Reset();
        g_Brush.Reset();
        g_CellRT.Reset();
        g_CellBitmap.Reset();
        g_TextFormat.Reset();
        g_WIC.Reset();
        g_D2D.Reset();
        g_DWrite.Reset();
        g_NextCell = 0;
        g_Ready = false;
    }

    bool GetGlyph(uint32_t codepoint, GlyphUV& outUV)
    {
        if (!g_Ready) return false;

        auto it = g_Cache.find(codepoint);
        if (it != g_Cache.end())
        {
            outUV = it->second;
            return true;
        }

        GlyphUV uv;
        if (!RasterizeGlyph(codepoint, uv)) return false;
        g_Cache.emplace(codepoint, uv);
        outUV = uv;
        return true;
    }

    ID3D11ShaderResourceView* GetAtlasSRV()
    {
        return g_AtlasSRV.Get();
    }
}
