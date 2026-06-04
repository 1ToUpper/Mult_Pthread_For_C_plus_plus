/* 
 * Author: zang  2026/5/20
 * 在上一个例子中，我们使用了锁来保护共享资源，避免了数据竞争的发生。    
 * 但是在某些情况下，锁可能会导致性能问题，甚至死锁的发生。
 * 现在我们来看看一个间歇性锁的例子，体会一下锁的性能问题
 */
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

using namespace std;

int g_count = 0;
std::mutex mtx_1;
std::mutex mtx_2;

int main(int argc, char** argv)
{
    std::thread t1([&]()
    {
        for(int i = 0; i < 1000000; i++)
        {
            std::lock_guard<std::mutex> lock(mtx_1);
            g_count++;
            this_thread::sleep_for(std::chrono::milliseconds(1));
            std::lock_guard<std::mutex> lock2(mtx_2);  
            std::cout<< "Thread 1: finished." << std::endl;
        }
    });

    std::thread t2([&]()
    {
        for(int i = 0; i < 1000000; i++)
        {
            std::lock_guard<std::mutex> lock(mtx_2);
            g_count++;
            this_thread::sleep_for(std::chrono::milliseconds(1));
            std::lock_guard<std::mutex> lock2(mtx_1);  
            std::cout<< "Thread 2: finished." << std::endl;
        }
    });

    t1.join();
    t2.join();

    std::cout << "Final count value:" << g_count << std::endl;
    return 0;
}