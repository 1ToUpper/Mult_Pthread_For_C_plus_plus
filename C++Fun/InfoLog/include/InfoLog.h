#ifndef INFOLOG_H
#define INFOLOG_H

#include <sys/time.h>
#include <iostream>
#include <queue>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <future>
#include <mutex>
#include <ctime>
#include <chrono>
#include "PthreadPool.h"
#include <atomic>

const int g_log_level = 0;
const int FILE_MAX_NUM = 500;
const int WRITER_BUFF_SIZE = 100000;
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

}

#endif