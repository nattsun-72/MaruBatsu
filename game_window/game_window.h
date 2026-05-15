/****************************************
 * @file game_window.h
 * @brief ゲームウィンドウの生成と管理
 * @author Natsume Shidara
 * @date 2025/06/06
 ****************************************/

#ifndef GAME_WINDOW_H
#define GAME_WINDOW_H

#include <SDKDDKVer.h>
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

/** @brief ゲームウィンドウを生成する */
HWND GameWindow_Create(_In_ HINSTANCE hInstance);

/** @brief ボーダーレスウィンドウモードの切り替え */
void GameWindow_SetBorderless(bool borderless);

/** @brief ウィンドウハンドルの取得 */
HWND GameWindow_GetHWND();

#endif // GAME_WINDOW_H