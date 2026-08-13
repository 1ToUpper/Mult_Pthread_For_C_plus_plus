#include "../include/InfoLog.h"
#include "../include/FileOperator.h"
namespace LOGGER
{
PthreadPool MsgLog::s_pth_pools_(8, 4096, PthreadPool::RejectPolicy::DISCARD_OLDEST);
std::atomic<bool> MsgLog::init_flag_(true);
MsgLog* MsgLog::p_instance_ = nullptr;

MsgLog* MsgLog::get_instance()
{
    if(p_instance_ == nullptr)
    {
        p_instance_ = new MsgLog();
    }
    return p_instance_;
}
MsgLog::MsgLog():exchange_ready_(false) 
{
    if(MsgLog::init_flag_.load() == true)
    {
        MsgLog::init_flag_.store(false);
        Init_Run();
    }
};

void MsgLog::Init_Run()
{
    start_scan_logs_queue_pthread();
    start_write_dumping_logs_pthread();
}

std::string MsgLog::get_curren_time_ms()
{
    timeval tv;
    gettimeofday(&tv, nullptr);
    std::tm tm_info = *std::localtime(&tv.tv_sec);

    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_info);
    return std::string(buf) + "." +std::to_string(tv.tv_usec / 1000);
}

std::string MsgLog::format_string_impl(LogLevel level, const char* fmt, va_list args)
{
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
    
    std::string buff;
    buff.resize(len + 1);
    len = vsnprintf(&buff[0], len + 1, fmt, args);
    buff.resize(len);
    return get_curren_time_ms() + "\t" + level_str + " " + buff + "\n";
}

void MsgLog::log(LogLevel lv, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::string log_msg = format_string_impl(lv, fmt, args);
    va_end(args);

    s_pth_pools_.submit([log_msg, this]()
    {
        std::lock_guard<std::mutex> log_lock(log_buff_mtx_);
        log_buff_queue_.emplace_back(log_msg);
        cv_notify_exchange_queue_.notify_one();
    });

}

void MsgLog::start_scan_logs_queue_pthread()
{
    s_pth_pools_.submit([this]()
    {
        while(true)
        {
            std::unique_lock<std::mutex> log_lock(log_buff_mtx_);
            cv_notify_exchange_queue_.wait(log_lock, [this](){return (log_buff_queue_.size() >= WRITER_BUFF_SIZE && log_write_queue_.empty());});
            {
                log_buff_queue_.swap(log_write_queue_);
                exchange_ready_ = true;
                cv_notify_write_file_.notify_one();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
}

void MsgLog::start_write_dumping_logs_pthread()
{
    s_pth_pools_.submit([this]()
    {
        while(true)
        {
            std::unique_lock<std::mutex> lock(log_buff_mtx_);
            cv_notify_write_file_.wait(lock, [this](){return exchange_ready_;});
            exchange_ready_ = false;
            lock.unlock();
            for(size_t i = 0; i < log_write_queue_.size(); i++)
            {
                FileOperator::write_file_operation(log_write_queue_.at(i));
            }
            log_write_queue_.clear();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
}

}