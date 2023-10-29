#!/bin/sh

cd example

# protoc -I=./ --cpp_out=./ ./greet.proto

protoc -I ./  --cpp_out=./ --grpc_out=./ --plugin=protoc-gen-grpc="which grpc_cpp_plugin" ./greet.proto


cd ..
