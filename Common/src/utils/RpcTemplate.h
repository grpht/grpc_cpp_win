#pragma once
#include <functional>

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <tuple>
#include <thread>
#include <chrono>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "RpcJob.h"

#pragma region DECLARE_SERVER_UNARY
#define DECLARE_SERVER_UNARY(FUNC, REQ, RES)
class SayHelloSvrStream : public grpc::ServerUnaryReactor {
public:
	void SetPtr(std::shared_ptr< SayHelloSvrStream> ptr) { _ptr = ptr; }
private:
	void OnDone() override { _ptr = nullptr; }
	void OnCancel() override { _ptr = nullptr; }
private:
	std::shared_ptr< SayHelloSvrStream> _ptr = nullptr;
};
#pragma endregion

#pragma region DECLARE_SERVER_BISTREAM
#define DECLARE_SERVER_BISTREAM(FUNC, REQ, RES) \
class FUNC##SvrStream \
	: public grpc::ServerBidiReactor<REQ, RES> \
	, public std::enable_shared_from_this<FUNC##SvrStream> \
{ \
public: \
	FUNC##SvrStream(grpc::CallbackServerContext* context, RpcJobQueue<RpcJobBase>* jobQ) \
		:_context(context), _jobQueue(jobQ) { } \
	void SetId(std::string& id) { if (_id.empty()) _id = id; } \
	void SetPtr(std::shared_ptr< FUNC##SvrStream> ptr) { _ptr = ptr; } \
	void RegisterDone(std::function<void(FUNC##SvrStream*, const grpc::Status&)> doneCallback) \
	{ _doneCallback = doneCallback; } \
	void OnDone() override \
	{ \
		_done.store(true); \
		if (_doneCallback) \
		{ \
			auto* call = new RpcJob<REQ>(); \
			call->stream = shared_from_this(); \
			call->execute = [this](google::protobuf::Message* message, std::any stream) { \
				_doneCallback(this, grpc::Status::OK); \
				}; \
			_jobQueue->Push(call); \
		} \
		TrashPendingSend(); \
		_ptr = nullptr; \
	} \
	bool IsDone() { return _done.load(); } \
	void Close(const grpc::Status& status) { if (!_close.exchange(true)) Finish(status); } \
	void Send(RES& message) \
	{ \
		if (_done.load()) return; \
		auto response = std::make_unique<RES>(message); \
		std::lock_guard<std::mutex> lock(_mu); \
		if (!_sending.exchange(true)) { \
			_currentSending = std::move(response); \
			StartWrite(_currentSending.get()); \
		} \
		else \
			_pendingSend.push(std::move(response)); \
	} \
	void OnWriteDone(bool ok) override \
	{ \
		if (!ok || _done.load()) return; \
		std::lock_guard<std::mutex> lock(_mu); \
		if (!_pendingSend.empty()) { \
			_currentSending = std::move(_pendingSend.front()); \
			_pendingSend.pop(); \
			StartWrite(_currentSending.get()); \
		} \
		else { \
			_currentSending = nullptr; \
			_sending.store(false); \
		} \
	} \
	void TrashPendingSend() \
	{ \
		std::lock_guard<std::mutex> lock(_mu); \
		while (!_pendingSend.empty()) \
			_pendingSend.pop(); \
	} \
	void RegisterRead(std::function<void(grpc::CallbackServerContext*, const REQ*, std::any stream)> readCallback) \
	{ \
		_readCallback = readCallback; \
		StartRead(&_readMessage); \
	} \
	void OnReadDone(bool ok) override \
	{ \
		if (!ok) { \
			Close(grpc::Status::OK); \
			return; \
		} \
		auto* call = new RpcJob<REQ>(); \
		call->data = std::make_unique<REQ>(_readMessage); \
		call->stream = shared_from_this(); \
		call->execute = [this](google::protobuf::Message* message, std::any stream) { \
			auto* castedMessage = static_cast<REQ*>(message); \
			this->_readCallback(_context, castedMessage, stream); \
			}; \
		_jobQueue->Push(call); \
		StartRead(&_readMessage); \
	} \
private: \
	std::string _id; \
	grpc::CallbackServerContext* _context = nullptr; \
	std::shared_ptr<FUNC##SvrStream> _ptr; \
	RpcJobQueue<RpcJobBase>* _jobQueue = nullptr; \
	std::atomic_bool _done = false; \
	std::function<void(FUNC##SvrStream*, const grpc::Status& status)> _doneCallback; \
	std::atomic_bool _close = false; \
	std::atomic<bool> _sending = false; \
	std::queue<std::unique_ptr<RES>> _pendingSend; \
	std::unique_ptr<RES> _currentSending; \
	std::mutex _mu; \
	REQ _readMessage; \
	std::function<void(grpc::CallbackServerContext*, const REQ*, std::any stream)> _readCallback; \
};
#pragma endregion

#pragma region DECLARE_SERVER_SSTREAM
#define DECLARE_SERVER_SSTREAM(FUNC, REQ, RES) \
	class FUNC##SvrStream \
		: public grpc::ServerWriteReactor<RES> \
		, public std::enable_shared_from_this<FUNC##SvrStream> \
	{ \
	public: \
		FUNC##SvrStream(grpc::CallbackServerContext* context, RpcJobQueue<RpcJobBase>* jobQ) \
			:_context(context), _jobQueue(jobQ) {} \
		void SetId(std::string& id) { if (_id.empty()) _id = id; } \
		void SetPtr(std::shared_ptr<FUNC##SvrStream> ptr) { _ptr = ptr; } \
		void RegisterDone(std::function<void(FUNC##SvrStream*, const grpc::Status&)> doneCallback) \
		{ \
			_doneCallback = doneCallback; \
		} \
		void OnDone() override \
		{ \
			_done = true; \
			if (_doneCallback) \
			{ \
				auto* call = new RpcJob<REQ>(); \
				call->stream = shared_from_this(); \
				call->execute = [this](google::protobuf::Message* message, std::any stream) { \
					_doneCallback(this, grpc::Status::OK); \
					}; \
				_jobQueue->Push(call); \
			} \
			TrashPendingSend(); \
			_ptr = nullptr; \
		} \
		void Close(const grpc::Status& status) { if (!_close.exchange(true)) Finish(status); } \
		void Send(RES& message) \
		{ \
			if (_done) return; \
			auto response = std::make_unique<RES>(); \
			response->CopyFrom(message); \
			std::lock_guard<std::mutex> lock(_mu); \
			if (!_sending.exchange(true)) \
			{ \
				_currentSending = std::move(response); \
				StartWrite(_currentSending.get()); \
			} \
			else \
			{ \
				_pendingSend.push(std::move(response)); \
			} \
		} \
		void OnWriteDone(bool ok) override \
		{ \
			if (!ok) { \
				Close(grpc::Status(grpc::StatusCode::UNKNOWN, "Unexpected Failure")); \
				return; \
			} \
			std::lock_guard<std::mutex> lock(_mu); \
			if (!_pendingSend.empty()) { \
				_currentSending = std::move(_pendingSend.front()); \
				_pendingSend.pop(); \
				StartWrite(_currentSending.get()); \
			} \
			else { \
				_currentSending = nullptr; \
				_sending.store(false); \
			} \
		} \
		void TrashPendingSend() \
		{ \
			std::lock_guard<std::mutex> lock(_mu); \
			while (!_pendingSend.empty()) \
				_pendingSend.pop(); \
		} \
	private: \
		std::string _id; \
		grpc::CallbackServerContext* _context = nullptr; \
		std::shared_ptr<FUNC##SvrStream> _ptr; \
		RpcJobQueue<RpcJobBase>* _jobQueue = nullptr; \
		std::atomic_bool _done = false; \
		std::atomic_bool _close = false; \
		std::function<void(FUNC##SvrStream*, const grpc::Status&)> _doneCallback = nullptr; \
		std::atomic_bool _sending = false; \
		std::queue<std::unique_ptr<RES>> _pendingSend; \
		std::unique_ptr<RES> _currentSending; \
		std::mutex _mu; \
	};
#pragma endregion

#pragma region DECLARE_SERVER_CSTREAM
#define DECLARE_SERVER_CSTREAM(FUNC, REQ, RES) \
	class FUNC##SvrStream \
		: public grpc::ServerReadReactor<REQ> \
		, public std::enable_shared_from_this<FUNC##SvrStream> \
	{ \
	public: \
		FUNC##SvrStream(grpc::CallbackServerContext* context, RES* response, RpcJobQueue<RpcJobBase>* jobQ) \
			:_context(context), _response(response), _jobQueue(jobQ) {} \
		void SetId(std::string& id) { if (_id.empty()) _id = id; } \
		void SetPtr(std::shared_ptr< FUNC##SvrStream> ptr) { _ptr = ptr; } \
		void RegisterDone(std::function<void(FUNC##SvrStream*, const grpc::Status&)> doneCallback) \
		{ \
			_doneCallback = doneCallback; \
		} \
		void OnDone() override \
		{ \
			_done.store(true); \
			if (_doneCallback) \
			{ \
				auto* call = new RpcJob<REQ>(); \
				call->stream = shared_from_this(); \
				call->execute = [this](google::protobuf::Message* message, std::any stream) { \
					_doneCallback(this, grpc::Status::OK); \
					}; \
				_jobQueue->Push(call); \
			} \
			_ptr = nullptr; \
		} \
		bool IsDone() { return _done.load(); } \
		void Close(const grpc::Status& status) { if (!_close.exchange(true)) Finish(status); } \
		void RegisterRead(std::function<void(grpc::CallbackServerContext*, const REQ*, std::any stream, bool)> readCallback) \
		{ \
			_readCallback = readCallback; \
			StartRead(&_readMessage); \
		} \
		void OnReadDone(bool ok) override \
		{ \
			auto* call = new RpcJob<REQ>(); \
			call->data = std::make_unique<REQ>(_readMessage); \
			call->stream = shared_from_this(); \
			call->execute = [this, ok](google::protobuf::Message* message, std::any stream) { \
				auto* castedMessage = static_cast<REQ*>(message); \
				this->_readCallback(_context, castedMessage, stream, ok); \
				}; \
			_jobQueue->Push(call); \
			if (ok) StartRead(&_readMessage); \
		} \
	private: \
		std::string _id; \
		grpc::CallbackServerContext* _context = nullptr; \
		std::shared_ptr<FUNC##SvrStream> _ptr = nullptr; \
		RpcJobQueue<RpcJobBase>* _jobQueue = nullptr; \
		std::atomic_bool _done = false; \
		std::function<void(FUNC##SvrStream*, const grpc::Status& status)> _doneCallback; \
		std::atomic_bool _close = false; \
		REQ _readMessage; \
		std::function<void(grpc::CallbackServerContext*, const REQ*, std::any, bool)> _readCallback; \
		RES* _response = nullptr; \
	};
#pragma endregion

#pragma region METHOD_SERVER_UNARY
#define METHOD_SERVER_UNARY(FUNC, REQ, RES) \
protected: \
	virtual grpc::Status Server##FUNC(grpc::CallbackServerContext* context, const REQ* request, RES* response) = 0; \
	grpc::ServerUnaryReactor* FUNC(grpc::CallbackServerContext* context, const REQ* request, RES* response) override { \
		auto stream = std::make_shared<FUNC##SvrStream>(); \
		stream->SetPtr(stream); \
		auto* call = new RpcJob<REQ>(); \
		call->stream = stream; \
		call->execute = [this, stream, context, request, response](google::protobuf::Message* m, std::any s) \
			{ stream->Finish(GetInstance()->Server##FUNC(context, request, response)); }; \
		_jobQueue->Push(call); \
		return stream.get(); \
	}
#pragma endregion

#pragma region METHOD_SERVER_BISTREAM
#define METHOD_SERVER_BISTREAM(FUNC, REQ, RES) \
protected: \
	virtual void Server##FUNC(grpc::CallbackServerContext* context, const REQ* request, std::any stream) = 0; \
	virtual void OnOpen##FUNC(std::shared_ptr<FUNC##SvrStream> stream) {}; \
	virtual void OnClose##FUNC(std::shared_ptr<FUNC##SvrStream> stream) {}; \
public: \
	grpc::ServerBidiReactor<REQ, RES>* FUNC(grpc::CallbackServerContext* context) override \
	{ \
		std::string id = GetContextMetaData(context, "id"); \
		auto& client = _clients[id]; \
		std::shared_ptr<FUNC##SvrStream> stream = client.FUNC##Ptr; \
		if (!stream) \
		{ \
			stream = std::make_shared<FUNC##SvrStream>(context, _jobQueue); \
			stream->SetPtr(stream); \
			stream->SetId(id); \
			client.SetId(id); \
			client.FUNC##Ptr = stream; \
			auto* openCall = new RpcJob<REQ>(); \
			openCall->stream = stream; \
			openCall->execute = [this, stream](google::protobuf::Message* message, std::any s) \
				{ GetInstance()->OnOpen##FUNC(stream); }; \
			_jobQueue->Push(openCall); \
			stream->RegisterRead([this](grpc::CallbackServerContext* ctx, const REQ* req, std::any s) \
				{ GetInstance()->Server##FUNC(ctx, req, s); }); \
			stream->RegisterDone([this, &client, stream](FUNC##SvrStream* self, const grpc::Status& status) { \
				GetInstance()->OnClose##FUNC(stream); \
				client.FUNC##Ptr = nullptr; \
				}); \
		} \
		return stream.get(); \
	}
#pragma endregion

#pragma region METHOD_SERVER_SSTREAM
#define METHOD_SERVER_SSTREAM(FUNC, REQ, RES) \
protected: \
	virtual void Server##FUNC(grpc::CallbackServerContext* context, const REQ* request, std::any stream) = 0; \
	virtual void OnOpen##FUNC(std::shared_ptr<FUNC##SvrStream> stream) {}; \
	virtual void OnClose##FUNC(std::shared_ptr<FUNC##SvrStream> stream) {}; \
public: \
	grpc::ServerWriteReactor<RES>* FUNC(grpc::CallbackServerContext* context, const REQ* request) override \
	{ \
		std::string id = GetContextMetaData(context, "id"); \
		auto& client = _clients[id]; \
		std::shared_ptr<FUNC##SvrStream> stream = client.FUNC##Ptr; \
		if (!stream) \
		{ \
			stream = std::make_shared<FUNC##SvrStream>(context, _jobQueue); \
			stream->SetPtr(stream); \
			stream->SetId(id); \
			client.FUNC##Ptr = stream; \
			auto* openCall = new RpcJob<REQ>(); \
			openCall->stream = stream; \
			openCall->execute = [this, stream](google::protobuf::Message* message, std::any s) \
				{ GetInstance()->OnOpen##FUNC(stream); }; \
			_jobQueue->Push(openCall); \
			stream->RegisterDone([this, &client, stream](FUNC##SvrStream* self, const grpc::Status& status) { \
				GetInstance()->OnClose##FUNC(stream); \
				client.FUNC##Ptr = nullptr; \
				}); \
		} \
		auto* call = new RpcJob<REQ>(); \
		call->data = std::make_unique<REQ>(*request); \
		call->stream = stream; \
		call->execute = [this, context](google::protobuf::Message* m, std::any s) \
			{ \
				auto* castdMessage = static_cast<REQ*>(m); \
				GetInstance()->Server##FUNC(context, castdMessage, s); \
			}; \
		_jobQueue->Push(call); \
		return stream.get(); \
	}
#pragma endregion

#pragma region METHOD_SERVER_CSTREAM
#define METHOD_SERVER_CSTREAM(FUNC, REQ, RES) \
protected: \
	virtual void Server##FUNC(grpc::CallbackServerContext* context, const REQ* request, std::any stream) = 0; \
	virtual grpc::Status ServerFinish##FUNC(RES* response, std::shared_ptr<FUNC##SvrStream> stream) = 0; \
	virtual void OnOpen##FUNC(std::shared_ptr<FUNC##SvrStream> stream) {}; \
	virtual void OnClose##FUNC(std::shared_ptr<FUNC##SvrStream> stream) {}; \
public: \
	grpc::ServerReadReactor<REQ>* FUNC(grpc::CallbackServerContext* context, RES* response) override { \
		std::string id = GetContextMetaData(context, "id"); \
		auto& client = _clients[id]; \
		std::shared_ptr<FUNC##SvrStream> stream = client.FUNC##Ptr; \
		if (!stream) \
		{ \
			stream = std::make_shared<FUNC##SvrStream>(context, response, _jobQueue); \
			stream->SetPtr(stream); \
			stream->SetId(id); \
			client.SetId(id); \
			client.FUNC##Ptr = stream; \
			auto* openCall = new RpcJob<REQ>(); \
			openCall->stream = stream; \
			openCall->execute = [this, stream](google::protobuf::Message* message, std::any s) \
				{ GetInstance()->OnOpen##FUNC(stream); }; \
			_jobQueue->Push(openCall); \
			stream->RegisterRead([response, stream, this](grpc::CallbackServerContext* ctx, const REQ* request, std::any s, bool ok) { \
				if (!ok) stream->Close(ServerFinish##FUNC(response, stream)); \
				else GetInstance()->Server##FUNC(ctx, request, s); \
				}); \
			stream->RegisterDone([this, &client, stream](FUNC##SvrStream* self, const grpc::Status& status) { \
				GetInstance()->OnClose##FUNC(stream); \
				client.FUNC##Ptr = nullptr; \
				}); \
		} \
		return stream.get(); \
	}
#pragma endregion

#pragma region INNER_SERVER_UNARY
#define INNER_SERVER_UNARY(FUNC, REQ, RES)
#pragma endregion

#pragma region INNER_SERVER_BISTREAM
#define INNER_SERVER_BISTREAM(FUNC, REQ, RES) \
std::shared_ptr<FUNC##SvrStream> FUNC##Ptr = nullptr; \
void Client##FUNC(RES& response) { if (FUNC##Ptr) FUNC##Ptr->Send(response); } \
void Close##FUNC(const grpc::Status& status = grpc::Status::OK) \
{ \
	if (FUNC##Ptr) FUNC##Ptr->Close(status); \
}
#pragma endregion

#pragma region INNER_SERVER_SSTREAM
#define INNER_SERVER_SSTREAM(FUNC, REQ, RES) \
std::shared_ptr<FUNC##SvrStream> FUNC##Ptr = nullptr; \
void Client##FUNC(RES& response) { if (FUNC##Ptr) FUNC##Ptr->Send(response); } \
void Close##FUNC(const grpc::Status& status = grpc::Status::OK) \
{ \
	if (FUNC##Ptr) FUNC##Ptr->Close(status); \
}
#pragma endregion

#pragma region INNER_SERVER_CSTREAM
#define INNER_SERVER_CSTREAM(FUNC, REQ, RES) \
std::shared_ptr<FUNC##SvrStream> FUNC##Ptr = nullptr;
#pragma endregion

#pragma region CLIENT_UNARY
#define CLIENT_UNARY(FUNC,REQ,RES) \
public: \
void Server##FUNC (const REQ& request) { \
auto* call = new RpcJob<RES>(); \
call->data = std::make_unique<RES>(); \
call->execute = [this](google::protobuf::Message* response, std::any stream) { \
	auto* typedResponse = static_cast<RES*>(response); \
	this->On##FUNC(typedResponse); \
}; \
call->response_reader = _stub->PrepareAsync##FUNC(&call->context, request, &_rpcCompletionQueue); \
call->response_reader->StartCall(); \
call->response_reader->Finish(static_cast<RES*>(call->data.get()), &call->status, (void*)call); \
} \
protected: \
virtual void On##FUNC(const RES* response) = 0;
#pragma endregion

#pragma region CLIENT_BISTREAM
#define CLIENT_BISTREAM(FUNC, REQ, RES) \
protected: \
	class FUNC##CltStream \
		: public grpc::ClientBidiReactor<REQ, RES> \
		, public std::enable_shared_from_this<FUNC##CltStream> \
	{ \
	public: \
		FUNC##CltStream(RpcJobQueue<RpcJobBase>* jobQ) : _jobQueue(jobQ) {} \
		void Start(Greeter::Stub* stub, const std::string& id) \
		{ \
			_id = id; \
			_context.AddMetadata("id", _id); \
			stub->async()->FUNC##(&_context, this); \
			StartRead(&_readMessage); \
			StartCall(); \
		} \
		void SetPtr(std::shared_ptr<FUNC##CltStream> ptr) { _ptr = ptr; } \
		void RegisterDone(std::function<void(FUNC##CltStream*, const grpc::Status&)> doneCallback) \
		{ \
			_doneCallback = doneCallback; \
		} \
		void OnDone(const grpc::Status& s) override \
		{ \
			_status = std::move(s); \
			_done = true; \
			if (_doneCallback) \
			{ \
				auto* call = new RpcJob<REQ>(); \
				call->stream = shared_from_this(); \
				call->execute = [this](google::protobuf::Message* message, std::any stream) { \
					_doneCallback(this, _status); \
					}; \
				_jobQueue->Push(call); \
			} \
			if (!_sending.load()) \
				_ptr = nullptr; \
		} \
		void Send(REQ& message) { \
			if (_done) return; \
			auto request = std::make_unique<REQ>(message); \
			std::lock_guard<std::mutex> lock(_mu); \
			if (!_sending.exchange(true)) \
			{ \
				_currentSending = std::move(request); \
				StartWrite(_currentSending.get()); \
			} \
			else \
			{ \
				_pendingSend.push(std::move(request)); \
			} \
		} \
		void OnWriteDone(bool ok) override { \
			if (!ok) { \
				RemoveHold(); \
				return; \
			} \
			std::lock_guard<std::mutex> lock(_mu); \
			if (!_pendingSend.empty()) { \
				_currentSending = std::move(_pendingSend.front()); \
				_pendingSend.pop(); \
				StartWrite(_currentSending.get()); \
			} \
			else { \
				_currentSending = nullptr; \
				_sending.store(false); \
				if (_done) _ptr = nullptr; \
			} \
		} \
		void RegisterRead(std::function<void(const RES*, std::any stream)> readCallback) \
		{ \
			_readCallback = readCallback; \
		} \
		void OnReadDone(bool ok) override \
		{ \
			if (!ok) return; \
			auto* call = new RpcJob<RES>(); \
			call->data = std::make_unique<RES>(_readMessage); \
			call->stream = shared_from_this(); \
			call->execute = [this](google::protobuf::Message* message, std::any stream) { \
				auto* castedMessage = static_cast<RES*>(message); \
				this->_readCallback(castedMessage, stream); \
				}; \
			_jobQueue->Push(call); \
			StartRead(&_readMessage); \
		} \
	private: \
		std::string _id; \
		grpc::ClientContext _context; \
		std::shared_ptr<FUNC##CltStream> _ptr; \
		RpcJobQueue<RpcJobBase>* _jobQueue = nullptr; \
		grpc::Status _status; \
		bool _done = false; \
		std::function<void(FUNC##CltStream*, const grpc::Status&)> _doneCallback; \
		std::atomic<bool> _sending{ false }; \
		std::queue<std::unique_ptr<REQ>> _pendingSend; \
		std::unique_ptr<REQ> _currentSending; \
		std::mutex _mu; \
		RES _readMessage; \
		std::function<void(const RES*, std::any stream)> _readCallback; \
	}; \
	std::shared_ptr<FUNC##CltStream> FUNC##Ptr; \
	virtual void Client##FUNC(const RES* response, std::any stream) = 0; \
	virtual void OnClose##FUNC(const grpc::Status& status) {}; \
public: \
	void Server##FUNC(REQ& request) { \
		Open##FUNC(); \
		if (FUNC##Ptr) FUNC##Ptr->Send(request); \
	} \
	void Open##FUNC() \
	{ \
		if (!FUNC##Ptr) \
		{ \
			FUNC##Ptr = std::make_shared<FUNC##CltStream>(&_jobQueue); \
			FUNC##Ptr->SetPtr(FUNC##Ptr); \
			FUNC##Ptr->RegisterRead([this](const RES* response, std::any stream) { GetInstance()->Client##FUNC(response, stream); }); \
			FUNC##Ptr->RegisterDone([this](FUNC##CltStream* self, const grpc::Status& s) { FUNC##Ptr = nullptr; }); \
			FUNC##Ptr->Start(_stub.get(), _id); \
		} \
	}
#pragma endregion

#pragma region CLIENT_SSTREAM
#define CLIENT_SSTREAM(FUNC, REQ, RES) \
protected: \
	class FUNC##CltStream \
		: public grpc::ClientReadReactor<RES> \
		, public std::enable_shared_from_this<FUNC##CltStream> \
	{ \
	public: \
		FUNC##CltStream(RpcJobQueue<RpcJobBase>* jobQ) : _jobQueue(jobQ) {} \
		void Start(Greeter::Stub* stub, const std::string& id, const REQ& request) \
		{ \
			_id = id; \
			_context.AddMetadata("id", _id); \
			stub->async()->FUNC(&_context, &request, this); \
			StartRead(&_readMessage); \
			StartCall(); \
		} \
		void SetPtr(std::shared_ptr<FUNC##CltStream> ptr) { _ptr = ptr; } \
		void RegisterDone(std::function<void(FUNC##CltStream*, const grpc::Status&)> doneCallback) \
		{ \
			_doneCallback = doneCallback; \
		} \
		void OnDone(const grpc::Status& s) override \
		{ \
			_status = std::move(s); \
			_done = true; \
			if (_doneCallback) \
			{ \
				auto* call = new RpcJob<REQ>(); \
				call->stream = shared_from_this(); \
				call->execute = [this](google::protobuf::Message* message, std::any stream) { \
					_doneCallback(this, _status); \
					}; \
				_jobQueue->Push(call); \
			} \
			_ptr = nullptr; \
		}; \
		void RegisterRead(std::function<void(const RES*, std::any stream)> readCallback) \
		{ \
			_readCallback = readCallback; \
		} \
		void OnReadDone(bool ok) override { \
			if (!ok) return; \
			auto* call = new RpcJob<RES>(); \
			call->data = std::make_unique<RES>(_readMessage); \
			call->stream = shared_from_this(); \
			call->execute = [this](google::protobuf::Message* message, std::any stream) { \
				auto* castedMessage = static_cast<RES*>(message); \
				this->_readCallback(castedMessage, stream); \
				}; \
			_jobQueue->Push(call); \
			StartRead(&_readMessage); \
		} \
	protected: \
		std::string _id; \
		grpc::ClientContext _context; \
		std::shared_ptr<FUNC##CltStream> _ptr = nullptr; \
		RpcJobQueue<RpcJobBase>* _jobQueue = nullptr; \
		grpc::Status _status; \
		bool _done = false; \
		std::function<void(FUNC##CltStream*, const grpc::Status&)> _doneCallback; \
		RES _readMessage; \
		std::function<void(const RES*, std::any stream)> _readCallback; \
	}; \
	virtual void Client##FUNC(const RES* response, std::any stream) = 0; \
	virtual void OnClose##FUNC(const ::grpc::Status& status) {}; \
public: \
	void Server##FUNC(const REQ& request) { \
		auto stream = std::make_shared<FUNC##CltStream>(&_jobQueue); \
		stream->SetPtr(stream); \
		stream->RegisterRead([this](const RES* response, std::any stream) { GetInstance()->Client##FUNC(response, stream); }); \
		stream->RegisterDone([this](FUNC##CltStream* self, const grpc::Status& s) { GetInstance()->OnClose##FUNC(s); }); \
		stream->Start(_stub.get(), _id, request); \
	}
#pragma endregion

#pragma region CLIENT_CSTREAM
#define CLIENT_CSTREAM(FUNC, REQ, RES) \
protected: \
	class FUNC##CltStream \
		: public grpc::ClientWriteReactor<REQ> \
		, public std::enable_shared_from_this<FUNC##CltStream> \
	{ \
	public: \
		FUNC##CltStream(RpcJobQueue<RpcJobBase>* jobQ) : _jobQueue(jobQ) {} \
		void Start(Greeter::Stub* stub, const std::string& id) \
		{ \
			_id = id; \
			_context.AddMetadata("id", _id); \
			stub->async()->FUNC##(&_context, &_readMessage, this); \
			StartCall(); \
		} \
		void SetPtr(std::shared_ptr<FUNC##CltStream> ptr) { _ptr = ptr; } \
		void RegisterDone(std::function<void(FUNC##CltStream*, const grpc::Status&)> doneCallback) \
		{ \
			_doneCallback = doneCallback; \
		} \
		void OnDone(const grpc::Status& s) override \
		{ \
			_status = std::move(s); \
			_done = true; \
			if (_doneCallback) \
			{ \
				auto* call = new RpcJob<REQ>(); \
				call->stream = shared_from_this(); \
				call->execute = [this](google::protobuf::Message* message, std::any stream) { \
					_doneCallback(this, _status); \
					}; \
				_jobQueue->Push(call); \
			} \
			if (!_sending.load()) \
				_ptr = nullptr; \
		} \
		void Send(REQ& message) { \
			if (_done) return; \
			auto request = std::make_unique<REQ>(message); \
			std::lock_guard<std::mutex> lock(_mu); \
			if (!_sending.exchange(true)) \
			{ \
				_currentSending = std::move(request); \
				StartWrite(_currentSending.get()); \
			} \
			else \
			{ \
				_pendingSend.push(std::move(request)); \
			} \
		} \
		void FinishSend() \
		{ \
			_finishSend.store(true); \
			if (!_sending.load()) StartWritesDone(); \
		} \
		void OnWriteDone(bool ok) override { \
			if (!ok) { \
				return; \
			} \
			std::lock_guard<std::mutex> lock(_mu); \
			if (!_pendingSend.empty()) { \
				_currentSending = std::move(_pendingSend.front()); \
				_pendingSend.pop(); \
				StartWrite(_currentSending.get()); \
			} \
			else { \
				_currentSending = nullptr; \
				_sending.store(false); \
				if (_finishSend.load()) StartWritesDone(); \
				if (_done) _ptr = nullptr; \
			} \
		} \
		RES* GetResponse() { return &_readMessage; } \
	private: \
		std::string _id; \
		grpc::ClientContext _context; \
		std::shared_ptr<FUNC##CltStream> _ptr; \
		RpcJobQueue<RpcJobBase>* _jobQueue = nullptr; \
		grpc::Status _status; \
		bool _done = false; \
		std::function<void(FUNC##CltStream*, const grpc::Status&)> _doneCallback; \
		std::atomic<bool> _sending{ false }; \
		std::queue<std::unique_ptr<REQ>> _pendingSend; \
		std::unique_ptr<REQ> _currentSending; \
		std::mutex _mu; \
		std::atomic<bool> _finishSend{ false }; \
		RES _readMessage; \
	}; \
	std::shared_ptr<FUNC##CltStream> FUNC##Ptr = nullptr; \
	virtual void OnFinish##FUNC(RES* response, const grpc::Status& status) = 0; \
public: \
	void Server##FUNC(REQ& request) \
	{ \
		if (FUNC##Ptr == nullptr) \
		{ \
			FUNC##Ptr = std::make_shared<FUNC##CltStream>(&_jobQueue); \
			FUNC##Ptr->SetPtr(##FUNC##Ptr); \
			FUNC##Ptr->RegisterDone( \
				[this](FUNC##CltStream* self, const grpc::Status& s) { \
					GetInstance()->OnFinish##FUNC##(self->GetResponse(), s); \
					FUNC##Ptr = nullptr; \
				}); \
			FUNC##Ptr->Start(_stub.get(), _id); \
		} \
		FUNC##Ptr->Send(request); \
	} \
	void ServerFinish##FUNC() { if (FUNC##Ptr != nullptr) FUNC##Ptr->FinishSend(); }
#pragma endregion

#define CAST_SERVER_STREAM(FUNC, stream) std::any_cast<std::shared_ptr<FUNC##SvrStream>>(stream)
#define CAST_CLIENT_STREAM(FUNC, stream) std::any_cast<std::shared_ptr<FUNC##CltStream>>(stream)

template<typename T>
class RpcDoneModule
{
public:
	void RegisterDone(std::function<void(T*, const grpc::Status&)> doneCallback)
	{
		_doneCallback = doneCallback;
	}
protected:
	grpc::Status _status;
	bool _done = false;
	std::function<void(T*, const grpc::Status&)> _doneCallback;
};

template<typename T>
class RpcWriteModule
{
protected:
	virtual void RpcWrite(T* request) = 0;
	void RpcRequestWrite(const T& message)
	{
		auto request = std::make_unique<T>();
		request->CopyFrom(message);
		if (!_sending.exchange(true))
		{
			{
				std::lock_guard<std::mutex> lock(_mu);
				_currentSending = std::move(request);
			}
			RpcWrite(_currentSending.get());
		}
		else
		{
			std::lock_guard<std::mutex> lock(_mu);
			_pendingSend.push(std::move(request));
		}
	}
	void RpcNextWrite()
	{
		if (!_pendingSend.empty()) {
			{
				std::lock_guard<std::mutex> lock(_mu);
				//std::cout << "q size: " << _pendingSend.size() << std::endl;
				_currentSending = std::move(_pendingSend.front());
				_pendingSend.pop();
			}
			RpcWrite(_currentSending.get());
		}
		else {
			_currentSending = nullptr;
			_sending.store(false);
		}
	}

protected:
	std::atomic<bool> _sending{ false };
	std::queue<std::unique_ptr<T>> _pendingSend;
	std::unique_ptr<T> _currentSending;
	std::mutex _mu;
};

template<typename T>
class RpcServerReadModule
{
public:
	void RegisterRead(RpcJobQueue<RpcJobBase>* jobQ, std::function<void(grpc::CallbackServerContext*, const T*, std::any stream)> readCallback)
	{
		_jobQueue = jobQ;
		_readCallback = readCallback;
	}

protected:
	virtual void RpcRead(T* message) = 0;
	virtual grpc::CallbackServerContext* GetServerContext() = 0;
	void RpcNextRead()
	{
		auto* call = new RpcJob<T>();
		call->data = std::make_unique<T>(_readMessage);
		call->stream = this;
		call->execute = [this](google::protobuf::Message* message, std::any stream) {
			auto* castedMessage = static_cast<T*>(message);
			this->_readCallback(GetServerContext(), castedMessage, stream);
			};
		_jobQueue->Push(call);
		RpcRead(&_readMessage);
	}
protected:
	T _readMessage;
	RpcJobQueue<RpcJobBase>* _jobQueue = nullptr;
	std::function<void(grpc::CallbackServerContext*, const T*, std::any stream)> _readCallback;
};

template<typename T>
class RpcClientReadModule
{
public:
	void RegisterRead(RpcJobQueue<RpcJobBase>* jobQ, std::function<void(const T*, std::any stream)> readCallback)
	{
		_jobQueue = jobQ;
		_readCallback = readCallback;
	}
protected:
	virtual void RpcRead(T* message) = 0;
	void RpcNextRead()
	{
		auto* call = new RpcJob<T>();
		call->data = std::make_unique<T>(_readMessage);
		call->stream = this;
		call->execute = [this](google::protobuf::Message* message, std::any stream) {
			auto* castedMessage = static_cast<T*>(message);
			this->_readCallback(castedMessage, stream);
			};
		_jobQueue->Push(call);
		RpcRead(&_readMessage);
	}
protected:
	T _readMessage;
	RpcJobQueue<RpcJobBase>* _jobQueue = nullptr;
	std::function<void(const T*, std::any stream)> _readCallback;
};