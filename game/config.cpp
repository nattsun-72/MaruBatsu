/****************************************
 * @file   config.cpp
 * @brief  ゲームパラメータ設定の実装 (小型JSONパーサ)
 * @author Natsume Shidara
 * @date   2026/06/11
 *
 * 外部ライブラリに依存しない最小限の再帰下降JSONパーサ。
 * オブジェクト/配列を平坦化し、"a.b.c" / "a.b[0]" 形式のパスで
 * 文字列マップへ格納する。// 行コメントを許容する(JSONC)。
 ****************************************/
#include "config.h"

#include "debug_ostream.h"

#include <cctype>
#include <fstream>
#include <sstream>
#include <unordered_map>

//--------------------------------------
// 内部状態・パーサ
//--------------------------------------
namespace
{
    const char* CONFIG_PATH = "assets/game_config.json";   // 設定ファイルの場所

    std::unordered_map<std::string, std::string> g_Values;  // パス→値(文字列)
    bool g_Loaded = false;                                   // 一度でも読込を試みたか

    /**
     * @struct Parser
     * @brief  JSONテキストを平坦化マップへ展開する再帰下降パーサ
     */
    struct Parser
    {
        const char* p;     // 現在位置
        const char* end;   // 終端
        bool        ok = true;

        /** @brief 空白と // 行コメントを読み飛ばす */
        void SkipWs()
        {
            while (p < end)
            {
                if (std::isspace(static_cast<unsigned char>(*p))) { ++p; }
                else if (*p == '/' && p + 1 < end && p[1] == '/')
                {
                    while (p < end && *p != '\n') ++p;   // 行末まで
                }
                else break;
            }
        }

        /** @brief 次の非空白が c なら消費して true */
        bool Match(char c)
        {
            SkipWs();
            if (p < end && *p == c) { ++p; return true; }
            return false;
        }

        /** @brief 文字列リテラルを読む (先頭の " にいる前提) */
        std::string ParseString()
        {
            ++p;   // 開き "
            std::string s;
            while (p < end && *p != '"')
            {
                if (*p == '\\' && p + 1 < end)
                {
                    ++p;
                    switch (*p)
                    {
                    case 'n':  s += '\n'; break;
                    case 't':  s += '\t'; break;
                    default:   s += *p;   break;   // \" \\ / など
                    }
                }
                else
                {
                    s += *p;
                }
                ++p;
            }
            if (p < end) ++p;   // 閉じ "
            return s;
        }

        /**
         * @brief  値を1つ解析し、スカラなら g_Values[path] へ格納する
         * @param  path 現在のドット区切りパス
         */
        void ParseValue(const std::string& path)
        {
            SkipWs();
            if (p >= end) { ok = false; return; }

            const char c = *p;
            if (c == '{')
            {
                /*--- オブジェクト: 各キーをパスへ連結して再帰 ---*/
                ++p;
                if (Match('}')) return;   // 空オブジェクト
                while (true)
                {
                    SkipWs();
                    if (p >= end || *p != '"') { ok = false; return; }
                    const std::string key = ParseString();
                    if (!Match(':')) { ok = false; return; }
                    ParseValue(path.empty() ? key : path + "." + key);
                    if (!ok) return;
                    if (Match(',')) continue;
                    if (Match('}')) return;
                    ok = false; return;
                }
            }
            else if (c == '[')
            {
                /*--- 配列: 添字付きパスで再帰 ---*/
                ++p;
                if (Match(']')) return;   // 空配列
                int i = 0;
                while (true)
                {
                    ParseValue(path + "[" + std::to_string(i) + "]");
                    ++i;
                    if (!ok) return;
                    if (Match(',')) continue;
                    if (Match(']')) return;
                    ok = false; return;
                }
            }
            else if (c == '"')
            {
                g_Values[path] = ParseString();
            }
            else
            {
                /*--- 数値 / true / false / null をそのまま文字列で保持 ---*/
                const char* s = p;
                while (p < end &&
                       (std::isalnum(static_cast<unsigned char>(*p))
                        || *p == '-' || *p == '+' || *p == '.'))
                {
                    ++p;
                }
                if (s == p) { ok = false; return; }   // 解釈不能文字
                g_Values[path] = std::string(s, p);
            }
        }
    };

    /** @brief 初回アクセス時に自動で読み込む */
    void EnsureLoaded()
    {
        if (!g_Loaded) Config::Reload();
    }
}

namespace Config
{
    //======================================
    // 読み込み
    //======================================
    bool Reload()
    {
        g_Loaded = true;
        g_Values.clear();

        /*--- ファイル全体を読み込む ---*/
        std::ifstream ifs(CONFIG_PATH, std::ios::binary);
        if (!ifs)
        {
            // ファイル無し → 全て既定値で動作 (兆候を残す)
            hal::dout << "[Config] not found: " << CONFIG_PATH
                      << " (using defaults)" << std::endl;
            return false;
        }
        std::ostringstream oss;
        oss << ifs.rdbuf();
        const std::string text = oss.str();

        /*--- 解析 (失敗したら中途半端な値を残さず全クリア) ---*/
        Parser ps{ text.c_str(), text.c_str() + text.size() };
        ps.ParseValue("");
        if (!ps.ok)
        {
            // 構文エラー → 全クリアして既定値へ。原因に気づけるよう警告を出す。
            g_Values.clear();
            hal::dout << "[Config] parse failed: " << CONFIG_PATH
                      << " (check JSON syntax; using defaults)" << std::endl;
            return false;
        }
        return true;
    }

    //======================================
    // 値の取得
    //======================================
    double GetDouble(const char* path, double def)
    {
        EnsureLoaded();
        auto it = g_Values.find(path);
        if (it == g_Values.end()) return def;
        try { return std::stod(it->second); }
        catch (...) { return def; }
    }

    int GetInt(const char* path, int def)
    {
        EnsureLoaded();
        auto it = g_Values.find(path);
        if (it == g_Values.end()) return def;
        try { return std::stoi(it->second); }
        catch (...) { return def; }
    }

    bool GetBool(const char* path, bool def)
    {
        EnsureLoaded();
        auto it = g_Values.find(path);
        if (it == g_Values.end()) return def;
        return (it->second == "true" || it->second == "1");
    }

    std::string GetString(const char* path, const std::string& def)
    {
        EnsureLoaded();
        auto it = g_Values.find(path);
        if (it == g_Values.end()) return def;
        return it->second;
    }
}
