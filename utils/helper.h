/****************************************
 * @file helper.h
 * @brief ユーティリティ関数群
 * @author Natsume Shidara
 * @date 2025/06/06
 *
 * 乱数生成などの汎用ヘルパー関数を提供する。
 ****************************************/

#ifndef HELPER_H
#define HELPER_H

#include <random>
#include <type_traits>

namespace helper {

    /** @brief 指定範囲の整数乱数を生成 */
    inline int random_int(int min, int max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> distribution(min, max);
        return distribution(gen);
    }

    /** @brief 列挙型の範囲からランダムな値を返す */
    template<typename EnumType>
    EnumType random_enum(EnumType min = EnumType(0), EnumType max = EnumType::End) {
        static_assert(std::is_enum<EnumType>::value, "Template parameter must be an enum type");
        
        using Underlying = std::underlying_type_t<EnumType>;
        Underlying minVal = static_cast<Underlying>(min);
        Underlying maxVal = static_cast<Underlying>(max) - 1;
        Underlying value = static_cast<Underlying>(random_int(minVal, maxVal));
        return static_cast<EnumType>(value);
    }

}
#endif // HELPER_H
