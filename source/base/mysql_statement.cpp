#include "mysql_statement.hh"

#include <fmt/core.h>
#include <assert.h>

#include "string_tools.hh"

namespace chrindex::andren::base
{

    std::string mysqlErrors(::MYSQL *_handle)
    {
        return mysql_error(_handle);
    }

    int mysqlErrno(::MYSQL *_handle)
    {
        return mysql_errno(_handle);
    }

    MySQLDateTime::MySQLDateTime(std::string const &value, Level level)
    {
        switch (level)
        {
        case Level::DATE:
        {
            timeValue = convertStringToDate(value);
            break;
        }
        case Level::TIME:
        {
            timeValue = convertStringToTime(value);
            break;
        }
        case Level::DATETIME:
        case Level::YEAR:
            timeValue = value;
            break;
        default:
        {
            break;
        }
        }
    }

    MySQLResult::MySQLResult(MYSQL_RES *mysqlResult)
    {
        cover(mysqlResult);
    }

    MySQLResult &MySQLResult::operator=(MySQLResult &&_)
    {
        fieldMap = std::move(_.fieldMap);
        rows = std::move(_.rows);
        return *this;
    }

    MySQLResult &MySQLResult::operator=(MySQLResult const &_)
    {
        fieldMap = std::move(_.fieldMap);
        rows = std::move(_.rows);
        return *this;
    }

    int MySQLResult::fieldPos(std::string const &fieldName)
    {
        int pos = -1;
        auto iter = fieldMap.find(fieldName);
        if (iter != fieldMap.end())
        {
            pos = iter->second;
        }
        return pos;
    }

    std::vector<std::string> MySQLResult::fields() const
    {
        if (fieldMap.empty())
        {
            return {};
        }
        std::vector<std::string> fieldVec;
        fieldVec.resize(fieldMap.size());
        for (auto &iter : fieldMap)
        {
            fieldVec[iter.second] = iter.first;
        }
        return fieldVec;
    }

    std::vector<MySQLResult::Value> MySQLResult::fetchOneRow()
    {
        auto row = std::move(rows.front());
        rows.pop_front();
        return row;
    }

    bool MySQLResult::hasNextRow() const
    {
        return !rows.empty();
    }

    void MySQLResult::cover(MYSQL_RES *mysqlResult)
    {
        // 清空现有数据
        fieldMap.clear();
        rows.clear();

        // 获取结果集字段数量
        int numFields = mysql_num_fields(mysqlResult);

        // 获取结果集字段信息
        MYSQL_FIELD *fields = mysql_fetch_fields(mysqlResult);

        // 构建字段名和索引的映射关系
        for (int i = 0; i < numFields; ++i)
        {
            std::string fieldName = fields[i].name;
            fieldMap[fieldName] = i;
        }

        // 遍历结果集行
        std::string buffer;
        Value value;
        std::vector<Value> rowData;
        unsigned long *pvalues = 0;
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(mysqlResult)) != nullptr)
        {
            pvalues = mysql_fetch_lengths(mysqlResult);
            // 遍历每一列数据
            for (int i = 0; i < numFields; ++i)
            {
                buffer = std::string(row[i], pvalues[i]);

                // 根据字段类型将数据转换为对应的类型
                switch (fields[i].type)
                {
                case MYSQL_TYPE_TINY:
                case MYSQL_TYPE_SHORT:
                case MYSQL_TYPE_LONG:
                case MYSQL_TYPE_LONGLONG:
                    value = std::stoll(buffer);
                    break;
                case MYSQL_TYPE_FLOAT:
                case MYSQL_TYPE_DOUBLE:
                    value = std::stod(buffer);
                    break;
                case MYSQL_TYPE_DECIMAL:
                    value = MySQLDecimal{std::move(buffer)};
                    break;
                case MYSQL_TYPE_STRING:
                case MYSQL_TYPE_VAR_STRING:
                    value = std::move(buffer);
                    break;
                case MYSQL_TYPE_DATE:
                    value = MySQLDateTime{std::move(buffer), MySQLDateTime::Level::DATE};
                    break;
                case MYSQL_TYPE_TIME:
                    value = MySQLDateTime{std::move(buffer), MySQLDateTime::Level::TIME};
                    break;
                case MYSQL_TYPE_DATETIME:
                    value = MySQLDateTime{std::move(buffer), MySQLDateTime::Level::DATETIME};
                    break;
                case MYSQL_TYPE_YEAR:
                    value = MySQLDateTime{std::move(buffer), MySQLDateTime::Level::YEAR};
                    break;
                case MYSQL_TYPE_BLOB:
                case MYSQL_TYPE_TINY_BLOB:
                case MYSQL_TYPE_LONG_BLOB:
                case MYSQL_TYPE_MEDIUM_BLOB:
                    value = std::move(buffer);
                    break;
                default:
                    // 其他类型处理方式
                    break;
                }
                rowData.push_back(std::move(value));
            }
            rows.push_back(std::move(rowData));
        }
    }

    MySQLExecutor::MySQLExecutor(MySQLConn *_)
    {
        m_conn = _;
    }

    MySQLExecutor::~MySQLExecutor() {}

    int MySQLExecutor::executeNoReturn(std::string const &sql)
    {
        if (!m_conn || !m_conn->valid())
        {
            return -1;
        }
        return ::mysql_query(m_conn->handle(), sql.c_str());
    }
    int MySQLExecutor::execute(std::string const &sql, std::function<void(MySQLResult *_ptr)> _onResult)
    {
        if (!m_conn || !m_conn->valid())
        {
            return -1;
        }
        int ret = ::mysql_query(m_conn->handle(), sql.c_str());
        if (ret)
        {
            return {};
        }
        auto res = mysql_store_result(m_conn->handle());

        if (_onResult)
        {
            if (res == 0)
            {
                _onResult(0);
            }
            else
            {
                MySQLResult result{res};
                _onResult(&result);
            }
        }
        mysql_free_result(res);
        return 0;
    }

    ////////////////// MySQLConn

    MySQLConn::MySQLConn()
    {
        m_handle = ::mysql_init(0);
    }

    MySQLConn::MySQLConn(MySQLConn &&_)
    {
        m_handle = _.m_handle;
        _.m_handle = 0;
    }

    MySQLConn::MySQLConn(::MYSQL *handle)
    {
        m_handle = handle;
    }

    MySQLConn::~MySQLConn()
    {
        close();
    }

    MySQLConn &MySQLConn::operator=(MySQLConn &&_)
    {
        this->~MySQLConn();
        m_handle = _.m_handle;
        _.m_handle = 0;
        return *this;
    }

    std::shared_ptr<MySQLConn> MySQLConn::create() { return std::make_shared<MySQLConn>(); }

    bool MySQLConn::connect(std::string const &host,
                            std::string const &user,
                            std::string const &pswd,
                            std::string const &db,
                            uint32_t port,
                            const char *unix_socket,
                            u_long flags)
    {
        auto ret = ::mysql_real_connect(m_handle, host.c_str(), user.c_str(), pswd.c_str(),
                                        db.c_str(), port, unix_socket, flags);
        return ret != 0;
    }

    void MySQLConn::close()
    {
        if (m_handle)
        {
            ::mysql_close(m_handle);
            m_handle = 0;
        }
    }

    bool MySQLConn::valid() const { return m_handle != 0; }

    ::MYSQL *MySQLConn::handle() const { return m_handle; };

    ::MYSQL *MySQLConn::take_handle()
    {
        auto handle = m_handle;
        m_handle = 0;
        return handle;
    };

    MySQLExecutor MySQLConn::executor()
    {
        return this;
    }
}