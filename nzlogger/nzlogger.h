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
       LogLine(const char *file,
               int line,
               const char *function,
               Level level,
               std::size_t logid);

       ~LogLine();
       std::ostringstream &Stream();

    protected:
       const char *file_;
       int line_;
       const char *function_;
       Level level_;
       std::size_t logid_;
       std::ofstream dev_null_stream_;
       std::ostringstream stream_;
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

    std::size_t AddCategory(const std::string &name);
    const std::string &GetCategoryName(std::size_t logid);
    static std::string GetExePathName();
    static std::string GetExePath();
protected:
    Logger();
    ~Logger();
    void LogWriter();
    void PushBack(const QueueData &&data);

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
    nz::Logger::LogLine(__FILE__, __LINE__, static_cast<const char*>(__PRETTY_FUNCTION__), nz::Logger::level, nz::logid).Stream() \
    << "[tid:" << syscall(SYS_gettid) << "][" << nz::Logger::Instance()->GetCategoryName(nz::logid) << "] "

