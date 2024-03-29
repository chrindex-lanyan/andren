﻿

#include "pgsql_api.hh"


namespace chrindex::andren::base
{

    ////////////////////////////////// PostgresQLConn

    PostgresQLConn::PostgresQLConn()
    {
        m_handle = 0;
    }

    PostgresQLConn::PostgresQLConn(PGconn *conn)
    {
        m_handle = conn;
    }

    PostgresQLConn::PostgresQLConn(PostgresQLConn &&_)
    {
        m_handle = _.m_handle;
        _.m_handle = 0;
    }

    PostgresQLConn::~PostgresQLConn()
    {
        if (m_handle)
        {
            PQfinish(m_handle);
        }
    }

    PostgresQLConn &PostgresQLConn::operator=(PostgresQLConn &&_)
    {
        this->~PostgresQLConn();
        m_handle = _.m_handle;
        _.m_handle = 0;
        return *this;
    }

    bool PostgresQLConn::connectByString(std::string const &connstr)
    {
        m_handle = PQconnectdb(connstr.c_str());
        return m_handle != nullptr;
    }

    bool PostgresQLConn::connectByParam(char const *const *keyworks,
                                        char const *const *values,
                                        int expand_dbname)
    {
        m_handle = PQconnectdbParams(keyworks, values, expand_dbname);
        return m_handle != nullptr;
    }

    bool PostgresQLConn::connectStartByString(std::string const &connstr)
    {
        return nullptr != (m_handle= PQconnectStart(connstr.c_str()));
    }

    bool PostgresQLConn::connectStartParams(const char *const *keywords,
                                              const char *const *values,
                                              int expand_dbname)
    {
        return nullptr != (m_handle= PQconnectStartParams(keywords ,values , expand_dbname));
    }

    PostgresPollingStatusType PostgresQLConn::connectPoll()
    {
        return PQconnectPoll(m_handle);
    }

    int PostgresQLConn::states() const
    {
        return PQstatus(m_handle);
    }

    std::string PostgresQLConn::errorMessage() const
    {
        return PQerrorMessage(m_handle);
    }

    bool PostgresQLConn::isBusy() const
    {
        return PQisBusy(m_handle) == 1; // return 1 is busy.
    }

    bool PostgresQLConn::setNonblocking(bool arg) const
    {
        return PQsetnonblocking(m_handle, arg ? 1 : 0);
    }

    bool PostgresQLConn::isNonblocking() const
    {
        return PQisnonblocking(m_handle) == 1; // return 1 is nonblocking
    }

    int PostgresQLConn::flushToServer() const
    {
        return PQflush(m_handle);
    }

    PGresult *PostgresQLConn::execute(std::string const &sql)
    {
        return PQexec(m_handle, sql.c_str());
    }

    PGresult *PostgresQLConn::executeWithParams(
        std::string const &command,
        int nParams,
        Oid const *paramTypes,
        char const *const *paramValues,
        int const *paramLengths,
        int const *paramFormats,
        int resultFormat)
    {
        return PQexecParams(m_handle, command.c_str(), nParams,
                            paramTypes, paramValues,
                            paramLengths, paramFormats, resultFormat);
    }

    PGresult *PostgresQLConn::prepare(std::string const &stmtName,
                                      std::string const &query, int nParams,
                                      Oid const *paramTypes)
    {
        return PQprepare(m_handle, stmtName.c_str(), query.c_str(), nParams, paramTypes);
    }

    PGresult *PostgresQLConn::executePrepared(
        std::string const &stmtName,
        int nParams,
        char const *const *paramValues,
        int const *paramLengths,
        int const *paramFormats,
        int resultFormat)
    {
        return PQexecPrepared(m_handle, stmtName.c_str(),
                              nParams, paramValues, paramLengths,
                              paramFormats, resultFormat);
    }

    bool PostgresQLConn::sendQuery(std::string const &query)
    {
        return PQsendQuery(m_handle, query.c_str()) == 1;
    }

    bool PostgresQLConn::sendQueryParams(
        std::string const &command,
        int nParams,
        Oid const *paramTypes,
        char const *const *paramValues,
        int const *paramLengths,
        int const *paramFormats,
        int resultFormat)
    {
        return PQsendQueryParams(m_handle, command.c_str(), nParams, paramTypes,
                                 paramValues, paramLengths,
                                 paramFormats, resultFormat) == 1;
    }

    bool PostgresQLConn::sendPrepare(std::string const &stmtName,
                                     std::string const &query, int nParams,
                                     Oid const *paramTypes)
    {
        return PQsendPrepare(m_handle, stmtName.c_str(), query.c_str(),
                             nParams, paramTypes) == 1;
    }

    bool PostgresQLConn::sendExecutePrepared(
        std::string const &stmtName,
        int nParams,
        char const *const *paramValues,
        int const *paramLengths,
        int const *paramFormats,
        int resultFormat)
    {
        return PQsendQueryPrepared(m_handle, stmtName.c_str(), nParams,
                                   paramValues, paramLengths,
                                   paramFormats, resultFormat) == 1;
    }

    bool PostgresQLConn::sendDescribePortal(std::string const &portal)
    {
        return PQsendDescribePortal(m_handle, portal.c_str()) == 1;
    }

    bool PostgresQLConn::sendDescribePrepared(std::string const &stmt)
    {
        return PQsendDescribePrepared(m_handle, stmt.c_str()) == 1;
    }

    bool PostgresQLConn::pipelineSync()
    {
        return PQpipelineSync(m_handle) == 1;
    }

    bool PostgresQLConn::valid() const
    {
        return m_handle != nullptr;
    }

    PGconn *PostgresQLConn::handle()
    {
        return m_handle;
    }

    bool PostgresQLConn::consumeInput()
    {
        return PQconsumeInput(m_handle) == 1;
    }

    PGresult *PostgresQLConn::getResult()
    {
        return PQgetResult(m_handle);
    }

    ////////////////////// PostgresQLResult

    PostgresQLResult ::PostgresQLResult() { m_res = 0; }

    PostgresQLResult ::PostgresQLResult(PostgresQLResult &&_)
    {
        m_res = _.m_res;
        _.m_res = 0;
    }
    PostgresQLResult ::PostgresQLResult(PGresult *res) { m_res = res; }
    PostgresQLResult ::~PostgresQLResult()
    {
        if (m_res)
        {
            PQclear(m_res);
        }
    }

    PostgresQLResult &PostgresQLResult ::operator=(PostgresQLResult &&_)
    {
        this->~PostgresQLResult();
        m_res = _.m_res;
        _.m_res = 0;
        return *this;
    }

    PostgresQLResult &PostgresQLResult ::operator=(PGresult *res)
    {
        this->~PostgresQLResult();
        m_res = res;
        return *this;
    }

    int PostgresQLResult ::rows() const
    {
        return PQntuples(m_res);
    }

    int PostgresQLResult ::cols() const
    {
        return PQnfields(m_res);
    }

    int PostgresQLResult ::getFieldColByName(std::string const &fieldName) const
    {
        return PQfnumber(m_res, fieldName.c_str());
    }

    std::string PostgresQLResult ::getFieldNameByColumnIndex(int col) const
    {
        return PQfname(m_res, col);
    }

    int PostgresQLResult ::getValueLength(int r, int c) const
    {
        return PQgetlength(m_res, r, c);
    }

    char *PostgresQLResult ::getValue(int r, int c) const
    {
        return PQgetvalue(m_res, r, c);
    }

    int PostgresQLResult ::status() const
    {
        return PQresultStatus(m_res);
    }

    bool PostgresQLResult ::valid() const
    {
        return m_res != nullptr;
    }

    PGresult *PostgresQLResult ::handle()
    {
        return m_res;
    }

    /////////////////////  PostgresQLNotify

    void PostgresQLStatic::setNoticeReceiver(PGconn *conn, std::function<void(const PGresult *res)> func)
    {
        auto pfunc = new (decltype(func))(std::move(func));
        ::PQsetNoticeReceiver(
            conn, [](void *arg, PGresult const *res)
            {
                auto _pfunc = reinterpret_cast<decltype(pfunc)>(arg);
                if (_pfunc && (*_pfunc))
                {
                    (*_pfunc)(res);
                }
                delete _pfunc; },
            pfunc);
    }

    void PostgresQLStatic::setNoticeProcessor(PGconn *conn, std::function<void(std::string const &msg)> func)
    {
        auto pfunc = new (decltype(func))(std::move(func));
        ::PQsetNoticeProcessor(
            conn, [](void *arg, char const *str)
            {
                auto _pfunc = reinterpret_cast<decltype(pfunc)>(arg);
                if (_pfunc && (*_pfunc))
                {
                    (*_pfunc)(str);
                }
                delete _pfunc; },
            pfunc);
    }

#ifdef _MSC_VER

#define __htobe64(l)            \
            ( ( ((l) >> 56) & 0x00000000000000FFLL ) |       \
              ( ((l) >> 40) & 0x000000000000FF00LL ) |       \
              ( ((l) >> 24) & 0x0000000000FF0000LL ) |       \
              ( ((l) >>  8) & 0x00000000FF000000LL ) |       \
              ( ((l) <<  8) & 0x000000FF00000000LL ) |       \
              ( ((l) << 24) & 0x0000FF0000000000LL ) |       \
              ( ((l) << 40) & 0x00FF000000000000LL ) |       \
              ( ((l) << 56) & 0xFF00000000000000LL ) )

#define __htobe32(x)  \
            ( (((x) & 0xFF000000U) >> 24) | (((x) & 0x00FF0000U) >>  8) | \
            (((x) & 0x0000FF00U) <<  8) | (((x) & 0x000000FFU) << 24) )
   
    uint64_t htobe64(uint64_t val)
    {
        return __htobe64(val);
    }

    uint32_t htobe32(uint32_t val) 
    {
        return __htobe32(val);
    }

#endif

    double htobeF64(double value)
    {
        uint64_t *pf64 = reinterpret_cast<uint64_t *>(&value);
        uint64_t res = htobe64(*pf64);
        double *pres = reinterpret_cast<double *>(&res);
        return *pres;
    }

    float htonf32(float value)
    {
        uint32_t *pf32 = reinterpret_cast<uint32_t *>(&value);
        uint32_t res = htobe32(*pf32);
        float *pres = reinterpret_cast<float *>(&res);
        return *pres;
    }


}