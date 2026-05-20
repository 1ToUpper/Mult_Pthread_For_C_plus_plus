/* 
 * Author: zang  2026/5/18
 * 在上一个例子中，我们看到了数据竞争的情况，最终的结果是不可预测的。
 * 现在我们来看看如何使用锁来保护共享资源，避免数据竞争的发生
 */
#include <iostream>
#include <thread>
#include <mutex>

using namespace std;

int g_count = 0;
std::mutex g_mtx;

void func()
{
    //注意加上锁的
    std::lock_guard<std::mutex> lock(g_mtx);
    for(int i = 0;i < 1000000; i++)
    {
        g_count++;
    }
}

int main(int argc, char** argv)
{
    std::thread t1(func);
    std::thread t2(func);

    t1.join();
    t2.join();

    std::cout<<"Final count value:"<<g_count<<std::endl;
    return 0;
}