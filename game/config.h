/****************************************
 * @file   config.h
 * @brief  ゲームパラメータ設定 (JSON読み込み)
 * @author Natsume Shidara
 * @date   2026/06/11
 *
 * assets/game_config.json から各種パラメータを読み込む。
 * デザイナーがコードを触らずに、思考時間・アビリティ効果量・
 * レアリティ重み・カメラ等を調整できるようにする。
 *
 * 対応形式: JSON + 行コメント(//) ※いわゆるJSONC。BOMなしUTF-8。
 * 値はドット区切りパス("rules.turnTimeSeconds" 等)で取得する。
 * ファイルが無い/壊れている場合は、各呼び出しの既定値が使われる。
 ****************************************/
#ifndef CONFIG_H
#define CONFIG_H

#include <string>

//======================================
// 設定 名前空間
//======================================
namespace Config
{
    /**
     * @brief  設定ファイルを再読み込みする
     * @detail タイトル表示のたびに呼ばれるため、ゲームを再起動せずに
     *         パラメータ調整を反映できる。
     * @return 読み込み+解析に成功したら true
     */
    bool Reload();

    /**
     * @brief  double値を取得
     * @param  path ドット区切りのキーパス (例 "rules.turnTimeSeconds")
     * @param  def  キーが無い/解釈不能な場合の既定値
     */
    double GetDouble(const char* path, double def);

    /** @brief int値を取得 @param path キーパス @param def 既定値 */
    int GetInt(const char* path, int def);

    /** @brief bool値を取得 (true/false/1/0) @param path キーパス @param def 既定値 */
    bool GetBool(const char* path, bool def);

    /** @brief 文字列値を取得 @param path キーパス @param def 既定値 */
    std::string GetString(const char* path, const std::string& def);
}

#endif // CONFIG_H
