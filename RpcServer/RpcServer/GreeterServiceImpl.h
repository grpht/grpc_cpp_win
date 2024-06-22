#pragma once
#pragma warning(disable : 4251)
#pragma warning(disable : 4819)

#include "GreeterService.h"

class GreeterServiceImpl : public GreeterService
{
	// GreeterService��(��) ���� ��ӵ�
	void ServerSayHelloBDS(grpc::CallbackServerContext* context, const HelloRequest* request, std::any stream) override;
	void ServerSayHelloStreamReply(grpc::CallbackServerContext* context, const HelloRequest* request, std::any stream) override;
	grpc::Status ServerSayHello(const helloworld::HelloRequest* request, helloworld::HelloReply* response) override;

	// GreeterService��(��) ���� ��ӵ�
	GreeterService* GetInstance() override;
};