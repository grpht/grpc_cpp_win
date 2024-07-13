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

#include "commons/RpcJob.h"
#include "commons/RpcServiceBase.h"

class RpcServer
{
public:
	~RpcServer()
	{
		_server->Shutdown();
		_services.clear();
	}

	void Run(std::string ip, uint16_t port)
	{
		std::string address = std::format("{}:{}", ip, port);
		std::cout << "Rpc Server Run ~ " << address << std::endl;

		grpc::EnableDefaultHealthCheckService(true);
		grpc::reflection::InitProtoReflectionServerBuilderPlugin();
		grpc::ServerBuilder builder;
		for (auto& [_, service] : _services)
		{
			builder.RegisterService(service.get());
		}
		builder.AddListeningPort(address, CredentialSetting());
		ServerSetting(builder);

		_server = builder.BuildAndStart();
		std::cout << "Server listening on" << std::endl;
	}

	void Flush()
	{
		_jobQueue.Flush();
	}

	void Wait()
	{
		_server->Wait();
	}
	void AddService(const std::string& serviceName, std::shared_ptr<grpc::Service> service)
	{
		_services.insert(std::pair(serviceName, service));
		if (auto casted = std::dynamic_pointer_cast<RpcServiceBase>(service))
		{
			casted->Prepare(this, &_jobQueue);
		}
	}

	template<typename T>
	T* GetService(const std::string& serviceName)
	{
		auto iter = _services.find(serviceName);
		if (iter != _services.end())
			return static_cast<T*>(iter->second);
		return nullptr;
	}
protected:
	virtual void ServerSetting(grpc::ServerBuilder& builder)
	{
		/*builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 10000);
		builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 5000);
		builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);*/
	}

	virtual std::shared_ptr<grpc::ServerCredentials> CredentialSetting()
	{
		return grpc::InsecureServerCredentials();
	}
protected:
	std::unique_ptr<grpc::Server> _server;
	std::map<std::string, std::shared_ptr<grpc::Service>> _services;
	RpcJobQueue<RpcJobBase> _jobQueue;
};