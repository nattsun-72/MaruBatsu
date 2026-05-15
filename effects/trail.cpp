/****************************************
 * @file trail.cpp
 * @brief 軌跡エフェクトの描画・更新処理
 * @author Natsume Shidara
 * @date 2025/09/03
 * @update 2025/11/21
 ****************************************/

#include "trail.h"
#include "billboard.h"
#include "direct3d.h"
#include "shader_billboard.h"
#include "sprite.h"
#include "texture.h"
#ifdef USE_ASSET_DLL
#include "asset_dll.h"
#include "resource.h"
#endif

using namespace DirectX;

// 定数定義・マクロ定義
static constexpr unsigned int TRAIL_MAX = 4096; // 軌跡の最大数
static constexpr double MIN_BIRTH_TIME = 0.00001; // 発生時刻の最小値（0と区別するための補正値）

// テクスチャパス（開発用/本番用で切り替え可能にしておくのが好ましい）
// constexpr const wchar_t* TRAIL_TEXTURE_PATH = L"release/resource/SpaceShooterAssets/Artwork/effect/effect000.jpg";
constexpr const wchar_t* TRAIL_TEXTURE_PATH = L"assets/white.png";

// 構造体定義
/****************************************
 * @struct Trail
 * @brief 軌跡エフェクトの個体データ
 * @detail 座標、色、寿命などのパラメータを管理
 ****************************************/
struct Trail
{
    XMFLOAT3 pos; // 発生位置
    XMFLOAT4 color; // 基本色
    float size; // 基本サイズ
    double lifeTime; // 寿命（秒）
    double birthTime; // 発生時刻（0以下の場合は未使用とみなす）
};

// グローバル変数
static int g_TrailTexId = -1; // テクスチャID
static Trail g_Trails[TRAIL_MAX]{}; // 軌跡データ配列
static double g_Time = 0.0; // 全体経過時間

// フリーリスト（O(1)でスロット割り当て・返却）
static unsigned int g_FreeList[TRAIL_MAX]; // フリーインデックスのスタック
static unsigned int g_FreeCount = 0;       // フリーリスト内の要素数

// 関数定義

/**
 * @brief 軌跡システムの初期化
 * @detail 配列のクリアとテクスチャ読み込みを行う
 */
void Trail_Initialize()
{
    // 配列の初期化（birthTimeを0にして未使用扱いにする）
    for (unsigned int i = 0; i < TRAIL_MAX; i++)
    {
        g_Trails[i].birthTime = 0.0;
        g_FreeList[i] = i; // 全スロットをフリーリストに
    }
    g_FreeCount = TRAIL_MAX;

#ifdef USE_ASSET_DLL
    {
        const void* pData = nullptr;
        size_t dataSize = 0;
        if (AssetDLL_GetData(IDR_TEX_WHITE, &pData, &dataSize))
            g_TrailTexId = Texture_LoadFromMemory(pData, dataSize, L"trail_white");
    }
#else
    g_TrailTexId = Texture_Load(TRAIL_TEXTURE_PATH);
#endif
}

/**
 * @brief 軌跡システムの終了処理
 */
void Trail_Finalize()
{
    // 必要であればテクスチャの解放処理などを記述
}

/**
 * @brief 軌跡の更新
 * @param elapsed_time 前フレームからの経過時間
 * @detail 寿命を超えた軌跡を無効化する
 */
void Trail_Update(double elapsed_time)
{
    g_Time += elapsed_time;

    for (unsigned int i = 0; i < TRAIL_MAX; i++)
    {
        Trail& t = g_Trails[i];

        // 未使用のスロットはスキップ
        if (t.birthTime <= 0.0)
            continue;

        double trailElapsedTime = g_Time - t.birthTime;

        // 寿命判定
        if (trailElapsedTime > t.lifeTime)
        {
            t.birthTime = 0.0; // 寿命切れにより無効化
            // フリーリストに返却（O(1)）
            g_FreeList[g_FreeCount++] = i;
        }
    }
}

/**
 * @brief 軌跡の描画
 * @detail ビルボードを使用して描画を行う。加算合成を推奨。
 */
void Trail_Draw()
{
    //--------------------------------------
    // レンダーステート設定（ループ外で一度だけ行う）
    //--------------------------------------
    // 深度書き込みを無効化（半透明合成のため）
    Direct3D_SetDepthWriteEnable(false);
    Direct3D_SetBlendState(BlendMode::Add);

    // 加算合成を有効にする場合（エフェクトとして光らせたい場合）
    // Direct3D_SetAlphaBlend_Add();

    //--------------------------------------
    // 描画ループ（バッチ描画で高速化）
    //--------------------------------------
    Billboard_BeginBatch();

    for (Trail& t : g_Trails)
    {
        if (t.birthTime <= 0.0)
            continue;

        double trailElapsedTime = g_Time - t.birthTime;

        // 寿命に対する進行割合 (0.0 -> 1.0)
        float ratio = (float)(trailElapsedTime / t.lifeTime);

        // 安全策：ratioが1.0を超えないようにクランプ（計算誤差対策）
        if (ratio > 1.0f)
            ratio = 1.0f;

        // 経時変化計算
        // 元の t.size や t.color を書き換えないよう、ローカル変数で計算する
        float drawSize = t.size * (1.0f - ratio);

        XMFLOAT4 drawColor = t.color;
        drawColor.w = t.color.w * (1.0f - ratio); // アルファ値をフェードアウト
        XMFLOAT2 pivot = {0.0f,0.0f};
        Billboard_DrawBatch(g_TrailTexId, t.pos, { drawSize, drawSize }, pivot,t.color);
    }

    Billboard_EndBatch();

    //--------------------------------------
    // レンダーステート復帰
    //--------------------------------------
    Direct3D_SetBlendState(BlendMode::None);
    Direct3D_SetDepthWriteEnable(true);
}

/**
 * @brief 新しい軌跡を生成する
 * @param position 発生座標
 * @param color 色
 * @param size サイズ
 * @param lifeTime 寿命（秒）
 */
void Trail_Create(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4& color, float size, double lifeTime)
{
    // フリーリストから O(1) でスロットを取得
    if (g_FreeCount == 0) return; // 空きなし

    unsigned int idx = g_FreeList[--g_FreeCount];
    Trail& t = g_Trails[idx];

    t.pos = position;
    t.color = color;
    t.size = size;
    t.lifeTime = lifeTime;

    // 現在時刻を発生時刻として記録（これにより有効化される）
    t.birthTime = g_Time;

    // 万が一 g_Time が 0.0 の場合、即座に無効扱いされないよう補正
    if (t.birthTime == 0.0)
    {
        t.birthTime = MIN_BIRTH_TIME;
    }
}
