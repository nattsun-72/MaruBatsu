/****************************************
 * @file slice_task_manager.cpp
 * @brief 非同期スライス処理の実装
 * @author Natsume Shidara
 * @date 2025/01/05
 * @update 2026/01/13 - planeNormalをSliceResultに引き継ぎ
 ****************************************/

#include "slice_task_manager.h"
#include "debug_ostream.h"
#include <algorithm>

using namespace DirectX;

namespace {
    constexpr int FALLBACK_WORKER_COUNT = 2;   // コア数取得失敗時のデフォルトワーカー数
    constexpr int RESERVED_CORES = 2;          // メイン・描画スレッド用の確保コア数
    constexpr int MAX_WORKER_THREADS = 10;     // スライスワーカースレッドの上限数
}

//--------------------------------------
// 静的メンバ変数の定義
//--------------------------------------
std::vector<std::thread> SliceTaskManager::s_WorkerThreads;
std::queue<SliceRequest> SliceTaskManager::s_RequestQueue;
std::queue<SliceResult> SliceTaskManager::s_ResultQueue;
std::mutex SliceTaskManager::s_RequestMutex;
std::mutex SliceTaskManager::s_ResultMutex;
std::condition_variable SliceTaskManager::s_RequestCondition;
std::atomic<bool> SliceTaskManager::s_ShouldTerminate(false);
std::atomic<int> SliceTaskManager::s_NextRequestId(1);
std::atomic<int> SliceTaskManager::s_PendingTaskCount(0);

// 初期化・終了

int SliceTaskManager::CalcOptimalWorkerCount()
{
    // std::thread::hardware_concurrency() でPCの論理コア数を取得
    // （内部的に Windows では GetLogicalProcessorInformation 等を使用）
    unsigned int hwThreads = std::thread::hardware_concurrency();

    // 取得失敗時（0が返る）は安全なデフォルト値
    if (hwThreads == 0)
    {
        OutputDebugStringA("[SliceTaskManager] hardware_concurrency() returned 0, using fallback: 2\n");
        return FALLBACK_WORKER_COUNT;
    }

    // メインスレッド + レンダリング等に最低2コアを確保し、残りの半分をスライス用に割り当て
    // 最低1、最大8スレッド
    int available = static_cast<int>(hwThreads) - RESERVED_CORES;  // メイン＋描画分を確保
    int optimal = std::max(1, available / 2);                      // 残りの半分を使用
    optimal = std::min(optimal, MAX_WORKER_THREADS);               // 上限キャップ

    return optimal;
}

void SliceTaskManager::Initialize(int workerThreadCount)
{
    s_ShouldTerminate = false;
    s_NextRequestId = 1;
    s_PendingTaskCount = 0;

    // 0以下なら自動検出
    if (workerThreadCount <= 0)
    {
        workerThreadCount = CalcOptimalWorkerCount();
    }

    // ワーカースレッドを起動
    for (int i = 0; i < workerThreadCount; ++i)
    {
        s_WorkerThreads.emplace_back(WorkerThreadFunction);
    }

    // ログ出力
    unsigned int hwThreads = std::thread::hardware_concurrency();
    std::string msg = "[SliceTaskManager] CPU logical cores: " + std::to_string(hwThreads)
                    + " -> Slice workers: " + std::to_string(workerThreadCount) + "\n";
    OutputDebugStringA(msg.c_str());
}

void SliceTaskManager::Finalize()
{
    // 終了フラグを立てる
    s_ShouldTerminate = true;
    s_RequestCondition.notify_all();

    // 全スレッドの終了を待機
    for (auto& thread : s_WorkerThreads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
    s_WorkerThreads.clear();

    // 未処理リクエストのクリーンアップ
    {
        std::lock_guard<std::mutex> lock(s_RequestMutex);
        while (!s_RequestQueue.empty())
        {
            SliceRequest& req = s_RequestQueue.front();
            ModelRelease(req.targetModel);
            s_RequestQueue.pop();
        }
    }

    // 未取得の結果のクリーンアップ
    {
        std::lock_guard<std::mutex> lock(s_ResultMutex);
        while (!s_ResultQueue.empty())
        {
            SliceResult& result = s_ResultQueue.front();
            ModelRelease(result.originalModel);
            s_ResultQueue.pop();
        }
    }

    OutputDebugStringA("[SliceTaskManager] Finalized\n");
}

// リクエスト送信

int SliceTaskManager::EnqueueSlice(const SliceRequest& request)
{
    int requestId = s_NextRequestId++;

    SliceRequest req = request;
    req.requestId = requestId;

    // モデルの参照カウントを増加（ワーカースレッドが使用するため）
    ModelAddRef(req.targetModel);

    {
        std::lock_guard<std::mutex> lock(s_RequestMutex);
        s_RequestQueue.push(req);
    }

    s_PendingTaskCount++;
    s_RequestCondition.notify_one();

    return requestId;
}

// 結果取得

bool SliceTaskManager::TryGetCompletedResult(SliceResult& outResult)
{
    std::lock_guard<std::mutex> lock(s_ResultMutex);

    if (s_ResultQueue.empty())
    {
        return false;
    }

    outResult = std::move(s_ResultQueue.front());
    s_ResultQueue.pop();
    s_PendingTaskCount--;

    return true;
}

/** @brief 結果を結果キューに戻す（他モジュール宛ての結果を返却する場合） */
void SliceTaskManager::PushBackResult(SliceResult&& result)
{
    std::lock_guard<std::mutex> lock(s_ResultMutex);
    s_PendingTaskCount++;
    s_ResultQueue.push(std::move(result));
}

// 状態取得

int SliceTaskManager::GetPendingTaskCount()
{
    return s_PendingTaskCount.load();
}

// ワーカースレッド処理

void SliceTaskManager::WorkerThreadFunction()
{
    while (!s_ShouldTerminate)
    {
        SliceRequest request;

        // リクエストキューから取得
        {
            std::unique_lock<std::mutex> lock(s_RequestMutex);

            // リクエストが来るまで待機
            s_RequestCondition.wait(lock, []() {
                return !s_RequestQueue.empty() || s_ShouldTerminate;
                                    });

            if (s_ShouldTerminate)
            {
                break;
            }

            if (s_RequestQueue.empty())
            {
                continue;
            }

            request = s_RequestQueue.front();
            s_RequestQueue.pop();
        }

        // スライス処理実行（CPUのみ、GPUリソース生成なし）
        SliceResult result;
        result.requestId = request.requestId;
        result.originalModel = request.targetModel;
        result.originalPosition = request.originalPosition;
        result.originalVelocity = request.originalVelocity;
        result.originalMass = request.originalMass;
        result.rootVolume = request.rootVolume;
        result.colliderType = request.colliderType;
        result.planeNormal = request.planeNormal;  // 切断平面法線を引き継ぎ

        // Slicer::SliceをCPUデータのみ取得する版で呼び出す
        result.success = Slicer::SliceCPUOnly(
            request.targetModel,
            request.worldMatrix,
            request.planePoint,
            request.planeNormal,
            result.frontMeshes,
            result.backMeshes
        );

        // 結果をキューに追加
        {
            std::lock_guard<std::mutex> lock(s_ResultMutex);
            s_ResultQueue.push(std::move(result));
        }
    }
}