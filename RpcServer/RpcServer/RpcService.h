#pragma once

#include <iostream>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "RpcJob.h"

class RpcService
{
public:
	void Prepare(class RpcServer* server, RpcJobQueue<RpcJobBase>* jobQ)
	{
		_server = server;
		_jobQueue = jobQ;
	}

	bool TryFindContextMetaData(grpc::CallbackServerContext* context, const std::string& key, std::string& outValue)
	{
		auto it = context->client_metadata().find(key);
		if (it != context->client_metadata().end())
		{
			outValue = std::string(it->second.data(), it->second.length());
			return  true;
		}
		return false;
	}
protected:
	RpcServer* _server = nullptr;
	RpcJobQueue<RpcJobBase>* _jobQueue = nullptr;
};

