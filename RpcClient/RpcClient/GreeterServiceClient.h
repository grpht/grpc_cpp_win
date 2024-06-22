#pragma once
#include "RpcTemplate.h"

#include "greeter.grpc.pb.h"

#include "RpcServiceClient.h"

using namespace helloworld;

class GreeterServiceClient : public RpcServiceClient
{
protected:
	void InitStub(std::shared_ptr<grpc::Channel> channel) override { _stub = Greeter::NewStub(channel); }
	virtual GreeterServiceClient* GetInstance() = 0;
protected:
	std::unique_ptr<Greeter::Stub> _stub;
public:
	//@SECTION_CLIENT_UNARY
	CLIENT_UNARY(SayHello, HelloRequest, HelloReply)

	//@SECTION_CLIENT_BISTREAM
	//CLIENT_BISTREAM
protected:
	class SayHelloBDSCltBiStream
		: public grpc::ClientBidiReactor<HelloRequest, HelloReply>
		, public RpcWriteModule<HelloRequest>
		, public RpcClientReadModule<HelloReply>
		, public RpcDoneModule<SayHelloBDSCltBiStream>
	{
	public:
		void Start(Greeter::Stub* stub, const std::string& id)
		{
			_id = id;
			_context.AddMetadata("id", _id);
			stub->async()->SayHelloBDS(&_context, this);
			StartRead(&_readMessage);
			StartCall();
		}
		void OnReadDone(bool ok) override {
			if (!ok) return;
			RpcNextRead();
		}
		void OnDone(const grpc::Status& s) override
		{
			_status = s; _done = true; _doneCallback(this, s); delete this;
		}
		void Send(HelloRequest& message) {
			if (_done) return;
			RpcRequestWrite(message);
		}
		void OnWriteDone(bool ok) override {
			if (!ok) {
				RemoveHold();
				return;
			}
			RpcNextWrite();
		}
	protected:
		void RpcWrite(HelloRequest* message) override { StartWrite(message); }
		void RpcRead(HelloReply* message) override { StartRead(message); }
	private:
		std::string _id;
		grpc::ClientContext _context;
	};
	SayHelloBDSCltBiStream* SayHelloBDSStream;
	virtual void ClientSayHelloBDS(const HelloReply* response, std::any stream) = 0;
public:
	void ServerSayHelloBDS(HelloRequest& request) { SayHelloBDSStream->Send(request); }

	//@SECTION_CLIENT_SSTREAM
	//CLIENT_SSTREAM
protected:
	class SayHelloStreamCltReader
		: public grpc::ClientReadReactor<HelloReply>
		, public RpcClientReadModule<HelloReply>
		, public RpcDoneModule<SayHelloStreamCltReader>
	{
	public:
		void Start(Greeter::Stub* stub, const std::string& id, const HelloRequest& request)
		{
			_id = id;
			_context.AddMetadata("id", _id);
			stub->async()->SayHelloStreamReply(&_context, &request, this);
			StartRead(&_readMessage);
			StartCall();
		}
		void OnReadDone(bool ok) override {
			if (!ok) return;
			RpcNextRead();
		}
		void OnDone(const grpc::Status& s) override 
		{
			_status = s; _done = true; _doneCallback(this, s); delete this;
		};
	protected:
		void RpcRead(HelloReply* message) override { StartRead(message); }
	protected:
		std::string _id;
		grpc::ClientContext _context;
	};
	virtual void ClientSayHelloStreamReply(const HelloReply* response, std::any stream) = 0;
	virtual void OnDoneSayHelloStreamReply(const ::grpc::Status& status) = 0;
public:
	void ServerSayHelloStreamReply(const HelloRequest& request) {
		auto stream = new SayHelloStreamCltReader();
		stream->RegisterRead(&_jobQueue, [this](const HelloReply* response, std::any stream) { GetInstance()->ClientSayHelloStreamReply(response, stream); });
		stream->RegisterDone([this](SayHelloStreamCltReader* self, const grpc::Status& s) { GetInstance()->OnDoneSayHelloStreamReply(s); });
		stream->Start(_stub.get(), _id, request);
	}
protected:
	void RegisterStream() override
	{
		SayHelloBDSStream = new SayHelloBDSCltBiStream();
		SayHelloBDSStream->RegisterRead(&_jobQueue, [this](const HelloReply* response, std::any stream) { GetInstance()->ClientSayHelloBDS(response, stream); });
		SayHelloBDSStream->RegisterDone([this](SayHelloBDSCltBiStream* self, const grpc::Status& s) { SayHelloBDSStream = nullptr; });
		SayHelloBDSStream->Start(_stub.get(), _id);
	}
};