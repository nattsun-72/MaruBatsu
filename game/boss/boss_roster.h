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
     * @brief  ボス出現順を game_config.json の "bossOrder" から再構築する
     * @detail Config::Reload() の直後に呼ぶ。未指定/空/全て不正なら
     *         カタログ定義順へフォールバックする。
     */
    void Reload();

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
