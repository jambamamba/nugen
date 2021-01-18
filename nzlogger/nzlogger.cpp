#include "nzlogger.h"

#include <iostream>

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
//todo : push to queue, let another thread empty the queue and write to file somewhere
    std::cout << stream_.str() << "\n";
}

std::ostringstream &Logger::LogLine::Stream()
{
    return enabled_ ?
                stream_ :
                *(static_cast<std::basic_ostringstream<char>*>(
                      (static_cast<std::basic_ostream<char>*>(&dev_null_stream_))));
}

Logger::Logger()
{}

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

}//nz
