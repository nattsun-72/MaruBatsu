/****************************************
 * @file debug_ostream.h
 * @brief デスクトップアプリのコンソールデバッグ用出力
 * @author Natsume Shidara
 * @date 2025/06/06
 *
 * OutputDebugStringAを利用したストリーム出力。
 * hal::dout でデバッグ出力が可能。
 * @note Shift_JISのみ対応
 ****************************************/

#ifndef DEBUG_OSTREAM_H
#define DEBUG_OSTREAM_H

#include <Windows.h>
#include <sstream>

namespace hal
{
    /** @brief OutputDebugStringAへ出力するストリームバッファ */
    class debugbuf : public std::basic_stringbuf<char, std::char_traits<char>>
    {
    public:
        virtual ~debugbuf()
        {
            sync();
        }

    protected:
        int sync()
        {
            OutputDebugStringA(str().c_str());
            str(std::basic_string<char>());
            return 0;
        }
    };

    /** @brief デバッグ出力用のostreamラッパー */
    class debug_ostream : public std::basic_ostream<char, std::char_traits<char>>
    {
    public:
        debug_ostream()
            : std::basic_ostream<char, std::char_traits<char>>(new debugbuf())
        {
        }

        ~debug_ostream() { delete rdbuf(); }
    };

    extern debug_ostream dout;
}
#endif // DEBUG_OSTREAM_H
