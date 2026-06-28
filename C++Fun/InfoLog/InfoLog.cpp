#include "InfoLog.h"
#include "test.cpp"
#include <iostream>

int main()
{
    bool ok = test_format_string();
    ok = ok && test_pthread_pool();
    return ok ? 0 : 1;
}