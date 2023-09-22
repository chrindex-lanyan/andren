
#include "../src/network/andren_network.hh"
#include <cassert>
#include <chrono>
#include <cmath>
#include <fcntl.h>
#include <thread>



using namespace chrindex::andren;



#define errout(...) fprintf(stderr, __VA_ARGS__)
#define genout(...) fprintf(stdout, __VA_ARGS__)

std::atomic<int> m_exit;
std::string file_path = "io_file.txt";
int directory_fd = AT_FDCWD ;

int test_io_file()
{
    network::EventLoop eventloop(4);
    network::IOService io_service (base::create_uid_u64(), 32);
    network::io_file myfile ;
    sigset_t sigmask;
    bool bret ;
    
    sigemptyset(&sigmask);
    sigaddset(&sigmask, SIGINT);

    eventloop.start();

    io_service.init(sigmask);
    bret = eventloop.addService(&io_service);
    assert(bret);

    bret = myfile.async_open([&io_service](network::io_file * pfile,int32_t ret)
    {
        bool bret;
        genout("Open File , Fd = %d.\n",ret);
        if (ret <=0)
        {
            return ;
        }
        bret = pfile->async_read([](network::io_file * pfile, std::string && data, size_t size) 
        {
            genout("Read Done. Size = %zu , Contents = %s.\n", size, data.c_str());
        }, 
        io_service);
        assert(bret);
    }, 
    io_service, 
    file_path, 
    directory_fd , 
    O_RDWR | O_CREAT,
    0644);

    assert(bret);

    int count = 500;

    while (m_exit != 1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if ( 0 > count--)
        {
            break;
        }
    }

    eventloop = network::EventLoop{};

    return 0;
}




int main(int argc , char ** argv)
{
    m_exit = 0;
    if (signal(SIGINT,
               [](int sig) -> void
               {
                   genout(" \n::准备退出....\n");
                   m_exit = 1;
               }) == SIG_ERR)
    {
        errout("Cannot registering signal handler");
        return -3;
    }

    test_io_file();

    return 0;
}


