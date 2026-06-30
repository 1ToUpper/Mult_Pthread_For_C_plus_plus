#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <stdexcept>

class ThreadPool
{
public:
    enum class RejectPolicy
    {
        BLOCK_,
        DISCARD_OLDEST_,
        ABORT_,
        CALLER_RUNS_
    };

private:
    std::vector<std::thread> workers_;
    std::mutex mtx_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    std::queue<std::function<void()>> tasks_;
    bool stop_ = false;
    size_t max_queue_size_;
    RejectPolicy reject_policy_;

public:
    ThreadPool() = delete;
    ThreadPool(ThreadPool& tp) = delete;
    ThreadPool(const ThreadPool& tp) = delete;
    ThreadPool& operator=(ThreadPool& tp) = delete;
    ThreadPool& operator=(const ThreadPool& tp) = delete;
    ThreadPool(ThreadPool&& tp) = delete;
    ThreadPool(const ThreadPool&& tp) = delete;

    explicit ThreadPool(size_t tasks_num, size_t max_queue_size = 1024):
        max_queue_size_(max_queue_size), reject_policy_(RejectPolicy::BLOCK_)
    {
        for (size_t i = 0; i < tasks_num; ++i)
        {
            workers_.emplace_back([this]() {
                while (true)
                {
                    std::function<void()> task;
                    std::unique_lock<std::mutex> lock(mtx_);
                    not_empty_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                    if (stop_ && tasks_.empty())
                    {
                        break;
                    }
                    task = std::move(tasks_.front());
                    tasks_.pop();
                    lock.unlock();
                    not_full_.notify_one();
                    task();
                }
            });
        }
    }

    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))>
    {
        using ReturnType = decltype(f(args...));

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<ReturnType> res = task->get_future();
        std::unique_lock<std::mutex> lock(mtx_);
        if (stop_)
        {
            throw std::runtime_error("submit on stopped ThreadPool");
        }

        while (tasks_.size() >= max_queue_size_)
        {
            switch (reject_policy_)
            {
                case RejectPolicy::BLOCK_:
                    not_full_.wait(lock);
                    break;
                case RejectPolicy::DISCARD_OLDEST_:
                    tasks_.pop();
                    break;
                case RejectPolicy::ABORT_:
                    throw std::runtime_error("ThreadPool queue is full");
                case RejectPolicy::CALLER_RUNS_:
                    lock.unlock();
                    (*task)();
                    return res;
            }
        }

        tasks_.emplace([task]() { (*task)(); });
        lock.unlock();
        not_empty_.notify_one();
        return res;
    }

    ~ThreadPool()
    {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            stop_ = true;
        }
        not_full_.notify_all();
        not_empty_.notify_all();
        for (std::thread& worker : workers_)
        {
            if (worker.joinable())
                worker.join();
        }
    }

    size_t queue_size()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return tasks_.size();
    }
};

#endif // THREADPOOL_H
