/****************************************
 * @file   boss_roster.cpp
 * @brief  ボスロスターの実装 (カタログ + 外部定義の出現順)
 * @author Natsume Shidara
 * @date   2026/06/05
 * @update 2026/06/19 - ボス出現順を game_config.json から差し替え可能に
 *
 * 全ボスのヘッダを include する唯一の集約点。
 *
 * 構成:
 *   ・カタログ(g_Catalog): 利用可能な全ボスの一覧。ここに1行足すと
 *     新ボスが「使える駒」として増える(難易度・セーブID・生成方法)。
 *   ・出現順(g_Order): 実際の1ラン進行順。game_config.json の
 *     "bossOrder"(コード名の配列)から構築する。未指定/空/全て不正なら
 *     カタログの定義順をそのまま使う(従来挙動へのフォールバック)。
 *
 * これにより、付属のGUIツール(tools/boss_order_editor.html)等で
 * "bossOrder" を編集するだけで、コードを触らずに順序を組み替えられる。
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
#include <string>
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

    // 実際の出現順 (カタログ添字の並び)。BuildOrder() で構築する。
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
     * @brief  出現順を構築する
     * @detail game_config.json の "bossOrder"(コード名配列)を順に読み、
     *         カタログに在るものだけを採用する。1つも採れなければ
     *         カタログ定義順をそのまま使う(従来挙動)。
     */
    void BuildOrder()
    {
        g_Order.clear();

        // "bossOrder[0]", "bossOrder[1]", ... を空になるまで読む。
        for (int i = 0; ; ++i)
        {
            const std::string path = "bossOrder[" + std::to_string(i) + "]";
            const std::string key  = Config::GetString(path.c_str(), "");
            if (key.empty()) break;            // 配列終端
            const int ci = FindInCatalog(key);
            if (ci >= 0) g_Order.push_back(ci); // 未知のコード名は読み飛ばす
        }

        // フォールバック: 有効な順序が無ければカタログ全体を定義順で使う。
        if (g_Order.empty())
        {
            for (int i = 0; i < g_CatalogCount; ++i) g_Order.push_back(i);
        }

        g_OrderBuilt = true;
    }

    /** @brief 未構築なら構築する (初回アクセス時の保険) */
    void EnsureOrder()
    {
        if (!g_OrderBuilt) BuildOrder();
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
        BuildOrder();
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
