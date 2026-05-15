/****************************************
 * @file reticle.h
 * @brief 画面中央のレティクル（照準）表示
 * @author Natsume Shidara
 * @date 2026/02/22
 * @update 2026/02/22 - フィーバーティアに応じた色変化
 ****************************************/

#ifndef RETICLE_H
#define RETICLE_H

void Reticle_Initialize();
void Reticle_Finalize();

/**
 * @brief レティクル色の更新（フィーバーティアに応じた補間）
 * @param dt デルタタイム（秒）
 */
void Reticle_Update(float dt);

void Reticle_Draw();

#endif // RETICLE_H
