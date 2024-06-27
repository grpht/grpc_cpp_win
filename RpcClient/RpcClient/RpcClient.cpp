// RpcClient.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"

#include "GreeterServiceClientImpl.h"

void TestUnary(GreeterServiceClientImpl* greeter)
{
    for (int i = 0; i < 5; ++i)
    {
        std::string user("unary" + std::to_string(i));
        HelloRequest request;
        request.set_name(user);
        greeter->ServerSayHello(request);
    }
}

void TestBiStream(GreeterServiceClientImpl* greeter)
{
    for (int i = 0; i < 5000; i++) {
        std::string user("bidistream " + std::to_string(i));
        HelloRequest request;
        request.set_name(user);
        greeter->ServerSayHelloBDS(request);
    }
}

void TestServerStream(GreeterServiceClientImpl* greeter)
{
    HelloRequest req;
    req.set_name("zzz");
    greeter->ServerSayHelloStreamReply(req);
}

int main(int argc, char* argv[])
{
    std::cout << "Hello World!\n";
    GreeterServiceClientImpl client;
    client.SetId(argc == 1 ? "0": argv[1]);

    std::thread unaryCallbackThread = std::thread(&RpcServiceClient::UnaryReceiveCallback, &client);
    std::thread flushThread = std::thread(&RpcServiceClient::Flush, &client);

    client.Connect("localhost", 9999);

    //test
    //std::thread testUnary = std::thread(TestUnary, &client);
    //std::thread testBistream = std::thread(TestBiStream, &client);
    std::thread testServerStream = std::thread(TestServerStream, &client);

    //testUnary.join();
    //testBistream.join();
    testServerStream.join();

    unaryCallbackThread.join();
    flushThread.join();
    return 0;
}

