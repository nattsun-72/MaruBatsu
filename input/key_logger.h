/****************************************
 * @file key_logger.h
 * @brief キーボード入力のトリガー・リリース判定
 * @author Natsume Shidara
 * @date 2025/06/06
 ****************************************/
#pragma once

#include "keyboard.h"

/** @brief キーロガー初期化 */
void KeyLogger_Initialize();

/** @brief キーロガー更新（毎フレーム呼び出し） */
void KeyLogger_Update();

/** @brief キーが押されているか */
bool KeyLogger_IsPressed(Keyboard_Keys key);

/** @brief キーが押された瞬間か（トリガー判定） */
bool KeyLogger_IsTrigger(Keyboard_Keys key);

/** @brief キーが離された瞬間か（リリース判定） */
bool KeyLogger_IsRelease(Keyboard_Keys key);
