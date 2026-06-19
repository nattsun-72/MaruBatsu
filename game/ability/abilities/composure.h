/****************************************
 * @file   composure.h
 * @brief  平静 — 思考時間 +15秒 (恒久・コモン)
 * @author Natsume Shidara
 * @date   2026/06/19
 *
 * 熟考(ContemplationAbility)の軽量版。序盤に拾える基礎コモンとして、
 * 加算量を控えめにした派生クラス。
 ****************************************/
#ifndef ABILITY_COMPOSURE_H
#define ABILITY_COMPOSURE_H

#include "ability/abilities/contemplation.h"

//======================================
// 平静アビリティ
//======================================
/**
 * @class  ComposureAbility
 * @brief  思考時間を +15秒する基礎コモンアビリティ
 * @detail ContemplationAbility を継承し、名前・説明・加算量のみ変更する。
 */
class ComposureAbility : public ContemplationAbility
{
public:
    ComposureAbility();
};

#endif // ABILITY_COMPOSURE_H
