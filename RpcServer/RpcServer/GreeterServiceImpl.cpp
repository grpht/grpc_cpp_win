#include "GreeterServiceImpl.h"

using namespace std;
using namespace helloworld;

GreeterService* GreeterServiceImpl::GetInstance()
{
	return this;
}

grpc::Status GreeterServiceImpl::ServerSayHello(const HelloRequest* request, HelloReply* response, SayHelloSvrStream* stream)
{
	cout << "UNARY" << request->name() << endl;
	response->set_message("UNARY " + request->name());

	return grpc::Status::OK;
}

void GreeterServiceImpl::ServerSayHelloBDS(const HelloRequest* request, SayHelloBDSSvrStream* stream)
{
	cout << "BiStream " << request->name() << endl;
	HelloReply response;
	response.set_message("BISTREAM " + request->name());
	stream->Send(response);
}

void helloworld::GreeterServiceImpl::OnOpenSayHelloBDS(std::shared_ptr<SayHelloBDSSvrStream> stream)
{
	auto& id = stream->GetId();
	auto& client = _clients[id];
	client.SetId(id);
	client.SayHelloBDSPtr = stream;
}

void helloworld::GreeterServiceImpl::OnCloseSayHelloBDS(std::shared_ptr<SayHelloBDSSvrStream> stream)
{
	auto& id = stream->GetId();
	auto& client = _clients[id];
	client.SayHelloBDSPtr = nullptr;
}

void GreeterServiceImpl::ServerSayHelloStreamReply(const HelloRequest* request, SayHelloStreamReplySvrStream* stream)
{
	cout << "ServerSayHelloStreamReply" << endl;
	for (int i = 0; i < 100; ++i)
	{
		HelloReply response;
		response.set_message("S->C SStream " + request->name() + to_string(i));
		stream->Send(response);
	}
	stream->Close(grpc::Status::OK);
}

void helloworld::GreeterServiceImpl::OnOpenSayHelloStreamReply(std::shared_ptr<SayHelloStreamReplySvrStream> stream)
{
	auto& id = stream->GetId();
	auto& client = _clients[id];
	client.SetId(id);
	client.SayHelloStreamReplyPtr = stream;
}

void helloworld::GreeterServiceImpl::OnCloseSayHelloStreamReply(std::shared_ptr<SayHelloStreamReplySvrStream> stream)
{
	auto& id = stream->GetId();
	if (!_clients.contains(id))
		return;
	auto& client = _clients[id];
	client.SayHelloStreamReplyPtr = nullptr;
}

void GreeterServiceImpl::ServerSayHelloRecord(const HelloRequest* request, SayHelloRecordSvrStream* stream)
{
	cout << "CSTREAM " <<request->name() << endl;
	++cStreamCall;
}

void helloworld::GreeterServiceImpl::OnOpenSayHelloRecord(std::shared_ptr<SayHelloRecordSvrStream> stream)
{
	auto& id = stream->GetId();
	auto& client = _clients[id];
	client.SetId(id);
	client.SayHelloRecordPtr = stream;
}

void helloworld::GreeterServiceImpl::OnCloseSayHelloRecord(std::shared_ptr<SayHelloRecordSvrStream> stream)
{
	auto& id = stream->GetId();
	if (!_clients.contains(id))
		return;
	auto& client = _clients[id];
	client.SayHelloRecordPtr = nullptr;
}

grpc::Status GreeterServiceImpl::ServerFinishSayHelloRecord(HelloReply* response, SayHelloRecordSvrStream* stream)
{
	response->set_message("CStream Result:" + std::to_string(cStreamCall));
	return grpc::Status::OK;
}
