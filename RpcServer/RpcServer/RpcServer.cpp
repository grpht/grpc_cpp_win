// RpcServer.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//
#include <iostream>
#include <crtdbg.h>
#include <memory>

#include "commons/RpcServer.h"
#include "GreeterServiceImpl.h"

std::shared_ptr<helloworld::GreeterService> greeterService = nullptr;
RpcServer* server = nullptr;

void Flush(RpcServer* runServer)
{
    while (true) {
        runServer->Flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(9));
    }
}


int main()
{
    //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    server = new RpcServer();
    
    greeterService = std::make_shared<helloworld::GreeterServiceImpl>();
    server->AddService("greeter", greeterService);
    
    server->Run("0.0.0.0", 9999);
    std::thread flushThread = std::thread(Flush, server);

    server->Wait();
    flushThread.join();
}
