#include <iostream>
#include "test.h"

using namespace std;

int main()
{
    bool ok = test_file_write();
    // also run fileoperator unit tests
    extern bool run_all_fileoperator_unit_tests();
    ok &= run_all_fileoperator_unit_tests();
    //test_printf_format_string();
    //test_get_current_time();
    //bool ok = test_format_string();
    //ok = ok && test_pthread_pool() && test_pthread_pool_and_logger();
    return 0;
}