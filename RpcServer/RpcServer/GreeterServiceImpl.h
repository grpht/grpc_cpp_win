#pragma once
#pragma warning(disable : 4251)
#pragma warning(disable : 4819)

#include "GreeterService.h"

namespace helloworld
{
	class GreeterServiceImpl : public GreeterService
	{
		// GreeterService을(를) 통해 상속됨
		GreeterService* GetInstance() override;

		class RpcClientSession
		{
		public:
			//BiStream
			std::shared_ptr<SayHelloBDSSvrStream> SayHelloBDSPtr = nullptr;
			void ClientSayHelloBDS(HelloReply& response) { if (SayHelloBDSPtr) SayHelloBDSPtr->Send(response); }
			void CloseSayHelloBDS(const grpc::Status& status = grpc::Status::OK)
			{
				if (SayHelloBDSPtr) SayHelloBDSPtr->Close(status);
			}

			//ServerStream
			std::shared_ptr<SayHelloStreamReplySvrStream> SayHelloStreamReplyPtr = nullptr;
			void ClientSayHelloStreamReply(HelloReply& response) { if (SayHelloStreamReplyPtr) SayHelloStreamReplyPtr->Send(response); }
			void CloseSayHelloStreamReplyPtr(const grpc::Status& status = grpc::Status::OK)
			{
				if (SayHelloStreamReplyPtr) SayHelloStreamReplyPtr->Close(status);
			}

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

		grpc::Status ServerSayHello(const HelloRequest* request, HelloReply* response, SayHelloSvrStream* stream) override;
		
		
		void ServerSayHelloBDS(const HelloRequest* request, SayHelloBDSSvrStream* stream) override;
		void OnOpenSayHelloBDS(std::shared_ptr<SayHelloBDSSvrStream> stream) override;
		void OnCloseSayHelloBDS(std::shared_ptr<SayHelloBDSSvrStream> stream) override;

		void ServerSayHelloStreamReply(const HelloRequest* request, SayHelloStreamReplySvrStream* stream) override;
		void OnOpenSayHelloStreamReply(std::shared_ptr<SayHelloStreamReplySvrStream> stream);
		void OnCloseSayHelloStreamReply(std::shared_ptr<SayHelloStreamReplySvrStream> stream);

		void ServerSayHelloRecord(const HelloRequest* request, SayHelloRecordSvrStream* stream) override;
		void OnOpenSayHelloRecord(std::shared_ptr< SayHelloRecordSvrStream> stream);
		void OnCloseSayHelloRecord(std::shared_ptr< SayHelloRecordSvrStream> stream);
		grpc::Status ServerFinishSayHelloRecord(HelloReply* response, SayHelloRecordSvrStream* stream) override;

	private:
		int cStreamCall = 0;
	};

}
