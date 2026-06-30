#include "../include/test.h"

bool test_file_write()
{
    bool res = true;
    auto t1 = LOGGER::format_string(LOGGER::DEBUG_, "hello");
    res &= write_file_operation(t1);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto t2 = LOGGER::format_string(LOGGER::WARNING_, "number %d", 42);
     res &= write_file_operation(t2);
    return res;
}

void test_printf_format_string()
{
    auto t1 = LOGGER::format_string(LOGGER::DEBUG_, "hello");
    std::cout << t1 << std::endl; 
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto t2 = LOGGER::format_string(LOGGER::WARNING_, "number %d", 42);
    std::cout << t2 << std::endl; 
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto t3 = LOGGER::format_string(LOGGER::DEBUG_, "float %.2f", 3.14159);
    std::cout << t3 << std::endl; 
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    const char* name = "world";
    auto t4 = LOGGER::format_string(LOGGER::ERROR_, "hello %s %d %.1f", name, 7, 2.5);
    std::cout << t4 << std::endl; 
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

void test_get_current_time()
{
    std::string str_tm = LOGGER::get_curren_time_ms();
    std::cout<< str_tm << std::endl;
}

bool test_pthread_pool_and_logger()
{
    const unsigned int worker_count = 4;
    const size_t queue_size = 8;
    PthreadPool pool(worker_count, queue_size, PthreadPool::RejectPolicy::BLOCK);
    pool.log(LOGGER::WARNING_, "number %d", 42);
    pool.log(LOGGER::WARNING_, "number %d", 41);
    pool.log(LOGGER::WARNING_, "number %d", 43);
    pool.log(LOGGER::WARNING_, "number %d", 44);
    pool.log(LOGGER::WARNING_, "number %d", 48);
    pool.log(LOGGER::WARNING_, "number %d", 45);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::lock_guard<std::mutex> lock(log_buff_mtx_);
    for(size_t i = 0; i < log_buff_queue_.size(); i++)
    {
        std::cout<< log_buff_queue_.at(i)<<std::endl;
    }   
    return true;
}

bool test_format_string()
{
    int failed = 0;

    auto t1 = LOGGER::format_string(LOGGER::DEBUG_, "hello");
    if (t1 != "DEBUG: hello") { std::cout << "test1 failed: " << t1 << "" << std::endl; ++failed; } 
    else { std::cout << "test1 passed" << std::endl; }

    auto t2 = LOGGER::format_string(LOGGER::WARNING_, "number %d", 42);
    if (t2 != "WARNING: number 42") { std::cout << "test2 failed: " << t2 << "" << std::endl; ++failed; } 
    else { std::cout << "test2 passed" << std::endl; }

    auto t3 = LOGGER::format_string(LOGGER::DEBUG_, "float %.2f", 3.14159);
    if (t3 != "DEBUG: float 3.14") { std::cout << "test3 failed: " << t3 << "" << std::endl; ++failed; } 
    else { std::cout << "test3 passed" << std::endl; }

    const char* name = "world";
    auto t4 = LOGGER::format_string(LOGGER::ERROR_, "hello %s %d %.1f", name, 7, 2.5);
    if (t4 != "ERROR: hello world 7 2.5") { std::cout << "test4 failed: " << t4 << "" << std::endl; ++failed; } 
    else { std::cout << "test4 passed" << std::endl; }

    if (failed == 0) { std::cout << "format_string tests passed"; return true; }
    std::cout << failed << " format_string tests failed" << std::endl;
    return false;
}

bool test_pthread_pool_block()
{
    const unsigned int worker_count = 2;
    const size_t queue_size = 4;
    PthreadPool pool(worker_count, queue_size, PthreadPool::RejectPolicy::BLOCK);

    std::atomic<int> counter{0};
    const int tasks_to_submit = 10;
    std::vector<std::future<void>> futures;
    futures.reserve(tasks_to_submit);

    for (int i = 0; i < tasks_to_submit; ++i)
    {
        futures.emplace_back(pool.submit([&counter] {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            ++counter;
        }));
    }

    for (auto &f : futures)
    {
        f.get();
    }

    if (counter.load() == tasks_to_submit)
    {
        std::cout << "pthread pool BLOCK policy test passed" << std::endl;
        return true;
    }

    std::cout << "pthread pool BLOCK policy test failed: expected " << tasks_to_submit
              << " completed tasks, got " << counter.load() << std::endl;
    return false;
}

bool test_pthread_pool_discard_oldest()
{
    const unsigned int worker_count = 1;
    const size_t queue_size = 2;
    PthreadPool pool(worker_count, queue_size, PthreadPool::RejectPolicy::DISCARD_OLDEST);

    std::atomic<int> counter{0};
    const int tasks_to_submit = 6;
    std::vector<std::future<void>> futures;
    futures.reserve(tasks_to_submit);
    std::promise<void> start_promise;
    auto start_signal = start_promise.get_future().share();

    for (int i = 0; i < tasks_to_submit; ++i)
    {
        futures.emplace_back(pool.submit([&counter, start_signal] {
            start_signal.wait();
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            ++counter;
        }));
    }

    start_promise.set_value();
    for (auto &f : futures)
    {
        if (f.valid())
        {
            try { f.get(); } catch (...) { }
        }
    }

    if (counter.load() < tasks_to_submit && counter.load() > 0)
    {
        std::cout << "pthread pool DISCARD_OLDEST policy test passed (executed "
                  << counter.load() << " tasks)" << std::endl;
        return true;
    }

    std::cout << "pthread pool DISCARD_OLDEST policy test failed: expected fewer than "
              << tasks_to_submit << " executed tasks, got " << counter.load() << "" << std::endl;
    return false;
}

bool test_pthread_pool_abort()
{
    const unsigned int worker_count = 1;
    const size_t queue_size = 2;
    PthreadPool pool(worker_count, queue_size, PthreadPool::RejectPolicy::ABORT);

    std::atomic<int> counter{0};
    const int tasks_to_submit = 4;
    std::vector<std::future<void>> futures;
    futures.reserve(tasks_to_submit);
    bool abort_thrown = false;

    for (int i = 0; i < tasks_to_submit; ++i)
    {
        try
        {
            futures.emplace_back(pool.submit([&counter] {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                ++counter;
            }));
        }
        catch (const std::runtime_error&)
        {
            abort_thrown = true;
            break;
        }
    }

    for (auto &f : futures)
    {
        f.get();
    }

    if (abort_thrown)
    {
        std::cout << "pthread pool ABORT policy test passed" << std::endl;
        return true;
    }
    std::cout << "pthread pool ABORT policy test failed: no exception thrown" << std::endl;
    return false;
}

bool test_pthread_pool_caller_runs()
{
    const unsigned int worker_count = 1;
    const size_t queue_size = 2;
    PthreadPool pool(worker_count, queue_size, PthreadPool::RejectPolicy::CALLER_RUNS);

    std::atomic<int> counter{0};
    const int tasks_to_submit = 6;
    std::vector<std::future<void>> futures;
    futures.reserve(tasks_to_submit);

    for (int i = 0; i < tasks_to_submit; ++i)
    {
        futures.emplace_back(pool.submit([&counter] {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            ++counter;
        }));
    }

    for (auto &f : futures)
    {
        f.get();
    }

    if (counter.load() == tasks_to_submit)
    {
        std::cout << "pthread pool CALLER_RUNS policy test passed" << std::endl;
        return true;
    }

    std::cout << "pthread pool CALLER_RUNS policy test failed: expected " << tasks_to_submit
              << " completed tasks, got " << counter.load() << "" << std::endl;
    return false;
}

bool test_pthread_pool()
{
    bool ok = true;
    ok = ok && test_pthread_pool_block();
    ok = ok && test_pthread_pool_discard_oldest();
    ok = ok && test_pthread_pool_abort();
    ok = ok && test_pthread_pool_caller_runs();
    return ok;
}
