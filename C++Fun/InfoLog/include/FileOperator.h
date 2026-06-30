#ifndef FILEOPERATOR_H
#define FILEOPERATOR_H

#include <string>
#include <fstream>
#include <iostream>

const std::string log_folder_dir = "./log";

//1.写入文件操作
bool write_file_operation(const std::string &log_text);
//2.检索文件
std::string seek_log_files();
//3.开机创建新文件
void open_create_new_log_file();
#endif