

#include <vector>
#include <string>
#include <map>
#include <fmt/core.h>
#include <assert.h>
#include <iostream>

#include <iomanip>


#include "mysql_statement.hh"
#include "string_tools.hh"


using namespace chrindex::andren::base;

int test_date()
{
    std::string dateString = "2023-07-03";
    std::chrono::system_clock::time_point timePoint = convertStringToDate(dateString);

    // 输出时间点
    std::time_t t = std::chrono::system_clock::to_time_t(timePoint);
    std::cout << "Time Point: " << std::put_time(std::localtime(&t), "%Y-%m-%d") << std::endl;

    return 0;
}

int test_time()
{
    std::string timeString = "10:25:30";
    std::chrono::seconds duration = convertStringToTime(timeString);

    // 输出秒数
    std::cout << "Duration: " << duration.count() << " seconds" << std::endl;

    return 0;
}



int test_mysql()
{
    MySQLConn conn;
    std::string host = "192.168.88.2", user = "developer", pswd = "1qaz@2wsx", db = "testdb";
    bool bret = conn.connect(host, user, pswd, db, 3306, 0, 0);
    if (!bret)
    {
        printf("connect mysql server %s failed.\n", host.c_str());
        return -1;
    }
    MySQLExecutor executor = conn.executor();
    int ret = executor.execute("create table if not exists t1(id int primary key ,fullname text, registtime datetime,money double)");
    if (ret != 0)
    {
        printf("cannot create table t1.\n");
    }
    printf("created table t1.\n");
    executor.executeNoReturn("alter table t1 modify id int AUTO_INCREMENT ; ");
    printf("start clear t1...\n");
    executor.executeNoReturn("delete from t1;");
    printf("clear t1 done.\nstart insert data.\n");
    for (int i = 0; i < 5; i++)
    {
        std::string sql = "insert into t1( fullname , registtime , money) values ";
        int per = 0;
        for (per = 0; per < 5; per++)
        {
            sql += fmt::format(" (  '{}' , '{}' , {}  ) ,",
                               "lihua" + std::to_string(i * 100 + per + 1),
                               "2023-07-12",
                               (1.0 * i * 100 + per + 1));
        }
        sql.pop_back();
        executor.executeNoReturn(std::move(sql));
        printf("Insert [%d].\n", ((i + 1) * 5));
    }
    executor.execute("select id, fullname , registtime , money from t1 ;",
                     [](MySQLResult *ptr)
                     {
                         if (!ptr)
                         {
                             return;
                         }

                         std::vector<std::string> fields = ptr->fields();
                         assert(fields.size() == 4);
                         fmt::print("-----------------\n{}\t{}\t{}\t{}\n\n",
                                    fields[0], fields[1], fields[2], fields[3]);

                         while (ptr->hasNextRow())
                         {
                             int64_t id;
                             std::string fullname;
                             MySQLDateTime registtime;
                             double money;
                             std::vector<MySQLResult::Value> row = ptr->fetchOneRow();
                             assert(row.size() == 4);

                             id = std::get<int64_t>(row[0]);
                             fullname = std::get<std::string>(row[1]);
                             registtime = std::get<MySQLDateTime>(row[2]);
                             money = std::get<double>(row[3]);

                             fmt::print("{}\t{}\t\t{}\t{}\n", id, fullname, std::get<std::string>(registtime.timeValue), money);
                         }
                     });

    printf("--------\nMySQL Statement Test Done.\n");
    return 0;
}



int main(int argc, char **argv)
{
    // test_date();
    // test_time();

    /// .....

    test_mysql();
    return 0;
}