/****************************************
 * @file   commons.h
 * @brief  コモンアビリティ群 (基礎調整10種)
 * @author Natsume Shidara
 * @date   2026/06/25
 *
 * 既存コモン同様、思考時間の管理を中心とした低インパクトな調整枠。
 * 加算系は熟考(ContemplationAbility)、任意発動系は集中(FocusAbility)を
 * 基底に流用し、名前・数値だけを差し替えて実装する。
 *
 *   1. 心構え     +20秒/ターン (恒久)        [Contemplation派生]
 *   2. 平常心     +25秒/ターン (恒久)        [Contemplation派生]
 *   3. 一服       +40秒/ターン (恒久)        [Contemplation派生]
 *   4. 深謀遠慮   +50秒/ターン (恒久)        [Contemplation派生]
 *   5. 拾い読み   思考時間×1.2 (恒久)        [Contemplation派生 + 乗算]
 *   6. 底力       任意発動 +90秒  (×2回)      [Focus派生]
 *   7. 小休止     任意発動 +45秒  (×3回)      [Focus派生]
 *   8. 再考       任意発動 +180秒 (×1回)      [Focus派生]
 *   9. 早見え     任意発動 +120秒 (×2回)      [Focus派生]
 *  10. 見極め     任意発動 残時間を100秒へ (×2回) [Focus派生 + 補充上書き]
 ****************************************/
#ifndef ABILITY_COMMONS_H
#define ABILITY_COMMONS_H

#include "ability/abilities/contemplation.h"
#include "ability/abilities/focus.h"

//======================================
// 加算系 (熟考の数値違い)
//======================================
class ReadyMindCommon : public ContemplationAbility { public: ReadyMindCommon(); };  // 心構え +20
class CalmMindCommon  : public ContemplationAbility { public: CalmMindCommon();  };  // 平常心 +25
class BreatherCommon  : public ContemplationAbility { public: BreatherCommon();  };  // 一服   +40
class DeepPlanCommon  : public ContemplationAbility { public: DeepPlanCommon();  };  // 深謀遠慮 +50

//======================================
// 拾い読み — 思考時間を乗算で底上げ (熟考の判定だけ差し替え)
//======================================
class SkimCommon : public ContemplationAbility
{
public:
    double factor = 1.2;   // 思考時間の倍率
    SkimCommon();
    /** @brief targetSide のみ時間を factor 倍にする (加算ではなく乗算) */
    double GetTurnTimeSeconds(Piece player) override;
};

//======================================
// 任意発動・加算系 (集中の数値違い)
//======================================
class ReserveCommon    : public FocusAbility { public: ReserveCommon();    };  // 底力   +90 ×2
class ShortRestCommon  : public FocusAbility { public: ShortRestCommon();  };  // 小休止 +45 ×3
class ReconsiderCommon : public FocusAbility { public: ReconsiderCommon(); };  // 再考   +180 ×1
class QuickSightCommon : public FocusAbility { public: QuickSightCommon(); };  // 早見え +120 ×2

//======================================
// 見極め — 任意発動で残り時間を floor まで補充 (集中の発動だけ差し替え)
//======================================
class DiscernCommon : public FocusAbility
{
public:
    double floorSeconds = 100.0;   // 補充後の最低残り時間
    DiscernCommon();
    /** @brief 残り時間が floor 未満なら floor まで引き上げる */
    void Activate(GameState& state) override;
    /** @brief 発動可能か (チャージ有 かつ 残り時間が floor 未満) */
    bool CanActivate(const GameState& state) const override;
};

#endif // ABILITY_COMMONS_H
