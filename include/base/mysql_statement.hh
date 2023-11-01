#include <mysql/mysql.h>

#include <stdint.h>
#include <variant>
#include <string>
#include <vector>
#include <functional>
#include <deque>
#include <memory>
#include <map>
#include <optional>
#include <chrono>


using MYSQL = struct MYSQL;
using MYSQL_STMT = struct MYSQL_STMT;

namespace chrindex::andren::base
{
    extern std::string mysqlErrors(::MYSQL *_handle);

    extern int mysqlErrno(::MYSQL *_handle);

    struct MySQLDateTime
    {
        enum class Level : int
        {
            DATE,
            TIME,
            DATETIME,
            YEAR,
        };

        MySQLDateTime() {};
        MySQLDateTime(std::string const &value, Level level);
        std::variant<std::chrono::system_clock::time_point,
                     std::chrono::seconds, std::string>
            timeValue;
        Level level;
    };

    struct MySQLDecimal
    {
        std::string decimalValue;
    };

    struct MySQLResult
    {
        using Value =
            std::variant<int64_t, uint64_t,
                         double, std::string,
                         MySQLDateTime, MySQLDecimal>;

        MySQLResult() {};
        ~MySQLResult() {};
        MySQLResult(MySQLResult &&) = default;
        MySQLResult(MySQLResult const &) = default;

        MySQLResult(MYSQL_RES *mysqlResult);
        MySQLResult &operator=(MySQLResult &&_);

        MySQLResult &operator=(MySQLResult const &_);

        int fieldPos(std::string const &fieldName);

        std::vector<std::string> fields() const;

        std::vector<Value> fetchOneRow();

        bool hasNextRow() const;

        void cover(MYSQL_RES *mysqlResult);

    private:
        std::map<std::string, uint32_t> fieldMap;
        std::deque<std::vector<Value>> rows;
    };

    class MySQLConn;

    class MySQLExecutor
    {
    public:
        MySQLExecutor(MySQLConn *_);

        ~MySQLExecutor();

        int executeNoReturn(std::string const &sql);
        int execute(std::string const &sql, std::function<void(MySQLResult *_ptr)> _onResult = {});

    private:
        MySQLConn *m_conn;
    };

    class MySQLConn : public std::enable_shared_from_this<MySQLConn>
    {
    public:
        MySQLConn();

        MySQLConn(MySQLConn &&_);

        MySQLConn(::MYSQL *handle);

        ~MySQLConn();

        MySQLConn &operator=(MySQLConn &&_);

        static std::shared_ptr<MySQLConn> create();

        bool connect(std::string const &host,
                     std::string const &user,
                     std::string const &pswd,
                     std::string const &db,
                     uint32_t port,
                     const char *unix_socket,
                     u_long flags);

        void close();

        bool valid() const;

        ::MYSQL *handle() const;

        ::MYSQL *take_handle();

        MySQLExecutor executor();

    private:
        ::MYSQL *m_handle;
    };

}

