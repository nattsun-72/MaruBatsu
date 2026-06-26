/****************************************
 * @file   view3d.cpp
 * @brief  簡易3Dビューの実装
 * @author Natsume Shidara
 * @date   2026/06/11
 *
 * カメラモデル:
 *   - 盤面はXZ平面(原点=盤面中心)、Yが高さ。
 *   - カメラは盤面中心を注視し、ピッチ(俯角)のみ持つ (ロール/ヨー無し)。
 *   - 距離は「手前の辺の見かけ幅が view.boardScreenWidth になる」よう自動調整。
 *
 * 投影式 (ピッチ P, カメラ距離 D, 焦点距離 f):
 *   depth  = D - sinP*y - cosP*z      (カメラからの奥行き)
 *   xv     = x                        (ビュー空間X)
 *   yv     = cosP*y - sinP*z          (ビュー空間Y)
 *   screen = (cx + f*xv/depth, cy - f*yv/depth)
 ****************************************/
#include "view3d.h"
#include "config.h"

#include <cmath>

//--------------------------------------
// 内部状態 (Setup で構成)
//--------------------------------------
namespace
{
    constexpr float PI_F = 3.14159265358979323846f;

    bool  g_Enabled = true;    // 3Dビュー有効フラグ
    float g_Cols    = 3.0f;    // 盤面の横マス数
    float g_Rows    = 3.0f;    // 盤面の縦マス数
    float g_SinP    = 0.0f;    // sin(ピッチ)
    float g_CosP    = 0.0f;    // cos(ピッチ)
    float g_Focal   = 1000.0f; // 焦点距離(px)
    float g_Dist    = 8.0f;    // カメラ距離(マス単位)
    float g_CenterX = 800.0f;  // 盤面中心のスクリーンX
    float g_CenterY = 480.0f;  // 盤面中心のスクリーンY

    //--------------------------------------
    // 着手の衝撃 (盤面を着手点起点に減衰振動させる)
    //--------------------------------------
    bool   g_ImpactActive  = false;   // 衝撃が有効か
    float  g_ImpactX       = 0.0f;    // 起点(セル中心)の列座標
    float  g_ImpactY       = 0.0f;    // 起点(セル中心)の行座標
    double g_ImpactElapsed = 0.0;     // 衝撃開始からの経過(秒)

    constexpr double IMPACT_DURATION  = 0.36;   // 振動の継続時間(秒)
    constexpr float  IMPACT_AMPLITUDE = 0.18f;  // 起点での最大変位(マス)
    constexpr float  IMPACT_FREQUENCY = 30.0f;  // 振動の角周波数(rad/s)
    constexpr float  IMPACT_FALLOFF   = 0.6f;   // 起点からの距離減衰係数
}

namespace View3D
{
    //======================================
    // 構成
    //======================================
    bool Enabled()
    {
        return g_Enabled;
    }

    void Setup(int cols, int rows, float screenW, float screenH)
    {
        /*--- 設定値の取得 ---*/
        g_Enabled = Config::GetBool("view.use3D", true);
        const float pitchDeg = static_cast<float>(Config::GetDouble("view.cameraPitchDeg",    58.0));
        const float fovDeg   = static_cast<float>(Config::GetDouble("view.cameraFovDeg",      38.0));
        const float boardPx  = static_cast<float>(Config::GetDouble("view.boardScreenWidth", 560.0));
        const float centerYR = static_cast<float>(Config::GetDouble("view.boardCenterYRatio", 0.54));

        g_Cols = static_cast<float>(cols > 0 ? cols : 1);
        g_Rows = static_cast<float>(rows > 0 ? rows : 1);

        /*--- カメラパラメータの算出 ---*/
        const float pitch = pitchDeg * PI_F / 180.0f;
        g_SinP  = std::sin(pitch);
        g_CosP  = std::cos(pitch);
        g_Focal = (screenH * 0.5f) / std::tan(fovDeg * 0.5f * PI_F / 180.0f);

        // 手前の辺(z=+rows/2)の見かけ幅が boardPx になる距離へ自動調整する。
        //   cols * f / depthNear = boardPx  →  depthNear = cols*f/boardPx
        //   depthNear = D - cosP*(rows/2)   →  D = depthNear + cosP*rows/2
        const float depthNear = g_Cols * g_Focal / (boardPx > 1.0f ? boardPx : 1.0f);
        g_Dist = depthNear + g_CosP * (g_Rows * 0.5f);

        g_CenterX = screenW * 0.5f;
        g_CenterY = screenH * centerYR;
    }

    //======================================
    // 投影
    //======================================
    void SetImpact(float cellX, float cellY)
    {
        g_ImpactActive  = true;
        g_ImpactX       = cellX;
        g_ImpactY       = cellY;
        g_ImpactElapsed = 0.0;
    }

    void UpdateImpact(double dt)
    {
        if (!g_ImpactActive) return;
        g_ImpactElapsed += dt;
        if (g_ImpactElapsed >= IMPACT_DURATION) g_ImpactActive = false;
    }

    void ClearImpact()
    {
        g_ImpactActive = false;
    }

    /**
     * @brief  着手の衝撃による高さ変位を返す (起点が沈んで弾む減衰振動)
     * @detail 起点ほど大きく、距離で指数減衰。時間で k² 減衰しつつ振動する。
     */
    static float ImpactHeight(float u, float v)
    {
        if (!g_ImpactActive) return 0.0f;
        const float dx   = u - g_ImpactX;
        const float dz   = v - g_ImpactY;
        const float dist = std::sqrt(dx * dx + dz * dz);
        const float k    = 1.0f - static_cast<float>(g_ImpactElapsed / IMPACT_DURATION); // 1→0
        const float falloff = std::exp(-dist * IMPACT_FALLOFF);
        const float wave    = std::cos(static_cast<float>(g_ImpactElapsed) * IMPACT_FREQUENCY);
        // 開始直後(cos=1)は起点が沈む(マイナス)。以降は減衰しながら弾む。
        return -IMPACT_AMPLITUDE * k * k * falloff * wave;
    }

    P2 Project(float u, float v, float h)
    {
        /*--- 盤面セル座標 → ワールド座標 (盤面中心が原点) ---*/
        const float x = u - g_Cols * 0.5f;
        const float z = v - g_Rows * 0.5f;
        const float y = h + ImpactHeight(u, v);   // 着手の衝撃を高さへ加算

        /*--- ビュー変換 + 透視投影 ---*/
        float depth = g_Dist - g_SinP * y - g_CosP * z;
        if (depth < 0.1f) depth = 0.1f;   // 背面への発散を防ぐ保険
        const float yv = g_CosP * y - g_SinP * z;

        return { g_CenterX + g_Focal * x  / depth,
                 g_CenterY - g_Focal * yv / depth };
    }

    float Scale(float v)
    {
        const float z = v - g_Rows * 0.5f;
        float depth = g_Dist - g_CosP * z;
        if (depth < 0.1f) depth = 0.1f;
        return g_Focal / depth;   // 1マス(ワールド1.0)が何pxに見えるか
    }
}
