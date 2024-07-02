#pragma once
#pragma warning(disable : 4251)
#pragma warning(disable : 4819)

#include "GreeterService.h"

class GreeterServiceImpl : public GreeterService
{
	// GreeterService을(를) 통해 상속됨
	GreeterService* GetInstance() override;

	grpc::Status ServerSayHello(const helloworld::HelloRequest* request, helloworld::HelloReply* response) override;
	void ServerSayHelloBDS(grpc::CallbackServerContext* context, const HelloRequest* request, std::any stream) override;
	void ServerSayHelloStreamReply(grpc::CallbackServerContext* context, const HelloRequest* request, std::any stream) override;
	void ServerSayHelloRecord(grpc::CallbackServerContext* context, const HelloRequest* request, std::any stream) override;
	grpc::Status FinishServerSayHelloRecord(HelloReply* response, std::shared_ptr<SayHelloRecordSvrReader> stream) override;
};