#pragma once

#include <chrono>
#include <condition_variable>
#include <deque>
#include <fstream>
#include <future>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <sys/syscall.h>
#include <unistd.h>

namespace nz {
class Logger
{
public:
    static Logger* Instance();
    enum Level
    {
        DEBUG,
        INFO,
        WARNING,
        FATAL,
    };
    struct LogLine
    {
       LogLine(Level level);
       ~LogLine();
       std::ostringstream &Stream();

    protected:
       std::ofstream dev_null_stream_;
       std::ostringstream stream_;
       Level level_;
       bool enabled_ = true;
    };
    struct QueueData
    {
        std::string logline_;
        std::time_t time_;
        Level level_;

        QueueData(const std::string &logline="", Level level=DEBUG)
            : logline_(logline), time_(std::time(nullptr)), level_(level) {}
    };

    void SetLogDirectory(const std::string &dir = "/tmp");
    std::size_t AddCategory(const std::string &name);
    const std::string &GetCategoryName(std::size_t logid);
    static std::string GetExePathName();
    static std::string GetExePath();
    static std::string GetExeName();
protected:
    Logger();
    ~Logger();
    void LogWriter();
    void PushBack(const QueueData &&data);

    std::string log_file_;
    std::map<std::size_t, std::string> dictionary_;
    std::deque<Logger::QueueData> queue_;
    std::mutex m_;
    std::condition_variable cv_;
    std::future<void> thread_;
    bool die_ = false;
};
}//nz

#define NEW_LOG_CATEGORY(logid)  \
   namespace nz {\
      std::size_t logid = Logger::Instance()->AddCategory( #logid ); \
   }\

#define LOG_C(logid, level)  \
    nz::Logger::LogLine(nz::Logger::level).Stream() \
    << "[tid:" << syscall(SYS_gettid) \
    << "][" << nz::Logger::Instance()->GetCategoryName(nz::logid) \
    << "][" << __FILE__ << ":" << __LINE__ << "] "//[" << __PRETTY_FUNCTION__ << "] "

