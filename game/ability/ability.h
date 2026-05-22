/****************************************
 * @file   ability.h
 * @brief  アビリティ基底クラス + レアリティ定義
 * @author Natsume Shidara
 * @date   2026/05/15
 *
 * 全アビリティの共通基底。各アビリティはフック(IWinCondition 等)を
 * 継承し、RegisterTo で自身を AbilityRegistry に登録する。
 ****************************************/
#ifndef ABILITY_H
#define ABILITY_H

//--------------------------------------
// インクルード・前方宣言
//--------------------------------------
#include <memory>
#include <string>

class AbilityRegistry;  // 前方宣言
struct GameState;       // 前方宣言

//======================================
// レアリティ
//======================================
/**
 * @enum   Rarity
 * @brief  アビリティのレアリティ階層
 * @detail 報酬抽選の重み付けや、カード枠の色分けに使う。
 */
enum class Rarity
{
    Common,     // コモン (基礎調整)
    Rare,       // レア (ビルドの方向性を決める)
    Epic,       // エピック (シナジーの核)
    Legendary,  // レジェンダリー (ビルドの完成形)
    Curse,      // 呪い (ハイリスク・ハイリターン)
};

//======================================
// アビリティ基底クラス
//======================================
/**
 * @class  Ability
 * @brief  全アビリティの共通基底
 * @detail 各派生クラスは RegisterTo で1つ以上のフックを registry に登録する。
 *         例: IceTileAbility は IPlacementHandler を継承し、
 *         registry.AddPlacementHandler(self) で自身を登録する。
 *         enable_shared_from_this 継承により、RegisterTo 内で
 *         shared_from_this() から各フック型へ cast して登録できる。
 */
class Ability : public std::enable_shared_from_this<Ability>
{
public:
    virtual ~Ability() = default;

    //--------------------------------------
    // メンバ変数（アビリティ情報）
    //--------------------------------------
    std::string name;                          // アビリティ名
    std::string description;                   // 説明文 (報酬カード等に表示)
    Rarity      rarity      = Rarity::Common;  // レアリティ

    /// 任意発動(クリック発動)できるアビリティか。
    /// true のものはゲーム中にボタンとして表示され、クリックで Activate される。
    bool        activatable = false;

    //======================================
    // フック登録
    //======================================
    /**
     * @brief  自身を1つ以上のフックとして registry に登録する
     * @detail 派生クラスは IPlacementHandler / ITurnHandler 等を継承し、
     *         shared_from_this() を dynamic_pointer_cast で取り出して登録する。
     *         任意発動のみのアビリティはフック登録不要 (空実装でよい)。
     * @param  registry 登録先のレジストリ
     */
    virtual void RegisterTo(AbilityRegistry& registry) = 0;

    //======================================
    // 任意発動アビリティ用 (activatable == true のとき使用)
    //======================================
    /**
     * @brief  クリック発動時の効果
     * @detail state を直接書き換えてよい。既定実装は何もしない。
     * @param  state 試合状態
     */
    virtual void Activate(GameState& /*state*/) {}

    /**
     * @brief  現在発動可能かを返す
     * @detail チャージ切れ等で false を返すと、ボタンが無効表示になる。
     * @return 発動可能なら true
     */
    virtual bool CanActivate(const GameState& /*state*/) const { return true; }

    /**
     * @brief  残り使用回数を返す
     * @return 残り回数。-1 は回数制限なし(UIに回数を出さない)。
     */
    virtual int  ChargesLeft() const { return -1; }
};

#endif // ABILITY_H
