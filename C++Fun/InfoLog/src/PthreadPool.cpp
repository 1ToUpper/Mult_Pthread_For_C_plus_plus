#include "../include/PthreadPool.h"

PthreadPool::PthreadPool(unsigned int size, size_t queue_size, RejectPolicy policy)
{
    stop_ = false;
    max_queue_size_ = queue_size;
    reject_policy_ = policy;
    for(unsigned int i = 0; i < size; i++)
    {
        workers_.emplace_back([&]{
            while(true)
            {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queue_mtx_);
                    cv_not_empty_.wait(lock, [this] {return stop_ || !tasks_.empty();});
                    if(stop_ && tasks_.empty()) 
                    {
                        break;
                    } 
                    task = std::move(tasks_.front());
                    tasks_.pop_front();
                    cv_not_full_.notify_one();
                    lock.unlock();
                }
                task();
            }
        });
    }
}

PthreadPool::~PthreadPool()
{
    {
        std::lock_guard<std::mutex> lock(queue_mtx_);
        stop_ = true;
    }
    cv_not_empty_.notify_all();
    cv_not_full_.notify_all();
    for(std::thread &worker : workers_)
    {
        if(worker.joinable())
            worker.join();
    }
}