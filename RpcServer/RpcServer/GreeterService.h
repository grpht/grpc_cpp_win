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

#include "commons/RpcServiceBase.h"
#include "commons/RpcTemplate.h"

#include "models/greeter.grpc.pb.h"

using namespace helloworld;

namespace helloworld
{
	class GreeterService : public helloworld::Greeter::CallbackService, public RpcServiceBase
	{
	protected:
		virtual GreeterService* GetInstance() = 0;
	public:
		
		//SERVER_BISTREAM_RECV
		//SERVER_UNARY(SayHello, HelloRequest, HelloReply)
	
#pragma region UNARY
	public:
		//UNARY
		class SayHelloSvrStream : public grpc::ServerUnaryReactor {
		public:
			SayHelloSvrStream(grpc::CallbackServerContext* context) : _context(context) {}
			void SetPtr(std::shared_ptr< SayHelloSvrStream> ptr) { _ptr = ptr; }
			grpc::CallbackServerContext* GetContext() { return _context; }
		private:
			void OnDone() override { _ptr = nullptr; }
			void OnCancel() override { _ptr = nullptr; }
		private:
			std::string _id;
			std::shared_ptr< SayHelloSvrStream> _ptr = nullptr;
			grpc::CallbackServerContext* _context = nullptr;
		};
	protected:
		
		virtual grpc::Status ServerSayHello(const HelloRequest* request, HelloReply* response, SayHelloSvrStream* stream) = 0;

		grpc::ServerUnaryReactor* SayHello(grpc::CallbackServerContext* context, const HelloRequest* request, HelloReply* response) override {
			auto stream = std::make_shared<SayHelloSvrStream>(context);
			auto ptr = stream.get();
			stream->SetPtr(stream);
			auto* call = new RpcJob<HelloRequest>();
			call->stream = stream;
			call->execute = [this, request, response, ptr](google::protobuf::Message* m, std::any any)
				{ ptr->Finish(GetInstance()->ServerSayHello(request, response, ptr)); };
			_jobQueue->Push(call);
			return stream.get();
		}
#pragma endregion

#pragma region BISTREAM
	public:
		//SERVER_BISTREAM_RECV
		class SayHelloBDSSvrStream
			: public grpc::ServerBidiReactor<HelloRequest, HelloReply>
			, public std::enable_shared_from_this<SayHelloBDSSvrStream>
		{
		public:
			SayHelloBDSSvrStream(grpc::CallbackServerContext* context, RpcJobQueue<RpcJobBase>* jobQ)
				:_context(context), _jobQueue(jobQ) { }
			void SetId(std::string& id) { if (_id.empty()) _id = id; }
			const std::string& GetId() const { return _id; }
			void SetPtr(std::shared_ptr< SayHelloBDSSvrStream> ptr) { _ptr = ptr; }
			grpc::CallbackServerContext* GetContext() { return _context; }
			void RegisterDone(std::function<void(std::shared_ptr<SayHelloBDSSvrStream>, const grpc::Status&)> doneCallback)
			{
				_doneCallback = doneCallback;
			}
			void OnDone() override
			{
				_done.store(true);
				if (_doneCallback)
				{
					auto* call = new RpcJob<HelloRequest>();
					call->stream = shared_from_this();
					call->execute = [this](google::protobuf::Message* message, std::any any) {
						_doneCallback(shared_from_this(), grpc::Status::OK);
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

			void RegisterRead(std::function<void(const HelloRequest*)> readCallback)
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
				call->execute = [this](google::protobuf::Message* message, std::any any) {
					auto* castedMessage = static_cast<HelloRequest*>(message);
					this->_readCallback(castedMessage);
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
			std::function<void(std::shared_ptr<SayHelloBDSSvrStream>, const grpc::Status&)> _doneCallback;
			std::atomic_bool _close = false;

			std::atomic<bool> _sending = false;
			std::queue<std::unique_ptr<HelloReply>> _pendingSend;
			std::unique_ptr<HelloReply> _currentSending;
			std::mutex _mu;

			HelloRequest _readMessage;
			std::function<void(const HelloRequest*)> _readCallback;
		};

	protected:
		virtual void ServerSayHelloBDS(const HelloRequest* request, SayHelloBDSSvrStream* stream) = 0;
		virtual void OnOpenSayHelloBDS(std::shared_ptr<SayHelloBDSSvrStream> stream) {};
		virtual void OnCloseSayHelloBDS(std::shared_ptr<SayHelloBDSSvrStream> stream) {};
	public:
		grpc::ServerBidiReactor<HelloRequest, HelloReply>* SayHelloBDS(grpc::CallbackServerContext* context) override 
		{
			std::string id = GetContextMetaData(context, "id");
			std::shared_ptr<SayHelloBDSSvrStream> stream = std::make_shared<SayHelloBDSSvrStream>(context, _jobQueue);
			auto ptr = stream.get();
			stream->SetPtr(stream);
			stream->SetId(id);
			OnOpenSayHelloBDS(stream);
			stream->RegisterRead([this, ptr](const HelloRequest* req)
				{ GetInstance()->ServerSayHelloBDS(req, ptr); });
			stream->RegisterDone([this](std::shared_ptr<SayHelloBDSSvrStream> self, const grpc::Status& status) {
				GetInstance()->OnCloseSayHelloBDS(self);
				});
			return stream.get();
		}
#pragma endregion

#pragma region SSTREAM
	public:
		class SayHelloStreamReplySvrStream
			: public grpc::ServerWriteReactor<HelloReply>
			, public std::enable_shared_from_this<SayHelloStreamReplySvrStream>
		{
		public:
			SayHelloStreamReplySvrStream(grpc::CallbackServerContext* context, RpcJobQueue<RpcJobBase>* jobQ)
				:_context(context), _jobQueue(jobQ) {}
			void SetId(std::string& id) { if (_id.empty()) _id = id; }
			const std::string& GetId() const { return _id; }
			void SetPtr(std::shared_ptr<SayHelloStreamReplySvrStream> ptr) { _ptr = ptr; }
			grpc::CallbackServerContext* GetContext() { return _context; }
			void RegisterDone(std::function<void(std::shared_ptr<SayHelloStreamReplySvrStream>, const grpc::Status&)> doneCallback)
			{
				_doneCallback = doneCallback;
			}
			void OnDone() override
			{
				_done = true;
				if (_doneCallback)
				{
					auto* call = new RpcJob<HelloRequest>();
					call->stream = shared_from_this();
					call->execute = [this](google::protobuf::Message* message, std::any any) {
						_doneCallback(shared_from_this(), grpc::Status::OK);
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

			std::function<void(std::shared_ptr<SayHelloStreamReplySvrStream>, const grpc::Status&)> _doneCallback = nullptr;
			std::atomic_bool _sending = false;
			std::queue<std::unique_ptr<HelloReply>> _pendingSend;
			std::unique_ptr<HelloReply> _currentSending;
			std::mutex _mu;
		};
	protected:
		virtual void ServerSayHelloStreamReply(const HelloRequest* request, SayHelloStreamReplySvrStream* stream) = 0;
		virtual void OnOpenSayHelloStreamReply(std::shared_ptr<SayHelloStreamReplySvrStream> stream) {};
		virtual void OnCloseSayHelloStreamReply(std::shared_ptr<SayHelloStreamReplySvrStream> stream) {};
	public:
		//SERVER_SSTREAM_RECV
		grpc::ServerWriteReactor<HelloReply>* SayHelloStreamReply(grpc::CallbackServerContext* context, const HelloRequest* request) override
		{
			std::string id = GetContextMetaData(context, "id");
			std::shared_ptr<SayHelloStreamReplySvrStream> stream = std::make_shared<SayHelloStreamReplySvrStream>(context, _jobQueue);
			auto ptr = stream.get();
			stream->SetPtr(stream);
			stream->SetId(id);
			OnOpenSayHelloStreamReply(stream);
			stream->RegisterDone([this](std::shared_ptr<SayHelloStreamReplySvrStream> self, const grpc::Status& status) {
				GetInstance()->OnCloseSayHelloStreamReply(self);
				});
			auto* call = new RpcJob<HelloRequest>();
			call->data = std::make_unique<HelloRequest>(*request);
			call->stream = stream;
			call->execute = [this, context, ptr](google::protobuf::Message* m, std::any any)
				{
					auto* castdMessage = static_cast<HelloRequest*>(m);
					GetInstance()->ServerSayHelloStreamReply(castdMessage, ptr);
				};
			_jobQueue->Push(call);
			return stream.get();
		}
#pragma endregion
	
#pragma region CLIENT_STREAM
	public:
		class SayHelloRecordSvrStream
			: public grpc::ServerReadReactor<HelloRequest>
			, public std::enable_shared_from_this<SayHelloRecordSvrStream>
		{
		public:
			SayHelloRecordSvrStream(grpc::CallbackServerContext* context, HelloReply* response, RpcJobQueue<RpcJobBase>* jobQ)
				:_context(context), _response(response), _jobQueue(jobQ) {}

			void SetId(std::string& id) { if (_id.empty()) _id = id; }
			const std::string& GetId() const { return _id; }
			void SetPtr(std::shared_ptr< SayHelloRecordSvrStream> ptr) { _ptr = ptr; }
			grpc::CallbackServerContext* GetContext() { return _context; }
			void RegisterDone(std::function<void(std::shared_ptr<SayHelloRecordSvrStream>, const grpc::Status&)> doneCallback)
			{
				_doneCallback = doneCallback;
			}
			void OnDone() override
			{
				_done.store(true);
				if (_doneCallback)
				{
					auto* call = new RpcJob<HelloRequest>();
					call->stream = shared_from_this();
					call->execute = [this](google::protobuf::Message* message, std::any any) {
						_doneCallback(shared_from_this(), grpc::Status::OK);
						};
					_jobQueue->Push(call);
				}
				_ptr = nullptr;
			}
			bool IsDone() { return _done.load(); }
			void Close(const grpc::Status& status) { if (!_close.exchange(true)) Finish(status); }
			void RegisterRead(std::function<void(const HelloRequest*, bool)> readCallback)
			{
				_readCallback = readCallback;
				StartRead(&_readMessage);
			}
			void OnReadDone(bool ok) override
			{
				auto* call = new RpcJob<HelloRequest>();
				call->data = std::make_unique<HelloRequest>(_readMessage);
				call->stream = shared_from_this();
				call->execute = [this, ok](google::protobuf::Message* message, std::any any) {
					auto* castedMessage = static_cast<HelloRequest*>(message);
					this->_readCallback(castedMessage, ok);
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
			std::function<void(std::shared_ptr<SayHelloRecordSvrStream>, const grpc::Status&)> _doneCallback;
			std::atomic_bool _close = false;

			HelloRequest _readMessage;
			std::function<void(const HelloRequest*, bool)> _readCallback;

			HelloReply* _response = nullptr;
		};

		//CLIENT_STREAM
	protected:
		virtual void ServerSayHelloRecord(const HelloRequest* request, SayHelloRecordSvrStream* stream) = 0;
		virtual grpc::Status ServerFinishSayHelloRecord(HelloReply* response, SayHelloRecordSvrStream* stream) = 0;
		virtual void OnOpenSayHelloRecord(std::shared_ptr<SayHelloRecordSvrStream> stream) {};
		virtual void OnCloseSayHelloRecord(std::shared_ptr<SayHelloRecordSvrStream> stream) {};
	public:
		grpc::ServerReadReactor<HelloRequest>* SayHelloRecord(grpc::CallbackServerContext* context, HelloReply* response) override {
			std::string id = GetContextMetaData(context, "id");
			std::shared_ptr< SayHelloRecordSvrStream> stream = std::make_shared<SayHelloRecordSvrStream>(context, response, _jobQueue);
			auto ptr = stream.get();
			stream->SetPtr(stream);
			stream->SetId(id);
			OnOpenSayHelloRecord(stream);
			stream->RegisterRead([this, response, ptr](const HelloRequest* request, bool ok) {
				if (!ok) ptr->Close(ServerFinishSayHelloRecord(response, ptr));
				else GetInstance()->ServerSayHelloRecord(request, ptr);
				});
			stream->RegisterDone([this, stream](std::shared_ptr<SayHelloRecordSvrStream> self, const grpc::Status& status) {
				GetInstance()->OnCloseSayHelloRecord(stream);
				});
			return stream.get();
		}
	};
#pragma endregion;
}
