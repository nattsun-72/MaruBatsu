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
#include <random>
#include <string>
#include <vector>

//--------------------------------------
// 内部状態
//--------------------------------------
namespace
{
    std::vector<std::shared_ptr<Ability>> g_PlayerAbilities;  // 所持アビリティ(累積)
    std::vector<std::shared_ptr<Ability>> g_RewardChoices;    // 報酬画面の3択
    int          g_BossIndex     = 0;     // 現在挑戦中のボスindex
    bool         g_RunCleared    = false; // 直近ランをクリアしたか(制覇表示用)
    double       g_RunTime       = 0.0;   // ランの累積プレイ時間(秒)
    int          g_RunTurns      = 0;     // ランの累積ターン数
    unsigned int g_BossOrderSeed = 0;     // 今ランのボス出現順シャッフル種(セーブで永続)
    RunResult    g_LastResult;            // 直近に確定した戦績(リザルト画面用)

    /** @brief ボス出現順シャッフル用の新しい乱数シードを生成する */
    unsigned int NewBossOrderSeed()
    {
        std::random_device rd;
        return static_cast<unsigned int>(rd());
    }

    // 直近に確定付与したボス報酬 (報酬画面の表示用)
    std::shared_ptr<Ability> g_LastBossReward;

    // 撃破ボスが提示する保留中のスキル (ボス撃破画面で獲得可否を選ぶ)
    std::shared_ptr<Ability> g_PendingBossReward;
    std::string              g_PendingBossName;
    std::string              g_PendingBossDesc;

    // メタ進行 (ラン跨ぎで永続。save/progress.dat)
    const char* PROGRESS_PATH      = "save/progress.dat";
    bool        g_ProgressLoaded   = false;
    bool        g_FinalBossDefeated = false;

    /** @brief 進行データを初回アクセス時に読み込む */
    void EnsureProgressLoaded()
    {
        if (g_ProgressLoaded) return;
        g_ProgressLoaded = true;
        std::ifstream ifs(PROGRESS_PATH, std::ios::binary);
        if (!ifs) return;
        int v = 0;
        ifs >> v;
        g_FinalBossDefeated = (v != 0);
    }

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
        // TAB区切り: 日時 \t cleared \t 撃破 \t 総数 \t 秒 \t ターン \t 敗北ボス \t アビリティ \t 引分
        // ※ 引分フラグは9列目として末尾追加。旧8列の履歴とも前方互換(欠落時は false 扱い)。
        ofs << r.dateTime               << "\t"
            << (r.cleared ? 1 : 0)      << "\t"
            << r.bossesDefeated         << "\t"
            << r.bossTotal              << "\t"
            << static_cast<long long>(r.timeSeconds) << "\t"
            << r.turns                  << "\t"
            << r.defeatedByBoss         << "\t"
            << abil                     << "\t"
            << (r.wasDraw ? 1 : 0)      << "\n";
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
        // (メタ進行 g_FinalBossDefeated はラン跨ぎで永続するため触らない)
        g_PlayerAbilities.clear();
        g_RewardChoices.clear();
        g_LastBossReward.reset();
        g_PendingBossReward.reset();
        g_PendingBossName.clear();
        g_PendingBossDesc.clear();
        g_BossIndex   = 0;
        g_RunCleared  = false;
        g_RunTime     = 0.0;
        g_RunTurns    = 0;

        // 新ランのボス出現順を確定する: 新しいシードでベース順を控えめにシャッフル。
        // ラン中はこの並びで固定され、SaveRun でシードごと保存される。
        g_BossOrderSeed = NewBossOrderSeed();
        BossRoster::ApplyRunSeed(g_BossOrderSeed);
    }

    //======================================
    // ラン統計
    //======================================
    void   AddRunTime(double dt) { g_RunTime += dt; }
    void   AddRunTurn()          { ++g_RunTurns; }
    double RunTime()             { return g_RunTime; }
    int    RunTurns()            { return g_RunTurns; }

    void CaptureResult(bool cleared, const std::string& defeatedByBoss, bool wasDraw)
    {
        g_LastResult.cleared        = cleared;
        g_LastResult.wasDraw        = (!cleared && wasDraw);   // クリア時は引分扱いしない
        g_LastResult.bossesDefeated = g_BossIndex;             // クリア時は BossCount に一致
        g_LastResult.bossTotal      = BossRoster::Count();
        g_LastResult.timeSeconds    = g_RunTime;
        g_LastResult.turns          = g_RunTurns;
        g_LastResult.defeatedByBoss = defeatedByBoss;
        g_LastResult.dateTime       = NowString();

        // クリア時はラスボス撃破を永続化し、初解放かどうかを記録する
        g_LastResult.legendaryUnlocked = cleared ? MarkFinalBossDefeated() : false;

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
            // 9列目(引分フラグ)は旧履歴に無いため、存在する場合のみ読む。
            if (f.size() >= 9) { try { r.wasDraw = (std::stoi(f[8]) != 0); } catch (...) {} }

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

    void GrantBossReward(std::shared_ptr<Ability> reward)
    {
        g_LastBossReward.reset();
        if (!reward) return;

        // 一度限りアビリティを既に所持しているなら付与しない(重複無意味)
        if (reward->unique)
        {
            for (const auto& a : g_PlayerAbilities)
            {
                if (a->name == reward->name) return;
            }
        }

        // 確定報酬として所持に追加し、表示用に控える
        g_PlayerAbilities.push_back(reward);
        g_LastBossReward = reward;
    }

    const std::shared_ptr<Ability>& LastBossReward() { return g_LastBossReward; }

    //======================================
    // 撃破ボスの固有スキル提示 (保留報酬)
    //======================================
    void SetPendingBossReward(std::shared_ptr<Ability> reward,
                              const std::string& bossName,
                              const std::string& bossDesc)
    {
        g_PendingBossReward = std::move(reward);
        g_PendingBossName   = bossName;
        g_PendingBossDesc   = bossDesc;
    }

    const std::shared_ptr<Ability>& PendingBossReward() { return g_PendingBossReward; }
    const std::string&              PendingBossName()   { return g_PendingBossName; }
    const std::string&              PendingBossDesc()   { return g_PendingBossDesc; }

    void AcceptPendingBossReward()
    {
        // 「はい」: 保留中スキルを所持へ加える (一度限り所持済みなら GrantBossReward が弾く)
        if (g_PendingBossReward) GrantBossReward(g_PendingBossReward);
        g_PendingBossReward.reset();
        g_PendingBossName.clear();
        g_PendingBossDesc.clear();
    }

    void DeclinePendingBossReward()
    {
        // 「いいえ」: 付与せず破棄する
        g_PendingBossReward.reset();
        g_PendingBossName.clear();
        g_PendingBossDesc.clear();
        g_LastBossReward.reset();
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

        // レジェンダリー未解放(ラスボス未撃破)なら、抽選候補から除外する
        if (!IsLegendaryUnlocked())
        {
            for (const auto& a : AbilityPool::CreateAll())
            {
                if (a->rarity == Rarity::Legendary) excludeNames.push_back(a->name);
            }
        }

        // 確定報酬とは別枠の3択をレアリティ重み付き抽選で生成する
        g_RewardChoices = AbilityPool::PickRandom(choiceCount, excludeNames);
    }

    //======================================
    // メタ進行
    //======================================
    bool IsLegendaryUnlocked()
    {
        EnsureProgressLoaded();
        return g_FinalBossDefeated;
    }

    bool MarkFinalBossDefeated()
    {
        EnsureProgressLoaded();
        const bool wasFirst = !g_FinalBossDefeated;
        g_FinalBossDefeated = true;
        if (wasFirst)
        {
            std::error_code ec;
            std::filesystem::create_directories(SAVE_DIR, ec);
            std::ofstream ofs(PROGRESS_PATH, std::ios::binary | std::ios::trunc);
            if (ofs) ofs << "1\n";
        }
        return wasFirst;
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

        // ヘッダ4行: ボス番号 / プレイ時間(秒) / ターン数 / ボス順シャッフル種
        ofs << g_BossIndex << "\n";
        ofs << static_cast<long long>(g_RunTime) << "\n";
        ofs << g_RunTurns << "\n";
        ofs << g_BossOrderSeed << "\n";
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

        // 4行目: ボス順シャッフル種 (新形式)。旧形式(3行ヘッダ)ではここが
        // アビリティ名のため、行全体が数値として読めなければ旧形式とみなし、
        // その行をアビリティ名として持ち越す(後方互換)。
        unsigned int seed     = 0;
        bool         haveSeed = false;
        std::string  carriedAbil;          // 旧形式で先読みしたアビリティ名
        if (readLine(line))
        {
            try
            {
                size_t pos = 0;
                const unsigned long v = std::stoul(line, &pos);
                if (pos == line.size())    // 行全体が数値 = シード
                {
                    seed     = static_cast<unsigned int>(v);
                    haveSeed = true;
                }
                else carriedAbil = line;   // 数値+余字 → アビリティ名とみなす
            }
            catch (...) { carriedAbil = line; }  // 非数値 = 旧形式のアビリティ名
        }

        // 以降: アビリティ名を順に復元 (旧形式は持ち越した行を先頭に)
        std::vector<std::shared_ptr<Ability>> loaded;
        if (!carriedAbil.empty())
        {
            if (auto a = AbilityPool::CreateByName(carriedAbil)) loaded.push_back(a);
        }
        while (readLine(line))
        {
            if (line.empty()) continue;
            if (auto a = AbilityPool::CreateByName(line)) loaded.push_back(a);
        }

        // ボス出現順を復元する: 種があればその並びを再現、無ければ(旧セーブ)
        // ベース順(JSON順そのもの)を使う = 従来挙動のまま。
        if (haveSeed) BossRoster::ApplyRunSeed(seed);
        else          BossRoster::UseBaseOrder();

        // 復元成功 — ラン状態へ反映
        g_BossIndex       = idx;
        g_RunTime         = runTime;
        g_RunTurns        = runTurns;
        g_BossOrderSeed   = seed;
        g_PlayerAbilities = std::move(loaded);
        g_RewardChoices.clear();
        g_LastBossReward.reset();
        g_PendingBossReward.reset();
        g_PendingBossName.clear();
        g_PendingBossDesc.clear();
        g_RunCleared      = false;
        return true;
    }

    void ClearSave()
    {
        std::error_code ec;
        std::filesystem::remove(SAVE_PATH, ec);
    }
}
