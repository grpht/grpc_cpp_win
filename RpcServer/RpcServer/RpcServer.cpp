﻿// RpcServer.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//
#include "RpcServer.h"
#include <iostream>
#include <crtdbg.h>

int main()
{
    //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    RpcServer server;
    server.Run("0.0.0.0", 9999);
}
