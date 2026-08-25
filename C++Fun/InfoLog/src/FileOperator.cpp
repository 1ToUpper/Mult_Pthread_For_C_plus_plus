#include "../include/FileOperator.h"
#include "../include/InfoLog.h"
#include <algorithm>

namespace FileOperator
{
static bool gb_first_open = true;
static const size_t max_file_byte = 1024 * 1024;

bool write_file_operation(const std::string &log_text)
{
    //防止多线程同时创建文件的情况，加锁处理
    std::lock_guard<std::mutex> lock(g_file_mtx); 
    {
        std::string file_dir_name = seek_log_files(log_text);
        std::string full_path = log_folder_dir + "/" + file_dir_name;
        std::ofstream out_file(full_path, std::ios::app);
        if(!out_file.is_open())
        {
            std::cerr << "文件" + file_dir_name << "不存在" << std::endl;
            return false;
        }
        out_file << log_text;
        // metrics: count written messages and bytes
        LOGGER::metrics_written.fetch_add(1, std::memory_order_relaxed);
        LOGGER::metrics_bytes_written.fetch_add(log_text.size(), std::memory_order_relaxed);
    }
    return true;
}

//2.检索文件
std::string seek_log_files(const std::string &ready_log_msg)
{
    namespace fs = std::filesystem;
    std::string file_dir;
    std::vector<int> nums;
    //找文件夹
    if(!fs::exists(log_folder_dir) || !fs::is_directory(log_folder_dir))
    {
        fs::create_directory(log_folder_dir);
        return "Info.0";
    }
    else
    {
        //截取字符串
        for(const auto& entry : fs::directory_iterator(log_folder_dir))
        {
            if(fs::is_regular_file(entry.status()))
            {
                std::string file = entry.path().filename().string();
                if(file.substr(0, 5) == "Info.")
                {
                    std::string num = file.substr(5);
                    int val;
                    try
                    {
                        val = std::stoi(num);
                    }
                    catch(...)
                    {
                        //如果无法转换为数字，继续下一个
                        continue;
                    }
                    nums.push_back(val);
                }
            }
        }
        std::sort(nums.begin(), nums.end());
        if(nums.size() == 0) 
        {
            return "Info.0";
        }
        else 
        {
            //如果是重新启动程序，则创建一个新的文件
            if(gb_first_open)
            {
                gb_first_open = false;
                return "Info." + std::to_string(nums.back() + 1);
            }
            else 
            {   //如果文件大小+即将写入的字符大小超过1024KB，则创建一个新的文件
                size_t file_size = fs::file_size(log_folder_dir + "/Info." + std::to_string(nums.back())); 
                if(file_size + ready_log_msg.size() > max_file_byte)   
                {
                    return "Info." + std::to_string(nums.back() + 1);
                }  
            }
            return "Info." + std::to_string(nums.back());
        }
    }
    return file_dir;
}

}
