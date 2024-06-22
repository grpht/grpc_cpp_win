#pragma once

#include "GreeterServiceClient.h"

class GreeterServiceClientImpl : public GreeterServiceClient
{
	// GreeterServiceClient을(를) 통해 상속됨
	GreeterServiceClient* GetInstance() override;

	// GreeterServiceClient을(를) 통해 상속됨
	void ClientSayHelloBDS(const HelloReply* response, std::any stream) override;
	void ClientSayHelloStreamReply(const HelloReply* response, std::any stream) override;
	void OnDoneSayHelloStreamReply(const::grpc::Status& status) override;
	void OnSayHello(const helloworld::HelloReply* response) override;
};