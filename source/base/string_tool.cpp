
#include "string_tools.hh"

#include <vector>
#include <string>
#include <chrono>
#include <sstream>
#include <iomanip>

#include <fmt/core.h>

namespace chrindex::andren::base
{

    std::vector<std::string> splitStringByChar(std::string const &str, char delimiter)
    {
        std::vector<std::string> result;
        std::size_t start = 0;
        std::size_t end = str.find(delimiter);

        while (end != std::string::npos)
        {
            result.push_back(str.substr(start, end - start));
            start = end + 1;
            end = str.find(delimiter, start);
        }
        result.push_back(str.substr(start));

        return result;
    }

    std::vector<std::string> splitStringBySubstring(std::string const &str, std::string const &delimiter)
    {
        std::vector<std::string> result;
        std::size_t start = 0;
        std::size_t end = str.find(delimiter);

        while (end != std::string::npos)
        {
            result.push_back(str.substr(start, end - start));
            start = end + delimiter.length();
            end = str.find(delimiter, start);
        }
        result.push_back(str.substr(start));

        return result;
    }

    std::chrono::system_clock::time_point convertStringToDate(std::string const &dateString)
    {
        std::tm tm = {};
        std::istringstream iss(dateString);

        // 解析字符串日期
        iss >> std::get_time(&tm, "%Y-%m-%d");

        // 转换为时间点
        std::time_t t = std::mktime(&tm);
        return std::chrono::system_clock::from_time_t(t);
    }

    std::chrono::seconds convertStringToTime(std::string const &timeString)
    {
        std::tm tm = {};
        std::istringstream iss(timeString);

        // 解析字符串时间
        iss >> std::get_time(&tm, "%H:%M:%S");

        // 获取时、分、秒
        int hours = tm.tm_hour;
        int minutes = tm.tm_min;
        int seconds = tm.tm_sec;

        // 计算总秒数
        std::chrono::seconds duration = std::chrono::hours(hours) + std::chrono::minutes(minutes) + std::chrono::seconds(seconds);
        return duration;
    }

    std::string convertDateToString(std::chrono::system_clock::time_point timePoint)
    {
        std::time_t t = std::chrono::system_clock::to_time_t(timePoint);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&t), "%Y-%m-%d");
        return ss.str();
    }

    std::string convertTimeToString(std::chrono::seconds duration)
    {
        return std::to_string(duration.count());
    }

}