#include "nzlogger.h"

#include <chrono>
#include <experimental/filesystem>
#include <iostream>
#include <string.h>
#include <time.h>

namespace fs = std::experimental::filesystem;

namespace nz {

Logger::LogLine::LogLine(const char *file,
        int line,
        const char *function,
        Level level,
        std::size_t logid)
    : file_(file)
    , line_(line)
    , function_(function)
    , level_(level)
    , logid_(logid)
{
    dev_null_stream_.open("/dev/null", std::ofstream::out | std::ofstream::app);
}

Logger::LogLine::~LogLine()
{
    if(!enabled_) { return; }
    Logger::Instance()->PushBack(Logger::QueueData(stream_.str(), level_));
}

std::ostringstream &Logger::LogLine::Stream()
{
    return enabled_ ?
                stream_ :
                *(static_cast<std::basic_ostringstream<char>*>(
                      (static_cast<std::basic_ostream<char>*>(&dev_null_stream_))));
}

Logger::Logger()
{
    thread_ = std::async(std::launch::async, [this]() {
        while(!die_)
        {
            LogWriter();
        }
    });
}
Logger::~Logger()
{
    die_ = true;
    thread_.wait();
}
Logger* Logger::Instance()
{
    static Logger logger_;
    return &logger_;
}

std::size_t Logger::AddCategory(const std::string &name)
{
    std::size_t key = -1;
    for(const auto &it : dictionary_)
    {
        key = it.first;
        if(it.second == name)
        {
            return key;
        }
    }
    key++;
    dictionary_.insert(std::pair<std::size_t, std::string>(key, name));
    return key;
}

const std::string &Logger::GetCategoryName(std::size_t logid)
{
    const auto &it = dictionary_.find(logid);
    if(it != dictionary_.end())
    {
        return it->second;
    }
    static const std::string nameless("Nameless");
    return nameless;
}
std::string Logger::GetExePathName()  // full path including executable file name and extension
{
   constexpr int pathlen = 1024;
   char selfpath[pathlen] = {0};
   if(readlink("/proc/self/exe", selfpath, sizeof(selfpath) - 1) < 0 || !*selfpath)
   {
      return std::string("");
   }
   return std::string(selfpath);
}
std::string Logger::GetExePath()   // path without executable file name
{
   auto fullPath = GetExePathName();
   if(fullPath.size() == 0)
   {
      return std::string("");
   }
   auto pos = static_cast<int>(fullPath.find_last_of("/"));
   if(pos == -1)
   {
      return std::string("");
   }
   return fullPath.substr(0, pos+1);
}
void Logger::PushBack(const QueueData &&data)
{
    std::unique_lock<std::mutex> lk(m_);
    Logger::Instance()->queue_.push_back(data);
    cv_.notify_one();
}
void Logger::LogWriter()
{
    Logger::QueueData data;

    {
        using namespace std::chrono_literals;
        std::unique_lock<std::mutex> lk(m_);
        if(!cv_.wait_for(lk, 2s, [this]{ return queue_.size() > 0; }))
        { return; }
        data = queue_.front();
        queue_.pop_front();
    }

    std::tm tm = *std::localtime(&data.time_);
    char timestamp[128] = {0};
    strftime(timestamp, sizeof(timestamp), "%a %b %d %H:%M:%S %Y", &tm);

    switch(data.level_)//31m:red, 32m:green, 33m:yellow, 37m:white, 0m:reset; https://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
    {
    case FATAL: {std::cout << "\033[1;31m"; break;}
    case WARNING: {std::cout << "\033[1;33m"; break;}
    case INFO: {std::cout << "\033[1;32m"; break;}
    case DEBUG: default: {std::cout << "\033[1;37m"; break;}
    }

    std::ostringstream oss;
    oss << "[" << timestamp << "]" << data.logline_ << "\033[0m" << "\n";
    std::cout << oss.str();

    FILE* fp = fopen("/tmp/nugen.log", "a+t");
    fwrite(timestamp, strlen(timestamp), 1, fp);
    fwrite(data.logline_.c_str(), data.logline_.size(), 1, fp);
    fwrite("\n", 1, 1, fp);
    fclose(fp);
}

}//nz
