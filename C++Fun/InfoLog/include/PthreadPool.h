#include <iostream>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <functional>
#include <vector>
#include <thread>
#include <future>
#include <chrono>
#include <stdexcept>

class PthreadPool
{
public:
    enum class RejectPolicy
    {
        BLOCK,
        DISCARD_OLDEST,
        ABORT,
        CALLER_RUNS
    };

    PthreadPool() = delete;
    PthreadPool(PthreadPool& obj) = delete;
    PthreadPool(const PthreadPool& obj) = delete;
    PthreadPool& operator=(PthreadPool& obj) = delete;
    PthreadPool& operator=(const PthreadPool& obj) = delete;
    PthreadPool(PthreadPool&& obj) = delete;
    PthreadPool(const PthreadPool&& obj) = delete;

    PthreadPool(unsigned int size, size_t queue_size = 1024, RejectPolicy policy = RejectPolicy::BLOCK);
    ~PthreadPool();

    void set_reject_policy(RejectPolicy policy)
    {
        std::lock_guard<std::mutex> lock(queue_mtx_);
        reject_policy_ = policy;
    }

    RejectPolicy reject_policy()
    {
        std::lock_guard<std::mutex> lock(queue_mtx_);
        return reject_policy_;
    }

    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))>
    {
        using ReturnType = decltype(f(args...));
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<ReturnType> res = task->get_future();
        std::unique_lock<std::mutex> lock(queue_mtx_);

        if (stop_)
        {
            throw std::runtime_error("submit on stopped ThreadPool");
        }

        while (tasks_.size() >= max_queue_size_)
        {
            switch (reject_policy_)
            {
                case RejectPolicy::BLOCK:
                    cv_not_full_.wait(lock);
                    break;
                case RejectPolicy::DISCARD_OLDEST:
                    tasks_.pop_front();
                    break;
                case RejectPolicy::ABORT:
                    throw std::runtime_error("ThreadPool queue is full");
                case RejectPolicy::CALLER_RUNS:
                    lock.unlock();
                    (*task)();
                    return res;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        tasks_.emplace_back([task]() { (*task)(); });
        lock.unlock();
        cv_not_empty_.notify_one();
        return res;
    }

    void log(LOGGER::LogLevel lv, const char* fmt, ...)
    {
        std::unique_lock<std::mutex> lock(queue_mtx_);
        if(stop_)
        {
            throw std::runtime_error("logger on stopped ThreadPool");
        }

        va_list args;
        va_start(args, fmt);
        std::string log_msg = LOGGER::format_string_impl(lv, fmt, args);
        va_end(args);

        while(tasks_.size() >= max_queue_size_ && log_buff_queue_.size() >= max_queue_size_/2)
        {
        // RejectPolicy::BLOCK:
            cv_not_full_.wait(lock);
            break;
        }
        auto log_msg_copy = log_msg;
        tasks_.emplace_back([log_msg_copy]()
        {
            std::lock_guard<std::mutex> log_lock(log_buff_mtx_);
            log_buff_queue_.emplace_back(log_msg_copy);
        });
        lock.unlock();
        cv_not_empty_.notify_one();
    }
private:
    std::vector<std::thread> workers_;
    std::deque<std::function<void()>> tasks_;
    std::mutex queue_mtx_;
    std::condition_variable cv_not_empty_;
    std::condition_variable cv_not_full_;
    bool stop_;
    size_t max_queue_size_;
    RejectPolicy reject_policy_;
};