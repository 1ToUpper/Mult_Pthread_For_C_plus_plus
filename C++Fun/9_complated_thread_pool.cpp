#include <iostream>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <functional>
#include <vector>
#include <thread>
#include <future>
#include <chrono>

using namespace std;

class ThreadPool
{
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
        for(size_t i = 0; i < tasks_num; i++)
        {
            workers_.emplace_back([&](){
                while(true)
                {
                    std::function<void()> task;
                    std::unique_lock<std::mutex> lock(mtx_);
                    not_empty_.wait(lock, [&](){return stop_ || !tasks_.empty();});
                    if(stop_ && tasks_.empty())
                    {
                        break;
                    }
                    task = tasks_.front();
                    tasks_.pop();
                    lock.unlock();
                    not_full_.notify_one(); //通知BLOCK的方式，submit可以将任务加入队列
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
        if(stop_)
        {
            throw std::runtime_error("submit on stopped ThreadPool");
        }

        //队列满的时候处理
        while(tasks_.size() >= max_queue_size_)
        {
            switch(reject_policy_)
            {
                case RejectPolicy::BLOCK_:
                    not_full_.wait(lock);
                    break;
                case RejectPolicy::DISCARD_OLDEST_:
                    tasks_.pop();
                    break;
                case RejectPolicy::ABORT_:
                    throw std::runtime_error("ThreadPool queue is full");
                    break;
                case RejectPolicy::CALLER_RUNS_:
                    lock.unlock();
                    (*task)();
                    return res;
            }
        }
        tasks_.emplace([task](){(*task)();});
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
        for(std::thread &worker : workers_)
        {
            if(worker.joinable())
                worker.join();
        }
    }
    size_t queue_size() 
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return tasks_.size();
    }
    size_t thread_count()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return workers_.size();
    }
    bool is_stopped() 
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return stop_;
    }
    void clear_queue()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        while(!tasks_.empty())
        {
            tasks_.pop();
        }
    }
    void wait_all()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        not_empty_.wait(lock, [this]{
            return tasks_.empty();
        });
    }
};

int main(int argc, char** argv)
{
    ThreadPool pool(2, 4);
    std::vector<std::future<int>> futures;
    futures.reserve(10);

    for(int i = 0; i < 10; i++)
    {
        std::cout << "submit " << i << " begin, queue_size=" << pool.queue_size() << std::endl;
        futures.emplace_back(pool.submit([i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            std::cout << "task " << i << " executed" << std::endl;
            return i;
        }));
        std::cout << "submit " << i << " end" << std::endl;
    }

    for(auto &f : futures)
    {
        std::cout << "result=" << f.get() << std::endl;
    }

    return 0;
}
