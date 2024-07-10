#include "GreeterServiceClientImpl.h"

using namespace std;
GreeterServiceClient* GreeterServiceClientImpl::GetInstance()
{
	return this;
}


void GreeterServiceClientImpl::OnSayHello(const helloworld::HelloReply* response)
{
	cout << "OnSayHello: " << response->message() << endl;
}

void GreeterServiceClientImpl::ClientSayHelloBDS(const HelloReply* response, SayHelloBDSCltStream* stream)
{
	cout << "ClientSayHelloBDS: " << response->message() << endl;
}

void GreeterServiceClientImpl::ClientSayHelloStreamReply(const HelloReply* response, SayHelloStreamReplyCltStream* stream)
{
	cout << "ClientSayHelloStreamReply: " << response->message() << endl;
}

void GreeterServiceClientImpl::OnCloseSayHelloStreamReply(const::grpc::Status& status)
{
	cout << "OnDoneSayHelloStreamReply: " << status.error_code() << endl;
}

void GreeterServiceClientImpl::OnFinishSayHelloRecord(HelloReply* response, const grpc::Status& status)
{
	cout << "OnFinishSayHelloRecord: " << response->message() << endl;
}