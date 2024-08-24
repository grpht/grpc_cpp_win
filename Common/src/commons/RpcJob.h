#pragma once
#pragma warning(disable : 4251)
#pragma warning(disable : 4819)

#include <mutex>
#include <queue>
#include <functional>
#include <memory>
#include <any>

#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#include <grpcpp/generic/async_generic_service.h>
#include <grpcpp/support/async_stream.h>
#include <grpcpp/support/async_unary_call.h>
#include <grpcpp/support/client_callback.h>
#include <grpcpp/client_context.h>
#include <grpcpp/completion_queue.h>
#include <grpcpp/support/message_allocator.h>
#include <grpcpp/support/method_handler.h>
#include <grpcpp/impl/codegen/proto_utils.h>
#include <grpcpp/impl/rpc_method.h>
#include <grpcpp/support/server_callback.h>
#include <grpcpp/impl/codegen/server_callback_handlers.h>
#include <grpcpp/server_context.h>
#include <grpcpp/impl/service_type.h>
#include <grpcpp/impl/codegen/status.h>
#include <grpcpp/support/stub_options.h>
#include <grpcpp/support/sync_stream.h>

struct RpcJobBase {
    std::unique_ptr<google::protobuf::Message> data;
    grpc::ClientContext context;
    grpc::Status status;
    std::any stream;
    std::function<void(google::protobuf::Message*, std::any stream)> execute;
};

template<typename T>
struct RpcJob final : public RpcJobBase
{
    std::unique_ptr<grpc::ClientAsyncResponseReader<T>> response_reader;
};

template<typename T>
class RpcJobQueue
{
public:
    void Push(RpcJobBase* response)
    {
        {
            std::lock_guard<std::mutex> lock(_m);
            _q.push(response);
        }
    }

    void Flush()
    {
        while (!_q.empty())
        {
            RpcJobBase* response = nullptr;
            {
                std::lock_guard<std::mutex> lock(_m);
                response = _q.front();
                _q.pop();
            }
            if (response == nullptr)
                return;
            response->execute(response->data.get(), response->stream);
            delete response;

            std::cout << "jobQ Size : " << _q.size() << std::endl;
        }
    }

    size_t Size() { return _q.size(); }
private:
    std::queue<RpcJobBase*> _q;
    std::mutex _m;
    std::atomic_bool _flushing;
};
