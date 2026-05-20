#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>

using namespace std;

template<typename T>
class ThreadSafeQueue
{
private:
    std::queue<T> queue_;
    std::mutex mtx_;
    std::condition_variable cv_;
public:
    void push(const T& value)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        queue_.push(value);
        cv_.notify_one();
    }
    void pop(T& value)  
    {
        std::unique_lock<std::mutex> lock(mtx_);
        while(queue_.empty())
        {
            cv_.wait(lock);   
        }
        value = queue_.front();
        queue_.pop();
    } 
};       

ThreadSafeQueue<int> g_queue_;

int main(int argc, char** argv)
{
    std::thread producer([&](){
        for(int i = 0;i < 200000; i++)
        {
            g_queue_.push(i);
            if(i % 1000 == 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
        std::cout << "Producer finished." << std::endl;
    });
    std::thread consumer_1([&](){
        int value;
        for(int i = 0; i < 100000; i++)
        {
            g_queue_.pop(value);
            std::cout << "Consumer got value: " << value << std::endl;
        }
        std::cout << "Consumer finished." << std::endl;
    });
        std::thread consumer_2([&](){
        int value;
        for(int i = 0; i < 100000; i++)
        {
            g_queue_.pop(value);
            std::cout << "Consumer got value: " << value << std::endl;
        }
        std::cout << "Consumer finished." << std::endl;
    });

    auto start_time = std::chrono::high_resolution_clock::now();

    producer.join();
    consumer_1.join();
    consumer_2.join();

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cout << "Total time taken: " << duration << " ms" << std::endl;
    return 0;
}