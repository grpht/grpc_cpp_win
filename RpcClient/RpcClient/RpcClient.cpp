// RpcClient.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"

#include "GreeterServiceClientImpl.h"

using namespace helloworld;

void TestUnary(GreeterServiceClientImpl* greeter)
{
    for (int i = 0; i < 100; ++i)
    {
        std::string user("unary" + std::to_string(i));
        HelloRequest request;
        request.set_name(user);
        greeter->ServerSayHello(request);
        std::this_thread::sleep_for(std::chrono::milliseconds(9));
    }
}

void TestBiStream(GreeterServiceClientImpl* greeter)
{
    for (int i = 0; i < 100; i++) {
        std::string user("bidistream " + std::to_string(i));
        HelloRequest request;
        request.set_name(user);
        greeter->ServerSayHelloBDS(request);
        std::this_thread::sleep_for(std::chrono::milliseconds(9));
    }
}

void TestServerStream(GreeterServiceClientImpl* greeter)
{
    HelloRequest req;
    req.set_name("TestServerStream");
    greeter->ServerSayHelloStreamReply(req);
}

void TestClientStream(GreeterServiceClientImpl* greeter)
{
    for (int i = 0; i < 100; ++i)
    {
        HelloRequest req;
        req.set_name("TestClientStream" + std::to_string(i));
        greeter->ServerSayHelloRecord(req);
        std::this_thread::sleep_for(std::chrono::milliseconds(9));
    }
    greeter->ServerFinishSayHelloRecord();
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
    std::thread testUnary = std::thread(TestUnary, &client);
    std::thread testBistream = std::thread(TestBiStream, &client);
    std::thread testServerStream = std::thread(TestServerStream, &client);
    std::thread testClientStream = std::thread(TestClientStream, &client);
    testUnary.join();
    testBistream.join();
    testServerStream.join();
    testClientStream.join();

    unaryCallbackThread.join();
    flushThread.join();
    return 0;
}

