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

#include "RpcService.h"
#include "RpcTemplate.h"

#include "greeter.grpc.pb.h"

namespace helloworld
{
	DECLARE_SERVER_UNARY(SayHello, HelloRequest, HelloReply)
	DECLARE_SERVER_BISTREAM(SayHelloBDS, HelloRequest, HelloReply)
	DECLARE_SERVER_SSTREAM(SayHelloStreamReply, HelloRequest, HelloReply)
	DECLARE_SERVER_CSTREAM(SayHelloRecord, HelloRequest, HelloReply)

	class GreeterService : public helloworld::Greeter::CallbackService, public RpcService
	{
	protected:
		virtual GreeterService* GetInstance() = 0;

		class RpcClientSession
		{
		public:
			INNER_SERVER_BISTREAM(SayHelloBDS, HelloRequest, HelloReply)
			INNER_SERVER_SSTREAM(SayHelloStreamReply, HelloRequest, HelloReply)
			INNER_SERVER_CSTREAM(SayHelloRecord, HelloRequest, HelloReply)
		public:
			void SetId(const std::string& id) { if (_id.empty()) _id = id; }
			const std::string& GetId() const { return _id; }
		private:
			std::string _id;
		};
		std::unordered_map<std::string, RpcClientSession> _clients;
	
	public:
		RpcClientSession* GetClient(grpc::CallbackServerContext* context)
		{ return GetClient(GetContextMetaData(context, "id")); }

		RpcClientSession* GetClient(const std::string& id)
		{
			auto it = _clients.find(id);
			if (it != _clients.end())
				return &it->second;
			return nullptr;
		}
	
		METHOD_SERVER_UNARY(SayHello, HelloRequest, HelloReply)
		METHOD_SERVER_BISTREAM(SayHelloBDS, HelloRequest, HelloReply)
		METHOD_SERVER_SSTREAM(SayHelloStreamReply, HelloRequest, HelloReply)
		METHOD_SERVER_CSTREAM(SayHelloRecord, HelloRequest, HelloReply)
	};
}
