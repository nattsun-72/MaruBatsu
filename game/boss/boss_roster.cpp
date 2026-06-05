/****************************************
 * @file   boss_roster.cpp
 * @brief  ボスロスターの実装
 * @author Natsume Shidara
 * @date   2026/06/05
 *
 * 全ボスのヘッダを include する唯一の集約点。ここのテーブル g_Roster に
 * 1行足すだけでボスを追加でき、総数・難易度・セーブIDが自動追従する。
 ****************************************/
#include "boss/boss_roster.h"

//--------------------------------------
// 各ボスの具象クラス (このファイルだけが全ボスを知る)
//--------------------------------------
#include "boss/ice_board_boss.h"
#include "boss/spiral_queen_boss.h"
#include "boss/chain_pulse_boss.h"
#include "boss/gravity_tyrant_boss.h"
#include "boss/engraved_one_boss.h"

#include <functional>

//--------------------------------------
// ロスターテーブル (難易度順)
//--------------------------------------
namespace
{
    /**
     * @struct Entry
     * @brief  1ボスぶんのロスター定義
     */
    struct Entry
    {
        int         difficulty;  // 難易度(★の数)
        const char* codeName;    // セーブ等で使う安定ID
        std::function<std::unique_ptr<Boss>()> factory;  // 生成関数
    };

    // ボス追加はこの配列に1行足すだけ。順序がそのままラン進行順になる。
    const Entry g_Roster[] = {
        { 1, "ice",     [] () -> std::unique_ptr<Boss> { return std::make_unique<IceBoardBoss>();     } },
        { 2, "spiral",  [] () -> std::unique_ptr<Boss> { return std::make_unique<SpiralQueenBoss>();   } },
        { 3, "chain",   [] () -> std::unique_ptr<Boss> { return std::make_unique<ChainPulseBoss>();    } },
        { 4, "gravity", [] () -> std::unique_ptr<Boss> { return std::make_unique<GravityTyrantBoss>(); } },
        { 5, "final",   [] () -> std::unique_ptr<Boss> { return std::make_unique<EngravedOneBoss>();   } },
    };
    const int g_Count = static_cast<int>(sizeof(g_Roster) / sizeof(g_Roster[0]));

    /** @brief 番号を [0, g_Count-1] の範囲へ丸める (範囲外アクセスの保険) */
    int Clamp(int index)
    {
        if (index < 0)        return 0;
        if (index >= g_Count) return g_Count - 1;
        return index;
    }
}

namespace BossRoster
{
    //======================================
    // 問い合わせ
    //======================================
    int Count()
    {
        return g_Count;
    }

    std::unique_ptr<Boss> Create(int index)
    {
        return g_Roster[Clamp(index)].factory();
    }

    const char* CodeName(int index)
    {
        return g_Roster[Clamp(index)].codeName;
    }

    int Difficulty(int index)
    {
        return g_Roster[Clamp(index)].difficulty;
    }
}
