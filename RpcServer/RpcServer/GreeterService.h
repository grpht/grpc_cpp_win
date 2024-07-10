#pragma once
#pragma warning(disable :4251)
#pragma warning(disable :4819)

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <tuple>
#include <thread>
#include <chrono>

#include "RpcService.h"
#include "RpcTemplate.h"

#include "greeter.grpc.pb.h"

using namespace helloworld;

#define USING_SERVER_BISTREAM(FUNC,REQ,RES) \
class FUNC##SvrBiStream : public grpc::ServerBidiReactor<REQ, RES> {\
public: \
	FUNC##BiStream(grpc::CallbackServerContext* context, RpcJobQueue<RpcJobBase>* jobQ, \
		std::function<void(grpc::CallbackServerContext*, const REQ*, std::any stream)> callback) \
		:_context(context), _jobQueue(jobQ), _callback(callback) \
	{ StartRead(&_request); } \
	void SetId(std::string& id) { if (_id.empty()) _id = id; } \
	void OnReadDone(bool ok) override { \
		if (!ok) { Finish(grpc::Status::OK); return; } \
		auto* call = new RpcJob<REQ>(); \
		call->data = std::make_unique<REQ>(_request); \
		call->stream = this; \
		call->execute = [this](google::protobuf::Message* message, std::any stream) { \
			auto* castedMessage = static_cast<REQ*>(message); \
			this->_callback(_context, castedMessage, stream); \
			}; \
		_jobQueue->Push(call); \
		StartRead(&_request); \
	} \
	void OnDone() override { } \
	void Send(RES& message) { \
		auto response = std::make_unique<RES>(); \
		response->CopyFrom(message); \
		if (!_sending.exchange(true)) { \
			{ \
				std::lock_guard<std::mutex> lock(_mu); \
				_currentSending = std::move(response); \
			} \
			StartWrite(_currentSending.get()); \
		} \
		else { \
			std::lock_guard<std::mutex> lock(_mu); \
			_pendingSend.push(std::move(response)); \
		} \
	} \
	void OnWriteDone(bool ok) override { \
		if (!ok) { \
			std::cerr << "Server Write Error" << std::endl; \
			Finish(grpc::Status::OK); \
			return; \
		} \
		if (!_pendingSend.empty()) { \
			{ \
				std::lock_guard<std::mutex> lock(_mu); \
				_currentSending = std::move(_pendingSend.front()); \
				_pendingSend.pop(); \
			} \
			StartWrite(_currentSending.get()); \
		} \
		else { \
			_currentSending = nullptr; \
			_sending.store(false); \
		} \
	} \
private: \
	grpc::CallbackServerContext* _context = nullptr; \
	std::string _id; \
	std::atomic<bool> _sending = false; \
	std::queue<std::unique_ptr<RES>> _pendingSend; \
	std::unique_ptr<RES> _currentSending; \
	std::mutex _mu; \
	REQ _request; \
	RpcJobQueue<RpcJobBase>* _jobQueue; \
	std::function<void(grpc::CallbackServerContext*, const REQ*, std::any stream)> _callback; \
}; \

#define USING_SERVER_SSTREAM(FUNC,REQ,RES) \
class FUNC##SvrWriter : public grpc::ServerWriteReactor<RES>{ \
public: \
	FUNC##Writer(grpc::CallbackServerContext* context, const REQ& request) \
		:_context(context) { _request.CopyFrom(request); } \
	void SetId(std::string& id) { if (_id.empty()) _id = id; } \
	void SetThis(std::shared_ptr<FUNC##Writer> myInstance) { mThis = myInstance; } \
	void Send(RES& message) { \
		auto response = std::make_unique<RES>(); response->CopyFrom(message); \
		if (!_sending.exchange(true)) { \
			{ \
				std::lock_guard<std::mutex> lock(_mu); \
				_currentSending = std::move(response); \
			} \
			StartWrite(_currentSending.get()); \
		} \
		else { \
			std::lock_guard<std::mutex> lock(_mu); _pendingSend.push(std::move(response)); \
		} \
	} \
	void OnDone() override { mThis = nullptr; } \
	void OnWriteDone(bool ok) override { \
		if (!ok) { \
			Finish(grpc::Status::OK); \
			return; \
		} \
		if (!_pendingSend.empty()) { \
			{ \
				std::lock_guard<std::mutex> lock(_mu); \
				_currentSending = std::move(_pendingSend.front()); \
				_pendingSend.pop(); \
			} \
			StartWrite(_currentSending.get()); \
		} \
		else { \
			_currentSending = nullptr; \
			_sending.store(false); \
		} \
	} \
private: \
	grpc::CallbackServerContext* _context = nullptr; \
	std::string _id; \
	std::atomic<bool> _sending = false; \
	std::queue<std::unique_ptr<RES>> _pendingSend; \
	std::unique_ptr<RES> _currentSending; std::mutex _mu; \
	REQ _request; \
	std::shared_ptr<FUNC##Writer> mThis; \
}; \

class SayHelloSvrStream : public grpc::ServerUnaryReactor {
public:
	void SetPtr(std::shared_ptr< SayHelloSvrStream> ptr) { _ptr = ptr; }
private:
	void OnDone() override { _ptr = nullptr; }
	void OnCancel() override { _ptr = nullptr; }
private:
	std::shared_ptr< SayHelloSvrStream> _ptr = nullptr;
};

class SayHelloBDSSvrStream
	: public grpc::ServerBidiReactor<HelloRequest, HelloReply>
	, public std::enable_shared_from_this<SayHelloBDSSvrStream>
{
public:
	SayHelloBDSSvrStream(grpc::CallbackServerContext* context, RpcJobQueue<RpcJobBase>* jobQ)
		:_context(context), _jobQueue(jobQ) { }
	void SetId(std::string& id) { if (_id.empty()) _id = id; }
	void SetPtr(std::shared_ptr< SayHelloBDSSvrStream> ptr) { _ptr = ptr; }
	void RegisterDone(std::function<void(SayHelloBDSSvrStream*, const grpc::Status&)> doneCallback)
	{ _doneCallback = doneCallback; }
	void OnDone() override
	{
		_done.store(true);
		if (_doneCallback)
		{
			auto* call = new RpcJob<HelloRequest>();
			call->stream = shared_from_this();
			call->execute = [this](google::protobuf::Message* message, std::any stream) {
				_doneCallback(this, grpc::Status::OK);
				};
			_jobQueue->Push(call);
		}
		TrashPendingSend();
		_ptr = nullptr; 
	}
	bool IsDone() { return _done.load(); }
	void Close(const grpc::Status& status) { if (!_close.exchange(true)) Finish(status); }
	void Send(HelloReply& message)
	{
		if (_done.load()) return;
		auto response = std::make_unique<HelloReply>(message);
		std::lock_guard<std::mutex> lock(_mu);
		if (!_sending.exchange(true)) {
			_currentSending = std::move(response);
			StartWrite(_currentSending.get());
		}
		else
			_pendingSend.push(std::move(response));
	}
	void OnWriteDone(bool ok) override
	{
		if (!ok || _done.load()) return;
		std::lock_guard<std::mutex> lock(_mu);
		if (!_pendingSend.empty()) {
			_currentSending = std::move(_pendingSend.front());
			_pendingSend.pop();
			StartWrite(_currentSending.get());
		}
		else {
			_currentSending = nullptr;
			_sending.store(false);
		}
	}
	void TrashPendingSend()
	{
		std::lock_guard<std::mutex> lock(_mu);
		while (!_pendingSend.empty())
			_pendingSend.pop();
	}

	void RegisterRead(std::function<void(grpc::CallbackServerContext*, const HelloRequest*, std::any stream)> readCallback)
	{
		_readCallback = readCallback;
		StartRead(&_readMessage);
	}
	void OnReadDone(bool ok) override
	{
		if (!ok) {
			Close(grpc::Status::OK);
			return;
		}
		auto* call = new RpcJob<HelloRequest>();
		call->data = std::make_unique<HelloRequest>(_readMessage);
		call->stream = shared_from_this();
		call->execute = [this](google::protobuf::Message* message, std::any stream) {
			auto* castedMessage = static_cast<HelloRequest*>(message);
			this->_readCallback(_context, castedMessage, stream);
			};
		_jobQueue->Push(call);
		StartRead(&_readMessage);
	}
private:
	std::string _id;
	grpc::CallbackServerContext* _context = nullptr;
	std::shared_ptr<SayHelloBDSSvrStream> _ptr;
	RpcJobQueue<RpcJobBase>* _jobQueue = nullptr;

	std::atomic_bool _done = false;
	std::function<void(SayHelloBDSSvrStream*, const grpc::Status& status)> _doneCallback;
	std::atomic_bool _close = false;

	std::atomic<bool> _sending = false; 
	std::queue<std::unique_ptr<HelloReply>> _pendingSend;
	std::unique_ptr<HelloReply> _currentSending;
	std::mutex _mu; 
	
	HelloRequest _readMessage; 
	std::function<void(grpc::CallbackServerContext*, const HelloRequest*, std::any stream)> _readCallback;
};

class SayHelloStreamReplySvrStream
	: public grpc::ServerWriteReactor<HelloReply>
	, public std::enable_shared_from_this<SayHelloStreamReplySvrStream>
{
public:
	SayHelloStreamReplySvrStream(grpc::CallbackServerContext* context, RpcJobQueue<RpcJobBase>* jobQ)
		:_context(context), _jobQueue(jobQ) {} 
	void SetId(std::string& id) { if (_id.empty()) _id = id; }
	void SetPtr(std::shared_ptr<SayHelloStreamReplySvrStream> ptr) { _ptr = ptr; }
	void RegisterDone(std::function<void(SayHelloStreamReplySvrStream*, const grpc::Status&)> doneCallback)
	{ _doneCallback = doneCallback; }
	void OnDone() override
	{
		_done = true;
		if (_doneCallback)
		{
			auto* call = new RpcJob<HelloRequest>();
			call->stream = shared_from_this();
			call->execute = [this](google::protobuf::Message* message, std::any stream) {
				_doneCallback(this, grpc::Status::OK);
				};
			_jobQueue->Push(call);
		}
		TrashPendingSend();
		_ptr = nullptr;
	}
	void Close(const grpc::Status& status) { if (!_close.exchange(true)) Finish(status); }
	void Send(HelloReply& message)
	{
		if (_done) return;
		auto response = std::make_unique<HelloReply>(); 
		response->CopyFrom(message);
		std::lock_guard<std::mutex> lock(_mu);
		if (!_sending.exchange(true)) 
		{
			_currentSending = std::move(response);
			StartWrite(_currentSending.get());
		}
		else 
		{
			_pendingSend.push(std::move(response));
		}
	}
	void OnWriteDone(bool ok) override
	{
		if (!ok) {
			Close(grpc::Status(grpc::StatusCode::UNKNOWN, "Unexpected Failure"));
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
		}
	}
	void TrashPendingSend()
	{
		std::lock_guard<std::mutex> lock(_mu);
		while (!_pendingSend.empty())
			_pendingSend.pop();
	}
private:
	std::string _id;
	grpc::CallbackServerContext* _context = nullptr;
	std::shared_ptr<SayHelloStreamReplySvrStream> _ptr;
	RpcJobQueue<RpcJobBase>* _jobQueue = nullptr;

	std::atomic_bool _done = false;
	std::atomic_bool _close = false;

	std::function<void(SayHelloStreamReplySvrStream*, const grpc::Status&)> _doneCallback = nullptr;
	std::atomic_bool _sending = false;
	std::queue<std::unique_ptr<HelloReply>> _pendingSend;
	std::unique_ptr<HelloReply> _currentSending;
	std::mutex _mu;
};

class SayHelloRecordSvrStream
	: public grpc::ServerReadReactor<HelloRequest>
	, public std::enable_shared_from_this<SayHelloRecordSvrStream>
{
public:
	SayHelloRecordSvrStream(grpc::CallbackServerContext* context, HelloReply* response, RpcJobQueue<RpcJobBase>* jobQ)
		:_context(context), _response(response), _jobQueue(jobQ) {}

	void SetId(std::string& id) { if (_id.empty()) _id = id; }
	void SetPtr(std::shared_ptr< SayHelloRecordSvrStream> ptr) { _ptr = ptr; }
	void RegisterDone(std::function<void(SayHelloRecordSvrStream*, const grpc::Status&)> doneCallback)
	{ _doneCallback = doneCallback; }
	void OnDone() override
	{
		_done.store(true);
		if (_doneCallback)
		{
			auto* call = new RpcJob<HelloRequest>();
			call->stream = shared_from_this();
			call->execute = [this](google::protobuf::Message* message, std::any stream) {
				_doneCallback(this, grpc::Status::OK);
				};
			_jobQueue->Push(call);
		}
		_ptr = nullptr;
	}
	bool IsDone() { return _done.load(); }
	void Close(const grpc::Status& status) { if (!_close.exchange(true)) Finish(status); }
	void RegisterRead(std::function<void(grpc::CallbackServerContext*, const HelloRequest*, std::any stream, bool)> readCallback)
	{
		_readCallback = readCallback;
		StartRead(&_readMessage);
	}
	void OnReadDone(bool ok) override
	{
		auto* call = new RpcJob<HelloRequest>();
		call->data = std::make_unique<HelloRequest>(_readMessage);
		call->stream = shared_from_this();
		call->execute = [this, ok](google::protobuf::Message* message, std::any stream) {
			auto* castedMessage = static_cast<HelloRequest*>(message);
			this->_readCallback(_context, castedMessage, stream, ok);
			};
		_jobQueue->Push(call);
		if (ok) StartRead(&_readMessage);
	}
private:
	std::string _id;
	grpc::CallbackServerContext* _context = nullptr;
	std::shared_ptr<SayHelloRecordSvrStream> _ptr = nullptr;
	RpcJobQueue<RpcJobBase>* _jobQueue = nullptr;

	std::atomic_bool _done = false;
	std::function<void(SayHelloRecordSvrStream*, const grpc::Status& status)> _doneCallback;
	std::atomic_bool _close = false;

	HelloRequest _readMessage;
	std::function<void(grpc::CallbackServerContext*, const HelloRequest*, std::any, bool)> _readCallback;

	HelloReply* _response = nullptr;
};

class GreeterService : public helloworld::Greeter::CallbackService, public RpcService
{
protected:
	virtual GreeterService* GetInstance() = 0;

	class RpcClientSession
	{
	public:
		//BiStream
		std::shared_ptr<SayHelloBDSSvrStream> SayHelloBDSStream = nullptr;
		void ClientSayHelloBDS(HelloReply& response) { if (SayHelloBDSStream) SayHelloBDSStream->Send(response); }
		void CloseSayHelloBDS(const grpc::Status& status = grpc::Status::OK) 
		{ if (SayHelloBDSStream) SayHelloBDSStream->Close(status); }

		//ServerStream
		std::shared_ptr<SayHelloStreamReplySvrStream> SayHelloStreamReplyPtr = nullptr;
		void ClientSayHelloStreamReply(HelloReply& response) { if (SayHelloStreamReplyPtr) SayHelloStreamReplyPtr->Send(response); }
		void CloseSayHelloStreamReplyPtr(const grpc::Status& status = grpc::Status::OK)
		{ if (SayHelloStreamReplyPtr) SayHelloStreamReplyPtr->Close(status); }

		//ClientStream
		std::shared_ptr<SayHelloRecordSvrStream> SayHelloRecordPtr = nullptr;
	public:
		void SetId(const std::string& id) { if (_id.empty()) _id = id; }
		const std::string& GetId() const { return _id; }
	private:
		std::string _id;
	};
	std::unordered_map<std::string, RpcClientSession> _clients;
	
public:
	RpcClientSession* GetClient(grpc::CallbackServerContext* context)
	{
		return GetClient(GetContextMetaData(context, "id"));
	}

	RpcClientSession* GetClient(const std::string& id)
	{
		auto it = _clients.find(id);
		if (it != _clients.end())
			return &it->second;
		return nullptr;
	}

public:
	//SERVER_BISTREAM_RECV
	//SERVER_UNARY(SayHello, HelloRequest, HelloReply)
	
protected:
	virtual grpc::Status ServerSayHello(grpc::CallbackServerContext* context, const HelloRequest* request, HelloReply* response) = 0;

	grpc::ServerUnaryReactor* SayHello(grpc::CallbackServerContext* context, const HelloRequest* request, HelloReply* response) override {
		auto stream = std::make_shared<SayHelloSvrStream>();
		stream->SetPtr(stream);
		auto* call = new RpcJob<HelloRequest>();
		call->stream = stream;
		call->execute = [this, stream, context, request, response](google::protobuf::Message* m, std::any s)
			{ stream->Finish(GetInstance()->ServerSayHello(context, request, response)); };
		_jobQueue->Push(call);
		return stream.get();
	}

	//SERVER_BISTREAM_RECV
protected:
	virtual void ServerSayHelloBDS(grpc::CallbackServerContext* context, const HelloRequest* request, std::any stream) = 0;
	virtual void OnOpenSayHelloBDS(std::shared_ptr<SayHelloBDSSvrStream> stream) {};
	virtual void OnCloseSayHelloBDS(std::shared_ptr<SayHelloBDSSvrStream> stream) {};
public:
	grpc::ServerBidiReactor<HelloRequest, HelloReply>* SayHelloBDS(grpc::CallbackServerContext* context) override 
	{
		std::string id = GetContextMetaData(context, "id");
		auto& client = _clients[id];
		std::shared_ptr<SayHelloBDSSvrStream> stream = client.SayHelloBDSStream;
		if (!stream)
		{
			stream = std::make_shared<SayHelloBDSSvrStream>(context, _jobQueue);
			stream->SetPtr(stream);
			stream->SetId(id);
			client.SetId(id);
			client.SayHelloBDSStream = stream;
			auto* openCall = new RpcJob<HelloRequest>();
			openCall->stream = stream;
			openCall->execute = [this, stream](google::protobuf::Message* message, std::any s)
				{ GetInstance()->OnOpenSayHelloBDS(stream); };
			_jobQueue->Push(openCall);
			stream->RegisterRead([this](grpc::CallbackServerContext* ctx, const HelloRequest* req, std::any s)
				{ GetInstance()->ServerSayHelloBDS(ctx, req, s); });
			stream->RegisterDone([this, &client, stream](SayHelloBDSSvrStream* self, const grpc::Status& status) {
				GetInstance()->OnCloseSayHelloBDS(stream);
				client.SayHelloBDSStream = nullptr;
				});
		}
		return stream.get();
	}

	//SERVER_SSTREAM_RECV
protected:
	virtual void ServerSayHelloStreamReply(grpc::CallbackServerContext* context, const HelloRequest* request, std::any stream) = 0;
	virtual void OnOpenSayHelloStreamReply(std::shared_ptr<SayHelloStreamReplySvrStream> stream) {};
	virtual void OnCloseSayHelloStreamReply(std::shared_ptr<SayHelloStreamReplySvrStream> stream) {};
public:
	grpc::ServerWriteReactor<HelloReply>* SayHelloStreamReply(grpc::CallbackServerContext* context, const HelloRequest* request) override
	{
		std::string id = GetContextMetaData(context, "id");
		auto& client = _clients[id];
		std::shared_ptr<SayHelloStreamReplySvrStream> stream = client.SayHelloStreamReplyPtr;
		if (!stream)
		{
			stream = std::make_shared<SayHelloStreamReplySvrStream>(context, _jobQueue);
			stream->SetPtr(stream);
			stream->SetId(id);
			client.SayHelloStreamReplyPtr = stream;
			auto* openCall = new RpcJob<HelloRequest>();
			openCall->stream = stream;
			openCall->execute = [this, stream](google::protobuf::Message* message, std::any s)
				{ GetInstance()->OnOpenSayHelloStreamReply(stream); };
			_jobQueue->Push(openCall);
			stream->RegisterDone([this, &client, stream](SayHelloStreamReplySvrStream* self, const grpc::Status& status) {
				GetInstance()->OnCloseSayHelloStreamReply(stream);
				client.SayHelloStreamReplyPtr = nullptr;
				});
		}
		auto* call = new RpcJob<HelloRequest>();
		call->data = std::make_unique<HelloRequest>(*request);
		call->stream = stream;
		call->execute = [this, context](google::protobuf::Message* m, std::any s)
			{
				auto* castdMessage = static_cast<HelloRequest*>(m);
				GetInstance()->ServerSayHelloStreamReply(context, castdMessage, s);
			};
		_jobQueue->Push(call);
		return stream.get();
	}
	
	//CLIENT_STREAM
protected:
	virtual void ServerSayHelloRecord(grpc::CallbackServerContext* context, const HelloRequest* request, std::any stream) = 0;
	virtual grpc::Status ServerFinishSayHelloRecord(HelloReply* response, std::shared_ptr<SayHelloRecordSvrStream> stream) = 0;
	virtual void OnOpenSayHelloRecord(std::shared_ptr< SayHelloRecordSvrStream> stream) {};
	virtual void OnCloseSayHelloRecord(std::shared_ptr< SayHelloRecordSvrStream> stream) {};
public:
	grpc::ServerReadReactor<HelloRequest>* SayHelloRecord(grpc::CallbackServerContext* context, HelloReply* response) override {
		std::string id = GetContextMetaData(context, "id");
		auto& client = _clients[id];
		std::shared_ptr< SayHelloRecordSvrStream> stream = client.SayHelloRecordPtr;
		if (!stream)
		{
			stream = std::make_shared<SayHelloRecordSvrStream>(context, response, _jobQueue);
			stream->SetPtr(stream);
			stream->SetId(id);
			client.SetId(id);
			client.SayHelloRecordPtr = stream;
			auto* openCall = new RpcJob<HelloRequest>();
			openCall->stream = stream;
			openCall->execute = [this, stream](google::protobuf::Message* message, std::any s)
				{ GetInstance()->OnOpenSayHelloRecord(stream); };
			_jobQueue->Push(openCall);
			stream->RegisterRead([response, stream, this](grpc::CallbackServerContext* ctx, const HelloRequest* request, std::any s, bool ok) {
				if (!ok) stream->Close(ServerFinishSayHelloRecord(response, stream));
				else GetInstance()->ServerSayHelloRecord(ctx, request, s);
				});
			stream->RegisterDone([this, &client, stream](SayHelloRecordSvrStream* self, const grpc::Status& status) {
				GetInstance()->OnCloseSayHelloRecord(stream);
				client.SayHelloRecordPtr = nullptr;
				});
		}
		return stream.get();
	}
};