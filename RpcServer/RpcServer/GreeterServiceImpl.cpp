#include "GreeterServiceImpl.h"

using namespace std;

GreeterService* GreeterServiceImpl::GetInstance()
{
	return this;
}

grpc::Status GreeterServiceImpl::ServerSayHello(grpc::CallbackServerContext* context, const HelloRequest* request, HelloReply* response)
{
	cout << "UNARY" << request->name() << endl;
	response->set_message("UNARY " + request->name());

	return grpc::Status::OK;
}

void GreeterServiceImpl::ServerSayHelloBDS(grpc::CallbackServerContext* context, const HelloRequest* request, std::any stream)
{
	cout << "BiStream " << request->name() << endl;
	HelloReply response;
	response.set_message("BISTREAM " + request->name());
	if (auto s = CAST_SERVER_STREAM(SayHelloBDS, stream))
	{
		s->Send(response);
	}
}

void GreeterServiceImpl::ServerSayHelloStreamReply(grpc::CallbackServerContext* context, const HelloRequest* request, std::any stream)
{
	cout << "ServerSayHelloStreamReply" << endl;
	if (auto s = CAST_SERVER_STREAM(SayHelloStreamReply, stream))
	{
		for (int i = 0; i < 100; ++i)
		{
			HelloReply response;
			response.set_message("S->C SStream " + request->name() + to_string(i));
			s->Send(response);
			std::this_thread::sleep_for(std::chrono::milliseconds(9));
		}
		s->Close(grpc::Status::OK);
	}
}

void GreeterServiceImpl::ServerSayHelloRecord(grpc::CallbackServerContext* context, const HelloRequest* request, std::any stream)
{
	cout << "CSTREAM " <<request->name() << endl;
	++cStreamCall;
}

grpc::Status GreeterServiceImpl::ServerFinishSayHelloRecord(HelloReply* response, std::shared_ptr<SayHelloRecordSvrStream> stream)
{
	response->set_message("CStream Result:" + std::to_string(cStreamCall));
	return grpc::Status::OK;
}
