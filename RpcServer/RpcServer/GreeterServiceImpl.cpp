#include "GreeterServiceImpl.h"

using namespace std;

GreeterService* GreeterServiceImpl::GetInstance()
{
	return this;
}

grpc::Status GreeterServiceImpl::ServerSayHello(const helloworld::HelloRequest* request, helloworld::HelloReply* response)
{
	cout << "serverSayHello" << endl;

	string prefix("Hello");
	response->set_message(prefix + request->name());
	cout << response->message() << endl;

	return grpc::Status::OK;
}

void GreeterServiceImpl::ServerSayHelloBDS(grpc::CallbackServerContext* context, const HelloRequest* request, std::any stream)
{
	HelloReply response;
	string prefix("Hello ");
	response.set_message(prefix + request->name());
	auto client = GetClient(context);
	if (client)
	{
		cout << "session: " << client->GetId() << endl;
		cout << request->name() << endl;
		client->ClientSayHelloBDS(response);
	}
}

void GreeterServiceImpl::ServerSayHelloStreamReply(grpc::CallbackServerContext* context, const HelloRequest* request, std::any stream)
{
	cout << "ServerSayHelloStreamReply" << endl;
	if (auto s = CAST_SERVER_WRITER(SayHelloStreamReply, stream))
	{
		for (int i = 0; i < 5000; ++i)
		{
			HelloReply response;
			response.set_message(request->name() + "hi" + to_string(i));
			cout << response.message() << endl;
			s->Send(response);
		}
		s->Finish(grpc::Status::OK);
	}
}
