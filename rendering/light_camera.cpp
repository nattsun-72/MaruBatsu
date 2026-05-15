/****************************************
 * @file light_camera.cpp
 * @brief シャドウマップ生成用ライトカメラの実装
 * @author Natsume Shidara
 * @date 2025/12/10
 *
 * 平行光源方向からの正射影カメラを管理し、
 * LookToLH + OrthographicLH でビュー・プロジェクション行列を構築する。
 ****************************************/

#include "light_camera.h"

using namespace DirectX;

namespace
{
    // --- ライトカメラ初期設定 ---
    constexpr float DEFAULT_LIGHT_HEIGHT = 20.0f;        // ライトカメラの初期高さ
    constexpr float DEFAULT_SHADOW_PROJ_SIZE = 40.0f;    // 影の投影範囲（幅・高さ）
    constexpr float DEFAULT_SHADOW_FAR = 100.0f;         // 影の投影遠方クリップ距離
}

static XMFLOAT3 g_Position{ 0.0f, DEFAULT_LIGHT_HEIGHT, 0.0f }; // 初期位置も適当に入れておく
static XMFLOAT3 g_Front{ 0.0f, -1.0f, 0.0f };    // 初期値を「真下」にしてゼロを防ぐ
static XMFLOAT3 g_Up{ 0.0f, 0.0f, 1.0f };        // 真下を向くときのUpはY軸以外(Zなど)にする

// ビュー行列・プロジェクション行列のキャッシュ
static XMFLOAT4X4 g_ViewMatrix{};
static XMFLOAT4X4 g_ProjectionMatrix{};

// 影を生成する範囲
static float g_Width = DEFAULT_SHADOW_PROJ_SIZE;
static float g_Height = DEFAULT_SHADOW_PROJ_SIZE;
static float g_Near = 1.0f;
static float g_Far = DEFAULT_SHADOW_FAR;

/** @brief ライトカメラの初期化（方向と位置を設定し行列を計算） */
void LightCamera_Initialize(const DirectX::XMFLOAT3& world_directional, const DirectX::XMFLOAT3& world_Position)
{
    g_Front = world_directional;
    g_Position = world_Position;

    if (g_Front.x == 0 && g_Front.y == 0 && g_Front.z == 0) {
        g_Front = { 0.0f, -1.0f, 0.0f };
    }

    // 初期計算
    LightCamera_Update();
}

/** @brief 終了処理（現在は特に解放するリソースなし） */
void LightCamera_Finalize()
{
}

/** @brief ライトカメラの位置を更新し行列を再計算 */
void LightCamera_SetPosition(const DirectX::XMFLOAT3& position)
{
    g_Position = position;
    LightCamera_Update();
}

/** @brief ライトカメラの正面方向を更新し行列を再計算 */
void LightCamera_SetFront(const DirectX::XMFLOAT3& front)
{
    g_Front = front;

    if (g_Front.x == 0 && g_Front.y == 0 && g_Front.z == 0) {
        g_Front = { 0.0f, -1.0f, 0.0f };
    }

    LightCamera_Update();
}

/** @brief 正射影の範囲パラメータを設定し行列を再計算 */
void LightCamera_SetRange(float w, float h, float n, float f)
{
    g_Width = w;
    g_Height = h;
    g_Near = n;
    g_Far = f;
    LightCamera_Update();
}

/** @brief ビュー・プロジェクション行列を現在のパラメータから再計算 */
void LightCamera_Update()
{
    XMVECTOR pos = XMLoadFloat3(&g_Position);
    XMVECTOR dir = XMLoadFloat3(&g_Front);
    XMVECTOR up = XMLoadFloat3(&g_Up);

    // LookToLH は dir がゼロだとクラッシュ。
    // dirの長さがほぼ0なら計算しない、または正規化する処理を入れる
    if (XMVector3Equal(dir, XMVectorZero())) {
        dir = XMVectorSet(0, -1, 0, 0); // 強制リカバリ
    }
    dir = XMVector3Normalize(dir);

    // UpベクトルとDirが平行だと計算がおかしくなるので、Upを調整
    XMVECTOR cross = XMVector3Cross(dir, up);
    if (XMVector3Equal(cross, XMVectorZero())) {
        // 平行な場合、UpをX軸などにずらす
        up = XMVectorSet(1, 0, 0, 0);
    }

    XMMATRIX view = XMMatrixLookToLH(pos, dir, up);
    XMStoreFloat4x4(&g_ViewMatrix, view);

    XMMATRIX proj = XMMatrixOrthographicLH(g_Width, g_Height, g_Near, g_Far);
    XMStoreFloat4x4(&g_ProjectionMatrix, proj);
}

/** @brief ビュー行列を取得 */
const DirectX::XMFLOAT4X4& LightCamera_GetViewMatrix()
{
    return g_ViewMatrix;
}

/** @brief プロジェクション行列を取得 */
const DirectX::XMFLOAT4X4& LightCamera_GetProjectionMatrix()
{
    return g_ProjectionMatrix;
}