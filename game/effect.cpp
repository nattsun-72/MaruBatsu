/****************************************
 * @file effect.cpp
 * @brief 2Dエフェクト管理の実装
 * @author Natsume Shidara
 * @date 2025/07/04
 ****************************************/

#include "effect.h"

#include "audio.h"
#include "DirectXMath.h"
#include "sprite_anim.h"
#include "texture.h"
#ifdef USE_ASSET_DLL
#include "asset_dll.h"
#include "resource.h"
#endif

using namespace DirectX;

// 定数
namespace
{
    // アニメーション設定
    constexpr int ANIM_COLUMNS = 16;            // スプライトシート列数
    constexpr int ANIM_ROWS = 4;                // スプライトシート行数
    constexpr float ANIM_FRAME_INTERVAL = 0.01f; // フレーム間隔（秒）
    constexpr int ANIM_FRAME_SIZE = 256;        // フレームサイズ（ピクセル）

    // エフェクト描画サイズ
    constexpr float EFFECT_DRAW_SIZE = 64.0f;   // 描画幅・高さ（ピクセル）
}

struct Effect
{
    XMFLOAT2 position;
    bool isEnable;
    int sprite_anim_id;
};

static constexpr int EFFECT_MAX = 256;
static Effect g_Effects[EFFECT_MAX];

static int g_AnimPatternId = -1;
static int g_EffectTexId = -1;

static int g_EffectSoundId = -1;

void Effect_Initialize()
{
    for (Effect& effect : g_Effects)
    {
        effect.isEnable = false;
    }
#ifdef USE_ASSET_DLL
    {
        const void* pData = nullptr;
        size_t dataSize = 0;
        if (AssetDLL_GetData(IDR_TEX_EXPLOSION, &pData, &dataSize))
            g_EffectTexId = Texture_LoadFromMemory(pData, dataSize, L"effect_explosion");
    }
#else
    g_EffectTexId = Texture_Load(L"assets/Explosion.png");
#endif
    g_AnimPatternId = SpriteAnim_RegisterPattern(g_EffectTexId,
                                                 ANIM_COLUMNS, ANIM_ROWS, ANIM_FRAME_INTERVAL,
                                                 {ANIM_FRAME_SIZE, ANIM_FRAME_SIZE}, {0, 0}, false);

    // NOTE: hit.wav は現在アセットに存在しないため、無効なハンドル(-1)が返る
    // DLLモードでも同様に-1を返すようにする
    g_EffectSoundId = LoadAudio("assets/audio/hit.wav");
}

void Effect_Finalize()
{
    UnloadAudio(g_EffectSoundId);
}

void Effect_Update(double)
{
    for (Effect& effect : g_Effects)
    {
        if (!effect.isEnable) continue;

        if (SpriteAnim_IsStopped(effect.sprite_anim_id))
        {
            SpriteAnim_DestroyPlayer(effect.sprite_anim_id);
            effect.isEnable = false;
        }
    }
}

void Effect_Draw()
{
    for (Effect& effect : g_Effects)
    {
        if (!effect.isEnable) continue;

        SpriteAnim_Draw(effect.sprite_anim_id, effect.position.x, effect.position.y, EFFECT_DRAW_SIZE, EFFECT_DRAW_SIZE);
    }
}

void Effect_Create(const XMFLOAT2& position)
{
    for (Effect& effect : g_Effects)
    {
        if (effect.isEnable) continue;

        effect.isEnable = true;
        effect.position = position;
        effect.sprite_anim_id = SpriteAnim_CreatePlayer(g_AnimPatternId);

        PlayAudio(g_EffectSoundId, false);

        break;
    }
}
