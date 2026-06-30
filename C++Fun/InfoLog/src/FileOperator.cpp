#include "../include/FileOperator.h"

bool write_file_operation(const std::string &log_text)
{
    std::string file_dir_name("Info.1");
    std::ofstream out_file(file_dir_name, std::ios::app);
    if(!out_file.is_open())
    {
        std::cerr << "文件" + file_dir_name << "不存在" << std::endl;
        return false;
    }
    out_file << log_text;
    return true;
}

//2.检索文件
std::string seek_log_files()
{
    std::string file_dir;
    //扫描目录/log下文件Info.xxx是否存在
    //若存在，判断文件大小+写入内容大小是否超过1024KB，超过则创建，不超过就追加写入
    //若不存在，则创建新的文件写入
    return file_dir;
}

//3.开机创建新文件
void open_create_new_log_file()
{
    
}