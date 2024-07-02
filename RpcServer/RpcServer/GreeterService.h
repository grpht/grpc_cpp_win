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

class SayHelloBDSSvrBiStream
	: public grpc::ServerBidiReactor<HelloRequest, HelloReply>
	, public std::enable_shared_from_this<SayHelloBDSSvrBiStream>
{
public:
	SayHelloBDSSvrBiStream(grpc::CallbackServerContext* context)
		:_context(context)
	{ }
	void SetId(std::string& id) { if (_id.empty()) _id = id; }
	void SetPtr(std::shared_ptr< SayHelloBDSSvrBiStream> ptr) { _ptr = ptr; }
	
	void RegisterDone(std::function<void(SayHelloBDSSvrBiStream*, const grpc::Status&)> doneCallback)
	{ _doneCallback = doneCallback; }
	void OnDone() override {
		_done.store(true);
		if (_doneCallback) _doneCallback(this, grpc::Status::OK);
		TrashPendingSend();
		_ptr = nullptr; 
	}
	bool IsDone() { return _done.load(); }

	void Send(HelloReply& message) {
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
	void OnWriteDone(bool ok) override {
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

	void RegisterRead(RpcJobQueue<RpcJobBase>* jobQ, std::function<void(grpc::CallbackServerContext*, const HelloRequest*, std::any stream)> readCallback)
	{
		_jobQueue = jobQ;
		_readCallback = readCallback;
		StartRead(&_readMessage);
	}
	void OnReadDone(bool ok) override {
		if (!ok) {
			Finish(grpc::Status::OK);
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
	std::shared_ptr<SayHelloBDSSvrBiStream> _ptr;

	std::atomic_bool _done = false;
	std::function<void(SayHelloBDSSvrBiStream*, const grpc::Status& status)> _doneCallback;
	
	std::atomic<bool> _sending = false; 
	std::queue<std::unique_ptr<HelloReply>> _pendingSend;
	std::unique_ptr<HelloReply> _currentSending;
	std::mutex _mu; 
	
	HelloRequest _readMessage; 
	RpcJobQueue<RpcJobBase>* _jobQueue = nullptr;
	std::function<void(grpc::CallbackServerContext*, const HelloRequest*, std::any stream)> _readCallback;
};

class SayHelloStreamReplySvrWriter
	: public grpc::ServerWriteReactor<HelloReply>
	, public std::enable_shared_from_this<SayHelloStreamReplySvrWriter>
{
public:
	SayHelloStreamReplySvrWriter(grpc::CallbackServerContext* context)
		:_context(context) 
	{
	} 
	void SetId(std::string& id) 
	{
		if (_id.empty()) _id = id;
	}
	void SetPtr(std::shared_ptr<SayHelloStreamReplySvrWriter> ptr) { _ptr = ptr; }
	
	void RegisterDone(std::function<void(SayHelloStreamReplySvrWriter*, const grpc::Status&)> doneCallback)
	{
		_doneCallback = doneCallback;
	}
	void OnDone() override
	{
		_done = true;
		if (_doneCallback) _doneCallback(this, grpc::Status::OK);
		TrashPendingSend();
		_ptr = nullptr;
	}

	void Send(HelloReply& message) {
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
	
	void OnWriteDone(bool ok) override {
		if (!ok) {
			Finish(grpc::Status(grpc::StatusCode::UNKNOWN, "Unexpected Failure"));
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
	std::shared_ptr<SayHelloStreamReplySvrWriter> _ptr;

	std::atomic_bool _done = false;

	std::function<void(SayHelloStreamReplySvrWriter*, const grpc::Status&)> _doneCallback = nullptr;
	std::atomic_bool _sending = false;
	std::queue<std::unique_ptr<HelloReply>> _pendingSend;
	std::unique_ptr<HelloReply> _currentSending;
	std::mutex _mu;
};

class SayHelloRecordSvrReader
	: public grpc::ServerReadReactor<HelloRequest>
	, public std::enable_shared_from_this<SayHelloRecordSvrReader>
{
public:
	SayHelloRecordSvrReader(grpc::CallbackServerContext* context, HelloReply* response)
	{
		_context = context;
		_response = response;
	}

	void SetId(std::string& id) { if (_id.empty()) _id = id; }
	void SetPtr(std::shared_ptr< SayHelloRecordSvrReader> ptr) { _ptr = ptr; }

	void RegisterDone(std::function<void(SayHelloRecordSvrReader*, const grpc::Status&)> doneCallback)
	{ _doneCallback = doneCallback; }
	void OnDone() override {
		_done.store(true);
		if (_doneCallback) _doneCallback(this, grpc::Status::OK);
		_ptr = nullptr;
	}
	bool IsDone() { return _done.load(); }

	void RegisterRead(RpcJobQueue<RpcJobBase>* jobQ, std::function<void(grpc::CallbackServerContext*, const HelloRequest*, std::any stream, bool)> readCallback)
	{
		_jobQueue = jobQ;
		_readCallback = readCallback;
		StartRead(&_readMessage);
	}

	void OnReadDone(bool ok) override {
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
	std::shared_ptr<SayHelloRecordSvrReader> _ptr = nullptr;

	std::atomic_bool _done = false;
	std::function<void(SayHelloRecordSvrReader*, const grpc::Status& status)> _doneCallback;

	HelloRequest _readMessage;
	RpcJobQueue<RpcJobBase>* _jobQueue = nullptr;
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
		std::shared_ptr<SayHelloBDSSvrBiStream> SayHelloBDSStream = nullptr;
		void ClientSayHelloBDS(HelloReply& response) { if (SayHelloBDSStream) SayHelloBDSStream->Send(response); }

		std::shared_ptr<SayHelloRecordSvrReader> SayHelloRecordPtr = nullptr;
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
		std::string id;
		if (!TryFindContextMetaData(context, "id", id))
			return nullptr;
		return GetClient(id);
	}

	RpcClientSession* GetClient(const std::string& id)
	{
		auto it = _clients.find(id);
		if (it != _clients.end())
			return &it->second;
		return nullptr;
	}

public:
	//@ SECTION_SERVER_UNARY
	//SERVER_BISTREAM_RECV
	SERVER_UNARY(SayHello, HelloRequest, HelloReply)

	//@ SECTION_SERVER_BISTREAM_RECV
	//SERVER_BISTREAM_RECV
protected:
	virtual void ServerSayHelloBDS(grpc::CallbackServerContext* context, const HelloRequest* request, std::any stream) = 0;
public:
	grpc::ServerBidiReactor<HelloRequest, HelloReply>* SayHelloBDS(grpc::CallbackServerContext* context) override 
	{
		auto stream = std::make_shared<SayHelloBDSSvrBiStream>(context);
		stream->SetPtr(stream);
		stream->RegisterRead(_jobQueue, [this](grpc::CallbackServerContext* ctx, const HelloRequest* req, std::any s) {
				GetInstance()->ServerSayHelloBDS(ctx, req, s);
			});
		std::string id;
		if (TryFindContextMetaData(context, "id", id))
		{
			auto& client = _clients[id];
			client.SetId(id);
			client.SayHelloBDSStream = stream;
		}
		return stream.get();
	}

	//@ SECTION_SERVER_SSTREAM_RECV
	//SERVER_SSTREAM_RECV
protected:
	virtual void ServerSayHelloStreamReply(grpc::CallbackServerContext* context, const HelloRequest* request, std::any stream) = 0;
public:
	grpc::ServerWriteReactor<HelloReply>* SayHelloStreamReply(grpc::CallbackServerContext* context, const HelloRequest* request) override
	{
		auto stream = std::make_shared<SayHelloStreamReplySvrWriter>(context);
		stream->SetPtr(stream);
		auto* call = new RpcJob<HelloRequest>();
		call->data = std::make_unique<HelloRequest>(*request);
		call->stream = stream;
		call->execute = [this, context](google::protobuf::Message* m, std::any s)
			{
				auto* castdMessage = static_cast<HelloRequest*>(m);
				this->ServerSayHelloStreamReply(context, castdMessage, s);
			};
		_jobQueue->Push(call);
		return stream.get();
	}

protected:
	virtual void ServerSayHelloRecord(grpc::CallbackServerContext* context, const HelloRequest* request, std::any stream) = 0;
	virtual grpc::Status FinishServerSayHelloRecord(HelloReply* response, std::shared_ptr<SayHelloRecordSvrReader> stream) = 0;
public:
	grpc::ServerReadReactor<HelloRequest>* SayHelloRecord(grpc::CallbackServerContext* context, HelloReply* response) override {
		auto stream = std::make_shared<SayHelloRecordSvrReader>(context, response);
		stream->SetPtr(stream);
		stream->RegisterRead(_jobQueue, [response, stream, this](grpc::CallbackServerContext* ctx, const HelloRequest* request, std::any s, bool ok) {
			if (!ok)
				stream->Finish(FinishServerSayHelloRecord(response, stream));
			else
				ServerSayHelloRecord(ctx, request, s);
			});
		std::string id;
		if (TryFindContextMetaData(context, "id", id))
		{
			auto& client = _clients[id];
			client.SetId(id);
			client.SayHelloRecordPtr = stream;
		}
		return stream.get();
	}
};