#pragma once
#pragma warning(disable :4251)
#pragma warning(disable :4819)

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <tuple>
#include <thread>
#include <chrono>
#include <map>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"

#include "GreeterServiceImpl.h"

using namespace helloworld;

enum class RpcServiceType
{
	//@SERVICE_TYPE
	Greeter,
};

class RpcServer
{
public:
	~RpcServer()
	{
		_server->Shutdown();
		for (auto& [type, service] : _services)
		{
			delete service;
			service = nullptr;
		}
	}

	void Run(std::string ip, uint16_t port)
	{
		std::string address = absl::StrFormat("%s:%d", ip, port);
		std::cout << "Rpc Server Run ~ " << address << std::endl;

		std::thread flushThread = std::thread(&RpcServer::Flush, this);

		grpc::EnableDefaultHealthCheckService(true);
		grpc::reflection::InitProtoReflectionServerBuilderPlugin();
		grpc::ServerBuilder builder;
		builder.AddListeningPort(address, GetServerCredentials());
		ServerSetting(builder);

		//@REGISTER_SERVICE
		auto Greeter = new GreeterServiceImpl();
		Greeter->Prepare(this, &_jobQueue);
		_services.insert(std::pair(RpcServiceType::Greeter, Greeter));
		builder.RegisterService(Greeter);

		_server = builder.BuildAndStart();
		std::cout << "Server listening on" << std::endl;
		_server->Wait();
		flushThread.join();
	}

	void Flush()
	{
		while (true)
		{
			_jobQueue.Flush();
			std::this_thread::sleep_for(std::chrono::milliseconds(9));
		}
	}

	template<typename T>
	T* GetService(RpcServiceType type)
	{
		auto iter = _services.find(type);
		if (iter != _services.end())
			return static_cast<T*>(iter->second);
		return nullptr;
	}
protected:
	virtual void ServerSetting(grpc::ServerBuilder& builder)
	{
		builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 10000);
		builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 5000);
		builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
	}

	virtual std::shared_ptr<grpc::ServerCredentials> GetServerCredentials()
	{
		return grpc::InsecureServerCredentials();
	}
protected:
	std::unique_ptr<grpc::Server> _server;
	std::map<RpcServiceType, RpcService*> _services;
	RpcJobQueue<RpcJobBase> _jobQueue;
};