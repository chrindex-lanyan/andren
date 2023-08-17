// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: greet.proto

#include "greet.pb.h"
#include "greet.grpc.pb.h"

#include <functional>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/channel_interface.h>
#include <grpcpp/impl/codegen/client_unary_call.h>
#include <grpcpp/impl/codegen/client_callback.h>
#include <grpcpp/impl/codegen/message_allocator.h>
#include <grpcpp/impl/codegen/method_handler.h>
#include <grpcpp/impl/codegen/rpc_service_method.h>
#include <grpcpp/impl/codegen/server_callback.h>
#include <grpcpp/impl/codegen/server_callback_handlers.h>
#include <grpcpp/impl/codegen/server_context.h>
#include <grpcpp/impl/codegen/service_type.h>
#include <grpcpp/impl/codegen/sync_stream.h>
namespace greet {

static const char* GreetService_method_names[] = {
  "/greet.GreetService/SayHello",
};

std::unique_ptr< GreetService::Stub> GreetService::NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options) {
  (void)options;
  std::unique_ptr< GreetService::Stub> stub(new GreetService::Stub(channel));
  return stub;
}

GreetService::Stub::Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel)
  : channel_(channel), rpcmethod_SayHello_(GreetService_method_names[0], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  {}

::grpc::Status GreetService::Stub::SayHello(::grpc::ClientContext* context, const ::greet::HelloRequest& request, ::greet::HelloResponse* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_SayHello_, context, request, response);
}

void GreetService::Stub::experimental_async::SayHello(::grpc::ClientContext* context, const ::greet::HelloRequest* request, ::greet::HelloResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc_impl::internal::CallbackUnaryCall(stub_->channel_.get(), stub_->rpcmethod_SayHello_, context, request, response, std::move(f));
}

void GreetService::Stub::experimental_async::SayHello(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::greet::HelloResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc_impl::internal::CallbackUnaryCall(stub_->channel_.get(), stub_->rpcmethod_SayHello_, context, request, response, std::move(f));
}

void GreetService::Stub::experimental_async::SayHello(::grpc::ClientContext* context, const ::greet::HelloRequest* request, ::greet::HelloResponse* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc_impl::internal::ClientCallbackUnaryFactory::Create(stub_->channel_.get(), stub_->rpcmethod_SayHello_, context, request, response, reactor);
}

void GreetService::Stub::experimental_async::SayHello(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::greet::HelloResponse* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc_impl::internal::ClientCallbackUnaryFactory::Create(stub_->channel_.get(), stub_->rpcmethod_SayHello_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::greet::HelloResponse>* GreetService::Stub::AsyncSayHelloRaw(::grpc::ClientContext* context, const ::greet::HelloRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc_impl::internal::ClientAsyncResponseReaderFactory< ::greet::HelloResponse>::Create(channel_.get(), cq, rpcmethod_SayHello_, context, request, true);
}

::grpc::ClientAsyncResponseReader< ::greet::HelloResponse>* GreetService::Stub::PrepareAsyncSayHelloRaw(::grpc::ClientContext* context, const ::greet::HelloRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc_impl::internal::ClientAsyncResponseReaderFactory< ::greet::HelloResponse>::Create(channel_.get(), cq, rpcmethod_SayHello_, context, request, false);
}

GreetService::Service::Service() {
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      GreetService_method_names[0],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< GreetService::Service, ::greet::HelloRequest, ::greet::HelloResponse>(
          [](GreetService::Service* service,
             ::grpc_impl::ServerContext* ctx,
             const ::greet::HelloRequest* req,
             ::greet::HelloResponse* resp) {
               return service->SayHello(ctx, req, resp);
             }, this)));
}

GreetService::Service::~Service() {
}

::grpc::Status GreetService::Service::SayHello(::grpc::ServerContext* context, const ::greet::HelloRequest* request, ::greet::HelloResponse* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}


}  // namespace greet
