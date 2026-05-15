/****************************************
 * @file particle_test.h
 * @brief 汎用パーティクル・エミッターのテスト実装
 * @author Natsume Shidara
 * @date 2026/02/04
 ****************************************/
#ifndef PARTICLE_TEST_H
#define PARTICLE_TEST_H

#include "particle.h"
#include "texture.h"
#include <random>
#ifdef USE_ASSET_DLL
#include "asset_dll.h"
#include "resource.h"
#endif

/** @brief フェードアウト付き標準パーティクル */
class NormalParticle : public Particle {
private:
    int m_texId = -1;
    float m_scale = 1.0f;
    float m_alpha = 1.0f;

public:
    NormalParticle(int texId, const DirectX::XMVECTOR& position, const DirectX::XMVECTOR& velocity, double lifeTime)
        : Particle(position, velocity, lifeTime), m_texId(texId) {
    }

    void Update(double elapsedTime) override;
    void Draw() const override;
};

/** @brief 放射状にパーティクルを放出するテスト用エミッター */
class NormalEmitter : public Emitter {
private:
    

    int m_texId = -1;
    std::mt19937 m_mt{ std::random_device{}() };
    

protected:
    Particle* CreateParticle() override;

public:
    NormalEmitter(size_t capacity, const DirectX::XMVECTOR& position, double particlesPerSecond, bool isEmit = false)
        : Emitter(capacity, position, particlesPerSecond, isEmit)
    {
#ifdef USE_ASSET_DLL
        const void* pData = nullptr;
        size_t dataSize = 0;
        if (AssetDLL_GetData(IDR_TEX_EFFECT000, &pData, &dataSize))
            m_texId = Texture_LoadFromMemory(pData, dataSize, L"particle_effect");
#else
        m_texId = Texture_Load(L"assets/effect000.jpg");
#endif
    }
};

#endif // PARTICLE_TEST_H