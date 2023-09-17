
#include "../src//network//andren_network.hh"
#include <cassert>
#include <fcntl.h>
#include <memory>



using namespace chrindex::andren;

#define errout(...) fprintf(stderr, __VA_ARGS__)
#define genout(...) fprintf(stdout, __VA_ARGS__)

std::atomic<int> g_exit;



int testAFIO()
{
    std::shared_ptr<network::TaskDistributor> ev = std::make_shared<network::TaskDistributor>(4);
    bool bret = false;

    bret = ev->start();
    assert(bret);

    std::shared_ptr<network::AFile> afile = std::make_shared<network::AFile>();

    afile->setEventLoop(ev);

    /// 这个是一次性的任务，
    /// 因此保存强引用不会导致无法析构
    bret = afile->areopen([afile](bool isOpen)
    {
        if(!isOpen)
        {
            errout("Cannot Open File...\n");
            return ;
        }
        genout("Opened!!.\n");

        bool bret = afile->aread([](ssize_t ret , std::string && data)
        {
            genout("Read Done! Ssize_t = %ld , Data = %s.",ret,data.c_str());
        });

        assert(bret);

    },"/proc/version",O_RDONLY);

    assert(bret);

    while(1)
    {
        if (g_exit == 1)
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return 0;
}




int main(int argc, char ** argv)
{
    g_exit = 0;
    if (signal(SIGINT,
               [](int sig) -> void
               {
                   genout("\n准备退出....\n");
                   g_exit = 1;
               }) == SIG_ERR)
    {
        errout("Cannot registering signal handler");
        return -3;
    }


    testAFIO();


    return 0;
}




