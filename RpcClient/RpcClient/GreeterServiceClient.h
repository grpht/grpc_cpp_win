#pragma once
#include "RpcTemplate.h"

#include "greeter.grpc.pb.h"

#include "RpcServiceClient.h"

namespace helloworld
{
	class GreeterServiceClient : public RpcServiceClient
	{
	protected:
		void InitStub(std::shared_ptr<grpc::Channel> channel) override { _stub = Greeter::NewStub(channel); }
		virtual GreeterServiceClient* GetInstance() = 0;
	protected:
		std::unique_ptr<Greeter::Stub> _stub;
	public:
		CLIENT_UNARY(SayHello, HelloRequest, HelloReply)
		CLIENT_BISTREAM(SayHelloBDS, HelloRequest, HelloReply)
		CLIENT_SSTREAM(SayHelloStreamReply, HelloRequest, HelloReply)
		CLIENT_CSTREAM(SayHelloRecord, HelloRequest, HelloReply)
	};
}
