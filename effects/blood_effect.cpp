/****************************************
 * @file blood_effect.cpp
 * @brief 敵斬撃時の血しぶきパーティクルエフェクト実装
 * @detail
 *   - BloodParticle: 重力影響・フェードアウト・縮小する赤色パーティクル
 *   - BloodEmitter: ワンショットバーストで一斉発射するエミッター
 *   - グローバル管理関数: BulletHitEffectと同じプール管理パターン
 * @author Natsume Shidara
 * @date 2026/02/18
 ****************************************/
#include "blood_effect.h"
#include "particle.h"
#include "billboard.h"
#include "direct3d.h"
#include "texture.h"
#ifdef USE_ASSET_DLL
#include "asset_dll.h"
#include "resource.h"
#endif

#include <DirectXMath.h>
#include <random>

using namespace DirectX;

// 定数
namespace BloodConfig
{
    // バースト設定
    constexpr int BURST_COUNT = 30;
    constexpr int MAX_SIMULTANEOUS = 64;

    // パーティクル物理
    constexpr float GRAVITY = 9.8f;
    constexpr float MIN_SPEED = 3.0f;
    constexpr float MAX_SPEED = 10.0f;
    constexpr float MIN_UPWARD = 0.2f;
    constexpr float MAX_UPWARD = 1.5f;

    // パーティクル外見
    constexpr float MIN_SCALE = 0.08f;
    constexpr float MAX_SCALE = 0.25f;
    constexpr float MIN_LIFETIME = 0.4f;
    constexpr float MAX_LIFETIME = 1.2f;
    constexpr float SHRINK_FACTOR = 0.5f;

    // 色範囲（暗赤色）
    constexpr float MIN_RED = 0.4f;
    constexpr float MAX_RED = 0.7f;
    constexpr float MIN_GREEN = 0.0f;
    constexpr float MAX_GREEN = 0.08f;
    constexpr float BLUE = 0.0f;

    // スポーンオフセット
    constexpr float SPAWN_RADIUS = 0.3f;

    // 放出レート（1フレームで全弾放出する疑似値）
    constexpr double EMISSION_RATE = 100000.0;
}

// BloodParticle
class BloodParticle : public Particle
{
private:
    int m_texId = -1;
    float m_scale = 1.0f;
    float m_initialScale = 1.0f;
    float m_alpha = 1.0f;
    XMFLOAT4 m_color{};

public:
    BloodParticle(int texId,
                  const XMVECTOR& position,
                  const XMVECTOR& velocity,
                  double lifeTime,
                  float scale,
                  const XMFLOAT4& color)
        : Particle(position, velocity, lifeTime)
        , m_texId(texId)
        , m_scale(scale)
        , m_initialScale(scale)
        , m_color(color)
    {
    }

    void Update(double elapsedTime) override
    {
        float dt = static_cast<float>(elapsedTime);

        // 重力を速度に加算
        AddVelocity(XMVectorSet(0.0f, -BloodConfig::GRAVITY * dt, 0.0f, 0.0f));

        // 速度による移動
        XMVECTOR deltaPos = XMVectorScale(GetVelocity(), dt);
        AddPosition(deltaPos);

        // ライフタイム比率（0.0 → 1.0）
        double lifeTime = GetLifeTime();
        float lifeRatio = (lifeTime > 0.0)
            ? std::fminf(static_cast<float>(GetAccumulatedTime() / lifeTime), 1.0f)
            : 1.0f;

        // フェードアウト
        m_alpha = 1.0f - lifeRatio;

        // 縮小（寿命終了時に初期サイズの50%まで）
        m_scale = m_initialScale * (1.0f - lifeRatio * BloodConfig::SHRINK_FACTOR);

        // 基底クラスのライフタイム管理
        Particle::Update(elapsedTime);
    }

    void Draw() const override
    {
        XMFLOAT3 pos;
        XMStoreFloat3(&pos, GetPosition());

        XMFLOAT4 drawColor = m_color;
        drawColor.w = m_alpha;

        Billboard_DrawBatch(m_texId, pos, { m_scale, m_scale }, { 0.0f, 0.0f }, drawColor);
    }
};

// BloodEmitter
class BloodEmitter : public Emitter
{
private:
    int m_texId = -1;
    std::mt19937 m_mt{ std::random_device{}() };
    int m_burstCount = 0;
    int m_emittedCount = 0;
    bool m_burstComplete = false;
    bool m_firstUpdateDone = false;

protected:
    Particle* CreateParticle() override
    {
        m_emittedCount++;

        // 乱数分布
        std::uniform_real_distribution<float> angleDist(-XM_PI, XM_PI);
        std::uniform_real_distribution<float> upDist(BloodConfig::MIN_UPWARD, BloodConfig::MAX_UPWARD);
        std::uniform_real_distribution<float> speedDist(BloodConfig::MIN_SPEED, BloodConfig::MAX_SPEED);
        std::uniform_real_distribution<float> lifeDist(BloodConfig::MIN_LIFETIME, BloodConfig::MAX_LIFETIME);
        std::uniform_real_distribution<float> scaleDist(BloodConfig::MIN_SCALE, BloodConfig::MAX_SCALE);
        std::uniform_real_distribution<float> redDist(BloodConfig::MIN_RED, BloodConfig::MAX_RED);
        std::uniform_real_distribution<float> greenDist(BloodConfig::MIN_GREEN, BloodConfig::MAX_GREEN);
        std::uniform_real_distribution<float> radiusDist(0.0f, BloodConfig::SPAWN_RADIUS);

        float angle = angleDist(m_mt);
        float upward = upDist(m_mt);
        float speed = speedDist(m_mt);

        // 射出方向（放射状・上方バイアス）
        XMVECTOR direction = XMVector3Normalize(XMVectorSet(
            cosf(angle),
            upward,
            sinf(angle),
            0.0f
        ));
        XMVECTOR velocity = XMVectorScale(direction, speed);

        // エミッター中心からわずかにオフセット
        float radius = radiusDist(m_mt);
        XMVECTOR offset = XMVectorSet(
            cosf(angle) * radius,
            0.0f,
            sinf(angle) * radius,
            0.0f
        );
        XMVECTOR particlePos = XMVectorAdd(GetPosition(), offset);

        float lifeTime = lifeDist(m_mt);
        float scale = scaleDist(m_mt);

        // 血赤色（パーティクル毎に微妙に変化）
        XMFLOAT4 color = {
            redDist(m_mt),
            greenDist(m_mt),
            BloodConfig::BLUE,
            1.0f
        };

        return new BloodParticle(m_texId, particlePos, velocity, lifeTime, scale, color);
    }

public:
    BloodEmitter(size_t capacity, const XMVECTOR& position, int burstCount)
        : Emitter(capacity, position, BloodConfig::EMISSION_RATE, true)
        , m_burstCount(burstCount)
    {
#ifdef USE_ASSET_DLL
        const void* pData = nullptr;
        size_t dataSize = 0;
        if (AssetDLL_GetData(IDR_TEX_EFFECT000, &pData, &dataSize))
            m_texId = Texture_LoadFromMemory(pData, dataSize, L"blood_effect");
#else
        m_texId = Texture_Load(L"assets/effect000.jpg");
#endif
    }

    void Update(double elapsedTime) override
    {
        // 基底クラスでパーティクル生成・更新・削除
        Emitter::Update(elapsedTime);

        // 初回フレーム後、発射を停止（バースト完了）
        if (!m_firstUpdateDone)
        {
            m_firstUpdateDone = true;
            Emit(false);
        }

        // 全パーティクルが消滅 → エフェクト完了
        if (!IsEmitting() && m_particleCount == 0 && m_emittedCount > 0)
        {
            m_burstComplete = true;
        }
    }

    bool IsBurstComplete() const { return m_burstComplete; }
};

// グローバル管理
static BloodEmitter* g_pBloodEffects[BloodConfig::MAX_SIMULTANEOUS]{};
static int g_BloodEffectCount = 0;

void BloodEffect_Initialize()
{
    for (int i = 0; i < BloodConfig::MAX_SIMULTANEOUS; i++)
    {
        g_pBloodEffects[i] = nullptr;
    }
    g_BloodEffectCount = 0;
}

void BloodEffect_Finalize()
{
    for (int i = 0; i < g_BloodEffectCount; i++)
    {
        delete g_pBloodEffects[i];
        g_pBloodEffects[i] = nullptr;
    }
    g_BloodEffectCount = 0;
}

void BloodEffect_Update(double elapsed_time)
{
    // 全エミッター更新
    for (int i = 0; i < g_BloodEffectCount; i++)
    {
        g_pBloodEffects[i]->Update(elapsed_time);
    }

    // 完了したエミッターを削除（swap-with-last）
    for (int i = g_BloodEffectCount - 1; i >= 0; i--)
    {
        if (g_pBloodEffects[i]->IsBurstComplete())
        {
            delete g_pBloodEffects[i];
            g_pBloodEffects[i] = g_pBloodEffects[g_BloodEffectCount - 1];
            g_pBloodEffects[g_BloodEffectCount - 1] = nullptr;
            g_BloodEffectCount--;
        }
    }
}

void BloodEffect_Draw()
{
    if (g_BloodEffectCount == 0) return;

    Direct3D_SetDepthWriteEnable(false);

    // バッチ描画: シェーダー/VB/トポロジー/ビルボード行列を1回だけ設定
    Billboard_BeginBatch();

    for (int i = 0; i < g_BloodEffectCount; i++)
    {
        g_pBloodEffects[i]->Draw();
    }

    Billboard_EndBatch();
    Direct3D_SetDepthWriteEnable(true);
}

void BloodEffect_Create(const DirectX::XMFLOAT3& position)
{
    if (g_BloodEffectCount >= BloodConfig::MAX_SIMULTANEOUS) return;

    XMVECTOR pos = XMLoadFloat3(&position);
    g_pBloodEffects[g_BloodEffectCount++] =
        new BloodEmitter(BloodConfig::BURST_COUNT, pos, BloodConfig::BURST_COUNT);
}
