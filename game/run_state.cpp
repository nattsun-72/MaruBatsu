/****************************************
 * @file   run_state.cpp
 * @brief  ラン進行状態の実装
 * @author Natsume Shidara
 * @date   2026/05/15
 ****************************************/
#include "run_state.h"
#include "ability/ability_pool.h"
#include "boss/boss_roster.h"

#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

//--------------------------------------
// 内部状態
//--------------------------------------
namespace
{
    std::vector<std::shared_ptr<Ability>> g_PlayerAbilities;  // 所持アビリティ(累積)
    std::vector<std::shared_ptr<Ability>> g_RewardChoices;    // 報酬画面の3択
    int    g_BossIndex  = 0;        // 現在挑戦中のボスindex
    bool   g_RunCleared = false;    // 直近ランをクリアしたか(制覇表示用)
    double g_RunTime    = 0.0;      // ランの累積プレイ時間(秒)
    int    g_RunTurns   = 0;        // ランの累積ターン数
    RunResult g_LastResult;         // 直近に確定した戦績(リザルト画面用)

    // セーブファイル (実行ディレクトリ相対、BOMなしUTF-8テキスト)
    const char* SAVE_DIR  = "save";
    const char* SAVE_PATH = "save/run.sav";

    /** @brief 現在のローカル日時を "YYYY/MM/DD HH:MM" 形式で返す */
    std::string NowString()
    {
        std::time_t t = std::time(nullptr);
        std::tm     tm{};
        localtime_s(&tm, &t);
        char buf[32] = {};
        std::strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M", &tm);
        return std::string(buf);
    }
}

namespace RunState
{
    //======================================
    // アクセサ
    //======================================
    std::vector<std::shared_ptr<Ability>>& PlayerAbilities() { return g_PlayerAbilities; }
    std::vector<std::shared_ptr<Ability>>& RewardChoices()  { return g_RewardChoices;  }

    int  CurrentBossIndex() { return g_BossIndex; }
    void IncrementBoss()    { ++g_BossIndex; }

    int  BossCount()        { return BossRoster::Count(); }
    bool IsRunComplete()    { return g_BossIndex >= BossCount(); }

    bool IsRunCleared()     { return g_RunCleared; }
    void MarkRunCleared()   { g_RunCleared = true; }

    //======================================
    // ラン制御
    //======================================
    void ResetRun()
    {
        // 所持アビリティ・報酬候補・ボス進行・クリア表示・統計をすべて初期化
        g_PlayerAbilities.clear();
        g_RewardChoices.clear();
        g_BossIndex   = 0;
        g_RunCleared  = false;
        g_RunTime     = 0.0;
        g_RunTurns    = 0;
    }

    //======================================
    // ラン統計
    //======================================
    void   AddRunTime(double dt) { g_RunTime += dt; }
    void   AddRunTurn()          { ++g_RunTurns; }
    double RunTime()             { return g_RunTime; }
    int    RunTurns()            { return g_RunTurns; }

    void CaptureResult(bool cleared, const std::string& defeatedByBoss)
    {
        g_LastResult.cleared        = cleared;
        g_LastResult.bossesDefeated = g_BossIndex;             // クリア時は BossCount に一致
        g_LastResult.bossTotal      = BossRoster::Count();
        g_LastResult.timeSeconds    = g_RunTime;
        g_LastResult.turns          = g_RunTurns;
        g_LastResult.defeatedByBoss = defeatedByBoss;
        g_LastResult.dateTime       = NowString();

        // 取得アビリティ名のスナップショット(履歴記録・表示用)
        g_LastResult.abilityNames.clear();
        for (const auto& a : g_PlayerAbilities)
        {
            g_LastResult.abilityNames.push_back(a->name);
        }
    }

    const RunResult& LastResult() { return g_LastResult; }

    void GenerateRewardChoices()
    {
        // 取得済みの「一度限り」アビリティ名を集め、抽選候補から除外する
        std::vector<std::string> excludeNames;
        for (const auto& a : g_PlayerAbilities)
        {
            if (a->unique) excludeNames.push_back(a->name);
        }
        // アビリティプールから3つをランダム抽選して報酬候補とする
        g_RewardChoices = AbilityPool::PickRandom(3, excludeNames);
    }

    //======================================
    // セーブ／ロード
    //======================================
    bool HasSave()
    {
        std::error_code ec;
        return std::filesystem::exists(SAVE_PATH, ec);
    }

    void SaveRun()
    {
        // save/ ディレクトリを用意し、ボス番号と所持アビリティ名を書き出す
        std::error_code ec;
        std::filesystem::create_directories(SAVE_DIR, ec);

        std::ofstream ofs(SAVE_PATH, std::ios::binary | std::ios::trunc);
        if (!ofs) return;

        // ヘッダ3行: ボス番号 / プレイ時間(秒) / ターン数
        ofs << g_BossIndex << "\n";
        ofs << static_cast<long long>(g_RunTime) << "\n";
        ofs << g_RunTurns << "\n";
        // 以降: 所持アビリティ名(1行1個)
        for (const auto& a : g_PlayerAbilities)
        {
            ofs << a->name << "\n";
        }
    }

    bool LoadRun()
    {
        std::ifstream ifs(SAVE_PATH, std::ios::binary);
        if (!ifs) return false;

        // CRを除去して1行読むヘルパ
        auto readLine = [&](std::string& out) -> bool {
            if (!std::getline(ifs, out)) return false;
            if (!out.empty() && out.back() == '\r') out.pop_back();
            return true;
        };

        // ヘッダ3行: ボス番号 / プレイ時間(秒) / ターン数
        std::string line;
        if (!readLine(line)) return false;
        int    idx = 0;
        double runTime = 0.0;
        int    runTurns = 0;
        try { idx = std::stoi(line); }
        catch (...) { return false; }
        if (idx < 0 || idx >= BossRoster::Count()) return false;  // 不正なセーブ
        if (!readLine(line)) return false;
        try { runTime = static_cast<double>(std::stoll(line)); }
        catch (...) { return false; }
        if (!readLine(line)) return false;
        try { runTurns = std::stoi(line); }
        catch (...) { return false; }

        // 以降: アビリティ名を順に復元
        std::vector<std::shared_ptr<Ability>> loaded;
        while (readLine(line))
        {
            if (line.empty()) continue;
            if (auto a = AbilityPool::CreateByName(line)) loaded.push_back(a);
        }

        // 復元成功 — ラン状態へ反映
        g_BossIndex       = idx;
        g_RunTime         = runTime;
        g_RunTurns        = runTurns;
        g_PlayerAbilities = std::move(loaded);
        g_RewardChoices.clear();
        g_RunCleared      = false;
        return true;
    }

    void ClearSave()
    {
        std::error_code ec;
        std::filesystem::remove(SAVE_PATH, ec);
    }
}
