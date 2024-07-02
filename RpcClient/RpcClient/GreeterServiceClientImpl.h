#pragma once

#include "GreeterServiceClient.h"

class GreeterServiceClientImpl : public GreeterServiceClient
{
	// GreeterServiceClient��(��) ���� ��ӵ�
	GreeterServiceClient* GetInstance() override;

	// GreeterServiceClient��(��) ���� ��ӵ�
	void ClientSayHelloBDS(const HelloReply* response, std::any stream) override;
	void ClientSayHelloStreamReply(const HelloReply* response, std::any stream) override;
	void OnDoneSayHelloStreamReply(const::grpc::Status& status) override;
	void OnSayHello(const helloworld::HelloReply* response) override;

	// GreeterServiceClient��(��) ���� ��ӵ�
	void OnDoneSayHelloRecord(HelloReply* reponse, const grpc::Status& status) override;
};