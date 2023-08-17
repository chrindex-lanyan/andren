
#include "../src/network/andren_network.hh"
#include <chrono>
#include <thread>
#include <unistd.h>

#include <fmt/core.h>

#include <grpc++/grpc++.h>
#include "greet.grpc.pb.h"




using namespace chrindex::andren;

#define errout(...) fprintf(stderr, __VA_ARGS__)
#define genout(...) fprintf(stdout, __VA_ARGS__)

std::atomic<int> g_exit;

std::string serverip = "192.168.88.3";
int32_t serverport = 8317;

using namespace grpc;
using namespace greet;


class GreetServiceImpl final : public GreetService::Service {
    Status SayHello(ServerContext* context, const HelloRequest* request, HelloResponse* response) override 
    {
        genout("Client In....Name = %s.\n",request->name().c_str());
        response->set_message("Hello, " + request->name());
        return Status::OK;
    }
};

int test_server()
{

    ServerBuilder builder;
    GreetServiceImpl service;

    builder.AddListeningPort(fmt::format("{}:{}",serverip,serverport), grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());

    std::cout << "GRPC Server : Server listening on " <<fmt::format("{}:{}",serverip,serverport) << std::endl;

    std::thread([&server]()
    {
        server->Wait();
    }).detach();
    

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


int test_client()
{

    ChannelArguments args;
    args.SetInt(GRPC_ARG_MAX_RECEIVE_MESSAGE_LENGTH, INT_MAX);

    std::shared_ptr<Channel> channel = CreateCustomChannel(fmt::format("{}:{}",serverip,serverport), InsecureChannelCredentials(), args);
    std::unique_ptr<GreetService::Stub> stub = GreetService::NewStub(channel);

    HelloRequest request;
    HelloResponse response;

    request.set_name("Alice");

    for (int i =0 ; i< 10 ;i++)
    {    
        ClientContext context;
        Status status = stub->SayHello(&context, request, &response);
        if (status.ok()) 
        {
            std::cout << "GRPC Client : "<<fmt::format("Count = [{}] , ", i)<<"Server response: " << response.message() << std::endl;
        } 
        else
        {
            std::cout << "GRPC Client : RPC failed with error: " << status.error_message() << std::endl;
        }
    }

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
    int pid = fork();

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

    if (pid > 0)
    {
        test_server();
    }
    else 
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        test_client();
    }
    
    return 0;
}


