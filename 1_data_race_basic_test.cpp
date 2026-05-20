/* 
 * Author: zang  2026/5/18
 * 写一个数据竞争的例子，体会一下
 */
#include <iostream>
#include <vector>
#include <thread>

using namespace std;

//全局共享变量，没有任何保护
int count = 0;

//线程函数
void consumer()
{
    for(int i = 0; i < 10000000; i++)
    {
        //自增操作: 一行代码但是三条指令
        //1.内存将变量读到寄存器
        //2.寄存器的值+1
        //3.将寄存器的值写回内存
        count++;
    }
}

int main(int argc, char** argv)
{

    std::thread t1(consumer);
    std::thread t2(consumer);
    
    t1.join();
    t2.join();

    std::cout << "Final count value:" << count << std::endl;
    return 0;
}