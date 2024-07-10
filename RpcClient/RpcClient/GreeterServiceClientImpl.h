#pragma once

#include "GreeterServiceClient.h"

class GreeterServiceClientImpl : public GreeterServiceClient
{
	GreeterServiceClient* GetInstance() override;

	void OnSayHello(const helloworld::HelloReply* response) override;
	void ClientSayHelloBDS(const HelloReply* response, SayHelloBDSCltStream* stream) override;
	void ClientSayHelloStreamReply(const HelloReply* response, SayHelloStreamReplyCltStream* stream) override;
	void OnCloseSayHelloStreamReply(const::grpc::Status& status) override;
	void OnFinishSayHelloRecord(HelloReply* response, const grpc::Status& status) override;
};