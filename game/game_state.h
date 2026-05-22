/****************************************
 * @file game_state.h
 * @brief 試合中の状態 (フック設計の引き渡し対象)
 * @author Natsume Shidara
 * @date 2026/05/15
 *
 * Board + 現プレイヤー + ターン残り時間 など、
 * 1試合のスナップショットをまとめる構造体。
 * 各フックは GameState を参照/書き換えできる。
 ****************************************/
#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "board.h"
#include "piece.h"

enum class MatchResult
{
    Playing,
    Win,      // プレイヤー勝利
    Lose,     // AI勝利
    Draw,     // 引き分け (盤面満杯)
    Timeout,  // 時間切れ
};

struct GameState
{
    Board       board;
    Piece       currentPlayer        = Piece::Player;
    double      remainingTime        = 0.0;
    int         turnCount            = 0;
    int         placementsRemaining  = 0;   // 現ターン残り着手数
    MatchResult result               = MatchResult::Playing;
    Piece       winner               = Piece::Empty;
};

#endif // GAME_STATE_H
