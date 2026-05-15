/****************************************
 * @file player.cpp
 * @brief プレイヤー制御モジュール（ステートマシン対応版）
 * @author Natsume Shidara
 * @date 2025/10/31
 * @update 2026/01/10 - リファクタリング
 * @update 2026/01/13 - サウンド対応・デバッグ機能分離
 ****************************************/

#include "player.h"
#include "player_camera.h"
#include "camera.h"
#include "key_logger.h"
#include "pad_logger.h"
#include "mouse.h"
#include "input_manager.h"
#include "light.h"
#include "model.h"
#include "stage.h"
#include "blade.h"
#include "post_process.h"
#include "damage_effect.h"
#include "collider_generator.h"
#include "collision.h"
#include "sound_manager.h"
#include "fever.h"

#ifdef _DEBUG
#include "debug_renderer.h"
#endif

#include <DirectXMath.h>
#include <algorithm>
#include <cmath>

using namespace DirectX;

// 定数
namespace
{
    constexpr float MOVE_SPEED = 75.0f;
    constexpr float ATTACK_MOVE_SPEED = 50.0f;
    constexpr float AIR_CONTROL = 0.8f;
    constexpr float FRICTION_GROUND = 0.85f;
    constexpr float FRICTION_AIR = 0.85f;

    constexpr float GRAVITY = -9.8f * 2.5f;
    constexpr float JUMP_POWER = 12.0f;
    constexpr float DOUBLE_JUMP_POWER = 10.0f;
    constexpr int MAX_JUMP_COUNT = 2;

    constexpr float AIR_DASH_SPEED = 50.0f;
    constexpr float AIR_DASH_DURATION = 0.25f;
    constexpr float AIR_DASH_CHARGE_DURATION = 0.02f;
    constexpr int MAX_AIR_DASH_COUNT = 1;

    constexpr float FOV_NORMAL = 1.0f;
    constexpr float FOV_CHARGE = 1.0f;
    constexpr float FOV_DASH = 1.15f;
    constexpr float FOV_SPEED_FAST = 15.0f;
    constexpr float FOV_SPEED_NORMAL = 8.0f;

    constexpr float STEP_SPEED = 90.0f;
    constexpr float STEP_DURATION = 0.15f;
    constexpr float STEP_COOLDOWN = 0.125f;

    constexpr float ATTACK_DURATION = 0.3f;

    constexpr float COLLISION_EPSILON = 0.001f;
    constexpr int BASE_MAX_HP = 20;

    constexpr float PLAYER_MODEL_SCALE = 0.02f;          // プレイヤーモデルのスケール
    constexpr float INPUT_LENGTH_SQ_THRESHOLD = 0.01f;    // 入力ベクトル長の閾値（二乗）
    constexpr float BLUR_DURATION_MULTIPLIER = 2.5f;      // ブラー持続時間の倍率
    constexpr float STEP_BLUR_INTENSITY = 0.1f;           // ステップ時のブラー強度
    constexpr float RIGHT_TRIGGER_THRESHOLD = 0.5f;       // 右トリガーの入力閾値
    constexpr float MOVEMENT_ACCEL_MULTIPLIER = 10.0f;    // 移動加速度の係数
    constexpr float RISING_VELOCITY_THRESHOLD = 2.0f;     // 上昇判定の速度閾値
    constexpr float AIR_DASH_GROUND_CHECK_VEL = -0.5f;    // エアダッシュ中の接地判定速度閾値
    constexpr float SPECULAR_INTENSITY = 0.01f;           // スペキュラーライトの強度
    constexpr float SPECULAR_SHININESS = 0.3f;            // スペキュラーの光沢度
    constexpr float DAMAGE_CAMERA_SHAKE_POWER = 1.0f;     // ダメージ時のカメラシェイク強度
    constexpr int STATE_COLOR_COUNT = 8;                  // ステートカラーの数
    constexpr float DEBUG_FRONT_LINE_LENGTH = 2.0f;       // デバッグ用前方向ラインの長さ
    constexpr float DEBUG_DASH_LINE_LENGTH = 5.0f;        // デバッグ用ダッシュ方向ラインの長さ
}

static int g_MaxHP = BASE_MAX_HP;

// プレイヤーデータ
struct PlayerData
{
    XMFLOAT3 position;
    XMFLOAT3 velocity;
    XMFLOAT3 front;

    PlayerState state;
    float stateTimer;

    int jumpCount;
    int airDashCount;
    float stepCooldown;

    int hp;
    float invincibleTimer;

    XMFLOAT3 actionDir;
    bool grounded;
    bool wasGrounded;  // 前フレームの接地状態

    MODEL* pModel;
    OBB localOBB;
};

static PlayerData g_Player{};

// 内部関数プロトタイプ
static void ChangeState(PlayerState newState);
static bool CanChangeState(PlayerState newState);

static XMVECTOR GetMoveInput();
static bool IsJumpInput();
static bool IsAttackInput();
static bool IsStepInput();
static bool IsAirDashInput();
static bool HasMoveInput();

static void UpdateState_Ground(float dt, bool isWalking);
static void UpdateState_Attack(float dt);
static void UpdateState_Step(float dt);
static void UpdateState_Airborne(float dt);
static void UpdateState_AirDashCharge(float dt);
static void UpdateState_AirDash(float dt);

static void ApplyGravity(float dt);
static void ApplyMovement(float dt, float speed, float control = 1.0f);
static void ApplyFriction(bool grounded);
static void UpdateCollision(float dt);
static void UpdateRotation();

static bool TryTransitionCommon();
static bool TryTransitionAirborne();

// 初期化・終了
void Player_Initialize(const XMFLOAT3& position, const XMFLOAT3& front)
{
    g_Player = {};

    g_Player.position = position;
    XMStoreFloat3(&g_Player.front, XMVector3Normalize(XMLoadFloat3(&front)));

    g_Player.state = PlayerState::Idle;
    g_Player.jumpCount = MAX_JUMP_COUNT;
    g_Player.airDashCount = MAX_AIR_DASH_COUNT;
    g_MaxHP = BASE_MAX_HP;
    g_Player.hp = g_MaxHP;
    g_Player.actionDir = { 0.0f, 0.0f, 1.0f };
    g_Player.wasGrounded = true;

    g_Player.pModel = ModelLoad("assets/fbx/BOX.fbx", PLAYER_MODEL_SCALE);

    if (g_Player.pModel)
    {
        Collider col = ColliderGenerator::GenerateBestFit(g_Player.pModel);
        g_Player.localOBB = col.obb;
    }
}

void Player_Finalize()
{
    if (g_Player.pModel)
    {
        ModelRelease(g_Player.pModel);
        g_Player.pModel = nullptr;
    }
}

// 更新
void Player_Update(double elapsed_time)
{
    float dt = static_cast<float>(elapsed_time);

    // 前フレームの接地状態を保存
    g_Player.wasGrounded = g_Player.grounded;

    g_Player.stateTimer += dt;
    g_Player.stepCooldown = std::max(0.0f, g_Player.stepCooldown - dt);
    g_Player.invincibleTimer = std::max(0.0f, g_Player.invincibleTimer - dt);

    switch (g_Player.state)
    {
    case PlayerState::Idle:
    case PlayerState::Walk:
        UpdateState_Ground(dt, g_Player.state == PlayerState::Walk);
        break;
    case PlayerState::Attack:
        UpdateState_Attack(dt);
        break;
    case PlayerState::Step:
        UpdateState_Step(dt);
        break;
    case PlayerState::Jump:
    case PlayerState::Fall:
        UpdateState_Airborne(dt);
        break;
    case PlayerState::AirDashCharge:
        UpdateState_AirDashCharge(dt);
        break;
    case PlayerState::AirDash:
        UpdateState_AirDash(dt);
        break;
    }

    UpdateCollision(dt);
    UpdateRotation();

    // 着地SE（空中→地上に変わった瞬間）
    if (!g_Player.wasGrounded && g_Player.grounded)
    {
        SoundManager_PlaySE(SOUND_SE_LAND);
    }
}

// 状態遷移
static void ChangeState(PlayerState newState)
{
    if (g_Player.state == newState) return;

    g_Player.state = newState;
    g_Player.stateTimer = 0.0f;

    switch (newState)
    {
    case PlayerState::Jump:
        if (g_Player.grounded)
        {
            g_Player.velocity.y = JUMP_POWER;
            g_Player.jumpCount = Fever_GetMaxJumpCount() - 1;
            // ジャンプSE
        }
        else if (g_Player.jumpCount > 0)
        {
            g_Player.velocity.y = DOUBLE_JUMP_POWER;
            g_Player.jumpCount--;
            // 二段ジャンプSE
        }
        SoundManager_PlaySE(SOUND_SE_JUMP);
        g_Player.grounded = false;
        break;

    case PlayerState::AirDashCharge:
        if (g_Player.airDashCount > 0)
        {
            XMFLOAT3 camFront = PLCamera_GetFront();
            XMVECTOR vCamFront = XMVector3Normalize(XMLoadFloat3(&camFront));

            XMVECTOR input = GetMoveInput();
            if (XMVectorGetX(XMVector3LengthSq(input)) > INPUT_LENGTH_SQ_THRESHOLD)
            {
                XMVECTOR horizontal = XMVector3Normalize(input);
                float pitch = XMVectorGetY(vCamFront);
                XMVECTOR combined = XMVectorSetY(horizontal, pitch);
                XMStoreFloat3(&g_Player.actionDir, XMVector3Normalize(combined));
            }
            else
            {
                XMStoreFloat3(&g_Player.actionDir, vCamFront);
            }

            g_Player.velocity = { 0.0f, 0.0f, 0.0f };
            PLCamera_SetTargetFOV(FOV_CHARGE, FOV_SPEED_FAST);

            // ダッシュチャージSE
            SoundManager_PlaySE(SOUND_SE_DASH_CHARGE);
        }
        break;

    case PlayerState::AirDash:
        g_Player.airDashCount--;
        {
            XMVECTOR dashVel = XMLoadFloat3(&g_Player.actionDir) * AIR_DASH_SPEED;
            XMStoreFloat3(&g_Player.velocity, dashVel);
        }
        PLCamera_SetTargetFOV(FOV_DASH, FOV_SPEED_FAST);
        Blade_TriggerHorizontalSlash();
        PostProcess_StartRadialBlur(1.0f, AIR_DASH_DURATION * BLUR_DURATION_MULTIPLIER, true);

        // ダッシュSE
        SoundManager_PlaySE(SOUND_SE_DASH);
        break;

    case PlayerState::Step:
        g_Player.stepCooldown = STEP_COOLDOWN + STEP_DURATION;
        {
            XMVECTOR input = GetMoveInput();
            if (XMVectorGetX(XMVector3LengthSq(input)) > INPUT_LENGTH_SQ_THRESHOLD)
                XMStoreFloat3(&g_Player.actionDir, XMVector3Normalize(input));
            else
                XMStoreFloat3(&g_Player.actionDir, XMVectorNegate(XMLoadFloat3(&g_Player.front)));

            XMFLOAT3 camRight = PLCamera_GetRight();
            XMFLOAT3 camFront = PLCamera_GetFront();
            XMVECTOR vAction = XMLoadFloat3(&g_Player.actionDir);
            XMVECTOR vRight = XMVector3Normalize(XMVectorSetY(XMLoadFloat3(&camRight), 0.0f));
            XMVECTOR vFront = XMVector3Normalize(XMVectorSetY(XMLoadFloat3(&camFront), 0.0f));

            float screenX = XMVectorGetX(XMVector3Dot(vAction, vRight));
            float screenY = -XMVectorGetX(XMVector3Dot(vAction, vFront));
            PostProcess_StartDirectionalBlur({ screenX, screenY }, STEP_BLUR_INTENSITY, STEP_DURATION * BLUR_DURATION_MULTIPLIER);
        }

        // ステップSE
        SoundManager_PlaySE(SOUND_SE_STEP);
        break;

    case PlayerState::Attack:
        Blade_TriggerVerticalSlash();
        break;

    case PlayerState::Idle:
    case PlayerState::Walk:
        if (g_Player.grounded)
        {
            g_Player.jumpCount = Fever_GetMaxJumpCount();
            g_Player.airDashCount = Fever_GetMaxAirDashCount();
        }
        PLCamera_ResetFOV(FOV_SPEED_NORMAL);
        break;

    case PlayerState::Fall:
        PLCamera_ResetFOV(FOV_SPEED_NORMAL);
        break;
    }
}

static bool CanChangeState(PlayerState newState)
{
    if (newState == PlayerState::Step && g_Player.stepCooldown > 0.0f)
        return false;

    if ((newState == PlayerState::AirDash || newState == PlayerState::AirDashCharge) && g_Player.airDashCount <= 0)
        return false;

    if (newState == PlayerState::Jump && !g_Player.grounded && g_Player.jumpCount <= 0)
        return false;

    if (g_Player.state == PlayerState::AirDashCharge)
        return (newState == PlayerState::AirDash || newState == PlayerState::Step);

    return true;
}

// 入力処理
/** @brief カメラ基準の移動入力ベクトルを取得（キーボード＋パッド） */
static XMVECTOR GetMoveInput()
{
    XMVECTOR dir = XMVectorZero();
    XMVECTOR camFront = XMVector3Normalize(XMLoadFloat3(&PLCamera_GetFront()) * XMVectorSet(1, 0, 1, 0));
    XMVECTOR camRight = XMVector3Cross(XMVectorSet(0, 1, 0, 0), camFront);

    if (KeyLogger_IsPressed(KK_W)) dir += camFront;
    if (KeyLogger_IsPressed(KK_S)) dir -= camFront;
    if (KeyLogger_IsPressed(KK_D)) dir += camRight;
    if (KeyLogger_IsPressed(KK_A)) dir -= camRight;

    XMFLOAT2 stick = PadLogger_GetLeftThumbStick(0);
    if (isfinite(stick.x) && isfinite(stick.y))
    {
        constexpr float deadzone = 0.2f;
        if (fabsf(stick.x) > deadzone) dir += camRight * stick.x;
        if (fabsf(stick.y) > deadzone) dir += camFront * stick.y;
    }

    return dir;
}

static bool HasMoveInput()
{
    return XMVectorGetX(XMVector3LengthSq(GetMoveInput())) > INPUT_LENGTH_SQ_THRESHOLD;
}

static bool IsJumpInput()
{
    return KeyLogger_IsTrigger(KK_SPACE) || PadLogger_IsTrigger(0, XINPUT_GAMEPAD_A);
}

static bool IsAttackInput()
{
    static bool s_RightTriggerWasPressed = false;
    bool rightTriggerPressed = PadLogger_GetRightTrigger(0) > RIGHT_TRIGGER_THRESHOLD;
    bool rightTriggerTriggered = rightTriggerPressed && !s_RightTriggerWasPressed;
    s_RightTriggerWasPressed = rightTriggerPressed;

    const Mouse_State& ms = InputManager_GetMouseState();

    if (ms.leftButton) return true;
    if (PadLogger_IsTrigger(0, XINPUT_GAMEPAD_X)) return true;
    if (rightTriggerTriggered) return true;

    return false;
}

static bool IsStepInput()
{
    return KeyLogger_IsTrigger(KK_LEFTSHIFT) || PadLogger_IsTrigger(0, XINPUT_GAMEPAD_LEFT_SHOULDER);
}

static bool IsAirDashInput()
{
    if (PadLogger_IsTrigger(0, XINPUT_GAMEPAD_B)) return true;
    if (InputManager_IsMouseRightTrigger()) return true;
    return false;
}

// 状態遷移チェック
/** @brief 地上・空中共通の状態遷移チェック */
static bool TryTransitionCommon()
{
    if (IsJumpInput() && CanChangeState(PlayerState::Jump))
    {
        ChangeState(PlayerState::Jump);
        return true;
    }
    // 地上でもダッシュ攻撃（右クリック）
    if (IsAirDashInput() && CanChangeState(PlayerState::AirDashCharge))
    {
        ChangeState(PlayerState::AirDashCharge);
        return true;
    }
    if (IsStepInput() && CanChangeState(PlayerState::Step))
    {
        ChangeState(PlayerState::Step);
        return true;
    }
    if (IsAttackInput())
    {
        ChangeState(PlayerState::Attack);
        return true;
    }
    return false;
}

/** @brief 空中状態での状態遷移チェック */
static bool TryTransitionAirborne()
{
    if (IsJumpInput() && g_Player.jumpCount > 0 && CanChangeState(PlayerState::Jump))
    {
        ChangeState(PlayerState::Jump);
        return true;
    }
    if (IsAirDashInput() && CanChangeState(PlayerState::AirDashCharge))
    {
        ChangeState(PlayerState::AirDashCharge);
        return true;
    }
    if (IsStepInput() && CanChangeState(PlayerState::Step))
    {
        ChangeState(PlayerState::Step);
        return true;
    }
    if (IsAttackInput())
    {
        ChangeState(PlayerState::Attack);
        return true;
    }
    return false;
}

// 状態別更新
static void UpdateState_Ground(float dt, bool isWalking)
{
    if (TryTransitionCommon()) return;

    if (isWalking && !HasMoveInput())
    {
        ChangeState(PlayerState::Idle);
        return;
    }
    if (!isWalking && HasMoveInput())
    {
        ChangeState(PlayerState::Walk);
        return;
    }
    if (!g_Player.grounded)
    {
        ChangeState(PlayerState::Fall);
        return;
    }

    if (isWalking) ApplyMovement(dt, MOVE_SPEED);
    ApplyGravity(dt);
    ApplyFriction(true);
}

static void UpdateState_Attack(float dt)
{
    if (IsJumpInput() && CanChangeState(PlayerState::Jump))
    {
        ChangeState(PlayerState::Jump);
        return;
    }
    if (IsStepInput() && CanChangeState(PlayerState::Step))
    {
        ChangeState(PlayerState::Step);
        return;
    }

    if (g_Player.stateTimer >= ATTACK_DURATION)
    {
        ChangeState(g_Player.grounded ? PlayerState::Idle : PlayerState::Fall);
        return;
    }

    ApplyMovement(dt, ATTACK_MOVE_SPEED);
    ApplyGravity(dt);
    ApplyFriction(g_Player.grounded);
}

static void UpdateState_Step(float dt)
{
    if (g_Player.stateTimer >= STEP_DURATION)
    {
        ChangeState(g_Player.grounded ? PlayerState::Idle : PlayerState::Fall);
        return;
    }

    XMVECTOR stepVel = XMLoadFloat3(&g_Player.actionDir) * STEP_SPEED;
    float currentY = g_Player.velocity.y;
    XMStoreFloat3(&g_Player.velocity, stepVel);

    if (!g_Player.grounded)
    {
        g_Player.velocity.y = currentY;
        ApplyGravity(dt);
    }
}

static void UpdateState_Airborne(float dt)
{
    if (g_Player.state == PlayerState::Jump && g_Player.velocity.y <= 0.0f)
    {
        ChangeState(PlayerState::Fall);
        return;
    }

    if (g_Player.state == PlayerState::Fall && g_Player.grounded)
    {
        // 着地時にジャンプ回数をリセット（Idle遷移より先に回復させる）
        g_Player.jumpCount = Fever_GetMaxJumpCount();
        g_Player.airDashCount = Fever_GetMaxAirDashCount();
        ChangeState(PlayerState::Idle);
        return;
    }

    if (TryTransitionAirborne()) return;

    ApplyMovement(dt, MOVE_SPEED, AIR_CONTROL);
    ApplyGravity(dt);
    ApplyFriction(false);
}

static void UpdateState_AirDashCharge(float dt)
{
    (void)dt;

    if (g_Player.stateTimer >= AIR_DASH_CHARGE_DURATION)
    {
        ChangeState(PlayerState::AirDash);
        return;
    }

    if (IsStepInput() && CanChangeState(PlayerState::Step))
    {
        PLCamera_ResetFOV(FOV_SPEED_FAST);
        ChangeState(PlayerState::Step);
        return;
    }

    g_Player.velocity = { 0.0f, 0.0f, 0.0f };
    XMStoreFloat3(&g_Player.front, XMLoadFloat3(&g_Player.actionDir));
}

static void UpdateState_AirDash(float dt)
{
    (void)dt;

    if (g_Player.stateTimer >= AIR_DASH_DURATION)
    {
        ChangeState(g_Player.grounded ? PlayerState::Idle : PlayerState::Fall);
        return;
    }

    if (IsStepInput() && CanChangeState(PlayerState::Step))
    {
        ChangeState(PlayerState::Step);
        return;
    }

    XMVECTOR dashVel = XMLoadFloat3(&g_Player.actionDir) * AIR_DASH_SPEED;
    XMStoreFloat3(&g_Player.velocity, dashVel);

    Blade_TriggerHorizontalSlash();
}

// 物理処理
static void ApplyGravity(float dt)
{
    g_Player.velocity.y += GRAVITY * dt;
}

static void ApplyMovement(float dt, float speed, float control)
{
    XMVECTOR input = GetMoveInput();
    if (XMVectorGetX(XMVector3LengthSq(input)) > INPUT_LENGTH_SQ_THRESHOLD)
    {
        XMVECTOR accel = XMVector3Normalize(input) * speed * Fever_GetSpeedMultiplier() * control * dt * MOVEMENT_ACCEL_MULTIPLIER;
        XMVECTOR vel = XMLoadFloat3(&g_Player.velocity) + accel;
        XMStoreFloat3(&g_Player.velocity, vel);
    }
}

static void ApplyFriction(bool grounded)
{
    float f = grounded ? FRICTION_GROUND : FRICTION_AIR;
    g_Player.velocity.x *= f;
    g_Player.velocity.z *= f;
}

/** @brief 位置更新・地形衝突判定・プレイエリアクランプ */
static void UpdateCollision(float dt)
{
    XMVECTOR pos = XMLoadFloat3(&g_Player.position);
    XMVECTOR vel = XMLoadFloat3(&g_Player.velocity);
    pos += vel * dt;

    XMFLOAT3 newPos;
    XMStoreFloat3(&newPos, pos);

    XMFLOAT3 velF;
    XMStoreFloat3(&velF, vel);

    float terrainHeight = Stage_GetTerrainHeight(newPos.x, newPos.z);
    float playerFootY = newPos.y;

    // でこぼこ地形でも安定するように余裕を大きめに（0.3→0.6）
    constexpr float GROUND_CHECK_MARGIN = 0.6f;

    // 上昇中の判定：閾値を上げて微小な上向き速度では接地を外さない（0.5→2.0）
    bool isRising = velF.y > RISING_VELOCITY_THRESHOLD;
    bool isAirDashing = (g_Player.state == PlayerState::AirDash || g_Player.state == PlayerState::AirDashCharge);

    bool skipGroundCheck = isRising || (isAirDashing && velF.y > AIR_DASH_GROUND_CHECK_VEL);

    if (!skipGroundCheck)
    {
        if (playerFootY <= terrainHeight + GROUND_CHECK_MARGIN)
        {
            if (playerFootY < terrainHeight)
            {
                newPos.y = terrainHeight;
            }

            g_Player.grounded = true;

            if (velF.y < 0.0f)
            {
                velF.y = 0.0f;
                vel = XMLoadFloat3(&velF);
            }
        }
        else
        {
            g_Player.grounded = false;
        }
    }
    else
    {
        // 上昇中でも地面貫通は補正（grounded はジャンプ直後なので false のまま）
        g_Player.grounded = false;

        if (playerFootY < terrainHeight)
        {
            newPos.y = terrainHeight;
        }
    }

    pos = XMLoadFloat3(&newPos);

    XMStoreFloat3(&newPos, pos);
    XMFLOAT3 clampedPos = Stage_ClampToPlayArea(newPos);
    clampedPos.y = newPos.y;
    pos = XMLoadFloat3(&clampedPos);

    XMStoreFloat3(&g_Player.position, pos);
    XMStoreFloat3(&g_Player.velocity, vel);
}

/** @brief カメラYawからプレイヤーの前方向を更新 */
static void UpdateRotation()
{
    float yaw = PLCamera_GetYaw();
    g_Player.front = { sinf(yaw), 0.0f, cosf(yaw) };
}

// 描画
void Player_Draw()
{
    Light_SetSpecular({ SPECULAR_INTENSITY, SPECULAR_INTENSITY, SPECULAR_INTENSITY, 1.0f }, Camera_GetPosition(), SPECULAR_SHININESS);

    float angle = atan2f(-g_Player.front.x, -g_Player.front.z);
    XMMATRIX world = XMMatrixRotationY(angle) *
        XMMatrixTranslation(g_Player.position.x, g_Player.position.y, g_Player.position.z);

    ModelDraw(g_Player.pModel, world);
}

void Player_DrawShadow()
{
    float angle = atan2f(-g_Player.front.x, -g_Player.front.z);
    XMMATRIX world = XMMatrixRotationY(angle) *
        XMMatrixTranslation(g_Player.position.x, g_Player.position.y, g_Player.position.z);

    ModelDrawShadow(g_Player.pModel, world);
}

void Player_DrawDebug()
{
#ifdef _DEBUG
    DebugRenderer::DrawOBB(Player_GetWorldOBB(XMLoadFloat3(&g_Player.position)), { 0, 0.5f, 0.5f, 1 });

    static const XMFLOAT4 stateColors[] = {
        { 0.5f, 0.5f, 0.5f, 1.0f },
        { 0.0f, 1.0f, 0.0f, 1.0f },
        { 1.0f, 0.0f, 0.0f, 1.0f },
        { 1.0f, 1.0f, 0.0f, 1.0f },
        { 0.0f, 1.0f, 1.0f, 1.0f },
        { 0.5f, 0.0f, 0.5f, 1.0f },
        { 1.0f, 0.0f, 1.0f, 1.0f },
        { 1.0f, 0.5f, 0.0f, 1.0f },
    };

    int idx = static_cast<int>(g_Player.state);
    XMFLOAT4 color = (idx < STATE_COLOR_COUNT) ? stateColors[idx] : XMFLOAT4{ 1, 1, 1, 1 };

    XMFLOAT3 lineEnd;
    XMStoreFloat3(&lineEnd, XMLoadFloat3(&g_Player.position) + XMLoadFloat3(&g_Player.front) * DEBUG_FRONT_LINE_LENGTH);
    DebugRenderer::DrawLine(g_Player.position, lineEnd, color);

    if (g_Player.state == PlayerState::AirDashCharge || g_Player.state == PlayerState::AirDash)
    {
        XMFLOAT3 dashEnd;
        XMStoreFloat3(&dashEnd, XMLoadFloat3(&g_Player.position) + XMLoadFloat3(&g_Player.actionDir) * DEBUG_DASH_LINE_LENGTH);
        DebugRenderer::DrawLine(g_Player.position, dashEnd, { 1, 0, 0, 1 });
    }
#endif
}

// ゲッター
const XMFLOAT3& Player_GetPosition() { return g_Player.position; }
const XMFLOAT3& Player_GetFront() { return g_Player.front; }
const XMFLOAT3& Player_GetVelocity() { return g_Player.velocity; }
PlayerState Player_GetState() { return g_Player.state; }
int Player_GetHP() { return g_Player.hp; }
int Player_GetMaxHP() { return g_MaxHP; }
bool Player_IsGrounded() { return g_Player.grounded; }
int Player_GetJumpCount() { return g_Player.jumpCount; }
int Player_GetAirDashCount() { return g_Player.airDashCount; }

// セッター
void Player_SetPosition(const XMFLOAT3& position)
{
    g_Player.position = position;
}

/** @brief ワールド座標系でのプレイヤーOBBを取得 */
OBB Player_GetWorldOBB(const XMVECTOR& position)
{
    OBB obb = g_Player.localOBB;

    float angle = atan2f(-g_Player.front.x, -g_Player.front.z);
    XMVECTOR qRot = XMQuaternionRotationRollPitchYaw(0.0f, angle, 0.0f);

    XMVECTOR center = XMVector3Rotate(XMLoadFloat3(&obb.center), qRot) + position;
    XMStoreFloat3(&obb.center, center);

    XMVECTOR orient = XMQuaternionMultiply(XMLoadFloat4(&obb.orientation), qRot);
    XMStoreFloat4(&obb.orientation, orient);

    return obb;
}

// 外部操作
void Player_TakeDamage(int damage)
{
    g_Player.hp = std::max(0, g_Player.hp - damage);

    PLCamera_Shake(DAMAGE_CAMERA_SHAKE_POWER);
    DamageEffect_TriggerHitFlash();
    // ダメージSE
    SoundManager_PlaySE(SOUND_SE_PLAYER_DAMAGE);
}

void Player_RecoverAirDash(int amount)
{
    g_Player.airDashCount = std::min(g_Player.airDashCount + amount, Fever_GetMaxAirDashCount());
}

void Player_RecoverHP(int amount)
{
    g_Player.hp = std::min(g_Player.hp + amount, g_MaxHP);
}

void Player_IncreaseMaxHP(int amount)
{
    g_MaxHP += amount;
    // 最大HP増加時に現在HPも同量回復
    g_Player.hp = std::min(g_Player.hp + amount, g_MaxHP);
}
