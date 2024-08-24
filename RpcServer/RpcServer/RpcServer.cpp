// RpcServer.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//
#include <iostream>
#include <crtdbg.h>
#include <memory>

#include "commons/RpcServer.h"
#include "GreeterServiceImpl.h"

helloworld::GreeterService* greeterService = nullptr;
RpcServer* server = nullptr;
bool _quit = false;
void Flush(RpcServer* runServer)
{
    while (!runServer->IsShutdown()) {
        runServer->Flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(9));
    }
}

void Quit(RpcServer* runServer)
{
    std::string s;
    std::cin >> s;
    runServer->Shutdown();
}


int main()
{
    //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    server = new RpcServer();
    
    greeterService = server->AddService<helloworld::GreeterServiceImpl>("greeter");
    
    server->Run("0.0.0.0", 9999);
    std::thread flushThread = std::thread(Flush, server);
    std::thread quitThread = std::thread(Quit, server);

    server->Wait();
    if (flushThread.joinable())
        flushThread.join();
    if (quitThread.joinable())
        quitThread.join();
}
