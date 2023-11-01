


#include <stdio.h>
#include <stdint.h>
#include <sys/wait.h>
#include <signal.h>
#include <thread>

#include "pipe.hh"


using namespace chrindex::andren::base;

// 测试两个进程之间使用同一名称的管道进行通信
int test_fifopipe()
{
    int pid = 0;
    std::string path = "/tmp/TEST_FIFO";
    int loop = 5;
    std::string msg = "hello world";

    fprintf(stdout, "\n###### Started Test FIFO Pipe.... ######\n");

    SysNamedPipe::pipe_result_t pipe_result;

    // 判断有没有创建
    if (SysNamedPipe::access(path,F_OK) ==-1){
        // 需要创建pipe
        pipe_result = SysNamedPipe::mkfifo(path, 0777);
        if (pipe_result.ret)
        {
            fprintf(stdout, "[parent] cannot create fifo pipe.errno=%d.\n", pipe_result.eno);
            return -1;
        }
    }

    // 创建进程
    if ((pid = fork()) == 0)
    {
        // child
        SysNamedPipe::pipe_result_t pipe_result = SysNamedPipe::open(path, O_RDONLY);
        if (pipe_result.ret)
        {
            fprintf(stdout, "[child] cannot open fifo pipe file.errno=%d.\n", pipe_result.eno);
            exit(-1);
        }
        int rfd = pipe_result.fd;
        int rwcount = 0;
        for (int i = 0; i < 100; i++)
        {
            std::string data;
            ssize_t ret = SysPipe::read(rfd, data);
            if (ret >0)
            {
                fprintf(stdout, "[child] Recv %ld bytes. Times=[%d].\n", ret, rwcount);
                if (data != msg)
                {
                    fprintf(stdout, "[child] Error. Received data is not equal.\n");
                }
                rwcount++;
            }
            else if(ret ==0){
                // no data
            }
            else
            {
                fprintf(stdout, "[child] recv Error. ret=%ld, errno=%m.\n", ret, errno);
            }
            if (rwcount >= loop)
            {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        SysPipe::close(rfd);
        ::exit(0);
    }

    // parent
    pipe_result = SysNamedPipe::open(path, O_WRONLY);
    if (pipe_result.ret)
    {
        fprintf(stdout, "[parent] cannot open fifo pipe file.errno=%d.\n", pipe_result.eno);
        return -1;
    }
    int wfd = pipe_result.fd;

    fprintf(stdout, "[parent] started. wfd = %d.\n", wfd);

    for (int i = 0; i < loop; i++)
    {
        ssize_t ret = SysPipe::write(wfd, msg);
        if (ret == msg.size())
        {
            fprintf(stdout, "[parent] send done [%d].\n", i);
        }
        else
        {
            fprintf(stdout, "[parent] send failed [%d].\n", i);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    SysPipe::close(wfd);

    ::wait(0);

    fprintf(stdout, "\n###### End Test FIFO Pipe.... ######\n");
    return 0;
}

// 测试具有亲缘关系的两个进程之间使用同一组文件描述符通信。
int test_upipe()
{
    SysUnnamedPipe::pipe_result_t pipe_group = SysUnnamedPipe::pipe2(O_DIRECT | O_NONBLOCK);

    std::string msg = "hello world";

    fprintf(stdout, "\n###### Started Test UnNamed Pipe.... ######\n");

    if (pipe_group.ret)
    {
        fprintf(stdout, "[test_upipe] cannot create unnamed pipe.errno=%d.\n", pipe_group.eno);
        return -1;
    }

    int pid = 0;
    int maxrw = 5;

    if ((pid = fork()) == 0)
    {
        // child
        int rfd = pipe_group.rfd;
        int wfd = pipe_group.wfd;
        int rwcount = 0;
        int loop = 100;

        SysPipe::close(wfd);

        fprintf(stdout, "[child] started. rfd = %d.\n", rfd);

        while (maxrw > rwcount)
        {
            std::string data;
            ssize_t ret = SysPipe::read(rfd, data);
            if (ret > 0)
            {
                fprintf(stdout, "[child] Recv %ld bytes. Times=[%d].\n", ret, rwcount);
                if (data != msg)
                {
                    fprintf(stdout, "[child] Error. Received data is not equal.\n");
                }
                rwcount++;
            }
            else
            {
                fprintf(stdout, "[child] recv Error. ret=%ld, errno=%m.\n", ret, errno);
            }
            if (loop <= 0)
            {
                break;
            }
            loop--;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        SysPipe::close(rfd);
        ::exit(0);
    }

    int wfd = pipe_group.wfd;
    int rfd = pipe_group.rfd;

    SysPipe::close(rfd);

    fprintf(stdout, "[parent] started. wfd = %d.\n", wfd);

    for (int i = 0; i < maxrw; i++)
    {
        ssize_t ret = SysPipe::write(wfd, msg);
        if (ret == msg.size())
        {
            fprintf(stdout, "[parent] send done [%d].\n", i);
        }
        else
        {
            fprintf(stdout, "[parent] send failed [%d].\n", i);
        }
    }

    SysPipe::close(wfd);

    ::wait(0);

    fprintf(stdout, "\n###### End Test UnNamed Pipe....###### \n");

    return 0;
}

int main(int argc, char **argv)
{
    test_upipe();
    test_fifopipe();
    return 0;
}
