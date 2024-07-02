// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: greeter.proto

#include "greeter.pb.h"
#include "greeter.grpc.pb.h"

#include <functional>
#include <grpcpp/support/async_stream.h>
#include <grpcpp/support/async_unary_call.h>
#include <grpcpp/impl/channel_interface.h>
#include <grpcpp/impl/client_unary_call.h>
#include <grpcpp/support/client_callback.h>
#include <grpcpp/support/message_allocator.h>
#include <grpcpp/support/method_handler.h>
#include <grpcpp/impl/rpc_service_method.h>
#include <grpcpp/support/server_callback.h>
#include <grpcpp/impl/codegen/server_callback_handlers.h>
#include <grpcpp/server_context.h>
#include <grpcpp/impl/service_type.h>
#include <grpcpp/support/sync_stream.h>
namespace helloworld {

static const char* Greeter_method_names[] = {
  "/helloworld.Greeter/SayHello",
  "/helloworld.Greeter/SayHelloStreamReply",
  "/helloworld.Greeter/SayHelloBDS",
  "/helloworld.Greeter/SayHelloRecord",
};

std::unique_ptr< Greeter::Stub> Greeter::NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options) {
  (void)options;
  std::unique_ptr< Greeter::Stub> stub(new Greeter::Stub(channel, options));
  return stub;
}

Greeter::Stub::Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options)
  : channel_(channel), rpcmethod_SayHello_(Greeter_method_names[0], options.suffix_for_stats(),::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_SayHelloStreamReply_(Greeter_method_names[1], options.suffix_for_stats(),::grpc::internal::RpcMethod::SERVER_STREAMING, channel)
  , rpcmethod_SayHelloBDS_(Greeter_method_names[2], options.suffix_for_stats(),::grpc::internal::RpcMethod::BIDI_STREAMING, channel)
  , rpcmethod_SayHelloRecord_(Greeter_method_names[3], options.suffix_for_stats(),::grpc::internal::RpcMethod::CLIENT_STREAMING, channel)
  {}

::grpc::Status Greeter::Stub::SayHello(::grpc::ClientContext* context, const ::helloworld::HelloRequest& request, ::helloworld::HelloReply* response) {
  return ::grpc::internal::BlockingUnaryCall< ::helloworld::HelloRequest, ::helloworld::HelloReply, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), rpcmethod_SayHello_, context, request, response);
}

void Greeter::Stub::async::SayHello(::grpc::ClientContext* context, const ::helloworld::HelloRequest* request, ::helloworld::HelloReply* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall< ::helloworld::HelloRequest, ::helloworld::HelloReply, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_SayHello_, context, request, response, std::move(f));
}

void Greeter::Stub::async::SayHello(::grpc::ClientContext* context, const ::helloworld::HelloRequest* request, ::helloworld::HelloReply* response, ::grpc::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create< ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_SayHello_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::helloworld::HelloReply>* Greeter::Stub::PrepareAsyncSayHelloRaw(::grpc::ClientContext* context, const ::helloworld::HelloRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderHelper::Create< ::helloworld::HelloReply, ::helloworld::HelloRequest, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), cq, rpcmethod_SayHello_, context, request);
}

::grpc::ClientAsyncResponseReader< ::helloworld::HelloReply>* Greeter::Stub::AsyncSayHelloRaw(::grpc::ClientContext* context, const ::helloworld::HelloRequest& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncSayHelloRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::ClientReader< ::helloworld::HelloReply>* Greeter::Stub::SayHelloStreamReplyRaw(::grpc::ClientContext* context, const ::helloworld::HelloRequest& request) {
  return ::grpc::internal::ClientReaderFactory< ::helloworld::HelloReply>::Create(channel_.get(), rpcmethod_SayHelloStreamReply_, context, request);
}

void Greeter::Stub::async::SayHelloStreamReply(::grpc::ClientContext* context, const ::helloworld::HelloRequest* request, ::grpc::ClientReadReactor< ::helloworld::HelloReply>* reactor) {
  ::grpc::internal::ClientCallbackReaderFactory< ::helloworld::HelloReply>::Create(stub_->channel_.get(), stub_->rpcmethod_SayHelloStreamReply_, context, request, reactor);
}

::grpc::ClientAsyncReader< ::helloworld::HelloReply>* Greeter::Stub::AsyncSayHelloStreamReplyRaw(::grpc::ClientContext* context, const ::helloworld::HelloRequest& request, ::grpc::CompletionQueue* cq, void* tag) {
  return ::grpc::internal::ClientAsyncReaderFactory< ::helloworld::HelloReply>::Create(channel_.get(), cq, rpcmethod_SayHelloStreamReply_, context, request, true, tag);
}

::grpc::ClientAsyncReader< ::helloworld::HelloReply>* Greeter::Stub::PrepareAsyncSayHelloStreamReplyRaw(::grpc::ClientContext* context, const ::helloworld::HelloRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncReaderFactory< ::helloworld::HelloReply>::Create(channel_.get(), cq, rpcmethod_SayHelloStreamReply_, context, request, false, nullptr);
}

::grpc::ClientReaderWriter< ::helloworld::HelloRequest, ::helloworld::HelloReply>* Greeter::Stub::SayHelloBDSRaw(::grpc::ClientContext* context) {
  return ::grpc::internal::ClientReaderWriterFactory< ::helloworld::HelloRequest, ::helloworld::HelloReply>::Create(channel_.get(), rpcmethod_SayHelloBDS_, context);
}

void Greeter::Stub::async::SayHelloBDS(::grpc::ClientContext* context, ::grpc::ClientBidiReactor< ::helloworld::HelloRequest,::helloworld::HelloReply>* reactor) {
  ::grpc::internal::ClientCallbackReaderWriterFactory< ::helloworld::HelloRequest,::helloworld::HelloReply>::Create(stub_->channel_.get(), stub_->rpcmethod_SayHelloBDS_, context, reactor);
}

::grpc::ClientAsyncReaderWriter< ::helloworld::HelloRequest, ::helloworld::HelloReply>* Greeter::Stub::AsyncSayHelloBDSRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq, void* tag) {
  return ::grpc::internal::ClientAsyncReaderWriterFactory< ::helloworld::HelloRequest, ::helloworld::HelloReply>::Create(channel_.get(), cq, rpcmethod_SayHelloBDS_, context, true, tag);
}

::grpc::ClientAsyncReaderWriter< ::helloworld::HelloRequest, ::helloworld::HelloReply>* Greeter::Stub::PrepareAsyncSayHelloBDSRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncReaderWriterFactory< ::helloworld::HelloRequest, ::helloworld::HelloReply>::Create(channel_.get(), cq, rpcmethod_SayHelloBDS_, context, false, nullptr);
}

::grpc::ClientWriter< ::helloworld::HelloRequest>* Greeter::Stub::SayHelloRecordRaw(::grpc::ClientContext* context, ::helloworld::HelloReply* response) {
  return ::grpc::internal::ClientWriterFactory< ::helloworld::HelloRequest>::Create(channel_.get(), rpcmethod_SayHelloRecord_, context, response);
}

void Greeter::Stub::async::SayHelloRecord(::grpc::ClientContext* context, ::helloworld::HelloReply* response, ::grpc::ClientWriteReactor< ::helloworld::HelloRequest>* reactor) {
  ::grpc::internal::ClientCallbackWriterFactory< ::helloworld::HelloRequest>::Create(stub_->channel_.get(), stub_->rpcmethod_SayHelloRecord_, context, response, reactor);
}

::grpc::ClientAsyncWriter< ::helloworld::HelloRequest>* Greeter::Stub::AsyncSayHelloRecordRaw(::grpc::ClientContext* context, ::helloworld::HelloReply* response, ::grpc::CompletionQueue* cq, void* tag) {
  return ::grpc::internal::ClientAsyncWriterFactory< ::helloworld::HelloRequest>::Create(channel_.get(), cq, rpcmethod_SayHelloRecord_, context, response, true, tag);
}

::grpc::ClientAsyncWriter< ::helloworld::HelloRequest>* Greeter::Stub::PrepareAsyncSayHelloRecordRaw(::grpc::ClientContext* context, ::helloworld::HelloReply* response, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncWriterFactory< ::helloworld::HelloRequest>::Create(channel_.get(), cq, rpcmethod_SayHelloRecord_, context, response, false, nullptr);
}

Greeter::Service::Service() {
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      Greeter_method_names[0],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< Greeter::Service, ::helloworld::HelloRequest, ::helloworld::HelloReply, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(
          [](Greeter::Service* service,
             ::grpc::ServerContext* ctx,
             const ::helloworld::HelloRequest* req,
             ::helloworld::HelloReply* resp) {
               return service->SayHello(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      Greeter_method_names[1],
      ::grpc::internal::RpcMethod::SERVER_STREAMING,
      new ::grpc::internal::ServerStreamingHandler< Greeter::Service, ::helloworld::HelloRequest, ::helloworld::HelloReply>(
          [](Greeter::Service* service,
             ::grpc::ServerContext* ctx,
             const ::helloworld::HelloRequest* req,
             ::grpc::ServerWriter<::helloworld::HelloReply>* writer) {
               return service->SayHelloStreamReply(ctx, req, writer);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      Greeter_method_names[2],
      ::grpc::internal::RpcMethod::BIDI_STREAMING,
      new ::grpc::internal::BidiStreamingHandler< Greeter::Service, ::helloworld::HelloRequest, ::helloworld::HelloReply>(
          [](Greeter::Service* service,
             ::grpc::ServerContext* ctx,
             ::grpc::ServerReaderWriter<::helloworld::HelloReply,
             ::helloworld::HelloRequest>* stream) {
               return service->SayHelloBDS(ctx, stream);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      Greeter_method_names[3],
      ::grpc::internal::RpcMethod::CLIENT_STREAMING,
      new ::grpc::internal::ClientStreamingHandler< Greeter::Service, ::helloworld::HelloRequest, ::helloworld::HelloReply>(
          [](Greeter::Service* service,
             ::grpc::ServerContext* ctx,
             ::grpc::ServerReader<::helloworld::HelloRequest>* reader,
             ::helloworld::HelloReply* resp) {
               return service->SayHelloRecord(ctx, reader, resp);
             }, this)));
}

Greeter::Service::~Service() {
}

::grpc::Status Greeter::Service::SayHello(::grpc::ServerContext* context, const ::helloworld::HelloRequest* request, ::helloworld::HelloReply* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status Greeter::Service::SayHelloStreamReply(::grpc::ServerContext* context, const ::helloworld::HelloRequest* request, ::grpc::ServerWriter< ::helloworld::HelloReply>* writer) {
  (void) context;
  (void) request;
  (void) writer;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status Greeter::Service::SayHelloBDS(::grpc::ServerContext* context, ::grpc::ServerReaderWriter< ::helloworld::HelloReply, ::helloworld::HelloRequest>* stream) {
  (void) context;
  (void) stream;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status Greeter::Service::SayHelloRecord(::grpc::ServerContext* context, ::grpc::ServerReader< ::helloworld::HelloRequest>* reader, ::helloworld::HelloReply* response) {
  (void) context;
  (void) reader;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}


}  // namespace helloworld

