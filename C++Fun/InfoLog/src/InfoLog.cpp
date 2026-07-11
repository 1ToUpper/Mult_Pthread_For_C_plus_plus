#include "InfoLog.h"
#include <iostream>

void test_get_current_time();
void test_printf_format_string();
bool test_format_string();
bool test_pthread_pool();
bool test_pthread_pool_and_logger();
bool run_all_fileoperator_unit_tests();

int main()
{
    // run new fileoperator unit tests
    bool ok = run_all_fileoperator_unit_tests();
    std::cout << "fileoperator unit tests overall: " << (ok ? "passed" : "FAILED") << std::endl;
    return ok ? 0 : 1;
}