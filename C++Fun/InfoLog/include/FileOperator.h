#ifndef FILEOPERATOR_H
#define FILEOPERATOR_H

#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>
#include <mutex>

namespace FileOperator
{
    const std::string log_folder_dir = "./log";
    std::mutex g_file_mtx; //文件锁

    //1.写入文件操作
    bool write_file_operation(const std::string &log_text);
    //2.检索文件
    std::string seek_log_files(const std::string &ready_log_msg);
}

#endif