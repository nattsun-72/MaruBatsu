/****************************************
 * @file boss.h
 * @brief ボス基底クラス
 * @author Natsume Shidara
 * @date 2026/05/15
 *
 * 各ボスは GetBossAbilities() で自身のギミック (ボス側強化版) を、
 * GetRewardAbility() で撃破時にプレイヤーが得る弱化版アビリティを返す。
 ****************************************/
#ifndef BOSS_H
#define BOSS_H

#include "ability/ability.h"

#include <memory>
#include <string>
#include <vector>

class Boss
{
public:
    virtual ~Boss() = default;

    std::string name;
    std::string description;
    int         difficulty = 1;    // ★の数

    /** @brief ボス側として動作するアビリティ一覧 */
    virtual std::vector<std::shared_ptr<Ability>> GetBossAbilities() = 0;

    /** @brief 撃破時にプレイヤーが獲得するアビリティ (弱化版) */
    virtual std::shared_ptr<Ability> GetRewardAbility() = 0;
};

#endif // BOSS_H
