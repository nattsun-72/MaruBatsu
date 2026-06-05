/****************************************
 * @file   run_state.cpp
 * @brief  ラン進行状態の実装
 * @author Natsume Shidara
 * @date   2026/05/15
 ****************************************/
#include "run_state.h"
#include "ability/ability_pool.h"
#include "boss/boss_roster.h"

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
    int  g_BossIndex   = 0;                                   // 現在挑戦中のボスindex
    bool g_RunCleared  = false;                               // 直近ランをクリアしたか(制覇表示用)

    // セーブファイル (実行ディレクトリ相対、BOMなしUTF-8テキスト)
    const char* SAVE_DIR  = "save";
    const char* SAVE_PATH = "save/run.sav";
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
        // 所持アビリティ・報酬候補・ボス進行・クリア表示をすべて初期化
        g_PlayerAbilities.clear();
        g_RewardChoices.clear();
        g_BossIndex   = 0;
        g_RunCleared  = false;
    }

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

        ofs << g_BossIndex << "\n";                 // 1行目: ボス番号
        for (const auto& a : g_PlayerAbilities)
        {
            ofs << a->name << "\n";                 // 以降: 所持アビリティ名(1行1個)
        }
    }

    bool LoadRun()
    {
        std::ifstream ifs(SAVE_PATH, std::ios::binary);
        if (!ifs) return false;

        // 1行目: ボス番号を解釈
        std::string line;
        if (!std::getline(ifs, line)) return false;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        int idx = 0;
        try { idx = std::stoi(line); }
        catch (...) { return false; }
        if (idx < 0 || idx >= BossRoster::Count()) return false;  // 不正なセーブ

        // 以降: アビリティ名を順に復元
        std::vector<std::shared_ptr<Ability>> loaded;
        while (std::getline(ifs, line))
        {
            if (!line.empty() && line.back() == '\r') line.pop_back();  // CR耐性
            if (line.empty()) continue;
            if (auto a = AbilityPool::CreateByName(line)) loaded.push_back(a);
        }

        // 復元成功 — ラン状態へ反映
        g_BossIndex       = idx;
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
