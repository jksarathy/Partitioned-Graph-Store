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

#include "headers.h"
#include <grpc++/grpc++.h>
#include "replicator.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using replicator::Node;
using replicator::Edge;
using replicator::Ack;
using replicator::ReplicatorService;

class ReplicatorClient {
 public:
  ReplicatorClient(std::shared_ptr<Channel> channel)
      : stub_(ReplicatorService::NewStub(channel)) {}

  int SendAddNode(const uint64_t node_id) {
    Node node;
    node.set_node_id(node_id);

    Ack ack;

    ClientContext context;

    Status status = stub_->AddNode(&context, node, &ack);

    if (status.ok()) {
      return ack.status();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return RPC_FAILED;
    }
  }

  int SendRemoveNode(const uint64_t node_id) {
    Node node;
    node.set_node_id(node_id);

    Ack ack;

    ClientContext context;

    Status status = stub_->RemoveNode(&context, node, &ack);

    if (status.ok()) {
      return ack.status();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return RPC_FAILED;
    }
  }

  int SendAddEdge(const uint64_t node_a_id, const uint64_t node_b_id) {
    Node *node_a(new Node);
    node_a->set_node_id(node_a_id);

    Node *node_b(new Node);
    node_b->set_node_id(node_b_id);

    Edge edge;
    edge.set_allocated_node_a(node_a);
    edge.set_allocated_node_b(node_b);

    Ack ack;
    ClientContext context;

    Status status = stub_->AddEdge(&context, edge, &ack);

    if (status.ok()) {
      return ack.status();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return RPC_FAILED;
    }
  }

  int SendRemoveEdge(const uint64_t node_a_id, const uint64_t node_b_id) {
    Node *node_a(new Node);
    node_a->set_node_id(node_a_id);

    Node *node_b(new Node);
    node_b->set_node_id(node_b_id);

    Edge edge;
    edge.set_allocated_node_a(node_a);
    edge.set_allocated_node_b(node_b);

    Ack ack;
    ClientContext context;

    Status status = stub_->RemoveEdge(&context, edge, &ack);

    if (status.ok()) {
      return ack.status();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return RPC_FAILED;
    }
  }

 private:
  std::unique_ptr<ReplicatorService::Stub> stub_;
};

int propogate(const int op, const uint64_t node_a_id, const uint64_t node_b_id) {
  int status = 0;

  uint64_t node_id = (node_a_id < node_b_id)? node_b_id : node_a_id;
  int modulo = node_id % 3;

  if (modulo == part-1) {
    std::cout << "Propogate received RPC request even though you are the higher partition" << std::endl;
    return 200;
  }

  if (ip_list[modulo] == NULL) {
    std::cout << "Error: ip address " << modulo+1 << " undefined" << std::endl;
    return RPC_FAILED;
  }

  std::string server_address(ip_list[modulo]);
  std::cout << "Propogate created server address " << server_address << std::endl;

  ReplicatorClient client(grpc::CreateChannel(
      server_address, grpc::InsecureChannelCredentials()));

  std::cout << "Propogate created client" << std::endl;

  switch (op) {
    case ADD_NODE: {
      std::cout << "Client calling: ADD_NODE" << std::endl;
      status = client.SendAddNode(node_a_id);
      std::cout << "Client received: ADD_NODE" << std::endl;
      break;
    }
    case REMOVE_NODE: {
      std::cout << "Client calling: REMOVE_NODE" << std::endl;
      status = client.SendRemoveNode(node_a_id);
      std::cout << "Client received: REMOVE_NODE" << std::endl;
      break;
    }
    case ADD_EDGE: {
      std::cout << "Client calling: ADD_EDGE" << std::endl;
      status = client.SendAddEdge(node_a_id, node_b_id);
      std::cout << "Client received: ADD_EDGE" << std::endl;
      break;
    }
    case REMOVE_EDGE: {
      std::cout << "Client calling: REMOVE_EDGE" << std::endl;
      status = client.SendRemoveEdge(node_a_id, node_b_id);
      std::cout << "Client received: REMOVE_EDGE" << std::endl;
      break;
    }
  }

  std::cout << "Client received status: " << status << std::endl;

  return status;
}
