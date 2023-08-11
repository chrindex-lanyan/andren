
#include "../src/network/andren_network.hh"


using namespace chrindex::andren;

#define errprintf(...) fprintf(stderr, __VA_ARGS__)
#define stdprintf(...) fprintf(stdout, __VA_ARGS__)

std::atomic<int> m_exit = 0;

std::string serverip = "192.168.88.3";
int32_t serverport = 8317;

int testTcpServer()
{
    // 设置CTRL+C信号回调
    if (signal(SIGINT,
               [](int sig) -> void
               {
                   stdprintf("TCP Server : 准备退出....\n");
                   m_exit = 1;
               }) == SIG_ERR)
    {
        errprintf("TCP Server : cannot registering signal handler");
        return -3;
    }

    


}

int testTcpClient()
{
    // 设置CTRL+C信号回调
    if (signal(SIGINT,
               [](int sig) -> void
               {
                   stdprintf("TCP Client : 准备退出....\n");
                   m_exit = 1;
               }) == SIG_ERR)
    {
        errprintf("TCP Client : cannot registering signal handler");
        return -3;
    }



    return 0;
}

int testTCPServerAndClient()
{
    int pid = fork();

    if (pid ==0)
    {
        testTcpClient();
    }else 
    {
        testTcpServer();
    }

    return 0;
}



int main()
{
    testTCPServerAndClient();
    return 0;
}





