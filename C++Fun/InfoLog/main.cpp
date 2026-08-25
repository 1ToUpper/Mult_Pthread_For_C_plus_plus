// Simple benchmark runner
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>
#include "InfoLog.h"

using namespace std::chrono_literals;

//测试吞吐量，使用64个线程，每个线程每秒提交1000条日志，日志大小为256字节，持续运行60秒
void run_bench(unsigned int producers = 64, size_t msg_size = 256, int duration_seconds = 60)
{
    // Prepare a message payload
    std::string payload(msg_size, 'A');

    // Start reporter thread
    std::atomic<bool> stop{false};
    std::thread reporter([&stop]() {
        while(!stop.load()) {
            uint64_t s = LOGGER::metrics_submitted.exchange(0);
            uint64_t w = LOGGER::metrics_written.exchange(0);
            uint64_t b = LOGGER::metrics_bytes_written.exchange(0);
            uint64_t d = LOGGER::metrics_dropped.exchange(0);
            std::cout << "submitted=" << s << " msg/s, written=" << w << " msg/s, throughput=" << (b/1024.0/1024.0) << " MB/s, dropped=" << d << std::endl;
            std::this_thread::sleep_for(5s);
        }
    });

    // Start producer threads
    std::vector<std::thread> producers_v;
    producers_v.reserve(producers);
    for(unsigned int i = 0; i < producers; ++i) {
        producers_v.emplace_back([&payload, &stop]() {
            auto logger = LOGGER::MsgLog::get_instance();
            while(!stop.load()) {
                logger->log(LOGGER::DEBUG_, "%s", payload.c_str());
            }
        });
    }

    // Run for duration
    std::this_thread::sleep_for(std::chrono::seconds(duration_seconds));
    stop.store(true);

    for(auto &t : producers_v) if(t.joinable()) t.join();
    // allow background writers to flush
    std::this_thread::sleep_for(500ms);
    stop.store(true);
    if(reporter.joinable()) reporter.join();

    // Print final cumulative totals
    std::cout << "FINAL totals (approx): submitted total and written total are reset per-second, check logs for cumulative if needed" << std::endl;
}

int main(int argc, char** argv)
{
    run_bench();
    return 0;
}