/****************************************
 * @file enemy_flying.h
 * @brief 飛行型敵（突進・体当たり型）
 * @detail
 *   - 空中をホバリングしながら旋回
 *   - プレイヤーに向かって突進攻撃
 *   - 攻撃後は上方へ撤退してクールダウン
 * @author Natsume Shidara
 * @date 2026/01/13
 ****************************************/

#ifndef ENEMY_FLYING_H
#define ENEMY_FLYING_H

#include "enemy.h"
#include "physics_model.h"
#include <DirectXMath.h>

 // 定数パラメータ
namespace EnemyFlyingConfig
{
    // 飛行パラメータ
    constexpr float HOVER_HEIGHT_MIN = 2.5f;       // 最低ホバリング高度（低めで威圧感）
    constexpr float HOVER_HEIGHT_MAX = 5.0f;       // 最高高度（プレイヤーに近い）
    constexpr float HOVER_AMPLITUDE = 0.8f;        // ホバリング振幅
    constexpr float HOVER_SPEED = 3.0f;            // ホバリング振幅速度（速くうねる）
    constexpr float HOVER_MOVE_SPEED = 5.0f;       // ホバリング中の旋回移動速度（素早い）

    // 突進パラメータ
    constexpr float CHARGE_SPEED = 35000.0f;       // 突進速度（爆速）
    constexpr float CHARGE_ACCELERATION = 2000.0f; // 突進加速度（一気に加速）
    constexpr float CHARGE_DISTANCE = 2.0f;        // 突進終了距離（ギリギリまで迫る）
    constexpr float CHARGE_WINDUP_TIME = 0.15f;    // 突進前の溜め（短い＝予備動作が少ない）
    constexpr float CHARGE_MAX_DURATION = 2.0f;    // 突進タイムアウト（短縮）

    // 退避パラメータ
    constexpr float RETREAT_SPEED = 18.0f;         // 撤退速度（素早く離脱）
    constexpr float RETREAT_HEIGHT = 5.0f;         // 撤退高度（低めで次の攻撃が早い）
    constexpr float RETREAT_DISTANCE = 8.0f;       // 撤退距離（近めで次の攻撃が早い）
    constexpr float RETREAT_DURATION = 0.35f;      // 撤退時間（短い＝すぐ復帰）

    // クールダウン
    constexpr float COOLDOWN_DURATION = 0.25f;     // 次の攻撃までの待機（ほぼ即攻撃）

    // 旋回パラメータ
    constexpr float CIRCLE_SPEED = 4.0f;           // 旋回角速度（速く回る＝威圧）
    constexpr float CIRCLE_RADIUS = 5.0f;          // 旋回半径（プレイヤーに近い）

    // 攻撃範囲
    constexpr float ATTACK_RANGE = 35.0f;          // 攻撃開始距離（遠くからでも突撃）

    // 物理パラメータ
    constexpr float MASS = 30.0f;
    constexpr float DRAG = 5.0f;                   // 空気抵抗

    // 切断パラメータ
    constexpr float DEATH_VOLUME_THRESHOLD = 0.95f;
    constexpr float SLICE_SEPARATION_SPEED = 8.0f;    // 分離速度（旧5.0f → 拡大で団子防止）
    constexpr float SLICE_OFFSET_DISTANCE = 0.4f;     // 位置オフセット（旧0.15f → 拡大で団子防止）
    constexpr float SLICE_TORQUE_STRENGTH = 10.0f;
    constexpr float SLICE_IMMUNITY_TIME = 0.8f;       // 衝突無効時間（旧0.1f → 延長で団子防止）

    // モデルパス（旧FBOXを使用）
    constexpr const char* MODEL_PATH = "assets/fbx/option_booster/option_booster2.fbx";
    constexpr const wchar_t* TEXTURE_PATH = L"assets/fbx/option_booster/drone_danmen.png";     // 横切り断面テクスチャ
    constexpr const wchar_t* TEXTURE_VERTICAL_PATH = nullptr;                                    // 縦切り断面テクスチャ（nullptr = 横切りで代用）
    constexpr float MODEL_SCALE = 1.8f;
}

// 前方宣言
struct SlicedFlyingEnemyParams;

// 飛行型敵クラス
class EnemyFlying : public Enemy
{
    friend EnemyFlying* EnemyFlying_CreateFromSlice(const SlicedFlyingEnemyParams& params);

public:
    //----------------------------------
    // 状態クラス（前方宣言）
    //----------------------------------
    class StateHover;      // ホバリング（待機・旋回）
    class StateWindup;     // 突進前の溜め
    class StateCharge;     // 突進
    class StateRetreat;    // 退避
    class StateCooldown;   // クールダウン

private:
    //----------------------------------
    // 物理モデル
    //----------------------------------
    PhysicsModel* m_pPhysics = nullptr;

    //----------------------------------
    // AI用
    //----------------------------------
    float m_DistanceToPlayer = 0.0f;
    float m_HoverTimer = 0.0f;           // ホバリング用タイマー
    float m_CircleAngle = 0.0f;          // 旋回角度
    DirectX::XMFLOAT3 m_ChargeTarget;    // 突進目標位置
    DirectX::XMFLOAT3 m_ChargeDirection; // 突進方向

public:
    //----------------------------------
    // コンストラクタ/デストラクタ
    //----------------------------------
    EnemyFlying(const DirectX::XMFLOAT3& position);
    ~EnemyFlying();

    // コピー禁止
    EnemyFlying(const EnemyFlying&) = delete;
    EnemyFlying& operator=(const EnemyFlying&) = delete;

    //----------------------------------
    // オーバーライド
    //----------------------------------
    bool IsDestroy() const override;

    //----------------------------------
    // PhysicsModelアクセサ
    //----------------------------------
    PhysicsModel* GetPhysicsModel() { return m_pPhysics; }
    const PhysicsModel* GetPhysicsModel() const { return m_pPhysics; }

    DirectX::XMFLOAT3 GetPosition() const;
    void SetPosition(const DirectX::XMFLOAT3& pos);
    DirectX::XMFLOAT3 GetVelocity() const;
    void SetVelocity(const DirectX::XMFLOAT3& vel);

    //----------------------------------
    // AI用アクセサ
    //----------------------------------
    float GetDistanceToPlayer() const { return m_DistanceToPlayer; }
    float& GetHoverTimer() { return m_HoverTimer; }
    float& GetCircleAngle() { return m_CircleAngle; }
    DirectX::XMFLOAT3& GetChargeTarget() { return m_ChargeTarget; }
    DirectX::XMFLOAT3& GetChargeDirection() { return m_ChargeDirection; }

    //----------------------------------
    // AI動作
    //----------------------------------
    void UpdateDistanceAndFacing();
    void ApplyHoverForce(float dt);
    void MoveToward(const DirectX::XMFLOAT3& target, float speed, float dt);
    void CircleAroundPlayer(float dt);

    //----------------------------------
    // 突進関連
    //----------------------------------
    void PrepareCharge();
    void ExecuteCharge(float dt);
    bool HasReachedChargeTarget() const;

    void ClampToTerrain();
};

// 状態クラス定義

class EnemyFlying::StateHover : public Enemy::State
{
private:
    float m_StateTimer = 0.0f;
public:
    StateHover(EnemyFlying* pOwner) : State(pOwner) {}
    void Enter() override;
    void Update(float dt) override;
    void Draw() const override;
    void DrawShadow() const override;
    void DrawDebug() const override;
};

class EnemyFlying::StateWindup : public Enemy::State
{
private:
    float m_StateTimer = 0.0f;
public:
    StateWindup(EnemyFlying* pOwner) : State(pOwner) {}
    void Enter() override;
    void Update(float dt) override;
    void Draw() const override;
    void DrawShadow() const override;
    void DrawDebug() const override;
};

class EnemyFlying::StateCharge : public Enemy::State
{
private:
    float m_StateTimer = 0.0f;
public:
    StateCharge(EnemyFlying* pOwner) : State(pOwner) {}
    void Enter() override;
    void Update(float dt) override;
    void Draw() const override;
    void DrawShadow() const override;
    void DrawDebug() const override;
};

class EnemyFlying::StateRetreat : public Enemy::State
{
private:
    float m_StateTimer = 0.0f;
    DirectX::XMFLOAT3 m_RetreatTarget;
public:
    StateRetreat(EnemyFlying* pOwner) : State(pOwner) {}
    void Enter() override;
    void Update(float dt) override;
    void Draw() const override;
    void DrawShadow() const override;
    void DrawDebug() const override;
};

class EnemyFlying::StateCooldown : public Enemy::State
{
private:
    float m_StateTimer = 0.0f;
public:
    StateCooldown(EnemyFlying* pOwner) : State(pOwner) {}
    void Enter() override;
    void Update(float dt) override;
    void Draw() const override;
    void DrawShadow() const override;
    void DrawDebug() const override;
};

// 共有リソース管理
void EnemyFlying_InitializeShared();
void EnemyFlying_FinalizeShared();

// 切断結果からの生成
struct SlicedFlyingEnemyParams
{
    std::vector<MeshData> meshes;
    MODEL* originalModel;
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 velocity;
    DirectX::XMFLOAT3 planeNormal;
    float mass;
    float rootVolume;
    ColliderType colliderType;
    bool isFrontSide;
};

EnemyFlying* EnemyFlying_CreateFromSlice(const SlicedFlyingEnemyParams& params);

#endif // ENEMY_FLYING_H
