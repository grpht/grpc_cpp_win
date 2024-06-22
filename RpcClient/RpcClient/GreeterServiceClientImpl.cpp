#include "GreeterServiceClientImpl.h"

using namespace std;
GreeterServiceClient* GreeterServiceClientImpl::GetInstance()
{
	return this;
}


void GreeterServiceClientImpl::OnSayHello(const helloworld::HelloReply* response)
{
	cout << "OnSayHello" << endl;
	cout << response->message() << endl;
}


void GreeterServiceClientImpl::ClientSayHelloBDS(const HelloReply* response, std::any stream)
{
	cout << "ClientSayHelloBDS" << endl;
	cout << response->message() << endl;
}

void GreeterServiceClientImpl::ClientSayHelloStreamReply(const HelloReply* response, std::any stream)
{
	cout << "ClientSayHelloStreamReply" << endl;
	cout << response->message() << endl;
}

void GreeterServiceClientImpl::OnDoneSayHelloStreamReply(const::grpc::Status& status)
{
	cout << "OnDoneSayHelloStreamReply" << endl;
	cout << status.error_code() << endl;
}
