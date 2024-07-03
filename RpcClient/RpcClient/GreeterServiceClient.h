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
	class SayHelloBDSCltStream
		: public grpc::ClientBidiReactor<HelloRequest, HelloReply>
		, public std::enable_shared_from_this<SayHelloBDSCltStream>
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
		void SetPtr(std::shared_ptr< SayHelloBDSCltStream> ptr) { _ptr = ptr; }
		void RegisterDone(std::function<void(SayHelloBDSCltStream*, const grpc::Status&)> doneCallback)
		{ _doneCallback = doneCallback; }
		void OnDone(const grpc::Status& s) override
		{
			_status = s;
			_done = true;
			if (_doneCallback) _doneCallback(this, s);
			if (!_sending.load())
				_ptr = nullptr;
		}
		void Send(HelloRequest& message) {
			if (_done) return;
			auto request = std::make_unique<HelloRequest>(message);
			
			std::lock_guard<std::mutex> lock(_mu);
			if (!_sending.exchange(true))
			{
				_currentSending = std::move(request);
				StartWrite(_currentSending.get());
			}
			else
			{
				_pendingSend.push(std::move(request));
			}
		}

		void OnWriteDone(bool ok) override {
			if (!ok) {
				RemoveHold();
				return;
			}
			std::lock_guard<std::mutex> lock(_mu);
			if (!_pendingSend.empty()) {
				_currentSending = std::move(_pendingSend.front());
				_pendingSend.pop();
				StartWrite(_currentSending.get());
			}
			else {
				_currentSending = nullptr;
				_sending.store(false);
				if (_done) _ptr = nullptr;
			}
		}

		void RegisterRead(RpcJobQueue<RpcJobBase>* jobQ, std::function<void(const HelloReply*, std::any stream)> readCallback)
		{
			_jobQueue = jobQ;
			_readCallback = readCallback;
		}

		void OnReadDone(bool ok) override {
			if (!ok) return;
			auto* call = new RpcJob<HelloReply>();
			call->data = std::make_unique<HelloReply>(_readMessage);
			call->stream = shared_from_this();
			call->execute = [this](google::protobuf::Message* message, std::any stream) {
				auto* castedMessage = static_cast<HelloReply*>(message);
				this->_readCallback(castedMessage, stream);
				};
			_jobQueue->Push(call);
			StartRead(&_readMessage);
		}
	private:
		std::string _id;
		grpc::ClientContext _context;
		std::shared_ptr<SayHelloBDSCltStream> _ptr;

		grpc::Status _status;
		bool _done = false;
		std::function<void(SayHelloBDSCltStream*, const grpc::Status&)> _doneCallback;

		std::atomic<bool> _sending{ false };
		std::queue<std::unique_ptr<HelloRequest>> _pendingSend;
		std::unique_ptr<HelloRequest> _currentSending;
		std::mutex _mu;

		HelloReply _readMessage;
		RpcJobQueue<RpcJobBase>* _jobQueue = nullptr;
		std::function<void(const HelloReply*, std::any stream)> _readCallback;
	};
	std::shared_ptr<SayHelloBDSCltStream> SayHelloBDSStream;
	virtual void ClientSayHelloBDS(const HelloReply* response, std::any stream) = 0;
	virtual void OnCloseSayHelloBDS(const grpc::Status& status) {};
public:
	void ServerSayHelloBDS(HelloRequest& request) {
		OpenSayHelloBDS();
		if (SayHelloBDSStream) SayHelloBDSStream->Send(request);
	}
	void OpenSayHelloBDS()
	{
		if (!SayHelloBDSStream)
		{
			SayHelloBDSStream = std::make_shared<SayHelloBDSCltStream>();
			SayHelloBDSStream->SetPtr(SayHelloBDSStream);
			SayHelloBDSStream->RegisterRead(&_jobQueue, [this](const HelloReply* response, std::any stream) { GetInstance()->ClientSayHelloBDS(response, stream); });
			SayHelloBDSStream->RegisterDone([this](SayHelloBDSCltStream* self, const grpc::Status& s) { SayHelloBDSStream = nullptr; });
			SayHelloBDSStream->Start(_stub.get(), _id);
		}
	}

	//CLIENT_SSTREAM
protected:
	class SayHelloStreamReplyCltStream
		: public grpc::ClientReadReactor<HelloReply>
		, public std::enable_shared_from_this<SayHelloStreamReplyCltStream>
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
		void SetPtr(std::shared_ptr<SayHelloStreamReplyCltStream> ptr) { _ptr = ptr; }
		void RegisterDone(std::function<void(SayHelloStreamReplyCltStream*, const grpc::Status&)> doneCallback)
		{ _doneCallback = doneCallback; }
		void OnDone(const grpc::Status& s) override
		{
			_status = s;
			_done = true;
			if (_doneCallback) _doneCallback(this, s);
			_ptr = nullptr;
		};

		void RegisterRead(RpcJobQueue<RpcJobBase>* jobQ, std::function<void(const HelloReply*, std::any stream)> readCallback)
		{
			_jobQueue = jobQ;
			_readCallback = readCallback;
		}
		void OnReadDone(bool ok) override {
			if (!ok) return;
			auto* call = new RpcJob<HelloReply>();
			call->data = std::make_unique<HelloReply>(_readMessage);
			call->stream = shared_from_this();
			call->execute = [this](google::protobuf::Message* message, std::any stream) {
				auto* castedMessage = static_cast<HelloReply*>(message);
				this->_readCallback(castedMessage, stream);
				};
			_jobQueue->Push(call);
			StartRead(&_readMessage);
		}
	protected:
		std::string _id;
		grpc::ClientContext _context;
		std::shared_ptr<SayHelloStreamReplyCltStream> _ptr = nullptr;

		grpc::Status _status;
		bool _done = false;
		std::function<void(SayHelloStreamReplyCltStream*, const grpc::Status&)> _doneCallback;

		HelloReply _readMessage;
		RpcJobQueue<RpcJobBase>* _jobQueue = nullptr;
		std::function<void(const HelloReply*, std::any stream)> _readCallback;
	};
	virtual void ClientSayHelloStreamReply(const HelloReply* response, std::any stream) = 0;
	virtual void OnCloseSayHelloStreamReply(const ::grpc::Status& status) {};
public:
	void ServerSayHelloStreamReply(const HelloRequest& request) {
		auto stream = std::make_shared<SayHelloStreamReplyCltStream>();
		stream->SetPtr(stream);
		stream->RegisterRead(&_jobQueue, [this](const HelloReply* response, std::any stream) { GetInstance()->ClientSayHelloStreamReply(response, stream); });
		stream->RegisterDone([this](SayHelloStreamReplyCltStream* self, const grpc::Status& s) { GetInstance()->OnCloseSayHelloStreamReply(s); });
		stream->Start(_stub.get(), _id, request);
	}

	//CLIENT_CSTREAM
protected:
	class SayHelloReplyCltStream
		: public grpc::ClientWriteReactor<HelloRequest>
		, public std::enable_shared_from_this<SayHelloReplyCltStream> 
	{
	public:
		void Start(Greeter::Stub* stub, const std::string& id)
		{
			_id = id;
			_context.AddMetadata("id", _id);
			stub->async()->SayHelloRecord(&_context, &_readMessage, this);
			StartCall();
		}
		void SetPtr(std::shared_ptr<SayHelloReplyCltStream> ptr) { _ptr = ptr; }
		void RegisterDone(std::function<void(SayHelloReplyCltStream*, const grpc::Status&)> doneCallback)
		{ _doneCallback = doneCallback; }
		void OnDone(const grpc::Status& s) override
		{
			_status = s;
			_done = true;
			if (_doneCallback) _doneCallback(this, s);
			if (!_sending.load())
				_ptr = nullptr;
		}
		void Send(HelloRequest& message) {
			if (_done) return;
			auto request = std::make_unique<HelloRequest>(message);

			std::lock_guard<std::mutex> lock(_mu);
			if (!_sending.exchange(true))
			{
				_currentSending = std::move(request);
				StartWrite(_currentSending.get());
			}
			else
			{
				_pendingSend.push(std::move(request));
			}
		}
		void FinishSend()
		{
			_finishSend.store(true);
			if (!_sending.load()) StartWritesDone();
		}

		void OnWriteDone(bool ok) override {
			if (!ok) {
				return;
			}
			std::lock_guard<std::mutex> lock(_mu);
			if (!_pendingSend.empty()) {
				_currentSending = std::move(_pendingSend.front());
				_pendingSend.pop();
				StartWrite(_currentSending.get());
			}
			else {
				_currentSending = nullptr;
				_sending.store(false);
				if (_finishSend.load()) StartWritesDone();
				if (_done) _ptr = nullptr;
			}
		}

		HelloReply* GetResponse() { return &_readMessage; }
	private:
		std::string _id;
		grpc::ClientContext _context;
		std::shared_ptr<SayHelloReplyCltStream> _ptr;

		grpc::Status _status;
		bool _done = false;
		std::function<void(SayHelloReplyCltStream*, const grpc::Status&)> _doneCallback;

		std::atomic<bool> _sending{ false };
		std::queue<std::unique_ptr<HelloRequest>> _pendingSend;
		std::unique_ptr<HelloRequest> _currentSending;
		std::mutex _mu;
		std::atomic<bool> _finishSend{ false };

		HelloReply _readMessage;
	};
	std::shared_ptr<SayHelloReplyCltStream> SayHelloRecordWriter = nullptr;
	virtual void OnFinishSayHelloRecord(HelloReply* response, const grpc::Status& status) = 0;
public:
	void ServerSayHelloRecord(HelloRequest& request)
	{
		if (SayHelloRecordWriter == nullptr)
		{
			SayHelloRecordWriter = std::make_shared<SayHelloReplyCltStream>();
			SayHelloRecordWriter->SetPtr(SayHelloRecordWriter);
			SayHelloRecordWriter->RegisterDone(
				[this](SayHelloReplyCltStream* self, const grpc::Status& s){
					GetInstance()->OnFinishSayHelloRecord(self->GetResponse(), s);
					SayHelloRecordWriter = nullptr; 
				});
			SayHelloRecordWriter->Start(_stub.get(), _id);
		}
		SayHelloRecordWriter->Send(request);
	}
	void ServerFinishSayHelloRecord() {
		if (SayHelloRecordWriter != nullptr)
		{
			SayHelloRecordWriter->FinishSend();
		}
	}
};