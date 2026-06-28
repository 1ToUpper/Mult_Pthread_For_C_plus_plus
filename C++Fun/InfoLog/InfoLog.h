#ifndef INFOLOG_H
#define INFOLOG_H

#include <queue>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>

const int g_log_level = 1;
const int FILE_MAX_SIZE = 1024; //KB
const int FILE_MAX_NUM = 100;

namespace LOGGER
{
enum LogLevel
{
    DEBUG_ = 0,
    WARNING_ = 1,
    ERROR_ = 2
};

__attribute__((format(printf, 1, 2)))
std::string format_string(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    //1. 计算传入缓冲区的大小
    int len = vsnprintf(nullptr, 0, fmt, args);
    va_end(args);
    if (len < 0) return {};
    //2. 分配缓冲区并且实际格式化
    std::string buff;
    buff.resize(len + 1);
    va_start(args, fmt);
    len = vsnprintf(&buff[0], len + 1, fmt, args);
    va_end(args);
    buff.resize(len);
    return buff;
}

class InfoLog
{

};

}



// template<typename... Args>
// class Logger
// {
// private:
//     std::string message_;
//     va_list args_;
// };

#endif