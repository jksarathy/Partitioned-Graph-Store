CC = g++
CPPFLAGS += -I/usr/local/include -pthread
CFLAGS = -std=g++11 -g -Wall
LDFLAGS += -L/usr/local/lib `pkg-config --libs grpc++ grpc`       \
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed \
           -lprotobuf -lpthread -ldl

PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`


PROTOS_PATH = .

vpath %.proto $(PROTOS_PATH)

all: cs426_graph_server

cs426_graph_server: cs426_graph_server.c mongoose.c Graph.cpp replicator_client.cc replicator_server.cc replicator.pb.cc replicator.grpc.pb.cc
	g++ $^ -L/usr/local/lib `pkg-config --libs grpc++ grpc` -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -lprotobuf -lpthread -ldl -std=c++0x -pthread -o cs426_graph_server

.PRECIOUS: %.grpc.pb.cc
%.grpc.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<
.PRECIOUS: %.pb.cc
%.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=. $<

clean:
	rm -f *.o *.pb.cc *.pb.h cs426_graph_server


