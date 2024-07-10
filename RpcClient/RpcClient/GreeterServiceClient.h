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
		SayHelloBDSCltStream(const std::string& id, RpcJobQueue<RpcJobBase>* jobQ)
			: _id(id), _jobQueue(jobQ) {}
		void Start(Greeter::Stub* stub)
		{
			_context.AddMetadata("id", _id);
			stub->async()->SayHelloBDS(&_context, this);
			StartRead(&_readMessage);
			StartCall();
		}
		void SetPtr(std::shared_ptr< SayHelloBDSCltStream> ptr) { _ptr = ptr; }
		grpc::ClientContext* GetContext() { return &_context; }
		void RegisterDone(std::function<void(SayHelloBDSCltStream*, const grpc::Status&)> doneCallback)
		{ _doneCallback = doneCallback; }
		void OnDone(const grpc::Status& s) override
		{
			_status = s;
			_done = true;
			if (_doneCallback)
			{
				auto* call = new RpcJob<HelloRequest>();
				call->stream = shared_from_this();
				call->execute = [this](google::protobuf::Message* message, std::any stream) {
					_doneCallback(this, _status);
					};
				_jobQueue->Push(call);
			}
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
		void RegisterRead(std::function<void(const HelloReply*)> readCallback)
		{ _readCallback = readCallback; }
		void OnReadDone(bool ok) override
		{
			if (!ok) return;
			auto* call = new RpcJob<HelloReply>();
			call->data = std::make_unique<HelloReply>(_readMessage);
			call->stream = shared_from_this();
			call->execute = [this](google::protobuf::Message* message, std::any stream) {
				auto* castedMessage = static_cast<HelloReply*>(message);
				this->_readCallback(castedMessage);
				};
			_jobQueue->Push(call);
			StartRead(&_readMessage);
		}
	private:
		std::string _id;
		grpc::ClientContext _context;
		std::shared_ptr<SayHelloBDSCltStream> _ptr;
		RpcJobQueue<RpcJobBase>* _jobQueue = nullptr;

		grpc::Status _status;
		bool _done = false;
		std::function<void(SayHelloBDSCltStream*, const grpc::Status&)> _doneCallback;

		std::atomic<bool> _sending{ false };
		std::queue<std::unique_ptr<HelloRequest>> _pendingSend;
		std::unique_ptr<HelloRequest> _currentSending;
		std::mutex _mu;

		HelloReply _readMessage;
		std::function<void(const HelloReply*)> _readCallback;
	};
	std::shared_ptr<SayHelloBDSCltStream> SayHelloBDSPtr;
	virtual void ClientSayHelloBDS(const HelloReply* response, SayHelloBDSCltStream* stream) = 0;
	virtual void OnCloseSayHelloBDS(const grpc::Status& status, SayHelloBDSCltStream* stream) {};
public:
	void ServerSayHelloBDS(HelloRequest& request) {
		OpenSayHelloBDS();
		if (SayHelloBDSPtr) SayHelloBDSPtr->Send(request);
	}
	void OpenSayHelloBDS()
	{
		if (!SayHelloBDSPtr)
		{
			SayHelloBDSPtr = std::make_shared<SayHelloBDSCltStream>(_id, &_jobQueue);
			auto ptr = SayHelloBDSPtr.get();
			SayHelloBDSPtr->SetPtr(SayHelloBDSPtr);
			SayHelloBDSPtr->RegisterRead([this, ptr](const HelloReply* response) { GetInstance()->ClientSayHelloBDS(response, ptr); });
			SayHelloBDSPtr->RegisterDone([this](SayHelloBDSCltStream* self, const grpc::Status& s) {
				GetInstance()->OnCloseSayHelloBDS(s, self);
				SayHelloBDSPtr = nullptr; 
				});
			SayHelloBDSPtr->Start(_stub.get());
		}
	}

	//CLIENT_SSTREAM
protected:
	class SayHelloStreamReplyCltStream
		: public grpc::ClientReadReactor<HelloReply>
		, public std::enable_shared_from_this<SayHelloStreamReplyCltStream>
	{
	public:
		SayHelloStreamReplyCltStream(const std::string& id, RpcJobQueue<RpcJobBase>* jobQ)
			: _id(id), _jobQueue(jobQ) {}
		void Start(Greeter::Stub* stub, const HelloRequest& request)
		{
			_context.AddMetadata("id", _id);
			stub->async()->SayHelloStreamReply(&_context, &request, this);
			StartRead(&_readMessage);
			StartCall();
		}
		void SetPtr(std::shared_ptr<SayHelloStreamReplyCltStream> ptr) { _ptr = ptr; }
		grpc::ClientContext* GetContext() { return &_context; }
		void RegisterDone(std::function<void(SayHelloStreamReplyCltStream*, const grpc::Status&)> doneCallback)
		{ _doneCallback = doneCallback; }
		void OnDone(const grpc::Status& s) override
		{
			_status = s;
			_done = true;
			if (_doneCallback)
			{
				auto* call = new RpcJob<HelloRequest>();
				call->stream = shared_from_this();
				call->execute = [this](google::protobuf::Message* message, std::any stream) {
					_doneCallback(this, _status);
					};
				_jobQueue->Push(call);
			}
			_ptr = nullptr;
		};

		void RegisterRead(std::function<void(const HelloReply*)> readCallback)
		{ _readCallback = readCallback; }
		void OnReadDone(bool ok) override {
			if (!ok) return;
			auto* call = new RpcJob<HelloReply>();
			call->data = std::make_unique<HelloReply>(_readMessage);
			call->stream = shared_from_this();
			call->execute = [this](google::protobuf::Message* message, std::any stream) {
				auto* castedMessage = static_cast<HelloReply*>(message);
				this->_readCallback(castedMessage);
				};
			_jobQueue->Push(call);
			StartRead(&_readMessage);
		}
	protected:
		std::string _id;
		grpc::ClientContext _context;
		std::shared_ptr<SayHelloStreamReplyCltStream> _ptr = nullptr;
		RpcJobQueue<RpcJobBase>* _jobQueue = nullptr;

		grpc::Status _status;
		bool _done = false;
		std::function<void(SayHelloStreamReplyCltStream*, const grpc::Status&)> _doneCallback;

		HelloReply _readMessage;
		std::function<void(const HelloReply*)> _readCallback;
	};
	virtual void ClientSayHelloStreamReply(const HelloReply* response, SayHelloStreamReplyCltStream* stream) = 0;
	virtual void OnCloseSayHelloStreamReply(const ::grpc::Status& status) {};
public:
	void ServerSayHelloStreamReply(const HelloRequest& request) {
		auto stream = std::make_shared<SayHelloStreamReplyCltStream>(_id, &_jobQueue);
		auto ptr = stream.get();
		stream->SetPtr(stream);
		stream->RegisterRead([this, ptr](const HelloReply* response) {GetInstance()->ClientSayHelloStreamReply(response, ptr); });
		stream->RegisterDone([this](SayHelloStreamReplyCltStream* self, const grpc::Status& s) { GetInstance()->OnCloseSayHelloStreamReply(s); });
		stream->Start(_stub.get(), request);
	}

	//CLIENT_CSTREAM
protected:
	class SayHelloReplyCltStream
		: public grpc::ClientWriteReactor<HelloRequest>
		, public std::enable_shared_from_this<SayHelloReplyCltStream> 
	{
	public:
		SayHelloReplyCltStream(const std::string& id, RpcJobQueue<RpcJobBase>* jobQ)
			: _id(id), _jobQueue(jobQ) {}
		void Start(Greeter::Stub* stub)
		{
			_context.AddMetadata("id", _id);
			stub->async()->SayHelloRecord(&_context, &_readMessage, this);
			StartCall();
		}
		void SetPtr(std::shared_ptr<SayHelloReplyCltStream> ptr) { _ptr = ptr; }
		grpc::ClientContext* GetContext() { return &_context; }
		void RegisterDone(std::function<void(SayHelloReplyCltStream*, const grpc::Status&)> doneCallback)
		{ _doneCallback = doneCallback; }
		void OnDone(const grpc::Status& s) override
		{
			_status = s;
			_done = true;
			if (_doneCallback)
			{
				auto* call = new RpcJob<HelloRequest>();
				call->stream = shared_from_this();
				call->execute = [this](google::protobuf::Message* message, std::any stream) {
					_doneCallback(this, _status);
					};
				_jobQueue->Push(call);
			}
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
		RpcJobQueue<RpcJobBase>* _jobQueue = nullptr;

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
	std::shared_ptr<SayHelloReplyCltStream> SayHelloRecordPtr = nullptr;
	virtual void OnFinishSayHelloRecord(HelloReply* response, const grpc::Status& status) = 0;
public:
	void ServerSayHelloRecord(HelloRequest& request)
	{
		if (SayHelloRecordPtr == nullptr)
		{
			SayHelloRecordPtr = std::make_shared<SayHelloReplyCltStream>(_id, &_jobQueue);
			SayHelloRecordPtr->SetPtr(SayHelloRecordPtr);
			SayHelloRecordPtr->RegisterDone(
				[this](SayHelloReplyCltStream* self, const grpc::Status& s){
					GetInstance()->OnFinishSayHelloRecord(self->GetResponse(), s);
					SayHelloRecordPtr = nullptr; 
				});
			SayHelloRecordPtr->Start(_stub.get());
		}
		SayHelloRecordPtr->Send(request);
	}
	void ServerFinishSayHelloRecord() { if (SayHelloRecordPtr != nullptr) SayHelloRecordPtr->FinishSend(); }
};