/****************************************
 * @file   boss_roster.h
 * @brief  ボスロスター — ボス番号からボスを生成するデータ駆動テーブル
 * @author Natsume Shidara
 * @date   2026/06/05
 *
 * ランの進行(ボス番号)に応じて、対応するボスを生成する集約点。
 * ボスの総数・難易度・セーブ用ID・生成方法を単一の情報源にまとめる。
 * 新しいボスの追加は boss_roster.cpp のテーブルに1行足すだけで済む。
 ****************************************/
#ifndef BOSS_ROSTER_H
#define BOSS_ROSTER_H

//--------------------------------------
// インクルード
//--------------------------------------
#include "boss/boss.h"

#include <memory>

//======================================
// ボスロスター 名前空間
//======================================
namespace BossRoster
{
    /**
     * @brief  ボス出現順(ベース)を game_config.json の "bossOrder" から再構築する
     * @detail Config::Reload() の直後に呼ぶ。未指定/空/全て不正なら
     *         カタログ定義順へフォールバックする。
     *         再構築直後の実出現順は、このベース順そのもの(未シャッフル)。
     *         ラン開始時に ApplyRunSeed() で控えめにシャッフルする。
     */
    void Reload();

    /**
     * @brief  ラン用のシードでベース順を控えめにシャッフルし、実出現順を確定する
     * @param  seed ランごとに固定するシャッフル種(セーブで永続)
     * @detail "bossOrderShuffle" 設定に従い、隣接スワップでJSON順を軽く前後
     *         させる(各ボスの移動は最大±1)。シャッフル無効時はベース順のまま。
     *         同じ (ベース順, seed, 設定) なら毎回同じ並びを再現する。
     */
    void ApplyRunSeed(unsigned int seed);

    /**
     * @brief  実出現順をベース順(JSON順そのもの)に戻す
     * @detail シャッフル種を持たない旧セーブの復元など、従来挙動が必要な場面で使う。
     */
    void UseBaseOrder();

    /** @brief 現在の出現順におけるボス総数 (=1ランの戦数) */
    int Count();

    /**
     * @brief  指定番号のボスを生成する
     * @param  index ボス番号 (0始まり)。範囲外は内部でクランプする。
     * @return 生成したボスの所有権付きポインタ
     */
    std::unique_ptr<Boss> Create(int index);

    /**
     * @brief  指定番号のボスの安定ID(セーブ用)を取得する
     * @param  index ボス番号 (0始まり)。範囲外はクランプ。
     * @return ロスター定義のコード名 (例 "ice")
     */
    const char* CodeName(int index);

    /**
     * @brief  指定番号のボスの難易度(★の数)を取得する
     * @param  index ボス番号 (0始まり)。範囲外はクランプ。
     * @return 難易度
     */
    int Difficulty(int index);
}

#endif // BOSS_ROSTER_H
