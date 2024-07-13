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

#include "commons/RpcJob.h"

#define SERVER_UNARY(FUNC,REQ,RES) \
virtual ::grpc::ServerUnaryReactor* FUNC( \
::grpc::CallbackServerContext* context, const REQ* request, RES* response) \
{ \
	grpc::ServerUnaryReactor* reactor = context->DefaultReactor(); \
	reactor->Finish(Server##FUNC(request, response)); \
	return reactor; \
} \
virtual ::grpc::Status Server##FUNC(const REQ* request, RES* response) = 0; \

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
virtual void On##FUNC(const RES* response) = 0; \

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