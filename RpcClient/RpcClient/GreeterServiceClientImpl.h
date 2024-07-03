#pragma once

#include "GreeterServiceClient.h"

class GreeterServiceClientImpl : public GreeterServiceClient
{
	GreeterServiceClient* GetInstance() override;

	void OnSayHello(const helloworld::HelloReply* response) override;
	void ClientSayHelloBDS(const HelloReply* response, std::any stream) override;
	void ClientSayHelloStreamReply(const HelloReply* response, std::any stream) override;
	void OnCloseSayHelloStreamReply(const::grpc::Status& status) override;
	void OnFinishSayHelloRecord(HelloReply* response, const grpc::Status& status) override;
};