#pragma once

#include <postgresql/libpq-fe.h>
#include <postgresql/libpq-events.h>
#include <postgresql/pg_config.h>
#include <postgresql/postgres_ext.h>

#include <functional>
#include <string>
#include <vector>

namespace chrindex::andren::base
{

    class PostgresQLConn
    {
    public:
        PostgresQLConn();
        PostgresQLConn(PGconn *conn);
        PostgresQLConn(PostgresQLConn &&_);
        ~PostgresQLConn();

        PostgresQLConn &operator=(PostgresQLConn &&_);

        /// @brief 通过字符串配置进行连接。
        /// example: dbname=mydb user=myuser password=mypassword host=localhost
        /// @param connstr
        /// @return
        bool connectByString(std::string const &connstr);

        /// @brief 通过参数进行连接
        /// @param keyworks
        /// example :
        /// [ "dbname", 
        /// "user", 
        /// "password", 
        /// "hostaddr", 
        /// "port", 
        /// NULL ]  // 参数数组以NULL结尾
        /// @param values
        /// example :
        /// [ "your_database_name",
        ///  "your_username",
        ///  "your_password",
        ///  "your_host_address",
        ///  "your_port_number",
        /// NULL ] // 参数数组以NULL结尾
        /// @param expand_dbname
        /// @return
        bool connectByParam(char const *const *keyworks,
                            char const *const *values,
                            int expand_dbname);

        /// @brief 上一次操作的状态
        /// @return
        int states() const;

        /// @brief 返回连接上的操作最近生成的错误消息。
        /// 该函数返回的错误信息可能是多行的。
        /// @return
        std::string errorMessage() const;

        /// @brief 当Connection实例在进行异步操作时，此函数查询其忙碌状态。
        /// @return
        bool isBusy() const;

        /// @brief 设置实例为非阻塞。
        /// @param arg true为非阻塞，否则阻塞。
        /// @return 如果成功则返回 true，如果错误则返回 false。
        bool setNonblocking(bool arg) const;

        /// @brief 用于判断Connection是否运行在非阻塞模式。
        /// @return
        bool isNonblocking() const;

        /// @brief 查询本连接实例是否使用了SSL。
        /// @return true为已使用。
        bool sslInUse() const;

        /// @brief 返回使用的SSL的信息。
        /// 如果连接不使用 SSL 或未为正在使用的库定义指定的属性名称，则返回空串。
        /// @param attribute_name
        /// [library]
        ///     Name of the SSL implementation in use. (Currently, only "OpenSSL" is implemented)
        /// [protocol]
        ///     SSL/TLS version in use. Common values are "TLSv1", "TLSv1.1" and "TLSv1.2",
        ///     but an implementation may return other strings if some other protocol is used.
        /// [key_bits]
        ///     Number of key bits used by the encryption algorithm.
        /// [cipher]
        ///     A short name of the ciphersuite used, e.g., "DHE-RSA-DES-CBC3-SHA". The names are specific to each SSL implementation.
        /// [compression]
        ///     Returns "on" if SSL compression is in use, else it returns "off".
        /// @return
        std::string sslAttribute(std::string const &attribute_name);

        /// @brief 尝试将任何排队的输出数据刷新到服务器。
        /// @return 如果成功（或者发送队列为空），则返回 true；如果由于某种原因失败，则返回 false；
        /// 如果尚未发送发送队列中的所有数据，则返回 1（这种情况仅在连接为非阻塞时才会发生）。
        int flushToServer() const;

        /// @brief 执行并等待结果返回
        /// @param sql
        /// @return 成功则返回结果集指针，否则返回空指针
        PGresult *execute(std::string const &sql);

        /// @brief 带参数地执行并等待结果返回
        /// @param command 命令
        /// @param nParams 参数数量
        /// @param paramTypes 参数类型列表
        /// @param paramValues 参数列表
        /// @param paramLengths 参数长度列表
        /// @param paramFormats 参数格式列表
        /// @param resultFormat 结果格式
        /// @return 成功则返回结果集指针，否则返回空指针
        PGresult *executeWithParams(
            std::string const &command,
            int nParams,
            Oid const * paramTypes,
            char const * const * paramValues,
            int const * paramLengths,
            int const * paramFormats,
            int resultFormat);

        /// @brief 创建一条预处理语句，并为其命名
        /// @param stmtName 预处理语句标识
        /// @param query 预处理语句
        /// @param nParams 参数数量
        /// @param paramTypes 参数类型列表
        /// @return 成功则返回结果集指针，否则返回空指针
        PGresult *prepare(std::string const &stmtName,
                          std::string const &query, int nParams,
                          Oid const * paramTypes);

        /// @brief 执行一次已创建好的预处理语句
        /// @param stmtName 预处理语句标识
        /// @param nParams 参数数量
        /// @param paramValues 参数列表
        /// @param paramLengths 参数长度列表
        /// @param paramFormats 参数格式列表
        /// @param resultFormat 结果格式
        /// @return 成功则返回结果集指针，否则返回空指针
        PGresult *executePrepared(
            std::string const &stmtName,
            int nParams,
            char const * const * paramValues,
            int const * paramLengths,
            int const * paramFormats,
            int resultFormat);

        /// @brief 发送一条[执行语句]请求并立即返回（不等待结果）
        /// @param query
        /// @return 成功分派请求则返回true，否则返回false
        bool sendQuery(std::string const &query);

        /// @brief 发送一条[执行带参语句]请求并立即返回（不等待结果）
        /// @param command 命令
        /// @param nParams 参数数量
        /// @param paramTypes 参数类型列表
        /// @param paramValues 参数列表
        /// @param paramLengths 参数长度列表
        /// @param paramFormats 参数格式列表
        /// @param resultFormat 结果格式
        /// @return 成功分派请求则返回true，否则返回false
        bool sendQueryParams(
            std::string const &command,
            int nParams,
            Oid const * paramTypes,
            char const * const * paramValues,
            int const * paramLengths,
            int const * paramFormats,
            int resultFormat);

        /// @brief 发送一条[创建预处理语句]的请求，并立即返回（不等待创建结果）
        /// @param stmtName 预处理语句标识
        /// @param query 预处理语句
        /// @param nParams 参数数量
        /// @param paramTypes 参数类型列表
        /// @return 成功分派请求则返回true，否则返回false
        bool sendPrepare(std::string const &stmtName,
                         std::string const &query, int nParams,
                         Oid const * paramTypes);

        /// @brief 发送一条[执行已创建好的预处理语句]的请求，并立即返回（不等待执行结果）
        /// @param stmtName 预处理语句标识
        /// @param nParams 参数数量
        /// @param paramValues 参数列表
        /// @param paramLengths 参数长度列表
        /// @param paramFormats 参数格式列表
        /// @param resultFormat 结果格式
        /// @return 成功分派请求则返回true，否则返回false
        bool sendExecutePrepared(
            std::string const &stmtName,
            int nParams,
            char const * const * paramValues,
            int const * paramLengths,
            int const * paramFormats,
            int resultFormat);

        /// @brief 异步操作。该操作用于获取异步查询中执行查询的结果集元数据，
        /// 包括查询结果的列数、每列的数据类型等信息。
        /// @param portal
        /// @return 成功分派请求则返回true，否则返回false
        bool sendDescribePortal(std::string const &portal);

        /// @brief 用于在异步查询中获取已经预处理的查询（prepared statement）的描述信息，
        /// 包括查询结果的列数、每列的数据类型等信息。该函数发送一个 Describe 请求给 PostgreSQL 服务器，
        /// 告知服务器要获取指定预处理查询的描述信息
        /// @param stmt_name 预处理语句标识
        /// @return 成功分派请求则返回true，否则返回false
        bool sendDescribePrepared(std::string const &stmt_name);

        /// @brief 通过发送同步消息并刷新发送缓冲区来标记管道中的同步点。
        /// 这充当隐式事务和错误恢复点的分隔符.
        /// @return 返回 true 表示成功。如果连接不是管道模式或发送同步消息失败，则返回 false。
        bool pipelineSync();

        /// @brief 处理服务器的输入、接收和响应。
        /// 该函数用于处理 PostgreSQL 客户端连接的输入数据。
        /// 当 PostgreSQL 服务器有数据返回给客户端时（例如查询结果、通知等），
        /// 这些数据会被存储在连接缓冲区中。使用 PQconsumeInput 可以告知 libpq 库处理缓冲区中的数据，
        /// 以便在后续代码中使用 PQgetResult 或 PQgetResultWait 来获取实际的查询结果数据。
        /// @return 返回true为无错发生。
        bool consumeInput();

        /// @brief 获取异步执行操作完成后的结果集。
        /// 等待先前 sendQuery、sendQueryParams、
        /// sendPrepare、sendExecutePrepared、PQsendDescribePrepared、
        /// PQsendDescribePortal 或 PQpipelineSync 调用的下一个结果，
        /// 并将其返回。当命令完成时，将返回一个空指针，并且不会有更多结果。
        /// @return 结果集指针
        PGresult *getResult();

        /// @brief 内部指针是否有效
        /// @return true为有效指针（不代表实例有效）
        bool valid() const;

        /// @brief 获取原始指针
        /// @return 连接实例指针
        PGconn *handle();

    private:
        PGconn *m_handle;
    };

    class PostgresQLResult
    {
    public:
        PostgresQLResult();
        PostgresQLResult(PostgresQLResult &&_);
        PostgresQLResult(PGresult *res);
        ~PostgresQLResult();

        PostgresQLResult &operator=(PostgresQLResult &&_);

        PostgresQLResult &operator=(PGresult * res);

        /// @brief 获取结果集的行数
        /// @return 行数
        int rows() const;

        /// @brief 获取结果集的列数
        /// @return 列数
        int cols() const;

        /// @brief 根据名字获取结果集的列索引
        /// @param fieldName
        /// @return 列索引
        int getFieldColByName(std::string const &fieldName) const;

        /// @brief 根据列索引获取列名
        /// @param col 列索引号
        /// @return 列名
        std::string getFieldNameByColumnIndex(int col)const;

        /// @brief 根据行列号获取结果中的某个值的二进制长度
        /// @param r 行号
        /// @param c 列号
        /// @return 长度（字节数）
        int getValueLength(int r, int c) const;

        /// @brief 根据行列号获取结果集中的某个值
        /// @param r 行号
        /// @param c 列号
        /// @return 值缓冲区指针
        char *getValue(int r, int c) const;

        /// @brief 获取上一次操作的状态
        /// @return 状态号
        int status() const;

        /// @brief 内部指针是否有效
        /// @return true为内部指针有效（不代表实例有效）
        bool valid() const;

        /// @brief 原始指针
        /// @return 指针指向的实例
        PGresult *handle();

    private:
        PGresult *m_res;
    };

    class PostgresQLStatic
    {
    public:
        PostgresQLStatic() = default;
        ~PostgresQLStatic() = default;

        /// @brief 设置通知接收器，但不阻止控制台输出。
        /// 操作的成功与否，错误及其它通知，会调用该通知接收器，当且仅当通知接收函数被设置为有效。
        /// @param conn 连接实例
        /// @param func 通知接收函数
        static void setNoticeReceiver(PGconn *conn, std::function<void(const PGresult *res)> func);

        /// @brief 设置通知处理器，且阻止控制台输出。
        /// 操作的成功与否，错误及其它通知，会调用该通知处理器，当且仅当通知处理函数被设置为有效。
        /// @param conn 连接实例
        /// @param func 通知处理函数
        static void setNoticeProcessor(PGconn *conn, std::function<void(std::string const &msg)> func);

    private:
    };

}
