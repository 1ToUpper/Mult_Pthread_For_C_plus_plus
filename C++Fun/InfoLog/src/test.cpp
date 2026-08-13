#include "../include/test.h"

void test_seek_file_write()
{
    std::string str = seek_log_files("");
    std::cout<< str <<std::endl;
}

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

// Unit tests for seek_log_files and file rotation
bool unit_test_create_log_folder_when_missing()
{
    namespace fs = std::filesystem;
    if (fs::exists(log_folder_dir)) fs::remove_all(log_folder_dir);
    std::string name = seek_log_files("");
    bool created = fs::exists(log_folder_dir) && name == "Info.0";
    std::cout << "unit_test_create_log_folder_when_missing: " << (created ? "passed" : "FAILED") << std::endl;
    return created;
}

bool unit_test_create_file_when_no_logs()
{
    namespace fs = std::filesystem;
    if (fs::exists(log_folder_dir)) fs::remove_all(log_folder_dir);
    fs::create_directory(log_folder_dir);
    std::string name = seek_log_files("");
    bool ok = (name == "Info.0");
    if (ok) {
        bool wrote = write_file_operation("HEADER\n");
        ok = ok && wrote && fs::exists(log_folder_dir + "/Info.0");
    }
    std::cout << "unit_test_create_file_when_no_logs: " << (ok ? "passed" : "FAILED") << std::endl;
    return ok;
}

bool unit_test_restart_creates_new_file()
{
    namespace fs = std::filesystem;
    if (fs::exists(log_folder_dir)) fs::remove_all(log_folder_dir);
    fs::create_directory(log_folder_dir);
    // create an existing Info.0
    std::ofstream out(log_folder_dir + "/Info.0", std::ios::app);
    out << "old\n";
    out.close();
    reset_first_open_flag();
    std::string name = seek_log_files("");
    bool ok = (name == "Info.1");
    if (!ok)
    {
        std::cout << "DEBUG: seek_log_files returned '" << name << "'" << std::endl;
        namespace fs = std::filesystem;
        std::cout << "DEBUG: files in folder:" << std::endl;
        for (auto &e : fs::directory_iterator(log_folder_dir)) std::cout << " - " << e.path().filename().string() << std::endl;
    }
    if (ok) {
        write_file_operation("HEADER: test\n");
        std::ifstream in(log_folder_dir + "/Info.1");
        std::string first;
        std::getline(in, first);
        ok = ok && (first == "HEADER: test");
    }
    std::cout << "unit_test_restart_creates_new_file: " << (ok ? "passed" : "FAILED") << std::endl;
    return ok;
}

bool unit_test_rotation_on_size()
{
    namespace fs = std::filesystem;
    if (fs::exists(log_folder_dir)) fs::remove_all(log_folder_dir);
    fs::create_directory(log_folder_dir);
    set_max_file_byte(10); // small for test
    // create Info.0 with 8 bytes
    {
        std::ofstream out(log_folder_dir + "/Info.0", std::ios::trunc);
        out << "12345678";
    }
    // writing 4 bytes should force rotation (8 + 4 > 10)
    bool wrote = write_file_operation("abcd");
    bool exists_new = fs::exists(log_folder_dir + "/Info.1");
    std::cout << "unit_test_rotation_on_size: " << ((wrote && exists_new) ? "passed" : "FAILED") << std::endl;
    return (wrote && exists_new);
}

bool run_all_fileoperator_unit_tests()
{
    bool ok = true;
    ok &= unit_test_create_log_folder_when_missing();
    ok &= unit_test_create_file_when_no_logs();
    ok &= unit_test_restart_creates_new_file();
    ok &= unit_test_rotation_on_size();
    return ok;
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
