#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <vector>

using namespace std;

class ThreadPool 
{
private:
    std::vector<std::thread> workers_;          //工作线程数组
    std::queue<std::function<void()>> tasks_;   //任务队列
    std::mutex queue_mutex_;                    //保护任务队列的互斥锁
    std::condition_variable cv_;                //条件变量，用于通知工作线程有新任务到来
    bool stop_;                                 //线程池停止标志
public:
    ThreadPool() = delete; //禁止默认构造函数
    ThreadPool(const ThreadPool&) = delete; //禁止拷贝构造函数
    explicit ThreadPool(size_t num_threads) : stop_(false)
    {
        for(size_t i = 0; i < num_threads; ++i)
        {
            workers_.emplace_back([this]{
                while(true)
                {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        cv_.wait(lock, [this] { return stop_ || !tasks_.empty();});
                        if(stop_ && tasks_.empty())
                            break;
                        task = std::move(tasks_.front());
                        tasks_.pop();
                        lock.unlock();
                    }
                    task();
                }
            });
        }
    }

    void submit(std::function<void()> task)
    {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if(stop_)
            {
                throw std::runtime_error("ThreadPool has been stopped");
            }
            tasks_.emplace(std::move(task));
        }
        cv_.notify_one();
    }

    ~ThreadPool()
    {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }
        cv_.notify_all();
        for(std::thread &worker : workers_)
        {
            if(worker.joinable())
                worker.join();
        }
    }
};

int main(int argc, char* argv[])
{
    ThreadPool pool(4);

    for(int i = 0; i < 10; ++i)
    {
        pool.submit([i]{
            std::cout << "Task " << i << " is being processed by thread "
                      << std::this_thread::get_id() << std::endl;
        });
    }

    // 线程池将在 main 结束时自动销毁，并等待所有任务完成。
    return 0;
}