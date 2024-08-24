#pragma once

#include <iostream>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "commons/RpcJob.h"

class RpcServiceBase
{
public:
	void Prepare(class RpcServer* server, RpcJobQueue<RpcJobBase>* jobQ)
	{
		_server = server;
		_jobQueue = jobQ;
	}

	std::string GetContextMetaData(grpc::CallbackServerContext* context, const std::string& key)
	{
		auto it = context->client_metadata().find(key);
		if (it == context->client_metadata().end())
			return  "";
		return std::string(it->second.data(), it->second.length());
	}

	unsigned int GetContextId(grpc::CallbackServerContext* context)
	{
		return static_cast<unsigned int>(std::stoul(GetContextMetaData(context, "id")));
	}
	virtual void Shutdown() {}
protected:
	RpcServer* _server = nullptr;
	RpcJobQueue<RpcJobBase>* _jobQueue = nullptr;
};

