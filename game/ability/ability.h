/****************************************
 * @file ability.h
 * @brief アビリティ基底クラス + レアリティ
 * @author Natsume Shidara
 * @date 2026/05/15
 ****************************************/
#ifndef ABILITY_H
#define ABILITY_H

#include <memory>
#include <string>

class AbilityRegistry;

enum class Rarity
{
    Common,
    Rare,
    Epic,
    Legendary,
    Curse,
};

/**
 * @brief アビリティ基底
 *
 * 各派生クラスは RegisterTo で 1つ以上のフックを registry に登録する。
 * 例: IceTileAbility は IPlacementHandler を継承し、自身を
 *     registry.AddPlacementHandler(self) する。
 */
class Ability : public std::enable_shared_from_this<Ability>
{
public:
    virtual ~Ability() = default;

    std::string name;
    std::string description;
    Rarity      rarity = Rarity::Common;

    /**
     * @brief 自身を1つ以上のフックとして registry に登録
     *
     * 派生クラスは IPlacementHandler / ITurnHandler 等を継承し、
     * shared_from_this() を dynamic_pointer_cast で取り出して登録する。
     */
    virtual void RegisterTo(AbilityRegistry& registry) = 0;
};

#endif // ABILITY_H
