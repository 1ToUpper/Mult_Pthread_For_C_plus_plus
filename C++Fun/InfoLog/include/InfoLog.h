#ifndef INFOLOG_H
#define INFOLOG_H

#include <sys/time.h>
#include <iostream>
#include <queue>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <mutex>
#include <ctime>
#include <chrono>

const int g_log_level = 0;
const int FILE_MAX_SIZE = 1024; //KB
const int FILE_MAX_NUM = 100;
//双缓冲区，大小512，建议比线程的1024小，否则线程池阻塞时，迟迟不交换队列
inline std::mutex log_buff_mtx_;
inline std::vector<std::string> log_buff_queue_;
inline std::vector<std::string> log_write_queue_;

namespace LOGGER
{
enum LogLevel
{
    DEBUG_ = 0,
    WARNING_ = 1,
    ERROR_ = 2
};
class MsgLog
{
public:
    static MsgLog* get_instance();
    ~MsgLog() { };
    void log(LogLevel lv, const char* fmt, ...);
private:
    /*
    * @name Init_Run()
    * @note 启动交换数据线程，仅在首次调用对象时调用一次，使用标志位置管理
    */
    void Init_Run();
    void start_write_dumping_logs_pthread();
    void start_scan_logs_queue_pthread();
    std::string format_string_impl(LogLevel level, const char* fmt, va_list args);
    std::string get_curren_time_ms();
private:
    static MsgLog* p_instance_;
    explicit MsgLog():exchange_ready_(false) { };
public:
    static PthreadPool s_pth_pools_;
    static std::atomic<bool> init_flag_;
private:
    bool exchange_ready_;
    std::condition_variable cv_notify_write_file_;
    std::condition_variable cv_notify_exchange_queue_;
};


inline std::string get_curren_time_ms()
{
    timeval tv;
    gettimeofday(&tv, nullptr);
    std::tm tm_info = *std::localtime(&tv.tv_sec);

    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_info);
    return std::string(buf) + "." +std::to_string(tv.tv_usec / 1000);
}

inline std::string format_string_impl(LogLevel level, const char* fmt, va_list args)
{
    //1. 处理日志等级
    std::string level_str;
    if (level >= g_log_level)
    {
        switch (level)
        {
            case DEBUG_:
                level_str = "[DEBUG]: ";
                break;
            case WARNING_:
                level_str = "[WARNING]: ";
                break;
            case ERROR_:
                level_str = "[ERROR]: ";
                break;
            default:
                level_str = "[UNKNOW]: ";
                break;
        }
    }
    else 
    {
        return "";
    }

    va_list args_copy;
    va_copy(args_copy, args);
    int len = vsnprintf(nullptr, 0, fmt, args_copy);
    va_end(args_copy);
    if (len < 0) return "";
    
    //3. 分配缓冲区并且实际格式化
    std::string buff;
    buff.resize(len + 1);
    len = vsnprintf(&buff[0], len + 1, fmt, args);
    buff.resize(len);
    return get_curren_time_ms() + "\t" + level_str + " " + buff + "\n";
}

__attribute__((format(printf, 2, 3)))
inline std::string format_string(LogLevel level, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::string result = format_string_impl(level, fmt, args);
    va_end(args);
    return result;
}
}

#endif