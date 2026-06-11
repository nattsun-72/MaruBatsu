/****************************************
 * @file   view3d.h
 * @brief  簡易3Dビュー (斜め俯瞰カメラの透視投影)
 * @author Natsume Shidara
 * @date   2026/06/11
 *
 * 盤面を「やや斜め上から見下ろす」3D視点で描くための、
 * ソフトウェア透視投影モジュール。3D頂点(盤面セル座標+高さ)を
 * スクリーン座標へ変換し、既存の2Dスプライト描画でレンダリングする。
 * 専用の3Dパイプラインを持たないため軽量で、既存の描画基盤と
 * 完全に共存できる。view.use3D=false で従来の真上視点へ戻せる。
 *
 * 座標系: u=列方向(0..cols), v=行方向(0..rows), h=高さ(マス一辺=1)。
 * カメラは盤面中心を注視し、俯角・視野角・盤面の見かけ幅は
 * assets/game_config.json の view セクションで調整できる。
 ****************************************/
#ifndef VIEW3D_H
#define VIEW3D_H

//======================================
// 簡易3Dビュー 名前空間
//======================================
namespace View3D
{
    /**
     * @struct P2
     * @brief  投影後のスクリーン座標
     */
    struct P2
    {
        float x;   // スクリーンX
        float y;   // スクリーンY
    };

    /** @brief 3Dビューが有効か (設定 view.use3D) */
    bool Enabled();

    /**
     * @brief  カメラを盤面サイズと画面サイズに合わせて構成する
     * @detail 毎フレームの描画/判定前に呼ぶ (安価な三角関数のみ)。
     * @param  cols    盤面の横マス数
     * @param  rows    盤面の縦マス数
     * @param  screenW 画面幅(px)
     * @param  screenH 画面高さ(px)
     */
    void Setup(int cols, int rows, float screenW, float screenH);

    /**
     * @brief  盤面セル座標をスクリーン座標へ透視投影する
     * @param  u 列方向の位置 (0..cols。盤外も可)
     * @param  v 行方向の位置 (0..rows。盤外も可)
     * @param  h 盤面からの高さ (マス一辺=1)
     * @return スクリーン座標
     */
    P2 Project(float u, float v, float h = 0.0f);

    /**
     * @brief  位置 v における「1マスあたりのピクセル数」を返す
     * @detail 奥の行ほど小さくなる。線の太さや文字サイズの
     *         遠近スケーリングに使う。
     * @param  v 行方向の位置
     * @return ピクセル/マス
     */
    float Scale(float v);
}

#endif // VIEW3D_H
