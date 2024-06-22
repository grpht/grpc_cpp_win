#pragma once
#pragma warning(disable :4251)
#pragma warning(disable :4819)

#include <iostream>
#include <memory>
#include <string>
#include <unordered_set>
#include <thread>
#include <chrono>
#include <functional>

#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#include "absl/log/check.h"
#include "absl/strings/str_format.h"

#include "RpcJob.h"

class RpcServiceClient
{
public:
	void Connect(const std::string& ip, uint16_t port)
	{
		OnBeforeConnect();
		std::string address = absl::StrFormat("%s:%d", ip, port);
		grpc::ChannelArguments channelArgs;
		channelArgs.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 10000);
		channelArgs.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 5000);
		channelArgs.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
		_channel = grpc::CreateCustomChannel(address, grpc::InsecureChannelCredentials(), channelArgs);
		InitStub(_channel);
		RegisterStream();
		OnAfterConnected();
	}

	void Flush()
	{
		while (true)
		{
			_jobQueue.Flush();
			std::this_thread::sleep_for(std::chrono::milliseconds(9));
		}
	}

	void UnaryReceiveCallback() {
		void* gotTag;
		bool ok = false;
		while (_rpcCompletionQueue.Next(&gotTag, &ok)) {
			auto* call = static_cast<RpcJobBase*>(gotTag);
			CHECK(ok);
			if (call->status.ok())
				_jobQueue.Push(call);
			else
			{
				std::cout << "RPC failed" << std::endl;
				delete call;
			}
		}
	}

	void SetId(const std::string& id) { _id = id; }
	const std::string& GetId() const { return _id; }
protected:
	virtual void InitStub(std::shared_ptr<grpc::Channel> channel) = 0;
	virtual void RegisterStream() {}
	virtual void OnBeforeConnect() {}
	virtual void OnAfterConnected() {}
	
protected:
	std::shared_ptr<grpc::Channel> _channel;
	std::string _id;
	
	grpc::CompletionQueue _rpcCompletionQueue;
	RpcJobQueue<RpcJobBase> _jobQueue;
};