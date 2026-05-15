/****************************************
 * @file bullet_hit_effect.cpp
 * @brief 弾丸ヒット時の爆発エフェクト実装
 * @author Natsume Shidara
 * @date 2025/09/11
 *
 * スプライトシートアニメーションによる爆発演出。
 * アニメーション完了後に自動で破棄される。
 ****************************************/
#include "bullet_hit_effect.h"
#include "DirectXMath.h"
#include "sprite_anim.h"
#include "texture.h"
#include "direct3d.h"
#ifdef USE_ASSET_DLL
#include "asset_dll.h"
#include "resource.h"
#endif

using namespace DirectX;

namespace
{
    // アニメーションスプライトシート設定
    constexpr int ANIM_COLUMNS = 16;          // スプライトシートの列数
    constexpr int ANIM_ROWS = 4;              // スプライトシートの行数
    constexpr double ANIM_FRAME_DURATION = 0.01; // 1フレームの表示時間（秒）
    constexpr int ANIM_FRAME_SIZE = 256;      // 1フレームのサイズ（ピクセル）
}

static int g_TexId = -1;
static int g_AnimPatternId = -1;
static int g_EffectsCount = 0;

/**
 * @brief 弾丸ヒットエフェクトの個体クラス
 *
 * スプライトアニメーションで爆発を表現し、
 * アニメーション再生完了後に自動破棄される。
 */
class BulletHitEffect
{
private:
    XMFLOAT3 m_position{};
    int m_anim_play_id = -1;
    bool m_is_destroy{ false };

public:
    BulletHitEffect(const XMFLOAT3& position) : m_position(position), m_anim_play_id(SpriteAnim_CreatePlayer(g_AnimPatternId)) {}

    ~BulletHitEffect() { SpriteAnim_DestroyPlayer(m_anim_play_id); }

    void Update();
    void Draw() const;
    bool IsDestroy() const { return m_is_destroy; }
};

static constexpr int EFFECT_MAX = 256;
static BulletHitEffect* g_pEffects[EFFECT_MAX]{};

/** @brief 弾丸ヒットエフェクトシステムの初期化 */
void BulletHItEffect_Initialize()
{
#ifdef USE_ASSET_DLL
    {
        const void* pData = nullptr;
        size_t dataSize = 0;
        if (AssetDLL_GetData(IDR_TEX_EXPLOSION, &pData, &dataSize))
            g_TexId = Texture_LoadFromMemory(pData, dataSize, L"bullet_hit_explosion");
    }
#else
    g_TexId = Texture_Load(L"assets/Explosion.png");
#endif
    g_AnimPatternId = SpriteAnim_RegisterPattern(g_TexId, ANIM_COLUMNS, ANIM_ROWS, ANIM_FRAME_DURATION, { ANIM_FRAME_SIZE, ANIM_FRAME_SIZE }, { 0, 0 }, false);
    g_EffectsCount = 0; // 初期化を明示的に
}

/** @brief 弾丸ヒットエフェクトシステムの終了処理 */
void BulletHItEffect_Finalize(void)
{
    for (int i = 0; i < g_EffectsCount; i++)
    {
        delete g_pEffects[i];
        g_pEffects[i] = nullptr; // 念のためnullptrに
    }
    g_EffectsCount = 0;
}

/** @brief 全エフェクトの更新と完了エフェクトの削除 */
void BulletHItEffect_Update(double elapsed_time)
{
    (void)elapsed_time;
    // 全エフェクトを更新
    for (int i = 0; i < g_EffectsCount; i++)
    {
        g_pEffects[i]->Update();
    }

    // 破棄フラグが立ったものを削除
    for (int i = g_EffectsCount - 1; i >= 0; i--)
    {
        if (g_pEffects[i]->IsDestroy())
        {
            delete g_pEffects[i];
            g_pEffects[i] = g_pEffects[g_EffectsCount - 1];
            g_pEffects[g_EffectsCount - 1] = nullptr; // 念のため
            g_EffectsCount--;
        }
    }
}

/** @brief 指定座標に弾丸ヒットエフェクトを生成 */
void BulletHItEffect_Create(const DirectX::XMFLOAT3 position)
{
    // 配列の上限チェック
    if (g_EffectsCount >= EFFECT_MAX)
        return;

    // ヒープに確保（修正点）
    g_pEffects[g_EffectsCount++] = new BulletHitEffect(position);
}

/** @brief 全エフェクトの描画 */
void BulletHItEffect_Draw()
{
    Direct3D_SetDepthWriteEnable(false);
    for (int i = 0; i < g_EffectsCount; i++)
    {
        g_pEffects[i]->Draw();
    }
    Direct3D_SetDepthWriteEnable(true);
}

/** @brief アニメーション再生完了時に破棄フラグを立てる */
void BulletHitEffect::Update()
{
    if (SpriteAnim_IsStopped(m_anim_play_id))
    {
        m_is_destroy = true;
    }
}

/** @brief ビルボードアニメーションとして描画 */
void BulletHitEffect::Draw() const { BillboardAnim_Draw(m_anim_play_id, m_position, { 1.0f, 1.0f }); }