#include "mongoose.h"
#include "headers.h"

int head;
int tail;
char *ip_next;

int part;
char *rpc_port;
char **ip_list;
char *ip1;
char *ip2;
char *ip3;

pthread_mutex_t mutex;

typedef struct {
  Graph *graph;
} Data;


static void add_node(struct mg_connection *nc, struct http_message *hm, void *user_data) {
  Data *data = (Data *) user_data;
  Graph *graph = data->graph;

  const char *json = hm->body.p;
  int json_len = (int) hm->body.len;

  struct json_token *arr, *tok;
  int status;

  arr = parse_json2(json, json_len);
  if (arr == NULL) {
    mg_printf(nc, "Error in JSON\n");
    free(arr);
    return;
  }

  tok = find_json_token(arr, "node_id");
  if (tok == NULL) {
    mg_printf(nc, "Could not find node_id in JSON\n");
    free(arr);
    return;
  }

  status = graph->addNode(strtoull(tok->ptr, NULL, 10)); 

  //DEBUG
  fprintf(stderr, "add_node: %.*s = %d\n", tok->len, tok->ptr, status); 

  if (status == SUCCESS) {
    mg_printf(nc, "HTTP/1.1 %d OK\r\n"
                  "Content-Length: %d\r\n"
                  "Content-Type: application/json\r\n"
                  "\r\n%.*s", status, json_len, json_len, json); 
  } else if (status == EXISTS) {
    mg_printf(nc, "HTTP/1.1 204 OK\r\n");
    
  } else {
    mg_printf(nc, "HTTP/1.1 400 Bad Request\r\n");
  }

  free(arr);
}

static void add_edge(struct mg_connection *nc, struct http_message *hm, void *user_data) {

  Data *data = (Data *) user_data;
  Graph *graph = data->graph;

  const char *json = hm->body.p;
  int json_len = (int) hm->body.len;

  struct json_token *arr, *tok, *tok1;
  int status = 0;

  arr = parse_json2(json, json_len);
  if (arr == NULL) {
    mg_printf(nc, "Error in JSON\n");
    free(arr);
    return;
  }

  tok = find_json_token(arr, "node_a_id");
  if (tok == NULL) {
    mg_printf(nc, "Could not find node_a_id in JSON\n");
    free(arr);
    return;
  }

  tok1 = find_json_token(arr, "node_b_id");
  if (tok1 == NULL) {
    mg_printf(nc, "Could not find node_b_id in JSON\n");
    free(arr);
    return;
  }

  // Get node IDs
  uint64_t node_a_id = strtoull(tok->ptr, NULL, 10);
  uint64_t node_b_id = strtoull(tok1->ptr, NULL, 10);

  // Neither node in this partition
  if (node_a_id % 3 != part-1 && node_b_id % 3 != part-1) {
    fprintf(stderr, "BAD REQUEST: Neither node is in this partition \n");
    status = ERROR;
  }

  // Both nodes in this partition
  else if (node_a_id % 3 == part-1 && node_b_id % 3 == part-1) {
    fprintf(stderr, "Both nodes in this partition \n");
    status = graph->addEdge(node_a_id, node_b_id); 
  }

  // Exactly one node in this partition
  else {
    uint64_t max_node_id = ((node_a_id % 3) < (node_b_id % 3))? node_b_id : node_a_id;
    uint64_t min_node_id = ((node_a_id % 3) > (node_b_id % 3))? node_b_id : node_a_id;

    // I am the higher partition
    if (max_node_id % 3 == part-1) {
      fprintf(stderr, "BAD REQUEST: Request sent to higher partition \n");
      status = ERROR; 
    }
  
    // I am the lower partition
    else {
      fprintf(stderr, "Add_edge: I am the lower partition, about to send RPC to higher partition \n");
      
      // Check if this partition has lower node
      std::pair<int, bool> result;
      result = graph->getNode(min_node_id);
      bool in_graph = std::get<1>(result);

      // If lower node doesn't exist, return
      if (!in_graph) {
        status = ERROR;
        fprintf(stderr, "Add_edge: lower node doesn't exist \n");
        mg_printf(nc, "HTTP/1.1 400 Bad Request\r\n");
        fprintf(stderr, "add_edge: %.*s, %.*s = %d\n", tok->len, tok->ptr, tok1->len, tok1->ptr, status);
        free(arr);
        return;
      }

      status = propogate(ADD_EDGE, min_node_id, max_node_id);
      if (status == RPC_FAILED) {
        fprintf(stderr, "Add_edge: RPC failed \n");
        mg_printf(nc, "HTTP/1.1 500 RPC Failed\r\n");
        fprintf(stderr, "add_edge: %.*s, %.*s = %d\n", tok->len, tok->ptr, tok1->len, tok1->ptr, status);
        free(arr);
        return;
      }

      if (status == SUCCESS) {
        graph->addNode(max_node_id);
        status = graph->addEdge(min_node_id, max_node_id); 
      }
    }
  } 

  //DEBUG
  fprintf(stderr, "add_edge: %.*s, %.*s = %d\n", tok->len, tok->ptr, tok1->len, tok1->ptr, status); 

  if (status == SUCCESS) {
    mg_printf(nc, "HTTP/1.1 %d OK\r\n"
                  "Content-Length: %d\r\n"
                  "Content-Type: application/json\r\n"
                  "\r\n%.*s", status, json_len, json_len, json);  
  } else if (status == EXISTS) {
    mg_printf(nc, "HTTP/1.1 204 OK\r\n");
  } else if (status == ERROR) {
    mg_printf(nc, "HTTP/1.1 400 Bad Request\r\n");
  } else {
    mg_printf(nc, "HTTP/1.1 404 Not Found\r\n");
  }

  free(arr);
}

static void remove_node(struct mg_connection *nc, struct http_message *hm, void *user_data) {
  Data *data = (Data *) user_data;
  Graph *graph = data->graph;

  const char *json = hm->body.p;
  int json_len = (int) hm->body.len;

  struct json_token *arr, *tok;
  int status = 0;

  arr = parse_json2(json, json_len);
  if (arr == NULL) {
    mg_printf(nc, "Error in JSON\n");
    free(arr);
    return;
  }

  tok = find_json_token(arr, "node_id");
  if (tok == NULL) {
    mg_printf(nc, "Could not find node_id in JSON\n");
    free(arr);
    return;
  }

  status = graph->removeNode(strtoull(tok->ptr, NULL, 10)); 

  //DEBUG
  fprintf(stderr, "remove_node: %.*s = %d\n", tok->len, tok->ptr, status);

  if (status == SUCCESS) {
    mg_printf(nc, "HTTP/1.1 %d OK\r\n"
                  "Content-Length: %d\r\n"
                  "Content-Type: application/json\r\n"
                  "\r\n%.*s", status, json_len, json_len, json); 
    
    
  } else if (status == ERROR) {
    mg_printf(nc, "HTTP/1.1 400 Bad Request\r\n");
    
  } else {
    mg_printf(nc, "HTTP/1.1 404 Not Found\r\n");
  }

  free(arr);
}

static void remove_edge(struct mg_connection *nc, struct http_message *hm, void *user_data) {
  Data *data = (Data *) user_data;
  Graph *graph = data->graph;

  const char *json = hm->body.p;
  int json_len = (int) hm->body.len;

  struct json_token *arr, *tok, *tok1;
  int status = 0;

  arr = parse_json2(json, json_len);
  if (arr == NULL) {
    mg_printf(nc, "Error in JSON\n");
    free(arr);
    return;
  }

  tok = find_json_token(arr, "node_a_id");
  if (tok == NULL) {
    mg_printf(nc, "Could not find node_a_id in JSON\n");
    free(arr);
    return;
  }

  tok1 = find_json_token(arr, "node_b_id");
  if (tok1 == NULL) {
    mg_printf(nc, "Could not find node_b_id in JSON\n");
    free(arr);
    return;
  }

  // Get node IDs
  uint64_t node_a_id = strtoull(tok->ptr, NULL, 10);
  uint64_t node_b_id = strtoull(tok1->ptr, NULL, 10);

  // Neither node in this partition
  if (node_a_id % 3 != part-1 && node_b_id % 3 != part-1) {
    fprintf(stderr, "BAD REQUEST: Neither node is in this partition \n");
    status = ERROR;
  }

  // Both nodes in this partition
  else if (node_a_id % 3 == part-1 && node_b_id % 3 == part-1) {
    fprintf(stderr, "Both nodes in this partition \n");
    status = graph->removeEdge(node_a_id, node_b_id); 
  }

  // Exactly one node in this partition
  else {
    uint64_t max_node_id = ((node_a_id % 3) < (node_b_id % 3))? node_b_id : node_a_id;
    uint64_t min_node_id = ((node_a_id % 3) > (node_b_id % 3))? node_b_id : node_a_id;

    // I am the higher partition
    if (max_node_id % 3 == part-1) {
      fprintf(stderr, "BAD REQUEST: Request sent to higher partition \n");
      status = ERROR; 
    }
  
    // I am the lower partition
    else {
      fprintf(stderr, "Remove_edge: I am the lower partition, about to send RPC to higher partition \n");

      // Check if this partition has lower node
      std::pair<int, bool> result;
      result = graph->getNode(min_node_id);
      bool in_graph = std::get<1>(result);

      // If lower node doesn't exist, return
      if (!in_graph) {
        status = ERROR;
        fprintf(stderr, "Remove_edge: lower node doesn't exist \n");
        mg_printf(nc, "HTTP/1.1 400 Bad Request\r\n");
        fprintf(stderr, "remove_edge: %.*s, %.*s = %d\n", tok->len, tok->ptr, tok1->len, tok1->ptr, status);
        free(arr);
        return;
      }

      status = propogate(REMOVE_EDGE, min_node_id, max_node_id);
      if (status == RPC_FAILED) {
        mg_printf(nc, "HTTP/1.1 500 RPC Failed\r\n");
        fprintf(stderr, "remove_edge: %.*s, %.*s = %d\n", tok->len, tok->ptr, tok1->len, tok1->ptr, status);
        free(arr);
        return;
      }
      
      status = graph->removeEdge(min_node_id, max_node_id); 
    }

  } 

  //DEBUG
  fprintf(stderr, "remove_edge: %.*s, %.*s = %d\n", tok->len, tok->ptr, tok1->len, tok1->ptr, status);

  if (status == SUCCESS) {
    mg_printf(nc, "HTTP/1.1 %d OK\r\n"
                  "Content-Length: %d\r\n"
                  "Content-Type: application/json\r\n"
                  "\r\n%.*s", status, json_len, json_len, json);  
    
    
  } else if (status == ERROR) {
    mg_printf(nc, "HTTP/1.1 400 Bad Request\r\n");
    
  } else {
    mg_printf(nc, "HTTP/1.1 404 Not Found\r\n");
  }

  free(arr);
}

static void get_node(struct mg_connection *nc, struct http_message *hm, void *user_data) {
  Data *data = (Data *) user_data;
  Graph *graph = data->graph;

  const char *json = hm->body.p;
  int json_len = (int) hm->body.len;

  struct json_token *arr, *tok;

  std::pair<int, bool> result;
  int status;
  bool in_graph;

  char buf[1000];
  int json_buf_size = sizeof(buf);

  arr = parse_json2(json, json_len);
  if (arr == NULL) {
    mg_printf(nc, "Error in JSON\n");
    free(arr);
    return;
  }

  tok = find_json_token(arr, "node_id");
  if (tok == NULL) {
    mg_printf(nc, "Could not find node_id in JSON\n");
    free(arr);
    return;
  }

  result = graph->getNode(strtoull(tok->ptr, NULL, 10));
  status = std::get<0>(result); 
  in_graph = std::get<1>(result);

  //DEBUG
  fprintf(stderr, "get_node: %.*s = %d\n", tok->len, tok->ptr, status);

  if (in_graph == true) {
    json_buf_size = json_emit(buf, sizeof(buf), "{s : T}", "in_graph");
    assert(json_buf_size >= 0 && (size_t) json_buf_size <= sizeof(buf));
  } else if (in_graph == false) {
    json_buf_size = json_emit(buf, sizeof(buf), "{s : F}", "in_graph");
    assert(json_buf_size >= 0 && (size_t) json_buf_size <= sizeof(buf));
  } else {
    mg_printf(nc, "Error creating JSON in_graph field \n");
    free(arr);
    return;
  }

  if (status == SUCCESS && buf != NULL) {
    mg_printf(nc, "HTTP/1.1 %d OK\r\n"
                  "Content-Length: %d\r\n"
                  "Content-Type: application/json\r\n"
                  "\r\n%.*s", status, json_buf_size, json_buf_size, buf); 
    
  } else {
    mg_printf(nc, "HTTP/1.1 404 Not Found\r\n");
  }

  free(arr);
}

static void get_edge(struct mg_connection *nc, struct http_message *hm, void *user_data) {
  Data *data = (Data *) user_data;
  Graph *graph = data->graph;

  const char *json = hm->body.p;
  int json_len = (int) hm->body.len;

  struct json_token *arr, *tok, *tok1;

  std::pair<int, bool> result;
  int status;
  bool in_graph;

  char buf[1000];
  int json_buf_size = sizeof(buf);

  arr = parse_json2(json, json_len);
  if (arr == NULL) {
    mg_printf(nc, "Error in JSON\n");
    free(arr);
    return;
  }

  tok = find_json_token(arr, "node_a_id");
  if (tok == NULL) {
    mg_printf(nc, "Could not find node_a_id in JSON\n");
    free(arr);
    return;
  }

  tok1 = find_json_token(arr, "node_b_id");
  if (tok1 == NULL) {
    mg_printf(nc, "Could not find node_b_id in JSON\n");
    free(arr);
    return;
  }

  result = graph->getEdge(strtoull(tok->ptr, NULL, 10), strtoull(tok1->ptr, NULL, 10));
  status = std::get<0>(result); 
  in_graph = std::get<1>(result); 

  //DEBUG
  fprintf(stderr, "get_edge: %.*s, %.*s = %d\n", tok->len, tok->ptr, tok1->len, tok1->ptr, status);

  if (in_graph == true) {
    json_buf_size = json_emit(buf, sizeof(buf), "{s : T}", "in_graph");
    assert(json_buf_size >= 0 && (size_t) json_buf_size <= sizeof(buf));
  } else if (in_graph == false) {
    json_buf_size = json_emit(buf, sizeof(buf), "{s : F}", "in_graph");
    assert(json_buf_size >= 0 && (size_t) json_buf_size <= sizeof(buf));
  } else {
    mg_printf(nc, "Error creating JSON in_graph field \n");
    free(arr);
    return;
  }

  if (status == SUCCESS && buf != NULL) {
    mg_printf(nc, "HTTP/1.1 %d OK\r\n"
                  "Content-Length: %d\r\n"
                  "Content-Type: application/json\r\n"
                  "\r\n%.*s", status, json_buf_size, json_buf_size, buf); 
    
  } else if (status == ERROR) {
    mg_printf(nc, "HTTP/1.1 400 Bad Request\r\n");
  } else {
    mg_printf(nc, "HTTP/1.1 404 Not Found\r\n");
  }

  free(arr);
}

static void get_neighbors(struct mg_connection *nc, struct http_message *hm, void *user_data) {
  Data *data = (Data *) user_data;
  Graph *graph = data->graph;

  const char *json = hm->body.p;
  int json_len = (int) hm->body.len;

  struct json_token *arr, *tok;

  std::pair<int, std::string> result;
  int status;
  std::string neighbor_list;

  char buf[1000];
  int json_buf_size = sizeof(buf);

  arr = parse_json2(json, json_len);
  if (arr == NULL) {
    mg_printf(nc, "Error in JSON\n");
    free(arr);
    return;
  }

  tok = find_json_token(arr, "node_id");
  if (tok == NULL) {
    mg_printf(nc, "Could not find node_id in JSON\n");
    free(arr);
    return;
  }

  result = graph->getNeighbors(strtoull(tok->ptr, NULL, 10));

  status = std::get<0>(result); 
  neighbor_list = std::get<1>(result);

  //DEBUG
  fprintf(stderr, "get_neighbors: %.*s = %d\n", tok->len, tok->ptr, status);

  json_buf_size = json_emit(buf, sizeof(buf), "{s : V, s : [S]}", 
                            "node_id", tok->ptr, tok->len, "neighbors", neighbor_list.c_str());
  assert(json_buf_size >= 0 && (size_t) json_buf_size <= sizeof(buf));

  if (status == SUCCESS && buf != NULL) {
    mg_printf(nc, "HTTP/1.1 %d OK\r\n"
                  "Content-Length: %d\r\n"
                  "Content-Type: application/json\r\n"
                  "\r\n%.*s", status, json_buf_size, json_buf_size, buf); 
    
  } else if (status == ERROR) {
    mg_printf(nc, "HTTP/1.1 400 Bad Request\r\n");
  } else {
    mg_printf(nc, "HTTP/1.1 404 Not Found\r\n");
  }

  free(arr);
}

static void shortest_path(struct mg_connection *nc, struct http_message *hm, void *user_data) {
  Data *data = (Data *) user_data;
  Graph *graph = data->graph;

  const char *json = hm->body.p;
  int json_len = (int) hm->body.len;

  struct json_token *arr, *tok, *tok1;
  std::pair<int, uint64_t> result;
  int status;
  uint64_t distance;

  char buf[1000];
  int json_buf_size = sizeof(buf);

  char distance_buf[22];

  arr = parse_json2(json, json_len);
  if (arr == NULL) {
    mg_printf(nc, "Error in JSON\n");
    free(arr);
    return;
  }

  tok = find_json_token(arr, "node_a_id");
  if (tok == NULL) {
    mg_printf(nc, "Could not find node_id in JSON\n");
    free(arr);
    return;
  }

  tok1 = find_json_token(arr, "node_b_id");
  if (tok1 == NULL) {
    mg_printf(nc, "Could not find node_b_id in JSON\n");
    free(arr);
    return;
  }

  result = graph->shortestPath(strtoull(tok->ptr, NULL, 10), strtoull(tok1->ptr, NULL, 10));
  status = std::get<0>(result);
  distance = std::get<1>(result);

  //DEBUG
  fprintf(stderr, "shortest_path: %.*s, %.*s = %d\n", tok->len, tok->ptr, tok1->len, tok1->ptr, status);

  snprintf(distance_buf, sizeof(distance_buf), "%lu", distance);

  json_buf_size = json_emit(buf, sizeof(buf), "{s : S}", "distance", distance_buf);
  assert(json_buf_size >= 0 && (size_t) json_buf_size <= sizeof(buf));

  if (status == SUCCESS && buf != NULL) {
    mg_printf(nc, "HTTP/1.1 %d OK\r\n"
                  "Content-Length: %d\r\n"
                  "Content-Type: application/json\r\n"
                  "\r\n%.*s", status, json_buf_size, json_buf_size, buf); 
    
  } else if (status == EXISTS) {
    mg_printf(nc, "HTTP/1.1 204 OK\r\n");
  } else if (status == ERROR) {
    mg_printf(nc, "HTTP/1.1 400 Bad Request\r\n");
  } else {
    mg_printf(nc, "HTTP/1.1 404 Not Found\r\n");
  }

  free(arr);
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {

  if (ev == MG_EV_HTTP_REQUEST) {

    struct http_message *hm = (struct http_message *) ev_data;
    struct mg_str *uri = &(hm->uri);

    pthread_mutex_lock(&mutex);

    if (mg_vcmp(uri, "/api/v1/add_node") == 0) {
      add_node(nc, hm, nc->mgr->user_data);
    } else if (mg_vcmp(uri, "/api/v1/add_edge") == 0) {
      add_edge(nc, hm, nc->mgr->user_data);
    } else if (mg_vcmp(uri, "/api/v1/remove_node") == 0) {
      remove_node(nc, hm, nc->mgr->user_data);
    } else if (mg_vcmp(uri, "/api/v1/remove_edge") == 0) {
      remove_edge(nc, hm, nc->mgr->user_data);
    } else if (mg_vcmp(uri, "/api/v1/get_node") == 0) {
      get_node(nc, hm, nc->mgr->user_data);
    } else if (mg_vcmp(uri, "/api/v1/get_edge") == 0) {
      get_edge(nc, hm, nc->mgr->user_data);
    } else if (mg_vcmp(uri, "/api/v1/get_neighbors") == 0) {
      get_neighbors(nc, hm, nc->mgr->user_data);
    } else if (mg_vcmp(uri, "/api/v1/shortest_path") == 0) {
      shortest_path(nc, hm, nc->mgr->user_data);
    } else {
      mg_printf(nc, "HTTP/1.1 404 Not Found\r\n");
    }

    pthread_mutex_unlock(&mutex); 

    nc->flags |= MG_F_SEND_AND_CLOSE;
  }
}

int main(int argc, char *argv[]) {

  if (argc != 8) {
    fprintf(stderr, 
      "Usage: ./cs426_graph_server <graph_server_port> -p <partnum> -l <partlist> \n");
    return 1;
  }

  // Parse arguments
  char *port;
  int c;

  while ((c = getopt(argc, argv, "p:l:")) != -1)
    switch (c)
      {
      case 'p':
        part = atoi(optarg);
        break;
      case 'l':
        ip1 = optarg;
        break;
      case '?':
        if (optopt == 'p')
          fprintf(stderr, "Option -%c requires an argument. \n", optopt);
        else if (isprint (optopt))
          fprintf(stderr, "Unknown option '-%c'.\n", optopt);
        else
          fprintf (stderr,
                   "Unknown option character `\\x%x'.\n",
                   optopt);
        return 1;
      default:
        fprintf (stderr,"Aborting\n");
        abort();
      }

  if (argc - optind != 3) {
    fprintf(stderr, "Incorrect number of arguments\n");
    return 1;
  }
  
  port = argv[optind++];
  ip2 = argv[optind++];
  ip3 = argv[optind];
  fprintf(stderr, "Port: %s, part: %d, address1: %s, address2: %s, address3: %s\n", 
    port, part, ip1, ip2, ip3);

  ip_list = (char **) malloc(3* sizeof(char *));
  ip_list[0] = ip1;
  ip_list[1] = ip2;
  ip_list[2] = ip3;
  rpc_port = strchr(ip_list[part-1], ':');

  // Create new graph
  Graph *graph = new Graph();

  // RPC Server
  pthread_t rpc_thread;

  if (pthread_create(&rpc_thread, NULL, RunServer, graph)) {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }

  // HTTP Server
  Data *data = (Data *) malloc(sizeof(Data));
  data->graph = graph;

  struct mg_mgr mgr;
  struct mg_connection *nc;

  mg_mgr_init(&mgr, (void *) data);

  nc = mg_bind(&mgr, port, ev_handler);

  if (nc == NULL) {
    printf("Failed to create listener\n");
    return 1;
  }

  mg_set_protocol_http_websocket(nc);

  for (;;) {
    mg_mgr_poll(&mgr, 1000);
  }
  mg_mgr_free(&mgr);

  // Free 
  free(data);
  free(ip_list);

  return 0;
}

