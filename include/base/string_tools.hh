
#include <string>
#include <vector>
#include <chrono>

namespace chrindex::andren::base
{
    std::vector<std::string> splitStringByChar(std::string const &str, char delimiter);

    std::vector<std::string> splitStringBySubstring(std::string const &str, std::string const &delimiter);

    std::chrono::system_clock::time_point convertStringToDate(std::string const &dateString);

    std::chrono::seconds convertStringToTime(std::string const &timeString);

    std::string convertDateToString(std::chrono::system_clock::time_point timePoint);

    std::string convertTimeToString(std::chrono::seconds duration);

}
