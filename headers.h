#include <pthread.h>
#include <unistd.h>

#include "Graph.h"

#define I1_ADDRESS "104.197.8.216"
#define I2_ADDRESS "104.197.8.216"
#define I3_ADDRESS "104.198.211.89"
#define RPC_PORT "50051"

#define RPC_FAILED 500

#define ADD_NODE 0
#define REMOVE_NODE 1
#define ADD_EDGE 2
#define REMOVE_EDGE 3

extern int head;
extern int tail;
extern char *ip_next;

extern int part;
extern char *rpc_port;
extern char **ip_list;
extern char *ip1;
extern char *ip2;
extern char *ip3;

#ifdef __cplusplus
	#define EXTERNC extern "C"
#else
	#define EXTERNC
#endif

EXTERNC void *RunServer(void *);
EXTERNC int propogate(const int, const uint64_t, const uint64_t);

#undef EXTERNC