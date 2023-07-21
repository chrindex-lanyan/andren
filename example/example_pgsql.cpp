#include <iostream>
#include <fmt/core.h>
#include <string>
#include <functional>
#include <stdint.h>
#include <assert.h>

#include "../include/andren.hh"


using namespace chrindex::andren::base;

int test_pgsql()
{
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
        printf("error message = %s\n",conn.errorMessage().c_str());
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
        "b71a9507-55f3-4122-a290-82671ecf3415", "f42c88d2-1d18-4e03-9986-7b2cebaa0f22", 
        "bbdd1262-4aac-4bd7-a50d-66a221eb9f85", "172f66bc-2ab7-4b13-b9ee-1c2ce9540967", 
    };
    std::string registip = "192.168.88.3/32"; 
    std::string config_content = "{\"state\" : \"ok\" }";
    
    for (size_t i = 0; i< uuids.size(); i++)
    {
        sql = fmt::format("insert into t1"
        "(fullname,registtime,money,icon,user_uuid,registip,config_content)"
        "values( '{}' , '{}' , {} , '{}' , '{}' , '{}' , '{}' )",
        (fullname+std::to_string(i+1)) , registtime, money, icon, uuids[i], registip, config_content);
        res = conn.execute(sql);
        assert(res.status() == PGRES_COMMAND_OK);
    }

    res = conn.execute("select id, fullname,registtime,money,icon,user_uuid,registip,config_content from t1;");
    assert(res.status() == PGRES_TUPLES_OK);
    if (res.valid())
    {
        int row = res.rows();
        int col = res.cols();

        printf("\n--------------------------\n");
        for (int c = 0 ; c< col ; c++)
        {
            printf("%s \t",res.getFieldNameByColumnIndex(c).c_str());
        }
        printf("\n--------------------------\n");
        for (int r = 0 ; r < row ; r++)
        {
            for (int c = 0 ; c< col;c++){
                char const * pval = res.getValue(r,c);
                printf("%s \t",pval);
            }
            printf ("\n");
        }
    }

    return 0;
}

int test_pgsql_prepared()
{
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
        printf("error message = %s\n",conn.errorMessage().c_str());
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
        "values( %1 , %2 , %3 , %4 , %5 , %6 , %7 )";
    Oid const * paramTypes = 0; // 让后端推导参数类型
    res = conn.prepare("stmt_insert" , sql, 7, paramTypes);

    assert(res.status() == PGRES_COMMAND_OK);
    conn.executePrepared("stmt_insert", 7, 0,0,0,0);
    

    return 0;
}

int test_pgsql_nonblocking()
{
    return 0;
}

int test_pgsql_prepared_nonblocking()
{
    return 0;
}

int main(int argc ,char ** argv)
{
    test_pgsql();
    // test_pgsql_prepared();
    // test_pgsql_nonblocking();
    // test_pgsql_prepared_nonblocking();

    return 0;
}
