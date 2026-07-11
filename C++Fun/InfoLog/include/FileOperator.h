#ifndef FILEOPERATOR_H
#define FILEOPERATOR_H

#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>
#include <mutex>

inline const std::string log_folder_dir = "./log";
inline std::mutex g_file_mtx; //文件锁

//1.写入文件操作
bool write_file_operation(const std::string &log_text);
//2.检索文件
std::string seek_log_files(const std::string &ready_log_msg);
// test helpers
void set_max_file_byte(size_t bytes);
void reset_first_open_flag();
//3.开机创建新文件
void open_create_new_log_file();


#endif