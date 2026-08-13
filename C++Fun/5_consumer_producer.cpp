/* 
 * Author: zang  2026/5/20
 * 使用互斥锁实现一个简单的线程队列，生产者线程不断地往队列中添加数据，消费者线程不断地从队列中取出数据进行处理。
 * 生产者线程和消费者线程通过条件变量来进行同步，保证生产者线程在队列满的时候等待，消费者线程在队列空的时候等待，避免了忙等待的情况。
 * 这个例子中，我们使用了std::mutex来保护共享资源std::queue，使用std::condition_variable来进行线程同步，保证了线程安全和高效的线程通信。
 */
# include <iostream>
# include <thread>
# include <mutex>
# include <condition_variable>
# include <queue>
# include <chrono>

using namespace std;

template<typename T>
class BadThreadSafeQueue
{
private:
    std::queue<T> queue_;
    std::mutex mtx_;
public:
    void push(const T& value)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        queue_.push(value);
    }
    void pop(T& value)
    {
        while(true)
        {
            std::lock_guard<std::mutex> lock(mtx_);
            if(!queue_.empty())
            {
                value = queue_.front();
                queue_.pop();
                return;
            }
        }
    }
};

BadThreadSafeQueue<int> g_queue_;

void producer()
{
    for(int i = 0; i< 200000; i++)
    {
        g_queue_.push(i);
        if(i % 1000 == 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    std::cout << "Producer finished." << std::endl;
}

void consumer()
{
    int count = 0;
    for(int i = 0;i < 100000; i++)
    {
        int value;
        g_queue_.pop(value);
        count++;
        std::cout << "Consumer got value: " << value << std::endl;
    }
    std::cout << "Consumer processed " << count << " items." << std::endl;
}

int main(int argc, char**argv)
{
    auto start_time = std::chrono::high_resolution_clock::now();

    std::thread t1(producer);
    std::thread t2(consumer);
    std::thread t3(consumer);

    t1.join();
    t2.join();
    t3.join();

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cout << "Total time taken: " << duration << " ms" << std::endl;
    return 0;
}