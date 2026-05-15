/****************************************
 * @file particle.cpp
 * @brief パーティクルシステムのエミッター実装
 * @author Natsume Shidara
 * @date 2026/02/04
 ****************************************/
#include "particle.h"

/** @brief エミッターの更新（パーティクル生成・更新・破棄） */
void Emitter::Update(double elapsedTime)
{
	m_accumulatedTime += elapsedTime;

	const double secPerParticle = 1.0 / m_particlesPerSecond;

	// 噴出処理
	while (m_accumulatedTime >= secPerParticle){
		if (m_particleCount >= m_particleCapacity) break;

		if (m_isEmit) {
			m_particles[m_particleCount++] = CreateParticle();
		}

		m_accumulatedTime -= secPerParticle;
	}

	// 全パーティクルの更新
	for(size_t i=0;i<m_particleCount;i++){
		m_particles[i]->Update(elapsedTime);
	}

	// 寿命切れパーティクルの削除（末尾スワップ方式）
	for (size_t i = m_particleCount; i > 0; --i) {
		size_t idx = i - 1;
		if (m_particles[idx]->IsDestroy()) {
			delete m_particles[idx];
			m_particles[idx] = m_particles[--m_particleCount];
		}
	}
}

/** @brief 全パーティクルの描画 */
void Emitter::Draw() const
{
	for (int i = 0; i < m_particleCount; i++) {
		m_particles[i]->Draw();
	}
}
