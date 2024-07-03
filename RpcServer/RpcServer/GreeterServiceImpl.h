#pragma once
#pragma warning(disable : 4251)
#pragma warning(disable : 4819)

#include "GreeterService.h"

class GreeterServiceImpl : public GreeterService
{
	// GreeterService��(��) ���� ��ӵ�
	GreeterService* GetInstance() override;

	grpc::Status ServerSayHello(const HelloRequest* request, HelloReply* response) override;
	void ServerSayHelloBDS(grpc::CallbackServerContext* context, const HelloRequest* request, std::any stream) override;
	void ServerSayHelloStreamReply(grpc::CallbackServerContext* context, const HelloRequest* request, std::any stream) override;
	void ServerSayHelloRecord(grpc::CallbackServerContext* context, const HelloRequest* request, std::any stream) override;
	grpc::Status ServerFinishSayHelloRecord(HelloReply* response, std::shared_ptr<SayHelloRecordSvrStream> stream) override;

private:
	int cStreamCall = 0;
};