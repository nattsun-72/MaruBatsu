/****************************************
 * @file   margin.h
 * @brief  余裕 — 思考時間 +60秒 (恒久・強コモン)
 * @author Natsume Shidara
 * @date   2026/05/15
 *
 * 熟考(ContemplationAbility)の加算量を強化した派生クラス。
 ****************************************/
#ifndef ABILITY_MARGIN_H
#define ABILITY_MARGIN_H

#include "ability/abilities/contemplation.h"

//======================================
// 余裕アビリティ
//======================================
/**
 * @class  MarginAbility
 * @brief  思考時間を +60秒する強コモンアビリティ
 * @detail ContemplationAbility を継承し、名前・説明・加算量のみ変更する。
 */
class MarginAbility : public ContemplationAbility
{
public:
    MarginAbility();
};

#endif // ABILITY_MARGIN_H
