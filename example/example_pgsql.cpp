#include <iostream>
#include <fmt/core.h>
#include <string>
#include <functional>
#include <stdint.h>
#include <assert.h>

#include "../include/andren.hh"

using namespace chrindex::andren::base;

void printResult(PostgresQLResult &res)
{
    int row = res.rows();
    int col = res.cols();

    printf("\n--------------------------\n");
    for (int c = 0; c < col; c++)
    {
        printf("%s \t", res.getFieldNameByColumnIndex(c).c_str());
    }
    printf("\n--------------------------\n");
    for (int r = 0; r < row; r++)
    {
        for (int c = 0; c < col; c++)
        {
            char const *pval = res.getValue(r, c);
            printf("%s \t", pval);
        }
        printf("\n");
    }
}

int test_pgsql()
{

    printf("\n------------ TEST PGSQL BLOCKING STATEMENT API --------------\n");

    PostgresQLConn conn;
    PostgresQLResult res;
    std::string sql;

    /// 已经创建了用户developer
    /// CREATE USER developer WITH PASSWORD "1qaz@2wsx" LOGIN;
    /// GRANT ALL PRIVILEGES ON DATABASE shop TO developer;
    /// GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO developer;
    /// GRANT ALL ON SCHEMA public TO developer;
    /// SELECT pg_reload_conf();
    conn.connectByString("dbname=shop user=developer password=1qaz@2wsx host=192.168.88.2 port=5432 ");

    if (conn.states() != CONNECTION_OK)
    {
        printf("cannot connect postgresql. login user = developer\n");
        printf("error message = %s\n", conn.errorMessage().c_str());
        return -1;
    }
    printf("connected postgresql. login user = developer\n");

    sql = "CREATE TABLE IF NOT EXISTS public.t1"
          "(id bigint NOT NULL GENERATED ALWAYS AS IDENTITY "
          "( INCREMENT 1 START 100000001 MINVALUE 1 MAXVALUE 9223372036854775807 CACHE 1 ),"
          "  fullname character varying(64) COLLATE pg_catalog.\"default\","
          "  registtime date,"
          "  money double precision,"
          "  icon bytea,"
          "  user_uuid uuid,"
          "  registip cidr,"
          "  config_content json,"
          "  CONSTRAINT t1_pkey PRIMARY KEY (id)"
          ")";
    res = conn.execute(sql);
    assert(res.status() == PGRES_COMMAND_OK);

    res = conn.execute("delete from t1");
    assert(res.status() == PGRES_COMMAND_OK);

    std::string fullname = "lihua";
    std::string registtime = "2023-07-22";
    double money = 3.14;
    std::string icon = "\x12\x34\x56\x78"; // 5bytes
    std::vector<std::string> uuids = {
        "b71a9507-55f3-4122-a290-82671ecf3415",
        "f42c88d2-1d18-4e03-9986-7b2cebaa0f22",
        "bbdd1262-4aac-4bd7-a50d-66a221eb9f85",
        "172f66bc-2ab7-4b13-b9ee-1c2ce9540967",
    };
    std::string registip = "192.168.88.3/32";
    std::string config_content = "{\"state\" : \"ok\" }";

    for (size_t i = 0; i < uuids.size(); i++)
    {
        sql = fmt::format("insert into t1"
                          "(fullname,registtime,money,icon,user_uuid,registip,config_content)"
                          "values( '{}' , '{}' , {} , '{}' , '{}' , '{}' , '{}' )",
                          (fullname + std::to_string(i + 1)), registtime, money, icon, uuids[i], registip, config_content);
        res = conn.execute(sql);
        assert(res.status() == PGRES_COMMAND_OK);
    }

    res = conn.execute("select id, fullname,registtime,money,icon,user_uuid,registip,config_content from t1;");
    assert(res.status() == PGRES_TUPLES_OK);
    if (res.valid())
    {
        printResult(res);
    }

    printf("\n------------ END TEST PGSQL BLOCKING STATEMENT API --------------\n");

    return 0;
}

int test_pgsql_prepared()
{
    printf("\n------------ TEST PGSQL BLOCKING PREPARE STATEMENT API --------------\n");

    PostgresQLConn conn;
    PostgresQLResult res;
    std::string sql;

    /// 已经创建了用户developer
    /// CREATE USER developer WITH PASSWORD "1qaz@2wsx" LOGIN;
    /// GRANT ALL PRIVILEGES ON DATABASE shop TO developer;
    /// GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO developer;
    /// GRANT ALL ON SCHEMA public TO developer;
    /// SELECT pg_reload_conf();
    conn.connectByString("dbname=shop user=developer password=1qaz@2wsx host=192.168.88.2 port=5432 ");

    if (conn.states() != CONNECTION_OK)
    {
        printf("cannot connect postgresql. login user = developer\n");
        printf("error message = %s\n", conn.errorMessage().c_str());
        return -1;
    }
    printf("connected postgresql. login user = developer\n");

    sql = "CREATE TABLE IF NOT EXISTS public.t1"
          "(id bigint NOT NULL GENERATED ALWAYS AS IDENTITY "
          "( INCREMENT 1 START 100000001 MINVALUE 1 MAXVALUE 9223372036854775807 CACHE 1 ),"
          "  fullname character varying(64) COLLATE pg_catalog.\"default\","
          "  registtime date,"
          "  money double precision,"
          "  icon bytea,"
          "  user_uuid uuid,"
          "  registip cidr,"
          "  config_content json,"
          "  CONSTRAINT t1_pkey PRIMARY KEY (id)"
          ")";
    res = conn.execute(sql);
    assert(res.status() == PGRES_COMMAND_OK);

    res = conn.execute("delete from t1");
    assert(res.status() == PGRES_COMMAND_OK);

    sql = "insert into t1"
          "(fullname,registtime,money,icon,user_uuid,registip,config_content)"
          //"values( $1::varchar , $2::date , $3::double , $4::bytea , $5::uuid , $6::cidr , $7::json )"; // 预处理无需显式指定类型
          "values( $1 , $2 , $3 , $4 , $5 , $6 , $7 )";
    Oid const *paramTypes = 0; // 在SQL的参数中指定类型，比使用Oid值更好。因为libpq并没有给出类型的Oid值定义。
    res = conn.prepare("stmt_insert", sql, 7, paramTypes);

    if (res.status() != PGRES_COMMAND_OK)
    {
        printf("Error : %d. In SQL [ %s ]\n", res.status(), sql.c_str());
        assert(false);
    }

    for (int i = 0; i < 4; i++)
    {
        std::string fullname = "lihua_p" + std::to_string(i + 1);
        std::string registtime = "2023-07-23";
        double money = 3.14;
        std::string icon = "\x76\x34\x23\22\54";
        std::string user_uuid = "bbdd1262-4aac-4bd7-a50d-66a221eb9f8" + std::to_string(i);
        std::string registip = "192.168.11.1/32";
        std::string config_content = "{ \"state\":\"yes\" }";

        /// 对于二进制类型记得转换网络字节序
        money = htobeF64(money);

        /// 开始绑定

        char const *const values[7] = {
            fullname.c_str(),
            registtime.c_str(),
            reinterpret_cast<char const *>(&money),
            icon.c_str(),
            user_uuid.c_str(),
            registip.c_str(),
            config_content.c_str()};

        const int length[] = {
            fullname.size(),
            registtime.size(),
            sizeof(money),
            icon.size(),
            user_uuid.size(),
            registip.size(),
            config_content.size()};

        const int formates[] = {
            0,
            0,
            1, // money 是double类型，使用二进制。当然也可以使用文本类型。
            0,
            0,
            0,
            0};

        int resultFormat = 0; // insert 语句没有结果值，填0即可。

        res = conn.executePrepared("stmt_insert",
                                   7,           // 参数数量
                                   values,      // 每个参数的值
                                   length,      // 每个参数的长度
                                   formates,    // 参数值类型： 0 - 文本格式 ； 1 - 二进制格式
                                   resultFormat // 结果值类型： 0 - 文本格式 ； 1 - 二进制格式
        );

        if (res.status() != PGRES_COMMAND_OK)
        {
            printf("Error : %d.\n", res.status());
            assert(false);
        }
    } // end for ...

    ////  select 查询
    sql = "select id, fullname,registtime,money,icon,user_uuid,registip,config_content from t1; ";
    res = conn.prepare("stmt_select", sql, 0, 0);
    if (res.status() != PGRES_COMMAND_OK)
    {
        printf("Error : %d.\n", res.status());
        assert(false);
    }

    res = conn.executePrepared("stmt_select", 0, 0, 0, 0, 0); // 最后一个0意思是结果用文本表示，而不是二进制
    if (res.status() != PGRES_TUPLES_OK)
    {
        printf("Error : %d.\n", res.status());
        assert(false);
    }

    if (res.valid())
    {
        printResult(res);
    }

    printf("\n------------ END TEST PGSQL BLOCKING PREPARE STATEMENT API --------------\n");

    return 0;
}

int test_pgsql_nonblocking()
{
    printf("\n------------ TEST PGSQL NON-BLOCKING STATEMENT API --------------\n");

    PostgresQLConn conn;
    PostgresQLResult res;
    std::string sql;

    /// 已经创建了用户developer
    /// CREATE USER developer WITH PASSWORD "1qaz@2wsx" LOGIN;
    /// GRANT ALL PRIVILEGES ON DATABASE shop TO developer;
    /// GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO developer;
    /// GRANT ALL ON SCHEMA public TO developer;
    /// SELECT pg_reload_conf();
    conn.connectStartByString("dbname=shop user=developer password=1qaz@2wsx host=192.168.88.2 port=5432 ");
    while (1)
    {
        PostgresPollingStatusType pres = conn.connectPoll();
        if (conn.states() == CONNECTION_OK)
        {
            printf("connect pgsql server OK.\n Poll Status Code = %d\n", pres);
            break;
        }
        if (conn.states() == CONNECTION_BAD)
        {
            printf("cannot connect pgsql server.\n");
            assert(false);
            break;
        }
    }

    if (conn.states() != CONNECTION_OK)
    {
        printf("cannot connect postgresql. login user = developer\n");
        printf("error message = %s\n", conn.errorMessage().c_str());
        return -1;
    }
    printf("connected postgresql. login user = developer\n");

    conn.setNonblocking(true);

    sql = "CREATE TABLE IF NOT EXISTS public.t1"
          "(id bigint NOT NULL GENERATED ALWAYS AS IDENTITY "
          "( INCREMENT 1 START 100000001 MINVALUE 1 MAXVALUE 9223372036854775807 CACHE 1 ),"
          "  fullname character varying(64) COLLATE pg_catalog.\"default\","
          "  registtime date,"
          "  money double precision,"
          "  icon bytea,"
          "  user_uuid uuid,"
          "  registip cidr,"
          "  config_content json,"
          "  CONSTRAINT t1_pkey PRIMARY KEY (id)"
          ")";

    bool bret = conn.sendQuery(sql);
    if (!bret)
    {
        printf("error.cannot send query to execute. %s\n", conn.errorMessage().c_str());
        assert(bret);
    }
    while (conn.isBusy())
    {
        bret = conn.consumeInput();
        if (!bret)
        {
            printf("PGSQL Socket Closed.\n");
            assert(false);
            break;
        }
        res = conn.getResult();
        if (!res.valid() || res.status() != PGRES_COMMAND_OK)
        {
            break;
        }
    }

    sql = "delete from t1";
    bret = conn.sendQuery(sql);
    if (!bret)
    {
        printf("error.cannot send query to execute. %s\n", conn.errorMessage().c_str());
        assert(bret);
    }
    while (conn.isBusy())
    {
        bret = conn.consumeInput();
        if (!bret)
        {
            printf("PGSQL Socket Closed.\n");
            assert(false);
            break;
        }
        res = conn.getResult();
        if (!res.valid() || res.status() != PGRES_COMMAND_OK)
        {
            break;
        }
    }

    std::string fullname = "lihua";
    std::string registtime = "2023-07-22";
    double money = 3.14;
    std::string icon = "\x12\x34\x56\x78"; // 5bytes
    std::vector<std::string> uuids = {
        "b71a9507-55f3-4122-a290-82671ecf3415",
        "f42c88d2-1d18-4e03-9986-7b2cebaa0f22",
        "bbdd1262-4aac-4bd7-a50d-66a221eb9f85",
        "172f66bc-2ab7-4b13-b9ee-1c2ce9540967",
    };
    std::string registip = "192.168.88.3/32";
    std::string config_content = "{\"state\" : \"ok\" }";

    for (size_t i = 0; i < uuids.size(); i++)
    {
        sql = fmt::format("insert into t1"
                          "(fullname,registtime,money,icon,user_uuid,registip,config_content)"
                          "values( '{}' , '{}' , {} , '{}' , '{}' , '{}' , '{}' )",
                          (fullname + std::to_string(i + 1)), registtime, money, icon, uuids[i], registip, config_content);
        bret = conn.sendQuery(sql);
        if (!bret)
        {
            printf("error.cannot send query to execute.\n");
            assert(bret);
        }
        while (conn.isBusy())
        {
            bret = conn.consumeInput();
            if (!bret)
            {
                printf("PGSQL Socket Closed.\n");
                assert(false);
                break;
            }
            res = conn.getResult();
            if (!res.valid() || res.status() != PGRES_COMMAND_OK)
            {
                break;
            }
        }
    }

    sql = "select id, fullname,registtime,money,icon,user_uuid,registip,config_content from t1;";
    bret = conn.sendQuery(sql);
    if (!bret)
    {
        printf("error.cannot send query to execute.\n");
        assert(bret);
    }

    while (conn.isBusy())
    {
        bret = conn.consumeInput();
        if (!bret)
        {
            printf("PGSQL Socket Closed.\n");
            assert(false);
            break;
        }
        res = conn.getResult();
        if (!res.valid())
        {
            printf("OK... All Result fetch done.\n");
            break;
        }
        if (res.status() == PGRES_TUPLES_OK)
        {
            printResult(res);
        }
    }

    printf("\n------------ END TEST PGSQL NON-BLOCKING STATEMENT API --------------\n");
    return 0;
}

int test_pgsql_prepared_nonblocking()
{
    printf("\n------------ TEST PGSQL NON-BLOCKING PREPARE STATEMENT API --------------\n");

    PostgresQLConn conn;
    PostgresQLResult res;
    std::string sql;

    /// 已经创建了用户developer
    /// CREATE USER developer WITH PASSWORD "1qaz@2wsx" LOGIN;
    /// GRANT ALL PRIVILEGES ON DATABASE shop TO developer;
    /// GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO developer;
    /// GRANT ALL ON SCHEMA public TO developer;
    /// SELECT pg_reload_conf();
    conn.connectStartByString("dbname=shop user=developer password=1qaz@2wsx host=192.168.88.2 port=5432 ");
    while (1)
    {
        PostgresPollingStatusType pres = conn.connectPoll();
        if (conn.states() == CONNECTION_OK)
        {
            printf("connect pgsql server OK.\n Poll Status Code = %d\n", pres);
            break;
        }
        if (conn.states() == CONNECTION_BAD)
        {
            printf("cannot connect pgsql server.\n");
            assert(false);
            break;
        }
    }

    if (conn.states() != CONNECTION_OK)
    {
        printf("cannot connect postgresql. login user = developer\n");
        printf("error message = %s\n", conn.errorMessage().c_str());
        return -1;
    }
    printf("connected postgresql. login user = developer\n");

    conn.setNonblocking(true);

    sql = "CREATE TABLE IF NOT EXISTS public.t1"
          "(id bigint NOT NULL GENERATED ALWAYS AS IDENTITY "
          "( INCREMENT 1 START 100000001 MINVALUE 1 MAXVALUE 9223372036854775807 CACHE 1 ),"
          "  fullname character varying(64) COLLATE pg_catalog.\"default\","
          "  registtime date,"
          "  money double precision,"
          "  icon bytea,"
          "  user_uuid uuid,"
          "  registip cidr,"
          "  config_content json,"
          "  CONSTRAINT t1_pkey PRIMARY KEY (id)"
          ")";

    bool bret = conn.sendQuery(sql);
    if (!bret)
    {
        printf("error.cannot send query to execute. %s\n", conn.errorMessage().c_str());
        assert(bret);
    }
    while (conn.isBusy())
    {
        bret = conn.consumeInput();
        if (!bret)
        {
            printf("PGSQL Socket Closed.\n");
            assert(false);
            break;
        }
        res = conn.getResult();
        if (!res.valid() || res.status() != PGRES_COMMAND_OK)
        {
            break;
        }
    }

    sql = "delete from t1";
    bret = conn.sendQuery(sql);
    if (!bret)
    {
        printf("error.cannot send query to execute. %s\n", conn.errorMessage().c_str());
        assert(bret);
    }
    while (conn.isBusy())
    {
        bret = conn.consumeInput();
        if (!bret)
        {
            printf("PGSQL Socket Closed.\n");
            assert(false);
            break;
        }
        res = conn.getResult();
        if (!res.valid() || res.status() != PGRES_COMMAND_OK)
        {
            break;
        }
    }

    sql = "insert into t1"
          "(fullname,registtime,money,icon,user_uuid,registip,config_content)"
          //"values( $1::varchar , $2::date , $3::double , $4::bytea , $5::uuid , $6::cidr , $7::json )"; // 预处理无需显式指定类型
          "values( $1 , $2 , $3 , $4 , $5 , $6 , $7 )";
    Oid const *paramTypes = 0; // 在SQL的参数中指定类型，比使用Oid值更好。因为libpq并没有给出类型的Oid值定义。
    bret = conn.sendPrepare("stmt_insert", sql, 7, paramTypes);

    if (!bret)
    {
        printf("error.cannot send query to execute.\n");
        assert(bret);
    }
    while (conn.isBusy())
    {
        bret = conn.consumeInput();
        if (!bret)
        {
            printf("PGSQL Socket Closed.\n");
            assert(false);
            break;
        }
        res = conn.getResult();
        if (!res.valid() || res.status() != PGRES_COMMAND_OK)
        {
            break;
        }
    }

    for (int i = 0; i < 4; i++)
    {
        std::string fullname = "lihua_p" + std::to_string(i + 1);
        std::string registtime = "2023-07-23";
        double money = 3.14;
        std::string icon = "\x76\x34\x23\22\54";
        std::string user_uuid = "bbdd1262-4aac-4bd7-a50d-66a221eb9f8" + std::to_string(i);
        std::string registip = "192.168.11.1/32";
        std::string config_content = "{ \"state\":\"yes\" }";

        /// 对于二进制类型记得转换网络字节序
        money = htobeF64(money);

        /// 开始绑定

        char const *const values[7] = {
            fullname.c_str(),
            registtime.c_str(),
            reinterpret_cast<char const *>(&money),
            icon.c_str(),
            user_uuid.c_str(),
            registip.c_str(),
            config_content.c_str()};

        const int length[] = {
            fullname.size(),
            registtime.size(),
            sizeof(money),
            icon.size(),
            user_uuid.size(),
            registip.size(),
            config_content.size()};

        const int formates[] = {
            0,
            0,
            1, // money 是double类型，使用二进制。当然也可以使用文本类型。
            0,
            0,
            0,
            0};

        int resultFormat = 0; // insert 语句没有结果值，填0即可。

        bret = conn.sendExecutePrepared("stmt_insert",
                                        7,           // 参数数量
                                        values,      // 每个参数的值
                                        length,      // 每个参数的长度
                                        formates,    // 参数值类型： 0 - 文本格式 ； 1 - 二进制格式
                                        resultFormat // 结果值类型： 0 - 文本格式 ； 1 - 二进制格式
        );

        if (!bret)
        {
            printf("error.cannot send query to execute.\n");
            assert(bret);
        }
        while (conn.isBusy())
        {
            bret = conn.consumeInput();
            if (!bret)
            {
                printf("PGSQL Socket Closed.\n");
                assert(false);
                break;
            }
            res = conn.getResult();
            if (!res.valid() || res.status() != PGRES_COMMAND_OK)
            {
                break;
            }
        }
    } // end for ...

    ////  select 查询
    sql = "select id, fullname,registtime,money,icon,user_uuid,registip,config_content from t1; ";
    bret = conn.sendPrepare("stmt_select", sql, 0, 0);
    if (!bret)
    {
        printf("error.cannot send query to execute.\n");
        assert(bret);
    }
    while (conn.isBusy())
    {
        bret = conn.consumeInput();
        if (!bret)
        {
            printf("PGSQL Socket Closed.\n");
            assert(false);
            break;
        }
        res = conn.getResult();
        if (!res.valid() || res.status() != PGRES_COMMAND_OK)
        {
            break;
        }
    }

    bret = conn.sendExecutePrepared("stmt_select", 0, 0, 0, 0, 0); // 最后一个0意思是结果用文本表示，而不是二进制
    if (!bret)
    {
        printf("error.cannot send query to execute.\n");
        assert(bret);
    }
    while (conn.isBusy())
    {
        bret = conn.consumeInput();
        if (!bret)
        {
            printf("PGSQL Socket Closed.\n");
            assert(false);
            break;
        }
        res = conn.getResult();
        if (!res.valid() || res.status() != PGRES_COMMAND_OK)
        {
            break;
        }
    }

    if (res.valid())
    {
        printResult(res);
    }


    printf("\n------------ END TEST PGSQL NON-BLOCKING PREPARE STATEMENT API --------------\n");
    return 0;
}

int main(int argc, char **argv)
{
    test_pgsql();
    test_pgsql_prepared();
    test_pgsql_nonblocking();
    test_pgsql_prepared_nonblocking();

    return 0;
}
