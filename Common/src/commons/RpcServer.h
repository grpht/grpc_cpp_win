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
#include <atomic>

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
		if (!_wait)
		{
			_wait = true;
			_server->Wait();
		}
	}

	template<class T>
	T* AddService(const std::string& serviceName)
	{
		auto result = _services.emplace(serviceName, std::make_unique<T>());
		if (!result.second) return nullptr;
		T* casted = dynamic_cast<T*>(result.first->second.get());
		if (casted)
			casted->Prepare(this, &_jobQueue);
		return casted;
	}

	template<typename T>
	T* GetService(const std::string& serviceName)
	{
		auto iter = _services.find(serviceName);
		if (iter != _services.end())
			return static_cast<T*>(iter->second.get());
		return nullptr;
	}

	bool IsShutdown() { return _shutdown.load(); }

	void Shutdown()
	{
		if (!_shutdown.exchange(true))
		{
			for (auto& [serviceName, service] : _services)
			{
				auto castedService = dynamic_cast<RpcServiceBase*>(service.get());
				if (castedService) castedService->Shutdown();
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			while (JobQueueSize() > 0)
			{
				Flush();
				std::this_thread::yield();
			}
			_services.clear();
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			_server->Shutdown(std::chrono::system_clock::now() + std::chrono::seconds(30));
			Wait();
		}
	}

	size_t JobQueueSize() { return _jobQueue.Size(); }
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
	std::map<std::string, std::unique_ptr<grpc::Service>> _services;
	RpcJobQueue<RpcJobBase> _jobQueue;
	std::atomic_bool _shutdown = false;
	bool _wait = false;
};