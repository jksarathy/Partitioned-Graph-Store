// Copyright 2015, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>

#include "headers.h"
#include "replicator.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using replicator::Node;
using replicator::Edge;
using replicator::Ack;
using replicator::ReplicatorService;

class ReplicatorImpl final : public ReplicatorService::Service {
 public:
  explicit ReplicatorImpl(Graph *g) {
    graph = g;
  }

  Status AddNode(ServerContext* context, const Node* node, Ack *ack) override {
    pthread_mutex_lock(&mutex);

    std::cout << "RPC Server " << part << " adding node: " << node->node_id() << std::endl;

    int status;

    status = graph->addNode(node->node_id());
    ack->set_status(status);

    pthread_mutex_unlock(&mutex);
    return Status::OK;
  }

  Status RemoveNode(ServerContext* context, const Node* node, Ack *ack) override {
    pthread_mutex_lock(&mutex);

    std::cout << "RPC Server " << part << " removing node: " << node->node_id() << std::endl;

    int status;

    status = graph->removeNode(node->node_id());
    ack->set_status(status);

    pthread_mutex_unlock(&mutex);
    return Status::OK;
  }

  Status AddEdge(ServerContext* context, const Edge* edge, Ack *ack) override {
    pthread_mutex_lock(&mutex);

    std::cout << "RPC Server " << part << " adding edge: " << edge->node_a().node_id() << ", " << edge->node_b().node_id() << std::endl;

    int status;

    // Check if this partition has higher node
    std::pair<int, bool> result;
    result = graph->getNode(edge->node_b().node_id());
    bool in_graph = std::get<1>(result);

    // Only add lower node if higher node existed
    if (in_graph) {
      graph->addNode(edge->node_a().node_id());
    }

    status = graph->addEdge(edge->node_a().node_id(), edge->node_b().node_id());
    ack->set_status(status);

    pthread_mutex_unlock(&mutex);
    return Status::OK;
  }

  Status RemoveEdge(ServerContext* context, const Edge* edge, Ack *ack) override {
    pthread_mutex_lock(&mutex);

    std::cout << "RPC Server " << part << " removing edge: " << edge->node_a().node_id() << ", " << edge->node_b().node_id() << std::endl;

    int status;

    status = graph->removeEdge(edge->node_a().node_id(), edge->node_b().node_id());
    ack->set_status(status);

    pthread_mutex_unlock(&mutex);
    return Status::OK;
  }

 private:
  Graph *graph;
};

void *RunServer(void *v) {
  char str[80];
  strcpy(str, "0.0.0.0");
  strcat(str, rpc_port);

  std::string server_address(str);
  ReplicatorImpl service((Graph *) v);

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "RPC Server " << part << " listening on " << server_address << std::endl;
  server->Wait();
}