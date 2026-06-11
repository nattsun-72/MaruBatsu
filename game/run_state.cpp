/****************************************
 * @file   run_state.cpp
 * @brief  ラン進行状態の実装
 * @author Natsume Shidara
 * @date   2026/05/15
 ****************************************/
#include "run_state.h"
#include "ability/ability_pool.h"
#include "boss/boss_roster.h"
#include "config.h"

#include <algorithm>
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

    // 直前に撃破したボスの固有報酬 (次の報酬候補へ1枠確保される)
    std::shared_ptr<Ability> g_PendingBossReward;

    // セーブ/履歴ファイル (実行ディレクトリ相対、BOMなしUTF-8テキスト)
    const char* SAVE_DIR     = "save";
    const char* SAVE_PATH    = "save/run.sav";
    const char* HISTORY_PATH = "save/history.tsv";   // 戦績履歴(1行1ラン、TAB区切り)

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

    /** @brief 文字列を区切り文字 sep で分割する */
    std::vector<std::string> Split(const std::string& s, char sep)
    {
        std::vector<std::string> out;
        std::string cur;
        for (char c : s)
        {
            if (c == sep) { out.push_back(cur); cur.clear(); }
            else          { cur.push_back(c); }
        }
        out.push_back(cur);
        return out;
    }

    /** @brief g_LastResult を履歴ファイルへ1行追記する (ラン終了時に呼ぶ) */
    void AppendHistory(const RunResult& r)
    {
        std::error_code ec;
        std::filesystem::create_directories(SAVE_DIR, ec);

        std::ofstream ofs(HISTORY_PATH, std::ios::binary | std::ios::app);
        if (!ofs) return;

        // 取得アビリティ名はカンマ連結 (名前にTAB/カンマは含まれない)
        std::string abil;
        for (size_t i = 0; i < r.abilityNames.size(); ++i)
        {
            if (i) abil += ",";
            abil += r.abilityNames[i];
        }
        // TAB区切り: 日時 \t cleared \t 撃破 \t 総数 \t 秒 \t ターン \t 敗北ボス \t アビリティ
        ofs << r.dateTime               << "\t"
            << (r.cleared ? 1 : 0)      << "\t"
            << r.bossesDefeated         << "\t"
            << r.bossTotal              << "\t"
            << static_cast<long long>(r.timeSeconds) << "\t"
            << r.turns                  << "\t"
            << r.defeatedByBoss         << "\t"
            << abil                     << "\n";
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
        g_PendingBossReward.reset();
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

        // 確定した戦績を履歴へ追記 (ラン終了の単一地点)
        AppendHistory(g_LastResult);
    }

    const RunResult& LastResult() { return g_LastResult; }

    std::vector<RunResult> LoadHistory()
    {
        std::vector<RunResult> all;

        std::ifstream ifs(HISTORY_PATH, std::ios::binary);
        if (!ifs) return all;   // 履歴なし

        std::string line;
        while (std::getline(ifs, line))
        {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;

            const auto f = Split(line, '\t');
            if (f.size() < 8) continue;   // 不正行はスキップ

            RunResult r;
            try
            {
                r.cleared        = (std::stoi(f[1]) != 0);
                r.bossesDefeated = std::stoi(f[2]);
                r.bossTotal      = std::stoi(f[3]);
                r.timeSeconds    = static_cast<double>(std::stoll(f[4]));
                r.turns          = std::stoi(f[5]);
            }
            catch (...) { continue; }     // 数値破損行はスキップ
            r.dateTime       = f[0];
            r.defeatedByBoss = f[6];
            if (!f[7].empty()) r.abilityNames = Split(f[7], ',');

            all.push_back(std::move(r));
        }

        // 新しい順に並べ替え、最大100件に制限
        std::vector<RunResult> recent;
        const size_t maxKeep = 100;
        for (size_t i = all.size(); i-- > 0; )
        {
            recent.push_back(all[i]);
            if (recent.size() >= maxKeep) break;
        }
        return recent;
    }

    void SetPendingBossReward(std::shared_ptr<Ability> reward)
    {
        g_PendingBossReward = std::move(reward);
    }

    void GenerateRewardChoices()
    {
        const int choiceCount = Config::GetInt("rules.rewardChoiceCount", 3);

        // 取得済みの「一度限り」アビリティ名を集め、抽選候補から除外する
        std::vector<std::string> excludeNames;
        for (const auto& a : g_PlayerAbilities)
        {
            if (a->unique) excludeNames.push_back(a->name);
        }

        g_RewardChoices.clear();

        /*--- 撃破ボスの固有報酬を1枠目に確保する ---*/
        // 一度限りで所持済みの場合は確保せず、通常抽選にフォールバック。
        if (g_PendingBossReward)
        {
            const bool excluded =
                std::find(excludeNames.begin(), excludeNames.end(),
                          g_PendingBossReward->name) != excludeNames.end();
            if (!excluded)
            {
                g_RewardChoices.push_back(g_PendingBossReward);
                excludeNames.push_back(g_PendingBossReward->name);  // 残枠との重複防止
            }
            g_PendingBossReward.reset();
        }

        /*--- 残り枠をレアリティ重み付き抽選で埋める ---*/
        const int remain = choiceCount - static_cast<int>(g_RewardChoices.size());
        if (remain > 0)
        {
            auto picked = AbilityPool::PickRandom(remain, excludeNames);
            for (auto& a : picked) g_RewardChoices.push_back(std::move(a));
        }
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
        g_PendingBossReward.reset();
        g_RunCleared      = false;
        return true;
    }

    void ClearSave()
    {
        std::error_code ec;
        std::filesystem::remove(SAVE_PATH, ec);
    }
}
