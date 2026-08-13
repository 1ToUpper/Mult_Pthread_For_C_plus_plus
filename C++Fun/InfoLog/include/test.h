#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <thread>
#include "InfoLog.h"
#include "PthreadPool.h"
#include "FileOperator.h"

//system test function
void test_get_current_time();

void test_printf_format_string();

bool test_format_string();

bool test_pthread_pool();

bool test_pthread_pool_and_logger();

bool run_all_fileoperator_unit_tests();

//detail test function
bool test_file_write();

void test_printf_format_string();

void test_get_current_time();

bool test_pthread_pool_and_logger();

bool test_format_string();

bool test_pthread_pool_block();

bool test_pthread_pool_discard_oldest();

bool test_pthread_pool_abort();

bool test_pthread_pool_caller_runs();

bool test_pthread_pool();

