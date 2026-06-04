/* 
 * Author: zang  2026/5/20
 * 在上一个例子中，我们使用了锁来保护共享资源，发生死锁。    
 * 在这个例子中，我们有两个线程t1和t2，它们都需要同时获取mtx_1和mtx_2这两个锁来完成它们的工作。
 * 但是t1先获取了mtx_1，然后休眠了一段时间，导致t2无法获取mtx_1而被阻塞，同时t2又获取了mtx_2，导致t1无法获取mtx_2而被阻塞，最终导致死锁的发生。 
 * 在实际开发中，我们应该尽量避免这种情况的发生，可以通过一些技巧来避免死锁的发生，比如说按照一定的顺序来获取锁，或者使用std::lock来同时获取多个锁等等。
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
        for(int i = 0; i < 1000; i++)
        {
            std::lock_guard<std::mutex> lock(mtx_1);
            g_count++;
            this_thread::sleep_for(std::chrono::milliseconds(1));
            std::lock_guard<std::mutex> lock2(mtx_2);  
        }
        std::cout<< "Thread 1: finished." << std::endl;
    });

    std::thread t2([&]()
    {
        for(int i = 0; i < 1000; i++)
        {
            std::lock_guard<std::mutex> lock(mtx_1);
            g_count++;
            this_thread::sleep_for(std::chrono::milliseconds(1));
            std::lock_guard<std::mutex> lock2(mtx_2);  
        }
        std::cout<< "Thread 2: finished." << std::endl;
    });

    t1.join();
    t2.join();

    std::cout << "Final count value:" << g_count << std::endl;
    return 0;
}