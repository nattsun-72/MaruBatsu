/****************************************
 * @file   boss_roster.cpp
 * @brief  ボスロスターの実装 (カタログ + 外部定義の出現順)
 * @author Natsume Shidara
 * @date   2026/06/05
 * @update 2026/06/19 - ボス出現順を game_config.json から差し替え可能に
 * @update 2026/06/24 - 出現順をラン毎に控えめにシャッフル(JSON順をベースに軽く前後)
 *
 * 全ボスのヘッダを include する唯一の集約点。
 *
 * 構成:
 *   ・カタログ(g_Catalog): 利用可能な全ボスの一覧。ここに1行足すと
 *     新ボスが「使える駒」として増える(難易度・セーブID・生成方法)。
 *   ・ベース順(g_BaseOrder): game_config.json の "bossOrder"(コード名の配列)
 *     から構築する1ラン進行順の土台。未指定/空/全て不正ならカタログの
 *     定義順をそのまま使う(従来挙動へのフォールバック)。
 *   ・実出現順(g_Order): ベース順をラン毎にシードでシャッフルした結果。
 *     Create/CodeName/Difficulty はこれを参照する。シャッフルは "軽い隣接
 *     スワップ"(各ボスの移動は最大±1)で、JSON順から大きく崩さない。
 *     シードは RunState 側でラン毎に生成・セーブされ、つづきからで同じ並びを再現する。
 *
 * これにより、付属のGUIツール(tools/boss_order_editor.html)等で
 * "bossOrder" を編集すれば土台の順序を組み替えられ、"bossOrderShuffle" で
 * ラン毎のばらつき具合を調整できる(いずれもコードを触らずに済む)。
 ****************************************/
#include "boss/boss_roster.h"

//--------------------------------------
// 各ボスの具象クラス (このファイルだけが全ボスを知る)
//--------------------------------------
#include "boss/ice_board_boss.h"
#include "boss/mirror_warden_boss.h"
#include "boss/spiral_queen_boss.h"
#include "boss/chain_pulse_boss.h"
#include "boss/time_devourer_boss.h"
#include "boss/gravity_tyrant_boss.h"
#include "boss/engraved_one_boss.h"

#include "config.h"

#include <functional>
#include <random>
#include <string>
#include <utility>
#include <vector>

//--------------------------------------
// カタログ (利用可能な全ボス) と 出現順
//--------------------------------------
namespace
{
    /**
     * @struct CatalogEntry
     * @brief  1ボスぶんのカタログ定義 (利用可能なボスの素材)
     */
    struct CatalogEntry
    {
        int         difficulty;  // 難易度(★の数)
        const char* codeName;    // セーブ/順序定義で使う安定ID
        std::function<std::unique_ptr<Boss>()> factory;  // 生成関数
    };

    // 利用可能な全ボス。新ボスの追加はこの配列に1行足すだけ。
    // ※ codeName は tools/boss_order_editor.html のカタログとも一致させること。
    const CatalogEntry g_Catalog[] = {
        { 1, "ice",     [] () -> std::unique_ptr<Boss> { return std::make_unique<IceBoardBoss>();     } },
        { 2, "mirror",  [] () -> std::unique_ptr<Boss> { return std::make_unique<MirrorWardenBoss>();  } },
        { 2, "spiral",  [] () -> std::unique_ptr<Boss> { return std::make_unique<SpiralQueenBoss>();   } },
        { 3, "chain",   [] () -> std::unique_ptr<Boss> { return std::make_unique<ChainPulseBoss>();    } },
        { 3, "time",    [] () -> std::unique_ptr<Boss> { return std::make_unique<TimeDevourerBoss>();  } },
        { 4, "gravity", [] () -> std::unique_ptr<Boss> { return std::make_unique<GravityTyrantBoss>(); } },
        { 5, "final",   [] () -> std::unique_ptr<Boss> { return std::make_unique<EngravedOneBoss>();   } },
    };
    const int g_CatalogCount = static_cast<int>(sizeof(g_Catalog) / sizeof(g_Catalog[0]));

    // ベース出現順 (JSON "bossOrder" 由来。カタログ添字の並び)。
    std::vector<int> g_BaseOrder;
    // 実際の出現順 (ベース順をラン用にシャッフルした結果。Create等が参照する)。
    std::vector<int> g_Order;
    bool             g_OrderBuilt = false;

    /** @brief コード名からカタログ添字を引く (無ければ -1) */
    int FindInCatalog(const std::string& key)
    {
        for (int i = 0; i < g_CatalogCount; ++i)
        {
            if (key == g_Catalog[i].codeName) return i;
        }
        return -1;
    }

    /**
     * @brief  ベース出現順を構築する
     * @detail game_config.json の "bossOrder"(コード名配列)を順に読み、
     *         カタログに在るものだけを採用する。1つも採れなければ
     *         カタログ定義順をそのまま使う(従来挙動)。
     *         構築直後は実出現順(g_Order)もベース順で初期化する。
     */
    void BuildBaseOrder()
    {
        g_BaseOrder.clear();

        // "bossOrder[0]", "bossOrder[1]", ... を空になるまで読む。
        for (int i = 0; ; ++i)
        {
            const std::string path = "bossOrder[" + std::to_string(i) + "]";
            const std::string key  = Config::GetString(path.c_str(), "");
            if (key.empty()) break;                 // 配列終端
            const int ci = FindInCatalog(key);
            if (ci >= 0) g_BaseOrder.push_back(ci);  // 未知のコード名は読み飛ばす
        }

        // フォールバック: 有効な順序が無ければカタログ全体を定義順で使う。
        if (g_BaseOrder.empty())
        {
            for (int i = 0; i < g_CatalogCount; ++i) g_BaseOrder.push_back(i);
        }

        g_Order      = g_BaseOrder;   // ラン適用前の既定はベース順そのもの
        g_OrderBuilt = true;
    }

    /** @brief 未構築なら構築する (初回アクセス時の保険) */
    void EnsureOrder()
    {
        if (!g_OrderBuilt) BuildBaseOrder();
    }

    /**
     * @brief  ベース順を「軽い隣接スワップ」で控えめにシャッフルし g_Order を作る
     * @param  seed ラン固定のシャッフル種
     * @detail JSON順を大きく崩さない局所シャッフル。左から隣り合うボスを
     *         確率 swapChance で入れ替え、入れ替えた要素は再度動かさない。
     *         これにより各ボスの移動は最大±1マスに収まる("軽く前後")。
     *         pinFirst/pinLast で先頭(1戦目)・末尾(ラスボス)を固定できる。
     */
    void ShuffleInto(unsigned int seed)
    {
        g_Order = g_BaseOrder;

        const bool   enabled    = Config::GetBool  ("bossOrderShuffle.enabled",    true);
        const double swapChance = Config::GetDouble("bossOrderShuffle.swapChance", 0.5);
        const bool   pinFirst   = Config::GetBool  ("bossOrderShuffle.pinFirst",   false);
        const bool   pinLast    = Config::GetBool  ("bossOrderShuffle.pinLast",    true);

        const int n = static_cast<int>(g_Order.size());
        if (!enabled || n < 2) return;   // 無効/要素不足ならベース順のまま

        std::mt19937 rng(seed);
        std::uniform_real_distribution<double> roll(0.0, 1.0);

        // 入れ替え可能な隣接ペア (i, i+1) の範囲:
        //   pinFirst → index0 を動かさない → 先頭ペアは (1,2) から
        //   pinLast  → index n-1 を動かさない → 末尾ペアは (n-3, n-2) まで
        const int firstPair = pinFirst ? 1 : 0;
        const int lastPair  = (pinLast ? n - 3 : n - 2);
        for (int i = firstPair; i <= lastPair; ++i)
        {
            if (roll(rng) < swapChance)
            {
                std::swap(g_Order[i], g_Order[i + 1]);
                ++i;   // 入れ替えた要素は次ペアで再度動かさない(変位を±1に制限)
            }
        }
    }

    /** @brief 出現順の番号を [0, size-1] へ丸める (範囲外アクセスの保険) */
    int ClampOrder(int index)
    {
        const int n = static_cast<int>(g_Order.size());
        if (index < 0)  return 0;
        if (index >= n) return n - 1;
        return index;
    }
}

namespace BossRoster
{
    //======================================
    // 構築 / 再読込
    //======================================
    void Reload()
    {
        // Config::Reload() 後に呼ぶことで、最新の "bossOrder" を反映する。
        // (実出現順はベース順で初期化。ラン開始時に ApplyRunSeed でシャッフルする)
        BuildBaseOrder();
    }

    void ApplyRunSeed(unsigned int seed)
    {
        EnsureOrder();        // ベース順を確保してからシャッフルする
        ShuffleInto(seed);
    }

    void UseBaseOrder()
    {
        EnsureOrder();
        g_Order = g_BaseOrder;
    }

    //======================================
    // 問い合わせ (実出現順ベース)
    //======================================
    int Count()
    {
        EnsureOrder();
        return static_cast<int>(g_Order.size());
    }

    std::unique_ptr<Boss> Create(int index)
    {
        EnsureOrder();
        return g_Catalog[g_Order[ClampOrder(index)]].factory();
    }

    const char* CodeName(int index)
    {
        EnsureOrder();
        return g_Catalog[g_Order[ClampOrder(index)]].codeName;
    }

    int Difficulty(int index)
    {
        EnsureOrder();
        return g_Catalog[g_Order[ClampOrder(index)]].difficulty;
    }
}
